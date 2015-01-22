QT += core testlib
TEMPLATE = app
TARGET = t_dcmididata
include("../../defaults.pri")
SOURCES +=  $$SRC_DIR/DcMidiData.cpp
SOURCES += t_dcmididata.cpp
