project(konversation)

#paranoid?
cmake_minimum_required(VERSION 2.6.2)

set(KDE_MIN_VERSION "4.3.0")
#atm this gets us FindQt4 and FindX11 (provided that the default cmake FindQt4 doesn't get used)
find_package(KDE4 ${KDE_MIN_VERSION} REQUIRED)
find_package(KdepimLibs REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(CheckIncludeFile)
include(CMakeDependentOption)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_TOLOWER)
if(CMAKE_BUILD_TYPE_TOLOWER MATCHES debugfull)
add_definitions (-DQT_STRICT_ITERATORS)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES})

#check for KDE greater than 4.3.80 (used for KStatusNotifier)
macro_ensure_version("4.3.80" ${KDE_VERSION} HAVEKDE4_4)

#check for KDE greater than 4.4.3 (usable version of KIdleTime)
macro_ensure_version("4.4.3" ${KDE_VERSION} HAVEKDE4_4_3)

if (HAVEKDE4_4_3)
    option(USE_KIDLETIME "Use KIdleTime for user inactivity detection." ON)
endif (HAVEKDE4_4_3)

if (HAVEKDE4_4_3 AND USE_KIDLETIME)
    macro_bool_to_01(USE_KIDLETIME HAVE_KIDLETIME)
    #only log the status of the feature if the user wants to build with KIdleTime support
    macro_log_feature(HAVE_KIDLETIME "KIdleTime" "KDE Idle Time" "http://www.kde.org/" FALSE "" "KIdleTime based user inactivity detection for the auto-away functionality")
endif (HAVEKDE4_4_3 AND USE_KIDLETIME)

#looks like the original test is from Kopete, their port doesn't use Xext so neither shall ours
if (Q_WS_X11 AND NOT HAVE_KIDLETIME)
    macro_bool_to_01(X11_Xscreensaver_FOUND HAVE_XSCREENSAVER)
    macro_log_feature(HAVE_XSCREENSAVER "libXss" "X ScreenSaver Library" "http://www.x.org/" FALSE "" "Screensaver detection support for the auto-away functionality")

    macro_bool_to_01(X11_FOUND HAVE_X11)
    macro_log_feature(HAVE_X11 "libx11" "X Window System Client Library" "http://www.x.org/" FALSE "" "User inactivity detection support for the auto-away functionality")

    macro_bool_to_01(X11_Xutil_FOUND HAVE_XUTIL)
    macro_log_feature(HAVE_XUTIL "X11 Xutil.h header" "X11 Xutil header" "http://www.x.org/" FALSE "" "User inactivity detection support for the auto-away functionality")
endif (Q_WS_X11 AND NOT HAVE_KIDLETIME)

macro_optional_find_package(QCA2)
macro_bool_to_01(QCA2_FOUND HAVE_QCA2)
macro_log_feature(QCA2_FOUND "QCA" "Qt Cryptographic Architecture" "http://delta.affinix.com/qca" FALSE "2.0.0" "Required for using encryption in IRC")

#define the value for HAVE_KSTATUSNOTIFIERITEM
macro_bool_to_01(HAVEKDE4_4 HAVE_KSTATUSNOTIFIERITEM)

check_include_file("stropts.h" HAVE_STROPTS_H)
check_include_file("byteswap.h" HAVE_BYTESWAP_H)
check_include_file("sys/endian.h" HAVE_SYS_ENDIAN_H)

configure_file(config-konversation.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-konversation.h )

add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(doc)

macro_display_feature_log()
