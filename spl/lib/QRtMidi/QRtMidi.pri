DEPENDPATH += $$PWD

HEADERS += \
    $$PWD/RtMidi/RtError.h \
    $$PWD/RtMidi/RtMidi.h \
    $$PWD/QRtMidi.h \
    $$PWD/QRtMidiIn.h \
    $$PWD/QRtMidiOut.h \
    $$PWD/QRtMidiData.h \
    $$PWD/QRtMidiSettings.h \
    lib/QRtMidi/QRtMidiDefs.h

SOURCES += \
    $$PWD/RtMidi/RtMidi.cpp \
    $$PWD/QRtMidi.cpp \
    $$PWD/QRtMidiIn.cpp \
    $$PWD/QRtMidiOut.cpp \
    $$PWD/QRtMidiData.cpp \
    $$PWD/QRtMidiSettings.cpp \

FORMS += $$PWD/MidiPortSelect.ui

win32 {

#        DEFINES += __WINDOWS_KS__
#        LIBS += setupapi.lib ksuser.lib

        DEFINES +=__WINDOWS_MM__
        LIBS +=  WinMM.Lib
        message(Including WinMM lib for QRtMidi)
}

macx {
        DEFINES += __MACOSX_CORE__
        LIBS += -framework CoreAudio -framework CoreFoundation -framework CoreMidi
        message(Including CoreAudio CoreMidi and CoreFoundation for QRtMidi)
}
