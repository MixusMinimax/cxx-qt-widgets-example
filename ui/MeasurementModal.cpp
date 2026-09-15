//
// Created by maxi on 9/7/26.
//

// You may need to build the project (run Qt uic code generator) to get "ui_MeasurementModal.h" resolved

#include "MeasurementModal.h"

#include "overloaded.h"
#include "ui_MeasurementModal.h"

#include <backend/src/backend.cxxqt.h>

#include "qcustomplot.h"
#include "util.h"


MeasurementModal::MeasurementModal(QWidget *parent) : QDialog{parent}, m_ui{std::make_unique<Ui::MeasurementModal>()}
{
    m_ui->setupUi(this);

    connect(m_ui->systolicEdit, &QSpinBox::editingFinished, this, &MeasurementModal::validate);
    connect(m_ui->diastolicEdit, &QSpinBox::editingFinished, this, &MeasurementModal::validate);
    connect(m_ui->mapEdit, &QSpinBox::editingFinished, this, &MeasurementModal::validate);
    connect(m_ui->pulseEdit, &QSpinBox::editingFinished, this, &MeasurementModal::validate);
    connect(m_ui->systolicEdit, &QSpinBox::textChanged, this, &MeasurementModal::validate);
    connect(m_ui->diastolicEdit, &QSpinBox::textChanged, this, &MeasurementModal::validate);
    connect(m_ui->mapEdit, &QSpinBox::textChanged, this, &MeasurementModal::validate);
    connect(m_ui->pulseEdit, &QSpinBox::textChanged, this, &MeasurementModal::validate);
    connect(m_ui->dateTimeEdit, &QDateTimeEdit::dateTimeChanged, this, &MeasurementModal::validate);

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
                m_acceptMode = AcceptUpdate;
                m_ui->internalIDEdit->setText(std::format("{:x}", util::uuid{measurement.id}).data());
                initialize(m_ui->systolicEdit, measurement.systolic);
                initialize(m_ui->diastolicEdit, measurement.diastolic);
                initialize(m_ui->mapEdit, measurement.map);
                initialize(m_ui->pulseEdit, measurement.pulse);
                m_ui->dateTimeEdit->setDateTime(QCPAxisTickerDateTime::keyToDateTime(measurement.key));
            },
            [&, this](const QDateTime &date_time) {
                m_acceptMode = AcceptCreate;
                m_ui->internalIDEdit->setText(tr("new"));
                m_ui->systolicEdit->clear();
                m_ui->diastolicEdit->clear();
                m_ui->mapEdit->clear();
                m_ui->pulseEdit->clear();
                m_ui->dateTimeEdit->setDateTime(date_time);
            }
        },
        opts
    );

    validate();
    m_ui->systolicEdit->setFocus(Qt::PopupFocusReason);
}

measurements::measurement MeasurementModal::measurement() const
{
    return measurements::measurement{
        .systolic = static_cast<double>(m_ui->systolicEdit->value()),
        .diastolic = static_cast<double>(m_ui->diastolicEdit->value()),
        .map = static_cast<double>(m_ui->mapEdit->hasAcceptableInput() ? m_ui->mapEdit->value() : 0),
        .pulse = static_cast<double>(m_ui->pulseEdit->value()),
        .key = QCPAxisTickerDateTime::dateTimeToKey(m_ui->dateTimeEdit->dateTime())
    };
}

MeasurementModal::AcceptMode MeasurementModal::acceptMode() const { return m_acceptMode; }

bool MeasurementModal::validate()
{
    // we do not validate map as it can be automatically calculated. Maybe I'll add a button for that.
    const auto valid = m_ui->systolicEdit->hasAcceptableInput()
        && m_ui->diastolicEdit->hasAcceptableInput()
        && m_ui->pulseEdit->hasAcceptableInput()
        && m_ui->dateTimeEdit->hasAcceptableInput();

    m_ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(valid);

    return valid;
}

void MeasurementModal::accept()
{
    if (validate()) {
        QDialog::accept();
    }
}
