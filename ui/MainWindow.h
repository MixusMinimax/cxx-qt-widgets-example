//
// Created by maxi on 8/11/26.
//

#ifndef MYAPP_MAINWINDOW_H
#define MYAPP_MAINWINDOW_H

#include <QMainWindow>
#include <backend/src/backend.cxxqt.h>
#include <backend/src/controller.cxx.h>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(rust::Box<AsyncControllerHandle> tokio_handle, QWidget *parent = nullptr);

    ~MainWindow() override;

private:
    Backend *backend;
};


#endif //MYAPP_MAINWINDOW_H
