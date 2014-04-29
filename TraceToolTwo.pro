QT       += opengl core gui concurrent

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
    stepvis.cpp \
    colormap.cpp \
    commrecord.cpp \
    eventrecord.cpp \
    otfimporter.cpp \
    rawtrace.cpp \
    otfconverter.cpp \
    function.cpp \
    importoptionsdialog.cpp \
    otfimportoptions.cpp \
    timelinevis.cpp \
    traditionalvis.cpp \
    visoptionsdialog.cpp \
    visoptions.cpp \
    otfimportfunctor.cpp \
    gnome.cpp \
    exchangegnome.cpp \
    collectiverecord.cpp \
    partitioncluster.cpp \
    clusterevent.cpp \
    clustervis.cpp \
    clustertreevis.cpp \
    verticallabel.cpp \
    rpartition.cpp \
    clusterprocess.cpp \
    metricrangedialog.cpp


HEADERS += \
    trace.h \
    event.h \
    message.h \
    mainwindow.h \
    viswidget.h \
    overviewvis.h \
    stepvis.h \
    colormap.h \
    commrecord.h \
    eventrecord.h \
    otfimporter.h \
    rawtrace.h \
    otfconverter.h \
    function.h \
    general_util.h \
    importoptionsdialog.h \
    otfimportoptions.h \
    timelinevis.h \
    traditionalvis.h \
    visoptionsdialog.h \
    visoptions.h \
    otfimportfunctor.h \
    gnome.h \
    exchangegnome.h \
    collectiverecord.h \
    partitioncluster.h \
    clusterevent.h \
    clustervis.h \
    clustertreevis.h \
    verticallabel.h \
    rpartition.h \
    clusterprocess.h \
    metricrangedialog.h


FORMS += \
    mainwindow.ui \
    importoptionsdialog.ui \
    visoptionsdialog.ui \
    metricrangedialog.ui

HOME = $$system(echo $HOME)

unix:!macx: LIBS += -lotf

macx: LIBS += -L$$PWD/../../libs/lib/ -lopen-trace-format

INCLUDEPATH += $$PWD/../../libs/include/open-trace-format/
DEPENDPATH += $$PWD/../../libs/include/open-trace-format/

unix: LIBS += -L$${HOME}/opt/lib -lmuster

INCLUDEPATH += $${HOME}/opt/include
DEPENDPATH += $${HOME}/opt/include

