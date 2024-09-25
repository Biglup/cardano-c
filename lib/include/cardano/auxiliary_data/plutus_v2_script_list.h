/**
 * \file plutus_v2_script_list.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_PLUTUS_V2_SCRIPT_LIST_H
#define BIGLUP_LABS_INCLUDE_CARDANO_PLUTUS_V2_SCRIPT_LIST_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Plutus' scripts are pieces of code that implement pure functions with True or False outputs. These functions take
 * several inputs such as Datum, Redeemer and the transaction context to decide whether an output can be spent or not.
 *
 * V2 was introduced in the Vasil hard fork.
 *
 * The main changes in V2 of Plutus were to the interface to scripts. The ScriptContext was extended
 * to include the following information:
 *
 *  - The full “redeemers” structure, which contains all the redeemers used in the transaction
 *  - Reference inputs in the transaction (proposed in CIP-31)
 *  - Inline datums in the transaction (proposed in CIP-32)
 *  - Reference scripts in the transaction (proposed in CIP-33)
 */
typedef struct cardano_plutus_v2_script_t cardano_plutus_v2_script_t;

/**
 * \brief Represents a list of plutus v2 scripts.
 */
typedef struct cardano_plutus_v2_script_list_t cardano_plutus_v2_script_list_t;

/**
 * \brief Creates and initializes a new instance of a plutus_v2_script_list.
 *
 * This function allocates and initializes a new instance of \ref cardano_plutus_v2_script_list_t.
 * It returns an error code to indicate success or failure of the operation.
 *
 * \param[out] plutus_v2_script_list On successful initialization, this will point to a newly created
 *             \ref cardano_plutus_v2_script_list_t object. This object represents a "strong reference"
 *             to the plutus_v2_script_list, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the plutus_v2_script_list is no longer needed, the caller must release it
 *             by calling \ref cardano_plutus_v2_script_list_unref.
 *
 * \return \ref CARDANO_SUCCESS if the plutus_v2_script_list was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_v2_script_list_t* plutus_v2_script_list = NULL;
 *
 * // Attempt to create a new plutus_v2_script_list
 * cardano_error_t result = cardano_plutus_v2_script_list_new(&plutus_v2_script_list);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the plutus_v2_script_list
 *
 *   // Once done, ensure to clean up and release the plutus_v2_script_list
 *   cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_plutus_v2_script_list_new(cardano_plutus_v2_script_list_t** plutus_v2_script_list);

/**
 * \brief Creates a plutus v2 script list from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_plutus_v2_script_list_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a plutus_v2_script.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded plutus_v2_script data.
 * \param[out] plutus_v2_scripts A pointer to a pointer of \ref cardano_plutus_v2_script_list_t that will be set to the address
 *                        of the newly created plutus_v2_script object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the plutus_v2_script was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_plutus_v2_script_list_t object by calling
 *       \ref cardano_plutus_v2_script_list_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_plutus_v2_script_list_t* plutus_v2_script = NULL;
 *
 * cardano_error_t result = cardano_plutus_v2_script_list_from_cbor(reader, &plutus_v2_script);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the plutus_v2_script
 *
 *   // Once done, ensure to clean up and release the plutus_v2_script
 *   cardano_plutus_v2_script_list_unref(&plutus_v2_script);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode plutus_v2_script: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_plutus_v2_script_list_from_cbor(cardano_cbor_reader_t* reader, cardano_plutus_v2_script_list_t** plutus_v2_scripts);

/**
 * \brief Serializes a plutus_v2_script into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_plutus_v2_script_list_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] plutus_v2_scripts A constant pointer to the \ref cardano_plutus_v2_script_list_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p plutus_v2_script or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_v2_script_list_t* plutus_v2_script = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_plutus_v2_script_list_to_cbor(plutus_v2_script, writer);
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
 * cardano_plutus_v2_script_list_unref(&plutus_v2_script);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_plutus_v2_script_list_to_cbor(
  const cardano_plutus_v2_script_list_t* plutus_v2_scripts,
  cardano_cbor_writer_t*                 writer);

/**
 * \brief Retrieves the length of a plutus v2 script list.
 *
 * This function retrieves the number of elements in the provided \ref cardano_plutus_v2_script_list_t object.
 *
 * \param[in] plutus_v2_script_list A constant pointer to the \ref cardano_plutus_v2_script_list_t object whose length is to be retrieved.
 *
 * \return The number of elements in the plutus_v2_script_list. Return 0 if the plutus_v2_script_list is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_v2_script_list_t* plutus_v2_script_list = cardano_plutus_v2_script_list_new();
 *
 * // Populate plutus_v2_script_list with elements
 *
 * size_t length = cardano_plutus_v2_script_list_get_length(plutus_v2_script_list);
 * printf("Length of the plutus_v2_script_list: %zu\n", length);
 *
 * // Clean up the plutus_v2_script_list object once done
 * cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_plutus_v2_script_list_get_length(const cardano_plutus_v2_script_list_t* plutus_v2_script_list);

/**
 * \brief Retrieves an element from a plutus v2 script list by index.
 *
 * This function retrieves the element at the specified index from the provided \ref cardano_plutus_v2_script_list_t object
 * and stores it in the output parameter.
 *
 * \param[in] plutus_v2_script_list A constant pointer to the \ref cardano_plutus_v2_script_list_t object from which
 *                        the element is to be retrieved.
 * \param[in] index The index of the element to retrieve from the plutus v2 script list. Indexing starts at 0.
 * \param[out] element Pointer to a variable where the retrieved element will be stored.
 *                     This variable will point to the retrieved \ref cardano_plutus_v2_script_t object.
 *                     The caller is responsible for managing the lifecycle of the element by calling
 *                     \ref cardano_plutus_v2_script_unref when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the element was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_v2_script_list_t* plutus_v2_script_list = cardano_plutus_v2_script_list_new();
 *
 * // Populate plutus_v2_script_list with elements
 *
 * cardano_plutus_v2_script_t* element = NULL;
 * cardano_error_t result = cardano_plutus_v2_script_list_get(plutus_v2_script_list, 2, &element);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the retrieved element
 *   // Remember to unreference the element once done if it's no longer needed
 *   cardano_plutus_v2_script_unref(&element);
 * }
 *
 * // Clean up the plutus_v2_script_list object once done
 * cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_plutus_v2_script_list_get(const cardano_plutus_v2_script_list_t* plutus_v2_script_list, size_t index, cardano_plutus_v2_script_t** element);

/**
 * \brief Adds an element to a plutus v2 script list.
 *
 * This function adds the specified element to the end of the provided \ref cardano_plutus_v2_script_list_t object.
 *
 * \param[in] plutus_v2_script_list A constant pointer to the \ref cardano_plutus_v2_script_list_t object to which
 *                        the element is to be added.
 * \param[in] element Pointer to the \ref cardano_plutus_v2_script_t object that is to be added to the plutus_v2_script_list.
 *                    The element will be referenced by the plutus_v2_script_list after addition.
 *
 * \return \ref CARDANO_SUCCESS if the element was successfully added to the plutus_v2_script_list, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_v2_script_list_t* plutus_v2_script_list = cardano_plutus_v2_script_list_new();
 *
 * // Create and initialize a new plutus v2 script element
 * cardano_plutus_v2_script_t* element = { ... };
 *
 * // Add the element to the plutus_v2_script_list
 * cardano_error_t result = cardano_plutus_v2_script_list_add(plutus_v2_script_list, element);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Element added successfully
 * }
 *
 * // Clean up the plutus_v2_script_list object once done
 * cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_plutus_v2_script_list_add(cardano_plutus_v2_script_list_t* plutus_v2_script_list, cardano_plutus_v2_script_t* element);

/**
 * \brief Decrements the reference count of a plutus_v2_script_list object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_plutus_v2_script_list_t object
 * by decreasing its reference count. When the reference count reaches zero, the plutus_v2_script_list is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] plutus_v2_script_list A pointer to the pointer of the plutus_v2_script_list object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_v2_script_list_t* plutus_v2_script_list = cardano_plutus_v2_script_list_new();
 *
 * // Perform operations with the plutus_v2_script_list...
 *
 * cardano_plutus_v2_script_list_unref(&plutus_v2_script_list);
 * // At this point, plutus_v2_script_list is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_plutus_v2_script_list_unref, the pointer to the \ref cardano_plutus_v2_script_list_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_plutus_v2_script_list_unref(cardano_plutus_v2_script_list_t** plutus_v2_script_list);

/**
 * \brief Increases the reference count of the cardano_plutus_v2_script_list_t object.
 *
 * This function is used to manually increment the reference count of a plutus_v2_script_list
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_plutus_v2_script_list_unref.
 *
 * \param plutus_v2_script_list A pointer to the plutus_v2_script_list object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming plutus_v2_script_list is a previously created plutus_v2_script_list object
 *
 * cardano_plutus_v2_script_list_ref(plutus_v2_script_list);
 *
 * // Now plutus_v2_script_list can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_plutus_v2_script_list_ref there is a corresponding
 * call to \ref cardano_plutus_v2_script_list_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_plutus_v2_script_list_ref(cardano_plutus_v2_script_list_t* plutus_v2_script_list);

/**
 * \brief Retrieves the current reference count of the cardano_plutus_v2_script_list_t object.
 *
 * This function returns the number of active references to a plutus_v2_script_list object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_plutus_v2_script_list_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param plutus_v2_script_list A pointer to the plutus_v2_script_list object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified plutus_v2_script_list object. If the object
 * is properly managed (i.e., every \ref cardano_plutus_v2_script_list_ref call is matched with a
 * \ref cardano_plutus_v2_script_list_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming plutus_v2_script_list is a previously created plutus_v2_script_list object
 *
 * size_t ref_count = cardano_plutus_v2_script_list_refcount(plutus_v2_script_list);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_plutus_v2_script_list_refcount(const cardano_plutus_v2_script_list_t* plutus_v2_script_list);

/**
 * \brief Sets the last error message for a given plutus_v2_script_list object.
 *
 * Records an error message in the plutus_v2_script_list's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] plutus_v2_script_list A pointer to the \ref cardano_plutus_v2_script_list_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the plutus_v2_script_list's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_plutus_v2_script_list_set_last_error(cardano_plutus_v2_script_list_t* plutus_v2_script_list, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific plutus_v2_script_list.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_plutus_v2_script_list_set_last_error for the given
 * plutus_v2_script_list. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] plutus_v2_script_list A pointer to the \ref cardano_plutus_v2_script_list_t instance whose last error
 *                   message is to be retrieved. If the plutus_v2_script_list is NULL, the function
 *                   returns a generic error message indicating the null plutus_v2_script_list.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified plutus_v2_script_list. If the plutus_v2_script_list is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_plutus_v2_script_list_set_last_error for the same plutus_v2_script_list, or until
 *       the plutus_v2_script_list is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_plutus_v2_script_list_get_last_error(const cardano_plutus_v2_script_list_t* plutus_v2_script_list);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_PLUTUS_V2_SCRIPT_LIST_H