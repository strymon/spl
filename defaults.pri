# warn when multiply included

contains (INCLUDED_FILES, NameOfPriFile) {
warning (multiple inclusion of NameOfPriFile)
}
INCLUDED_FILES += defaults.pri

LIB_OUT=$$top_builddir/lib
LIB_DIR=$$top_srcdir/lib

INCLUDEPATH += $$PWD $$LIB_DIR

CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
}

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui

    
#CONFIG(debug, debug|release):win32 {
#    QMAKE_CXXFLAGS += -Fd$${DESTDIR}/$${TARGET}.pdb
#}
