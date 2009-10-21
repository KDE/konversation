
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/config)
include_directories(config dcc irc viewer linkaddressbook upnp)

if (QCA2_FOUND)
    include_directories(${QCA2_INCLUDE_DIR})
endif (QCA2_FOUND)

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

kde4_add_ui_files(core_SRCS
    urlcatcherui.ui
)

set (irc_SRCS
    #== IRC
    irc/server.cpp

    irc/query.cpp
    irc/channel.cpp
    irc/channellistpanel.cpp
    irc/channeloptionsdialog.cpp
    irc/channelnick.cpp
    irc/modebutton.cpp
    irc/joinchanneldialog.cpp
    irc/invitedialog.cpp

    irc/irccharsets.cpp

    irc/nick.cpp
    irc/nickinfo.cpp
    irc/nicklistview.cpp
    irc/nicksonline.cpp
    irc/nicksonlineitem.cpp

    #=== Server
    irc/inputfilter.cpp
    irc/outputfilter.cpp
    irc/ircqueue.cpp
    irc/servergroupdialog.cpp
    irc/servergroupsettings.cpp
    irc/serverison.cpp
    irc/serverlistdialog.cpp
    irc/serverlistview.cpp
    irc/serversettings.cpp
)

kde4_add_ui_files(irc_SRCS
    irc/channellistpanelui.ui
    irc/channeldialogui.ui
    irc/channeloptionsui.ui
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
    config/watchednicknames_config.cpp
    config/connectionbehavior_config.cpp
)

kde4_add_ui_files(config_SRCS
    config/alias_configui.ui
    config/autoreplace_configui.ui
    config/chatwindowappearance_config.ui
    config/chatwindowbehaviour_config.ui
    config/colorsappearance_config.ui
    config/connectionbehavior_config.ui
    config/dcc_configui.ui
    config/fontappearance_config.ui
    config/generalbehavior_config.ui
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
    viewer/viewtree.cpp
    viewer/viewtreeitem.cpp
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
    viewer/trayicon.cpp
    viewer/editnotifydialog.cpp
    viewer/emoticons.cpp
    viewer/images.cpp
    viewer/quickbutton.cpp
    viewer/searchbar.cpp
)

kde4_add_ui_files(viewer_SRCS
    viewer/irccolorchooserui.ui
    viewer/pasteeditor.ui
)

    #=== DCC
set(dcc_SRCS
    dcc/chat.cpp
    dcc/dcccommon.cpp
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
)

kde4_add_ui_files(dcc_SRCS
    dcc/transferdetailedinfopanelui.ui
    dcc/transferdetailedtimeinfopanelui.ui
)

set(linkaddressbook_SRCS
    linkaddressbook/linkaddressbookui.cpp
    linkaddressbook/addressbook_base.cpp
    linkaddressbook/addressbook.cpp
    linkaddressbook/addresseeitem.cpp
)

kde4_add_ui_files(linkaddressbook_SRCS
    linkaddressbook/linkaddressbookui_base.ui
)
if (QCA2_FOUND)
    set(cipher_SRCS
        cipher.cpp)
endif (QCA2_FOUND)

set(upnp_SRCS
    upnp/soap.cpp
    upnp/upnpdescriptionparser.cpp
    upnp/upnpmcastsocket.cpp
    upnp/upnprouter.cpp
)

set (completed_SRCS ${core_SRCS}  ${irc_SRCS} ${viewer_SRCS} ${config_SRCS} ${dcc_SRCS} ${linkaddressbook_SRCS} ${cipher_SRCS} ${upnp_SRCS})
set (konversation_SRCS ${completed_SRCS})

kde4_add_ui_files(konversation_SRCS
    identitydialog.ui
    queuetunerbase.ui
    viewer/searchbarbase.ui
)

kde4_add_kcfg_files(konversation_SRCS config/preferences_base.kcfgc)

kde4_add_app_icon(konversation_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../data/images/icons/hi*-app-konversation.png")

kde4_add_executable(konversation ${konversation_SRCS})

target_link_libraries(konversation ${KDE4_KIO_LIBS} ${KDE4_KUTILS_LIBS} ${KDE4_KPARTS_LIBS}
                            ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KABC_LIBS}
                            ${KDE4_KNOTIFYCONFIG_LIBRARY} ${PHONON_LIBRARY} ${KDE4_SOLID_LIBS})
if (X11_FOUND AND X11_Xutil_FOUND)
    target_link_libraries(konversation ${X11_X11_LIB})
endif (X11_FOUND AND X11_Xutil_FOUND)

if (X11_Xscreensaver_FOUND)
    target_link_libraries(konversation ${X11_Xscreensaver_LIB})
endif (X11_Xscreensaver_FOUND)

if (QCA2_FOUND)
    target_link_libraries(konversation ${QCA2_LIBRARIES})
endif (QCA2_FOUND)

CHECK_INCLUDE_FILE(
  "stropts.h" HAVE_STROPTS_H)
IF(HAVE_STROPTS_H)
  ADD_DEFINITIONS(-DHAVE_STROPTS_H)
ENDIF(HAVE_STROPTS_H)


install(TARGETS konversation ${INSTALL_TARGETS_DEFAULT_ARGS})
