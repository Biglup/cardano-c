/**
 * \file pool_owners.h
 *
 * \author angel.castillo
 * \date   Jun 26, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_POOL_OWNERS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_POOL_OWNERS_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a set of pool owners.
 */
typedef struct cardano_pool_owners_t cardano_pool_owners_t;

/**
 * \brief Creates and initializes a new instance of a pool_owners.
 *
 * This function allocates and initializes a new instance of \ref cardano_pool_owners_t.
 * It returns an error code to indicate success or failure of the operation.
 *
 * \param[out] pool_owners On successful initialization, this will point to a newly created
 *             \ref cardano_pool_owners_t object. This object represents a "strong reference"
 *             to the pool_owners, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the pool_owners is no longer needed, the caller must release it
 *             by calling \ref cardano_pool_owners_unref.
 *
 * \return \ref CARDANO_SUCCESS if the pool_owners was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_owners_t* pool_owners = NULL;
 *
 * // Attempt to create a new pool_owners
 * cardano_error_t result = cardano_pool_owners_new(&pool_owners);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the pool_owners
 *
 *   // Once done, ensure to clean up and release the pool_owners
 *   cardano_pool_owners_unref(&pool_owners);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_pool_owners_new(cardano_pool_owners_t** pool_owners);

/**
 * \brief Creates a pool_owners from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_pool_owners_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a pool_owners.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded pool_owners data.
 * \param[out] pool_owners A pointer to a pointer of \ref cardano_pool_owners_t that will be set to the address
 *                        of the newly created pool_owners object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the pool_owners was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_pool_owners_t object by calling
 *       \ref cardano_pool_owners_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_pool_owners_t* pool_owners = NULL;
 *
 * cardano_error_t result = cardano_pool_owners_from_cbor(reader, &pool_owners);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the pool_owners
 *
 *   // Once done, ensure to clean up and release the pool_owners
 *   cardano_pool_owners_unref(&pool_owners);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode pool_owners: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_pool_owners_from_cbor(cardano_cbor_reader_t* reader, cardano_pool_owners_t** pool_owners);

/**
 * \brief Serializes a pool_owners into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_pool_owners_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] pool_owners A constant pointer to the \ref cardano_pool_owners_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p pool_owners or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_owners_t* pool_owners = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_pool_owners_to_cbor(pool_owners, writer);
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
 * cardano_pool_owners_unref(&pool_owners);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_owners_to_cbor(
  const cardano_pool_owners_t* pool_owners,
  cardano_cbor_writer_t*       writer);

/**
 * \brief Retrieves the length of a owners set.
 *
 * This function retrieves the number of owners in the provided \ref cardano_pool_owners_t object.
 *
 * \param[in] pool_owners A constant pointer to the \ref cardano_pool_owners_t object whose length is to be retrieved.
 *
 * \return The number of owners in the pool_owners. Return 0 if the pool_owners is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_owners_t* pool_owners = cardano_pool_owners_new();
 *
 * // Populate pool_owners with owners
 *
 * size_t length = cardano_pool_owners_get_length(pool_owners);
 * printf("Length of the pool_owners: %zu\n", length);
 *
 * // Clean up the pool_owners object once done
 * cardano_pool_owners_unref(&pool_owners);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_pool_owners_get_length(const cardano_pool_owners_t* pool_owners);

/**
 * \brief Retrieves an owner from an owner set by index.
 *
 * This function retrieves the owner at the specified index from the provided \ref cardano_pool_owners_t object
 * and stores it in the output parameter.
 *
 * \param[in] pool_owners A constant pointer to the \ref cardano_pool_owners_t object from which
 *                        the owner is to be retrieved.
 * \param[in] index The index of the owner to retrieve from the owner set. Indexing starts at 0.
 * \param[out] owner Pointer to a variable where the retrieved owner will be stored.
 *                     This variable will point to the retrieved \ref cardano_blake2b_hash_t object.
 *                     The caller is responsible for managing the lifecycle of the owner by calling
 *                     \ref cardano_blake2b_hash_unref when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the owner was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_owners_t* pool_owners = cardano_pool_owners_new();
 *
 * // Populate pool_owners with owners
 *
 * cardano_blake2b_hash_t* owner = NULL;
 * cardano_error_t result = cardano_pool_owners_get(pool_owners, 2, &owner);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the retrieved owner
 *   // Remember to unreference the owner once done if it's no longer needed
 *   cardano_blake2b_hash_unref(&owner);
 * }
 *
 * // Clean up the pool_owners object once done
 * cardano_pool_owners_unref(&pool_owners);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_owners_get(const cardano_pool_owners_t* pool_owners, size_t index, cardano_blake2b_hash_t** owner);

/**
 * \brief Adds an owner to a owner set.
 *
 * This function adds the specified owner to the provided \ref cardano_pool_owners_t object.
 *
 * \param[in] pool_owners A constant pointer to the \ref cardano_pool_owners_t object to which
 *                        the owner is to be added.
 * \param[in] owner Pointer to the \ref cardano_blake2b_hash_t object that is to be added to the pool_owners.
 *                    The owner will be referenced by the pool_owners after addition.
 *
 * \return \ref CARDANO_SUCCESS if the owner was successfully added to the pool_owners, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_owners_t* pool_owners = cardano_pool_owners_new();
 *
 * // Create and initialize a new plutus_data owner
 * cardano_blake2b_hash_t* owner = { ... };
 *
 * // Add the owner to the pool_owners
 * cardano_error_t result = cardano_pool_owners_add(pool_owners, owner);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Element added successfully
 * }
 *
 * // Clean up the pool_owners object once done
 * cardano_pool_owners_unref(&pool_owners);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_owners_add(cardano_pool_owners_t* pool_owners, cardano_blake2b_hash_t* owner);

/**
 * \brief Decrements the reference count of a pool_owners object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_pool_owners_t object
 * by decreasing its reference count. When the reference count reaches zero, the pool_owners is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] pool_owners A pointer to the pointer of the pool_owners object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_owners_t* pool_owners = cardano_pool_owners_new();
 *
 * // Perform operations with the pool_owners...
 *
 * cardano_pool_owners_unref(&pool_owners);
 * // At this point, pool_owners is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_pool_owners_unref, the pointer to the \ref cardano_pool_owners_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_pool_owners_unref(cardano_pool_owners_t** pool_owners);

/**
 * \brief Increases the reference count of the cardano_pool_owners_t object.
 *
 * This function is used to manually increment the reference count of a pool_owners
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_pool_owners_unref.
 *
 * \param pool_owners A pointer to the pool_owners object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming pool_owners is a previously created pool_owners object
 *
 * cardano_pool_owners_ref(pool_owners);
 *
 * // Now pool_owners can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_pool_owners_ref there is a corresponding
 * call to \ref cardano_pool_owners_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_pool_owners_ref(cardano_pool_owners_t* pool_owners);

/**
 * \brief Retrieves the current reference count of the cardano_pool_owners_t object.
 *
 * This function returns the number of active references to a pool_owners object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_pool_owners_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param pool_owners A pointer to the pool_owners object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified pool_owners object. If the object
 * is properly managed (i.e., every \ref cardano_pool_owners_ref call is matched with a
 * \ref cardano_pool_owners_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming pool_owners is a previously created pool_owners object
 *
 * size_t ref_count = cardano_pool_owners_refcount(pool_owners);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_pool_owners_refcount(const cardano_pool_owners_t* pool_owners);

/**
 * \brief Sets the last error message for a given pool_owners object.
 *
 * Records an error message in the pool_owners's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] pool_owners A pointer to the \ref cardano_pool_owners_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the pool_owners's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_pool_owners_set_last_error(cardano_pool_owners_t* pool_owners, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific pool_owners.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_pool_owners_set_last_error for the given
 * pool_owners. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] pool_owners A pointer to the \ref cardano_pool_owners_t instance whose last error
 *                   message is to be retrieved. If the pool_owners is NULL, the function
 *                   returns a generic error message indicating the null pool_owners.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified pool_owners. If the pool_owners is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_pool_owners_set_last_error for the same pool_owners, or until
 *       the pool_owners is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_pool_owners_get_last_error(const cardano_pool_owners_t* pool_owners);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_POOL_OWNERS_H