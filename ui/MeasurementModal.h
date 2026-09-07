//
// Created by maxi on 9/7/26.
//

#ifndef MYAPP_MEASUREMENTMODAL_H
#define MYAPP_MEASUREMENTMODAL_H

#include <QDialog>

#include <memory>


QT_BEGIN_NAMESPACE
namespace Ui
{
    class MeasurementModal;
}
QT_END_NAMESPACE

class MeasurementModal : public QDialog
{
    Q_OBJECT

public:
    explicit MeasurementModal(QWidget *parent = nullptr);
    ~MeasurementModal() override;

private:
    std::unique_ptr<Ui::MeasurementModal> m_ui;
};


#endif // MYAPP_MEASUREMENTMODAL_H
