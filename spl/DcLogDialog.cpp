#include "DcLogDialog.h"
#include "ui_DcLogDialog.h"
#include <QProcess>

#include "cmn/DcLog.h"
#include "IoProgressDialog.h"

#include <qs3/qs3.h>

DcLogDialog::DcLogDialog(QWidget *parent, DcLog *lg) :
    QDialog(parent),
    _log(lg),
    ui(new Ui::DcLogDialog)
{
    ui->setupUi(this);

    if(_log)
    {
       // QString d = LoadLog(*_log);
        //ui->textBrowser->setText( d );
        startTimer(10);
    }
}

DcLogDialog::~DcLogDialog()
{
    delete ui;
}

void DcLogDialog::on_pushButton_2_clicked()
{
    // Dismiss button
}

void DcLogDialog::on_pushButton_clicked()
{
    // send to strymon
    pushUsersLog(*_log,ui->lineEdit->text());

}

void DcLogDialog::showEvent(QShowEvent *)
{

}

void DcLogDialog::timerEvent(QTimerEvent *e)
{
     killTimer(e->timerId());

    if(_log)
    {
        QString d = LoadLog(*_log);
        int len = qMin(100000,d.length());
        ui->textEdit->setText(d.right(len));
    }


}

QByteArray DcLogDialog::LoadLog(const DcLog &log)
{
    QByteArray logdata;

   QString logpath = log.getLogPath();


   {
      QFile f( logpath );
      f.open( QIODevice::ReadOnly | QIODevice::Text );
      if( f.isOpen() )
      {
           logdata += f.readAll();
      }
   }

   logpath = log.getLogPath(0);
   {
       QFile f( logpath );
       if(f.exists())
       {
           f.open( QIODevice::ReadOnly | QIODevice::Text );
           if( f.isOpen() )
           {
               logdata += f.readAll();
           }
       }
   }

    return logdata;
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

    QString logName = "spl.log.";
    QString username = "unknown";

    QStringList environment = QProcess::systemEnvironment();
    int index = environment.indexOf(QRegExp("USERNAME"));
    if (index != -1)
    {
       QStringList stringList = environment.at(index).split('=');
       if (stringList.size() == 2)
       {
           username = stringList.at(1).toUtf8();
        }
    }
    else
    {
        int index = environment.indexOf(QRegExp("USERNAME"));
        if (index != -1)
        {
            QStringList stringList = environment.at(index).split('=');
            if (stringList.size() == 2)
            {
                username = stringList.at(1).toUtf8();
            }
        }
    }


    logName += username;
    QByteArray compressedData = qCompress(logdata);

// Test example
//    QByteArray uncompresseddata = qUncompress(compressedData);
//    if(uncompresseddata == logdata)
//    {
//        *_con << "same\n";
//    }

    dlg->reset();
    dlg->show();
    dlg->setMessage("Sending to Strymon Support");
    QApplication::processEvents();

    bucket->upload(logName,compressedData);

    // wait finish
    QEventLoop loop;

    bool done = false;
    connect(bucket.data(), &QS3::Bucket::finished, [&] ()
    {
        done = true;
    });


//    connect(bucket.data(), &QS3::Bucket::progress, [&] (const QString& key, qint64 sent, qint64 total)
//    {

//    });

    dlg->setMax(1);
    while(!done && dlg->cancled())
    {
        loop.processEvents();
        QApplication::processEvents();
        dlg->inc();
    }
    dlg->hide();

    delete dlg;

}
