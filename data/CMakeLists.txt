
add_subdirectory(images)
add_subdirectory(scripts)


########### install files ###############

install(FILES   konversation.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES   konversationui.rc konversation.notifyrc DESTINATION
	${DATA_INSTALL_DIR}/konversation)
#install(FILES   updaters/konvi.upd
#                updaters/konvi-0.19-colors.pl
#                updaters/konvi-0.19-sortorder.pl
#                updaters/konvi-0.19-appearance.pl
#                updaters/konvi-0.19-tabplacement.pl
#                updaters/konvi-0.19-custombrowser.pl
#                updaters/konvi-0.19-colorcodes.pl
#                updaters/konvi-0.19-notifylists.pl
#                updaters/konvi-0.20-quickbuttons.pl
#                updaters/konvi-0.20-customfonts.pl
#                DESTINATION ${DATA_INSTALL_DIR}/kconf_update)
install(FILES  konvirc.protocol konvirc6.protocol DESTINATION ${SERVICES_INSTALL_DIR})
