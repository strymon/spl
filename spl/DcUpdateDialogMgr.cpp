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
 \file DcUpdateDialogMgr.cpp
--------------------------------------------------------------------------*/
#include "DcUpdateDialogMgr.h"

#include <QtWidgets>
#include "DcFileDownloader.h"
#include "DcSoftwareUpdate.h"
#include "DcUpdateAvailableDialog.h"
#include "DcBootControl.h"
#include "DcQUtils.h"
#include "DcLog.h"

DcUpdateDialogMgr::DcUpdateDialogMgr(QString localUpdatePath, DcBootControl* bctl, DcDeviceDetails &details, QWidget *parent /*=0*/)
{
    _parent = parent;
    _bootCtl = bctl;
    _devDetails = details;
    _updatesPath = localUpdatePath;
}

DcUpdateDialogMgr::~DcUpdateDialogMgr()
{
    if(_progressDialog)
    {
        _progressDialog->hide();
    }
}


DcUpdateDialogMgr::DcUpdate_Result DcUpdateDialogMgr::getLatestAndShowDialog()
{
    DcUpdate_Result rtval = DcUpdate_Unknown;
    
    DcSoftwareUpdate pm;

    _progressDialog = new IoProgressDialog(_parent);
    _progressDialog->setFormat("%p%");
    _progressDialog->reset();
    _progressDialog->setModal(true);
    _progressDialog->setMessage("Checking for updates...");
    _progressDialog->show();
    QApplication::processEvents();



    if(pm.init(_updatesPath,DcSoftwareUpdate::kIndexFileUrl))
    {
            QString newVer;

            DcUpdateAvailableDialog* ud = new DcUpdateAvailableDialog(_parent);
            ud->setCurrent(_devDetails.FwVersion);
            
            _progressDialog->reset();
            _progressDialog->setModal(true);
            _progressDialog->setMessage("Downloading update data...");
            _progressDialog->show();
            QApplication::processEvents();

            if(pm.prepForUpdateToLatest(_devDetails,_updateWl))
            {
                // Check for new firmware
                if(!_updateWl.fwFile.isEmpty())
                {
                    DCLOG() << "New Firmware Available";
                    DCLOG() << "Your Version : " << _devDetails.FwVersion;
                    DCLOG() << "Latest Version: " << _updateWl.info.getVersionString();

                    
                    ud->setLatest(_updateWl.info.getVersionString(),_devDetails.DeviceIconResPath,false/*_updateWl.hasPresets()*/);
                    ud->setChangeLogUrl(_updateWl.changeLogUrl);

                    _progressDialog->reset();
                    _progressDialog->hide();
                    QApplication::processEvents();

                    if(ud->exec())
                    {
                        DCLOG() << "User clicked install";
                        _progressDialog->setMessage(QLatin1String("Preparing Update"));
                        _progressDialog->show();

                        QApplication::processEvents();
                        _installUpdateResult = DcUpdate_Unknown;
                        if(installUpdate(pm,ud->okToInstallPresets()))
                        {
                            DCLOG() << QLatin1String("update completed successfully");
                            rtval = DcUpdate_Success;
                        }
                        else
                        {
                            DCWARN() << "There was a problem during the update";
                            rtval = _installUpdateResult;
                        }
                    }
                    else
                    {
                        DCWARN() << "User cancled update";
                        rtval = DcUpdate_SuccessNoUpdate;
                    }

                }
                else
                {
                    _progressDialog->hide();

                    // The firmware will not need updating.
                    DCLOG() << "Success, no updates are available, device has version " << _devDetails.FwVersion;
                    ud->exec();
                    rtval = DcUpdate_SuccessNoUpdate;
                }
            }
            else
            {
                _progressDialog->hide();
                DCLOG() << pm.getLastResult();
                rtval = DcUpdate_Failed;
            }


        }
        else
        {
            _progressDialog->show();
            _lastErrorMsgStr = "<h2>Error Obtaining Update Information</h2>";
            DCLOG() << pm.getLastResult();
            DCLOG() << "Current Version: " << _devDetails.FwVersion;
            rtval = DcUpdate_Failed;
        }

    return rtval;
}


QString DcUpdateDialogMgr::updatesPath() const
{
    return _updatesPath;
}

void DcUpdateDialogMgr::setUpdatesPath(const QString &updatesPath)
{
    _updatesPath = updatesPath;
}


