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
 \file DcSoftwareUpdate.cpp
 --------------------------------------------------------------------------*/
#include "DcSoftwareUpdate.h"
#include "DcFileDownloader.h"
#include "DcDeviceDetails.h"
#include <QSettings>
#include <QRegExp>

// Test URL: "http://sarris/foo.html"; // 
const char* DcSoftwareUpdate::kIndexFileUrl = "http://s3-us-west-1.amazonaws.com/strymon/pkgs/idx.ini"; // TODO: use a virtual host
const char* DcSoftwareUpdate::kIndexFileName = "idx.ini";


//-------------------------------------------------------------------------
bool DcSoftwareUpdate::verifyLocalPath(DcPackageIndex::DcPackageDesc desc)
{
    QString destroot = toLocalPath(desc);

    QDir().mkpath(destroot);

    return QDir().exists(destroot);
}

//-------------------------------------------------------------------------
bool DcSoftwareUpdate::downloadPackage( DcPackageIndex::DcPackageDesc desc)
{
    // TODO: Add progress reporting to this method via some optional method.
    
    // The method relies on a remote web server so there are many reasons for
    // failure.  TODO: provide a method to report failures.

    
    bool rtval = false;

    if(!desc.isEmpty())
    {
        if(verifyLocalPath(desc))
        {
            QString baseUrl = _swPackageIndex.getBaseUrl(desc);
            QStringList lst = _swPackageIndex.getFileList(desc);
            foreach(QString f, lst)
            {
                QString dest = appendWithLocalPath(desc,f);
                if(downloadFile(baseUrl+f,dest))
                {
                   rtval = true;     
                }
            }
        }
    }
    return rtval;
}

QString DcSoftwareUpdate::downloadFileWithPartialString( DcPackageIndex::DcPackageDesc desc,QString partialString )
{
    
    QString rtval = "";

    if(!desc.isEmpty() && !partialString.isEmpty())
    {
        if(verifyLocalPath(desc))
        {
            QString baseUrl = _swPackageIndex.getBaseUrl(desc);
            QStringList lst = _swPackageIndex.getFileList(desc);
            foreach(QString f, lst)
            {
                if(f.contains(partialString))
                {
                    QString dest = appendWithLocalPath(desc,f);
                    if(downloadFile(baseUrl+f,dest))
                    {
                        rtval = dest;     
                        break;
                    }
                }
            }
        }
    }
    return rtval;
}

//-------------------------------------------------------------------------
bool DcSoftwareUpdate::downloadFile( QString srcUrl, QString destPath )
{
    // TODO: Broadcast progress
    QUrl url(srcUrl);
    DcFileDownloader* dl = new DcFileDownloader(url, this);

    // TODO: how to check for errors?
    if(dl)
    {
        dl->saveData(destPath);
        delete dl;
        return true;
    }
    else
    {
        _lastResult << "Unable to download file\n" << "From: " << srcUrl;
    }
    return false;
}

bool DcSoftwareUpdate::getLatestInfo( DcDeviceDetails& dev,QString& verStr, QStringList& changeLog)
{
    DcPackageIndex::DcPackageDesc newPackageDesc;
    bool rtval = false;
    if( _swPackageIndex.findNewer(dev.getShortId(),dev.FwVersion,true,newPackageDesc) )
    {
        if(!newPackageDesc.isEmpty())
        {
            verStr = newPackageDesc.getVersionString();
            QString fullPath = downloadFileWithPartialString(newPackageDesc,"cl.txt");
            if(!fullPath.isEmpty())
            {
                QFile cl(fullPath);
                if(cl.open(QIODevice::Text | QIODevice::ReadOnly) )
                {
                    while (!cl.atEnd()) 
                    {
                        QByteArray line = cl.readLine();
                        changeLog << line;
                    }
                }
            }

            rtval = true;
        }
        else
        {
            _lastResult << "Nothing newer was found";
        }
    }
    else
    {
       _lastResult << "Error getting latest info: " << _swPackageIndex.getLastError();
    }
    return rtval;
}


