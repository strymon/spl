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
 \file DcBootControl.cpp
--------------------------------------------------------------------------*/
#include "DcBootControl.h"
#include <QMutexLocker>
#include <QDebug>
#include <QDateTime>
#include <QThread>
#include "QRtMidi/QRtMidiIdent.h"
#include <QApplication>

const char* DcBootControl::kPrivateResetPartial = "F0 00 01 55 vv vv 1B F7";
const char* DcBootControl::kFUGood = "F0 00 01 55 42 0C 00 F7";
const char* DcBootControl::kFUBad = "F0 00 01 55 42 0C 01 F7";
const char* DcBootControl::kFUFailed = "F0 00 01 55 42 0C 02 F7";
const char* DcBootControl::kFUResponcePattern = "F0 00 01 55 42 0C .. F7";


//-------------------------------------------------------------------------
DcBootControl::DcBootControl( QRtMidiIn& i, QRtMidiOut& o )
{
    // TODO: sharing the QRtMidiIn and QRtMidiOut objects should be managed properly 
    // and in a brainless easy manner.
    _pMidiIn = &i;
    _pMidiOut = &o;
    _lastErrorMsg.setString(&_lastErrorMsgStr);

#if defined(Q_OS_WIN32)
    _maxDataOut = 128;
#else
    _maxDataOut = 0;
#endif

    _delayBetweenDataOut = 0;
}


//-------------------------------------------------------------------------
bool DcBootControl::enableBootcode( )
{
    QRtMidiData md;
   
    QRtMidiData responceData;

    // The device is in boot mode if it responses to the Echo command.
    if(isBootcode())
    {
        return true;
    }
    
    QRtMidiData priRst = makePrivateResetCmd();
    if(0 == priRst.length())
        return false;

    QRtMidiTrigger tc(RESPONCE_ENABLE_RECOVERY_ANY);
    QRtAutoTrigger autoch(&tc,_pMidiIn);
    
    // Issue a private reset
    _pMidiOut->dataOut(priRst);

    QThread::msleep(100);
    
    // Issue "enable recovery" no more than 300ms after reset to keep the device in boot code.
    for (int idx = 0; idx < 40 ; idx++)
    {
    	_pMidiOut->dataOut(CMD_ENABLE_RECOVERY);
        QThread::msleep(20);
        if(tc.dequeue(responceData))
        {
            break;
        }
    }

    bool rtval = false;
    if(responceData.match(RESPONCE_ENABLE_RECOVERY_ACK))
    {
        // Verify device is in boot code
        if(!isBootcode())
        {
            qWarning() << "Failed to verify device is in boot code";
        }
        else
        {
            qDebug() << "Device is running boot code";
            rtval = true;
        }

    }
    else if(responceData.match(RESPONCE_ENABLE_RECOVERY_REJECTED))
    {
        qWarning() << "The device has rejected the enable recovery command";
    }
    else if(responceData.match(RESPONCE_ENABLE_RECOVERY_FAILED))
    {
        qWarning() << "Device has failed the enabled recovery command";
    }
    else
    {
        qWarning() << "Timeout entering boot code";
    }
    return rtval;
}


//-------------------------------------------------------------------------
bool DcBootControl::identify(QRtMidiDevIdent* id /*=0 */)
{
    bool rtval = false;
    QRtAutoTrigger autotc("F0 7E .. 06 02 00 01 55",_pMidiIn);

    _pMidiOut->dataOut("F0 7E 7F 06 01 F7");

    // Wait for the response data, or timeout after 300ms
    if(autotc.wait(3000))
    {
        if(id)
        {
            QRtMidiData md;
            if(autotc.dequeue(md))
            {
                id->fromIdentData(md);
                qDebug() << id->toString() << "\n";
                rtval = true;
            }
        }
        else
        {
            rtval = true;
        }
    }
    else
    {
        _lastErrorMsg << "Timeout waiting for identity response";
    }
    
    return rtval;
}

bool DcBootControl::writeMidi(QRtMidiData& msg)
{
    _pMidiOut->dataOut(msg);
    return true;
}

bool DcBootControl::writeFirmwareUpdateMsg(QRtMidiData& msg,int timeOutMs /*= 2000*/)
{
    bool rtval = false;
    QRtAutoTrigger autotc(kFUResponcePattern,_pMidiIn);

    // Magic number 8 is the response control flags, a 3 will
    // deliver status.
    msg[8] = 0x03;

    _pMidiOut->dataOutSplit(msg,_maxDataOut,_delayBetweenDataOut);

    QRtMidiData md;
    

    // Wait for the response data, or timeout after 300ms
    if(autotc.wait(timeOutMs))
    {
        if(autotc.dequeue(md))
        {
            if(md == kFUGood)
            {
                rtval =  true;
            }
            else if(md == kFUBad)
            {
                qDebug() << "kFUBad";
                _lastErrorMsg << "Device reject firmware command - BAD packet.";
            }
            else if(md == kFUFailed)
            {
                qDebug() << "kFUFailed";
                _lastErrorMsg << "Device failed firmware command.";
            }
            else
            {
                qDebug() << "Unknown response: " << md.toString(' ') << "\n";
                _lastErrorMsg << "Firmware write generated an unknown response from the device.";
            }
        }
    }
    else
    {
        qDebug() << "Timeout waiting on " << msg.toString(' ') << "\n";
        _lastErrorMsg << "Firmware update failure - timeout after write command.\n" << msg.toString(' ').mid(15,38);
    }

    return rtval;
}
//-------------------------------------------------------------------------
QString DcBootControl::getBankInfoString()
{
    DcBootCodeInfo info;    
    if(!getBootCodeInfo(info))
    {
        return "Failed to access boot code information";
    }

    QString rtstr = "Bank0: " + info.getBank(0).toString();
    rtstr += " Bank1: " + info.getBank(1).toString();
    
    return rtstr;
}

