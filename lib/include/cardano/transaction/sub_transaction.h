/**
 * \file sub_transaction.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SUB_TRANSACTION_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SUB_TRANSACTION_H

/* INCLUDES ******************************************************************/

#include <cardano/auxiliary_data/auxiliary_data.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/transaction_body/sub_transaction_body.h>
#include <cardano/typedefs.h>
#include <cardano/witness_set/witness_set.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A sub transaction is a transaction nested inside a top-level transaction.
 *
 * Sub transactions allow several parties to compose intents that are validated and applied
 * atomically as part of a single top-level transaction. Each sub transaction carries its own
 * body, its own witness set and, optionally, its own auxiliary data.
 *
 * On the wire a sub transaction is a three element array of the form
 * [sub_transaction_body, transaction_witness_set, auxiliary_data / null]. Unlike the top-level
 * mempool encoding, a sub transaction carries no validity flag, and absent auxiliary data is
 * encoded as a CBOR null.
 */
typedef struct cardano_sub_transaction_t cardano_sub_transaction_t;

/**
 * \brief Creates and initializes a new Cardano sub transaction.
 *
 * This function allocates and initializes a new instance of a \ref cardano_sub_transaction_t object,
 * which represents a Cardano sub transaction consisting of a sub transaction body, witness set, and optional auxiliary data.
 *
 * \param[in] body A pointer to an initialized \ref cardano_sub_transaction_body_t object that represents the sub transaction body.
 *                 This parameter is required and must not be NULL.
 * \param[in] witness_set A pointer to an initialized \ref cardano_witness_set_t object that contains the witness data for the sub transaction.
 *                        This parameter is required and must not be NULL.
 * \param[in] auxiliary_data An optional pointer to a \ref cardano_auxiliary_data_t object representing any auxiliary data associated
 *                           with the sub transaction. This parameter can be NULL if no auxiliary data is provided.
 * \param[out] sub_transaction On successful creation, this will point to a newly created \ref cardano_sub_transaction_t object. The caller is
 *                             responsible for managing the lifecycle of this object. Specifically, once the sub transaction is no longer needed,
 *                             the caller must release it by calling \ref cardano_sub_transaction_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the sub transaction was successfully created,
 *         or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if required inputs (body or witness_set) are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_body_t* body = ...; // Assume body is initialized
 * cardano_witness_set_t* witness_set = ...; // Assume witness set is initialized
 * cardano_auxiliary_data_t* auxiliary_data = ...; // Optional: can be NULL if no auxiliary data is present
 * cardano_sub_transaction_t* sub_transaction = NULL;
 *
 * cardano_error_t result = cardano_sub_transaction_new(body, witness_set, auxiliary_data, &sub_transaction);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the sub transaction
 *
 *   // Once done, ensure to clean up and release the sub transaction
 *   cardano_sub_transaction_unref(&sub_transaction);
 * }
 * else
 * {
 *   printf("Failed to create sub transaction: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_new(
  cardano_sub_transaction_body_t* body,
  cardano_witness_set_t*          witness_set,
  cardano_auxiliary_data_t*       auxiliary_data,
  cardano_sub_transaction_t**     sub_transaction);

/**
 * \brief Creates a \ref cardano_sub_transaction_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_sub_transaction_t object.
 * The CBOR data must be a three element array of the form [sub_transaction_body, transaction_witness_set, auxiliary_data / null];
 * arrays with any other number of elements are rejected.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] sub_transaction A pointer to a pointer of \ref cardano_sub_transaction_t that will be set to the address
 *                             of the newly created sub transaction object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the sub transaction was successfully created, or an appropriate error code if an error occurred.
 *
 * \remark In Cardano, transactions are encoded in CBOR, but CBOR allows multiple valid ways to encode the same data. The Cardano blockchain
 *         does not enforce a canonical transaction representation, meaning that if you decode a sub transaction from CBOR and then re-encode it,
 *         the resulting encoding could be different. This would change the sub transaction id and invalidate any existing signatures.
 *         To prevent this, when a sub transaction object is created using \ref cardano_sub_transaction_from_cbor, it caches the original
 *         CBOR representation internally. When \ref cardano_sub_transaction_to_cbor is called, it will output the cached CBOR.
 *         If the cached CBOR representation is not needed, the client can call \ref cardano_sub_transaction_clear_cbor_cache after the object has been created.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_sub_transaction_t object by calling
 *       \ref cardano_sub_transaction_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_sub_transaction_t* sub_transaction = NULL;
 *
 * cardano_error_t result = cardano_sub_transaction_from_cbor(reader, &sub_transaction);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the sub transaction
 *
 *   // Once done, ensure to clean up and release the sub transaction
 *   cardano_sub_transaction_unref(&sub_transaction);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode sub transaction: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_sub_transaction_from_cbor(cardano_cbor_reader_t* reader, cardano_sub_transaction_t** sub_transaction);

/**
 * \brief Serializes a sub transaction into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_sub_transaction_t object using a \ref cardano_cbor_writer_t.
 * The sub transaction is written as a three element array of the form
 * [sub_transaction_body, transaction_witness_set, auxiliary_data / null]. If the sub transaction has no
 * auxiliary data, a CBOR null is written in its place.
 *
 * \param[in] sub_transaction A constant pointer to the \ref cardano_sub_transaction_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p sub_transaction or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * \remark In Cardano, transactions are encoded in CBOR, but CBOR allows multiple valid ways to encode the same data. The Cardano blockchain
 *         does not enforce a canonical transaction representation, meaning that if you decode a sub transaction from CBOR and then re-encode it,
 *         the resulting encoding could be different. This would change the sub transaction id and invalidate any existing signatures.
 *         To prevent this, when a sub transaction object is created using \ref cardano_sub_transaction_from_cbor, it caches the original
 *         CBOR representation internally. When \ref cardano_sub_transaction_to_cbor is called, it will output the cached CBOR.
 *         If the cached CBOR representation is not needed, the client can call \ref cardano_sub_transaction_clear_cbor_cache after the object has been created.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_t* sub_transaction = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_sub_transaction_to_cbor(sub_transaction, writer);
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
 * cardano_sub_transaction_unref(&sub_transaction);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_sub_transaction_to_cbor(
  const cardano_sub_transaction_t* sub_transaction,
  cardano_cbor_writer_t*           writer);

/**
 * \brief Retrieves the sub transaction body from a sub transaction object.
 *
 * This function extracts the sub transaction body (\ref cardano_sub_transaction_body_t) from a given
 * \ref cardano_sub_transaction_t object. The sub transaction body contains the essential elements of the
 * sub transaction, such as inputs, outputs, and more.
 *
 * \param[in] sub_transaction A pointer to an initialized \ref cardano_sub_transaction_t object.
 *
 * \return A pointer to the \ref cardano_sub_transaction_body_t object representing the body of the sub transaction.
 *         The returned body is a new reference, and the caller is responsible for managing its lifecycle.
 *         Once the body is no longer needed, the caller must release it by calling \ref cardano_sub_transaction_body_unref.
 *         If the input sub transaction pointer is NULL, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_t* sub_transaction = ...; // Assume sub transaction is already initialized
 * cardano_sub_transaction_body_t* body = cardano_sub_transaction_get_body(sub_transaction);
 *
 * if (body != NULL)
 * {
 *   // Use the sub transaction body
 *
 *   // Clean up the body reference when done
 *   cardano_sub_transaction_body_unref(&body);
 * }
 * else
 * {
 *   printf("Failed to retrieve sub transaction body.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_sub_transaction_body_t* cardano_sub_transaction_get_body(cardano_sub_transaction_t* sub_transaction);

/**
 * \brief Sets the sub transaction body for a sub transaction.
 *
 * This function allows replacing or setting the body for a given \ref cardano_sub_transaction_t object.
 * The sub transaction body encapsulates inputs, outputs, and other transaction-level data.
 *
 * \param[in,out] sub_transaction A pointer to an initialized \ref cardano_sub_transaction_t object in which the body will be set.
 * \param[in] body A pointer to an initialized \ref cardano_sub_transaction_body_t object representing the new sub transaction body.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the body was successfully set,
 *         or an appropriate error code such as \ref CARDANO_ERROR_POINTER_IS_NULL if the sub transaction or body pointers are NULL.
 *
 * \note The reference count of the sub transaction body will be incremented by this function, so the caller retains ownership of
 *       the body object and is responsible for releasing it when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_t* sub_transaction = ...; // Assume sub transaction is already initialized
 * cardano_sub_transaction_body_t* new_body = ...; // Assume new_body is already initialized
 *
 * cardano_error_t result = cardano_sub_transaction_set_body(sub_transaction, new_body);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Sub transaction body set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set sub transaction body.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_sub_transaction_set_body(cardano_sub_transaction_t* sub_transaction, cardano_sub_transaction_body_t* body);

/**
 * \brief Retrieves the witness set associated with a sub transaction.
 *
 * This function returns the witness set from the given \ref cardano_sub_transaction_t object.
 * The witness set contains cryptographic signatures, public keys, and other necessary
 * information required to verify the sub transaction.
 *
 * \param[in] sub_transaction A pointer to an initialized \ref cardano_sub_transaction_t object.
 *
 * \return A pointer to a \ref cardano_witness_set_t object representing the witness set.
 *         This function returns a new reference to the witness set, and the caller is
 *         responsible for managing its lifecycle by calling \ref cardano_witness_set_unref
 *         when it is no longer needed.
 *
 * \note The returned witness set is a "strong reference" and must be unref'd by the caller
 *       once it's no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_t* sub_transaction = ...; // Assume initialized
 * cardano_witness_set_t* witness_set = cardano_sub_transaction_get_witness_set(sub_transaction);
 *
 * if (witness_set != NULL)
 * {
 *   // Use the witness set
 *
 *   // Cleanup the witness set once done
 *   cardano_witness_set_unref(&witness_set);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_witness_set_t* cardano_sub_transaction_get_witness_set(cardano_sub_transaction_t* sub_transaction);

/**
 * \brief Sets the witness set for a sub transaction.
 *
 * This function assigns a witness set to the specified \ref cardano_sub_transaction_t object.
 * A witness set contains the cryptographic signatures and other witness data needed to
 * validate the sub transaction on the Cardano blockchain.
 *
 * \param[in,out] sub_transaction A pointer to an initialized \ref cardano_sub_transaction_t object
 *                                to which the witness set will be added. This parameter must
 *                                not be NULL.
 * \param[in] witness_set A pointer to an initialized \ref cardano_witness_set_t object
 *                        representing the witness data. This parameter must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns
 *         \ref CARDANO_SUCCESS if the witness set was successfully added to the sub transaction,
 *         or an appropriate error code if any of the input pointers (sub_transaction or
 *         witness_set) is NULL, such as \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * \note This function increases the reference count of the \p witness_set object, meaning
 *       the caller retains ownership of the witness set and must release it by calling
 *       \ref cardano_witness_set_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_t* sub_transaction = ...; // Assume sub transaction is already initialized
 * cardano_witness_set_t* witness_set = ...; // Assume witness_set is already initialized
 *
 * cardano_error_t result = cardano_sub_transaction_set_witness_set(sub_transaction, witness_set);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Witness set successfully added to the sub transaction.\n");
 * }
 * else
 * {
 *   printf("Failed to set witness set: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up when no longer needed
 * cardano_witness_set_unref(&witness_set);
 * cardano_sub_transaction_unref(&sub_transaction);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_sub_transaction_set_witness_set(cardano_sub_transaction_t* sub_transaction, cardano_witness_set_t* witness_set);

/**
 * \brief Retrieves the auxiliary data from a sub transaction.
 *
 * This function fetches the auxiliary data associated with a given \ref cardano_sub_transaction_t object.
 *
 * \param[in] sub_transaction A pointer to an initialized \ref cardano_sub_transaction_t object.
 *                            This parameter must not be NULL.
 *
 * \return A pointer to a \ref cardano_auxiliary_data_t object representing the auxiliary data in the sub transaction.
 *         If the sub transaction does not contain auxiliary data, the function will return NULL.
 *         The returned object is a new reference, and the caller is responsible for managing its lifecycle.
 *         Specifically, the caller must release the object by calling \ref cardano_auxiliary_data_unref
 *         when it is no longer needed.
 *
 * \note Auxiliary data is optional in a sub transaction and may not always be present.
 *       Therefore, it is important to check the return value for NULL before dereferencing it.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_t* sub_transaction = ...; // Assume sub transaction is already initialized
 * cardano_auxiliary_data_t* aux_data = cardano_sub_transaction_get_auxiliary_data(sub_transaction);
 *
 * if (aux_data != NULL)
 * {
 *   // Process the auxiliary data
 *   cardano_auxiliary_data_unref(&aux_data); // Clean up when done
 * }
 * else
 * {
 *   printf("No auxiliary data found in the sub transaction.\n");
 * }
 *
 * // Clean up the sub transaction when no longer needed
 * cardano_sub_transaction_unref(&sub_transaction);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_auxiliary_data_t* cardano_sub_transaction_get_auxiliary_data(cardano_sub_transaction_t* sub_transaction);

/**
 * \brief Sets the auxiliary data for a sub transaction.
 *
 * This function assigns auxiliary data to the specified \ref cardano_sub_transaction_t object.
 * Auxiliary data typically contains additional metadata associated with the sub transaction.
 *
 * \param[in,out] sub_transaction A pointer to an initialized \ref cardano_sub_transaction_t object
 *                                where the auxiliary data will be set. This parameter must not be NULL.
 * \param[in] auxiliary_data A pointer to an initialized \ref cardano_auxiliary_data_t object
 *                           representing the auxiliary data to be included in the sub transaction.
 *                           This parameter can be NULL to unset the auxiliary data in the sub transaction.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the auxiliary data was successfully set. Returns \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         the \p sub_transaction is NULL.
 *
 * \note This function increases the reference count of the \p auxiliary_data object. The caller retains
 *       ownership of the auxiliary data and must release it by calling \ref cardano_auxiliary_data_unref
 *       when it is no longer needed.
 *
 * \remark A sub transaction without auxiliary data is serialized with a CBOR null in place of the
 *         auxiliary data element.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_t* sub_transaction = ...; // Assume sub transaction is initialized
 * cardano_auxiliary_data_t* aux_data = ...; // Assume auxiliary_data is initialized
 *
 * cardano_error_t result = cardano_sub_transaction_set_auxiliary_data(sub_transaction, aux_data);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Auxiliary data successfully set for the sub transaction.\n");
 * }
 * else
 * {
 *   printf("Failed to set auxiliary data: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up resources
 * cardano_auxiliary_data_unref(&aux_data);
 * cardano_sub_transaction_unref(&sub_transaction);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_sub_transaction_set_auxiliary_data(cardano_sub_transaction_t* sub_transaction, cardano_auxiliary_data_t* auxiliary_data);

/**
 * \brief Retrieves the sub transaction ID (hash) from a sub transaction.
 *
 * This function computes and returns the unique identifier (ID) for the specified \ref cardano_sub_transaction_t object.
 * The sub transaction ID is the blake2b-256 hash of the sub transaction body, following the same convention as the
 * top-level transaction ID.
 *
 * \param[in] sub_transaction A pointer to an initialized \ref cardano_sub_transaction_t object.
 *                            This parameter must not be NULL.
 *
 * \return A pointer to a \ref cardano_blake2b_hash_t object representing the sub transaction ID (hash).
 *         The returned object is a new reference, and the caller is responsible for managing its lifecycle.
 *         Specifically, the caller must release the object by calling \ref cardano_blake2b_hash_unref
 *         when it is no longer needed. If the \p sub_transaction is NULL, the function returns NULL.
 *
 * \note The sub transaction ID is the primary reference used to identify and track a sub transaction within
 *       its enclosing top-level transaction.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_t* sub_transaction = ...; // Assume sub transaction is already initialized
 * cardano_blake2b_hash_t* sub_tx_id = cardano_sub_transaction_get_id(sub_transaction);
 *
 * if (sub_tx_id != NULL)
 * {
 *   // Use the sub transaction ID for reference or verification
 *
 *   // Clean up when done
 *   cardano_blake2b_hash_unref(&sub_tx_id);
 * }
 * else
 * {
 *   printf("Failed to retrieve the sub transaction ID.\n");
 * }
 *
 * // Clean up resources
 * cardano_sub_transaction_unref(&sub_transaction);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_sub_transaction_get_id(cardano_sub_transaction_t* sub_transaction);

/**
 * \brief Clears the cached CBOR representation from a sub transaction.
 *
 * This function removes the internally cached CBOR data from a \ref cardano_sub_transaction_t object.
 * It is useful when you have modified the sub transaction after it was created from CBOR using
 * \ref cardano_sub_transaction_from_cbor and you want to ensure that the next serialization reflects
 * the current state of the sub transaction, rather than using the original cached CBOR.
 *
 * \param[in,out] sub_transaction A pointer to an initialized \ref cardano_sub_transaction_t object
 *                                from which the CBOR cache will be cleared.
 *
 * \note After calling this function, subsequent calls to \ref cardano_sub_transaction_to_cbor will
 *       serialize the sub transaction using the standard encoding as defined in
 *       [CIP-21](https://cips.cardano.org/cip/CIP-21), rather than reusing the original cached CBOR.
 *
 * \warning Clearing the CBOR cache may change the binary representation of the sub transaction when
 *          serialized, which can alter the sub transaction id and invalidate any existing signatures.
 *          Use this function with caution, especially if the sub transaction has already been signed or
 *          if preserving the exact CBOR encoding is important for your application.
 *
 * Usage Example:
 * \code{.c}
 * // Assume sub_transaction was created using cardano_sub_transaction_from_cbor
 * cardano_sub_transaction_t* sub_transaction = ...;
 *
 * // Modify the sub transaction as needed
 *
 * // Clear the CBOR cache to ensure serialization uses the updated sub transaction
 * cardano_sub_transaction_clear_cbor_cache(sub_transaction);
 *
 * // Serialize the sub transaction to CBOR
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * cardano_error_t result = cardano_sub_transaction_to_cbor(sub_transaction, writer);
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
 * cardano_sub_transaction_unref(&sub_transaction);
 * \endcode
 */
