project(konversation)

cmake_minimum_required (VERSION 2.8.12 FATAL_ERROR)
set (QT_MIN_VERSION "5.3.0")
set(KF5_MIN_VERSION "5.2.0")

find_package(ECM 1.2.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(CheckIncludeFile)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMInstallIcons)
include(FeatureSummary)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core Widgets)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED
    Archive
    Bookmarks
    Config
    ConfigWidgets
    CoreAddons
    DocTools
    Emoticons
    I18n
    IdleTime
    NotifyConfig
    KDELibs4Support
    KIO
    Parts
    Solid
    Sonnet
    Wallet
    WidgetsAddons
    GlobalAccel
    DBusAddons
    Notifications
    WindowSystem
    IconThemes
    ItemViews
)

find_package(Phonon4Qt5 4.6.60 REQUIRED)
include_directories(${PHONON_INCLUDES})

find_package(Qca 2.1.0)
set_package_properties(Qca PROPERTIES DESCRIPTION "Support for encryption"
                       URL "http://delta.affinix.com/qca"
                       TYPE OPTIONAL)
check_include_file("stropts.h" HAVE_STROPTS_H)
check_include_file("byteswap.h" HAVE_BYTESWAP_H)
check_include_file("sys/endian.h" HAVE_SYS_ENDIAN_H)

configure_file(config-konversation.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-konversation.h )
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(HAVE_QCA2 ${Qca_FOUND})

add_definitions(
  -DQT_USE_QSTRINGBUILDER
  -DQT_NO_CAST_TO_ASCII
# -DQT_NO_CAST_FROM_ASCII FIXME KF5 port
  -DQT_STRICT_ITERATORS
  -DQT_NO_URL_CAST_FROM_STRING
  -DQT_NO_CAST_FROM_BYTEARRAY
# -DQT_NO_SIGNALS_SLOTS_KEYWORDS FIXME KF5 port (QCA includes cause trouble)
  -DQT_USE_FAST_OPERATOR_PLUS
)

add_definitions(-DTRANSLATION_DOMAIN=\"konversation\")

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

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
