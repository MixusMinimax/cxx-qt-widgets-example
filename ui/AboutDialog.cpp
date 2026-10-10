#include "AboutDialog.h"

#include "qcustomplot.h"
#include "ui_AboutDialog.h"
#include "version.h"

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent), ui{std::make_unique<Ui::AboutDialog>()}
{
    ui->setupUi(this);
    auto text = ui->aboutLabel->text();
    text.replace("{{VERSION}}", BPD_VERSION);
    text.replace("{{QCP_VERSION}}", QCUSTOMPLOT_VERSION_STR);
    ui->aboutLabel->setText(text);
}

AboutDialog::~AboutDialog() = default;
