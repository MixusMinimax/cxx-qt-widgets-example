#ifndef MYAPP_MAINWINDOW_H
#define MYAPP_MAINWINDOW_H

#include <QMainWindow>
#include <backend/src/backend.cxxqt.h>
#include <backend/src/controller.cxx.h>

#include <memory>

class MyModel;

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

    void setModel(MyModel *model);

private slots:
    void newProject() const;

    void open() const;

    void save() const;

    void save_as() const;

    void quit();

    void about() const;

private:
    backend::Backend *m_backend;
    MyModel *m_model;
    std::unique_ptr<Ui::MainWindow> m_ui;
};


#endif //MYAPP_MAINWINDOW_H
