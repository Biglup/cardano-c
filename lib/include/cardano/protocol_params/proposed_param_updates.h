/**
 * \file proposed_param_updates.h
 *
 * \author angel.castillo
 * \date   Jun 09, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_PROPOSED_PARAM_UPDATES_H
#define BIGLUP_LABS_INCLUDE_CARDANO_PROPOSED_PARAM_UPDATES_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/unit_interval.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/protocol_params/protocol_param_update.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief In the Cardano network, stakeholders can propose changes to the protocol parameters. These proposals are then
 * collected into a set which represents the ProposedProtocolParameterUpdates.
 *
 * This proposed protocol parameter updates are represented as a map of genesis delegate key hash to parameters updates. So in principles,
 * each genesis delegate can propose a different update.
 */
typedef struct cardano_proposed_param_updates_t cardano_proposed_param_updates_t;

/**
 * \brief Creates and initializes a new instance of the proposed protocol parameter updates.
 *
 * This function allocates and initializes a new instance of the proposed protocol parameter updates,
 * representing a set of proposed changes to the Cardano protocol parameters.
 *
 * \param[out] proposed_param_updates On successful initialization, this will point to a newly created
 *            proposed protocol parameter updates object. This object represents a "strong reference",
 *            meaning that it is fully initialized and ready for use.
 *            The caller is responsible for managing the lifecycle of this object.
 *            Specifically, once the proposed protocol parameter updates object is no longer needed,
 *            the caller must release it by calling \ref cardano_proposed_param_updates_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the proposed protocol parameter updates object was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposed_param_updates_t* proposed_param_updates = NULL;
 *
 * // Attempt to create a new proposed protocol parameter updates object
 * cardano_error_t result = cardano_proposed_param_updates_new(&proposed_param_updates);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the proposed_param_updates
 *
 *   // Once done, ensure to clean up and release the proposed_param_updates
 *   cardano_proposed_param_updates_unref(&proposed_param_updates);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposed_param_updates_new(cardano_proposed_param_updates_t** proposed_param_updates);

/**
 * \brief Creates a \ref cardano_proposed_param_updates_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_proposed_param_updates_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a proposed_param_updates.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] proposed_param_updates A pointer to a pointer of \ref cardano_proposed_param_updates_t that will be set to the address
 *                        of the newly created proposed_param_updates object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_proposed_param_updates_t object by calling
 *       \ref cardano_proposed_param_updates_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_proposed_param_updates_t* proposed_param_updates = NULL;
 *
 * cardano_error_t result = cardano_proposed_param_updates_from_cbor(reader, &proposed_param_updates);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the proposed_param_updates
 *
 *   // Once done, ensure to clean up and release the proposed_param_updates
 *   cardano_proposed_param_updates_unref(&proposed_param_updates);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode proposed_param_updates: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposed_param_updates_from_cbor(cardano_cbor_reader_t* reader, cardano_proposed_param_updates_t** proposed_param_updates);

/**
 * \brief Serializes protocol version into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_proposed_param_updates_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] proposed_param_updates A constant pointer to the \ref cardano_proposed_param_updates_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p proposed_param_updates or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposed_param_updates_t* proposed_param_updates = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_proposed_param_updates_to_cbor(proposed_param_updates, writer);
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
 * cardano_proposed_param_updates_unref(&proposed_param_updates);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_proposed_param_updates_to_cbor(
  const cardano_proposed_param_updates_t* proposed_param_updates,
  cardano_cbor_writer_t*                  writer);

/**
 * \brief Retrieves the number of proposed parameter updates.
 *
 * This function returns the number of proposed parameter updates in the
 * proposed protocol parameter updates object.
 *
 * \param[in] proposed_param_updates Pointer to the proposed protocol parameter updates object.
 *
 * \return The number of proposed parameter updates in the map.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposed_param_updates_t* proposed_param_updates = NULL;
 *
 * // Assume proposed_param_updates is initialized properly
 *
 * size_t size = cardano_proposed_param_updates_get_size(proposed_param_updates);
 * printf("Number of proposed parameter updates: %zu\n", size);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_proposed_param_updates_get_size(const cardano_proposed_param_updates_t* proposed_param_updates);

/**
 * \brief Inserts a new Protocol Parameter Update into the proposed protocol parameter updates.
 *
 * This function inserts a new protocol parameter update into the proposed protocol parameter updates map.
 *
 * \param[in] proposed_param_updates Pointer to the proposed protocol parameter updates object.
 * \param[in] genesis_delegate_key_hash Pointer to the key hash of the genesis delegate proposing the update.
 * \param[in] protocol_param_update Pointer to the protocol parameter update being proposed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the Protocol Parameter Update was successfully inserted, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposed_param_updates_t* proposed_param_updates = NULL;
 * cardano_blake2b_hash_t* genesis_delegate_key_hash = NULL;
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 *
 * // Assume proposed_param_updates, genesis_delegate_key_hash, and protocol_param_update are initialized properly
 *
 * cardano_error_t result = cardano_proposed_param_updates_insert(proposed_param_updates, genesis_delegate_key_hash, protocol_param_update);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The protocol parameter update was successfully inserted
 * }
 * else
 * {
 *   // Handle the error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_proposed_param_updates_insert(
  cardano_proposed_param_updates_t* proposed_param_updates,
  cardano_blake2b_hash_t*           genesis_delegate_key_hash,
  cardano_protocol_param_update_t*  protocol_param_update);

/**
 * \brief Retrieves a Protocol Parameter Update from the proposed protocol parameter updates.
 *
 * This function retrieves a protocol parameter update from the proposed protocol parameter updates map.
 *
 * \param[in] proposed_param_updates Pointer to the proposed protocol parameter updates object.
 * \param[in] genesis_delegate_key_hash Pointer to the key hash of the genesis delegate proposing the update.
 * \param[out] protocol_param_update On successful retrieval, this will point to the protocol parameter update
 *                                   associated with the given genesis delegate key hash. The caller is responsible
 *                                   for managing the lifecycle of this object. Specifically, once the protocol parameter update
 *                                   is no longer needed, the caller must release it by calling \ref cardano_protocol_param_update_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the Protocol Parameter Update was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposed_param_updates_t* proposed_param_updates = NULL;
 * cardano_blake2b_hash_t* genesis_delegate_key_hash = NULL;
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 *
 * // Assume proposed_param_updates and genesis_delegate_key_hash are initialized properly
 *
 * cardano_error_t result = cardano_proposed_param_updates_get(proposed_param_updates, genesis_delegate_key_hash, &protocol_param_update);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the protocol_param_update
 *
 *   // Once done, ensure to clean up and release the protocol_param_update
 *   cardano_protocol_param_update_unref(&protocol_param_update);
 * }
 * else
 * {
 *   // Handle the error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_proposed_param_updates_get(
  const cardano_proposed_param_updates_t* proposed_param_updates,
  cardano_blake2b_hash_t*                 genesis_delegate_key_hash,
  cardano_protocol_param_update_t**       protocol_param_update);

/**
 * \brief Retrieves the genesis delegate key hash at a specific index from the proposed protocol parameter updates.
 *
 * This function retrieves the genesis delegate key hash at the specified index from the proposed protocol parameter updates map.
 *
 * \param[in] proposed_param_updates Pointer to the proposed protocol parameter updates object.
 * \param[in] index The index of the genesis delegate key hash to retrieve.
 * \param[out] genesis_delegate_key_hash On successful retrieval, this will point to the genesis delegate key hash
 *                                       at the specified index. The caller is responsible for managing the lifecycle
 *                                       of this object. Specifically, once the genesis delegate key hash is no longer needed,
 *                                       the caller must release it by calling \ref cardano_blake2b_hash_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the genesis delegate key hash was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposed_param_updates_t* proposed_param_updates = NULL;
 * cardano_blake2b_hash_t* genesis_delegate_key_hash = NULL;
 * size_t index = 0; // Index of the key hash to retrieve
 *
 * // Assume proposed_param_updates is initialized properly
 *
 * cardano_error_t result = cardano_proposed_param_updates_get_key_at(proposed_param_updates, index, &genesis_delegate_key_hash);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the genesis_delegate_key_hash
 *
 *   // Once done, ensure to clean up and release the genesis_delegate_key_hash
 *   cardano_blake2b_hash_unref(&genesis_delegate_key_hash);
 * }
 * else
 * {
 *   // Handle the error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_proposed_param_updates_get_key_at(
  const cardano_proposed_param_updates_t* proposed_param_updates,
  size_t                                  index,
  cardano_blake2b_hash_t**                genesis_delegate_key_hash);

/**
 * \brief Retrieves the protocol parameter update at a specific index from the proposed protocol parameter updates.
 *
 * This function retrieves the protocol parameter update at the specified index from the proposed protocol parameter updates map.
 *
 * \param[in] proposed_param_updates Pointer to the proposed protocol parameter updates object.
 * \param[in] index The index of the protocol parameter update to retrieve.
 * \param[out] protocol_param_update On successful retrieval, this will point to the protocol parameter update
 *                                   at the specified index. The caller is responsible for managing the lifecycle
 *                                   of this object. Specifically, once the protocol parameter update is no longer needed,
 *                                   the caller must release it by calling \ref cardano_protocol_param_update_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the protocol parameter update was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposed_param_updates_t* proposed_param_updates = NULL;
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * size_t index = 0; // Index of the parameter update to retrieve
 *
 * // Assume proposed_param_updates is initialized properly
 *
 * cardano_error_t result = cardano_proposed_param_updates_get_value_at(proposed_param_updates, index, &protocol_param_update);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the protocol_param_update
 *
 *   // Once done, ensure to clean up and release the protocol_param_update
 *   cardano_protocol_param_update_unref(&protocol_param_update);
 * }
 * else
 * {
 *   // Handle the error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_proposed_param_updates_get_value_at(
  const cardano_proposed_param_updates_t* proposed_param_updates,
  size_t                                  index,
  cardano_protocol_param_update_t**       protocol_param_update);

/**
 * \brief Retrieves the genesis delegate key hash and protocol parameter update at the specified index.
 *
 * This function retrieves the genesis delegate key hash and protocol parameter update from the proposed parameter updates
 * at the specified index.
 *
 * \param[in]  proposed_param_updates    Pointer to the proposed parameter updates object.
 * \param[in]  index                     The index at which to retrieve the key-value pair.
 * \param[out] genesis_delegate_key_hash On successful retrieval, this will point to the genesis delegate key hash at the specified index.
 *                                       The caller is responsible for managing the lifecycle of this object and should release it using
 *                                       \ref cardano_blake2b_hash_unref when it is no longer needed.
 * \param[out] protocol_param_update     On successful retrieval, this will point to the protocol parameter update at the specified index.
 *                                       The caller is responsible for managing the lifecycle of this object and should release it using
 *                                       \ref cardano_protocol_param_update_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the key-value pair was successfully retrieved, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposed_param_updates_t* proposed_param_updates = NULL;
 * // Assume proposed_param_updates is initialized properly
 *
 * size_t index = 0;
 * cardano_blake2b_hash_t* genesis_delegate_key_hash = NULL;
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 *
 * cardano_error_t result = cardano_proposed_param_updates_get_key_value_at(proposed_param_updates, index, &genesis_delegate_key_hash, &protocol_param_update);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   if (genesis_delegate_key_hash != NULL && protocol_param_update != NULL)
 *   {
 *     // Use the genesis delegate key hash and protocol parameter update
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
 * cardano_proposed_param_updates_unref(&proposed_param_updates);
 * cardano_blake2b_hash_unref(&genesis_delegate_key_hash);
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_proposed_param_updates_get_key_value_at(
  const cardano_proposed_param_updates_t* proposed_param_updates,
  size_t                                  index,
  cardano_blake2b_hash_t**                genesis_delegate_key_hash,
  cardano_protocol_param_update_t**       protocol_param_update);

/**
 * \brief Decrements the reference count of a cardano_proposed_param_updates_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_proposed_param_updates_t object
 * by decreasing its reference count. When the reference count reaches zero, the proposed_param_updates is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] proposed_param_updates A pointer to the pointer of the proposed_param_updates object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposed_param_updates_t* proposed_param_updates = cardano_proposed_param_updates_new(major, minor);
 *
 * // Perform operations with the proposed_param_updates...
 *
 * cardano_proposed_param_updates_unref(&proposed_param_updates);
 * // At this point, proposed_param_updates is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_proposed_param_updates_unref, the pointer to the \ref cardano_proposed_param_updates_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_proposed_param_updates_unref(cardano_proposed_param_updates_t** proposed_param_updates);

/**
 * \brief Increases the reference count of the cardano_proposed_param_updates_t object.
 *
 * This function is used to manually increment the reference count of an cardano_proposed_param_updates_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_proposed_param_updates_unref.
 *
 * \param proposed_param_updates A pointer to the cardano_proposed_param_updates_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming proposed_param_updates is a previously created proposed_param_updates object
 *
 * cardano_proposed_param_updates_ref(proposed_param_updates);
 *
 * // Now proposed_param_updates can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_proposed_param_updates_ref there is a corresponding
 * call to \ref cardano_proposed_param_updates_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_proposed_param_updates_ref(cardano_proposed_param_updates_t* proposed_param_updates);

/**
 * \brief Retrieves the current reference count of the cardano_proposed_param_updates_t object.
 *
 * This function returns the number of active references to an cardano_proposed_param_updates_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_proposed_param_updates_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param proposed_param_updates A pointer to the cardano_proposed_param_updates_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_proposed_param_updates_t object. If the object
 * is properly managed (i.e., every \ref cardano_proposed_param_updates_ref call is matched with a
 * \ref cardano_proposed_param_updates_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming proposed_param_updates is a previously created proposed_param_updates object
 *
 * size_t ref_count = cardano_proposed_param_updates_refcount(proposed_param_updates);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_proposed_param_updates_refcount(const cardano_proposed_param_updates_t* proposed_param_updates);

/**
 * \brief Sets the last error message for a given cardano_proposed_param_updates_t object.
 *
 * Records an error message in the proposed_param_updates's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] proposed_param_updates A pointer to the \ref cardano_proposed_param_updates_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the proposed_param_updates's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_proposed_param_updates_set_last_error(
  cardano_proposed_param_updates_t* proposed_param_updates,
  const char*                       message);

/**
 * \brief Retrieves the last error message recorded for a specific proposed_param_updates.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_proposed_param_updates_set_last_error for the given
 * proposed_param_updates. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] proposed_param_updates A pointer to the \ref cardano_proposed_param_updates_t instance whose last error
 *                   message is to be retrieved. If the proposed_param_updates is NULL, the function
 *                   returns a generic error message indicating the null proposed_param_updates.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified proposed_param_updates. If the proposed_param_updates is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_proposed_param_updates_set_last_error for the same proposed_param_updates, or until
 *       the proposed_param_updates is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_proposed_param_updates_get_last_error(
  const cardano_proposed_param_updates_t* proposed_param_updates);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_PROPOSED_PARAM_UPDATES_H