/**
 * \file constr_plutus_data.h
 *
 * \author angel.castillo
 * \date   Apr 12, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CONSTR_PLUTUS_DATA_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CONSTR_PLUTUS_DATA_H

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
 * \brief Represents a list of plutus data.
 */
typedef struct cardano_plutus_list_t cardano_plutus_list_t;

/**
 * \brief The main datatype `Constr` represents the nth constructor
 * along with its arguments.
 *
 * Remark: We don't directly serialize the alternative in the tag,
 * instead the scheme is:
 *
 * - Alternatives 0-6 -> tags 121-127, followed by the arguments in a list.
 * - Alternatives 7-127 -> tags 1280-1400, followed by the arguments in a list.
 * - Any alternatives, including those that don't fit in the above -> tag 102 followed by a list containing
 * an unsigned integer for the actual alternative, and then the arguments in a (nested!) list.
 */
typedef struct cardano_constr_plutus_data_t cardano_constr_plutus_data_t;

/**
 * \brief Creates and initializes a new instance of a plutus data constructor.
 *
 * This function allocates and initializes a new instance of \ref cardano_constr_plutus_data_t,
 * using the provided Constr alternative number and list of arguments as a PlutusList. It returns an error code
 * to indicate success or failure of the operation.
 *
 * \param[in] alternative The Constr alternative number, representing the nth constructor
 *             of a 'Sum Type'.
 * \param[in] data A pointer to a \ref cardano_plutus_list_t object representing the list of arguments
 *             of the 'Sum Type' as a 'PlutusList'.
 * \param[out] constr_plutus_data On successful initialization, this will point to a newly created
 *             \ref cardano_constr_plutus_data_t object. This object represents a "strong reference"
 *             to the constr_plutus_data, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the constr_plutus_data is no longer needed, the caller must release it
 *             by calling \ref cardano_constr_plutus_data_unref.
 *
 * \return \ref CARDANO_SUCCESS if the constr_plutus_data was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * uint64_t alternative = 1; // Example alternative number
 * cardano_plutus_list_t* data = { ... }; // Assume data is initialized here
 * cardano_constr_plutus_data_t* constr_plutus_data = NULL;
 *
 * // Attempt to create a new constr_plutus_data
 * cardano_error_t result = cardano_constr_plutus_data_new(alternative, data, &constr_plutus_data);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the constr_plutus_data
 *
 *   // Once done, ensure to clean up and release the constr_plutus_data
 *   cardano_constr_plutus_data_unref(&constr_plutus_data);
 * }
 *
 * cardano_plutus_list_unref(&data);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_constr_plutus_data_new(
  uint64_t                       alternative,
  cardano_plutus_list_t*         data,
  cardano_constr_plutus_data_t** constr_plutus_data);

/**
 * \brief Creates a \ref cardano_cbor_reader_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_constr_plutus_data_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a constr_plutus_data.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded constr_plutus_data data.
 * \param[out] constr_plutus_data A pointer to a pointer of \ref cardano_constr_plutus_data_t that will be set to the address
 *                        of the newly created constr_plutus_data object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the constr_plutus_data was successfully created, or an appropriate error code if an error occurred.
 *
 * \remark In Cardano, entities are encoded in CBOR, but CBOR allows multiple valid ways to encode the same data. The Cardano blockchain
 *         does not enforce a canonical CBOR representation, meaning that if you decode a transaction from CBOR and then re-encode it,
 *         the resulting encoding could be different. This would change the data and invalidate any existing signatures.
 *         To prevent this, when a plutus data object is created using \ref cardano_constr_plutus_data_from_cbor, it caches the original
 *         CBOR representation internally. When \ref cardano_constr_plutus_data_to_cbor is called, it will output the cached CBOR.
 *         If the cached CBOR representation is not needed, the client can call \ref cardano_constr_plutus_data_clear_cbor_cache after the object has been created.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_constr_plutus_data_t object by calling
 *       \ref cardano_constr_plutus_data_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_constr_plutus_data_t* constr_plutus_data = NULL;
 *
 * cardano_error_t result = cardano_constr_plutus_data_from_cbor(reader, &constr_plutus_data);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the constr_plutus_data
 *
 *   // Once done, ensure to clean up and release the constr_plutus_data
 *   cardano_constr_plutus_data_unref(&constr_plutus_data);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode constr_plutus_data: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_constr_plutus_data_from_cbor(cardano_cbor_reader_t* reader, cardano_constr_plutus_data_t** constr_plutus_data);

/**
 * \brief Serializes a \ref cardano_cbor_reader_t into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_constr_plutus_data_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] constr_plutus_data A constant pointer to the \ref cardano_constr_plutus_data_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p constr_plutus_data or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * \remark In Cardano, entities are encoded in CBOR, but CBOR allows multiple valid ways to encode the same data. The Cardano blockchain
 *         does not enforce a canonical CBOR representation, meaning that if you decode a transaction from CBOR and then re-encode it,
 *         the resulting encoding could be different. This would change the data and invalidate any existing signatures.
 *         To prevent this, when a plutus data object is created using \ref cardano_constr_plutus_data_from_cbor, it caches the original
 *         CBOR representation internally. When \ref cardano_constr_plutus_data_to_cbor is called, it will output the cached CBOR.
 *         If the cached CBOR representation is not needed, the client can call \ref cardano_constr_plutus_data_clear_cbor_cache after the object has been created.
 *
 * Usage Example:
 * \code{.c}
 * cardano_constr_plutus_data_t* constr_plutus_data = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_constr_plutus_data_to_cbor(constr_plutus_data, writer);
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
 * cardano_constr_plutus_data_unref(&constr_plutus_data);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_constr_plutus_data_to_cbor(
  const cardano_constr_plutus_data_t* constr_plutus_data,
  cardano_cbor_writer_t*              writer);

/**
 * \brief Retrieves the data associated with a \ref cardano_cbor_reader_t.
 *
 * This function provides access to the data part of a \ref cardano_constr_plutus_data_t object.
 * It returns a new reference to a \ref cardano_plutus_list_t object representing the data.
 * This allows the data to be used independently of the original \ref cardano_cbor_reader_t object. The
 * reference count of the data object is increased by one, making it the caller's responsibility
 * to release it by calling \ref cardano_plutus_list_unref when it is no longer needed.
 *
 * \param[in] constr_plutus_data A constant pointer to the \ref cardano_constr_plutus_data_t object from which
 *                   the data is to be retrieved.
 * \param[out] data A pointer to store the reference to the \ref cardano_plutus_list_t object containing the data.
 *                  If the input constr_plutus_data is NULL, returns NULL. The caller is responsible for
 *                  managing the lifecycle of this object, including releasing it with
 *                  \ref cardano_plutus_list_unref.
 *
 * \return \ref CARDANO_SUCCESS if the data was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_constr_plutus_data_t* original_constr_plutus_data = cardano_constr_plutus_data_new(...);
 * cardano_plutus_list_t* data_constr_plutus_data = NULL;
 *
 * // Attempt to retrieve the data associated with the constr_plutus_data
 * cardano_error_t result = cardano_constr_plutus_data_get_data(original_constr_plutus_data, &data_constr_plutus_data);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the data_constr_plutus_data
 *
 *   // Once done, ensure to clean up and release the data_constr_plutus_data
 *   cardano_plutus_list_unref(&data_constr_plutus_data);
 * }
 *
 * // Release the original constr_plutus_data after use
 * cardano_constr_plutus_data_unref(&original_constr_plutus_data);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_constr_plutus_data_get_data(cardano_constr_plutus_data_t* constr_plutus_data, cardano_plutus_list_t** data);

/**
 * \brief Sets the data associated with a \ref cardano_constr_plutus_data_t.
 *
 * This function sets the data part of a \ref cardano_constr_plutus_data_t object to the provided
 * \ref cardano_plutus_list_t object. It replaces any existing data associated with the \ref cardano_constr_plutus_data_t.
 * The reference count of the provided data object is increased by one, making it the caller's responsibility
 * to release it when it is no longer needed.
 *
 * \param[in] constr_plutus_data A constant pointer to the \ref cardano_constr_plutus_data_t object to which
 *                   the data is to be set.
 * \param[in] data A pointer to the \ref cardano_plutus_list_t object containing the new data to be set.
 *
 * \return \ref CARDANO_SUCCESS if the data was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_constr_plutus_data_t* original_constr_plutus_data = cardano_constr_plutus_data_new(...);
 * cardano_plutus_list_t* new_data = { ... }; // Assume new_data is initialized here
 *
 * // Attempt to set the new data associated with the constr_plutus_data
 * cardano_error_t result = cardano_constr_plutus_data_set_data(original_constr_plutus_data, new_data);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Data successfully set
 *   printf("Data set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   fprintf(stderr, "Failed to set data: %s.\n", cardano_error_to_string(result));
 * }
 *
 * // Release the original constr_plutus_data after use
 * cardano_constr_plutus_data_unref(&original_constr_plutus_data);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_constr_plutus_data_set_data(cardano_constr_plutus_data_t* constr_plutus_data, cardano_plutus_list_t* data);

/**
 * \brief Retrieves the alternative of the \ref cardano_constr_plutus_data_t.
 *
 * This function retrieves the alternative of a given \ref cardano_constr_plutus_data_t object and stores it in the provided
 * output parameter. The alternative represents the nth constructor of a 'Sum Type'.
 *
 * \param[in] constr_plutus_data A constant pointer to the \ref cardano_constr_plutus_data_t object from which
 *                       the alternative is to be retrieved. The object must not be NULL.
 * \param[out] alternative Pointer to a variable where the alternative will be stored.
 *                         This variable will be set to the alternative number.
 *
 * \return \ref CARDANO_SUCCESS if the alternative was successfully retrieved, or an appropriate error code if the
 *         input is NULL or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_constr_plutus_data_t* constr_plutus_data = cardano_constr_plutus_data_new(...);
 * uint64_t alternative;
 * cardano_error_t result = cardano_constr_plutus_data_get_alternative(constr_plutus_data, &alternative);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the alternative number
 *   printf("Alternative number: %llu\n", (unsigned long long)alternative);
 * }
 * else
 * {
 *   // Handle error
 *   fprintf(stderr, "Failed to retrieve alternative: %s.\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up the constr_plutus_data object once done
 * cardano_constr_plutus_data_unref(&constr_plutus_data);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_constr_plutus_data_get_alternative(const cardano_constr_plutus_data_t* constr_plutus_data, uint64_t* alternative);

/**
 * \brief Sets the alternative of the \ref cardano_constr_plutus_data_t.
 *
 * This function sets the alternative of a given \ref cardano_constr_plutus_data_t object to the provided
 * alternative number.
 *
 * \param[in] constr_plutus_data A constant pointer to the \ref cardano_constr_plutus_data_t object to which
 *                               the alternative is to be set.
 * \param[in] alternative The alternative number to set.
 *
 * \return \ref CARDANO_SUCCESS if the alternative was successfully set, or an appropriate error code if the
 *         input is NULL or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_constr_plutus_data_t* constr_plutus_data = cardano_constr_plutus_data_new(...);
 * uint64_t new_alternative = 2; // Example alternative number
 *
 * cardano_error_t result = cardano_constr_plutus_data_set_alternative(constr_plutus_data, new_alternative);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Alternative successfully set
 *   printf("Alternative set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   fprintf(stderr, "Failed to set alternative: %s.\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up the constr_plutus_data object once done
 * cardano_constr_plutus_data_unref(&constr_plutus_data);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_constr_plutus_data_set_alternative(cardano_constr_plutus_data_t* constr_plutus_data, uint64_t alternative);

/**
 * \brief Checks whether two \ref cardano_constr_plutus_data_t objects are equal.
 *
 * This function compares two \ref cardano_constr_plutus_data_t objects, \p lhs and \p rhs,
 * to determine if they represent the same data. It returns true if the contents of the two objects
 * are equal, and false otherwise.
 *
 * \param[in] lhs A constant pointer to the first \ref cardano_constr_plutus_data_t object to compare.
 * \param[in] rhs A constant pointer to the second \ref cardano_constr_plutus_data_t object to compare.
 *
 * \return True if the contents of the two constr_plutus_data objects are equal, false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_constr_plutus_data_t* data1 = cardano_constr_plutus_data_new(...);
 * cardano_constr_plutus_data_t* data2 = cardano_constr_plutus_data_new(...);
 *
 * bool are_equal = cardano_constr_plutus_equals(data1, data2);
 *
 * if (are_equal)
 * {
 *   // The two constr_plutus_data objects are equal
 *   printf("The two constr_plutus_data objects are equal.\n");
 * }
 * else
 * {
 *   // The two constr_plutus_data objects are not equal
 *   printf("The two constr_plutus_data objects are not equal.\n");
 * }
 *
 * // Release the constr_plutus_data objects after use
 * cardano_constr_plutus_data_unref(&data1);
 * cardano_constr_plutus_data_unref(&data2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_constr_plutus_equals(const cardano_constr_plutus_data_t* lhs, const cardano_constr_plutus_data_t* rhs);

/**
 * \brief Clears the cached CBOR representation from a constr plutus data.
 *
 * This function removes the internally cached CBOR data from a \ref cardano_constr_plutus_data_t object.
 * It is useful when you have modified the constr_plutus_data after it was created from CBOR using
 * \ref cardano_constr_plutus_data_from_cbor and you want to ensure that the next serialization reflects
 * the current state of the constr_plutus_data, rather than using the original cached CBOR.
 *
 * \param[in,out] constr_plutus_data A pointer to an initialized \ref cardano_constr_plutus_data_t object
 *                         from which the CBOR cache will be cleared.
 *
 * \warning Clearing the CBOR cache may change the binary representation of the constr plutus data when
 *          serialized, which can alter the constr plutus data and invalidate any existing signatures.
 *          Use this function with caution, especially if the transaction has already been signed or
 *          if preserving the exact CBOR encoding is important for your application.
 *
 * Usage Example:
 * \code{.c}
 * // Assume constr_plutus_data was created using cardano_constr_plutus_data_from_cbor
 * cardano_constr_plutus_data_t* constr_plutus_data = ...;
 *
 * // Modify the constr_plutus_data as needed
 * // For example, change the fee

 * if (result != CARDANO_SUCCESS)
 * {
 *   printf("Failed to set new fee: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clear the CBOR cache to ensure serialization uses the updated constr_plutus_data
 * cardano_constr_plutus_data_clear_cbor_cache(constr_plutus_data);
 *
 * // Serialize the constr_plutus_data to CBOR
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * result = cardano_constr_plutus_data_to_cbor(constr_plutus_data, writer);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Process the CBOR data as needed
 * }
 * else
 * {
 *   const char* error_message = cardano_cbor_writer_get_last_error(writer);
 *   printf("Serialization failed: %s\n", error_message);
 * }
 *
 * // Clean up resources
 * cardano_cbor_writer_unref(&writer);
 * cardano_constr_plutus_data_unref(&constr_plutus_data);
 * \endcode
 */
