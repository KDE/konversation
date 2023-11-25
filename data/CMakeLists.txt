
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
install(FILES org.kde.konversation.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES konversation_nicklist_theme.knsrc  DESTINATION ${KDE_INSTALL_KNSRCDIR})
