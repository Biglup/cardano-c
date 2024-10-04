/**
 * \file script_pubkey.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_PUBKEY_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_PUBKEY_H

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
 * \brief The Native scripts form an expression tree, the evaluation of the script produces either true or false.
 *
 * Note that it is recursive. There are no constraints on the nesting or size, except that imposed by the overall
 * transaction size limit (given that the script must be included in the transaction in a script witnesses).
 */
typedef struct cardano_native_script_t cardano_native_script_t;

/**
 * \brief This script evaluates to true if the transaction also includes a valid key witness
 * where the witness verification key hashes to the given hash.
 *
 * In other words, this checks that the transaction is signed by a particular key, identified by its verification
 * key hash.
 */
typedef struct cardano_script_pubkey_t cardano_script_pubkey_t;

/**
 * \brief Creates and initializes a new instance of a script_pubkey.
 *
 * This function allocates and initializes a new instance of \ref cardano_script_pubkey_t
 * with a provided key hash. It returns an error code to indicate success or failure of the operation.
 *
 * \param[in] key_hash A constant pointer to the \ref cardano_blake2b_hash_t object representing the key hash.
 * \param[out] script_pubkey On successful initialization, this will point to a newly created
 *             \ref cardano_script_pubkey_t object. This object represents a "strong reference"
 *             to the script_pubkey, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the script_pubkey is no longer needed, the caller must release it
 *             by calling \ref cardano_script_pubkey_unref.
 *
 * \return \ref CARDANO_SUCCESS if the script_pubkey was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_t* key_hash = ...; // Assume key_hash is initialized here
 * cardano_script_pubkey_t* script_pubkey = NULL;
 *
 * // Attempt to create a new script_pubkey
 * cardano_error_t result = cardano_script_pubkey_new(key_hash, &script_pubkey);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script_pubkey
 *
 *   // Once done, ensure to clean up and release the script_pubkey
 *   cardano_script_pubkey_unref(&script_pubkey);
 * }
 *
 * cardano_blake2b_hash_unref(&key_hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_script_pubkey_new(cardano_blake2b_hash_t* key_hash, cardano_script_pubkey_t** script_pubkey);

/**
 * \brief Creates a script_pubkey from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_script_pubkey_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a script_pubkey.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded script_pubkey data.
 * \param[out] script_pubkey A pointer to a pointer of \ref cardano_script_pubkey_t that will be set to the address
 *                        of the newly created script_pubkey object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the script_pubkey was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_script_pubkey_t object by calling
 *       \ref cardano_script_pubkey_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_script_pubkey_t* script_pubkey = NULL;
 *
 * cardano_error_t result = cardano_script_pubkey_from_cbor(reader, &script_pubkey);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script_pubkey
 *
 *   // Once done, ensure to clean up and release the script_pubkey
 *   cardano_script_pubkey_unref(&script_pubkey);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode script_pubkey: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_script_pubkey_from_cbor(cardano_cbor_reader_t* reader, cardano_script_pubkey_t** script_pubkey);

/**
 * \brief Serializes a script_pubkey into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_script_pubkey_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] script_pubkey A constant pointer to the \ref cardano_script_pubkey_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p script_pubkey or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_pubkey_t* script_pubkey = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_script_pubkey_to_cbor(script_pubkey, writer);
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
 * cardano_script_pubkey_unref(&script_pubkey);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_script_pubkey_to_cbor(
  const cardano_script_pubkey_t* script_pubkey,
  cardano_cbor_writer_t*         writer);

/**
 * \brief Creates a script_pubkey from a JSON string.
 *
 * This function parses JSON data using a provided JSON string and constructs a \ref cardano_script_pubkey_t object.
 * It assumes that the JSON data corresponds to the structure expected for a script_pubkey.
 *
 * \param[in] json A pointer to a null-terminated string containing the JSON-encoded script_pubkey data. This string
 *                 must not be NULL.
 * \param[in] json_size The size of the JSON string, excluding the null terminator.
 * \param[out] native_script A pointer to a pointer of \ref cardano_script_pubkey_t that will be set to the address
 *                           of the newly created script_pubkey object upon successful decoding. The caller is responsible
 *                           for managing the lifecycle of this object by calling \ref cardano_script_pubkey_unref when it
 *                           is no longer needed.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the script_pubkey was successfully created, or an appropriate error code if an error occurred.
 *
 * Usage Example:
 * \code{.c}
 * const char* json_string = "{\"type\": \"sig\", \"keyHash\": \"...\"}"; // Example JSON string
 * size_t json_size = strlen(json_string); // Calculate the size of the JSON string
 * cardano_script_pubkey_t* script_pubkey = NULL;
 *
 * // Attempt to create a new script_pubkey from a JSON string
 * cardano_error_t result = cardano_script_pubkey_from_json(json_string, json_size, &script_pubkey);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script_pubkey
 *
 *   // Once done, ensure to clean up and release the script_pubkey
 *   cardano_script_pubkey_unref(&script_pubkey);
 * }
 * else
 * {
 *   printf("Failed to create script_pubkey from JSON. Error code: %d\n", result);
 * }
 * \endcode
 */
