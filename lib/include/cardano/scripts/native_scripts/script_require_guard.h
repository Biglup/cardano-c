/**
 * \file script_require_guard.h
 *
 * \author angel.castillo
 * \date   Jul 16, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_REQUIRE_GUARD_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_REQUIRE_GUARD_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/credential.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/json/json_writer.h>
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
 * \brief This script evaluates to true if the given credential is present in the
 * transaction guards.
 *
 * In other words, this checks that the transaction was authorized by a particular guard,
 * identified by its key hash or script hash credential.
 */
typedef struct cardano_script_require_guard_t cardano_script_require_guard_t;

/**
 * \brief Creates and initializes a new instance of a script_require_guard.
 *
 * This function allocates and initializes a new instance of \ref cardano_script_require_guard_t
 * with a provided credential. It returns an error code to indicate success or failure of the operation.
 *
 * \param[in] credential A constant pointer to the \ref cardano_credential_t object representing the guard credential.
 * \param[out] script_require_guard On successful initialization, this will point to a newly created
 *             \ref cardano_script_require_guard_t object. This object represents a "strong reference"
 *             to the script_require_guard, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the script_require_guard is no longer needed, the caller must release it
 *             by calling \ref cardano_script_require_guard_unref.
 *
 * \return \ref CARDANO_SUCCESS if the script_require_guard was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential = ...; // Assume credential is initialized here
 * cardano_script_require_guard_t* script_require_guard = NULL;
 *
 * // Attempt to create a new script_require_guard
 * cardano_error_t result = cardano_script_require_guard_new(credential, &script_require_guard);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script_require_guard
 *
 *   // Once done, ensure to clean up and release the script_require_guard
 *   cardano_script_require_guard_unref(&script_require_guard);
 * }
 *
 * cardano_credential_unref(&credential);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_script_require_guard_new(cardano_credential_t* credential, cardano_script_require_guard_t** script_require_guard);

/**
 * \brief Creates a script_require_guard from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_script_require_guard_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a script_require_guard.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded script_require_guard data.
 * \param[out] script_require_guard A pointer to a pointer of \ref cardano_script_require_guard_t that will be set to the address
 *                        of the newly created script_require_guard object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the script_require_guard was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_script_require_guard_t object by calling
 *       \ref cardano_script_require_guard_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_script_require_guard_t* script_require_guard = NULL;
 *
 * cardano_error_t result = cardano_script_require_guard_from_cbor(reader, &script_require_guard);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script_require_guard
 *
 *   // Once done, ensure to clean up and release the script_require_guard
 *   cardano_script_require_guard_unref(&script_require_guard);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode script_require_guard: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_script_require_guard_from_cbor(cardano_cbor_reader_t* reader, cardano_script_require_guard_t** script_require_guard);

/**
 * \brief Serializes a script_require_guard into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_script_require_guard_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] script_require_guard A constant pointer to the \ref cardano_script_require_guard_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p script_require_guard or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_require_guard_t* script_require_guard = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_script_require_guard_to_cbor(script_require_guard, writer);
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
 * cardano_script_require_guard_unref(&script_require_guard);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_script_require_guard_to_cbor(
  const cardano_script_require_guard_t* script_require_guard,
  cardano_cbor_writer_t*                writer);

/**
 * \brief Serializes a script require guard to CIP-116 JSON.
 *
 * The script require guard is emitted as a JSON object with the following shape:
 *
 * \code{.json}
 * {
 *   "tag": "require_guard",
 *   "credential": {
 *     "tag": "pubkey_hash",
 *     "value": "<28-byte-blake2b-hex>"
 *   }
 * }
 * \endcode
 *
 * \param[in]     script_require_guard A valid pointer to the script require guard to serialize.
 * \param[in,out] writer               A valid JSON writer, positioned where a value is expected.
 *
 * \retval CARDANO_SUCCESS               On success.
 * \retval CARDANO_ERROR_POINTER_IS_NULL If \p script_require_guard or \p writer is NULL.
 * \retval <propagated error>            If underlying operations (e.g., writing JSON,
 *                                       converting the credential to hex) fail.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_script_require_guard_to_cip116_json(
  const cardano_script_require_guard_t* script_require_guard,
  cardano_json_writer_t*                writer);

/**
 * \brief Creates a script_require_guard from a JSON string.
 *
 * This function parses JSON data using a provided JSON string and constructs a \ref cardano_script_require_guard_t object.
 * It assumes that the JSON data corresponds to the structure expected for a script_require_guard. The guard credential
 * is given either as a "keyHash" or a "scriptHash" field.
 *
 * \param[in] json A pointer to a null-terminated string containing the JSON-encoded script_require_guard data. This string
 *                 must not be NULL.
 * \param[in] json_size The size of the JSON string, excluding the null terminator.
 * \param[out] native_script A pointer to a pointer of \ref cardano_script_require_guard_t that will be set to the address
 *                           of the newly created script_require_guard object upon successful decoding. The caller is responsible
 *                           for managing the lifecycle of this object by calling \ref cardano_script_require_guard_unref when it
 *                           is no longer needed.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the script_require_guard was successfully created, or an appropriate error code if an error occurred.
 *
 * Usage Example:
 * \code{.c}
 * const char* json_string = "{\"type\": \"guard\", \"keyHash\": \"...\"}"; // Example JSON string
 * size_t json_size = strlen(json_string); // Calculate the size of the JSON string
 * cardano_script_require_guard_t* script_require_guard = NULL;
 *
 * // Attempt to create a new script_require_guard from a JSON string
 * cardano_error_t result = cardano_script_require_guard_from_json(json_string, json_size, &script_require_guard);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script_require_guard
 *
 *   // Once done, ensure to clean up and release the script_require_guard
 *   cardano_script_require_guard_unref(&script_require_guard);
 * }
 * else
 * {
 *   printf("Failed to create script_require_guard from JSON. Error code: %d\n", result);
 * }
 * \endcode
 */
cardano_error_t
cardano_script_require_guard_from_json(const char* json, size_t json_size, cardano_script_require_guard_t** native_script);

/**
 * \brief Retrieves the credential associated with a script_require_guard.
 *
 * This function retrieves the credential from the provided \ref cardano_script_require_guard_t object
 * and stores it in the output parameter.
 *
 * \param[in] script_require_guard A constant pointer to the \ref cardano_script_require_guard_t object from which
 *                                 the credential is to be retrieved.
 * \param[out] credential Pointer to a variable where the retrieved credential will be stored.
 *                        This variable will point to the retrieved \ref cardano_credential_t object.
 *                        The caller is responsible for managing the lifecycle of the credential by calling
 *                        \ref cardano_credential_unref when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the credential was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_require_guard_t* script_require_guard = ...; // Assume script_require_guard is initialized
 *
 * cardano_credential_t* retrieved_credential = NULL;
 * cardano_error_t result = cardano_script_require_guard_get_credential(script_require_guard, &retrieved_credential);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the retrieved credential
 *   // Remember to unreference the credential once done if it's no longer needed
 *   cardano_credential_unref(&retrieved_credential);
 * }
 *
 * // Clean up the script_require_guard object once done
 * cardano_script_require_guard_unref(&script_require_guard);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_script_require_guard_get_credential(cardano_script_require_guard_t* script_require_guard, cardano_credential_t** credential);

/**
 * \brief Sets the credential for a script_require_guard.
 *
 * This function sets the credential for the provided \ref cardano_script_require_guard_t object.
 *
 * \param[in] script_require_guard A pointer to the \ref cardano_script_require_guard_t object to which
 *                                 the credential is to be set.
 * \param[in] credential A pointer to the \ref cardano_credential_t object that is to be set as the credential for the script_require_guard.
 *                       The credential will be referenced by the script_require_guard after setting.
 *
 * \return \ref CARDANO_SUCCESS if the credential was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_require_guard_t* script_require_guard = ...; // Assume script_require_guard is initialized
 * cardano_credential_t* credential = ...; // Assume credential is initialized
 *
 * // Set the credential for the script_require_guard
 * cardano_error_t result = cardano_script_require_guard_set_credential(script_require_guard, credential);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Credential set successfully
 * }
 *
 * // Clean up both the script_require_guard and credential object once done
 * cardano_script_require_guard_unref(&script_require_guard);
 * cardano_credential_unref(&credential);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_script_require_guard_set_credential(cardano_script_require_guard_t* script_require_guard, cardano_credential_t* credential);

/**
 * \brief Checks if two script_require_guard objects are equal.
 *
 * This function compares two \ref cardano_script_require_guard_t objects to determine if they are equal. Two script_require_guard
 * objects are considered equal if their credentials are equal.
 *
 * \param[in] lhs A constant pointer to the first \ref cardano_script_require_guard_t object to be compared.
 * \param[in] rhs A constant pointer to the second \ref cardano_script_require_guard_t object to be compared.
 *
 * \return \c true if the two script_require_guard objects are equal, \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential1 = ...; // Assume credential1 is initialized here
 * cardano_credential_t* credential2 = ...; // Assume credential2 is initialized here
 *
 * cardano_script_require_guard_t* script_require_guard1 = NULL;
 * cardano_script_require_guard_t* script_require_guard2 = NULL;
 *
 * cardano_script_require_guard_new(credential1, &script_require_guard1);
 * cardano_script_require_guard_new(credential2, &script_require_guard2);
 *
 * if (cardano_script_require_guard_equals(script_require_guard1, script_require_guard2))
 * {
 *   // The two script_require_guards are equal
 * }
 * else
 * {
 *   // The two script_require_guards are not equal
 * }
 *
 * // Clean up the objects once done
 * cardano_script_require_guard_unref(&script_require_guard1);
 * cardano_script_require_guard_unref(&script_require_guard2);
 * cardano_credential_unref(&credential1);
 * cardano_credential_unref(&credential2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_script_require_guard_equals(const cardano_script_require_guard_t* lhs, const cardano_script_require_guard_t* rhs);

/**
 * \brief Decrements the reference count of a script_require_guard object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_script_require_guard_t object
 * by decreasing its reference count. When the reference count reaches zero, the script_require_guard is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] script_require_guard A pointer to the pointer of the script_require_guard object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_require_guard_t* script_require_guard = cardano_script_require_guard_new();
 *
 * // Perform operations with the script_require_guard...
 *
 * cardano_script_require_guard_unref(&script_require_guard);
 * // At this point, script_require_guard is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_script_require_guard_unref, the pointer to the \ref cardano_script_require_guard_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_script_require_guard_unref(cardano_script_require_guard_t** script_require_guard);

/**
 * \brief Increases the reference count of the cardano_script_require_guard_t object.
 *
 * This function is used to manually increment the reference count of a script_require_guard
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_script_require_guard_unref.
 *
 * \param script_require_guard A pointer to the script_require_guard object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming script_require_guard is a previously created script_require_guard object
 *
 * cardano_script_require_guard_ref(script_require_guard);
 *
 * // Now script_require_guard can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_script_require_guard_ref there is a corresponding
 * call to \ref cardano_script_require_guard_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_script_require_guard_ref(cardano_script_require_guard_t* script_require_guard);

/**
 * \brief Retrieves the current reference count of the cardano_script_require_guard_t object.
 *
 * This function returns the number of active references to a script_require_guard object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_script_require_guard_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param script_require_guard A pointer to the script_require_guard object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified script_require_guard object. If the object
 * is properly managed (i.e., every \ref cardano_script_require_guard_ref call is matched with a
 * \ref cardano_script_require_guard_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming script_require_guard is a previously created script_require_guard object
 *
 * size_t ref_count = cardano_script_require_guard_refcount(script_require_guard);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_script_require_guard_refcount(const cardano_script_require_guard_t* script_require_guard);

/**
 * \brief Sets the last error message for a given script_require_guard object.
 *
 * Records an error message in the script_require_guard's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] script_require_guard A pointer to the \ref cardano_script_require_guard_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the script_require_guard's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_script_require_guard_set_last_error(cardano_script_require_guard_t* script_require_guard, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific script_require_guard.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_script_require_guard_set_last_error for the given
 * script_require_guard. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] script_require_guard A pointer to the \ref cardano_script_require_guard_t instance whose last error
 *                   message is to be retrieved. If the script_require_guard is NULL, the function
 *                   returns a generic error message indicating the null script_require_guard.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified script_require_guard. If the script_require_guard is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_script_require_guard_set_last_error for the same script_require_guard, or until
 *       the script_require_guard is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_script_require_guard_get_last_error(const cardano_script_require_guard_t* script_require_guard);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_REQUIRE_GUARD_H
