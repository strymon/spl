# qmake include file
# The files here should be cross-platform and common to all projects
CMN=$$top_srcdir/lib/cmn

SOURCES += \
      $$CMN/DcLog.cpp \
      $$CMN/DcState.cpp \
      $$CMN/DcStateMachineHelpers.cpp \
      $$CMN/DcQUtils.cpp

HEADERS += \
      $$CMN/DcQUtils.h \
      $$CMN/DcLog.h \
      $$CMN/DcCallOnce.h \
      $$CMN/DcGlobals.h \
      $$CMN/DcSigleton.h \
      $$CMN/DcState.h \
      $$CMN/DcStateMachineHelpers.h
      

