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
#include "DcMidi/DcMidiData.h"
#include "DcMidi/DcMidiOut.h"
#include <QTimer>
#include "cmn/DcState.h"
#include "IoProgressDialog.h"


struct DcDeviceDetails;

typedef QList<DcMidiData> DcMidiDataList;

class DcXferMachine : public QObject
{

    Q_OBJECT
public:

    static const int kNumRetries = 4;

    DcXferMachine() { }
     ~DcXferMachine () {}

    DcMidiDataList_t getCmdsWritten();

public slots:

 /*!
 	Called to setup the transfer engine
 */
 DcState* setupStateMachine(QStateMachine* m,DcMidiOut* out,DcState* doneState,DcState* errorState,DcState* cancelState);

  // State Handlers
  void sendNext_entered();
    
  /*!
      Data input slot for reading data and the expected
	  input is a block of MIDI data or a NACK message.
  */
  void replySlotForDataIn( const DcMidiData &data );

  
  /*!
    Verify preset data.  The method expects that a complete
    preset is given.
  */ 
  bool verifyPresetData( const DcMidiData &data, IoProgressDialog* progDialog, const DcDeviceDetails* devinfo);


  /*!
	  Data input slot used when writing data - the expected
	  input is an acknowledge (ACK) or negatively acknowledge 
	  (NACK) MIDI message.
  */
  void replySlotForDataOut( const DcMidiData &data );

  /*!
  	Timer handler - triggered on device timeout
  */
  void xferTimeout();
  int getTimeout() const { return _timeout; }
  void setTimeout(int val) { _timeout = val; }
  
  QList<DcMidiData>& getDataList() { return _midiDataList; }
  
  void setCancel() { _cancel = true; }
  void setProgressDialog( IoProgressDialog* progressDialog );

  void go(DcDeviceDetails* _devDetails, int maxPacketSize = -1, int delayPerPacket = 0);

  void append( DcMidiData& cmdStr );

  void reset(bool isWriteMachine);
  //void strickedReplySlotForDataOut( const DcMidiData &data );
private:

    int _timeout;
    QTimer _watchdog;

    bool _cancel;
    IoProgressDialog* _progressDialog;
    
    QStateMachine*  _machine;
    DcMidiOut*     _midiOut;
    QList<DcMidiData>  _cmdList;
    QList<DcMidiData>  _midiDataList;
    DcMidiData _activeCmd;
    int _pacPerCmd;
    
    DcDeviceDetails* _devDetails;
    int _retryCount;
    int _numRetries;
    bool _isWriteMachine;
    DcMidiDataList_t _writeSuccessList;
};
