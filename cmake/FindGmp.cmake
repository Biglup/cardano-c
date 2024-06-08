# Tries to find the local libgmp installation.
#
# On Windows the gmp_DIR environment variable is used as a default
# hint which can be overridden by setting the corresponding cmake variable.
#
# Once done the following variables will be defined:
#
#   gmp_FOUND
#   gmp_INCLUDE_DIR
#   gmp_LIBRARY_DEBUG
#   gmp_LIBRARY_RELEASE
#
#
# Furthermore an imported "gmp" target is created.
#

if (CMAKE_C_COMPILER_ID STREQUAL "GNU"
    OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(_GCC_COMPATIBLE 1)
endif()

# static library option
if (NOT DEFINED gmp_USE_STATIC_LIBS)
    option(gmp_USE_STATIC_LIBS "enable to statically link against gmp" OFF)
endif()
if(NOT (gmp_USE_STATIC_LIBS EQUAL gmp_USE_STATIC_LIBS_LAST))
    unset(gmp_LIBRARY CACHE)
    unset(gmp_LIBRARY_DEBUG CACHE)
    unset(gmp_LIBRARY_RELEASE CACHE)
    unset(gmp_DLL_DEBUG CACHE)
    unset(gmp_DLL_RELEASE CACHE)
    set(gmp_USE_STATIC_LIBS_LAST ${gmp_USE_STATIC_LIBS} CACHE INTERNAL "internal change tracking variable")
endif()


########################################################################
# UNIX
if (UNIX)
    # import pkg-config
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(gmp_PKG QUIET gmp)
    endif()

    if(gmp_USE_STATIC_LIBS)
        foreach(_libname ${gmp_PKG_STATIC_LIBRARIES})
            if (NOT _libname MATCHES "^lib.*\\.a$") # ignore strings already ending with .a
                list(INSERT gmp_PKG_STATIC_LIBRARIES 0 "lib${_libname}.a")
            endif()
        endforeach()

        # if pkgconfig for libgmp doesn't provide
        # static lib info, then override PKG_STATIC here..
        if (NOT gmp_PKG_STATIC_FOUND)
            set(gmp_PKG_STATIC_LIBRARIES libgmp.a)
        endif()
        set(XPREFIX gmp_PKG_STATIC)
    else()
        if (NOT gmp_PKG_FOUND)
            set(gmp_PKG_LIBRARIES gmp)
        endif()

        set(XPREFIX gmp_PKG)
    endif()

    find_path(gmp_INCLUDE_DIR gmp.h
        HINTS ${${XPREFIX}_INCLUDE_DIRS}
    )

    find_library(gmp_LIBRARY_DEBUG NAMES ${${XPREFIX}_LIBRARIES}
        HINTS ${${XPREFIX}_LIBRARY_DIRS}
    )
    find_library(gmp_LIBRARY_RELEASE NAMES ${${XPREFIX}_LIBRARIES}
        HINTS ${${XPREFIX}_LIBRARY_DIRS}
    )


########################################################################
# Windows
elseif (WIN32)
    set(gmp_DIR "$ENV{gmp_DIR}" CACHE FILEPATH "gmp install directory")
    mark_as_advanced(gmp_DIR)

    find_path(gmp_INCLUDE_DIR gmp.h
        HINTS ${gmp_DIR}
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
            message(FATAL_ERROR "the ${_TARGET_ARCH} architecture is not supported by Findgmp.cmake.")
        endif()
        string(APPEND _PLATFORM_PATH "/$$CONFIG$$")

        if (MSVC_VERSION LESS 1900)
            math(EXPR _VS_VERSION "${MSVC_VERSION} / 10 - 60")
        else()
            math(EXPR _VS_VERSION "${MSVC_VERSION} / 10 - 50")
        endif()
        string(APPEND _PLATFORM_PATH "/v${_VS_VERSION}")

        if (gmp_USE_STATIC_LIBS)
            string(APPEND _PLATFORM_PATH "/static")
        else()
            string(APPEND _PLATFORM_PATH "/dynamic")
        endif()

        string(REPLACE "$$CONFIG$$" "Debug" _DEBUG_PATH_SUFFIX "${_PLATFORM_PATH}")
        string(REPLACE "$$CONFIG$$" "Release" _RELEASE_PATH_SUFFIX "${_PLATFORM_PATH}")

        find_library(gmp_LIBRARY_DEBUG libgmp.lib
            HINTS ${gmp_DIR}
            PATH_SUFFIXES ${_DEBUG_PATH_SUFFIX}
        )
        find_library(gmp_LIBRARY_RELEASE libgmp.lib
            HINTS ${gmp_DIR}
            PATH_SUFFIXES ${_RELEASE_PATH_SUFFIX}
        )
        if (NOT gmp_USE_STATIC_LIBS)
            set(CMAKE_FIND_LIBRARY_SUFFIXES_BCK ${CMAKE_FIND_LIBRARY_SUFFIXES})
            set(CMAKE_FIND_LIBRARY_SUFFIXES ".dll")
            find_library(gmp_DLL_DEBUG libgmp
                HINTS ${gmp_DIR}
                PATH_SUFFIXES ${_DEBUG_PATH_SUFFIX}
            )
            find_library(gmp_DLL_RELEASE libgmp
                HINTS ${gmp_DIR}
                PATH_SUFFIXES ${_RELEASE_PATH_SUFFIX}
            )
            set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_BCK})
        endif()

    elseif(_GCC_COMPATIBLE)
        if (gmp_USE_STATIC_LIBS)
            find_library(gmp_LIBRARY_DEBUG libgmp.a
                HINTS ${gmp_DIR}
                PATH_SUFFIXES lib
            )
            find_library(gmp_LIBRARY_RELEASE libgmp.a
                HINTS ${gmp_DIR}
                PATH_SUFFIXES lib
            )
        else()
            find_library(gmp_LIBRARY_DEBUG libgmp.dll.a
                HINTS ${gmp_DIR}
                PATH_SUFFIXES lib
            )
            find_library(gmp_LIBRARY_RELEASE libgmp.dll.a
                HINTS ${gmp_DIR}
                PATH_SUFFIXES lib
            )

            file(GLOB _DLL
                LIST_DIRECTORIES false
                RELATIVE "${gmp_DIR}/bin"
                "${gmp_DIR}/bin/libgmp*.dll"
            )
            find_library(gmp_DLL_DEBUG ${_DLL} libgmp
                HINTS ${gmp_DIR}
                PATH_SUFFIXES bin
            )
            find_library(gmp_DLL_RELEASE ${_DLL} libgmp
                HINTS ${gmp_DIR}
                PATH_SUFFIXES bin
            )
        endif()
    else()
        message(FATAL_ERROR "this platform is not supported by FindGmp.cmake")
    endif()


