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

#pragma once

#include "MeasurementDialog.h"
#include "PreferencesDialog.h"

#include <QMainWindow>
#include <backend/src/backend.cxxqt.h>

#include <array>
#include <memory>


QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}

QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow() override;

    void setModel(
        measurements::MeasurementModel *model,
        const std::function<rust::Box<backend::AsyncControllerHandle>()> &get_handle
    );

protected slots:
    void import_csv();
    void export_csv();

public slots:
    void writeSettings() const;
    void readSettings();
    void openPreferences() const;
    void openAbout() const;
    void quit();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    measurements::MeasurementModel *m_model{nullptr};
    MeasurementDialog *m_measurementDialog;
    PreferencesDialog *m_preferencesDialog;
    std::array<QMetaObject::Connection, 4> m_modelConnections{};
    std::unique_ptr<Ui::MainWindow> m_ui;
};
