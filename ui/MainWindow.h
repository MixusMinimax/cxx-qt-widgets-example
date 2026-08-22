//
// Created by maxi on 8/11/26.
//

#ifndef MYAPP_MAINWINDOW_H
#define MYAPP_MAINWINDOW_H

#include <QMainWindow>
#include <backend/src/backend.cxxqt.h>
#include <backend/src/controller.cxx.h>

#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}

QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(rust::Box<backend::AsyncControllerHandle> tokio_handle, QWidget *parent = nullptr);

    ~MainWindow() override;

private:
    backend::Backend *backend;
    std::unique_ptr<Ui::MainWindow> ui;
};


#endif //MYAPP_MAINWINDOW_H
