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

#include "QRtMidi/QRtMidiData.h"
#include "QRtMidi/QRtMidiIn.h"
#include "QRtMidi/QRtMidiOut.h"

#include "ui_DcPresetLib.h"
#include "PresetLibSMDefs.h"
#include "IoProgressDialog.h"
#include "DcXferMachine.h"
#include "DcDeviceDetails.h"
#include "DcFileDownloader.h"

class QRtMidiSettings;

class DcPresetLib : public QMainWindow
{
    Q_OBJECT

public:
    DcPresetLib(QWidget *parent = 0);

    void setupConsole();

    ~DcPresetLib();


signals:

// State Machine Signals
    void deviceReady();
    void deviceNotFound();
    
    // ReadPresets
    void readPresets_setupDone_signal();
    void writePresets_setupDone_signal();

    // Process DataIn
    void procDataIn_cmdListEmtpy_signal();
    void procDataIn_NACK_signal();
    void procDataIn_ACK_signal();
    void procDataIn_ready_signal();
    void procDataIn_timeout_signal();
    void procDataIn_failure_signal();
    

    void test_signal();

private slots:

    void conCmd_MidiMonCtrl(DcConArgs args);
    void conCmd_MidiOut(DcConArgs args);
    void conCmd_MidiWriteFile(DcConArgs args);
    void conCmd_GetUrl(DcConArgs args);
    void conCmd_Fetch(DcConArgs args);

    // Work list and it's buttons 
    void on_workList_itemSelectionChanged();

    void updateWorklistControlState();

    void on_workList_itemDoubleClicked();
    void on_workList_currentRowChanged(int row);

    // Top button handlers
    void on_actionSave_triggered();
    void on_actionOpen_triggered();

    QString execOpenDialog( QString lastOpened );


    // Side button handlers
    void on_actionMove_triggered();
    void on_actionRename_triggered();
    void on_actionLoad_One_triggered();
    void on_actionShow_Console_triggered();
         


    void dispLastErrorMsgBox();

    void on_actionSave_One_triggered();

    // StateMachine
    void setupStateMachine();

    // StateMachine - Transition Slots
    void detectDevice_entered();

    void shutdownMidiIo();


    void appDataClear();

    void recvIdData(const QRtMidiData &data);
    void midiDataInToConHandler(const QRtMidiData &data);
    void midiDataOutToConHandler(const QRtMidiData &data);
    void devIdWatchDog();
    void userCanFetch_entered();
    void noDevice_entered();
    void setupReadPresetXfer_entered();
    void readPresetsComplete_entered();
    void writePresetsComplete_entered();

    void syncWorklistToDevList();

    void setupWritePresetXfer_entered();


    void midiPortSelect_enter();
    void presetEdit_entered();


    void on_actionAbout_triggered();
    // void on_actionQt_triggered();

    // Knob Frame Handlers
    void on_dial0_sliderMoved(int vv);
    void on_dial1_sliderMoved(int vv);
    void on_dial2_sliderMoved(int vv);
    void on_dial3_sliderMoved(int vv);
    void on_dial4_sliderMoved(int vv);
    void on_dial5_sliderMoved(int vv);
    void on_dial6_sliderMoved(int vv);
    void on_dial7_sliderMoved(int vv);
    void on_vs0_sliderMoved(int vv);

    void conCmd_qss( DcConArgs args );
    void conCmd_lswl( DcConArgs args );
    void conCmd_exec( DcConArgs args );
    void conCmd_smtrace( DcConArgs args );
    
    void downloadDone();

private:
    
    // Compares the device and work lists, updates the dirtyItems
    // list and decorates the work list names if found different.
    // Will also generate WorkListIsDifferntThanDeviceListEvent and
    // WorkListIsSameAsDeviceListEvent.
    bool checkSyncState();
    void drawWorklist();




    Ui::DcPresetLibClass ui;

    // MIDI Stuff
    QByteArray          _SysexHdr;
    QByteArray          _dataInNACK;
    QByteArray          _ReadPresetCmd;

    QRtMidiIn           _midiIn;
    QRtMidiOut          _midiOut;
    QRtMidiSettings*    _midiSettings;
    
    // State Machine
    QStateMachine _machine;
    DcXferMachine _xferInMachine;
    DcXferMachine _xferOutMachine;

    
    // Event Types
    InPresetEditEvent _fetchSuccess;


    // Console
    //QTextStream _con;
    QString _consoleMsg;
    QString _sayView;
    QStringList _dbgOutputList;

    DcConsoleForm* _con;

    QStringList _history;
    int _historyIndex;

    // Process DataIn
    void procDataIn_setup(int xferTimeout_ms=2000);
    QList<QString>      _dataInCmdList;
    QList<QRtMidiData>     _deviceListData;
    QList<QRtMidiData>     _workListData;
    QString                _workListDataDeviceUid; // The device from which the data was received
    
    IoProgressDialog* _progressDialog;

    QByteArray      _dataInNACKString;
    QTimer          _procDataIn_watchdog;

    QTimer          _watchdog_timer;

    // UI Elements that are activated when something in the workList is selected
    QList<QWidget*> _workListControls;
    QList<QAction*> _workListActions;
    QList<int> _dirtyItemsIndex;
    int _maxPresetCount;
    int _presetOffset;

    DcDeviceDetails _devDetails;

    QTextStream _lastErrorMsg;
    QString _lastErrorMsgStr;
    
    QString _worklistBackupPath;
    QString _devlistBackupPath;
    QString _dataPath;
    bool _backupEnabled;
    QStringList _styleHistory;

    DcFileDownloader* _fileDownloader;
    QStringList _stateTrace;

    // A set of devices
    QSet<const char*> _supportedDevicesSet;

protected:

    /*!
      Translate the list of patch files to the current devices bank/patch name format      
    */ 
    QStringList presetListToBankPatchName(QList<QRtMidiData>& listData);
    QString presetNumberToBankNum(int i);
    QString presetToBankNum(const QRtMidiData& preset );
    QString presetToName(QRtMidiData& p);
    QString presetToBankPatchName(QRtMidiData& p);

//    bool eventFilter(QObject *o, QEvent *evt);

    void dclog(const QString str);

    void closeEvent(QCloseEvent *e);

    // Logging 'QsLog'
    void setupFilePaths();
    
    // QSettings helpers
    void writeSettings();
    void readSettings();

    // Test Helper
    void testRtMidiData();
    bool savePresetBinary(const QString &fileName,const QList<QRtMidiData>& dataList);
    bool savePresetBinary(const QString &fileName,const QRtMidiData& md);
    bool loadPresetBinary(const QString &fileName,QList<QRtMidiData>& dataList);
    bool loadPresetBinary( const QString &fileName,QRtMidiData& md );
    void backupDeviceList();
    void backupWorklist();
    
    bool loadSysexFile( const QString &fileName,QList<QRtMidiData>& dataList  );
    bool hasDevSupport( const QRtMidiData &data );

};

#endif // DcPresetLib_H
