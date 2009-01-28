
include_directories(${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


########### install files ###############

install(FILES  bug fortune gauge uptime kdeversion cmd sayclip weather sysinfo media mail tinyurl fortunes.dat DESTINATION ${DATA_INSTALL_DIR}/konversation/scripts)



#original Makefile.am contents follow:

#scriptsdir=$(kde_datadir)/konversation/scripts
#scripts_SCRIPTS=bug fortune gauge uptime kdeversion cmd sayclip weather sysinfo media mail tinyurl
#scripts_DATA=fortunes.dat
#
