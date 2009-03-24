install(PROGRAMS
    bug
    cmd
    fortune
    gauge
    kdeversion
    #mail
    media
    sayclip
    sysinfo
    tinyurl
    uptime
    #weather
    DESTINATION ${DATA_INSTALL_DIR}/konversation/scripts)

install(FILES
    fortunes.dat
    DESTINATION ${DATA_INSTALL_DIR}/konversation/scripts)