########################################################################
# unsupported
else()
    message(FATAL_ERROR "this platform is not supported by FindGmp.cmake")
endif()


########################################################################
# communicate results
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Gmp # The name must be either uppercase or match the filename case.
    REQUIRED_VARS
        gmp_LIBRARY_RELEASE
        gmp_LIBRARY_DEBUG
        gmp_INCLUDE_DIR
)

if(gmp_FOUND)
    set(gmp_LIBRARIES
        optimized ${gmp_LIBRARY_RELEASE} debug ${gmp_LIBRARY_DEBUG})
endif()

# mark file paths as advanced
mark_as_advanced(gmp_INCLUDE_DIR)
mark_as_advanced(gmp_LIBRARY_DEBUG)
mark_as_advanced(gmp_LIBRARY_RELEASE)
if (WIN32)
    mark_as_advanced(gmp_DLL_DEBUG)
    mark_as_advanced(gmp_DLL_RELEASE)
endif()

# create imported target
if(gmp_USE_STATIC_LIBS)
    set(_LIB_TYPE STATIC)
else()
    set(_LIB_TYPE SHARED)
endif()

if(NOT TARGET gmp)
    add_library(gmp ${_LIB_TYPE} IMPORTED)
endif()

set_target_properties(gmp PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${gmp_INCLUDE_DIR}"
    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
)

if (gmp_USE_STATIC_LIBS)
    set_target_properties(gmp PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "GMP_STATIC"
        IMPORTED_LOCATION "${gmp_LIBRARY_RELEASE}"
        IMPORTED_LOCATION_DEBUG "${gmp_LIBRARY_DEBUG}"
    )
else()
    if (UNIX)
        set_target_properties(gmp PROPERTIES
            IMPORTED_LOCATION "${gmp_LIBRARY_RELEASE}"
            IMPORTED_LOCATION_DEBUG "${gmp_LIBRARY_DEBUG}"
        )
    elseif (WIN32)
        set_target_properties(gmp PROPERTIES
            IMPORTED_IMPLIB "${gmp_LIBRARY_RELEASE}"
            IMPORTED_IMPLIB_DEBUG "${gmp_LIBRARY_DEBUG}"
        )
        if (NOT (gmp_DLL_DEBUG MATCHES ".*-NOTFOUND"))
            set_target_properties(gmp PROPERTIES
                IMPORTED_LOCATION_DEBUG "${gmp_DLL_DEBUG}"
            )
        endif()
        if (NOT (gmp_DLL_RELEASE MATCHES ".*-NOTFOUND"))
            set_target_properties(gmp PROPERTIES
                IMPORTED_LOCATION_RELWITHDEBINFO "${gmp_DLL_RELEASE}"
                IMPORTED_LOCATION_MINSIZEREL "${gmp_DLL_RELEASE}"
                IMPORTED_LOCATION_RELEASE "${gmp_DLL_RELEASE}"
            )
        endif()
    endif()
endif()
