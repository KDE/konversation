METASOURCES = AUTO

AM_CPPFLAGS = $(all_includes)

noinst_LTLIBRARIES = liblinkaddressbookui.la
noinst_HEADERS = linkaddressbookui.h kimiface.h

liblinkaddressbookui_la_SOURCES = linkaddressbookui_base.ui linkaddressbookui.cpp addressbook.cpp nicklisttooltip.cpp kimiface.skel
kimiface_DIR = $(kde_includes)
liblinkaddressbookui_la_LDFLAGS = $(all_libraries) -no-undefined
liblinkaddressbookui_la_LIBADD = $(LIB_KDEUI) $(LIB_KABC)
INCLUDES=-DUSE_NICKINFO
# vim: set noet:
