/**
 * \file utxo.h
 *
 * \author angel.castillo
 * \date   Sep 23, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UTXO_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UTXO_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/transaction_body/transaction_input.h>
#include <cardano/transaction_body/transaction_output.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a unspent output (UTxO).
 */
typedef struct cardano_utxo_t cardano_utxo_t;

/**
 * \brief Creates a new UTXO (Unspent Transaction Output) object.
 *
 * This function creates and initializes a new \ref cardano_utxo_t object by associating a
 * transaction input with its corresponding transaction output. UTXOs represent unspent
 * outputs from previous transactions that can be used as inputs in new transactions.
 *
 * \param[in] input A pointer to an initialized \ref cardano_transaction_input_t object,
 *                  representing the transaction input. This parameter must not be NULL.
 * \param[in] output A pointer to an initialized \ref cardano_transaction_output_t object,
 *                   representing the corresponding transaction output. This parameter must not be NULL.
 * \param[out] utxo A double pointer to a \ref cardano_utxo_t object. On successful creation,
 *                  this will point to the newly created UTXO object. The caller is responsible
 *                  for managing the lifecycle of this object and must release it by calling
 *                  \ref cardano_utxo_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the UTXO was successfully created. If either \p input, \p output, or \p utxo is NULL,
 *         returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * \note A UTXO links a specific input to its corresponding output, representing spendable value
 *       in the Cardano blockchain. The newly created UTXO object must be properly freed by the
 *       caller using \ref cardano_utxo_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_input_t* input = ...;  // Assume input is initialized
 * cardano_transaction_output_t* output = ...;  // Assume output is initialized
 * cardano_utxo_t* utxo = NULL;
 *
 * cardano_error_t result = cardano_utxo_new(input, output, &utxo);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // UTXO successfully created, use utxo as needed
 *
 *   // Clean up when done
 *   cardano_utxo_unref(&utxo);
 * }
 * else
 * {
 *   printf("Failed to create UTXO: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up input and output when no longer needed
 * cardano_transaction_input_unref(&input);
 * cardano_transaction_output_unref(&output);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_utxo_new(
  cardano_transaction_input_t*  input,
  cardano_transaction_output_t* output,
  cardano_utxo_t**              utxo);

/**
 * \brief Creates a \ref cardano_utxo_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_utxo_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a utxo.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] utxo A pointer to a pointer of \ref cardano_utxo_t that will be set to the address
 *                        of the newly created utxo object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the utxo were successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_utxo_t object by calling
 *       \ref cardano_utxo_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_utxo_t* utxo = NULL;
 *
 * cardano_error_t result = cardano_utxo_from_cbor(reader, &utxo);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the utxo
 *
 *   // Once done, ensure to clean up and release the utxo
 *   cardano_utxo_unref(&utxo);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode utxo: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_utxo_from_cbor(cardano_cbor_reader_t* reader, cardano_utxo_t** utxo);

/**
 * \brief Serializes utxo into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_utxo_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] utxo A constant pointer to the \ref cardano_utxo_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p utxo or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_utxo_t* utxo = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_utxo_to_cbor(utxo, writer);
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
 * cardano_utxo_unref(&utxo);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_utxo_to_cbor(
  const cardano_utxo_t*  utxo,
  cardano_cbor_writer_t* writer);

/**
 * \brief Retrieves the transaction input from a UTXO.
 *
 * This function fetches the transaction input associated with a given \ref cardano_utxo_t object.
 * The transaction input represents the source of the UTXO, which can be used as an input in a new transaction.
 *
 * \param[in] utxo A pointer to an initialized \ref cardano_utxo_t object. This parameter must not be NULL.
 *
 * \return A pointer to a \ref cardano_transaction_input_t object representing the transaction input of the UTXO.
 *         The returned object is a new reference, and the caller is responsible for managing its lifecycle.
 *         Specifically, the caller must release the object by calling \ref cardano_transaction_input_unref
 *         when it is no longer needed. If the \p utxo is NULL, the function returns NULL.
 *
 * \note The transaction input refers to the previous transaction from which the UTXO originates,
 *       representing a spendable output.
 *
 * Usage Example:
 * \code{.c}
 * cardano_utxo_t* utxo = ...; // Assume utxo is already initialized
 * cardano_transaction_input_t* input = cardano_utxo_get_input(utxo);
 *
 * if (input != NULL)
 * {
 *   // Use the transaction input as needed
 *
 *   // Clean up when done
 *   cardano_transaction_input_unref(&input);
 * }
 * else
 * {
 *   printf("Failed to retrieve the transaction input.\n");
 * }
 *
 * // Clean up the utxo when no longer needed
 * cardano_utxo_unref(&utxo);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_transaction_input_t* cardano_utxo_get_input(cardano_utxo_t* utxo);

/**
 * \brief Sets the transaction input for a UTXO.
 *
 * This function assigns a transaction input to the specified \ref cardano_utxo_t object. The transaction input
 * represents the source of the UTXO, which can be used as an input in a new transaction.
 *
 * \param[in,out] utxo A pointer to an initialized \ref cardano_utxo_t object where the input will be set.
 *                     This parameter must not be NULL.
 * \param[in] input A pointer to an initialized \ref cardano_transaction_input_t object representing the input
 *                  to be associated with the UTXO. This parameter must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the transaction input was successfully set. If either \p utxo or \p input is NULL, returns
 *         \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * \note This function increases the reference count of the \p input object, meaning the caller retains
 *       ownership of the input and must release it by calling \ref cardano_transaction_input_unref
 *       when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_utxo_t* utxo = ...; // Assume utxo is initialized
 * cardano_transaction_input_t* input = ...; // Assume input is initialized
 *
 * cardano_error_t result = cardano_utxo_set_input(utxo, input);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Transaction input successfully set for the UTXO.\n");
 * }
 * else
 * {
 *   printf("Failed to set transaction input: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up resources
 * cardano_transaction_input_unref(&input);
 * cardano_utxo_unref(&utxo);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_utxo_set_input(cardano_utxo_t* utxo, cardano_transaction_input_t* input);

/**
 * \brief Retrieves the transaction output from a UTXO.
 *
 * This function fetches the transaction output associated with a given \ref cardano_utxo_t object.
 * The transaction output represents the destination of the UTXO, indicating the amount of value and recipient.
 *
 * \param[in] utxo A pointer to an initialized \ref cardano_utxo_t object. This parameter must not be NULL.
 *
 * \return A pointer to a \ref cardano_transaction_output_t object representing the transaction output of the UTXO.
 *         The returned object is a new reference, and the caller is responsible for managing its lifecycle.
 *         Specifically, the caller must release the object by calling \ref cardano_transaction_output_unref
 *         when it is no longer needed. If the \p utxo is NULL, the function returns NULL.
 *
 * \note The transaction output refers to the recipient and the amount of value in the UTXO, representing spendable value.
 *
 * Usage Example:
 * \code{.c}
 * cardano_utxo_t* utxo = ...; // Assume utxo is already initialized
 * cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
 *
 * if (output != NULL)
 * {
 *   // Use the transaction output as needed
 *
 *   // Clean up when done
 *   cardano_transaction_output_unref(&output);
 * }
 * else
 * {
 *   printf("Failed to retrieve the transaction output.\n");
 * }
 *
 * // Clean up the utxo when no longer needed
 * cardano_utxo_unref(&utxo);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_transaction_output_t* cardano_utxo_get_output(cardano_utxo_t* utxo);

/**
 * \brief Sets the transaction output for a UTXO.
 *
 * This function assigns a transaction output to the specified \ref cardano_utxo_t object.
 * The transaction output represents the destination of the UTXO, including the amount of value
 * and the recipient's address.
 *
 * \param[in,out] utxo A pointer to an initialized \ref cardano_utxo_t object where the output will be set.
 *                     This parameter must not be NULL.
 * \param[in] output A pointer to an initialized \ref cardano_transaction_output_t object representing the output
 *                   to be associated with the UTXO. This parameter must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the transaction output was successfully set. If either \p utxo or \p output is NULL,
 *         returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * \note This function increases the reference count of the \p output object, meaning the caller retains
 *       ownership of the output and must release it by calling \ref cardano_transaction_output_unref
 *       when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_utxo_t* utxo = ...; // Assume utxo is initialized
 * cardano_transaction_output_t* output = ...; // Assume output is initialized
 *
 * cardano_error_t result = cardano_utxo_set_output(utxo, output);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Transaction output successfully set for the UTXO.\n");
 * }
 * else
 * {
 *   printf("Failed to set transaction output: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up resources
 * cardano_transaction_output_unref(&output);
 * cardano_utxo_unref(&utxo);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_utxo_set_output(cardano_utxo_t* utxo, cardano_transaction_output_t* output);

/**
 * \brief Compares two UTxO objects for equality.
 *
 * This function compares two UTxO objects to determine if they are equal.
 *
 * \param[in] lhs Pointer to the first UTxO object.
 * \param[in] rhs Pointer to the second UTxO object.
 *
 * \return \c true if the utxo objects are equal, \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_utxo_t* utxo1 = NULL;
 * cardano_utxo_t* utxo2 = NULL;
 *
 * // Assume utxo1 and utxo2 are initialized properly
 *
 * bool are_equal = cardano_utxo_equals(utxo1, utxo2);
 *
 * if (are_equal)
 * {
 *   printf("The utxo objects are equal.\n");
 * }
 * else
 * {
 *   printf("The utxo objects are not equal.\n");
 * }
 *
 * // Clean up
 * cardano_utxo_unref(&utxo1);
 * cardano_utxo_unref(&utxo2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_utxo_equals(const cardano_utxo_t* lhs, const cardano_utxo_t* rhs);

/**
 * \brief Decrements the reference count of a cardano_utxo_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_utxo_t object
 * by decreasing its reference count. When the reference count reaches zero, the utxo is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] utxo A pointer to the pointer of the utxo object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_utxo_t* utxo = cardano_utxo_new(major, minor);
 *
 * // Perform operations with the utxo...
 *
 * cardano_utxo_unref(&utxo);
 * // At this point, utxo is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_utxo_unref, the pointer to the \ref cardano_utxo_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_utxo_unref(cardano_utxo_t** utxo);

/**
 * \brief Increases the reference count of the cardano_utxo_t object.
 *
 * This function is used to manually increment the reference count of an cardano_utxo_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_utxo_unref.
 *
 * \param utxo A pointer to the cardano_utxo_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming utxo is a previously created utxo object
 *
 * cardano_utxo_ref(utxo);
 *
 * // Now utxo can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_utxo_ref there is a corresponding
 * call to \ref cardano_utxo_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_utxo_ref(cardano_utxo_t* utxo);

/**
 * \brief Retrieves the current reference count of the cardano_utxo_t object.
 *
 * This function returns the number of active references to an cardano_utxo_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_utxo_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param utxo A pointer to the cardano_utxo_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_utxo_t object. If the object
 * is properly managed (i.e., every \ref cardano_utxo_ref call is matched with a
 * \ref cardano_utxo_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming utxo is a previously created utxo object
 *
 * size_t ref_count = cardano_utxo_refcount(utxo);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_utxo_refcount(const cardano_utxo_t* utxo);

/**
 * \brief Sets the last error message for a given cardano_utxo_t object.
 *
 * Records an error message in the utxo's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] utxo A pointer to the \ref cardano_utxo_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the utxo's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_utxo_set_last_error(
  cardano_utxo_t* utxo,
  const char*     message);

/**
 * \brief Retrieves the last error message recorded for a specific utxo.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_utxo_set_last_error for the given
 * utxo. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] utxo A pointer to the \ref cardano_utxo_t instance whose last error
 *                   message is to be retrieved. If the utxo is NULL, the function
 *                   returns a generic error message indicating the null utxo.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified utxo. If the utxo is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_utxo_set_last_error for the same utxo, or until
 *       the utxo is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_utxo_get_last_error(
  const cardano_utxo_t* utxo);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_UTXO_H