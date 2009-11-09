# - Try to find the libssh2 library
# Once done this will define
#
# LIBSSH2_FOUND - system has the libssh2 library
# LIBSSH2_INCLUDE_DIR - the libssh2 include directory
# LIBSSH2_LIBRARY - the libssh2 library name

if (LIBSSH2_INCLUDE_DIR AND LIBSSH2_LIBRARY)
  set(LibSSH2_FIND_QUIETLY TRUE)
endif (LIBSSH2_INCLUDE_DIR AND LIBSSH2_LIBRARY)

FIND_PATH(LIBSSH2_INCLUDE_DIR libssh2.h
)

FIND_LIBRARY(LIBSSH2_LIBRARY NAMES ssh2
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibSSH2 DEFAULT_MSG LIBSSH2_INCLUDE_DIR LIBSSH2_LIBRARY )

MARK_AS_ADVANCED(LIBSSH2_INCLUDE_DIR LIBSSH2_LIBRARY)