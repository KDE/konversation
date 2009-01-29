
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/config ${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )

add_subdirectory(config)
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
#    konsolepanel.cpp
    notificationhandler.cpp

    awaymanager.cpp
    connectionmanager.cpp
    connectionsettings.cpp
    identity.cpp
#    identitydialog.cpp

    #=== GUI
    urlcatcher.cpp
#    queuetuner.cpp
#    quickconnectdialog.cpp
)

set (irc_SRCS
    #== IRC
    irc/server.cpp

    irc/query.cpp
    irc/channel.cpp
    irc/channeldialog.cpp
    irc/channellistpanel.cpp
    irc/channellistviewitem.cpp
#    irc/channeloptionsdialog.cpp
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

kde4_add_ui3_files(irc_SRCS irc/joinchannelui.ui irc/servergroupdialogui.ui)

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
#    viewer/osd.cpp
    viewer/topiccombobox.cpp
#    viewer/topiclabel.cpp
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
    dcc/resumedialog.cpp
    dcc/transfer.cpp
    dcc/transferdetailedinfopanel.cpp
    dcc/transfermanager.cpp
    dcc/transferpanel.cpp
    dcc/transferpanelitem.cpp
    dcc/transferrecv.cpp
    dcc/transfersend.cpp
)

set (completed_SRCS ${core_SRCS}  ${irc_SRCS} ${viewer_SRCS})
set (konvi_SRCS ${completed_SRCS}) #${prefs_SRCS})  #${dcc_SRCS})



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



#original Makefile.am contents follow:

#SUBDIRS = config linkaddressbook blowfish .
#
#METASOURCES = AUTO
#
#bin_PROGRAMS = konversation
#
#konversation_SOURCES = konviface.skel konviconfigdialog.cpp konversationstatusbar.cpp \
#	konvisettingsdialog.cpp viewcontainer.cpp viewtree.cpp viewtreeitem.cpp konversationmainwindow.cpp \
#	valuelistviewitem.cpp urlcatcher.cpp scriptlauncher.cpp rawlog.cpp konvdcop.cpp channellistpanel.cpp \
#	konsolepanel.cpp identity.cpp statuspanel.cpp dccrecipientdialog.cpp topiccombobox.cpp nicksonline.cpp \
#	dcctransferpanel.cpp dcctransferpanelitem.cpp highlight.cpp highlightviewitem.cpp modebutton.cpp \
#	ignore_preferencesui.ui ignore_preferences.cpp ignore.cpp nicklistview.cpp ircinput.cpp \
#	channeloptionsui.ui  channeloptionsdialog.cpp ignorelistviewitem.cpp images.cpp quickbutton.cpp \
#	chatwindow.cpp outputfilter.cpp logfilereader.cpp query.cpp konversationapplication.cpp nick.cpp \
#	inputfilter.cpp channel.cpp ircview.cpp server.cpp main.cpp dcc_preferencesui.ui log_preferences.ui \
#	tabs_preferencesui.ui tabs_preferences.cpp chatwindowappearance_preferences.ui irccolorchooserui.ui \
#	colorsappearance_preferences.ui irccolorchooser.cpp channellistviewitem.cpp osd.cpp trayicon.cpp \
#	dccchat.cpp multilineedit.cpp nickinfo.cpp dccresumedialog.cpp konversationsound.cpp quickconnectdialog.cpp \
#	serverlistdialog.cpp dcctransfersend.cpp dcctransferrecv.cpp channelnick.cpp insertchardialog.cpp \
#	irccharsets.cpp editnotifydialog.cpp common.cpp serverison.cpp sslsocket.cpp servergroupsettings.cpp \
#	serversettings.cpp servergroupdialog.cpp ssllabel.cpp serverdialog.cpp channeldialog.cpp identitydialog.cpp \
#	topiclabel.cpp notificationhandler.cpp joinchannelui.ui joinchanneldialog.cpp emoticon.cpp \
#	chatwindowbehaviour_preferences.ui alias_preferencesui.ui osd_preferencesui.ui theme_preferencesui.ui \
#	highlight_preferencesui.ui warnings_preferencesui.ui warnings_preferences.cpp quickbuttons_preferencesui.ui \
#	watchednicknames_preferencesui.ui generalbehavior_preferences.ui connectionbehavior_preferences.ui \
#	fontappearance_preferences.ui nicklistbehavior_preferencesui.ui konvibookmarkhandler.cpp konvibookmarkmenu.cpp \
#	ircviewbox.cpp searchbar.cpp osd_preferences.cpp theme_preferences.cpp dcc_preferences.cpp \
#	alias_preferences.cpp highlight_preferences.cpp watchednicknames_preferences.cpp quickbuttons_preferences.cpp \
#	nicklistbehavior_preferences.cpp tabnotifications_preferences.ui multilinetextedit.cpp serverlistview.cpp \
#	nicksonlineitem.cpp searchbarbase.ui autoreplace_preferencesui.ui autoreplace_preferences.cpp \
#	servergroupdialogui.ui dcctransfer.cpp dcctransfermanager.cpp dcctransferdetailedinfopanelui.ui \
#	dcctransferdetailedinfopanel.cpp dcccommon.cpp queuetunerbase.ui queuetuner.cpp ircqueue.cpp \
#	connectionsettings.cpp connectionmanager.cpp awaymanager.cpp
#
#konversation_COMPILE_FIRST = config/preferences_base.h
#konversation_LDADD = $(LIB_KIO) $(LIB_XSS) $(LIB_KABC) $(LIB_KIMIFACE) linkaddressbook/liblinkaddressbookui.la blowfish/libblowfish.la config/libkonversationconfig.la
#
#xdg_apps_DATA = konversation.desktop
#
## set the include path for X, qt and KDE
#INCLUDES= -Ilinkaddressbook -I$(srcdir)/linkaddressbook -Iblowfish -I$(srcdir)/blowfish\
#          -Iconfig -I$(srcdir)/config $(all_includes)
## the library search path.
#konversation_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#
#rcdir = $(kde_datadir)/konversation
#rc_DATA = konversationui.rc eventsrc
#
#kdelnk_DATA = konvirc.protocol konvirc6.protocol
#kdelnkdir = $(kde_servicesdir)
#
#updatedir = $(kde_datadir)/kconf_update
#update_DATA = konversation.upd
#update_SCRIPTS = konversation-0.19-colors.pl konversation-0.19-sortorder.pl konversation-0.19-appearance.pl \
#				 konversation-0.19-tabplacement.pl konversation-0.19-custombrowser.pl \
#			     konversation-0.19-colorcodes.pl konversation-0.19-notifylists.pl \
#				 konversation-0.20-quickbuttons.pl konversation-0.20-customfonts.pl
#
#messages: rc.cpp
#	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
#	if test -n "$$LIST"; then \
#		$(XGETTEXT) $$LIST -o $(podir)/konversation.pot; \
#	fi
#
#noinst_HEADERS = serverlistdialog.h dcctransfersend.h dcctransferrecv.h 		 \
#	insertchardialog.h editnotifydialog.h serverison.h 		 \
#	servergroupsettings.h serversettings.h	servergroupdialog.h serverdialog.h channeldialog.h 		 \
#	identitydialog.h topiclabel.h channeloptionsdialog.h joinchanneldialog.h  		 \
#	highlight_preferences.h watchednicknames_preferences.h quickbuttons_preferences.h 		 \
#	nicklistbehavior_preferences.h konvisettingspage.h nicksonlineitem.h
#
## User might still have these in src/ instead of src/config, so remove these in that case
#CLEANFILES = preferences_base.h preferences_base.cpp watchednicknamesconfigcontroller.*
