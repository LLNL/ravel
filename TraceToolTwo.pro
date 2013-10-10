QT       += opengl core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

macx: QMAKE_CXXFLAGS += -Xarch_ppc64 -mmacosx-version-min=10.7
macx: QMAKE_CXXFLAGS += -fpermissive

TARGET = TraceToolTwo
TEMPLATE = app

SOURCES  += main.cpp \
    trace.cpp \
    event.cpp \
    message.cpp \
    mainwindow.cpp \
    viswidget.cpp \
    overviewvis.cpp \
    stepvis.cpp \
    colormap.cpp


HEADERS += \
    trace.h \
    event.h \
    message.h \
    mainwindow.h \
    viswidget.h \
    overviewvis.h \
    stepvis.h \
    colormap.h


FORMS += \
    mainwindow.ui

unix:!macx: LIBS += -L$$PWD/../../../../../cpp/jsoncpp-src-0.5.0/libs/linux-gcc-4.7/ -ljson_linux-gcc-4.7_libmt

INCLUDEPATH += $$PWD/../../../../../cpp/jsoncpp-src-0.5.0/include
DEPENDPATH += $$PWD/../../../../../cpp/jsoncpp-src-0.5.0/include

macx: LIBS += -L$$PWD/../../Downloads/jsoncpp-src-0.5.0/buildscons/linux-gcc-4.2.1/src/lib_json/ -ljson_linux-gcc-4.2.1_libmt

INCLUDEPATH += $$PWD/../../Downloads/jsoncpp-src-0.5.0/include
DEPENDPATH += $$PWD/../../Downloads/jsoncpp-src-0.5.0/include

