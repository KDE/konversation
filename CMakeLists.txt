#This isn't Konversation until it works.
project(konvi)

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
if (X11_Xscreensaver_FOUND)
    set(HAVE_XSCREENSAVER 1)
endif (X11_Xscreensaver_FOUND)

configure_file(config-konvi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-konvi.h )

add_subdirectory(src)
add_subdirectory(data)
macro_optional_add_subdirectory(doc)
