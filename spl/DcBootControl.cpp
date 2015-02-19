#include "DcBootControl.h"

#include <QMutexLocker>
#include <QDebug>
#include <QDateTime>
#include <QThread>
#include "DcMidi/DcMidiIdent.h"
#include <QApplication>
#include "cmn/DcLog.h"

const char* DcBootControl::kPrivateResetPartial = "F0 00 01 55 vv vv 1B F7";
const char* DcBootControl::kFUGood = "F0 00 01 55 42 0C 00 F7";
const char* DcBootControl::kFUBad = "F0 00 01 55 42 0C 01 F7";
const char* DcBootControl::kFUFailed = "F0 00 01 55 42 0C 02 F7";
const char* DcBootControl::kFUResponcePattern = "F0 00 01 55 42 0C .. F7";

DcBootControl::DcBootControl( DcMidiIn& i, DcMidiOut& o )
{
    // TODO: sharing the DcMidiIn and DcMidiOut objects should be managed properly 
    // and in a brainless easy manner.
    _pMidiIn = &i;
    _pMidiOut = &o;
    _lastErrorMsg.setString(&_lastErrorMsgStr);
}

bool DcBootControl::enableBootcode( )
{
    DcMidiData md;
   
    DcMidiData responceData;

    // The device is in boot mode if it responses to the Echo command.
    if(isBootcode())
    {
        return true;
    }
    
    DcMidiData priRst = makePrivateResetCmd();
    if(0 == priRst.length())
        return false;

#ifdef BAD_MIDI_WORKAROUND
    DcMidiTrigger tc("F0 00 01 55");
#else
    DcMidiTrigger tc(RESPONCE_ENABLE_RECOVERY_ANY);
#endif

    DcAutoTrigger autoch(&tc,_pMidiIn);
    
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
#ifdef BAD_MIDI_WORKAROUND
    return true;
#endif


    bool rtval = false;
    if(responceData.match(RESPONCE_ENABLE_RECOVERY_ACK))
    {
        // Verify device is in boot code
        if(!isBootcode())
        {
            DCLOG() << "Failed to verify device is in boot code";
            // Blind reset
            _pMidiOut->dataOut("F0 00 01 55 42 01 F7");
        }
        else
        {
            DCLOG() << "Device is running boot code";
            rtval = true;
        }
    }
    else if(responceData.match(RESPONCE_ENABLE_RECOVERY_REJECTED))
    {
        DCLOG() << "The device has rejected the enable recovery command";
    }
    else if(responceData.match(RESPONCE_ENABLE_RECOVERY_FAILED))
    {
        DCLOG() << "Device has failed the enabled recovery command";
    }
    else
    {
        // Never saw the requested responce from the device
        // Who knows what's going on now - incase the device is in boot-mode
        // Send a reset command.
        _pMidiOut->dataOut("F0 00 01 55 42 01 F7");

        DCLOG() << "Timeout entering boot code";
    }
    return rtval;
}

