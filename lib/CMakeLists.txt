CMAKE_MINIMUM_REQUIRED(VERSION 3.13)

# ----------------------------------------------------------------------------------
# Detect Emscripten
# ----------------------------------------------------------------------------------
IF("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
  SET(EMSCRIPTEN TRUE)
ELSE()
  SET(EMSCRIPTEN FALSE)
ENDIF()

# ----------------------------------------------------------------------------------
# If building for Emscripten, override shared/static library options
# ----------------------------------------------------------------------------------
IF(EMSCRIPTEN)
  MESSAGE(STATUS "Building for WebAssembly (Emscripten)")

  # Force building a static library for WASM
  SET(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  SET(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
ELSE()
  OPTION(BUILD_SHARED_LIBS "Default to building shared libraries" ON)
  OPTION(BUILD_STATIC_LIBS "Default to building static libraries" OFF)
ENDIF()

PROJECT(cardano-c C CXX)

SET(LIB_OUTPUT_NAME cardano-c)
SET(LIB_JS_OUTPUT_NAME cardano_c)
SET(sodium_USE_STATIC_LIBS ON)

INCLUDE_DIRECTORIES(SYSTEM ./include)

# ----------------------------------------------------------------------------
# Add libsodium
# ----------------------------------------------------------------------------
INCLUDE_DIRECTORIES(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/external/libsodium/include)
INCLUDE_DIRECTORIES(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/external/libsodium/include/sodium)

FILE(GLOB_RECURSE LIBSODIUM_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/external/libsodium/*.c)
FILE(GLOB_RECURSE LIBSODIUM_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/external/libsodium/*.h)

ADD_LIBRARY(libsodium OBJECT ${LIBSODIUM_SOURCES})
SET_TARGET_PROPERTIES(libsodium PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Suppress warnings for libsodium
SET_TARGET_PROPERTIES(libsodium PROPERTIES C_CLANG_TIDY "")
SET_TARGET_PROPERTIES(libsodium PROPERTIES CXX_CLANG_TIDY "")
IF(CMAKE_C_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
  TARGET_COMPILE_OPTIONS(libsodium PRIVATE -w)
ELSEIF(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  TARGET_COMPILE_OPTIONS(libsodium PRIVATE /W0)
ENDIF()

# ----------------------------------------------------------------------------
# Add GMP mini
# ----------------------------------------------------------------------------
INCLUDE_DIRECTORIES(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/external/gmp)
FILE(GLOB GMP_MINI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/external/gmp/*.c)
FILE(GLOB GMP_MINI_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/external/gmp/*.h)

ADD_LIBRARY(gmp-mini OBJECT ${GMP_MINI_SOURCES})
SET_TARGET_PROPERTIES(gmp-mini PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Suppress warnings for GMP mini
SET_TARGET_PROPERTIES(gmp-mini PROPERTIES C_CLANG_TIDY "")
SET_TARGET_PROPERTIES(gmp-mini PROPERTIES CXX_CLANG_TIDY "")
IF(CMAKE_C_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
  TARGET_COMPILE_OPTIONS(gmp-mini PRIVATE -w)
ELSEIF(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  TARGET_COMPILE_OPTIONS(gmp-mini PRIVATE /W0)
ENDIF()

IF(UNIX AND NOT EMSCRIPTEN)
  FIND_LIBRARY(MATH_LIBRARY m REQUIRED)
ELSE()
  SET(MATH_LIBRARY "")
ENDIF()

IF(UNIX AND NOT APPLE)
  SET(RT_LIBRARIES rt)
ELSE()
  SET(RT_LIBRARIES)
ENDIF()

# Create configuration header
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/config.h")
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/${LIB_OUTPUT_NAME}.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${LIB_OUTPUT_NAME}.pc")

# ----------------------------------------------------------------------------
# Source code files
# ----------------------------------------------------------------------------
FILE(GLOB_RECURSE HEADER_FILES   include/*.h  include/*.inl)
FILE(GLOB_RECURSE SOURCE_FILES   src/*.c      include/*.h  include/*.inl)
FILE(GLOB_RECURSE TEST_SRC_FILES tests/*.cpp  tests/*.h    tests/*.inl)

IF(NOT DISABLE_STATIC_FPIC)
  # Use '-fPIC'/'-fPIE' option.
  SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
ENDIF()

SET(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

IF(BUILD_SHARED_LIBS)
  ADD_DEFINITIONS(-D CARDANO_C_DLL)
ENDIF()

ADD_DEFINITIONS(-D SODIUM_STATIC)
ADD_DEFINITIONS(-D JSONC_STATIC)

# Compiler options
IF(CMAKE_COMPILER_IS_GNUC)
  SET(CMAKE_C_FLAGS
          "${CMAKE_C_FLAGS} -std=c99 -pedantic-errors -Wall -Wextra -Werror -Wswitch-enum -DHAVE_CONFIG_H")
ENDIF()

# ----------------------------------------------------------------------------
# Build the library for native or for Emscripten
# ----------------------------------------------------------------------------
IF(NOT EMSCRIPTEN)
  ADD_LIBRARY(${LIB_OUTPUT_NAME} SHARED
          ${SOURCE_FILES}
          $<TARGET_OBJECTS:gmp-mini>
          $<TARGET_OBJECTS:libsodium>
  )

  IF(WIN32)
    SET_TARGET_PROPERTIES(${LIB_OUTPUT_NAME} PROPERTIES C_CLANG_TIDY "")
  ENDIF()

  TARGET_LINK_LIBRARIES(${LIB_OUTPUT_NAME} ${MATH_LIBRARY})

  # Build static library if enabled
  IF(BUILD_STATIC_LIBS AND BUILD_SHARED_LIBS)
    SET(STATIC_LIB ${PROJECT_NAME}-static)
    ADD_LIBRARY(${STATIC_LIB} STATIC
            ${SOURCE_FILES}
            $<TARGET_OBJECTS:gmp-mini>
            $<TARGET_OBJECTS:libsodium>
    )

    TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}-static
            PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
            $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    )

    TARGET_LINK_LIBRARIES(${PROJECT_NAME}-static
            PRIVATE ${MATH_LIBRARY}
    )

    IF(NOT MSVC)
      SET_TARGET_PROPERTIES(${STATIC_LIB} PROPERTIES
              OUTPUT_NAME ${LIB_OUTPUT_NAME}
      )
    ENDIF()

    LIST(APPEND CMAKE_TARGETS ${STATIC_LIB})
  ENDIF()

ELSE()
  MESSAGE(STATUS "Building ${LIB_OUTPUT_NAME} for Emscripten as WASM")

  ADD_EXECUTABLE(${LIB_OUTPUT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
  SET_TARGET_PROPERTIES(${LIB_OUTPUT_NAME} PROPERTIES C_CLANG_TIDY "")
  SET_TARGET_PROPERTIES(${LIB_OUTPUT_NAME} PROPERTIES CXX_CLANG_TIDY "")

  TARGET_INCLUDE_DIRECTORIES(${LIB_OUTPUT_NAME}
          PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

  TARGET_LINK_LIBRARIES(${LIB_OUTPUT_NAME}
          PRIVATE $<TARGET_OBJECTS:gmp-mini>
          PRIVATE $<TARGET_OBJECTS:libsodium>
  )

  IF(CMAKE_BUILD_TYPE STREQUAL "Release")
    SET(EMS_BUILD_FLAGS "-O3")
  ELSEIF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    SET(EMS_BUILD_FLAGS "-g")
  ENDIF()

  SET_TARGET_PROPERTIES(${LIB_OUTPUT_NAME} PROPERTIES
          OUTPUT_NAME "${LIB_JS_OUTPUT_NAME}"
          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
          LINK_FLAGS "-s EXPORT_ALL=1 -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 --no-entry --whole-archive -s SINGLE_FILE=1 -s ASYNCIFY -s ENVIRONMENT=web,node ${EMS_BUILD_FLAGS}"
  )
ENDIF()

# ----------------------------------------------------------------------------
# Code Coverage
# ----------------------------------------------------------------------------
IF(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT EMSCRIPTEN)
  SET(GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
  SET(GCC_COVERAGE_LINK_FLAGS "-lgcov")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
ENDIF()

IF(CMAKE_BUILD_TYPE STREQUAL "Fuzz")
  # Set compiler to Clang explicitly
  SET(CMAKE_C_COMPILER "clang")
  SET(CMAKE_CXX_COMPILER "clang++")

  SET(CLANG_COMPILE_FLAGS "-g -O1 -fsanitize=fuzzer,address -fsanitize-address-use-after-scope -fno-omit-frame-pointer -fprofile-instr-generate -fcoverage-mapping -fprofile-arcs -ftest-coverage")

  SET(CLANG_LINK_FLAGS "-fsanitize=fuzzer,address -fsanitize-address-use-after-scope -fno-omit-frame-pointer -fprofile-instr-generate --coverage")

  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CLANG_COMPILE_FLAGS}")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CLANG_COMPILE_FLAGS}")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CLANG_LINK_FLAGS}")
ENDIF()

# ----------------------------------------------------------------------------
# Build tests
# ----------------------------------------------------------------------------
IF(TESTING_ENABLED AND TEST_SRC_FILES AND NOT EMSCRIPTEN)
  SET(PROJECT_TEST_NAME test-${LIB_OUTPUT_NAME})
  PROJECT(${PROJECT_TEST_NAME} CXX)

  SET(CMAKE_CXX_STANDARD 20)
  SET(CMAKE_CXX_STANDARD_REQUIRED ON)
  SET(CMAKE_CXX_EXTENSIONS OFF)

  FIND_PACKAGE(GTest REQUIRED)

  ADD_EXECUTABLE(${PROJECT_TEST_NAME} ${TEST_SRC_FILES})

  IF(WIN32)
    IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      TARGET_COMPILE_OPTIONS(${PROJECT_TEST_NAME} PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
    ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      TARGET_COMPILE_OPTIONS(${PROJECT_TEST_NAME} PRIVATE /std:c++20 /utf-8)
    ENDIF()

    TARGET_COMPILE_DEFINITIONS(${PROJECT_TEST_NAME} PRIVATE UNICODE _UNICODE)
  ENDIF()

  TARGET_LINK_LIBRARIES(${PROJECT_TEST_NAME}
          PRIVATE ${LIB_OUTPUT_NAME}
          PRIVATE ${GTEST_MAIN_LIBRARY}
          PRIVATE ${GTEST_LIBRARY}
          PRIVATE ${GTEST_MAIN_LIBRARIES}
          PRIVATE ${GTEST_LIBRARIES}
  )

  TARGET_INCLUDE_DIRECTORIES(${PROJECT_TEST_NAME} SYSTEM PRIVATE ${GTEST_INCLUDE_DIR})
  TARGET_INCLUDE_DIRECTORIES(${PROJECT_TEST_NAME} SYSTEM PRIVATE
          "${CMAKE_CURRENT_SOURCE_DIR}/include"
          "${CMAKE_CURRENT_SOURCE_DIR}/src"
  )

  GTEST_ADD_TESTS("${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_TEST_NAME}"
          ""
          ${TEST_SRC_FILES})
ENDIF()
