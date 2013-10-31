QT       += core gui network 

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = spl

TEMPLATE = app

CONFIG(debug, debug|release): DESTDIR = debug
CONFIG(release, debug|release): DESTDIR = release

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR     = $$DESTDIR/.moc
RCC_DIR     = $$DESTDIR/.qrc
UI_DIR      = $$DESTDIR/.ui

# This includes the Windows specific RC file
# This is how the application gets an icon
# and other Windows specific meta-data
win32:RC_FILE += win.rc
win32:QMAKE_RC = rc
# -D_MSC_VER

debug:win32 {
    QMAKE_CXXFLAGS += -Fd$${DESTDIR}/$${TARGET}.pdb
}


SOURCES += main.cpp\
        DcPresetLib.cpp\
        DcState.cpp\
        DcStateMachineHelpers.cpp\
        IoProgressDialog.cpp \
        MoveDialog.cpp \
        DcXferMachine.cpp \
        RenameDialog.cpp \
        DcListWidget.cpp \
        DcFileDownloader.cpp \
        DcConsoleForm.cpp \
        MidiSettings.cpp \
        DcConArgs.cpp \
        DcBootControl.cpp\
        dcconbool.cpp \
        DcPackageIndex.cpp \
        DcSoftwareUpdate.cpp \
        DcUpdateAvailableDialog.cpp \
        DcUpdateDialogMgr.cpp \
        DcQUtils.cpp\
        DcLog.cpp \
        DcImgLabel.cpp


HEADERS  += DcPresetLib.h \
            RenameDialog.h \
            MoveDialog.h \
            IoProgressDialog.h \
            DcStateMachineHelpers.h \
            DcState.h \
            DcDeviceDetails.h \
            DcMidiDevDefs.h \ 
            DcXferMachine.h \
            DcConsoleForm.h \
            DcFileDownloader.h \
            MidiSettings.h \
            DcConArgs.h \
            dcconbool.h \
            DcPackageIndex.h \
            DcSoftwareUpdate.h \
            DcUpdateAvailableDialog.h \
            DcUpdateDialogMgr.h \
            DcQUtils.h\
            DcLog.h \
    DcImgLabel.h

FORMS += DcPresetLib.ui IoProgressDialog.ui MoveDialog.ui RenameDialog.ui DcplAbout.ui \
    DcConsoleForm.ui MidiPortSelect.ui \
    DcUpdateAvailableDialog.ui

#include the linkage setup
include("$$top_srcdir/QRtMidi/QRtMidi.pri")

win32 {
  DEFINES += QT_DLL
}

macx {
        LIBS += -lz -framework Carbon 
}

RESOURCES += dcpl.qrc

INCLUDEPATH += $$top_srcdir
LIBS += -L$$top_builddir/QRtMidi/$$DESTDIR -lqrtmidi

win32:PRE_TARGETDEPS += $$top_builddir/QRtMidi/$$DESTDIR/qrtmidi.lib
else:macx:PRE_TARGETDEPS += $$top_builddir/QRtMidi/$$DESTDIR/libqrtmidi.a
else:unix: PRE_TARGETDEPS += $$top_builddir/QRtMidi/$$DESTDIR/libqrtmidi.a



