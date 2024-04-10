# Tries to find the local libjson-c installation.
#
# On Windows the jsonc_DIR environment variable is used as a default
# hint which can be overridden by setting the corresponding cmake variable.
#
# Once done the following variables will be defined:
#
#   jsonc_FOUND
#   jsonc_INCLUDE_DIR
#   jsonc_LIBRARY_DEBUG
#   jsonc_LIBRARY_RELEASE
#
#
# Furthermore an imported "jsonc" target is created.
#

if (CMAKE_C_COMPILER_ID STREQUAL "GNU"
    OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(_GCC_COMPATIBLE 1)
endif()

# static library option
if (NOT DEFINED jsonc_USE_STATIC_LIBS)
    option(jsonc_USE_STATIC_LIBS "enable to statically link against jsonc" OFF)
endif()
if(NOT (jsonc_USE_STATIC_LIBS EQUAL jsonc_USE_STATIC_LIBS_LAST))
    unset(jsonc_LIBRARY CACHE)
    unset(jsonc_LIBRARY_DEBUG CACHE)
    unset(jsonc_LIBRARY_RELEASE CACHE)
    unset(jsonc_DLL_DEBUG CACHE)
    unset(jsonc_DLL_RELEASE CACHE)
    set(jsonc_USE_STATIC_LIBS_LAST ${jsonc_USE_STATIC_LIBS} CACHE INTERNAL "internal change tracking variable")
endif()


########################################################################
# UNIX
if (UNIX)
    # import pkg-config
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(jsonc_PKG QUIET json-c)
    endif()

    if(jsonc_USE_STATIC_LIBS)
        foreach(_libname ${jsonc_PKG_STATIC_LIBRARIES})
            if (NOT _libname MATCHES "^lib.*\\.a$") # ignore strings already ending with .a
                list(INSERT jsonc_PKG_STATIC_LIBRARIES 0 "lib${_libname}.a")
            endif()
        endforeach()

        # if pkgconfig for libjson-c doesn't provide
        # static lib info, then override PKG_STATIC here..
        if (NOT jsonc_PKG_STATIC_FOUND)
            set(jsonc_PKG_STATIC_LIBRARIES libjson-c.a)
        endif()
        set(XPREFIX jsonc_PKG_STATIC)
    else()
        if (NOT jsonc_PKG_FOUND)
            set(jsonc_PKG_LIBRARIES jsonc)
        endif()

        set(XPREFIX jsonc_PKG)
    endif()

    find_path(jsonc_INCLUDE_DIR json_c_version.h
        HINTS ${${XPREFIX}_INCLUDE_DIRS}
    )

    find_library(jsonc_LIBRARY_DEBUG NAMES ${${XPREFIX}_LIBRARIES}
        HINTS ${${XPREFIX}_LIBRARY_DIRS}
    )
    find_library(jsonc_LIBRARY_RELEASE NAMES ${${XPREFIX}_LIBRARIES}
        HINTS ${${XPREFIX}_LIBRARY_DIRS}
    )


########################################################################
# Windows
elseif (WIN32)
    set(jsonc_DIR "$ENV{jsonc_DIR}" CACHE FILEPATH "jsonc install directory")
    mark_as_advanced(jsonc_DIR)

    find_path(jsonc_INCLUDE_DIR json_c_version.h
        HINTS ${jsonc_DIR}
        PATH_SUFFIXES include
    )

    if (MSVC)
        # detect target architecture
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/arch.cpp" [=[
            #if defined _M_IX86
            #error ARCH_VALUE x86_32
            #elif defined _M_X64
            #error ARCH_VALUE x86_64
            #endif
            #error ARCH_VALUE unknown
        ]=])
        try_compile(_UNUSED_VAR "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/arch.cpp"
            OUTPUT_VARIABLE _COMPILATION_LOG
        )
        string(REGEX REPLACE ".*ARCH_VALUE ([a-zA-Z0-9_]+).*" "\\1" _TARGET_ARCH "${_COMPILATION_LOG}")

        # construct library path
        if (_TARGET_ARCH STREQUAL "x86_32")
            string(APPEND _PLATFORM_PATH "Win32")
        elseif(_TARGET_ARCH STREQUAL "x86_64")
            string(APPEND _PLATFORM_PATH "x64")
        else()
            message(FATAL_ERROR "the ${_TARGET_ARCH} architecture is not supported by Findjsonc.cmake.")
        endif()
        string(APPEND _PLATFORM_PATH "/$$CONFIG$$")

        if (MSVC_VERSION LESS 1900)
            math(EXPR _VS_VERSION "${MSVC_VERSION} / 10 - 60")
        else()
            math(EXPR _VS_VERSION "${MSVC_VERSION} / 10 - 50")
        endif()
        string(APPEND _PLATFORM_PATH "/v${_VS_VERSION}")

        if (jsonc_USE_STATIC_LIBS)
            string(APPEND _PLATFORM_PATH "/static")
        else()
            string(APPEND _PLATFORM_PATH "/dynamic")
        endif()

        string(REPLACE "$$CONFIG$$" "Debug" _DEBUG_PATH_SUFFIX "${_PLATFORM_PATH}")
        string(REPLACE "$$CONFIG$$" "Release" _RELEASE_PATH_SUFFIX "${_PLATFORM_PATH}")

        find_library(jsonc_LIBRARY_DEBUG libjson-c.lib
            HINTS ${jsonc_DIR}
            PATH_SUFFIXES ${_DEBUG_PATH_SUFFIX}
        )
        find_library(jsonc_LIBRARY_RELEASE libjson-c.lib
            HINTS ${jsonc_DIR}
            PATH_SUFFIXES ${_RELEASE_PATH_SUFFIX}
        )
        if (NOT jsonc_USE_STATIC_LIBS)
            set(CMAKE_FIND_LIBRARY_SUFFIXES_BCK ${CMAKE_FIND_LIBRARY_SUFFIXES})
            set(CMAKE_FIND_LIBRARY_SUFFIXES ".dll")
            find_library(jsonc_DLL_DEBUG libjson-c
                HINTS ${jsonc_DIR}
                PATH_SUFFIXES ${_DEBUG_PATH_SUFFIX}
            )
            find_library(jsonc_DLL_RELEASE libjson-c
                HINTS ${jsonc_DIR}
                PATH_SUFFIXES ${_RELEASE_PATH_SUFFIX}
            )
            set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_BCK})
        endif()

    elseif(_GCC_COMPATIBLE)
        if (jsonc_USE_STATIC_LIBS)
            find_library(jsonc_LIBRARY_DEBUG libjson-c.a
                HINTS ${jsonc_DIR}
                PATH_SUFFIXES lib
            )
            find_library(jsonc_LIBRARY_RELEASE libjson-c.a
                HINTS ${jsonc_DIR}
                PATH_SUFFIXES lib
            )
        else()
            find_library(jsonc_LIBRARY_DEBUG libjson-c.dll.a
                HINTS ${jsonc_DIR}
                PATH_SUFFIXES lib
            )
            find_library(jsonc_LIBRARY_RELEASE libjson-c.dll.a
                HINTS ${jsonc_DIR}
                PATH_SUFFIXES lib
            )

            file(GLOB _DLL
                LIST_DIRECTORIES false
                RELATIVE "${jsonc_DIR}/bin"
                "${jsonc_DIR}/bin/libjson-c*.dll"
            )
            find_library(jsonc_DLL_DEBUG ${_DLL} libjson-c
                HINTS ${jsonc_DIR}
                PATH_SUFFIXES bin
            )
            find_library(jsonc_DLL_RELEASE ${_DLL} libjson-c
                HINTS ${jsonc_DIR}
                PATH_SUFFIXES bin
            )
        endif()
    else()
        message(FATAL_ERROR "this platform is not supported by FindJsonc.cmake")
    endif()


########################################################################
# unsupported
else()
    message(FATAL_ERROR "this platform is not supported by FindJsonc.cmake")
endif()


########################################################################
# communicate results
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Jsonc # The name must be either uppercase or match the filename case.
    REQUIRED_VARS
        jsonc_LIBRARY_RELEASE
        jsonc_LIBRARY_DEBUG
        jsonc_INCLUDE_DIR
)

if(jsonc_FOUND)
    set(jsonc_LIBRARIES
        optimized ${jsonc_LIBRARY_RELEASE} debug ${jsonc_LIBRARY_DEBUG})
endif()

# mark file paths as advanced
mark_as_advanced(jsonc_INCLUDE_DIR)
mark_as_advanced(jsonc_LIBRARY_DEBUG)
mark_as_advanced(jsonc_LIBRARY_RELEASE)
if (WIN32)
    mark_as_advanced(jsonc_DLL_DEBUG)
    mark_as_advanced(jsonc_DLL_RELEASE)
endif()

# create imported target
if(jsonc_USE_STATIC_LIBS)
    set(_LIB_TYPE STATIC)
else()
    set(_LIB_TYPE SHARED)
endif()

if(NOT TARGET jsonc)
    add_library(jsonc ${_LIB_TYPE} IMPORTED)
endif()

set_target_properties(jsonc PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${jsonc_INCLUDE_DIR}"
    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
)

if (jsonc_USE_STATIC_LIBS)
    set_target_properties(jsonc PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "CJSON_STATIC"
        IMPORTED_LOCATION "${jsonc_LIBRARY_RELEASE}"
        IMPORTED_LOCATION_DEBUG "${jsonc_LIBRARY_DEBUG}"
    )
else()
    if (UNIX)
        set_target_properties(jsonc PROPERTIES
            IMPORTED_LOCATION "${jsonc_LIBRARY_RELEASE}"
            IMPORTED_LOCATION_DEBUG "${jsonc_LIBRARY_DEBUG}"
        )
    elseif (WIN32)
        set_target_properties(jsonc PROPERTIES
            IMPORTED_IMPLIB "${jsonc_LIBRARY_RELEASE}"
            IMPORTED_IMPLIB_DEBUG "${jsonc_LIBRARY_DEBUG}"
        )
        if (NOT (jsonc_DLL_DEBUG MATCHES ".*-NOTFOUND"))
            set_target_properties(jsonc PROPERTIES
                IMPORTED_LOCATION_DEBUG "${jsonc_DLL_DEBUG}"
            )
        endif()
        if (NOT (jsonc_DLL_RELEASE MATCHES ".*-NOTFOUND"))
            set_target_properties(jsonc PROPERTIES
                IMPORTED_LOCATION_RELWITHDEBINFO "${jsonc_DLL_RELEASE}"
                IMPORTED_LOCATION_MINSIZEREL "${jsonc_DLL_RELEASE}"
                IMPORTED_LOCATION_RELEASE "${jsonc_DLL_RELEASE}"
            )
        endif()
    endif()
endif()
