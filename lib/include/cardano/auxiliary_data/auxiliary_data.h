/**
 * \file auxiliary_data.h
 *
 * \author angel.castillo
 * \date   Sep 19, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_AUXILIARY_DATA_H
#define BIGLUP_LABS_INCLUDE_CARDANO_AUXILIARY_DATA_H

/* INCLUDES ******************************************************************/

#include <cardano/auxiliary_data/plutus_v1_script_list.h>
#include <cardano/auxiliary_data/plutus_v2_script_list.h>
#include <cardano/auxiliary_data/plutus_v3_script_list.h>
#include <cardano/auxiliary_data/transaction_metadata.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/scripts/native_scripts/native_script_list.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Auxiliary Data encapsulate certain optional information that can be attached
 * to a transaction. This data includes transaction metadata and scripts.
 *
 * The Auxiliary Data is hashed and referenced in the transaction body.
 */
typedef struct cardano_auxiliary_data_t cardano_auxiliary_data_t;

/**
 * \brief Creates a new, empty auxiliary data object.
 *
 * This function allocates and initializes a new \ref cardano_auxiliary_data_t object, which can be used to
 * hold transaction metadata or scripts that are included as auxiliary data in a Cardano transaction.
 *
 * \param[out] auxiliary_data A pointer to a location where the newly created \ref cardano_auxiliary_data_t
 *                            object will be stored. This parameter must not be NULL. On success, \p *auxiliary_data
 *                            will point to the newly created auxiliary data object.
 *
 * \return \ref CARDANO_SUCCESS if the auxiliary data object was successfully created, or an appropriate
 *         error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the
 *         \p auxiliary_data pointer is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if memory allocation failed.
 *
 * \note The caller is responsible for managing the lifecycle of the created auxiliary data object.
 *       When the auxiliary data object is no longer needed, the caller must release it by calling
 *       \ref cardano_auxiliary_data_unref.
 *
 * Usage Example:
 * \code{.c}
 *
 * cardano_auxiliary_data_t* auxiliary_data = NULL;
 * cardano_error_t result = cardano_auxiliary_data_new(&transaction_output);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the auxiliary data
 *   // Free resources when done
 *   cardano_auxiliary_data_unref(&transaction_output);
 * }
 * else
 * {
 *   printf("Failed to create the auxiliary data: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_auxiliary_data_new(cardano_auxiliary_data_t** auxiliary_data);

/**
 * \brief Creates a \ref cardano_auxiliary_data_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_auxiliary_data_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a auxiliary_data.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] auxiliary_data A pointer to a pointer of \ref cardano_auxiliary_data_t that will be set to the address
 *                        of the newly created auxiliary_data object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the auxiliary data were successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_auxiliary_data_t object by calling
 *       \ref cardano_auxiliary_data_unref when it is no longer needed.
 *
 * \remark In Cardano, transactions are encoded in CBOR, but CBOR allows multiple valid ways to encode the same data. The Cardano blockchain
 *         does not enforce a canonical transaction representation, meaning that if you decode a transaction from CBOR and then re-encode it,
 *         the resulting encoding could be different. This would change the auxiliary data hash and invalidate any existing signatures.
 *         To prevent this, when a transaction auxiliary data object is created using \ref cardano_auxiliary_data_from_cbor, it caches the original
 *         CBOR representation internally. When \ref cardano_auxiliary_data_to_cbor is called, it will output the cached CBOR.
 *         If the cached CBOR representation is not needed, the client can call \ref cardano_auxiliary_data_clear_cbor_cache after the object has been created.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_auxiliary_data_t* auxiliary_data = NULL;
 *
 * cardano_error_t result = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the auxiliary_data
 *
 *   // Once done, ensure to clean up and release the auxiliary_data
 *   cardano_auxiliary_data_unref(&auxiliary_data);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode auxiliary_data: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_auxiliary_data_from_cbor(cardano_cbor_reader_t* reader, cardano_auxiliary_data_t** auxiliary_data);

/**
 * \brief Serializes auxiliary data into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_auxiliary_data_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] auxiliary_data A constant pointer to the \ref cardano_auxiliary_data_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p auxiliary_data or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * \remark If the auxiliary data object was created by \ref cardano_auxiliary_data_from_cbor, the original CBOR encoding
 *         will be cached and reused by this function to prevent round-trip encoding issues that can change the auxiliary data hash
 *         and invalidate signatures.
 *
 * Usage Example:
 * \code{.c}
 * cardano_auxiliary_data_t* auxiliary_data = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_auxiliary_data_to_cbor(auxiliary_data, writer);
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
 * cardano_auxiliary_data_unref(&auxiliary_data);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_auxiliary_data_to_cbor(
  const cardano_auxiliary_data_t* auxiliary_data,
  cardano_cbor_writer_t*          writer);

/**
 * \brief Retrieves the transaction metadata from an auxiliary data object.
 *
 * This function extracts the transaction metadata from a given \ref cardano_auxiliary_data_t object.
 *
 * \param[in] auxiliary_data A pointer to an initialized \ref cardano_auxiliary_data_t object from which to retrieve the transaction metadata.
 *                           This parameter must not be NULL.
 *
 * \return A pointer to a \ref cardano_transaction_metadata_t object containing the transaction metadata.
 *         If the auxiliary data does not contain any transaction metadata, the function returns \c NULL.
 *         The returned object is a new reference, and the caller is responsible for managing its lifecycle.
 *         Specifically, the caller must release it by calling \ref cardano_transaction_metadata_unref when it is no longer needed.
 *
 * \note The transaction metadata is optional in an auxiliary data object. Always check the returned pointer for \c NULL before using it.
 *
 * Usage Example:
 * \code{.c}
 * // Assume auxiliary_data is already initialized and may contain metadata
 * cardano_auxiliary_data_t* auxiliary_data = ...;
 *
 * // Retrieve the transaction metadata
 * cardano_transaction_metadata_t* metadata = cardano_auxiliary_data_get_transaction_metadata(auxiliary_data);
 *
 * if (metadata != NULL)
 * {
 *   // Process the metadata
 *   // Release the metadata object when done
 *   cardano_transaction_metadata_unref(&metadata);
 * }
 * else
 * {
 *   printf("No transaction metadata present in the auxiliary data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_transaction_metadata_t* cardano_auxiliary_data_get_transaction_metadata(cardano_auxiliary_data_t* auxiliary_data);

/**
 * \brief Sets or unsets the transaction metadata for an auxiliary data object.
 *
 * This function assigns the specified transaction metadata to a given \ref cardano_auxiliary_data_t object.
 * If the \p metadata parameter is `NULL`, it will remove any existing transaction metadata from the auxiliary data.
 *
 * \param[in,out] auxiliary_data A pointer to an initialized \ref cardano_auxiliary_data_t object
 *                               to which the transaction metadata will be set or unset. This parameter must not be `NULL`.
 * \param[in] metadata A pointer to an initialized \ref cardano_transaction_metadata_t object containing the metadata
 *                     to be associated with the auxiliary data, or `NULL` to unset the metadata.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the metadata
 *         was successfully set or unset, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the \p auxiliary_data pointer is `NULL`.
 *
 * \note If \p metadata is not `NULL`, this function increases the reference count of the \p metadata object, meaning that the caller retains
 *       ownership of their reference and is responsible for managing its lifecycle. The caller must ensure that
 *       the \ref cardano_transaction_metadata_unref function is called when the \p metadata object is no longer needed.
 *       If \p metadata is `NULL`, any existing metadata in the auxiliary data will be removed.
 *
 * Usage Example:
 * \code{.c}
 * // Assume auxiliary_data and metadata are already initialized
 * cardano_auxiliary_data_t* auxiliary_data = ...;
 * cardano_transaction_metadata_t* metadata = ...;
 *
 * // Set the transaction metadata for the auxiliary data object
 * cardano_error_t result = cardano_auxiliary_data_set_transaction_metadata(auxiliary_data, metadata);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Transaction metadata successfully set.\n");
 * }
 * else
 * {
 *   printf("Failed to set transaction metadata: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Unset the transaction metadata by passing NULL
 * result = cardano_auxiliary_data_set_transaction_metadata(auxiliary_data, NULL);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Transaction metadata successfully unset.\n");
 * }
 * else
 * {
 *   printf("Failed to unset transaction metadata: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_auxiliary_data_set_transaction_metadata(cardano_auxiliary_data_t* auxiliary_data, cardano_transaction_metadata_t* metadata);

/**
 * \brief Retrieves the list of native scripts from auxiliary data.
 *
 * This function extracts and returns a list of native scripts from the specified \ref cardano_auxiliary_data_t object.
 *
 * \param[in] auxiliary_data A pointer of an initialized \ref cardano_auxiliary_data_t object from which
 *                           the native scripts will be retrieved. This parameter must not be `NULL`.
 *
 * \return A pointer to a \ref cardano_native_script_list_t object representing the list of native scripts. If the auxiliary data
 *         does not contain any native scripts, this function returns `NULL`. The returned list is a new reference,
 *         and the caller is responsible for managing its lifecycle. Specifically, the caller must release the list by calling
 *         \ref cardano_native_script_list_unref when it is no longer needed.
 *
 * \note The returned native script list, if present, is a new reference, meaning the caller must manage the memory for this object.
 *       If the auxiliary data contains no native scripts, the function will return `NULL`, and the caller should handle this case appropriately.
 *
 * \return Returns \ref CARDANO_SUCCESS if the native scripts were successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_auxiliary_data_t* auxiliary_data = ...; // Assume initialized
 * cardano_native_script_list_t* native_scripts = cardano_auxiliary_data_get_native_scripts(auxiliary_data);
 *
 * if (native_scripts != NULL)
 * {
 *   // Use the native scripts
 *
 *   // Ensure to release the native script list when done
 *   cardano_native_script_list_unref(&native_scripts);
 * }
 * else
 * {
 *   printf("No native scripts found in auxiliary data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_native_script_list_t* cardano_auxiliary_data_get_native_scripts(cardano_auxiliary_data_t* auxiliary_data);

/**
 * \brief Sets the list of native scripts in auxiliary data.
 *
 * This function assigns a list of native scripts to the specified \ref cardano_auxiliary_data_t object.
 *
 * \param[in,out] auxiliary_data A pointer to an initialized \ref cardano_auxiliary_data_t object where the native scripts will be set.
 * \param[in] scripts A pointer to a \ref cardano_native_script_list_t object representing the list of native scripts to be added.
 *                    This parameter can be `NULL` to unset the native scripts in the auxiliary data.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the native scripts were
 *         successfully set, or an appropriate error code if an error occurred, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any
 *         required pointers are `NULL`.
 *
 * \note If the `scripts` parameter is `NULL`, any existing native scripts in the auxiliary data will be removed.
 *       The caller is responsible for managing the memory for the `scripts` object; this function will increment
 *       the reference count of the `scripts` object if provided.
 *
 * Usage Example:
 * \code{.c}
 * cardano_auxiliary_data_t* auxiliary_data = ...; // Assume initialized
 * cardano_native_script_list_t* native_scripts = ...; // Assume native_scripts is initialized
 *
 * cardano_error_t result = cardano_auxiliary_data_set_native_scripts(auxiliary_data, native_scripts);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Native scripts successfully set in auxiliary data.\n");
 * }
 * else
 * {
 *   printf("Failed to set native scripts: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up resources when done
 * cardano_native_script_list_unref(&native_scripts);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_auxiliary_data_set_native_scripts(cardano_auxiliary_data_t* auxiliary_data, cardano_native_script_list_t* scripts);

/**
 * \brief Retrieves the list of plutus v1 scripts from auxiliary data.
 *
 * This function extracts and returns a list of plutus v1 scripts from the specified \ref cardano_auxiliary_data_t object.
 *
 * \param[in] auxiliary_data A pointer of an initialized \ref cardano_auxiliary_data_t object from which
 *                           the plutus v1 scripts will be retrieved. This parameter must not be `NULL`.
 *
 * \return A pointer to a \ref cardano_plutus_v1_script_list_t object representing the list of plutus v1 scripts. If the auxiliary data
 *         does not contain any plutus v1 scripts, this function returns `NULL`. The returned list is a new reference,
 *         and the caller is responsible for managing its lifecycle. Specifically, the caller must release the list by calling
 *         \ref cardano_plutus_v1_script_list_unref when it is no longer needed.
 *
 * \note The returned plutus v1 script list, if present, is a new reference, meaning the caller must manage the memory for this object.
 *       If the auxiliary data contains no plutus v1 scripts, the function will return `NULL`, and the caller should handle this case appropriately.
 *
 * \return Returns \ref CARDANO_SUCCESS if the plutus v1 scripts were successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_auxiliary_data_t* auxiliary_data = ...; // Assume initialized
 * cardano_plutus_v1_script_list_t* plutus_v1_scripts = cardano_auxiliary_data_get_plutus_v1_scripts(auxiliary_data);
 *
 * if (plutus_v1_scripts != NULL)
 * {
 *   // Use the plutus v1 scripts
 *
 *   // Ensure to release the plutus v1 script list when done
 *   cardano_plutus_v1_script_list_unref(&plutus_v1_scripts);
 * }
 * else
 * {
 *   printf("No plutus v1 scripts found in auxiliary data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_plutus_v1_script_list_t* cardano_auxiliary_data_get_plutus_v1_scripts(cardano_auxiliary_data_t* auxiliary_data);

/**
 * \brief Sets the list of plutus v1 scripts in auxiliary data.
 *
 * This function assigns a list of plutus v1 scripts to the specified \ref cardano_auxiliary_data_t object.
 *
 * \param[in,out] auxiliary_data A pointer to an initialized \ref cardano_auxiliary_data_t object where the plutus v1 scripts will be set.
 * \param[in] scripts A pointer to a \ref cardano_plutus_v1_script_list_t object representing the list of plutus v1 scripts to be added.
 *                    This parameter can be `NULL` to unset the plutus v1 scripts in the auxiliary data.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the plutus v1 scripts were
 *         successfully set, or an appropriate error code if an error occurred, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any
 *         required pointers are `NULL`.
 *
 * \note If the `scripts` parameter is `NULL`, any existing plutus v1 scripts in the auxiliary data will be removed.
 *       The caller is responsible for managing the memory for the `scripts` object; this function will increment
 *       the reference count of the `scripts` object if provided.
 *
 * Usage Example:
 * \code{.c}
 * cardano_auxiliary_data_t* auxiliary_data = ...; // Assume initialized
 * cardano_plutus_v1_script_list_t* plutus_v1_scripts = ...; // Assume plutus_v1_scripts is initialized
 *
 * cardano_error_t result = cardano_auxiliary_data_set_plutus_v1_scripts(auxiliary_data, plutus_v1_scripts);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Native scripts successfully set in auxiliary data.\n");
 * }
 * else
 * {
 *   printf("Failed to set plutus v1 scripts: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up resources when done
 * cardano_plutus_v1_script_list_unref(&plutus_v1_scripts);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_auxiliary_data_set_plutus_v1_scripts(cardano_auxiliary_data_t* auxiliary_data, cardano_plutus_v1_script_list_t* scripts);

/**
 * \brief Retrieves the list of plutus v2 scripts from auxiliary data.
 *
 * This function extracts and returns a list of plutus v2 scripts from the specified \ref cardano_auxiliary_data_t object.
 *
 * \param[in] auxiliary_data A pointer of an initialized \ref cardano_auxiliary_data_t object from which
 *                           the plutus v2 scripts will be retrieved. This parameter must not be `NULL`.
 *
 * \return A pointer to a \ref cardano_plutus_v2_script_list_t object representing the list of plutus v2 scripts. If the auxiliary data
 *         does not contain any plutus v2 scripts, this function returns `NULL`. The returned list is a new reference,
 *         and the caller is responsible for managing its lifecycle. Specifically, the caller must release the list by calling
 *         \ref cardano_plutus_v2_script_list_unref when it is no longer needed.
 *
 * \note The returned plutus v2 script list, if present, is a new reference, meaning the caller must manage the memory for this object.
 *       If the auxiliary data contains no plutus v2 scripts, the function will return `NULL`, and the caller should handle this case appropriately.
 *
 * \return Returns \ref CARDANO_SUCCESS if the plutus v2 scripts were successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_auxiliary_data_t* auxiliary_data = ...; // Assume initialized
 * cardano_plutus_v2_script_list_t* plutus_v2_scripts = cardano_auxiliary_data_get_plutus_v2_scripts(auxiliary_data);
 *
 * if (plutus_v2_scripts != NULL)
 * {
 *   // Use the plutus v2 scripts
 *
 *   // Ensure to release the plutus v2 script list when done
 *   cardano_plutus_v2_script_list_unref(&plutus_v2_scripts);
 * }
 * else
 * {
 *   printf("No plutus v2 scripts found in auxiliary data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_plutus_v2_script_list_t* cardano_auxiliary_data_get_plutus_v2_scripts(cardano_auxiliary_data_t* auxiliary_data);

/**
 * \brief Sets the list of plutus v2 scripts in auxiliary data.
 *
 * This function assigns a list of plutus v2 scripts to the specified \ref cardano_auxiliary_data_t object.
 *
 * \param[in,out] auxiliary_data A pointer to an initialized \ref cardano_auxiliary_data_t object where the plutus v2 scripts will be set.
 * \param[in] scripts A pointer to a \ref cardano_plutus_v2_script_list_t object representing the list of plutus v2 scripts to be added.
 *                    This parameter can be `NULL` to unset the plutus v2 scripts in the auxiliary data.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the plutus v2 scripts were
 *         successfully set, or an appropriate error code if an error occurred, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any
 *         required pointers are `NULL`.
 *
 * \note If the `scripts` parameter is `NULL`, any existing plutus v2 scripts in the auxiliary data will be removed.
 *       The caller is responsible for managing the memory for the `scripts` object; this function will increment
 *       the reference count of the `scripts` object if provided.
 *
 * Usage Example:
 * \code{.c}
 * cardano_auxiliary_data_t* auxiliary_data = ...; // Assume initialized
 * cardano_plutus_v2_script_list_t* plutus_v2_scripts = ...; // Assume plutus_v2_scripts is initialized
 *
 * cardano_error_t result = cardano_auxiliary_data_set_plutus_v2_scripts(auxiliary_data, plutus_v2_scripts);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Native scripts successfully set in auxiliary data.\n");
 * }
 * else
 * {
 *   printf("Failed to set plutus v2 scripts: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up resources when done
 * cardano_plutus_v2_script_list_unref(&plutus_v2_scripts);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_auxiliary_data_set_plutus_v2_scripts(cardano_auxiliary_data_t* auxiliary_data, cardano_plutus_v2_script_list_t* scripts);

/**
 * \brief Retrieves the list of plutus v3 scripts from auxiliary data.
 *
 * This function extracts and returns a list of plutus v3 scripts from the specified \ref cardano_auxiliary_data_t object.
 *
 * \param[in] auxiliary_data A pointer of an initialized \ref cardano_auxiliary_data_t object from which
 *                           the plutus v3 scripts will be retrieved. This parameter must not be `NULL`.
 *
 * \return A pointer to a \ref cardano_plutus_v3_script_list_t object representing the list of plutus v3 scripts. If the auxiliary data
 *         does not contain any plutus v3 scripts, this function returns `NULL`. The returned list is a new reference,
 *         and the caller is responsible for managing its lifecycle. Specifically, the caller must release the list by calling
 *         \ref cardano_plutus_v3_script_list_unref when it is no longer needed.
 *
 * \note The returned plutus v3 script list, if present, is a new reference, meaning the caller must manage the memory for this object.
 *       If the auxiliary data contains no plutus v3 scripts, the function will return `NULL`, and the caller should handle this case appropriately.
 *
 * \return Returns \ref CARDANO_SUCCESS if the plutus v3 scripts were successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_auxiliary_data_t* auxiliary_data = ...; // Assume initialized
 * cardano_plutus_v3_script_list_t* plutus_v3_scripts = cardano_auxiliary_data_get_plutus_v3_scripts(auxiliary_data);
 *
 * if (plutus_v3_scripts != NULL)
 * {
 *   // Use the plutus v3 scripts
 *
 *   // Ensure to release the plutus v3 script list when done
 *   cardano_plutus_v3_script_list_unref(&plutus_v3_scripts);
 * }
 * else
 * {
 *   printf("No plutus v3 scripts found in auxiliary data.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_plutus_v3_script_list_t* cardano_auxiliary_data_get_plutus_v3_scripts(cardano_auxiliary_data_t* auxiliary_data);

/**
 * \brief Sets the list of plutus v3 scripts in auxiliary data.
 *
 * This function assigns a list of plutus v3 scripts to the specified \ref cardano_auxiliary_data_t object.
 *
 * \param[in,out] auxiliary_data A pointer to an initialized \ref cardano_auxiliary_data_t object where the plutus v3 scripts will be set.
 * \param[in] scripts A pointer to a \ref cardano_plutus_v3_script_list_t object representing the list of plutus v3 scripts to be added.
 *                    This parameter can be `NULL` to unset the plutus v3 scripts in the auxiliary data.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the plutus v3 scripts were
 *         successfully set, or an appropriate error code if an error occurred, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any
 *         required pointers are `NULL`.
 *
 * \note If the `scripts` parameter is `NULL`, any existing plutus v3 scripts in the auxiliary data will be removed.
 *       The caller is responsible for managing the memory for the `scripts` object; this function will increment
 *       the reference count of the `scripts` object if provided.
 *
 * Usage Example:
 * \code{.c}
 * cardano_auxiliary_data_t* auxiliary_data = ...; // Assume initialized
 * cardano_plutus_v3_script_list_t* plutus_v3_scripts = ...; // Assume plutus_v3_scripts is initialized
 *
 * cardano_error_t result = cardano_auxiliary_data_set_plutus_v3_scripts(auxiliary_data, plutus_v3_scripts);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Native scripts successfully set in auxiliary data.\n");
 * }
 * else
 * {
 *   printf("Failed to set plutus v3 scripts: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up resources when done
 * cardano_plutus_v3_script_list_unref(&plutus_v3_scripts);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_auxiliary_data_set_plutus_v3_scripts(cardano_auxiliary_data_t* auxiliary_data, cardano_plutus_v3_script_list_t* scripts);

/**
 * \brief Retrieves the hash of a auxiliary data.
 *
 * This function computes and returns the hash of the given \ref cardano_auxiliary_data_t object. The hash is a unique identifier for the auxiliary data.
 *
 * \param[in] auxiliary_data A pointer to an initialized \ref cardano_auxiliary_data_t object. The object must be valid and not NULL.
 *
 * \return A pointer to a \ref cardano_blake2b_hash_t object representing the auxiliary data hash. The returned object is a new reference, and
 *         the caller is responsible for releasing it by calling \ref cardano_blake2b_hash_unref when it is no longer needed.
 *         If the input is NULL, the function will return NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_auxiliary_data_t* auxiliary_data = ...; // Assume auxiliary_data is already initialized
 * cardano_blake2b_hash_t* tx_body_hash = cardano_auxiliary_data_get_hash(auxiliary_data);
 *
 * if (tx_body_hash != NULL)
 * {
 *   // Use the auxiliary data hash for signing or verification
 *
 *   // Clean up when done
 *   cardano_blake2b_hash_unref(&tx_body_hash);
 * }
 * else
 * {
 *   printf("Failed to retrieve the auxiliary data hash.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_auxiliary_data_get_hash(cardano_auxiliary_data_t* auxiliary_data);

/**
 * \brief Clears the cached CBOR representation from a auxiliary data.
 *
 * This function removes the internally cached CBOR data from a \ref cardano_auxiliary_data_t object.
 * It is useful when you have modified the auxiliary data after it was created from CBOR using
 * \ref cardano_auxiliary_data_from_cbor and you want to ensure that the next serialization reflects
 * the current state of the auxiliary data, rather than using the original cached CBOR.
 *
 * \param[in,out] auxiliary_data A pointer to an initialized \ref cardano_auxiliary_data_t object
 *                                 from which the CBOR cache will be cleared.
 *
 * \warning Clearing the CBOR cache may change the binary representation of the auxiliary data when
 *          serialized, which can alter the auxiliary data hash and invalidate any existing signatures.
 *          Use this function with caution, especially if the transaction has already been signed or
 *          if preserving the exact CBOR encoding is important for your application.
 *
 * Usage Example:
 * \code{.c}
 * // Assume auxiliary_data was created using cardano_auxiliary_data_from_cbor
 * cardano_auxiliary_data_t* auxiliary_data = ...;
 *
 * // Modify the auxiliary data as needed
 *
 * // Clear the CBOR cache to ensure serialization uses the updated auxiliary data
 * cardano_auxiliary_data_clear_cbor_cache(auxiliary_data);
 *
 * // Serialize the auxiliary data to CBOR
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * result = cardano_auxiliary_data_to_cbor(auxiliary_data, writer);
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
 * cardano_auxiliary_data_unref(&auxiliary_data);
 * \endcode
 */
