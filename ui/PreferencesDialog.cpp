//
// Created by maxi on 9/28/26.
//

#include "PreferencesDialog.h"

#include "config.h"
#include "ui_PreferencesDialog.h"

#include <QDir>
#include <QFileDialog>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>

PreferencesDialog::PreferencesDialog(QWidget *parent) : QDialog{parent}, m_ui{std::make_unique<Ui::PreferencesDialog>()}
{
    m_ui->setupUi(this);

    m_databaseLabelText = m_ui->databaseConnectionLabel->text();

    connect(m_ui->databaseConnectionEdit, &QLineEdit::textChanged, [this](const QString &s) {
        const auto old_dirty = m_databaseUrlDirty;
        m_databaseUrlDirty = s != m_databaseUrl;
        if (m_databaseUrlDirty && !old_dirty) {
            m_ui->databaseConnectionLabel->setText(QString{"<b>*%1</b>"}.arg(m_databaseLabelText));
        } else if (!m_databaseUrlDirty && old_dirty) {
            m_ui->databaseConnectionLabel->setText(m_databaseLabelText);
        }
        updateButtonState();
    });

    connect(m_ui->databaseConnectionPicker, &QPushButton::pressed, [this] {
        const auto url = QFileDialog::getSaveFileUrl(
            this, tr("Database Location"), QSettings{}.value(config::SETTING_DATABASE_CONNECTION).toUrl(),
            tr("Database Files (*.db)"), nullptr, QFileDialog::DontConfirmOverwrite
        );
        if (url.isValid()) {
            m_ui->databaseConnectionEdit->setText(url.toString());
        }
    });

    connect(m_ui->databaseConnectionDefault, &QPushButton::pressed, [this] {
        const auto url = QUrl::fromLocalFile(
            QDir{QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)}.filePath("data.db")
        );
        if (url.isValid()) {
            m_ui->databaseConnectionEdit->setText(url.toString());
        }
    });

    connect(m_ui->buttonBox, &QDialogButtonBox::clicked, [this](QAbstractButton *button) {
        if (m_ui->buttonBox->standardButton(button) == QDialogButtonBox::Apply) {
            apply();
        }
    });

    connect(this, &PreferencesDialog::accepted, [this] { apply(); });
}

PreferencesDialog::~PreferencesDialog() = default;

void PreferencesDialog::reset()
{
    m_databaseUrl = QSettings{}.value(config::SETTING_DATABASE_CONNECTION).toUrl().toString();
    m_databaseUrlDirty = false;
    m_ui->databaseConnectionLabel->setText(m_databaseLabelText);
    m_ui->databaseConnectionEdit->setText(m_databaseUrl);
    updateButtonState();
}

void PreferencesDialog::apply()
{
    qDebug() << "apply";
    if (m_databaseUrlDirty) {
        const auto url = QUrl::fromUserInput(m_ui->databaseConnectionEdit->text());
        if (url.isValid()) {
            QSettings{}.setValue(config::SETTING_DATABASE_CONNECTION, url);
            m_databaseUrl = url.toString();
            m_ui->databaseConnectionEdit->setText(url.toString());
            emit databaseUrlSaved(url);
            m_databaseUrlDirty = false;
            m_ui->databaseConnectionLabel->setText(m_databaseLabelText);
        }
    }
    updateButtonState();
}

void PreferencesDialog::updateButtonState() const
{
    const auto dirty = m_databaseUrlDirty;
    const auto valid = QUrl::fromUserInput(m_ui->databaseConnectionEdit->text()).isValid()
        && m_ui->databaseConnectionEdit->text().endsWith(".db");
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(dirty && valid);
    m_ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(dirty && valid);
}
