/**
 * \file script_invalid_after.h
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
 * WITHOUT WARRANTIES OR CONDITIONS OF N_OF KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_INVALID_AFTER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_INVALID_AFTER_H

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
 * \brief This script evaluates to true if the upper bound of the transaction validity interval is a
 * slot number Y, and X <= Y.
 *
 * This condition guarantees that the actual slot number in which the transaction is included is
 * (strictly) less than slot number X.
 */
typedef struct cardano_script_invalid_after_t cardano_script_invalid_after_t;

/**
 * \brief Creates and initializes a new instance of a script_invalid_after.
 *
 * This function allocates and initializes a new instance of \ref cardano_script_invalid_after_t
 * with a provided slot number. It returns an error code to indicate success or failure of the operation.
 *
 * \param[in] slot The slot number representing the upper bound of the transaction validity interval.
 * \param[out] script_invalid_after On successful initialization, this will point to a newly created
 *             \ref cardano_script_invalid_after_t object. This object represents a "strong reference"
 *             to the script_invalid_after, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the script_invalid_after is no longer needed, the caller must release it
 *             by calling \ref cardano_script_invalid_after_unref.
 *
 * \return \ref CARDANO_SUCCESS if the script_invalid_after was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * uint64_t slot = 1000;
 * cardano_script_invalid_after_t* script_invalid_after = NULL;
 *
 * // Attempt to create a new script_invalid_after
 * cardano_error_t result = cardano_script_invalid_after_new(slot, &script_invalid_after);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script_invalid_after
 *
 *   // Once done, ensure to clean up and release the script_invalid_after
 *   cardano_script_invalid_after_unref(&script_invalid_after);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_script_invalid_after_new(uint64_t slot, cardano_script_invalid_after_t** script_invalid_after);

/**
 * \brief Creates a script_invalid_after from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_script_invalid_after_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a script_invalid_after.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded script_invalid_after data.
 * \param[out] script_invalid_after A pointer to a pointer of \ref cardano_script_invalid_after_t that will be set to the address
 *                        of the newly created script_invalid_after object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the script_invalid_after was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_script_invalid_after_t object by calling
 *       \ref cardano_script_invalid_after_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_script_invalid_after_t* script_invalid_after = NULL;
 *
 * cardano_error_t result = cardano_script_invalid_after_from_cbor(reader, &script_invalid_after);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script_invalid_after
 *
 *   // Once done, ensure to clean up and release the script_invalid_after
 *   cardano_script_invalid_after_unref(&script_invalid_after);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode script_invalid_after: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_script_invalid_after_from_cbor(cardano_cbor_reader_t* reader, cardano_script_invalid_after_t** script_invalid_after);

/**
 * \brief Serializes a script_invalid_after into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_script_invalid_after_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] script_invalid_after A constant pointer to the \ref cardano_script_invalid_after_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p script_invalid_after or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_invalid_after_t* script_invalid_after = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_script_invalid_after_to_cbor(script_invalid_after, writer);
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
 * cardano_script_invalid_after_unref(&script_invalid_after);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_script_invalid_after_to_cbor(
  const cardano_script_invalid_after_t* script_invalid_after,
  cardano_cbor_writer_t*                writer);

/**
 * \brief Creates a script_invalid_after from a JSON string.
 *
 * This function parses JSON data using a provided JSON string and constructs a \ref cardano_script_invalid_after_t object.
 * It assumes that the JSON data corresponds to the structure expected for a script_invalid_after.
 *
 * \param[in] json A pointer to a null-terminated string containing the JSON-encoded script_invalid_after data. This string
 *                 must not be NULL.
 * \param[in] json_size The size of the JSON string, excluding the null terminator.
 * \param[out] native_script A pointer to a pointer of \ref cardano_script_invalid_after_t that will be set to the address
 *                                  of the newly created script_invalid_after object upon successful decoding. The caller is responsible
 *                                  for managing the lifecycle of this object by calling \ref cardano_script_invalid_after_unref when it
 *                                  is no longer needed.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the script_invalid_after was successfully created, or an appropriate error code if an error occurred.
 *
 * Usage Example:
 * \code{.c}
 * const char* json_string = "{\"type\": \"after\", \"slot\": 1000}"; // Example JSON string
 * size_t json_size = strlen(json_string); // Calculate the size of the JSON string
 * cardano_script_invalid_after_t* script_invalid_after = NULL;
 *
 * // Attempt to create a new script_invalid_after from a JSON string
 * cardano_error_t result = cardano_script_invalid_after_from_json(json_string, json_size, &script_invalid_after);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the script_invalid_after
 *
 *   // Once done, ensure to clean up and release the script_invalid_after
 *   cardano_script_invalid_after_unref(&script_invalid_after);
 * }
 * else
 * {
 *   printf("Failed to create script_invalid_after from JSON. Error code: %d\n", result);
 * }
 * \endcode
 */
