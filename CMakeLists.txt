cmake_minimum_required(VERSION 3.16)

project(VibeKoder VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

add_subdirectory(qmarkdowntextedit)

set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/project.cpp
    src/project.h
    src/projectconfig.cpp
    src/projectconfig.h
    src/session.cpp
    src/session.h
    src/sessiontabwidget.cpp
    src/sessiontabwidget.h
    src/commandpipemanager.cpp
    src/commandpipemanager.h
    src/configmanager.cpp
    src/configmanager.h
    src/draggabletabwidget.cpp
    src/draggabletabwidget.h
    src/detachedwindow.cpp
    src/detachedwindow.h
    src/aibackend.cpp
    src/aibackend.h
    src/openaibackend.cpp
    src/openaibackend.h
    src/tabmanager.cpp
    src/tabmanager.h
    src/appconfig.cpp
    src/appconfig.h
    src/appconfigtypes.cpp
    src/appconfigtypes.h
    src/projectsettingsdialog.cpp
    src/projectsettingsdialog.h
    src/projectsettingswidget.cpp
    src/projectsettingswidget.h
    src/descriptiongenerator.cpp
    src/descriptiongenerator.h
    src/applicationsettingsdialog.cpp
    src/applicationsettingsdialog.h
    src/files.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(VibeKoder
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(VibeKoder SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(VibeKoder
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_include_directories(VibeKoder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    src
)

target_link_libraries(VibeKoder PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network qmarkdowntextedit)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.VibeKoder)
endif()
set_target_properties(VibeKoder PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS VibeKoder
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(VibeKoder)
endif()
