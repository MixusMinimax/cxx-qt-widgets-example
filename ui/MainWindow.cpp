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

    // auto *fileMenu = menuBar()->addMenu(tr("&File"));
    // const auto *actionNew = fileMenu->addAction(QIcon::fromTheme(QIcon::ThemeIcon::DocumentNew), tr("&New"));
    // const auto *actionOpen = fileMenu->addAction(QIcon::fromTheme(QIcon::ThemeIcon::DocumentOpen), tr("&Open"));
    //
    // auto *button = new QPushButton(tr("asd"));
    // setCentralWidget(button); // takes ownership of button
    //
    // connect(actionNew, &QAction::triggered, [this] { statusBar()->showMessage(tr("actionNew")); });
    // connect(actionOpen, &QAction::triggered, [this] { statusBar()->showMessage(tr("actionOpen")); });
    // connect(button, &QPushButton::pressed, [this] { statusBar()->showMessage(backend->make_message(tr("button"))); });

    connect(backend, &backend::Backend::message_received, [this](const QString &msg) {
        statusBar()->showMessage(msg);
    });
}

MainWindow::~MainWindow() = default;
