CMAKE_MINIMUM_REQUIRED (VERSION 2.8)
PROJECT (cardano-c C)

SET(PROJECT_NAME "cardano-c")

# Option that the user can optionally select
OPTION (TESTING_ENABLED "Enables unit test build." ON)
OPTION (DOXYGEN_ENABLED "Build documentation" OFF)
OPTION (EXAMPLES_ENABLED "Build examples" OFF)

# Find external dependencies
LIST (APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Get the target architecture platform.
INCLUDE (CMakeInstallDirs)
INCLUDE (TargetArch)

TARGET_ARCHITECTURE (ARCHITECTURE ARCHITECTURE_ALTERNATIVE_NAME)

# Set common variables.
IF ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  SET (BUILD_DIR build/debug)
ELSE ()
  SET (BUILD_DIR build/release)
ENDIF ()

SET (CARDANO_C_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/${BUILD_DIR})

IF (DEFINED VERSION_MAJOR)
  SET (CARDANO_C_VERSION_MAJOR ${VERSION_MAJOR})
ELSE ()
  SET (CARDANO_C_VERSION_MAJOR 0)
ENDIF ()

IF (DEFINED VERSION_MINOR)
  SET (CARDANO_C_VERSION_MINOR ${VERSION_MINOR})
ELSE ()
  SET (CARDANO_C_VERSION_MINOR 0)
ENDIF ()

IF (DEFINED VERSION_PATCH)
  SET (CARDANO_C_VERSION_PATCH ${VERSION_PATCH})
ELSE ()
  SET (CARDANO_C_VERSION_PATCH 0)
ENDIF ()

IF (DEFINED COLLECTION_GROW_FACTOR )
    SET (CARDANO_C_COLLECTION_GROW_FACTOR  ${COLLECTION_GROW_FACTOR})
ELSE ()
    SET (CARDANO_C_COLLECTION_GROW_FACTOR 1.5) # See http://groups.google.com/group/comp.lang.c++.moderated/msg/ba558b4924758e2e
ENDIF ()

SET (CARDANO_C_VERSION "${CARDANO_C_VERSION_MAJOR}.${CARDANO_C_VERSION_MINOR}.${CARDANO_C_VERSION_PATCH}")

FIND_PROGRAM (CLANG_TIDY_EXE NAMES "clang-tidy")

if(CLANG_TIDY_EXE)
    SET (CMAKE_C_CLANG_TIDY "${CLANG_TIDY_EXE}")
endif()

# Print variables to stdout.
MESSAGE ( STATUS "################################################################" )
MESSAGE ( STATUS "Building Cardano C v${CARDANO_C_VERSION}" )
MESSAGE ( STATUS )
MESSAGE ( STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}" )
MESSAGE ( STATUS "CMAKE_BUILD_TYPE     = ${CMAKE_BUILD_TYPE}" )
MESSAGE ( STATUS "CMAKE_PREFIX_PATH    = ${CMAKE_PREFIX_PATH}" )
MESSAGE ( STATUS "ARCHITECTURE         = ${ARCHITECTURE}" )
MESSAGE ( STATUS "TESTING ENABLED      = ${TESTING_ENABLED}")
MESSAGE ( STATUS "DOXYGEN ENABLED      = ${DOXYGEN_ENABLED}")
MESSAGE ( STATUS "EXAMPLES ENABLED     = ${EXAMPLES_ENABLED}")
MESSAGE ( STATUS "CMAKE_C_CLANG_TIDY   = ${CMAKE_C_CLANG_TIDY}")
MESSAGE ( STATUS )
MESSAGE ( STATUS "change a configuration variable with: cmake -D<Variable>=<Value>" )
MESSAGE ( STATUS "################################################################" )

# Find external dependencies.
MESSAGE ( STATUS )
MESSAGE ( STATUS "Configuring basic solution settings:" )
MESSAGE ( STATUS )
INCLUDE (CMakeInstallDirs)
INCLUDE (ProcessDebugInformation)

# Enable unit test build
IF (TESTING_ENABLED)
    INCLUDE (CTest)
    ENABLE_TESTING ()

    FIND_PROGRAM (CTEST_MEMORYCHECK_COMMAND NAMES valgrind)
    SET (MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full")
ENDIF ()

# Find external dependencies.
MESSAGE ( STATUS )
MESSAGE ( STATUS "Configuring basic solution settings:" )
MESSAGE ( STATUS )

# Remove build directory on clean target.
SET_DIRECTORY_PROPERTIES (PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${PROJECT_BINARY_DIR}/${BUILD_DIR})
SET (CARDANO_C_DATA_DIR "${CMAKE_INSTALL_DATADIR}/cardano-c")

# Build documentation.
IF (DOXYGEN_ENABLED)
    IF (COMMAND FIND_HOST_PACKAGE)
        FIND_HOST_PACKAGE (Doxygen)
        FIND_HOST_PACKAGE (Sphinx)
        FIND_HOST_PACKAGE(Python3 REQUIRED COMPONENTS Interpreter)
    ELSE ()
        FIND_PACKAGE (Doxygen)
        FIND_PACKAGE (Sphinx)
        FIND_PACKAGE(Python3 REQUIRED COMPONENTS Interpreter)
    ENDIF ()

    # Configure tools
    IF (DOXYGEN_FOUND AND DOXYGEN_ENABLED AND SPHINX_FOUND)
        SET (DOXYGEN_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/doc")
        SET (DOXYGEN_BUILD_DIRECTORY   "${PROJECT_BINARY_DIR}/${BUILD_DIR}/doc")
        SET (SPHINX_SOURCE "${DOXYGEN_BUILD_DIRECTORY}/")
        SET (SPHINX_BUILD "./html")

        CONFIGURE_FILE ("${CMAKE_CURRENT_SOURCE_DIR}/doc/doxyfile.in" ${DOXYGEN_BUILD_DIRECTORY}/doxyfile @ONLY)
        CONFIGURE_FILE ("${CMAKE_CURRENT_SOURCE_DIR}/doc/sphinx/conf.py.in" ${DOXYGEN_BUILD_DIRECTORY}/conf.py @ONLY)

        FILE (COPY doc/src/_static DESTINATION ${DOXYGEN_BUILD_DIRECTORY}/)
        FILE (COPY doc/src/_templates DESTINATION ${DOXYGEN_BUILD_DIRECTORY}/)
        FILE (COPY ${CMAKE_CURRENT_SOURCE_DIR}/doc/sphinx/Makefile DESTINATION ${DOXYGEN_BUILD_DIRECTORY}/)
        FILE (COPY ${CMAKE_CURRENT_SOURCE_DIR}/doc/sphinx/make.bat DESTINATION ${DOXYGEN_BUILD_DIRECTORY}/)
        FILE (COPY doc/src/sections/ DESTINATION ${DOXYGEN_BUILD_DIRECTORY}/)

        # Add custom target for Doxygen
        ADD_CUSTOM_TARGET(doc_doxygen
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_BUILD_DIRECTORY}/doxyfile
                WORKING_DIRECTORY ${DOXYGEN_BUILD_DIRECTORY}
                COMMENT "Generating API documentation with Doxygen"
        )

        # Add custom target for Sphinx that depends on Doxygen
        ADD_CUSTOM_TARGET(doc_sphinx
                COMMAND sphinx-build -b html ${SPHINX_SOURCE} ${SPHINX_BUILD}
                WORKING_DIRECTORY ${DOXYGEN_BUILD_DIRECTORY}
                COMMENT "Generating documentation with Sphinx"
                DEPENDS doc_doxygen # Ensure Sphinx runs after Doxygen
        )

        # Add a custom target that builds both Doxygen and Sphinx documentation
        ADD_CUSTOM_TARGET(doc
                DEPENDS doc_doxygen doc_sphinx
        )
    ENDIF ()
ENDIF ()

SET (CARDANO_C_LIBRARY ${CMAKE_SOURCE_DIR}/${PROJECT_BINARY_DIR}/${BUILD_DIR}/lib/libcardano-c.so)
SET (CARDANO_C_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/lib/include")

# Build libraries.
INCLUDE_DIRECTORIES(SYSTEM ./lib)

ADD_SUBDIRECTORY (lib ${BUILD_DIR}/lib)

IF (CMAKE_BUILD_TYPE STREQUAL "Fuzz")
    ADD_SUBDIRECTORY (fuzz ${BUILD_DIR}/fuzz)
ENDIF ()

# Build examples
IF (EXAMPLES_ENABLED)
    ADD_SUBDIRECTORY (examples ${BUILD_DIR}/examples)
ENDIF ()

# Packaging.
SET (CPACK_SOURCE_IGNORE_FILES ".git,.swp")
SET (CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)

SET (CPACK_PACKAGE_DESCRIPTION_SUMMARY "A C library for interacting with the Cardano blockchain.")
SET (CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/COPYING)
SET (CPACK_PACKAGE_VENDOR "Biglup.")
SET (CPACK_PACKAGE_VERSION ${CARDANO_C_VERSION})
SET (CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${CPACK_PACKAGE_VERSION}-${ARCHITECTURE_ALTERNATIVE_NAME}")

# Debian.
SET (CPACK_SOURCE_GENERATOR DEB)
SET (CPACK_DEB_COMPONENT_INSTALL ON)
SET (CPACK_DEBIAN_COMPRESSION_TYPE "bzip2")
SET (CPACK_DEBIAN_PACKAGE_MAINTAINER "Angel Castillo <angel.castillob@protonmail.com>")
SET (CPACK_DEBIAN_DEV_PACKAGE_DEPENDS "libcardano-c-${CARDANO_C_VERSION}")

IF (ARCHITECTURE)
  SET (CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${ARCHITECTURE})
ENDIF ()

INCLUDE (CPack)
