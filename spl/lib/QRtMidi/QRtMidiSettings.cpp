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

#include "QRtMidiSettings.h"
#include "QRtMidi/QRtMidiData.h"
#include <QSettings>
#include <QThread>

#include <QSetIterator>

#include "QRtMidi/QRtMidiIdent.h"
#include "DcDeviceDetails.h"


QRtMidiSettings::QRtMidiSettings( QWidget *parent /*= 0*/ )
    : QDialog(parent)
{
    ui.setupUi(this);
}


 void QRtMidiSettings::showEvent(QShowEvent *e)
 {
     Q_UNUSED(e);
     _timer.stop();

     connect(&_timer, SIGNAL(timeout()), this, SLOT(updateTestResult()));
     QObject::connect(&_midiIn, &QRtMidiIn::dataIn, this,&QRtMidiSettings::recvDataForTest );


     // Setup the midi IO
     _midiIn.init();
     _midiOut.init();

     ui.testButton->setEnabled(false);

     ui.midiInCombo->clear();
     ui.midiOutCombo->clear();

     // Populate the comboBox names
     ui.midiInCombo->addItems(_midiIn.getPortNames());
     ui.midiOutCombo->addItems(_midiOut.getPortNames());

     QSettings settings;
     // Make initial port selections if possible
     QString portName = settings.value("midi/inport-name").toString();
     int idx = ui.midiInCombo->findText(portName);
     ui.midiInCombo->setCurrentIndex(idx);

     portName = settings.value("midi/outport-name").toString();
     idx = ui.midiOutCombo->findText(portName);
     ui.midiOutCombo->setCurrentIndex(idx);

     ui.resultLabel->setText("no result");
     ui.resultLabel->setStyleSheet("QLabel {background-color: rgb(255, 170, 0);}");
     //pLabel->setStyleSheet("QLabel { background-color : red; color : blue; }");

      this->setWindowIcon(QIcon(":/images/res/dcpm_256x256x32.png"));

     checkPortSelections();

 }
//-------------------------------------------------------------------------
void QRtMidiSettings::on_testButton_clicked()
{
    qDebug() << "----------- Test MIDI -------------";
    
    // Shutdown MIDI 
    _midiIn.destroy();
    _midiOut.destroy();

    // Now restart MIDI

    _midiIn.init();
    _midiOut.init();

    _testResult = QRtTestResults::NotFoundTimeout;
    ui.resultLabel->setStyleSheet("QLabel {background-color: rgb(255, 170, 0);}");

    if(!_midiOut.open(ui.midiOutCombo->currentText()))
    {
        ui.resultLabel->setText("MIDI Out Busy");
        ui.resultLabel->setStyleSheet("QLabel {background-color: rgb(170, 0, 0);}");
        _testResult = QRtTestResults::PortBusy;;
    }

    if(!_midiIn.open(ui.midiInCombo->currentText()))
    {
        ui.resultLabel->setText("MIDI In Busy");
        ui.resultLabel->setStyleSheet("QLabel {background-color: rgb(170, 0, 0);}");
        _testResult = QRtTestResults::PortBusy;
    }

    qDebug() << "Start Timout";
        _timer.start(1000);
    qDebug() << "Req sent";

 /*
        0xF0  SysEx
        0x7E  Non-Realtime
        0x7F  The SysEx channel. Could be from 0x00 to 0x7F. (7F indicates "disregard channel")
        0x06  Sub-ID -- General Information
        0x01  Sub-ID2 -- Identity Request
        0xF7  End of SysEx
*/
    _midiOut.dataOut("F0 7E 7F 06 01 F7");

}

/* Identity Request Response 
    0xF0  SysEx
    0x7E  Non-real time
    0x7F  The SysEx channel. Could be from 0x00 to 0x7F.  7F indicates "disregard channel".
    0x06  Sub-ID -- General Information
    0x02  Sub-ID2 -- Identity Reply

    0xID  Manufacturer's ID
    0xf1  The f1 and f2 bytes make up the family code. Each
    0xf2  manufacturer assigns different family codes to his products.
    
    0xp1  The p1 and p2 bytes make up the model number. Each
    0xp2  manufacturer assigns different model numbers to his products.
    0xv1  The v1, v2, v3 and v4 bytes make up the version number.
    0xv2
    0xv3
    0xv4
    0xF7  End of SysEx
*/

