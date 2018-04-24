add_subdirectory(uicomponents)

if (NOT ${USE_QRC})
    add_subdirectory(uipackages)
    add_subdirectory(kpackage_packagestructure_konversation_uipackage)
endif()

