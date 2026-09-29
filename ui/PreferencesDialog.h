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
#include <QUrl>

#include <memory>


QT_BEGIN_NAMESPACE
namespace Ui
{
    class PreferencesDialog;
}
QT_END_NAMESPACE


class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);
    ~PreferencesDialog() override;

public slots:
    void reset();
    void apply();

signals:
    void databaseUrlSaved(QUrl url);

private:
    void updateButtonState() const;

    std::unique_ptr<Ui::PreferencesDialog> m_ui;

    QString m_databaseUrl;
    bool m_databaseUrlDirty;
    QString m_databaseLabelText;
};
