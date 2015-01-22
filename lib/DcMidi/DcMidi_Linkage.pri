win32 {
    mingw {
        LIBS +=  -lwinmm
    } else {
        LIBS +=  WinMM.Lib
    }
}

macx {
        LIBS += -framework CoreAudio -framework CoreFoundation -framework CoreMidi
        message(Including CoreAudio CoreMidi and CoreFoundation for DcMidi)
}

win32 {
    DEFINES +=__WINDOWS_MM__
}else:macx {
    DEFINES += __MACOSX_CORE__
}

win32:LIBS += -L$$LIB_OUT/DcMidi/$$DESTDIR/ -ldcmidi
else:unix:LIBS += -L$$LIB_OUT/DcMidi/$$DESTDIR/ -ldcmidi

win32:PRE_TARGETDEPS += $$LIB_OUT/DcMidi/$$DESTDIR/dcmidi.lib
else:macx:PRE_TARGETDEPS += $$LIB_OUT/DcMidi/$$DESTDIR/libdcmidi.a
else:unix:PRE_TARGETDEPS += $$LIB_OUT/DcMidi/$$DESTDIR/libdcmidi.a


