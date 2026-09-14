//
// Created by maxi on 9/7/26.
//

#ifndef MYAPP_MEASUREMENTMODAL_H
#define MYAPP_MEASUREMENTMODAL_H

#include <QDialog>

#include <memory>


namespace measurements
{
    struct measurement;
}


QT_BEGIN_NAMESPACE
namespace Ui
{
    class MeasurementModal;
}
QT_END_NAMESPACE

using initialize_opts = std::variant<QDateTime, measurements::measurement>;

class MeasurementModal : public QDialog
{
    Q_OBJECT

public:
    explicit MeasurementModal(QWidget *parent = nullptr);
    ~MeasurementModal() override;

    void initialize(const initialize_opts &opts);

    [[nodiscard]] measurements::measurement measurement() const;

private:
    std::unique_ptr<Ui::MeasurementModal> m_ui;
};


#endif // MYAPP_MEASUREMENTMODAL_H
