/**
 * \file datum.h
 *
 * \author luisd.bianchi
 * \date   May 15, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_DATUM_H
#define BIGLUP_LABS_INCLUDE_CARDANO_DATUM_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/datum_type.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a piece of data attached to a UTxO that a Plutus script can read when the
 *        UTxO is being spent. Essentially, the Datum acts as a state for that UTxO, allowing
 *        Plutus scripts to perform more complex logic based on this stored state.
 */
typedef struct cardano_datum_t cardano_datum_t;

/**
 * \brief Creates and initializes a new instance of a datum.
 *
 * This function allocates and initializes a new instance of \ref cardano_datum_t,
 * using the provided hash. It returns an error code to indicate success
 * or failure of the operation.
 *
 * \param[in] hash A pointer to \ref cardano_blake2b_hash_t representing the hash associated
 *             with this datum. The hash must be properly initialized before being
 *             passed to this function.
 * \param[out] datum On successful initialization, this will point to a newly created
 *             \ref cardano_datum_t object. This object represents a "strong reference"
 *             to the datum, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the datum is no longer needed, the caller must release it
 *             by calling \ref cardano_datum_unref.
 *
 * \return \ref CARDANO_SUCCESS if the datum was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_t* hash = { ... };  // Assume hash is initialized here
 * cardano_datum_t* datum = NULL;
 *
 * // Attempt to create a new datum
 * cardano_error_t result = cardano_datum_new_data_hash(hash, &datum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the datum
 *
 *   // Once done, ensure to clean up and release the datum
 *   cardano_datum_unref(&datum);
 * }
 *
 * cardano_blake2b_hash_unref(&hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_datum_new_data_hash(
  const cardano_blake2b_hash_t* hash,
  cardano_datum_t**             datum);

/**
 * \brief Creates a datum from a hexadecimal hash string.
 *
 * This function constructs a \ref cardano_datum_t object by interpreting the provided
 * hexadecimal string as a hash value. It returns an error code indicating the success or failure of the operation.
 *
 * \param[in] hex A pointer to a character array containing the hexadecimal representation of the hash.
 * \param[in] hex_size The size of the hexadecimal string in bytes.
 * \param[out] datum On successful initialization, this will point to a newly created
 *                 \ref cardano_datum_t object. This object represents a "strong reference"
 *                 to the datum, meaning that it is fully initialized and ready for use.
 *                 The caller is responsible for managing the lifecycle of this object,
 *                 specifically, once the datum is no longer needed, the caller must release it
 *                 by calling \ref cardano_datum_unref.
 *
 * \return \ref CARDANO_SUCCESS if the datum was successfully created, or an appropriate error code
 *         indicating the reason for failure.
 *
 * \note The function assumes that the hexadecimal string is valid and correctly formatted.
 *       Malformed input may lead to incorrect or undefined behavior.
 *
 * Usage Example:
 * \code{.c}
 * const char* hash_hex = "abcdef1234567890abcdef1234567890abcdef12....";
 * size_t hex_size = strlen(hash_hex);
 * cardano_datum_t* datum = NULL;
 *
 * // Attempt to create a new datum from a hexadecimal hash
 * cardano_error_t result = cardano_datum_new_data_hash_hex(hash_hex, hex_size, &datum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the datum
 *
 *   // Once done, ensure to clean up and release the datum
 *   cardano_datum_unref(&datum);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_datum_new_data_hash_hex(
  const char*       hex,
  size_t            hex_size,
  cardano_datum_t** datum);

/**
 * \brief Creates a datum from a byte array representing a hash.
 *
 * This function constructs a \ref cardano_datum_t object by using the provided
 * byte array as a hash value. It returns an error code indicating the success or failure of the operation.
 *
 * \param[in] data A pointer to the byte array containing the hash data.
 * \param[in] data_size The size of the byte array in bytes.
 * \param[out] datum On successful initialization, this will point to a newly created
 *                 \ref cardano_datum_t object. This object represents a "strong reference"
 *                 to the datum, meaning that it is fully initialized and ready for use.
 *                 The caller is responsible for managing the lifecycle of this object,
 *                 specifically, once the datum is no longer needed, the caller must release it
 *                 by calling \ref cardano_datum_unref.
 *
 * \return \ref CARDANO_SUCCESS if the datum was successfully created, or an appropriate error code
 *         indicating the reason for failure.
 *
 * Usage Example:
 * \code{.c}
 * const byte_t hash_data[] = { 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef ... };
 * size_t data_size = sizeof(hash_data);
 * cardano_datum_t* datum = NULL;
 *
 * // Attempt to create a new datum from byte array hash
 * cardano_error_t result = cardano_datum_new_data_hash_bytes(hash_data, data_size, &datum);
 *
 * if (result == CARDANO_SUCCESS && datum)
 * {
 *   // Use the datum
 *
 *   // Once done, ensure to clean up and release the datum
 *   cardano_datum_unref(&datum);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_datum_new_data_hash_bytes(
  const byte_t*     data,
  size_t            data_size,
  cardano_datum_t** datum);

/**
 * \brief Creates a datum from inline data.
 *
 * This function constructs a \ref cardano_datum_t object using the provided
 * \ref cardano_plutus_data_t object. It returns an error code indicating the success or failure of the operation.
 *
 * \param[in] data A pointer to the \ref cardano_plutus_data_t object representing the inline data.
 * \param[out] datum On successful initialization, this will point to a newly created
 *                 \ref cardano_datum_t object. This object represents a "strong reference"
 *                 to the datum, meaning that it is fully initialized and ready for use.
 *                 The caller is responsible for managing the lifecycle of this object,
 *                 specifically, once the datum is no longer needed, the caller must release it
 *                 by calling \ref cardano_datum_unref.
 *
 * \return \ref CARDANO_SUCCESS if the datum was successfully created, or an appropriate error code
 *         indicating the reason for failure.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_data_t* data = ...;  // Assume data is initialized here
 * cardano_datum_t* datum = NULL;
 *
 * // Attempt to create a new datum from inline data
 * cardano_error_t result = cardano_datum_new_inline_data(data, &datum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the datum
 *
 *   // Once done, ensure to clean up and release the datum
 *   cardano_datum_unref(&datum);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_datum_new_inline_data(
  cardano_plutus_data_t* data,
  cardano_datum_t**      datum);

/**
 * \brief Creates a datum from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_datum_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a datum.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded datum data.
 * \param[out] datum A pointer to a pointer of \ref cardano_datum_t that will be set to the address
 *                        of the newly created datum object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the datum was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_datum_t object by calling
 *       \ref cardano_datum_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_datum_t* datum = NULL;
 *
 * cardano_error_t result = cardano_datum_from_cbor(reader, &datum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the datum
 *
 *   // Once done, ensure to clean up and release the datum
 *   cardano_datum_unref(&datum);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode datum: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_datum_from_cbor(cardano_cbor_reader_t* reader, cardano_datum_t** datum);

/**
 * \brief Serializes a datum into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_datum_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] datum A constant pointer to the \ref cardano_datum_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p datum or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_datum_t* datum = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_datum_to_cbor(datum, writer);
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
 * cardano_datum_unref(&datum);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_datum_to_cbor(
  const cardano_datum_t* datum,
  cardano_cbor_writer_t* writer);

/**
 * \brief Retrieves the inline data associated with a datum.
 *
 * This function provides access to the inline data part of a \ref cardano_datum_t object.
 * It returns a pointer to a \ref cardano_plutus_data_t object representing the inline data.
 * The caller is responsible for managing the lifecycle of this object, including releasing it with
 * \ref cardano_plutus_data_unref when it is no longer needed.
 *
 * \param datum A constant pointer to the \ref cardano_datum_t object from which
 *              the inline data is to be retrieved.
 *
 * \return A pointer to a \ref cardano_plutus_data_t object containing the inline data.
 *         If the input datum is NULL, returns NULL. The caller is responsible for
 *         managing the lifecycle of this object, including releasing it with
 *         \ref cardano_plutus_data_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_datum_t* datum = ...;
 * cardano_plutus_data_t* data = cardano_datum_get_inline_data(datum);
 *
 * if (data)
 * {
 *   // Use the inline data
 *
 *   // Once done, ensure to clean up and release the inline data
 *   cardano_plutus_data_unref(&data);
 * }
 *
 * // Clean up the datum object once done
 * cardano_datum_unref(&datum);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_plutus_data_t* cardano_datum_get_inline_data(cardano_datum_t* datum);

/**
 * \brief Retrieves the hash associated with a datum.
 *
 * This function provides access to the hash part of a \ref cardano_datum_t object.
 * It returns a new reference to a \ref cardano_blake2b_hash_t object representing the hash.
 * This allows the hash to be used independently of the original datum object. The
 * reference count of the hash object is increased by one, making it the caller's responsibility
 * to release it by calling \ref cardano_blake2b_hash_unref when it is no longer needed.
 *
 * \param datum A constant pointer to the \ref cardano_datum_t object from which
 *                   the hash is to be retrieved.
 *
 * \return A pointer to a new \ref cardano_blake2b_hash_t object containing the hash.
 *         If the input datum is NULL, returns NULL. The caller is responsible for
 *         managing the lifecycle of this object, including releasing it with
 *         \ref cardano_blake2b_hash_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_datum_t* original_datum = cardano_datum_new(...);
 * cardano_blake2b_hash_t* hash_datum = cardano_datum_get_data_hash(original_datum);
 *
 * if (hash_datum)
 * {
 *   // Use the hash datum
 *
 *   // Once done, ensure to clean up and release the hash datum
 *   cardano_blake2b_hash_unref(&hash_datum);
 * }
 * // Release the original datum after use
 * cardano_datum_unref(&original_datum);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_datum_get_data_hash(const cardano_datum_t* datum);

/**
 * \brief Retrieves the size of the hash bytes stored in the datum.
 *
 * This function computes the size of the hash bytes stored within a \ref cardano_datum_t object.
 * It is particularly useful for determining the buffer size needed to store the hash bytes when
 * retrieving them via \ref cardano_datum_get_data_hash_bytes.
 *
 * \param[in] datum A constant pointer to the \ref cardano_datum_t object from which
 *                       the size of the hash bytes is to be retrieved.
 *
 * \return The size of the hash bytes. If the datum is NULL, the function returns zero.
 *
 * Usage Example:
 * \code{.c}
 * cardano_datum_t* datum = cardano_datum_new(...);
 * size_t hash_size = cardano_datum_get_data_hash_bytes_size(datum);
 *
 * if (hash_size > 0)
 * {
 *   byte_t* hash_bytes = malloc(hash_size);
 *   if (hash_bytes)
 *   {
 *     // Proceed to get the hash bytes
 *   }
 * }
 * cardano_datum_unref(&datum);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_datum_get_data_hash_bytes_size(const cardano_datum_t* datum);

/**
 * \brief Retrieves the byte array representation of the hash from a datum.
 *
 * This function accesses the byte representation of the hash associated with a given
 * \ref cardano_datum_t object. It provides a direct pointer to the internal byte array
 * representing the hash, which should not be modified or freed by the caller.
 *
 * \param datum A constant pointer to the \ref cardano_datum_t object from which
 *                   the hash bytes are to be retrieved.
 *
 * \return A pointer to a constant byte array containing the hash data. If the input datum
 *         is NULL, returns NULL. The data remains valid as long as the datum object is not
 *         freed or modified.
 *
 * Usage Example:
 * \code{.c}
 * cardano_datum_t* datum = cardano_datum_new(...);
 * const byte_t* hash_bytes = cardano_datum_get_data_hash_bytes(datum);
 *
 * if (hash_bytes)
 * {
 *   // Use the hash bytes for operations like comparison or display
 * }
 *
 * // Clean up the datum object once done
 * cardano_datum_unref(&datum);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const byte_t* cardano_datum_get_data_hash_bytes(const cardano_datum_t* datum);

/**
 * \brief Retrieves the size needed for the hexadecimal string representation of the datum's hash.
 *
 * This function calculates the size required to store the hexadecimal string representation of the hash
 * associated with a \ref cardano_datum_t object. This size includes the space needed for the null-terminator.
 *
 * \param[in] datum A constant pointer to the \ref cardano_datum_t object whose hash size is to be determined.
 *
 * \return The size in bytes required to store the hexadecimal representation of the hash, including the null terminator.
 *
 * Usage Example:
 * \code{.c}
 * cardano_datum_t* datum = cardano_datum_new(...);
 * size_t hex_size = cardano_datum_get_data_hash_hex_size(datum);
 * char* hex_string = malloc(hex_size);
 *
 * if (hex_string)
 * {
 *   // Now use hex_string to get the hash or do other operations
 *   free(hex_string);
 * }
 *
 * cardano_datum_unref(&datum);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_datum_get_data_hash_hex_size(const cardano_datum_t* datum);

/**
 * \brief Retrieves the hexadecimal string representation of the hash from a datum.
 *
 * This function provides access to the hexadecimal (hex) string representation of the hash
 * associated with a given \ref cardano_datum_t object. It returns a direct pointer to the
 * internal hex string which should not be modified or freed by the caller.
 *
 * \param datum A constant pointer to the \ref cardano_datum_t object from which
 *                   the hex string of the hash is to be retrieved. The object must not be NULL.
 *
 * \return A pointer to a constant character array containing the hex representation of the hash.
 *         If the input datum is NULL, returns NULL. The data remains valid as long as the
 *         datum object is not freed or modified.
 *
 * Usage Example:
 * \code{.c}
 * cardano_datum_t* datum = cardano_datum_new(...);
 * const char* hash_hex = cardano_datum_get_data_hash_hex(datum);
 *
 * if (hash_hex)
 * {
 *   // Use the hash hex for operations like logging, display, or comparison
 * }
 *
 * // Clean up the datum object once done
 * cardano_datum_unref(&datum);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_datum_get_data_hash_hex(const cardano_datum_t* datum);

/**
 * \brief Retrieves the type of the datum.
 *
 * This function retrieves the type of a given \ref cardano_datum_t object and stores it in the provided
 * output parameter. The datum type is defined in the \ref cardano_datum_type_t enumeration.
 *
 * \param[in] datum A constant pointer to the \ref cardano_datum_t object from which
 *                       the type is to be retrieved. The object must not be NULL.
 * \param[out] type Pointer to a variable where the datum type will be stored. This variable will
 *                  be set to the value from the \ref cardano_datum_type_t enumeration.
 *
 * \return CARDANO_SUCCESS if the type was successfully retrieved, or an appropriate error code if the
 *         input is NULL or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_datum_t* datum = cardano_datum_new(...);
 * cardano_datum_type_t type;
 * cardano_error_t result = cardano_datum_get_type(datum, &type);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the type
 * }
 *
 * // Clean up the datum object once done
 * cardano_datum_unref(&datum);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_datum_get_type(const cardano_datum_t* datum, cardano_datum_type_t* type);

/**
 * \brief Sets the hash for a datum.
 *
 * This function assigns a new hash to an existing \ref cardano_datum_t object. The hash
 * represents the identifying data for the datum. The function copies the provided hash into
 * the datum, so the original hash object may be modified or freed after this operation without
 * affecting the datum's hash.
 *
 * \param[in,out] datum A pointer to the \ref cardano_datum_t object whose hash is to be set.
 *                           This object must have been previously created and not yet freed.
 * \param[in] hash A pointer to a \ref cardano_blake2b_hash_t object containing the new hash to be set.
 *                 This parameter must not be NULL.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the hash was successfully set. If the \p datum or \p hash is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_datum_t* datum = ...;
 * cardano_blake2b_hash_t* new_hash = cardano_blake2b_compute_hash(...);
 *
 * cardano_error_t result = cardano_datum_set_data_hash(datum, new_hash);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *     // The hash was successfully set
 * }
 *
 * // Clean up
 * cardano_datum_unref(&datum);
 * cardano_blake2b_hash_unref(&new_hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_datum_set_data_hash(cardano_datum_t* datum, const cardano_blake2b_hash_t* hash);

/**
 * \brief Compares two datum objects for equality.
 *
 * This function compares two datum objects to determine if they are equal.
 *
 * \param[in] lhs Pointer to the first datum object.
 * \param[in] rhs Pointer to the second datum object.
 *
 * \return \c true if the datum objects are equal, \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_datum_t* datum1 = NULL;
 * cardano_datum_t* datum2 = NULL;
 *
 * // Assume datum1 and datum2 are initialized properly
 *
 * bool are_equal = cardano_datum_equals(datum1, datum2);
 *
 * if (are_equal)
 * {
 *   printf("The datum objects are equal.\n");
 * }
 * else
 * {
 *   printf("The datum objects are not equal.\n");
 * }
 *
 * // Clean up
 * cardano_datum_unref(&datum1);
 * cardano_datum_unref(&datum2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_datum_equals(const cardano_datum_t* lhs, const cardano_datum_t* rhs);

/**
 * \brief Decrements the reference count of a datum object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_datum_t object
 * by decreasing its reference count. When the reference count reaches zero, the datum is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] datum A pointer to the pointer of the datum object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_datum_t* datum = cardano_datum_new();
 *
 * // Perform operations with the datum...
 *
 * cardano_datum_unref(&datum);
 * // At this point, datum is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_datum_unref, the pointer to the \ref cardano_datum_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_datum_unref(cardano_datum_t** datum);

/**
 * \brief Increases the reference count of the cardano_datum_t object.
 *
 * This function is used to manually increment the reference count of a datum
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_datum_unref.
 *
 * \param datum A pointer to the datum object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming datum is a previously created datum object
 *
 * cardano_datum_ref(datum);
 *
 * // Now datum can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_datum_ref there is a corresponding
 * call to \ref cardano_datum_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_datum_ref(cardano_datum_t* datum);

/**
 * \brief Retrieves the current reference count of the cardano_datum_t object.
 *
 * This function returns the number of active references to a datum object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_datum_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param datum A pointer to the datum object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified datum object. If the object
 * is properly managed (i.e., every \ref cardano_datum_ref call is matched with a
 * \ref cardano_datum_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming datum is a previously created datum object
 *
 * size_t ref_count = cardano_datum_refcount(datum);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_datum_refcount(const cardano_datum_t* datum);

/**
 * \brief Sets the last error message for a given datum object.
 *
 * Records an error message in the datum's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] datum A pointer to the \ref cardano_datum_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the datum's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_datum_set_last_error(cardano_datum_t* datum, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific datum.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_datum_set_last_error for the given
 * datum. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] datum A pointer to the \ref cardano_datum_t instance whose last error
 *                   message is to be retrieved. If the datum is NULL, the function
 *                   returns a generic error message indicating the null datum.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified datum. If the datum is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_datum_set_last_error for the same datum, or until
 *       the datum is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_datum_get_last_error(const cardano_datum_t* datum);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_DATUM_H