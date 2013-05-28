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

#include "DcPresetLib.h"
#include <QTextStream>

#include "QRtMidi/QRtMidiData.h"
#include <QKeyEvent>

#include <QDir>
#include <QSysInfo>
#include <QStandardPaths>
#include <QSettings>
#include <QScrollBar>

#include <QFileDialog>
#include <QMessageBox>

// State machine
#include <QFinalState>
#include <QHistoryState>
#include <QSignalTransition>
#include "DcState.h"


// Animation
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QGraphicsOpacityEffect>

#include "QRtMidi/QRtMidiSettings.h"
#include "DcMidiDevDefs.h"
#include <QThread>
#include "RenameDialog.h"
#include "MoveDialog.h"
#include "RenameDialog.h"
#include "MoveDialog.h"
#include "DcListWidget.h"

#include <QTime>
#include <QDesktopServices>

#include <QDate>
#include <QUrl>

#include "ui_DcplAbout.h"
#include <QApplication>
#include "QRtMidi/QRtMidiDefs.h"

#include "DcConsoleForm.h"

#include <QDebug>
#include "DcFileDownloader.h"

#include <QProcess>


DcPresetLib::DcPresetLib(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    ui.actionOpen->setEnabled(false);
    ui.actionSave->setEnabled(false);
    ui.actionSave_One->setEnabled(false);
    ui.actionMove->setEnabled(false);
    ui.actionRename->setEnabled(false);
    ui.actionLoad_One->setEnabled(false);
    
    setupFilePaths();
    readSettings();

    _lastErrorMsg.setString(&_lastErrorMsgStr);
    
    _midiSettings = new QRtMidiSettings(this);

    _workListControls << ui.renameButton << ui.moveButton << ui.loadOneButton << ui.saveOneButton;
    _workListActions  << ui.actionSave_One << ui.actionLoad_One << ui.actionRename << ui.actionMove;

    _maxPresetCount = 200;
    _presetOffset = 0;

    installEventFilter(this);

    QObject::connect(&_watchdog_timer,&QTimer::timeout,this,&DcPresetLib::devIdWatchDog);

    _progressDialog = new IoProgressDialog(this);
    _progressDialog->setModal(true);

    ui.mainToolBar->hide();
    this->setWindowIcon(QIcon(":/images/res/dcpm_256x256x32.png"));
  
    setupConsole();
    
    ui.fetchButton->setFocus();
    
    // Do the rest of the application startup in this timer callback
    QTimer::singleShot(10, this, SLOT(setupStateMachine()));
}

//-------------------------------------------------------------------------
DcPresetLib::~DcPresetLib()
{
    delete _midiSettings;
    delete _progressDialog;
}

//-------------------------------------------------------------------------
void DcPresetLib::dclog( const QString str )
{
    //*_con << str << "\n";
    //qDebug() << str << "\n";
}

//-------------------------------------------------------------------------
bool DcPresetLib::eventFilter(QObject* obj, QEvent *e)
{
    Q_UNUSED(obj);

    if (e->type() == QEvent::ShortcutOverride)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
        if (keyEvent->key() == Qt::Key_QuoteLeft)
        {
            _con->toggleVisible();
            e->accept();
            return true;
        }
    }

    return false;
}

//-------------------------------------------------------------------------
void DcPresetLib::closeEvent( QCloseEvent *e )
{
    writeSettings();

    if(_dirtyItemsIndex.length())
    {
        if (QMessageBox::Discard == QMessageBox::question(this, "Unsaved Worklist Warning",
            "You have unsaved presets in the Worklist.\nAre you sure you want to quit?",
                                                      QMessageBox::Discard | QMessageBox::Abort,
                                                                      QMessageBox::Abort))
        {
            e->accept();
            _con->execCmd("defsv default_defs.bin");
        }
        else
        {
             e->ignore();
// Crappy Workaround for QTBUG 30899
// SEE: https://bugreports.qt-project.org/browse/QTBUG-30899
             QTimer::singleShot(0,this,SLOT(hide()));
             QTimer::singleShot(100,this,SLOT(show()));
        }
    }
    else
    {
         e->accept();
         _con->execCmd("defsv default_defs.bin");
    }

   
}

//-------------------------------------------------------------------------
void DcPresetLib::testRtMidiData()
{
    static const char* tdata = "0xF01233";
    QRtMidiData mdt1("F0F67FF7");
    QRtMidiData mdt2("F0 F6 7F F7");

    bool t = mdt1 == mdt2;
    t = mdt2 == "F0F67FF7";
    t = mdt2 == "F0F67F";
    t = mdt1 == "F0 F6 7F F7";
    t = mdt1 == "0xF0 0xF6 0x7F 0xF7";
    QRtMidiData mdt3(tdata);
    QRtMidiData foo;
    foo.setData("F0 vv vv F7",0xa, 0x12,3);
    qDebug() << foo.toString();
    foo.setData("F0 p14 F7",0x222);
    qDebug() << foo.toString();
    foo.setData("F0 p14 F7",0x222);
    qDebug() << foo.toString();
    foo.setData("F0 p14 vv F7",0x222,0xa5);
    qDebug() << foo.toString();
    foo.setData("F0 mstr F7","7F 55 02");
    qDebug() << foo.toString();

    foo.setData("F0 cstr F7","AABB");
    qDebug() << foo.toString();

}

//-------------------------------------------------------------------------
void DcPresetLib::midiPortSelect_enter()
{
    shutdownMidiIo();
    _midiSettings->exec();
}

//-------------------------------------------------------------------------
void DcPresetLib::setupFilePaths()
{
    QSettings settings;
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/spl/";
    
    _dataPath = settings.value("data/location",defaultPath).toString();
    QString baseBackupPath = settings.value("backup/location",defaultPath).toString();
    
    QDir().mkpath(QDir::toNativeSeparators(_dataPath + "downloads/"));

    _backupEnabled = settings.value("backup/enabled",true).toBool();
    _worklistBackupPath = QDir::toNativeSeparators(baseBackupPath  + "worklist/");
    _devlistBackupPath = QDir::toNativeSeparators(baseBackupPath  + "device/");

    if(_backupEnabled)
    {
        QDir().mkpath(_worklistBackupPath);
        QDir().mkpath(_devlistBackupPath);
    }

    QString qsspath = QDir::toNativeSeparators(_dataPath + "qss/");
    QDir().mkpath(qsspath);

}

//-------------------------------------------------------------------------
void DcPresetLib::writeSettings()
{
    QSettings settings;
    settings.beginGroup("mainwindow");
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.endGroup();
}

//-------------------------------------------------------------------------
void DcPresetLib::readSettings()
{
    QSettings settings;

    settings.beginGroup("mainwindow");
    resize(settings.value("size", QSize(560, 700)).toSize());
    move(settings.value("pos", QPoint(560, 200)).toPoint());
    settings.endGroup();
}