// This method expects to receive a MIDI "Identity Request Response"
void QRtMidiSettings::recvDataForTest(const QRtMidiData &data)
{
    // If the timer is not active, the timeout has happened.
    if(!_timer.isActive())
    {
        return;    
    }

    QRtMidiDevIdent ident(data);

    //QString d = data.toString();

    // Make sure the incoming data is an Identity response, if not, no need
    // to bother - the watchdog timer is still running so there's not hang.
    if(!ident.isEmpty())
    {
        _timer.stop();
        if(hasDevSupport(data))
        {
            _testResult = QRtTestResults::Success;
            qDebug() << "success";
        }
        else
        {
            _testResult = QRtTestResults::UnknownDevice;
          
            qDebug() << "unknown: " << data.data();
        }

        // The timer event hander looks at the _testResult member and takes appropriate action
        // Restart the timer so this will happen.
        _timer.start(0);
    }

}

//-------------------------------------------------------------------------
void QRtMidiSettings::accept()
{
    QSettings settings;
    settings.setValue("midi/inport-name",ui.midiInCombo->currentText());
    settings.setValue("midi/outport-name",ui.midiOutCombo->currentText());
    settings.sync();
    
    cleanup();
     
    QDialog::accept();
    emit midiAccept();
}

//-------------------------------------------------------------------------
void QRtMidiSettings::checkPortSelections()
{
    bool selectionsMade = (ui.midiInCombo->currentIndex() != -1 ) && (ui.midiOutCombo->currentIndex() != -1);
    
    ui.testButton->setEnabled(selectionsMade);
    ui.resultLabel->setStyleSheet("QLabel {background-color: rgb(255, 170, 0);}");
}

//-------------------------------------------------------------------------
void QRtMidiSettings::on_midiInCombo_currentIndexChanged()
{
    checkPortSelections();
}

//-------------------------------------------------------------------------
void QRtMidiSettings::on_midiOutCombo_currentIndexChanged()
{
    checkPortSelections();
}

//-------------------------------------------------------------------------
void QRtMidiSettings::updateTestResult()
{
    //If the timer is not active, the timeout has happened, so we can ignore.
    if(!_timer.isActive())
        return;

    _timer.stop();
    qDebug() << "updateTestResult - timer has stopped";
    if(_testResult == QRtTestResults::Success)
    {
        ui.resultLabel->setText("success");
        ui.resultLabel->setStyleSheet("QLabel {background-color: rgb(85, 170, 0);}");
    }
    else if(_testResult == QRtTestResults::UnknownDevice)
    {
        ui.resultLabel->setText("unknown");
        ui.resultLabel->setStyleSheet("QLabel {background-color: rgb(170, 0, 0);}");

    }
    else if(_testResult == QRtTestResults::NotFoundTimeout)
    {
        ui.resultLabel->setText("no response");
        ui.resultLabel->setStyleSheet("QLabel {background-color: rgb(170, 0, 0);}");
    }
}


//-------------------------------------------------------------------------
QString QRtMidiSettings::getInPortName()
{
    QSettings settings;
    return settings.value("midi/inport-name").toString();
}

//-------------------------------------------------------------------------
QString QRtMidiSettings::getOutPortName()
{
    QSettings settings;
    return settings.value("midi/outport-name").toString();
}

//-------------------------------------------------------------------------
void QRtMidiSettings::reject()
{
    cleanup();

    QDialog::reject();
    emit midiReject();
 }

//-------------------------------------------------------------------------
void QRtMidiSettings::cleanup()
{
     QObject::disconnect(&_midiIn, &QRtMidiIn::dataIn, 0,0); 
    _midiIn.close();
    _midiOut.close();
}

//-------------------------------------------------------------------------
bool QRtMidiSettings::hasDevSupport( const QRtMidiData &data )
{
 
    // TODO: supported devices shall be defined outside of this module and added to the test class
    // externally then kept in a list.
    // For now - we hard code the constance.
//     
//     const char* TimeLineIdent = "F0 7E XX 06 02 00 01 55 12 00 01";  // Partial TimeLine Identity Response
//     const char* MobiusIdent   = "F0 7E XX 06 02 00 01 55 12 00 02";    // Partial Mobius Identity Response 

    QSetIterator<const char*> i(_supportSet);
    while (i.hasNext())
    {
        if(data.contains(i.next()))
        {
            return true;
        }
    }
    return false;
// 
//     return data.contains(TimeLineIdent) || data.contains(MobiusIdent);
}

//-------------------------------------------------------------------------
void QRtMidiSettings::addSupportedIdentity( const char* id )
{
   _supportSet.insert(id);
}