CARDANO_EXPORT void cardano_sub_transaction_clear_cbor_cache(cardano_sub_transaction_t* sub_transaction);

/**
 * \brief Decrements the reference count of a cardano_sub_transaction_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_sub_transaction_t object
 * by decreasing its reference count. When the reference count reaches zero, the sub transaction is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] sub_transaction A pointer to the pointer of the sub transaction object. This double
 *                                indirection allows the function to set the caller's pointer to
 *                                NULL, avoiding dangling pointer issues after the object has been
 *                                freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_sub_transaction_t* sub_transaction = ...; // Assume sub transaction is already initialized
 *
 * // Perform operations with the sub transaction...
 *
 * cardano_sub_transaction_unref(&sub_transaction);
 * // At this point, sub_transaction is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_sub_transaction_unref, the pointer to the \ref cardano_sub_transaction_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_sub_transaction_unref(cardano_sub_transaction_t** sub_transaction);

/**
 * \brief Increases the reference count of the cardano_sub_transaction_t object.
 *
 * This function is used to manually increment the reference count of an cardano_sub_transaction_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_sub_transaction_unref.
 *
 * \param sub_transaction A pointer to the cardano_sub_transaction_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming sub_transaction is a previously created sub transaction object
 *
 * cardano_sub_transaction_ref(sub_transaction);
 *
 * // Now sub_transaction can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_sub_transaction_ref there is a corresponding
 * call to \ref cardano_sub_transaction_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_sub_transaction_ref(cardano_sub_transaction_t* sub_transaction);

/**
 * \brief Retrieves the current reference count of the cardano_sub_transaction_t object.
 *
 * This function returns the number of active references to an cardano_sub_transaction_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_sub_transaction_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param sub_transaction A pointer to the cardano_sub_transaction_t object whose reference count is queried.
 *                        The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_sub_transaction_t object. If the object
 * is properly managed (i.e., every \ref cardano_sub_transaction_ref call is matched with a
 * \ref cardano_sub_transaction_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming sub_transaction is a previously created sub transaction object
 *
 * size_t ref_count = cardano_sub_transaction_refcount(sub_transaction);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_sub_transaction_refcount(const cardano_sub_transaction_t* sub_transaction);

/**
 * \brief Sets the last error message for a given cardano_sub_transaction_t object.
 *
 * Records an error message in the sub transaction's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] sub_transaction A pointer to the \ref cardano_sub_transaction_t instance whose last error message is
 *                            to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the sub transaction's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_sub_transaction_set_last_error(
  cardano_sub_transaction_t* sub_transaction,
  const char*                message);

/**
 * \brief Retrieves the last error message recorded for a specific sub transaction.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_sub_transaction_set_last_error for the given
 * sub transaction. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] sub_transaction A pointer to the \ref cardano_sub_transaction_t instance whose last error
 *                            message is to be retrieved. If the sub transaction is NULL, the function
 *                            returns a generic error message indicating the null sub transaction.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified sub transaction. If the sub transaction is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_sub_transaction_set_last_error for the same sub transaction, or until
 *       the sub transaction is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_sub_transaction_get_last_error(
  const cardano_sub_transaction_t* sub_transaction);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SUB_TRANSACTION_H
