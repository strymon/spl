#include "DcFileDownloader.h"

DcFileDownloader::DcFileDownloader(QUrl url, QObject *parent) :
    QObject(parent)
{
    connect(&m_WebCtrl, SIGNAL(finished(QNetworkReply*)),
        SLOT(fileDownloaded(QNetworkReply*)));

    _url = url;
    QNetworkRequest request(url);
    m_WebCtrl.get(request);
}

DcFileDownloader::~DcFileDownloader()
{

}

void DcFileDownloader::fileDownloaded(QNetworkReply* pReply)
{
    m_DownloadedData = pReply->readAll();
    //emit a signal
    pReply->deleteLater();
    emit downloaded();
}

QByteArray DcFileDownloader::downloadedData() const
{
    return m_DownloadedData;
}

