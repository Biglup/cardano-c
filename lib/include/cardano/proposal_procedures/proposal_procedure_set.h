/**
 * \file proposal_procedure_set.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_PROPOSAL_PROCEDURE_SET_H
#define BIGLUP_LABS_INCLUDE_CARDANO_PROPOSAL_PROCEDURE_SET_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/proposal_procedures/proposal_procedure.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Governance proposal procedure for the Cardano blockchain, it supports various types of governance actions.
 */
typedef struct cardano_proposal_procedure_t cardano_proposal_procedure_t;

/**
 * \brief Represents a set of proposal procedures.
 */
typedef struct cardano_proposal_procedure_set_t cardano_proposal_procedure_set_t;

/**
 * \brief Creates and initializes a new instance of a proposal_procedure_set.
 *
 * This function allocates and initializes a new instance of \ref cardano_proposal_procedure_set_t.
 * It returns an error code to indicate success or failure of the operation.
 *
 * \param[out] proposal_procedure_set On successful initialization, this will point to a newly created
 *             \ref cardano_proposal_procedure_set_t object. This object represents a "strong reference"
 *             to the proposal_procedure_set, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the proposal_procedure_set is no longer needed, the caller must release it
 *             by calling \ref cardano_proposal_procedure_set_unref.
 *
 * \return \ref CARDANO_SUCCESS if the proposal_procedure_set was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_set_t* proposal_procedure_set = NULL;
 *
 * // Attempt to create a new proposal_procedure_set
 * cardano_error_t result = cardano_proposal_procedure_set_new(&proposal_procedure_set);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the proposal_procedure_set
 *
 *   // Once done, ensure to clean up and release the proposal_procedure_set
 *   cardano_proposal_procedure_set_unref(&proposal_procedure_set);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_set_new(cardano_proposal_procedure_set_t** proposal_procedure_set);

/**
 * \brief Creates a proposal_procedure set from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_proposal_procedure_set_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a proposal_procedure.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded proposal_procedure data.
 * \param[out] proposal_procedures A pointer to a pointer of \ref cardano_proposal_procedure_set_t that will be set to the address
 *                        of the newly created proposal_procedure object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the proposal_procedure was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_proposal_procedure_set_t object by calling
 *       \ref cardano_proposal_procedure_set_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_proposal_procedure_set_t* proposal_procedure = NULL;
 *
 * cardano_error_t result = cardano_proposal_procedure_set_from_cbor(reader, &proposal_procedure);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the proposal_procedure
 *
 *   // Once done, ensure to clean up and release the proposal_procedure
 *   cardano_proposal_procedure_set_unref(&proposal_procedure);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode proposal_procedure: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_set_from_cbor(cardano_cbor_reader_t* reader, cardano_proposal_procedure_set_t** proposal_procedures);

/**
 * \brief Serializes a proposal_procedure into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_proposal_procedure_set_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] proposal_procedures A constant pointer to the \ref cardano_proposal_procedure_set_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p proposal_procedure or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_set_t* proposal_procedure = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_proposal_procedure_set_to_cbor(proposal_procedure, writer);
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
 * cardano_proposal_procedure_set_unref(&proposal_procedure);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_proposal_procedure_set_to_cbor(
  const cardano_proposal_procedure_set_t* proposal_procedures,
  cardano_cbor_writer_t*                  writer);

/**
 * \brief Retrieves the length of a proposal_procedure list.
 *
 * This function retrieves the number of elements in the provided \ref cardano_proposal_procedure_set_t object.
 *
 * \param[in] proposal_procedure_set A constant pointer to the \ref cardano_proposal_procedure_set_t object whose length is to be retrieved.
 *
 * \return The number of elements in the proposal_procedure_set. Return 0 if the proposal_procedure_set is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_set_t* proposal_procedure_set = cardano_proposal_procedure_set_new();
 *
 * // Populate proposal_procedure_set with elements
 *
 * size_t length = cardano_proposal_procedure_set_get_length(proposal_procedure_set);
 * printf("Length of the proposal_procedure_set: %zu\n", length);
 *
 * // Clean up the proposal_procedure_set object once done
 * cardano_proposal_procedure_set_unref(&proposal_procedure_set);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_proposal_procedure_set_get_length(const cardano_proposal_procedure_set_t* proposal_procedure_set);

/**
 * \brief Retrieves an element from a proposal_procedure list by index.
 *
 * This function retrieves the element at the specified index from the provided \ref cardano_proposal_procedure_set_t object
 * and stores it in the output parameter.
 *
 * \param[in] proposal_procedure_set A constant pointer to the \ref cardano_proposal_procedure_set_t object from which
 *                        the element is to be retrieved.
 * \param[in] index The index of the element to retrieve from the proposal_procedure list. Indexing starts at 0.
 * \param[out] element Pointer to a variable where the retrieved element will be stored.
 *                     This variable will point to the retrieved \ref cardano_proposal_procedure_t object.
 *                     The caller is responsible for managing the lifecycle of the element by calling
 *                     \ref cardano_proposal_procedure_unref when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the element was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_set_t* proposal_procedure_set = cardano_proposal_procedure_set_new();
 *
 * // Populate proposal_procedure_set with elements
 *
 * cardano_proposal_procedure_t* element = NULL;
 * cardano_error_t result = cardano_proposal_procedure_set_get(proposal_procedure_set, 2, &element);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the retrieved element
 *   // Remember to unreference the element once done if it's no longer needed
 *   cardano_proposal_procedure_unref(&element);
 * }
 *
 * // Clean up the proposal_procedure_set object once done
 * cardano_proposal_procedure_set_unref(&proposal_procedure_set);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_proposal_procedure_set_get(const cardano_proposal_procedure_set_t* proposal_procedure_set, size_t index, cardano_proposal_procedure_t** element);

/**
 * \brief Adds an element to a proposal_procedure list.
 *
 * This function adds the specified element to the end of the provided \ref cardano_proposal_procedure_set_t object.
 *
 * \param[in] proposal_procedure_set A constant pointer to the \ref cardano_proposal_procedure_set_t object to which
 *                        the element is to be added.
 * \param[in] element Pointer to the \ref cardano_proposal_procedure_t object that is to be added to the proposal_procedure_set.
 *                    The element will be referenced by the proposal_procedure_set after addition.
 *
 * \return \ref CARDANO_SUCCESS if the element was successfully added to the proposal_procedure_set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_set_t* proposal_procedure_set = cardano_proposal_procedure_set_new();
 *
 * // Create and initialize a new proposal_procedure element
 * cardano_proposal_procedure_t* element = { ... };
 *
 * // Add the element to the proposal_procedure_set
 * cardano_error_t result = cardano_proposal_procedure_set_add(proposal_procedure_set, element);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Element added successfully
 * }
 *
 * // Clean up the proposal_procedure_set object once done
 * cardano_proposal_procedure_set_unref(&proposal_procedure_set);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_proposal_procedure_set_add(cardano_proposal_procedure_set_t* proposal_procedure_set, cardano_proposal_procedure_t* element);

/**
 * \brief Decrements the reference count of a proposal_procedure_set object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_proposal_procedure_set_t object
 * by decreasing its reference count. When the reference count reaches zero, the proposal_procedure_set is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] proposal_procedure_set A pointer to the pointer of the proposal_procedure_set object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_set_t* proposal_procedure_set = cardano_proposal_procedure_set_new();
 *
 * // Perform operations with the proposal_procedure_set...
 *
 * cardano_proposal_procedure_set_unref(&proposal_procedure_set);
 * // At this point, proposal_procedure_set is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_proposal_procedure_set_unref, the pointer to the \ref cardano_proposal_procedure_set_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_proposal_procedure_set_unref(cardano_proposal_procedure_set_t** proposal_procedure_set);

/**
 * \brief Increases the reference count of the cardano_proposal_procedure_set_t object.
 *
 * This function is used to manually increment the reference count of a proposal_procedure_set
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_proposal_procedure_set_unref.
 *
 * \param proposal_procedure_set A pointer to the proposal_procedure_set object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming proposal_procedure_set is a previously created proposal_procedure_set object
 *
 * cardano_proposal_procedure_set_ref(proposal_procedure_set);
 *
 * // Now proposal_procedure_set can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_proposal_procedure_set_ref there is a corresponding
 * call to \ref cardano_proposal_procedure_set_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_proposal_procedure_set_ref(cardano_proposal_procedure_set_t* proposal_procedure_set);

/**
 * \brief Retrieves the current reference count of the cardano_proposal_procedure_set_t object.
 *
 * This function returns the number of active references to a proposal_procedure_set object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_proposal_procedure_set_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param proposal_procedure_set A pointer to the proposal_procedure_set object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified proposal_procedure_set object. If the object
 * is properly managed (i.e., every \ref cardano_proposal_procedure_set_ref call is matched with a
 * \ref cardano_proposal_procedure_set_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming proposal_procedure_set is a previously created proposal_procedure_set object
 *
 * size_t ref_count = cardano_proposal_procedure_set_refcount(proposal_procedure_set);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_proposal_procedure_set_refcount(const cardano_proposal_procedure_set_t* proposal_procedure_set);

/**
 * \brief Sets the last error message for a given proposal_procedure_set object.
 *
 * Records an error message in the proposal_procedure_set's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] proposal_procedure_set A pointer to the \ref cardano_proposal_procedure_set_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the proposal_procedure_set's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_proposal_procedure_set_set_last_error(cardano_proposal_procedure_set_t* proposal_procedure_set, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific proposal_procedure_set.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_proposal_procedure_set_set_last_error for the given
 * proposal_procedure_set. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] proposal_procedure_set A pointer to the \ref cardano_proposal_procedure_set_t instance whose last error
 *                   message is to be retrieved. If the proposal_procedure_set is NULL, the function
 *                   returns a generic error message indicating the null proposal_procedure_set.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified proposal_procedure_set. If the proposal_procedure_set is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_proposal_procedure_set_set_last_error for the same proposal_procedure_set, or until
 *       the proposal_procedure_set is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_proposal_procedure_set_get_last_error(const cardano_proposal_procedure_set_t* proposal_procedure_set);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_PROPOSAL_PROCEDURE_SET_H