METASOURCES = AUTO

bin_PROGRAMS = konversation

konversation_SOURCES = konviface.skel konversationmainwindow.cpp valuelistviewitem.cpp urlcatcher.cpp searchdialog.cpp prefspagehighlight.cpp scriptlauncher.cpp rawlog.cpp prefspagenotify.cpp prefspagedialogs.cpp prefspagecolorsimages.cpp prefspagebuttons.cpp konvdcop.cpp channellistpanel.cpp konsolepanel.cpp dcctransferhandler.cpp prefspageignore.cpp ircresolver.cpp identity.cpp statuspanel.cpp prefspageappearance.cpp dccrecipientdialog.cpp topiccombobox.cpp prefspagegeneralsettings.cpp prefspagelog.cpp prefspagedccsettings.cpp prefspagescripts.cpp nicksonline.cpp prefspageidentity.cpp prefspage.cpp prefspageserverlist.cpp dccpanel.cpp dcctransfer.cpp ledlistviewitem.cpp highlight.cpp highlightviewitem.cpp nickchangedialog.cpp modebutton.cpp ignorelistviewitem.cpp ignorecheckbox.cpp ignore.cpp nicklistview.cpp ircinput.cpp ledtabbar.cpp ledtab.cpp ledtabwidget.cpp images.cpp editserverdialog.cpp serverlistitem.cpp serverentry.cpp prefsdialog.cpp quickbutton.cpp preferences.cpp chatwindow.cpp outputfilter.cpp query.cpp konversationapplication.cpp nick.cpp inputfilter.cpp channel.cpp ircview.cpp server.cpp ircserversocket.cpp main.cpp prefspageirccolorsui.ui prefspageirccolors.cpp irccolorchooserui.ui irccolorchooser.cpp prefspagealiases.cpp channellistviewitem.cpp osd.cpp prefspageosd.cpp ircevent.cpp trayicon.cpp tabaction.cpp prefspagenickcompletion.cpp prefspagetabbehavior.cpp dccchat.cpp logfilereader.cpp prefspagewebbrowser.cpp multilineedit.cpp nickinfo.cpp dccresumedialog.cpp konversationsound.cpp quickconnectdialog.cpp
konversation_LDADD   = $(LIB_KIO) -lm $(LIB_ARTS)

xdg_apps_DATA = konversation.desktop

# set the include path for X, qt and KDE
INCLUDES= $(all_includes)

# the library search path.
konversation_LDFLAGS = $(all_libraries) $(KDE_RPATH)

rcdir = $(kde_datadir)/konversation
rc_DATA = konversationui.rc eventsrc
KDE_ICON = AUTO

updatedir = $(kde_datadir)/kconf_update
update_DATA = konversation.upd
update_SCRIPTS = konversation-0.14-mainwindow.pl

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/konversation.pot; \
	fi
