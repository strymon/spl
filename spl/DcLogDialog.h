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

    QString GetEnviValue( const QString& key );

    QByteArray LoadLog(const DcLog &log,bool lastlogtoo,int limit=999999999);

    QByteArray Load( const QString& logpath)
    {
        QByteArray rtdata;
        QFile f( logpath );
        
        f.open( QIODevice::ReadOnly | QIODevice::Text );
        if( f.isOpen() )
        {
            rtdata += f.readAll();
        }
        
        return rtdata;
    }

private slots:
    void on_pushButton_2_clicked();

    void on_pushButton_clicked();

    void on_pushButton_3_clicked();

    void on_horizontalSlider_rangeChanged(int min, int max);

    void on_horizontalSlider_sliderMoved(int position);

    void on_horizontalSlider_sliderReleased();

private:
    DcLog* _log;
    Ui::DcLogDialog *ui;
    QString _logText;
    bool _useCompression;
    // QWidget interface
    int _t;
    int _max;
protected:
    void showEvent(QShowEvent *);

    // QObject interface
protected:
    void timerEvent(QTimerEvent *);

    void Load(bool lastonetoo);

};

#endif // DCLOGDIALOG_H
