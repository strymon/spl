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
#include <QElapsedTimer>

#include "DcMidi.h"

class RtMidiOut;
class RtMidi;

class DcMidiOut : public DcMidi
{
    Q_OBJECT

    static const int kDefaultSafeMaxPacketSize = 32;
    static const int kDefaultSafeDelayBetweenPackets = 20000;

public:

    DcMidiOut(QObject* parent = 0);
    virtual ~DcMidiOut();
    
    /** Set the maximum packet size sent to MIDI driver
     *  int szInBytes - size in bytes
     *  @return void
     */
    void setMaxPacketSize(int szInBytes);
    /** Set the minimum time between MIDI packets
     *  int us - time in microseconds
     *  @return void
     */
    void setDelayBetweenBackets(int micros);
    /** Reset the transfer speed settings.
     *  
     *  @return void
     */
    void resetSpeed();
    /** Relax the MIDI output data rate
     *
     * @return void
     */
    void setSafeMode();

    /** Return true if in 'safe mode'
     *
     * @return bool
     */
    bool isSafeMode() const { return (_maxDataOut==_safeModeMax) && (_delayBetweenPackets==_safeModeDelay);}

    int getSafeModeMaxPacketSize() const { return _safeModeMax; }
    int getSafeModeDelay() const { return _safeModeDelay; }

    int getCurMaxPacketSize() const { return _maxDataOut; }
    int getCurDelay() const { return _delayBetweenPackets; }
    void setSafeModeDefaults(int maxSizePerCmd, int delay);

signals:
    void dataOutMonitor(const DcMidiData& data);

public slots:    
    void dataOut( const DcMidiData& data );
    void dataOut( const QString& hexStr);
    void dataOut( const char* hexStr);

    void dataOutSplit( const DcMidiData& data, int maxMsg, int delayPerMsg );
    bool dataOutNoSplit(const DcMidiData &data);
    
    void dataOutThrottled(const DcMidiData& data);


private:

    // Must create these methods
    virtual bool createRtMidiDev( );
    virtual void destoryRtMidiDev();
    virtual RtMidi* getRtMidi() {return (RtMidi*)_rtMidiOut;}
    virtual void setupAfterOpen(quint32 flags = 0);
   

    RtMidiOut*  _rtMidiOut;

    // working around poor MIDI devices
    int _maxDataOut;
    int _delayBetweenPackets;
    int _safeModeMax;
    int _safeModeDelay;
};
#endif // DcMidiOut_h__
