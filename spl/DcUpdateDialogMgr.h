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
 \file DcUpdateDialogMgr.h
 \brief Top level class that manages the software update process.
--------------------------------------------------------------------------*/
#ifndef DCUPDATEDIALOGMGR_H
#define DCUPDATEDIALOGMGR_H

#include "DcBootControl.h"
#include "DcDeviceDetails.h"
#include "DcSoftwareUpdate.h"

#include <QDialog>
#include <QListWidget>
#include <QTextStream>
#include "IoProgressDialog.h"

class DcUpdateDialogMgr
{

public:

    enum DcUpdate_Result {
        DcUpdate_Success,
        DcUpdate_SuccessNoUpdate,
        DcUpdate_Cancled,
        DcUpdate_Failed,
        DcUpdate_PatchFileUpdateFailure,
        DcUpdate_PresetUpdateCancled,
        DcUpdate_PresetUpdateFailure,
        DcUpdate_Unknown
    };

     DcUpdateDialogMgr(QString updatePath, DcBootControl* bctl, DcDeviceDetails& details, QWidget *parent = 0);

     ~DcUpdateDialogMgr();

    DcUpdate_Result getLatestAndShowDialog();

    QString updatesPath() const;

    void setUpdatesPath(const QString &updatesPath);
    
    bool installUpdate(DcSoftwareUpdate &pm,bool okToInstallPresets = false);

    bool updateFirmware(QString fileName);

    bool loadSysexFile(const QString &fileName, QList<QRtMidiData> &dataList, QList<QRtMidiData> *pRejectDataList = 0);

    QString getLastErrorMsg();

private:

    QWidget *_parent;
    
    QString _updatesPath; // Path in local files system

    QString _lastErrorMsgStr;

    DcBootControl *_bootCtl;
    DcDeviceDetails _devDetails;

    DcUpdateWorklist _updateWl;
    IoProgressDialog* _progressDialog; 
    DcUpdate_Result _installUpdateResult;
    QString _urlFileName;
};

#endif // DCUPDATEDIALOG_H
