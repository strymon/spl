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
#include "DcState.h"
#include <QDebug>
#include <QTimer>
#include "PresetLibSMDefs.h"
#include <QThread>
#include "DcMidiDevDefs.h"

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
        QRtMidiData cmd = _cmdList.takeFirst();
        _watchdog.start(_timeout);
        _midiOut->dataOut(cmd);
    }
}
//-------------------------------------------------------------------------
void DcXferMachine::replySlotForDataIn( const QRtMidiData &data )
{
    // Expect to see sysex coming from the currently attached device.
    // This check will prevent other legal messages from blowing up
    // out preset transfer - like controller messages
    if(data.contains(kStrymonDevice))
    {
        // cancel the watchdog timer
        _watchdog.stop();

        // Check for a Negative Acknowledgment of the data in request
        if(data.contains("F0 00 01 55 XX XX 47 F7"))
        {
            _progressDialog->setError("Device Rejected Command");
            _machine->postEvent(new DataXfer_NACKEvent());
        }
        else if(data.contains("F0 00 01 55 XX XX 62")) // A preset opcode is 62h
        {
            _progressDialog->inc();
            _midiDataList.append(data);
            _machine->postEvent(new DataXfer_ACKEvent());
        }
        else
        {
            // Getting here means an unexpected message was received.
            say("Unexpected messaged received - transfer protocol violation - sadness");
            _progressDialog->setError("The device responded incorrectly to the read request, sorry!");
            _machine->postEvent(new DataXfer_NACKEvent());
        }
    }
}

//-------------------------------------------------------------------------
void DcXferMachine::replySlotForDataOut( const QRtMidiData &data )
{
    // Expect to see sysex coming from the currently attached device.
    // This check will prevent other legal messages from blowing up
    // out preset transfer - like controller messages
    if(data.contains(kStrymonDevice))
    {
        // cancel the watchdog timer
        _watchdog.stop();
        // Check for a Negative Acknowledgment of the data in request
        if(data.contains("F0 00 01 55 XX XX XX XX 46 F7"))
        {
            _progressDialog->setError("Device Rejected Write Command");
            _machine->postEvent(new DataXfer_NACKEvent());
        }
        else if(data.contains("F0 00 01 55 XX XX XX XX 45 F7"))
        {
            _progressDialog->inc();
            _machine->postEvent(new DataXfer_ACKEvent());
        }
        else
        {
            // Getting here means an unexpected message was received.
            say("Unexpected messaged received - transfer protocol violation - sadness");
            _progressDialog->setError("The device responded incorrectly to the write request, sorry!");
            _machine->postEvent(new DataXfer_NACKEvent());
        }
    }
}

//-------------------------------------------------------------------------
void DcXferMachine::xferTimeout()
{
    _watchdog.stop();
    
    _progressDialog->setError("Sorry, the device is not responding");
    say("DcXferMachine::xferTimeout()");
    _machine->postEvent(new DataXfer_TimeoutEvent());
}

//-------------------------------------------------------------------------
void DcXferMachine::say( QString s )
{
    qDebug() << s;
}

//-------------------------------------------------------------------------
// The object will activate when the returned stated is entered.
// The object completes by transitioning to doneState or errorState
DcState* DcXferMachine::setupStateMachine(QStateMachine* m,QRtMidiOut* out,DcState* readDataComplete,DcState* errorState,DcState* cancelState)
{
    _machine = m;
    _midiOut = out;

    DcState *sendNext        = new DcState(QString("sendNext"));

    //DcState *dataXferComplete            = new DcState(QString("dataXferComplete"),QState::ExclusiveStates);
    
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
    _progressDialog->setMax(_cmdList.length());    
    _progressDialog->reset();

}

//-------------------------------------------------------------------------
void DcXferMachine::append( QRtMidiData& cmd )
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
void DcXferMachine::go()
{
    _timeout = 2000;
    _cancel = false;
    _progressDialog->setProgress(0);
    _progressDialog->setMax(_cmdList.length());
    _progressDialog->show();
}
