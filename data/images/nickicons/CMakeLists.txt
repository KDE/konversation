
include_directories(${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )

add_subdirectory(alternative)
add_subdirectory(christmas)
add_subdirectory(classic)
add_subdirectory(default)
add_subdirectory(smiling)
add_subdirectory(square)
add_subdirectory(oxygen)


########### install files ###############




#original Makefile.am contents follow:

#SUBDIRS = alternative christmas classic default smiling square oxygen
