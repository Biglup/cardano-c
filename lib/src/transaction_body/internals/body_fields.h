/**
 * \file body_fields.h
 *
 * \author angel.castillo
 * \date   Jul 18, 2026
 *
 * Copyright 2026 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BODY_FIELDS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BODY_FIELDS_H

/* INCLUDES ******************************************************************/

#include <cardano/assets/multi_asset.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/certs/certificate_set.h>
#include <cardano/common/guard_set.h>
#include <cardano/common/network_id.h>
#include <cardano/common/withdrawal_map.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/proposal_procedures/proposal_procedure_set.h>
#include <cardano/transaction_body/account_balance_intervals_map.h>
#include <cardano/transaction_body/direct_deposit_map.h>
#include <cardano/transaction_body/required_guards_map.h>
#include <cardano/transaction_body/transaction_input_set.h>
#include <cardano/transaction_body/transaction_output_list.h>
#include <cardano/voting_procedures/voting_procedures.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Reads the wire form of a body inputs field value.
 *
 * This function decodes a transaction input set from the given CBOR reader. The map key is
 * expected to have already been consumed by the caller.
 *
 * \param[in] reader A pointer to the CBOR reader positioned at the field value. This parameter
 *                   must not be NULL.
 * \param[out] inputs On success, points to the decoded transaction input set. The caller owns
 *                    the returned reference and must release it. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value was decoded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_body_read_inputs(cardano_cbor_reader_t* reader, cardano_transaction_input_set_t** inputs);

