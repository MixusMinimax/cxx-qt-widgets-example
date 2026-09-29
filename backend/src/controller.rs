// Copyright (C) 2026  Maxi Barmetler <maxi@barmetler.com>
//
// Use of this source code is governed by either the MIT or Apache-2.0 license, at your choice.
// A copy of each can be found in the corresponding LICENSE-* file, or online (respectively):
// https://opensource.org/licenses/MIT.
// http://www.apache.org/licenses/LICENSE-2.0

use cxx::ExternType;
use std::pin::Pin;
use std::task::{Context, Poll};
use tokio::runtime::{Handle, Runtime};
use tokio::task::JoinHandle;
use tokio_util::sync::{CancellationToken, WaitForCancellationFuture};
use tokio_util::task::TaskTracker;

unsafe impl ExternType for AsyncControllerHandle {
    type Id = cxx::type_id!("backend::AsyncControllerHandle");
    type Kind = cxx::kind::Opaque;
}

#[cxx::bridge(namespace = "backend")]
mod ffi {
    extern "Rust" {
        type AsyncController;
        type AsyncControllerHandle;

        pub fn create_async_controller() -> Box<AsyncController>;

        pub fn handle(self: &AsyncController) -> Box<AsyncControllerHandle>;

        pub fn begin_shutdown(self: &mut AsyncController);

        pub fn shutdown(self: &mut AsyncController);
    }
}

#[derive(Debug)]
pub struct AsyncController {
    runtime: Runtime,
    shutdown_token: CancellationToken,
    tasks: TaskTracker,
}

#[derive(Clone, Debug)]
pub struct AsyncControllerHandle {
    runtime: Handle,
    shutdown_token: CancellationToken,
    tasks: TaskTracker,
}

impl AsyncControllerHandle {
    /// Send a cancellation event to every task spawned from this handle.
    /// The task tracker is not closed, as it is shared with other handles.
    pub fn cancel(&self) {
        self.shutdown_token.cancel();
    }
}

pub fn create_async_controller() -> Box<AsyncController> {
    Box::new(AsyncController {
        runtime: Runtime::new().expect("Failed to start tokio runtime"),
        shutdown_token: CancellationToken::new(),
        tasks: TaskTracker::new(),
    })
}

impl AsyncController {
    pub fn handle(&self) -> Box<AsyncControllerHandle> {
        Box::new(AsyncControllerHandle {
            runtime: self.runtime.handle().clone(),
            shutdown_token: self.shutdown_token.child_token(),
            tasks: self.tasks.clone(),
        })
    }

    pub fn begin_shutdown(&mut self) {
        if self.shutdown_token.is_cancelled() {
            return;
        }
        self.tasks.close();
        self.shutdown_token.cancel();
    }

    pub fn shutdown(&mut self) {
        self.begin_shutdown();
        self.runtime.block_on(async {
            self.tasks.wait().await;
        });
    }
}

pub struct CancellableJoinHandle {
    token: CancellationToken,
    handle: JoinHandle<()>,
}

impl CancellableJoinHandle {
    pub fn cancel(&self) {
        self.token.cancel();
    }

    pub fn is_cancelled(&self) -> bool {
        self.token.is_cancelled()
    }

    pub fn cancelled(&'_ self) -> WaitForCancellationFuture<'_> {
        self.token.cancelled()
    }
}

impl Future for CancellableJoinHandle {
    type Output = <JoinHandle<()> as Future>::Output;

    fn poll(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        unsafe { self.map_unchecked_mut(|this| &mut this.handle) }.poll(cx)
    }
}

#[derive(Debug, thiserror::Error)]
pub enum SpawnError {
    #[error("canceled")]
    Canceled,
}

impl AsyncControllerHandle {
    /// Spawn a callback to be executed on the runtime, if the cancellation
    /// token has not been triggerd. The callback accepts a child token of the
    /// one present in this handle. If already canceled at the current moment,
    /// [Err](Err)([SpawnError](SpawnError)::[Canceled](SpawnError::Canceled))
    /// is returned.
    pub fn spawn_cb<
        Fut: Future<Output = ()> + Send + 'static,
        Task: FnOnce(CancellationToken) -> Fut + Send + 'static,
    >(
        &self,
        task: Task,
    ) -> Result<CancellableJoinHandle, SpawnError> {
        let token = self.shutdown_token.child_token();
        if token.is_cancelled() {
            return Err(SpawnError::Canceled);
        }
        let task_token = token.clone();
        let handle = self
            .tasks
            .spawn_on(async move { task(task_token).await }, &self.runtime);
        Ok(CancellableJoinHandle { token, handle })
    }
}
