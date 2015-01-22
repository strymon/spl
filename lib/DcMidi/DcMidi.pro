TARGET   = dcmidi
TEMPLATE = lib
CONFIG += staticlib

RTMIDI_DIR = RtMidi

INCLUDEPATH = $$RTMIDI_DIR $$PWD

HEADERS += \
    $$RTMIDI_DIR/RtMidi.h \
    DcMidi.h \
    DcMidiIn.h \
    DcMidiOut.h \
    DcMidiData.h \
    DcMidiIdent.h \
    DcMidiTrigger.h

SOURCES += \
    $$RTMIDI_DIR/RtMidi.cpp \
    DcMidi.cpp \
    DcMidiIn.cpp \
    DcMidiOut.cpp \
    DcMidiData.cpp \
    DcMidiIdent.cpp \
    DcMidiTrigger.cpp 

# include("defaults.pri")
include("$$top_srcdir/defaults.pri")

# Options for building RtMidi
win32 {
    DEFINES +=__WINDOWS_MM__
}else:macx {
    DEFINES += __MACOSX_CORE__
}


