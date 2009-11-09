ADD_SUBDIRECTORY( misc ) 
ADD_SUBDIRECTORY( engine ) 
ADD_SUBDIRECTORY( ui ) 
ADD_SUBDIRECTORY( widgets ) 

include_directories(
	misc
	widgets
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${KDE4_INCLUDE_DIR}
	${QT_INCLUDES}
)


########### next target ###############

SET(kftpgrabber_SRCS
main.cpp
mainwindow.cpp
checksumverifier.cpp
kftpbookmarks.cpp
kftpqueue.cpp
kftpqueueprocessor.cpp
kftpsession.cpp
kftpqueueconverter.cpp
kftptransfer.cpp
kftptransferfile.cpp
kftptransferdir.cpp
fileexistsactions.cpp
statistics.cpp
site.cpp
queueobject.cpp
queuegroup.cpp
directoryscanner.cpp
)


kde4_add_executable(kftpgrabber ${kftpgrabber_SRCS})

# bookmarkwidgets 
target_link_libraries(kftpgrabber ${KDE4_KDECORE_LIBS} ${KDE4_KDNSSD_LIBRARY} ${KDE4_KDE3SUPPORT_LIBRARY} ${KDE4_KUTILS_LIBRARY} ${KDE4_KFILE_LIBRARY} ${LIBSSH2_LIBRARY})
target_link_libraries(kftpgrabber ssl browser bookmarkwidgets queueviewwidget failedtransferswidget widgets engine misc customcommands ui kftpinterfaces)

install(TARGETS kftpgrabber ${INSTALL_TARGETS_DEFAULT_ARGS})


########### install files ###############

install(FILES kftpgrabber.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES kftpgrabberui.rc kftpgrabber-logo.png kftpgrabber-bi-wizard.png DESTINATION ${DATA_INSTALL_DIR}/kftpgrabber)

kde4_install_icons( ${ICON_INSTALL_DIR} )

