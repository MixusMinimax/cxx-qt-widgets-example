//
// Created by maxi on 9/7/26.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MeasurementModal.h" resolved

#include "MeasurementModal.h"

#include "overloaded.h"
#include "ui_MeasurementModal.h"

#include <backend/src/backend.cxxqt.h>


MeasurementModal::MeasurementModal(QWidget *parent) : QDialog{parent}, m_ui{std::make_unique<Ui::MeasurementModal>()}
{
    m_ui->setupUi(this);

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

MeasurementModal::~MeasurementModal() = default;

void MeasurementModal::initialize(const initialize_opts &opts)
{
    const auto initialize = [](QSpinBox *input, const double value) {
        input->setValue(
            static_cast<int>(std::lround(
                std::clamp(value, static_cast<double>(input->minimum()), static_cast<double>(input->maximum()))
            ))
        );
    };

    std::visit(
        overloaded{
            [&, this](const measurements::measurement &measurement) {
                initialize(m_ui->systolicEdit, measurement.systolic);
                initialize(m_ui->diastolicEdit, measurement.diastolic);
                initialize(m_ui->mapEdit, measurement.map);
                initialize(m_ui->pulseEdit, measurement.pulse);
                m_ui->dateTimeEdit->setDateTime(measurement.date_time);
            },
            [&, this](const QDateTime &date_time) {
                m_ui->systolicEdit->clear();
                m_ui->diastolicEdit->clear();
                m_ui->mapEdit->clear();
                m_ui->pulseEdit->clear();
                m_ui->dateTimeEdit->setDateTime(date_time);
            }
        },
        opts
    );

    m_ui->systolicEdit->setFocus(Qt::PopupFocusReason);
}
