install(FILES qmldir DESTINATION ${QML_INSTALL_DIR}/org/kde/konversation/uicomponents)

FILE(GLOB qmlfiles "${CMAKE_CURRENT_SOURCE_DIR}/*.qml")
install(FILES ${qmlfiles} DESTINATION ${QML_INSTALL_DIR}/org/kde/konversation/uicomponents)
