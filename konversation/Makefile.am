####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = konversation

## INCLUDES were found outside kdevelop specific part

konversation_SOURCES = konviface.skel konversationmainwindow.cpp valuelistviewitem.cpp urlcatcher.cpp searchdialog.cpp prefspagehighlight.cpp scriptlauncher.cpp rawlog.cpp prefspagenotify.cpp prefspagedialogs.cpp prefspagecolorsimages.cpp prefspagebuttons.cpp konvdcop.cpp channellistpanel.cpp konsolepanel.cpp dcctransferhandler.cpp prefspageignore.cpp ircresolver.cpp identity.cpp statuspanel.cpp prefspageappearance.cpp dccrecipientdialog.cpp topiccombobox.cpp prefspagegeneralsettings.cpp prefspagelog.cpp prefspagedccsettings.cpp prefspagescripts.cpp nicksonline.cpp prefspageidentity.cpp prefspage.cpp prefspageserverlist.cpp dccpanel.cpp dcctransfer.cpp ledlistviewitem.cpp highlight.cpp highlightviewitem.cpp nickchangedialog.cpp modebutton.cpp ignorelistviewitem.cpp ignorecheckbox.cpp ignore.cpp nicklistview.cpp ircinput.cpp ledtabbar.cpp ledtab.cpp ledtabwidget.cpp images.cpp editserverdialog.cpp serverlistitem.cpp serverentry.cpp prefsdialog.cpp quickbutton.cpp preferences.cpp chatwindow.cpp outputfilter.cpp query.cpp konversationapplication.cpp nick.cpp inputfilter.cpp channel.cpp ircview.cpp server.cpp ircserversocket.cpp main.cpp prefspageirccolorsui.ui prefspageirccolors.cpp irccolorchooserui.ui irccolorchooser.cpp prefspagealiases.cpp channellistviewitem.cpp osd.cpp prefspageosd.cpp ircevent.cpp
konversation_LDADD   = $(LIB_KIO) -lm


EXTRA_DIST = main.cpp konversationui.rc ircserversocket.cpp ircserversocket.h server.cpp server.h ircview.cpp ircview.h channel.cpp channel.h inputfilter.cpp inputfilter.h errorcodes.h nick.cpp nick.h konversationapplication.cpp konversationapplication.h query.cpp query.h outputfilter.cpp outputfilter.h chatwindow.cpp chatwindow.h preferences.cpp preferences.h quickbutton.cpp quickbutton.h prefsdialog.cpp prefsdialog.h serverentry.cpp serverentry.h serverlistitem.cpp serverlistitem.h editserverdialog.cpp editserverdialog.h images.cpp images.h ledtabwidget.cpp ledtabwidget.h ledtab.cpp ledtab.h ledtabbar.cpp ledtabbar.h ircinput.cpp ircinput.h nicklistview.cpp nicklistview.h konversation.desktop ignore.cpp ignore.h ignorecheckbox.cpp ignorecheckbox.h ignorelistviewitem.cpp ignorelistviewitem.h modebutton.cpp modebutton.h nickchangedialog.cpp nickchangedialog.h highlightviewitem.cpp highlightviewitem.h highlight.cpp highlight.h ledlistviewitem.cpp ledlistviewitem.h dcctransfer.cpp dcctransfer.h dccpanel.cpp dccpanel.h prefspageserverlist.cpp prefspageserverlist.h prefspage.cpp prefspage.h prefspageidentity.cpp prefspageidentity.h nicksonline.cpp nicksonline.h prefspagescripts.cpp prefspagescripts.h prefspagedccsettings.cpp prefspagedccsettings.h prefspagelog.cpp prefspagelog.h prefspagegeneralsettings.h prefspagegeneralsettings.cpp topiccombobox.cpp topiccombobox.h dccrecipientdialog.cpp dccrecipientdialog.h prefspageappearance.cpp prefspageappearance.h statuspanel.cpp statuspanel.h identity.cpp identity.h main.h ircresolver.cpp ircresolver.h app_icon_16.png app_icon_32.png app_icon_48.png prefspageignore.cpp prefspageignore.h dcctransferhandler.cpp dcctransferhandler.h konsolepanel.cpp konsolepanel.h event.h channellistpanel.cpp channellistpanel.h konvdcop.cpp prefspagebuttons.cpp konviface.h prefspagebuttons.h prefspagecolorsimages.cpp prefspagecolorsimages.h prefspagedialogs.cpp prefspagedialogs.h prefspagenotify.cpp prefspagenotify.h rawlog.cpp rawlog.h scriptlauncher.cpp scriptlauncher.h prefspagehighlight.cpp prefspagehighlight.h searchdialog.cpp searchdialog.h urlcatcher.cpp urlcatcher.h valuelistviewitem.cpp valuelistviewitem.h konversationmainwindow.cpp konversationmainwindow.h konvdcop.h version.h

xdg_apps_DATA = konversation.desktop

####### kdevelop will overwrite this part!!! (end)############
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

# set the include path for X, qt and KDE
INCLUDES= $(all_includes)

METASOURCES = AUTO

# the library search path.
konversation_LDFLAGS = $(all_libraries) $(KDE_RPATH)

rcdir = $(kde_datadir)/konversation
rc_DATA = konversationui.rc eventsrc
KDE_ICON = AUTO

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/konversation.pot; \
	fi