//-------------------------------------------------------------------------
bool DcSoftwareUpdate::prepForUpdateToLatest( DcDeviceDetails& devDetails,DcUpdateWorklist& updateWorkList)
{
    bool rtval = true;

    DcPackageIndex::DcPackageDesc newPackageDesc;
    
    if(devDetails.isEmpty())
    {
        return false;
    }

    if(_swPackageIndex.findNewer(devDetails.getShortId(),devDetails.FwVersion,true,newPackageDesc))
    {
        if(!newPackageDesc.isEmpty())
        {
            if(downloadPackage(newPackageDesc))
            {
                if(createUpdateProgressTrackingFile(newPackageDesc))
                {
                    QString nextFile;
                    do 
                    {
                        if(selectNextFwAssetForUpdate(newPackageDesc,nextFile))
                        {
                            if(!nextFile.isEmpty())
                            {
                                if(isFirmwareFile(nextFile))
                                {
                                    updateWorkList.fwFile  = nextFile;
                                }
                                else if(isPresetFile(nextFile))
                                {
                                    updateWorkList.presetSysexFileList << nextFile;
                                }
                                else
                                {
                                    updateWorkList.otherSysexFileList << nextFile;
                                }
                                
                                // Success, remove file
                                markFileComplete(newPackageDesc,nextFile);
                            }

                        }
                        else
                        {
                            rtval = false;
                            // TODO: log error
                            break;
                            
                        }
                    } while (!nextFile.isEmpty());

                    if(rtval)
                    {
                        if(getChangelog(newPackageDesc,nextFile))
                        {
                            updateWorkList.changeLog = nextFile;
                            updateWorkList.changeLogUrl = _swPackageIndex.getBaseUrl(newPackageDesc) + 
                                QFileInfo(nextFile).baseName() + ".html";
                        }
                        
                        // Update the changeLogUrl - this one lives on the server
                        ;

                        updateWorkList.info = newPackageDesc;
                    }
                }
                else
                {
                    // TODO: Log error - 
                    rtval = false;
                }
           }
            else
            {
                // TODO: Log error - failed to download package files
                rtval = false;
            }
        }
        else
        {
            // Nothing to update
            rtval = true;
        }
    }
    else
    {
        // TODO: get error from index object
        rtval = false;
    }
    
    return rtval;
}

//-------------------------------------------------------------------------
bool DcSoftwareUpdate::init( QString localPath, QString idxUrl )
{
    uninit();

    if(!QFile::exists(localPath))
    {
        _lastResult << "The path used to store updates was not found\n" << "Path: " << localPath;
        return false;
    }

    _rootPath = localPath;
    _idxUrl = idxUrl;
    _idxFilePath = QDir::toNativeSeparators(_rootPath + kIndexFileName);

    try
    {
	    if(downloadFile(idxUrl,_idxFilePath))
	    {
	        if(_swPackageIndex.init(_idxFilePath))
	        {
	            return true;
	        }
            else
            {
                _lastResult << "PackageIndex Error: "  << _swPackageIndex.getLastError();
            }
	    }
    }
    catch (...)
    {
        _lastResult << "Network Exception";
    }

    return false;
}

//-------------------------------------------------------------------------
bool DcSoftwareUpdate::checkLocalPath( DcPackageIndex::DcPackageDesc desc )
{
    QString destroot = toLocalPath(desc);
    return QDir().exists(destroot);
}

