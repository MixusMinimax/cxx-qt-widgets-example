use crate::backend::ffi::QString;
use crate::controller::AsyncControllerHandle;
use crate::domain::measurement::{Measurement, MeasurementService};
use cxx_qt::{CxxQtType, Threading};
use std::cell::OnceCell;
use std::pin::Pin;
use std::sync::Arc;

#[cxx_qt::bridge]
mod ffi {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qstring.h");
        include!("cxx-qt-lib/qdatetime.h");

        type QString = cxx_qt_lib::QString;
        type QDateTime = cxx_qt_lib::QDateTime;
    }

    #[namespace = "backend"]
    unsafe extern "C++" {
        include!("backend/src/controller.cxx.h");

        type AsyncControllerHandle = crate::controller::AsyncControllerHandle;
    }

    #[namespace = "backend"]
    extern "RustQt" {
        #[qobject]
        type MeasurementModel = super::MeasurementModelRust;

        fn initialize(self: Pin<&mut MeasurementModel>, tokio_handle: Box<AsyncControllerHandle>);

        fn load_measurements(self: Pin<&mut MeasurementModel>, req_id: u64);

        #[qsignal]
        fn measurements_loaded(
            self: Pin<&mut MeasurementModel>,
            req_id: u64,
            measurements: Vec<Measurement>,
        );

        #[qsignal]
        fn failure(self: Pin<&mut MeasurementModel>, req_id: u64, msg: QString);
    }

    impl cxx_qt::Threading for MeasurementModel {}

    #[namespace = "backend"]
    #[derive(Clone, PartialEq, Debug, Default)]
    pub struct Measurement {
        pub id: [u8; 16],
        pub systolic: f64,
        pub diastolic: f64,
        pub map: f64,
        pub pulse: f64,
        pub key: f64,
        pub date_time: QDateTime,
    }

    #[namespace = "backend"]
    #[derive(Clone, PartialEq, Debug)]
    pub enum MeasurementType {
        Systolic,
        Diastolic,
        Map,
        Pulse,
    }
}

#[derive(Default)]
pub struct MeasurementModelRust {
    inner: OnceCell<MeasurementModelRustInner>,
}

#[derive(Debug)]
struct MeasurementModelRustInner {
    tokio_handle: Box<AsyncControllerHandle>,
    measurement_service: Arc<MeasurementService>,
}

impl ffi::MeasurementModel {
    pub fn initialize(self: Pin<&mut Self>, tokio_handle: Box<AsyncControllerHandle>) {
        self.rust_mut()
            .inner
            .set(MeasurementModelRustInner {
                tokio_handle,
                measurement_service: Arc::default(),
            })
            .unwrap();
    }

    pub fn load_measurements(self: Pin<&mut Self>, req_id: u64) {
        let rust = self.rust().inner.get().unwrap();
        let qt_thread = self.qt_thread();
        let measurement_service: Arc<MeasurementService> = rust.measurement_service.clone();
        rust.tokio_handle.spawn_cb(async move |cb| {
            match measurement_service.load_measurements(cb).await {
                Ok(v) => {
                    let v = v
                        .into_iter()
                        .map(convert_measurement)
                        .collect::<Vec<ffi::Measurement>>();
                    qt_thread
                        .queue(move |backend| {
                            backend.measurements_loaded(req_id, v);
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
    }
}

fn convert_measurement(m: Measurement) -> ffi::Measurement {
    ffi::Measurement {
        id: m.id.into_bytes(),
        systolic: m.systolic,
        diastolic: m.diastolic,
        map: m.map,
        pulse: m.pulse,
        key: 0.,
        date_time: Default::default(),
    }
}
