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
#include "RtMidi/RtMidi.h"
#include "DcMidi.h"
#include <QThread>
//#include <QApplication>
#define QRTMIDI_USE_TRY
//-------------------------------------------------------------------------
DcMidi::DcMidi()
{
    _curOpenPortIdx = kNotOpen;
}

//-------------------------------------------------------------------------
DcMidi::DcMidi( QObject* parent )
    : QObject(parent)
{
    _curOpenPortIdx = kNotOpen;
}

//-------------------------------------------------------------------------
void DcMidi::clearLastStatus()
{
    return      _lastError.clear();
}

//-------------------------------------------------------------------------
void DcMidi::setError( const char* msg, const std::string& msg2) // RtMidiError &error )
{
    QTextStream emsg(&_lastError);
    emsg << msg << " : " << msg2.c_str();
}

//-------------------------------------------------------------------------
void DcMidi::setError( const char* msg)
{
    QTextStream emsg(&_lastError);
    emsg << msg;
}

//-------------------------------------------------------------------------
void DcMidi::setError( const char* msg1,QString msg2,const char* msg3 )
{
    QTextStream emsg(&_lastError);
    emsg << msg1 << msg2 << msg3;
}

//-------------------------------------------------------------------------
bool DcMidi::isOpen()
{
    return _curOpenPortIdx != kNotOpen;
}
//-------------------------------------------------------------------------
bool DcMidi::isOk()
{
    return getRtMidi() != 0;
}

void DcMidi::close()
{
    if(!isOk())
        return;

    // TODO: Deal with any outstanding I/O?
    stopIo();
    getRtMidi()->closePort();
    _curOpenPortIdx = kNotOpen;
}


//-------------------------------------------------------------------------
// Filter out words like 'in' and 'out' that may make this port name direction
// specific. e.g. 'MIDIPORT A IN' will become 'MIDIPORT A'
QString DcMidi::filterPortName( QString& portName )
{
    QString rtval = portName;

    rtval.replace(QRegExp("([- ]in|[- ]out)",Qt::CaseInsensitive),"");
    rtval.replace(QRegExp("  ")," ");

    return rtval;
}


//-------------------------------------------------------------------------
QString DcMidi::getLastErrorString()
{
    return _lastError;
}

//-------------------------------------------------------------------------
void DcMidi::buildPortNameList(RtMidi* pRtMidi)
{
    QString portName;
    _lastError.clear();
    _portNameIndexHash.clear();

    int total = pRtMidi->getPortCount();
    for (int idx = 0; idx < total; ++idx) 
    {
            portName = QString(pRtMidi->getPortName(idx).c_str());
#ifdef QRT_FILTER_PORT_NAMES
            portName = filterPortName(portName);
#endif
        
            portName = portName.remove(QString::number(idx)).trimmed();
            _portNameIndexHash.insert(portName,idx);
    }
}

//-------------------------------------------------------------------------
int DcMidi::getPortCount()
{
    return getRtMidi()->getPortCount();
}


bool DcMidi::update()
{
    if(!isOk()) 
    {
        if(!init())
        {
            return false;
        }
    }

    _portNameIndexHash.clear();
    _lastError.clear();

    buildPortNameList(getRtMidi());

    return true;
}

QStringList DcMidi::getPortNames()
{
    return QStringList(_portNameIndexHash.keys()); 
}


//-------------------------------------------------------------------------
void DcMidi::destroy()
{
    _portNameIndexHash.clear();
    _curPortName.clear();
    
    destoryRtMidiDev();
}

//-------------------------------------------------------------------------
bool DcMidi::init()
{
    bool result = true;

    // it's always ok to call init
    destroy();

    result = createRtMidiDev();

    if(result)
    {
        update();
    }
    else
    {
        destroy();
    }

    return result;
}

bool DcMidi::open( QString portName /*= ""*/,quint32 flags /*= 0*/ )
{
    clearLastStatus();

    if(!isOk())
    {
        setError("Can't open, not initialized");
        return false;
    }

    // Close current port if needed
    if(_curOpenPortIdx != kNotOpen)
    {
        close();
    }

    // Verify input argument
    if(portName.isEmpty())
    {
        if(_curPortName.isEmpty())
        {
            setError("Can't open, no port selected");
            return false;
        }
    }
    else
    {
        _curPortName = portName;
    }

    int idx = _portNameIndexHash.value(_curPortName,-1);
    if(-1 != idx)
    {
#ifdef QRTMIDI_USE_TRY
        try
        {
#endif // QRTMIDI_USE_TRY
            // Open device idx
            getRtMidi()->openPort(idx);
            _curOpenPortIdx = idx;
            setupAfterOpen(flags);
#ifdef QRTMIDI_USE_TRY
        }
        catch (RtMidiError &error)
        {
            close();
            setError("open",error.getMessage());
            qDebug() << "RtMidi Open Error: " << error.getMessage().c_str();
        }
#endif // QRTMIDI_USE_TRY
    }
    else
    {
        setError("open  - Port Name \"",_curPortName,"\" was not found");
    }

    return _curOpenPortIdx != kNotOpen;
}
