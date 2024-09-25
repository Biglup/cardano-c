# Proof of Achievement Report - CBOR serialization/deserialization for types up to Conway era

## Introduction

This report outlines the deliverables submitted for the third proof of achievement under the Catalyst Project, focusing on the Serialization Module development milestone. This milestone aimed to develop, document, and perform quality assurance on a C source code module implementing CBOR serialization/deserialization for the supported Cardano transaction structures/types:

 - [x] asset_id
 - [x] asset_id_list
 - [x] asset_id_map
 - [x] asset_name
 - [x] asset_name_list
 - [x] asset_name_map
 - [x] multi_asset
 - [x] policy_id_list
 - [x] auxiliary_data
 - [x] metadatum
 - [x] metadatum_kind
 - [x] metadatum_label_list
 - [x] metadatum_list
 - [x] metadatum_map
 - [x] plutus_v1_script_list
 - [x] plutus_v2_script_list
 - [x] plutus_v3_script_list
 - [x] transaction_metadata
 - [x] auth_committee_hot_cert
 - [x] cert_type
 - [x] certificate
 - [x] certificate_set
 - [x] genesis_key_delegation_cert
 - [x] mir_cert
 - [x] mir_cert_pot_type
 - [x] mir_cert_type
 - [x] mir_to_pot_cert
 - [x] mir_to_stake_creds_cert
 - [x] pool_registration_cert
 - [x] pool_retirement_cert
 - [x] register_drep_cert
 - [x] registration_cert
 - [x] resign_committee_cold_cert
 - [x] stake_delegation_cert
 - [x] stake_deregistration_cert
 - [x] stake_registration_cert
 - [x] stake_registration_delegation_cert
 - [x] stake_vote_delegation_cert
 - [x] stake_vote_registration_delegation_cert
 - [x] unregister_drep_cert
 - [x] unregistration_cert
 - [x] update_drep_cert
 - [x] vote_delegation_cert
 - [x] vote_registration_delegation_cert
 - [x] anchor
 - [x] bigint
 - [x] credential
 - [x] credential_type
 - [x] datum
 - [x] datum_type
 - [x] drep
 - [x] drep_type
 - [x] ex_units
 - [x] governance_action_id
 - [x] network_id
 - [x] protocol_version
 - [x] reward_address_list
 - [x] unit_interval
 - [x] utxo
 - [x] withdrawal_map
 - [x] constr_plutus_data
 - [x] plutus_data
 - [x] plutus_data_kind
 - [x] plutus_list
 - [x] plutus_map
 - [x] ipv4
 - [x] ipv6
 - [x] multi_host_name_relay
 - [x] pool_metadata
 - [x] pool_owners
 - [x] pool_params
 - [x] relay
 - [x] relay_type
 - [x] relays
 - [x] single_host_addr_relay
 - [x] single_host_name_relay
 - [x] committee
 - [x] committee_members_map
 - [x] constitution
 - [x] credential_set
 - [x] governance_action_type
 - [x] hard_fork_initiation_action
 - [x] info_action
 - [x] new_constitution_action
 - [x] no_confidence_action
 - [x] parameter_change_action
 - [x] proposal_procedure
 - [x] proposal_procedure_set
 - [x] treasury_withdrawals_action
 - [x] update_committee_action
 - [x] cost_model
 - [x] costmdls
 - [x] drep_voting_thresholds
 - [x] ex_unit_prices
 - [x] pool_voting_thresholds
 - [x] proposed_param_updates
 - [x] protocol_param_update
 - [x] update
 - [x] native_script
 - [x] native_script_list
 - [x] native_script_type
 - [x] script_all
 - [x] script_any
 - [x] script_invalid_after
 - [x] script_invalid_before
 - [x] script_n_of_k
 - [x] script_pubkey
 - [x] plutus_language_version
 - [x] plutus_v1_script
 - [x] plutus_v2_script
 - [x] plutus_v3_script
 - [x] script
 - [x] script_language
 - [x] transaction
 - [x] transaction_body
 - [x] transaction_input
 - [x] transaction_input_set
 - [x] transaction_output
 - [x] transaction_output_list
 - [x] value
 - [x] governance_action_id_list
 - [x] vote
 - [x] voter
 - [x] voter_list
 - [x] voter_type
 - [x] voting_procedure
 - [x] voting_procedures
 - [x] bootstrap_witness
 - [x] bootstrap_witness_set
 - [x] native_script_set
 - [x] plutus_data_set
 - [x] plutus_v1_script_set
 - [x] plutus_v2_script_set
 - [x] plutus_v3_script_set
 - [x] redeemer
 - [x] redeemer_list
 - [x] redeemer_tag
 - [x] vkey_witness
 - [x] vkey_witness_set
 - [x] witness_set

