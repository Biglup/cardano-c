/**
 * \file transaction_body.h
 *
 * \author angel.castillo
 * \date   Sep 17, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_BODY_H
#define BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_BODY_H

/* INCLUDES ******************************************************************/

#include <cardano/assets/multi_asset.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/certs/certificate_set.h>
#include <cardano/common/network_id.h>
#include <cardano/common/withdrawal_map.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/crypto/blake2b_hash_set.h>
#include <cardano/error.h>
#include <cardano/proposal_procedures/proposal_procedure_set.h>
#include <cardano/protocol_params/update.h>
#include <cardano/transaction_body/transaction_input.h>
#include <cardano/transaction_body/transaction_input_set.h>
#include <cardano/transaction_body/transaction_output.h>
#include <cardano/transaction_body/transaction_output_list.h>
#include <cardano/typedefs.h>
#include <cardano/voting_procedures/voting_procedures.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The transaction body encapsulates the core details of a transaction.
 */
typedef struct cardano_transaction_body_t cardano_transaction_body_t;

/**
 * \brief Creates and initializes a new instance of a Cardano transaction body.
 *
 * This function allocates and initializes a new \ref cardano_transaction_body_t object.
 *
 * \param[in] inputs A pointer to an initialized \ref cardano_transaction_input_set_t object
 *                   representing the set of transaction inputs. This parameter must not be NULL.
 * \param[in] outputs A pointer to an initialized \ref cardano_transaction_output_list_t object
 *                    representing the list of transaction outputs. This parameter must not be NULL.
 * \param[in] fee The transaction fee, specified in lovelaces (1 ADA = 1,000,000 lovelaces).
 * \param[in] ttl An optional pointer to a uint64_t representing the time-to-live (TTL) parameter, specified as a slot number.
 *                The transaction will be invalid if not included in a block by the time this slot is reached.
 *                If no TTL is required, this pointer can be set to NULL.
 * \param[out] transaction_body On successful initialization, this will point to a newly created
 *             \ref cardano_transaction_body_t object. This object represents a "strong reference"
 *             to the transaction body, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object. Specifically,
 *             once the transaction body is no longer needed, the caller must release it by calling
 *             \ref cardano_transaction_body_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the transaction body was successfully created, or an appropriate error code indicating
 *         the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the required input pointers (inputs, outputs) are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_input_set_t* inputs = ...;  // Assume inputs are initialized
 * cardano_transaction_output_list_t* outputs = ...;  // Assume outputs are initialized
 * uint64_t fee = 200000;  // 0.2 ADA transaction fee
 * uint64_t ttl_value = 100000;  // Optional TTL for the transaction
 * cardano_transaction_body_t* transaction_body = NULL;
 *
 * // Pass the TTL as a pointer
 * cardano_error_t result = cardano_transaction_body_new(inputs, outputs, fee, &ttl_value, &transaction_body);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the transaction body
 *
 *   // Free resources when done
 *   cardano_transaction_body_unref(&transaction_body);
 * }
 * else
 * {
 *   printf("Failed to create the transaction body: %s\n", cardano_error_to_string(result));
 * }
 *
 * // If no TTL is required, set the ttl pointer to NULL
 * cardano_error_t no_ttl_result = cardano_transaction_body_new(inputs, outputs, fee, NULL, &transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_new(
  cardano_transaction_input_set_t*   inputs,
  cardano_transaction_output_list_t* outputs,
  uint64_t                           fee,
  const uint64_t*                    ttl,
  cardano_transaction_body_t**       transaction_body);

/**
 * \brief Creates a \ref cardano_transaction_body_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_transaction_body_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a transaction body.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] transaction_body A pointer to a pointer of \ref cardano_transaction_body_t that will be set to the address
 *                        of the newly created transaction body object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the transaction body was successfully created, or an appropriate error code if an error occurred.
 *
 * \remark In Cardano, transactions are encoded in CBOR, but CBOR allows multiple valid ways to encode the same data. The Cardano blockchain
 *         does not enforce a canonical transaction representation, meaning that if you decode a transaction from CBOR and then re-encode it,
 *         the resulting encoding could be different. This would change the transaction body hash and invalidate any existing signatures.
 *         To prevent this, when a transaction body object is created using \ref cardano_transaction_body_from_cbor, it caches the original
 *         CBOR representation internally. When \ref cardano_transaction_body_to_cbor is called, it will output the cached CBOR.
 *         If the cached CBOR representation is not needed, the client can call \ref cardano_transaction_body_clear_cbor_cache after the object has been created.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_transaction_body_t object by calling
 *       \ref cardano_transaction_body_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_transaction_body_t* transaction_body = NULL;
 *
 * cardano_error_t result = cardano_transaction_body_from_cbor(reader, &transaction_body);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the transaction_body
 *
 *   // Once done, ensure to clean up and release the transaction_body
 *   cardano_transaction_body_unref(&transaction_body);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode transaction_body: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_from_cbor(cardano_cbor_reader_t* reader, cardano_transaction_body_t** transaction_body);

/**
 * \brief Serializes a transaction body into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_transaction_body_t object using a \ref cardano_cbor_writer_t.
 * If the transaction body was created by \ref cardano_transaction_body_from_cbor, the function will use the cached CBOR representation
 * to maintain the original encoding. This avoids potential differences in CBOR encoding which could invalidate signatures.
 * If no cached CBOR is available, the function will serialize the transaction following the Cardano serialization format as outlined in
 * [CIP-21](https://cips.cardano.org/cip/CIP-21).
 *
 * \param[in] transaction_body A constant pointer to the \ref cardano_transaction_body_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p transaction_body or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * \remark If the transaction body object was created by \ref cardano_transaction_body_from_cbor, the original CBOR encoding
 *         will be cached and reused by this function to prevent round-trip encoding issues that can change the transaction body hash
 *         and invalidate signatures. If the cached CBOR is not available or has been cleared, the function will serialize the transaction
 *         body in accordance with [CIP-21](https://cips.cardano.org/cip/CIP-21).
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_transaction_body_to_cbor(transaction_body, writer);
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
 * cardano_transaction_body_unref(&transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_transaction_body_to_cbor(
  const cardano_transaction_body_t* transaction_body,
  cardano_cbor_writer_t*            writer);

/**
 * \brief Retrieves the set of transaction inputs from a transaction body.
 *
 * This function returns the set of transaction inputs associated with a given \ref cardano_transaction_body_t object.
 * These inputs represent the UTXOs (Unspent Transaction Outputs) being used as inputs in the transaction.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_transaction_input_set_t object representing the set of transaction inputs.
 *         The returned object is a new reference, and the caller is responsible for managing its lifecycle.
 *         Specifically, the caller must release it by calling \ref cardano_transaction_input_set_unref when it is no longer needed.
 *         If the \p transaction_body is NULL, the function will return NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_transaction_input_set_t* inputs = cardano_transaction_body_get_inputs(transaction_body);
 *
 * if (inputs != NULL)
 * {
 *   // Process the inputs
 *   cardano_transaction_input_set_unref(&inputs); // Ensure to release the inputs when done
 * }
 * else
 * {
 *   printf("Failed to retrieve transaction inputs or transaction body is NULL.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_transaction_input_set_t* cardano_transaction_body_get_inputs(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the transaction inputs for a transaction body.
 *
 * This function assigns a set of transaction inputs to a \ref cardano_transaction_body_t object. The inputs represent
 * the UTXOs (Unspent Transaction Outputs) that will be used as inputs for the transaction.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 * \param[in] inputs A pointer to an initialized \ref cardano_transaction_input_set_t object representing the inputs to be set.
 *                   This parameter must not be NULL.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the inputs were successfully set.
 *         Returns \ref CARDANO_ERROR_POINTER_IS_NULL if either the \p transaction_body or \p inputs is NULL.
 *
 * \note This function increases the reference count of the \p inputs object; the caller retains ownership of the inputs
 *       and must release their reference to the inputs when no longer needed by calling \ref cardano_transaction_input_set_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is initialized
 * cardano_transaction_input_set_t* inputs = ...; // Assume inputs is initialized
 *
 * cardano_error_t result = cardano_transaction_body_set_inputs(transaction_body, inputs);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Transaction inputs successfully set.\n");
 * }
 * else
 * {
 *   printf("Failed to set transaction inputs: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up resources when no longer needed
 * cardano_transaction_input_set_unref(&inputs);
 * cardano_transaction_body_unref(&transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_inputs(cardano_transaction_body_t* transaction_body, cardano_transaction_input_set_t* inputs);

/**
 * \brief Retrieves the list of outputs from a transaction body.
 *
 * This function returns the list of outputs associated with a given \ref cardano_transaction_body_t object. The outputs represent
 * the recipients and amounts being transferred as part of the transaction.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_transaction_output_list_t object representing the list of transaction outputs. If the
 *         \p transaction_body is NULL or there are no outputs, this function returns NULL.
 *         The caller is responsible for managing the lifecycle of the returned list by calling
 *         \ref cardano_transaction_output_list_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is initialized
 * cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(transaction_body);
 *
 * if (outputs != NULL)
 * {
 *   // Use the outputs list
 *   // Clean up resources when no longer needed
 *   cardano_transaction_output_list_unref(&outputs);
 * }
 * else
 * {
 *   printf("Failed to retrieve transaction outputs or no outputs exist.\n");
 * }
 *
 * cardano_transaction_body_unref(&transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_transaction_output_list_t* cardano_transaction_body_get_outputs(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the list of outputs for a transaction body.
 *
 * This function assigns a new list of outputs to the specified \ref cardano_transaction_body_t object. The outputs represent
 * the recipients and amounts that are being transferred in the transaction.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object where the outputs will be set.
 * \param[in] outputs A pointer to an initialized \ref cardano_transaction_output_list_t object representing the list of transaction outputs.
 *                    This value must not be NULL, and it will replace any existing outputs in the \p transaction_body.
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
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_transaction_output_list_t* outputs = ...; // Assume outputs are initialized
 *
 * cardano_error_t result = cardano_transaction_body_set_outputs(transaction_body, outputs);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Transaction outputs successfully set.\n");
 * }
 * else
 * {
 *   printf("Failed to set transaction outputs.\n");
 * }
 *
 * // Clean up resources when no longer needed
 * cardano_transaction_body_unref(&transaction_body);
 * cardano_transaction_output_list_unref(&outputs);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_outputs(cardano_transaction_body_t* transaction_body, cardano_transaction_output_list_t* outputs);

/**
 * \brief Retrieves the fee associated with the transaction body.
 *
 * This function returns the fee specified in the \ref cardano_transaction_body_t object. The fee represents
 * the amount of lovelace (Cardano's native cryptocurrency) required to process the transaction.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 *                             This value must not be NULL.
 *
 * \return The fee amount in lovelaces. If the \p transaction_body pointer is NULL, the function will return 0.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * uint64_t fee = cardano_transaction_body_get_fee(transaction_body);
 *
 * if (fee > 0)
 * {
 *   printf("Transaction fee: %llu lovelaces\n", fee);
 * }
 * else
 * {
 *   printf("Failed to retrieve transaction fee.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_transaction_body_get_fee(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the fee for the transaction body.
 *
 * This function assigns a fee to the \ref cardano_transaction_body_t object. The fee represents the amount of lovelace
 * (Cardano's native cryptocurrency) required to process the transaction.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object where the fee will be set.
 * \param[in] fee The fee amount in lovelaces to be assigned to the transaction.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the fee
 *         was successfully set, or an appropriate error code if an error occurred, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the
 *         \p transaction_body pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * uint64_t fee = 200000; // Setting a fee of 200,000 lovelaces
 *
 * cardano_error_t result = cardano_transaction_body_set_fee(transaction_body, fee);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Transaction fee set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set transaction fee: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_fee(cardano_transaction_body_t* transaction_body, uint64_t fee);

/**
 * \brief Retrieves the "invalid after" value from the transaction body.
 *
 * This function fetches the "invalid after" field from a \ref cardano_transaction_body_t object, representing
 * the slot number after which the transaction is considered invalid. This field is optional, and the function
 * will return a pointer to a `uint64_t` if the field is set, or `NULL` if it is not.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 *
 * \return A pointer to a \c uint64_t representing the "invalid after" slot number if set. If this field is not set,
 *         the function will return `NULL`. The returned pointer is managed internally and must not be freed by the caller.
 *         The caller can use the pointer to read the value but not modify or free it.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume initialized
 * const uint64_t* invalid_after = cardano_transaction_body_get_invalid_after(transaction_body);
 *
 * if (invalid_after != NULL)
 * {
 *   printf("Transaction is invalid after slot: %llu\n", *invalid_after);
 * }
 * else
 * {
 *   printf("No invalid_after field is set for this transaction.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const uint64_t* cardano_transaction_body_get_invalid_after(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the "invalid after" field in the transaction body.
 *
 * This function assigns a value to the "invalid after" field of a \ref cardano_transaction_body_t object, specifying
 * the slot number after which the transaction becomes invalid. The "invalid after" field is optional, and passing a
 * `NULL` pointer will unset the value, effectively removing the "invalid after" field from the transaction.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object where the field will be set.
 * \param[in] epoch A constant pointer to a \c uint64_t representing the "invalid after" slot number. If `NULL`, this will unset the field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the field was successfully set or unset,
 *         or an appropriate error code if an error occurred, such as \ref CARDANO_ERROR_POINTER_IS_NULL if \p transaction_body is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume initialized
 * uint64_t epoch = 123456; // Slot number
 *
 * cardano_error_t result = cardano_transaction_body_set_invalid_after(transaction_body, &epoch);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Invalid after field set to slot: %llu\n", epoch);
 * }
 * else
 * {
 *   printf("Failed to set invalid after: %s\n", cardano_error_to_string(result));
 * }
 *
 * // To unset the invalid after field:
 * result = cardano_transaction_body_set_invalid_after(transaction_body, NULL);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_invalid_after(cardano_transaction_body_t* transaction_body, const uint64_t* epoch);

/**
 * \brief Retrieves the set of certificates associated with the transaction body.
 *
 * This function returns the certificates from a \ref cardano_transaction_body_t object, if they are present.
 * Certificates in Cardano are used for various purposes, such as stake delegation and pool registration.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_certificate_set_t object representing the set of certificates. If the transaction body
 *         does not contain any certificates, the function returns `NULL`. If a valid certificate set is returned, it is a new
 *         reference, and the caller is responsible for managing its lifecycle. The caller must release the certificate set
 *         using \ref cardano_certificate_set_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is initialized
 * cardano_certificate_set_t* certificates = cardano_transaction_body_get_certificates(transaction_body);
 *
 * if (certificates != NULL)
 * {
 *   // Process the certificates
 *
 *   // Once done, ensure to clean up and release the certificate set
 *   cardano_certificate_set_unref(&certificates);
 * }
 * else
 * {
 *   printf("No certificates present in the transaction body.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_certificate_set_t* cardano_transaction_body_get_certificates(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the certificates for the transaction body.
 *
 * This function assigns a set of certificates to a \ref cardano_transaction_body_t object. Certificates in Cardano are used
 * for various purposes such as stake delegation, pool registration, and other governance actions.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 * \param[in] certificates A pointer to an initialized \ref cardano_certificate_set_t object representing the certificates to
 *                         be assigned to the transaction body. The certificates must be properly initialized and cannot be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificates were
 *         successfully set, or an appropriate error code such as \ref CARDANO_ERROR_POINTER_IS_NULL if either \p transaction_body or
 *         \p certificates is NULL.
 *
 * \note The function increases the reference count of the \p certificates object. The caller retains ownership of the certificate
 *       set and is responsible for releasing it when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume initialized
 * cardano_certificate_set_t* certificates = ...; // Assume certificates are initialized
 *
 * cardano_error_t result = cardano_transaction_body_set_certificates(transaction_body, certificates);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Certificates were successfully set.\n");
 * }
 * else
 * {
 *   printf("Failed to set certificates: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up certificates and transaction_body when done
 * cardano_certificate_set_unref(&certificates);
 * cardano_transaction_body_unref(&transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_certificates(cardano_transaction_body_t* transaction_body, cardano_certificate_set_t* certificates);

/**
 * \brief Retrieves the withdrawals from a transaction body.
 *
 * This function fetches the withdrawals specified in the given \ref cardano_transaction_body_t object.
 * Withdrawals allow the transaction to withdraw rewards from staking addresses.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_withdrawal_map_t object representing the withdrawals in the transaction body.
 *         If the transaction body does not contain any withdrawals, the function returns NULL.
 *         The returned \ref cardano_withdrawal_map_t object is a new reference, and the caller is responsible
 *         for managing its lifecycle by calling \ref cardano_withdrawal_map_unref when it is no longer needed.
 *
 * \note If the returned value is NULL, it indicates that there are no withdrawals set in the transaction body.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_withdrawal_map_t* withdrawals = cardano_transaction_body_get_withdrawals(transaction_body);
 *
 * if (withdrawals != NULL)
 * {
 *   // Process the withdrawals
 *
 *   // Once done, ensure to release the withdrawals
 *   cardano_withdrawal_map_unref(&withdrawals);
 * }
 * else
 * {
 *   printf("No withdrawals present in the transaction body.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_withdrawal_map_t* cardano_transaction_body_get_withdrawals(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets or unsets the withdrawals in the transaction body.
 *
 * This function assigns a map of withdrawals to a \ref cardano_transaction_body_t object. Withdrawals in Cardano represent
 * the withdrawal of rewards from a stake address.
 *
 * If \p withdrawals is NULL, this function will unset any previously assigned withdrawals from the transaction body.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 * \param[in] withdrawals A pointer to an initialized \ref cardano_withdrawal_map_t object representing the withdrawals to
 *                        be assigned to the transaction body, or NULL to unset any previously set withdrawals.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the withdrawals were
 *         successfully set or unset, or an appropriate error code such as \ref CARDANO_ERROR_POINTER_IS_NULL if \p transaction_body is NULL.
 *
 * \note If \p withdrawals is non-NULL, the function increases the reference count of the \p withdrawals object. The caller retains
 *       ownership of the withdrawal map and is responsible for releasing it when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume initialized
 * cardano_withdrawal_map_t* withdrawals = ...; // Assume withdrawals are initialized
 *
 * cardano_error_t result = cardano_transaction_body_set_withdrawals(transaction_body, withdrawals);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Withdrawals were successfully set.\n");
 * }
 * else
 * {
 *   printf("Failed to set withdrawals: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Unset the withdrawals by passing NULL
 * result = cardano_transaction_body_set_withdrawals(transaction_body, NULL);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Withdrawals were successfully unset.\n");
 * }
 *
 * // Clean up withdrawals and transaction_body when done
 * cardano_withdrawal_map_unref(&withdrawals);
 * cardano_transaction_body_unref(&transaction_body);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_withdrawals(cardano_transaction_body_t* transaction_body, cardano_withdrawal_map_t* withdrawals);

/**
 * \brief Retrieves the update field from a transaction body.
 *
 * This function fetches the update information, if any, from a given \ref cardano_transaction_body_t object.
 * Updates are used in Cardano transactions to propose changes to the protocol parameters.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_update_t object representing the update field of the transaction body.
 *         If the update field is not set, the function returns NULL. If a valid \ref cardano_update_t object
 *         is returned, it is a new reference, and the caller is responsible for managing its lifecycle.
 *         The returned \ref cardano_update_t object must be released by calling \ref cardano_update_unref when
 *         it is no longer needed.
 *
 * \note The returned pointer may be NULL if no update is set in the transaction body.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_update_t* update = cardano_transaction_body_get_update(transaction_body);
 *
 * if (update != NULL)
 * {
 *   // Use the update object
 *
 *   // Clean up when done
 *   cardano_update_unref(&update);
 * }
 * else
 * {
 *   printf("No update is set in this transaction body.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_update_t* cardano_transaction_body_get_update(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the update field in a transaction body.
 *
 * This function assigns an update to a \ref cardano_transaction_body_t object. Updates in a Cardano transaction
 * can include protocol parameter changes.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object where the update will be set.
 * \param[in] update A pointer to an initialized \ref cardano_update_t object representing the update to be added to the transaction body.
 *                   This parameter can be NULL, in which case the update field will be unset (cleared).
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the update was
 *         successfully set, or an appropriate error code if an error occurred (e.g., \ref CARDANO_ERROR_POINTER_IS_NULL if the transaction body pointer is NULL).
 *
 * \note If a valid update object is provided, its reference count will be incremented. The caller retains ownership of their
 *       reference to the update object and is responsible for managing its lifecycle (e.g., by calling \ref cardano_update_unref
 *       when the update is no longer needed).
 * \note If `update` is NULL, the current update field in the transaction body will be removed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_update_t* update = cardano_update_new(...); // Assume update is already initialized
 *
 * cardano_error_t result = cardano_transaction_body_set_update(transaction_body, update);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Update successfully set.\n");
 * }
 * else
 * {
 *   printf("Failed to set the update: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up when done
 * cardano_update_unref(&update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_update(cardano_transaction_body_t* transaction_body, cardano_update_t* update);

/**
 * \brief Retrieves the auxiliary data hash from a transaction body.
 *
 * This function fetches the auxiliary data hash associated with the given \ref cardano_transaction_body_t object.
 * The auxiliary data hash represents the hash of additional metadata included with the transaction.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object from which the auxiliary data hash will be retrieved.
 *
 * \return A pointer to the \ref cardano_blake2b_hash_t object representing the auxiliary data hash. If the auxiliary data hash is not set, or if the input is NULL,
 *         this function returns NULL. The returned object is a new reference, and the caller is responsible for releasing it by calling \ref cardano_blake2b_hash_unref
 *         when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_blake2b_hash_t* aux_data_hash = cardano_transaction_body_get_aux_data_hash(transaction_body);
 *
 * if (aux_data_hash != NULL)
 * {
 *   // Process the auxiliary data hash
 *   cardano_blake2b_hash_unref(&aux_data_hash); // Clean up when done
 * }
 * else
 * {
 *   printf("No auxiliary data hash found or transaction_body is NULL.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_transaction_body_get_aux_data_hash(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the auxiliary data hash for a transaction body.
 *
 * This function assigns a new auxiliary data hash to the given \ref cardano_transaction_body_t object. The auxiliary data hash
 * is used to reference additional metadata that may be included with the transaction.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object to which the auxiliary data hash will be set.
 * \param[in] aux_data_hash A pointer to an initialized \ref cardano_blake2b_hash_t object representing the auxiliary data hash to set.
 *                          This parameter can be NULL to unset or clear the auxiliary data hash.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the auxiliary data hash was successfully set,
 *         or an appropriate error code, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the \p transaction_body pointer is NULL.
 *
 * \note If a valid auxiliary data hash object is provided, its reference count will be incremented. The caller retains ownership of their
 *       reference to the update object and is responsible for managing its lifecycle (e.g., by calling \ref cardano_blake2b_hash_unref
 *       when the auxiliary data hash is no longer needed).
 * \note If \p aux_data_hash is set to NULL, it will clear the current auxiliary data hash in the transaction body, if any.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_blake2b_hash_t* aux_data_hash = cardano_blake2b_hash_new(...); // Assume aux_data_hash is initialized
 *
 * cardano_error_t result = cardano_transaction_body_set_aux_data_hash(transaction_body, aux_data_hash);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Auxiliary data hash successfully set.\n");
 * }
 * else
 * {
 *   printf("Failed to set auxiliary data hash.\n");
 * }
 *
 * // Optionally clean up the aux_data_hash when no longer needed
 * cardano_blake2b_hash_unref(&aux_data_hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_aux_data_hash(cardano_transaction_body_t* transaction_body, cardano_blake2b_hash_t* aux_data_hash);

/**
 * \brief Retrieves the "invalid before" field from a transaction body.
 *
 * This function retrieves the "invalid before" field from a given \ref cardano_transaction_body_t object.
 * The "invalid before" field specifies the earliest slot in which the transaction is valid. If this field
 * is not set, the function will return NULL.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 *
 * \return A pointer to a \c uint64_t representing the "invalid before" field. If the field is set, it returns a pointer
 *         to the value. If the field is not set, the function returns NULL.
 *         The returned pointer points to internally managed memory and must not be freed or modified by the caller.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * const uint64_t* invalid_before = cardano_transaction_body_get_invalid_before(transaction_body);
 *
 * if (invalid_before != NULL)
 * {
 *   printf("Transaction invalid before slot: %llu\n", *invalid_before);
 * }
 * else
 * {
 *   printf("Invalid before field not set.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const uint64_t* cardano_transaction_body_get_invalid_before(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the "invalid before" field in a transaction body.
 *
 * This function sets the "invalid before" field in a given \ref cardano_transaction_body_t object.
 * The "invalid before" field specifies the earliest slot in which the transaction is valid.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 * \param[in] epoch A pointer to a \c uint64_t representing the "invalid before" field. This specifies
 *                  the earliest slot at which the transaction is valid. If this parameter is NULL, the "invalid before" field
 *                  will be unset (i.e., removed from the transaction body).
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the
 *         "invalid before" field was successfully set or unset, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if the transaction body pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * uint64_t invalid_before_slot = 500000; // Set the transaction to be valid after slot 500000
 *
 * cardano_error_t result = cardano_transaction_body_set_invalid_before(transaction_body, &invalid_before_slot);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Successfully set the invalid before field.\n");
 * }
 * else
 * {
 *   printf("Failed to set the invalid before field.\n");
 * }
 * \endcode
 *
 * // Unsetting the invalid before field:
 * \code{.c}
 * result = cardano_transaction_body_set_invalid_before(transaction_body, NULL);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Successfully unset the invalid before field.\n");
 * }
 * else
 * {
 *   printf("Failed to unset the invalid before field.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_invalid_before(cardano_transaction_body_t* transaction_body, const uint64_t* epoch);

/**
 * \brief Retrieves the mint field from a transaction body.
 *
 * This function retrieves the minting assets contained in the mint field of the given \ref cardano_transaction_body_t object.
 * The mint field specifies the assets to be minted or burned in the transaction.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 *
 * \return A pointer to a newly created \ref cardano_multi_asset_t object representing the minting/burning assets.
 *         If the mint field is not set, this function will return NULL.
 *         The caller is responsible for managing the lifecycle of the returned object, and must release it by calling \ref cardano_multi_asset_unref
 *         when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_multi_asset_t* mint_assets = cardano_transaction_body_get_mint(transaction_body);
 *
 * if (mint_assets != NULL)
 * {
 *   // Use the mint assets
 *
 *   // Once done, release the mint assets
 *   cardano_multi_asset_unref(&mint_assets);
 * }
 * else
 * {
 *   printf("No mint field set in this transaction body.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_multi_asset_t* cardano_transaction_body_get_mint(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the mint field for a transaction body.
 *
 * This function assigns the minting assets to the mint field of the specified \ref cardano_transaction_body_t object.
 * The mint field specifies the assets to be minted or burned in the transaction.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object to which the mint field will be set.
 * \param[in] mint A pointer to an initialized \ref cardano_multi_asset_t object representing the assets to be minted or burned.
 *                 This parameter can be NULL if the mint field should be unset (i.e., no assets to mint or burn).
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the mint field
 *         was successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the transaction_body pointer is NULL.
 *
 * \note This function increases the reference count of the mint object, if provided. The caller retains ownership of the provided
 *       mint object and must release it when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_multi_asset_t* mint_assets = ...; // Assume mint_assets is initialized
 *
 * cardano_error_t result = cardano_transaction_body_set_mint(transaction_body, mint_assets);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Mint field set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set mint field: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up resources when no longer needed
 * cardano_multi_asset_unref(&mint_assets);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_mint(cardano_transaction_body_t* transaction_body, cardano_multi_asset_t* mint);

/**
 * \brief Retrieves the script data hash from a transaction body.
 *
 * This function extracts the script data hash from a given \ref cardano_transaction_body_t object.
 * The script data hash is used in Cardano's Plutus transactions to ensure the integrity of script data (datums and redeemers).
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 *                             This pointer must not be NULL.
 *
 * \return A pointer to a \ref cardano_blake2b_hash_t object representing the script data hash.
 *         If the script data hash is not set for the transaction body, the function returns NULL.
 *         The returned script data hash is a new reference, and the caller is responsible for releasing it
 *         with \ref cardano_blake2b_hash_unref when it is no longer needed.
 *
 * \note It is important to call \ref cardano_transaction_body_get_script_data_hash only after ensuring the transaction
 *       body contains a script data hash. If the transaction body does not have this field set, the function will return NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_blake2b_hash_t* script_data_hash = cardano_transaction_body_get_script_data_hash(transaction_body);
 *
 * if (script_data_hash != NULL)
 * {
 *   // Process the script data hash
 *   cardano_blake2b_hash_unref(&script_data_hash);
 * }
 * else
 * {
 *   printf("No script data hash set for this transaction body.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_transaction_body_get_script_data_hash(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the script data hash for a transaction body.
 *
 * This function assigns a script data hash to a given \ref cardano_transaction_body_t object. The script data hash
 * is used in Cardano's Plutus transactions to ensure the integrity of the script data (datums and redeemers).
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object to which the script data hash will be set.
 * \param[in] script_data_hash A pointer to an initialized \ref cardano_blake2b_hash_t object representing the new script data hash.
 *                             This parameter can be NULL if the script data hash is to be unset.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the script data hash
 *         was successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the transaction_body pointer is NULL.
 *
 * \note If a script data hash was previously set, this function will replace it with the new value.
 *       If the script data hash is no longer needed, passing a NULL pointer will unset it.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_blake2b_hash_t* script_data_hash = ...; // Assume script_data_hash is initialized
 *
 * cardano_error_t result = cardano_transaction_body_set_script_data_hash(transaction_body, script_data_hash);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The script data hash was successfully set for the transaction body
 * }
 * else
 * {
 *   printf("Failed to set the script data hash.\n");
 * }
 *
 * // Cleanup resources if necessary
 * cardano_blake2b_hash_unref(&script_data_hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_script_data_hash(cardano_transaction_body_t* transaction_body, cardano_blake2b_hash_t* script_data_hash);

/**
 * \brief Retrieves the collateral inputs from a transaction body.
 *
 * This function extracts the set of collateral inputs from a given \ref cardano_transaction_body_t object.
 * Collateral inputs are used in Plutus transactions to cover the fees if script execution fails.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_transaction_input_set_t object representing the collateral inputs of the transaction.
 *         If the collateral inputs are not set, this function returns NULL.
 *         The returned set is a new reference, and the caller is responsible for releasing it with \ref cardano_transaction_input_set_unref
 *         when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_transaction_input_set_t* collateral_inputs = cardano_transaction_body_get_collateral(transaction_body);
 *
 * if (collateral_inputs != NULL)
 * {
 *   // Use the collateral inputs
 *   cardano_transaction_input_set_unref(&collateral_inputs);
 * }
 * else
 * {
 *   printf("No collateral inputs set for this transaction.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_transaction_input_set_t* cardano_transaction_body_get_collateral(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the collateral inputs for a transaction body.
 *
 * This function assigns a set of collateral inputs to a \ref cardano_transaction_body_t object.
 * Collateral inputs are used in Plutus transactions to cover fees in case script execution fails.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object to which the collateral inputs will be set.
 * \param[in] collateral A pointer to an initialized \ref cardano_transaction_input_set_t object representing the collateral inputs.
 *                       This parameter can be NULL to unset the collateral inputs.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the collateral inputs were successfully set,
 *         or an appropriate error code such as \ref CARDANO_ERROR_POINTER_IS_NULL if the transaction_body pointer is NULL.
 *
 * \note If the collateral is set, the reference count of the \p collateral object will be incremented. The caller retains ownership of their references
 *       and is responsible for releasing their reference when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_transaction_input_set_t* collateral_inputs = ...; // Assume collateral_inputs is initialized
 *
 * cardano_error_t result = cardano_transaction_body_set_collateral(transaction_body, collateral_inputs);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Collateral inputs successfully set.\n");
 * }
 * else
 * {
 *   printf("Failed to set collateral inputs: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up collateral_inputs if needed
 * cardano_transaction_input_set_unref(&collateral_inputs);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_collateral(cardano_transaction_body_t* transaction_body, cardano_transaction_input_set_t* collateral);

/**
 * \brief Retrieves the set of required signers for a transaction body.
 *
 * This function fetches the set of required signers from the given \ref cardano_transaction_body_t object.
 * Required signers are represented as hashes of public keys and are needed to authorize the transaction.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_blake2b_hash_set_t object representing the set of required signers. If no required signers are set,
 *         this function returns NULL. If a set of required signers is returned, it is a new reference, and the caller is responsible for releasing
 *         it by calling \ref cardano_blake2b_hash_set_unref when no longer needed.
 *
 * \note If the returned value is NULL, it means there are no required signers for this transaction body.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_blake2b_hash_set_t* required_signers = cardano_transaction_body_get_required_signers(transaction_body);
 *
 * if (required_signers != NULL)
 * {
 *   // Use the required_signers set
 *   // Ensure to release the required_signers set once done
 *   cardano_blake2b_hash_set_unref(&required_signers);
 * }
 * else
 * {
 *   printf("No required signers are set for this transaction body.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_set_t* cardano_transaction_body_get_required_signers(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the required signers for a transaction body.
 *
 * This function assigns a set of required signers to the given \ref cardano_transaction_body_t object.
 * Required signers are represented as hashes of public keys and are necessary to authorize the transaction.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object to which the required signers will be set.
 * \param[in] required_signers A pointer to a \ref cardano_blake2b_hash_set_t object representing the set of required signers. This parameter can be NULL
 *                             to unset the required signers.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the required signers were
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the
 *         transaction_body pointer is NULL.
 *
 * \note If the required signers are no longer needed or need to be cleared, passing a NULL value for the `required_signers` parameter will unset the field.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_blake2b_hash_set_t* required_signers = ...; // Assume required_signers is initialized
 *
 * cardano_error_t result = cardano_transaction_body_set_required_signers(transaction_body, required_signers);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Required signers successfully set for the transaction body
 * }
 * else
 * {
 *   printf("Failed to set required signers.\n");
 * }
 *
 * // Clean up the required signers if necessary
 * cardano_blake2b_hash_set_unref(&required_signers);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_required_signers(cardano_transaction_body_t* transaction_body, cardano_blake2b_hash_set_t* required_signers);

/**
 * \brief Retrieves the network ID from a transaction body.
 *
 * This function extracts the network ID from a given \ref cardano_transaction_body_t object, which identifies the Cardano network
 * (mainnet or testnet) where the transaction will be valid. The network ID is optional and may not always be present in the transaction.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 *
 * \return A constant pointer to a \ref cardano_network_id_t object representing the network ID, or NULL if the network ID is not set in the transaction body.
 *         The returned pointer must not be modified or freed by the caller. If the input is NULL, the function returns NULL.
 *
 * \note If the network ID is present, it is internally managed by the transaction body object and does not need to be explicitly released by the caller.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is initialized
 * const cardano_network_id_t* network_id = cardano_transaction_body_get_network_id(transaction_body);
 *
 * if (network_id != NULL)
 * {
 *   // Process the network ID
 *   printf("Transaction network ID: %u\n", *network_id);
 * }
 * else
 * {
 *   printf("No network ID is set for this transaction.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const cardano_network_id_t* cardano_transaction_body_get_network_id(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the network ID for a transaction body.
 *
 * This function assigns a network ID to a given \ref cardano_transaction_body_t object. The network ID specifies which Cardano network
 * (mainnet or testnet) the transaction is intended for. The network ID is optional and can be set to NULL if the network ID is to be unset.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object to which the network ID will be set.
 * \param[in] network_id A constant pointer to a \ref cardano_network_id_t object representing the network ID. This parameter can be NULL
 *                       if the network ID is to be unset.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the network ID was successfully set,
 *         or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the transaction_body pointer is NULL.
 *
 * \note If \p network_id is set to NULL, it will unset any previously set network ID in the transaction body.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is initialized
 * cardano_network_id_t network_id = CARDANO_NETWORK_ID_MAIN_NET; // Example network ID for mainnet
 *
 * cardano_error_t result = cardano_transaction_body_set_network_id(transaction_body, &network_id);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Network ID set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set network ID: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_network_id(cardano_transaction_body_t* transaction_body, const cardano_network_id_t* network_id);

/**
 * \brief Retrieves the collateral return output from a transaction body.
 *
 * This function fetches the collateral return output from a given \ref cardano_transaction_body_t object.
 * The collateral return is an optional field that specifies the output to which excess collateral is returned
 * in case the collateral provided in the transaction exceeds what is needed.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_transaction_output_t object representing the collateral return output.
 *         If the collateral return output is not set, this function will return NULL. If a valid output is returned,
 *         the caller is responsible for managing the returned object and must release it using
 *         \ref cardano_transaction_output_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is initialized
 * cardano_transaction_output_t* collateral_return = cardano_transaction_body_get_collateral_return(transaction_body);
 *
 * if (collateral_return != NULL)
 * {
 *   // Use the collateral return output
 *   cardano_transaction_output_unref(&collateral_return);
 * }
 * else
 * {
 *   printf("No collateral return output is set.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_transaction_output_t* cardano_transaction_body_get_collateral_return(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the collateral return output in the transaction body.
 *
 * This function assigns a new collateral return output to a \ref cardano_transaction_body_t object.
 * The collateral return output specifies where any excess collateral provided by the transaction will be returned,
 * if the collateral exceeds what is needed.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object
 *                                 to which the collateral return output will be set.
 * \param[in] output A pointer to an initialized \ref cardano_transaction_output_t object representing
 *                   the collateral return output. This parameter can be NULL to unset the collateral return.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the
 *         collateral return output was successfully set, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if the \p transaction_body pointer is NULL.
 *
 * \note This function increases the reference count of the output object. It is the caller's responsibility to
 *       manage the lifetime of their references, including calling \ref cardano_transaction_output_unref
 *       when the output is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_transaction_output_t* collateral_return_output = ...; // Assume collateral_return_output is initialized
 *
 * cardano_error_t result = cardano_transaction_body_set_collateral_return(transaction_body, collateral_return_output);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Collateral return output successfully set.\n");
 * }
 * else
 * {
 *   printf("Failed to set collateral return output.\n");
 * }
 *
 * // Clean up references
 * cardano_transaction_output_unref(&collateral_return_output);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_collateral_return(cardano_transaction_body_t* transaction_body, cardano_transaction_output_t* output);

/**
 * \brief Retrieves the total collateral from the transaction body.
 *
 * This function fetches the total collateral value specified in the transaction body. The total collateral represents
 * the amount of ADA (in lovelaces) that the transaction is providing as collateral.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 *
 * \return A constant pointer to a \c uint64_t that contains the total collateral in lovelaces.
 *         If the total collateral is not set, the function will return NULL.
 *         The memory of the returned \c uint64_t is managed internally and must not be freed by the caller.
 *
 * \note This function follows the same convention as other getters of this type—returning \c NULL if the field is not set.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is initialized
 *
 * const uint64_t* total_collateral = cardano_transaction_body_get_total_collateral(transaction_body);
 *
 * if (total_collateral != NULL)
 * {
 *   printf("Total collateral: %llu lovelaces\n", *total_collateral);
 * }
 * else
 * {
 *   printf("Total collateral is not set for this transaction.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const uint64_t* cardano_transaction_body_get_total_collateral(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the total collateral for a transaction body.
 *
 * This function sets the total collateral amount (in lovelaces) for the transaction body.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object to which the total collateral will be set.
 * \param[in] total_collateral A constant pointer to a \c uint64_t representing the total collateral value in lovelaces.
 *                             If set to \c NULL, the total collateral will be unset.
 *
 * \return A \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the total collateral
 *         was successfully set, or an appropriate error code if the input pointers are NULL or if another failure occurs.
 *
 * \note If the \c total_collateral is \c NULL, this will unset the total collateral in the transaction body.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is initialized
 * uint64_t total_collateral_value = 5000000; // 5 ADA in lovelaces
 *
 * cardano_error_t result = cardano_transaction_body_set_total_collateral(transaction_body, &total_collateral_value);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Total collateral set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set total collateral.\n");
 * }
 *
 * // Unset the total collateral
 * result = cardano_transaction_body_set_total_collateral(transaction_body, NULL);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Total collateral unset successfully.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_total_collateral(cardano_transaction_body_t* transaction_body, const uint64_t* total_collateral);

/**
 * \brief Retrieves the reference inputs from a transaction body.
 *
 * This function fetches the reference inputs from a given \ref cardano_transaction_body_t object.
 * Reference inputs allow scripts to use outputs without consuming them, enabling transaction builders
 * to reference the necessary data (e.g., Plutus scripts) without spending the UTxO.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 *
 * \return A pointer to a \ref cardano_transaction_input_set_t object representing the set of reference inputs.
 *         The returned reference input set is a new reference, and the caller is responsible for releasing it
 *         with \ref cardano_transaction_input_set_unref when it is no longer needed. If there are no reference inputs or
 *         if the transaction body does not contain reference inputs, the function returns \c NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_transaction_input_set_t* reference_inputs = cardano_transaction_body_get_reference_inputs(transaction_body);
 *
 * if (reference_inputs != NULL)
 * {
 *   // Process the reference inputs
 *   cardano_transaction_input_set_unref(&reference_inputs); // Free the reference inputs after use
 * }
 * else
 * {
 *   printf("No reference inputs found or transaction body is invalid.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_transaction_input_set_t* cardano_transaction_body_get_reference_inputs(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the reference inputs in a transaction body.
 *
 * This function assigns a set of reference inputs to a given \ref cardano_transaction_body_t object.
 * Reference inputs allow scripts to access outputs without consuming them, enabling transaction
 * builders to use specific UTxOs for reference purposes, such as providing Plutus scripts access
 * to necessary data.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object
 *                                 where the reference inputs will be set.
 * \param[in] reference_inputs A pointer to an initialized \ref cardano_transaction_input_set_t object
 *                             representing the set of reference inputs to assign to the transaction body.
 *                             This parameter can be NULL if no reference inputs are to be set, effectively
 *                             clearing the reference inputs from the transaction body.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the
 *         reference inputs were successfully set, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if the \p transaction_body pointer is NULL.
 *
 * \note If \p reference_inputs is NULL, any previously set reference inputs in the transaction body will be cleared.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_transaction_input_set_t* reference_inputs = ...; // Assume reference_inputs is initialized
 *
 * cardano_error_t result = cardano_transaction_body_set_reference_inputs(transaction_body, reference_inputs);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Reference inputs set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set reference inputs.\n");
 * }
 *
 * // Cleanup if necessary (only if reference_inputs was dynamically allocated elsewhere)
 * cardano_transaction_input_set_unref(&reference_inputs);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_reference_inputs(cardano_transaction_body_t* transaction_body, cardano_transaction_input_set_t* reference_inputs);

/**
 * \brief Retrieves the voting procedures associated with a transaction body.
 *
 * This function fetches the voting procedures from a given \ref cardano_transaction_body_t object.
 * Voting procedures represent governance-related actions that may be included in the transaction,
 * such as votes on proposals or governance actions.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object
 *                             from which the voting procedures will be retrieved.
 *
 * \return A pointer to a \ref cardano_voting_procedures_t object representing the voting procedures
 *         included in the transaction body. If the transaction body does not have voting procedures set,
 *         this function returns NULL. The returned object is a new reference, and the caller is responsible
 *         for releasing it with \ref cardano_voting_procedures_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_voting_procedures_t* voting_procedures = cardano_transaction_body_get_voting_procedures(transaction_body);
 *
 * if (voting_procedures != NULL)
 * {
 *   // Process the voting procedures
 *
 *   // Ensure to clean up and release the voting_procedures when done
 *   cardano_voting_procedures_unref(&voting_procedures);
 * }
 * else
 * {
 *   printf("No voting procedures found in this transaction body.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_voting_procedures_t* cardano_transaction_body_get_voting_procedures(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the voting procedures for a transaction body.
 *
 * This function assigns a \ref cardano_voting_procedures_t object to a given \ref cardano_transaction_body_t object.
 * Voting procedures represent governance-related actions, such as votes on proposals or governance actions, that
 * may be included in the transaction body.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object to which the voting procedures will be set.
 * \param[in] voting_procedures A pointer to an initialized \ref cardano_voting_procedures_t object representing the voting procedures.
 *                              This parameter can be set to NULL to remove the voting procedures from the transaction body.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the voting procedures
 *         were successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the transaction body pointer is NULL.
 *
 * \note The function increases the reference count of the \p voting_procedures object, and the caller retains ownership of their references.
 *       It is the caller's responsibility to release their reference to \p voting_procedures when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_voting_procedures_t* voting_procedures = ...; // Assume voting_procedures is already initialized
 *
 * cardano_error_t result = cardano_transaction_body_set_voting_procedures(transaction_body, voting_procedures);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The voting procedures have been successfully set for the transaction body
 * }
 * else
 * {
 *   printf("Failed to set the voting procedures.\n");
 * }
 *
 * // Clean up if necessary
 * cardano_voting_procedures_unref(&voting_procedures);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_voting_procedures(cardano_transaction_body_t* transaction_body, cardano_voting_procedures_t* voting_procedures);

/**
 * \brief Retrieves the proposal procedure set from a transaction body.
 *
 * This function extracts the \ref cardano_proposal_procedure_set_t object from the given \ref cardano_transaction_body_t object.
 * The proposal procedure set represents the set of governance proposals or actions included in the transaction body.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object from which the proposal procedure set is retrieved.
 *
 * \return A pointer to the retrieved \ref cardano_proposal_procedure_set_t object, or NULL if no proposal procedure set is present.
 *         The caller is responsible for managing the returned object, and must call \ref cardano_proposal_procedure_set_unref
 *         to release it when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_proposal_procedure_set_t* proposal_procedure_set = cardano_transaction_body_get_proposal_procedures(transaction_body);
 *
 * if (proposal_procedure_set != NULL)
 * {
 *   // Use the proposal procedure set
 *
 *   // Once done, ensure to clean up and release the proposal_procedure_set
 *   cardano_proposal_procedure_set_unref(&proposal_procedure_set);
 * }
 * else
 * {
 *   printf("No proposal procedure set present in the transaction body.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_proposal_procedure_set_t* cardano_transaction_body_get_proposal_procedures(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the proposal procedure set for a transaction body.
 *
 * This function assigns a \ref cardano_proposal_procedure_set_t object to the given \ref cardano_transaction_body_t object.
 * The proposal procedure set represents a collection of governance proposals or actions that are to be included in the transaction body.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object to which the proposal procedure set will be assigned.
 * \param[in] proposal_procedures A pointer to a \ref cardano_proposal_procedure_set_t object representing the new proposal procedure set.
 *                                This parameter can be NULL to unset the proposal procedures from the transaction body.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the proposal procedure set was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the \p transaction_body pointer is NULL.
 *
 * \note The function increases the reference count of the \p proposal_procedures object, so the caller retains ownership
 *       and is still responsible for managing its lifecycle. To remove the proposal procedures, pass \p proposal_procedures as NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_proposal_procedure_set_t* proposal_procedures = ...; // Assume proposal_procedures is initialized
 *
 * cardano_error_t result = cardano_transaction_body_set_proposal_procedure(transaction_body, proposal_procedures);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The proposal procedures have been set for the transaction body
 * }
 * else
 * {
 *   printf("Failed to set the proposal procedures: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up proposal_procedures if it's no longer needed
 * cardano_proposal_procedure_set_unref(&proposal_procedures);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_proposal_procedure(cardano_transaction_body_t* transaction_body, cardano_proposal_procedure_set_t* proposal_procedures);

/**
 * \brief Retrieves the treasury value from a transaction body.
 *
 * This function returns a pointer to the treasury value (in ADA, represented in lovelaces) associated with a \ref cardano_transaction_body_t object.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 *
 * \return A pointer to a \c uint64_t representing the treasury value in lovelaces. The pointer will be NULL if the treasury value is not set in this transaction.
 *         The memory of the returned value is managed internally by the \ref cardano_transaction_body_t object and must not be freed by the caller.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * const uint64_t* treasury_value = cardano_transaction_body_get_treasury_value(transaction_body);
 *
 * if (treasury_value != NULL)
 * {
 *   printf("Treasury Value: %llu lovelaces\n", *treasury_value);
 * }
 * else
 * {
 *   printf("No treasury value set for this transaction.\n");
 * }
 * \endcode
 *
 * \note Since the returned pointer can be NULL if the treasury value is not set, callers should always check for NULL before dereferencing it.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const uint64_t* cardano_transaction_body_get_treasury_value(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the treasury value in a transaction body.
 *
 * This function sets the treasury value (in ADA, represented in lovelaces) in a \ref cardano_transaction_body_t object.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object in which to set the treasury value.
 * \param[in] treasury_value A pointer to a \c uint64_t representing the treasury value in lovelaces. This parameter can be NULL to unset the treasury value.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the treasury value was successfully set,
 *         or an appropriate error code if the operation failed, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the \p transaction_body pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * uint64_t treasury_value = 1000000; // 1 ADA in lovelaces
 *
 * cardano_error_t result = cardano_transaction_body_set_treasury_value(transaction_body, &treasury_value);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Treasury value set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set treasury value: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 *
 * \note Passing NULL for \p treasury_value will unset the treasury value in the transaction body.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_treasury_value(cardano_transaction_body_t* transaction_body, const uint64_t* treasury_value);

/**
 * \brief Retrieves the donation value from a transaction body.
 *
 * This function returns the donation value (in ADA, represented in lovelaces) from a \ref cardano_transaction_body_t object.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object from which to retrieve the donation value.
 *
 * \return A pointer to a \c uint64_t representing the donation value in lovelaces. If the donation value is not set, this function returns NULL.
 *         The memory for the returned \c uint64_t is managed internally by the transaction body object and must not be freed by the caller.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * const uint64_t* donation_value = cardano_transaction_body_get_donation(transaction_body);
 *
 * if (donation_value != NULL)
 * {
 *   printf("Donation value: %llu lovelaces\n", *donation_value);
 * }
 * else
 * {
 *   printf("No donation value set.\n");
 * }
 * \endcode
 *
 * \note The returned pointer must not be freed by the caller. If no donation value is set, this function returns NULL.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const uint64_t* cardano_transaction_body_get_donation(cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the donation value in a transaction body.
 *
 * This function assigns a donation value to a \ref cardano_transaction_body_t object.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object to which the donation value will be set.
 * \param[in] donation A pointer to a \c uint64_t representing the donation value in lovelaces. This parameter can be NULL to unset
 *                     the donation value.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the donation value was successfully set,
 *         or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the transaction_body pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * uint64_t donation_value = 5000000; // 5 ADA in lovelaces
 *
 * cardano_error_t result = cardano_transaction_body_set_donation(transaction_body, &donation_value);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Donation value set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set the donation value: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 *
 * \note If the \p donation parameter is set to NULL, it will unset any previously set donation value in the transaction body.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_body_set_donation(cardano_transaction_body_t* transaction_body, const uint64_t* donation);

/**
 * \brief Retrieves the hash of a transaction body.
 *
 * This function computes and returns the hash of the given \ref cardano_transaction_body_t object. The hash is a unique identifier for the transaction body,
 * which can be used to reference the transaction in other parts of the blockchain.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object. The object must be valid and not NULL.
 *
 * \return A pointer to a \ref cardano_blake2b_hash_t object representing the transaction body hash. The returned object is a new reference, and
 *         the caller is responsible for releasing it by calling \ref cardano_blake2b_hash_unref when it is no longer needed.
 *         If the input is NULL, the function will return NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * cardano_blake2b_hash_t* tx_body_hash = cardano_transaction_body_get_hash(transaction_body);
 *
 * if (tx_body_hash != NULL)
 * {
 *   // Use the transaction body hash for signing or verification
 *
 *   // Clean up when done
 *   cardano_blake2b_hash_unref(&tx_body_hash);
 * }
 * else
 * {
 *   printf("Failed to retrieve the transaction body hash.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_transaction_body_get_hash(cardano_transaction_body_t* transaction_body);

/**
 * \brief Determines if the transaction body uses tagged sets in its encoding.
 *
 * This function checks whether the given \ref cardano_transaction_body_t object
 * uses the new Conway era encoding for sets, which are collections identified by a tag.
 * In the Conway era, certain collections within the transaction body are encoded as tagged sets,
 * whereas in previous eras, these collections were encoded as arrays without the tag.
 *
 * \param[in] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object.
 *
 * \return A boolean value:
 *         - \c true if the transaction body uses the Conway era encoding for sets (tagged sets).
 *         - \c false if the transaction body uses the older encoding (arrays without tags).
 *
 * \note This function helps in determining the serialization format of the transaction body,
 *       which may be necessary for compatibility with other transaction encoders/decoders.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = ...; // Assume transaction_body is already initialized
 * bool has_tagged_sets = cardano_transaction_body_has_tagged_sets(transaction_body);
 *
 * if (has_tagged_sets)
 * {
 *   printf("The transaction body uses Conway era tagged sets.\n");
 * }
 * else
 * {
 *   printf("The transaction body uses pre-Conway era encoding.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_transaction_body_has_tagged_sets(cardano_transaction_body_t* transaction_body);

/**
 * \brief Clears the cached CBOR representation from a transaction body.
 *
 * This function removes the internally cached CBOR data from a \ref cardano_transaction_body_t object.
 * It is useful when you have modified the transaction body after it was created from CBOR using
 * \ref cardano_transaction_body_from_cbor and you want to ensure that the next serialization reflects
 * the current state of the transaction body, rather than using the original cached CBOR.
 *
 * \param[in,out] transaction_body A pointer to an initialized \ref cardano_transaction_body_t object
 *                                 from which the CBOR cache will be cleared.
 *
 * \note After calling this function, subsequent calls to \ref cardano_transaction_body_to_cbor will
 *       serialize the transaction body using the standard encoding as defined in
 *       [CIP-21](https://cips.cardano.org/cip/CIP-21), rather than reusing the original cached CBOR.
 *
 * \warning Clearing the CBOR cache may change the binary representation of the transaction body when
 *          serialized, which can alter the transaction body hash and invalidate any existing signatures.
 *          Use this function with caution, especially if the transaction has already been signed or
 *          if preserving the exact CBOR encoding is important for your application.
 *
 * Usage Example:
 * \code{.c}
 * // Assume transaction_body was created using cardano_transaction_body_from_cbor
 * cardano_transaction_body_t* transaction_body = ...;
 *
 * // Modify the transaction body as needed
 * // For example, change the fee
 * uint64_t new_fee = 500000;
 * cardano_error_t result = cardano_transaction_body_set_fee(transaction_body, new_fee);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   printf("Failed to set new fee: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clear the CBOR cache to ensure serialization uses the updated transaction body
 * cardano_transaction_body_clear_cbor_cache(transaction_body);
 *
 * // Serialize the transaction body to CBOR
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * result = cardano_transaction_body_to_cbor(transaction_body, writer);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Process the CBOR data as needed
 * }
 * else
 * {
 *   const char* error_message = cardano_cbor_writer_get_last_error(writer);
 *   printf("Serialization failed: %s\n", error_message);
 * }
 *
 * // Clean up resources
 * cardano_cbor_writer_unref(&writer);
 * cardano_transaction_body_unref(&transaction_body);
 * \endcode
 */
