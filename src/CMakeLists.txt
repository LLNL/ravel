set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Qt5 + Modules
find_package(Qt5 REQUIRED Core Widgets OpenGL Concurrent)

# Dependencies over Qt5
find_package(OpenGL)
find_package(Muster REQUIRED)
find_package(OTF)
find_package(OTF2 REQUIRED)
find_package(ZLIB REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

set(CMAKE_AUTOMOC ON)

# Includes, Definitions, Flags
include_directories(${Qt5Widgets_INCLUDE_DIRS}
                    ${Muster_INCLUDE_DIRS}
                    ${OTF2_INCLUDE_DIRS}
                    ${ZLIB_INCLUDE_DIRS}
                   )

SET(ADDED_SOURCES "")
SET(ADDED_HEADERS "")

if (OTF_FOUND)
    include_directories(${OTF_INCLUDE_DIRS})
    add_definitions(-DOTF1LIB)
    list(APPEND ADDED_HEADERS otfimporter.h)
    list(APPEND ADDED_SOURCES otfimporter.cpp)
endif()

add_definitions(${Qt5Widgets_DEFINITIONS})

# ui files
qt5_wrap_ui(ui_mainwindow.h mainwindow.ui)
qt5_wrap_ui(ui_importoptionsdialog.h importoptionsdialog.ui)
qt5_wrap_ui(ui_metricrangedialog.h metricrangedialog.ui)
qt5_wrap_ui(ui_visoptionsdialog.h visoptionsdialog.ui)

# Sources and UI Files
set(Ravel_SOURCES
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
    rawtrace.cpp
    otfconverter.cpp
    function.cpp
    importoptionsdialog.cpp
    importoptions.cpp
    timelinevis.cpp
    traditionalvis.cpp
    visoptionsdialog.cpp
    visoptions.cpp
    importfunctor.cpp
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
    entity.cpp
    clusterentity.cpp
    entitygroup.cpp
    otf2exporter.cpp
    otf2exportfunctor.cpp
    charmimporter.cpp
    primaryentitygroup.cpp
    metrics.cpp
    ${ADDED_SOURCES}
)

set(Ravel_HEADERS
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
    rawtrace.h
    otfconverter.h
    function.h
    ravelutils.h
    importoptionsdialog.h
    importoptions.h
    timelinevis.h
    traditionalvis.h
    visoptionsdialog.h
    visoptions.h
    importfunctor.h
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
    entity.h
    clusterentity.h
    entitygroup.h
    otf2exporter.h
    otf2exportfunctor.h
    charmimporter.h
    primaryentitygroup.h
    metrics.h
    ${ADDED_HEADERS}
)

set(Ravel_UIC
    ui_mainwindow.h
    ui_importoptionsdialog.h
    ui_visoptionsdialog.h
    ui_metricrangedialog.h
)

# Build Target
add_executable(Ravel MACOSX_BUNDLE ${Ravel_SOURCES} ${Ravel_UIC})

qt5_use_modules(Ravel Widgets OpenGL Concurrent)

target_link_libraries(Ravel
                      Qt5::Widgets
                      Qt5::OpenGL
                      Qt5::Concurrent
                      ${OPENGL_LIBRARIES}
                      ${Muster_LIBRARIES}
                      ${OTF2_LIBRARIES}
                      ${ZLIB_LIBRARIES}
                     )

if (OTF_FOUND)
    target_link_libraries(Ravel
                          ${OTF_LIBRARIES}
                         )
endif()

install(TARGETS Ravel DESTINATION bin)
