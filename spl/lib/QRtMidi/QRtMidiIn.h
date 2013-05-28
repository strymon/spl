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
#ifndef QRtMidiIn_h__
#define QRtMidiIn_h__

#include "QRtMidi/RtMidi/RtMidi.h"
#include <QStringList>
#include <QHash>
#include <QTextStream>
#include <QObject>

#include "QRtMidi/QRtMidi.h"

class QRtMidiIn : public QRtMidi
{
    Q_OBJECT

public:
    static const quint32 kOpen_Default = 0;
    static const quint32 kOpen_NoCallback = 1;
    
    QRtMidiIn();
    virtual ~QRtMidiIn();
    
    void midiDataIn( double deltatime, std::vector< unsigned char > * message );
    
    QRtMidiData getLastMidiIn();

    // Set the receiver object and member
    void setSlot( const QObject *receiver,const char* member);
   
signals:
    void dataIn(const QRtMidiData& data);

private:
    
    // Must create these methods
    virtual bool createRtMidiDev( );
    virtual void destoryRtMidiDev();
    virtual RtMidi* getRtMidi() {return _rtMidiIn;}
    virtual void setupAfterOpen(quint32 flags = 0);
    
    RtMidiIn*  _rtMidiIn;
    QRtMidiData _lastMidiIn;
    bool _assertInCallback;
    
    QMetaObject::Connection _reciver;
};
#endif // QRtMidiIn_h__
