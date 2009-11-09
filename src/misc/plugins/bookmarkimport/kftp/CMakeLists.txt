INCLUDE_DIRECTORIES(
	../../../interfaces
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${KDE4_INCLUDE_DIR}
	${QT_INCLUDES}
)

########### next target ###############

SET(kftpimportplugin_kftp_PART_SRCS
kftpimportkftpplugin.cpp
)

kde4_add_plugin(kftpimportplugin_kftp ${kftpimportplugin_kftp_PART_SRCS})

TARGET_LINK_LIBRARIES(kftpimportplugin_kftp ${KDE4_KPARTS_LIBS} kftpinterfaces)

install(TARGETS kftpimportplugin_kftp DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install(FILES kftpimportplugin_kftp.desktop DESTINATION ${SERVICES_INSTALL_DIR})

