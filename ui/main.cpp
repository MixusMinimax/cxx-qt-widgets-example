#include "MainWindow.h"
#include "MyModel.h"

#include <QApplication>
#include <backend/src/controller.cxx.h>

#include <memory>

int main(int argc, char *argv[]) {
    auto controller = backend::create_async_controller();

    const QApplication app(argc, argv);

    const auto model = std::make_unique<MyModel>();

    MainWindow window{controller->handle()};
    window.setModel(model.get());
    window.show();

    QApplication::connect(&app, &QCoreApplication::aboutToQuit, [&controller] {
        controller->begin_shutdown();
#ifdef _WIN32
        // On windows, QApplication::exec() is not guaranteed to return. It may exit instead.
        // We therefore need to fully wait for tokio to shut down.
        controller->shutdown();
#endif
    });

    const int ret = QApplication::exec();
    controller->shutdown();
    return ret;
}
