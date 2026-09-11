use crate::controller::AsyncControllerHandle;
use crate::domain::measurement::{Measurement, MeasurementService};
use cxx_qt::{CxxQtType, Threading};
use cxx_qt_lib::QString;
use sql_uuid::Uuid;
use static_assertions::{assert_eq_align, assert_eq_size, const_assert_eq};
use std::cell::OnceCell;
use std::mem::offset_of;
use std::pin::Pin;
use std::sync::Arc;
use std::{mem, slice};

#[cxx_qt::bridge]
mod ffi {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qstring.h");
        include!("cxx-qt-lib/qdatetime.h");

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

        fn initialize(self: Pin<&mut MeasurementModel>, tokio_handle: Box<AsyncControllerHandle>);

        fn load_measurements(self: Pin<&mut MeasurementModel>) -> u32;

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
    inner: OnceCell<MeasurementModelRustInner>,
}

#[derive(Debug)]
struct MeasurementModelRustInner {
    tokio_handle: Box<AsyncControllerHandle>,
    measurement_service: Arc<MeasurementService>,
    req_id: u32, // no multithreading needed
    measurements: Vec<ffi::Measurement>,
}

impl ffi::MeasurementModel {
    pub fn initialize(self: Pin<&mut Self>, tokio_handle: Box<AsyncControllerHandle>) {
        self.rust_mut()
            .inner
            .set(MeasurementModelRustInner {
                tokio_handle,
                measurement_service: Default::default(),
                req_id: Default::default(),
                measurements: Default::default(),
            })
            .unwrap();
    }

    pub fn load_measurements(self: Pin<&mut Self>) -> u32 {
        let qt_thread = self.qt_thread();
        let mut rm = self.rust_mut();
        let rust = rm.inner.get_mut().unwrap();
        let req_id = {
            let req_id = rust.req_id;
            rust.req_id += 1;
            req_id
        };
        let measurement_service: Arc<MeasurementService> = rust.measurement_service.clone();
        rust.tokio_handle.spawn_cb(async move |ct| {
            match measurement_service.load_measurements(ct).await {
                Ok(v) => {
                    let v = unsafe { mem::transmute::<Vec<Measurement>, Vec<ffi::Measurement>>(v) };
                    qt_thread
                        .queue(move |mut backend| {
                            backend
                                .as_mut()
                                .rust_mut()
                                .inner
                                .get_mut()
                                .unwrap()
                                .measurements = v;
                            let measurements = unsafe {
                                let r = &backend.inner.get().unwrap().measurements;
                                slice::from_raw_parts(r.as_ptr(), r.len())
                            };
                            backend.measurements_loaded(req_id, measurements);
                        })
                        .inspect_err(|e| eprintln!("{e}"))
                        .ok();
                }
                Err(e) => {
                    qt_thread
                        .queue(move |backend| backend.failure(req_id, QString::from(e.to_string())))
                        .inspect_err(|e| eprintln!("{e}"))
                        .ok();
                }
            };
        });
        req_id
    }

    fn measurements(&self) -> &[ffi::Measurement] {
        &self.rust().inner.get().unwrap().measurements
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
