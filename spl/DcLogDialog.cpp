#include "DcLogDialog.h"
#include "ui_DcLogDialog.h"
#include <QProcess>

#include "cmn/DcLog.h"
#include "IoProgressDialog.h"
#include "cmn/DcQUtils.h"
#include <qs3/qs3.h>
#include <QScrollBar>

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
       // QString d = LoadLog(*_log);
        //ui->textBrowser->setText( d );
        startTimer(1);
    }
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
    // send to strymon

    pushUsersLog(_logText, ui->lineEdit->text());

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

            _logText = LoadLog(*_log,true,30000);
            ui->horizontalSlider->setMaximum(_max);
            ui->horizontalSlider->setValue(qMin(30000,_max));

            ui->label_3->setText(QString("All(%1)").arg(_max));
            ui->textEdit->setText(_logText);
            ui->pushButton_3->setText("Refresh ("+QString::number(ui->horizontalSlider->value()) + ")");

            QApplication::processEvents();

            QScrollBar *sb = ui->textEdit->verticalScrollBar();
            sb->setValue(sb->maximum() - 1);

            ui->lineEdit->setFocus();

            QString logName = GetEnviValue("USERNAME") + "_" + GetEnviValue("COMPUTERNAME") + "_" + DcQUtils::getTimeStamp() + "_spl.log";
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

void DcLogDialog::pushUsersLog(const QString textToSend, const QString note /*=""*/)

{
    IoProgressDialog* dlg=new IoProgressDialog(this);
    dlg->setModal(true);

    const QString s3Host = "s3.amazonaws.com";
    const QString s3Proxy = "";

    QS3::S3 s3(s3Host, s3Proxy);

    // The S3 keys will get you access to the strymon-inbox, it will allow object writes only.
    QScopedPointer<QS3::Bucket> bucket(s3.bucket("strymon-inbox", "AKIAJZFPMQ3GFR57BS4A", "ecYiGu3RD4jPOyrEMHHfPHw4W9nKcPM3CfcHuQwX"));

    QByteArray logdata;

    if(!note.isEmpty())
    {
        logdata = "User Note: ";
        logdata += note;
        logdata += "\n";
    }

    logdata += textToSend.toLatin1();

    
    // QString logName = GetEnviValue("(USER:USERNAME") + "_" + GetEnviValue("(HOSTNAME|COMPUTERNAME") + "_" + DcQUtils::getTimeStamp() + "_spl.log"; 
    

    if(_useCompression)
        logdata = qCompress(logdata);

// Test example
//    QByteArray uncompresseddata = qUncompress(compressedData);
//    if(uncompresseddata == logdata)
//    {
//        *_con << "same\n";
//    }

    dlg->reset();
    dlg->setMessage("Sending to Strymon Support");
    dlg->show();
    QApplication::processEvents();

    bucket->upload(windowTitle(),logdata);

    bool done = false;
    connect(bucket.data(), &QS3::Bucket::finished, [&] ()
    {
        done = true;
    });

    dlg->setMax(100);

    while(!done && !dlg->cancled())
    {
        dlg->show();
        QApplication::processEvents();
        dlg->inc();
    }
    
    ui->pushButton->setEnabled(dlg->cancled());

    dlg->hide();
    dlg->deleteLater();

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
