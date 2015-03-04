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
#include "RtMidi/RtMidi.h"
#include "DcMidiOut.h"
//#define VERBOSE_MIDI_DEBUG 1
//-------------------------------------------------------------------------
DcMidiOut::DcMidiOut(QObject* parent)
    : DcMidi(parent),_rtMidiOut(0),_maxDataOut(0),_delayBetweenPackets(0),_safeModeMax(kDefaultSafeMaxPacketSize),
      _safeModeDelay(kDefaultSafeDelayBetweenPackets)
{

}

//-------------------------------------------------------------------------
DcMidiOut::~DcMidiOut()
{
    destoryRtMidiDev();
}

//-------------------------------------------------------------------------
bool DcMidiOut::createRtMidiDev()
{
    bool result = true;

#ifdef QRTMIDI_USE_TRY
    try 
    {
#endif // QRTMIDI_USE_TRY
        _rtMidiOut = new RtMidiOut;
#ifdef QRTMIDI_USE_TRY
    }
    catch (RtMidiError &error)
    {
        result = false;
        setError("create RtMidiOut",error.getMessage());
    } 
#endif // QRTMIDI_USE_TRY

    return result;
}

//-------------------------------------------------------------------------
void DcMidiOut::destoryRtMidiDev()
{
    if(_rtMidiOut)
    {
        _rtMidiOut->closePort();
        delete _rtMidiOut;
        _rtMidiOut = 0;
    }
}

//-------------------------------------------------------------------------
void DcMidiOut::setupAfterOpen(quint32 flags /*=0*/)
{
    Q_UNUSED(flags);
}

void DcMidiOut::dataOutThrottled(const DcMidiData& data)
{
    dataOutSplit(data,_maxDataOut,_delayBetweenPackets);
}

//-------------------------------------------------------------------------
void DcMidiOut::dataOutSplit( const DcMidiData& data, int maxMsg, int delayUs )
{   
    QElapsedTimer sendtime;

    if(maxMsg <= 0 || data.length() < maxMsg)
    {
        //sendtime.start();
        dataOutNoSplit(data);
    }
    else
    {
        QList<DcMidiData> list = data.split(maxMsg);
        QList<DcMidiData>::iterator i;
        //sendtime.start();
        for (i = list.begin(); i != list.end(); ++i)
        {
            if( !dataOutNoSplit( *i ) )
                break;
            // This is a very gross timing loop - it's not
            // very real time and it will probably be way longer than the values suggest
            if(delayUs >= 320 && delayUs < 10000000)
            {
                QThread::usleep( delayUs);
            }
        }
    }

    if(getLoglevel() == 2)
    {
        quint64 ns = sendtime.nsecsElapsed();
        qDebug() << "Sent " << data.length() << " bytes in " << ns / 1000 << "us";
    }

}

//-------------------------------------------------------------------------
bool DcMidiOut::dataOutNoSplit( const DcMidiData& data )
{
    bool rtval = false;
    std::vector<unsigned char> vec;
    data.copyToStdVec(vec);
    if(isOk())
    {
        try
        {
            _rtMidiOut->sendMessage( &vec );
            rtval = true;
            emit dataOutMonitor(data);

            if( getLoglevel() )
            {
                qDebug() << "tx:" << data.toString();
            }

        }
        catch (RtMidiError &error)
        {
            if( getLoglevel() )
            {
                qDebug() << "tx-error:" << data.toString();
            }
            qDebug() << "tx-error-message: " << error.getMessage().c_str();
            setError("RtMidiOut::dataOut ",error.getMessage());
        }
    }
    return rtval;
}
//-------------------------------------------------------------------------
void DcMidiOut::dataOut( const DcMidiData& data )
{
    dataOutSplit(data,_maxDataOut,_delayBetweenPackets);
}

//-------------------------------------------------------------------------
void DcMidiOut::dataOut( const QString& hexStr )
{
    dataOut(DcMidiData(hexStr.toStdString().c_str()));
}

//-------------------------------------------------------------------------
void DcMidiOut::dataOut( const char* hexStr )
{
    dataOut(DcMidiData(hexStr));
}

void DcMidiOut::setMaxPacketSize( int szInBytes )
{
    _maxDataOut = szInBytes;
}

void DcMidiOut::setDelayBetweenBackets( int micros )
{
    _delayBetweenPackets = micros;
}

void DcMidiOut::resetSpeed()
{
    _delayBetweenPackets = -1;
}

void DcMidiOut::setSafeModeDefaults(int maxSizePerCmd, int delay)
{
    _safeModeDelay = (delay==-1) ? kDefaultSafeDelayBetweenPackets : delay;
    _safeModeMax = (maxSizePerCmd==-1) ? kDefaultSafeMaxPacketSize : maxSizePerCmd;
}

void DcMidiOut::setSafeMode()
{
    setMaxPacketSize(_safeModeMax);
    setDelayBetweenBackets(_safeModeDelay);
}