//-------------------------------------------------------------------------
bool DcSoftwareUpdate::prepProgressFile(DcPackageIndex::DcPackageDesc& desc,QString& progFilePath)
{
    bool rtval = true;
    if(!checkLocalPath(desc))
    {
       // "package path does not exist"
        rtval = false;
    }
    else
    {
        progFilePath             = progFileName(desc);
        QString prevProgFilePath = prevProgFileName(desc);
        if(QFile::exists(progFilePath))
        {
            if(QFile::exists(prevProgFilePath))
            {
                if(QFile::remove(prevProgFilePath))
                {
                    rtval = true;
                }
                else
                {
                    // TODO: LOG "Unable to remove/write the update progress file: " + progFilePath;
                    rtval = false;
                }
            }
            
            if(rtval && QFile::copy(progFilePath,prevProgFilePath))
            {
                if(QFile::remove(progFilePath))
                {
                    rtval = true;
                }
                else
                {
                    // TODO: LOG "Unable to remove/write the update progress file: " + progFilePath;
                    rtval = false;
                }
            }
            else
            {
                // TODO: LOG "Unable to copy update progress file to : " + _rootPath
                rtval = false;
            }
        }
        else
        {
           try
           {
	           QSettings testfile(progFilePath,QSettings::IniFormat);
	           testfile.setValue("testvalue",0x0143);
               testfile.sync();
               if(QFile::exists(progFilePath))
               {
                    if(!QFile::remove(progFilePath))
                    {
                        // TODO: LOG " Unable to remove update file"
                        rtval = false;
                    }
                    else
                    {
                        rtval = true;
                    }
               }
           }
           catch (...)
           {
               rtval = false;	
               // TODO: LOG "Unable to create update progress file
           }
        }

    }

    if(!rtval)
    {
        // Could not write the update progress file, collect some information about the problem
        // TODO: Log this stuff
/*
        
        QFileInfo info(progFilePath);
        
        QString owner = info.owner();
        int perm = info.permissions();
        int sz = info.size();
        bool readable = info.isReadable();
        bool writeable = info.isWritable();
        QString created = info.created().toString(Qt::TextDate);
*/
    }
    return rtval;
}

//-------------------------------------------------------------------------
bool DcSoftwareUpdate::createUpdateProgressTrackingFile(DcPackageIndex::DcPackageDesc desc)
{
    bool rtval = false;

    if(!desc.isEmpty())
    {
        QString fpath;

        if(prepProgressFile(desc,fpath))
        {
            QSettings usfile(fpath,QSettings::IniFormat);
            usfile.beginGroup("info");
            usfile.setValue("creation_date",QDate::currentDate().toString());
            usfile.setValue("desc_beta",desc.value.beta);
            usfile.setValue("desc_iver",desc.value.iver);
            usfile.setValue("desc_id",desc.id);
            usfile.setValue("desc_hash",desc.toHash());
            QString baseUrl = _swPackageIndex.getBaseUrl(desc);
            usfile.setValue("url",baseUrl);
            QString changeLog = _swPackageIndex.getChangeLog(desc);
            usfile.setValue("cl",changeLog);
            usfile.endGroup();

            QStringList lst = _swPackageIndex.getFileList(desc);
            int cnt = 0;
            usfile.beginWriteArray("files");
            foreach(QString f, lst)
            {
                if(f.contains("cl.txt"))
                    continue;

                usfile.setArrayIndex(cnt++);
                usfile.setValue("path", appendWithLocalPath(desc,f));
            }
            usfile.endArray();
            usfile.sync();
            rtval = true;
        }
        else
        {
            // TODO: Log failure to prepare progress file, this could indicate a permission issues,
            // or other file system problem.
        }
    }
    return rtval;
}

//-------------------------------------------------------------------------
bool DcSoftwareUpdate::getAndVerifyProgressFilePath(DcPackageIndex::DcPackageDesc& desc,QString& progFilePath)
{
    bool rtval = false;
    progFilePath = progFileName(desc);
    if(QFile::exists(progFilePath))
    {
        QSettings usfile(progFilePath,QSettings::IniFormat);
        QString hashStr = usfile.value("info/desc_hash","0").toString();
        rtval = hashStr == desc.toHash();
        if(!rtval)
        {
            // TODO: Log, package description miss match
        }
    }
    return rtval;
}

