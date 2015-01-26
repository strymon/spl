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
#ifndef DcPresetLib_H
#define DcPresetLib_H

#include <QtWidgets/QMainWindow>
#include <QStandardItemModel>
#include <QTextStream>
#include <QTimer>
#include <QStateMachine>
#include <QDir>


#include "DcMidi/DcMidiData.h"
#include "DcMidi/DcMidiIn.h"
#include "DcMidi/DcMidiOut.h"

#include "ui_DcPresetLib.h"
#include "PresetLibSMDefs.h"
#include "IoProgressDialog.h"
#include "DcXferMachine.h"
#include "DcDeviceDetails.h"
#include "DcFileDownloader.h"

#include "dcconbool.h"

#include <QMutex>
#include "cmn/DcQUtils.h"
#include "cmn/DcLog.h"
// #include "DcDeviceManager.h"


class MidiSettings;


class DcPresetLib : public QMainWindow
{
    Q_OBJECT

public:

    DcPresetLib(QWidget *parent = 0);
    ~DcPresetLib();

//    // Plugin Test Code
//    void loadConsolePlugins();
//    QDir locatePluginsPath();


signals:

// State Machine Signals
    // TODO: Convert these to events 
    void deviceReady();
    void deviceNotFound();
    void readPresets_setupDone_signal();
    void writePresets_setupDone_signal();

private slots:

    void on_devImgClicked();
    void on_workList_itemSelectionChanged();
    void on_workList_itemDoubleClicked();
    void on_workList_currentRowChanged(int row);
    void on_actionSave_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_One_triggered();
    void on_actionMove_triggered();
    void on_actionRename_triggered();
    void on_actionLoad_One_triggered();
    void on_actionShow_Console_triggered();
    void on_actionAbout_triggered();
    void on_actionShow_Update_Pandel_triggered();

    void UpdateFirmwareHelper(const QString& FirmwareFile = QString());

    void on_fileDropped( const QString& fileName );

    /*!
      Display a file open dialog
    */ 
    QString execOpenDialog( QString lastOpened );

    /*!
      Display an error message dialog for the given c string
    */ 
    void dispErrorMsgBox(const char* msg =  0);
    

    /*!
       When connected to a DcMidiIn object, this slot/method expects to receive
       MIDI Identify Response data.  The method participates in the device detections and
       identification process, that is, it also expects some other stuff to be setup
       (like a watchdog timer, etc..., yes, very complected indeed)
    */ 
    void recvIdData(const DcMidiData &data);
   
    /*!
      Identity response MIDI "trigger" slot.  Method
      is assigned to the "Identify Device" MIDI trigger.
      
      This is a second method/way the system receives MIDI
      identity data.
      
      It's called with Identity response data is detected.
      The method then posts the "DetectDevice" Event to
      transition the state machine.
    */ 
    void recvIdDataTrigger(void* mt);
    
//    void portScanTrigger(void* mt);


    void devIdWatchDogHandler();
    void setupStateMachineHandler();

// State machine handlers
    void readPresetsComplete_entered();
    void writePresetsComplete_entered();
    void setupWritePresetXfer_entered();
    void midiPortSelect_enter();
    void presetEdit_entered();
    void setupReadPresetXfer_entered();
    void userCanFetch_entered();
    void noDevice_entered();

    void clearDeviceUI();

    void detectDevice_entered();
    void erroRecovery_entered();
    void shutdownMidiIo();

    void conCmd_qss( DcConArgs args );
    void conCmd_lswl( DcConArgs args );
    void conCmd_exec( DcConArgs args );
    void conCmd_showlog( DcConArgs args );
    void conCmd_char( DcConArgs args );
    void conCmd_uuid( DcConArgs args );
    void conCmd_outn( DcConArgs args );
    void conCmd_timeCall( DcConArgs args );
    void conCmd_delay( DcConArgs args );
    void conCmd_getBootCodeInfo( DcConArgs args );
    void conCmd_switchToBootcode( DcConArgs args );
    void conCmd_exitBootcode( DcConArgs args );
    void conCmd_changeActiveBootBank( DcConArgs args );
    void conCmd_smtrace( DcConArgs args );
    void conCmd_ioConfig( DcConArgs args );
    void conCmd_pinit( DcConArgs args );
    void conCmd_cpsel( DcConArgs args );
    void conCmd_MidiMonCtrl(DcConArgs args);
    void conCmd_MidiOut(DcConArgs args);

