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
#include "QRtMidi/QRtMidiIn.h"

#include <QStringList>
#include <QTextStream>
#include <QThread>
#include <QDateTime>
#include <vector>

//-------------------------------------------------------------------------
QRtMidiIn::QRtMidiIn()
{
    _rtMidiIn = 0;
}

//-------------------------------------------------------------------------
QRtMidiIn::~QRtMidiIn()
{
    destoryRtMidiDev();
}

//-------------------------------------------------------------------------
bool QRtMidiIn::createRtMidiDev()
{
    bool result = true;

#ifdef QRTMIDI_USE_TRY
    try 
    {
#endif // QRTMIDI_USE_TRY
        _rtMidiIn = new RtMidiIn;
#ifdef QRTMIDI_USE_TRY
    }
    catch (RtError &error) 
   {
        result = false;
        setError("create RtMidiIn",error);
    } 

#endif // QRTMIDI_USE_TRY
    return result;
}

//-------------------------------------------------------------------------
void QRtMidiIn::destoryRtMidiDev()
{
    if(_rtMidiIn)
    {
    	
        _rtMidiIn->closePort();
        _rtMidiIn->cancelCallback();
        delete _rtMidiIn;
    	_rtMidiIn = 0;
    }
}


void QRtMidiIn_midiInCallback( double deltatime, std::vector< unsigned char > *message, void *userData )
{
    QRtMidiIn* pIoDev = (QRtMidiIn*)userData;
    pIoDev->midiDataIn(deltatime,message);
}

//-------------------------------------------------------------------------
void QRtMidiIn::midiDataIn( double deltatime, std::vector< unsigned char > * message )
{
    Q_UNUSED(deltatime);

#ifdef QRT_ENABLE_BROKEN_SYNC_CODE    
    Q_ASSERT(_assertInCallback);
#endif
    
#ifdef VERBOSE_MIDI_DEBUG
    qDebug() << "QRtMidiIn::midiDataIn: " << QRtMidiData(*message).toString();
#endif // VERBOSE_MIDI_DEBUG

    emit dataIn(QRtMidiData(*message));
}

//-------------------------------------------------------------------------
void QRtMidiIn::setupAfterOpen( quint32 flags /*=0*/)
{
    Q_UNUSED(flags);

#ifdef QRT_ENABLE_BROKEN_SYNC_CODE
    _assertInCallback = true;
#endif

    _rtMidiIn->setCallback(&QRtMidiIn_midiInCallback,this);
    
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
void QRtMidiIn::setSlot( const QObject *receiver,const char *member)
{
   
    if( !disconnect(this,&QRtMidiIn::dataIn,0,0) )
    {
        qDebug() << __FUNCTION__ << " Failed to disconnect previous receiver";
    }

    connect(this, SIGNAL(QRtMidiIn::dataIn()), receiver, member);
    
}




