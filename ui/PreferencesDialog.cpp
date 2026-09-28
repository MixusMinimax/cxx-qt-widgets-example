//
// Created by maxi on 9/28/26.
//

#include "PreferencesDialog.h"
#include "config.h"

#include "ui_PreferencesDialog.h"

#include <QSettings>

PreferencesDialog::PreferencesDialog(QWidget *parent) : QDialog{parent}, m_ui{std::make_unique<Ui::PreferencesDialog>()}
{
    m_ui->setupUi(this);
}

PreferencesDialog::~PreferencesDialog() = default;

void PreferencesDialog::reset()
{
    m_ui->databaseConnectionEdit->setText(QSettings{}.value(config::SETTING_DATABASE_CONNECTION).toUrl().toString());
}
