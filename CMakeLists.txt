#This isn't Konversation until it works.
project(konversation)

#paranoid?
cmake_minimum_required(VERSION 2.6.0)

set(KDE_MIN_VERSION "4.00.82")

#atm this gets is FindQt4 and FindX11 (provided that the default cmake FindQt4 doesn't get used)
find_package(KDE4 REQUIRED)
find_package(KdepimLibs REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(CheckIncludeFile)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS} -DQT_STRICT_ITERATORS)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES})

#looks like the original test is from Kopete, their port doesn't use Xext so neither shall ours
if (Q_WS_X11)
    macro_bool_to_01(X11_Xscreensaver_FOUND HAVE_XSCREENSAVER)
    macro_log_feature(HAVE_XSCREENSAVER "libXss" "X ScreenSaver Library" "http://www.x.org/" FALSE "" "Screensaver detection support for Away Manager")

    macro_bool_to_01(X11_FOUND HAVE_X11)
    macro_log_feature(HAVE_X11 "libx11" "X Window System Client Library" "http://www.x.org/" FALSE "" "User inactivity detection support for Away Manager")

    macro_bool_to_01(X11_Xutil_FOUND HAVE_XUTIL)
    macro_log_feature(HAVE_XUTIL "X11 Xutil.h header" "X11 Xutil header" "http://www.x.org/" FALSE "" "User inactivity detection support for Away Manager")
endif (Q_WS_X11)

configure_file(config-konversation.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-konversation.h )

add_subdirectory(src)
add_subdirectory(data)
macro_optional_add_subdirectory(doc)

macro_display_feature_log()
