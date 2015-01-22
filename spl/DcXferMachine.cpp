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
#include "DcXferMachine.h"

#include <QSignalTransition>
#include "cmn/DcState.h"
#include <QDebug>
#include <QTimer>
#include "PresetLibSMDefs.h"
#include <QThread>
#include "DcMidiDevDefs.h"
#include "DcDeviceDetails.h"
#include "cmn/DcLog.h"
extern bool gUseAltPresetSize;
#define ALT_PRESET

//-------------------------------------------------------------------------
void DcXferMachine::sendNext_entered()
{
    if(_progressDialog->cancled())
    {
        _watchdog.stop();
        _machine->postEvent(new DataXfer_CancledEvent());
    }
    else if(_cmdList.isEmpty())
    {
        _progressDialog->hide();
        _machine->postEvent(new DataXfer_ListEmptyEvent());
    }
    else
    {
        _activeCmd = _cmdList.takeFirst();
        _midiOut->dataOutSplit(_activeCmd,_maxPacketSize,_msPerPacket);
        _retryCount = _numRetries;
        _watchdog.start(_timeout);
    }
}
//-------------------------------------------------------------------------
void DcXferMachine::replySlotForDataIn( const DcMidiData &data )
{
    // Expect to see sysex coming from the currently attached device.
    // This check will prevent other legal messages from blowing up
    // the transfer - like controller messages, etc...
    if(data.contains(_devDetails->SOXHdr))
    {
        if(data == _activeCmd)
        {
            // If the data coming back is the same as what was sent, 
            // it was echoed back to us, just ignore as it was probably due
            // to a device with MIDI "soft" THRU (midi-merge)
            return;
        } 

        // cancel the watchdog timer
        _watchdog.stop();

        // Check for a Negative Acknowledgment of the data in request
        if( data.match(_devDetails->PresetRd_NAK,true) )
        {
            _progressDialog->setError("Device Rejected Command");
            _machine->postEvent(new DataXfer_NACKEvent());
        }
        else if( data.match(_devDetails->PresetRd_ACK) )
        {
            DcMidiData recompinded;

            if(gUseAltPresetSize)
            {
                int chnksz   = data.get14bit(9+2);
                recompinded = data.mid(0,9);
                recompinded[6] = 0x62;
                recompinded.append(DcMidiData(data.mid(9+6,chnksz)));
                recompinded.append(DcMidiData("18191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132337F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F7F"));
                recompinded.append(DcMidiData(data.mid(9+6+chnksz)));
            }
            else
            {
                        recompinded = data;
            }


            if( verifyPresetData(recompinded,_progressDialog,_devDetails) == true )
            {    
                _progressDialog->inc();
                _midiDataList.append(recompinded);
                _machine->postEvent(new DataXfer_ACKEvent());
            }
            else
            {
                _machine->postEvent(new DataXfer_NACKEvent()); 
            }
        }
        else
        {
            DCLOG() << "Unexpected data received after preset read";
            DCLOG() << data.toString();
            _progressDialog->setError("Unexpected data received after requesting the preset.");
            _machine->postEvent(new DataXfer_NACKEvent());
        }
    }
}

