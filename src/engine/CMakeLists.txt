include_directories(
	..
	../misc
	${CMAKE_CURRENT_BINARY_DIR}/..
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${KDE4_INCLUDE_DIR}
	${QT_INCLUDES}
)


########### next target ###############

SET(engine_SRCS
settings.cpp
socket.cpp
thread.cpp
directorylisting.cpp
commands.cpp
event.cpp
ftpsocket.cpp
ftpdirectoryparser.cpp
cache.cpp
sftpsocket.cpp
connectionretry.cpp
speedlimiter.cpp
otpgenerator.cpp
)

kde4_add_library(engine STATIC ${engine_SRCS})
target_link_libraries(engine ${LIBSSH2_LIBRARY})

add_dependencies(engine misc)

