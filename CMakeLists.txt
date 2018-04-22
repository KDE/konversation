project(konversation)

cmake_minimum_required (VERSION 3.1 FATAL_ERROR)
set (CMAKE_CXX_STANDARD 11)
set (QT_MIN_VERSION "5.5.0")
set(KF5_MIN_VERSION "5.25.0")

find_package(ECM 1.2.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(CheckIncludeFile)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(FeatureSummary)
include(ECMQMLModules)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core Widgets Quick QuickWidgets) # WIPQTQUICK

if (ANDROID)
#   we want to make sure that generally all components are found

    set(optionalComponents "OPTIONAL_COMPONENTS")
endif()
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED
    Archive
    Bookmarks
    Config
    ConfigWidgets
    CoreAddons
    Crash
    Emoticons
    I18n
    IdleTime
    NotifyConfig
    KIO
    Parts
    Solid
    WidgetsAddons
    GlobalAccel
    DBusAddons
    Notifications
    WindowSystem
    IconThemes
    ItemViews
    ItemModels # WIPQTQUICK Destructuring ViewContainer to flat list
    ${optionalComponents}
    Package # WIPQTQUICK: Used to load the QML UI package
    Wallet
    DocTools
)

ecm_find_qmlmodule(org.kde.kirigami 2.1) # WIPQTQUICK

find_package(Phonon4Qt5 4.6.60 REQUIRED)
include_directories(${PHONON_INCLUDES})

find_package(Qca-qt5 2.1.0)
set_package_properties(Qca-qt5 PROPERTIES DESCRIPTION "Support for encryption"
                       URL "http://download.kde.org/stable/qca-qt5/"
                       TYPE OPTIONAL)
check_include_file("stropts.h" HAVE_STROPTS_H)
check_include_file("byteswap.h" HAVE_BYTESWAP_H)
check_include_file("sys/endian.h" HAVE_SYS_ENDIAN_H)

set(HAVE_QCA2 ${Qca-qt5_FOUND})

configure_file(config-konversation.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-konversation.h )
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(KF5Wallet_FOUND)
    add_definitions(-DWITH_KWALLET=1)
endif()

add_definitions(
  -DQT_USE_QSTRINGBUILDER
  -DQT_NO_CAST_TO_ASCII
  -DQT_STRICT_ITERATORS
  -DQT_NO_URL_CAST_FROM_STRING
  -DQT_NO_CAST_FROM_BYTEARRAY
# -DQT_NO_SIGNALS_SLOTS_KEYWORDS FIXME KF5 Port: QCA includes cause trouble.
  -DQT_USE_FAST_OPERATOR_PLUS
)

add_subdirectory(src)
add_subdirectory(data)
if(KF5DocTools_FOUND)
    add_subdirectory(doc)
endif()
# ki18n_install(po)

get_filename_component(_doc_translations_path doc-translations ABSOLUTE)

if (EXISTS ${_doc_translations_path}/CMakeLists.txt)
    add_subdirectory(doc-translations)
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
