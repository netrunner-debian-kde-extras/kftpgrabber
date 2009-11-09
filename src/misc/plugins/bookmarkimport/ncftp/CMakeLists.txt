INCLUDE_DIRECTORIES(
	../../../interfaces
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${KDE4_INCLUDE_DIR}
	${QT_INCLUDES}
)

########### next target ###############

SET(kftpimportplugin_ncftp_PART_SRCS
kftpimportncftpplugin.cpp
)

kde4_add_plugin(kftpimportplugin_ncftp ${kftpimportplugin_ncftp_PART_SRCS})

target_link_libraries(kftpimportplugin_ncftp ${KDE4_KPARTS_LIBS} kftpinterfaces)

install(TARGETS kftpimportplugin_ncftp DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install(FILES kftpimportplugin_ncftp.desktop DESTINATION ${SERVICES_INSTALL_DIR})

