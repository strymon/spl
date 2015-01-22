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
#ifndef DcMidiIn_h__
#define DcMidiIn_h__


#include <QStringList>
#include <QHash>
#include <QTextStream>
#include <QObject>
#include <QPair>
#include <QList>
#include <QQueue>

#include <QRegExp>
#include <QReadWriteLock>
#include <QMutexLocker>


#include "DcMidi.h"



class RtMidiIn;
class RtMidi;

class DcMidiTrigger;

class DcMidiIn : public DcMidi
{
    Q_OBJECT

public:
    static const quint32 kOpen_Default = 0;
    static const quint32 kOpen_NoCallback = 1;

    DcMidiIn(QObject* parent);
    DcMidiIn();
    virtual ~DcMidiIn();
    
    void midiDataIn( double deltatime, std::vector< unsigned char > * message );
    
    DcMidiData getLastMidiIn();

    // Set the receiver object and member
    void setSlot( const QObject *receiver,const char* member);
   
    void addTrigger(DcMidiTrigger& tc);

    void removeTrigger(DcMidiTrigger& tc);
    
    void removeAllTriggers();

signals:
    void dataIn(const DcMidiData& data);

private:
    
    // Must create these methods
    virtual bool createRtMidiDev( );
    virtual void destoryRtMidiDev();
    virtual RtMidi* getRtMidi() {return (RtMidi*)_rtMidiIn;}
    virtual void setupAfterOpen(quint32 flags = 0);

    RtMidiIn*  _rtMidiIn;
    DcMidiData _lastMidiIn;
    bool _assertInCallback;
    
    QMetaObject::Connection _reciver;
    
    QReadWriteLock _triggersLock;
    QList<DcMidiTrigger*> _triggers;
};
#endif // DcMidiIn_h__