//-------------------------------------------------------------------------
void DcPresetLib::detectDevice_entered()
{
    shutdownMidiIo();

    QString in_port = _midiSettings->getInPortName();
    QString out_port = _midiSettings->getOutPortName();
    
    bool success = false;
    
    _devDetails.clear();

    if(!in_port.isEmpty() && !out_port.isEmpty())
    {
        _midiIn.init();
        _midiOut.init();
        // Setup to receive identity data
        QObject::disconnect(&_midiIn, &QRtMidiIn::dataIn, &_xferInMachine, &DcXferMachine::replySlotForDataIn);
        QObject::disconnect(&_midiIn, &QRtMidiIn::dataIn, &_xferOutMachine, &DcXferMachine::replySlotForDataOut);
        
        if(_midiIn.open( in_port) && _midiOut.open( out_port ))
        {
            QThread::msleep(25);
            QObject::connect(&_midiIn, &QRtMidiIn::dataIn, this, &DcPresetLib::recvIdData);    

            // Send global Identify Request
            _midiOut.dataOut("F0 7E 7F 06 01 F7");
            
            // Devices only have 2 seconds to reply with the identity information
            _watchdog_timer.setInterval(2000);
            _watchdog_timer.start();

            success = true;
        }
        else
        {
            _midiIn.close();
            _midiOut.close();
        }
    }

    if(!success)
    {
        emit deviceNotFound();
    }

}