//-------------------------------------------------------------------------
bool DcBootControl::getBootCodeInfo(DcBootCodeInfo& bcInfo)
{
    DcCodeBankInfo codeInfo;

    if(!isBootcode())
    {
        return false;
    }
    else
    {
     
        QRtMidiDevIdent id;
        if(identify(&id))
        {
            bcInfo.setVersion(id.FwVersion);
        }

        QRtAutoTrigger autotc(RESPONCE_BANK_INFO_ANY,_pMidiIn);
        _pMidiOut->dataOut(CMD_GET_BANK0_INFO);

        if(!autotc.wait(500))
        {
            codeInfo.clear();
        }
        else
        {
           QRtMidiData md;
           autotc.dequeue(md);
           codeInfo.init(md);    
        }
        
        bcInfo.setBank(0,codeInfo);
        _pMidiOut->dataOut(CMD_GET_BANK1_INFO);

        if(!autotc.wait(500))
        {
            codeInfo.clear();
        }
        else
        {
            QRtMidiData md;
            autotc.dequeue(md);
            codeInfo.init(md);
        }
        bcInfo.setBank(1,codeInfo);

    }

    return bcInfo.isOk();
}

//-------------------------------------------------------------------------
bool DcBootControl::isBootcode()
{
    QRtAutoTrigger autotc(RESPONCE_BANK_INFO_ANY,_pMidiIn);
    _pMidiOut->dataOut(CMD_GET_BANK0_INFO);
    
    bool rtval = autotc.wait(400);

    return rtval;
}

//-------------------------------------------------------------------------
bool DcBootControl::activateBank( int bankNumber )
{
    bool rtval = false;

    // Parameter Checking
    if(bankNumber != 0 && bankNumber != 1)
    {
        qDebug() << "Invalid bank number specified";
        return false;
    }

    // Verify system state
    if(!isBootcode())
    {
        qDebug() << "not in boot code, can't activate a bank";
        return false;
    }
    
    // Do it
    if(0 == bankNumber)
    {
        QRtAutoTrigger mtrigger(DCBC_ACTIVATE_BANK0_SUCCESS,_pMidiIn);
        _pMidiOut->dataOut(DCBC_ACTIVATE_BANK0);
        if(mtrigger.wait(1000))
        {
            mtrigger.setPattern(DCBC_DEACTIVATE_BANK1_SUCCESS);
            _pMidiOut->dataOut(DCBC_DEACTIVATE_BANK1);
            if(mtrigger.wait(1000))
            {
                rtval = true;
            }
        }
    }
    else
    {
        QRtAutoTrigger mtrigger(DCBC_ACTIVATE_BANK1_SUCCESS,_pMidiIn);
        _pMidiOut->dataOut(DCBC_ACTIVATE_BANK1);
        if(mtrigger.wait(1000))
        {
            mtrigger.setPattern(DCBC_DEACTIVATE_BANK0_SUCCESS);
            _pMidiOut->dataOut(DCBC_DEACTIVATE_BANK0);
            if(mtrigger.wait(1000))
            {
                rtval = true;
            }
        }

    }

    return rtval;
}

//-------------------------------------------------------------------------
bool DcBootControl::exitBoot( QRtMidiDevIdent* id /*= 0*/ )
{
    bool rtval = false;
    

    // Setup to wait for Strymon identity data
    QRtAutoTrigger autotc("F0 7E .. 06 02 00 01 55",_pMidiIn);
    
    _pMidiOut->dataOut("F0 00 01 55 42 01 F7");

    // Wait 4 seconds for the response data
    if(autotc.wait(4000))
    {
        rtval = true;
        
        if(id)
        {
            QRtMidiData md;
            if(autotc.dequeue(md))
            {
                id->fromIdentData(md);
            }
            else
            {
                rtval  = false;
            }
        }
    }
    
    return rtval;
}

//-------------------------------------------------------------------------
QRtMidiData DcBootControl::makePrivateResetCmd()
{
    QRtMidiDevIdent id;
    QRtMidiData priRst;

    // Verify we can ID the connected device
    if(false == identify(&id))
    {
        qWarning() << "No response from identity request";
    }
    else
    {
        // Build private reset command using device product ID.
        priRst.setData(kPrivateResetPartial,id.getFamilyByte(),id.getProductByte());
    }

    return priRst;
}

//-------------------------------------------------------------------------
bool DcBootControl::privateReset()
{
    QRtMidiData priRst = makePrivateResetCmd();
    if(priRst.isEmpty())
    {
        return false;
    }
    
    _pMidiOut->dataOut(priRst);
    
    return true;
}


