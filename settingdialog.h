#ifndef SETTINGDIALOG_H
#define SETTINGDIALOG_H

#include <QDialog>

namespace Ui {
    class SettingDialog;
}
class QweenSettings;
class QLineEdit;
class SettingDialog : public QDialog {
    Q_OBJECT
public:
    SettingDialog(QWidget *parent = 0);
    ~SettingDialog();

    void updateUi();

public slots:
    virtual void accept();

protected:
    void changeEvent(QEvent *e);

private:
    static void setLineEditBgColor(QLineEdit *edit, const QColor& color);
    Ui::SettingDialog *ui;
    QweenSettings *settings;
    QColor m_inputBgColor;

private slots:
    void on_btnInputFont_clicked();
    void on_btnInputBgColor_clicked();
};

#endif // SETTINGDIALOG_H
