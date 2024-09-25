/**
 * \file relays.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_RELAYS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_RELAYS_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/pool_params/relay.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a set of pool relays.
 */
typedef struct cardano_relays_t cardano_relays_t;

/**
 * \brief Creates and initializes a new instance of a relays.
 *
 * This function allocates and initializes a new instance of \ref cardano_relays_t.
 * It returns an error code to indicate success or failure of the operation.
 *
 * \param[out] relays On successful initialization, this will point to a newly created
 *             \ref cardano_relays_t object. This object represents a "strong reference"
 *             to the relays, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the relays is no longer needed, the caller must release it
 *             by calling \ref cardano_relays_unref.
 *
 * \return \ref CARDANO_SUCCESS if the relays was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_relays_t* relays = NULL;
 *
 * // Attempt to create a new relays
 * cardano_error_t result = cardano_relays_new(&relays);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the relays
 *
 *   // Once done, ensure to clean up and release the relays
 *   cardano_relays_unref(&relays);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_relays_new(cardano_relays_t** relays);

/**
 * \brief Creates a relays from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_relays_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a relays.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded relays data.
 * \param[out] relays A pointer to a pointer of \ref cardano_relays_t that will be set to the address
 *                        of the newly created relays object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the relays was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_relays_t object by calling
 *       \ref cardano_relays_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_relays_t* relays = NULL;
 *
 * cardano_error_t result = cardano_relays_from_cbor(reader, &relays);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the relays
 *
 *   // Once done, ensure to clean up and release the relays
 *   cardano_relays_unref(&relays);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode relays: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_relays_from_cbor(cardano_cbor_reader_t* reader, cardano_relays_t** relays);

/**
 * \brief Serializes a relays into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_relays_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] relays A constant pointer to the \ref cardano_relays_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p relays or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_relays_t* relays = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_relays_to_cbor(relays, writer);
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
 * cardano_relays_unref(&relays);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_relays_to_cbor(
  const cardano_relays_t* relays,
  cardano_cbor_writer_t*  writer);

/**
 * \brief Retrieves the length of a relays set.
 *
 * This function retrieves the number of relays in the provided \ref cardano_relays_t object.
 *
 * \param[in] relays A constant pointer to the \ref cardano_relays_t object whose length is to be retrieved.
 *
 * \return The number of relays in the relays. Return 0 if the relays is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_relays_t* relays = cardano_relays_new();
 *
 * // Populate relays with relays
 *
 * size_t length = cardano_relays_get_length(relays);
 * printf("Length of the relays: %zu\n", length);
 *
 * // Clean up the relays object once done
 * cardano_relays_unref(&relays);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_relays_get_length(const cardano_relays_t* relays);

/**
 * \brief Retrieves an relay from an relay set by index.
 *
 * This function retrieves the relay at the specified index from the provided \ref cardano_relays_t object
 * and stores it in the output parameter.
 *
 * \param[in] relays A constant pointer to the \ref cardano_relays_t object from which
 *                        the relay is to be retrieved.
 * \param[in] index The index of the relay to retrieve from the relay set. Indexing starts at 0.
 * \param[out] relay Pointer to a variable where the retrieved relay will be stored.
 *                     This variable will point to the retrieved \ref cardano_relay_t object.
 *                     The caller is responsible for managing the lifecycle of the relay by calling
 *                     \ref cardano_relay_unref when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the relay was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_relays_t* relays = cardano_relays_new();
 *
 * // Populate relays with relays
 *
 * cardano_relay_t* relay = NULL;
 * cardano_error_t result = cardano_relays_get(relays, 2, &relay);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the retrieved relay
 *   // Remember to unreference the relay once done if it's no longer needed
 *   cardano_relay_unref(&relay);
 * }
 *
 * // Clean up the relays object once done
 * cardano_relays_unref(&relays);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_relays_get(const cardano_relays_t* relays, size_t index, cardano_relay_t** relay);

/**
 * \brief Adds an relay to a relay list.
 *
 * This function adds the specified relay to the provided \ref cardano_relays_t object.
 *
 * \param[in] relays A constant pointer to the \ref cardano_relays_t object to which
 *                        the relay is to be added.
 * \param[in] relay Pointer to the \ref cardano_relay_t object that is to be added to the relays.
 *                    The relay will be referenced by the relays after addition.
 *
 * \return \ref CARDANO_SUCCESS if the relay was successfully added to the relays, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_relays_t* relays = cardano_relays_new();
 *
 * // Create and initialize a new plutus_data relay
 * cardano_relay_t* relay = { ... };
 *
 * // Add the relay to the relays
 * cardano_error_t result = cardano_relays_add(relays, relay);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Element added successfully
 * }
 *
 * // Clean up the relays object once done
 * cardano_relays_unref(&relays);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_relays_add(cardano_relays_t* relays, cardano_relay_t* relay);

/**
 * \brief Decrements the reference count of a relays object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_relays_t object
 * by decreasing its reference count. When the reference count reaches zero, the relays is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] relays A pointer to the pointer of the relays object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_relays_t* relays = cardano_relays_new();
 *
 * // Perform operations with the relays...
 *
 * cardano_relays_unref(&relays);
 * // At this point, relays is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_relays_unref, the pointer to the \ref cardano_relays_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_relays_unref(cardano_relays_t** relays);

/**
 * \brief Increases the reference count of the cardano_relays_t object.
 *
 * This function is used to manually increment the reference count of a relays
 * object, indicating that another part of the code has taken relayship of it. This
 * ensures the object remains allocated and valid until all relays have released their
 * reference by calling \ref cardano_relays_unref.
 *
 * \param relays A pointer to the relays object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming relays is a previously created relays object
 *
 * cardano_relays_ref(relays);
 *
 * // Now relays can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_relays_ref there is a corresponding
 * call to \ref cardano_relays_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_relays_ref(cardano_relays_t* relays);

/**
 * \brief Retrieves the current reference count of the cardano_relays_t object.
 *
 * This function returns the number of active references to a relays object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_relays_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param relays A pointer to the relays object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified relays object. If the object
 * is properly managed (i.e., every \ref cardano_relays_ref call is matched with a
 * \ref cardano_relays_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming relays is a previously created relays object
 *
 * size_t ref_count = cardano_relays_refcount(relays);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_relays_refcount(const cardano_relays_t* relays);

/**
 * \brief Sets the last error message for a given relays object.
 *
 * Records an error message in the relays's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] relays A pointer to the \ref cardano_relays_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the relays's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_relays_set_last_error(cardano_relays_t* relays, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific relays.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_relays_set_last_error for the given
 * relays. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] relays A pointer to the \ref cardano_relays_t instance whose last error
 *                   message is to be retrieved. If the relays is NULL, the function
 *                   returns a generic error message indicating the null relays.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified relays. If the relays is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_relays_set_last_error for the same relays, or until
 *       the relays is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_relays_get_last_error(const cardano_relays_t* relays);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_RELAYS_H