bool DcUpdateDialogMgr::installUpdate( DcSoftwareUpdate &pm,bool okToInstallPresets /*= false*/ )
{
    Q_UNUSED(pm);
        
    bool rtval = false;

    // This method assumes that _updateWl has been initialized, if it has not, 
    // the method will not attempt an update.
    if(_updateWl.isValid())
    {
        if(!updateFirmware(_updateWl.fwFile))
        {
            DCLOG() << "Updated failed: " << _lastErrorMsgStr;
        }
        else
        {
            // Firmware update complete, if the "update work list" has other sysex files, apply these files.
            QList<QRtMidiData> sysexList;

            bool bailError = false;


#ifdef DC_UDM_SUPPORT_OTHER_SYSEX
// When enabled, "other" sysex files that are not firmware or factor/user presets can be written to the attached device.

            int patchCount = _updateWl.otherSysexFileList.count();
            // Send any other sysex file to the device if needed
            foreach(QString f, _updateWl.otherSysexFileList)
            {
                _progressDialog->reset();
                _progressDialog->setMessage("Loading Patch Sysex #" + QString("%1").arg(patchCount));
                QApplication::processEvents();            
                if(!loadSysexFile(f,sysexList))
                {
                     DCLOG << "failed loading sysex patch file: "  << f << " - " << _lastErrorMsgStr << "\n";
                     _installUpdateResult = DcUpdate_Cancled;
                     bailError = true;
                    break;
                }
                
                _progressDialog->reset();
                _progressDialog->setMessage("Applying Patch " + QString(" %1 out of %2").arg(patchCount++).arg(_updateWl.presetSysexFileList.count()));
                _progressDialog->setMax(sysexList.count());
                QApplication::processEvents();
                for (int idx = 0; idx < sysexList.count() ; idx++)
                {
                    _progressDialog->inc();
                    QApplication::processEvents();

                    // If some error
                    // TODO - _installUpdateResult = DcUpdate_PatchFileUpdateFailure;
                    // bailError = true;
                }
            }
#endif // DC_UDM_SUPPORT_OTHER_SYSEX
            
            // If the user has agreed and there are preset files to update
            if(!bailError && true == okToInstallPresets)
            {
                _progressDialog->show();
                int presetSet = 0;
                
                foreach(QString f, _updateWl.presetSysexFileList)
                {
                    DCLOG() << "Loading preset file " << f;
                    _progressDialog->reset();
                    _progressDialog->setMessage("Loading Preset Update #" + QString("%1").arg(presetSet));
                    
                    QApplication::processEvents();
                    if(!loadSysexFile(f,sysexList))
                    {
                        DCLOG() << "failed reading preset file: "  << f << " - " << _lastErrorMsgStr;
                        _installUpdateResult = DcUpdate_PresetUpdateFailure;
                        break;
                    }

                    _progressDialog->reset();
                    _progressDialog->setMessage("Preset Update " + QString(" %1 out of %2").arg(presetSet++).arg(_updateWl.presetSysexFileList.count()));
                    _progressDialog->setMax(sysexList.count());
                    QApplication::processEvents();
                    DCLOG() << "Programming preset data";
                    for (int idx = 0; idx < sysexList.count() ; idx++)
                    {
                    	_bootCtl->writeMidi(sysexList[idx]);
                        _progressDialog->inc();
                        if(_progressDialog->cancled())
                        {
                            DCLOG() << "Preset programming canceled";
                            _installUpdateResult = DcUpdate_PresetUpdateCancled;
                        }
                        QApplication::processEvents();
                        // TODO: on error set: _installUpdateResult = DcUpdate_PresetUpdateFailure;
                    }
                    
                }
                
                DCLOG() << "Preset update complete";
            }

            
            // End of firmware/preset update process
            rtval = true;
        }
    }
    else
    {
        // TODO: Internal error - logic error, method called with invalid argument
        DCLOG() << "Internal logic error: " << __FILE__;
    }

    return rtval;
}


