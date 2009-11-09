ADD_SUBDIRECTORY( interfaces ) 
ADD_SUBDIRECTORY( plugins ) 
ADD_SUBDIRECTORY( customcommands ) 

INCLUDE_DIRECTORIES(
	..
	interfaces
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${KDE4_INCLUDE_DIR}
	${QT_INCLUDES}
)

########### next target ###############

SET(misc_SRCS
wallet.cpp
pluginmanager.cpp
zeroconf.cpp
configbase.cpp
filter.cpp
filterwidgethandler.cpp
certificatestore.cpp
)

kde4_add_kcfg_files(misc_SRCS config.kcfgc)

kde4_add_library(misc STATIC ${misc_SRCS})