CARDANO_EXPORT void cardano_transaction_body_clear_cbor_cache(cardano_transaction_body_t* transaction_body);

/**
 * \brief Decrements the reference count of a cardano_transaction_body_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_transaction_body_t object
 * by decreasing its reference count. When the reference count reaches zero, the transaction_body is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] transaction_body A pointer to the pointer of the transaction_body object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_body_t* transaction_body = cardano_transaction_body_new(major, minor);
 *
 * // Perform operations with the transaction_body...
 *
 * cardano_transaction_body_unref(&transaction_body);
 * // At this point, transaction_body is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_transaction_body_unref, the pointer to the \ref cardano_transaction_body_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_transaction_body_unref(cardano_transaction_body_t** transaction_body);

/**
 * \brief Increases the reference count of the cardano_transaction_body_t object.
 *
 * This function is used to manually increment the reference count of an cardano_transaction_body_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_transaction_body_unref.
 *
 * \param transaction_body A pointer to the cardano_transaction_body_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming transaction_body is a previously created transaction_body object
 *
 * cardano_transaction_body_ref(transaction_body);
 *
 * // Now transaction_body can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_transaction_body_ref there is a corresponding
 * call to \ref cardano_transaction_body_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_transaction_body_ref(cardano_transaction_body_t* transaction_body);

/**
 * \brief Retrieves the current reference count of the cardano_transaction_body_t object.
 *
 * This function returns the number of active references to an cardano_transaction_body_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_transaction_body_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param transaction_body A pointer to the cardano_transaction_body_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_transaction_body_t object. If the object
 * is properly managed (i.e., every \ref cardano_transaction_body_ref call is matched with a
 * \ref cardano_transaction_body_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming transaction_body is a previously created transaction_body object
 *
 * size_t ref_count = cardano_transaction_body_refcount(transaction_body);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_transaction_body_refcount(const cardano_transaction_body_t* transaction_body);

/**
 * \brief Sets the last error message for a given cardano_transaction_body_t object.
 *
 * Records an error message in the transaction_body's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] transaction_body A pointer to the \ref cardano_transaction_body_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the transaction_body's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_transaction_body_set_last_error(
  cardano_transaction_body_t* transaction_body,
  const char*                 message);

/**
 * \brief Retrieves the last error message recorded for a specific transaction_body.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_transaction_body_set_last_error for the given
 * transaction_body. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] transaction_body A pointer to the \ref cardano_transaction_body_t instance whose last error
 *                   message is to be retrieved. If the transaction_body is NULL, the function
 *                   returns a generic error message indicating the null transaction_body.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified transaction_body. If the transaction_body is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_transaction_body_set_last_error for the same transaction_body, or until
 *       the transaction_body is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_transaction_body_get_last_error(
  const cardano_transaction_body_t* transaction_body);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_BODY_H