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
#include "DcMidiIn.h"
#include "RtMidi/RtMidi.h"

#include <QStringList>
#include <QTextStream>
#include <QThread>
#include <QDateTime>
#include <vector>

#include <QWaitCondition>
#include <QReadWriteLock>
#include "DcMidiTrigger.h"

// #define VERBOSE_MIDI_DEBUG 1
//-------------------------------------------------------------------------
DcMidiIn::DcMidiIn()
{
    _rtMidiIn = 0;
}

DcMidiIn::DcMidiIn( QObject* parent ) : 
    DcMidi(parent),_rtMidiIn(0)
{

}

//-------------------------------------------------------------------------
DcMidiIn::~DcMidiIn()
{
    destoryRtMidiDev();
}

//-------------------------------------------------------------------------
bool DcMidiIn::createRtMidiDev()
{
    bool result = true;

#ifdef QRTMIDI_USE_TRY
    try 
    {
#endif // QRTMIDI_USE_TRY
        _rtMidiIn = new RtMidiIn;
#ifdef QRTMIDI_USE_TRY
    }
    catch (RtMidiError &error)
   {
        result = false;
        setError("create RtMidiIn",error.getMessage());
    } 

#endif // QRTMIDI_USE_TRY
    return result;
}

//-------------------------------------------------------------------------
void DcMidiIn::destoryRtMidiDev()
{
    if(_rtMidiIn)
    {
    	
        _rtMidiIn->closePort();

        removeAllTriggers( );

        delete _rtMidiIn;
    	_rtMidiIn = 0;
    }
}


void DcMidiIn_midiInCallback( double deltatime, std::vector< unsigned char > *message, void *userData )
{
    DcMidiIn* pIoDev = (DcMidiIn*)userData;
    pIoDev->midiDataIn(deltatime,message);
}

//-------------------------------------------------------------------------
void DcMidiIn::midiDataIn( double deltatime, std::vector< unsigned char > * message )
{
    Q_UNUSED(deltatime);

#ifdef QRT_ENABLE_BROKEN_SYNC_CODE    
    Q_ASSERT(_assertInCallback);
#endif
    
#ifdef VERBOSE_MIDI_DEBUG
    qDebug() << "DcMidiIn::midiDataIn: " << DcMidiData(*message).toString();
#endif // VERBOSE_MIDI_DEBUG
    
    DcMidiData md = DcMidiData(*message,this);
    
   
    // Very soft time stamp, should use deltatime.
    md.setTimeStamp();

    bool emitMessage = true;
    if(_triggers.size())
    {
        QReadLocker locker(&_triggersLock);
        for (int i = 0; i < _triggers.size(); ++i) 
        {
            DcMidiTrigger* pTc = _triggers.at(i);
            if ( md.match(pTc->getRegExp()) )
            {
                emitMessage  = pTc->handler(md);
            }
        }
    }

    emit dataIn(md);
}

//-------------------------------------------------------------------------
void DcMidiIn::setupAfterOpen( quint32 flags /*=0*/)
{
    Q_UNUSED(flags);

#ifdef QRT_ENABLE_BROKEN_SYNC_CODE
    _assertInCallback = true;
#endif

    _rtMidiIn->setCallback(&DcMidiIn_midiInCallback,this);
    
    #ifdef VERBOSE_MIDI_DEBUG
qDebug() << "setupAfterOpen - callback registered";
    #endif // VERBOSE_MIDI_DEBUG


#ifdef QRT_ENABLE_BROKEN_SYNC_CODE
    // There might be some junk in the input queue that got missed before
    // the callback was setup and after the openPort was call.  If there's
    // any data in the quest, emit a message
    std::vector<unsigned char> message;
    double stamp;

    // Only empty up to 20 messages, 
    for (int idx = 0; idx < 20 ; idx++)
    {
        stamp = _rtMidiIn->getMessage( &message);
        if(message.size())
        {
            midiDataIn(stamp,&message);
        }
        else
        {
            _assertInCallback = false;
            break;
        }
    }
#endif

    _rtMidiIn->ignoreTypes( false, false, false );
        
}

//-------------------------------------------------------------------------
void DcMidiIn::setSlot( const QObject *receiver,const char *member)
{
   
    if( !disconnect(this,&DcMidiIn::dataIn,0,0) )
    {
        qDebug() << __FUNCTION__ << " Failed to disconnect previous receiver";
    }

    connect(this, SIGNAL(DcMidiIn::dataIn()), receiver, member);
    
}

//-------------------------------------------------------------------------
void DcMidiIn::addTrigger( DcMidiTrigger& tc)
{

    if(_triggers.contains(&tc))
    {
        removeTrigger(tc);
    }
    
    QWriteLocker locker(&_triggersLock);
    _triggers.append(&tc);
}


//-------------------------------------------------------------------------
void DcMidiIn::removeTrigger( DcMidiTrigger& tc)
{
    DcMidiTrigger* pTc = &tc;
    QWriteLocker locker(&_triggersLock);
    
    for (int i = 0; i < _triggers.size(); ++i) 
    {
        if ( pTc == _triggers.at(i))
        {
            _triggers.removeAt(i);
            break;
        }
    }
}



//-------------------------------------------------------------------------
void DcMidiIn::removeAllTriggers( )
{
    QWriteLocker locker(&_triggersLock);
    _triggers.clear();
}



