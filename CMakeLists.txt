cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

# Release Service version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "20")
set (RELEASE_SERVICE_VERSION_MINOR "11")
set (RELEASE_SERVICE_VERSION_MICRO "70")
set (RELEASE_SERVICE_COMPACT_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}${RELEASE_SERVICE_VERSION_MINOR}${RELEASE_SERVICE_VERSION_MICRO}")

# Bump KONVERSATION_BASE_VERSION once new features are added
set(KONVERSATION_BASE_VERSION "1.8")
set(KONVERSATION_VERSION "${KONVERSATION_BASE_VERSION}.${RELEASE_SERVICE_COMPACT_VERSION}")

project(konversation VERSION ${KONVERSATION_VERSION})

set (QT_MIN_VERSION "5.12.0")
set(KF5_MIN_VERSION "5.71.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(CheckIncludeFile)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(FeatureSummary)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core Network Widgets)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED
    Archive
    Bookmarks
    Config
    ConfigWidgets
    CoreAddons
    Crash
    DocTools
    I18n
    IdleTime
    NotifyConfig
    KIO
    Parts
    Wallet
    WidgetsAddons
    GlobalAccel
    DBusAddons
    NewStuff
    Notifications
    WindowSystem
    ItemViews
)

find_package(Phonon4Qt5 4.6.60 REQUIRED)

find_package(Qca-qt5 2.1.0)
set_package_properties(Qca-qt5 PROPERTIES DESCRIPTION "Support for encryption"
                       URL "https://download.kde.org/stable/qca/"
                       TYPE OPTIONAL)
check_include_file("stropts.h" HAVE_STROPTS_H)

set(HAVE_QCA2 ${Qca-qt5_FOUND})

configure_file(config-konversation.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-konversation.h )
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_definitions(
  -DQT_USE_QSTRINGBUILDER
  -DQT_STRICT_ITERATORS
  -DQT_NO_URL_CAST_FROM_STRING
  -DQT_NO_CAST_FROM_BYTEARRAY
  -DQT_NO_CAST_TO_ASCII
  -DQT_NO_CAST_FROM_ASCII
# -DQT_NO_SIGNALS_SLOTS_KEYWORDS FIXME KF5 Port: QCA includes cause trouble.
  -DQT_NO_FOREACH
  -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT
)

add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(doc)

ki18n_install(po)
kdoctools_install(po)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
