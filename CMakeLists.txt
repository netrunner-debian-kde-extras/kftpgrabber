project(KFTPgrabber)

cmake_minimum_required(VERSION 2.6)
find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(CheckIncludeFile)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
                      ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
                      ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules)

find_package(LibSSH2 REQUIRED)

add_definitions(-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS)
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES})

check_include_file(sys/poll.h         HAVE_SYS_POLL_H)
check_include_file(sys/time.h         HAVE_SYS_TIME_H)
check_include_file(openssl/aes.h      HAVE_OPENSSL_AES_H)
check_include_file(openssl/blowfish.h HAVE_OPENSSL_BLOWFISH_H)

check_symbol_exists(gethostbyname     "netdb.h"       HAVE_GETHOSTBYNAME)
check_symbol_exists(gethostbyaddr     "netdb.h"       HAVE_GETHOSTBYADDR)
check_symbol_exists(poll              "poll.h"        HAVE_POLL)
check_symbol_exists(select            "sys/select.h"  HAVE_SELECT)

configure_file(config-kftpgrabber.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kftpgrabber.h)

add_subdirectory( src ) 

