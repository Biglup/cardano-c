/**
 * \file script_any.h
 *
 * \author angel.castillo
 * \date   May 19, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_ANY_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_ANY_H

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
 * \brief The Native scripts form an expression tree, the evaluation of the script produces either true or false.
 *
 * Note that it is recursive. There are no constraints on the nesting or size, except that imposed by the overall
 * transaction size limit (given that the script must be included in the transaction in a script witnesses).
 */
typedef struct cardano_native_script_t cardano_native_script_t;

/**
 * \brief This script evaluates to true if any the sub-scripts evaluate to true.
 *
 * If the list of sub-scripts is empty, this script evaluates to true.
 */
typedef struct cardano_script_any_t cardano_script_any_t;

/**
 * \brief The Native scripts form an expression tree, the evaluation of the script produces either true or false.
 *
 * Note that it is recursive. There are no constraints on the nesting or size, except that imposed by the overall
 * transaction size limit (given that the script must be included in the transaction in a script witnesses).
 */
typedef struct cardano_native_script_list_t cardano_native_script_list_t;

/**
 * \brief Creates and initializes a new instance of a script_any.
 *
 * This function allocates and initializes a new instance of \ref cardano_script_any_t
 * with a provided list of native scripts. It returns an error code to indicate success or failure of the operation.
 *
 * \param[in] native_scripts A pointer to a \ref cardano_native_script_list_t containing the list of native scripts.
 * \param[out] script_any On successful initialization, this will point to a newly created
 *             \ref cardano_script_any_t object. This object represents a "strong reference"
 *             to the script_any, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the script_any is no longer needed, the caller must release it
 *             by calling \ref cardano_script_any_unref.
 *
 * \return \ref CARDANO_SUCCESS if the script_any was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_native_script_list_t* native_scripts = ...; // Assume this is initialized
 * cardano_script_any_t* script_any = NULL;
 *
 * // Attempt to create a new script_any
 * cardano_error_t result = cardano_script_any_new(native_scripts, &script_any);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script_any
 *
 *   // Once done, ensure to clean up and release the script_any
 *   cardano_script_any_unref(&script_any);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_script_any_new(cardano_native_script_list_t* native_scripts, cardano_script_any_t** script_any);

/**
 * \brief Creates a script_any from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_script_any_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a script_any.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded script_any data.
 * \param[out] script_any A pointer to a pointer of \ref cardano_script_any_t that will be set to the address
 *                        of the newly created script_any object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the script_any was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_script_any_t object by calling
 *       \ref cardano_script_any_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_script_any_t* script_any = NULL;
 *
 * cardano_error_t result = cardano_script_any_from_cbor(reader, &script_any);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script_any
 *
 *   // Once done, ensure to clean up and release the script_any
 *   cardano_script_any_unref(&script_any);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode script_any: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_script_any_from_cbor(cardano_cbor_reader_t* reader, cardano_script_any_t** script_any);

/**
 * \brief Serializes a script_any into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_script_any_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] script_any A constant pointer to the \ref cardano_script_any_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p script_any or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_any_t* script_any = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_script_any_to_cbor(script_any, writer);
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
 * cardano_script_any_unref(&script_any);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_script_any_to_cbor(
  const cardano_script_any_t* script_any,
  cardano_cbor_writer_t*      writer);

/**
 * \brief Creates a script_any from a JSON string.
 *
 * This function parses JSON data using a provided JSON string and constructs a \ref cardano_script_any_t object.
 * It assumes that the JSON data corresponds to the structure expected for a script_any.
 *
 * \param[in] json A pointer to a null-terminated string containing the JSON-encoded script_any data. This string
 *                 must not be NULL.
 * \param[in] json_size The size of the JSON string, excluding the null terminator.
 * \param[out] native_script A pointer to a pointer of \ref cardano_script_any_t that will be set to the address
 *                        of the newly created script_any object upon successful decoding. The caller is responsible
 *                        for managing the lifecycle of this object by calling \ref cardano_script_any_unref when it
 *                        is no longer needed.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the script_any was successfully created, or an appropriate error code if an error occurred.
 *
 * Usage Example:
 * \code{.c}
 * const char* json_string = "{\"type\": \"any\", \"scripts\": [...]}"; // Example JSON string
 * size_t json_size = strlen(json_string); // Calculate the size of the JSON string
 * cardano_script_any_t* script_any = NULL;
 *
 * // Attempt to create a new script_any from a JSON string
 * cardano_error_t result = cardano_script_any_from_json(json_string, json_size, &script_any);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script_any
 *
 *   // Once done, ensure to clean up and release the script_any
 *   cardano_script_any_unref(&script_any);
 * }
 * else
 * {
 *   printf("Failed to create script_any from JSON. Error code: %d\n", result);
 * }
 * \endcode
 */
