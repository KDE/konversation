METASOURCES = AUTO

INCLUDES = -I$(top_srcdir)/konversation/src -I$(top_builddir)/konversation/src -I$(top_srcdir)/konversation/src/config -I$(top_builddir)/konversation/src/config
AM_CPPFLAGS = $(all_includes) -I$(top_builddir)/konversation/src

noinst_LTLIBRARIES = liblinkaddressbookui.la
noinst_HEADERS = linkaddressbookui.h kimiface.h

liblinkaddressbookui_la_SOURCES = linkaddressbookui_base.ui linkaddressbookui.cpp addressbook_base.cpp addressbook.cpp nicklisttooltip.cpp nicksonlinetooltip.cpp kimiface.skel addresseeitem.cpp
kimiface_DIR = $(kde_includes)
liblinkaddressbookui_la_LDFLAGS = $(all_libraries) -no-undefined
liblinkaddressbookui_la_LIBADD = $(COMPAT_LIBS) $(LIB_KDEUI) $(LIB_KABC)

# vim: set noet:
