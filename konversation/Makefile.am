####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = konversation

## INCLUDES were found outside kdevelop specific part

konversation_SOURCES = konviface.skel searchdialog.cpp scriptlauncher.cpp ircresolver.cpp konvdcop.cpp statuspanel.cpp identity.cpp prefspageappearance.cpp topiccombobox.cpp dccrecipientdialog.cpp prefspagegeneralsettings.cpp prefspagelog.cpp prefspagedccsettings.cpp prefspagescripts.cpp mycolorcombo.cpp nicksonline.cpp prefspageidentity.cpp prefspage.cpp prefspageserverlist.cpp dccpanel.cpp dcctransfer.cpp ledlistviewitem.cpp highlight.cpp highlightviewitem.cpp highlightview.cpp highlightdialog.cpp colorconfiguration.cpp notifydialog.cpp nickchangedialog.cpp modebutton.cpp ignorelistviewitem.cpp ignorecheckbox.cpp ignore.cpp ignoredialog.cpp quickbuttonsdialog.cpp nicklistview.cpp ircinput.cpp ledtabbar.cpp ledtab.cpp ledtabwidget.cpp images.cpp editserverdialog.cpp serverlistitem.cpp serverentry.cpp prefsdialog.cpp quickbutton.cpp preferences.cpp chatwindow.cpp outputfilter.cpp query.cpp konversationapplication.cpp nick.cpp inputfilter.cpp channel.cpp ircview.cpp server.cpp ircserversocket.cpp main.cpp rawlog.cpp rawlog.h konversationmainwindow.h konversationmainwindow.cpp dcctransferhandler.h dcctransferhandler.cpp valuelistviewitem.h valuelistviewitem.cpp errorcodes.h event.h channellistpanel.h channellistpanel.cpp
konversation_LDADD   = -lkio $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBSOCKET)


EXTRA_DIST = main.cpp konversationui.rc ircserversocket.cpp ircserversocket.h serverwindow.cpp serverwindow.h server.cpp server.h ircview.cpp ircview.h channel.cpp channel.h inputfilter.cpp inputfilter.h errorcodes.h nick.cpp nick.h konversationapplication.cpp konversationapplication.h query.cpp query.h outputfilter.cpp outputfilter.h chatwindow.cpp chatwindow.h preferences.cpp preferences.h quickbutton.cpp quickbutton.h prefsdialog.cpp prefsdialog.h serverentry.cpp serverentry.h serverlistitem.cpp serverlistitem.h editserverdialog.cpp editserverdialog.h images.cpp images.h ledtabwidget.cpp ledtabwidget.h ledtab.cpp ledtab.h ledtabbar.cpp ledtabbar.h led_blue_off.png led_blue_on.png led_green_off.png led_green_on.png led_red_off.png led_red_on.png led_yellow_off.png led_yellow_on.png close_pane.png ircinput.cpp ircinput.h nicklistview.cpp nicklistview.h quickbuttonsdialog.cpp quickbuttonsdialog.h konversation.desktop ignoredialog.cpp ignoredialog.h ignore.cpp ignore.h ignorecheckbox.cpp ignorecheckbox.h ignorelistviewitem.cpp ignorelistviewitem.h modebutton.cpp modebutton.h nickchangedialog.cpp nickchangedialog.h notifydialog.cpp notifydialog.h colorconfiguration.cpp colorconfiguration.h highlightdialog.cpp highlightdialog.h highlightview.cpp highlightview.h highlightviewitem.cpp highlightviewitem.h highlight.cpp highlight.h ledlistviewitem.cpp ledlistviewitem.h dcctransfer.cpp dcctransfer.h dccpanel.cpp dccpanel.h prefspageserverlist.cpp prefspageserverlist.h prefspage.cpp prefspage.h prefspageidentity.cpp prefspageidentity.h nicksonline.cpp nicksonline.h mycolorcombo.cpp mycolorcombo.h prefspagescripts.cpp prefspagescripts.h prefspagedccsettings.cpp prefspagedccsettings.h prefspagelog.cpp prefspagelog.h prefspagegeneralsettings.cpp prefspagegeneralsettings.h dccrecipientdialog.cpp dccrecipientdialog.h topiccombobox.cpp topiccombobox.h prefspageappearance.cpp prefspageappearance.h identity.cpp identity.h statuspanel.cpp statuspanel.h main.h konviface.h konvdcop.cpp konvdcop.h ircresolver.cpp ircresolver.h scriptlauncher.cpp scriptlauncher.h searchdialog.cpp searchdialog.h app_icon_16.png app_icon_32.png 

install-data-local:
	$(mkinstalldirs) $(kde_datadir)/konversation/images/
	$(INSTALL_DATA) $(srcdir)/led_blue_off.png $(kde_datadir)/konversation/images/led_blue_off.png
	$(mkinstalldirs) $(kde_datadir)/konversation/images/
	$(INSTALL_DATA) $(srcdir)/led_blue_on.png $(kde_datadir)/konversation/images/led_blue_on.png
	$(mkinstalldirs) $(kde_datadir)/konversation/images/
	$(INSTALL_DATA) $(srcdir)/led_green_off.png $(kde_datadir)/konversation/images/led_green_off.png
	$(mkinstalldirs) $(kde_datadir)/konversation/images/
	$(INSTALL_DATA) $(srcdir)/led_green_on.png $(kde_datadir)/konversation/images/led_green_on.png
	$(mkinstalldirs) $(kde_datadir)/konversation/images/
	$(INSTALL_DATA) $(srcdir)/led_red_off.png $(kde_datadir)/konversation/images/led_red_off.png
	$(mkinstalldirs) $(kde_datadir)/konversation/images/
	$(INSTALL_DATA) $(srcdir)/led_red_on.png $(kde_datadir)/konversation/images/led_red_on.png
	$(mkinstalldirs) $(kde_datadir)/konversation/images/
	$(INSTALL_DATA) $(srcdir)/led_yellow_off.png $(kde_datadir)/konversation/images/led_yellow_off.png
	$(mkinstalldirs) $(kde_datadir)/konversation/images/
	$(INSTALL_DATA) $(srcdir)/led_yellow_on.png $(kde_datadir)/konversation/images/led_yellow_on.png
	$(mkinstalldirs) $(kde_datadir)/konversation/images/
	$(INSTALL_DATA) $(srcdir)/close_pane.png $(kde_datadir)/konversation/images/close_pane.png
	$(mkinstalldirs) $(kde_appsdir)/Internet/
	$(INSTALL_DATA) $(srcdir)/konversation.desktop $(kde_appsdir)/Internet/konversation.desktop
	$(mkinstalldirs) $(kde_icondir)/locolor/16x16/apps/
	$(INSTALL_DATA) $(srcdir)/app_icon_16.png $(kde_icondir)/locolor/16x16/apps/konversation.png
	$(mkinstalldirs) $(kde_icondir)/locolor/32x32/apps/
	$(INSTALL_DATA) $(srcdir)/app_icon_32.png $(kde_icondir)/locolor/32x32/apps/konversation.png

uninstall-local:
	-rm -f $(kde_datadir)/konversation/images/led_blue_off.png
	-rm -f $(kde_datadir)/konversation/images/led_blue_on.png
	-rm -f $(kde_datadir)/konversation/images/led_green_off.png
	-rm -f $(kde_datadir)/konversation/images/led_green_on.png
	-rm -f $(kde_datadir)/konversation/images/led_red_off.png
	-rm -f $(kde_datadir)/konversation/images/led_red_on.png
	-rm -f $(kde_datadir)/konversation/images/led_yellow_off.png
	-rm -f $(kde_datadir)/konversation/images/led_yellow_on.png
	-rm -f $(kde_datadir)/konversation/images/close_pane.png
	-rm -f $(kde_appsdir)/Internet/konversation.desktop
	-rm -f $(kde_icondir)/locolor/16x16/apps/konversation.png
	-rm -f $(kde_icondir)/locolor/32x32/apps/konversation.png

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

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/konversation.pot; \
	fi

