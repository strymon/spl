#ifndef DCLOGDIALOG_H
#define DCLOGDIALOG_H

#include <QDialog>

#include <cmn/DcLog.h>

namespace Ui {
class DcLogDialog;
}

class DcLogDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DcLogDialog(QWidget *parent = 0,DcLog* lg =0);
    ~DcLogDialog();

    void pushUsersLog(const QString textToSend, const QString note);

    QByteArray LoadLog(const DcLog &log);
private slots:
    void on_pushButton_2_clicked();

    void on_pushButton_clicked();

private:
    DcLog* _log;
    Ui::DcLogDialog *ui;
    QString _logText;

    // QWidget interface
protected:
    void showEvent(QShowEvent *);

    // QObject interface
protected:
    void timerEvent(QTimerEvent *);
};

#endif // DCLOGDIALOG_H
