QT       += core gui network 

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = spl

TEMPLATE = app

include("$$top_srcdir/defaults.pri")
CONFIG += depend_includepath

INCLUDEPATH += $$top_srcdir/spl

# This includes the Windows specific RC file
# This is how the application gets an icon
# and other Windows specific meta-data
win32:RC_FILE += win.rc
win32:QMAKE_RC = rc
# -D_MSC_VER

#include the Dc common files
include("$$LIB_DIR/cmn/cmn.pri")

SOURCES += main.cpp\
        DcPresetLib.cpp\
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
        DcImgLabel.cpp


HEADERS  += DcPresetLib.h \
            RenameDialog.h \
            MoveDialog.h \
            IoProgressDialog.h \
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
            DcImgLabel.h

FORMS += DcPresetLib.ui IoProgressDialog.ui MoveDialog.ui RenameDialog.ui DcplAbout.ui \
    DcConsoleForm.ui MidiPortSelect.ui \
    DcUpdateAvailableDialog.ui

#include the linkage setup
include("$$LIB_DIR/DcMidi/DcMidi.pri")


RESOURCES += dcpl.qrc
win32 {
  DEFINES += QT_DLL
  QMAKE_LFLAGS_WINDOWS += /verbose:lib  /SAFESEH:NO
}

macx {
        LIBS += -lz -framework Carbon 
}

# Include DcMidi
win32:LIBS += -L$$LIB_OUT/DcMidi/$$DESTDIR/ -ldcmidi
else:unix:LIBS += -L$$LIB_OUT/DcMidi/$$DESTDIR/ -ldcmidi

win32:PRE_TARGETDEPS += $$LIB_OUT/DcMidi/$$DESTDIR/dcmidi.lib
else:macx:PRE_TARGETDEPS += $$LIB_OUT/DcMidi/$$DESTDIR/libdcmidi.a
else:unix:PRE_TARGETDEPS += $$LIB_OUT/DcMidi/$$DESTDIR/libdcmidi.a

### INCLUDEPATH += $$top_srcdir



