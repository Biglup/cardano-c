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

# ----------------------------------------------------------------------------
# Add SHA-3 / Keccak (tiny_sha3)
# ----------------------------------------------------------------------------
INCLUDE_DIRECTORIES(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/external/sha3)
FILE(GLOB SHA3_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/external/sha3/*.c)
FILE(GLOB SHA3_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/external/sha3/*.h)

ADD_LIBRARY(sha3 OBJECT ${SHA3_SOURCES})
SET_TARGET_PROPERTIES(sha3 PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Suppress warnings for tiny_sha3
SET_TARGET_PROPERTIES(sha3 PROPERTIES C_CLANG_TIDY "")
SET_TARGET_PROPERTIES(sha3 PROPERTIES CXX_CLANG_TIDY "")
IF(CMAKE_C_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
  TARGET_COMPILE_OPTIONS(sha3 PRIVATE -w)
ELSEIF(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  TARGET_COMPILE_OPTIONS(sha3 PRIVATE /W0)
ENDIF()

# ----------------------------------------------------------------------------
# Add RIPEMD-160
# ----------------------------------------------------------------------------
INCLUDE_DIRECTORIES(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/external/ripemd160)
FILE(GLOB RIPEMD160_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/external/ripemd160/*.c)
FILE(GLOB RIPEMD160_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/external/ripemd160/*.h)

ADD_LIBRARY(ripemd160 OBJECT ${RIPEMD160_SOURCES})
SET_TARGET_PROPERTIES(ripemd160 PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Suppress warnings for RIPEMD-160
SET_TARGET_PROPERTIES(ripemd160 PROPERTIES C_CLANG_TIDY "")
SET_TARGET_PROPERTIES(ripemd160 PROPERTIES CXX_CLANG_TIDY "")
IF(CMAKE_C_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
  TARGET_COMPILE_OPTIONS(ripemd160 PRIVATE -w)
ELSEIF(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  TARGET_COMPILE_OPTIONS(ripemd160 PRIVATE /W0)
ENDIF()

# ----------------------------------------------------------------------------
# Add secp256k1 (ECDSA-verify + schnorrsig + extrakeys only)
#
# Built without autotools and without codegen: this release is configured purely
# by compile defines and ships the precomputed ecmult tables checked in, so the
# main translation unit plus the two table files compile directly.
# ----------------------------------------------------------------------------
INCLUDE_DIRECTORIES(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/external/secp256k1/include)

SET(SECP256K1_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/external/secp256k1/src/secp256k1.c
        ${CMAKE_CURRENT_SOURCE_DIR}/external/secp256k1/src/precomputed_ecmult.c
        ${CMAKE_CURRENT_SOURCE_DIR}/external/secp256k1/src/precomputed_ecmult_gen.c
)

ADD_LIBRARY(secp256k1 OBJECT ${SECP256K1_SOURCES})
SET_TARGET_PROPERTIES(secp256k1 PROPERTIES POSITION_INDEPENDENT_CODE ON)

TARGET_COMPILE_DEFINITIONS(secp256k1 PRIVATE
        ENABLE_MODULE_EXTRAKEYS=1
        ENABLE_MODULE_SCHNORRSIG=1
        ECMULT_WINDOW_SIZE=8
        SECP256K1_STATIC
)

# Suppress warnings for secp256k1
SET_TARGET_PROPERTIES(secp256k1 PROPERTIES C_CLANG_TIDY "")
SET_TARGET_PROPERTIES(secp256k1 PROPERTIES CXX_CLANG_TIDY "")
IF(CMAKE_C_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
  TARGET_COMPILE_OPTIONS(secp256k1 PRIVATE -w)
ELSEIF(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  TARGET_COMPILE_OPTIONS(secp256k1 PRIVATE /W0)
ENDIF()

# ----------------------------------------------------------------------------
# Add blst (BLS12-381) as the portable, no-assembly build.
#
# Only src/server.c is a compilation unit; it #includes the implementation .c
# files it needs. __BLST_NO_ASM__ selects the reference C field arithmetic and
# __BLST_PORTABLE__ disables the runtime-dispatched SHA-256 assembly kernels,
# so there is no assembly anywhere and the target builds on WASM and the
# armv6/armv7 cross builds. See lib/external/blst/VERSION.txt.
# ----------------------------------------------------------------------------
INCLUDE_DIRECTORIES(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/external/blst/bindings)

ADD_LIBRARY(blst OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/external/blst/src/server.c)
SET_TARGET_PROPERTIES(blst PROPERTIES POSITION_INDEPENDENT_CODE ON)

TARGET_COMPILE_DEFINITIONS(blst PRIVATE __BLST_NO_ASM__ __BLST_PORTABLE__)

# Suppress warnings for blst
SET_TARGET_PROPERTIES(blst PROPERTIES C_CLANG_TIDY "")
SET_TARGET_PROPERTIES(blst PROPERTIES CXX_CLANG_TIDY "")
IF(CMAKE_C_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
  TARGET_COMPILE_OPTIONS(blst PRIVATE -w)
ELSEIF(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  TARGET_COMPILE_OPTIONS(blst PRIVATE /W0)
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

# Link-time optimization for release builds. Compiling the library sources as a
# whole lets the hot interpreter paths inline the arena allocator, the bit
# reader and the value constructors across translation units, which a per-file
# build cannot do. Applied per configuration so only Release is affected, and
# guarded so toolchains without it fall back to a normal build.
INCLUDE(CheckIPOSupported)
CHECK_IPO_SUPPORTED(RESULT CARDANO_IPO_SUPPORTED OUTPUT CARDANO_IPO_OUTPUT)

IF(CARDANO_DISABLE_IPO)
  SET(CARDANO_IPO_SUPPORTED FALSE)
ENDIF()

# ----------------------------------------------------------------------------
# Build the library for native or for Emscripten
# ----------------------------------------------------------------------------
IF(NOT EMSCRIPTEN)
  ADD_LIBRARY(${LIB_OUTPUT_NAME} SHARED
          ${SOURCE_FILES}
          $<TARGET_OBJECTS:gmp-mini>
          $<TARGET_OBJECTS:libsodium>
          $<TARGET_OBJECTS:sha3>
          $<TARGET_OBJECTS:ripemd160>
          $<TARGET_OBJECTS:secp256k1>
          $<TARGET_OBJECTS:blst>
  )

  IF(WIN32)
    SET_TARGET_PROPERTIES(${LIB_OUTPUT_NAME} PROPERTIES C_CLANG_TIDY "")
  ENDIF()

  TARGET_LINK_LIBRARIES(${LIB_OUTPUT_NAME} ${MATH_LIBRARY})

  IF(CARDANO_IPO_SUPPORTED)
    SET_TARGET_PROPERTIES(${LIB_OUTPUT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
  ENDIF()

  # Build static library if enabled
  IF(BUILD_STATIC_LIBS AND BUILD_SHARED_LIBS)
    SET(STATIC_LIB ${PROJECT_NAME}-static)
    ADD_LIBRARY(${STATIC_LIB} STATIC
            ${SOURCE_FILES}
            $<TARGET_OBJECTS:gmp-mini>
            $<TARGET_OBJECTS:libsodium>
            $<TARGET_OBJECTS:sha3>
            $<TARGET_OBJECTS:ripemd160>
            $<TARGET_OBJECTS:secp256k1>
            $<TARGET_OBJECTS:blst>
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

    IF(CARDANO_IPO_SUPPORTED)
      SET_TARGET_PROPERTIES(${STATIC_LIB} PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
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
          PRIVATE $<TARGET_OBJECTS:sha3>
          PRIVATE $<TARGET_OBJECTS:ripemd160>
          PRIVATE $<TARGET_OBJECTS:secp256k1>
          PRIVATE $<TARGET_OBJECTS:blst>
  )

  IF(CMAKE_BUILD_TYPE STREQUAL "Release")
    SET(EMS_BUILD_FLAGS "-Oz")
  ELSEIF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    SET(EMS_BUILD_FLAGS "-g")
  ENDIF()

  IF(EMS_BUILD_FLAGS)
    TARGET_COMPILE_OPTIONS(${LIB_OUTPUT_NAME} PRIVATE ${EMS_BUILD_FLAGS})
  ENDIF()

  SET_TARGET_PROPERTIES(${LIB_OUTPUT_NAME} PROPERTIES
          OUTPUT_NAME "${LIB_JS_OUTPUT_NAME}"
          RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
          LINK_FLAGS "-s EXPORT_ALL=1 -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 --no-entry --whole-archive -s SINGLE_FILE=1 -s ASYNCIFY -s WASM_BIGINT=0 -s ENVIRONMENT=web,node ${EMS_BUILD_FLAGS}"
  )
ENDIF()

# ----------------------------------------------------------------------------
# Code Coverage
# ----------------------------------------------------------------------------
IF(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT EMSCRIPTEN)
  IF(CMAKE_C_COMPILER_ID MATCHES "AppleClang|Clang")
    SET(COVERAGE_COMPILE_FLAGS "-fprofile-instr-generate -fcoverage-mapping")
    SET(COVERAGE_LINK_FLAGS "-fprofile-instr-generate")
  ELSEIF(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    SET(COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
    SET(COVERAGE_LINK_FLAGS "-lgcov")
  ENDIF()

  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COVERAGE_COMPILE_FLAGS}")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COVERAGE_COMPILE_FLAGS}")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COVERAGE_LINK_FLAGS}")
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
  IF(APPLE)
    SET(CMAKE_THREAD_LIBS_INIT "-lpthread")
    SET(CMAKE_HAVE_THREADS_LIBRARY 1)
    SET(CMAKE_USE_WIN32_THREADS_INIT 0)
    SET(CMAKE_USE_PTHREADS_INIT 1)
    SET(THREADS_PREFER_PTHREAD_FLAG ON)
  ENDIF()

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
