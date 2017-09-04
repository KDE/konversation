set(themestructure_SRCS
    themestructure.cpp
)

add_library(konversation_packagestructure_uipackage MODULE ${themestructure_SRCS})

target_link_libraries(konversation_packagestructure_uipackage
   KF5::I18n
   KF5::Package
)

install(TARGETS konversation_packagestructure_uipackage DESTINATION ${KDE_INSTALL_PLUGINDIR}/kpackage/packagestructure)

install(FILES konversation-uipackage.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})
