METASOURCES = AUTO
AM_CPPFLAGS = -I. $(all_includes)

SUBDIRS = .

noinst_LTLIBRARIES  = libkonversationcompat.la

libkonversationcompat_la_SOURCES = kresolver.cpp kresolvermanager.cpp kresolverworkerbase.cpp \
                ksocketaddress.cpp kresolverstandardworkers.cpp kreverseresolver.cpp \
                ksocketdevice.cpp ksocketbase.cpp kclientsocketbase.cpp kstreamsocket.cpp \
                kserversocket.cpp kdatagramsocket.cpp khttpproxysocketdevice.cpp ksockssocketdevice.cpp \
                kbufferedsocket.cpp ksocketbuffer.cpp netsupp.cpp klistviewsearchline.cpp
libkonversationcompat_la_LDFLAGS = -no-undefined $(LIB_KDECORE) $(all_libraries)
noinst_HEADERS = kresolver.h kreverseresolver.h ksocketaddress.h ksocketbase.h ksocketdevice.h \
                 kclientsocketbase.h kstreamsocket.h kserversocket.h kdatagramsocket.h\
		 kmulticastsocketdevice.h kmulticastsocket.h knetworkinterface.h khttpproxysocketdevice.h\
		 ksockssocketdevice.h kbufferedsocket.h  kresolver_p.h kresolverworkerbase.h\
		 kresolverstandardworkers_p.h ksocketbuffer_p.h syssocket.h netsupp.h klistviewsearchline.h
