/**
 * \file unique_signers.h
 *
 * \author angel.castillo
 * \date   Nov 07, 2024
 *
 * Copyright 2024 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SIGNERS_COUNT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SIGNERS_COUNT_H

/* INCLUDES ******************************************************************/

#include <cardano/common/utxo_list.h>
#include <cardano/error.h>
#include <cardano/transaction/transaction.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Checks if a specified public key hash exists within a Cardano Blake2b hash set.
 *
 * This function determines whether a given public key hash is present in the specified `set`
 * of Blake2b hashes. It is typically used to verify if a particular signer’s hash has already
 * been added to a collection of required or unique signers.
 *
 * \param[in] set A pointer to an initialized \ref cardano_blake2b_hash_set_t object representing the set
 *                of hashes to search. This parameter is required and must not be NULL.
 * \param[in] hash A pointer to a \ref cardano_blake2b_hash_t object representing the hash to search for within `set`.
 *                 This parameter is required and must not be NULL.
 *
 * \return A boolean value indicating the presence of the specified hash in the set:
 *         - `true` if the hash exists in `set`
 *         - `false` if the hash does not exist in `set` or if an error occurs
 */
bool
_cardano_blake2b_hash_set_has(
  cardano_blake2b_hash_set_t*   set,
  const cardano_blake2b_hash_t* hash);

/**
 * \brief Adds public key hashes from a set of required signers to the unique signers set.
 *
 * This function iterates through a set of required public key hashes (signers) and adds each unique hash
 * to the specified `unique_signers` set.
 *
 * \param[in,out] unique_signers A pointer to an initialized \ref cardano_blake2b_hash_set_t object where the required
 *                               public key hashes will be added. This parameter must not be NULL.
 * \param[in] required_signers A pointer to an initialized \ref cardano_blake2b_hash_set_t object representing the set
 *                             of required public key hashes to be added to `unique_signers`. This parameter is required
 *                             and must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if all required signers
 *         were successfully added to `unique_signers`, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if required inputs (`unique_signers` or `required_signers`) are NULL.
 */
cardano_error_t
_cardano_add_required_signers(
  cardano_blake2b_hash_set_t* unique_signers,
  cardano_blake2b_hash_set_t* required_signers);

/**
 * \brief Retrieves the public key hash associated with the payment credential of a Cardano address.
 *
 * This function extracts the public key hash from the payment credential of the specified Cardano address,
 * if the address contains a payment credential represented by a key hash.
 *
 * \param[in] address A pointer to an initialized \ref cardano_address_t object representing the Cardano address
 *                    from which the payment public key hash will be retrieved. This parameter is required and must not be NULL.
 *
 * \return A pointer to a \ref cardano_blake2b_hash_t object containing the public key hash associated with the payment
 *         credential, or NULL if the address does not contain a valid payment key hash credential or if an error occurs.
 *         The caller is responsible for managing the lifecycle of this object. Specifically, the caller must release it by
 *         calling \ref cardano_blake2b_hash_unref once it is no longer needed.
 */
cardano_blake2b_hash_t*
_cardano_get_payment_pub_key_hash(cardano_address_t* address);

/**
 * \brief Adds unique public key hashes required for a set of Cardano transaction inputs.
 *
 * This function processes a set of Cardano transaction inputs and their corresponding resolved UTXOs,
 * extracts the public key hashes required for each input, and adds these hashes to the provided `unique_signers` set.
 * Each input may require authorization from different signers, identified by their public key hashes.
 *
 * \param[in,out] unique_signers A pointer to an initialized \ref cardano_blake2b_hash_set_t object where the unique
 *                               public key hashes required for the transaction inputs will be added. This parameter must not be NULL.
 * \param[in] set A pointer to an initialized \ref cardano_transaction_input_set_t object representing the set of transaction
 *                inputs to be processed. This parameter is required and must not be NULL.
 * \param[in] resolved_inputs A pointer to an initialized \ref cardano_utxo_list_t object containing the list of resolved UTXOs
 *                            (inputs) corresponding to the transaction inputs. This parameter is required and must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the public key hashes
 *         were successfully extracted and added to `unique_signers`, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if required inputs (`unique_signers`, `set`, or `resolved_inputs`) are NULL.
 */
cardano_error_t
_cardano_add_input_signers(
  cardano_blake2b_hash_set_t*      unique_signers,
  cardano_transaction_input_set_t* set,
  cardano_utxo_list_t*             resolved_inputs);

