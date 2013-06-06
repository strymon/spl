DEPENDPATH += $$PWD

HEADERS += \
    $$PWD/RtMidi/RtError.h \
    $$PWD/RtMidi/RtMidi.h \
    $$PWD/QRtMidi.h \
    $$PWD/QRtMidiIn.h \
    $$PWD/QRtMidiOut.h \
    $$PWD/QRtMidiData.h \
    $$PWD/QRtMidiSettings.h \
    $$PWD/QRtMidiIdent.h

SOURCES += \
    $$PWD/RtMidi/RtMidi.cpp \
    $$PWD/QRtMidi.cpp \
    $$PWD/QRtMidiIn.cpp \
    $$PWD/QRtMidiOut.cpp \
    $$PWD/QRtMidiData.cpp \
    $$PWD/QRtMidiSettings.cpp \
    $$PWD/QRtMidiIdent.cpp

FORMS += $$PWD/MidiPortSelect.ui

win32 {
    DEFINES +=__WINDOWS_MM__

    mingw {
        LIBS +=  -lwinmm
    } else {
        LIBS +=  WinMM.Lib
    }
}

macx {
        DEFINES += __MACOSX_CORE__
        LIBS += -framework CoreAudio -framework CoreFoundation -framework CoreMidi
        message(Including CoreAudio CoreMidi and CoreFoundation for QRtMidi)
}
