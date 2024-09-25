/**
 * \file plutus_data.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_PLUTUS_DATA_H
#define BIGLUP_LABS_INCLUDE_CARDANO_PLUTUS_DATA_H

/* INCLUDES ******************************************************************/

#include <cardano/buffer.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/bigint.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/plutus_data/plutus_data_kind.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The main datatype `Constr` represents the nth constructor
 * along with its arguments.
 **/
typedef struct cardano_constr_plutus_data_t cardano_constr_plutus_data_t;

/**
 * \brief Represents a map of plutus data.
 */
typedef struct cardano_plutus_map_t cardano_plutus_map_t;

/**
 * \brief Represents a list of plutus data.
 */
typedef struct cardano_plutus_list_t cardano_plutus_list_t;

/**
 * \brief A type corresponding to the Plutus Core Data datatype.
 *
 * The point of this type is to be opaque as to ensure that it is only used in ways
 * that plutus scripts can handle.
 *
 * Use this type to build any data structures that you want to be representable on-chain.
 */
typedef struct cardano_plutus_data_t cardano_plutus_data_t;

/**
 * \brief Creates and initializes a new instance of a plutus_data from a constr_plutus_data.
 *
 * This function creates and initializes a new instance of a \ref cardano_plutus_data_t object
 * from a given \ref cardano_constr_plutus_data_t object. It essentially converts a constr_plutus_data
 * into a plutus_data. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] constr A constant pointer to the \ref cardano_constr_plutus_data_t object from which
 *                   the plutus_data will be created. The object must not be NULL.
 * \param[out] plutus_data On successful initialization, this will point to a newly created
 *             \ref cardano_plutus_data_t object. This object represents a "strong reference"
 *             to the plutus_data, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the plutus_data is no longer needed, the caller must release it
 *             by calling \ref cardano_plutus_data_unref.
 *
 * \return \ref CARDANO_SUCCESS if the plutus_data was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_constr_plutus_data_t* constr_plutus_data = cardano_constr_plutus_data_new(...);
 * cardano_plutus_data_t* plutus_data = NULL;
 *
 * // Attempt to create a new plutus_data from a constr_plutus_data
 * cardano_error_t result = cardano_plutus_data_new_constr(constr_plutus_data, &plutus_data);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the plutus_data
 *
 *   // Once done, ensure to clean up and release the plutus_data
 *   cardano_plutus_data_unref(&plutus_data);
 * }
 *
 * // Clean up the constr_plutus_data object once done
 * cardano_constr_plutus_data_unref(&constr_plutus_data);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_plutus_data_new_constr(
  cardano_constr_plutus_data_t* constr,
  cardano_plutus_data_t**       plutus_data);

/**
 * \brief Creates and initializes a new instance of a plutus_data from a plutus_map.
 *
 * This function creates and initializes a new instance of a \ref cardano_plutus_data_t object
 * from a given \ref cardano_plutus_map_t object. It essentially converts a plutus map
 * into a plutus data. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] map A pointer to the \ref cardano_plutus_map_t object from which
 *                the plutus data will be created.
 * \param[out] plutus_data On successful initialization, this will point to a newly created
 *             \ref cardano_plutus_data_t object. This object represents a "strong reference"
 *             to the plutus data, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the plutus_data is no longer needed, the caller must release it
 *             by calling \ref cardano_plutus_data_unref.
 *
 * \return \ref CARDANO_SUCCESS if the plutus_data was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_map_t* map = cardano_plutus_map_new(...);
 * cardano_plutus_data_t* plutus_data = NULL;
 *
 * // Attempt to create a new plutus_data from a plutus_map
 * cardano_error_t result = cardano_plutus_data_new_map(map, &plutus_data);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the plutus_data
 *
 *   // Once done, ensure to clean up and release the plutus_data
 *   cardano_plutus_data_unref(&plutus_data);
 * }
 *
 * // Clean up the plutus_map object once done
 * cardano_plutus_map_unref(&map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_plutus_data_new_map(
  cardano_plutus_map_t*   map,
  cardano_plutus_data_t** plutus_data);

/**
 * \brief Creates and initializes a new instance of a plutus data from a plutus list.
 *
 * This function creates and initializes a new instance of a \ref cardano_plutus_data_t object
 * from a given \ref cardano_plutus_list_t object. It essentially converts a plutus list
 * into a plutus data. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] list A pointer to the \ref cardano_plutus_list_t object from which
 *                 the plutus data will be created. The object must not be NULL.
 * \param[out] plutus_data On successful initialization, this will point to a newly created
 *             \ref cardano_plutus_data_t object. This object represents a "strong reference"
 *             to the plutus_data, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the plutus_data is no longer needed, the caller must release it
 *             by calling \ref cardano_plutus_data_unref.
 *
 * \return \ref CARDANO_SUCCESS if the plutus_data was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_list_t* list = cardano_plutus_list_new(...);
 * cardano_plutus_data_t* plutus_data = NULL;
 *
 * // Attempt to create a new plutus_data from a plutus_list
 * cardano_error_t result = cardano_plutus_data_new_list(list, &plutus_data);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the plutus_data
 *
 *   // Once done, ensure to clean up and release the plutus_data
 *   cardano_plutus_data_unref(&plutus_data);
 * }
 *
 * // Clean up the plutus_list object once done
 * cardano_plutus_list_unref(&list);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_plutus_data_new_list(
  cardano_plutus_list_t*  list,
  cardano_plutus_data_t** plutus_data);

/**
 * \brief Creates and initializes a new instance of plutus data from a bigint object.
 *
 * This function creates and initializes a new instance of a \ref cardano_plutus_data_t object
 * from a given bigint object.
 *
 * \param[in] bigint A constant pointer to the \ref cardano_bigint_t object from which the plutus_data will be created.
 * \param[out] plutus_data On successful initialization, this will point to a newly created
 *             \ref cardano_plutus_data_t object. This object represents a "strong reference"
 *             to the plutus data, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the plutus data is no longer needed, the caller must release it
 *             by calling \ref cardano_plutus_data_unref.
 *
 * \return \ref CARDANO_SUCCESS if the plutus data was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * // Assume bigint is already initialized and set to a value
 * cardano_bigint_t* bigint = ...;
 * cardano_plutus_data_t* plutus_data = NULL;
 *
 * // Attempt to create a new plutus_data from a bigint
 * cardano_error_t result = cardano_plutus_data_new_integer(bigint, &plutus_data);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the plutus_data
 *
 *   // Once done, ensure to clean up and release the plutus_data
 *   cardano_plutus_data_unref(&plutus_data);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_plutus_data_new_integer(
  const cardano_bigint_t* bigint,
  cardano_plutus_data_t** plutus_data);

/**
 * \brief Creates and initializes a new instance of plutus data from an integer value.
 *
 * This function creates and initializes a new instance of a \ref cardano_plutus_data_t object
 * from a given `int64_t` integer. It converts an integer into a plutus data object, suitable
 * for transactions and other blockchain-related operations. It returns an error code to indicate
 * the success or failure of the operation.
 *
 * \param[in] integer The `int64_t` integer value from which the plutus_data will be created.
 * \param[out] plutus_data On successful initialization, this will point to a newly created
 *             \ref cardano_plutus_data_t object. This object represents a "strong reference"
 *             to the plutus data, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the plutus data is no longer needed, the caller must release it
 *             by calling \ref cardano_plutus_data_unref.
 *
 * \return \ref CARDANO_SUCCESS if the plutus data was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * int64_t integer_value = 42; // Example integer value
 * cardano_plutus_data_t* plutus_data = NULL;
 *
 * // Attempt to create a new plutus_data from an integer value
 * cardano_error_t result = cardano_plutus_data_new_integer_from_int(integer_value, &plutus_data);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the plutus_data
 *
 *   // Once done, ensure to clean up and release the plutus_data
 *   cardano_plutus_data_unref(&plutus_data);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_plutus_data_new_integer_from_int(
  int64_t                 integer,
  cardano_plutus_data_t** plutus_data);

/**
 * \brief Creates and initializes a new instance of plutus data from an unsigned integer value.
 *
 * This function creates and initializes a new instance of a \ref cardano_plutus_data_t object
 * from a given `uint64_t` integer.
 *
 * \param[in] integer The `uint64_t` integer value from which the plutus_data will be created.
 * \param[out] plutus_data On successful initialization, this will point to a newly created
 *             \ref cardano_plutus_data_t object. This object represents a "strong reference"
 *             to the plutus data, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the plutus data is no longer needed, the caller must release it
 *             by calling \ref cardano_plutus_data_unref.
 *
 * \return \ref CARDANO_SUCCESS if the plutus data was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * uint64_t integer_value = 18446744073709551615u; // Example unsigned integer value
 * cardano_plutus_data_t* plutus_data = NULL;
 *
 * // Attempt to create a new plutus_data from an unsigned integer value
 * cardano_error_t result = cardano_plutus_data_new_integer_from_uint(integer_value, &plutus_data);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the plutus_data
 *
 *   // Once done, ensure to clean up and release the plutus_data
 *   cardano_plutus_data_unref(&plutus_data);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_plutus_data_new_integer_from_uint(
  uint64_t                integer,
  cardano_plutus_data_t** plutus_data);

/**
 * \brief Creates and initializes a new instance of Plutus data from a string representation of an integer in a specified base.
 *
 * This function creates and initializes a new instance of a \ref cardano_plutus_data_t object from a given string
 * representing an integer, interpreted according to the specified base (e.g., 10 for decimal, 16 for hexadecimal).
 *
 * \param[in] string A pointer to the character string containing the numeric representation of the integer.
 * \param[in] size The number of characters in the string, excluding the null terminator.
 * \param[in] base The numerical base in which the string representation is expressed. Valid values are between 2 and 36.
 * \param[out] plutus_data On successful initialization, this will point to a newly created
 *                         \ref cardano_plutus_data_t object. This object represents a "strong reference"
 *                         to the plutus data, meaning that it is fully initialized and ready for use.
 *                         The caller is responsible for managing the lifecycle of this object.
 *                         Specifically, once the plutus data is no longer needed, the caller must release it
 *                         by calling \ref cardano_plutus_data_unref.
 *
 * \return \ref CARDANO_SUCCESS if the plutus data was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * const char* integer_str = "12345678901234567890"; // Example large integer value in string form
 * cardano_plutus_data_t* plutus_data = NULL;
 * int32_t base = 10; // Decimal base
 *
 * // Attempt to create a new plutus_data from a string representation of an integer in decimal
 * cardano_error_t result = cardano_plutus_data_new_integer_from_string(integer_str, strlen(integer_str), base, &plutus_data);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the plutus_data
 *
 *   // Once done, ensure to clean up and release the plutus_data
 *   cardano_plutus_data_unref(&plutus_data);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_plutus_data_new_integer_from_string(
  const char*             string,
  size_t                  size,
  int32_t                 base,
  cardano_plutus_data_t** plutus_data);

/**
 * \brief Creates and initializes a new instance of a plutus data from a byte array.
 *
 * This function creates and initializes a new instance of a \ref cardano_plutus_data_t object
 * from a given byte array. It essentially converts a byte array into a plutus data.
 * It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] bytes A pointer to the byte array from which the plutus_data will be created.
 *                  The byte array must not be NULL.
 * \param[in] size The size of the byte array.
 * \param[out] plutus_data On successful initialization, this will point to a newly created
 *             \ref cardano_plutus_data_t object. This object represents a "strong reference"
 *             to the plutus data, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the plutus data is no longer needed, the caller must release it
 *             by calling \ref cardano_plutus_data_unref.
 *
 * \return \ref CARDANO_SUCCESS if the plutus data was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * const byte_t bytes[] = {0x01, 0x02, 0x03, 0x04}; // Example byte array
 * size_t size = sizeof(bytes) / sizeof(byte_t); // Calculate the size of the byte array
 * cardano_plutus_data_t* plutus_data = NULL;
 *
 * // Attempt to create a new plutus data from a byte array
 * cardano_error_t result = cardano_plutus_data_new_bytes(bytes, size, &plutus_data);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the plutus data
 *
 *   // Once done, ensure to clean up and release the plutus data
 *   cardano_plutus_data_unref(&plutus_data);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_plutus_data_new_bytes(
  const byte_t*           bytes,
  size_t                  size,
  cardano_plutus_data_t** plutus_data);

/**
 * \brief Creates and initializes a new instance of a plutus data from a hexadecimal string.
 *
 * This function creates and initializes a new instance of a \ref cardano_plutus_data_t object
 * from a given hexadecimal string. It essentially converts a hexadecimal string into a plutus data,
 * treating each pair of characters as a byte. It returns an error code to indicate the success or
 * failure of the operation.
 *
 * \param[in] hex A pointer to the hexadecimal string from which the plutus data will be created.
 *                The hexadecimal string must not be NULL.
 * \param[in] size The size of the hexadecimal string.
 * \param[out] plutus_data On successful initialization, this will point to a newly created
 *             \ref cardano_plutus_data_t object. This object represents a "strong reference"
 *             to the plutus data, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the plutus data is no longer needed, the caller must release it
 *             by calling \ref cardano_plutus_data_unref.
 *
 * \return \ref CARDANO_SUCCESS if the plutus data was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * const char* hex_string = "0123456789abcdef"; // Example hexadecimal string
 * size_t size = strlen(hex_string); // Calculate the size of the string
 * cardano_plutus_data_t* plutus_data = NULL;
 *
 * // Attempt to create a new plutus data from a hexadecimal string
 * cardano_error_t result = cardano_plutus_data_new_bytes_from_hex(hex_string, size, &plutus_data);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the plutus data
 *
 *   // Once done, ensure to clean up and release the plutus data
 *   cardano_plutus_data_unref(&plutus_data);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_plutus_data_new_bytes_from_hex(
  const char*             hex,
  size_t                  size,
  cardano_plutus_data_t** plutus_data);

/**
 * \brief Creates a plutus_data from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_plutus_data_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a plutus_data.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded plutus_data data.
 * \param[out] plutus_data A pointer to a pointer of \ref cardano_plutus_data_t that will be set to the address
 *                        of the newly created plutus_data object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the plutus_data was successfully created, or an appropriate error code if an error occurred.
 *
 * \remark In Cardano, entities are encoded in CBOR, but CBOR allows multiple valid ways to encode the same data. The Cardano blockchain
 *         does not enforce a canonical CBOR representation, meaning that if you decode a transaction from CBOR and then re-encode it,
 *         the resulting encoding could be different. This would change the data and invalidate any existing signatures.
 *         To prevent this, when a plutus data object is created using \ref cardano_plutus_data_from_cbor, it caches the original
 *         CBOR representation internally. When \ref cardano_plutus_data_to_cbor is called, it will output the cached CBOR.
 *         If the cached CBOR representation is not needed, the client can call \ref cardano_plutus_data_clear_cbor_cache after the object has been created.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_plutus_data_t object by calling
 *       \ref cardano_plutus_data_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_plutus_data_t* plutus_data = NULL;
 *
 * cardano_error_t result = cardano_plutus_data_from_cbor(reader, &plutus_data);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the plutus_data
 *
 *   // Once done, ensure to clean up and release the plutus_data
 *   cardano_plutus_data_unref(&plutus_data);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode plutus_data: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_plutus_data_from_cbor(cardano_cbor_reader_t* reader, cardano_plutus_data_t** plutus_data);

/**
 * \brief Serializes a plutus_data into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_plutus_data_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] plutus_data A constant pointer to the \ref cardano_plutus_data_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p plutus_data or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * \remark In Cardano, entities are encoded in CBOR, but CBOR allows multiple valid ways to encode the same data. The Cardano blockchain
 *         does not enforce a canonical CBOR representation, meaning that if you decode a transaction from CBOR and then re-encode it,
 *         the resulting encoding could be different. This would change the data and invalidate any existing signatures.
 *         To prevent this, when a plutus data object is created using \ref cardano_plutus_data_from_cbor, it caches the original
 *         CBOR representation internally. When \ref cardano_plutus_data_to_cbor is called, it will output the cached CBOR.
 *         If the cached CBOR representation is not needed, the client can call \ref cardano_plutus_data_clear_cbor_cache after the object has been created.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_data_t* plutus_data = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_plutus_data_to_cbor(plutus_data, writer);
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
 * cardano_plutus_data_unref(&plutus_data);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_plutus_data_to_cbor(
  const cardano_plutus_data_t* plutus_data,
  cardano_cbor_writer_t*       writer);

/**
 * \brief Retrieves the kind of the plutus_data.
 *
 * This function retrieves the kind of a given \ref cardano_plutus_data_t object and stores it in the provided
 * output parameter. The plutus data kind is defined in the \ref cardano_plutus_data_kind_t enumeration, which
 * specifies whether the plutus data represents an integer, byte array, or other types.
 *
 * \param[in] plutus_data A constant pointer to the \ref cardano_plutus_data_t object from which
 *                        the kind is to be retrieved. The object must not be NULL.
 * \param[out] kind Pointer to a variable where the plutus_data kind will be stored. This variable will
 *                  be set to the value from the \ref cardano_plutus_data_kind_t enumeration.
 *
 * \return \ref CARDANO_SUCCESS if the kind was successfully retrieved, or an appropriate error code if the
 *         input is NULL or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_data_t* plutus_data = cardano_plutus_data_new_bytes_from_hex(...);
 * cardano_plutus_data_kind_t kind;
 * cardano_error_t result = cardano_plutus_data_get_kind(plutus_data, &kind);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   if (kind == CARDANO_PLUTUS_DATA_KIND_INTEGER)
 *   {
 *     // Handle integer type plutus_data
 *   }
 *   else if (kind == CARDANO_PLUTUS_DATA_KIND_BYTES)
 *   {
 *     // Handle byte array type plutus_data
 *   }
 *   // Add more cases as needed for other kinds
 * }
 *
 * // Clean up the plutus_data object once done
 * cardano_plutus_data_unref(&plutus_data);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_plutus_data_get_kind(
  const cardano_plutus_data_t* plutus_data,
  cardano_plutus_data_kind_t*  kind);

/**
 * \brief Converts a plutus data object to a constr plutus data object.
 *
 * This function converts a \ref cardano_plutus_data_t object to a \ref cardano_constr_plutus_data_t object.
 * It essentially creates a constr_plutus_data object from the given plutus_data, allowing the conversion
 * between different types of plutus data. It returns an error code to indicate the success or failure of
 * the operation.
 *
 * \param[in] plutus_data A constant pointer to the \ref cardano_plutus_data_t object to be converted.
 * \param[out] constr On successful conversion, this will point to a newly created
 *                    \ref cardano_constr_plutus_data_t object. This object represents a "strong reference"
 *                    to the constr_plutus_data, meaning that it is fully initialized and ready for use.
 *                    The caller is responsible for managing the lifecycle of this object.
 *                    Specifically, once the constr_plutus_data is no longer needed, the caller must release it
 *                    by calling \ref cardano_constr_plutus_data_unref.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_data_t* plutus_data = ...; // Assume plutus_data is a valid constr plutus data object
 * cardano_constr_plutus_data_t* constr = NULL;
 * cardano_error_t result = cardano_plutus_data_to_constr(plutus_data, &constr);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the constr_plutus_data
 *
 *   // Once done, ensure to clean up and release the constr_plutus_data
 *   cardano_constr_plutus_data_unref(&constr);
 * }
 *
 * // Clean up the plutus_data object once done
 * cardano_plutus_data_unref(&plutus_data);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_plutus_data_to_constr(
  cardano_plutus_data_t*         plutus_data,
  cardano_constr_plutus_data_t** constr);

/**
 * \brief Converts a plutus data object to a plutus map object.
 *
 * This function converts a \ref cardano_plutus_data_t object to a \ref cardano_plutus_map_t object.
 * It essentially creates a plutus map object from the given plutus data, allowing the conversion
 * between different types of plutus data. It returns an error code to indicate the success or failure of
 * the operation.
 *
 * \param[in] plutus_data A constant pointer to the \ref cardano_plutus_data_t object to be converted.
 *                        The object must not be NULL.
 * \param[out] map On successful conversion, this will point to a newly created
 *                 \ref cardano_plutus_map_t object. This object represents a "strong reference"
 *                 to the plutus_map, meaning that it is fully initialized and ready for use.
 *                 The caller is responsible for managing the lifecycle of this object.
 *                 Specifically, once the plutus_map is no longer needed, the caller must release it
 *                 by calling \ref cardano_plutus_map_unref.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_data_t* plutus_data = ...; // Assume plutus_data is a valid plutus map object
 * cardano_plutus_map_t* map = NULL;
 * cardano_error_t result = cardano_plutus_data_to_map(plutus_data, &map);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the plutus_map
 *
 *   // Once done, ensure to clean up and release the plutus_map
 *   cardano_plutus_map_unref(&map);
 * }
 *
 * // Clean up the plutus_data object once done
 * cardano_plutus_data_unref(&plutus_data);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_plutus_data_to_map(
  cardano_plutus_data_t* plutus_data,
  cardano_plutus_map_t** map);

/**
 * \brief Converts a plutus data object to a plutus list object.
 *
 * This function converts a \ref cardano_plutus_data_t object to a \ref cardano_plutus_list_t object.
 * It essentially creates a plutus list object from the given plutus data, allowing the conversion
 * between different types of plutus data. It returns an error code to indicate the success or failure of
 * the operation.
 *
 * \param[in] plutus_data A constant pointer to the \ref cardano_plutus_data_t object to be converted.
 * \param[out] list On successful conversion, this will point to a newly created
 *                  \ref cardano_plutus_list_t object. This object represents a "strong reference"
 *                  to the plutus_list, meaning that it is fully initialized and ready for use.
 *                  The caller is responsible for managing the lifecycle of this object.
 *                  Specifically, once the plutus_list is no longer needed, the caller must release it
 *                  by calling \ref cardano_plutus_list_unref.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_data_t* plutus_data = ...; // Assume plutus_data is a valid plutus list object
 * cardano_plutus_list_t* list = NULL;
 * cardano_error_t result = cardano_plutus_data_to_list(plutus_data, &list);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the plutus_list
 *
 *   // Once done, ensure to clean up and release the plutus_list
 *   cardano_plutus_list_unref(&list);
 * }
 *
 * // Clean up the plutus_data object once done
 * cardano_plutus_data_unref(&plutus_data);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_plutus_data_to_list(
  cardano_plutus_data_t*  plutus_data,
  cardano_plutus_list_t** list);

/**
 * \brief Converts a plutus data object to a bigint object.
 *
 * This function converts a \ref cardano_plutus_data_t object to a \ref cardano_bigint_t object.
 *
 * \param[in] plutus_data A constant pointer to the \ref cardano_plutus_data_t object to be converted.
 * \param[out] integer On successful conversion, this will point to a newly created \ref cardano_bigint_t object
 *                     representing the extracted integer value. The caller is responsible for managing the memory
 *                     of this object, including releasing it when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_data_t* plutus_data = cardano_plutus_data_new_integer_from_string("987456321478523698745");
 * cardano_bigint_t* bigint = NULL;
 * cardano_error_t result = cardano_plutus_data_to_integer(plutus_data, &bigint);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the bigint object
 *   const size_t size = cardano_bigint_get_string_size(bigint, 10);
 *   char* buffer = (char*)malloc(size);
 *
 *   if (cardano_bigint_to_string(bigint, buffer, size, 10) == CARDANO_SUCCESS)
 *   {
 *     printf("Extracted bigint value: %s\n", buffer);
 *   }
 * }
 *
 * // Clean up
 * cardano_plutus_data_unref(&plutus_data);
 * cardano_bigint_unref(&bigint);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_plutus_data_to_integer(
  const cardano_plutus_data_t* plutus_data,
  cardano_bigint_t**           integer);

/**
 * \brief Converts a plutus data object to bounded bytes.
 *
 * This function converts a \ref cardano_plutus_data_t object to bounded bytes.
 * It extracts the bytes from the given plutus data, allowing the conversion
 * between different types of plutus data. The resulting bytes are stored in a bounded bytes buffer.
 * It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] plutus_data A constant pointer to the \ref cardano_plutus_data_t object to be converted.
 * \param[out] bounded_bytes On successful conversion, this will point to a newly created
 *                           \ref cardano_buffer_t object containing the extracted bytes.
 *                           This object represents a "strong reference" to the bounded bytes,
 *                           meaning that it is fully initialized and ready for use.
 *                           The caller is responsible for managing the lifecycle of this object.
 *                           Specifically, once the bounded bytes are no longer needed, the caller must release it
 *                           by calling \ref cardano_buffer_unref.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_data_t* plutus_data = cardano_plutus_data_new_bytes_from_hex("DEADBEEF", 8);
 * cardano_buffer_t* bounded_bytes = NULL;
 * cardano_error_t result = cardano_plutus_data_to_bounded_bytes(plutus_data, &bounded_bytes);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the bounded bytes
 *   printf("Extracted bounded bytes: ");
 *
 *   for (size_t i = 0; i < cardano_buffer_get_size(bounded_bytes); ++i)
 *   {
 *     printf("%02X ", cardano_buffer_get_data(bounded_bytes)[i]);
 *   }
 *
 *   printf("\n");
 *
 *   // Once done, ensure to clean up and release the bounded bytes
 *   cardano_buffer_unref(&bounded_bytes);
 * }
 *
 * // Clean up the plutus_data object once done
 * cardano_plutus_data_unref(&plutus_data);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_plutus_data_to_bounded_bytes(
  cardano_plutus_data_t* plutus_data,
  cardano_buffer_t**     bounded_bytes);

/**
 * \brief Checks if two plutus_data objects are equal.
 *
 * This function compares two \ref cardano_plutus_data_t objects for equality.
 * It checks if the contents of the two plutus_data objects are identical.
 *
 * \param[in] lhs A constant pointer to the first \ref cardano_plutus_data_t object to be compared.
 * \param[in] rhs A constant pointer to the second \ref cardano_plutus_data_t object to be compared.
 *
 * \return \c true if the two plutus_data objects are equal (have the same contents), \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_data_t* plutus_data1 = cardano_plutus_data_new_bytes_from_hex("DEADBEEF", 8);
 * cardano_plutus_data_t* plutus_data2 = cardano_plutus_data_new_bytes_from_hex("DEADBEEF", 8);
 *
 * if (cardano_plutus_data_equals(plutus_data1, plutus_data2))
 * {
 *   printf("plutus_data1 is equal to plutus_data2\n");
 * }
 * else
 * {
 *   printf("plutus_data1 is not equal to plutus_data2\n");
 * }
 *
 * // Clean up the plutus_data objects once done
 * cardano_plutus_data_unref(&plutus_data1);
 * cardano_plutus_data_unref(&plutus_data2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_plutus_data_equals(const cardano_plutus_data_t* lhs, const cardano_plutus_data_t* rhs);

/**
 * \brief Clears the cached CBOR representation from a plutus data.
 *
 * This function removes the internally cached CBOR data from a \ref cardano_plutus_data_t object.
 * It is useful when you have modified the plutus_data after it was created from CBOR using
 * \ref cardano_plutus_data_from_cbor and you want to ensure that the next serialization reflects
 * the current state of the plutus_data, rather than using the original cached CBOR.
 *
 * \param[in,out] plutus_data A pointer to an initialized \ref cardano_plutus_data_t object
 *                         from which the CBOR cache will be cleared.
 *
 * \warning Clearing the CBOR cache may change the binary representation of the plutus data when
 *          serialized, which can alter the plutus data and invalidate any existing signatures.
 *          Use this function with caution, especially if the transaction has already been signed or
 *          if preserving the exact CBOR encoding is important for your application.
 *
 * Usage Example:
 * \code{.c}
 * // Assume plutus_data was created using cardano_plutus_data_from_cbor
 * cardano_plutus_data_t* plutus_data = ...;
 *
 * // Modify the plutus_data as needed
 * // For example, change the fee

 * if (result != CARDANO_SUCCESS)
 * {
 *   printf("Failed to set new fee: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clear the CBOR cache to ensure serialization uses the updated plutus_data
 * cardano_plutus_data_clear_cbor_cache(plutus_data);
 *
 * // Serialize the plutus_data to CBOR
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * result = cardano_plutus_data_to_cbor(plutus_data, writer);
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
 * cardano_plutus_data_unref(&plutus_data);
 * \endcode
 */
