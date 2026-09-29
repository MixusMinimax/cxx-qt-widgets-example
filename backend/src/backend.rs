// Copyright (C) 2026  Maxi Barmetler <maxi@barmetler.com>
//
// Use of this source code is governed by either the MIT or Apache-2.0 license, at your choice.
// A copy of each can be found in the corresponding LICENSE-* file, or online (respectively):
// https://opensource.org/licenses/MIT.
// http://www.apache.org/licenses/LICENSE-2.0

use crate::controller::AsyncControllerHandle;
use crate::domain::measurement::{
    ExportOptions, ImportCollisionStrategy, ImportOptions, Measurement, MeasurementChangeset,
    MeasurementService, MeasurementServiceError, MeasurementUpdated,
};
use cxx_qt::{CxxQtType, Threading};
use cxx_qt_lib::QString;
use sql_uuid::Uuid;
use static_assertions::{assert_eq_align, assert_eq_size, const_assert_eq};
use std::cmp::Ordering;
use std::fs::{File, create_dir_all};
use std::io::Write;
use std::mem::offset_of;
use std::pin::Pin;
use std::sync::Arc;
use std::{mem, slice};
use tokio_util::sync::CancellationToken;
use url::Url;

#[cxx_qt::bridge]
mod ffi {
    unsafe extern "C++" {
        // workaround because cxx-qt's qstring.h file does not include it, and
        // qt itself references QChar in some places where the type is not
        // complete yet. Usually, you don't notice, as c++ headers can be
        // included from other places and QChar is usually complete.
        // specifically qarray has some static_assertions that require QChar to
        // be complete.
        include!(<QtCore/QChar>);
        include!("cxx-qt-lib/qstring.h");

        type QString = cxx_qt_lib::QString;
    }

    #[namespace = "measurements"]
    #[cxx_name = "measurement"]
    #[derive(Clone, PartialEq, Debug, Default)]
    pub struct Measurement {
        pub id: [u8; 16],
        pub systolic: f64,
        pub diastolic: f64,
        pub map: f64,
        pub pulse: f64,
        pub key: f64,
    }

    #[namespace = "measurements"]
    #[cxx_name = "measurement_type"]
    #[derive(Clone, PartialEq, Debug)]
    pub enum MeasurementType {
        #[cxx_name = "systolic"]
        Systolic,
        #[cxx_name = "diastolic"]
        Diastolic,
        #[cxx_name = "map"]
        Map,
        #[cxx_name = "pulse"]
        Pulse,
    }

    #[namespace = "backend"]
    unsafe extern "C++" {
        include!("backend/src/controller.cxx.h");
        type AsyncControllerHandle = crate::controller::AsyncControllerHandle;
    }

    #[namespace = "measurements"]
    extern "RustQt" {
        #[qobject]
        type MeasurementModel = super::MeasurementModelRust;

        fn initialize(
            self: Pin<&mut MeasurementModel>,
            tokio_handle: Box<AsyncControllerHandle>,
            connection_string: String,
        ) -> Result<()>;

        fn load_measurements(self: Pin<&mut MeasurementModel>) -> u32;

        fn create_measurement(self: Pin<&mut MeasurementModel>, measurement: Measurement) -> u32;

        fn update_measurement(self: Pin<&mut MeasurementModel>, measurement: Measurement) -> u32;

        fn delete_measurement(self: Pin<&mut MeasurementModel>, id: [u8; 16]) -> u32;

        fn export_measurements(self: Pin<&mut MeasurementModel>, url: String) -> u32;

        fn import_measurements(self: Pin<&mut MeasurementModel>, url: String) -> u32;

        fn measurements(self: &MeasurementModel) -> &[Measurement];

        #[qsignal]
        fn measurements_loaded(
            self: Pin<&mut MeasurementModel>,
            req_id: u32,
            measurements: &[Measurement],
        );

        #[qsignal]
        fn failure(self: Pin<&mut MeasurementModel>, req_id: u32, msg: QString);
    }

    impl cxx_qt::Threading for MeasurementModel {}
}

#[derive(Default)]
pub struct MeasurementModelRust {
    inner: Option<MeasurementModelRustInner>,
}

#[derive(Debug)]
struct MeasurementModelRustInner {
    tokio_handle: Box<AsyncControllerHandle>,
    measurement_service: Arc<MeasurementService>,
    req_id: u32, // no multithreading needed
    measurements: Vec<ffi::Measurement>,
}