cardano_error_t
cardano_script_any_from_json(const char* json, size_t json_size, cardano_script_any_t** native_script);

/**
 * \brief Retrieves the length of the script list in a script_any object.
 *
 * This function retrieves the number of elements in the provided \ref cardano_script_any_t script list.
 *
 * \param[in] script_any A constant pointer to the \ref cardano_script_any_t object whose scripts list length is to be retrieved.
 *
 * \return The number of scripts in the script_any. Returns 0 if the script_any is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_native_script_list_t* native_scripts = ...; // Assume this is initialized and populated
 * cardano_script_any_t* script_any = NULL;
 * cardano_script_any_new(native_scripts, &script_any);
 *
 * size_t length = cardano_script_any_get_length(script_any);
 * printf("Length of the script_any scripts: %zu\n", length);
 *
 * // Clean up the script_any object once done
 * cardano_script_any_unref(&script_any);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_script_any_get_length(const cardano_script_any_t* script_any);

/**
 * \brief Retrieves the list of native scripts associated with a \ref cardano_script_any_t.
 *
 * This function provides access to the list of native scripts in a \ref cardano_script_any_t object.
 * It returns a reference to a \ref cardano_native_script_list_t object representing the scripts.
 * This allows the scripts to be used independently of the original \ref cardano_script_any_t object. The
 * reference count of the list object is increased by one, making it the caller's responsibility
 * to release it by calling \ref cardano_native_script_list_unref when it is no longer needed.
 *
 * \param[in] script_any A constant pointer to the \ref cardano_script_any_t object from which
 *                       the list of native scripts is to be retrieved.
 * \param[out] list A pointer to store the reference to the \ref cardano_native_script_list_t object containing the scripts.
 *                  If the input script_any is NULL, returns NULL. The caller is responsible for
 *                  managing the lifecycle of this object, including releasing it with
 *                  \ref cardano_native_script_list_unref.
 *
 * \return \ref CARDANO_SUCCESS if the list of scripts was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_any_t* script_any = cardano_script_any_new(...);
 * cardano_native_script_list_t* list_scripts = NULL;
 *
 * // Attempt to retrieve the list of native scripts associated with script_any
 * cardano_error_t result = cardano_script_any_get_scripts(script_any, &list_scripts);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the list_scripts
 *
 *   // Once done, ensure to clean up and release the list_scripts
 *   cardano_native_script_list_unref(&list_scripts);
 * }
 *
 * // Release the script_any object after use
 * cardano_script_any_unref(&script_any);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_script_any_get_scripts(
  cardano_script_any_t*          script_any,
  cardano_native_script_list_t** list);

/**
 * \brief Sets the list of native scripts associated with a \ref cardano_script_any_t.
 *
 * This function sets the list of native scripts for a \ref cardano_script_any_t object to the provided
 * \ref cardano_native_script_list_t object. It replaces any existing scripts associated with the \ref cardano_script_any_t.
 * The reference count of the provided list object is increased by one, making it the caller's responsibility
 * to release it when it is no longer needed.
 *
 * \param[in] script_any A pointer to the \ref cardano_script_any_t object to which
 *                       the list of native scripts is to be set.
 * \param[in] list A pointer to the \ref cardano_native_script_list_t object containing the new scripts to be set.
 *
 * \return \ref CARDANO_SUCCESS if the scripts were successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_any_t* script_any = cardano_script_any_new(...);
 * cardano_native_script_list_t* new_list = ...; // Assume new_list is initialized here
 *
 * // Attempt to set the new list of scripts associated with the script_any
 * cardano_error_t result = cardano_script_any_set_scripts(script_any, new_list);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Scripts successfully set
 *   printf("Scripts set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   fprintf(stderr, "Failed to set scripts: %s.\n", cardano_error_to_string(result));
 * }
 *
 * // Release the script_any object after use
 * cardano_script_any_unref(&script_any);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_script_any_set_scripts(
  cardano_script_any_t*         script_any,
  cardano_native_script_list_t* list);

/**
 * \brief Checks if two script_any objects are equal.
 *
 * This function compares two \ref cardano_script_any_t objects to determine if they are equal. Two script_any
 * objects are considered equal if they have the same number of elements and each corresponding pair of elements
 * is equal according to the \ref cardano_native_script_equals function.
 *
 * \param[in] lhs A constant pointer to the first \ref cardano_script_any_t object to be compared.
 * \param[in] rhs A constant pointer to the second \ref cardano_script_any_t object to be compared.
 *
 * \return \c true if the two script_any objects are equal, \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_any_t* script1 = ...;  // Assume script1 is initialized here
 * cardano_script_any_t* script2 = ...;  // Assume script2 is initialized here
 *
 * if (cardano_script_any_equals(script1, script2))
 * {
 *   // The two script_any objects are equal
 * }
 * else
 * {
 *   // The two script_any objects are not equal
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_script_any_equals(const cardano_script_any_t* lhs, const cardano_script_any_t* rhs);

/**
 * \brief Decrements the reference count of a script_any object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_script_any_t object
 * by decreasing its reference count. When the reference count reaches zero, the script_any is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] script_any A pointer to the pointer of the script_any object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_any_t* script_any = ...; // Assume script_any is initialized here
 *
 * // Perform operations with the script_any...
 *
 * cardano_script_any_unref(&script_any);
 * // At this point, script_any is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_script_any_unref, the pointer to the \ref cardano_script_any_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_script_any_unref(cardano_script_any_t** script_any);

/**
 * \brief Increases the reference count of the cardano_script_any_t object.
 *
 * This function is used to manually increment the reference count of a script_any
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until any owners have released their
 * reference by calling \ref cardano_script_any_unref.
 *
 * \param script_any A pointer to the script_any object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming script_any is a previously created script_any object
 *
 * cardano_script_any_ref(script_any);
 *
 * // Now script_any can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_script_any_ref there is a corresponding
 * call to \ref cardano_script_any_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_script_any_ref(cardano_script_any_t* script_any);

/**
 * \brief Retrieves the current reference count of the cardano_script_any_t object.
 *
 * This function returns the number of active references to a script_any object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_script_any_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param script_any A pointer to the script_any object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified script_any object. If the object
 * is properly managed (i.e., every \ref cardano_script_any_ref call is matched with a
 * \ref cardano_script_any_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming script_any is a previously created script_any object
 *
 * size_t ref_count = cardano_script_any_refcount(script_any);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_script_any_refcount(const cardano_script_any_t* script_any);

/**
 * \brief Sets the last error message for a given script_any object.
 *
 * Records an error message in the script_any's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] script_any A pointer to the \ref cardano_script_any_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the script_any's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_script_any_set_last_error(cardano_script_any_t* script_any, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific script_any.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_script_any_set_last_error for the given
 * script_any. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] script_any A pointer to the \ref cardano_script_any_t instance whose last error
 *                   message is to be retrieved. If the script_any is NULL, the function
 *                   returns a generic error message indicating the null script_any.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified script_any. If the script_any is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_script_any_set_last_error for the same script_any, or until
 *       the script_any is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_script_any_get_last_error(const cardano_script_any_t* script_any);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_ANY_H