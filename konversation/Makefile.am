# set the include path for X, qt and KDE
INCLUDES= $(all_includes)

lib_LTLIBRARIES = konversation.la

konversation_la_SOURCES = konviface.skel konversationmainwindow.cpp \
	valuelistviewitem.cpp urlcatcher.cpp searchdialog.cpp prefspagehighlight.cpp \
	scriptlauncher.cpp rawlog.cpp prefspagenotify.cpp prefspagedialogs.cpp \
	prefspagecolorsimages.cpp prefspagebuttons.cpp konvdcop.cpp channellistpanel.cpp \
	konsolepanel.cpp dcctransferhandler.cpp prefspageignore.cpp ircresolver.cpp identity.cpp \
	statuspanel.cpp prefspageappearance.cpp dccrecipientdialog.cpp topiccombobox.cpp \
	prefspagegeneralsettings.cpp prefspagelog.cpp prefspagedccsettings.cpp \
	prefspagescripts.cpp nicksonline.cpp prefspageidentity.cpp prefspage.cpp \
	prefspageserverlist.cpp dccpanel.cpp dcctransfer.cpp ledlistviewitem.cpp highlight.cpp \
	highlightviewitem.cpp nickchangedialog.cpp modebutton.cpp ignorelistviewitem.cpp \
	ignorecheckbox.cpp ignore.cpp nicklistview.cpp ircinput.cpp ledtabbar.cpp ledtab.cpp \
	ledtabwidget.cpp images.cpp editserverdialog.cpp serverlistitem.cpp serverentry.cpp \
	prefsdialog.cpp quickbutton.cpp preferences.cpp chatwindow.cpp outputfilter.cpp \
	query.cpp konversationapplication.cpp nick.cpp inputfilter.cpp channel.cpp \
	ircview.cpp server.cpp ircserversocket.cpp main.cpp prefspageirccolorsui.ui \
	prefspageirccolors.cpp irccolorchooserui.ui irccolorchooser.cpp

konversation_la_LIBADD   = -lkio -lm $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBSOCKET)
konversation_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -avoid-version
konversation_la_METASOURCES = AUTO


bin_PROGRAMS = konversation
konversation_SOURCES = dummy.cpp prefspagealiases.h prefspagealiases.cpp
konversation_LDFLAGS = $(all_libraries) $(KDE_RPATH)
konversation_LDADD = konversation.la


dummy.cpp:
	echo > dummy.cpp

# this 10 paths are KDE specific. Use them:
# kde_htmldir       Where your docs should go to. (contains lang subdirs)
# kde_appsdir       Where your application file (.kdelnk) should go to.
# kde_icondir       Where your icon should go to.
# kde_minidir       Where your mini icon should go to.
# kde_datadir       Where you install application data. (Use a subdir)
# kde_locale        Where translation files should go to.(contains lang subdirs)
# kde_cgidir        Where cgi-bin executables should go to.
# kde_confdir       Where config files should go to.
# kde_mimedir       Where mimetypes should go to.
# kde_toolbardir    Where general toolbar icons should go to.
# kde_wallpaperdir  Where general wallpapers should go to.


rcdir = $(kde_datadir)/konversation
rc_DATA = konversationui.rc eventsrc

KDE_ICON = konversation

kdelnkdir = $(kde_appsdir)/Internet/
kdelnk_DATA = konversation.desktop

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/konversation.pot; \
	fi