CARDANO_EXPORT void cardano_plutus_data_clear_cbor_cache(cardano_plutus_data_t* plutus_data);

/**
 * \brief Decrements the reference count of a plutus_data object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_plutus_data_t object
 * by decreasing its reference count. When the reference count reaches zero, the plutus_data is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] plutus_data A pointer to the pointer of the plutus_data object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_plutus_data_t* plutus_data = cardano_plutus_data_new();
 *
 * // Perform operations with the plutus_data...
 *
 * cardano_plutus_data_unref(&plutus_data);
 * // At this point, plutus_data is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_plutus_data_unref, the pointer to the \ref cardano_plutus_data_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_plutus_data_unref(cardano_plutus_data_t** plutus_data);

/**
 * \brief Increases the reference count of the cardano_plutus_data_t object.
 *
 * This function is used to manually increment the reference count of a plutus_data
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_plutus_data_unref.
 *
 * \param plutus_data A pointer to the plutus_data object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming plutus_data is a previously created plutus_data object
 *
 * cardano_plutus_data_ref(plutus_data);
 *
 * // Now plutus_data can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_plutus_data_ref there is a corresponding
 * call to \ref cardano_plutus_data_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_plutus_data_ref(cardano_plutus_data_t* plutus_data);

/**
 * \brief Retrieves the current reference count of the cardano_plutus_data_t object.
 *
 * This function returns the number of active references to a plutus_data object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_plutus_data_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param plutus_data A pointer to the plutus_data object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified plutus_data object. If the object
 * is properly managed (i.e., every \ref cardano_plutus_data_ref call is matched with a
 * \ref cardano_plutus_data_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming plutus_data is a previously created plutus_data object
 *
 * size_t ref_count = cardano_plutus_data_refcount(plutus_data);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_plutus_data_refcount(const cardano_plutus_data_t* plutus_data);

/**
 * \brief Sets the last error message for a given plutus_data object.
 *
 * Records an error message in the plutus_data's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] plutus_data A pointer to the \ref cardano_plutus_data_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the plutus_data's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_plutus_data_set_last_error(cardano_plutus_data_t* plutus_data, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific plutus_data.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_plutus_data_set_last_error for the given
 * plutus_data. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] plutus_data A pointer to the \ref cardano_plutus_data_t instance whose last error
 *                   message is to be retrieved. If the plutus_data is NULL, the function
 *                   returns a generic error message indicating the null plutus_data.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified plutus_data. If the plutus_data is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_plutus_data_set_last_error for the same plutus_data, or until
 *       the plutus_data is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_plutus_data_get_last_error(const cardano_plutus_data_t* plutus_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_PLUTUS_DATA_H