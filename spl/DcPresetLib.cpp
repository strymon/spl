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

#include <QtTest/QTest>


#include "DcMidi/DcMidiData.h"
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
#include "cmn/DcState.h"
#include "DcBootControl.h"



// Animation
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QGraphicsOpacityEffect>

#include "MidiSettings.h"
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
#include "DcMidi/DcMidiIdent.h"

#include "DcConsoleForm.h"
#include "DcUpdateDialogMgr.h"

#include <QDebug>
#include "DcFileDownloader.h"

#include <QProcess>
#include "DcSoftwareUpdate.h"
#include <QUuid>

#include "DcUpdateAvailableDialog.h"

#include "cmn/DcLog.h"
#include <QDesktopWidget>


// Strymon Pedal Specific
const char* DcMidiDevDefs::kStrymonDevice = "F0 00 01 55";
const char* DcMidiDevDefs::kIdentReply    = "F0 7E .. 06 02";
const char* DcMidiDevDefs::kTimeLineIdent = "F0 7E .. 06 02 00 01 55 12 00 01"; // Partial TimeLine Identity Response
const char* DcMidiDevDefs::kMobiusIdent   = "F0 7E .. 06 02 00 01 55 12 00 02"; // Partial Mobius Identity Response
const char* DcMidiDevDefs::kBigSkyIdent   = "F0 7E .. 06 02 00 01 55 12 00 03"; // Big sky
const char* DcMidiDevDefs::kTestDevice    = "F0 7E .. 06 02 00 01 55 12 00 04"; // Test Device

bool gUseAltPresetSize = true;

DcPresetLib::DcPresetLib(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    setupFilePaths();

    _log = new DcLog(QDir::toNativeSeparators(_dataPath + QApplication::applicationName() + ".log"));

    ui.actionOpen->setEnabled(false);
    ui.actionSave->setEnabled(false);
    ui.actionSave_One->setEnabled(false);
    ui.actionMove->setEnabled(false);
    ui.actionRename->setEnabled(false);
    ui.actionLoad_One->setEnabled(false);
    
    _midiOutDecimalMode = new DcConBool(this);
    _lastErrorMsg.setString(&_lastErrorMsgStr);

    _supportedDevicesSet.insert(DcMidiDevDefs::kTimeLineIdent);
    _supportedDevicesSet.insert(DcMidiDevDefs::kMobiusIdent);
    _supportedDevicesSet.insert(DcMidiDevDefs::kBigSkyIdent);
    
    _midiSettings = new MidiSettings(this);
    _midiSettings->addSupportedIdentities(_supportedDevicesSet);

    _workListControls << ui.renameButton << ui.moveButton << ui.loadOneButton << ui.saveOneButton;
    _workListActions  << ui.actionSave_One << ui.actionLoad_One << ui.actionRename << ui.actionMove;

    _maxPresetCount = 0;
    _presetOffset = 0;
    
    _fileDownloader = 0;

    installEventFilter(this);

    QObject::connect(&_watchdog_timer,&QTimer::timeout,this,&DcPresetLib::devIdWatchDogHandler);

    _progressDialog = new IoProgressDialog(this);
    _progressDialog->setModal(true);

    ui.mainToolBar->hide();
    this->setWindowIcon(QIcon(":/images/res/dcpm_256x256x32.png"));
  
    QObject::connect(ui.devImgLabel,SIGNAL(clicked()),this,SLOT(on_devImgClicked()));

    setupConsole();
    
    ui.fetchButton->setFocus();
    
    readSettings();

    // Verify we didn't fly off the screen if a multi-monitor system changed
    QDesktopWidget* dw = QApplication::desktop();
    QRect rec = dw->availableGeometry(this);
    QPoint appLoc = this->pos();
    
    if(!rec.contains(appLoc))
    {
        // The app is in the weeds, bring it back to the primary display
        int w = dw->width();
        int h = dw->height();
        QSize size = this->size();
        int mw = size.width();
        int mh = size.height();
        int cw = (w/2) - (mw/2);
        int ch = (h/2) - (mh/2);

        move(cw,ch);
    }


    // Do the rest of the application startup in this timer callback
     QTimer::singleShot(10, this, SLOT(setupStateMachineHandler()));

    _idResponceTrigger = new DcMidiTrigger(DcMidiDevDefs::kIdentReply,this,SLOT(recvIdDataTrigger(void*)));
    // _portScanTrigger = new DcMidiTrigger(DcMidiDevDefs::kIdentReply,this,SLOT(portScanTrigger(void*)));
    
    // loadConsolePlugins();

}
void DcPresetLib::on_devImgClicked()
{
       _machine.postEvent(new VerifyDeviceConnection());
}

//-------------------------------------------------------------------------
DcPresetLib::~DcPresetLib()
{
    _midiIn.removeTrigger(*_idResponceTrigger);

    delete _midiSettings;
    delete _progressDialog;
    delete _idResponceTrigger;
    delete _log;
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
// Workaround for QTBUG 30899
// SEE: https://bugreports.qt-project.org/browse/QTBUG-30899
// TODO: Reverify this and remove the workaround
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
    
    _updatesPath = QDir::toNativeSeparators(_dataPath + "updates/");
    QDir().mkpath(_updatesPath);
    
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
    
    settings.beginGroup("console");
    settings.setValue("show",ui.actionShow_Console->isChecked());
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

    settings.beginGroup("console");
    enableMidiMonitor(settings.value("midimonitor",false).toBool());
    ui.actionShow_Console->setChecked(settings.value("show",false).toBool());
    on_actionShow_Console_triggered();
    settings.endGroup();

    settings.beginGroup("midiio");
    _maxMsgSize = settings.value("MaxMsgSize",-1).toInt();
    _delayPerMsgChunk = settings.value("DelayPerMsgChunk",0).toInt();
    settings.endGroup();

}