CARDANO_EXPORT void cardano_auxiliary_data_clear_cbor_cache(cardano_auxiliary_data_t* auxiliary_data);

/**
 * \brief Decrements the reference count of a cardano_auxiliary_data_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_auxiliary_data_t object
 * by decreasing its reference count. When the reference count reaches zero, the auxiliary_data is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] auxiliary_data A pointer to the pointer of the auxiliary_data object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_auxiliary_data_t* auxiliary_data = cardano_auxiliary_data_new(major, minor);
 *
 * // Perform operations with the auxiliary_data...
 *
 * cardano_auxiliary_data_unref(&auxiliary_data);
 * // At this point, auxiliary_data is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_auxiliary_data_unref, the pointer to the \ref cardano_auxiliary_data_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_auxiliary_data_unref(cardano_auxiliary_data_t** auxiliary_data);

/**
 * \brief Increases the reference count of the cardano_auxiliary_data_t object.
 *
 * This function is used to manually increment the reference count of an cardano_auxiliary_data_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_auxiliary_data_unref.
 *
 * \param auxiliary_data A pointer to the cardano_auxiliary_data_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming auxiliary_data is a previously created auxiliary_data object
 *
 * cardano_auxiliary_data_ref(auxiliary_data);
 *
 * // Now auxiliary_data can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_auxiliary_data_ref there is a corresponding
 * call to \ref cardano_auxiliary_data_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_auxiliary_data_ref(cardano_auxiliary_data_t* auxiliary_data);

/**
 * \brief Retrieves the current reference count of the cardano_auxiliary_data_t object.
 *
 * This function returns the number of active references to an cardano_auxiliary_data_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_auxiliary_data_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param auxiliary_data A pointer to the cardano_auxiliary_data_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_auxiliary_data_t object. If the object
 * is properly managed (i.e., every \ref cardano_auxiliary_data_ref call is matched with a
 * \ref cardano_auxiliary_data_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming auxiliary_data is a previously created auxiliary_data object
 *
 * size_t ref_count = cardano_auxiliary_data_refcount(auxiliary_data);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_auxiliary_data_refcount(const cardano_auxiliary_data_t* auxiliary_data);

/**
 * \brief Sets the last error message for a given cardano_auxiliary_data_t object.
 *
 * Records an error message in the auxiliary_data's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] auxiliary_data A pointer to the \ref cardano_auxiliary_data_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the auxiliary_data's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_auxiliary_data_set_last_error(
  cardano_auxiliary_data_t* auxiliary_data,
  const char*               message);

/**
 * \brief Retrieves the last error message recorded for a specific auxiliary_data.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_auxiliary_data_set_last_error for the given
 * auxiliary_data. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] auxiliary_data A pointer to the \ref cardano_auxiliary_data_t instance whose last error
 *                   message is to be retrieved. If the auxiliary_data is NULL, the function
 *                   returns a generic error message indicating the null auxiliary_data.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified auxiliary_data. If the auxiliary_data is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_auxiliary_data_set_last_error for the same auxiliary_data, or until
 *       the auxiliary_data is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_auxiliary_data_get_last_error(
  const cardano_auxiliary_data_t* auxiliary_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_AUXILIARY_DATA_H