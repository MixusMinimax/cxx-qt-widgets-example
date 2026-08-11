#include <QApplication>

#include "MainWindow.h"
#include <backend/src/controller.cxx.h>

int main(int argc, char *argv[]) {
    auto controller = create_async_controller();

    const QApplication app(argc, argv);

    MainWindow window{controller->handle()};
    window.show();

    QApplication::connect(&app, &QCoreApplication::aboutToQuit, [&controller] {
        controller->begin_shutdown();
    });

    const int ret = QApplication::exec();
    controller->shutdown();
    return ret;
}
