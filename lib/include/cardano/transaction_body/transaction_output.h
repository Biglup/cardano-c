/**
 * \file transaction_output.h
 *
 * \author angel.castillo
 * \date   Sep 06, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_OUTPUT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_OUTPUT_H

/* INCLUDES ******************************************************************/

#include <cardano/address/address.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/datum.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/scripts/script.h>
#include <cardano/transaction_body/value.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A transaction output object includes the address which represents a public key
 * hash or a script hash that can unlock the output, and the funds that are held
 * inside.
 */
typedef struct cardano_transaction_output_t cardano_transaction_output_t;

/**
 * \brief Creates and initializes a new instance of a transaction output.
 *
 * This function allocates and initializes a new instance of a \ref cardano_transaction_output_t object,
 * which represents an output of a transaction in the Cardano network.
 *
 * \param[in] address A pointer to a \ref cardano_address_t object representing the recipient's address.
 * \param[in] amount The amount of ADA in lovelaces that the output will hold.
 * \param[out] transaction_output On successful initialization, this will point to a newly created
 *             \ref cardano_transaction_output_t object. This object represents a "strong reference",
 *             meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the transaction output is no longer needed, the caller must release it
 *             by calling \ref cardano_transaction_output_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the transaction output was successfully created, or an appropriate error code
 *         indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the input address pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_address_t* address = ...; // Assume address is already initialized
 * uint64_t amount = 1000000; // 1 ADA in lovelaces
 *
 * cardano_transaction_output_t* transaction_output = NULL;
 * cardano_error_t result = cardano_transaction_output_new(address, amount, &transaction_output);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the transaction output
 *   // Free resources when done
 *   cardano_transaction_output_unref(&transaction_output);
 * }
 * else
 * {
 *   printf("Failed to create the transaction output: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_address_unref(&address); // Cleanup the address object
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_output_new(
  cardano_address_t*             address,
  uint64_t                       amount,
  cardano_transaction_output_t** transaction_output);

/**
 * \brief Creates a \ref cardano_transaction_output_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_transaction_output_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a transaction_output.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] transaction_output A pointer to a pointer of \ref cardano_transaction_output_t that will be set to the address
 *                        of the newly created transaction_output object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the transaction output were successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_transaction_output_t object by calling
 *       \ref cardano_transaction_output_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_transaction_output_t* transaction_output = NULL;
 *
 * cardano_error_t result = cardano_transaction_output_from_cbor(reader, &transaction_output);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the transaction_output
 *
 *   // Once done, ensure to clean up and release the transaction_output
 *   cardano_transaction_output_unref(&transaction_output);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode transaction_output: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_output_from_cbor(cardano_cbor_reader_t* reader, cardano_transaction_output_t** transaction_output);

/**
 * \brief Serializes transaction output into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_transaction_output_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] transaction_output A constant pointer to the \ref cardano_transaction_output_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p transaction_output or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_output_t* transaction_output = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_transaction_output_to_cbor(transaction_output, writer);
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
 * cardano_transaction_output_unref(&transaction_output);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_transaction_output_to_cbor(
  const cardano_transaction_output_t* transaction_output,
  cardano_cbor_writer_t*              writer);

/**
 * \brief Retrieves the address associated with a transaction output.
 *
 * This function fetches the address from a given \ref cardano_transaction_output_t object. The address indicates
 * the recipient of the funds specified in the output of a transaction.
 *
 * \param[in] output A pointer to an initialized \ref cardano_transaction_output_t object.
 *
 * \return A pointer to the \ref cardano_address_t object representing the address. If the output pointer is NULL, this function returns NULL.
 *         Note that the returned address is a new reference and must be released using \ref cardano_address_unref
 *         when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_output_t* output = ...; // Assume output is already initialized
 * cardano_address_t* address = cardano_transaction_output_get_address(output);
 *
 * if (address != NULL)
 * {
 *   // Process the address
 *   cardano_address_unref(&address);  // Release the address when done
 * }
 * else
 * {
 *   printf("Invalid transaction output or uninitialized output.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_address_t* cardano_transaction_output_get_address(cardano_transaction_output_t* output);

/**
 * \brief Sets the address for a transaction output.
 *
 * This function assigns a new address to a \ref cardano_transaction_output_t object.
 * The address is where the output, holding certain funds or assets, is intended to be sent.
 *
 * \param[in,out] output A pointer to an initialized \ref cardano_transaction_output_t object to which the address will be set.
 * \param[in] address A pointer to an initialized \ref cardano_address_t object representing the address.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the address
 *         was successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * \note This function increases the reference count of the address object; therefore, the caller retains ownership of their respective references.
 *       It is the caller's responsibility to release their reference to the address when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_output_t* output = ...; // Assume output is already initialized
 * cardano_address_t* address = ...; // Assume address is initialized
 *
 * cardano_error_t result = cardano_transaction_output_set_address(output, address);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The address is now set for the output
 * }
 * else
 * {
 *   printf("Failed to set the address.\n");
 * }
 *
 * // Clean up resources
 * cardano_address_unref(&address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_output_set_address(cardano_transaction_output_t* output, cardano_address_t* address);

/**
 * \brief Retrieves the value held by a transaction output.
 *
 * This function fetches the value contained in the transaction output.
 *
 * \param[in] output A constant pointer to an initialized \ref cardano_transaction_output_t object.
 *
 * \return The value contained in the transaction output. If the output pointer is NULL, the function will return NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_transaction_output_t* output = ...; // Assume output is already initialized
 *
 * cardano_value_t* value = cardano_transaction_output_get_value(output);
 *
 * if (value != NULL)
 * {
 *    // Process the value
 *    cardano_value_unref(&amount); // Release the value when done
 *  }
 *  else
 *  {
 *    printf("Invalid transaction output or uninitialized output.\n");
 *  }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_value_t* cardano_transaction_output_get_value(cardano_transaction_output_t* output);

/**
 * \brief Sets the value for a transaction output.
 *
 * This function assigns a specified value to a \ref cardano_transaction_output_t object.
 *
 * \param[in,out] output A pointer to an initialized \ref cardano_transaction_output_t object to which the amount will be set.
 * \param[in] value The value to set for the transaction output.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the amount was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the output pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_output_t* output = ...; // Assume output is already initialized
 * cardano_value_t* value = {...};
 *
 * cardano_error_t result = cardano_transaction_output_set_amount(output, value);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Amount set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set the amount.\n");
 * }
 *
 * // Release the value when done
 * cardano_value_unref(&value);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_output_set_value(cardano_transaction_output_t* output, cardano_value_t* value);

/**
 * \brief Retrieves the datum associated with a transaction output.
 *
 * This function fetches the datum from a given \ref cardano_transaction_output_t object. A datum is optional state data
 * associated with a transaction output that can be utilized by Plutus scripts to dictate transaction validity based on
 * script logic. This function is used primarily in the context of transactions involving smart contracts on the Cardano network.
 *
 * \param[in] output A pointer to an initialized \ref cardano_transaction_output_t object from which the datum is retrieved.
 *
 * \return A pointer to the \ref cardano_datum_t object representing the datum, or NULL if no datum is associated with the transaction output.
 *         Note that the returned datum is a new reference and must be released using \ref cardano_datum_unref
 *         when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_output_t* output = ...; // Assume output is already initialized
 * cardano_datum_t* datum = cardano_transaction_output_get_datum(output);
 *
 * if (datum != NULL)
 * {
 *   // Process the datum
 *   cardano_datum_unref(&datum);
 * }
 * else
 * {
 *   printf("No datum associated with this output.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_datum_t* cardano_transaction_output_get_datum(cardano_transaction_output_t* output);

/**
 * \brief Sets the datum for a transaction output.
 *
 * This function assigns a datum to a \ref cardano_transaction_output_t object. A datum is a piece of state data
 * that can be used by Plutus scripts to influence the behavior of smart contracts on the Cardano network.
 * The datum is optional and can be set to NULL to indicate that no datum is associated with the transaction output.
 *
 * \param[in,out] output A pointer to an initialized \ref cardano_transaction_output_t object to which the datum will be set.
 * \param[in] datum A pointer to an initialized \ref cardano_datum_t object representing the datum to be associated with the output.
 *                  This parameter can be NULL if the datum is to be removed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the datum was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the output pointer is NULL.
 *
 * \note This function increases the reference count of the datum object; therefore, the caller retains ownership of their respective references.
 *       It is the caller's responsibility to release their reference to the datum when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_output_t* output = ...; // Assume output is already initialized
 * cardano_datum_t* datum = cardano_datum_new(...); // Assume datum is initialized, or NULL to unset
 *
 * cardano_error_t result = cardano_transaction_output_set_datum(output, datum);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Datum set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set the datum.\n");
 * }
 *
 * // If datum is not NULL, release it after setting
 * if (datum)
 * {
 *   cardano_datum_unref(&datum);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_output_set_datum(cardano_transaction_output_t* output, cardano_datum_t* datum);

/**
 * \brief Retrieves the script reference from a transaction output.
 *
 * This function fetches the script reference from a specified \ref cardano_transaction_output_t object.
 * A script reference is a mechanism in Cardano that allows transactions to refer to scripts included in other outputs.
 * By using script references, a transaction can fulfill script execution requirements without directly including the script,
 * thereby reducing transaction size and simplifying script management.
 *
 * \param[in] output A pointer to an initialized \ref cardano_transaction_output_t object from which the script reference is to be retrieved.
 *
 * \return A pointer to the \ref cardano_script_t object representing the script reference. If the output does not contain a script reference,
 *         this function returns NULL. Note that the returned script reference is a new reference and must be released using
 *         \ref cardano_script_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_output_t* output = ...; // Assume output is already initialized
 * cardano_script_t* script_ref = cardano_transaction_output_get_script_ref(output);
 *
 * if (script_ref != NULL)
 * {
 *   // Process the script reference
 *   cardano_script_unref(&script_ref);
 * }
 * else
 * {
 *   printf("No script reference found in this output.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_script_t* cardano_transaction_output_get_script_ref(cardano_transaction_output_t* output);

/**
 * \brief Compares two transaction output objects for equality.
 *
 * This function compares two transaction output objects to determine if they are equal.
 *
 * \param[in] lhs Pointer to the first transaction output object.
 * \param[in] rhs Pointer to the second transaction output object.
 *
 * \return \c true if the transaction_output objects are equal, \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_output_t* transaction_output1 = NULL;
 * cardano_transaction_output_t* transaction_output2 = NULL;
 *
 * // Assume transaction_output1 and transaction_output2 are initialized properly
 *
 * bool are_equal = cardano_transaction_output_equals(transaction_output1, transaction_output2);
 *
 * if (are_equal)
 * {
 *   printf("The transaction_output objects are equal.\n");
 * }
 * else
 * {
 *   printf("The transaction_output objects are not equal.\n");
 * }
 *
 * // Clean up
 * cardano_transaction_output_unref(&transaction_output1);
 * cardano_transaction_output_unref(&transaction_output2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_transaction_output_equals(const cardano_transaction_output_t* lhs, const cardano_transaction_output_t* rhs);

/**
 * \brief Sets the script reference for a transaction output.
 *
 * This function assigns a script reference to a specified \ref cardano_transaction_output_t object.
 * A script reference allows a transaction output to indirectly satisfy script execution requirements by referencing
 * a script present in another output. This capability can lead to more efficient transaction sizes and simplified
 * script management by reusing existing scripts.
 *
 * \param[in,out] output A pointer to an initialized \ref cardano_transaction_output_t object to which the script reference will be set.
 * \param[in] script_ref A pointer to an initialized \ref cardano_script_t object representing the script reference. This parameter
 *                       can be NULL if the intention is to remove an existing script reference from the output.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the script reference
 *         was successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the output pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_output_t* output = ...; // Assume output is already initialized
 * cardano_script_t* script_ref = ...; // Assume script_ref is initialized or NULL to remove the reference
 *
 * cardano_error_t result = cardano_transaction_output_set_script_ref(output, script_ref);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Script reference set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set the script reference.\n");
 * }
 *
 * // Note: Cleanup the script reference if it was newly created and is no longer needed
 * if (script_ref)
 * {
 *   cardano_script_unref(&script_ref);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_transaction_output_set_script_ref(cardano_transaction_output_t* output, cardano_script_t* script_ref);

/**
 * \brief Decrements the reference count of a cardano_transaction_output_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_transaction_output_t object
 * by decreasing its reference count. When the reference count reaches zero, the transaction_output is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] transaction_output A pointer to the pointer of the transaction_output object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_output_t* transaction_output = cardano_transaction_output_new(major, minor);
 *
 * // Perform operations with the transaction_output...
 *
 * cardano_transaction_output_unref(&transaction_output);
 * // At this point, transaction_output is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_transaction_output_unref, the pointer to the \ref cardano_transaction_output_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_transaction_output_unref(cardano_transaction_output_t** transaction_output);

/**
 * \brief Increases the reference count of the cardano_transaction_output_t object.
 *
 * This function is used to manually increment the reference count of an cardano_transaction_output_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_transaction_output_unref.
 *
 * \param transaction_output A pointer to the cardano_transaction_output_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming transaction_output is a previously created transaction_output object
 *
 * cardano_transaction_output_ref(transaction_output);
 *
 * // Now transaction_output can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_transaction_output_ref there is a corresponding
 * call to \ref cardano_transaction_output_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_transaction_output_ref(cardano_transaction_output_t* transaction_output);

/**
 * \brief Retrieves the current reference count of the cardano_transaction_output_t object.
 *
 * This function returns the number of active references to an cardano_transaction_output_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_transaction_output_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param transaction_output A pointer to the cardano_transaction_output_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_transaction_output_t object. If the object
 * is properly managed (i.e., every \ref cardano_transaction_output_ref call is matched with a
 * \ref cardano_transaction_output_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming transaction_output is a previously created transaction_output object
 *
 * size_t ref_count = cardano_transaction_output_refcount(transaction_output);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_transaction_output_refcount(const cardano_transaction_output_t* transaction_output);

/**
 * \brief Sets the last error message for a given cardano_transaction_output_t object.
 *
 * Records an error message in the transaction_output's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] transaction_output A pointer to the \ref cardano_transaction_output_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the transaction_output's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_transaction_output_set_last_error(
  cardano_transaction_output_t* transaction_output,
  const char*                   message);

/**
 * \brief Retrieves the last error message recorded for a specific transaction_output.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_transaction_output_set_last_error for the given
 * transaction_output. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] transaction_output A pointer to the \ref cardano_transaction_output_t instance whose last error
 *                   message is to be retrieved. If the transaction_output is NULL, the function
 *                   returns a generic error message indicating the null transaction_output.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified transaction_output. If the transaction_output is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_transaction_output_set_last_error for the same transaction_output, or until
 *       the transaction_output is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_transaction_output_get_last_error(
  const cardano_transaction_output_t* transaction_output);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_TRANSACTION_OUTPUT_H