## Deliverables

Completeness checklist:

 - [x] GitHub repository link to the C source code.
 - [x] Documentation.
 - [x] MISRA compliance report.
 - [x] Unit test coverage report.
 - [x] Valgrind memory check results.

We have tagged the relevant version of the source code in our GitHub repository. The tag v0.0.3 marks the specific commit that represents the cutoff point for the code as delivered for this milestone. This allows reviewers, contributors, and users to easily access and review the exact state of the code that was evaluated and reported on in this milestone's deliverables. For convenience and further inspection, the tagged version can be found at https://github.com/Biglup/cardano-c/releases/tag/v0.0.3.

### 1. C Source Code

The source code for the address Encoding/Decoding module is available on GitHub:

- **GitHub Repository**: https://github.com/Biglup/cardano-c/

### 2. Documentation

We have compiled detailed documentation for the module, accessible at:

- **Documentation Site**: https://cardano-c.readthedocs.io/en/latest/

The documentation includes an API reference and usage examples.

### 3. Initial Quality Assurance Reports

#### MISRA Compliance Report

A PDF report detailing our adherence to MISRA C coding standards is available at the following link:

- **MISRA Compliance Report**: [MISRA_c_compliance_report_4fcca311f8dbbb1e0468eddbf54a0899f0fdb426.pdf](MISRA_c_compliance_report_4fcca311f8dbbb1e0468eddbf54a0899f0fdb426.pdf) 

The specific run of cppcheck and clang-tidy with the MISRA 2012 results can be found in the CI workflow logs for the PR that merged the changes:

https://github.com/Biglup/cardano-c/actions/runs/11015140276/job/30587454920 *(Navigate to "Check MISRA 2012 Rules" step for details)*

MISRA 2012 checking results confirmed: 0 MISRA violations

You can also find the comment by the bot that generates the report in the PR:

https://github.com/Biglup/cardano-c/pull/59#issuecomment-2370256588


#### Unit Test Coverage Report

The project as of commit 24ab8c95d5a7f301dfe9a99cd1f95470e4798d7a has a unit test coverage of 99%, as reported by Codecov:
- **Codecov Report**: https://app.codecov.io/gh/Biglup/cardano-c

#### Valgrind Memory Check Results

Memory integrity and leak checks were conducted using Valgrind, with results accessible through our Continuous Integration (CI) workflow:

- **Pull Request with Changes**: https://github.com/Biglup/cardano-c/pull/59
- **Valgrind Job**: https://github.com/Biglup/cardano-c/actions/runs/11015140311/job/30587454960 *(Navigate to "Run Unit Tests" step for details)*

Memory checking results confirmed: "No memory leaks detected," validating our module's memory safety.

### Additional Notes

- **Memory Leak Checking**: Memory integrity and leak checks are an integral part of our testing process. We utilize Valgrind in conjunction with unit tests (which currently boast 99% coverage) to ensure memory safety. Furthermore.

- **Location of Unit Tests**: https://github.com/Biglup/cardano-c/tree/main/lib/tests

### Conclusion

The CBOR serialization/deserialization Module Development milestone has been met with a keen focus on quality, security, and comprehensive documentation. We are confident in the robustness and reliability of our implementation. We look forward to feedback and suggestions from the community and reviewers to further enhance our work.
