# UPLC conformance corpus - provenance

This directory holds the Plutus Core (UPLC) conformance test corpus used as the
correctness oracle for the cardano-c native UPLC VM. It is test data only. No file
here is compiled, covered, or MISRA-checked (see "Build exclusion" below).

## Upstream source

The cases originate from the official Plutus conformance suite:

  IntersectMBO/plutus, plutus-conformance test data
  https://github.com/IntersectMBO/plutus

Each case carries an expected result and an exact CPU/mem budget, and the corpus
spans the full V1 through V4 builtin surface. The case count and file format match
the upstream plutus-conformance layout.

## Case count and layout

Total cases: 991 (each case = one .uplc input + one .uplc.expected + one
.uplc.budget.expected). Total files: 2973 case files (991 x 3) plus upstream
README.md files and this PROVENANCE.md.

  builtin/   904 cases
    constant/        97
    interleaving/    18
    semantics/      789
  example/    17 cases
  term/       70 cases

UPLC language versions present in the inputs: program 1.0.0 (1452 occurrences) and
program 1.1.0 (81 occurrences; 1.1.0 adds constr/case for V3+).

## File layout per case

Each case lives in its own directory named after the case. Inside:

  <case>.uplc                  The UPLC program in textual surface syntax
                               (the conformance runner's input).
  <case>.uplc.expected         The expected result. Either the pretty-printed
                               result term, e.g. "(program 1.0.0 (con integer 24))",
                               or the literal text "evaluation failure" when the
                               program is expected to fail (error / out of budget /
                               type error), or "parse error" for malformed input.
  <case>.uplc.budget.expected  The expected execution budget on success, in the
                               format:
                                 ({cpu: <n>
                                 | mem: <n>})
                               For cases whose result is "evaluation failure" this
                               file also contains "evaluation failure" (no budget).

A few directories also contain an upstream README.md describing how the test
vectors were generated (secp256k1, bls12_381, builtin semantics/interleaving).
These are documentation, not cases.

## Build exclusion (verified)

The CMake source globs in lib/CMakeLists.txt are:

  SOURCE_FILES   = src/*.c  include/*.h  include/*.inl
  TEST_SRC_FILES = tests/*.cpp  tests/*.h  tests/*.inl

Nothing under this directory matches: the only extensions present are .uplc,
.uplc.expected, .uplc.budget.expected and .md. There are no .c, .cpp, .h, or .inl
files in this corpus, so no file here is compiled, linked, covered, or MISRA-checked.
No CMake change was required to exclude the corpus.

The conformance runner reads these files at run time as data; it does not compile
them.
