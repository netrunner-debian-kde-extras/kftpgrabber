INCLUDE_DIRECTORIES(
	..
	../..
	../../misc
	${CMAKE_CURRENT_BINARY_DIR}/../..
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${KDE4_INCLUDE_DIR}
	${QT_INCLUDES}
)

########### next target ###############

SET(failedtransferswidget_SRCS
model.cpp
view.cpp
)


kde4_add_library(failedtransferswidget STATIC ${failedtransferswidget_SRCS})

add_dependencies(failedtransferswidget widgets)

