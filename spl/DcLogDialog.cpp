#include "DcLogDialog.h"
#include "ui_DcLogDialog.h"
#include <QProcess>

#include "cmn/DcLog.h"
#include "IoProgressDialog.h"
#include "cmn/DcQUtils.h"

#include <QScrollBar>
#include <QHostInfo>
#include <QClipboard>

DcLogDialog::DcLogDialog(QWidget *parent, DcLog *lg) :
    QDialog(parent),
    _log(lg),
    ui(new Ui::DcLogDialog),
    _useCompression(false),
    _t(0)
{
    ui->setupUi(this);

    if(_log)
    {
        startTimer(1);
    }
    ui->controlFrame->setVisible(false);
}

DcLogDialog::~DcLogDialog()
{
    delete ui;
}

void DcLogDialog::on_pushButton_2_clicked()
{
    // Dismiss button
    close();
}

void DcLogDialog::on_pushButton_clicked()
{

    // pushUsersLog(_logText, ui->lineEdit->text());

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(_logText + "\n" + ui->lineEdit->text());
}

void DcLogDialog::showEvent(QShowEvent *)
{

}

void DcLogDialog::timerEvent(QTimerEvent *e)
{
    int id = e->timerId();

    if(_t != id)
    {
        killTimer(id);

        if(_log)
        {
            ui->textEdit->setText("<br><br><br><center><h1>Loading...</h1></center>");
            QApplication::processEvents();

            _logText = LoadLog(*_log,true,250000);
            ui->horizontalSlider->setMaximum(_max);
            ui->horizontalSlider->setValue( qMin( 250000,_max ) );

            ui->label_3->setText(QString("All(%1)").arg(_max));
            ui->textEdit->setText(_logText);
            ui->pushButton_3->setText("Refresh ("+QString::number(ui->horizontalSlider->value()) + ")");

            QApplication::processEvents();

            QScrollBar *sb = ui->textEdit->verticalScrollBar();
            sb->setValue(sb->maximum() - 1);

            ui->lineEdit->setFocus();

#ifdef Q_OS_OSX
            QString logName = GetEnviValue("USER") + "_" + QHostInfo::localHostName();
#else
    QString logName = GetEnviValue("USERNAME") + "_" + GetEnviValue("COMPUTERNAME");
#endif

            QApplication::processEvents();
            setWindowTitle(logName);
            QApplication::processEvents();
        }
    }
    else
    {
       killTimer(_t);

        int hs = ui->horizontalSlider->value();
       QString d = "<html>" + LoadLog(*_log,false,hs) + "</html>";
            d.replace("Log Startup","<b>Log Startup</b>");
            d.replace("\n","<br>");

            ui->textEdit->setText(d);

        QApplication::processEvents();
        QScrollBar *sb = ui->textEdit->verticalScrollBar();
        sb->setValue(sb->maximum() - 1);
        ui->pushButton->setEnabled(true);
        _t=0;
    }
}

QByteArray DcLogDialog::LoadLog(const DcLog &log,bool loglasttoo,int limit)
{
    QByteArray logdata;

    logdata = Load(log.getLogPath());
   
    if(logdata.length()<limit && loglasttoo)
    {
        logdata += Load(log.getLogPath(0));
    }

    _max = logdata.length();

    int len = qMin(limit,logdata.length());
    return logdata.right(len);
}


QString DcLogDialog::GetEnviValue( const QString& key )
{
    QString rtval = QString("%1uk").arg(key);
    
    QStringList environment = QProcess::systemEnvironment();
    int index = environment.indexOf(QRegExp(key+".*"));
    if (index != -1)
    {
        QStringList stringList = environment.at(index).split('=');
        if (stringList.size() == 2)
        {
            rtval = stringList.at(1).toUtf8();
        }
    }
    
    return rtval;
}

void DcLogDialog::on_pushButton_3_clicked()
{
     int hs = ui->horizontalSlider->value();
    QString d = "<html>" + LoadLog(*_log,false,hs) + "</html>";
         d.replace("Log Startup","<h1>Log Startup</h1>");
         d.replace("\n","<br>");
         
         ui->textEdit->setText(d);

     QApplication::processEvents();
     QScrollBar *sb = ui->textEdit->verticalScrollBar();
     sb->setValue(sb->maximum() - 1);

     ui->pushButton->setEnabled(true);
}

void DcLogDialog::on_horizontalSlider_rangeChanged(int min, int max)
{
(void)min;
    (void)max;
}

void DcLogDialog::on_horizontalSlider_sliderMoved(int)
{
    ui->pushButton_3->setText("Refresh");
    if(_t) {
        killTimer(_t);
        _t=0;
    }

}

void DcLogDialog::on_horizontalSlider_sliderReleased()
{
    _t = startTimer(400);
}

void DcLogDialog::on_lineEdit_returnPressed()
{
    if(ui->lineEdit->text() ==
            "show")
    {
        ui->controlFrame->setVisible(true);
    }
}