/**
 * \brief Adds unique public key hashes required for a set of Cardano withdrawals.
 *
 * This function processes a set of Cardano withdrawals, extracts the public key hashes associated
 * with each withdrawal address, and adds these hashes to the provided `unique_signers` set. Each withdrawal
 * may require authorization from different signers, identified by their public key hashes.
 *
 * \param[in,out] unique_signers A pointer to an initialized \ref cardano_blake2b_hash_set_t object where the unique
 *                               public key hashes required for the withdrawals will be added. This parameter must not be NULL.
 * \param[in] withdrawals A pointer to an initialized \ref cardano_withdrawal_map_t object representing the set of withdrawals
 *                        to be processed. This parameter is required and must not be NULL. If the map is empty, the function
 *                        will complete successfully without modifying `unique_signers`.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the public key hashes
 *         were successfully extracted and added to `unique_signers`, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if required inputs (`unique_signers` or `withdrawals`) are NULL.
 */
cardano_error_t
_cardano_add_withdrawals(
  cardano_blake2b_hash_set_t* unique_signers,
  cardano_withdrawal_map_t*   withdrawals);

/**
 * \brief Processes a Cardano credential by extracting and adding its unique public key hash.
 *
 * This function processes a given Cardano credential, extracts the public key hash associated
 * with it if it represents a key hash credential type, and adds this hash to the provided `unique_signers` set.
 * This is commonly used in processing various certificate types where credentials are involved.
 *
 * \param[in,out] unique_signers A pointer to an initialized \ref cardano_blake2b_hash_set_t object where the unique
 *                               public key hash required for the credential will be added. This parameter must not be NULL.
 * \param[in] credential A pointer to an initialized \ref cardano_credential_t object representing the credential
 *                       to be processed. This parameter is required and must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the public key hash
 *         was successfully extracted and added to `unique_signers`, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if required inputs (`unique_signers` or `credential`) are NULL.
 */
cardano_error_t
_process_credential(
  cardano_blake2b_hash_set_t* unique_signers,
  cardano_credential_t*       credential);

/**
 * \brief Processes a pool registration certificate by extracting and adding its unique public key hashes.
 *
 * This function processes a Cardano pool registration certificate, extracts the public key hashes associated
 * with the pool owners, and adds these hashes to the provided `unique_signers` set. This certificate type
 * represents the registration of a staking pool within the Cardano network, and multiple owners may be associated
 * with a single pool.
 *
 * \param[in,out] unique_signers A pointer to an initialized \ref cardano_blake2b_hash_set_t object where the unique
 *                               public key hashes required for the pool registration certificate will be added.
 *                               This parameter must not be NULL.
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object representing the pool registration
 *                        certificate to be processed. This parameter is required and must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the public key hashes
 *         were successfully extracted and added to `unique_signers`, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if required inputs (`unique_signers` or `certificate`) are NULL.
 */
cardano_error_t
_process_pool_registration(
  cardano_blake2b_hash_set_t* unique_signers,
  cardano_certificate_t*      certificate);

/**
 * \brief Processes a pool retirement certificate by extracting and adding its unique public key hash.
 *
 * This function processes a Cardano pool retirement certificate, extracts the public key hash associated
 * with the retiring pool, and adds this hash to the provided `unique_signers` set. This certificate type
 * represents the retirement of a staking pool within the Cardano network.
 *
 * \param[in,out] unique_signers A pointer to an initialized \ref cardano_blake2b_hash_set_t object where the unique
 *                               public key hash required for the pool retirement certificate will be added.
 *                               This parameter must not be NULL.
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object representing the pool retirement
 *                        certificate to be processed. This parameter is required and must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the public key hash
 *         was successfully extracted and added to `unique_signers`, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if required inputs (`unique_signers` or `certificate`) are NULL.
 */
cardano_error_t
_process_pool_retirement(
  cardano_blake2b_hash_set_t* unique_signers,
  cardano_certificate_t*      certificate);

/**
 * \brief Processes an authorization committee hot certificate by extracting and adding its unique public key hash.
 *
 * This function processes a Cardano authorization committee hot certificate, extracts the public key hash associated
 * with it, and adds this hash to the provided `unique_signers` set.
 *
 * \param[in,out] unique_signers A pointer to an initialized \ref cardano_blake2b_hash_set_t object where the unique
 *                               public key hash required for the authorization committee hot certificate will be added.
 *                               This parameter must not be NULL.
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object representing the authorization
 *                        committee hot certificate to be processed. This parameter is required and must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the public key hash
 *         was successfully extracted and added to `unique_signers`, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if required inputs (`unique_signers` or `certificate`) are NULL.
 */
cardano_error_t
_process_auth_committee_hot(
  cardano_blake2b_hash_set_t* unique_signers,
  cardano_certificate_t*      certificate);

