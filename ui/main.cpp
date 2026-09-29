/*==========================================================================**
**                                                                          **
**  Copyright (C) 2026  Maxi Barmetler <maxi@barmetler.com>                 **
**                                                                          **
**  This program is free software: you can redistribute it and/or modify    **
**  it under the terms of the GNU General Public License as published by    **
**  the Free Software Foundation, either version 3 of the License, or       **
**  (at your option) any later version.                                     **
**                                                                          **
**  This program is distributed in the hope that it will be useful,         **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of          **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           **
**  GNU General Public License for more details.                            **
**                                                                          **
**  You should have received a copy of the GNU General Public License       **
**  along with this program.  If not, see <https://www.gnu.org/licenses/>.  **
**                                                                          **
**==========================================================================*/

#include "MainWindow.h"
#include "config.h"

#include <QApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <backend/src/backend.cxxqt.h>
#include <backend/src/controller.cxx.h>

#include <memory>

int main(int argc, char *argv[])
{
    auto controller = backend::create_async_controller();

    const QApplication app{argc, argv};

    QCoreApplication::setOrganizationName(config::ORGANIZATION);
    QCoreApplication::setOrganizationDomain(config::ORGANIZATION_DOMAIN);
    QCoreApplication::setApplicationName(config::APPLICATION_NAME);

    if (const auto url = QSettings{}.value(config::SETTING_DATABASE_CONNECTION).toUrl();
        !url.isValid() || !url.path().endsWith(".db")) {
        QSettings{}.setValue(
            config::SETTING_DATABASE_CONNECTION,
            QUrl::fromLocalFile(
                QDir{QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)}.filePath("data.db")
            )
        );
    }

    const auto model = std::make_unique<measurements::MeasurementModel>();
    model->initialize(
        controller->handle(), QSettings{}.value(config::SETTING_DATABASE_CONNECTION).toUrl().toString().toStdString()
    );

    MainWindow window{};
    window.setModel(model.get(), [&controller] { return controller->handle(); });
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