//-------------------------------------------------------------------------
void DcXferMachine::replySlotForDataOut( const DcMidiData &data )
{
    DCLOG() << "Midi In";
    // Expect to see sysex coming from the currently attached device.
    // This check will prevent other legal messages from blowing up
    // out preset transfer - like controller messages
    if(data.contains(_devDetails->SOXHdr))
    {
        bool NAK = data.match(_devDetails->PresetWr_NAK);
        bool ACK = data.match(_devDetails->PresetWr_ACK);
        
        // If it's not an ACK or NAK, then just ignore the response data
        if(!(NAK || ACK))
        {
            // Check for a strymon MIDITH
            if(data == _activeCmd)
            {
                return;
            }
            
            // This is an unexpected Sysex message, it has a valid strymon HDR, 
            // but the payload was unexpected.
            DCLOG() << "Ignoring unexpected/invalid Strymon Sysex message";
            DCLOG() << "    Sent: " << _activeCmd.toString();
            DCLOG() << "Received: " << data.toString();
            return;
        }

        // This is the response we were looking for, cancel the transfer timeout watchdog
        _watchdog.stop();
        
        // Check for write preset Negative Acknowledgment
        if(NAK)
        {
            if(--_retryCount <= 0)
            {
                DCLOG() << "Retries exhausted, device rejected write command";
                _progressDialog->setError("Device Rejected Write Command");
                _machine->postEvent(new DataXfer_NACKEvent());
            }
            else
            {
                // Retry the Write Command
                DCLOG() << "Command was NAK'ed, retrying";
                QThread::msleep(100);
                _midiOut->dataOutSplit(_activeCmd,_maxPacketSize,_msPerPacket);
                // Restart watchdog
                _watchdog.start(_timeout);
            }
        }
        else if(ACK)
        {
            _progressDialog->inc();
            if( _retryCount < _numRetries )
            {
                _progressDialog->setError( QString("Success after %1 retries").arg(_retryCount) );
                QApplication::processEvents();
            }
            else
            {
                _progressDialog->setError("");
            }
            _machine->postEvent(new DataXfer_ACKEvent());
        }
        else
        {
            // Getting here means an unexpected message was received.
            DCLOG() << "Unexpected data received after preset write";
            DCLOG() << "    Sent: " << _activeCmd.toString();
            DCLOG() << "Received: " << data.toString();

            _progressDialog->setError("Unexpected data after preset write");
            _machine->postEvent(new DataXfer_NACKEvent());
        }
    }
}

void DcXferMachine::strickedReplySlotForDataOut( const DcMidiData &data )
{
    // Expect to see sysex coming from the currently attached device.
    // This check will prevent other legal messages from blowing up
    // out preset transfer - like controller messages
    if(data.contains(_devDetails->SOXHdr))
    {

        // Ignore data 
        if(data == _activeCmd)
        {
            // If the data coming back is the same as what was sent, 
            // it was echoed back to us, just ignore as it was probably due
            // to a device with MIDI "soft" THRU (midi-merge)
            return;
        } 

        // Cancel the watchdog timer
        _watchdog.stop();

        // Check for write preset Negative Acknowledgment
        if( data.match(_devDetails->PresetWr_NAK) )
        {
            _progressDialog->setError("Device Rejected Write Command");
            _machine->postEvent(new DataXfer_NACKEvent());
        }
        else if( data.match(_devDetails->PresetWr_ACK) )
        {
            _progressDialog->inc();
            _machine->postEvent(new DataXfer_ACKEvent());
        }
        else
        {
            // Getting here means an unexpected message was received.
            DCLOG() << "Unexpected data received after preset write";
            DCLOG() << "    Sent: " << _activeCmd.toString();
            DCLOG() << "Received: " << data.toString();

            _progressDialog->setError("Unexpected data after preset write");
            _machine->postEvent(new DataXfer_NACKEvent());
        }
    }
}
//-------------------------------------------------------------------------
void DcXferMachine::xferTimeout()
{
    _watchdog.stop();
    
    _progressDialog->setError("Unable to communicate with the device.");
    DCLOG() << "Transfer Timeout";
    _machine->postEvent(new DataXfer_TimeoutEvent());
}

