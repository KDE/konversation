METASOURCES = AUTO

AM_CPPFLAGS = $(all_includes)

noinst_LTLIBRARIES = liblinkaddressbookui.la
noinst_HEADERS = linkaddressbookui.h

liblinkaddressbookui_la_SOURCES = linkaddressbookui_base.ui linkaddressbookui.cpp addressbook.cpp
liblinkaddressbookui_la_LDFLAGS = $(all_libraries) -no-undefined
liblinkaddressbookui_la_LIBADD = $(LIB_KDEUI) $(LIB_KABC)

# vim: set noet:
