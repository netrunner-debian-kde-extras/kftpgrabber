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

SET(browser_SRCS
dirlister.cpp
dirmodel.cpp
dirsortfilterproxymodel.cpp
locationnavigator.cpp
detailsview.cpp
view.cpp
actions.cpp
treeview.cpp
#propsplugin.cpp
#filterwidget.cpp
)


kde4_add_library(browser STATIC ${browser_SRCS})

add_dependencies(browser widgets bookmarkwidgets)

