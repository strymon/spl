#ifndef DCLOG_H
#define DCLOG_H

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QTextStream>


class DcLog
{

public:

    static QString* LogPath;

    DcLog();
    DcLog(QString logPath);

    QString makeNewName( QString fileName );
    int getFileOrdinal( const QString& fileName);
    virtual ~DcLog();

    static void muteQDebug();
    void test();
    QString setFileOrdinal(const QString& fileName, int ord);
    QString removeFileOrdinal( const QString& fileName);
    void backUp(const QString& filePath,const int maxBackupCount);
private:
    QString _str;
    QtMessageHandler _orig;

};

#define DCLOG  qDebug
#define DCWARN  qWarning
#define DCCRIT  qCritical


#endif // DCLOG_H
