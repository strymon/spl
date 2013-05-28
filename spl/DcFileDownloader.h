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
#pragma once


// This class was taken from: https://qt-project.org/wiki/Download_Data_from_URL
// With a very slight modification

#ifndef DCFILEDOWNLOADER_H
#define DCFILEDOWNLOADER_H

#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDir>
class DcFileDownloader : public QObject
{
    Q_OBJECT
public:
    explicit DcFileDownloader(QUrl url, QObject *parent = 0);

    virtual ~DcFileDownloader();

    QByteArray downloadedData() const;
    QString getName() { return QFileInfo(_url.path()).fileName();}
signals:
    void downloaded();

    private slots:

        void fileDownloaded(QNetworkReply* pReply);

private:

    QNetworkAccessManager m_WebCtrl;
    QUrl _url;
    QByteArray m_DownloadedData;

};

#endif // DCFILEDOWNLOADER_H

