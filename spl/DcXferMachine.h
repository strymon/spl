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
#pragma once

#include <QString>
#include <QStateMachine>
#include <QObject>
#include "QRtMidi/QRtMidiData.h"
#include "QRtMidi/QRtMidiOut.h"
#include <QTimer>
#include "DcState.h"
#include "IoProgressDialog.h"

typedef QList<QRtMidiData> DcMidiDataList;

class DcXferMachine : public QObject
{

    Q_OBJECT
public:
    DcXferMachine() { }

     ~DcXferMachine () {}

public slots:

  // Called to setup the transfer engine
 DcState* setupStateMachine(QStateMachine* m,QRtMidiOut* out,DcState* doneState,DcState* errorState,DcState* cancelState);

  // State Handlers
  void sendNext_entered();
    
  // Data input slot for reading data and the expected
  // input is a block of MIDI data or a NACK message.
  void replySlotForDataIn( const QRtMidiData &data );
  
  // Data input slot used when writing data - the expected
  // input is an acknowledge (ACK) or negatively acknowledge 
  // (NACK) MIDI message.
  void replySlotForDataOut( const QRtMidiData &data );

  // Timer handler - triggered on device timeout
  void xferTimeout();
 
  void say( QString s );

  int getTimeout() const { return _timeout; }
  void setTimeout(int val) { _timeout = val; }
  
  QList<QRtMidiData>& getDataList() { return _midiDataList; }
  
  void setCancel() { _cancel = true; }
  void setProgressDialog( IoProgressDialog* progressDialog );

  void go();

  void append( QRtMidiData& cmdStr );
  void reset();
 


private:
    int _timeout;
    QTimer _watchdog;

    bool _cancel;
    IoProgressDialog* _progressDialog;
    
    QStateMachine*  _machine;
    QRtMidiOut*     _midiOut;
    QList<QRtMidiData>  _cmdList;
    QList<QRtMidiData>  _midiDataList;
    QRtMidiData _activeCmd;
};
