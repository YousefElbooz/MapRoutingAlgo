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

# Install additional files
install(FILES "${CMAKE_SOURCE_DIR}/README.md" DESTINATION . OPTIONAL)
install(FILES "${CMAKE_SOURCE_DIR}/icon.ico" DESTINATION . OPTIONAL)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/styles" DESTINATION . OPTIONAL)

# Install Qt DLLs for Windows
if(WIN32)
    # Install Qt platform plugins
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/platforms" DESTINATION . OPTIONAL)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/imageformats" DESTINATION . OPTIONAL)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/iconengines" DESTINATION . OPTIONAL)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/generic" DESTINATION . OPTIONAL)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/networkinformation" DESTINATION . OPTIONAL)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/tls" DESTINATION . OPTIONAL)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/translations" DESTINATION . OPTIONAL)
endif()

# Set install prefix to avoid permission issues
if(WIN32)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Installation Directory" FORCE)
endif()

# Deploy Qt dependencies on Windows
if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    # Find windeployqt
    get_target_property(QT_QMAKE_EXECUTABLE Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    if(QT_QMAKE_EXECUTABLE)
        get_filename_component(QT_WINDEPLOYQT_EXECUTABLE ${QT_QMAKE_EXECUTABLE} PATH)
        set(QT_WINDEPLOYQT_EXECUTABLE "${QT_WINDEPLOYQT_EXECUTABLE}/windeployqt.exe")
        
        if(EXISTS ${QT_WINDEPLOYQT_EXECUTABLE})
            # Deploy Qt libraries during build
            add_custom_command(TARGET MapRoutingApp POST_BUILD
                COMMAND ${QT_WINDEPLOYQT_EXECUTABLE} --verbose 0 --no-translations --no-system-d3d-compiler --no-opengl-sw $<TARGET_FILE:MapRoutingApp>
                COMMENT "Deploying Qt libraries")
        endif()
    endif()
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(MapRoutingApp)
endif()

# ==================== CPACK CONFIGURATION ====================

# Include CPack
include(CPack)

# Basic package information
set(CPACK_PACKAGE_NAME "MapRoutingApp")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Map Routing Application - Find optimal routes on maps")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "MapRoutingApp")

# Set the license file if you have one
# set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

# Set the readme file
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")

# Package file name
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}")

# Windows-specific settings
if(WIN32)
    # Available generators: NSIS, ZIP, 7Z
    set(CPACK_GENERATOR "NSIS;ZIP")
    
    # NSIS specific settings
    set(CPACK_NSIS_DISPLAY_NAME "Map Routing App")
    set(CPACK_NSIS_PACKAGE_NAME "MapRoutingApp")
    set(CPACK_NSIS_MODIFY_PATH ON)
endif()

# Linux-specific settings
if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
endif()

# macOS-specific settings
if(APPLE)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
endif()