if compile_LIBKONVERSATION_COMPAT
COMPAT_INCLUDE=-I../compat
endif

METASOURCES = AUTO

AM_CPPFLAGS = $(COMPAT_INCLUDE) $(all_includes)

noinst_LTLIBRARIES = liblinkaddressbookui.la
noinst_HEADERS = linkaddressbookui.h kimiface.h

liblinkaddressbookui_la_SOURCES = linkaddressbookui_base.ui linkaddressbookui.cpp addressbook_base.cpp addressbook.cpp nicklisttooltip.cpp nicksonlinetooltip.cpp kimiface.skel addresseeitem.cpp
kimiface_DIR = $(kde_includes)
liblinkaddressbookui_la_LDFLAGS = $(all_libraries) -no-undefined
liblinkaddressbookui_la_LIBADD = $(LIB_KDEUI) $(LIB_KABC)
# vim: set noet:
