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
 \file DcSoftwareUpdate.h

--------------------------------------------------------------------------*/
#pragma once
#include <QUrl>
#include "DcFileDownloader.h"
#include <QString>
#include <QFile>

#include "DcPackageIndex.h"
#include "DcDeviceDetails.h"


struct DcUpdateWorklist
{
    DcUpdateWorklist() {fwFile.clear();presetSysexFileList.clear();otherSysexFileList.clear();changeLog.clear();changeLogFilePath.clear();info.clear();}
    
    QString         fwFile;
    QStringList     presetSysexFileList;
    QStringList     otherSysexFileList;
    QString         changeLog;
    QString         changeLogFilePath;
    QString         changeLogUrl;
    DcPackageIndex::DcPackageDesc info;

    bool isValid() { return !fwFile.isEmpty();}
    bool hasPresets();
};


class DcSoftwareUpdate : public QObject
{
           Q_OBJECT;
public:
    static const char* kRedirectFileUrl;

    DcSoftwareUpdate() {_lastResult.setString(&_lastResultString);};
    DcSoftwareUpdate(QString localPath, QString idxUrl) : _rootPath(localPath), _idxUrl(idxUrl) 
    {
        _lastResult.setString(&_lastResultString);
    }

    ~DcSoftwareUpdate() {};
    
    bool init(QString localPath, QString idxUrl);

    bool getIndexUrlFromFile( QString urlFilePath, QString& url );

    void uninit();

    bool prepForUpdateToLatest(DcDeviceDetails& dev, DcUpdateWorklist& updateWorkList);

    bool getLatestInfo( DcDeviceDetails& dev,QString& verStr, QStringList& changeLog);

    // For methods that return bool, use this method for more details.
    QString getLastResult();

protected:

    bool downloadFile( QString srcUrl, QString destPath );

    // Create the "update progress/status" file
    bool createUpdateProgressTrackingFile(DcPackageIndex::DcPackageDesc desc);

    // Verify the update progress file can be created
    bool prepProgressFile( DcPackageIndex::DcPackageDesc& desc,QString& progFilePath);

    inline QString prevProgFileName(DcPackageIndex::DcPackageDesc& desc)
    {
        return QDir::toNativeSeparators(_rootPath + desc.id + "/_prev_update_progress.ini");
    }

    inline QString progFileName(DcPackageIndex::DcPackageDesc& desc)
    {
        return QDir::toNativeSeparators(_rootPath + desc.id + "/_update_progress.ini");
    }
   
    // If needed, create the package path, return true if it exists or it was created.
    bool verifyLocalPath(DcPackageIndex::DcPackageDesc desc);
    
    // Return true if the local package path exists.
    bool checkLocalPath(DcPackageIndex::DcPackageDesc desc);

    // For the given package description, return the local path
    QString toLocalPath( DcPackageIndex::DcPackageDesc &desc )
    {
        return QDir::toNativeSeparators(_rootPath + desc.id);
    }
    
    // Append the full local to the given file 'path'
    QString appendWithLocalPath( DcPackageIndex::DcPackageDesc &desc,QString path )
    {
        return QDir::toNativeSeparators(_rootPath + desc.id + "/" + path); 
    }

    // Download the "package" contents from the remote server.
    bool downloadPackage( DcPackageIndex::DcPackageDesc desc);
    
    bool getAndVerifyProgressFilePath(DcPackageIndex::DcPackageDesc& desc,QString& progFilePath);

    // Returns true if a valid progress file was found.  If a no file is available for update,
    // nextFile will be empty.  This indicates that all firmware assets have been programmed.
    bool selectNextFwAssetForUpdate(DcPackageIndex::DcPackageDesc desc,QString& nextFile);

    // Return true if a valid progress file was found and accessible.  chgLogFile is
    // updated with the change log file path, or it's returned empty.
    bool getChangelog(DcPackageIndex::DcPackageDesc desc,QString& chgLogFile);
    // Returns true if the 'completedFile' was removed from the progress file,
    // false indicates an error with the progress file or the file was not found.
    bool markFileComplete(DcPackageIndex::DcPackageDesc desc,QString& completedFile);
    
    // Return true if the file name contains a "firmware" file
    bool isFirmwareFile( QString& nextFile );
    
    // Return true if the file name indicates a preset. For example, files
    // names as "bob_up.syx" are user preset files.  Other examples: "0123_0102_fp.syx" - this
    // would indicate a factory preset
    bool isPresetFile( QString& nextFile );

    // Download the file from the given package that contains the 
    QString downloadFileWithPartialString( DcPackageIndex::DcPackageDesc desc,QString partialString);
   
    // Returns true if the URL file is valid, and if valid, the url contained in the file
    // is placed in newUrl
    bool checkUrlFile(QString fname,QString& newUrl);
    
    // Return digest for given key and string
    QByteArray makeDigest(QByteArray key, QByteArray str);

    
private:
    DcPackageIndex _swPackageIndex;
    QString _rootPath;
    QString _idxUrl;
    QString _idxFilePath;

    QString _lastResultString;
    QTextStream _lastResult;
};