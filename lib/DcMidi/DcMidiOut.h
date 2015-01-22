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
#ifndef DcMidiOut_h__
#define DcMidiOut_h__


#include <QStringList>
#include <QHash>
#include <QTextStream>
#include <QObject>

#include "DcMidi.h"

class RtMidiOut;
class RtMidi;

class DcMidiOut : public DcMidi
{
    Q_OBJECT

public:

    DcMidiOut(QObject* parent = 0);
    virtual ~DcMidiOut();
     
signals:
    void dataOutMonitor(const DcMidiData& data);

public slots:    
    void dataOut( const DcMidiData& data );
    void dataOut( const QString& hexStr);
    void dataOut( const char* hexStr);
    
    void dataOutSplit( const DcMidiData& data, int maxMsg, int delayPerMsg );


private:

    // Must create these methods
    virtual bool createRtMidiDev( );
    virtual void destoryRtMidiDev();
    virtual RtMidi* getRtMidi() {return (RtMidi*)_rtMidiOut;}
    virtual void setupAfterOpen(quint32 flags = 0);
   

    RtMidiOut*  _rtMidiOut;
};
#endif // DcMidiOut_h__
