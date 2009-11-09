INCLUDE_DIRECTORIES(
	..
	../..
	../../misc
	${CMAKE_CURRENT_BINARY_DIR}/../..
	${CMAKE_CURRENT_BINARY_DIR}/..
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${KDE4_INCLUDE_DIR}
	${QT_INCLUDES}
)

########### next target ###############

SET(bookmarkwidgets_SRCS
model.cpp
treeview.cpp
#bookmarksdelegate.cpp
editor.cpp
)

SET(bookmarkwidgets_UI
editor.ui
)

kde4_add_ui_files(bookmarkwidgets_SRCS ${bookmarkwidgets_UI})

kde4_add_library(bookmarkwidgets STATIC ${bookmarkwidgets_SRCS})

add_dependencies(bookmarkwidgets widgets)

