
add_subdirectory(images)
add_subdirectory(scripts)
add_subdirectory(scripting_support)

ecm_generate_dbus_service_file(
    NAME org.kde.konversation
    EXECUTABLE ${KDE_INSTALL_FULL_BINDIR}/konversation
    DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR}
)

########### install files ###############

install(PROGRAMS   org.kde.konversation.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES   konversation.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
install(FILES   updaters/konversation.upd DESTINATION ${KDE_INSTALL_DATADIR}/kconf_update)
install(PROGRAMS updaters/konversation-0.19-colors.pl
                updaters/konversation-0.19-sortorder.pl
                updaters/konversation-0.19-appearance.pl
                updaters/konversation-0.19-tabplacement.pl
                updaters/konversation-0.19-custombrowser.pl
                updaters/konversation-0.19-colorcodes.pl
                updaters/konversation-0.19-notifylists.pl
                updaters/konversation-0.20-quickbuttons.pl
                updaters/konversation-0.20-customfonts.pl
                DESTINATION ${KDE_INSTALL_DATADIR}/kconf_update)
install(FILES org.kde.konversation.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES konversation_nicklist_theme.knsrc  DESTINATION ${KDE_INSTALL_KNSRCDIR})