bool DcBootControl::identify(DcMidiDevIdent* id /*=0 */)
{
    bool rtval = false;

    DcAutoTrigger autotc("F0 7E .. 06 02 00 01 55",_pMidiIn);
#ifdef BAD_MIDI_WORKAROUND
    _pMidiOut->dataOut("F0 00 01 55 12 01 21 F7 F0 7E 7F 06 01 F7");
#else
       _pMidiOut->dataOut("F0 7E 7F 06 01 F7");
#endif

    // Wait for the response data, or timeout after 300ms
    if(autotc.wait(3000))
    {
        if(id)
        {
            DcMidiData md;
            if(autotc.dequeue(md))
            {
                id->fromIdentData(md);
                DCLOG() << id->toString() << "\n";
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

bool DcBootControl::writeMidi(DcMidiData& msg)
{
    _pMidiOut->dataOutThrottled(msg);
    return true;
}

bool DcBootControl::writeFirmwareUpdateMsg(DcMidiData& msg,int timeOutMs /*= 2000*/)
{
    bool rtval = false;

#ifdef BAD_MIDI_WORKAROUND
    DcAutoTrigger autotc("F0 00 01 55 42",_pMidiIn);
#else
    DcAutoTrigger autotc(kFUResponcePattern,_pMidiIn);
#endif

    // Magic number 8 is the response control flags, a 3 will
    // deliver status.
    msg[8] = 0x03;

    _pMidiOut->dataOutThrottled(msg);

    DcMidiData md;

    // Wait for the response data, or timeout after 300ms
    if(autotc.wait(timeOutMs))
    {
        if(autotc.dequeue(md))
        {

#ifdef BAD_MIDI_WORKAROUND
            // nop - can't get full status from a bad midi device
            rtval =  true;
#else
            if(md == kFUGood)
            {
                rtval =  true;
            }
            else if(md == kFUBad)
            {
                DCLOG() << "kFUBad";
                _lastErrorMsg << "Device reject firmware command - BAD packet.";
            }
            else if(md == kFUFailed)
            {
                DCLOG() << "kFUFailed";
                _lastErrorMsg << "Device failed firmware command.";
            }
            else
            {
                DCLOG() << "Unknown response: " << md.toString(' ') << "\n";
                _lastErrorMsg << "Firmware write generated an unknown response from the device.";
            }
#endif
        }
    }
    else
    {
        DCLOG() << "Timeout waiting on " << msg.toString(' ') << "\n";
        _lastErrorMsg << "Firmware update failure - timeout after write command.\n" << msg.toString(' ').mid(15,38);
    }

    return rtval;
}

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

bool DcBootControl::getBootCodeInfo(DcBootCodeInfo& bcInfo)
{
    DcCodeBankInfo codeInfo;

    if(!isBootcode())
    {
        return false;
    }
    else
    {
     
        DcMidiDevIdent id;
        if(identify(&id))
        {
            bcInfo.setVersion(id.FwVersion);
        }

        DcAutoTrigger autotc(RESPONCE_BANK_INFO_ANY,_pMidiIn);
        _pMidiOut->dataOutThrottled(CMD_GET_BANK0_INFO);

        if(!autotc.wait(500))
        {
            codeInfo.clear();
        }
        else
        {
           DcMidiData md;
           autotc.dequeue(md);
           codeInfo.init(md);    
        }
        
        bcInfo.setBank(0,codeInfo);
        _pMidiOut->dataOutThrottled(CMD_GET_BANK1_INFO);

        if(!autotc.wait(500))
        {
            codeInfo.clear();
        }
        else
        {
            DcMidiData md;
            autotc.dequeue(md);
            codeInfo.init(md);
        }
        bcInfo.setBank(1,codeInfo);

    }

    return bcInfo.isOk();
}


bool DcBootControl::isBootcode()
{
    DcAutoTrigger autotc(RESPONCE_BANK_INFO_ANY,_pMidiIn);
    _pMidiOut->dataOutThrottled(CMD_GET_BANK0_INFO);
    
    bool rtval = autotc.wait(400);

    return rtval;
}

bool DcBootControl::checkPid(int pid)
{
    QString responce = QString( RESPONCE_READ_PID_FID ).arg( (pid & 0xFF00) >> 8).arg(pid & 0xFF);
    DcAutoTrigger autotc( responce,_pMidiIn );
    _pMidiOut->dataOutThrottled( DCBC_READ_PID_FID);

    bool rtval = autotc.wait( 400 );

    return rtval;
}

int DcBootControl::countResponcePattern( const QString& cmd, const QString& pattern, int timeOutMs /*= 800*/)
{
    DcAutoTrigger autotc( pattern,_pMidiIn );
    _pMidiOut->dataOutThrottled( cmd);
    QThread::msleep( timeOutMs );
    int cnt = autotc.getCount();
    return cnt;
}

void DcBootControl::setMidiOutSafeMode()
{
    if(_pMidiOut)
    {
        _pMidiOut->setSafeMode();
    }
}

bool DcBootControl::activateBank( int bankNumber )
{
    bool rtval = false;

    // Parameter Checking
    if(bankNumber != 0 && bankNumber != 1)
    {
        DCLOG() << "Invalid bank number specified";
        return false;
    }

    // Verify system state
    if(!isBootcode())
    {
        DCLOG() << "not in boot code, can't activate a bank";
        return false;
    }
    
    // Do it
    if(0 == bankNumber)
    {
        DcAutoTrigger mtrigger(DCBC_ACTIVATE_BANK0_SUCCESS,_pMidiIn);
        _pMidiOut->dataOutThrottled(DCBC_ACTIVATE_BANK0);
        if(mtrigger.wait(1000))
        {
            mtrigger.setPattern(DCBC_DEACTIVATE_BANK1_SUCCESS);
            _pMidiOut->dataOutThrottled(DCBC_DEACTIVATE_BANK1);
            if(mtrigger.wait(1000))
            {
                rtval = true;
            }
        }
    }
    else
    {
        DcAutoTrigger mtrigger(DCBC_ACTIVATE_BANK1_SUCCESS,_pMidiIn);
        _pMidiOut->dataOutThrottled(DCBC_ACTIVATE_BANK1);
        if(mtrigger.wait(1000))
        {
            mtrigger.setPattern(DCBC_DEACTIVATE_BANK0_SUCCESS);
            _pMidiOut->dataOutThrottled(DCBC_DEACTIVATE_BANK0);
            if(mtrigger.wait(1000))
            {
                rtval = true;
            }
        }

    }

    return rtval;
}


bool DcBootControl::exitBoot( DcMidiDevIdent* id /*= 0*/ )
{
    bool rtval = false;
    
#ifdef BAD_MIDI_WORKAROUND
    DcAutoTrigger autotc("F0 7E .. 06",_pMidiIn);
    _pMidiOut->dataOutThrottled("F0 00 01 55 42 01 F7");
#else
    // Setup to wait for Strymon identity data
    DcAutoTrigger autotc("F0 7E .. 06 02 00 01 55",_pMidiIn);
    _pMidiOut->dataOutThrottled("F0 00 01 55 42 01 F7");
#endif

    // Wait 4 seconds for the response data
    if(autotc.wait(4000))
    {
        rtval = true;
        
        if(id)
        {
            DcMidiData md;
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


DcMidiData DcBootControl::makePrivateResetCmd()
{
    DcMidiDevIdent id;
    DcMidiData priRst;

    // Verify we can ID the connected device
    if(false == identify(&id))
    {
        DCLOG() << "No response from identity request";
    }
    else
    {
        // Build private reset command using device product ID.
        priRst.setData(kPrivateResetPartial,id.getFamilyByte(),id.getProductByte());
    }

    return priRst;
}


bool DcBootControl::privateReset()
{
    DcMidiData priRst = makePrivateResetCmd();
    if(priRst.isEmpty())
    {
        return false;
    }
    
    _pMidiOut->dataOutThrottled(priRst);
    
    return true;
}


