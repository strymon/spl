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
 \file DcLog.h
 \brief Logging class that wraps the QDebug facility.
--------------------------------------------------------------------------*/
#ifndef DCLOG_H
#define DCLOG_H

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QTextStream>


class DcLog
{

    // Log size limit in bytes.
#ifndef QT_NO_DEBUG 
    static const quint64 kLogSizeLimit = 1024*1024;
#else
    static const quint64 kLogSizeLimit = 1024*100;
#endif
    
    // The number of log files to keep once they exceed
    // the size limit.
    static const int kLogFileHistoryCount = 2;

public:

    static QString* LogPath;
    
    DcLog();
    DcLog(QString logPath);

    void printLogHdr();

    virtual ~DcLog();

    static void muteQDebug();
private:

    QString setFileOrdinal(const QString& fileName, int ord);
    QString removeFileOrdinal( const QString& fileName);
    void backUp(const QString& filePath,const int maxBackupCount);
    QString makeNewName( QString fileName );
    int getFileOrdinal( const QString& fileName);

    QString _str;
    QtMessageHandler _orig;
    qint64 _logSizeLimit;
};

// An example how this is used in an application
// DcLog* _log is defined in the header
// Put this in the constructor
#define DCLOG_INIT_LOG_FOR_APP _log = new DcLog(QDir::toNativeSeparators( QApplication::applicationName() + ".log" ))

#ifndef DCNODBG
#define DCDBG   qDebug
#else
#define DCDBG while (false) QMessageLogger().noDebug
#endif

#ifndef DCNOLOG
#define DCLOG   qDebug
#else
#define DCLOG while (false) QMessageLogger().noDebug
#endif

#define DCERROR qWarning
#define DCWARN  qWarning
#define DCCRIT  qCritical


#endif // DCLOG_H
