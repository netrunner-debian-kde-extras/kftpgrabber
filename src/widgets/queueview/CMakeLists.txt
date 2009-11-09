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

SET(queueviewwidget_SRCS
model.cpp
delegate.cpp
treeview.cpp
#queueeditor.cpp
queueview.cpp
threadview.cpp
)


kde4_add_library(queueviewwidget STATIC ${queueviewwidget_SRCS})

add_dependencies(queueviewwidget widgets)