//-------------------------------------------------------------------------
// This singleShot slot is used to detect a timeout during the detectDevice
// state.  It will always run, but the timeout is avoided if the recvIdData
// runs first. 
void DcPresetLib::devIdWatchDog()
{
    _watchdog_timer.stop();

    // If this is not set, then we timed out
    if(_devDetails.isEmpty())
    {
        QObject::disconnect(&_midiIn, &QRtMidiIn::dataIn, this, &DcPresetLib::recvIdData);
        emit deviceNotFound();
    }
}
//-------------------------------------------------------------------------
void DcPresetLib::recvIdData( const QRtMidiData &data )
{
    // Ignore any message that's not an MIDI Identity reply
    if(!data.contains(kIdentReply))
        return;
    
    // Shutdown the watchdog timer and disconnect this handler from the MIDI IN port
    _watchdog_timer.stop();
    QObject::disconnect(&_midiIn, &QRtMidiIn::dataIn, this, &DcPresetLib::recvIdData);
    
    // Clear device specific details
    _devDetails.clear();
    _con->clearRoSymDefs();

    // Assume a supported device is found.
    bool foundSupportedDevice = true;
    
    // Parse the Identify response

/*  
       0  0xF0  SOX
       1  0x7E  Non-Realtime
       2  0x7F  The SysEx channel. Could be from 0x00 to 0x7F.
       3  0x06  Sub-ID -- General Information
       4  0x02  Sub-ID2 -- Identity Reply

       5  0xID  Manufacturer's ID
          (ID is one byte for codes between 1 and 7F, if the 5th byte is zero
           there will be two more ID bytes, e.g. 01 55 for Strymon)
        /6 0xID1
        /7  0xID2

       6/8  0xf1  The f1 and f2 bytes make up the family code. Each
       7/9  0xf2  manufacturer assigns different family codes to his products.
        
       8/10  0xp1  The p1 and p2 bytes make up the model number. Each
       9/11  0xp2  manufacturer assigns different model numbers to his products.
        
      10/12  0xv1  The v1, v2, v3 and v4 bytes make up the version number.
      11/13  0xv2
      12/14  0xv3
      13/15  0xv4
      14/16  0xF7
*/
    QString version;

    // If the Manufacturer's Id is the optional 3 bytes, then adjust the 
    // byte position of the identity data by the extra two bytes.
     int adj = (data.at(5) == 0) ? 2 : 0;

    // See the comment above to understand the byte positions of this data
    _devDetails.Manufacture = data.mid(5,3);
    _devDetails.SyxCh       = data.at(2);
    _devDetails.Family      = data.mid(6+adj,2);
    _devDetails.Product     = data.mid(8+adj,2);
    _devDetails.ShortHdr    = _devDetails.Manufacture + _devDetails.Family.mid(0,1) + _devDetails.Product.mid(0,1);
    _devDetails.FwVersion.sprintf("%c.%c.%c.%c",data.at(10+adj),data.at(11+adj),data.at(12+adj),data.at(13+adj));

    // TODO: Put all the device details in a persistent Map of some kind allowing
    // for new devices to be supported.

    // TODO: _devDetails is just a quick structure to organize the device specific goo
    // to make the preset librarian work.  A structure is mostly overkill for this and perhaps this data
    // should remain as 'data' and therefor get stored as name/values in a map.
    // Examples of poor generalization:  Protocol details, such as "preset data byte offset" 
    // may only make since for a few products.  A method to hide these details, or to not need 
    // these details would be more useful.  The program needs to know how to "grab" preset data 
    // and work with it.  What's the best way to do this?

    QPixmap pm;

    if(data.contains(kTimeLineIdent))
    {
        _devDetails.Name                 = "TimeLine";
        pm = QPixmap(":/images/res/timeline_100.png");

        _devDetails.DevSOX               = QRtMidiData("F0") + _devDetails.ShortHdr;
        _devDetails.PresetHdr            = QRtMidiData("F0") + _devDetails.ShortHdr + "62";
        _devDetails.DataInNACK           = QRtMidiData("F0") + _devDetails.ShortHdr + "47 F7";
        _devDetails.ReadPresetTemplate   = "F0 " + _devDetails.ShortHdr.toByteArray(' ') + " 63 p14 F7";

        _devDetails.PresetSize           = 650;
        _devDetails.PresetsPerBank       = 2;
        _devDetails.PresetCount          = 200;
        _devDetails.BankCount            = 100;
        _devDetails.PresetDataByteOffset = 7;


    }
    else if(data.contains(kMobiusIdent) )
    {
        _devDetails.Name            = "Mobius";
        pm = QPixmap(":/images/res/mobius_100.png");

        _devDetails.DevSOX               = QRtMidiData("F0") + _devDetails.ShortHdr;
        _devDetails.PresetHdr            = QRtMidiData("F0") + _devDetails.ShortHdr + "62";
        _devDetails.DataInNACK           = QRtMidiData("F0") + _devDetails.ShortHdr + "47 F7";
        _devDetails.ReadPresetTemplate   = "F0 " + _devDetails.ShortHdr.toByteArray(' ') + " 63 p14 F7";

        
        _devDetails.PresetsPerBank       = 2;
        _devDetails.PresetSize           = 650;
        _devDetails.PresetCount          = 200;
        _devDetails.BankCount            = 100;
        _devDetails.PresetDataByteOffset = 7;
    }
    else
    {
        pm = QPixmap(":/images/res/devunknown_100.png");
        foundSupportedDevice = false;
    }
    
    ui.devInfoLabel->setText(_devDetails.Name);
    ui.devImgLabel->setToolTip(_devDetails.FwVersion);
    ui.devImgLabel->setPixmap(pm);

    if(foundSupportedDevice)
    {
        // Adding device specific details to the console
        _con->addRoSymDef("hdr",_devDetails.ShortHdr.toString(' '));
        _con->addRoSymDef("dev.ver",_devDetails.FwVersion);
        _con->addRoSymDef("dev.sox",_devDetails.DevSOX.toString(' '));
        _con->addRoSymDef("dev.name",_devDetails.Name);

        emit deviceReady();
    }
    else
    {
        emit deviceNotFound();
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::userCanFetch_entered()
{
    if(_workListDataDeviceUid.isEmpty() || _devDetails.getUid() != _workListDataDeviceUid)
    {
        if(_dirtyItemsIndex.length())
        {
            if (QMessageBox::Save == QMessageBox::question(this, "The worklist was not inSync with the device",
                "Would you like to save the changes?", QMessageBox::Save|QMessageBox::No))
            {
                on_actionSave_triggered();
            }
        }
        
        appDataClear();
    }
    else
    {
        // There's data in the work list, goto the preset edit state
        _machine.postEvent(new FetchDataExistsEvent());
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::setupStateMachine()
{
    DcState::SetTraceStringList(&_stateTrace);
    
    _machine.setGlobalRestorePolicy(QStateMachine::RestoreProperties);
    DcState *detectDevice       = new DcState(QString("detectDevice"));

    DcState *userCanFetch       = new DcState(QString("userCanFetch"));
    DcState *setupReadPresetsState     = new DcState(QString("setupReadPresetsState"));
    DcState *readPresetsCompleteState = new DcState(QString("readPresetsCompleteState"));

    DcState *midiPortSelect  = new DcState(QString("midiPortSelect"));
    DcState *noDevice        = new DcState(QString("noDevice"));

    // Preset Edit State
    DcState *presetEdit = new DcState(QString("presetEdit"),QState::ParallelStates);

    DcState *s11                        = new DcState(QString("s11"), presetEdit);
    DcState *inSyncState                = new DcState(QString("inSyncState"), s11);
    DcState *notInSyncState             = new DcState(QString("notInSyncState"), s11);
    DcState *setupWritePresetsState     = new DcState(QString("setupWritePresetsState"),s11);
    DcState *writePresetsCompleteState   = new DcState(QString("writePresetsCompleteState"),s11);

    s11->setInitialState(inSyncState);

    DcState *s12             = new DcState(QString("s12"),presetEdit);
    DcState *workListClean   = new DcState(QString("workListClean"),s12);
    DcState *workListDirty   = new DcState(QString("workListDirty"),s12);
    s12->setInitialState(workListClean);
    //QFinalState* cancelIoState = new QFinalState(presetEdit);


    DcState *procDataIn = new DcState(QString("procDataIn"),QState::ExclusiveStates);
    DcState *procDataIn_sendNext = new DcState(QString("procDataIn_sendNext"),procDataIn);
    procDataIn->setInitialState(procDataIn_sendNext);

    _machine.addState(detectDevice);

    _machine.addState(userCanFetch);
    _machine.addState(setupReadPresetsState);
    _machine.addState(readPresetsCompleteState);

    _machine.addState(midiPortSelect);
    _machine.addState(noDevice);
    _machine.addState(procDataIn);
    _machine.addState(presetEdit);

    // app Startup
    QObject::connect(detectDevice, SIGNAL(entered()), this, SLOT(detectDevice_entered()));
    
    detectDevice->addTransition(this,SIGNAL(deviceReady()),     userCanFetch);
    detectDevice->addTransition(this,SIGNAL(deviceNotFound()),  noDevice);
    detectDevice->addTransition(ui.actionMIDI_Ports,SIGNAL(triggered()),midiPortSelect);
    
    // Preset Edit UI properties
    presetEdit->assignProperty(ui.fetchButton, "enabled", true);
    presetEdit->assignProperty(ui.workList, "enabled", true);
    presetEdit->assignProperty(ui.deviceList, "enabled", true);
    presetEdit->assignProperty(ui.openButton, "enabled", true);
    presetEdit->assignProperty(ui.saveButton, "enabled", true);
    presetEdit->assignProperty(ui.actionOpen, "enabled", true);
    presetEdit->assignProperty(ui.actionSave, "enabled", true);

    notInSyncState->assignProperty(ui.syncButton, "enabled", true);
    notInSyncState->assignProperty(ui.actionSync, "enabled", true);


    userCanFetch->assignProperty(ui.fetchButton,"enabled",true);
    userCanFetch->assignProperty(ui.actionFetch,"enabled",true);


    // noDevice
    QObject::connect(noDevice, SIGNAL(entered()), this, SLOT(noDevice_entered()));
    noDevice->addTransition(ui.actionMIDI_Ports,SIGNAL(triggered()),midiPortSelect);
    
    // midiPortSelect
    QObject::connect(midiPortSelect, SIGNAL(entered()), this, SLOT(midiPortSelect_enter()));
    // No matter what, we always check for the device after this state
    midiPortSelect->addTransition(_midiSettings,SIGNAL(midiReject()),detectDevice);
    midiPortSelect->addTransition(_midiSettings,SIGNAL(midiAccept()),detectDevice);

    QObject::connect(userCanFetch, SIGNAL(entered()), this, SLOT(userCanFetch_entered()));
    userCanFetch->addTransition(ui.actionMIDI_Ports,SIGNAL(triggered()),midiPortSelect);
    userCanFetch->addTransition(ui.actionFetch,SIGNAL(triggered()),setupReadPresetsState);
    
    DcCustomTransition* ucf = new DcCustomTransition(FetchDataExistsEvent::TYPE,readPresetsCompleteState);
    ucf->setTargetState(presetEdit);
    userCanFetch->addTransition(ucf);
    
    // Setup the dataIn system
    DcState* xferInState = _xferInMachine.setupStateMachine(&_machine,&_midiOut,readPresetsCompleteState,detectDevice,detectDevice);
    
    // Setup the dataOut system
    DcState* xferOutState = _xferOutMachine.setupStateMachine(&_machine,&_midiOut,writePresetsCompleteState,detectDevice,detectDevice);
    
    // State readPreset will call setupReadPresetXfer and transition to the xferMachine
    QObject::connect(setupReadPresetsState, SIGNAL(entered()), this, SLOT(setupReadPresetXfer_entered())); // IN
    setupReadPresetsState->addTransition(this,SIGNAL(readPresets_setupDone_signal()),xferInState); // OUT - procDataIn

    QObject::connect(readPresetsCompleteState, SIGNAL(entered()), this, SLOT(readPresetsComplete_entered()));
    DcCustomTransition* wpt = new DcCustomTransition(FetchCompleteSuccessEvent::TYPE,readPresetsCompleteState);
    wpt->setTargetState(presetEdit);
    readPresetsCompleteState->addTransition(wpt);

    QObject::connect(writePresetsCompleteState, SIGNAL(entered()), this, SLOT(writePresetsComplete_entered()));
    wpt = new DcCustomTransition(WriteCompleteSuccessEvent::TYPE,writePresetsCompleteState);
    wpt->setTargetState(presetEdit);
    writePresetsCompleteState->addTransition(wpt);
    
    // Xfer Done - edit presets
    QObject::connect(presetEdit, SIGNAL(entered()), this, SLOT(presetEdit_entered())); // IN
    presetEdit->addTransition(ui.actionMIDI_Ports,SIGNAL(triggered()),midiPortSelect);
    presetEdit->addTransition(ui.fetchButton,SIGNAL(clicked()),setupReadPresetsState);

    DcCustomTransition *dct = new DcCustomTransition(WorkListDirtyEvent::TYPE,workListClean);
    dct->setTargetState(workListDirty);
    workListClean->addTransition(dct);

    dct = new DcCustomTransition(WorkListIsDifferentThanDeviceListEvent::TYPE,inSyncState);
    dct->setTargetState(notInSyncState);
    inSyncState->addTransition(dct);
    
    dct = new DcCustomTransition(WorkListIsSameAsDeviceListEvent::TYPE,notInSyncState);
    dct->setTargetState(inSyncState);
    notInSyncState->addTransition(dct);

    notInSyncState->addTransition(ui.actionSync,SIGNAL(triggered()),setupWritePresetsState);
    notInSyncState->assignProperty(ui.syncButton,"enabled",true);
    notInSyncState->assignProperty(ui.actionSync,"enabled",true);
    
    QObject::connect(setupWritePresetsState, SIGNAL(entered()), this, SLOT(setupWritePresetXfer_entered())); // IN
    setupWritePresetsState->addTransition(this,SIGNAL(writePresets_setupDone_signal()),xferOutState); // OUT - procDataIn


    dclog("Starting state machine");
    _machine.setInitialState(detectDevice);
    _machine.start();
}

//-------------------------------------------------------------------------
void DcPresetLib::noDevice_entered()
{
    _con->clearRoSymDefs();
    QPixmap pm = QPixmap(":/images/res/devunknown_100.png");
    ui.devImgLabel->setPixmap(pm);
    ui.devInfoLabel->setText("");
    ui.devImgLabel->setToolTip("");
} 

//-------------------------------------------------------------------------
void DcPresetLib::setupReadPresetXfer_entered()
{
    dclog("STATE entered: readPresets");

    ui.deviceList->clear();
    ui.workList->clear();
    _workListDataDeviceUid.clear();

    _xferInMachine.reset();
    
    // Get the number of presets that will be transfered from the device
    int presetCount     = _maxPresetCount;
    int presetOffset    = _presetOffset;
    
    // Build a list of commands, the state machine will process each one in turn
    QRtMidiData cmd;
    for (int presetId = presetOffset; presetId < presetCount+presetOffset; presetId++)
    {
        cmd.setData(_devDetails.ReadPresetTemplate,presetId);
        _xferInMachine.append(cmd);
    }
    
    // Setup to receive preset data
    QObject::disconnect(&_midiIn, &QRtMidiIn::dataIn, this, &DcPresetLib::recvIdData);
    QObject::disconnect(&_midiIn, &QRtMidiIn::dataIn, &_xferInMachine, &DcXferMachine::replySlotForDataIn);
    QObject::disconnect(&_midiIn, &QRtMidiIn::dataIn, &_xferOutMachine, &DcXferMachine::replySlotForDataOut);
    QObject::connect(&_midiIn, &QRtMidiIn::dataIn, &_xferInMachine, &DcXferMachine::replySlotForDataIn);

    _xferInMachine.setProgressDialog(_progressDialog);

    _xferInMachine.go();

    // Signal the state machine to begin
    emit readPresets_setupDone_signal();
}

//-------------------------------------------------------------------------
void DcPresetLib::setupWritePresetXfer_entered()
{
    // backup the work list
    backupWorklist();

    _xferOutMachine.reset();

    // Get the number of dirty presets
    int presetCount = _dirtyItemsIndex.length();

    
    for (int idx = 0; idx < presetCount; idx++)
    {
        int pid = _dirtyItemsIndex.at(idx);
        QRtMidiData md = _workListData.at(pid);
        // Update the work list too.
        _deviceListData[pid] = md;
        
        // xferOutMachine will keep a list of the MIDI commands and process them
        // when 'go' is invoked
        _xferOutMachine.append(md);
    }

    QObject::disconnect(&_midiIn, &QRtMidiIn::dataIn, this, &DcPresetLib::recvIdData);
    QObject::disconnect(&_midiIn, &QRtMidiIn::dataIn, &_xferInMachine, &DcXferMachine::replySlotForDataIn);
    QObject::disconnect(&_midiIn, &QRtMidiIn::dataIn, &_xferOutMachine, &DcXferMachine::replySlotForDataOut);

    QObject::connect(&_midiIn, &QRtMidiIn::dataIn, &_xferOutMachine, &DcXferMachine::replySlotForDataOut);
    
    _xferOutMachine.setProgressDialog(_progressDialog);

    _xferOutMachine.go();
    
    // Signal the state machine to begin
    emit writePresets_setupDone_signal();

}

//-------------------------------------------------------------------------
void DcPresetLib::presetEdit_entered()
{
    Q_ASSERT(_deviceListData.length());
    
    checkSyncState();
    _machine.postEvent(new InPresetEditEvent());
}

//-------------------------------------------------------------------------
void DcPresetLib::on_workList_itemSelectionChanged()
{
    updateWorklistControlState();
}

//-------------------------------------------------------------------------
void DcPresetLib::on_actionMove_triggered()
{
    // To start out a move, a preset must be selected.  This shall be 
    // known as preset 'A.'  It is this moved to, or swapped with another 
    // preset location, called preset 'B'.

    int presetA_row = ui.workList->currentRow();
    if(presetA_row != -1)
    {
        int offsetToId = _devDetails.PresetDataByteOffset;

        QRtMidiData presetA = _workListData.at(presetA_row);
        int presetA_id = presetA.get14bit(offsetToId,-1);

        if(presetA_id == -1)
        {
            dclog("ERROR - the format of the data in the workList is corrupt or unexpected");
            return;
        }

        QString presetA_name = presetA.mid(kPresetNameOffset,kPresetNameLen);

        // Prepare the move dialog
        MoveDialog* m = new MoveDialog(this);
        m->setModal(true);
        m->setDestinations(presetListToBankPatchName(_workListData));
        m->setTargetPreset(ui.workList->item(presetA_row)->text());
        
        if(QDialog::Accepted ==  m->exec())
        {
            // Get the Row, Data and ID of preset B
            int             presetB_row     = m->getDestIdx();


            QRtMidiData     presetB_data    = _workListData.at(presetB_row);
            int             presetB_id      = presetB_data.get14bit(offsetToId ,-1);


            if(presetB_id == -1)
            {
                dclog("ERROR - the format of the data in the workList is corrupt or unexpected");
                return;
            }

            if(m->swapClicked())
            {
                // Swap presets A and B
                presetB_data.set14bit(offsetToId,presetA_id);
                presetA.set14bit(_devDetails.PresetDataByteOffset,presetB_id);
                _workListData.replace(presetA_row,presetB_data);
                _workListData.replace(presetB_row,presetA);
            }
            else
            {
                // Replace preset B with preset A
                presetA.set14bit(_devDetails.PresetDataByteOffset,presetB_id); 
                _workListData.replace(presetB_row,presetA);
                _dirtyItemsIndex.append(presetB_row);
            }
            
            checkSyncState();
            _machine.postEvent(new WorkListDirtyEvent());
        }

        if(m)
        {
            delete m;
        }
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::on_actionRename_triggered()
{
    int row = ui.workList->currentRow();
    if(row != -1)
    {
        QRtMidiData md = _workListData.at(row);
        QString origName = md.mid(kPresetNameOffset,kPresetNameLen);
        RenameDialog* w = new RenameDialog(this);
        w->setModal(true);
        w->setName(origName.trimmed());
        if(QDialog::Accepted ==  w->exec())
        {
            int sum = md.sumOfSection(kPresetDataOffset,kPresetDataLength);
            //unsigned char chk = md.at(kPresetChecksumOffset);

            QByteArray ba = w->getName().toUtf8();
            ba = ba.leftJustified(16,' ');
            if(!origName.contains(ba.data()))
            {
                md.replace(kPresetNameOffset,kPresetNameLen,ba);
                sum = md.sumOfSection(kPresetDataOffset,kPresetDataLength);
                QByteArray chksum;
                chksum += sum;
                //itoa(sum,chksum,16);
                md.replace(kPresetChecksumOffset,1,QByteArray::fromHex(chksum.toHex()));
            
                // Update checksum
            
                _workListData.replace(row,md);

                checkSyncState();


                // Move the state machine to the unsynchronized/dirty state
                _machine.postEvent(new WorkListDirtyEvent());
            }
        }
        if(w)
        {
            delete w;
        }
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::on_workList_itemDoubleClicked()
{
     QMetaObject::invokeMethod(ui.renameButton, "click",Qt::DirectConnection);
}

//-------------------------------------------------------------------------
void DcPresetLib::drawWorklist()
{
    // Update the names in the work list
    QStringList names = presetListToBankPatchName(_workListData);

    // Update the name list 
    ui.workList->clear();
    ui.workList->addItems(names);
    
    // Redraw the modified items
    for (int i = 0; i < _dirtyItemsIndex.size(); ++i)
    {
        int ridx = _dirtyItemsIndex.at(i);
        
        // Indicate the list is modified
        QFont font = ui.workList->item(ridx)->font();
        font.setItalic(true);
        ui.workList->item(ridx)->setFont(font);
        ui.workList->item(ridx)->setForeground(Qt::red);
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::writePresetsComplete_entered()
{
    syncWorklistToDevList();
    _machine.postEvent(new WriteCompleteSuccessEvent());
}

//-------------------------------------------------------------------------
void DcPresetLib::readPresetsComplete_entered()
{
    _deviceListData = _xferInMachine.getDataList();
    // Keep track of what device this data was for
    _workListDataDeviceUid = _devDetails.getUid();

    // Now, create a backup of this fresh data
    backupDeviceList();

    syncWorklistToDevList();

    _machine.postEvent(new FetchCompleteSuccessEvent());
}

QString DcPresetLib::presetNumberToBankNum(int i)
{
    QString num;
    int presetsPerBank = _devDetails.PresetsPerBank ? _devDetails.PresetsPerBank : 2;

    char inBankAlpha = 'A' + i%presetsPerBank;
    int bank = i/presetsPerBank;
    return num.sprintf("%02d%c - ",bank,inBankAlpha);
    //return num.sprintf("%02d%c - ",i/2,i&1?'B':'A');
}

QString DcPresetLib::presetToBankNum(const QRtMidiData& preset )
{
    int presetId = preset.get14bit(_devDetails.PresetDataByteOffset,-1);
    return presetNumberToBankNum(presetId);
}

//-------------------------------------------------------------------------
QStringList DcPresetLib::presetListToBankPatchName(QList<QRtMidiData>& listData)
{
    QStringList names;
    for (int i = 0; i < listData.size(); ++i) 
    {
        QRtMidiData presetData = listData.at(i);

        names << presetToBankNum(presetData) + presetData.mid(kPresetNameOffset,kPresetNameLen);
    }

    return names;
}

//-------------------------------------------------------------------------
QString DcPresetLib::presetToName(QRtMidiData& p)
{
    return p.mid(kPresetNameOffset,kPresetNameLen).trimmed();
}

//-------------------------------------------------------------------------
QString DcPresetLib::presetToBankPatchName(QRtMidiData& p)
{
    return presetToBankNum(p) + presetToName(p);
}

//-------------------------------------------------------------------------
void DcPresetLib::on_actionLoad_One_triggered()
{
    QSettings settings;
    QString lastOpened = settings.value("lastSinglePreset","presets.syx").toString();

    // Is anything selected, if not, that's a programming error as this action should be 
    // disabled until a selection
    int cur_idx = ui.workList->currentRow();
    if(cur_idx < 0)
    {
        qWarning() << "Load on has no selection\n";
        return;
    }

    // Save the full working buffer

#ifdef Q_OS_MACX
    QStringList filenames;
    QString fileName;
    QString path = lastOpened;
    QFileDialog dialog(0,"Load One Preset");
    dialog.setDefaultSuffix("syx");
    dialog.setDirectory(path);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    //dialog.setOption(QFileDialog::DontConfirmOverwrite);
    if (dialog.exec())
        filenames = dialog.selectedFiles();
    if(filenames.size() == 1)
        fileName = filenames.first();
#else
    QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open Presets"), lastOpened, tr("Preset File (*.syx)"));
#endif

    if(!fileName.isEmpty())
    {
        QRtMidiData md;
        if(!loadPresetBinary(fileName,md))
        {
            dispLastErrorMsgBox();
        }
        else
        {
            // Get the preset number from the occupant of cur_idx in the worklist
            int presetNum = _workListData[cur_idx].get14bit(_devDetails.PresetDataByteOffset,-1);
            if(presetNum == -1)
            {
                Q_ASSERT(presetNum != -1);

                // Something is wrong with the preset in this location, just use the slot number,
                // it will probably be the same anyway.  THe above code is in place if we ever support
                // presets list that do not start at zero.
                presetNum = cur_idx;
            }

            md.set14bit(_devDetails.PresetDataByteOffset,presetNum);

            // Was this preset a change of the work-list?
            if(_workListData[cur_idx] != md)
            {
                _workListData[cur_idx] = md;
                checkSyncState();
            }
        }
        settings.setValue("lastSinglePreset",fileName);
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::on_actionSave_One_triggered()
{
    QSettings settings;
    QString lastSaveDir;

    QString lastSave = settings.value("lastSinglePreset","onepreset.syx").toString();
    if(lastSave.isEmpty())
    {
            lastSaveDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    else
    {
        QDir qDir;
        qDir.setPath(lastSave);
        lastSaveDir = qDir.dirName();
    }

    int curIdx = ui.workList->currentRow();
    if(curIdx < 0)
    {
        // Nothing is selected, this is badness.
        dclog("Save_One::nothing selected - error");
        return;
    }
    
    QRtMidiData md = _workListData.at(curIdx);
    QString name = presetToName(md) + QString(".") + _devDetails.Name.toLower() + QString(".syx");
    name = name.replace(" ","_");
    name = lastSaveDir + "/" + name;
    name = QDir().cleanPath(name);

    // Save the full working buffer
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save One Presets"), name, tr("Preset File (*.syx)"));
    

    if(!fileName.isEmpty())
    {
        if(!savePresetBinary(fileName,md))
        {
            dispLastErrorMsgBox();
        }
        settings.setValue("lastSinglePreset",fileName);
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::on_actionSave_triggered()
{
    QSettings settings;
    QString lastSave = settings.value("lastPresetBundleSave","presets.syx").toString();

    // Save the full working buffer
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Presets"), lastSave, tr("Preset File(*.syx)"));

    if(!fileName.isEmpty())
    {
        if(!savePresetBinary(fileName,_workListData))
        {
            dispLastErrorMsgBox();
        }
        settings.setValue("lastPresetBundleSave",fileName);
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::on_actionOpen_triggered()
{
    QSettings settings;
    QString lastOpened = settings.value("lastPresetBundleOpened","presets.syx").toString();
    
    QString fileName = execOpenDialog(lastOpened);

    if(!fileName.isEmpty())
    {
        
        QList<QRtMidiData> newPresetList;
        if(!loadPresetBinary(fileName,newPresetList))
        {
            dispLastErrorMsgBox();
        }
        else
        {
           if(newPresetList.length() < _maxPresetCount)
           {
               _lastErrorMsgStr.clear();
               _lastErrorMsg <<  "Operation will be aborted.\nThe preset file does not contain the correct number of presets.";
                
                dispLastErrorMsgBox();
                return;
           }

           _workListData = newPresetList;

           checkSyncState();

        }
        settings.setValue("lastPresetBundleOpened",fileName);
    }
}

//-------------------------------------------------------------------------
bool DcPresetLib::loadPresetBinary(const QString &fileName,QList<QRtMidiData>& dataList)
{ 
    _lastErrorMsgStr.clear();

    QFile file(fileName); 

    if (!file.open(QIODevice::ReadOnly)) 
    { 
        _lastErrorMsg << "Unable to open the file:\n" << fileName;
        return false; 
    } 
    
    QDataStream in(&file); 
    
    // Check the file size, is it at least one preset in length
    if(file.size() < _devDetails.PresetSize)
    {
       _lastErrorMsg << "Preset file is too small.";
       return false;
    }

    // Load one preset in, see if it matches the current Manufacture
    QByteArray ba(_devDetails.PresetSize,'\0');

    while(ba.length() == in.readRawData(ba.data(),ba.length()))
    {
        QRtMidiData md(ba);
        if(!md.contains(_devDetails.PresetHdr))
        {
            _lastErrorMsg << "File does not contain " << _devDetails.Name << " presets";
            return false;
        }
        // This is a good preset file
        dataList.append(md);
    }

    file.close();

    return true; 
}

//-------------------------------------------------------------------------
// Load one preset
bool DcPresetLib::loadPresetBinary( const QString &fileName,QRtMidiData& md )
{
    _lastErrorMsgStr.clear();
    QFile file(fileName); 

    if (!file.open(QIODevice::ReadOnly)) 
    { 
        _lastErrorMsg << "Unable to open the file:\n" << fileName;
        return false; 
    } 

    QDataStream in(&file); 

    // Check the file size, is it at least one preset in length
    if(file.size() < _devDetails.PresetSize)
    {
        _lastErrorMsg << "Preset file is too small.";
        return false;
    }

    // Is the file too big?
    if(file.size() > _devDetails.PresetSize)
    {
        _lastErrorMsg << "File has more than one preset.";
        return false;
    }

    QByteArray ba(_devDetails.PresetSize,'\0');
    
    in.readRawData(ba.data(),ba.length());

     md = ba;

     if(!md.contains(_devDetails.PresetHdr))
     {
         _lastErrorMsg << "File does not contain " << _devDetails.Name << " presets";
         return false;
     }

    file.close();

    return true; 
}

bool DcPresetLib::savePresetBinary(const QString &fileName,const QList<QRtMidiData>& dataList)
{ 
    _lastErrorMsgStr.clear();
    QFile file(fileName); 

    if (!file.open(QIODevice::WriteOnly)) 
    { 
        _lastErrorMsg << "Unable to open the file:\n" << fileName;
        return false; 
    } 

    QDataStream out(&file);   // we will serialize the data into the file
    
    for (int i = 0; i < dataList.size(); ++i) 
    {
        const QByteArray& ba = dataList.at(i).toByteArray();
        out.writeRawData(ba.data(),ba.length());
    }

    file.close();

    return true; 
}

//-------------------------------------------------------------------------
bool DcPresetLib::savePresetBinary( const QString &fileName,const QRtMidiData& md )
{
    _lastErrorMsgStr.clear();
    QFile file(fileName); 

    if (!file.open(QIODevice::WriteOnly)) 
    { 
        _lastErrorMsg << "Unable to open the file:\n" << fileName;
        return false; 
    } 
    QDataStream out(&file);   // we will serialize the data into the file
    out.writeRawData(md.data(),md.length());
    file.close();

    return true; 

}

void DcPresetLib::backupDeviceList()
{
    if(_backupEnabled && _deviceListData.length() == _devDetails.PresetCount)
    {
        QString t = QTime::currentTime().toString("'_'hhmmss'.syx'");
        QString filename = QDate::currentDate().toString("'_dl_'yy_MM_dd");
        QString fullPath = _devlistBackupPath + _devDetails.Name.toLower() + "_" + filename + t;

        if(QDir().exists(_devlistBackupPath))
        {
            savePresetBinary(fullPath,_deviceListData);  
        }
    }

}
void DcPresetLib::backupWorklist()
{
    // Only backup full preset files
    if(_backupEnabled && (_workListData.length() == _devDetails.PresetCount))
    {
        QString t = QTime::currentTime().toString("'_'hhmmss'.syx'");
        QString filename = QDate::currentDate().toString("'_wl_'yy_MM_dd");
        QString fullPath = _worklistBackupPath + _devDetails.Name.toLower() + "_" + filename + t;

        if(QDir().exists(_worklistBackupPath))
        {
            savePresetBinary(fullPath,_workListData);  
        }
    }
}

//-------------------------------------------------------------------------
bool DcPresetLib::checkSyncState()
{
    _dirtyItemsIndex.clear();

    // compare the work list with the device list and update the dirty list
    int len = qMin(_workListData.length(),_deviceListData.length());
    for (int i = 0; i < len; ++i) 
    {
        if(_workListData.at(i).toByteArray() != _deviceListData.at(i).toByteArray())
        {
            _dirtyItemsIndex.append(i);
        }
    }

    drawWorklist();
    bool isInSync = true;
    if(_dirtyItemsIndex.length() > 0)
    {
        _machine.postEvent(new WorkListIsDifferentThanDeviceListEvent());
        isInSync  = false;
    }
    else
    {
        _machine.postEvent(new WorkListIsSameAsDeviceListEvent());
    }

    return isInSync;
}

//-------------------------------------------------------------------------
void DcPresetLib::dispLastErrorMsgBox()
{
    QMessageBox* msgBox = new QMessageBox(this);

    msgBox->setText(_lastErrorMsgStr);
    msgBox->setWindowTitle("Error");

    int x = pos().x();
    int y = pos().y();
    x += (width()/2) - msgBox->width();
    y += (height()/2) - msgBox->height();

    msgBox->move(x,y);
    msgBox->setIcon(QMessageBox::Warning);
    msgBox->exec();
    delete msgBox;
}

//-------------------------------------------------------------------------
void DcPresetLib::on_actionAbout_triggered()
{
    Ui_DcAboutDialog abtui;
    QDialog* abt = new QDialog(this);
    abtui.setupUi(abt);
    abt->setWindowIcon(QIcon(":/images/res/dcpm_256x256x32.png"));
    abtui.label->setText("Version " + QApplication::applicationVersion());
    abt->exec();
    delete abt;

}

//-------------------------------------------------------------------------
void DcPresetLib::updateWorklistControlState()
{
    bool sel = ui.workList->currentRow() != -1;
    for (int i = 0; i < _workListControls.length() ; i++)
    {
        _workListControls[i]->setEnabled(sel);

    }

    for (int i = 0; i < _workListActions.length() ; i++)
    {
        _workListActions[i]->setEnabled(sel);
    }
}


//-------------------------------------------------------------------------
QString  DcPresetLib::execOpenDialog( QString lastOpened )
{
    // Save the full working buffer
#ifdef Q_OS_MACX

    QStringList filenames;
    QString fileName;
    QString path = lastOpened;
    QFileDialog dialog(0,"Load One Preset");

    dialog.setDefaultSuffix("syx");
    dialog.setDirectory(path);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setOption(QFileDialog::DontConfirmOverwrite);

    if (dialog.exec())
        filenames = dialog.selectedFiles();

    if(filenames.size() == 1)
        fileName = filenames.first();
#else
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Presets"), lastOpened, tr("Preset File (*.syx)"));
#endif

    return fileName;
}

//-------------------------------------------------------------------------
// Clear the work list and the device list
void DcPresetLib::appDataClear()
{
    ui.workList->clear();
    ui.deviceList->clear();
    _workListData.clear();
    _deviceListData.clear();
}

//-------------------------------------------------------------------------
void DcPresetLib::shutdownMidiIo()
{
    _midiIn.close();
    _midiIn.destroy();

    _midiOut.close();
    _midiOut.destroy();
}


//-------------------------------------------------------------------------
void DcPresetLib::syncWorklistToDevList()
{
    // Reset the preset work list cache
    _workListData.clear();
    _dirtyItemsIndex.clear();

    _workListData = _deviceListData;
    QStringList names = presetListToBankPatchName(_deviceListData);

    // Update preset displays
    ui.deviceList->clear();
    ui.deviceList->addItems(names);
    ui.workList->clear();
    ui.workList->addItems(names);
}

//-------------------------------------------------------------------------
// Experimental 
bool DcPresetLib::loadSysexFile( const QString &fileName,QList<QRtMidiData>& dataList )
{
    bool result = true;
    _lastErrorMsgStr.clear();

    QFile file(fileName); 

    if (!file.open(QIODevice::ReadOnly)) 
    { 
        _lastErrorMsg << "Unable to open the file:\n" << fileName;
        return false; 
    } 

    QDataStream in(&file); 

    // Load one preset in, see if it matches the current Manufacture
    unsigned char byte;
    QRtMidiData md;
    bool lookForStatusByte = true;

    while(1 == in.readRawData((char*)&byte,1))
    {
        if(lookForStatusByte)
        {
            if(byte == 0xF0)
            {
                md.clear();
                md.appendNum(byte);
                lookForStatusByte = false;
            }
        }
        else
        {
            if(byte == 0xF7)
            {
                md.appendNum(byte);
                dataList.append(md);
                lookForStatusByte = true;
            }
            else if(byte == 0xF0)
            {
                // Terminate the MIDI stream
                md.appendNum(0xF7);
                dataList.append(md);
                md.appendNum(0xF0);
            }
            else
            {
                md.appendNum(byte);
            }
        }
    }

    if(dataList.count() == 0)
    {
        _lastErrorMsg << fileName << " did not contain any MIDI data\n";
        result = false;
    }

    file.close();

    return result ; 
}

//-------------------------------------------------------------------------
void DcPresetLib::conCmd_MidiMonCtrl(DcConArgs args)
{
    if(args.argCount() != 1)
    {
        *_con << "must specify on of off\n";
    }
    else if(args.at(1).toString().contains("off"))
    {
        QObject::disconnect(&_midiIn, &QRtMidiIn::dataIn, this, &DcPresetLib::midiDataInToConHandler);
        QObject::disconnect(&_midiOut, &QRtMidiOut::dataOutMonitor, this, &DcPresetLib::midiDataOutToConHandler);
        *_con << "ok" << "\n";
    }
    else if(args.at(1).toString().contains("on"))
    {
        QObject::connect(&_midiIn, &QRtMidiIn::dataIn, this, &DcPresetLib::midiDataInToConHandler);
        QObject::connect(&_midiOut, &QRtMidiOut::dataOutMonitor, this, &DcPresetLib::midiDataOutToConHandler);
        *_con << "ok" << "\n";
    }
    else
    {
        *_con << "invalid argument: " << args.at(1).toString() << "\n";
    }
    
}

//-------------------------------------------------------------------------
void DcPresetLib::conCmd_Fetch( DcConArgs args )
{
    if(ui.fetchButton->isEnabled())
    {
        _presetOffset = args.at(2,0).toInt();
        _maxPresetCount = args.at(1,_devDetails.PresetCount).toInt();
        QMetaObject::invokeMethod(ui.fetchButton, "click",Qt::DirectConnection);
    }
    else
    {
        dclog("can't");
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::conCmd_MidiOut( DcConArgs args )
{
    if(args.argCount() > 0)
    {
        QRtMidiData md;

        md = args.hexJoin();
        _midiOut.dataOut(md);
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::midiDataInToConHandler( const QRtMidiData &data )
{
    _con->execCmd("append " + data.toString());
    *_con << "IN: " << data.toString().trimmed() << "\n";
}

//-------------------------------------------------------------------------
void DcPresetLib::midiDataOutToConHandler( const QRtMidiData &data )
{
    *_con << "OUT: " << data.toString() << "\n";
}

//-------------------------------------------------------------------------
void DcPresetLib::on_workList_currentRowChanged(int row)
{
    if(row == -1)
    {
        this->updateWorklistControlState();
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::conCmd_MidiWriteFile( DcConArgs args )
{
    QString fileName;

    // check args
    if(args.argCount() == 0)
    {
        fileName = execOpenDialog("");
        if(fileName.isEmpty())
        {
            // User canceled
            return;
        }
    }
    else
    {
        fileName = args.at(1).toString();
    }

    QList<QRtMidiData> sysexList;

    // check for file
    if(!loadSysexFile(fileName,sysexList))
    {
        *_con << _lastErrorMsgStr;
        return;
    }

   _con->setInputReady(false);
    for (int i = 0; i < sysexList.count() ; i++)
    {
        _midiOut.dataOut(sysexList.at(i));
        QApplication::processEvents();
    }

    _con->setInputReady(true);
    *_con << "wrote " << sysexList.count() << " sysex messages from " << fileName << "\n";

}

//-------------------------------------------------------------------------
void DcPresetLib::conCmd_GetUrl( DcConArgs args )
{
    if(args.argCount() == 0)
    {
        *_con << "must provide a url\n";
    }
    QUrl url(args.at(1).toString());
    _fileDownloader = new DcFileDownloader(url, this);

   *_con << "downloading data from " << args.at(1).toString() << "\n";
    
    
    connect(_fileDownloader, SIGNAL(downloaded()), SLOT(downloadDone()));

}

//-------------------------------------------------------------------------
void DcPresetLib::setupConsole()
{
    // _con is just an alias for ui.console
    _con = ui.console;
    _con->setVisible(false);
    _con->addCmd("fetch",ui.fetchButton,SLOT(clicked( )),"Execute a fetch");
    _con->addCmd("sync",ui.syncButton,SLOT(clicked( )),"Synchronize worklist with device (write preset changes)");
    _con->addCmd("show.bf",ui.buttonFrame,SLOT(show()),"Show the button frame");
    _con->addCmd("hide.bf",ui.buttonFrame,SLOT(hide()),"Hide the button frame");
    _con->addCmd("show.kf",ui.knobFrame,SLOT(show()),"Show the knob frame");
    _con->addCmd("hide.kf",ui.knobFrame,SLOT(hide()),"Hide the knob frame");
    _con->addCmd("fetch",this,SLOT(conCmd_Fetch(DcConArgs)),"<count> [<offset>] - Fetch count presets start at optional offset");
    _con->addCmd("mon",this,SLOT(conCmd_MidiMonCtrl(DcConArgs)),"<on|off> - display MIDI IN and OUT");
    _con->addCmd("out",this,SLOT(conCmd_MidiOut(DcConArgs)),"<midi hex bytes> - write midi bytes to connected device");
    _con->addCmd("writefile",this,SLOT(conCmd_MidiWriteFile(DcConArgs)),"[<filename>] - write the file to connected device");
    _con->addCmd("geturl",this,SLOT(conCmd_GetUrl(DcConArgs)),"<url> - file to download" );
    _con->addCmd("qss",this,SLOT(conCmd_qss(DcConArgs)),"Load the qss file" );
    _con->addCmd("lswl",this,SLOT(conCmd_lswl(DcConArgs)),"list the working list" );
    _con->setBaseDir(_dataPath);
    _con->execCmd("defld default_defs.bin");
    _con->addCmd("exec",this,SLOT(conCmd_exec(DcConArgs)),"Shell executes the file" );
    _con->addCmd("sm.trace",this,SLOT(conCmd_smtrace(DcConArgs)),"display state machine history" );

    ui.buttonFrame->setVisible(false);
    ui.knobFrame->setVisible(false);
}


//-------------------------------------------------------------------------
// display state machine history
void DcPresetLib::conCmd_smtrace( DcConArgs args )
{
    if(args.argCount() == 0)
    {
        *_con << _stateTrace.join("\n");
    }
}

//-------------------------------------------------------------------------
// Shell executes the file
void DcPresetLib::conCmd_exec( DcConArgs args )
{
    const char* shellCmd = "explorer.exe";

    if(args.argCount() == 1)
    {
        QProcess e;
        qApp->processEvents();
        e.start(shellCmd, QStringList() << args.at(1).toString());
        qApp->processEvents();
        QThread::msleep(130);
        if (!e.waitForStarted())
            return;
    }
}

//-------------------------------------------------------------------------
// list the working list
void DcPresetLib::conCmd_lswl( DcConArgs args )
{
    if(args.argCount() == 0)
    {
        for (int r = 0; r < ui.workList->count() ; r++)
        {
            *_con << ui.workList->item(r)->text() << "\n";	
        }
    }
}

//-------------------------------------------------------------------------
// Load the qss file
void DcPresetLib::conCmd_qss( DcConArgs args )
{
    if(args.argCount() == 0)
    {
        *_con << "todo - display qss file\n";
    }
    else
    {
        if(args.at(1).toString() == "last")
        {
            if(_styleHistory.count())
            {
                QString st = _styleHistory.takeFirst();
                qApp->setStyleSheet(st);
            }
            
        }
        else
        {
            QString fname = QDir::toNativeSeparators(_dataPath + "qss/" + args.at(1).toString());
            QFile file(fname);
            file.open(QFile::ReadOnly);
            _styleHistory << qApp->styleSheet();
            qApp->setStyleSheet(file.readAll());
        }
        
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::on_dial0_sliderMoved( int vv )
{
    QString s;
    QTextStream ts(&s);
    ts << "out d0 " << hex << (vv&0xFF);
    _con->execCmd(s);
}
//-------------------------------------------------------------------------
void DcPresetLib::on_dial1_sliderMoved( int vv )
{
    QString s;
    QTextStream ts(&s);
    ts << "out d1 " << hex << (vv&0xFF);
    _con->execCmd(s);
}
//-------------------------------------------------------------------------
void DcPresetLib::on_dial2_sliderMoved( int vv )
{
    QString s;
    QTextStream ts(&s);
    ts << "out d2 " << hex << (vv&0xFF);
    _con->execCmd(s);
}
//-------------------------------------------------------------------------
void DcPresetLib::on_dial3_sliderMoved( int vv )
{
    QString s;
    QTextStream ts(&s);
    ts << "out d3 " << hex << (vv&0xFF);
    _con->execCmd(s);
}
//-------------------------------------------------------------------------
void DcPresetLib::on_dial4_sliderMoved( int vv )
{
    QString s;
    QTextStream ts(&s);
    ts << "out d4 " << hex << (vv&0xFF);
    _con->execCmd(s);
}
//-------------------------------------------------------------------------
void DcPresetLib::on_dial5_sliderMoved( int vv )
{
    QString s;
    QTextStream ts(&s);
    ts << "out d5 " << hex << (vv&0xFF);
    _con->execCmd(s);
}
//-------------------------------------------------------------------------
void DcPresetLib::on_dial6_sliderMoved( int vv )
{
    QString s;
    QTextStream ts(&s);
    ts << "out d6 " << hex << (vv&0xFF);
    _con->execCmd(s);
}
//-------------------------------------------------------------------------
void DcPresetLib::on_dial7_sliderMoved( int vv )
{
    QString s;
    QTextStream ts(&s);
    ts << "out d7 " << hex << (vv&0xFF);
    _con->execCmd(s);
}

//-------------------------------------------------------------------------
void DcPresetLib::on_vs0_sliderMoved( int vv )
{
    QString s;
    QTextStream ts(&s);
    ts << "out vs7 " << hex << (vv&0xFF);
    _con->execCmd(s);

}


//-------------------------------------------------------------------------
void DcPresetLib::downloadDone()
{
    QByteArray ba = _fileDownloader->downloadedData();
    *_con << "data received: " << ba.count()/1024 << "KB\n";
    qApp->processEvents();
    QRtMidiData md(ba);
    
    QString fname = QDir::toNativeSeparators(_dataPath + "dl/" + _fileDownloader->getName());
    QFile file(fname);
    file.open(QFile::ReadWrite);
    file.write(md.data(),md.length());
    file.close();
    *_con << "File saved to " << fname << "\n";

}




































