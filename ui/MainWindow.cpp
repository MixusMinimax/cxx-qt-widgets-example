#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QStatusBar>
#include <QMenuBar>
#include <QPushButton>
#include <QWindowStateChangeEvent>
#include <QWindow>
#include <QWidget>

MainWindow::MainWindow(rust::Box<backend::AsyncControllerHandle> tokio_handle, QWidget *parent) : QMainWindow(parent),
    backend{new backend::Backend(this)},
    ui{std::make_unique<Ui::MainWindow>()} {
    backend->initialize(std::move(tokio_handle));
    ui->setupUi(this);

    statusBar()->showMessage(tr("Hello World!"));

    ui->actionNew->setShortcuts(QKeySequence::New);
    ui->actionOpen->setShortcuts(QKeySequence::Open);
    ui->actionSave->setShortcuts(QKeySequence::Save);
    ui->actionSaveAs->setShortcuts(QKeySequence::SaveAs);
    ui->actionQuit->setShortcuts(QKeySequence::Quit);

    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newProject);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::open);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::save);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::save_as);
    connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::quit);

    connect(ui->pushButton, &QPushButton::pressed, [this] {
        statusBar()->showMessage(backend->make_message(tr("button")));
    });

    connect(backend, &backend::Backend::message_received, [this](const QString &msg) {
        statusBar()->showMessage(msg);
    });
}

MainWindow::~MainWindow() = default;

void MainWindow::newProject() const {
    statusBar()->showMessage(tr("actionNew"));
}

void MainWindow::open() const {
    statusBar()->showMessage(tr("actionOpen"));
}

void MainWindow::save() {
}

void MainWindow::save_as() {
}

void MainWindow::quit() {
    close();
}

void MainWindow::about() {
}
