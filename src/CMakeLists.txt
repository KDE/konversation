include_directories(${CMAKE_CURRENT_SOURCE_DIR}/config)
include_directories(config dcc irc viewer upnp)

add_executable(konversation)

#Does this disto use qdbus? qdbus6? qdbus-qt6? qdbus_qt6? qt6_dbus?
#Is it even on the path, or do we need to specify something like /usr/lib64/qt6/bin/...?
#Perhaps comes from the Qt6Tools package?
include(ECMQueryQt)
ecm_query_qt(QT_INSTALL_BINS QT_INSTALL_BINS)
find_program(QDBUS_RUNTIME_BINARY_FULL_PATH
        NAMES qdbus qdbus6 qdbus-qt6
        REQUIRED
        DOC "Used to allow the installed scripts to communicate with Konversation without requiring code in every script to adapt to the distribution."
        HINTS ${QT_INSTALL_BINS}
)
configure_file(konvi_qdbus.h.in konvi_qdbus.h)

target_sources(konversation PRIVATE
    #====================================
    #=== Application
    config/preferences.cpp
    config/preferences.h

    application.cpp
    application.h
    dbus.cpp
    dbus.h
    mainwindow.cpp
    mainwindow.h
    main.cpp
    common.cpp
    common.h
    sound.cpp
    sound.h
    ssllabel.cpp
    ssllabel.h
    statusbar.cpp
    statusbar.h
    bookmarkhandler.cpp
    bookmarkhandler.h
    scriptlauncher.cpp
    scriptlauncher.h
    konsolepanel.cpp
    konsolepanel.h
    notificationhandler.cpp
    notificationhandler.h
    launcherentryhandler.cpp
    launcherentryhandler.h
    awaymanager.cpp
    awaymanager.h
    connectionmanager.cpp
    connectionmanager.h
    connectionsettings.cpp
    connectionsettings.h
    identity.cpp
    identity.h
    identitydialog.cpp
    identitydialog.h

    konversation.qrc

    #=== GUI
    urlcatcher.cpp
    urlcatcher.h
    queuetuner.cpp
    queuetuner.h
    quickconnectdialog.cpp
    quickconnectdialog.h
)

target_sources(konversation PRIVATE
    #== IRC
    irc/server.cpp

    irc/query.cpp
    irc/channel.cpp
    irc/channellistpanel.cpp
    irc/channelnick.cpp
    irc/modebutton.cpp
    irc/joinchanneldialog.cpp
    irc/invitedialog.cpp
    irc/topichistorymodel.cpp

    irc/irccharsets.cpp

    irc/nick.cpp
    irc/nickinfo.cpp
    irc/nicklistview.cpp
    irc/nicksonline.cpp
    irc/nicksonlineitem.cpp

    #=== Server
    irc/inputfilter.cpp
    irc/outputfilter.cpp
    irc/outputfilterresolvejob.cpp
    irc/ircqueue.cpp
    irc/servergroupdialog.cpp
    irc/servergroupsettings.cpp
    irc/serverison.cpp
    irc/serverlistdialog.cpp
    irc/serverlistview.cpp
    irc/serversettings.cpp
)

ki18n_wrap_ui(konversation
    irc/channellistpanelui.ui
    irc/channeldialogui.ui
    irc/joinchannelui.ui
    irc/serverdialogui.ui
    irc/servergroupdialogui.ui
    irc/serverlistdialogui.ui
    irc/invitedialog.ui
)

    #=== Configuration dialog pages
target_sources(konversation PRIVATE
    config/alias_config.cpp
    config/alias_config.h
    config/autoreplace_config.cpp
    config/autoreplace_config.h
    config/configdialog.cpp
    config/configdialog.h
    config/connectionbehavior_config.cpp
    config/connectionbehavior_config.h
    config/dcc_config.cpp
    config/dcc_config.h
    config/highlight_config.cpp
    config/highlight_config.h
    config/highlighttreewidget.cpp
    config/highlighttreewidget.h
    config/ignore_config.cpp
    config/ignore_config.h
    config/launcherentry_config.cpp
    config/launcherentry_config.h
    config/nicklistbehavior_config.cpp
    config/nicklistbehavior_config.h
    config/osd_config.cpp
    config/osd_config.h
    config/quickbuttons_config.cpp
    config/quickbuttons_config.h
    config/settingsdialog.cpp
    config/settingsdialog.h
    config/settingspage.h
    config/tabs_config.cpp
    config/tabs_config.h
    config/theme_config.cpp
    config/theme_config.h
    config/warnings_config.cpp
    config/warnings_config.h
)