    void conCmd_MidiWriteFile(DcConArgs args);
    void conCmd_ExportWorklistPresets(DcConArgs args);
    void conCmd_SplitPresetBundle( DcConArgs args );

    void conCmd_enableFastFetch(DcConArgs args);

    //void conCmd_devTestExec( DcConArgs args );
    //void launchDeviceTester( DcConArgs &args );
    //void conCmd_portTestExec( DcConArgs args );

    void conCmd_GetUrl(DcConArgs args);
    void conDownloadDone();
    void conCmd_Fetch(DcConArgs args);
    
    void conCmd_UpdateFirmware( DcConArgs args );
    void conCmd_RenameItemInWorklist( DcConArgs args );

    /*!
      For the given preset bundle file path, export each preset
      into separate files, each using the preset name as the file name.
      The destination path is also specified.
    */ 
    void exportPresetBundleToPath( QString fileName, QString &destPath );

    /*!
      Transform the given MIDI data list to text (HEX) and save using the 
      given file name.
    */ 
    bool midiDataToTextFile( QString fileName, QList<DcMidiData> dataList);
    
    /*!
        write the given data to the specified file as a text file.
        Bytes are convert to HEX.
    */ 
    bool midiDataToTextFile( QString fileName, DcMidiData data);
    
    /*!
      MIDI Monitor: data IN to console handler
    */ 
    void midiDataInToConHandler(const DcMidiData &data);
    
    /*!
      MIDI Monitor data OUT to console handler
    */ 
    void midiDataOutToConHandler(const DcMidiData &data);

private:
    

    /*!
      Enable/disable the console MIDI monitor,
      The console MIDI monitor is a pig.
    */ 
    bool enableMidiMonitor(bool enable);


    /*!
      Transmit the given data  to the device
    */ 
    void ioMidiListToDevice( QList<DcMidiData> &sysexList );

    void clearMidiInConnections();
    
    void updateWorkListFromDeviceList();

    /*!
      Method is given a ref to the device details var and updates the object
      with the basic identity information contained in the Identify response data
      received in response to an MIDI identity request.
    */ 
    bool updateDeviceDetails( const DcMidiData &data,DcDeviceDetails& details );

    /*!
      Method will modify the given "partially initialized" details object
      with further, family specific, Strymon MIDI device configuration.
    */ 
    void setFamilyDetails( DcDeviceDetails &details );
    /*!
      Return the effect type contain in the given 
      preset MIDI data.
    */ 
    QString getEffectType( const DcMidiData &data);
    
    /*!
      Returns true if the given string is a valid Bank/PresetNum
      For example: 2A or 23C
    */ 
    bool isBankPresetString( QString s );
    
    /*!
      Transform a bank preset string (e.g. 12B) into its linear integer 
      equivalent.
    */ 
    int bankPresetToNum( QString s );

    /*!
      Translate the list of patch files to the current devices bank/patch name format      
    */ 
    QStringList presetListToBankPatchName(QList<DcMidiData>& listData);

    /*!
      For the given linear preset number, return a string that represents its
      Bank/Num string. e.g. Assume a device has 2 presets per bank (A and B), 
      then the first 5 presets would generate 0A,0B 1A 1B 2A, so 5 = 2A
    */ 
    QString presetNumberToBankNum(int i);

    QString getPresetBankPresetNumber(const DcMidiData& preset );

    quint16 getPresetNumber( const DcMidiData &preset );

    QString presetToName(DcMidiData& p);

    QString presetToBankPatchName(DcMidiData& p);
   
    /*!
      Assumes the given MIDI data contains a preset, the method
      calculates the preset checksum and updates the given object.
    */ 
    void updatePresetChecksum(DcMidiData &md);

    // Compares the device and work lists, updates the dirtyItems
    // list and decorates the work list names if found different.
    // Will also generate WorkListIsDifferntThanDeviceListEvent and
    // WorkListIsSameAsDeviceListEvent.
    bool checkSyncState();

