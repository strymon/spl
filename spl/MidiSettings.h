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
#pragma once

#include <QDialog>
#include <QTimer>
#include <QSet>

#include "QRtMidi/QRtMidiIn.h"
#include "QRtMidi/QRtMidiOut.h"

#include "QRtMidi/QRtMidiIdent.h"

#include "ui_MidiPortSelect.h"


namespace QRtTestResults
{
    enum Result 
    {
        Success,          // A supported device was found
        UnknownDevice,    // Somethings there, but it's not supported
        NotFoundTimeout,  // Nothing came back
        PortBusy          // Can't open port
    };
}

class MidiSettings : public QDialog
{
    Q_OBJECT

public:
    MidiSettings(QWidget *parent = 0);

    QString getInPortName();
    QString getOutPortName();
    void addSupportedIdentity( const char* id );
    
    // Add a set of devices that, if found, will cause the test
    // result to turn green.
    void addSupportedIdentities( QSet<const char*> &devSet);

signals:
    void midiReject();
    void midiAccept();

public slots:
    void accept();
    void reject();

    void cleanup();

    void on_testButton_clicked();
    void on_midiInCombo_currentIndexChanged(int);
    void on_midiOutCombo_currentIndexChanged(int);
    void updateTestResult();
    void recvDataForTest(const QRtMidiData &data);

    bool hasDevSupport( const QRtMidiData &data );
    
    // Return true if a MIDI in and MIDI out device is selected
    bool isPortPairSelected();

protected:
    void checkPortSelections();


    void showEvent(QShowEvent *e);
    

private:

    QRtMidiIn  _midiIn;
    QRtMidiOut _midiOut;
    QSet<const char*> _supportSet;

    QRtMidiIdentList_t _unkownDevList;

    QRtTestResults::Result _testResult;
    QTimer _timer;

    Ui_MidiSelectDialog ui;
};


/*
//-------------------------------------------------------------------------
void DcPresetLib::showMidiPortSelectDialog()
{


    // Get access to the dialog controls
    QComboBox* cb_out       = dialog->findChild<QComboBox*>("midiOutCombo");
    QComboBox* cb_in        = dialog->findChild<QComboBox*>("midiInCombo");
    QPushButton* testbtn    = dialog->findChild<QPushButton*>("testButton");
    QPushButton* okbtn      = dialog->findChild<QPushButton*>("okButton");
    QPushButton* cancelbtn  = dialog->findChild<QPushButton*>("cancelButton");
    QLabel* resultLabel     = dialog->findChild<QLabel*>("resultLabel");

    // Populate the MIDI port combo boxes
    //         cb_out->addItems(_midiIo->getOutDevNames());
    //         cb_in->addItems(_midiIo->getInDevNames());

    // Connect the 'test' button
    QObject::connect(testbtn, &QPushButton::clicked,this, &DcPresetLib::midiDialog_testButton_clicked);
    QObject::connect(testbtn, &QPushButton::clicked,this, &DcPresetLib::midiDialog_testButton_clicked);
    QObject::connect(testbtn, &QPushButton::clicked,this, &DcPresetLib::midiDialog_testButton_clicked);

    // show
    dialog->setModal(true);
    dialog->show();
}*/