/**
 * \brief Reads the wire form of a body outputs field value.
 *
 * This function decodes a transaction output list from the given CBOR reader. The map key is
 * expected to have already been consumed by the caller.
 *
 * \param[in] reader A pointer to the CBOR reader positioned at the field value. This parameter
 *                   must not be NULL.
 * \param[out] outputs On success, points to the decoded transaction output list. The caller owns
 *                     the returned reference and must release it. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value was decoded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_body_read_outputs(cardano_cbor_reader_t* reader, cardano_transaction_output_list_t** outputs);

/**
 * \brief Reads the wire form of a body time to live field value.
 *
 * This function decodes an unsigned integer from the given CBOR reader and stores it in \p ttl.
 * The map key is expected to have already been consumed by the caller.
 *
 * \param[in] reader A pointer to the CBOR reader positioned at the field value. This parameter
 *                   must not be NULL.
 * \param[out] ttl A pointer to the storage that receives the decoded value. This parameter must
 *                 not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value was decoded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_body_read_ttl(cardano_cbor_reader_t* reader, uint64_t* ttl);

/**
 * \brief Reads the wire form of a body certificates field value.
 *
 * This function decodes a certificate set from the given CBOR reader. The map key is expected
 * to have already been consumed by the caller.
 *
 * \param[in] reader A pointer to the CBOR reader positioned at the field value. This parameter
 *                   must not be NULL.
 * \param[out] certificates On success, points to the decoded certificate set. The caller owns
 *                          the returned reference and must release it. This parameter must not
 *                          be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value was decoded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_body_read_certificates(cardano_cbor_reader_t* reader, cardano_certificate_set_t** certificates);

/**
 * \brief Reads the wire form of a body withdrawals field value.
 *
 * This function decodes a withdrawal map from the given CBOR reader. The map key is expected
 * to have already been consumed by the caller.
 *
 * \param[in] reader A pointer to the CBOR reader positioned at the field value. This parameter
 *                   must not be NULL.
 * \param[out] withdrawals On success, points to the decoded withdrawal map. The caller owns the
 *                         returned reference and must release it. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value was decoded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_body_read_withdrawals(cardano_cbor_reader_t* reader, cardano_withdrawal_map_t** withdrawals);

/**
 * \brief Reads the wire form of a body auxiliary data hash field value.
 *
 * This function decodes a blake2b hash from the given CBOR reader. The map key is expected to
 * have already been consumed by the caller.
 *
 * \param[in] reader A pointer to the CBOR reader positioned at the field value. This parameter
 *                   must not be NULL.
 * \param[out] aux_data_hash On success, points to the decoded hash. The caller owns the returned
 *                           reference and must release it. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value was decoded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_body_read_aux_data_hash(cardano_cbor_reader_t* reader, cardano_blake2b_hash_t** aux_data_hash);

/**
 * \brief Reads the wire form of a body validity start interval field value.
 *
 * This function decodes an unsigned integer from the given CBOR reader and stores it in
 * \p validity_start. The map key is expected to have already been consumed by the caller.
 *
 * \param[in] reader A pointer to the CBOR reader positioned at the field value. This parameter
 *                   must not be NULL.
 * \param[out] validity_start A pointer to the storage that receives the decoded value. This
 *                            parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value was decoded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_body_read_validity_start(cardano_cbor_reader_t* reader, uint64_t* validity_start);

/**
 * \brief Reads the wire form of a body mint field value.
 *
 * This function decodes a multi asset from the given CBOR reader. The map key is expected to
 * have already been consumed by the caller.
 *
 * \param[in] reader A pointer to the CBOR reader positioned at the field value. This parameter
 *                   must not be NULL.
 * \param[out] mint On success, points to the decoded multi asset. The caller owns the returned
 *                  reference and must release it. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value was decoded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_body_read_mint(cardano_cbor_reader_t* reader, cardano_multi_asset_t** mint);

/**
 * \brief Reads the wire form of a body script data hash field value.
 *
 * This function decodes a blake2b hash from the given CBOR reader. The map key is expected to
 * have already been consumed by the caller.
 *
 * \param[in] reader A pointer to the CBOR reader positioned at the field value. This parameter
 *                   must not be NULL.
 * \param[out] script_data_hash On success, points to the decoded hash. The caller owns the
 *                              returned reference and must release it. This parameter must not
 *                              be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value was decoded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_body_read_script_data_hash(cardano_cbor_reader_t* reader, cardano_blake2b_hash_t** script_data_hash);

/**
 * \brief Reads the wire form of a body guards field value.
 *
 * This function decodes a guard set from the given CBOR reader. The map key is expected to have
 * already been consumed by the caller.
 *
 * \param[in] reader A pointer to the CBOR reader positioned at the field value. This parameter
 *                   must not be NULL.
 * \param[out] guards On success, points to the decoded guard set. The caller owns the returned
 *                    reference and must release it. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value was decoded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_body_read_guards(cardano_cbor_reader_t* reader, cardano_guard_set_t** guards);

/**
 * \brief Reads the wire form of a body network id field value.
 *
 * This function decodes an unsigned integer from the given CBOR reader and stores it in the
 * storage pointed to by \p network_id. The map key is expected to have already been consumed by
 * the caller.
 *
 * \param[in] reader A pointer to the CBOR reader positioned at the field value. This parameter
 *                   must not be NULL.
 * \param[out] network_id A pointer to the storage that receives the decoded value. The storage
 *                        must be at least the size of an unsigned 64 bit integer. This parameter
 *                        must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value was decoded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_body_read_network_id(cardano_cbor_reader_t* reader, cardano_network_id_t* network_id);

/**
 * \brief Reads the wire form of a body reference inputs field value.
 *
 * This function decodes a transaction input set from the given CBOR reader. The map key is
 * expected to have already been consumed by the caller.
 *
 * \param[in] reader A pointer to the CBOR reader positioned at the field value. This parameter
 *                   must not be NULL.
 * \param[out] reference_inputs On success, points to the decoded transaction input set. The
 *                              caller owns the returned reference and must release it. This
 *                              parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value was decoded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_body_read_reference_inputs(cardano_cbor_reader_t* reader, cardano_transaction_input_set_t** reference_inputs);

/**
 * \brief Reads the wire form of a body voting procedures field value.
 *
 * This function decodes a voting procedures object from the given CBOR reader. The map key is
 * expected to have already been consumed by the caller.
 *
 * \param[in] reader A pointer to the CBOR reader positioned at the field value. This parameter
 *                   must not be NULL.
 * \param[out] voting_procedures On success, points to the decoded voting procedures. The caller
 *                               owns the returned reference and must release it. This parameter
 *                               must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value was decoded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_body_read_voting_procedures(cardano_cbor_reader_t* reader, cardano_voting_procedures_t** voting_procedures);

/**
 * \brief Reads the wire form of a body proposal procedures field value.
 *
 * This function decodes a proposal procedure set from the given CBOR reader. The map key is
 * expected to have already been consumed by the caller.
 *
 * \param[in] reader A pointer to the CBOR reader positioned at the field value. This parameter
 *                   must not be NULL.
 * \param[out] proposal_procedures On success, points to the decoded proposal procedure set. The
 *                                 caller owns the returned reference and must release it. This
 *                                 parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value was decoded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_body_read_proposal_procedures(cardano_cbor_reader_t* reader, cardano_proposal_procedure_set_t** proposal_procedures);

/**
 * \brief Reads the wire form of a body treasury value field value.
 *
 * This function decodes an unsigned integer from the given CBOR reader and stores it in
 * \p treasury_value. The map key is expected to have already been consumed by the caller.
 *
 * \param[in] reader A pointer to the CBOR reader positioned at the field value. This parameter
 *                   must not be NULL.
 * \param[out] treasury_value A pointer to the storage that receives the decoded value. This
 *                            parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value was decoded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_body_read_treasury_value(cardano_cbor_reader_t* reader, uint64_t* treasury_value);

/**
 * \brief Reads the wire form of a body donation field value.
 *
 * This function decodes an unsigned integer from the given CBOR reader and stores it in
 * \p donation. The map key is expected to have already been consumed by the caller.
 *
 * \param[in] reader A pointer to the CBOR reader positioned at the field value. This parameter
 *                   must not be NULL.
 * \param[out] donation A pointer to the storage that receives the decoded value. This parameter
 *                      must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value was decoded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_body_read_donation(cardano_cbor_reader_t* reader, uint64_t* donation);

/**
 * \brief Reads the wire form of a body required top level guards field value.
 *
 * This function decodes a required guards map from the given CBOR reader. The map key is
 * expected to have already been consumed by the caller.
 *
 * \param[in] reader A pointer to the CBOR reader positioned at the field value. This parameter
 *                   must not be NULL.
 * \param[out] required_top_level_guards On success, points to the decoded required guards map.
 *                                       The caller owns the returned reference and must release
 *                                       it. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value was decoded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_body_read_required_top_level_guards(cardano_cbor_reader_t* reader, cardano_required_guards_map_t** required_top_level_guards);

/**
 * \brief Reads the wire form of a body direct deposits field value.
 *
 * This function decodes a direct deposit map from the given CBOR reader. The map key is expected
 * to have already been consumed by the caller.
 *
 * \param[in] reader A pointer to the CBOR reader positioned at the field value. This parameter
 *                   must not be NULL.
 * \param[out] direct_deposits On success, points to the decoded direct deposit map. The caller
 *                             owns the returned reference and must release it. This parameter
 *                             must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value was decoded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_body_read_direct_deposits(cardano_cbor_reader_t* reader, cardano_direct_deposit_map_t** direct_deposits);

/**
 * \brief Reads the wire form of a body account balance intervals field value.
 *
 * This function decodes an account balance intervals map from the given CBOR reader. The map key
 * is expected to have already been consumed by the caller.
 *
 * \param[in] reader A pointer to the CBOR reader positioned at the field value. This parameter
 *                   must not be NULL.
 * \param[out] account_balance_intervals On success, points to the decoded account balance
 *                                       intervals map. The caller owns the returned reference and
 *                                       must release it. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value was decoded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_body_read_account_balance_intervals(cardano_cbor_reader_t* reader, cardano_account_balance_intervals_map_t** account_balance_intervals);

/**
 * \brief Writes a body inputs field as a key value pair when the value is present.
 *
 * This function writes map key 0 followed by the CBOR encoding of \p inputs to the given writer.
 * If \p inputs is NULL, nothing is written and the function succeeds.
 *
 * \param[in] writer A pointer to the CBOR writer. This parameter must not be NULL.
 * \param[in] inputs A pointer to the transaction input set to write. If NULL, nothing is written.
 *
 * \return \ref CARDANO_SUCCESS if the field was written or absent, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_body_write_inputs_if_present(cardano_cbor_writer_t* writer, const cardano_transaction_input_set_t* inputs);

/**
 * \brief Writes a body outputs field as a key value pair when the value is present.
 *
 * This function writes map key 1 followed by the CBOR encoding of \p outputs to the given writer.
 * If \p outputs is NULL, nothing is written and the function succeeds.
 *
 * \param[in] writer A pointer to the CBOR writer. This parameter must not be NULL.
 * \param[in] outputs A pointer to the transaction output list to write. If NULL, nothing is written.
 *
 * \return \ref CARDANO_SUCCESS if the field was written or absent, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_body_write_outputs_if_present(cardano_cbor_writer_t* writer, const cardano_transaction_output_list_t* outputs);

/**
 * \brief Writes a body time to live field as a key value pair when the value is present.
 *
 * This function writes map key 3 followed by the CBOR encoding of \p ttl to the given writer.
 * If \p ttl is NULL, nothing is written and the function succeeds.
 *
 * \param[in] writer A pointer to the CBOR writer. This parameter must not be NULL.
 * \param[in] ttl A pointer to the unsigned integer value to write. If NULL, nothing is written.
 *
 * \return \ref CARDANO_SUCCESS if the field was written or absent, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_body_write_ttl_if_present(cardano_cbor_writer_t* writer, const uint64_t* ttl);

/**
 * \brief Writes a body certificates field as a key value pair when the value is present.
 *
 * This function writes map key 4 followed by the CBOR encoding of \p certificates to the given
 * writer. If \p certificates is NULL, nothing is written and the function succeeds.
 *
 * \param[in] writer A pointer to the CBOR writer. This parameter must not be NULL.
 * \param[in] certificates A pointer to the certificate set to write. If NULL, nothing is written.
 *
 * \return \ref CARDANO_SUCCESS if the field was written or absent, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_body_write_certificates_if_present(cardano_cbor_writer_t* writer, const cardano_certificate_set_t* certificates);

/**
 * \brief Writes a body withdrawals field as a key value pair when the value is present.
 *
 * This function writes map key 5 followed by the CBOR encoding of \p withdrawals to the given
 * writer. If \p withdrawals is NULL, nothing is written and the function succeeds.
 *
 * \param[in] writer A pointer to the CBOR writer. This parameter must not be NULL.
 * \param[in] withdrawals A pointer to the withdrawal map to write. If NULL, nothing is written.
 *
 * \return \ref CARDANO_SUCCESS if the field was written or absent, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_body_write_withdrawals_if_present(cardano_cbor_writer_t* writer, const cardano_withdrawal_map_t* withdrawals);

/**
 * \brief Writes a body auxiliary data hash field as a key value pair when the value is present.
 *
 * This function writes map key 7 followed by the CBOR encoding of \p aux_data_hash to the given
 * writer. If \p aux_data_hash is NULL, nothing is written and the function succeeds.
 *
 * \param[in] writer A pointer to the CBOR writer. This parameter must not be NULL.
 * \param[in] aux_data_hash A pointer to the blake2b hash to write. If NULL, nothing is written.
 *
 * \return \ref CARDANO_SUCCESS if the field was written or absent, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_body_write_aux_data_hash_if_present(cardano_cbor_writer_t* writer, const cardano_blake2b_hash_t* aux_data_hash);

/**
 * \brief Writes a body validity start interval field as a key value pair when the value is present.
 *
 * This function writes map key 8 followed by the CBOR encoding of \p validity_start to the given
 * writer. If \p validity_start is NULL, nothing is written and the function succeeds.
 *
 * \param[in] writer A pointer to the CBOR writer. This parameter must not be NULL.
 * \param[in] validity_start A pointer to the unsigned integer value to write. If NULL, nothing is
 *                           written.
 *
 * \return \ref CARDANO_SUCCESS if the field was written or absent, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_body_write_validity_start_if_present(cardano_cbor_writer_t* writer, const uint64_t* validity_start);

/**
 * \brief Writes a body mint field as a key value pair when the value is present.
 *
 * This function writes map key 9 followed by the CBOR encoding of \p mint to the given writer.
 * If \p mint is NULL, nothing is written and the function succeeds.
 *
 * \param[in] writer A pointer to the CBOR writer. This parameter must not be NULL.
 * \param[in] mint A pointer to the multi asset to write. If NULL, nothing is written.
 *
 * \return \ref CARDANO_SUCCESS if the field was written or absent, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_body_write_mint_if_present(cardano_cbor_writer_t* writer, const cardano_multi_asset_t* mint);

/**
 * \brief Writes a body script data hash field as a key value pair when the value is present.
 *
 * This function writes map key 11 followed by the CBOR encoding of \p script_data_hash to the
 * given writer. If \p script_data_hash is NULL, nothing is written and the function succeeds.
 *
 * \param[in] writer A pointer to the CBOR writer. This parameter must not be NULL.
 * \param[in] script_data_hash A pointer to the blake2b hash to write. If NULL, nothing is written.
 *
 * \return \ref CARDANO_SUCCESS if the field was written or absent, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_body_write_script_data_hash_if_present(cardano_cbor_writer_t* writer, const cardano_blake2b_hash_t* script_data_hash);

/**
 * \brief Writes a body guards field as a key value pair when the value is present.
 *
 * This function writes map key 14 followed by the CBOR encoding of \p guards to the given writer.
 * If \p guards is NULL, nothing is written and the function succeeds.
 *
 * \param[in] writer A pointer to the CBOR writer. This parameter must not be NULL.
 * \param[in] guards A pointer to the guard set to write. If NULL, nothing is written.
 *
 * \return \ref CARDANO_SUCCESS if the field was written or absent, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_body_write_guards_if_present(cardano_cbor_writer_t* writer, const cardano_guard_set_t* guards);

/**
 * \brief Writes a body network id field as a key value pair when the value is present.
 *
 * This function writes map key 15 followed by the CBOR encoding of \p network_id to the given
 * writer. If \p network_id is NULL, nothing is written and the function succeeds.
 *
 * \param[in] writer A pointer to the CBOR writer. This parameter must not be NULL.
 * \param[in] network_id A pointer to the network id value to write. If NULL, nothing is written.
 *
 * \return \ref CARDANO_SUCCESS if the field was written or absent, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_body_write_network_id_if_present(cardano_cbor_writer_t* writer, const cardano_network_id_t* network_id);

/**
 * \brief Writes a body reference inputs field as a key value pair when the value is present.
 *
 * This function writes map key 18 followed by the CBOR encoding of \p reference_inputs to the
 * given writer. If \p reference_inputs is NULL, nothing is written and the function succeeds.
 *
 * \param[in] writer A pointer to the CBOR writer. This parameter must not be NULL.
 * \param[in] reference_inputs A pointer to the transaction input set to write. If NULL, nothing
 *                             is written.
 *
 * \return \ref CARDANO_SUCCESS if the field was written or absent, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_body_write_reference_inputs_if_present(cardano_cbor_writer_t* writer, const cardano_transaction_input_set_t* reference_inputs);

/**
 * \brief Writes a body voting procedures field as a key value pair when the value is present.
 *
 * This function writes map key 19 followed by the CBOR encoding of \p voting_procedures to the
 * given writer. If \p voting_procedures is NULL, nothing is written and the function succeeds.
 *
 * \param[in] writer A pointer to the CBOR writer. This parameter must not be NULL.
 * \param[in] voting_procedures A pointer to the voting procedures to write. If NULL, nothing is
 *                              written.
 *
 * \return \ref CARDANO_SUCCESS if the field was written or absent, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_body_write_voting_procedures_if_present(cardano_cbor_writer_t* writer, const cardano_voting_procedures_t* voting_procedures);

/**
 * \brief Writes a body proposal procedures field as a key value pair when the value is present.
 *
 * This function writes map key 20 followed by the CBOR encoding of \p proposal_procedures to the
 * given writer. If \p proposal_procedures is NULL, nothing is written and the function succeeds.
 *
 * \param[in] writer A pointer to the CBOR writer. This parameter must not be NULL.
 * \param[in] proposal_procedures A pointer to the proposal procedure set to write. If NULL,
 *                                nothing is written.
 *
 * \return \ref CARDANO_SUCCESS if the field was written or absent, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_body_write_proposal_procedures_if_present(cardano_cbor_writer_t* writer, const cardano_proposal_procedure_set_t* proposal_procedures);

/**
 * \brief Writes a body treasury value field as a key value pair when the value is present.
 *
 * This function writes map key 21 followed by the CBOR encoding of \p treasury_value to the given
 * writer. If \p treasury_value is NULL, nothing is written and the function succeeds.
 *
 * \param[in] writer A pointer to the CBOR writer. This parameter must not be NULL.
 * \param[in] treasury_value A pointer to the unsigned integer value to write. If NULL, nothing is
 *                           written.
 *
 * \return \ref CARDANO_SUCCESS if the field was written or absent, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_body_write_treasury_value_if_present(cardano_cbor_writer_t* writer, const uint64_t* treasury_value);

/**
 * \brief Writes a body donation field as a key value pair when the value is present.
 *
 * This function writes map key 22 followed by the CBOR encoding of \p donation to the given
 * writer. If \p donation is NULL, nothing is written and the function succeeds.
 *
 * \param[in] writer A pointer to the CBOR writer. This parameter must not be NULL.
 * \param[in] donation A pointer to the unsigned integer value to write. If NULL, nothing is
 *                     written.
 *
 * \return \ref CARDANO_SUCCESS if the field was written or absent, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_body_write_donation_if_present(cardano_cbor_writer_t* writer, const uint64_t* donation);

/**
 * \brief Writes a body required top level guards field as a key value pair when the value is present.
 *
 * This function writes map key 24 followed by the CBOR encoding of \p required_top_level_guards
 * to the given writer. If \p required_top_level_guards is NULL, nothing is written and the
 * function succeeds.
 *
 * \param[in] writer A pointer to the CBOR writer. This parameter must not be NULL.
 * \param[in] required_top_level_guards A pointer to the required guards map to write. If NULL,
 *                                      nothing is written.
 *
 * \return \ref CARDANO_SUCCESS if the field was written or absent, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_body_write_required_top_level_guards_if_present(cardano_cbor_writer_t* writer, const cardano_required_guards_map_t* required_top_level_guards);

/**
 * \brief Writes a body direct deposits field as a key value pair when the value is present.
 *
 * This function writes map key 25 followed by the CBOR encoding of \p direct_deposits to the
 * given writer. If \p direct_deposits is NULL, nothing is written and the function succeeds.
 *
 * \param[in] writer A pointer to the CBOR writer. This parameter must not be NULL.
 * \param[in] direct_deposits A pointer to the direct deposit map to write. If NULL, nothing is
 *                            written.
 *
 * \return \ref CARDANO_SUCCESS if the field was written or absent, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_body_write_direct_deposits_if_present(cardano_cbor_writer_t* writer, const cardano_direct_deposit_map_t* direct_deposits);

/**
 * \brief Writes a body account balance intervals field as a key value pair when the value is present.
 *
 * This function writes map key 26 followed by the CBOR encoding of \p account_balance_intervals
 * to the given writer. If \p account_balance_intervals is NULL, nothing is written and the
 * function succeeds.
 *
 * \param[in] writer A pointer to the CBOR writer. This parameter must not be NULL.
 * \param[in] account_balance_intervals A pointer to the account balance intervals map to write.
 *                                      If NULL, nothing is written.
 *
 * \return \ref CARDANO_SUCCESS if the field was written or absent, or an appropriate error code
 *         indicating the failure reason.
 */
cardano_error_t
cardano_body_write_account_balance_intervals_if_present(cardano_cbor_writer_t* writer, const cardano_account_balance_intervals_map_t* account_balance_intervals);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BODY_FIELDS_H
