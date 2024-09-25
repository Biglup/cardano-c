/**
 * \file transaction_metadata.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_METADATA_H
#define BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_METADATA_H

/* INCLUDES ******************************************************************/

#include <cardano/auxiliary_data/metadatum.h>
#include <cardano/auxiliary_data/metadatum_label_list.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a map of metadatum_label to metadatum.
 */
typedef struct cardano_transaction_metadata_t cardano_transaction_metadata_t;

/**
 * \brief Creates and initializes a new instance of a transaction_metadata.
 *
 * This function allocates and initializes a new instance of \ref cardano_transaction_metadata_t.
 * It returns an error code to indicate the success or failure of the operation.
 *
 * \param[out] transaction_metadata A pointer to a pointer to a \ref cardano_transaction_metadata_t object. Upon successful
 *                        initialization, this will point to a newly created \ref cardano_transaction_metadata_t
 *                        object. This object represents a "strong reference" to the transaction_metadata,
 *                        fully initialized and ready for use. The caller is responsible for managing
 *                        the lifecycle of this object. Specifically, once the transaction_metadata is no longer
 *                        needed, the caller must release it by calling \ref cardano_transaction_metadata_unref.
 *
 * \return \ref CARDANO_SUCCESS if the transaction_metadata was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_metadata_t* transaction_metadata = NULL;
 *
 * // Attempt to create a new transaction_metadata
 * cardano_error_t result = cardano_transaction_metadata_new(&transaction_metadata);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the transaction_metadata
 *
 *   // Once done, ensure to clean up and release the transaction_metadata
 *   cardano_transaction_metadata_unref(&transaction_metadata);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_metadata_new(cardano_transaction_metadata_t** transaction_metadata);

/**
 * \brief Creates a transaction_metadata from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_transaction_metadata_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a transaction_metadata.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded transaction_metadata data.
 * \param[out] transaction_metadata A pointer to a pointer of \ref cardano_transaction_metadata_t that will be set to the address
 *                        of the newly created transaction_metadata object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the transaction_metadata was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_transaction_metadata_t object by calling
 *       \ref cardano_transaction_metadata_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_transaction_metadata_t* transaction_metadata = NULL;
 *
 * cardano_error_t result = cardano_transaction_metadata_from_cbor(reader, &transaction_metadata);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the transaction_metadata
 *
 *   // Once done, ensure to clean up and release the transaction_metadata
 *   cardano_transaction_metadata_unref(&transaction_metadata);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode transaction_metadata: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_metadata_from_cbor(cardano_cbor_reader_t* reader, cardano_transaction_metadata_t** transaction_metadata);

/**
 * \brief Serializes a transaction_metadata into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_transaction_metadata_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] transaction_metadata A constant pointer to the \ref cardano_transaction_metadata_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p transaction_metadata or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_metadata_t* transaction_metadata = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_transaction_metadata_to_cbor(transaction_metadata, writer);
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
 * cardano_transaction_metadata_unref(&transaction_metadata);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_transaction_metadata_to_cbor(
  const cardano_transaction_metadata_t* transaction_metadata,
  cardano_cbor_writer_t*                writer);

/**
 * \brief Retrieves the length of the transaction metadata.
 *
 * This function returns the number of key-value pairs contained in the specified transaction metadata.
 *
 * \param[in] transaction_metadata A constant pointer to the \ref cardano_transaction_metadata_t object for which
 *                       the length is to be retrieved.
 *
 * \return The number of key-value pairs in the transaction_metadata. Returns 0 if the transaction_metadata is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_metadata_t* transaction_metadata = cardano_transaction_metadata_new();
 *
 * // Populate the transaction_metadata with key-value pairs
 *
 * size_t length = cardano_transaction_metadata_get_length(transaction_metadata);
 * printf("The length of the transaction_metadata is: %zu\n", length);
 *
 * cardano_transaction_metadata_unref(&transaction_metadata);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_transaction_metadata_get_length(const cardano_transaction_metadata_t* transaction_metadata);

/**
 * \brief Retrieves the value associated with a given key in the transaction_metadata.
 *
 * This function retrieves the value associated with the specified key in the provided transaction_metadata.
 * It returns the value through the output parameter `element`. If the key is not found in the transaction_metadata,
 * the output parameter `element` will be set to NULL.
 *
 * \param[in] transaction_metadata A constant pointer to the \ref cardano_transaction_metadata_t object from which
 *                       the value is to be retrieved.
 * \param[in] key The key whose associated value is to be retrieved from the transaction_metadata.
 * \param[out] element A pointer to a variable where the retrieved value will be stored. If the key
 *                     is found in the transaction_metadata, this variable will be set to the associated value.
 *                     If the key is not found, this variable will be set to NULL.
 *
 * \return \ref CARDANO_SUCCESS if the value associated with the key was successfully retrieved, or
 *         an appropriate error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_metadata_t* transaction_metadata = cardano_transaction_metadata_new();
 *
 * // Populate the transaction_metadata with key-value pairs
 *
 * uint64_t key = 0;
 * cardano_metadatum_t* value = NULL;
 * cardano_error_t result = cardano_transaction_metadata_get(transaction_metadata, key, &value);
 *
 * if (result == CARDANO_SUCCESS && value != NULL)
 * {
 *   // Use the retrieved value
 *   // ...
 * }
 * else
 * {
 *   // Handle error or key not found
 * }
 *
 * cardano_metadatum_unref(&value); // Clean up the value resource
 * cardano_transaction_metadata_unref(&transaction_metadata);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_transaction_metadata_get(cardano_transaction_metadata_t* transaction_metadata, uint64_t key, cardano_metadatum_t** element);

/**
 * \brief Inserts a key-value pair into the transaction_metadata.
 *
 * This function inserts the specified key-value pair into the provided transaction_metadata.
 *
 * \param[in] transaction_metadata A constant pointer to the \ref cardano_transaction_metadata_t object where
 *                       the key-value pair is to be inserted.
 * \param[in] key The key to be inserted into the transaction_metadata.
 * \param[in] value The value to be associated with the key and inserted into the transaction_metadata.
 *                  The caller is responsible for managing the lifecycle of the value object.
 *
 * \return \ref CARDANO_SUCCESS if the key-value pair was successfully inserted, or an appropriate
 *         error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_metadata_t* transaction_metadata = cardano_transaction_metadata_new();
 *
 * // Create key and value objects
 * uint64_t key = 755;
 * cardano_metadatum_t* value = {...}; // Assume value is properly initialized
 *
 * // Insert the key-value pair into the transaction_metadata
 * cardano_error_t result = cardano_transaction_metadata_insert(transaction_metadata, key, value);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   // Handle insertion failure
 * }
 *
 * // Clean up key and value objects
 * cardano_metadatum_unref(&value);
 * cardano_transaction_metadata_unref(&transaction_metadata);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_transaction_metadata_insert(cardano_transaction_metadata_t* transaction_metadata, uint64_t key, cardano_metadatum_t* value);

/**
 * \brief Retrieves the keys from the transaction_metadata.
 *
 * This function retrieves all the keys from the provided transaction_metadata and returns them as a list.
 * The caller is responsible for managing the lifecycle of the returned list by calling
 * \ref cardano_metadatum_label_list_t when it is no longer needed.
 *
 * \param[in] transaction_metadata A constant pointer to the \ref cardano_transaction_metadata_t object from which
 *                       the keys are to be retrieved.
 * \param[out] keys A pointer to a variable where the retrieved keys will be stored as a list.
 *                  If successful, this variable will be set to point to the list of keys.
 *                  The caller is responsible for managing the lifecycle of this list.
 *                  It must be released by calling \ref cardano_metadatum_label_list_unref when no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the keys were successfully retrieved, or an appropriate
 *         error code if the input parameters are invalid or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_metadata_t* transaction_metadata = cardano_transaction_metadata_new();
 *
 * // Populate the transaction_metadata with key-value pairs
 *
 * cardano_metadatum_label_list_t* keys = NULL;
 * cardano_error_t result = cardano_transaction_metadata_get_keys(transaction_metadata, &keys);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the list of keys
 *   // Keys must also be freed if retrieved from the list
 *
 *   // Once done, ensure to clean up and release the keys list
 *   cardano_metadatum_label_list_unref(&keys);
 * }
 * else
 * {
 *   // Handle error
 * }
 *
 * cardano_transaction_metadata_unref(&transaction_metadata);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_transaction_metadata_get_keys(cardano_transaction_metadata_t* transaction_metadata, cardano_metadatum_label_list_t** keys);

/**
 * \brief Retrieves the metadatum label at a specific index from the transaction metadata.
 *
 * This function retrieves the metadatum label at the specified index from the transaction_metadata.
 *
 * \param[in] transaction_metadata Pointer to the transaction_metadata object.
 * \param[in] index The index of the metadatum label to retrieve.
 * \param[out] metadatum_label The metadatum label as \c uint64_t.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the metadatum label was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_metadata_t* transaction_metadata = NULL;
 * uint64_t metadatum_label = 0;
 * size_t index = 0; // Index of the key hash to retrieve
 *
 * // Assume transaction_metadata is initialized properly
 *
 * cardano_error_t result = cardano_transaction_metadata_get_key_at(transaction_metadata, index, &metadatum_label);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the metadatum_label
 * }
 * else
 * {
 *   // Handle the error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_transaction_metadata_get_key_at(
  const cardano_transaction_metadata_t* transaction_metadata,
  size_t                                index,
  uint64_t*                             metadatum_label);

/**
 * \brief Retrieves the metadatum at a specific index from the transaction_metadata.
 *
 * This function retrieves the metadatum at the specified index from the transaction_metadata.
 *
 * \param[in] transaction_metadata Pointer to the transaction_metadata object.
 * \param[in] index The index of the metadatum to retrieve.
 * \param[out] metadatum On successful retrieval, this will point to the metadatum
 *                    at the specified index.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the metadatum was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_metadata_t* transaction_metadata = NULL;
 * cardano_metadatum_t* metadatum = 0;
 * size_t index = 0; // Index of the metadatum to retrieve
 *
 * // Assume transaction_metadata is initialized properly
 *
 * cardano_error_t result = cardano_transaction_metadata_get_value_at(transaction_metadata, index, &metadatum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the metadatum
 *   cardano_metadatum_unref(&metadatum);
 * }
 * else
 * {
 *   // Handle the error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_transaction_metadata_get_value_at(
  const cardano_transaction_metadata_t* transaction_metadata,
  size_t                                index,
  cardano_metadatum_t**                 metadatum);

/**
 * \brief Retrieves the metadatum label and metadatum at the specified index.
 *
 * This function retrieves the metadatum label and metadatum from the metadata at the specified index.
 *
 * \param[in]  transaction_metadata Pointer to the proposed metadata object.
 * \param[in]  index                The index at which to retrieve the key-value pair.
 * \param[out] metadatum_label On successful retrieval, this will point to the metadatum label at the specified index.
 * \param[out] metadatum On successful retrieval, this will point to the metadatum at the specified index.
 *                            The caller is responsible for managing the lifecycle of this object and should release it using
 *                            \ref cardano_metadatum_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the key-value pair was successfully retrieved, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_metadata_t* transaction_metadata = NULL;
 * // Assume transaction_metadata is initialized properly
 *
 * size_t index = 0;
 * uint64_t metadatum_label = 0;
 * cardano_metadatum_t* metadatum = NULL;
 *
 * cardano_error_t result = cardano_transaction_metadata_get_key_value_at(transaction_metadata, index, &metadatum_label, &metadatum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   if (metadatum != NULL)
 *   {
 *     // Use the metadatum label and metadatum
 *   }
 *   else
 *   {
 *     printf("Key-value pair not set at index %zu.\n", index);
 *   }
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get key-value pair at index %zu.\n", index);
 * }
 *
 * // Clean up
 * cardano_transaction_metadata_unref(&transaction_metadata);
 * cardano_metadatum_unref(&metadatum);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_transaction_metadata_get_key_value_at(
  const cardano_transaction_metadata_t* transaction_metadata,
  size_t                                index,
  uint64_t*                             metadatum_label,
  cardano_metadatum_t**                 metadatum);

/**
 * \brief Decrements the reference count of a transaction_metadata object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_transaction_metadata_t object
 * by decreasing its reference count. When the reference count reaches zero, the transaction_metadata is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] transaction_metadata A pointer to the pointer of the transaction_metadata object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_metadata_t* transaction_metadata = cardano_transaction_metadata_new();
 *
 * // Perform operations with the transaction_metadata...
 *
 * cardano_transaction_metadata_unref(&transaction_metadata);
 * // At this point, transaction_metadata is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_transaction_metadata_unref, the pointer to the \ref cardano_transaction_metadata_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_transaction_metadata_unref(cardano_transaction_metadata_t** transaction_metadata);

/**
 * \brief Increases the reference count of the cardano_transaction_metadata_t object.
 *
 * This function is used to manually increment the reference count of a transaction_metadata
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_transaction_metadata_unref.
 *
 * \param transaction_metadata A pointer to the transaction_metadata object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming transaction_metadata is a previously created transaction_metadata object
 *
 * cardano_transaction_metadata_ref(transaction_metadata);
 *
 * // Now transaction_metadata can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_transaction_metadata_ref there is a corresponding
 * call to \ref cardano_transaction_metadata_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_transaction_metadata_ref(cardano_transaction_metadata_t* transaction_metadata);

/**
 * \brief Retrieves the current reference count of the cardano_transaction_metadata_t object.
 *
 * This function returns the number of active references to a transaction_metadata object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_transaction_metadata_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param transaction_metadata A pointer to the transaction_metadata object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified transaction_metadata object. If the object
 * is properly managed (i.e., every \ref cardano_transaction_metadata_ref call is matched with a
 * \ref cardano_transaction_metadata_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming transaction_metadata is a previously created transaction_metadata object
 *
 * size_t ref_count = cardano_transaction_metadata_refcount(transaction_metadata);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_transaction_metadata_refcount(const cardano_transaction_metadata_t* transaction_metadata);

/**
 * \brief Sets the last error message for a given transaction_metadata object.
 *
 * Records an error message in the transaction_metadata's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] transaction_metadata A pointer to the \ref cardano_transaction_metadata_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the transaction_metadata's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_transaction_metadata_set_last_error(cardano_transaction_metadata_t* transaction_metadata, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific transaction_metadata.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_transaction_metadata_set_last_error for the given
 * transaction_metadata. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] transaction_metadata A pointer to the \ref cardano_transaction_metadata_t instance whose last error
 *                   message is to be retrieved. If the transaction_metadata is NULL, the function
 *                   returns a generic error message indicating the null transaction_metadata.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified transaction_metadata. If the transaction_metadata is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_transaction_metadata_set_last_error for the same transaction_metadata, or until
 *       the transaction_metadata is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_transaction_metadata_get_last_error(const cardano_transaction_metadata_t* transaction_metadata);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_METADATA_H