#[derive(Debug, thiserror::Error)]
enum HandlerError {
    #[error("MeasurementServiceError: {0}")]
    MeasurementServiceError(#[from] MeasurementServiceError),
    #[error("io error: {0}")]
    Io(#[from] std::io::Error),
    #[error("invalid Url: {0}")]
    InvalidUrl(#[from] UrlError),
}

#[derive(Debug, thiserror::Error)]
enum UrlError {
    #[error("{0}")]
    Parse(#[from] url::ParseError),
    #[error("unsupported scheme: {0}")]
    UnsupportedScheme(String),
    #[error("invalid host: {0}")]
    InvalidHost(String),
    #[error("path already exists and is not a file")]
    AlreadyExists,
    #[error("file does not exist")]
    NotFound,
    #[error("io error: {0}")]
    Io(#[from] std::io::Error),
}

fn handle_request<F, O, RF>(mut m: Pin<&mut ffi::MeasurementModel>, handler: F) -> u32
where
    RF: FnOnce(Pin<&mut ffi::MeasurementModel>) + Send + 'static,
    O: Future<Output = Result<RF, HandlerError>> + Send + 'static,
    F: FnOnce(CancellationToken, u32, Arc<MeasurementService>) -> O + Send + 'static,
{
    let mut rm = m.as_mut().rust_mut();
    let inner = rm.inner.as_mut().unwrap();
    let req_id = inner.req_id;
    inner.req_id += 1;
    let qt_thread = m.qt_thread();
    let rust = m.rust().inner.as_ref().unwrap();
    let measurement_service: Arc<MeasurementService> = rust.measurement_service.clone();
    let _ = rust.tokio_handle.spawn_cb(async move |ct| {
        let res = handler(ct.clone(), req_id, measurement_service).await;
        if ct.is_cancelled() {
            return;
        }
        match res {
            Ok(cb) => {
                qt_thread
                    .queue(move |be| {
                        // cb might already be queued by the time we try to
                        // cancel it.
                        if ct.is_cancelled() {
                            return;
                        }
                        cb(be)
                    })
                    .inspect_err(|e| eprintln!("{e}"))
                    .ok();
            }
            Err(e) => {
                eprintln!("{}", e);
                qt_thread
                    .queue(move |backend| {
                        if ct.is_cancelled() {
                            return;
                        }
                        backend.failure(req_id, QString::from(e.to_string()))
                    })
                    .inspect_err(|e| eprintln!("{e}"))
                    .ok();
            }
        }
    });

    req_id
}

impl ffi::MeasurementModel {
    fn initialize(
        self: Pin<&mut Self>,
        tokio_handle: Box<AsyncControllerHandle>,
        connection_string: String,
    ) -> Result<(), HandlerError> {
        (|| -> Result<(), UrlError> {
            let url = Url::parse(&connection_string)?;
            if url.scheme() != "file" {
                return Err(UrlError::UnsupportedScheme(url.scheme().to_string()));
            }
            let path = url.to_file_path().map_err(|()| {
                UrlError::InvalidHost(url.host_str().unwrap_or("<missing>").to_string())
            })?;
            let valid = !path.exists() || path.is_file();
            if !valid {
                return Err(UrlError::AlreadyExists);
            }
            if let Some(parent) = path.parent()
                && !parent.exists()
            {
                create_dir_all(parent)?;
            }
            Ok(())
        })()?;

        let mut rm = self.rust_mut();
        if let Some(old) = rm.inner.take() {
            old.tokio_handle.cancel();
            rm.inner = Some(MeasurementModelRustInner {
                tokio_handle,
                measurement_service: Arc::new(MeasurementService::new(connection_string)),
                ..old
            });
        } else {
            rm.inner = Some(MeasurementModelRustInner {
                tokio_handle,
                measurement_service: Arc::new(MeasurementService::new(connection_string)),
                req_id: Default::default(),
                measurements: Default::default(),
            });
        }
        Ok(())
    }

    fn load_measurements(self: Pin<&mut Self>) -> u32 {
        handle_request(self, async |_, req_id, measurement_service| {
            let v = measurement_service.load_measurements().await?;
            let v = unsafe { mem::transmute::<Vec<Measurement>, Vec<ffi::Measurement>>(v) };
            Ok(move |mut backend: Pin<&mut ffi::MeasurementModel>| {
                backend
                    .as_mut()
                    .rust_mut()
                    .inner
                    .as_mut()
                    .unwrap()
                    .measurements = v;
                let measurements = unsafe {
                    let r = &backend.inner.as_ref().unwrap().measurements;
                    slice::from_raw_parts(r.as_ptr(), r.len())
                };
                backend.measurements_loaded(req_id, measurements);
            })
        })
    }

    fn create_measurement(self: Pin<&mut Self>, measurement: ffi::Measurement) -> u32 {
        handle_request(self, async |_, req_id, measurement_service| {
            let m = measurement_service
                .save_measurement_new(measurement.into())
                .await?;
            let m = ffi::Measurement::from(m);
            Ok(move |mut backend: Pin<&mut ffi::MeasurementModel>| {
                let mut rm = backend.as_mut().rust_mut();
                let measurements = &mut rm.inner.as_mut().unwrap().measurements;
                let idx = measurements.partition_point(|x| x.key <= m.key);
                // expensive. Might consider something else down the line.
                measurements.insert(idx, m);
                let measurements = unsafe {
                    let r = measurements;
                    slice::from_raw_parts(r.as_ptr(), r.len())
                };
                backend.measurements_loaded(req_id, measurements);
            })
        })
    }

    fn update_measurement(self: Pin<&mut Self>, measurement: ffi::Measurement) -> u32 {
        handle_request(self, async |_, req_id, measurement_service| {
            let MeasurementUpdated { new, old_ts } = measurement_service
                .update_measurement(measurement.into())
                .await?;
            let new: ffi::Measurement = new.into();
            Ok(move |mut backend: Pin<&mut ffi::MeasurementModel>| {
                let mut rm = backend.as_mut().rust_mut();
                let measurements = &mut rm.inner.as_mut().unwrap().measurements;
                let idx = measurements.partition_point(|x| x.key < new.key);
                // PartialOrd vs Ord is not relevant because nothing is NAN or
                // infinity.

                if new.key != old_ts
                    && let Ok(old_idx) = if old_ts < new.key {
                        measurements[..=idx].binary_search_by(|x| {
                            x.key.partial_cmp(&old_ts).unwrap_or(Ordering::Equal)
                        })
                    } else {
                        measurements[idx..]
                            .binary_search_by(|x| {
                                x.key.partial_cmp(&old_ts).unwrap_or(Ordering::Equal)
                            })
                            .map(|i| i + idx)
                    }
                    && idx != old_idx
                {
                    measurements[old_idx] = new;
                    if idx < old_idx {
                        measurements[idx..old_idx].rotate_right(1);
                    } else {
                        measurements[old_idx..idx].rotate_left(1);
                    }
                } else {
                    measurements[idx] = new;
                }
                let measurements = unsafe {
                    let r = measurements;
                    slice::from_raw_parts(r.as_ptr(), r.len())
                };
                backend.measurements_loaded(req_id, measurements);
            })
        })
    }

    fn delete_measurement(self: Pin<&mut Self>, id: [u8; 16]) -> u32 {
        handle_request(self, async move |_, req_id, measurement_service| {
            let deleted = measurement_service
                .delete_measurement(Uuid::from_bytes(id))
                .await?;
            let deleted: ffi::Measurement = deleted.into();
            Ok(move |mut backend: Pin<&mut ffi::MeasurementModel>| {
                let mut rm = backend.as_mut().rust_mut();
                let measurements = &mut rm.inner.as_mut().unwrap().measurements;
                if let Ok(idx) = measurements.binary_search_by(|x| {
                    x.key.partial_cmp(&deleted.key).unwrap_or(Ordering::Equal)
                }) {
                    // expensive. Might consider something else down the line.
                    measurements.remove(idx);
                    let measurements = unsafe {
                        let r = measurements;
                        slice::from_raw_parts(r.as_ptr(), r.len())
                    };
                    backend.measurements_loaded(req_id, measurements);
                } else {
                    eprintln!("deleted measurement was not loaded: {:?}", deleted);
                }
            })
        })
    }

    fn export_measurements(self: Pin<&mut Self>, url: String) -> u32 {
        handle_request(self, async move |_, _, measurement_service| {
            let (file, path) = (|| -> Result<_, UrlError> {
                let url = Url::parse(&url)?;
                if url.scheme() != "file" {
                    return Err(UrlError::UnsupportedScheme(url.scheme().to_string()));
                }
                let path = url.to_file_path().map_err(|()| {
                    UrlError::InvalidHost(url.host_str().unwrap_or("<missing>").to_string())
                })?;
                let valid = !path.exists() || path.is_file();
                if !valid {
                    return Err(UrlError::AlreadyExists);
                }
                if let Some(parent) = path.parent()
                    && !parent.exists()
                {
                    create_dir_all(parent)?;
                }
                Ok((File::create(&path)?, path))
            })()?;
            let mut file = measurement_service
                .export_measurements(
                    file,
                    ExportOptions {
                        ..ExportOptions::default()
                    },
                )
                .await?;
            file.flush()?;
            // todo notify frontend of progress along the way for progress bar,
            //      or at least the fact it finished
            Ok(move |_: Pin<&mut ffi::MeasurementModel>| {})
        })
    }

    fn import_measurements(self: Pin<&mut Self>, url: String) -> u32 {
        handle_request(self, async move |_, _, measurement_service| {
            let (file, path) = (|| -> Result<_, UrlError> {
                let url = Url::parse(&url)?;
                if url.scheme() != "file" {
                    return Err(UrlError::UnsupportedScheme(url.scheme().to_string()));
                }
                let path = url.to_file_path().map_err(|()| {
                    UrlError::InvalidHost(url.host_str().unwrap_or("<missing>").to_string())
                })?;
                let valid = path.is_file();
                if !valid {
                    return Err(UrlError::NotFound);
                }
                Ok((File::open(&path)?, path))
            })()?;
            measurement_service
                .import_measurements(
                    file,
                    ImportOptions {
                        import_collision_strategy: ImportCollisionStrategy::Replace,
                        ..ImportOptions::default()
                    },
                )
                .await?;
            // todo notify frontend of progress along the way for progress bar,
            //      or at least the fact it finished
            Ok(move |backend: Pin<&mut ffi::MeasurementModel>| {
                backend.load_measurements();
            })
        })
    }

    fn measurements(&self) -> &[ffi::Measurement] {
        &self.rust().inner.as_ref().unwrap().measurements
    }
}

macro_rules! assert_eq_field_offsets {
    ($a:ty, $b:ty $(, $f:ident)*$(,)?) => {
        $(const_assert_eq!(offset_of!($a, $f), offset_of!($b, $f));)*
    };
}

assert_eq_size!(Uuid, [u8; 16]);
assert_eq_align!(Uuid, [u8; 16]);
assert_eq_size!(Measurement, ffi::Measurement);
assert_eq_align!(Measurement, ffi::Measurement);
assert_eq_field_offsets!(
    Measurement,
    ffi::Measurement,
    id,
    systolic,
    diastolic,
    map,
    pulse,
);
const_assert_eq!(
    offset_of!(Measurement, timestamp),
    offset_of!(ffi::Measurement, key)
);

impl From<Measurement> for ffi::Measurement {
    fn from(m: Measurement) -> Self {
        ffi::Measurement {
            id: m.id.into_bytes(),
            systolic: m.systolic,
            diastolic: m.diastolic,
            map: m.map,
            pulse: m.pulse,
            key: m.timestamp,
        }
    }
}

impl From<ffi::Measurement> for Measurement {
    fn from(m: ffi::Measurement) -> Self {
        Measurement {
            id: Uuid::from_bytes(m.id),
            systolic: m.systolic,
            diastolic: m.diastolic,
            map: m.map,
            pulse: m.pulse,
            timestamp: m.key,
        }
    }
}

impl From<ffi::Measurement> for MeasurementChangeset {
    fn from(m: ffi::Measurement) -> Self {
        fn nonzero(f: f64) -> Option<f64> {
            if f != 0. { Some(f) } else { None }
        }
        MeasurementChangeset {
            id: Uuid::from_bytes(m.id),
            systolic: nonzero(m.systolic),
            diastolic: nonzero(m.diastolic),
            map: nonzero(m.map),
            pulse: nonzero(m.pulse),
            timestamp: nonzero(m.key),
        }
    }
}
