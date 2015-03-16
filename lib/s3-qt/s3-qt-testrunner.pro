#-------------------------------------------------
#
# Project created by QtCreator 2014-12-09T17:34:44
#
#-------------------------------------------------

QT       += network testlib

QT       -= gui

TARGET = s3qttestrunner
CONFIG   += console
CONFIG   -= app_bundle
CONFIG  += c++11
TEMPLATE = app

HEADERS += \
    test/testutil.h \
    test/externalprocess.h \
    test/testrunner.h \
    test/testutiltest.h \
    test/s3test.h \
    test/buckettest.h

SOURCES += \
    test/main.cpp \
    test/testutil.cpp \
    test/externalprocess.cpp \
    test/testrunner.cpp \
    test/testutiltest.cpp \
    test/s3test.cpp \
    test/buckettest.cpp

include (s3-qt.pri)
