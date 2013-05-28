QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = spl

TEMPLATE = app

INCLUDEPATH += lib

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
        DcConArgs.cpp


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
            DcConArgs.h

include(lib/QRtMidi/QRtMidi.pri)




FORMS += DcPresetLib.ui IoProgressDialog.ui MoveDialog.ui RenameDialog.ui DcplAbout.ui \
    DcConsoleForm.ui

win32 {
  DEFINES += QT_DLL
}

macx {
        LIBS += -lz -framework Carbon 
}

RESOURCES += \
    dcpl.qrc
