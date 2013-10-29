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
 \file DcBootControl.h
 \brief Classes for managing boot code
--------------------------------------------------------------------------*/
#pragma once

#include <QObject>

#include <QMutex>
#include "QRtMidi/QRtMidiTrigger.h"

#include <QRtMidi/QRtMidiIn.h>
#include <QRtMidi/QRtMidiOut.h>

struct QRtMidiDevIdent;

#define CMD_ENABLE_RECOVERY                 "F0 00 01 55 42 11 F7"
#define CMD_GET_BANK0_INFO                  "F0 00 01 55 42 08 F7"
#define CMD_GET_BANK1_INFO                  "F0 00 01 55 42 09 F7"

// Empty or invalid code bank
#define RESPONCE_BANK_INFO_INVALID          "F0 00 01 55 42 0[89] 02 F7"

// BANK INFO: F0 00 01 55 42 0[89] V0 V1 V2 V3 S0 S1 S2 S3 S4 S5 S6 S7 state F7"
#define RESPONCE_BANK_INFO	                "F0 00 01 55 42 0[89] .. .. .. .. .. .. .. .. .. .. .. .. .. F7"
#define RESPONCE_BANK_INFO_ANY              "F0 00 01 55 42 0[89] .. .."

#define RESPONCE_ENABLE_RECOVERY_ANY        "F0 00 01 55 42 11 .. F7"
#define RESPONCE_ENABLE_RECOVERY_ACK        "F0 00 01 55 42 11 00 F7"
#define RESPONCE_ENABLE_RECOVERY_REJECTED   "F0 00 01 55 42 11 01 F7"
#define RESPONCE_ENABLE_RECOVERY_FAILED     "F0 00 01 55 42 11 02 F7"

#define DCBC_ACTIVATE_BANK0 "F0 00 01 55 42 02 F7"
#define DCBC_ACTIVATE_BANK0_SUCCESS "F0 00 01 55 42 02 00 F7"

#define DCBC_ACTIVATE_BANK1  "F0 00 01 55 42 03 F7"
#define DCBC_ACTIVATE_BANK1_SUCCESS "F0 00 01 55 42 03 00 F7"

#define DCBC_DEACTIVATE_BANK0  "F0 00 01 55 42 04 F7"
#define DCBC_DEACTIVATE_BANK0_SUCCESS "F0 00 01 55 42 04 00 F7"

#define DCBC_DEACTIVATE_BANK1  "F0 00 01 55 42 05 F7"
#define DCBC_DEACTIVATE_BANK1_SUCCESS "F0 00 01 55 42 05 00 F7"



/*! A class for parsing and accessing a DC a code "bank" */
class DcCodeBankInfo
{
    static const int kBankIsActive = 1;

public:

    DcCodeBankInfo() {clear();}
    DcCodeBankInfo(QRtMidiData& md) {init(md);}
    
    // true, if this object has been initialized
    bool isOk() { return _size != ~0; }
    
    /*!
      Initialize the boot info with the given MIDI
    */ 
    void init(QRtMidiData& md)
    {
        clear();

        if(md.match(RESPONCE_BANK_INFO_INVALID))
        {
            _valid = false;
        }
        else if(md.match(RESPONCE_BANK_INFO))
        {
            _valid = true;
            _version = bankInfoToCodeVer(md);
            _size = bankInfoToCodeSize(md);
            _state = bankInfoToState(md);
        }
        else
        {
            // Inidicate this object is not initalized;
            _size = ~0;
        }
    }

    /*!
      Clear and invalidate the info object
    */ 
    void clear()
    {
        _valid = false;
        _size = ~0;
        _version.clear();
        _state = false;
    }

    //-------------------------------------------------------------------------
    /*!
      For the given valid boot info response data, return the code size
    */ 
    quint32 bankInfoToCodeSize( QRtMidiData &md )
    {
        QString rtval;
        QTextStream ts(&rtval);
        int codesz = 0;

        if(md.length() >= 20)
        {
            int factor = 28;
            for (int idx = 0; idx < 8 ; idx++)
            {
                codesz += (md.toInt(10+idx,1,0)&0x0F)<<factor;	
                factor -= 4;
            }
        }

        return codesz;
    }

    //-------------------------------------------------------------------------
    /*!
      For the given valid boot info response MIDI data, return the code version.
    */ 
    QString bankInfoToCodeVer( QRtMidiData &md )
    {
        return md.toAsciiString(6,4);
    }

