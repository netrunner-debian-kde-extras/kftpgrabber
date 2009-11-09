ADD_SUBDIRECTORY( queueview ) 
ADD_SUBDIRECTORY( browser ) 
ADD_SUBDIRECTORY( bookmarks ) 
ADD_SUBDIRECTORY( failedtransfers )

INCLUDE_DIRECTORIES(
	..
	../misc
	${CMAKE_CURRENT_BINARY_DIR}/..
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${KDE4_INCLUDE_DIR}
	${QT_INCLUDES}
)

########### next target ###############

SET(widgets_SRCS
logview.cpp
#kftpselectserverdialog.cpp
#kftpselectserverdialog.h
#kftpserverlineedit.cpp
#kftpserverlineedit.h
listview.cpp
systemtray.cpp
#searchdialog.cpp
zeroconflistview.cpp
trafficgraph.cpp
listviewitem.cpp
configdialog.cpp
configfilter.cpp
verifier.cpp
filtereditor.cpp
widgetlister.cpp
overlaywidget.cpp
popupmessage.cpp
sslerrorsdialog.cpp
fingerprintverifydialog.cpp
)

SET(widgets_UI
sslerrorsdialog.ui
fingerprintverifydialog.ui
)

kde4_add_ui_files(widgets_SRCS ${widgets_UI})

kde4_add_library(widgets STATIC ${widgets_SRCS})

add_dependencies(widgets misc engine ui)