//-------------------------------------------------------------------------
// The object will activate when the returned stated is entered.
// The object completes by transitioning to doneState or errorState
DcState* DcXferMachine::setupStateMachine(QStateMachine* m,DcMidiOut* out,DcState* readDataComplete,DcState* errorState,DcState* cancelState)
{
    _machine = m;
    _midiOut = out;
    // DataXfer_RetryEvent

    DcState *sendNext        = new DcState(QString("sendNext"));
  
    _machine->addState(sendNext);

    QObject::connect(sendNext, SIGNAL(entered()), this, SLOT(sendNext_entered())); // IN
    
    QObject::disconnect(&_watchdog, &QTimer::timeout, 0,0);
    QObject::connect(&_watchdog, &QTimer::timeout, this, &DcXferMachine::xferTimeout);

    DcCustomTransition *ct = new DcCustomTransition(DataXfer_ACKEvent::TYPE,sendNext);
    ct->setTargetState(sendNext);
    sendNext->addTransition(ct);

    ct = new DcCustomTransition(DataXfer_NACKEvent::TYPE,sendNext);
    ct->setTargetState(errorState);
    sendNext->addTransition(ct);

    ct = new DcCustomTransition(DataXfer_ListEmptyEvent::TYPE,sendNext);
    ct->setTargetState(readDataComplete);
    sendNext->addTransition(ct);

    ct = new DcCustomTransition(DataXfer_TimeoutEvent::TYPE,sendNext);
    ct->setTargetState(errorState);
    sendNext->addTransition(ct);

    ct = new DcCustomTransition(DataXfer_CancledEvent::TYPE,sendNext);
    ct->setTargetState(cancelState);
    sendNext->addTransition(ct);

    return sendNext;

}


//-------------------------------------------------------------------------
void DcXferMachine::setProgressDialog( IoProgressDialog* progressDialog )
{
    _progressDialog = progressDialog;
    _progressDialog->reset();
    _progressDialog->setMax(_cmdList.length());    
}

//-------------------------------------------------------------------------
void DcXferMachine::append( DcMidiData& cmd )
{
    _cmdList.append(cmd);
}

//-------------------------------------------------------------------------
void DcXferMachine::reset()
{
    _cmdList.clear();
    _midiDataList.clear();
    _cancel = false;
    _watchdog.stop();
}

//-------------------------------------------------------------------------
void DcXferMachine::go(DcDeviceDetails* devDetails, int maxPacketSize/*=-1*/, int delayPerPacket /*=0*/)
{
    _devDetails = devDetails;

    _maxPacketSize = maxPacketSize;
    _msPerPacket = delayPerPacket;

    _timeout = 2000;
    _cancel = false;
    _numRetries = 4;
    _progressDialog->setProgress(0);
    _progressDialog->setMax(_cmdList.length());
    _progressDialog->show();
}

//-------------------------------------------------------------------------
bool DcXferMachine::verifyPresetData( const DcMidiData &data, IoProgressDialog* progDialog, const DcDeviceDetails* devinfo )
{
    // Verify Data transfer:
    bool rtval = false;

    // Verify: data must end with a EOX (F7)
    if( data.at(data.length()-1) != 0xF7 )
    {
        // Incomplete data response
        DCLOG() << "Data transfer IN - incomplete preset received, never say EOX";
        progDialog->setError("Received incomplete preset data");
    }
    else 
    {
        if( data.length() != devinfo->PresetSize )
        {
            DCLOG() << "Data transfer IN - packet size mismatch: expected " << devinfo->PresetSize << " got " << data.length();
            DCLOG() << "Bad Packet: " << data.toString();

            if(devinfo->PresetSize > data.length())
            {
                progDialog->setError("The preset data received is corrupt and too small");
            }
            else
            {
                progDialog->setError("The preset data received is too large");
            }
        }
        else 
        {
            // Verify the checksum
            int sum = data.sumOfSection(devinfo->PresetStartOfDataOffset,devinfo->PresetDataLength);
            int dataVal = data.at(devinfo->PresetChkSumOffset);
            if(sum != dataVal)
            {
                DCLOG() << "The preset data received is corrupt, checksum Error.";
                DCLOG() << data.toString();
            }
            else
            {
                rtval = true;                      
            }
        }
    }
    return rtval;
}
