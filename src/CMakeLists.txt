
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/config ${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )

#add_subdirectory(linkaddressbook)
add_subdirectory(blowfish)

add_subdirectory(irc)

include_directories(blowfish config dcc irc preferences viewer)

remove_definitions(-DKDE_DEPRECATED_WARNINGS)

########### next target ###############



set(core_SRCS
    #====================================
    #=== Application
#   dcop.cpp
    config/preferences.cpp
    application.cpp
    mainwindow.cpp
    main.cpp
    common.cpp
    sound.cpp
    ssllabel.cpp
    statusbar.cpp
#    bookmarkhandler.cpp
#    bookmarkmenu.cpp
#    configdialog.cpp
#    settingsdialog.cpp
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
    irc/channeldialog.cpp
    irc/channellistpanel.cpp
    irc/channellistviewitem.cpp
    irc/channeloptionsdialog.cpp
    irc/channelnick.cpp
    irc/modebutton.cpp
    irc/joinchanneldialog.cpp

    irc/irccharsets.cpp

    irc/nick.cpp
    irc/nickinfo.cpp
    irc/nicklistview.cpp
    irc/nicksonline.cpp
    irc/nicksonlineitem.cpp

    #=== Server
    irc/inputfilter.cpp
    irc/outputfilter.cpp
#    irc/sslsocket.cpp
    irc/ircqueue.cpp
    irc/serverdialog.cpp
    irc/servergroupdialog.cpp
    irc/servergroupsettings.cpp
    irc/serverison.cpp
    irc/serverlistdialog.cpp
    irc/serverlistview.cpp
    irc/serversettings.cpp


)

kde4_add_ui3_files(irc_SRCS irc/channeloptionsui.ui irc/joinchannelui.ui irc/servergroupdialogui.ui)

    #=== Preferences
set(prefs_SRCS
    preferences/alias_preferences.cpp
    preferences/autoreplace_preferences.cpp
    preferences/dcc_preferences.cpp
    preferences/highlight_preferences.cpp
    preferences/ignore_preferences.cpp
    preferences/nicklistbehavior_preferences.cpp
    preferences/osd_preferences.cpp
    preferences/tabs_preferences.cpp
    preferences/theme_preferences.cpp
    preferences/quickbuttons_preferences.cpp
    preferences/warnings_preferences.cpp

    preferences/watchednicknames_preferences.cpp
    preferences/valuelistviewitem.cpp
)

kde4_add_ui3_files(prefs_SRCS
    preferences/alias_preferencesui.ui
    preferences/autoreplace_preferencesui.ui
    preferences/chatwindowappearance_preferences.ui
    preferences/chatwindowbehaviour_preferences.ui
    preferences/colorsappearance_preferences.ui
    preferences/connectionbehavior_preferences.ui
    preferences/dcc_preferencesui.ui
    preferences/fontappearance_preferences.ui
    preferences/generalbehavior_preferences.ui
    preferences/highlight_preferencesui.ui
    preferences/ignore_preferencesui.ui
    preferences/log_preferences.ui
    preferences/nicklistbehavior_preferencesui.ui
    preferences/osd_preferencesui.ui
    preferences/quickbuttons_preferencesui.ui
    preferences/tabnotifications_preferences.ui
    preferences/tabs_preferencesui.ui
    preferences/theme_preferencesui.ui
    preferences/warnings_preferencesui.ui
    preferences/watchednicknames_preferencesui.ui
)

    #=== Viewer
set(viewer_SRCS
    viewer/ircinput.cpp
    viewer/ircview.cpp
    viewer/chatwindow.cpp
    viewer/rawlog.cpp

    statuspanel.cpp

    viewer/multilineedit.cpp
    viewer/multilinetextedit.cpp
    viewer/ircviewbox.cpp
    viewer/viewcontainer.cpp
    viewer/viewtree.cpp
    viewer/viewtreeitem.cpp

    viewer/highlight.cpp
    viewer/highlightviewitem.cpp
    viewer/ignore.cpp
    viewer/ignorelistviewitem.cpp
    viewer/irccolorchooser.cpp
    viewer/logfilereader.cpp
    viewer/insertchardialog.cpp
    viewer/osd.cpp
    viewer/topiccombobox.cpp
    #viewer/topiclabel.cpp
    viewer/trayicon.cpp
    viewer/editnotifydialog.cpp
    viewer/emoticon.cpp
    viewer/images.cpp
    viewer/quickbutton.cpp
    viewer/searchbar.cpp
)
kde4_add_ui3_files(viewer_SRCS viewer/irccolorchooserui.ui)


    #=== DCC
set(dcc_SRCS
    dcc/chat.cpp
    dcc/common.cpp
    dcc/recipientdialog.cpp
    #dcc/resumedialog.cpp
    dcc/transfer.cpp
    dcc/transferdetailedinfopanel.cpp
    dcc/transfermanager.cpp
    dcc/transferpanel.cpp
    dcc/transferpanelitem.cpp
    dcc/transferrecv.cpp
    dcc/transfersend.cpp
)

kde4_add_ui3_files(dcc_SRCS dcc/transferdetailedinfopanelui.ui)

set (completed_SRCS ${core_SRCS}  ${irc_SRCS} ${viewer_SRCS} ${prefs_SRCS} ${dcc_SRCS})
set (konvi_SRCS ${completed_SRCS}) 



message(STATUS "DCOP has been removed in KDE 4; port code to D-Bus. kde4_add_dcop_skels macro does not exist anymore. We keep it just to remember to port to D-Bus.")
#kde4_add_dcop_skels(konvi_SRCS konviface.h )
kde4_add_ui3_files(konvi_SRCS viewer/searchbarbase.ui queuetunerbase.ui)
kde4_add_kcfg_files(konvi_SRCS config/preferences_base.kcfgc)

kde4_add_executable(konvi ${konvi_SRCS})

target_link_libraries(konvi ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KDE3SUPPORT_LIBS} ${KDEPIMLIBS_KABC_LIBS})
if (X11_Xscreensaver_FOUND)
    target_link_libraries(konvi ${X11_Xscreensaver_LIB})
endif (X11_Xscreensaver_FOUND)

install(TARGETS konvi ${INSTALL_TARGETS_DEFAULT_ARGS})