CARDANO_EXPORT void cardano_constr_plutus_data_clear_cbor_cache(cardano_constr_plutus_data_t* constr_plutus_data);

/**
 * \brief Decrements the reference count of a \ref cardano_constr_plutus_data_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_constr_plutus_data_t object
 * by decreasing its reference count. When the reference count reaches zero, the constr_plutus_data is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] constr_plutus_data A pointer to the pointer of the constr_plutus_data object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_constr_plutus_data_t* constr_plutus_data = ...; // Assume constr_plutus_data is initialized here
 *
 * // Perform operations with the constr_plutus_data...
 *
 * cardano_constr_plutus_data_unref(&constr_plutus_data);
 * // At this point, constr_plutus_data is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_constr_plutus_data_unref, the pointer to the \ref cardano_constr_plutus_data_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_constr_plutus_data_unref(cardano_constr_plutus_data_t** constr_plutus_data);

/**
 * \brief Increases the reference count of the \ref cardano_constr_plutus_data_t object.
 *
 * This function is used to manually increment the reference count of a \ref cardano_constr_plutus_data_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_constr_plutus_data_unref.
 *
 * \param constr_plutus_data A pointer to the constr_plutus_data object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming constr_plutus_data is a previously created constr_plutus_data object
 *
 * cardano_constr_plutus_data_ref(constr_plutus_data);
 *
 * // Now constr_plutus_data can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_constr_plutus_data_ref there is a corresponding
 * call to \ref cardano_constr_plutus_data_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_constr_plutus_data_ref(cardano_constr_plutus_data_t* constr_plutus_data);

/**
 * \brief Retrieves the current reference count of the \ref cardano_constr_plutus_data_t object.
 *
 * This function returns the number of active references to a \ref cardano_constr_plutus_data_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_constr_plutus_data_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param constr_plutus_data A pointer to the constr_plutus_data object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified constr_plutus_data object. If the object
 * is properly managed (i.e., every \ref cardano_constr_plutus_data_ref call is matched with a
 * \ref cardano_constr_plutus_data_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming constr_plutus_data is a previously created constr_plutus_data object
 *
 * size_t ref_count = cardano_constr_plutus_data_refcount(constr_plutus_data);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_constr_plutus_data_refcount(const cardano_constr_plutus_data_t* constr_plutus_data);

/**
 * \brief Sets the last error message for a given \ref cardano_constr_plutus_data_t object.
 *
 * Records an error message in the constr_plutus_data's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] constr_plutus_data A pointer to the \ref cardano_constr_plutus_data_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the constr_plutus_data's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_constr_plutus_data_set_last_error(cardano_constr_plutus_data_t* constr_plutus_data, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific \ref cardano_constr_plutus_data_t.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_constr_plutus_data_set_last_error for the given
 * \ref cardano_constr_plutus_data_t. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] constr_plutus_data A pointer to the \ref cardano_constr_plutus_data_t instance whose last error
 *                   message is to be retrieved. If the constr_plutus_data is NULL, the function
 *                   returns a generic error message indicating the null constr_plutus_data.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified constr_plutus_data. If the constr_plutus_data is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_constr_plutus_data_set_last_error for the same constr_plutus_data, or until
 *       the constr_plutus_data is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_constr_plutus_data_get_last_error(const cardano_constr_plutus_data_t* constr_plutus_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CONSTR_PLUTUS_DATA_H