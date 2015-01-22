# This project include file contains the needed linker settings
# for the specified host OS. 

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
