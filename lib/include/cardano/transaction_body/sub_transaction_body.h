/**
 * \file sub_transaction_body.h
 *
 * \author angel.castillo
 * \date   Jul 17, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SUB_TRANSACTION_BODY_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SUB_TRANSACTION_BODY_H

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
#include <cardano/typedefs.h>
#include <cardano/voting_procedures/voting_procedures.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The sub transaction body encapsulates the core details of a sub transaction.
 *
 * A sub transaction is a transaction nested inside a top-level transaction. Its body
 * mirrors the top-level transaction body but carries no fee (the top-level transaction
 * pays for the whole batch), no collateral fields and no further nesting. In addition
 * to the shared fields, a sub transaction body may declare guards, required top-level
 * guards, direct deposits and account balance intervals.
 */
typedef struct cardano_sub_transaction_body_t cardano_sub_transaction_body_t;

/**
 * \brief Creates and initializes a new instance of a Cardano sub transaction body.
 *
 * This function allocates and initializes a new \ref cardano_sub_transaction_body_t object.
 *
 * \param[in] inputs A pointer to an initialized \ref cardano_transaction_input_set_t object
 *                   representing the set of transaction inputs. This parameter must not be NULL.
 * \param[in] outputs A pointer to an initialized \ref cardano_transaction_output_list_t object
 *                    representing the list of transaction outputs. This parameter must not be NULL.
 * \param[in] ttl An optional pointer to a uint64_t representing the time-to-live (TTL) parameter, specified as a slot number.
 *                The sub transaction will be invalid if not included in a block by the time this slot is reached.
 *                If no TTL is required, this pointer can be set to NULL.
 * \param[out] sub_transaction_body On successful initialization, this will point to a newly created
 *             \ref cardano_sub_transaction_body_t object. This object represents a "strong reference"
 *             to the sub transaction body, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object. Specifically,
 *             once the sub transaction body is no longer needed, the caller must release it by calling
 *             \ref cardano_sub_transaction_body_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the sub transaction body was successfully created, or an appropriate error code indicating
 *         the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the required input pointers (inputs, outputs) are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_input_set_t* inputs = ...;  // Assume inputs are initialized
 * cardano_transaction_output_list_t* outputs = ...;  // Assume outputs are initialized
 * uint64_t ttl_value = 100000;  // Optional TTL for the sub transaction
 * cardano_sub_transaction_body_t* sub_transaction_body = NULL;
 *
 * // Pass the TTL as a pointer
 * cardano_error_t result = cardano_sub_transaction_body_new(inputs, outputs, &ttl_value, &sub_transaction_body);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the sub transaction body
 *
 *   // Free resources when done
 *   cardano_sub_transaction_body_unref(&sub_transaction_body);
 * }
 * else
 * {
 *   printf("Failed to create the sub transaction body: %s\n", cardano_error_to_string(result));
 * }
 *
 * // If no TTL is required, set the ttl pointer to NULL
 * cardano_error_t no_ttl_result = cardano_sub_transaction_body_new(inputs, outputs, NULL, &sub_transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_new(
  cardano_transaction_input_set_t*   inputs,
  cardano_transaction_output_list_t* outputs,
  const uint64_t*                    ttl,
  cardano_sub_transaction_body_t**   sub_transaction_body);

/**
 * \brief Creates a \ref cardano_sub_transaction_body_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_sub_transaction_body_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a sub transaction body.
 *
 * The map keys admitted at the sub transaction level are 0 (inputs), 1 (outputs), 3 (ttl), 4 (certificates),
 * 5 (withdrawals), 7 (auxiliary data hash), 8 (validity interval start), 9 (mint), 11 (script data hash),
 * 14 (guards), 15 (network id), 18 (reference inputs), 19 (voting procedures), 20 (proposal procedures),
 * 21 (current treasury value), 22 (donation), 24 (required top level guards), 25 (direct deposits) and
 * 26 (account balance intervals). Keys 0 and 1 are required; decoding fails with \ref CARDANO_ERROR_DECODING
 * if either is absent. Any other key, including the top-level-only keys 2 (fee), 13 (collateral),
 * 16 (collateral return), 17 (total collateral) and 23 (sub transactions), is rejected with
 * \ref CARDANO_ERROR_INVALID_CBOR_MAP_KEY.
 *
 * The wire format requires the certificates (key 4), withdrawals (key 5) and mint (key 9) collections to be
 * non-empty; this decoder enforces that constraint, whereas \ref cardano_transaction_body_from_cbor accepts
 * empty collections at those keys for backward compatibility with historical top-level encodings. The donation
 * (key 22) is a positive coin on the wire, but a zero value is accepted on decode, mirroring
 * \ref cardano_transaction_body_from_cbor.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] sub_transaction_body A pointer to a pointer of \ref cardano_sub_transaction_body_t that will be set to the address
 *                        of the newly created sub transaction body object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the sub transaction body was successfully created, or an appropriate error code if an error occurred.
 *
 * \remark In Cardano, transactions are encoded in CBOR, but CBOR allows multiple valid ways to encode the same data. The Cardano blockchain
 *         does not enforce a canonical transaction representation, meaning that if you decode a sub transaction body from CBOR and then re-encode it,
 *         the resulting encoding could be different. This would change the sub transaction id and invalidate any existing signatures.
 *         To prevent this, when a sub transaction body object is created using \ref cardano_sub_transaction_body_from_cbor, it caches the original
 *         CBOR representation internally. When \ref cardano_sub_transaction_body_to_cbor is called, it will output the cached CBOR.
 *         If the cached CBOR representation is not needed, the client can call \ref cardano_sub_transaction_body_clear_cbor_cache after the object has been created.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_sub_transaction_body_t object by calling
 *       \ref cardano_sub_transaction_body_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_sub_transaction_body_t* sub_transaction_body = NULL;
 *
 * cardano_error_t result = cardano_sub_transaction_body_from_cbor(reader, &sub_transaction_body);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the sub_transaction_body
 *
 *   // Once done, ensure to clean up and release the sub_transaction_body
 *   cardano_sub_transaction_body_unref(&sub_transaction_body);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode sub_transaction_body: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_from_cbor(cardano_cbor_reader_t* reader, cardano_sub_transaction_body_t** sub_transaction_body);

/**
 * \brief Serializes a sub transaction body into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_sub_transaction_body_t object using a \ref cardano_cbor_writer_t.
 * If the sub transaction body was created by \ref cardano_sub_transaction_body_from_cbor, the function will use the cached CBOR representation
 * to maintain the original encoding. This avoids potential differences in CBOR encoding which could invalidate signatures.
 * If no cached CBOR is available, the function will serialize the sub transaction body as a map whose keys are
 * emitted in ascending numeric order.
 *
 * \param[in] sub_transaction_body A constant pointer to the \ref cardano_sub_transaction_body_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p sub_transaction_body or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * \remark If the sub transaction body object was created by \ref cardano_sub_transaction_body_from_cbor, the original CBOR encoding
 *         will be cached and reused by this function to prevent round-trip encoding issues that can change the sub transaction id
 *         and invalidate signatures. If the cached CBOR is not available or has been cleared, the function will serialize the
 *         sub transaction body with its map keys in ascending numeric order.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_sub_transaction_body_to_cbor(sub_transaction_body, writer);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     // Use the writer's buffer containing the serialized data
 *   }
 *   else
 *   {
 *     const char* error_message = cardano_cbor_writer_get_last_error(writer);
 *     printf("Serialization failed: %s\n", error_message);
 *   }
 *
 *   cardano_cbor_writer_unref(&writer);
 * }
 *
 * cardano_sub_transaction_body_unref(&sub_transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_sub_transaction_body_to_cbor(
  const cardano_sub_transaction_body_t* sub_transaction_body,
  cardano_cbor_writer_t*                writer);

/**
 * \brief Retrieves the set of transaction inputs from a sub transaction body.
 *
 * This function returns the set of transaction inputs associated with a given \ref cardano_sub_transaction_body_t object.
 * These inputs represent the UTXOs (Unspent Transaction Outputs) being spent by the sub transaction.
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_transaction_input_set_t object representing the set of transaction inputs.
 *         The returned object is a new reference, and the caller is responsible for managing its lifecycle.
 *         Specifically, the caller must release it by calling \ref cardano_transaction_input_set_unref when it is no longer needed.
 *         If the \p sub_transaction_body is NULL, the function will return NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is already initialized
 * cardano_transaction_input_set_t* inputs = cardano_sub_transaction_body_get_inputs(sub_transaction_body);
 *
 * if (inputs != NULL)
 * {
 *   // Process the inputs
 *   cardano_transaction_input_set_unref(&inputs); // Ensure to release the inputs when done
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_transaction_input_set_t* cardano_sub_transaction_body_get_inputs(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the transaction inputs for a sub transaction body.
 *
 * This function assigns a set of transaction inputs to a \ref cardano_sub_transaction_body_t object. The inputs represent
 * the UTXOs (Unspent Transaction Outputs) that will be spent by the sub transaction.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 * \param[in] inputs A pointer to an initialized \ref cardano_transaction_input_set_t object representing the inputs to be set.
 *                   This parameter must not be NULL.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the inputs were successfully set.
 *         Returns \ref CARDANO_ERROR_POINTER_IS_NULL if either the \p sub_transaction_body or \p inputs is NULL.
 *
 * \note This function increases the reference count of the \p inputs object; the caller retains ownership of the inputs
 *       and must release their reference to the inputs when no longer needed by calling \ref cardano_transaction_input_set_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_transaction_input_set_t* inputs = ...; // Assume inputs is initialized
 *
 * cardano_error_t result = cardano_sub_transaction_body_set_inputs(sub_transaction_body, inputs);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   printf("Failed to set transaction inputs: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up resources when no longer needed
 * cardano_transaction_input_set_unref(&inputs);
 * cardano_sub_transaction_body_unref(&sub_transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_set_inputs(cardano_sub_transaction_body_t* sub_transaction_body, cardano_transaction_input_set_t* inputs);

/**
 * \brief Retrieves the list of outputs from a sub transaction body.
 *
 * This function returns the list of outputs associated with a given \ref cardano_sub_transaction_body_t object. The outputs represent
 * the recipients and amounts being transferred as part of the sub transaction.
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_transaction_output_list_t object representing the list of transaction outputs. If the
 *         \p sub_transaction_body is NULL, this function returns NULL.
 *         The caller is responsible for managing the lifecycle of the returned list by calling
 *         \ref cardano_transaction_output_list_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_transaction_output_list_t* outputs = cardano_sub_transaction_body_get_outputs(sub_transaction_body);
 *
 * if (outputs != NULL)
 * {
 *   // Use the outputs list
 *   cardano_transaction_output_list_unref(&outputs);
 * }
 *
 * cardano_sub_transaction_body_unref(&sub_transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_transaction_output_list_t* cardano_sub_transaction_body_get_outputs(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the list of outputs for a sub transaction body.
 *
 * This function assigns a new list of outputs to the specified \ref cardano_sub_transaction_body_t object. The outputs represent
 * the recipients and amounts that are being transferred in the sub transaction.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object where the outputs will be set.
 * \param[in] outputs A pointer to an initialized \ref cardano_transaction_output_list_t object representing the list of transaction outputs.
 *                    This value must not be NULL, and it will replace any existing outputs in the \p sub_transaction_body.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the outputs were
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if any of the input pointers are NULL.
 *
 * \note This function increases the reference count of the \p outputs object, so the caller retains ownership of their respective references.
 *       It is the caller's responsibility to release their reference to the outputs when they are no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is already initialized
 * cardano_transaction_output_list_t* outputs = ...; // Assume outputs are initialized
 *
 * cardano_error_t result = cardano_sub_transaction_body_set_outputs(sub_transaction_body, outputs);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   printf("Failed to set transaction outputs.\n");
 * }
 *
 * // Clean up resources when no longer needed
 * cardano_sub_transaction_body_unref(&sub_transaction_body);
 * cardano_transaction_output_list_unref(&outputs);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_set_outputs(cardano_sub_transaction_body_t* sub_transaction_body, cardano_transaction_output_list_t* outputs);

/**
 * \brief Retrieves the "invalid after" value from the sub transaction body.
 *
 * This function fetches the "invalid after" field from a \ref cardano_sub_transaction_body_t object, representing
 * the slot number after which the sub transaction is considered invalid. This field is optional, and the function
 * will return a pointer to a `uint64_t` if the field is set, or `NULL` if it is not.
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 *
 * \return A pointer to a \c uint64_t representing the "invalid after" slot number if set. If this field is not set,
 *         the function will return `NULL`. The returned pointer is managed internally and must not be freed by the caller.
 *         The caller can use the pointer to read the value but not modify or free it.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume initialized
 * const uint64_t* invalid_after = cardano_sub_transaction_body_get_invalid_after(sub_transaction_body);
 *
 * if (invalid_after != NULL)
 * {
 *   printf("Sub transaction is invalid after slot: %llu\n", *invalid_after);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const uint64_t* cardano_sub_transaction_body_get_invalid_after(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the "invalid after" field in the sub transaction body.
 *
 * This function assigns a value to the "invalid after" field of a \ref cardano_sub_transaction_body_t object, specifying
 * the slot number after which the sub transaction becomes invalid. The "invalid after" field is optional, and passing a
 * `NULL` pointer will unset the value, effectively removing the "invalid after" field from the sub transaction.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object where the field will be set.
 * \param[in] slot A constant pointer to a \c uint64_t representing the "invalid after" slot number. If `NULL`, this will unset the field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the field was successfully set or unset,
 *         or an appropriate error code if an error occurred, such as \ref CARDANO_ERROR_POINTER_IS_NULL if \p sub_transaction_body is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume initialized
 * uint64_t slot = 123456; // Slot number
 *
 * cardano_error_t result = cardano_sub_transaction_body_set_invalid_after(sub_transaction_body, &slot);
 *
 * // To unset the invalid after field:
 * result = cardano_sub_transaction_body_set_invalid_after(sub_transaction_body, NULL);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_set_invalid_after(cardano_sub_transaction_body_t* sub_transaction_body, const uint64_t* slot);

/**
 * \brief Retrieves the set of certificates associated with the sub transaction body.
 *
 * This function returns the certificates from a \ref cardano_sub_transaction_body_t object, if they are present.
 * Certificates in Cardano are used for various purposes, such as stake delegation and pool registration.
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_certificate_set_t object representing the set of certificates. The returned object
 *         is a new reference, and the caller is responsible for releasing it by calling \ref cardano_certificate_set_unref
 *         when it is no longer needed. If the \p sub_transaction_body is NULL or has no certificates, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_certificate_set_t* certificates = cardano_sub_transaction_body_get_certificates(sub_transaction_body);
 *
 * if (certificates != NULL)
 * {
 *   // Use the certificates
 *   cardano_certificate_set_unref(&certificates);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_certificate_set_t* cardano_sub_transaction_body_get_certificates(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the certificates for a sub transaction body.
 *
 * This function assigns a set of certificates to a \ref cardano_sub_transaction_body_t object. Passing NULL
 * unsets the certificates, removing them from the sub transaction body.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 * \param[in] certificates A pointer to an initialized \ref cardano_certificate_set_t object, or NULL to unset the field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificates were
 *         successfully set or unset, or \ref CARDANO_ERROR_POINTER_IS_NULL if \p sub_transaction_body is NULL.
 *
 * \note This function increases the reference count of the \p certificates object when it is not NULL; the caller retains
 *       ownership of their reference and must release it when no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_certificate_set_t* certificates = ...; // Assume certificates are initialized
 *
 * cardano_error_t result = cardano_sub_transaction_body_set_certificates(sub_transaction_body, certificates);
 *
 * // Clean up resources when no longer needed
 * cardano_certificate_set_unref(&certificates);
 * cardano_sub_transaction_body_unref(&sub_transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_set_certificates(cardano_sub_transaction_body_t* sub_transaction_body, cardano_certificate_set_t* certificates);

/**
 * \brief Retrieves the withdrawals map from the sub transaction body.
 *
 * This function returns the withdrawals from a \ref cardano_sub_transaction_body_t object, if they are present.
 * Withdrawals map reward accounts to the amount of lovelace being withdrawn from them.
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_withdrawal_map_t object representing the withdrawals. The returned object
 *         is a new reference, and the caller is responsible for releasing it by calling \ref cardano_withdrawal_map_unref
 *         when it is no longer needed. If the \p sub_transaction_body is NULL or has no withdrawals, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_withdrawal_map_t* withdrawals = cardano_sub_transaction_body_get_withdrawals(sub_transaction_body);
 *
 * if (withdrawals != NULL)
 * {
 *   // Use the withdrawals
 *   cardano_withdrawal_map_unref(&withdrawals);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_withdrawal_map_t* cardano_sub_transaction_body_get_withdrawals(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the withdrawals for a sub transaction body.
 *
 * This function assigns a withdrawal map to a \ref cardano_sub_transaction_body_t object. Passing NULL
 * unsets the withdrawals, removing them from the sub transaction body.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 * \param[in] withdrawals A pointer to an initialized \ref cardano_withdrawal_map_t object, or NULL to unset the field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the withdrawals were
 *         successfully set or unset, or \ref CARDANO_ERROR_POINTER_IS_NULL if \p sub_transaction_body is NULL.
 *
 * \note This function increases the reference count of the \p withdrawals object when it is not NULL; the caller retains
 *       ownership of their reference and must release it when no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_withdrawal_map_t* withdrawals = ...; // Assume withdrawals are initialized
 *
 * cardano_error_t result = cardano_sub_transaction_body_set_withdrawals(sub_transaction_body, withdrawals);
 *
 * // Clean up resources when no longer needed
 * cardano_withdrawal_map_unref(&withdrawals);
 * cardano_sub_transaction_body_unref(&sub_transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_set_withdrawals(cardano_sub_transaction_body_t* sub_transaction_body, cardano_withdrawal_map_t* withdrawals);

/**
 * \brief Retrieves the auxiliary data hash from the sub transaction body.
 *
 * This function returns the auxiliary data hash from a \ref cardano_sub_transaction_body_t object, if it is present.
 * The auxiliary data hash commits the sub transaction to its auxiliary data (metadata and auxiliary scripts).
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_blake2b_hash_t object representing the auxiliary data hash. The returned object
 *         is a new reference, and the caller is responsible for releasing it by calling \ref cardano_blake2b_hash_unref
 *         when it is no longer needed. If the \p sub_transaction_body is NULL or has no auxiliary data hash, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_blake2b_hash_t* aux_data_hash = cardano_sub_transaction_body_get_aux_data_hash(sub_transaction_body);
 *
 * if (aux_data_hash != NULL)
 * {
 *   // Use the auxiliary data hash
 *   cardano_blake2b_hash_unref(&aux_data_hash);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_sub_transaction_body_get_aux_data_hash(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the auxiliary data hash for a sub transaction body.
 *
 * This function assigns an auxiliary data hash to a \ref cardano_sub_transaction_body_t object. Passing NULL
 * unsets the auxiliary data hash, removing it from the sub transaction body.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 * \param[in] aux_data_hash A pointer to an initialized \ref cardano_blake2b_hash_t object, or NULL to unset the field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the auxiliary data hash was
 *         successfully set or unset, or \ref CARDANO_ERROR_POINTER_IS_NULL if \p sub_transaction_body is NULL.
 *
 * \note This function increases the reference count of the \p aux_data_hash object when it is not NULL; the caller retains
 *       ownership of their reference and must release it when no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_blake2b_hash_t* aux_data_hash = ...; // Assume aux_data_hash is initialized
 *
 * cardano_error_t result = cardano_sub_transaction_body_set_aux_data_hash(sub_transaction_body, aux_data_hash);
 *
 * // Clean up resources when no longer needed
 * cardano_blake2b_hash_unref(&aux_data_hash);
 * cardano_sub_transaction_body_unref(&sub_transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_set_aux_data_hash(cardano_sub_transaction_body_t* sub_transaction_body, cardano_blake2b_hash_t* aux_data_hash);

/**
 * \brief Retrieves the "invalid before" value from the sub transaction body.
 *
 * This function fetches the "invalid before" field from a \ref cardano_sub_transaction_body_t object, representing
 * the slot number before which the sub transaction is considered invalid. This field is optional, and the function
 * will return a pointer to a `uint64_t` if the field is set, or `NULL` if it is not.
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 *
 * \return A pointer to a \c uint64_t representing the "invalid before" slot number if set. If this field is not set,
 *         the function will return `NULL`. The returned pointer is managed internally and must not be freed by the caller.
 *         The caller can use the pointer to read the value but not modify or free it.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume initialized
 * const uint64_t* invalid_before = cardano_sub_transaction_body_get_invalid_before(sub_transaction_body);
 *
 * if (invalid_before != NULL)
 * {
 *   printf("Sub transaction is invalid before slot: %llu\n", *invalid_before);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const uint64_t* cardano_sub_transaction_body_get_invalid_before(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the "invalid before" field in the sub transaction body.
 *
 * This function assigns a value to the "invalid before" field of a \ref cardano_sub_transaction_body_t object, specifying
 * the slot number before which the sub transaction is invalid. The "invalid before" field is optional, and passing a
 * `NULL` pointer will unset the value, effectively removing the "invalid before" field from the sub transaction.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object where the field will be set.
 * \param[in] slot A constant pointer to a \c uint64_t representing the "invalid before" slot number. If `NULL`, this will unset the field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the field was successfully set or unset,
 *         or an appropriate error code if an error occurred, such as \ref CARDANO_ERROR_POINTER_IS_NULL if \p sub_transaction_body is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume initialized
 * uint64_t slot = 100; // Slot number
 *
 * cardano_error_t result = cardano_sub_transaction_body_set_invalid_before(sub_transaction_body, &slot);
 *
 * // To unset the invalid before field:
 * result = cardano_sub_transaction_body_set_invalid_before(sub_transaction_body, NULL);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_set_invalid_before(cardano_sub_transaction_body_t* sub_transaction_body, const uint64_t* slot);

/**
 * \brief Retrieves the mint field from the sub transaction body.
 *
 * This function returns the multi asset mint field from a \ref cardano_sub_transaction_body_t object, if it is present.
 * The mint field describes the native assets being minted (positive amounts) or burned (negative amounts) by the sub transaction.
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_multi_asset_t object representing the mint field. The returned object
 *         is a new reference, and the caller is responsible for releasing it by calling \ref cardano_multi_asset_unref
 *         when it is no longer needed. If the \p sub_transaction_body is NULL or has no mint field, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_multi_asset_t* mint = cardano_sub_transaction_body_get_mint(sub_transaction_body);
 *
 * if (mint != NULL)
 * {
 *   // Use the mint field
 *   cardano_multi_asset_unref(&mint);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_multi_asset_t* cardano_sub_transaction_body_get_mint(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the mint field for a sub transaction body.
 *
 * This function assigns a multi asset mint field to a \ref cardano_sub_transaction_body_t object. Passing NULL
 * unsets the mint field, removing it from the sub transaction body.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 * \param[in] mint A pointer to an initialized \ref cardano_multi_asset_t object, or NULL to unset the field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the mint field was
 *         successfully set or unset, or \ref CARDANO_ERROR_POINTER_IS_NULL if \p sub_transaction_body is NULL.
 *
 * \note This function increases the reference count of the \p mint object when it is not NULL; the caller retains
 *       ownership of their reference and must release it when no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_multi_asset_t* mint = ...; // Assume mint is initialized
 *
 * cardano_error_t result = cardano_sub_transaction_body_set_mint(sub_transaction_body, mint);
 *
 * // Clean up resources when no longer needed
 * cardano_multi_asset_unref(&mint);
 * cardano_sub_transaction_body_unref(&sub_transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_set_mint(cardano_sub_transaction_body_t* sub_transaction_body, cardano_multi_asset_t* mint);

/**
 * \brief Retrieves the script data hash from the sub transaction body.
 *
 * This function returns the script data hash from a \ref cardano_sub_transaction_body_t object, if it is present.
 * The script data hash commits the sub transaction to its redeemers, datums and the cost models in force.
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_blake2b_hash_t object representing the script data hash. The returned object
 *         is a new reference, and the caller is responsible for releasing it by calling \ref cardano_blake2b_hash_unref
 *         when it is no longer needed. If the \p sub_transaction_body is NULL or has no script data hash, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_blake2b_hash_t* script_data_hash = cardano_sub_transaction_body_get_script_data_hash(sub_transaction_body);
 *
 * if (script_data_hash != NULL)
 * {
 *   // Use the script data hash
 *   cardano_blake2b_hash_unref(&script_data_hash);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_sub_transaction_body_get_script_data_hash(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the script data hash for a sub transaction body.
 *
 * This function assigns a script data hash to a \ref cardano_sub_transaction_body_t object. Passing NULL
 * unsets the script data hash, removing it from the sub transaction body.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 * \param[in] script_data_hash A pointer to an initialized \ref cardano_blake2b_hash_t object, or NULL to unset the field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the script data hash was
 *         successfully set or unset, or \ref CARDANO_ERROR_POINTER_IS_NULL if \p sub_transaction_body is NULL.
 *
 * \note This function increases the reference count of the \p script_data_hash object when it is not NULL; the caller retains
 *       ownership of their reference and must release it when no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_blake2b_hash_t* script_data_hash = ...; // Assume script_data_hash is initialized
 *
 * cardano_error_t result = cardano_sub_transaction_body_set_script_data_hash(sub_transaction_body, script_data_hash);
 *
 * // Clean up resources when no longer needed
 * cardano_blake2b_hash_unref(&script_data_hash);
 * cardano_sub_transaction_body_unref(&sub_transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_set_script_data_hash(cardano_sub_transaction_body_t* sub_transaction_body, cardano_blake2b_hash_t* script_data_hash);

/**
 * \brief Retrieves the guards from the sub transaction body.
 *
 * This function returns the guard set from a \ref cardano_sub_transaction_body_t object, if it is present.
 * Guards are credentials that must authorize the sub transaction, either with a signature (key hash guards)
 * or by running the corresponding script (script hash guards).
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_guard_set_t object representing the guards. The returned object
 *         is a new reference, and the caller is responsible for releasing it by calling \ref cardano_guard_set_unref
 *         when it is no longer needed. If the \p sub_transaction_body is NULL or has no guards, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_guard_set_t* guards = cardano_sub_transaction_body_get_guards(sub_transaction_body);
 *
 * if (guards != NULL)
 * {
 *   // Use the guards
 *   cardano_guard_set_unref(&guards);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_guard_set_t* cardano_sub_transaction_body_get_guards(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the guards for a sub transaction body.
 *
 * This function assigns a guard set to a \ref cardano_sub_transaction_body_t object. Passing NULL
 * unsets the guards, removing them from the sub transaction body.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 * \param[in] guards A pointer to an initialized \ref cardano_guard_set_t object, or NULL to unset the field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the guards were
 *         successfully set or unset, or \ref CARDANO_ERROR_POINTER_IS_NULL if \p sub_transaction_body is NULL.
 *
 * \note This function increases the reference count of the \p guards object when it is not NULL; the caller retains
 *       ownership of their reference and must release it when no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_guard_set_t* guards = ...; // Assume guards are initialized
 *
 * cardano_error_t result = cardano_sub_transaction_body_set_guards(sub_transaction_body, guards);
 *
 * // Clean up resources when no longer needed
 * cardano_guard_set_unref(&guards);
 * cardano_sub_transaction_body_unref(&sub_transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_set_guards(cardano_sub_transaction_body_t* sub_transaction_body, cardano_guard_set_t* guards);

/**
 * \brief Retrieves the network id from the sub transaction body.
 *
 * This function fetches the network id field from a \ref cardano_sub_transaction_body_t object, identifying the
 * network (mainnet or testnet) the sub transaction is intended for. This field is optional, and the function
 * will return a pointer to a \ref cardano_network_id_t if the field is set, or `NULL` if it is not.
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_network_id_t representing the network id if set. If this field is not set,
 *         the function will return `NULL`. The returned pointer is managed internally and must not be freed by the caller.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume initialized
 * const cardano_network_id_t* network_id = cardano_sub_transaction_body_get_network_id(sub_transaction_body);
 *
 * if (network_id != NULL)
 * {
 *   printf("Network id: %d\n", *network_id);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const cardano_network_id_t* cardano_sub_transaction_body_get_network_id(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the network id for a sub transaction body.
 *
 * This function assigns a network id to a \ref cardano_sub_transaction_body_t object. The network id field is optional,
 * and passing a `NULL` pointer will unset the value, effectively removing the network id from the sub transaction.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 * \param[in] network_id A constant pointer to a \ref cardano_network_id_t representing the network id. If `NULL`, this will unset the field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the field was successfully set or unset,
 *         or an appropriate error code if an error occurred, such as \ref CARDANO_ERROR_POINTER_IS_NULL if \p sub_transaction_body is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume initialized
 * cardano_network_id_t network_id = CARDANO_NETWORK_ID_MAIN_NET;
 *
 * cardano_error_t result = cardano_sub_transaction_body_set_network_id(sub_transaction_body, &network_id);
 *
 * // To unset the network id field:
 * result = cardano_sub_transaction_body_set_network_id(sub_transaction_body, NULL);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_set_network_id(cardano_sub_transaction_body_t* sub_transaction_body, const cardano_network_id_t* network_id);

/**
 * \brief Retrieves the set of reference inputs from a sub transaction body.
 *
 * This function returns the reference inputs associated with a given \ref cardano_sub_transaction_body_t object.
 * Reference inputs are UTXOs that the sub transaction can read without spending them.
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_transaction_input_set_t object representing the reference inputs. The returned object
 *         is a new reference, and the caller is responsible for releasing it by calling \ref cardano_transaction_input_set_unref
 *         when it is no longer needed. If the \p sub_transaction_body is NULL or has no reference inputs, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_transaction_input_set_t* reference_inputs = cardano_sub_transaction_body_get_reference_inputs(sub_transaction_body);
 *
 * if (reference_inputs != NULL)
 * {
 *   // Use the reference inputs
 *   cardano_transaction_input_set_unref(&reference_inputs);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_transaction_input_set_t* cardano_sub_transaction_body_get_reference_inputs(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the reference inputs for a sub transaction body.
 *
 * This function assigns a set of reference inputs to a \ref cardano_sub_transaction_body_t object. Passing NULL
 * unsets the reference inputs, removing them from the sub transaction body.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 * \param[in] reference_inputs A pointer to an initialized \ref cardano_transaction_input_set_t object, or NULL to unset the field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the reference inputs were
 *         successfully set or unset, or \ref CARDANO_ERROR_POINTER_IS_NULL if \p sub_transaction_body is NULL.
 *
 * \note This function increases the reference count of the \p reference_inputs object when it is not NULL; the caller retains
 *       ownership of their reference and must release it when no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_transaction_input_set_t* reference_inputs = ...; // Assume reference_inputs is initialized
 *
 * cardano_error_t result = cardano_sub_transaction_body_set_reference_inputs(sub_transaction_body, reference_inputs);
 *
 * // Clean up resources when no longer needed
 * cardano_transaction_input_set_unref(&reference_inputs);
 * cardano_sub_transaction_body_unref(&sub_transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_set_reference_inputs(
  cardano_sub_transaction_body_t*  sub_transaction_body,
  cardano_transaction_input_set_t* reference_inputs);

/**
 * \brief Retrieves the voting procedures from the sub transaction body.
 *
 * This function returns the voting procedures from a \ref cardano_sub_transaction_body_t object, if they are present.
 * Voting procedures record the votes cast by voters on governance actions.
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_voting_procedures_t object representing the voting procedures. The returned object
 *         is a new reference, and the caller is responsible for releasing it by calling \ref cardano_voting_procedures_unref
 *         when it is no longer needed. If the \p sub_transaction_body is NULL or has no voting procedures, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_voting_procedures_t* voting_procedures = cardano_sub_transaction_body_get_voting_procedures(sub_transaction_body);
 *
 * if (voting_procedures != NULL)
 * {
 *   // Use the voting procedures
 *   cardano_voting_procedures_unref(&voting_procedures);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_voting_procedures_t* cardano_sub_transaction_body_get_voting_procedures(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the voting procedures for a sub transaction body.
 *
 * This function assigns voting procedures to a \ref cardano_sub_transaction_body_t object. Passing NULL
 * unsets the voting procedures, removing them from the sub transaction body.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 * \param[in] voting_procedures A pointer to an initialized \ref cardano_voting_procedures_t object, or NULL to unset the field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the voting procedures were
 *         successfully set or unset, or \ref CARDANO_ERROR_POINTER_IS_NULL if \p sub_transaction_body is NULL.
 *
 * \note This function increases the reference count of the \p voting_procedures object when it is not NULL; the caller retains
 *       ownership of their reference and must release it when no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_voting_procedures_t* voting_procedures = ...; // Assume voting_procedures is initialized
 *
 * cardano_error_t result = cardano_sub_transaction_body_set_voting_procedures(sub_transaction_body, voting_procedures);
 *
 * // Clean up resources when no longer needed
 * cardano_voting_procedures_unref(&voting_procedures);
 * cardano_sub_transaction_body_unref(&sub_transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_set_voting_procedures(
  cardano_sub_transaction_body_t* sub_transaction_body,
  cardano_voting_procedures_t*    voting_procedures);

/**
 * \brief Retrieves the proposal procedures from the sub transaction body.
 *
 * This function returns the proposal procedures from a \ref cardano_sub_transaction_body_t object, if they are present.
 * Proposal procedures submit governance actions for the community to vote on.
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_proposal_procedure_set_t object representing the proposal procedures. The returned object
 *         is a new reference, and the caller is responsible for releasing it by calling \ref cardano_proposal_procedure_set_unref
 *         when it is no longer needed. If the \p sub_transaction_body is NULL or has no proposal procedures, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_proposal_procedure_set_t* proposal_procedures = cardano_sub_transaction_body_get_proposal_procedures(sub_transaction_body);
 *
 * if (proposal_procedures != NULL)
 * {
 *   // Use the proposal procedures
 *   cardano_proposal_procedure_set_unref(&proposal_procedures);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_proposal_procedure_set_t* cardano_sub_transaction_body_get_proposal_procedures(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the proposal procedures for a sub transaction body.
 *
 * This function assigns proposal procedures to a \ref cardano_sub_transaction_body_t object. Passing NULL
 * unsets the proposal procedures, removing them from the sub transaction body.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 * \param[in] proposal_procedures A pointer to an initialized \ref cardano_proposal_procedure_set_t object, or NULL to unset the field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the proposal procedures were
 *         successfully set or unset, or \ref CARDANO_ERROR_POINTER_IS_NULL if \p sub_transaction_body is NULL.
 *
 * \note This function increases the reference count of the \p proposal_procedures object when it is not NULL; the caller retains
 *       ownership of their reference and must release it when no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_proposal_procedure_set_t* proposal_procedures = ...; // Assume proposal_procedures is initialized
 *
 * cardano_error_t result = cardano_sub_transaction_body_set_proposal_procedures(sub_transaction_body, proposal_procedures);
 *
 * // Clean up resources when no longer needed
 * cardano_proposal_procedure_set_unref(&proposal_procedures);
 * cardano_sub_transaction_body_unref(&sub_transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_set_proposal_procedures(
  cardano_sub_transaction_body_t*   sub_transaction_body,
  cardano_proposal_procedure_set_t* proposal_procedures);

/**
 * \brief Retrieves the current treasury value from the sub transaction body.
 *
 * This function fetches the current treasury value field from a \ref cardano_sub_transaction_body_t object, an
 * assertion of the current amount of lovelace in the treasury. This field is optional, and the function will
 * return a pointer to a `uint64_t` if the field is set, or `NULL` if it is not.
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 *
 * \return A pointer to a \c uint64_t representing the current treasury value if set. If this field is not set,
 *         the function will return `NULL`. The returned pointer is managed internally and must not be freed by the caller.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume initialized
 * const uint64_t* treasury_value = cardano_sub_transaction_body_get_treasury_value(sub_transaction_body);
 *
 * if (treasury_value != NULL)
 * {
 *   printf("Current treasury value: %llu lovelaces\n", *treasury_value);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const uint64_t* cardano_sub_transaction_body_get_treasury_value(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the current treasury value for a sub transaction body.
 *
 * This function assigns a current treasury value to a \ref cardano_sub_transaction_body_t object. The field is optional,
 * and passing a `NULL` pointer will unset the value, removing it from the sub transaction body.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 * \param[in] treasury_value A constant pointer to a \c uint64_t representing the current treasury value in lovelaces.
 *                           If `NULL`, this will unset the field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the field was successfully set or unset,
 *         or an appropriate error code if an error occurred, such as \ref CARDANO_ERROR_POINTER_IS_NULL if \p sub_transaction_body is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume initialized
 * uint64_t treasury_value = 2000;
 *
 * cardano_error_t result = cardano_sub_transaction_body_set_treasury_value(sub_transaction_body, &treasury_value);
 *
 * // To unset the treasury value field:
 * result = cardano_sub_transaction_body_set_treasury_value(sub_transaction_body, NULL);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_set_treasury_value(cardano_sub_transaction_body_t* sub_transaction_body, const uint64_t* treasury_value);

/**
 * \brief Retrieves the donation from the sub transaction body.
 *
 * This function fetches the donation field from a \ref cardano_sub_transaction_body_t object, the amount of
 * lovelace donated to the treasury. This field is optional, and the function will return a pointer to a
 * `uint64_t` if the field is set, or `NULL` if it is not.
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 *
 * \return A pointer to a \c uint64_t representing the donation if set. If this field is not set,
 *         the function will return `NULL`. The returned pointer is managed internally and must not be freed by the caller.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume initialized
 * const uint64_t* donation = cardano_sub_transaction_body_get_donation(sub_transaction_body);
 *
 * if (donation != NULL)
 * {
 *   printf("Donation: %llu lovelaces\n", *donation);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const uint64_t* cardano_sub_transaction_body_get_donation(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the donation for a sub transaction body.
 *
 * This function assigns a donation to a \ref cardano_sub_transaction_body_t object. On the wire the donation is a
 * positive amount of lovelace donated to the treasury. The field is optional, and passing a `NULL` pointer will
 * unset the value, removing it from the sub transaction body.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 * \param[in] donation A constant pointer to a \c uint64_t representing the donation in lovelaces. If `NULL`, this will unset the field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the field was successfully set or unset,
 *         or an appropriate error code if an error occurred, such as \ref CARDANO_ERROR_POINTER_IS_NULL if \p sub_transaction_body is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume initialized
 * uint64_t donation = 1000;
 *
 * cardano_error_t result = cardano_sub_transaction_body_set_donation(sub_transaction_body, &donation);
 *
 * // To unset the donation field:
 * result = cardano_sub_transaction_body_set_donation(sub_transaction_body, NULL);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_set_donation(cardano_sub_transaction_body_t* sub_transaction_body, const uint64_t* donation);

/**
 * \brief Retrieves the required top level guards from the sub transaction body.
 *
 * This function returns the required top level guards from a \ref cardano_sub_transaction_body_t object, if they are present.
 * The map restricts which guards must be declared by the enclosing top-level transaction, each optionally paired with
 * the datum the guard must be invoked with.
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_required_guards_map_t object representing the required top level guards. The returned object
 *         is a new reference, and the caller is responsible for releasing it by calling \ref cardano_required_guards_map_unref
 *         when it is no longer needed. If the \p sub_transaction_body is NULL or has no required top level guards, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_required_guards_map_t* required_guards = cardano_sub_transaction_body_get_required_top_level_guards(sub_transaction_body);
 *
 * if (required_guards != NULL)
 * {
 *   // Use the required top level guards
 *   cardano_required_guards_map_unref(&required_guards);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_required_guards_map_t* cardano_sub_transaction_body_get_required_top_level_guards(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the required top level guards for a sub transaction body.
 *
 * This function assigns a required guards map to a \ref cardano_sub_transaction_body_t object. Passing NULL
 * unsets the required top level guards, removing them from the sub transaction body.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 * \param[in] required_top_level_guards A pointer to an initialized \ref cardano_required_guards_map_t object, or NULL to unset the field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the required top level guards were
 *         successfully set or unset, or \ref CARDANO_ERROR_POINTER_IS_NULL if \p sub_transaction_body is NULL.
 *
 * \note This function increases the reference count of the \p required_top_level_guards object when it is not NULL; the caller retains
 *       ownership of their reference and must release it when no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_required_guards_map_t* required_guards = ...; // Assume required_guards is initialized
 *
 * cardano_error_t result = cardano_sub_transaction_body_set_required_top_level_guards(sub_transaction_body, required_guards);
 *
 * // Clean up resources when no longer needed
 * cardano_required_guards_map_unref(&required_guards);
 * cardano_sub_transaction_body_unref(&sub_transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_set_required_top_level_guards(
  cardano_sub_transaction_body_t* sub_transaction_body,
  cardano_required_guards_map_t*  required_top_level_guards);

/**
 * \brief Retrieves the direct deposits from the sub transaction body.
 *
 * This function returns the direct deposits from a \ref cardano_sub_transaction_body_t object, if they are present.
 * Direct deposits map reward accounts to the amount of lovelace deposited directly into them.
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_direct_deposit_map_t object representing the direct deposits. The returned object
 *         is a new reference, and the caller is responsible for releasing it by calling \ref cardano_direct_deposit_map_unref
 *         when it is no longer needed. If the \p sub_transaction_body is NULL or has no direct deposits, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_direct_deposit_map_t* direct_deposits = cardano_sub_transaction_body_get_direct_deposits(sub_transaction_body);
 *
 * if (direct_deposits != NULL)
 * {
 *   // Use the direct deposits
 *   cardano_direct_deposit_map_unref(&direct_deposits);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_direct_deposit_map_t* cardano_sub_transaction_body_get_direct_deposits(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the direct deposits for a sub transaction body.
 *
 * This function assigns a direct deposit map to a \ref cardano_sub_transaction_body_t object. Passing NULL
 * unsets the direct deposits, removing them from the sub transaction body.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 * \param[in] direct_deposits A pointer to an initialized \ref cardano_direct_deposit_map_t object, or NULL to unset the field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the direct deposits were
 *         successfully set or unset, or \ref CARDANO_ERROR_POINTER_IS_NULL if \p sub_transaction_body is NULL.
 *
 * \note This function increases the reference count of the \p direct_deposits object when it is not NULL; the caller retains
 *       ownership of their reference and must release it when no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_direct_deposit_map_t* direct_deposits = ...; // Assume direct_deposits is initialized
 *
 * cardano_error_t result = cardano_sub_transaction_body_set_direct_deposits(sub_transaction_body, direct_deposits);
 *
 * // Clean up resources when no longer needed
 * cardano_direct_deposit_map_unref(&direct_deposits);
 * cardano_sub_transaction_body_unref(&sub_transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_set_direct_deposits(
  cardano_sub_transaction_body_t* sub_transaction_body,
  cardano_direct_deposit_map_t*   direct_deposits);

/**
 * \brief Retrieves the account balance intervals from the sub transaction body.
 *
 * This function returns the account balance intervals from a \ref cardano_sub_transaction_body_t object, if they are present.
 * The map constrains, per credential, the balance an account must fall within for the sub transaction to be valid.
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_account_balance_intervals_map_t object representing the account balance intervals. The returned object
 *         is a new reference, and the caller is responsible for releasing it by calling \ref cardano_account_balance_intervals_map_unref
 *         when it is no longer needed. If the \p sub_transaction_body is NULL or has no account balance intervals, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_account_balance_intervals_map_t* intervals = cardano_sub_transaction_body_get_account_balance_intervals(sub_transaction_body);
 *
 * if (intervals != NULL)
 * {
 *   // Use the account balance intervals
 *   cardano_account_balance_intervals_map_unref(&intervals);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_account_balance_intervals_map_t* cardano_sub_transaction_body_get_account_balance_intervals(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the account balance intervals for a sub transaction body.
 *
 * This function assigns an account balance intervals map to a \ref cardano_sub_transaction_body_t object. Passing NULL
 * unsets the account balance intervals, removing them from the sub transaction body.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object.
 * \param[in] account_balance_intervals A pointer to an initialized \ref cardano_account_balance_intervals_map_t object, or NULL to unset the field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the account balance intervals were
 *         successfully set or unset, or \ref CARDANO_ERROR_POINTER_IS_NULL if \p sub_transaction_body is NULL.
 *
 * \note This function increases the reference count of the \p account_balance_intervals object when it is not NULL; the caller retains
 *       ownership of their reference and must release it when no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 * cardano_account_balance_intervals_map_t* intervals = ...; // Assume intervals is initialized
 *
 * cardano_error_t result = cardano_sub_transaction_body_set_account_balance_intervals(sub_transaction_body, intervals);
 *
 * // Clean up resources when no longer needed
 * cardano_account_balance_intervals_map_unref(&intervals);
 * cardano_sub_transaction_body_unref(&sub_transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_body_set_account_balance_intervals(
  cardano_sub_transaction_body_t*          sub_transaction_body,
  cardano_account_balance_intervals_map_t* account_balance_intervals);

/**
 * \brief Retrieves the hash of a sub transaction body.
 *
 * This function computes and returns the hash of the given \ref cardano_sub_transaction_body_t object. The hash is the
 * blake2b-256 digest of the CBOR encoded body bytes and serves as the sub transaction id, following the same convention
 * as the top-level transaction id.
 *
 * \param[in] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object. The object must be valid and not NULL.
 *
 * \return A pointer to a \ref cardano_blake2b_hash_t object representing the sub transaction body hash. The returned object is a new reference, and
 *         the caller is responsible for releasing it by calling \ref cardano_blake2b_hash_unref when it is no longer needed.
 *         If the input is NULL, the function will return NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is already initialized
 * cardano_blake2b_hash_t* sub_tx_id = cardano_sub_transaction_body_get_hash(sub_transaction_body);
 *
 * if (sub_tx_id != NULL)
 * {
 *   // Use the sub transaction id for signing or verification
 *
 *   // Clean up when done
 *   cardano_blake2b_hash_unref(&sub_tx_id);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_sub_transaction_body_get_hash(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Clears the cached CBOR representation from a sub transaction body.
 *
 * This function removes the internally cached CBOR data from a \ref cardano_sub_transaction_body_t object.
 * It is useful when you have modified the sub transaction body after it was created from CBOR using
 * \ref cardano_sub_transaction_body_from_cbor and you want to ensure that the next serialization reflects
 * the current state of the sub transaction body, rather than using the original cached CBOR.
 *
 * \param[in,out] sub_transaction_body A pointer to an initialized \ref cardano_sub_transaction_body_t object
 *                                     from which the CBOR cache will be cleared.
 *
 * \note After calling this function, subsequent calls to \ref cardano_sub_transaction_body_to_cbor will
 *       serialize the sub transaction body as a map whose keys are emitted in ascending numeric order,
 *       rather than reusing the original cached CBOR.
 *
 * \warning Clearing the CBOR cache may change the binary representation of the sub transaction body when
 *          serialized, which can alter the sub transaction id and invalidate any existing signatures.
 *          Use this function with caution, especially if the sub transaction has already been signed or
 *          if preserving the exact CBOR encoding is important for your application.
 *
 * Usage Example:
 * \code{.c}
 * // Assume sub_transaction_body was created using cardano_sub_transaction_body_from_cbor
 * cardano_sub_transaction_body_t* sub_transaction_body = ...;
 *
 * // Modify the sub transaction body as needed
 * uint64_t slot = 200000;
 * cardano_error_t result = cardano_sub_transaction_body_set_invalid_after(sub_transaction_body, &slot);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   printf("Failed to set new TTL: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clear the CBOR cache to ensure serialization uses the updated sub transaction body
 * cardano_sub_transaction_body_clear_cbor_cache(sub_transaction_body);
 *
 * // Serialize the sub transaction body to CBOR
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * result = cardano_sub_transaction_body_to_cbor(sub_transaction_body, writer);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   const char* error_message = cardano_cbor_writer_get_last_error(writer);
 *   printf("Serialization failed: %s\n", error_message);
 * }
 *
 * // Clean up resources
 * cardano_cbor_writer_unref(&writer);
 * cardano_sub_transaction_body_unref(&sub_transaction_body);
 * \endcode
 */
