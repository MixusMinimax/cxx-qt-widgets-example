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
