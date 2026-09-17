#include "MainWindow.h"

#include "BloodPressureGraph.h"
#include "ui_MainWindow.h"

#include <QDebug>
#include <QStatusBar>
#include <QWidget>
#include <backend/src/backend.cxxqt.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow{parent}, m_measurementModal{new MeasurementModal{this}}, m_ui{std::make_unique<Ui::MainWindow>()}
{
    m_ui->setupUi(this);

    statusBar()->showMessage(tr("Hello World!"));

    m_ui->graphOutput->setMapSelectable(false);

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
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::about);

    connect(m_ui->graphOutput, &BloodPressureGraph::measurementEditStarted, [this](measurements::measurement m) {
        m_measurementModal->initialize(m);
        m_measurementModal->setModal(true);
        m_measurementModal->show();
        m_measurementModal->raise();
        m_measurementModal->activateWindow();
    });

    connect(m_ui->graphOutput, &BloodPressureGraph::measurementCreateStarted, [this](QDateTime date_time) {
        m_measurementModal->initialize(std::move(date_time));
        m_measurementModal->setModal(true);
        m_measurementModal->show();
        m_measurementModal->raise();
        m_measurementModal->activateWindow();
    });

    connect(m_measurementModal, &MeasurementModal::accepted, [this] {
        switch (m_measurementModal->acceptMode()) {
            case MeasurementModal::AcceptCreate:
                m_model->create_measurement(m_measurementModal->measurement());
                break;
            case MeasurementModal::AcceptUpdate:
                m_model->update_measurement(m_measurementModal->measurement());
                break;
            case MeasurementModal::AcceptDelete:
                m_model->delete_measurement(m_measurementModal->measurement().id);
                break;
        }
    });
}

MainWindow::~MainWindow() = default;

void MainWindow::setModel(measurements::MeasurementModel *model)
{
    if (m_model == model) return;
    if (m_model) {
        for (auto &&conn: m_modelConnections)
            disconnect(conn);
    }
    m_model = model;
    if (m_model) {
        m_modelConnections = {
            connect(
                m_model, &measurements::MeasurementModel::failure, [this](const uint32_t req_id, const QString &msg) {
                    qDebug() << QString{"%1: %2"}.arg(req_id).arg(msg);
                    statusBar()->showMessage(QString{"%1: %2"}.arg(req_id).arg(msg));
                }
            ),
        };
    } else {
        m_modelConnections = {};
    }
    m_ui->graphOutput->setModel(model);
}

void MainWindow::newProject() const
{
    qDebug() << "MainWindow::newProject()";
    statusBar()->showMessage(tr("newProject"));
}

void MainWindow::open() const
{
    qDebug() << "MainWindow::open()";
    statusBar()->showMessage(tr("open"));
}

void MainWindow::save() const
{
    qDebug() << "MainWindow::save()";
    statusBar()->showMessage(tr("save"));
}

void MainWindow::save_as() const
{
    qDebug() << "MainWindow::save_as()";
    statusBar()->showMessage(tr("save_as"));
}

void MainWindow::quit()
{
    qDebug() << "MainWindow::quit()";
    statusBar()->showMessage(tr("quit"));
    close();
}

void MainWindow::about() const
{
    qDebug() << "MainWindow::about()";
    statusBar()->showMessage(tr("about"));
}
