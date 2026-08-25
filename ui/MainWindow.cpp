#include "MainWindow.h"
#include "BloodPressureGraph.h"
#include "ui_MainWindow.h"

#include <QDebug>
#include <QPushButton>
#include <QStatusBar>
#include <QWidget>

#include "MyModel.h"

MainWindow::MainWindow(rust::Box<backend::AsyncControllerHandle> tokio_handle, QWidget *parent) :
    QMainWindow(parent), m_backend{new backend::Backend(this)}, m_ui{std::make_unique<Ui::MainWindow>()} {
    m_backend->initialize(std::move(tokio_handle));
    m_ui->setupUi(this);

    statusBar()->showMessage(tr("Hello World!"));

    m_ui->actionNew->setShortcuts(QKeySequence::New);
    m_ui->actionOpen->setShortcuts(QKeySequence::Open);
    m_ui->actionSave->setShortcuts(QKeySequence::Save);
    m_ui->actionSaveAs->setShortcuts(QKeySequence::SaveAs);
    m_ui->actionQuit->setShortcuts(QKeySequence::Quit);

    connect(m_ui->actionNew, &QAction::triggered, this, &MainWindow::newProject);
    connect(m_ui->actionOpen, &QAction::triggered, this, &MainWindow::open);
    connect(m_ui->actionSave, &QAction::triggered, this, &MainWindow::save);
    connect(m_ui->actionSaveAs, &QAction::triggered, this, &MainWindow::save_as);
    connect(m_ui->actionQuit, &QAction::triggered, this, &MainWindow::quit);
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::about);

    connect(m_ui->pushButton, &QPushButton::pressed,
            [this] { statusBar()->showMessage(m_backend->make_message(tr("button"))); });

    connect(m_backend, &backend::Backend::message_received,
            [this](const QString &msg) { statusBar()->showMessage(msg); });
}

MainWindow::~MainWindow() = default;

void MainWindow::setModel(MyModel *model) {
    if (m_model == model) return;
    if (m_model) {
        for (auto &&conn: m_modelConnections)
            disconnect(conn);
    }
    m_model = model;
    if (m_model) {
        m_modelConnections = {
            connect(m_ui->pushButton, &QPushButton::pressed, m_model, &MyModel::incrementSpeed),
        };
    }
    m_ui->graphOutput->setModel(model);
}

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
