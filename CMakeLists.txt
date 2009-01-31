#This isn't Konversation until it works.
project(konvi)

#paranoid?
cmake_minimum_required(VERSION 2.6.0)

set(KDE_MIN_VERSION "4.00.82")

#atm this gets is FindQt4 and FindX11 (provided that the default cmake FindQt4 doesn't get used)
find_package(KDE4 REQUIRED)
find_package(KdepimLibs REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(CheckIncludeFile)


add_definitions (-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)
add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

remove_definitions(-DKDE_DEPRECATED_WARNINGS)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES})


#looks like the original test is from Kopete, their port doesn't use Xext so neither shall ours
if (X11_Xscreensaver_FOUND)
    set(HAVE_XSCREENSAVER 1)
    message(STATUS "${PROJECT_NAME} -- ${X11_Xscreensaver_FOUND}")
endif (X11_Xscreensaver_FOUND)


configure_file(config-konvi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-konvi.h )



add_subdirectory(src)
add_subdirectory(data)


########### install files ###############


#original Makefile.am contents follow:

#SUBDIRS = src images scripts 
#EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL README TODO
#
#messages: rc.cpp
#	$(EXTRACTRC) `find . -name "*.rc" -o -name "*.ui" -o -name "*.kcfg"` > rc.cpp
## we haven't got a tips file atm, so there isn't a lot of point in making one...
## maybe someone will document the process of writing tips files, or tell me that
## it needs to be done
##	$(PREPARETIPS) > tips.cpp
#	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name *.cxx -o -name \*.ecpp -o -name \*.C`; \
#	if test -n "$$LIST"; then \
#		$(XGETTEXT) $$LIST -o $(podir)/konversation.pot; \
#	fi
##	rm -f tips.cpp
#	rm -f rc.cpp
#
#
#include $(top_srcdir)/admin/Doxyfile.am
#
