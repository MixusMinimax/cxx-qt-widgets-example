use crate::controller::AsyncControllerHandle;
use crate::domain::measurement::{Measurement, MeasurementService};
use cxx_qt::{CxxQtType, Threading};
use cxx_qt_lib::{QDate, QDateTime, QString, QTime, QTimeZone};
use diesel::internal::derives::multiconnection::chrono::{Datelike, Timelike};
use std::cell::{OnceCell, RefCell};
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
        pub date_time: QDateTime,
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

        #[qsignal]
        fn measurements_loaded(
            self: Pin<&mut MeasurementModel>,
            req_id: u32,
            measurements: Vec<Measurement>,
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
    req_id: RefCell<u32>, // no multithreading needed
}

impl ffi::MeasurementModel {
    pub fn initialize(self: Pin<&mut Self>, tokio_handle: Box<AsyncControllerHandle>) {
        self.rust_mut()
            .inner
            .set(MeasurementModelRustInner {
                tokio_handle,
                measurement_service: Default::default(),
                req_id: Default::default(),
            })
            .unwrap();
    }

    pub fn load_measurements(self: Pin<&mut Self>) -> u32 {
        let rust = self.rust().inner.get().unwrap();
        let req_id = rust.req_id.replace_with(|i| *i + 1);
        let qt_thread = self.qt_thread();
        let measurement_service: Arc<MeasurementService> = rust.measurement_service.clone();
        rust.tokio_handle.spawn_cb(async move |ct| {
            match measurement_service.load_measurements(ct).await {
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
        req_id
    }
}

fn convert_measurement(m: Measurement) -> ffi::Measurement {
    let dt = m.date_time;
    ffi::Measurement {
        id: m.id.into_bytes(),
        systolic: m.systolic,
        diastolic: m.diastolic,
        map: m.map,
        pulse: m.pulse,
        key: 0.,
        date_time: QDateTime::from_date_and_time_time_zone(
            &QDate::new(dt.year(), dt.month() as i32, dt.day() as i32),
            &QTime::from_msecs_since_start_of_day(
                dt.num_seconds_from_midnight() as i32 * 1000 + (dt.nanosecond() / 1_000_000) as i32,
            ),
            &QTimeZone::system_time_zone(),
        ),
    }
}
