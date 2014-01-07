project(konversation)

#paranoid?
cmake_minimum_required(VERSION 2.6.2)

set(KDE_MIN_VERSION "4.7.0")
#atm this gets us FindQt4 and FindX11 (provided that the default cmake FindQt4 doesn't get used)
find_package(KDE4 ${KDE_MIN_VERSION} REQUIRED)
find_package(KdepimLibs REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(CheckIncludeFile)
include(CMakeDependentOption)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_TOLOWER)
    if(CMAKE_BUILD_TYPE_TOLOWER MATCHES debugfull)
    add_definitions (-DQT_STRICT_ITERATORS)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${KDEPIMLIBS_INCLUDE_DIRS})

macro_optional_find_package(QCA2)
macro_bool_to_01(QCA2_FOUND HAVE_QCA2)
macro_log_feature(QCA2_FOUND "QCA" "Qt Cryptographic Architecture" "http://delta.affinix.com/qca" FALSE "2.0.0" "Required for using encryption in IRC")

check_include_file("stropts.h" HAVE_STROPTS_H)
check_include_file("byteswap.h" HAVE_BYTESWAP_H)
check_include_file("sys/endian.h" HAVE_SYS_ENDIAN_H)

configure_file(config-konversation.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-konversation.h )

add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(doc)

get_filename_component(_po_path po ABSOLUTE)
get_filename_component(_doc_translations_path doc-translations ABSOLUTE)

if (EXISTS ${_po_path}/CMakeLists.txt OR EXISTS ${_doc_translations_path}/CMakeLists.txt)
    find_package(Msgfmt REQUIRED)
    find_package(Gettext REQUIRED)
endif()

if (EXISTS ${_po_path}/CMakeLists.txt)
    add_subdirectory(po)
endif()

if (EXISTS ${_doc_translations_path}/CMakeLists.txt)
    add_subdirectory(doc-translations)
endif()

macro_display_feature_log()
