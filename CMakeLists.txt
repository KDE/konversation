cmake_minimum_required (VERSION 3.16 FATAL_ERROR)

# Release Service version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "22")
set (RELEASE_SERVICE_VERSION_MINOR "07")
set (RELEASE_SERVICE_VERSION_MICRO "90")
set (RELEASE_SERVICE_COMPACT_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}${RELEASE_SERVICE_VERSION_MINOR}${RELEASE_SERVICE_VERSION_MICRO}")

# Bump KONVERSATION_BASE_VERSION once new features are added
set(KONVERSATION_BASE_VERSION "1.9")
set(KONVERSATION_VERSION "${KONVERSATION_BASE_VERSION}.${RELEASE_SERVICE_COMPACT_VERSION}")

project(konversation VERSION ${KONVERSATION_VERSION})

set(QT_MIN_VERSION "5.15.0")
set(KF5_MIN_VERSION "5.90.0")
set(KDE_COMPILERSETTINGS_LEVEL "5.85")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(CheckIncludeFile)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMGenerateDBusServiceFile)
include(FeatureSummary)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED Core Multimedia Network Widgets)

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

if (KF5_VERSION VERSION_LESS 5.91.0)
    find_package(Qt5X11Extras ${QT_MIN_VERSION} CONFIG)
    set_package_properties(Qt5X11Extras PROPERTIES
        PURPOSE "Needed for startup activation handling on X11"
        TYPE OPTIONAL
    )
elseif(KF5_MIN_VERSION VERSION_GREATER 5.90)
    message(AUTHOR_WARNING "Please remove usage of Qt5X11Extras and HAVE_X11, no longer needed.")
endif()

find_package(Qca-qt${QT_MAJOR_VERSION} 2.2.0)
set_package_properties(Qca-qt${QT_MAJOR_VERSION} PROPERTIES DESCRIPTION "Support for encryption"
                       URL "https://download.kde.org/stable/qca/"
                       TYPE OPTIONAL)
check_include_file("stropts.h" HAVE_STROPTS_H)

set(HAVE_QCA2 ${Qca-qt${QT_MAJOR_VERSION}_FOUND})
if(TARGET Qt5::X11Extras)
    set(HAVE_X11 TRUE)
else()
    set(HAVE_X11 FALSE)
endif()

configure_file(config-konversation.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-konversation.h )
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050F00)
#add_definitions(-DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x060000)

add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(doc)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

ki18n_install(po)
kdoctools_install(po)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
