#include "MainWindow.h"

#include <QApplication>
#include <backend/src/backend.cxxqt.h>
#include <backend/src/controller.cxx.h>

#include <memory>

int main(int argc, char *argv[])
{
    auto controller = backend::create_async_controller();

    const QApplication app{argc, argv};

    QCoreApplication::setOrganizationName("Barmetler");
    QCoreApplication::setOrganizationDomain("barmetler.com");
    QCoreApplication::setApplicationName("Blood Pressure Diary");

    const auto model = std::make_unique<measurements::MeasurementModel>();
    model->initialize(controller->handle());

    MainWindow window{};
    window.setModel(model.get());
    window.show();

    QApplication::connect(&app, &QCoreApplication::aboutToQuit, [&controller] {
        controller->begin_shutdown();
#ifdef _WIN32
        // On windows, QApplication::exec() is not guaranteed to return. It may exit instead.
        // We therefore need to fully wait for tokio to shut down.
        controller->shutdown();
        // wait for blocking futures to finish:
        void(rust::Box{std::move(controller)});
#endif
    });

    const auto ret = QApplication::exec();
#ifndef _WIN32
    controller->shutdown();
    // wait for blocking futures to finish:
    void(rust::Box{std::move(controller)});
#endif
    return ret;
}
