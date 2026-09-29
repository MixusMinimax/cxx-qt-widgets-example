#include "MainWindow.h"

#include "BloodPressureGraph.h"
#include "ui_MainWindow.h"

#include <QDebug>
#include <QSettings>
#include <QStatusBar>
#include <QWidget>
#include <backend/src/backend.cxxqt.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow{parent},
      m_measurementDialog{new MeasurementDialog{this}},
      m_preferencesDialog{new PreferencesDialog{this}},
      m_ui{std::make_unique<Ui::MainWindow>()}
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
    connect(m_ui->actionSettings, &QAction::triggered, this, &MainWindow::openPreferences);
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::about);

    connect(m_ui->graphOutput, &BloodPressureGraph::measurementEditStarted, [this](measurements::measurement m) {
        m_measurementDialog->initialize(m);
        m_measurementDialog->setModal(true);
        m_measurementDialog->show();
        m_measurementDialog->raise();
        m_measurementDialog->activateWindow();
    });

    connect(m_ui->graphOutput, &BloodPressureGraph::measurementCreateStarted, [this](QDateTime date_time) {
        m_measurementDialog->initialize(std::move(date_time));
        m_measurementDialog->setModal(true);
        m_measurementDialog->show();
        m_measurementDialog->raise();
        m_measurementDialog->activateWindow();
    });
}

MainWindow::~MainWindow() = default;

void MainWindow::setModel(
    measurements::MeasurementModel *model, std::function<rust::Box<backend::AsyncControllerHandle>()> get_handle
)
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
                m_model, &measurements::MeasurementModel::failure,
                [this](const uint32_t req_id, const QString &msg) {
                    qDebug() << QString{"%1: %2"}.arg(req_id).arg(msg);
                    statusBar()->showMessage(QString{"%1: %2"}.arg(req_id).arg(msg));
                }
            ),
            connect(
                m_measurementDialog, &MeasurementDialog::accepted,
                [this] {
                    switch (m_measurementDialog->acceptMode()) {
                        case MeasurementDialog::AcceptCreate:
                            m_model->create_measurement(m_measurementDialog->measurement());
                            break;
                        case MeasurementDialog::AcceptUpdate:
                            m_model->update_measurement(m_measurementDialog->measurement());
                            break;
                        case MeasurementDialog::AcceptDelete:
                            m_model->delete_measurement(m_measurementDialog->measurement().id);
                            break;
                    }
                }
            ),
            connect(m_preferencesDialog, &PreferencesDialog::databaseUrlSaved, [this, get_handle](QUrl url) {
                m_model->initialize(get_handle(), url.toString().toStdString());
                m_model->load_measurements();
            }),
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

void MainWindow::open()
{
    constexpr static auto KEY = "transfer/import/dir";

    qDebug() << "MainWindow::open()";
    statusBar()->showMessage(tr("open"));
    QSettings settings{};
    auto url = settings.value(KEY).toUrl();
    if (!url.isValid()) {
        url = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    }
    url = QFileDialog::getOpenFileUrl(this, tr("Import CSV"), url, tr("CSV files (*.csv)"));
    qDebug() << "import file:" << url.toString();
    if (url.isEmpty()) {
        qDebug() << "MainWindow::open: canceled";
        return;
    }
    settings.setValue(KEY, url.adjusted(QUrl::RemoveFilename));
    if (!m_model) {
        qDebug() << "MainWindow::open: m_model was null";
        return;
    }
    m_model->import_measurements(url.toString().toStdString());
}

void MainWindow::save()
{
    qDebug() << "MainWindow::save()";
    statusBar()->showMessage(tr("save"));
    save_as(); // for now, later we will probably bind this to C-e (export)
}

void MainWindow::save_as()
{
    constexpr static auto KEY = "transfer/export/dir";

    qDebug() << "MainWindow::save_as()";
    statusBar()->showMessage(tr("save_as"));
    QSettings settings{};
    auto url = settings.value(KEY).toUrl();
    if (!url.isValid()) {
        url = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    }
    auto name = QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm");
    name.append(".csv");
    url.setPath(QDir{url.path()}.filePath(name));
    url = QFileDialog::getSaveFileUrl(this, tr("Export CSV"), url, tr("CSV files (*.csv)"));
    if (url.isEmpty()) {
        qDebug() << "MainWindow::save_as: canceled";
        return;
    }
    settings.setValue(KEY, url.adjusted(QUrl::RemoveFilename));
    if (!m_model) {
        qDebug() << "MainWindow::save_as: m_model was null";
        return;
    }
    m_model->export_measurements(url.toString().toStdString());
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
void MainWindow::openPreferences() const
{
    m_preferencesDialog->setModal(true);
    m_preferencesDialog->reset();
    m_preferencesDialog->show();
}
