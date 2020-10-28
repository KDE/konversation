install(PROGRAMS
    bug
    cmd
    fortune
    gauge
    media
    sayclip
    sysinfo
    tinyurl
    uptime
    DESTINATION ${KDE_INSTALL_DATADIR}/konversation/scripts)

install(FILES
    fortunes.dat
    DESTINATION ${KDE_INSTALL_DATADIR}/konversation/scripts)
