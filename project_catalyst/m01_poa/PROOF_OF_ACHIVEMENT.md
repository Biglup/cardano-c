# Proof of Achievement Report - Cryptography Module Development

## Introduction

This report outlines the deliverables submitted for the first proof of achievement under the Catalyst Project, focusing on the Cryptography Module Development milestone. This milestone aimed to develop, document, and perform initial quality assurance on a C source code module integrating cryptographic functionalities leveraging `libsodium`.

## Deliverables

Completeness checklist:

 - [x] GitHub repository link to the C source code.
 - [x] Documentation.
 - [x] MISRA compliance report.
 - [x] Unit test coverage report.
 - [x] Valgrind memory check results.

We have tagged the relevant version of the source code in our GitHub repository. The tag v0.0.1 marks the specific commit that represents the cutoff point for the code as delivered for this milestone. This allows reviewers, contributors, and users to easily access and review the exact state of the code that was evaluated and reported on in this milestone's deliverables. For convenience and further inspection, the tagged version can be found at https://github.com/Biglup/cardano-c/tree/v0.0.1.

### 1. C Source Code

The source code for the cryptography module is available on GitHub:

- **GitHub Repository**: https://github.com/Biglup/cardano-c/

The relevant code for the cryptography primitives is located at https://github.com/Biglup/cardano-c/tree/main/lib/src/crypto

### 2. Documentation

We have compiled detailed documentation for the module, accessible at:

- **Documentation Site**: https://cardano-c.readthedocs.io/en/latest/

The documentation includes an API reference and usage examples.

### 3. Initial Quality Assurance Reports

#### MISRA Compliance Report

A PDF report detailing our adherence to MISRA C coding standards is available at the following link:

- **MISRA Compliance Report**: [MISRA_c_compliance_report_6fd2cd9cbc1a1096ae974afec5941f95db91ea04.pdf](MISRA_c_compliance_report_6fd2cd9cbc1a1096ae974afec5941f95db91ea04%E2%80%AC.pdf) 

The specific run of cppcheck and clang-tidy with the MISRA 2012 results can be found in the CI workflow logs for the PR that merged the changes:

https://github.com/Biglup/cardano-c/actions/runs/8466890678/job/23196637862 *(Navigate to "Check MISRA 2012 Rules" step for details)*

MISRA 2012 checking results confirmed: 0 MISRA violations

#### Unit Test Coverage Report

The project as of commit 6fd2cd9cbc1a1096ae974afec5941f95db91ea04 has a unit test coverage of 98%, as reported by Codecov:
- **Codecov Report**: https://app.codecov.io/gh/Biglup/cardano-c

The cryptography modules have a 100% coverage: https://app.codecov.io/gh/Biglup/cardano-c/tree/main/lib%2Fsrc%2Fcrypto

#### Valgrind Memory Check Results

Memory integrity and leak checks were conducted using Valgrind, with results accessible through our Continuous Integration (CI) workflow:

- **Pull Request with Changes**: https://github.com/Biglup/cardano-c/pull/24
- **Valgrind Job**: https://github.com/Biglup/cardano-c/actions/runs/8466890668/job/23196637881 *(Navigate to "Run Unit Tests" step for details)*

Memory checking results confirmed: "No memory leaks detected," validating our module's memory safety.

### Additional Notes

- **Memory Leak Checking**: Memory integrity and leak checks are an integral part of our testing process. We utilize Valgrind in conjunction with unit tests (which currently boast 98% coverage) to ensure memory safety. Furthermore, we've employed a fuzzer with address sanitizers running for several days to bolster our module's resilience against potential vulnerabilities.

- **Location of Unit Tests**: https://github.com/Biglup/cardano-c/tree/main/lib/tests/crypto

### Conclusion

The Cryptography Module Development milestone has been met with a keen focus on quality, security, and comprehensive documentation. We are confident that these deliverables not only satisfy the milestone's requirements but also lay a solid foundation going forward.

