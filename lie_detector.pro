QT += widgets multimedia

CONFIG += c++11 console
CONFIG -= app_bundle

SOURCES += \
    main.cpp \
    kiss_fft.c

HEADERS += \
    kiss_fft.h

INCLUDEPATH += .
