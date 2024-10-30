cmake_minimum_required (VERSION 3.16 FATAL_ERROR)

# Release Service version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "24")
set (RELEASE_SERVICE_VERSION_MINOR "08")
set (RELEASE_SERVICE_VERSION_MICRO "3")
set (RELEASE_SERVICE_COMPACT_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}${RELEASE_SERVICE_VERSION_MINOR}${RELEASE_SERVICE_VERSION_MICRO}")

# Bump KONVERSATION_BASE_VERSION once new features are added
set(KONVERSATION_BASE_VERSION "1.10")
set(KONVERSATION_VERSION "${KONVERSATION_BASE_VERSION}.${RELEASE_SERVICE_COMPACT_VERSION}")

project(konversation VERSION ${KONVERSATION_VERSION})

set(QT_MIN_VERSION "6.4.0")
set(KF5_MIN_VERSION "5.240.0")
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

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED Core Multimedia Network Widgets)

#This is a RUNTIME dependency. I don't know of any way to indicate that the Tools
#component of the Qt package is needed at runtime, so trying to do so with Qt6Tools
#This hopefully provides the binary likely named /usr/lib64/qt6/bin/qdbus
find_package(Qt6Tools ${QT_MIN_VERSION} CONFIG REQUIRED)
set_package_properties(Qt6Tools PROPERTIES
    DESCRIPTION "Provider of qdbus binary"
    TYPE RUNTIME
    PURPOSE "Allows scripts to communicate with Konversation"
)

find_package(Qt6Core5Compat ${QT_MIN_VERSION} CONFIG REQUIRED) # QTextCodec

find_package(KF6 REQUIRED COMPONENTS
    Archive
    Bookmarks
    Codecs
    Config
    ConfigWidgets
    CoreAddons
    Crash
    DBusAddons
    DocTools
    I18n
    IdleTime
    ItemViews
    KIO
    NewStuff
    Notifications
    NotifyConfig
    Parts
    StatusNotifierItem
    TextWidgets
    Wallet
    WidgetsAddons
    WindowSystem
)

if(NOT WIN32)
    find_package(KF6 REQUIRED GlobalAccel)
endif()
set(HAVE_KGLOBALACCEL ${KF6GlobalAccel_FOUND})

if (UNIX AND NOT APPLE AND NOT HAIKU)
    set(HAVE_X11 TRUE)
endif()

find_package(Qca-qt6 2.2.0)
set_package_properties(Qca-qt6 PROPERTIES DESCRIPTION "Support for encryption"
                       URL "https://download.kde.org/stable/qca/"
                       TYPE OPTIONAL)
check_include_file("stropts.h" HAVE_STROPTS_H)

set(HAVE_QCA2 ${Qca-qt6_FOUND})

configure_file(config-konversation.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-konversation.h )
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(doc)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

ki18n_install(po)
kdoctools_install(po)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
