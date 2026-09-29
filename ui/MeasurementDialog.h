/*==========================================================================**
**                                                                          **
**  Copyright (C) 2026  Maxi Barmetler <maxi@barmetler.com>                 **
**                                                                          **
**  This program is free software: you can redistribute it and/or modify    **
**  it under the terms of the GNU General Public License as published by    **
**  the Free Software Foundation, either version 3 of the License, or       **
**  (at your option) any later version.                                     **
**                                                                          **
**  This program is distributed in the hope that it will be useful,         **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of          **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           **
**  GNU General Public License for more details.                            **
**                                                                          **
**  You should have received a copy of the GNU General Public License       **
**  along with this program.  If not, see <https://www.gnu.org/licenses/>.  **
**                                                                          **
**==========================================================================*/

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