cardano_error_t
cardano_script_pubkey_from_json(const char* json, size_t json_size, cardano_script_pubkey_t** native_script);

/**
 * \brief Retrieves the key hash associated with a script_pubkey.
 *
 * This function retrieves the key hash from the provided \ref cardano_script_pubkey_t object
 * and stores it in the output parameter.
 *
 * \param[in] script_pubkey A constant pointer to the \ref cardano_script_pubkey_t object from which
 *                          the key hash is to be retrieved.
 * \param[out] key_hash Pointer to a variable where the retrieved key hash will be stored.
 *                      This variable will point to the retrieved \ref cardano_blake2b_hash_t object.
 *                      The caller is responsible for managing the lifecycle of the key hash by calling
 *                      \ref cardano_blake2b_hash_unref when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the key hash was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_t* script_pubkey = ...; // Assume script_pubkey is initialized
 *
 * cardano_blake2b_hash_t* retrieved_key_hash = NULL;
 * cardano_error_t result = cardano_script_pubkey_get_key_hash(script_pubkey, &retrieved_key_hash);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the retrieved key hash
 *   // Remember to unreference the key hash once done if it's no longer needed
 *   cardano_blake2b_hash_unref(&retrieved_key_hash);
 * }
 *
 * // Clean up the script_pubkey object once done
 * cardano_script_pubkey_unref(&script_pubkey);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_script_pubkey_get_key_hash(cardano_script_pubkey_t* script_pubkey, cardano_blake2b_hash_t** key_hash);

/**
 * \brief Sets the key hash for a script_pubkey.
 *
 * This function sets the key hash for the provided \ref cardano_script_pubkey_t object.
 *
 * \param[in] script_pubkey A pointer to the \ref cardano_script_pubkey_t object to which
 *                          the key hash is to be set.
 * \param[in] key_hash A pointer to the \ref cardano_blake2b_hash_t object that is to be set as the key hash for the script_pubkey.
 *                     The key hash will be referenced by the script_pubkey after setting.
 *
 * \return \ref CARDANO_SUCCESS if the key hash was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_pubkey_t* script_pubkey = ...; // Assume script_pubkey is initialized
 * cardano_blake2b_hash_t* key_hash = ...; // Assume key_hash is initialized
 *
 * // Set the key hash for the script_pubkey
 * cardano_error_t result = cardano_script_pubkey_set_key_hash(script_pubkey, key_hash);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Key hash set successfully
 * }
 *
 * // Clean up both the script_pubkey and key_hash object once done
 * cardano_script_pubkey_unref(&script_pubkey);
 * cardano_blake2b_hash_unref(&key_hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_script_pubkey_set_key_hash(cardano_script_pubkey_t* script_pubkey, cardano_blake2b_hash_t* key_hash);

/**
 * \brief Checks if two script_pubkey objects are equal.
 *
 * This function compares two \ref cardano_script_pubkey_t objects to determine if they are equal. Two script_pubkey
 * objects are considered equal if their key hashes are equal.
 *
 * \param[in] lhs A constant pointer to the first \ref cardano_script_pubkey_t object to be compared.
 * \param[in] rhs A constant pointer to the second \ref cardano_script_pubkey_t object to be compared.
 *
 * \return \c true if the two script_pubkey objects are equal, \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_t* key_hash1 = ...; // Assume key_hash1 is initialized here
 * cardano_blake2b_hash_t* key_hash2 = ...; // Assume key_hash2 is initialized here
 *
 * cardano_script_pubkey_t* script_pubkey1 = NULL;
 * cardano_script_pubkey_t* script_pubkey2 = NULL;
 *
 * cardano_script_pubkey_new(key_hash1, &script_pubkey1);
 * cardano_script_pubkey_new(key_hash2, &script_pubkey2);
 *
 * if (cardano_script_pubkey_equals(script_pubkey1, script_pubkey2))
 * {
 *   // The two script_pubkeys are equal
 * }
 * else
 * {
 *   // The two script_pubkeys are not equal
 * }
 *
 * // Clean up the objects once done
 * cardano_script_pubkey_unref(&script_pubkey1);
 * cardano_script_pubkey_unref(&script_pubkey2);
 * cardano_blake2b_hash_unref(&key_hash1);
 * cardano_blake2b_hash_unref(&key_hash2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_script_pubkey_equals(const cardano_script_pubkey_t* lhs, const cardano_script_pubkey_t* rhs);

/**
 * \brief Decrements the reference count of a script_pubkey object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_script_pubkey_t object
 * by decreasing its reference count. When the reference count reaches zero, the script_pubkey is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] script_pubkey A pointer to the pointer of the script_pubkey object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_pubkey_t* script_pubkey = cardano_script_pubkey_new();
 *
 * // Perform operations with the script_pubkey...
 *
 * cardano_script_pubkey_unref(&script_pubkey);
 * // At this point, script_pubkey is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_script_pubkey_unref, the pointer to the \ref cardano_script_pubkey_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_script_pubkey_unref(cardano_script_pubkey_t** script_pubkey);

/**
 * \brief Increases the reference count of the cardano_script_pubkey_t object.
 *
 * This function is used to manually increment the reference count of a script_pubkey
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_script_pubkey_unref.
 *
 * \param script_pubkey A pointer to the script_pubkey object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming script_pubkey is a previously created script_pubkey object
 *
 * cardano_script_pubkey_ref(script_pubkey);
 *
 * // Now script_pubkey can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_script_pubkey_ref there is a corresponding
 * call to \ref cardano_script_pubkey_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_script_pubkey_ref(cardano_script_pubkey_t* script_pubkey);

/**
 * \brief Retrieves the current reference count of the cardano_script_pubkey_t object.
 *
 * This function returns the number of active references to a script_pubkey object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_script_pubkey_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param script_pubkey A pointer to the script_pubkey object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified script_pubkey object. If the object
 * is properly managed (i.e., every \ref cardano_script_pubkey_ref call is matched with a
 * \ref cardano_script_pubkey_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming script_pubkey is a previously created script_pubkey object
 *
 * size_t ref_count = cardano_script_pubkey_refcount(script_pubkey);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_script_pubkey_refcount(const cardano_script_pubkey_t* script_pubkey);

/**
 * \brief Sets the last error message for a given script_pubkey object.
 *
 * Records an error message in the script_pubkey's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] script_pubkey A pointer to the \ref cardano_script_pubkey_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the script_pubkey's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_script_pubkey_set_last_error(cardano_script_pubkey_t* script_pubkey, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific script_pubkey.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_script_pubkey_set_last_error for the given
 * script_pubkey. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] script_pubkey A pointer to the \ref cardano_script_pubkey_t instance whose last error
 *                   message is to be retrieved. If the script_pubkey is NULL, the function
 *                   returns a generic error message indicating the null script_pubkey.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified script_pubkey. If the script_pubkey is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_script_pubkey_set_last_error for the same script_pubkey, or until
 *       the script_pubkey is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_script_pubkey_get_last_error(const cardano_script_pubkey_t* script_pubkey);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_PUBKEY_H