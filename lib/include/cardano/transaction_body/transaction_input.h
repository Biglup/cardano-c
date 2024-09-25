/**
 * \file transaction_input.h
 *
 * \author angel.castillo
 * \date   Sep 05, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_INPUT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_INPUT_H

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
 * \brief Represents a reference to an unspent transaction output (UTxO) from a previous
 * transaction, which the current transaction intends to spend.
 */
typedef struct cardano_transaction_input_t cardano_transaction_input_t;

/**
 * \brief Creates and initializes a new instance of the Transaction Input.
 *
 * This function allocates and initializes a new instance of a \ref cardano_transaction_input_t object,
 * representing an input in a Cardano transaction. Each transaction input is a reference to an unspent transaction output (UTxO)
 * from a previous transaction, which the current transaction intends to spend.
 *
 * \param[in] id A pointer to a \ref cardano_blake2b_hash_t object representing the transaction ID of the UTxO.
 * \param[in] index The output index within the transaction identified by the ID, specifying which output from the given transaction is being spent.
 * \param[out] transaction_input On successful initialization, this will point to a newly created \ref cardano_transaction_input_t object.
 *             This object represents a "strong reference," meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object,
 *             specifically, once the Transaction Input is no longer needed, the caller must release it
 *             by calling \ref cardano_transaction_input_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the Transaction Input was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * uint64_t index = 0; // Index of the UTxO to be spent
 * cardano_blake2b_hash_t* id = cardano_blake2b_hash_new(...); // Assume id is already initialized
 * cardano_transaction_input_t* transaction_input = NULL;
 *
 * // Attempt to create a new Transaction Input object
 * cardano_error_t result = cardano_transaction_input_new(id, index, &transaction_input);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the transaction_input
 *
 *   // Once done, ensure to clean up and release the transaction_input
 *   cardano_transaction_input_unref(&transaction_input);
 * }
 *
 * // Cleanup the id after use
 * cardano_blake2b_hash_unref(&id);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_input_new(
  cardano_blake2b_hash_t*       id,
  uint64_t                      index,
  cardano_transaction_input_t** transaction_input);

/**
 * \brief Creates and initializes a new instance of the Transaction Input from a hexadecimal string.
 *
 * This function allocates and initializes a new instance of a \ref cardano_transaction_input_t object
 * by decoding a hexadecimal string that represents the transaction ID. The function takes an input index
 * indicating which output from the specified transaction ID is being referenced to be spent.
 *
 * \param[in] id_hex A pointer to a character array containing the hexadecimal representation of the transaction ID.
 * \param[in] id_hex_size The size of the hexadecimal string (number of characters).
 * \param[in] index The output index within the transaction identified by the ID, specifying which output from the
 *                  given transaction is intended to be spent.
 * \param[out] transaction_input On successful initialization, this will point to a newly created \ref cardano_transaction_input_t object.
 *              This object represents a "strong reference," meaning that it is fully initialized and ready for use.
 *              The caller is responsible for managing the lifecycle of this object,
 *              specifically, once the Transaction Input is no longer needed, the caller must release it
 *              by calling \ref cardano_transaction_input_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the Transaction Input was successfully created from the hexadecimal string, or an appropriate error code
 *         indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * const char* id_hex = "abc123..."; // Transaction ID in hexadecimal
 * size_t id_hex_size = strlen(id_hex);
 * uint64_t index = 0; // Index of the UTxO to be spent
 * cardano_transaction_input_t* transaction_input = NULL;
 *
 * // Attempt to create a new Transaction Input object from a hexadecimal string
 * cardano_error_t result = cardano_transaction_input_from_hex(id_hex, id_hex_size, index, &transaction_input);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the transaction_input
 *
 *   // Once done, ensure to clean up and release the transaction_input
 *   cardano_transaction_input_unref(&transaction_input);
 * }
 * else
 * {
 *   printf("Failed to create the transaction input: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_input_from_hex(
  const char*                   id_hex,
  size_t                        id_hex_size,
  uint64_t                      index,
  cardano_transaction_input_t** transaction_input);

/**
 * \brief Creates a \ref cardano_transaction_input_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_transaction_input_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a transaction_input.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] transaction_input A pointer to a pointer of \ref cardano_transaction_input_t that will be set to the address
 *                        of the newly created transaction_input object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the transaction input were successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_transaction_input_t object by calling
 *       \ref cardano_transaction_input_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_transaction_input_t* transaction_input = NULL;
 *
 * cardano_error_t result = cardano_transaction_input_from_cbor(reader, &transaction_input);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the transaction_input
 *
 *   // Once done, ensure to clean up and release the transaction_input
 *   cardano_transaction_input_unref(&transaction_input);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode transaction_input: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_input_from_cbor(cardano_cbor_reader_t* reader, cardano_transaction_input_t** transaction_input);

/**
 * \brief Serializes transaction input into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_transaction_input_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] transaction_input A constant pointer to the \ref cardano_transaction_input_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p transaction_input or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_input_t* transaction_input = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_transaction_input_to_cbor(transaction_input, writer);
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
 * cardano_transaction_input_unref(&transaction_input);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_transaction_input_to_cbor(
  const cardano_transaction_input_t* transaction_input,
  cardano_cbor_writer_t*             writer);

/**
 * \brief Retrieves the transaction ID associated with a transaction input.
 *
 * This function fetches the transaction ID from a given \ref cardano_transaction_input_t object. The transaction ID uniquely identifies
 * the transaction that contains the output being referenced as an input in another transaction.
 *
 * \param[in] input A pointer to an initialized \ref cardano_transaction_input_t object.
 *
 * \return A pointer to the \ref cardano_blake2b_hash_t object representing the transaction ID. If the input pointer is NULL, this function returns NULL.
 *         Note that the returned transaction ID is a new reference and must be released using \ref cardano_blake2b_hash_unref
 *         when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_input_t* input = ...; // Assume input is already initialized
 * cardano_blake2b_hash_t* transaction_id = cardano_transaction_input_get_id(input);
 *
 * if (transaction_id != NULL)
 * {
 *   // Process the transaction ID
 *   cardano_blake2b_hash_unref(&transaction_id);
 * }
 * else
 * {
 *   printf("Invalid transaction input or uninitialized input.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_transaction_input_get_id(cardano_transaction_input_t* input);

/**
 * \brief Sets the transaction ID for a transaction input.
 *
 * This function assigns a new transaction ID to a \ref cardano_transaction_input_t object.
 * The transaction ID uniquely identifies the transaction where the output, now used as an input, was originally created.
 *
 * \param[in,out] input A pointer to an initialized \ref cardano_transaction_input_t object to which the transaction ID will be set.
 * \param[in] id A pointer to an initialized \ref cardano_blake2b_hash_t object representing the transaction ID.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the transaction ID
 *         was successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * \note This function increases the reference count of the transaction ID object; therefore, the caller retains ownership of their respective references.
 *       It is the caller's responsibility to release their reference to the transaction ID when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_input_t* input = ...; // Assume input is already initialized
 * cardano_blake2b_hash_t* transaction_id = ...; // Assume transaction_id is initialized
 *
 * cardano_error_t result = cardano_transaction_input_set_id(input, transaction_id);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The transaction ID is now set for the input
 * }
 * else
 * {
 *   printf("Failed to set the transaction ID.\n");
 * }
 *
 * // Clean up resources
 * cardano_blake2b_hash_unref(&transaction_id);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_input_set_id(cardano_transaction_input_t* input, cardano_blake2b_hash_t* id);

/**
 * \brief Retrieves the index of the output used as an input from a previous transaction.
 *
 * This function fetches the index of the output in the transaction that is referenced by the transaction input.
 * The index specifies which output from the referenced transaction is being used as an input in the current transaction.
 *
 * \param[in] transaction_input A constant pointer to an initialized \ref cardano_transaction_input_t object.
 *
 * \return The index of the output from the referenced transaction. If the input is NULL, the function will return 0.
 *         It is important for the caller to ensure the input is not NULL before calling this function to distinguish
 *         from a genuine index of 0.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_transaction_input_t* transaction_input = ...; // Assume transaction_input is already initialized
 *
 * uint64_t index = cardano_transaction_input_get_index(transaction_input);
 * printf("Index of the used output: %llu\n", index);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_transaction_input_get_index(const cardano_transaction_input_t* transaction_input);

/**
 * \brief Sets the index of an output in the referenced transaction for use as an input in a new transaction.
 *
 * This function assigns an index to a \ref cardano_transaction_input_t object, specifying which output from a previously referenced transaction
 * is to be used as an input. The index corresponds to the position of the output within the transaction.
 *
 * \param[in,out] transaction_input A pointer to an initialized \ref cardano_transaction_input_t object to which the index will be set.
 * \param[in] index The index of the output in the previous transaction that is to be used as an input.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the index was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the transaction_input pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_input_t* transaction_input = ...; // Assume transaction_input is already initialized
 * uint64_t index = 0; // Index of the output in the previous transaction
 *
 * cardano_error_t result = cardano_transaction_input_set_index(transaction_input, index);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Index set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set the index.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_input_set_index(cardano_transaction_input_t* transaction_input, uint64_t index);

/**
 * \brief Compares two transaction input objects for equality.
 *
 * This function compares two transaction input objects to determine if they are equal.
 *
 * \param[in] lhs Pointer to the first transaction input object.
 * \param[in] rhs Pointer to the second transaction input object.
 *
 * \return \c true if the transaction_input objects are equal, \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_input_t* transaction_input1 = NULL;
 * cardano_transaction_input_t* transaction_input2 = NULL;
 *
 * // Assume transaction_input1 and transaction_input2 are initialized properly
 *
 * bool are_equal = cardano_transaction_input_equals(transaction_input1, transaction_input2);
 *
 * if (are_equal)
 * {
 *   printf("The transaction_input objects are equal.\n");
 * }
 * else
 * {
 *   printf("The transaction_input objects are not equal.\n");
 * }
 *
 * // Clean up
 * cardano_transaction_input_unref(&transaction_input1);
 * cardano_transaction_input_unref(&transaction_input2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_transaction_input_equals(const cardano_transaction_input_t* lhs, const cardano_transaction_input_t* rhs);

/**
 * \brief Compares two transaction input objects.
 *
 * This function compares two transaction input objects using their ids and index and returns an integer indicating
 * their relative order.
 *
 * \param[in] lhs Pointer to the first transaction input object.
 * \param[in] rhs Pointer to the second transaction input object.
 *
 * \return A negative value if lhs is less than rhs, zero if they are equal, and a positive value if lhs is greater than rhs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_input_t* input1 = NULL;
 * cardano_transaction_input_t* input2 = NULL;
 *
 * // Assume input1 and input2 are initialized properly
 *
 * int32_t comparison = cardano_transaction_input_compare(input1, input2);
 * if (comparison < 0)
 * {
 *   printf("input1 is less than input2.\n");
 * }
 * else if (comparison == 0)
 * {
 *   printf("input1 is equal to input2.\n");
 * }
 * else
 * {
 *   printf("input1 is greater than input2.\n");
 * }
 *
 * // Clean up
 * cardano_transaction_input_unref(&input1);
 * cardano_transaction_input_unref(&input2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT int32_t cardano_transaction_input_compare(
  const cardano_transaction_input_t* lhs,
  const cardano_transaction_input_t* rhs);

/**
 * \brief Decrements the reference count of a cardano_transaction_input_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_transaction_input_t object
 * by decreasing its reference count. When the reference count reaches zero, the transaction_input is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] transaction_input A pointer to the pointer of the transaction_input object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_input_t* transaction_input = cardano_transaction_input_new(major, minor);
 *
 * // Perform operations with the transaction_input...
 *
 * cardano_transaction_input_unref(&transaction_input);
 * // At this point, transaction_input is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_transaction_input_unref, the pointer to the \ref cardano_transaction_input_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_transaction_input_unref(cardano_transaction_input_t** transaction_input);

/**
 * \brief Increases the reference count of the cardano_transaction_input_t object.
 *
 * This function is used to manually increment the reference count of an cardano_transaction_input_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_transaction_input_unref.
 *
 * \param transaction_input A pointer to the cardano_transaction_input_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming transaction_input is a previously created transaction_input object
 *
 * cardano_transaction_input_ref(transaction_input);
 *
 * // Now transaction_input can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_transaction_input_ref there is a corresponding
 * call to \ref cardano_transaction_input_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_transaction_input_ref(cardano_transaction_input_t* transaction_input);

/**
 * \brief Retrieves the current reference count of the cardano_transaction_input_t object.
 *
 * This function returns the number of active references to an cardano_transaction_input_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_transaction_input_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param transaction_input A pointer to the cardano_transaction_input_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_transaction_input_t object. If the object
 * is properly managed (i.e., every \ref cardano_transaction_input_ref call is matched with a
 * \ref cardano_transaction_input_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming transaction_input is a previously created transaction_input object
 *
 * size_t ref_count = cardano_transaction_input_refcount(transaction_input);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_transaction_input_refcount(const cardano_transaction_input_t* transaction_input);

/**
 * \brief Sets the last error message for a given cardano_transaction_input_t object.
 *
 * Records an error message in the transaction_input's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] transaction_input A pointer to the \ref cardano_transaction_input_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the transaction_input's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_transaction_input_set_last_error(
  cardano_transaction_input_t* transaction_input,
  const char*                  message);

/**
 * \brief Retrieves the last error message recorded for a specific transaction_input.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_transaction_input_set_last_error for the given
 * transaction_input. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] transaction_input A pointer to the \ref cardano_transaction_input_t instance whose last error
 *                   message is to be retrieved. If the transaction_input is NULL, the function
 *                   returns a generic error message indicating the null transaction_input.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified transaction_input. If the transaction_input is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_transaction_input_set_last_error for the same transaction_input, or until
 *       the transaction_input is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_transaction_input_get_last_error(
  const cardano_transaction_input_t* transaction_input);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_INPUT_H