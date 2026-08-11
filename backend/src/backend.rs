use crate::controller::AsyncControllerHandle;
use cxx_qt::{CxxQtType, Threading};
use cxx_qt_lib::QString;
use std::cell::OnceCell;
use std::pin::Pin;
use std::time::Duration;

#[cxx_qt::bridge]
mod ffi {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qstring.h");

        type QString = cxx_qt_lib::QString;
    }

    #[namespace = "backend"]
    unsafe extern "C++" {
        include!("backend/src/controller.cxx.h");

        type AsyncControllerHandle = crate::controller::AsyncControllerHandle;
    }

    #[namespace = "backend"]
    extern "RustQt" {
        #[qobject]
        type Backend = super::BackendRust;

        pub fn initialize(self: Pin<&mut Backend>, tokio_handle: Box<AsyncControllerHandle>);

        pub fn make_message(&self, input: &QString) -> QString;

        #[qsignal]
        fn message_received(self: Pin<&mut Backend>, message: QString);
    }

    impl cxx_qt::Threading for Backend {}
}

#[derive(Default)]
pub struct BackendRust {
    tokio_handle: OnceCell<Box<AsyncControllerHandle>>,
}

impl ffi::Backend {
    pub fn initialize(mut self: Pin<&mut Self>, tokio_handle: Box<AsyncControllerHandle>) {
        let qt_thread = self.qt_thread();
        self.as_mut()
            .rust_mut()
            .tokio_handle
            .set(tokio_handle)
            .unwrap();
        self.rust()
            .tokio_handle
            .get()
            .unwrap()
            .spawn_cb(|token| async move {
                let mut counter = 0;
                loop {
                    tokio::select! {
                        _ = token.cancelled() => {
                            println!("Cancelled");
                            return;
                        }

                        _ = tokio::time::sleep(Duration::from_secs(2)) => {
                            counter += 1;
                            qt_thread
                                .queue(move |mut backend| {
                                    backend
                                        .as_mut()
                                        .message_received(QString::from(format!("Hello from Tokio! {counter}")));
                                })
                                .unwrap();
                        }
                    }
                }
            });
    }

    pub fn make_message(&self, input: &QString) -> QString {
        format!("Rust says: {input}").into()
    }
}
