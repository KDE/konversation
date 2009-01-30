
include_directories(${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )

########### next target ###############

set(linkaddressbookui_STAT_SRCS
    linkaddressbookui.cpp
    addressbook_base.cpp
    #addressbook.cpp
    #nicklisttooltip.cpp
    #nicksonlinetooltip.cpp
    addresseeitem.cpp)

kde4_add_ui3_files(linkaddressbookui_STAT_SRCS linkaddressbookui_base.ui)

message(STATUS "DCOP has been removed in KDE 4; port code to D-Bus. kde4_add_dcop_skels macro does not exist anymore. We keep it just to remember to port to D-Bus.")
#kde4_add_dcop_skels(linkaddressbookui_STAT_SRCS
#kimiface.h )

kde4_add_library(linkaddressbookui STATIC ${linkaddressbookui_STAT_SRCS})
