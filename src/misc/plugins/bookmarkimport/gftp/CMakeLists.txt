INCLUDE_DIRECTORIES(
	../../../interfaces
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${KDE4_INCLUDE_DIR}
	${QT_INCLUDES}
)

########### next target ###############

SET(kftpimportplugin_gftp_PART_SRCS
kftpimportgftpplugin.cpp
)


kde4_add_plugin(kftpimportplugin_gftp ${kftpimportplugin_gftp_PART_SRCS})

target_link_libraries(kftpimportplugin_gftp kftpinterfaces ${KDE4_KPARTS_LIBS})

install(TARGETS kftpimportplugin_gftp DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install(FILES kftpimportplugin_gftp.desktop DESTINATION ${SERVICES_INSTALL_DIR})

