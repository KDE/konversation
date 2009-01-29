
add_subdirectory(images)
add_subdirectory(scripts)


########### install files ###############

install(FILES   konvi.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES   konviui.rc eventsrc DESTINATION ${DATA_INSTALL_DIR}/konvi)
install(FILES   konvi.upd
                konvi-0.19-colors.pl
                konvi-0.19-sortorder.pl
                konvi-0.19-appearance.pl
                konvi-0.19-tabplacement.pl
                konvi-0.19-custombrowser.pl
                konvi-0.19-colorcodes.pl
                konvi-0.19-notifylists.pl
                konvi-0.20-quickbuttons.pl
                konvi-0.20-customfonts.pl
                DESTINATION ${DATA_INSTALL_DIR}/kconf_update)
install(FILES  konvirc.protocol konvirc6.protocol DESTINATION ${SERVICES_INSTALL_DIR})