cardano_error_t
cardano_script_invalid_after_from_json(const char* json, size_t json_size, cardano_script_invalid_after_t** native_script);

/**
 * \brief Retrieves the slot number from a script_invalid_after object.
 *
 * This function retrieves the slot number from the provided \ref cardano_script_invalid_after_t object and stores it in the provided
 * output parameter.
 *
 * \param[in] script_invalid_after A constant pointer to the \ref cardano_script_invalid_after_t object from which
 *                                 the slot number is to be retrieved. The object must not be NULL.
 * \param[out] slot Pointer to a variable where the slot number will be stored.
 *                  This variable will be set to the retrieved slot number.
 *
 * \return \ref CARDANO_SUCCESS if the slot number was successfully retrieved, or an appropriate error code if the
 *         input is NULL or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_invalid_after_t* script_invalid_after = ...; // Assume script_invalid_after is initialized
 * uint64_t slot;
 * cardano_error_t result = cardano_script_invalid_after_get_slot(script_invalid_after, &slot);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the slot number
 *   printf("Slot number: %llu\n", (unsigned long long)slot);
 * }
 * else
 * {
 *   // Handle error
 *   fprintf(stderr, "Failed to retrieve slot number: %s.\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up the script_invalid_after object once done
 * cardano_script_invalid_after_unref(&script_invalid_after);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_script_invalid_after_get_slot(const cardano_script_invalid_after_t* script_invalid_after, uint64_t* slot);

/**
 * \brief Sets the slot number for a script_invalid_after object.
 *
 * This function sets the slot number for the provided \ref cardano_script_invalid_after_t object.
 *
 * \param[in] script_invalid_after A pointer to the \ref cardano_script_invalid_after_t object for which
 *                                 the slot number is to be set.
 * \param[in] slot The slot number to set.
 *
 * \return \ref CARDANO_SUCCESS if the slot number was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_invalid_after_t* script_invalid_after = ...; // Assume script_invalid_after is initialized
 * uint64_t slot = 1000;
 *
 * // Set the slot number for the script_invalid_after
 * cardano_error_t result = cardano_script_invalid_after_set_slot(script_invalid_after, slot);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Slot number set successfully
 * }
 *
 * // Clean up the script_invalid_after object once done
 * cardano_script_invalid_after_unref(&script_invalid_after);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_script_invalid_after_set_slot(cardano_script_invalid_after_t* script_invalid_after, uint64_t slot);

/**
 * \brief Checks if two script_invalid_after objects are equal.
 *
 * This function compares two \ref cardano_script_invalid_after_t objects to determine if they are equal.
 * Two script_invalid_after objects are considered equal if they have the same slot number.
 *
 * \param[in] lhs A constant pointer to the first \ref cardano_script_invalid_after_t object to be compared.
 * \param[in] rhs A constant pointer to the second \ref cardano_script_invalid_after_t object to be compared.
 *
 * \return \c true if the two script_invalid_after objects are equal, \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * uint64_t slot1 = 1000;
 * uint64_t slot2 = 1000;
 * cardano_script_invalid_after_t* script_invalid_after1 = NULL;
 * cardano_script_invalid_after_t* script_invalid_after2 = NULL;
 *
 * cardano_script_invalid_after_new(slot1, &script_invalid_after1);
 * cardano_script_invalid_after_new(slot2, &script_invalid_after2);
 *
 * if (cardano_script_invalid_after_equals(script_invalid_after1, script_invalid_after2))
 * {
 *   // The two script_invalid_after objects are equal
 * }
 * else
 * {
 *   // The two script_invalid_after objects are not equal
 * }
 *
 * // Clean up the script_invalid_after objects once done
 * cardano_script_invalid_after_unref(&script_invalid_after1);
 * cardano_script_invalid_after_unref(&script_invalid_after2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_script_invalid_after_equals(const cardano_script_invalid_after_t* lhs, const cardano_script_invalid_after_t* rhs);

/**
 * \brief Decrements the reference count of a script_invalid_after object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_script_invalid_after_t object
 * by decreasing its reference count. When the reference count reaches zero, the script_invalid_after is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] script_invalid_after A pointer to the pointer of the script_invalid_after object. This double
 *                            indirection n_ofows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_script_invalid_after_t* script_invalid_after = cardano_script_invalid_after_new();
 *
 * // Perform operations with the script_invalid_after...
 *
 * cardano_script_invalid_after_unref(&script_invalid_after);
 * // At this point, script_invalid_after is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_script_invalid_after_unref, the pointer to the \ref cardano_script_invalid_after_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_script_invalid_after_unref(cardano_script_invalid_after_t** script_invalid_after);

/**
 * \brief Increases the reference count of the cardano_script_invalid_after_t object.
 *
 * This function is used to manun_ofy increment the reference count of a script_invalid_after
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains n_ofocated and valid until n_of owners have released their
 * reference by calling \ref cardano_script_invalid_after_unref.
 *
 * \param script_invalid_after A pointer to the script_invalid_after object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming script_invalid_after is a previously created script_invalid_after object
 *
 * cardano_script_invalid_after_ref(script_invalid_after);
 *
 * // Now script_invalid_after can be safely used elsewhere without worrying about premature den_ofocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_script_invalid_after_ref there is a corresponding
 * call to \ref cardano_script_invalid_after_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_script_invalid_after_ref(cardano_script_invalid_after_t* script_invalid_after);

/**
 * \brief Retrieves the current reference count of the cardano_script_invalid_after_t object.
 *
 * This function returns the number of active references to a script_invalid_after object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_script_invalid_after_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param script_invalid_after A pointer to the script_invalid_after object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified script_invalid_after object. If the object
 * is properly managed (i.e., every \ref cardano_script_invalid_after_ref call is matched with a
 * \ref cardano_script_invalid_after_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming script_invalid_after is a previously created script_invalid_after object
 *
 * size_t ref_count = cardano_script_invalid_after_refcount(script_invalid_after);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_script_invalid_after_refcount(const cardano_script_invalid_after_t* script_invalid_after);

/**
 * \brief Sets the last error message for a given script_invalid_after object.
 *
 * Records an error message in the script_invalid_after's last_error buffer, overwriting n_of existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] script_invalid_after A pointer to the \ref cardano_script_invalid_after_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the script_invalid_after's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_script_invalid_after_set_last_error(cardano_script_invalid_after_t* script_invalid_after, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific script_invalid_after.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_script_invalid_after_set_last_error for the given
 * script_invalid_after. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] script_invalid_after A pointer to the \ref cardano_script_invalid_after_t instance whose last error
 *                   message is to be retrieved. If the script_invalid_after is NULL, the function
 *                   returns a generic error message indicating the null script_invalid_after.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified script_invalid_after. If the script_invalid_after is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_script_invalid_after_set_last_error for the same script_invalid_after, or until
 *       the script_invalid_after is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_script_invalid_after_get_last_error(const cardano_script_invalid_after_t* script_invalid_after);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_INVALID_AFTER_H