//
// Created by maxi on 9/28/26.
//

#include "PreferencesDialog.h"

#include "ui_PreferencesDialog.h"

PreferencesDialog::PreferencesDialog(QWidget *parent) : QDialog{parent}, m_ui{std::make_unique<Ui::PreferencesDialog>()}
{
    m_ui->setupUi(this);
}

PreferencesDialog::~PreferencesDialog() = default;
