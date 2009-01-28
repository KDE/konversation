
include_directories(${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


########### next target ###############


########### install files ###############




#original Makefile.am contents follow:

#METASOURCES = AUTO
#
#INCLUDES = -I$(srcdir)/.. -I.. -I$(srcdir)/../config -I../config
#AM_CPPFLAGS = $(all_includes) -I$(top_builddir)/konversation/src
#
#noinst_LTLIBRARIES = liblinkaddressbookui.la
#noinst_HEADERS = linkaddressbookui.h kimiface.h
#
#liblinkaddressbookui_la_SOURCES = linkaddressbookui_base.ui linkaddressbookui.cpp addressbook_base.cpp addressbook.cpp nicklisttooltip.cpp nicksonlinetooltip.cpp kimiface.skel addresseeitem.cpp
#kimiface_DIR = $(kde_includes)
#liblinkaddressbookui_la_LDFLAGS = $(all_libraries) -no-undefined
#
## vim: set noet:
