INCLUDE (FindPackageHandleStandardArgs)

FIND_PACKAGE (Threads REQUIRED)

FIND_PATH (
  GOOGLE_MOCKS_INCLUDE_DIRS
  NAME gmock-actions.h
  DOC "Directory containing gmock-actions.h"
  PATH
    /usr/include
    /usr/local/include
    /opt/local/include
  PATH_SUFFIXES gmock
)

FIND_LIBRARY (
  GOOGLE_MOCKS_LIBRARY
  NAMES gmock gmock_main
  DOC "Path to the Google Mocks library"
  PATHS
    /usr/lib
    /usr/local/lib
    /opt/local/lib
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS (GoogleMocks REQUIRED_VARS GOOGLE_MOCKS_LIBRARY GOOGLE_MOCKS_INCLUDE_DIRS)

SET (GOOGLE_MOCKS_LIBRARIES ${GOOGLE_MOCKS_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

MARK_AS_ADVANCED (GOOGLE_MOCKS_LIBRARIES GOOGLE_MOCKS_INCLUDE_DIRS)
