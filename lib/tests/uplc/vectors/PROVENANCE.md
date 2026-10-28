# UPLC conformance corpus - provenance

This directory holds the Plutus Core (UPLC) conformance test corpus used as the
correctness oracle for the cardano-c native UPLC VM. It is test data only. No file
here is compiled, covered, or MISRA-checked (see "Build exclusion" below).

## Upstream source

The cases originate from the official Plutus conformance suite:

  IntersectMBO/plutus, plutus-conformance/test-cases/uplc/evaluation
  https://github.com/IntersectMBO/plutus

  Commit: 08c571515 (2026-10-27)

Each case carries an expected result and an exact CPU/mem budget, and the corpus
spans the full V1 through V4 builtin surface. The tree is copied verbatim from the
commit above.

## Case count and layout

Total cases: 1037. Each case is one .uplc input with a .uplc.expected and a
.budget.expected file; 934 cases also carry a .flat input with a .flat.expected
file. Upstream README.md files, the builtin/semantics/ren.sh helper and the
bls12_381_G1_multiScalarMul/files data file are kept as well.

  builtin/     943 cases
    interleaving/    18
    parser/         102
    semantics/      823
  example/    17 cases
  term/       77 cases

UPLC program versions present in the inputs: 1.0.0 (905), 1.1.0 (82), 1.2.0 (5).

## File layout per case

Each case lives in its own directory named after the case. Inside:

  <case>.uplc                The UPLC program in textual surface syntax
                             (the conformance runner's input).
  <case>.uplc.expected       The expected result. Either the pretty-printed
                             result term, e.g. "(program 1.0.0 (con integer 24))",
                             or the literal text "evaluation failure" when the
                             program is expected to fail (error / out of budget /
                             type error), or "parse/decode error" for malformed
                             input.
  <case>.budget.expected     The expected execution budget on success, in the
                             format:
                               ({cpu: <n>
                               | mem: <n>})
                             For cases whose result is "evaluation failure" or
                             "parse/decode error" this file holds that text instead
                             of a budget.
  <case>.flat                The flat encoding of the program, for most cases.
  <case>.flat.expected       The expected outcome of evaluating the flat input:
                             the flat-encoded result program, or the text
                             "evaluation failure" or "parse/decode error".

The conformance runner evaluates both the .uplc and the .flat inputs. A .flat case
is decoded as the upstream harness does (plutus-conformance/src/PlutusConformance/
Common.hs): an empty input or a decode failure is a parse/decode error, and so is a
program with a free variable, which the harness rejects when it converts de Bruijn
indices back to names. Its result is compared against the program decoded from
<case>.flat.expected (or the failure text that file holds), and its budget against
the same <case>.budget.expected as the .uplc case.

## Evaluation configuration

The reference (plutus-conformance/src/PlutusConformance/CekEvaluator.hs) evaluates
every case with the newest builtin semantics variant (G), the newest cost model
(E) and the case-on-builtin support of the newest protocol version (12), with an
unlimited budget whose spend is compared exactly. The runner uses the same
configuration (language V4, protocol version 12, semantics variant G, V4 cost
model) and every case must pass: there is no expected-failure list.

The run also pins the corpus size (1037 .uplc and 934 .flat cases), so a missing
input fails it.

A second run pins mainnet behaviour: Plutus V3 at protocol version 10 and
semantics variant C, with the cost model the library selects for V3 at protocol
version 10 from the default V3 parameter list, over the cases whose expectations
do not change under that configuration. A case is in that run when it parses, its
program version and every builtin it names are available to V3 at protocol
version 10, it holds no array or value constant, every case term scrutinises a
constr term directly, it names no builtin costed differently under that
configuration (the integer-division builtins and equalsByteString, whose cost
model C costs differ, and appendString, equalsString and encodeUtf8, whose string
costing differs), and, when it expects a failure, it names no builtin whose
arguments variant G bounds. The rule reads only the program and its expected
outcome. Every covered case costs the same under builtin cost models C and E, so
the parameter list carrying cost model E values reproduces the mainnet budgets.

## Build exclusion (verified)

The CMake source globs in lib/CMakeLists.txt are:

  SOURCE_FILES   = src/*.c  include/*.h  include/*.inl
  TEST_SRC_FILES = tests/*.cpp  tests/*.h  tests/*.inl

Nothing under this directory matches: the extensions present are .uplc,
.uplc.expected, .budget.expected, .flat, .flat.expected, .sh and .md. There are no .c,
.cpp, .h, or .inl files in this corpus, so no file here is compiled, linked, covered, or MISRA-checked.
No CMake change was required to exclude the corpus.

The conformance runner reads these files at run time as data; it does not compile
them.
