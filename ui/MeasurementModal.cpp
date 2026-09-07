//
// Created by maxi on 9/7/26.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MeasurementModal.h" resolved

#include "MeasurementModal.h"
#include "ui_MeasurementModal.h"


MeasurementModal::MeasurementModal(QWidget *parent) : QDialog{parent}, m_ui{std::make_unique<Ui::MeasurementModal>()}
{
    m_ui->setupUi(this);
}

MeasurementModal::~MeasurementModal() = default;
