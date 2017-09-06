
add_subdirectory(images)
add_subdirectory(scripts)
add_subdirectory(scripting_support)


########### install files ###############

install(PROGRAMS   org.kde.konversation.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES   konversationui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/konversation)
install(FILES   konversation.notifyrc DESTINATION ${KNOTIFYRC_INSTALL_DIR})
install(FILES   updaters/konversation.upd DESTINATION ${DATA_INSTALL_DIR}/kconf_update)
install(PROGRAMS updaters/konversation-0.19-colors.pl
                updaters/konversation-0.19-sortorder.pl
                updaters/konversation-0.19-appearance.pl
                updaters/konversation-0.19-tabplacement.pl
                updaters/konversation-0.19-custombrowser.pl
                updaters/konversation-0.19-colorcodes.pl
                updaters/konversation-0.19-notifylists.pl
                updaters/konversation-0.20-quickbuttons.pl
                updaters/konversation-0.20-customfonts.pl
                DESTINATION ${DATA_INSTALL_DIR}/kconf_update)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.konversation.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

install(PROGRAMS konvi2x DESTINATION bin)