ki18n_wrap_ui(konversation
    config/alias_configui.ui
    config/autoreplace_configui.ui
    config/chatwindowappearance_config.ui
    config/chatwindowbehaviour_config.ui
    config/colorsappearance_config.ui
    config/connectionbehavior_config.ui
    config/dcc_configui.ui
    config/fontappearance_config.ui
    config/generalbehavior_configui.ui
    config/highlight_configui.ui
    config/ignore_configui.ui
    config/launcherentry_config.ui
    config/log_config.ui
    config/nicklistbehavior_configui.ui
    config/osd_configui.ui
    config/quickbuttons_configui.ui
    config/tabnotifications_config.ui
    config/tabs_configui.ui
    config/theme_configui.ui
    config/warnings_configui.ui
    config/watchednicknames_configui.ui
)

    #=== Viewer
target_sources(konversation PRIVATE
    viewer/awaylabel.cpp
    viewer/awaylabel.h
    viewer/channeloptionsdialog.cpp
    viewer/channeloptionsdialog.h
    viewer/chatwindow.cpp
    viewer/chatwindow.h
    viewer/editnotifydialog.cpp
    viewer/editnotifydialog.h
    viewer/highlight.cpp
    viewer/highlight.h
    viewer/highlightviewitem.cpp
    viewer/highlightviewitem.h
    viewer/ignore.cpp
    viewer/ignore.h
    viewer/ignorelistviewitem.cpp
    viewer/ignorelistviewitem.h
    viewer/images.cpp
    viewer/images.h
    viewer/insertchardialog.cpp
    viewer/insertchardialog.h
    viewer/irccolorchooser.cpp
    viewer/irccolorchooser.h
    viewer/irccontextmenus.cpp
    viewer/irccontextmenus.h
    viewer/ircinput.cpp
    viewer/ircinput.h
    viewer/ircviewbox.cpp
    viewer/ircviewbox.h
    viewer/ircview.cpp
    viewer/ircview.h
    viewer/logfilereader.cpp
    viewer/logfilereader.h
    viewer/nickiconset.cpp
    viewer/nickiconset.h
    viewer/osd.cpp
    viewer/osd.h
    viewer/pasteeditor.cpp
    viewer/pasteeditor.h
    viewer/quickbutton.cpp
    viewer/quickbutton.h
    viewer/rawlog.cpp
    viewer/rawlog.h
    viewer/searchbar.cpp
    viewer/searchbar.h
    viewer/statuspanel.cpp
    viewer/statuspanel.h
    viewer/topicedit.cpp
    viewer/topicedit.h
    viewer/topichistoryview.cpp
    viewer/topichistoryview.h
    viewer/topiclabel.cpp
    viewer/topiclabel.h
    viewer/trayicon.cpp
    viewer/trayicon.h
    viewer/viewcontainer.cpp
    viewer/viewcontainer.h
    viewer/viewspringloader.cpp
    viewer/viewspringloader.h
    viewer/viewtree.cpp
    viewer/viewtree.h
)

ki18n_wrap_ui(konversation
    viewer/channeloptionsui.ui
    viewer/irccolorchooserui.ui
    viewer/pasteeditor.ui
)

     #=== DCC