/**
 * \brief Processes a Cardano certificate by extracting and adding the unique public key hash required
 * for a given certificate type.
 *
 * This function examines a specific Cardano certificate of a given type, extracts the public key hash associated
 * with it (if applicable), and adds this hash to the provided `unique_signers` set.
 *
 * \param[in,out] unique_signers A pointer to an initialized \ref cardano_blake2b_hash_set_t object where the unique
 *                               public key hash required for the certificate will be added. This parameter must not be NULL.
 * \param[in] certificate A pointer to an initialized \ref cardano_certificate_t object representing the certificate
 *                        to be processed. This parameter is required and must not be NULL.
 * \param[in] type The type of the certificate (\ref cardano_cert_type_t) to determine the specific processing logic.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the public key hash
 *         was successfully extracted and added to `unique_signers`, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if required inputs (`unique_signers` or `certificate`) are NULL.
 */
cardano_error_t
_process_certificate_with_credential(
  cardano_blake2b_hash_set_t* unique_signers,
  cardano_certificate_t*      certificate,
  cardano_cert_type_t         type);

/**
 * \brief Adds the unique set of public key hashes required for a set of Cardano certificates.
 *
 * This function iterates through a set of Cardano certificates and extracts the unique public key
 * hashes required to authorize each certificate. These hashes are added to the specified `unique_signers` set.
 *
 * \param[in,out] unique_signers A pointer to an initialized \ref cardano_blake2b_hash_set_t object where the unique
 *                               public key hashes required for the certificates will be added. This parameter must
 *                               not be NULL. The caller is responsible for managing this object and ensuring it is
 *                               correctly initialized before calling this function.
 * \param[in] certificates A pointer to an initialized \ref cardano_certificate_set_t object representing the set of
 *                         certificates to be analyzed. This parameter is required and must not be NULL. If the set
 *                         is empty, the function will complete successfully without modifying `unique_signers`.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the unique signers
 *         were successfully added to `unique_signers`, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if required inputs (`unique_signers` or `certificates`) are NULL.
 */
cardano_error_t
_cardano_add_certificates_pub_key_hashes(
  cardano_blake2b_hash_set_t* unique_signers,
  cardano_certificate_set_t*  certificates);

/**
 * \brief Extracts the unique set of public key hashes required for a set of Cardano voting procedures.
 *
 * This function processes a given set of voting procedures and adds the unique public key hashes
 * (representing the signers) required for each procedure to the specified `unique_signers` set.
 *
 * \param[in,out] unique_signers A pointer to an initialized \ref cardano_blake2b_hash_set_t object where the unique
 *                               public key hashes required for the voting procedures will be added. This parameter must
 *                               not be NULL. The caller is responsible for managing this object and ensuring it is
 *                               correctly initialized before calling this function.
 * \param[in] procedures A pointer to an initialized \ref cardano_voting_procedures_t object representing the set of
 *                       voting procedures to be analyzed. This parameter is required and must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the unique signers
 *         were successfully added to `unique_signers`, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if required inputs (`unique_signers` or `procedures`) are NULL.
 */
cardano_error_t
_cardano_voting_procedures_pub_key_hashes(
  cardano_blake2b_hash_set_t*  unique_signers,
  cardano_voting_procedures_t* procedures);

/**
 * \brief Extracts the unique set of public key hashes (signers) required to sign a Cardano transaction.
 *
 * This function computes the unique set of signers for a given Cardano transaction by analyzing the transaction body
 * and resolved inputs. This set is represented as a `cardano_blake2b_hash_set_t` containing the hashes of public keys
 * that are required to authorize the transaction.
 *
 * \param[in] tx A pointer to an initialized \ref cardano_transaction_t object that represents the Cardano transaction
 *               for which the unique signers are to be determined. This parameter is required and must not be NULL.
 * \param[in] resolved_inputs A pointer to an initialized \ref cardano_utxo_list_t object containing the list of resolved UTXOs
 *                            (inputs) referenced in the transaction. This parameter is required and must not be NULL.
 * \param[out] unique_signers On successful execution, this will point to a newly created \ref cardano_blake2b_hash_set_t object
 *                            containing the unique public key hashes required to authorize the transaction. The caller is responsible
 *                            for managing the lifecycle of this object. Specifically, the caller must release it by calling
 *                            \ref cardano_blake2b_hash_set_unref once it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the unique signers were
 *         successfully computed, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if required inputs (tx or resolved_inputs) are NULL.
 */
cardano_error_t
_cardano_get_unique_signers(
  cardano_transaction_t*       tx,
  cardano_utxo_list_t*         resolved_inputs,
  cardano_blake2b_hash_set_t** unique_signers);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SIGNERS_COUNT_H