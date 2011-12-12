install(PROGRAMS
    bug
    cmd
    fortune
    gauge
    mail
    media
    sayclip
    sysinfo
    tinyurl
    uptime
    DESTINATION ${DATA_INSTALL_DIR}/konversation/scripts)

install(FILES
    fortunes.dat
    DESTINATION ${DATA_INSTALL_DIR}/konversation/scripts)
