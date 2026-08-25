#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QStatusBar>
#include <QPushButton>
#include <QWindowStateChangeEvent>
#include <QWidget>
#include <QDebug>

#include "GraphWidget.h"

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
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::about);

    connect(ui->pushButton, &QPushButton::pressed, [this] {
        statusBar()->showMessage(backend->make_message(tr("button")));
    });

    connect(backend, &backend::Backend::message_received, [this](const QString &msg) {
        statusBar()->showMessage(msg);
    });


    // generate some data:
    QVector<double> x(101), y(101); // initialize with entries 0..100
    for (int i = 0; i < 101; ++i) {
        x[i] = i / 50.0 - 1; // x goes from -1 to 1
        y[i] = x[i] * x[i]; // let's plot a quadratic function
    }
    // create graph and assign data to it:
    ui->graphOutput->addGraph();
    ui->graphOutput->graph(0)->setData(x, y);
    // give the axes some labels:
    ui->graphOutput->xAxis->setLabel("x");
    ui->graphOutput->yAxis->setLabel("y");
    // set axes ranges, so we see all data:
    ui->graphOutput->xAxis->setRange(-1, 1);
    ui->graphOutput->yAxis->setRange(0, 1);
    ui->graphOutput->replot();
}

MainWindow::~MainWindow() = default;

void MainWindow::newProject() const {
    qDebug() << "MainWindow::newProject()";
    statusBar()->showMessage(tr("newProject"));
}

void MainWindow::open() const {
    qDebug() << "MainWindow::open()";
    statusBar()->showMessage(tr("open"));
}

void MainWindow::save() const {
    qDebug() << "MainWindow::save()";
    statusBar()->showMessage(tr("save"));
}

void MainWindow::save_as() const {
    qDebug() << "MainWindow::save_as()";
    statusBar()->showMessage(tr("save_as"));
}

void MainWindow::quit() {
    qDebug() << "MainWindow::quit()";
    statusBar()->showMessage(tr("quit"));
    close();
}

void MainWindow::about() const {
    qDebug() << "MainWindow::about()";
    statusBar()->showMessage(tr("about"));
}
