# Proof of Achievement Report - Address Encoding/Decoding Development

## Introduction

This report outlines the deliverables submitted for the second proof of achievement under the Catalyst Project, focusing on the address Encoding/Decoding Development milestone. This milestone aimed to develop, document, and perform quality assurance on a C source code module implementing address encoding and decoding for all [CIP-019 ](https://cips.cardano.org/cip/CIP-19) address formats:

- Byron Address
- Base Address
- Enterprise Address
- Pointer Address
- Reward Address

## Deliverables

Completeness checklist:

 - [x] GitHub repository link to the C source code.
 - [x] Documentation.
 - [x] MISRA compliance report.
 - [x] Unit test coverage report.
 - [x] Valgrind memory check results.

We have tagged the relevant version of the source code in our GitHub repository. The tag v0.0.2 marks the specific commit that represents the cutoff point for the code as delivered for this milestone. This allows reviewers, contributors, and users to easily access and review the exact state of the code that was evaluated and reported on in this milestone's deliverables. For convenience and further inspection, the tagged version can be found at https://github.com/Biglup/cardano-c/tree/v0.0.2.

### 1. C Source Code

The source code for the address Encoding/Decoding module is available on GitHub:

- **GitHub Repository**: https://github.com/Biglup/cardano-c/

The relevant code for the CIP-019 address primitives is located at https://github.com/Biglup/cardano-c/tree/main/lib/src/address

### 2. Documentation

We have compiled detailed documentation for the module, accessible at:

- **Documentation Site**: https://cardano-c.readthedocs.io/en/latest/

The documentation includes an API reference and usage examples.

### 3. Initial Quality Assurance Reports

#### MISRA Compliance Report

A PDF report detailing our adherence to MISRA C coding standards is available at the following link:

- **MISRA Compliance Report**: [MISRA_c_compliance_report_8a533e7d9722b5cb579fb8d35ff11f67df9d3805.pdf](MISRA_c_compliance_report_8a533e7d9722b5cb579fb8d35ff11f67df9d3805.pdf) 

The specific run of cppcheck and clang-tidy with the MISRA 2012 results can be found in the CI workflow logs for the PR that merged the changes:

https://github.com/Biglup/cardano-c/actions/runs/8850142669/job/24303720785 *(Navigate to "Check MISRA 2012 Rules" step for details)*

MISRA 2012 checking results confirmed: 0 MISRA violations

You can also find the comment by the bot that generates the report in the PR:

https://github.com/Biglup/cardano-c/pull/32#issuecomment-2079082659


#### Unit Test Coverage Report

The project as of commit 2b470ded8efa0648b77d0d6863537651664eb618 has a unit test coverage of 99%, as reported by Codecov:
- **Codecov Report**: https://app.codecov.io/gh/Biglup/cardano-c

The address modules have a 100% coverage: https://app.codecov.io/gh/Biglup/cardano-c/tree/main/lib/src/address

#### Valgrind Memory Check Results

Memory integrity and leak checks were conducted using Valgrind, with results accessible through our Continuous Integration (CI) workflow:

- **Pull Request with Changes**: https://github.com/Biglup/cardano-c/pull/32
- **Valgrind Job**: https://github.com/Biglup/cardano-c/actions/runs/8848991991/job/24299917793 *(Navigate to "Run Unit Tests" step for details)*

Memory checking results confirmed: "No memory leaks detected," validating our module's memory safety.

### Additional Notes

- **Memory Leak Checking**: Memory integrity and leak checks are an integral part of our testing process. We utilize Valgrind in conjunction with unit tests (which currently boast 99% coverage) to ensure memory safety. Furthermore.

- **Location of Unit Tests**: https://github.com/Biglup/cardano-c/tree/main/lib/tests/address

### Conclusion

The Address Encoding/Decoding Module Development milestone has been met with a keen focus on quality, security, and comprehensive documentation. We are confident in the robustness and reliability of our implementation. We look forward to feedback and suggestions from the community and reviewers to further enhance our work.
