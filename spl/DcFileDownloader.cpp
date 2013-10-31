#include "DcFileDownloader.h"
#include <QEventLoop>

DcFileDownloader::DcFileDownloader(QUrl url, QObject *parent) :
    QObject(parent)
{
    _errorString.clear();

    connect(&m_WebCtrl, SIGNAL(finished(QNetworkReply*)),
        SLOT(fileDownloaded(QNetworkReply*)));


    QEventLoop loop;
    connect(this, SIGNAL(Done()), &loop, SLOT(quit()));


    _url = url;

    QNetworkRequest request(url);
    m_WebCtrl.get(request);
    
    loop.exec();

    emit downloaded();
}

DcFileDownloader::~DcFileDownloader()
{

}

void DcFileDownloader::fileDownloaded(QNetworkReply* pReply)
{
    
    if(pReply->error() == QNetworkReply::NoError)
    {
        _errorString.clear();
        m_DownloadedData = pReply->readAll();
    }
    else
    {
        _errorString  = pReply->errorString();
        m_DownloadedData.clear();
    }
    
    //emit a signal
    pReply->deleteLater();

    emit Done();
}

QByteArray DcFileDownloader::downloadedData() const
{
    return m_DownloadedData;
}

bool DcFileDownloader::saveData(QString fname)
{
    QByteArray ba = downloadedData();
    QFile file(fname);
    file.open(QFile::ReadWrite);
    file.write(ba,ba.length());
    file.close();

    return true;
}

//-------------------------------------------------------------------------
bool DcFileDownloader::hasError()
{
    return !_errorString.isEmpty();
}

//-------------------------------------------------------------------------
QString DcFileDownloader::getErrorStr()
{
    return _errorString;
}

