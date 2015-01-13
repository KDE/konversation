include_directories(${CMAKE_CURRENT_SOURCE_DIR}/config)
include_directories(config dcc irc viewer upnp)

set(core_SRCS
    #====================================
    #=== Application
    config/preferences.cpp
    application.cpp
    dbus.cpp
    mainwindow.cpp
    main.cpp
    common.cpp
    sound.cpp
    ssllabel.cpp
    statusbar.cpp
    bookmarkhandler.cpp
    scriptlauncher.cpp
    konsolepanel.cpp
    notificationhandler.cpp
    awaymanager.cpp
    connectionmanager.cpp
    connectionsettings.cpp
    identity.cpp
    identitydialog.cpp

    #=== GUI
    urlcatcher.cpp
    queuetuner.cpp
    quickconnectdialog.cpp
)

set (irc_SRCS
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

ki18n_wrap_ui(irc_SRCS
    irc/channellistpanelui.ui
    irc/channeldialogui.ui
    irc/joinchannelui.ui
    irc/serverdialogui.ui
    irc/servergroupdialogui.ui
    irc/serverlistdialogui.ui
    irc/invitedialog.ui
)

    #=== Configuration dialog pages
set(config_SRCS
    config/configdialog.cpp
    config/settingsdialog.cpp
    config/alias_config.cpp
    config/autoreplace_config.cpp
    config/dcc_config.cpp
    config/highlight_config.cpp
    config/ignore_config.cpp
    config/nicklistbehavior_config.cpp
    config/osd_config.cpp
    config/tabs_config.cpp
    config/theme_config.cpp
    config/quickbuttons_config.cpp
    config/warnings_config.cpp
    config/connectionbehavior_config.cpp
)

ki18n_wrap_ui(config_SRCS
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
set(viewer_SRCS
    viewer/ircinput.cpp
    viewer/ircview.cpp
    viewer/chatwindow.cpp
    viewer/rawlog.cpp
    viewer/statuspanel.cpp
    viewer/ircviewbox.cpp
    viewer/viewcontainer.cpp
    viewer/pasteeditor.cpp
    viewer/highlight.cpp
    viewer/highlightviewitem.cpp
    viewer/ignore.cpp
    viewer/ignorelistviewitem.cpp
    viewer/irccolorchooser.cpp
    viewer/logfilereader.cpp
    viewer/insertchardialog.cpp
    viewer/osd.cpp
    viewer/topiclabel.cpp
    viewer/awaylabel.cpp
    viewer/editnotifydialog.cpp
    viewer/emoticons.cpp
    viewer/images.cpp
    viewer/quickbutton.cpp
    viewer/searchbar.cpp
    viewer/irccontextmenus.cpp
    viewer/trayicon.cpp
    viewer/viewspringloader.cpp
    viewer/channeloptionsdialog.cpp
    viewer/topicedit.cpp
    viewer/topichistoryview.cpp
    viewer/viewtree.cpp
)

ki18n_wrap_ui(viewer_SRCS
    viewer/channeloptionsui.ui
    viewer/irccolorchooserui.ui
    viewer/pasteeditor.ui
)

     #=== DCC
set(dcc_SRCS
     dcc/chat.cpp
     dcc/chatcontainer.cpp
     dcc/dcccommon.cpp
     dcc/dccfiledialog.cpp
     dcc/recipientdialog.cpp
     dcc/resumedialog.cpp
     dcc/transfer.cpp
     dcc/transferdetailedinfopanel.cpp
     dcc/transfermanager.cpp
     dcc/transferpanel.cpp
     dcc/transferrecv.cpp
     dcc/transfersend.cpp
     dcc/transferlistmodel.cpp
     dcc/transferview.cpp
     dcc/whiteboard.cpp
     dcc/whiteboardcolorchooser.cpp
     dcc/whiteboardfontchooser.cpp
     dcc/whiteboardglobals.cpp
     dcc/whiteboardpaintarea.cpp
     dcc/whiteboardtoolbar.cpp
)

ki18n_wrap_ui(dcc_SRCS
     dcc/transferdetailedinfopanelui.ui
     dcc/transferdetailedtimeinfopanelui.ui
     dcc/whiteboardtoolbarui.ui
     dcc/whiteboardfontchooserui.ui
)

if (Qca-qt5_FOUND)
    set(cipher_SRCS
        cipher.cpp)
endif (Qca-qt5_FOUND)

set(upnp_SRCS
    upnp/soap.cpp
    upnp/upnpdescriptionparser.cpp
    upnp/upnpmcastsocket.cpp
    upnp/upnprouter.cpp
)

set (completed_SRCS ${core_SRCS} ${irc_SRCS} ${viewer_SRCS} ${config_SRCS} ${cipher_SRCS} ${upnp_SRCS} ${dcc_SRCS})
set (konversation_SRCS ${completed_SRCS})

ki18n_wrap_ui(konversation_SRCS
    identitydialog.ui
    queuetunerbase.ui
    viewer/searchbarbase.ui
)

kconfig_add_kcfg_files(konversation_SRCS config/preferences_base.kcfgc)

add_executable(konversation ${konversation_SRCS})

target_link_libraries(konversation
    Qt5::Widgets
    KF5::Archive
    KF5::Bookmarks
    KF5::ConfigWidgets
    KF5::Emoticons
    KF5::I18n
    KF5::IdleTime
    KF5::NotifyConfig
    KF5::KIOFileWidgets
    KF5::KIOWidgets
    KF5::Parts
    KF5::Solid
    KF5::SonnetCore
    KF5::Wallet
    KF5::WidgetsAddons
    KF5::GlobalAccel
    KF5::DBusAddons
    KF5::CoreAddons
    KF5::Notifications
    KF5::WindowSystem
    KF5::IconThemes
    KF5::ItemViews
    Phonon::phonon4qt5)

if (Qca-qt5_FOUND)
    target_link_libraries(konversation qca-qt5)
endif ()

install(TARGETS konversation ${INSTALL_TARGETS_DEFAULT_ARGS})
