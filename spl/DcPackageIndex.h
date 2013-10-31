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
 \file DcPackageIndex.h
 \brief A package index is a plain text file formated as a QSettings ini
 file.  The file lists devices firmware using "short" MIDI product (pid) and 
 family ID (fid). Short, as in 8 bit pid and fid, not 16 bit as in the spec.
 The index file lists fw versions available and calls out other assets 
 needed for an update.
--------------------------------------------------------------------------*/
#pragma once
#include <QString>
#include <QTextStream>

#include <QDir>
#include <QSettings>
#include <QCryptographicHash>

typedef QString DcPackageId;
typedef QString DcVersionStr;
typedef QString DcProductIdStr;

/*!
	A class for managing update package version strings
*/
struct DcVersionValue
{
    bool fromString(DcVersionStr version)
    {
        bool rtval = false;
        if(version.count() >= 4)
        {
            beta = (version[0] != '0');
            version[0] = '0';
            iver = version.toInt(&rtval,10);
        }
        return rtval;
    }
    
    bool isValid() { return iver !=0;}
    bool isEmpty() { return iver ==0;}
    void clear() { iver = 0; beta = false;}
    
    QString toString() 
    {
        QString rtval;
        rtval.sprintf("%d,%d",iver,beta?1:0);
        return rtval;
    }
    
    bool beta;
    int iver;
    
};

/*!
	A class for managing update packages
*/
class DcPackageIndex 
{
    static const char* kNullId;

public:

    DcPackageIndex() 
    {
        _idxFile = 0;
        _lastError.setString(&_lastErrorMsgStr);

    };

    ~DcPackageIndex() 
    {
        uninit();
    };

    struct DcPackageDesc
    {
        DcPackageDesc() 
        {
            clear();
        }

        void clear()
        {
            value.clear();
            id = QLatin1String(kNullId);
            devuuid.clear();
        }

        bool isEmpty()
        {
            return value.isEmpty() || id.isEmpty();
        }
        
        QString toHash()
        {
            QCryptographicHash md5(QCryptographicHash::Md5);
            QByteArray ba;
            
            ba.append(value.toString());
            ba.append(id);
            ba.append(devuuid);
            md5.addData(ba);

            return md5.result().toHex();
        }
        
        QString getVersionString()
        {
            QString ver = id.mid(5,4);
            ver.insert(1,'.');
            ver.insert(3,'.');
            ver.insert(5,'.');
            return ver;
        }

        DcVersionValue value;
        DcPackageId id;
        QString devuuid;
    };

    /*!
        Attempts to load, the verifies the signature of the specified the index file.
        This method can be called more than once.
    */
    bool init(QString idxFilePath);
    
    /*!
    	Release resources
    */
    void uninit();

    /*!
    	Returns a list of the product ids contained in this update index
    */
    bool getProductIds(QStringList& lst);
    
    /*!
    	Returns the list of versions for the given product id.
    */
    bool getVersions(DcProductIdStr pid,QStringList& lst);
    
    /*!
    	Returns the DcUpdateId for the given product id and version number.
    */
    inline static DcPackageId  pidAndVerToUpdateId(DcProductIdStr pid, DcVersionStr version) {return pid + "/" + version;}
    
    /*!
	    Populates the DcPackageDesc 'latest' if a newer software version is found, otherwise latest is returned empty.
	    If useBeta is set, then beta version are also considered.
    */
    bool findNewer( DcProductIdStr pid, DcVersionStr version, bool useBeta, DcPackageDesc& latest);

    
    /*!
    	If possible, return a list of file names, otherwise an empty string.
    */
    QStringList getFileList(DcPackageDesc& desc);

    /*!
    	If possible, return the change log path or an empty string.
    */
    QString getChangeLog( DcPackageDesc& desc );
    
    /*!
    	If possible, return the base url for the files in the specified update, otherwise an empty string.
    */
    QString getBaseUrl(DcPackageDesc& desc);
    
    /*!
    	If possible, return the change log text for the given id, otherwise an empty string.
    */
    QString     getLogText(DcPackageDesc& desc);


    /*!
    	Returns true if the object is initialized
    */
    bool isOk() { return 0 != _idxFile; }
    
    /*!
    	If a method returns false, use this method to find out why
    */
    inline QString getLastError() const { return _lastErrorMsgStr; }
    
    /*!
    	Clear the last error message
    */
    void clearLastError() { _lastErrorMsgStr.clear(); }
    
    /*!
        Returns true if the index contains the given id
    */
    bool exists( DcPackageId id ) { return isOk() ? _idxFile->contains(id) : false;}

    /*!
	    Returns the latest version for the given pid.  If useBeta is set
	    the beta versions are also considered when finding the latest.
    */
    bool getLatest(DcProductIdStr pid,DcPackageDesc& latest,bool useBeta);

protected:
    
    bool initPackageDesc( DcVersionStr version, DcProductIdStr pid,DcPackageDesc& desc);
   


private:

    QSettings* _idxFile;
    
    QTextStream _lastError;
    QString _lastErrorMsgStr;


};


