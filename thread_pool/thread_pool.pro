#-------------------------------------------------
#
# Project created by QtCreator 2013-07-17T18:32:50
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = thread_pool
CONFIG   += console

CONFIG += C++11

TEMPLATE = app

INCLUDEPATH += ../boost_1_54_0
LIBS += -L../boost_1_54_0/stage/lib

win32:LIBS += -lboost_system-mgw47-mt-sd-1_54 -lws2_32 -lmswsock

SOURCES += main.cpp \
    thread_pool.ipp \
    test_header_only.cpp

HEADERS += \
    noncopyable.hpp \
    thread_pool.hpp \
    mpsc_bounded_queue.hpp \
    asio_thread_pool.hpp \
    callback.hpp
