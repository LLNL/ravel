QT       += opengl core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TraceToolTwo
TEMPLATE = app

SOURCES  += main.cpp \
    trace.cpp \
    event.cpp \
    message.cpp \
    mainwindow.cpp \
    viswidget.cpp \
    overviewvis.cpp \
    stepvis.cpp


HEADERS += \
    trace.h \
    event.h \
    message.h \
    mainwindow.h \
    viswidget.h \
    overviewvis.h \
    stepvis.h


FORMS += \
    mainwindow.ui

unix:!macx: LIBS += -L$$PWD/../../../../../cpp/jsoncpp-src-0.5.0/libs/linux-gcc-4.7/ -ljson_linux-gcc-4.7_libmt

INCLUDEPATH += $$PWD/../../../../../cpp/jsoncpp-src-0.5.0/include
DEPENDPATH += $$PWD/../../../../../cpp/jsoncpp-src-0.5.0/include
