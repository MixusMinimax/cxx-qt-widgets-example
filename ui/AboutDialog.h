#pragma once

#include <QDialog>

#include <memory>


QT_BEGIN_NAMESPACE
namespace Ui
{
    class AboutDialog;
}
QT_END_NAMESPACE

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);
    ~AboutDialog() override;

private:
    std::unique_ptr<Ui::AboutDialog> ui;
};
