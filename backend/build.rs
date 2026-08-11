use cxx_qt_build::CxxQtBuilder;

fn main() {
    CxxQtBuilder::new()
        .files(["src/backend.rs", "src/controller.rs"])
        .build();
}
