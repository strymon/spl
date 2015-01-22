/*-------------------------------------------------------------------------
	    Copyright 2013 Damage Control Engineering, LLC

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*-------------------------------------------------------------------------*/
/*!
 \file DcLog.cpp
--------------------------------------------------------------------------*/
#include "DcLog.h"
#include "DcQUtils.h"
#include <QApplication>
#include <QFileInfo>
#include <QDate>
#include <QDir>


QString* DcLog::LogPath = 0;
bool gDcLogShortPath = true;


void muteMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);
    Q_UNUSED(msg);
}

void DbgMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type);

    if(!DcLog::LogPath)
        return;

    QString srcfile = context.file;
    if(gDcLogShortPath)
    {
        srcfile = QFileInfo(srcfile).baseName();
    }

    QFile log_file (*DcLog::LogPath);
    log_file.open (QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    QTextStream text_stream (&log_file);
    text_stream << DcQUtils::getTimeStamp() << QLatin1String(" [") << srcfile << QLatin1String("::") << context.line << QLatin1String("] ") << msg << QLatin1String("\n");
}


void DcLog::backUp(const QString& filePath,const int maxBackupCopy)
{
    QFileInfo fi(filePath);
    QString fileName = fi.fileName();
    int ord = getFileOrdinal(fileName);

    if(ord == maxBackupCopy-1)
    {
        if(QFile::exists(filePath))
        {
            QFile::remove(filePath);
        }
        backUp(setFileOrdinal(filePath,ord-1),maxBackupCopy);
    }
    else if(ord == 0)
    {
        if(QFile::exists(filePath))
        {
            QFile::copy(filePath,setFileOrdinal(filePath,ord+1));
            QFile::remove(filePath);
        }

        QString fname = removeFileOrdinal(filePath);
        QFile::copy(fname,filePath);
        
        QFile::remove(fname);
        return;
    }
    else
    {
        if(QFile::exists(filePath))
        {
            QFile::copy(filePath,setFileOrdinal(filePath,ord+1));
            QFile::remove(filePath); 
        }
        backUp(setFileOrdinal(filePath,ord-1),maxBackupCopy);
    }
}

DcLog::DcLog(QString logPath)
{
    _logSizeLimit = kLogSizeLimit;

    QFileInfo logInfo(logPath);
    qint64 fs = logInfo.size();
    
    
    if(fs  > _logSizeLimit)
    {
        // Recursively backup the log file
        backUp(setFileOrdinal(logPath,kLogFileHistoryCount-1),kLogFileHistoryCount);
    }
    _str = logPath;
    DcLog::LogPath = &_str;
    _orig = qInstallMessageHandler(DbgMessageOutput);

    printLogHdr();


}

DcLog::DcLog()
{
    DcLog::LogPath = 0;
    _orig = 0;
}

DcLog::~DcLog()
{
    try
    {
        qDebug() << "Destructor Called";
    }
    catch (...)
    {
    }

    qInstallMessageHandler(_orig);
}

//-------------------------------------------------------------------------
void DcLog::muteQDebug()
{
    qInstallMessageHandler(muteMessageHandler);
}

//-------------------------------------------------------------------------
QString DcLog::makeNewName(  QString fileName )
{
    
    if( fileName.contains(QRegExp("[0-9]\\.[a-zA-Z]{1,}$")))
    {
        int idx = fileName.indexOf(QRegExp("[0-9]+\\.[a-zA-Z]{1,}$"));
        if(idx)
        {
            int ord = fileName.toUInt();
            ord++;
            fileName = fileName.replace(idx,1,QString::number(ord));
        }
    }
    else if( fileName.contains(QRegExp("\\.[a-zA-Z]{1,}$")))
    {
        int idx = fileName.indexOf(QRegExp("\\.[a-zA-Z]{1,}$"));
        if(idx)
        {
            fileName = fileName.replace(idx, 1, QString::number(0) + QLatin1Char('.') );
        }

    }
    
    //QString newLogName = logInfo.path() + "/" + fileName;
    
    return fileName;
}

//-------------------------------------------------------------------------
int DcLog::getFileOrdinal( const QString& fileName )
{
    int ord = 0;

    if( fileName.contains(QRegExp("[0-9]\\.[a-zA-Z]{1,}$")))
    {
        int idx = fileName.indexOf(QRegExp("[0-9]+\\.[a-zA-Z]{1,}$"));
        if(idx)
        {
            ord = fileName.mid(idx,1).toUInt();
        }
    }

    return ord;
}

QString DcLog::setFileOrdinal(const QString& fileName, int ord)
{
    QString newName = fileName;

    if( fileName.contains(QRegExp("[0-9]\\.[a-zA-Z]{1,}$")))
    {
        int idx = fileName.indexOf(QRegExp("[0-9]+\\.[a-zA-Z]{1,}$"));
        if(idx)
        {
            newName = newName.replace(idx,1,QString::number(ord));
        }
    }
    else if( fileName.contains(QRegExp("\\.[a-zA-Z]{1,}$")))
    {
        int idx = fileName.indexOf(QRegExp("\\.[a-zA-Z]{1,}$"));
        if(idx)
        {
            newName = newName.replace(idx, 1, QString::number(ord) + QLatin1Char('.') );
        }
    }
    
    return newName;
}

QString DcLog::removeFileOrdinal( const QString& fileName)
{
    QString newName = fileName;
    if( fileName.contains(QRegExp("[0-9}\\.[a-zA-Z]{1,}$")))
    {
        int idx = fileName.indexOf(QRegExp("[0-9]+\\.[a-zA-Z]{1,}$"));
        if(idx)
        {
            newName = newName.replace(idx,1,QLatin1String(""));
        }
    }

    return newName;
}

//-------------------------------------------------------------------------
void DcLog::printLogHdr()
{
    qDebug() << "------------------------------------------------------------------------------------------";
    qDebug() << "----------------------------------- Log Startup " << QDate::currentDate().toString("dd/MM/yy") << " -----------------------------";
    qDebug() << "------------------------------------------------------------------------------------------";
    qDebug() << "NAME: " << QApplication::applicationName();
    qDebug() << "APP VERSION: " << QApplication::applicationVersion();
    qDebug() << "PID: " << QApplication::applicationPid();
    qDebug() << "APP PATH: " << QApplication::applicationFilePath();
    qDebug() << "OS VERSION: " << DcQUtils::getOsVersion();
}