target_sources(konversation PRIVATE
    dcc/chatcontainer.cpp
    dcc/chatcontainer.h
    dcc/chat.cpp
    dcc/chat.h
    dcc/dcccommon.cpp
    dcc/dcccommon.h
    dcc/dccfiledialog.cpp
    dcc/dccfiledialog.h
    dcc/recipientdialog.cpp
    dcc/recipientdialog.h
    dcc/resumedialog.cpp
    dcc/resumedialog.h
    dcc/transfer.cpp
    dcc/transferdetailedinfopanel.cpp
    dcc/transferdetailedinfopanel.h
    dcc/transfer.h
    dcc/transferlistmodel.cpp
    dcc/transferlistmodel.h
    dcc/transfermanager.cpp
    dcc/transfermanager.h
    dcc/transferpanel.cpp
    dcc/transferpanel.h
    dcc/transferrecv.cpp
    dcc/transferrecv.h
    dcc/transfersend.cpp
    dcc/transfersend.h
    dcc/transferview.cpp
    dcc/transferview.h
    dcc/whiteboardcolorchooser.cpp
    dcc/whiteboardcolorchooser.h
    dcc/whiteboard.cpp
    dcc/whiteboardfontchooser.cpp
    dcc/whiteboardfontchooser.h
    dcc/whiteboardglobals.cpp
    dcc/whiteboardglobals.h
    dcc/whiteboard.h
    dcc/whiteboardpaintarea.cpp
    dcc/whiteboardpaintarea.h
    dcc/whiteboardtoolbar.cpp
    dcc/whiteboardtoolbar.h
)

ki18n_wrap_ui(konversation
     dcc/transferdetailedinfopanelui.ui
     dcc/transferdetailedtimeinfopanelui.ui
     dcc/whiteboardtoolbarui.ui
     dcc/whiteboardfontchooserui.ui
)

if (Qca-qt6_FOUND)
    target_sources(konversation PRIVATE
        cipher.cpp)
endif ()

target_sources(konversation PRIVATE
    upnp/soap.cpp
    upnp/soap.h
    upnp/upnpdescriptionparser.cpp
    upnp/upnpdescriptionparser.h
    upnp/upnpmcastsocket.cpp
    upnp/upnpmcastsocket.h
    upnp/upnprouter.cpp
    upnp/upnprouter.h
)

ki18n_wrap_ui(konversation
    identitydialog.ui
    queuetunerbase.ui
    viewer/searchbarbase.ui
)

kconfig_add_kcfg_files(konversation config/preferences_base.kcfgc)
kconfig_add_kcfg_files(konversation config/konversation_state.kcfgc)

ecm_qt_declare_logging_category(konversation
    HEADER konversation_log.h
    IDENTIFIER KONVERSATION_LOG
    CATEGORY_NAME konversation
    DESCRIPTION "Konversation"
    EXPORT KONVERSATION
)

ecm_setup_version(${KONVERSATION_VERSION}
    VARIABLE_PREFIX KONVI
    VERSION_HEADER version.h
)

# Sets the icon on Windows and OSX
file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../data/images/icons/*apps-konversation.png")
ecm_add_app_icon(konversation ICONS ${ICONS_SRCS})

target_link_libraries(konversation
    Qt::Multimedia
    Qt::Network
    Qt::Widgets
    Qt6::Core5Compat

    KF6::Archive
    KF6::Bookmarks
    KF6::ConfigWidgets
    KF6::Crash
    KF6::I18n
    KF6::IdleTime
    KF6::NotifyConfig
    KF6::KIOFileWidgets
    KF6::KIOWidgets
    KF6::Parts
    KF6::Wallet
    KF6::WidgetsAddons
    KF6::DBusAddons
    KF6::CoreAddons
    KF6::Notifications
    KF6::TextWidgets
    KF6::WindowSystem
    KF6::ItemViews
    KF6::Codecs
    KF6::NewStuffWidgets
    KF6::StatusNotifierItem
    KF6::BookmarksWidgets
)

if(TARGET KF6::GlobalAccel)
    target_link_libraries(konversation KF6::GlobalAccel)
endif()

if (Qca-qt6_FOUND)
    target_link_libraries(konversation qca-qt6)
endif ()
if (WIN32)
  target_link_libraries(konversation ws2_32) # for symbols from winsock2.h: ntohl, etc.
endif()
if (HAIKU)
  target_link_libraries(konversation network) # for undefined references inet_addr and setsockopt
endif ()

set_target_properties(konversation PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in)
install(TARGETS konversation ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

ecm_qt_install_logging_categories(
    EXPORT KONVERSATION
    FILE konversation.categories
    DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}"
)
