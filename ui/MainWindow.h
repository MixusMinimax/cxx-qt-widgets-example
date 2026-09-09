#ifndef MYAPP_MAINWINDOW_H
#define MYAPP_MAINWINDOW_H

#include "MeasurementModal.h"

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

    void setModel(measurements::MeasurementModel *model);

private slots:
    void newProject() const;
    void open() const;
    void save() const;
    void save_as() const;
    void quit();
    void about() const;

private:
    measurements::MeasurementModel *m_model{nullptr};
    MeasurementModal *m_measurementModal;
    std::array<QMetaObject::Connection, 0> m_modelConnections{};
    std::unique_ptr<Ui::MainWindow> m_ui;
};


#endif // MYAPP_MAINWINDOW_H
