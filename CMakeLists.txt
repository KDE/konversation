cmake_minimum_required (VERSION 3.16 FATAL_ERROR)

# Release Service version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "23")
set (RELEASE_SERVICE_VERSION_MINOR "08")
set (RELEASE_SERVICE_VERSION_MICRO "4")
set (RELEASE_SERVICE_COMPACT_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}${RELEASE_SERVICE_VERSION_MINOR}${RELEASE_SERVICE_VERSION_MICRO}")

# Bump KONVERSATION_BASE_VERSION once new features are added
set(KONVERSATION_BASE_VERSION "1.9")
set(KONVERSATION_VERSION "${KONVERSATION_BASE_VERSION}.${RELEASE_SERVICE_COMPACT_VERSION}")

project(konversation VERSION ${KONVERSATION_VERSION})

set(QT_MIN_VERSION "5.15.2")
set(KF5_MIN_VERSION "5.91.0")
set(KDE_COMPILERSETTINGS_LEVEL "5.85")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMGenerateDBusServiceFile)
include(ECMDeprecationSettings)
include(ECMAddAppIcon)
include(ECMQtDeclareLoggingCategory)
include(CheckIncludeFile)
include(FeatureSummary)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED Core Multimedia Network Widgets)
if (QT_MAJOR_VERSION STREQUAL "6")
    find_package(Qt6Core5Compat ${QT_MIN_VERSION} CONFIG REQUIRED) # QTextCodec
endif()

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
    DBusAddons
    NewStuff
    Notifications
    WindowSystem
    ItemViews
)

if(NOT WIN32)
    find_package(KF5 REQUIRED GlobalAccel)
endif()
set(HAVE_KGLOBALACCEL ${KF5GlobalAccel_FOUND})

find_package(Qca-qt${QT_MAJOR_VERSION} 2.2.0)
set_package_properties(Qca-qt${QT_MAJOR_VERSION} PROPERTIES DESCRIPTION "Support for encryption"
                       URL "https://download.kde.org/stable/qca/"
                       TYPE OPTIONAL)
check_include_file("stropts.h" HAVE_STROPTS_H)

set(HAVE_QCA2 ${Qca-qt${QT_MAJOR_VERSION}_FOUND})

configure_file(config-konversation.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-konversation.h )
include_directories(${CMAKE_CURRENT_BINARY_DIR})

ecm_set_disabled_deprecation_versions(
    QT 5.15.2
    KF 5.100.0
)

add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(doc)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

ki18n_install(po)
kdoctools_install(po)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
