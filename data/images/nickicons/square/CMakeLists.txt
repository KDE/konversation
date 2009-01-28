
include_directories(${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


########### install files ###############

install(FILES  irc_admin.png irc_away.png irc_halfop.png irc_normal.png irc_op.png  			irc_owner.png irc_voice.png index.desktop DESTINATION ${DATA_INSTALL_DIR}/konversation/themes/square)



#original Makefile.am contents follow:

#konversationicondir = $(kde_datadir)/konversation/themes/square
#konversationicon_DATA = irc_admin.png irc_away.png irc_halfop.png irc_normal.png irc_op.png \
#			irc_owner.png irc_voice.png index.desktop
