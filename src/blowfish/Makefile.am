AM_CPPFLAGS = $(all_includes) -I$(srcdir)/../
#KDE_CXXFLAGS = -w

noinst_LTLIBRARIES = libblowfish.la
noinst_HEADERS = b64stuff.h  BlowfishCbc.h  blowfish.h  mc_blowfish.h  newblowfish.h  oldblowfish1.h  oldblowfish2.h

libblowfish_la_SOURCES = b64stuff.cpp BlowfishCbc.cpp blowfish.cpp mc_blowfish.cpp newblowfish.cpp oldblowfish.cpp
libblowfish_la_LDFLAGS = $(all_libraries) -no-undefined
