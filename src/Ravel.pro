QT       += opengl core gui concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Ravel
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
    metricrangedialog.cpp \
    otfcollective.cpp \
    commevent.cpp \
    p2pevent.cpp \
    collectiveevent.cpp \
    commdrawinterface.cpp \
    counter.cpp \
    counterrecord.cpp \
    otf2importer.cpp \
    task.cpp \
    clustertask.cpp \
    taskgroup.cpp \
    otf2exporter.cpp \
    otf2exportfunctor.cpp

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
    metricrangedialog.h \
    otfcollective.h \
    commevent.h \
    p2pevent.h \
    collectiveevent.h \
    commbundle.h \
    commdrawinterface.h \
    counter.h \
    counterrecord.h \
    otf2importer.h \
    task.h \
    clustertask.h \
    taskgroup.h \
    otf2exporter.h \
    otf2exportfunctor.h

FORMS += \
    mainwindow.ui \
    importoptionsdialog.ui \
    visoptionsdialog.ui \
    metricrangedialog.ui

HOME = $$system(echo $HOME)

unix:!macx: LIBS += -lotf -lz

unix: INCLUDEPATH += $${HOME}/opt/include
unix: DEPENDPATH += $${HOME}/opt/include

unix:!macx: INCLUDEPATH += /opt/otf2/include
unix:!macx: DEPENDPATH += /opt/otf2/include

unix:!macx: LIBS += -L/opt/otf2/lib -lotf2

macx: LIBS += -lz
macx: LIBS += -L$${HOME}/opt/lib -lopen-trace-format -lotf2

macx: INCLUDEPATH += $${HOME}/opt/include/open-trace-format/
macx: DEPENDPATH += $${HOME}/opt/include/open-trace-format/

macx: INCLUDEPATH += $${HOME}/opt/include/otf2/
macx: DEPENDPATH += $${HOME}/opt/include/otf2/

unix:!macx: LIBS += -L$${HOME}/opt/lib -lmuster

macx: LIBS += -L$${HOME}/opt/muster/lib -lmuster
macx: INCLUDEPATH += $${HOME}/opt/muster/include
macx: DEPENDPATH += $${HOME}/opt/muster/include

OTHER_FILES += \
    CMakeLists.txt