//-------------------------------------------------------------------------
void DcPresetLib::detectDevice_entered()
{
    clearMidiInConnections();
    shutdownMidiIo();

    QString in_port = _midiSettings->getInPortName();
    QString out_port = _midiSettings->getOutPortName();
    
    DCLOG() << "Detecting devices on ports: " << in_port << "," << out_port;
    
    bool success = false;
    
    _devDetails.clear();
    conResetReadOnlySymbolDefines();


    if(!in_port.isEmpty() && !out_port.isEmpty())
    {
        _midiIn.init();
        _midiOut.init();

        if(_midiIn.open( in_port) && _midiOut.open( out_port ))
        {
            QThread::msleep(25);
            QObject::connect(&_midiIn, &DcMidiIn::dataIn, this, &DcPresetLib::recvIdData);    

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
void DcPresetLib::erroRecovery_entered()
{
    clearMidiInConnections();
    checkSyncState();    
    _machine.postEvent(new WorkListDirtyEvent());
}

//-------------------------------------------------------------------------
void DcPresetLib::cancelXfer_entered()
{
    clearMidiInConnections();
}

//-------------------------------------------------------------------------
bool DcPresetLib::hasDevSupport( const DcMidiData &data )
{
    QSetIterator<const char*> i(_supportedDevicesSet);
    while (i.hasNext())
    {
        if(data.contains(i.next()))
        {
            return true;
        }
    }
    return false;
}
//-------------------------------------------------------------------------
// This singleShot slot is used to detect a timeout during the detectDevice
// state.  It will always run, but the timeout is avoided if the recvIdData
// runs first. 
void DcPresetLib::devIdWatchDogHandler()
{
    _watchdog_timer.stop();
    QObject::disconnect(&_midiIn, &DcMidiIn::dataIn, this, &DcPresetLib::recvIdData);
    emit deviceNotFound();
}
//-------------------------------------------------------------------------
void DcPresetLib::recvIdData( const DcMidiData &data )
{
    // Ignore any message that's not an MIDI Identity reply
    if(!data.contains(DcMidiDevDefs::kIdentReply))
        return;

    // Parse the Identify response
    _devDetails.fromIdentData(data);
    
    // Now, see if the program supports this device
    if(!hasDevSupport(data))
    {
        // Bailing out here will let other devices on the same port have a chance to 
        // report there identity.  If a supported device is never found, the watchdog_timer 
        // timeouts (devIdWatchDog is called)

        // Note, as soon as a supported device is found the method continues and
        // will ignore any other device responding to the identity request.  This prevents
        // multi-device support on the same MIDI port.
        
        return;
    }
    
    
    // Shutdown the watchdog timer and disconnect this handler from the MIDI IN port
    _watchdog_timer.stop();
    QObject::disconnect(&_midiIn, &DcMidiIn::dataIn, this, &DcPresetLib::recvIdData);
    
    // TODO: Put all the device details in a persistent Map of some kind allowing
    // for new devices to be supported.

    // TODO: _devDetails is just a quick structure to organize the device specific goo
    // to make the preset librarian work.  A structure is mostly overkill for this and perhaps this data
    // should remain as 'data' and therefor get stored as name/values in a map.
    // Examples of poor generalization:  Protocol details, such as "preset data byte offset" 
    // may only make since for a few products.  A method to hide these details, or to not need 
    // these details would be more useful.  The program needs to know how to "grab" preset data 
    // and work with it.  What's the best way to do this?

    updateDeviceDetails(data,_devDetails);

    QPixmap pm;
    if(data.contains(DcMidiDevDefs::kTimeLineIdent))
    {
        _devDetails.DeviceIconResPath = ":/images/res/timeline_100.png";
        pm = QPixmap(":/images/res/timeline_100.png");
    }
    else if(data.contains(DcMidiDevDefs::kMobiusIdent) )
    {
        _devDetails.DeviceIconResPath = ":/images/res/mobius_100.png";
        pm = QPixmap(":/images/res/mobius_100.png");
    }
    else if(data.contains(DcMidiDevDefs::kBigSkyIdent) )
    {
        _devDetails.DeviceIconResPath = ":/images/res/bigsky_100.png";
        pm = QPixmap(":/images/res/bigsky_100.png");
    }

    _maxPresetCount = _devDetails.PresetCount;
    _presetOffset = 0;

    QString devNameVer = _devDetails.Name;
    if(!_devDetails.FwVersion.isEmpty() && _devDetails.FwVersion.length() > 2)
    {
           devNameVer += " v" + _devDetails.FwVersion.mid(2);
    }

    ui.devInfoLabel->setText(devNameVer);
    ui.devImgLabel->setToolTip(_devDetails.FwVersion);
    ui.devImgLabel->setPixmap(pm);
    
    // Adding device specific details to the console
    _con->addRoSymDef("EOX","F7");
    _con->addRoSymDef("SOX","F0");
    _con->addRoSymDef("dev.soxhdr",_devDetails.SOXHdr.toString(' '));
    _con->addRoSymDef("dev.ver",_devDetails.FwVersion);
    _con->addRoSymDef("dev.name",_devDetails.Name);



     emit deviceReady();
}

//-------------------------------------------------------------------------
// This method is called when a device has been detected.
void DcPresetLib::userCanFetch_entered()
{
    // See if the worklist contains data for the detected device
    if(_devDetails.getUid() != _workListDataDeviceUid)
    {
        if(_dirtyItemsIndex.length())
        {
            if (QMessageBox::Save == QMessageBox::question(this, "The worklist was not inSync with the device",
                "Would you like to save the changes?", QMessageBox::Save|QMessageBox::No))
            {
                on_actionSave_triggered();
            }
        }
        
        ui.workList->clear();
        ui.deviceList->clear();
        _workListData.clear();
        _deviceListData.clear();
    }
    else
    {
        // There's data in the work list, goto the preset edit state
        _machine.postEvent(new FetchDataExistsEvent());
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::setupStateMachineHandler()
{
    _con->initGuiElements();

    DcState::SetTraceStringList(&_stateTrace);
    _machine.setGlobalRestorePolicy(QStateMachine::RestoreProperties);
    DcState *detectDevice       = new DcState(QString("detectDevice"));
    
    DcState *errorRecovery      = new DcState(QString("errorRecovery"));
    DcState *cancelXfer         = new DcState(QString("cancelXfer"));

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
    
    _machine.addState(errorRecovery);
    _machine.addState(cancelXfer);

    _machine.addState(userCanFetch);
    _machine.addState(setupReadPresetsState);
    _machine.addState(readPresetsCompleteState);

    _machine.addState(midiPortSelect);
    _machine.addState(noDevice);
    _machine.addState(procDataIn);
    _machine.addState(presetEdit);

    // app Startup
    QObject::connect(detectDevice, SIGNAL(entered()), this, SLOT(detectDevice_entered()));

    QObject::connect(errorRecovery, SIGNAL(entered()), this, SLOT(erroRecovery_entered()));

    DcCustomTransition *cet = new DcCustomTransition(WorkListDirtyEvent::TYPE,errorRecovery);
    cet->setTargetState(notInSyncState);
    errorRecovery->addTransition(cet);

    QObject::connect(cancelXfer, SIGNAL(entered()), this, SLOT(cancelXfer_entered()));

    
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
    
    DcCustomTransition *nodt = new DcCustomTransition(DetectDevice::TYPE,noDevice);
    nodt->setTargetState(detectDevice);
    noDevice->addTransition(nodt);

    // VerifyDeviceConnection event causes: noDevice to detectDevice
    nodt = new DcCustomTransition(VerifyDeviceConnection::TYPE,noDevice);
    nodt->setTargetState(detectDevice);
    noDevice->addTransition(nodt);


    
    // midiPortSelect
    QObject::connect(midiPortSelect, SIGNAL(entered()), this, SLOT(midiPortSelect_enter()));

    // No matter what, we always check for the device after this state
    midiPortSelect->addTransition(_midiSettings,SIGNAL(midiReject()),detectDevice);
    midiPortSelect->addTransition(_midiSettings,SIGNAL(midiAccept()),detectDevice);

    QObject::connect(userCanFetch, SIGNAL(entered()), this, SLOT(userCanFetch_entered()));
    userCanFetch->addTransition(ui.actionMIDI_Ports,SIGNAL(triggered()),midiPortSelect);
    userCanFetch->addTransition(ui.actionFetch,SIGNAL(triggered()),setupReadPresetsState);
    
    DcCustomTransition *dcct = new DcCustomTransition(VerifyDeviceConnection::TYPE,userCanFetch);
    dcct->setTargetState(detectDevice);
    userCanFetch->addTransition(dcct);

    
    DcCustomTransition *ucfct = new DcCustomTransition(DetectDevice::TYPE,noDevice);
    ucfct->setTargetState(detectDevice);
    userCanFetch->addTransition(ucfct);




    
    DcCustomTransition* ucf = new DcCustomTransition(FetchDataExistsEvent::TYPE,readPresetsCompleteState);
    ucf->setTargetState(presetEdit);
    userCanFetch->addTransition(ucf);
    
    // Setup the dataIn system
    DcState* xferInState = _xferInMachine.setupStateMachine(&_machine,&_midiOut,readPresetsCompleteState,detectDevice,detectDevice);
    
    // Setup the dataOut system
    DcState* xferOutState = _xferOutMachine.setupStateMachine(&_machine,&_midiOut,writePresetsCompleteState,errorRecovery,cancelXfer);
    
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
    
    // Preset Transfer Complete, setup preset edit state
    QObject::connect(presetEdit, SIGNAL(entered()), this, SLOT(presetEdit_entered()));

    presetEdit->addTransition(ui.actionMIDI_Ports,SIGNAL(triggered()),midiPortSelect);
    presetEdit->addTransition(ui.fetchButton,SIGNAL(clicked()),setupReadPresetsState);
    
    wpt = new DcCustomTransition(VerifyDeviceConnection::TYPE,presetEdit);
    wpt->setTargetState(detectDevice);
    presetEdit->addTransition(wpt);

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

    _machine.setInitialState(detectDevice);
    _machine.start();
}

//-------------------------------------------------------------------------
void DcPresetLib::conResetReadOnlySymbolDefines()
{
    _con->clearRoSymDefs();
    _con->addRoSymDef("rootpath",QDir::toNativeSeparators(_dataPath));
    _con->addRoSymDef("logfile",QDir::toNativeSeparators(_dataPath + QApplication::applicationName() + ".log"));
}

//-------------------------------------------------------------------------
void DcPresetLib::noDevice_entered()
{
    qWarning() <<  "NO DEVICE state entered";
    
    conResetReadOnlySymbolDefines();
    
    clearDeviceUI();

    if(!_devDetails.Family.isEmpty())
    {
        *_con << "Unsupported Device: " << _devDetails.toString();
        qWarning() <<  "Unsupported Device: " << _devDetails.toString();
    }
    
    // Have the system watch for identity data:
    
    // Setup a trigger on midi in, if identity is detected, signal the slot.
    // See the instantiation of _idResponceTrigger for the slot details. 
    _midiIn.addTrigger(*_idResponceTrigger);

} 

//-------------------------------------------------------------------------
void DcPresetLib::setupReadPresetXfer_entered()
{
    ui.deviceList->clear();
    ui.workList->clear();
    _workListDataDeviceUid = 0;

    _xferInMachine.reset();
    
    // Get the number of presets that will be transfered from the device
    int presetCount     = _maxPresetCount;
    int presetOffset    = _presetOffset;
    
    // Build a list of commands, the state machine will process each one in turn
    DcMidiData cmd;
    for (int presetId = presetOffset; presetId < presetCount+presetOffset; presetId++)
    {
        cmd.setData(_devDetails.PresetReadTemplate,presetId);
        _xferInMachine.append(cmd);
    }
    
    // Setup to receive preset data
    clearMidiInConnections();
    QObject::connect(&_midiIn, &DcMidiIn::dataIn, &_xferInMachine, &DcXferMachine::replySlotForDataIn);

    _xferInMachine.setProgressDialog(_progressDialog);

    _xferInMachine.go(&_devDetails);

    qDebug() << "Reading presets: (ioconf " << _maxMsgSize << " " << _delayPerMsgChunk << ")";
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
        DcMidiData md = _workListData.at(pid);
        if(md.contains("F0 00 01 55 XX XX 62 XX XX 47 F7"))
        {
            // Skip this preset
        }
        else
        {
            // Update the work list too.
            _deviceListData[pid] = md;
            _xferOutMachine.append(md);
        }
    }

    QObject::disconnect(&_midiIn, &DcMidiIn::dataIn, this, &DcPresetLib::recvIdData);
    QObject::disconnect(&_midiIn, &DcMidiIn::dataIn, &_xferInMachine, &DcXferMachine::replySlotForDataIn);
    QObject::disconnect(&_midiIn, &DcMidiIn::dataIn, &_xferOutMachine, &DcXferMachine::replySlotForDataOut);

    QObject::connect(&_midiIn, &DcMidiIn::dataIn, &_xferOutMachine, &DcXferMachine::replySlotForDataOut);
    
    _xferOutMachine.setProgressDialog(_progressDialog);

    _xferOutMachine.go(&_devDetails,_maxMsgSize,_delayPerMsgChunk);
    
    qDebug() << "Writing presets: (ioconf " << _maxMsgSize << " " << _delayPerMsgChunk << ")";
    // Signal the state machine to begin
    emit writePresets_setupDone_signal();

}

//-------------------------------------------------------------------------
void DcPresetLib::presetEdit_entered()
{
    // Now, create a backup of this fresh data
    backupDeviceList();
    updateWorkListFromDeviceList();

    Q_ASSERT(_deviceListData.length());
    
    checkSyncState();
    _machine.postEvent(new InPresetEditEvent());
}

//-------------------------------------------------------------------------
void DcPresetLib::on_workList_itemSelectionChanged()
{
    updateWorklistButtonAndAcationState();
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
        DcMidiData presetA = _workListData.at(presetA_row);
        int presetA_id      = presetA.get14bit(_devDetails.PresetNumberOffset,-1);
        if(presetA_id == -1)
        {
            DCLOG() << "ERROR - the format of the data in the workList is corrupt or unexpected";
            return;
        }

        QString presetA_name = getPresetName(presetA);

        // Prepare the move dialog
        MoveDialog* m = new MoveDialog(this);
        m->setModal(true);
        m->setDestinations(presetListToBankPatchName(_workListData));
        m->setTargetPreset(ui.workList->item(presetA_row)->text());
        
        if(QDialog::Accepted ==  m->exec())
        {
            // Get the Row, Data and ID of preset B
            int             presetB_row     = m->getDestIdx();


            DcMidiData     presetB_data    = _workListData.at(presetB_row);
            int             presetB_id      = presetB_data.get14bit(_devDetails.PresetNumberOffset ,-1);


            if(presetB_id == -1)
            {
                qWarning() << "ERROR - the format of the data in the workList is corrupt or unexpected";
                return;
            }

            if(m->swapClicked())
            {
                // Swap presets A and B
                presetB_data.set14bit(_devDetails.PresetNumberOffset,presetA_id);
                presetA.set14bit(_devDetails.PresetNumberOffset,presetB_id);
                _workListData.replace(presetA_row,presetB_data);
                _workListData.replace(presetB_row,presetA);
            }
            else
            {
                // Replace preset B with preset A
                presetA.set14bit(_devDetails.PresetNumberOffset,presetB_id); 
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

// Copy the presets
// foreach(QRtMidiData md, selectedPresets)
// {
//     md.set14bit(_devDetails.PresetNumberOffset,dstPresetNum++);
//     _workListData.replace(dstIdx++,md);
// }


//-------------------------------------------------------------------------
void DcPresetLib::updatePresetChecksum(DcMidiData& md)
{
    int sum = md.sumOfSection(_devDetails.PresetStartOfDataOffset,_devDetails.PresetDataLength);
    md.replace(_devDetails.PresetChkSumOffset,1,QByteArray(1,(char)sum));
}

void DcPresetLib::on_actionRename_triggered()
{
    int row = ui.workList->currentRow();
    if(row != -1)
    {
        DcMidiData md = _workListData.at(row);
        QString origName = md.mid(_devDetails.PresetNameOffset,_devDetails.PresetNameLen);
        RenameDialog* w = new RenameDialog(this);
        w->setModal(true);
        w->setName(origName.trimmed());
        if(QDialog::Accepted ==  w->exec())
        {
            QByteArray ba = w->getName().toUtf8();
            ba = ba.leftJustified(_devDetails.PresetNameLen,' ');
            if(!origName.contains(ba.data()))
            {
                md.replace(_devDetails.PresetNameOffset,_devDetails.PresetNameLen,ba);

                updatePresetChecksum(md);
            

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

void DcPresetLib::setPresetName(DcMidiData &md, QString name)
{
    if(!md.isEmpty() && !name.isEmpty())
    {
        QString origName = getPresetName(md);
        QByteArray ba = name.toUtf8();
        ba = ba.leftJustified(_devDetails.PresetNameLen,' ');
        if(!origName.contains(ba.data()))
        {
            md.replace(_devDetails.PresetNameOffset,_devDetails.PresetNameLen,ba);
            int sum = md.sumOfSection(_devDetails.PresetStartOfDataOffset,_devDetails.PresetDataLength);
            QByteArray chksum;
            chksum += sum;
            md.replace(_devDetails.PresetChkSumOffset,1,QByteArray::fromHex(chksum.toHex()));
        }
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::on_workList_itemDoubleClicked()
{
     if(QApplication::keyboardModifiers() & Qt::ShiftModifier)
     {
         int cur_idx = ui.workList->currentRow();
         if(cur_idx > -1)
         {
             // Grab the preset data and update the bank/preset to PresetCount (that will place it in the edit buffer)  
             DcMidiData md = _workListData.at(cur_idx);
               md.set14bit(_devDetails.PresetNumberOffset,_devDetails.PresetCount);
               _midiOut.dataOutSplit(md,_maxMsgSize,_delayPerMsgChunk);
               
               // Send the update display message
               md = _devDetails.SOXHdr;
               md.append("26 F7");
               _midiOut.dataOut(md);

         }
     }
     else if(QApplication::keyboardModifiers() & Qt::ControlModifier)
     {
         int cur_idx = ui.workList->currentRow();
         if(cur_idx > -1)
         {
             int num = getPresetNumber(_workListData.at(cur_idx));
             int bnk = num/127;
             int pnum  = (bnk) ? (num % (bnk*127)) : num;
             DcMidiData md;
             md.setData("B0 00 vv",bnk);
             _con->execCmd("out " + md.toString(' '));
             md.setData("C0 vv",pnum);
             _con->execCmd("out " + md.toString(' '));
             

         }
     }
     else

     {
         QMetaObject::invokeMethod(ui.renameButton, "click",Qt::DirectConnection);
     }
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
    qDebug() << "Preset write complete";
    clearMidiInConnections();

    updateWorkListFromDeviceList();
    _machine.postEvent(new WriteCompleteSuccessEvent());
}

//-------------------------------------------------------------------------
void DcPresetLib::readPresetsComplete_entered()
{
     DCLOG() << "Preset read complete";
    
     clearMidiInConnections();

    _deviceListData = _xferInMachine.getDataList();
    
    // Keep track of what device this data was for
    _workListDataDeviceUid = _devDetails.getUid();



    _machine.postEvent(new FetchCompleteSuccessEvent());
}


//-------------------------------------------------------------------------
QString DcPresetLib::presetNumberToBankNum(int i)
{
    QString num;
    int presetsPerBank = _devDetails.PresetsPerBank ? _devDetails.PresetsPerBank : 2;

    char inBankAlpha = 'A' + i%presetsPerBank;
    int bank = i/presetsPerBank;
    return num.sprintf("%02d%c",bank,inBankAlpha);
}

QString DcPresetLib::getPresetBankPresetNumber(const DcMidiData& preset )
{
    int presetId = getPresetNumber(preset);

    return presetNumberToBankNum(presetId);
}

//-------------------------------------------------------------------------
int DcPresetLib::bankPresetToNum( QString s )
{
    int presetNumber = -1;
    int bpos = s.indexOf(QRegExp("[aAbBcCdDeEfFgGhH]"));
    if(bpos > 0)
    {
        bool ok;
        int bnk = s.mid(0,bpos).toInt(&ok);
        if(ok)
        {
            bnk *= _devDetails.PresetsPerBank;
            QString pstr = s.mid(bpos,1).toLower();
            char p = pstr.toLatin1()[0];
            char pval = p - 'a';
            if(pval < _devDetails.PresetsPerBank)
            {
                presetNumber  = pval + bnk;
            }
        }
    }

    return presetNumber ;

}

//-------------------------------------------------------------------------
bool DcPresetLib::isBankPresetString( QString s )
{
    return s.isEmpty() ? false : s.contains(QRegExp("[0-9]+[aAbBcCdDeEfFgGhH]"));
}


//-------------------------------------------------------------------------
QStringList DcPresetLib::presetListToBankPatchName(QList<DcMidiData>& listData)
{
    QStringList names;
    for (int i = 0; i < listData.size(); ++i) 
    {
        DcMidiData presetData = listData.at(i);

        names << getPresetBankPresetNumber(presetData) + " - " + getPresetName(presetData);
    }

    return names;
}

//-------------------------------------------------------------------------
QString DcPresetLib::presetToName(DcMidiData& p)
{
    return p.mid(kPresetNameOffset,kPresetNameLen).trimmed();
}

QString DcPresetLib::getPresetName(DcMidiData &md)
{
    return md.mid(_devDetails.PresetNameOffset,_devDetails.PresetNameLen).trimmed();
}


//-------------------------------------------------------------------------
QString DcPresetLib::presetToBankPatchName(DcMidiData& p)
{
    return getPresetBankPresetNumber(p) + " - " + getPresetName(p);
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
        DcMidiData md;
        if(!loadPresetBinary(fileName,md))
        {
            dispErrorMsgBox();
        }
        else
        {
            // Get the preset number from the occupant of cur_idx in the worklist
            int presetNum = _workListData[cur_idx].get14bit(_devDetails.PresetNumberOffset,-1);
            if(presetNum == -1)
            {
                Q_ASSERT(presetNum != -1);

                // Something is wrong with the preset in this location, just use the slot number,
                // it will probably be the same anyway.  THe above code is in place if we ever support
                // presets list that do not start at zero.
                presetNum = cur_idx;
            }

            md.set14bit(_devDetails.PresetNumberOffset,presetNum);

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
        qWarning() << "nothing selected";
        return;
    }
    
    DcMidiData md = _workListData.at(curIdx);
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
            dispErrorMsgBox();
        }
        settings.setValue("lastSinglePreset",fileName);
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::on_actionSave_triggered()
{
    QSettings settings;
    QString defaultPresetFileName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + _devDetails.Name + " Presets.syx";
    QString lastSave = settings.value("lastPresetBundleSave",defaultPresetFileName).toString();

    // Save the full working buffer
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Presets"), lastSave, tr("Preset File(*.syx)"));

    if(!fileName.isEmpty())
    {
        if(!savePresetBinary(fileName,_workListData))
        {
            dispErrorMsgBox();
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
        
        QList<DcMidiData> newPresetList;
        if(!loadPresetBinary(fileName,newPresetList))
        {
            dispErrorMsgBox();
        }
        else
        {
/*           

        if(newPresetList.length() < _maxPresetCount)
           {
               _lastErrorMsgStr.clear();
               _lastErrorMsg <<  "Operation will be aborted.\nThe preset file does not contain the correct number of presets.";
                
                dispErrorMsgBox();
                return;
           }
*/

           _workListData = newPresetList;

           checkSyncState();

        }
        settings.setValue("lastPresetBundleOpened",fileName);
    }
}

//-------------------------------------------------------------------------
bool DcPresetLib::loadPresetBinary(const QString &fileName,QList<DcMidiData>& dataList)
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
    if(file.size() < _devDetails.PresetSize*_devDetails.PresetCount)
    {
       _lastErrorMsg << "File does not contain enough presets.";
       return false;
    }

    // Load one preset in, see if it matches the current Manufacture
    QByteArray ba(_devDetails.PresetSize,'\0');

    while(ba.length() == in.readRawData(ba.data(),ba.length()))
    {
        DcMidiData md(ba);

        if(!md.contains(_devDetails.PresetWriteHdr))
        {
            _lastErrorMsg << "File does not contain " << _devDetails.Name << " presets";
            return false;
        }

        // Filter out any NAK commands
        if(md.match("47F7"))
        {
            continue;
        }


        // This is a good preset file
        dataList.append(md);
    }

    file.close();

    return true; 
}

//-------------------------------------------------------------------------
// Load one preset
bool DcPresetLib::loadPresetBinary( const QString &fileName,DcMidiData& md )
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

     if(!md.contains(_devDetails.PresetWriteHdr))
     {
         _lastErrorMsg << "File does not contain " << _devDetails.Name << " presets";
         return false;
     }

    file.close();

    return true; 
}

bool DcPresetLib::savePresetBinary(const QString &fileName,const QList<DcMidiData>& dataList)
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
    qDebug() << "Presets saved to: " << fileName;
    return true; 
}

//-------------------------------------------------------------------------
bool DcPresetLib::savePresetBinary( const QString &fileName,const DcMidiData& md )
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
    qDebug() << "Preset saved to: " << fileName;

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
void DcPresetLib::dispErrorMsgBox(const char* msg /*=0*/)
{
    qDebug() << "Display error box: " << msg;
    QMessageBox* msgBox = new QMessageBox(this);

    if(msg)
    {
        msgBox->setText(msg);
    }
    else
    {
        msgBox->setText(_lastErrorMsgStr);
    }

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
void DcPresetLib::updateWorklistButtonAndAcationState()
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
void DcPresetLib::shutdownMidiIo()
{
    _midiIn.close();
    _midiIn.destroy();

    _midiOut.close();
    _midiOut.destroy();
}


//-------------------------------------------------------------------------
void DcPresetLib::updateWorkListFromDeviceList()
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
bool DcPresetLib::loadSysexFile( const QString &fileName,QList<DcMidiData>& dataList, QList<DcMidiData>* pRejectDataList /* = 0 */)
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
    DcMidiData md;
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
                
                if(0 == pRejectDataList) 
                {
                    dataList.append(md);
                } 
                else 
                {
                    bool reject = false;
                    // Apply the filter
                    foreach(DcMidiData rejectMd,*pRejectDataList)
                    {
                        if(md.contains(rejectMd)) 
                        {
                            reject = true;
                            break;
                        }
                    }

                    if(!reject)
                    {
                        dataList.append(md);
                    }
                }
                
                lookForStatusByte = true;
            }
            else if(byte == 0xF0)
            {
                // Terminate the MIDI stream
                md.appendNum(0xF7);

                if(0 == pRejectDataList) 
                {
                    dataList.append(md);
                } 
                else if(!pRejectDataList->contains(md)) 
                {
                    dataList.append(md);
                }
                
                md.clear();
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
bool DcPresetLib::enableMidiMonitor( bool enable )
{
    QSettings settings;

    bool prevState = settings.value("console/midimonitor",false).toBool();

    QObject::disconnect(&_midiIn, &DcMidiIn::dataIn, this, &DcPresetLib::midiDataInToConHandler);
    QObject::disconnect(&_midiOut, &DcMidiOut::dataOutMonitor, this, &DcPresetLib::midiDataOutToConHandler);

    if(enable)
    {
        QObject::connect(&_midiIn, &DcMidiIn::dataIn, this, &DcPresetLib::midiDataInToConHandler);
        QObject::connect(&_midiOut, &DcMidiOut::dataOutMonitor, this, &DcPresetLib::midiDataOutToConHandler);
    }
    
    settings.setValue("console/midimonitor",enable);
    
    return prevState;
}

void DcPresetLib::conCmd_MidiMonCtrl(DcConArgs args)
{
    if(args.argCount() != 1)
    {
        *_con << "must specify on of off\n";
    }
    else if(args.at(1).toString().toLower().contains("off"))
    {
        enableMidiMonitor(false);
        *_con << "ok" << "\n";
    }
    else if(args.at(1).toString().toLower().contains("on"))
    {
        enableMidiMonitor(true);
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
        
        int pc = args.at(1,_devDetails.PresetCount).toInt();
        if(pc)  {
            _maxPresetCount = pc;
        } else {
            *_con << "preset count is invalid\n";
            return;
        }
        
        _presetOffset = 0;
        
        int poffset = -1;
        QString s = args.at(2,"").toString();
        if(isBankPresetString(s))
        {
            poffset = bankPresetToNum(s);
            if(-1 == poffset)
            {
                *_con << "invalid preset specified\n";
                return;
            }

            _presetOffset = poffset;
        }
        else
        {
            _presetOffset = args.at(2,0).toInt();
        }
        _con->clearCounterDisplay();
        QMetaObject::invokeMethod(ui.fetchButton, "click",Qt::DirectConnection);
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::conCmd_MidiOut( DcConArgs args )
{
    if(args.argCount() > 0)
    {
        DcMidiData md;

        if(_midiOutDecimalMode->isEnabled())
        {
            md = args.decJoin();
        }
        else
        {
            md = args.hexJoin();
        }
        _con->clearCounterDisplay();
        _midiOut.dataOutSplit(md,_maxMsgSize,_delayPerMsgChunk);
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::midiDataInToConHandler( const DcMidiData &data )
{
    if(_con->isVisible())
    {
        _con->execCmd("append " + data.toString());
        *_con << "IN: " << data.toString().trimmed() << "\n";
        _con->incCounterDisplay(data.length());
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::midiDataOutToConHandler( const DcMidiData &data )
{
    if(_con->isVisible())
    {
        *_con << "OUT: " << data.toString() << "\n";
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::on_workList_currentRowChanged(int row)
{
    if(row == -1)
    {
        this->updateWorklistButtonAndAcationState();
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

    QList<DcMidiData> sysexList;
    QList<DcMidiData>* pFilterList = 0;
    QList<DcMidiData> filterList;

    // If the user has provided any filter strings, set them up
    if(args.argCount() > 1)
    {
        
        for (int idx = 0; idx < (args.argCount() - 1); idx++)
        {
        	filterList << args.at(idx + 2).toString();
        }
        pFilterList = &filterList;
    }

    // check for file
    if(!loadSysexFile(fileName,sysexList,pFilterList))
    {
        *_con << _lastErrorMsgStr;
        return;
    }

 
    ioMidiListToDevice(sysexList);

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

    if(_fileDownloader)
    {
        QObject::disconnect(_fileDownloader, &DcFileDownloader::downloaded, this, &DcPresetLib::conDownloadDone);
    }
    else
    {
        _fileDownloader = new DcFileDownloader(url, this);
    }
    
    connect(_fileDownloader, SIGNAL(downloaded()), SLOT(conDownloadDone()));
    
   *_con << "starting download from " << args.at(1).toString() << "\n";
    
}

//-------------------------------------------------------------------------
void DcPresetLib::setupConsole()
{
    // _con is just an alias for ui.console
    _con = ui.console;
    _con->setVisible(false);
    _con->addCmd("fetch",ui.fetchButton,SLOT(clicked( )),"Execute a fetch");
    _con->addCmd("sync",ui.syncButton,SLOT(clicked( )),"Synchronize worklist with device (write preset changes)");

    _con->addCmd("midi.out.hex",_midiOutDecimalMode,SLOT(toggle()),"Toggles using hex or dec values for MIDI out data");

    _con->addCmd("fetch",this,SLOT(conCmd_Fetch(DcConArgs)),"<count> [<offset>] - Fetch 'count'' presets starting at the optional offset");
    _con->addCmd("mon",this,SLOT(conCmd_MidiMonCtrl(DcConArgs)),"<on|off> - display MIDI IN and OUT");
    _con->addCmd("out",this,SLOT(conCmd_MidiOut(DcConArgs)),"<midi hex bytes> - write midi bytes to connected device");

    _con->addCmd("writefile",this,SLOT(conCmd_MidiWriteFile(DcConArgs)),"[<filename>] - open file and write to connected device");
    _con->addCmd("geturl",this,SLOT(conCmd_GetUrl(DcConArgs)),"download specified URL" );

    _con->addCmd("qss",this,SLOT(conCmd_qss(DcConArgs)),"Load the qss file" );
    _con->addCmd("lswl",this,SLOT(conCmd_lswl(DcConArgs)),"list the working list" );

    _con->addCmd("exec",this,SLOT(conCmd_exec(DcConArgs)),"Shell executes the file" );
    _con->addCmd("showlog",this,SLOT(conCmd_showlog(DcConArgs)),"Shell executes the logfile path" );
    _con->addCmd("sm.trace",this,SLOT(conCmd_smtrace(DcConArgs)),"display state machine history" );
    
    _con->addCmd("char",this,SLOT(conCmd_char(DcConArgs)),"Converts the HEX byte strings to ASCII" );
    _con->addCmd("uuid",this,SLOT(conCmd_uuid(DcConArgs)),"Return a UUID");
    _con->addCmd("outn",this,SLOT(conCmd_outn(DcConArgs)),"<count> <midi hex bytes> - write midi bytes to connected device 'count' times" );
    _con->addCmd("bench",this,SLOT(conCmd_timeCall(DcConArgs)),"<count> <command string> - executes the command 'count' times and reports the average run-time in ms." );
    
    _con->addCmd("goboot",this,SLOT(conCmd_switchToBootcode(DcConArgs)),"Transition the Strymon device into boot code" );
    _con->addCmd("exitboot",this,SLOT(conCmd_exitBootcode(DcConArgs)),"Exit from boot code" );
    _con->addCmd("selectbank",this,SLOT(conCmd_changeActiveBootBank(DcConArgs)),"While in boot code, select bank 0 or bank 1" );
    
    _con->addCmd("info",this,SLOT(conCmd_getBootCodeInfo(DcConArgs)),"Display information about the boot code the contents of the code banks" );
    _con->addCmd("ioconf",this,SLOT(conCmd_ioConfig(DcConArgs)),"[<max msg sz> <delay per msg>] Displays or configures MIDI I/O settings" );
    
    
    _con->addCmd("sleep",this,SLOT(conCmd_delay(DcConArgs)),"<time in micro-seconds> - sleep for give time" );

    _con->addCmd("pinit",this,SLOT(conCmd_pinit(DcConArgs)),"Copies the selected preset to any preset that is invalid" );
    _con->addCmd("cpsel",this,SLOT(conCmd_cpsel(DcConArgs)),"Copies the selected presets to the specified stating location" );
    
    _con->addCmd("exportwl",this,SLOT(conCmd_ExportWorklistPresets(DcConArgs)),"Export the work list to given path" );

    _con->addCmd("exportfile",this,SLOT(conCmd_SplitPresetBundle(DcConArgs)),"<source preset file> [<destination path>] - export each preset found in the provided file." );

    _con->addCmd("fastfetch",this,SLOT(conCmd_enableFastFetch(DcConArgs)),"<on|off> - controls the preset fetch size.");

    _con->setBaseDir(_dataPath);

    _con->execCmd("defld default_defs.bin");

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
#if defined(Q_OS_MACX)
    const char* shellCmd = "open"; // Note tested
#elif defined(Q_OS_WIN)
    const char* shellCmd = "explorer.exe";

#else
    *_con << "not implemented\n";
    const char* shellCmd = "?";
     return;
#endif

    if(args.argCount() >= 1)
    {
        //QDesktopServices::openUrl(QUrl(url,QUrl::TolerantMode));
        QProcess::startDetached(shellCmd, args.argsAsStringList());
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::conCmd_enableFastFetch( DcConArgs args )
{
    if(args.noArgs())
    {
        *_con << (gUseAltPresetSize ? "fastfetch is enabled\n" : "fastfetch is disabled\n");
    }
    else if(!args.firstTruthy())
    {
        if(gUseAltPresetSize)
        {
            gUseAltPresetSize = false;
            setFamilyDetails(_devDetails);
        }
        *_con << "fastfetch is disabled\n";
    }
    else if(args.firstTruthy())
    {
        if(!gUseAltPresetSize)
        {
            gUseAltPresetSize = true;
            setFamilyDetails(_devDetails);
        }

        *_con << "fastfetch is enabled\n";
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::conCmd_char( DcConArgs args )
{
    if(args.noArgs())
    {
        *_con << "doc: " << args.cmd() << " " << args.meta("doc") << "\n";
    }
    else
    {
        QString outVal = "";
        DcMidiData md;
        for (int idx = 1; idx < args.count() ; idx++)
        {
            md.append(args.at(idx).toString());
        }

        for (int mdx = 0; mdx < md.length() ; mdx++)
        {
        	char cval = md.data()[mdx];
            outVal += QString(cval);
        }
        *_con << outVal << "\n";
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
void DcPresetLib::conDownloadDone()
{
    QByteArray ba = _fileDownloader->downloadedData();
    *_con << "data received: " << ba.count()/1024.0 << "kB\n";

    qApp->processEvents();
    DcMidiData md(ba);
    QString fname = QDir::toNativeSeparators(_dataPath + "downloads/" + _fileDownloader->getName());
    QFile file(fname);
    file.open(QFile::ReadWrite);
    file.write(md.data(),md.length());
    file.close();
    *_con << "File saved to " << fname << "\n";
}

//-------------------------------------------------------------------------
void DcPresetLib::on_actionShow_Console_triggered()
{
   _con->setVisible(ui.actionShow_Console->isChecked());
}

//-------------------------------------------------------------------------
void DcPresetLib::conCmd_outn( DcConArgs args )
{
    if(args.noArgs())
    {
        *_con << "doc: " << args.cmd() << " " << args.meta("doc") << "\n";
    }
    else
    {
        QString outCmd = "";
        int cnt = args.first().toInt();
        int timems = args.second().toInt();
        for (int idx = 3; idx < args.count() ; idx++)
        {
        	outCmd += args.at(idx).toString();
        }
        _midiOut.dataOut("F0 00 01 55 12 03 1B F7");
        _midiOut.dataOut(outCmd);
        QThread::msleep(timems);
        qApp->processEvents();
        for (int CmdCnt = 0; CmdCnt < cnt ; CmdCnt++)
        {
            _midiOut.dataOut(outCmd);
            qApp->processEvents();
        }
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::conCmd_timeCall( DcConArgs args )
{
    if(args.noArgs())
    {
        *_con << "doc: " << args.cmd() << " " << args.meta("doc") << "\n";
    }
    else
    {
        QString outCmd = "";
        int cnt = args.first().toInt();
        for (int idx = 2; idx < args.count() ; idx++)
        {
            outCmd += args.at(idx).toString() + " ";
        }
        outCmd = outCmd.trimmed();
        
        quint64 delta = 0;
        for (int CmdCnt = 0; CmdCnt < cnt ; CmdCnt++)
        {
            qApp->processEvents();
            quint64 start_time = QDateTime::currentMSecsSinceEpoch();

            // This will not work because the console class queues the handler signal
            _con->execCmd(outCmd,true);

            quint64 stop_time = QDateTime::currentMSecsSinceEpoch();
            delta +=  stop_time - start_time;
            //_con->execCmd("echo");
        }
        
        if(cnt > 0)
        {
            qApp->processEvents();
            QThread::msleep(100);
            qApp->processEvents();

            double d = cnt;
            *_con << delta/d << "ms\n";
        }
        else
        {
            *_con << 0 << "\n";
        }
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::conCmd_delay( DcConArgs args )
{
    if(args.noArgs())
    {
        *_con << "doc: " << args.cmd() << " " << args.meta("doc") << "\n";
    }
    else
    {
        QThread::usleep(args.first().toInt());
    }
}


//-------------------------------------------------------------------------
void DcPresetLib::conCmd_switchToBootcode( DcConArgs args )
{
    Q_UNUSED(args);

    DcBootControl dlfw(_midiIn,_midiOut);
    
    clearMidiInConnections();

    if(false == dlfw.enableBootcode())
    {
        *_con << "Failed to enable Boot-code\n";
    }
    else
    {
       *_con << "success\n";
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::conCmd_getBootCodeInfo( DcConArgs args )
{
    Q_UNUSED(args);
    DcBootControl dcfu(_midiIn,_midiOut);
    DcBootCodeInfo info;    
    
    if(!dcfu.getBootCodeInfo(info))
    {
        *_con << "Device is not running boot code.\n";
    }
    else
    {
        *_con << "Boot Code Version: " << info.getVersion() << "\n";
        *_con << "  Bank 0: " << info.getBank(0).toString() << "\n";
        *_con << "  Bank 1: " << info.getBank(1).toString() << "\n";
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::conCmd_ioConfig( DcConArgs args )
{
    Q_UNUSED(args);
    if(args.noArgs())
    {
        *_con << "MIDI I/O Configuration\n";
        *_con << "------------------------\n";
        *_con << "Max Message Size: " << _maxMsgSize << " bytes\n";
        *_con << "Delay Per Message Chunk: " << _delayPerMsgChunk << "ms\n";
    }
    else
    {
        int maxMsgSize = args.first().toInt();
        int delayPerMsgChunk = args.second().toInt();

        _maxMsgSize = maxMsgSize;
        _delayPerMsgChunk = delayPerMsgChunk;

        QSettings settings;
        settings.beginGroup("midiio");
        settings.setValue("MaxMsgSize", maxMsgSize);
        settings.setValue("DelayPerMsgChunk",delayPerMsgChunk);
        settings.endGroup();
    }
}


void DcPresetLib::copySelectedPresets( QString numAndBank )
{
    QList<QListWidgetItem *> itemsToMove = ui.workList->selectedItems();
    
    if(!itemsToMove.count())
    {
        return;
    }
    
    if(!isBankPresetString(numAndBank))
    {
        return;
    }

    // Verify that the destination preset exists in the currently loaded worklist
    int dstPresetNum = bankPresetToNum(numAndBank);
    int firstPresetNumberInWL = getPresetNumber(_workListData.first());
    int lastPresetNumberInWl = getPresetNumber(_workListData.last()) + 1;
    if(dstPresetNum < firstPresetNumberInWL || dstPresetNum+itemsToMove.count() > lastPresetNumberInWl )
    {
        dispErrorMsgBox("The destination preset/bank is not with in the preset/bank range of the current worklist");
        return;
    }
    
    // The destination is within the worklist, so this calculation will work:
    int dstIdx = dstPresetNum - firstPresetNumberInWL;

    // Store all the preset data
    QList<DcMidiData> selectedPresets;
    foreach(QListWidgetItem* item,itemsToMove)
    {
        int idx = ui.workList->row(item);

        DcMidiData srcPreset = _workListData.at(idx);
        selectedPresets.append(srcPreset);
    }

    // Copy the presets
    foreach(DcMidiData md, selectedPresets)
    {
        md.set14bit(_devDetails.PresetNumberOffset,dstPresetNum++);
        _workListData.replace(dstIdx++,md);
    }

    checkSyncState();
}


void DcPresetLib::initInvalidPresets()
{
    
    DcMidiData srcPreset = _workListData.at(ui.workList->currentRow());

    // compare the work list with the device list and update the dirty list
    int len = _workListData.length();

    for (int i = 0; i < len; ++i) 
    {
        if(_workListData.at(i).contains("F0 00 01 55 XX XX 62 XX XX 47 F7"))
        {
            int pnum = getPresetNumber(_workListData.at(i));
            srcPreset.set14bit(_devDetails.PresetNumberOffset,pnum);
            _workListData.replace(i,srcPreset);
        }
    }

    checkSyncState();
}

//-------------------------------------------------------------------------
void DcPresetLib::conCmd_pinit( DcConArgs args )
{
    Q_UNUSED(args);
    initInvalidPresets();
}

//-------------------------------------------------------------------------
quint16 DcPresetLib::getPresetNumber( const DcMidiData &preset )
{
    return preset.get14bit(_devDetails.PresetNumberOffset,-1);
}



bool DcPresetLib::programSysexFile(QString fileName)
{
    QList<DcMidiData> sysexList;

    // check for file
    if(!loadSysexFile(fileName,sysexList))
    {
        // TODO: log error
        return false;
    }

    int outputStat= sysexList.count()/20;
    int donecnt = 0;
    for (int i = 0; i < sysexList.count() ; i++)
    {
        _midiOut.dataOutSplit(sysexList.at(i),_maxMsgSize,_delayPerMsgChunk);
        if(i % outputStat == 0)
        {
            *_con << (donecnt*5) << "% complete " << "\n";
            donecnt++;
        }
        QApplication::processEvents();
    }

    return true;
}

//-------------------------------------------------------------------------
bool DcPresetLib::midiDataToTextFile( QString fileName, QList<DcMidiData> cmdList )
{
    QFile fout(fileName);
    if (!fout.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }

    QTextStream out(&fout);

    foreach(DcMidiData md, cmdList)
    {
        out << md.toString(' ') << "\n";            
    }
    fout.close();
    
    return true;
}

bool DcPresetLib::stringListToTextFile( QString fileName, QStringList strList )
{
    QFile fout(fileName);
    if (!fout.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }

    QTextStream out(&fout);
    out << "Preset Name,Location,Type,Author\n";

    foreach(QString str, strList)
    {
        out << str << "\n";            
    }
    fout.close();

    return true;
}

bool DcPresetLib::midiDataToTextFile( QString fileName, DcMidiData data )
{
    QFile fout(fileName);
    if (!fout.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }

    QTextStream out(&fout);
    out << data.toString(' ') << "\n";            
    
    fout.close();

    return true;
}

bool DcPresetLib::midiDataToBinFile( QString fileName, DcMidiData data )
{
    QFile fout(fileName);
    if (!fout.open(QIODevice::WriteOnly))
    {
        return false;
    }
    fout.write(data.toByteArray());
    
    fout.close();

    return true;
}


bool DcPresetLib::fetchFileFromUrl(QString fromurl, QString topath)
{
    QUrl url(fromurl);
    DcFileDownloader* dl = new DcFileDownloader(url, this);
    dl->saveData(topath);
    delete dl;
    return true;
}

//-------------------------------------------------------------------------
void DcPresetLib::recvIdDataTrigger( void* mt )
{
    Q_UNUSED(mt);
    _machine.postEvent(new DetectDevice());
}



//-------------------------------------------------------------------------
void DcPresetLib::conCmd_cpsel( DcConArgs args )
{
    if(args.noArgs())
    {
        *_con << "You must supply the starting destination preset\n";
    }
    else
    {
        // Create a preset index from the input (input should be in Num/Bank format)
        QString numBank = args.first("").toString();
        
        if(numBank.isEmpty())
        {
            *_con << "Specify the destination in the NUM/BANK format, e.g. 02A or 34C\n";
        }
        else
        {
            copySelectedPresets(numBank);
        }

    }
}

//-------------------------------------------------------------------------
void DcPresetLib::conCmd_ExportWorklistPresets( DcConArgs args )
{
    if(!args.noArgs())
    {
        QString dstPath = args.first("").toString();
        QString username = QFileInfo(QDir::homePath()).baseName();
        QString path = QDir::toNativeSeparators(dstPath + "/" + username + "_preset_wl");
        QDir().mkpath(path);
        QStringList nameList;

        QFile fout(QDir::toNativeSeparators(path + "/" + "preset_info.csv"));

        if (!fout.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            return;
        }
        QTextStream out(&fout);
        out << "Preset Name,Location,Type\n";
        foreach(DcMidiData p,_workListData)
        {
            if(p.contains("47 F7"))
                continue;
            QString location = getPresetBankPresetNumber(p);
            QString presetName = getPresetName(p);
            QString etype = getEffectType(p);
            out << presetName + "," + location + (etype.isEmpty() ? "\n" : QString("," + etype + "\n"));

            QString name = location + "_" + presetName + (etype.isEmpty() ? "\n" : QString("_" + etype));

            name = QDir::toNativeSeparators(path + "/" + name + ".syx");
            
            midiDataToBinFile(name,p);
        }

        fout.close();
        *_con << "The worklist has been exported to this directory: " << path << "\n";
    }
    else
    {
        *_con << "supply a destination path.  e.g. exportwl c:\\my_preset\\exports\n";
    }
}


//-------------------------------------------------------------------------
void DcPresetLib::conCmd_SplitPresetBundle( DcConArgs args )
{
    if(!args.noArgs())
    {
        // First arg is the source file
        QString fileName = args.first("").toString();

        // Second arg is the destination path
        QString destPath = args.second("").toString();

        exportPresetBundleToPath(fileName, destPath);
    }
}

//-------------------------------------------------------------------------
void DcPresetLib::setFamilyDetails( DcDeviceDetails &details )
{
    if(gUseAltPresetSize)
    {
        details.PresetRd_NAK.setPattern(details.SOXHdr.toString() + QLatin1String("(67....47|47)F7"));
        details.PresetRd_ACK.setPattern(details.SOXHdr.toString() + QLatin1String("67"));
        details.PresetReadTemplate      = details.SOXHdr.toByteArray(' ') + " 67 p14 F7";
    }
    else
    {
        details.PresetRd_NAK.setPattern(details.SOXHdr.toString() + QLatin1String("(62....47|47)F7"));
        details.PresetRd_ACK.setPattern(details.SOXHdr.toString() + QLatin1String("62"));
        details.PresetReadTemplate      = details.SOXHdr.toByteArray(' ') + " 63 p14 F7";
    }


    details.PresetWriteHdr          = details.SOXHdr + "62";
    details.FactoryPresetWriteHdr   = details.SOXHdr + "65";

    details.PresetRdResponce_NACK.setPattern(details.SOXHdr.toString() + QLatin1String("(62....47|47)F7"));
    

    details.PresetWr_NAK.setPattern(details.SOXHdr.toString() + QLatin1String("....46F7"));
    details.PresetWr_ACK.setPattern(details.SOXHdr.toString() + QLatin1String("....45F7"));
                                                                  
    
    details.PresetSize              = 650;
    details.PresetNumberOffset      = 7;
    details.PresetStartOfDataOffset = 9;
    details.PresetNameLen           = 16;
    details.PresetNameOffset        = 632;
    details.PresetChkSumOffset      = details.PresetSize - 2;
    details.PresetDataLength        = details.PresetChkSumOffset - details.PresetStartOfDataOffset;
}

//-------------------------------------------------------------------------
bool DcPresetLib::updateDeviceDetails( const DcMidiData &data,DcDeviceDetails& details )
{
    bool rtval = true;

    if(data.contains(DcMidiDevDefs::kTimeLineIdent) || data.contains("0001551201"))
    {
        setFamilyDetails(details);

        details.Name                    = "TimeLine";
        details.PresetsPerBank          = 2;
        details.PresetCount             = 200;


    }
    else if(data.contains(DcMidiDevDefs::kMobiusIdent) ||  data.contains("0001551202") )
    {
        setFamilyDetails(details);
        
        details.Name                      = "Mobius";
        details.PresetsPerBank          = 2;
        details.PresetCount             = 200;


    }
    else if(data.contains(DcMidiDevDefs::kBigSkyIdent) ||  data.contains("0001551203"))
    {
        setFamilyDetails(details);
        
        details.Name                      = "Big Sky";
        details.PresetsPerBank          = 3;
        details.PresetCount             = 300;
    }
    else
    {
        rtval = false;
    }

    return rtval;
}




//-------------------------------------------------------------------------
QString DcPresetLib::getEffectType( const DcMidiData &data )
{
    enum {
        BLOOM = 0,
        DUST,
        CHORALE,
        SHIMMER_1,
        MAG_REVERSE,
        NONLINEAR,
        REFLECTIONS,
        ROOM,
        HALL,
        PLATE,
        SPRING,	
        SWELL,
        NUM_EFFECT_TYPES
    } ;


    int etype = data.at(_devDetails.PresetNumberOffset + 2);
    QString rtval = "Unknown";
    
    if(data.contains("01551203"))
    {
        switch (etype)
        {
        case BLOOM:
            rtval = "Bloom";
    	    break;
        case DUST:
            rtval = "Cloud";
            break;
        case CHORALE:
            rtval = "Chorale";
            break;
        case SHIMMER_1:
            rtval = "Shimmer";
            break;
        case MAG_REVERSE:
            rtval = "Magneto";
            break;
        case NONLINEAR:
            rtval = "Nonlinear";
            break;
        case REFLECTIONS:
            rtval = "Reflection";
            break;
        case ROOM:
            rtval = "Room";
            break;
        case HALL:
            rtval = "Hall";
            break;
        case PLATE:
            rtval = "Plate";
            break;
        case SPRING:
            rtval = "Spring";
            break;
        case SWELL:
            rtval = "Swell";
            break;
        default:
            break;
        };
    }

    return rtval;

}

//-------------------------------------------------------------------------
void DcPresetLib::exportPresetBundleToPath( QString fileName, QString &destPath )
{
    QList<DcMidiData> sysexList;
    QList<DcMidiData> filterList;

    filterList << "47 F7";

    if(!loadSysexFile(fileName,sysexList,&filterList))
    {
        *_con << _lastErrorMsgStr;
        return;
    }

    QString path;


    if(destPath.isEmpty())
    {
        path  = QFileInfo(fileName).absolutePath();
    }
    else
    {
        path = destPath;
    }

    QString exportDirName  = QFileInfo(fileName).baseName();
    exportDirName.replace(" ","_");

    path = QDir::toNativeSeparators(path + "/" + exportDirName);

    // Make the preset export directory
    QDir().mkpath(path);

    DcDeviceDetails d = _devDetails;
    
    QFile fout(QDir::toNativeSeparators(path + "/" + "preset_info.csv"));

    if (!fout.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return;
    }
    
    QTextStream out(&fout);
    out << "Preset Name,Location,Type\n";

    updateDeviceDetails(sysexList[0],_devDetails);

    foreach(DcMidiData p,sysexList)
    {
        QString location = getPresetBankPresetNumber(p);
        QString presetName = getPresetName(p);
        QString etype = getEffectType(p);

        out << presetName + "," + location + (etype.isEmpty() ? "\n" : QString("," + etype + "\n"));
        
        QString name = location + "_" + presetName + (etype.isEmpty() ? "\n" : QString("_" + etype));
                
        name = QDir::toNativeSeparators(path + "/" + name + ".syx");

        midiDataToBinFile(name,p);
    }
    fout.close();

    _devDetails = d;
}

//-------------------------------------------------------------------------
void DcPresetLib::conCmd_exitBootcode( DcConArgs args )
{
   Q_UNUSED(args);
   DcBootControl bootctrl(_midiIn,_midiOut);

    if(!bootctrl.isBootcode())
    {
        *_con << "Can't exit, not in boot mode\n";
    }
    else
    {
        if(!bootctrl.exitBoot())
        {
            *_con << "Possible issue exiting boot mode\n";
        }
        else
        {
            _machine.postEvent(new VerifyDeviceConnection());
        }
    }
    
}

//-------------------------------------------------------------------------
void DcPresetLib::ioMidiListToDevice( QList<DcMidiData> &sysexList )
{
    bool prevState = enableMidiMonitor(false);
    _con->setInputReady(false);

    int outputStat= sysexList.count()/20;
    int donecnt = 0;
    for (int i = 0; i < sysexList.count() ; i++)
    {
        _midiOut.dataOutSplit(sysexList.at(i),_maxMsgSize,_delayPerMsgChunk);
        if(i % outputStat == 0)
        {
            *_con << (donecnt*5) << "% complete " << "\n";
            donecnt++;
        }
        QApplication::processEvents();

    }

    enableMidiMonitor(prevState);
    _con->setInputReady(true);
}

//-------------------------------------------------------------------------
void DcPresetLib::conCmd_changeActiveBootBank( DcConArgs args )
{
    int bank = args.first(-1).toInt();
    if(-1 == bank || bank > 1)
    {
        *_con << "invalid argument, must be 0 or 1";
    }
    else
    {
        DcBootControl bctrl(_midiIn,_midiOut);
        if(bctrl.activateBank(bank))
        {
            *_con << "Bank " << bank << " is now active\n";
        }
    }
}

void DcPresetLib::on_actionShow_Update_Pandel_triggered()
{

    QMessageBox* msgBox = new QMessageBox(this);
    msgBox->setModal(true);
    
    if(_dirtyItemsIndex.length())
    {
        if (QMessageBox::Discard != QMessageBox::question(this, "Unsaved Worklist Warning",
            "You have unsaved presets in the Worklist.\nAre you sure you want to update the device firmware?",
            QMessageBox::Discard | QMessageBox::Abort,
            QMessageBox::Abort))
        {
            return;
        }
        
        
        clearWorklist();
    }

    DCLOG() << "User clicked Update, VersionString =  " << _devDetails.FwVersion;
   
    if( _devDetails.FwVersion.isEmpty()  )
    {
        DCLOG() << "A device was not present ";
        
        msgBox->setWindowTitle(QLatin1String("Unable to Complete the Software Update"));
        msgBox->setText("<h2>Software update can not find a device.</h2>Please try using the menu 'Settings->MIDI Setup' and verify the device connection.");
        msgBox->setIcon(QMessageBox::Information);
        msgBox->exec();
        return;
    }
    else if( _devDetails.FwVersion[0] == 'B')
    {
        DCLOG() << "The device is running boot code, version = " << _devDetails.FwVersion;
        
        msgBox->setWindowTitle(QLatin1String("Unable to Complete the Software Update"));
        msgBox->setText("<h2>Unable to complete the software update.</h2>The connected device seems to \
                        be in \"update/boot mode.\"<br><br>Try this:<ol><li>Restart the device by unplugging \
                        the power.</li><li>From the menu 'Settings->MIDI Setup,' verify the device connection.</li></ol>");

        msgBox->setIcon(QMessageBox::Information);
        msgBox->exec();
        return;

    }

    msgBox->setWindowTitle(QLatin1String(""));

    clearMidiInConnections();

    bool prevState = enableMidiMonitor(false);
    _con->setInputReady(false);

    DcBootControl bctrl(_midiIn,_midiOut);

    DcUpdateDialogMgr* udmgr = new DcUpdateDialogMgr(_updatesPath,&bctrl,_devDetails,this);
    DcUpdateDialogMgr::DcUpdate_Result result = udmgr->getLatestAndShowDialog();
    
    if(result == DcUpdateDialogMgr::DcUpdate_Failed)
    {
        msgBox->setIcon(QMessageBox::Warning);
        msgBox->setText(udmgr->getLastErrorMsg());
        msgBox->exec();
    }
    else if(result != DcUpdateDialogMgr::DcUpdate_SuccessNoUpdate)
    {

        msgBox->setText(_lastErrorMsgStr);

        if(result == DcUpdateDialogMgr::DcUpdate_Cancled)
        {
            msgBox->setText("The update was cancled.");
            msgBox->setIcon(QMessageBox::Warning);
        }
        else if(result == DcUpdateDialogMgr::DcUpdate_Failed || result == DcUpdateDialogMgr::DcUpdate_PatchFileUpdateFailure)
        {
            msgBox->setWindowTitle("Update Error");
            msgBox->setText(QLatin1String("The update was unable to complete"));
            msgBox->setIcon(QMessageBox::Critical);
        }
        else if(result == DcUpdateDialogMgr::DcUpdate_Success)
        {
            msgBox->setText(QLatin1String("The update has completed successfully."));
            msgBox->setIcon(QMessageBox::Information);
        }
        else if(result == DcUpdateDialogMgr::DcUpdate_PresetUpdateCancled)
        {
            msgBox->setText(QLatin1String("The firmware update was successful.\n\nHowever, the optional preset update was cancled."));
            msgBox->setIcon(QMessageBox::Warning);
        }
        else if(result == DcUpdateDialogMgr::DcUpdate_PresetUpdateFailure)
        {
            msgBox->setText(QLatin1String("The firmware update was successful.\n\nHowever, the optional preset update failed to complete."));
            msgBox->setIcon(QMessageBox::Warning);
        }

        msgBox->setStandardButtons(QMessageBox::Ok);
        QApplication::processEvents();            
        msgBox->exec();
    }
    
    enableMidiMonitor(prevState);
    _con->setInputReady(true);
    _machine.postEvent(new VerifyDeviceConnection());

}

//-------------------------------------------------------------------------
void DcPresetLib::clearMidiInConnections()
{
    QObject::disconnect(&_midiIn, &DcMidiIn::dataIn, this, &DcPresetLib::recvIdData);
    QObject::disconnect(&_midiIn, &DcMidiIn::dataIn, &_xferInMachine, &DcXferMachine::replySlotForDataIn);
    QObject::disconnect(&_midiIn, &DcMidiIn::dataIn, &_xferOutMachine, &DcXferMachine::replySlotForDataOut);
    _midiIn.removeTrigger(*_idResponceTrigger);
}


QByteArray DcPresetLib::makeProductUID(DcDeviceDetails& details)
{
    QByteArray ba = QUuid::createUuid().toRfc4122();
    QString str = details.getShortId();
    QByteArray idba;
    idba.append(str);

    for (int i = 0; i < ba.length() ; i++)
    {
        quint8 bt = ba.at(i) ^ idba.at(0);
        ba[i] = bt;
    }

    for (int i = 0; i < ba.length() ; i++)
    {
        quint8 bt = ba.at(i) ^ idba.at(1);
        ba[i] = bt;
    }

    QCryptographicHash sha256(QCryptographicHash::Sha256);

    sha256.addData(ba);
    QByteArray rtval = sha256.result().mid(3,8);
    for (int i = 0; i < 8 ; i++)
    {
        quint8 bt = rtval.at(i)&0x7F;
        rtval[i] = bt;
    }

    return rtval;
}
//-------------------------------------------------------------------------
void DcPresetLib::conCmd_uuid( DcConArgs args )
{
    Q_UNUSED(args);
    DcMidiData md = makeProductUID(_devDetails);
    *_con << md.toString(' ') << "\n";
}

//-------------------------------------------------------------------------
void DcPresetLib::conCmd_showlog( DcConArgs args )
{
    _con->execCmd("exec logfile");
}

//-------------------------------------------------------------------------
void DcPresetLib::clearWorklist()
{
    _workListData.clear();
    _dirtyItemsIndex.clear();
    ui.workList->clear();
}
//-------------------------------------------------------------------------
void DcPresetLib::clearDeviceUI()
{
    QPixmap pm = QPixmap(":/images/res/devunknown_100.png");

    ui.devImgLabel->setPixmap(pm);
    ui.devInfoLabel->setText("");
    ui.devImgLabel->setToolTip("");
}
