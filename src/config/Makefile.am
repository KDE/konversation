INCLUDES = -I.. -I$(top_builddir)/konversation/src $(all_includes)
METASOURCES = AUTO

noinst_LTLIBRARIES = libkonversationconfig.la
libkonversationconfig_la_SOURCES = preferences_base.kcfgc preferences.cpp
libkonversationconfig_la_LDFLAGS = $(all_libraries) -no-undefined
libkonversationconfig_la_LIBADD = $(LIB_KDEUI)

kde_kcfg_DATA = konversation.kcfg