CARDANO_EXPORT void cardano_sub_transaction_body_clear_cbor_cache(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Decrements the reference count of a cardano_sub_transaction_body_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_sub_transaction_body_t object
 * by decreasing its reference count. When the reference count reaches zero, the sub transaction body is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] sub_transaction_body A pointer to the pointer of the sub transaction body object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* sub_transaction_body = ...; // Assume sub_transaction_body is initialized
 *
 * // Perform operations with the sub_transaction_body...
 *
 * cardano_sub_transaction_body_unref(&sub_transaction_body);
 * // At this point, sub_transaction_body is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_sub_transaction_body_unref, the pointer to the \ref cardano_sub_transaction_body_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_sub_transaction_body_unref(cardano_sub_transaction_body_t** sub_transaction_body);

/**
 * \brief Increases the reference count of the cardano_sub_transaction_body_t object.
 *
 * This function is used to manually increment the reference count of a sub transaction body
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_sub_transaction_body_unref.
 *
 * \param sub_transaction_body A pointer to the sub transaction body object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming sub_transaction_body is a previously created sub transaction body object
 *
 * cardano_sub_transaction_body_ref(sub_transaction_body);
 *
 * // Now sub_transaction_body can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_sub_transaction_body_ref there is a corresponding
 * call to \ref cardano_sub_transaction_body_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_sub_transaction_body_ref(cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Retrieves the current reference count of the cardano_sub_transaction_body_t object.
 *
 * This function returns the number of active references to a sub transaction body object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_sub_transaction_body_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param sub_transaction_body A pointer to the sub transaction body object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified sub transaction body object. If the object
 * is properly managed (i.e., every \ref cardano_sub_transaction_body_ref call is matched with a
 * \ref cardano_sub_transaction_body_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming sub_transaction_body is a previously created sub transaction body object
 *
 * size_t ref_count = cardano_sub_transaction_body_refcount(sub_transaction_body);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_sub_transaction_body_refcount(const cardano_sub_transaction_body_t* sub_transaction_body);

/**
 * \brief Sets the last error message for a given sub transaction body object.
 *
 * Records an error message in the sub transaction body's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] sub_transaction_body A pointer to the \ref cardano_sub_transaction_body_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the sub transaction body's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_sub_transaction_body_set_last_error(cardano_sub_transaction_body_t* sub_transaction_body, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific sub transaction body.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_sub_transaction_body_set_last_error for the given
 * sub transaction body. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] sub_transaction_body A pointer to the \ref cardano_sub_transaction_body_t instance whose last error
 *                   message is to be retrieved. If the sub transaction body is NULL, the function
 *                   returns a generic error message indicating the null sub transaction body.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified sub transaction body. If the sub transaction body is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_sub_transaction_body_set_last_error for the same sub transaction body, or until
 *       the sub transaction body is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_sub_transaction_body_get_last_error(const cardano_sub_transaction_body_t* sub_transaction_body);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SUB_TRANSACTION_BODY_H
