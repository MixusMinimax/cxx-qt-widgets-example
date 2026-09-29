#ifndef MYAPP_MAINWINDOW_H
#define MYAPP_MAINWINDOW_H

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
        measurements::MeasurementModel *model, std::function<rust::Box<backend::AsyncControllerHandle>()> get_handle
    );

private slots:
    void newProject() const;
    void open();
    void save();
    void save_as();
    void quit();
    void about() const;

public slots:
    void openPreferences() const;

private:
    measurements::MeasurementModel *m_model{nullptr};
    MeasurementDialog *m_measurementDialog;
    PreferencesDialog *m_preferencesDialog;
    std::array<QMetaObject::Connection, 3> m_modelConnections{};
    std::unique_ptr<Ui::MainWindow> m_ui;
};


#endif // MYAPP_MAINWINDOW_H
