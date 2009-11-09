INCLUDE_DIRECTORIES(
	../../../interfaces
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${KDE4_INCLUDE_DIR}
	${QT_INCLUDES}
)

########### next target ###############

SET(kftpimportplugin_filezilla3_PART_SRCS
kftpimportfz3plugin.cpp
)

kde4_add_plugin(kftpimportplugin_filezilla3 ${kftpimportplugin_filezilla3_PART_SRCS})

target_link_libraries(kftpimportplugin_filezilla3 ${KDE4_KPARTS_LIBS} kftpinterfaces)

install(TARGETS kftpimportplugin_filezilla3 DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install(FILES kftpimportplugin_filezilla3.desktop DESTINATION ${SERVICES_INSTALL_DIR})

