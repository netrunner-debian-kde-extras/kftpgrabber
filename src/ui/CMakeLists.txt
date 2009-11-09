INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${KDE4_INCLUDE_DIR}
	${QT_INCLUDES}
)

########### next target ###############

SET(ui_SRCS
foobar.cpp
)


SET( ui_UI
kftpfilteraddpatternlayout.ui
quickconnect.ui
kftpqueueeditorlayout.ui
kftpsearchlayout.ui
kftpbookmarkimportlayout.ui
config_general.ui
config_transfers.ui
config_log.ui
config_display.ui
config_filters.ui
checksum_verifier.ui
)

KDE4_ADD_UI_FILES(ui_SRCS ${ui_UI} )

KDE4_ADD_LIBRARY(ui STATIC ${ui_SRCS})
