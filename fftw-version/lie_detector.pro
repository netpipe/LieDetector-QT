QT += widgets multimedia

CONFIG += c++11 console
CONFIG -= app_bundle

SOURCES += \
    main.cpp

# Link against FFTW
LIBS += -lfftw3 -L/Users/macbook2015/Desktop/brew/lib
INCLUDEPATH += /usr/local/include /Users/macbook2015/Desktop/brew/include
