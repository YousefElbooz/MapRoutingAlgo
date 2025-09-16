cmake_minimum_required(VERSION 3.16)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

project(MapRoutingApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Release)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Concurrent)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        app.rc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(MapRoutingApp
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        mapgraph.cpp
        mapvisualizer.cpp
        mapgraph.h
        mapvisualizer.h
    )
else()
    if(ANDROID)
        add_library(MapRoutingApp SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(MapRoutingApp
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# Link Qt Widgets and Concurrent
target_link_libraries(MapRoutingApp PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Concurrent)

# Set Windows-specific properties
if(WIN32)
    set_target_properties(MapRoutingApp PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.MapRoutingApp)
endif()

set_target_properties(MapRoutingApp PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS MapRoutingApp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(MapRoutingApp)
endif()
