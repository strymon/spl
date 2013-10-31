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
 \file DcPackageIndex.cpp
--------------------------------------------------------------------------*/
#include "DcPackageIndex.h"
#include <QSettings>
#include "DcLog.h"

const char* DcPackageIndex::kNullId = "0";

//-------------------------------------------------------------------------
bool DcPackageIndex::init(QString idxPath)
{
    clearLastError();

    uninit();

    if(!QFile::exists(idxPath))
    {
        _lastError << "index file was not found: " << idxPath;        
        DCERROR() << getLastError();
        return false;
    }

    _idxFile = new QSettings(idxPath,QSettings::IniFormat);

    if(!_idxFile)
    {
        _lastError << "unable to create QSettings file from: " << idxPath;
        DCERROR() << getLastError();
        return false;
    }

    QString signature = _idxFile->value("sig",QVariant("0")).toString();
    
    // Check the file signature
    if(signature != "31.25")
    {
        _lastError << "index file signature file failure: wanted \"31.25\", got: " << signature;
        DCERROR() << getLastError();
        uninit();
    }

    return isOk();
}

//-------------------------------------------------------------------------
void DcPackageIndex::uninit()
{
    if(_idxFile)
    {
        delete _idxFile;
        _idxFile = 0;
    }
}

//-------------------------------------------------------------------------
bool DcPackageIndex::getProductIds(QStringList& lst)
{
    clearLastError();

    lst.clear();

    if(isOk())
    {
        lst = _idxFile->childGroups();
    }
    else
    {
        _lastError << "error - object not initialized";
        DCERROR() << getLastError();
        return false;
    }
    
    if(lst.isEmpty())
    {
        _lastError << "product list is empty";
        DCERROR() << getLastError();
        return false;
    }

    return true;
}

//-------------------------------------------------------------------------
bool DcPackageIndex::getVersions( DcProductIdStr pid,QStringList& lst )
{
    clearLastError();

    lst.clear();

    if(!isOk())
    {
        _lastError << "error - object not initialized";
        DCERROR() << getLastError();
        return false;
    }

    if(!_idxFile->childGroups().contains(pid))
    {
        _lastError << "Specified product id (" << pid << ") was not found";
        DCERROR() << getLastError();
        return false;
    }

    _idxFile->beginGroup(pid);
    lst = _idxFile->childGroups();
    _idxFile->endGroup();

    if(lst.isEmpty())
    {
        _lastError << "Version list is empty for product: " << pid;
        DCERROR() << getLastError();
        return false;
    }

    return true;
}



//-------------------------------------------------------------------------
bool DcPackageIndex::initPackageDesc( DcVersionStr version, DcProductIdStr pid, DcPackageDesc& desc )
{
    bool rtval = false;
    desc.clear();

    if(!isOk())
    {
        _lastError  << " error - object not initialized";
        DCLOG() << getLastError();
    }
    else if(version.isEmpty() || version.count() >= 4)
    {
        bool ok = desc.value.fromString(version);
        if(!ok)
        {
            _lastError << " - can't convert version to integer: " << version;
            DCLOG() << getLastError();
        }
        else
        {
            desc.id = pidAndVerToUpdateId(pid,version);
            rtval = true;
        }
    }
    else
    {
        _lastError << " - format of 'version' is incorrect: " << version;
        DCLOG() << getLastError();
    }
    
    return rtval;
}

bool DcPackageIndex::getLatest(DcProductIdStr pid, DcPackageDesc& latest,bool useBeta)
{
    bool ok;
    bool rtval = false;
    latest.clear();
    
    if(!isOk())
    {
        _lastError << " error - object not initialized";
        DCLOG() << getLastError();
    }
    else
    {
        int latestRelease = _idxFile->value(pid + "/latest").toInt(&ok);
        if(!ok && !useBeta)
        {
            _lastError << " error - product was not found";
            DCLOG() << getLastError();
        }
        else if(useBeta)
        {
            int latestBeta = _idxFile->value(pid + "/beta/latest").toInt(&ok);
            if(!ok && (0 == latestRelease))
            {
                _lastError << " error - product was not found";
                DCLOG() << getLastError();
            }

            if(latestRelease >= latestBeta)
            {
                latest.value.iver    = latestRelease;
                latest.value.beta     = false;
                latest.id       = pidAndVerToUpdateId(pid,_idxFile->value(pid + "/latest").toString());
            }
            else
            {
                latest.value.iver = latestBeta;
                latest.value.beta = true;
                latest.id       = pidAndVerToUpdateId(pid,_idxFile->value(pid + "/beta/latest").toString());
            }
            rtval = true;
        }
        else
        {
            latest.value.iver = latestRelease;
            latest.value.beta = false;
            latest.id       = pidAndVerToUpdateId(pid,_idxFile->value(pid + "/latest").toString());
            rtval = true;
        }
    }

    return rtval;
}

//-------------------------------------------------------------------------
bool DcPackageIndex::findNewer( DcProductIdStr pid, DcVersionStr version, bool useBeta, DcPackageDesc& latest)
{
    bool rtval = false;
    
    latest.clear();
    
    bool foundNewVersion = false;
   
    // Version could be in the form of 0.2.3.4
    // The format in the index is 0234
    version.replace(".","");

    rtval = getLatest(pid,latest,useBeta);

    if(rtval && !latest.isEmpty())
    {
        DcVersionValue cur;
        cur.fromString(version);
        if(!cur.isEmpty())
        {
            if(cur.iver == latest.value.iver && cur.beta)
            {
                foundNewVersion = true;
            }

            else if(cur.iver < latest.value.iver)
            {
                foundNewVersion = true;
            }

            rtval = true;
        }
        else
        {
             _lastError << " error - failed to convert version to a value: " << version;
             DCLOG() << getLastError();
        }
    }

    if(!foundNewVersion)
    {
        latest.clear();
    }

    return rtval;
}

//-------------------------------------------------------------------------
QStringList DcPackageIndex::getFileList( DcPackageDesc& desc )
{
    QStringList lst;

    if(!isOk())
    {
        _lastError << " error - object not initialized";
        DCLOG() << getLastError();
    }
    else
    {
        _idxFile->beginGroup(desc.id + "/files");
        QStringList keys = _idxFile->childKeys();
        foreach(QString k, keys)
        {
            lst << _idxFile->value(k).toString();        
        }
        _idxFile->endGroup();
    }
    
    return lst;
}

//-------------------------------------------------------------------------
QString DcPackageIndex::getChangeLog( DcPackageDesc& desc )
{
    QString rtval;

    if(!isOk())
    {
        _lastError << " error - object not initialized";
        DCLOG() << getLastError();
    }
    else
    {
        // Look through the file and find the change log '.*_cl.txt'
        _idxFile->beginGroup(desc.id + "/files");
        QStringList keys = _idxFile->childKeys();
        foreach(QString k, keys)
        {
            
            if(_idxFile->value(k).toString().contains("cl.txt"))
            {
                rtval = _idxFile->value(k).toString();
                break;
            }
        }
        _idxFile->endGroup();
    }

    return rtval;
}
//-------------------------------------------------------------------------
QString DcPackageIndex::getBaseUrl( DcPackageDesc& desc )
{
    QString rtval;

    _idxFile->beginGroup(desc.id);
    rtval = _idxFile->value("baseurl").toString();
    _idxFile->endGroup();
    return rtval;
}
