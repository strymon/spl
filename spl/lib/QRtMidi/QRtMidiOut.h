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
#ifndef QRtMidiOut_h__
#define QRtMidiOut_h__

#include "QRtMidi/RtMidi/RtMidi.h"
#include <QStringList>
#include <QHash>
#include <QTextStream>
#include <QObject>

#include "QRtMidi/QRtMidi.h"

class QRtMidiOut : public QRtMidi
{
    Q_OBJECT

public:

    QRtMidiOut();
    virtual ~QRtMidiOut();
     
signals:
    void dataOutMonitor(const QRtMidiData& data);

public slots:    
    void dataOut( const QRtMidiData& data );
    void dataOut( const QString& hexStr);
    void dataOut( const char* hexStr);


private:

    // Must create these methods
    virtual bool createRtMidiDev( );
    virtual void destoryRtMidiDev();
    virtual RtMidi* getRtMidi() {return _rtMidiOut;}
    virtual void setupAfterOpen(quint32 flags = 0);
   
    RtMidiOut*  _rtMidiOut;
};
#endif // QRtMidiOut_h__
