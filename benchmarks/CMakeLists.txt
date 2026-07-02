# Benchmark harnesses. Not part of the library; built only when
# BENCHMARKS_ENABLED is set. The uplc bench drives the module-internal
# UPLC headers directly (same include arrangement as the fuzz targets).

FILE (GLOB_RECURSE UPLC_BENCH_SOURCES uplc/*.c)

ADD_EXECUTABLE (uplc-bench ${UPLC_BENCH_SOURCES})

TARGET_INCLUDE_DIRECTORIES (uplc-bench PRIVATE ${CARDANO_C_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/lib/src ${CMAKE_CURRENT_SOURCE_DIR}/uplc)

# Benchmark harness code is not library code; keep clang-tidy off it.
SET_TARGET_PROPERTIES (uplc-bench PROPERTIES C_CLANG_TIDY "")

IF (TARGET cardano-c-static)
    TARGET_LINK_LIBRARIES (uplc-bench PRIVATE cardano-c-static m)
ELSE ()
    TARGET_LINK_LIBRARIES (uplc-bench PRIVATE cardano-c m)
ENDIF ()
