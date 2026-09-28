#pragma once

#include <QDialog>

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

private:
    std::unique_ptr<Ui::PreferencesDialog> m_ui;
};