bool DcUpdateDialogMgr::updateFirmware(QString fileName )
{
    // TODO: ADD PROGRESS UI IN THIS METHOD

    bool rtval = false;
    _lastErrorMsgStr.clear();
    _installUpdateResult = DcUpdate_Failed;

    if(QFile().exists(fileName))
    {
        QFile file(fileName);

        if (!file.open(QIODevice::ReadOnly))
        {
            _lastErrorMsgStr = "Unable to open the file: " + fileName;
            return false;
        }
        QList<QRtMidiData> sysexList;
        QList<QRtMidiData> filterList;

        // MIDI messages that are not used by the fw update code are added to this list.
        filterList << "F0 00 01 55 42 11 F7" << "F0 00 01 55 42 01 F7";

        if(loadSysexFile(fileName,sysexList,&filterList))
        {
            // Verify that the file contains firmware for the current device
            if(!sysexList.at(0).contains(_devDetails.SOXHdr))
            {
                // Wrong product id
                DCLOG() << "firmware file does not contain firmware for this device";
                _lastErrorMsgStr = "Internal update error - file does not contain firmware for current device";
                rtval = false;
            }
            else
            {
                // Remove the reset command (that contained the product id that was just verified)
                sysexList.removeFirst();
                
                _progressDialog->reset();
                _progressDialog->setNoCancel(true);
                _progressDialog->setMessage("Enabling Update Mode");

                QApplication::processEvents();

                if(false == _bootCtl->enableBootcode())
                {
                    _lastErrorMsgStr = "Failed to enable Boot-code\n";
                    rtval = false;
                }
                else
                {
                    rtval = true;
                    quint64 start_time = QDateTime::currentMSecsSinceEpoch();

                    int rptInterval = 20; // 20%
                    int rptEveryVal = sysexList.count()/rptInterval;

                    int chunkcnt = 0;
                    int cnt = 0;
                    _progressDialog->reset();
                    _progressDialog->setMax(sysexList.count());
                    _progressDialog->setMessage("Loading New Firmware");
                    QApplication::processEvents();
                    
                    DCLOG() << "Starting the update process";
                    int retryCnt = 3;
                    int totalWriteErrorCount = 0;

                    // The firmware is contained in a list of QRTMidiData objects, each object is a
                    // sysex message that shall be sent to device.
                    foreach(QRtMidiData md, sysexList)
                    {
                        bool writeStatus = false;
                        
                        while(retryCnt-- > 0 && !writeStatus )
                        {
                            writeStatus = _bootCtl->writeFirmwareUpdateMsg(md);
                            
                            if(writeStatus)
                            {
                                break;
                            }
                            else
                            {
                                totalWriteErrorCount++;
                                DCLOG() << "Firmware Write Failure" << ((retryCnt > 0) ? " Trying again" : "Giving Up");
                            }
                        }

                        if(!writeStatus)
                        {
                            DCLOG() << "Aborting Firmware Update - failures due to writeFirmwareUpdateMsg()";
                            DCLOG() << "BootCtrl Last Error = " << _bootCtl->getLastError(); 
                            _lastErrorMsgStr = "<h2>Firmware Updated Failed</h2>The system had trouble transferring data.";
                            rtval = false;
                            break;
                        }
                        else
                        {
                            _progressDialog->inc();
                            QApplication::processEvents();
                        
                            if(_progressDialog->cancled())
                            {
                                // Canceling update
                                rtval = false;
                                _lastErrorMsgStr = "Firmware update was cancled";
                                DCLOG() << _lastErrorMsgStr;
                                _installUpdateResult = DcUpdate_Cancled;
                                break;
                            }

                            // Progress Report
                            if(++cnt % rptEveryVal == 0)
                            {
                                chunkcnt++;
                                DCLOG() << ( chunkcnt * (100/rptInterval)) << "% complete";
                            }
                            retryCnt = 3;
                        }
                    }
                    
                    quint64 updateDurationMs = QDateTime::currentMSecsSinceEpoch() - start_time;
                    DCLOG() << "Update completed with " << totalWriteErrorCount << " write errors";
                    DCLOG() << "Reseting Device after " << (double)updateDurationMs/1000.0 << " seconds";

                    _progressDialog->reset();
                    _progressDialog->setNoCancel(true);
                    _progressDialog->setMessage("Device Reset In Progress...");
                    _progressDialog->show();
                    QApplication::processEvents();

                    bool exitBootResult = _bootCtl->exitBoot();
                    _progressDialog->hide();
                    QApplication::processEvents();
                    
                    // Command device to exit boot code and then wait
                    if(rtval && !exitBootResult)
                    {
                        rtval = false;
                        _lastErrorMsgStr = "Failed to exit boot code after firmware update\n";
                    }
                }

            }

        }
        file.close();
    }
    else
    {
        _lastErrorMsgStr = fileName + " dose not exists\n";
    }

    return rtval;
}
bool DcUpdateDialogMgr::loadSysexFile( const QString &fileName,QList<QRtMidiData>& dataList, QList<QRtMidiData>* pRejectDataList /* = 0 */)
{
    bool result = true;
    _lastErrorMsgStr.clear();

    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly))
    {
        _lastErrorMsgStr = "Unable to open the file: " + fileName;
        return false;
    }

    QDataStream in(&file);

    // Load one preset in, see if it matches the current Manufacture
    unsigned char byte;
    QRtMidiData md;
    bool lookForStatusByte = true;

    while(1 == in.readRawData((char*)&byte,1))
    {
        if(lookForStatusByte)
        {
            if(byte == 0xF0)
            {
                md.clear();
                md.appendNum(byte);
                lookForStatusByte = false;
            }
        }
        else
        {
            if(byte == 0xF7)
            {
                md.appendNum(byte);

                if(0 == pRejectDataList)
                {
                    dataList.append(md);
                }
                else
                {
                    bool reject = false;
                    // Apply the filter
                    foreach(QRtMidiData rejectMd,*pRejectDataList)
                    {
                        if(md.contains(rejectMd))
                        {
                            reject = true;
                            break;
                        }
                    }

                    if(!reject)
                    {
                        dataList.append(md);
                    }
                }

                lookForStatusByte = true;
            }
            else if(byte == 0xF0)
            {
                // Terminate the MIDI stream
                md.appendNum(0xF7);

                if(0 == pRejectDataList)
                {
                    dataList.append(md);
                }
                else if(!pRejectDataList->contains(md))
                {
                    dataList.append(md);
                }

                md.clear();
                md.appendNum(0xF0);
            }
            else
            {
                md.appendNum(byte);
            }
        }
    }

    if(dataList.count() == 0)
    {
        _lastErrorMsgStr = fileName + " did not contain any MIDI data\n";
        result = false;
    }

    file.close();

    return result ;
}

//-------------------------------------------------------------------------
QString DcUpdateDialogMgr::getLastErrorMsg()
{
    return _lastErrorMsgStr;
}