    //-------------------------------------------------------------------------
    /*!
      For the given valid boot info response data, return the state of the code
      bank: ACTIVE, or INVACTIVE
    */ 
    bool bankInfoToState( QRtMidiData &md )
    {
        int s = md.toInt(18,1);
        return (s == kBankIsActive);
    }
    
    /*!
      Pretty print the info object
    */ 
    QString toString()
    {
        QString s;
        QTextStream ts(&s);    
        
        if(_valid)
        {
            
            ts << "v" << _version << ", 0x" << hex << _size << (_state ? ", ACTIVE" : ", INACTIVE");
        }
        else
        {
            ts << "INVALID";
        }
        
        return s;
    }

private:
    bool    _valid;
    quint32 _size;
    QString _version;
    bool    _state;
};

/* A class to manage boot code information.  See the DcCodeBankInfo class
for more details about code banks */
class DcBootCodeInfo
{
public:
    DcBootCodeInfo() {}

    DcCodeBankInfo getBank(int num) const 
    { 
        return num==0 ? _bank0 : _bank1; 
    }

    // At least one bank must be valid or we have nothing to report
    bool isOk() { return _bank1.isOk() || _bank0.isOk(); }
    
    void setBank(int num, DcCodeBankInfo val) 
    { 
        if(num) 
        {
            _bank1 = val;
        }
        else
        {
            _bank0 = val;
        }
    }

    void setVersion( QString& FwVersion )
    {
        _version = FwVersion;
    }

    QString getVersion() { return _version; }

private:

    DcCodeBankInfo _bank0;
    DcCodeBankInfo _bank1;
    QString _version;
};

/*! A class to query boot code and and manipulate code blocks */
class DcBootControl 
{

    static const char* kPrivateResetPartial; /* = "F0 00 01 55 vv vv 1B F7" */
    static const char* kFUResponcePattern; /* = "F0 00 01 55 42 0C .. F7" */
    static const char* kFUGood; /* = "F0 00 01 55 42 0C 00 F7" */
    static const char* kFUBad; /* = "F0 00 01 55 42 0C 01 F7" */
    static const char* kFUFailed; /* = "F0 00 01 55 42 0C 02 F" */
    

public:    
    
    DcBootControl(QRtMidiIn& i, QRtMidiOut& o);
    ~DcBootControl()
    {

    }

    /*!
      Returns true if the attached device is in Bootcode. */
    */ 
    bool isBootcode();

    /*!
      If the device is not in boot code, this command will issue
      the private reset command.  If successful, the device will
      reset.
    */ 
    bool privateReset();

    /*!
      Set the specified code bank active.
    */ 
    bool activateBank(int num);
    
    /*!
      After sending an Identify request, the function will block until 
      a response is received or time-out.  
    */ 
    bool identify(QRtMidiDevIdent* id = 0);

    /*!
        Brings the device into boot mode.
    */
    bool enableBootcode();

    /*!
        If successful, a non-zero QRtMidiData object is returned
        that holds a private reset command.
   */
    QRtMidiData makePrivateResetCmd();

    /*!
      Pretty print the boot info
    */ 
    QString getBankInfoString();

    /*!
      Populate the give DcBoootCodeInfo object
    */ 
    bool getBootCodeInfo(DcBootCodeInfo& bcInfo);

    // Write midi data to the device, will not wait for response
    bool writeMidi(QRtMidiData& msg);

    /*!
      Write the given MIDI update message to the connected device and 
      wait for status. 
    */ 
    bool writeFirmwareUpdateMsg(QRtMidiData& msg,int timeOutMs = 2000);
    
    /*!
       Causes the device to "launch" the active FLASH 
    
    Details -  Whenever the device is reset it executes boot code which 
    it loads from the first sector of program flash. It  then waits 
    for either a firmware / flash maintenance request or a command 
    to launch the application firmware. If a launch request is received 
    or no boot code-specific messages are received within 300 milliseconds 
    the boot code attempts to launch the firmware. If the flash contains 
    valid application firmware, the firmware is loaded and executes. 
    Launching is unsuccessful if the flash does not contain valid firmware. 
   
    The link data rate should be set to 1X before or after a launch.
    */
    bool exitBoot(QRtMidiDevIdent* id = 0);

    // Get last error message
    QString getLastError() const { return _lastErrorMsgStr; }
   
private:

    QRtMidiIn*   _pMidiIn;
    QRtMidiOut* _pMidiOut;
    
    QTextStream _lastErrorMsg;
    QString _lastErrorMsgStr;    
    int _maxDataOut;
    bool _delayBetweenDataOut;
};
