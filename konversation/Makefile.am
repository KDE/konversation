if compile_LIBKONVERSATION_COMPAT
COMPAT_DIR = compat
COMPAT_INCLUDE = -Icompat
COMPAT_LIBS = compat/libkonversationcompat.la
endif

SUBDIRS = $(COMPAT_DIR) linkaddressbook

METASOURCES = AUTO

bin_PROGRAMS = konversation
bin_SCRIPTS = konversationircprotocolhandler

konversation_SOURCES = konviface.skel konversationmainwindow.cpp \
		valuelistviewitem.cpp urlcatcher.cpp searchdialog.cpp prefspagehighlight.cpp \
		scriptlauncher.cpp rawlog.cpp prefspagenotify.cpp prefspagedialogs.cpp \
		prefspagebuttons.cpp konvdcop.cpp channellistpanel.cpp konsolepanel.cpp \
		dcctransferhandler.cpp prefspageignore.cpp identity.cpp \
		statuspanel.cpp	dccrecipientdialog.cpp topiccombobox.cpp prefspagelog.cpp prefspagedccsettings.cpp \
		prefspagescripts.cpp nicksonline.cpp prefspage.cpp dccpanel.cpp dcctransfer.cpp \
		nicklistviewitem.cpp highlight.cpp highlightviewitem.cpp modebutton.cpp \
		ignorelistviewitem.cpp ignorecheckbox.cpp ignore.cpp nicklistview.cpp ircinput.cpp \
		ledtabbar.cpp ledtab.cpp ledtabwidget.cpp images.cpp prefsdialog.cpp \
		quickbutton.cpp preferences.cpp chatwindow.cpp outputfilter.cpp query.cpp \
		konversationapplication.cpp nick.cpp inputfilter.cpp channel.cpp ircview.cpp server.cpp main.cpp \
		dcc_preferences.ui log_preferences.ui tabbar_preferences.ui chatwindowappearance_preferences.ui irccolorchooserui.ui colorsappearance_preferences.ui irccolorchooser.cpp prefspagealiases.cpp channellistviewitem.cpp osd.cpp prefspageosd.cpp \
		ircevent.cpp trayicon.cpp tabaction.cpp prefspagetabbehavior.cpp dccchat.cpp \
		logfilereader.cpp multilineedit.cpp nickinfo.cpp dccresumedialog.cpp \
		konversationsound.cpp quickconnectdialog.cpp prefspagebehaviour.cpp \
		prefspagechatwinbehavior.cpp prefspagechatwinappearance.cpp prefspagecolorsappearance.cpp \
		serverlistdialog.cpp dcctransfersend.cpp dcctransferrecv.cpp channelnick.cpp \
		dccdetaildialog.cpp insertchardialog.cpp irccharsets.cpp editnotifydialog.cpp common.cpp \
		serverison.cpp sslsocket.cpp servergroupsettings.cpp serversettings.cpp \
		servergroupdialog.cpp ssllabel.cpp serverdialog.cpp channeldialog.cpp identitydialog.cpp \
		topiclabel.cpp channeloptionsui.ui channeloptionsdialog.cpp prefspagethemes.cpp notificationhandler.cpp \
		joinchannelui.ui joinchanneldialog.cpp konversation_settings.kcfgc

konversation_LDADD = $(COMPAT_LIBS) $(LIB_KIO) $(LIB_KABC) $(LIB_KIMIFACE) linkaddressbook/liblinkaddressbookui.la

xdg_apps_DATA = konversation.desktop

# set the include path for X, qt and KDE
INCLUDES= $(COMPAT_INCLUDE) -Ilinkaddressbook -I$(srcdir)/linkaddressbook $(all_includes)
# the library search path.
konversation_LDFLAGS = $(all_libraries) $(KDE_RPATH)

rcdir = $(kde_datadir)/konversation
rc_DATA = konversationui.rc eventsrc
KDE_ICON = AUTO

kdelnk_DATA = konvirc.protocol konvirc6.protocol
kdelnkdir = $(kde_servicesdir)

updatedir = $(kde_datadir)/kconf_update
update_DATA = konversation.upd
update_SCRIPTS = konversation-0.14-mainwindow.pl

kde_kcfg_DATA = konversation.kcfg

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
		$(XGETTEXT) $$LIST -o $(podir)/konversation.pot; \
	fi

noinst_HEADERS = prefspagebehaviour.h prefspagechatwinbehavior.h 	 \
		 prefspagechatwinappearance.h prefspagecolorsappearance.h serverlistdialog.h dcctransfersend.h \
		 dcctransferrecv.h dccdetaildialog.h insertchardialog.h editnotifydialog.h serverison.h \
		 servergroupsettings.h serversettings.h	servergroupdialog.h serverdialog.h channeldialog.h \
		 identitydialog.h topiclabel.h channeloptionsdialog.h joinchanneldialog.h

