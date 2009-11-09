INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/kftpgrabber/src/ftp
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${KDE4_INCLUDE_DIR}
	${QT_INCLUDES}
)


########### next target ###############

SET(kftpinterfaces_LIB_SRCS
kftpbookmarkimportplugin.cpp
)


kde4_add_library(kftpinterfaces SHARED ${kftpinterfaces_LIB_SRCS})

target_link_libraries(kftpinterfaces ${KDE4_KPARTS_LIBS} ${QT_QTCORE_LIBRARY})
set_target_properties(kftpinterfaces PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

install(TARGETS kftpinterfaces ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install(FILES kftpbookmarkimportplugin.desktop DESTINATION ${SERVICES_INSTALL_DIR})

