# from: https://github.com/google/fuzzing/issues/41#issuecomment-1027653690
# To have a good coverage report for the fuzzer, let it run for a while and then use the generation corpora for code
# coverage generation. It also will be faster to do so.
#
# 1.- run a normal fuzzer build (without coverage instrumentation)
#     ./fuzzer -any_other_runtime_flags=1 ./your_corpora_directory
# 2.- minimize the corpora
#     mkdir corpora_minimized
#     ./fuzzer -merge=1 ./corpora_minimized ./your_corpora_directory
#
# 3.- run coverage instrumented build over the minimized corpora
#     ./fuzzer -runs=0 ./corpora_minimized
#
# Basic instructions to generate coverage report:
# 1.- To generate profraw, you must generate the corpus first. The rerun the fuzzers with the corpus:
#     LLVM_PROFILE_FILE="./build/fuzzer.profraw" LD_LIBRARY_PATH=./build/release/lib ./build/cbor_reader_fuzzer -runs=0 ./fuzz/corpus
#
# 2.- After profraw is generated run:
#     llvm-profdata merge -sparse *.profraw -o merged.profdata
#
# 3.- To generate report run:
#     llvm-cov show -format=html -instr-profile=merged.profdata -o html_coverage_report ./release/lib/libcardano-c.so

# Detect all fuzz targets
FILE(GLOB FUZZ_SOURCES "*_fuzzer.c")
MESSAGE(STATUS "Fuzz sources: ${FUZZ_SOURCES}")
FOREACH(FUZZ_SOURCE ${FUZZ_SOURCES})
    GET_FILENAME_COMPONENT(FUZZ_TARGET_NAME ${FUZZ_SOURCE} NAME_WE) # Extracts the filename without extension

    # Define executable target for the fuzzer
    ADD_EXECUTABLE(${FUZZ_TARGET_NAME} ${FUZZ_SOURCE})
    MESSAGE(STATUS "Fuzz source: ${FUZZ_TARGET_NAME}")
    # Include directories
    TARGET_INCLUDE_DIRECTORIES(${FUZZ_TARGET_NAME} PRIVATE ${CARDANO_C_INCLUDE_DIR})

    # Set compiler to Clang explicitly
    SET (CMAKE_C_COMPILER "clang")
    SET (CMAKE_CXX_COMPILER "clang++")

    # Compiler and linker options for fuzzing and coverage
    TARGET_COMPILE_OPTIONS(${FUZZ_TARGET_NAME} PRIVATE -fsanitize=fuzzer,address -g -O1 -fprofile-instr-generate -fcoverage-mapping -fprofile-arcs -ftest-coverage)
    TARGET_LINK_LIBRARIES(${FUZZ_TARGET_NAME} PRIVATE  cardano-c gcov -fsanitize=fuzzer,address -fprofile-instr-generate)
ENDFOREACH()