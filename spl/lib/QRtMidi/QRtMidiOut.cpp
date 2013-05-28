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
#include <QStringList>
#include <QTextStream>
#include <QThread>

#include "QRtMidi/QRtMidiOut.h"

//-------------------------------------------------------------------------
QRtMidiOut::QRtMidiOut()
{
    _rtMidiOut = 0;
}

//-------------------------------------------------------------------------
QRtMidiOut::~QRtMidiOut()
{
    destoryRtMidiDev();
}

//-------------------------------------------------------------------------
bool QRtMidiOut::createRtMidiDev()
{
    bool result = true;

#ifdef QRTMIDI_USE_TRY
    try 
    {
#endif // QRTMIDI_USE_TRY
        _rtMidiOut = new RtMidiOut;
#ifdef QRTMIDI_USE_TRY
    }
    catch (RtError &error) 
    {
        result = false;
        setError("create RtMidiOut",error);
    } 

#endif // QRTMIDI_USE_TRY
    return result;
}

//-------------------------------------------------------------------------
void QRtMidiOut::destoryRtMidiDev()
{
    if(_rtMidiOut)
    {
        _rtMidiOut->closePort();
        delete _rtMidiOut;
        _rtMidiOut = 0;
    }
}

//-------------------------------------------------------------------------
void QRtMidiOut::setupAfterOpen(quint32 flags /*=0*/)
{
    Q_UNUSED(flags);
}


//-------------------------------------------------------------------------
void QRtMidiOut::dataOut( const QRtMidiData& data )
{
    std::vector<unsigned char> vec;
    data.copyToStdVec(vec);
    if(isOk())
    {
        _rtMidiOut->sendMessage( &vec );
#ifdef VERBOSE_MIDI_DEBUG
        qDebug() << "QRtMidiOut::dataOut: " << data.toString();
#endif // VERBOSE_MIDI_DEBUG

        emit dataOutMonitor(data);
    }
}

//-------------------------------------------------------------------------
void QRtMidiOut::dataOut( const QString& hexStr )
{
    dataOut(QRtMidiData(hexStr.toStdString().c_str()));
}

//-------------------------------------------------------------------------
void QRtMidiOut::dataOut( const char* hexStr )
{
    dataOut(QRtMidiData(hexStr));
}

