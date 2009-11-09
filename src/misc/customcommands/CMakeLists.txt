INCLUDE_DIRECTORIES(
	../..
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${KDE4_INCLUDE_DIR}
	${QT_INCLUDES}
)

########### next target ###############

SET(customcommands_SRCS
handlers.cpp
entry.cpp
manager.cpp
parameterentrydialog.cpp
responsedialog.cpp
)

kde4_add_library(customcommands STATIC ${customcommands_SRCS})


########### install files ###############

install(FILES commands.xml DESTINATION ${DATA_INSTALL_DIR}/kftpgrabber)

