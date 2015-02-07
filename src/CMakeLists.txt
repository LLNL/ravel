set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Qt5 + Modules
find_package(Qt5 REQUIRED Core Widgets OpenGL Concurrent)

# Dependencies over Qt5
find_package(OpenGL)
find_package(MUSTER REQUIRED)
find_package(OTF REQUIRED)
find_package(OTF2 REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

set(CMAKE_AUTOMOC ON)

# Includes, Definitions, Flags
include_directories(${Qt5Widgets_INCLUDE_DIRS}
                    ${MUSTER_INCLUDE_DIRS}
                    ${OTF_INCLUDE_DIRS}
                    ${OTF2_INCLUDE_DIRS}
                    )

add_definitions(${Qt5Widgets_DEFINITIONS})

# ui files
qt5_wrap_ui(mainwindow.ui importoptionsdialog.ui metricrangedialog.ui visoptionsdialog.ui)

# Sources and UI Files
set(RAVEL_SOURCES
    main.cpp
    trace.cpp
    event.cpp
    message.cpp
    mainwindow.cpp
    viswidget.cpp
    overviewvis.cpp
    stepvis.cpp
    colormap.cpp
    commrecord.cpp
    eventrecord.cpp
    otfimporter.cpp
    rawtrace.cpp
    otfconverter.cpp
    function.cpp
    importoptionsdialog.cpp
    otfimportoptions.cpp
    timelinevis.cpp
    traditionalvis.cpp
    visoptionsdialog.cpp
    visoptions.cpp
    otfimportfunctor.cpp
    gnome.cpp
    exchangegnome.cpp
    collectiverecord.cpp
    partitioncluster.cpp
    clusterevent.cpp
    clustervis.cpp
    clustertreevis.cpp
    verticallabel.cpp
    rpartition.cpp
    metricrangedialog.cpp
    otfcollective.cpp
    commevent.cpp
    p2pevent.cpp
    collectiveevent.cpp
    commdrawinterface.cpp
    counter.cpp
    counterrecord.cpp
    otf2importer.cpp
    task.cpp
    clustertask.cpp
    taskgroup.cpp
    otf2exporter.cpp
    otf2exportfunctor.cpp
)

set(RAVEL_HEADERS
    trace.h
    event.h
    message.h
    mainwindow.h
    viswidget.h
    overviewvis.h
    stepvis.h
    colormap.h
    commrecord.h
    eventrecord.h
    otfimporter.h
    rawtrace.h
    otfconverter.h
    function.h
    general_util.h
    importoptionsdialog.h
    otfimportoptions.h
    timelinevis.h
    traditionalvis.h
    visoptionsdialog.h
    visoptions.h
    otfimportfunctor.h
    gnome.h
    exchangegnome.h
    collectiverecord.h
    partitioncluster.h
    clusterevent.h
    clustervis.h
    clustertreevis.h
    verticallabel.h
    rpartition.h
    metricrangedialog.h
    otfcollective.h
    commevent.h
    p2pevent.h
    collectiveevent.h
    commbundle.h
    commdrawinterface.h
    counter.h
    counterrecord.h
    otf2importer.h
    task.h
    clustertask.h
    taskgroup.h
    otf2exporter.h
    otf2exportfunctor.h
)

set(RAVEL_UIC
    mainwindow.h
)

# Build Target
add_executable(Ravel MACOSX_BUNDLE ${RAVEL_SOURCES} ${RAVEL_UIC})

qt5_use_modules(Ravel Widgets OpenGL Concurrent)

target_link_libraries(Ravel
                      Qt5::Widgets
                      Qt5::OpenGL
                      Qt5::Concurrent
                      ${OPENGL_LIBRARIES}
                      ${MUSTER_LIBRARIES}
                      ${OTF_LIBRARIES}
                      ${OTF2_LIBRARIES}
                     )

install(TARGETS Ravel DESTINATION bin)
