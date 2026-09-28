#pragma once

#include <QDialog>

#include <array>
#include <memory>
#include <variant>


namespace measurements
{
    struct measurement;
}


QT_BEGIN_NAMESPACE
namespace Ui
{
    class MeasurementDialog;
}
QT_END_NAMESPACE

using initialize_opts = std::variant<QDateTime, measurements::measurement>;

class MeasurementDialog : public QDialog
{
    Q_OBJECT

public:
    enum AcceptMode
    {
        AcceptCreate,
        AcceptUpdate,
        AcceptDelete,
    };
    Q_ENUM(AcceptMode)

    explicit MeasurementDialog(QWidget *parent = nullptr);
    ~MeasurementDialog() override;

    void initialize(const initialize_opts &opts);
    [[nodiscard]] measurements::measurement measurement() const;

    [[nodiscard]] AcceptMode acceptMode() const;

protected slots:
    bool validate();

protected:
    void accept() override;

    AcceptMode m_acceptMode;

private:
    std::unique_ptr<Ui::MeasurementDialog> m_ui;
    std::array<std::uint8_t, 16> m_id{};
};
