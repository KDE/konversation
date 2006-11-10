SUBDIRS = config linkaddressbook blowfish .

METASOURCES = AUTO

bin_PROGRAMS = konversation
bin_SCRIPTS = konversationircprotocolhandler

konversation_SOURCES = konviface.skel konviconfigdialog.cpp konversationstatusbar.cpp \
		konvisettingsdialog.cpp konversationmainwindow.cpp valuelistviewitem.cpp urlcatcher.cpp \
		scriptlauncher.cpp rawlog.cpp konvdcop.cpp channellistpanel.cpp \
		konsolepanel.cpp identity.cpp statuspanel.cpp \
		dccrecipientdialog.cpp topiccombobox.cpp nicksonline.cpp dcctransferpanel.cpp dcctransferpanelitem.cpp \
		nicklistviewitem.cpp highlight.cpp highlightviewitem.cpp modebutton.cpp \
		ignore_preferencesui.ui ignore_preferences.cpp ignore.cpp nicklistview.cpp ircinput.cpp \
		ignorelistviewitem.cpp images.cpp quickbutton.cpp chatwindow.cpp outputfilter.cpp 	query.cpp \
	konversationapplication.cpp nick.cpp inputfilter.cpp channel.cpp 	ircview.cpp server.cpp main.cpp \
	dcc_preferencesui.ui log_preferences.ui tabs_preferencesui.ui tabs_preferences.cpp \
	chatwindowappearance_preferences.ui irccolorchooserui.ui 	colorsappearance_preferences.ui \
	irccolorchooser.cpp channellistviewitem.cpp osd.cpp trayicon.cpp dccchat.cpp \
	logfilereader.cpp multilineedit.cpp nickinfo.cpp 	dccresumedialog.cpp \
	konversationsound.cpp quickconnectdialog.cpp serverlistdialog.cpp 	dcctransfersend.cpp \
	dcctransferrecv.cpp channelnick.cpp dccdetaildialog.cpp 	insertchardialog.cpp \
	irccharsets.cpp editnotifydialog.cpp common.cpp serverison.cpp 	sslsocket.cpp \
	servergroupsettings.cpp serversettings.cpp servergroupdialog.cpp 	ssllabel.cpp serverdialog.cpp \
	channeldialog.cpp identitydialog.cpp topiclabel.cpp 	channeloptionsui.ui \
	channeloptionsdialog.cpp notificationhandler.cpp joinchannelui.ui 	joinchanneldialog.cpp \
	emoticon.cpp chatwindowbehaviour_preferences.ui alias_preferencesui.ui \
		osd_preferencesui.ui theme_preferencesui.ui highlight_preferencesui.ui \
		warnings_preferencesui.ui warnings_preferences.cpp quickbuttons_preferencesui.ui \
		watchednicknames_preferencesui.ui generalbehavior_preferences.ui connectionbehavior_preferences.ui \
		fontappearance_preferences.ui nicklistbehavior_preferencesui.ui konvibookmarkhandler.cpp \
		konvibookmarkmenu.cpp ircviewbox.cpp searchbar.cpp osd_preferences.cpp \
		theme_preferences.cpp dcc_preferences.cpp alias_preferences.cpp highlight_preferences.cpp \
		watchednicknames_preferences.cpp quickbuttons_preferences.cpp nicklistbehavior_preferences.cpp \
		konviiphelper.cpp tabnotifications_preferences.ui \
		multilinetextedit.cpp serverlistview.cpp nicksonlineitem.cpp searchbarbase.ui \
	autoreplace_preferencesui.ui autoreplace_preferences.cpp viewcontainer.cpp viewtree.cpp \
	viewtreeitem.cpp servergroupdialogui.ui

konversation_COMPILE_FIRST = config/preferences_base.h
konversation_LDADD = $(LIB_KIO) $(LIB_KABC) $(LIB_KIMIFACE) linkaddressbook/liblinkaddressbookui.la blowfish/libblowfish.la config/libkonversationconfig.la

xdg_apps_DATA = konversation.desktop

# set the include path for X, qt and KDE
INCLUDES= -Ilinkaddressbook -I$(srcdir)/linkaddressbook -Iblowfish -I$(srcdir)/blowfish\
          -Iconfig -I$(srcdir)/config $(all_includes)
# the library search path.
konversation_LDFLAGS = $(all_libraries) $(KDE_RPATH)

rcdir = $(kde_datadir)/konversation
rc_DATA = konversationui.rc eventsrc

kdelnk_DATA = konvirc.protocol konvirc6.protocol
kdelnkdir = $(kde_servicesdir)

updatedir = $(kde_datadir)/kconf_update
update_DATA = konversation.upd
update_SCRIPTS = konversation-0.19-colors.pl konversation-0.19-sortorder.pl konversation-0.19-appearance.pl \
				 konversation-0.19-tabplacement.pl konversation-0.19-custombrowser.pl \
			     konversation-0.19-colorcodes.pl konversation-0.19-notifylists.pl \
				 konversation-0.20-quickbuttons.pl konversation-0.20-customfonts.pl

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
		$(XGETTEXT) $$LIST -o $(podir)/konversation.pot; \
	fi

noinst_HEADERS = serverlistdialog.h dcctransfersend.h dcctransferrecv.h 		 \
	dccdetaildialog.h insertchardialog.h editnotifydialog.h serverison.h 		 \
	servergroupsettings.h serversettings.h	servergroupdialog.h serverdialog.h channeldialog.h 		 \
	identitydialog.h topiclabel.h channeloptionsdialog.h joinchanneldialog.h  		 \
	highlight_preferences.h watchednicknames_preferences.h quickbuttons_preferences.h 		 \
	nicklistbehavior_preferences.h konvisettingspage.h nicksonlineitem.h

# User might still have these in src/ instead of src/config, so remove these in that case
CLEANFILES = preferences_base.h preferences_base.cpp watchednicknamesconfigcontroller.*