//-------------------------------------------------------------------------
bool DcSoftwareUpdate::markFileComplete(DcPackageIndex::DcPackageDesc desc,QString& completedFile)
{
    bool rtval = false;
    if(!desc.isEmpty())
    {
        QString fpath;
        if(getAndVerifyProgressFilePath(desc,fpath))
        {
            QSettings progfile(fpath,QSettings::IniFormat);
            int size = progfile.beginReadArray("files");
            for (int i = 0; i < size; ++i) 
            {
                progfile.setArrayIndex(i);
                if( completedFile == progfile.value("path").toString())
                {
                    // TODO: Log this
                    progfile.remove("path");
                    rtval = true;
                    break;
                }
            }

            if(!rtval)
            {
                // TODO: Log - completed file to remove from progress file was not found
            }
        }
    }
    return rtval;
}

bool DcSoftwareUpdate::getChangelog(DcPackageIndex::DcPackageDesc desc,QString& chgLogFile)
{
    bool rtval = false;
    chgLogFile.clear();

    if(!desc.isEmpty())
    {
        QString fpath;
        if(getAndVerifyProgressFilePath(desc,fpath))
        {
            QSettings progfile(fpath,QSettings::IniFormat);
            chgLogFile = appendWithLocalPath(desc,progfile.value("info/cl","").toString());
            rtval = true;
        }
    }
    return rtval;
}

//-------------------------------------------------------------------------
bool DcSoftwareUpdate::selectNextFwAssetForUpdate(DcPackageIndex::DcPackageDesc desc,QString& nextFile)
{
    bool rtval = false;
    nextFile.clear();

    if(!desc.isEmpty())
    {
        QString fpath;
        if(getAndVerifyProgressFilePath(desc,fpath))
        {
            rtval = true;

            QSettings progfile(fpath,QSettings::IniFormat);
            int size = progfile.beginReadArray("files");

            QString best = "";
            int bestval = 99;


            for (int i = 0; i < size; ++i) 
            {
                progfile.setArrayIndex(i);

                QString fpath = progfile.value("path").toString();
                if(fpath.contains("fw.syx"))
                {
                    best = fpath;
                    break;
                }
                else if(fpath.contains("fw1.syx"))
                {
                    best = fpath;
                    bestval = 1;
                }
                else if(fpath.contains("fw2.syx"))
                {
                    if(bestval > 2)
                    {
                        best = fpath;
                        bestval = 2;
                    }
                }
                else if(fpath.contains("fw3.syx"))
                {
                    if(bestval  > 3)
                    {
                        best=fpath;
                        bestval = 3;
                    }
                }
                else if(fpath.contains("fw4.syx"))
                {
                    if(bestval  > 4)
                    {
                        best=fpath;
                        bestval = 4;
                    }
                }
                else if(fpath.contains(".syx"))
                {
                    if(bestval >= 99)
                    {
                        best=fpath;
                    }
                }
            }
            progfile.endArray();
            nextFile = best;
        }
    }
    return rtval;
}

//-------------------------------------------------------------------------
bool DcSoftwareUpdate::isFirmwareFile( QString& nextFile )
{
    return nextFile.contains(QRegExp("fw[0-9]?.syx$"));
}

//-------------------------------------------------------------------------
void DcSoftwareUpdate::uninit()
{
    _swPackageIndex.uninit();
    _rootPath.clear();
    _idxUrl.clear();
}

//-------------------------------------------------------------------------
bool DcSoftwareUpdate::isPresetFile( QString& nextFile )
{
    return nextFile.contains(QRegExp("_[uf]p.syx$"));
}

//-------------------------------------------------------------------------
QString DcSoftwareUpdate::getLastResult()
{
    return _lastResultString;
}

//-------------------------------------------------------------------------
bool DcUpdateWorklist::hasPresets()
{
    return !presetSysexFileList.isEmpty();
}
