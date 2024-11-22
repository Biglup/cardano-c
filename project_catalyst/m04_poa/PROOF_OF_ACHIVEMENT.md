# Proof of Achievement Report - Transaction Builder

## Introduction

This report outlines the deliverables submitted for the fourth proof of achievement under the Catalyst Project, focusing on the Transaction Builder development milestone. This milestone aimed to develop, document, and perform quality assurance on a C source code module implementing a high level Transaction Builder that can:

- [x] Transfer value between addresses (both ADA and native assets).
- [x] Spend outputs that are guarded by Plutus scripts (up to Plutus V3).
- [x] Mint assets using Plutus scripts (up to Plutus V3).
- [x] Spend outputs that are guarded by Native scripts.
- [x] Mint assets using Native scripts.
- [x] Issue all available certificates in Conway era as specified in https://github.com/IntersectMBO/cardano-ledger/blob/master/eras/conway/impl/cddl-files/conway.cddl#L275-L293.
- [x] Attach auxiliary data to the transaction.

## Deliverables

Completeness checklist:

 - [x] GitHub repository link to the C source code.
 - [x] Documentation.
 - [x] MISRA compliance report.
 - [x] Unit test coverage report.
 - [x] Valgrind memory check results.
 - [x] Practical examples.

We have tagged the relevant version of the source code in our GitHub repository. The tag v0.0.4 marks the specific commit that represents the cutoff point for the code as delivered for this milestone. This allows reviewers, contributors, and users to easily access and review the exact state of the code that was evaluated and reported on in this milestone's deliverables. For convenience and further inspection, the tagged version can be found at https://github.com/Biglup/cardano-c/releases/tag/v0.0.4.

### 1. C Source Code

The source code for the address Encoding/Decoding module is available on GitHub:

- **GitHub Repository**: https://github.com/Biglup/cardano-c/

### 2. Documentation

We have compiled detailed documentation for the module, accessible at:

- **Documentation Site**: https://cardano-c.readthedocs.io/en/latest/

The documentation includes an API reference and usage examples.

For thr Transaction Builder API you can find the relevant documentation here: https://cardano-c.readthedocs.io/en/latest/api/transaction_builder/transaction_builder.html

### 3. Quality Assurance Reports

#### MISRA Compliance Report

A PDF report detailing our adherence to MISRA C coding standards is available at the following link:

- **MISRA Compliance Report**: [MISRA_c_compliance_report_199ddbf8354a2ee8790f88351f06baf943b7de18.pdf](MISRA_c_compliance_report_199ddbf8354a2ee8790f88351f06baf943b7de18.pdf) 

The specific run of cppcheck and clang-tidy with the MISRA 2012 results can be found in the CI workflow logs for the PR that merged the changes:

https://github.com/Biglup/cardano-c/actions/runs/11957338509/job/33334258692 *(Navigate to "Check MISRA 2012 Rules" step for details)*

MISRA 2012 checking results confirmed: 0 MISRA violations

You can also find the comment by the bot that generates the report in the PR:

https://github.com/Biglup/cardano-c/pull/81#issuecomment-2491705879


#### Unit Test Coverage Report

The project as of commit 69accd108e7bdf0e638d22a70009f17d08072a15 has a unit test coverage of 93.04%, as reported by Codecov:
- **Codecov Report**: https://app.codecov.io/gh/Biglup/cardano-c

#### Valgrind Memory Check Results

Memory integrity and leak checks were conducted using Valgrind, with results accessible through our Continuous Integration (CI) workflow:

- **Pull Request with Changes**: https://github.com/Biglup/cardano-c/pull/81
- **Valgrind Job**: https://github.com/Biglup/cardano-c/actions/runs/11957338518/job/33334258693 *(Navigate to "Run Unit Tests" step for details)*

Memory checking results confirmed: "No memory leaks detected," validating our module's memory safety.

### Additional Notes

- **Memory Leak Checking**: Memory integrity and leak checks are an integral part of our testing process. We utilize Valgrind in conjunction with unit tests (which currently boast 93.04% coverage) to ensure memory safety. Furthermore.

- **Location of Unit Tests**: https://github.com/Biglup/cardano-c/tree/main/lib/tests

### 4. Practical examples

We included in the repository a set of practical examples that demonstrate the use of the Transaction Builder module. These examples are available in the `examples` directory, and they cover a range of scenarios, including:

- Transferring ADA between addresses:
    - https://github.com/Biglup/cardano-c/blob/main/examples/src/send_lovelace_example.c
- Spending outputs guarded by Plutus scripts.
    - https://github.com/Biglup/cardano-c/blob/main/examples/src/spend_from_validator_example.c
- Minting assets using Plutus scripts. 
    - https://github.com/Biglup/cardano-c/blob/main/examples/src/mint_burn_plutus_script_example.c
- Spending outputs guarded by Native scripts.
    - https://github.com/Biglup/cardano-c/blob/main/examples/src/spend_from_native_script_example.c
- Minting assets using Native scripts.
    - https://github.com/Biglup/cardano-c/blob/main/examples/src/mint_burn_native_script_example.c
- Issuing Conway era certificates.
    - https://github.com/Biglup/cardano-c/blob/main/examples/src/drep_pubkey_example.c
    - https://github.com/Biglup/cardano-c/blob/main/examples/src/drep_script_example.c
    - https://github.com/Biglup/cardano-c/blob/main/examples/src/reward_account_pubkey_example.c
    - https://github.com/Biglup/cardano-c/blob/main/examples/src/reward_account_script_example.c
- Attaching auxiliary data to the transaction.
    - https://github.com/Biglup/cardano-c/blob/main/examples/src/spend_from_native_script_example.c
    - https://github.com/Biglup/cardano-c/blob/main/examples/src/mint_burn_native_script_example.c

### Conclusion

The Transaction Builder Development milestone has been met with a keen focus on quality, security, and comprehensive documentation. We are confident in the robustness and reliability of our implementation. We look forward to feedback and suggestions from the community and reviewers to further enhance our work.
