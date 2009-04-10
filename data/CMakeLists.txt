
add_subdirectory(images)
add_subdirectory(scripts)


########### install files ###############

install(FILES   konversation.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES   konversationui.rc konversation.notifyrc DESTINATION
	${DATA_INSTALL_DIR}/konversation)
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
install(FILES  konvirc.protocol konvirc6.protocol DESTINATION ${SERVICES_INSTALL_DIR})