    /*!
      Update the worklist view by transforming the worklist data
      into the correct ListBox item strings.
    */ 
    void drawWorklist();

    /*!
      Update the state of the actions and buttons that
      depend of the state of the worklist.
    */ 
    void updateWorklistButtonAndAcationState();

    
    /*!
      Clear worklist data and view
    */ 
    void clearWorklist();

    /*!
      Clear the read only console symbols, then defined a few.

      Read only symbols in the console can be defined.  This app
      defines things like 'root path to the users 'user data path' so it can 
      be accessed via the console command line.
    */ 
    void conResetReadOnlySymbolDefines();

    /*!
      Method to wrap the console method registration
    */ 
    void setupConsole();

    DcLog* _log;

    Ui::DcPresetLibClass ui;

    // MIDI IO Settings Dialog
    MidiSettings*       _midiSettings;

    
    // State Machine
    QStateMachine _machine;


 
    
    DcConsoleForm* _con;
    
    // 
    
    // Preset Data transfer helper objects
    DcXferMachine _xferInMachine;
    DcXferMachine _xferOutMachine;


    // MOVED TO PEDAL CLASS
    QList<DcMidiData>     _deviceListData;
    QList<DcMidiData>     _workListData;
    QList<int>      _dirtyItemsIndex;
    unsigned int _workListDataDeviceUid; // The device from which the data was received
    // MIDI IO Stuff
    DcMidiIn           _midiIn;
    DcMidiOut          _midiOut;

    IoProgressDialog* _progressDialog;

    QTimer          _watchdog_timer;

    // UI Elements that are activated when something in the workList is selected
    QList<QWidget*> _workListControls;
    QList<QAction*> _workListActions;

    DcDeviceDetails _devDetails;

    // The console can limit this range, a devices support for
    // max preset is independent.
    int _maxPresetCount;
    int _presetOffset;

    QTextStream _lastErrorMsg;
    QString _lastErrorMsgStr;
    
    QString _worklistBackupPath;
    QString _devlistBackupPath;
    QString _dataPath;
    QString _updatesPath;
    bool _backupEnabled;

    QStringList _styleHistory;

    DcFileDownloader* _fileDownloader;
    QStringList _stateTrace;

    // A set of devices
    QSet<const char*> _supportedDevicesSet;

    DcConBool*  _midiOutDecimalMode;


    int         _maxMsgSize;
    int         _delayPerMsgChunk;
    
    //DcPortNamePairList_t _tstDevList;

protected:


    void closeEvent(QCloseEvent *e);

    void setupFilePaths();
    
    // QSettings helpers
    void writeSettings();
    void readSettings();


    bool savePresetBinary(const QString &fileName,const QList<DcMidiData>& dataList);
    bool savePresetBinary(const QString &fileName,const DcMidiData& md);
    bool loadPresetBinary(const QString &fileName,QList<DcMidiData>& dataList);
    bool loadPresetBinary( const QString &fileName,DcMidiData& md );
    void backupDeviceList();
    void backupWorklist();
    
    bool loadSysexFile( const QString &fileName,QList<DcMidiData>& dataList, QList<DcMidiData>* pRejectDataList  = 0 );
    bool hasDevSupport( const DcMidiData &data );
    void initInvalidPresets();
   
    QString getPresetName(DcMidiData &md);
    void setPresetName(DcMidiData &md, QString name);
    bool fetchFileFromUrl(QString u, QString d);
    QByteArray makeProductUID(DcDeviceDetails& details);
    bool programSysexFile(QString fileName);
    void copySelectedPresets(QString destWLIndex);
    bool midiDataToBinFile( QString fileName, DcMidiData data );
    bool stringListToTextFile( QString fileName, QStringList strList );
    void renameItemInWorklist(int row,const QString& newName);
   
    //    bool portScan();
    DcMidiTrigger* _idResponceTrigger;
    
    DcMidiTrigger* _portScanTrigger;

#ifdef DCSIO_FEATURE
        DcQSio* _sio;
#endif

    //    QDir  _pluginsDir;



};

#endif // DcPresetLib_H
