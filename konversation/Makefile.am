####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = konversation

## INCLUDES were found outside kdevelop specific part

konversation_SOURCES = konviface.skel valuelistviewitem.cpp channellistpanel.cpp konversationmainwindow.cpp rawlog.cpp searchdialog.cpp dcctransferhandler.cpp scriptlauncher.cpp konvdcop.cpp ircresolver.cpp identity.cpp statuspanel.cpp prefspageappearance.cpp dccrecipientdialog.cpp topiccombobox.cpp prefspagegeneralsettings.cpp prefspagelog.cpp prefspagedccsettings.cpp prefspagescripts.cpp nicksonline.cpp prefspageidentity.cpp prefspage.cpp prefspageserverlist.cpp dccpanel.cpp dcctransfer.cpp ledlistviewitem.cpp highlight.cpp highlightviewitem.cpp highlightview.cpp highlightdialog.cpp colorconfiguration.cpp notifydialog.cpp nickchangedialog.cpp modebutton.cpp ignorelistviewitem.cpp ignorecheckbox.cpp ignore.cpp ignoredialog.cpp quickbuttonsdialog.cpp nicklistview.cpp ircinput.cpp ledtabbar.cpp ledtab.cpp ledtabwidget.cpp images.cpp editserverdialog.cpp serverlistitem.cpp serverentry.cpp prefsdialog.cpp quickbutton.cpp preferences.cpp chatwindow.cpp outputfilter.cpp query.cpp konversationapplication.cpp nick.cpp inputfilter.cpp channel.cpp ircview.cpp server.cpp ircserversocket.cpp main.cpp konsolepanel.cpp konsolepanel.h commit.h urlcatcher.h urlcatcher.cpp prefspagedialogs.h prefspagedialogs.cpp prefspagecolorsimages.h prefspagecolorsimages.cpp
konversation_LDADD   = -lkio $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBSOCKET)


EXTRA_DIST = main.cpp konversationui.rc ircserversocket.cpp ircserversocket.h server.cpp server.h ircview.cpp ircview.h channel.cpp channel.h inputfilter.cpp inputfilter.h errorcodes.h nick.cpp nick.h konversationapplication.cpp konversationapplication.h query.cpp query.h outputfilter.cpp outputfilter.h chatwindow.cpp chatwindow.h preferences.cpp preferences.h quickbutton.cpp quickbutton.h prefsdialog.cpp prefsdialog.h serverentry.cpp serverentry.h serverlistitem.cpp serverlistitem.h editserverdialog.cpp editserverdialog.h images.cpp images.h ledtabwidget.cpp ledtabwidget.h ledtab.cpp ledtab.h ledtabbar.cpp ledtabbar.h led_blue_off.png led_blue_on.png led_green_off.png led_green_on.png led_red_off.png led_red_on.png led_yellow_off.png led_yellow_on.png close_pane.png ircinput.cpp ircinput.h nicklistview.cpp nicklistview.h quickbuttonsdialog.cpp quickbuttonsdialog.h konversation.desktop ignoredialog.cpp ignoredialog.h ignore.cpp ignore.h ignorecheckbox.cpp ignorecheckbox.h ignorelistviewitem.cpp ignorelistviewitem.h modebutton.cpp modebutton.h nickchangedialog.cpp nickchangedialog.h notifydialog.cpp notifydialog.h colorconfiguration.cpp colorconfiguration.h highlightdialog.cpp highlightdialog.h highlightview.cpp highlightview.h highlightviewitem.cpp highlightviewitem.h highlight.cpp highlight.h ledlistviewitem.cpp ledlistviewitem.h dcctransfer.cpp dcctransfer.h dccpanel.cpp dccpanel.h prefspageserverlist.cpp prefspageserverlist.h prefspage.cpp prefspage.h prefspageidentity.cpp prefspageidentity.h nicksonline.cpp nicksonline.h mycolorcombo.cpp mycolorcombo.h prefspagescripts.cpp prefspagescripts.h prefspagedccsettings.cpp prefspagedccsettings.h prefspagelog.cpp prefspagelog.h prefspagegeneralsettings.h prefspagegeneralsettings.cpp topiccombobox.cpp topiccombobox.h dccrecipientdialog.cpp dccrecipientdialog.h prefspageappearance.cpp prefspageappearance.h statuspanel.cpp statuspanel.h identity.cpp identity.h main.h ircresolver.cpp ircresolver.h konvdcop.cpp konvdcop.h event.h scriptlauncher.cpp scriptlauncher.h konviface.h dcctransferhandler.cpp dcctransferhandler.h searchdialog.cpp searchdialog.h rawlog.cpp rawlog.h konversationmainwindow.h konversationmainwindow.cpp channellistpanel.cpp channellistpanel.h valuelistviewitem.h valuelistviewitem.cpp 

install-data-local:
	$(mkinstalldirs) $(kde_appsdir)/Internet/
	$(INSTALL_DATA) $(srcdir)/konversation.desktop $(kde_appsdir)/Internet/konversation.desktop

uninstall-local:
	-rm -f $(kde_appsdir)/Internet/konversation.desktop

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
rc_DATA = konversationui.rc
KDE_ICON = AUTO

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/konversation.pot; \
	fi

