# Proof of Achievement Report - Final Milestone

## Introduction

This report outlines the deliverables submitted for the fifth proof of achievement under the Catalyst Project:

- [x] Comprehensive code documentation for all developed modules.
- [x] A detailed report on compliance with MISRA-c 2012 standards.
- [x] Test suite results showing at least 80% coverage.
- [x] Valgrind (or similar) reports demonstrating memory management proficiency.
- [x] Project Closeout Report
- [x] Project Closeout Video

## Deliverables

Completeness checklist:

- [x] Comprehensive code documentation for all developed modules.
- [x] A detailed report on compliance with MISRA-c 2012 standards.
- [x] Test suite results showing at least 80% coverage.
- [x] Valgrind (or similar) reports demonstrating memory management proficiency.
- [x] Project Closeout Report
- [x] Project Closeout Video

We have tagged the relevant version of the source code in our GitHub repository. The tag v1.0.0 marks the specific commit that represents the cutoff point for the code as delivered for this milestone. This allows reviewers, contributors, and users to easily access and review the exact state of the code that was evaluated and reported on in this milestone's deliverables. For convenience and further inspection, the tagged version can be found at https://github.com/Biglup/cardano-c/releases/tag/v1.0.0.

### 1. C Source Code

The source code for the address Encoding/Decoding module is available on GitHub:

- **GitHub Repository**: https://github.com/Biglup/cardano-c/

### 2. Documentation

We have compiled detailed documentation for the module, accessible at:

- **Documentation Site**: https://cardano-c.readthedocs.io/en/latest/

The documentation includes an API reference and usage examples.

### 3. Quality Assurance Reports

#### MISRA Compliance Report

A PDF report detailing our adherence to MISRA C coding standards is available at the following link:

- **MISRA Compliance Report**: [MISRA_c_compliance_report_038948e7134b5cb06f14f102f947edbca89ce38f.pdf](MISRA_c_compliance_report_199ddbf8354a2ee8790f88351f06baf943b7de18.pdf) 

The specific run of cppcheck and clang-tidy with the MISRA 2012 results can be found in the CI workflow logs for the PR that merged the changes:

https://github.com/Biglup/cardano-c/actions/runs/12124432547/job/33802153662 *(Navigate to "Check MISRA 2012 Rules" step for details)*

MISRA 2012 checking results confirmed: 0 MISRA violations

You can also find the comment by the bot that generates the report in the PR:

https://github.com/Biglup/cardano-c/pull/88#issuecomment-2507474518

#### Unit Test Coverage Report

The project as of commit 69accd108e7bdf0e638d22a70009f17d08072a15 has a unit test coverage of 93.04%, as reported by Codecov:
- **Codecov Report**: https://app.codecov.io/gh/Biglup/cardano-c

#### Valgrind Memory Check Results

Memory integrity and leak checks were conducted using Valgrind, with results accessible through our Continuous Integration (CI) workflow:

- **Pull Request with Changes**: https://github.com/Biglup/cardano-c/pull/88
- **Valgrind Job**: https://github.com/Biglup/cardano-c/actions/runs/12124432565/job/33802153674 *(Navigate to "Run Unit Tests" step for details)*

Memory checking results confirmed: "No memory leaks detected," validating our module's memory safety.

### Additional Notes

- **Memory Leak Checking**: Memory integrity and leak checks are an integral part of our testing process. We utilize Valgrind in conjunction with unit tests (which currently boast 93.04% coverage) to ensure memory safety. Furthermore.

- **Location of Unit Tests**: https://github.com/Biglup/cardano-c/tree/main/lib/tests

### 4. Practical examples

We included in the repository a set of practical examples. These examples are available in the `examples` directory, and they cover a range of scenarios, including:

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
- Governance transactions.
    - https://github.com/Biglup/cardano-c/blob/main/examples/src/drep_pubkey_example.c
    - https://github.com/Biglup/cardano-c/blob/main/examples/src/drep_script_example.c
    - https://github.com/Biglup/cardano-c/blob/main/examples/src/propose_treasury_withdrawal_example.c
    - https://github.com/Biglup/cardano-c/blob/main/examples/src/vote_for_proposal_pubkey_example.c
    - https://github.com/Biglup/cardano-c/blob/main/examples/src/vote_for_proposal_script_example.c

### 5. Project Closeout Report

- Close-out report: https://docs.google.com/document/d/1nBfPQ10V_28ubKZkpqAhtJANqMQvMqNNsBlOPUFB97Y/
- Close-out video: https://www.youtube.com/watch?v=NdU-Sy2LAsU

### Conclusion

We hope that the community find in the Cardano C library a strong foundation for building blockchain applications. With its focus on quality, safety, and ease of use, it’s set to become an important tool in expanding the Cardano ecosystem. By making it easier for developers using different programming languages to work with Cardano, the library helps improve the current dApp ecosystem and opens the door for future innovations.

