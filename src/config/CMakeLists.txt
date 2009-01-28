
include_directories(${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


########### next target ###############


########### install files ###############

install(FILES  konversation.kcfg DESTINATION ${KCFG_INSTALL_DIR})



#original Makefile.am contents follow:

#INCLUDES = -I.. -I$(srcdir)/.. $(all_includes)
#METASOURCES = AUTO
#
#noinst_LTLIBRARIES = libkonversationconfig.la
#libkonversationconfig_la_SOURCES = preferences_base.kcfgc preferences.cpp
#libkonversationconfig_la_LDFLAGS = $(all_libraries) -no-undefined
#
#kde_kcfg_DATA = konversation.kcfg
