ROOT_DIR = $$PWD
SRC_DIR = $$ROOT_DIR 
INCLUDEPATH += $$ROOT_DIR 

CONFIG(debug, debug|release): DESTDIR = debug
CONFIG(release, debug|release): DESTDIR = release

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui
    
#debug:win32 {
#    QMAKE_CXXFLAGS += -Fd$${DESTDIR}/$${TARGET}.pdb
#}
