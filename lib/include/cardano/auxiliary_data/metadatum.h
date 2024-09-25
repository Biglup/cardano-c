/**
 * \file metadatum.h
 *
 * \author angel.castillo
 * \date   Sep 19, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_METADATUM_H
#define BIGLUP_LABS_INCLUDE_CARDANO_METADATUM_H

/* INCLUDES ******************************************************************/

#include <cardano/auxiliary_data/metadatum_kind.h>
#include <cardano/buffer.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/bigint.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a map of metadatum.
 */
typedef struct cardano_metadatum_map_t cardano_metadatum_map_t;

/**
 * \brief Represents a list of metadatum.
 */
typedef struct cardano_metadatum_list_t cardano_metadatum_list_t;

/**
 * \brief A type corresponding to the metadatum type.
 *
 * Use this type to build metadata structures that you want to be representable on-chain.
 */
typedef struct cardano_metadatum_t cardano_metadatum_t;

/**
 * \brief Creates and initializes a new instance of a metadatum from a metadatum_map.
 *
 * This function creates and initializes a new instance of a \ref cardano_metadatum_t object
 * from a given \ref cardano_metadatum_map_t object. It essentially converts a metadatum map
 * into a metadatum. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] map A pointer to the \ref cardano_metadatum_map_t object from which
 *                the metadatum will be created.
 * \param[out] metadatum On successful initialization, this will point to a newly created
 *             \ref cardano_metadatum_t object. This object represents a "strong reference"
 *             to the metadatum, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the metadatum is no longer needed, the caller must release it
 *             by calling \ref cardano_metadatum_unref.
 *
 * \return \ref CARDANO_SUCCESS if the metadatum was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_metadatum_map_t* map = cardano_metadatum_map_new(...);
 * cardano_metadatum_t* metadatum = NULL;
 *
 * // Attempt to create a new metadatum from a metadatum_map
 * cardano_error_t result = cardano_metadatum_new_map(map, &metadatum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the metadatum
 *
 *   // Once done, ensure to clean up and release the metadatum
 *   cardano_metadatum_unref(&metadatum);
 * }
 *
 * // Clean up the metadatum_map object once done
 * cardano_metadatum_map_unref(&map);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_metadatum_new_map(
  cardano_metadatum_map_t* map,
  cardano_metadatum_t**    metadatum);

/**
 * \brief Creates and initializes a new instance of a metadatum from a metadatum list.
 *
 * This function creates and initializes a new instance of a \ref cardano_metadatum_t object
 * from a given \ref cardano_metadatum_list_t object. It essentially converts a metadatum list
 * into a metadatum. It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] list A pointer to the \ref cardano_metadatum_list_t object from which
 *                 the metadatum will be created. The object must not be NULL.
 * \param[out] metadatum On successful initialization, this will point to a newly created
 *             \ref cardano_metadatum_t object. This object represents a "strong reference"
 *             to the metadatum, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the metadatum is no longer needed, the caller must release it
 *             by calling \ref cardano_metadatum_unref.
 *
 * \return \ref CARDANO_SUCCESS if the metadatum was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_metadatum_list_t* list = cardano_metadatum_list_new(...);
 * cardano_metadatum_t* metadatum = NULL;
 *
 * // Attempt to create a new metadatum from a metadatum_list
 * cardano_error_t result = cardano_metadatum_new_list(list, &metadatum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the metadatum
 *
 *   // Once done, ensure to clean up and release the metadatum
 *   cardano_metadatum_unref(&metadatum);
 * }
 *
 * // Clean up the metadatum_list object once done
 * cardano_metadatum_list_unref(&list);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_metadatum_new_list(
  cardano_metadatum_list_t* list,
  cardano_metadatum_t**     metadatum);

/**
 * \brief Creates and initializes a new instance of metadatum from a bigint object.
 *
 * This function creates and initializes a new instance of a \ref cardano_metadatum_t object
 * from a given bigint object.
 *
 * \param[in] bigint A constant pointer to the \ref cardano_bigint_t object from which the metadatum will be created.
 * \param[out] metadatum On successful initialization, this will point to a newly created
 *             \ref cardano_metadatum_t object. This object represents a "strong reference"
 *             to the metadatum, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the metadatum is no longer needed, the caller must release it
 *             by calling \ref cardano_metadatum_unref.
 *
 * \return \ref CARDANO_SUCCESS if the metadatum was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * // Assume bigint is already initialized and set to a value
 * cardano_bigint_t* bigint = ...;
 * cardano_metadatum_t* metadatum = NULL;
 *
 * // Attempt to create a new metadatum from a bigint
 * cardano_error_t result = cardano_metadatum_new_integer(bigint, &metadatum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the metadatum
 *
 *   // Once done, ensure to clean up and release the metadatum
 *   cardano_metadatum_unref(&metadatum);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_metadatum_new_integer(
  const cardano_bigint_t* bigint,
  cardano_metadatum_t**   metadatum);

/**
 * \brief Creates and initializes a new instance of metadatum from an integer value.
 *
 * This function creates and initializes a new instance of a \ref cardano_metadatum_t object
 * from a given `int64_t` integer. It converts an integer into a metadatum object, suitable
 * for transactions and other blockchain-related operations. It returns an error code to indicate
 * the success or failure of the operation.
 *
 * \param[in] integer The `int64_t` integer value from which the metadatum will be created.
 * \param[out] metadatum On successful initialization, this will point to a newly created
 *             \ref cardano_metadatum_t object. This object represents a "strong reference"
 *             to the metadatum, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the metadatum is no longer needed, the caller must release it
 *             by calling \ref cardano_metadatum_unref.
 *
 * \return \ref CARDANO_SUCCESS if the metadatum was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * int64_t integer_value = 42; // Example integer value
 * cardano_metadatum_t* metadatum = NULL;
 *
 * // Attempt to create a new metadatum from an integer value
 * cardano_error_t result = cardano_metadatum_new_integer_from_int(integer_value, &metadatum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the metadatum
 *
 *   // Once done, ensure to clean up and release the metadatum
 *   cardano_metadatum_unref(&metadatum);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_metadatum_new_integer_from_int(
  int64_t               integer,
  cardano_metadatum_t** metadatum);

/**
 * \brief Creates and initializes a new instance of metadatum from an unsigned integer value.
 *
 * This function creates and initializes a new instance of a \ref cardano_metadatum_t object
 * from a given `uint64_t` integer.
 *
 * \param[in] integer The `uint64_t` integer value from which the metadatum will be created.
 * \param[out] metadatum On successful initialization, this will point to a newly created
 *             \ref cardano_metadatum_t object. This object represents a "strong reference"
 *             to the metadatum, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the metadatum is no longer needed, the caller must release it
 *             by calling \ref cardano_metadatum_unref.
 *
 * \return \ref CARDANO_SUCCESS if the metadatum was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * uint64_t integer_value = 18446744073709551615u; // Example unsigned integer value
 * cardano_metadatum_t* metadatum = NULL;
 *
 * // Attempt to create a new metadatum from an unsigned integer value
 * cardano_error_t result = cardano_metadatum_new_integer_from_uint(integer_value, &metadatum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the metadatum
 *
 *   // Once done, ensure to clean up and release the metadatum
 *   cardano_metadatum_unref(&metadatum);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_metadatum_new_integer_from_uint(
  uint64_t              integer,
  cardano_metadatum_t** metadatum);

/**
 * \brief Creates and initializes a new instance of Plutus data from a string representation of an integer in a specified base.
 *
 * This function creates and initializes a new instance of a \ref cardano_metadatum_t object from a given string
 * representing an integer, interpreted according to the specified base (e.g., 10 for decimal, 16 for hexadecimal).
 *
 * \param[in] string A pointer to the character string containing the numeric representation of the integer.
 * \param[in] size The number of characters in the string, excluding the null terminator.
 * \param[in] base The numerical base in which the string representation is expressed. Valid values are between 2 and 36.
 * \param[out] metadatum On successful initialization, this will point to a newly created
 *                         \ref cardano_metadatum_t object. This object represents a "strong reference"
 *                         to the metadatum, meaning that it is fully initialized and ready for use.
 *                         The caller is responsible for managing the lifecycle of this object.
 *                         Specifically, once the metadatum is no longer needed, the caller must release it
 *                         by calling \ref cardano_metadatum_unref.
 *
 * \return \ref CARDANO_SUCCESS if the metadatum was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * const char* integer_str = "12345678901234567890"; // Example large integer value in string form
 * cardano_metadatum_t* metadatum = NULL;
 * int32_t base = 10; // Decimal base
 *
 * // Attempt to create a new metadatum from a string representation of an integer in decimal
 * cardano_error_t result = cardano_metadatum_new_integer_from_string(integer_str, strlen(integer_str), base, &metadatum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the metadatum
 *
 *   // Once done, ensure to clean up and release the metadatum
 *   cardano_metadatum_unref(&metadatum);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_metadatum_new_integer_from_string(
  const char*           string,
  size_t                size,
  int32_t               base,
  cardano_metadatum_t** metadatum);

/**
 * \brief Creates and initializes a new instance of a metadatum from a byte array.
 *
 * This function creates and initializes a new instance of a \ref cardano_metadatum_t object
 * from a given byte array. It essentially converts a byte array into a metadatum.
 * It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] bytes A pointer to the byte array from which the metadatum will be created.
 *                  The byte array must not be NULL.
 * \param[in] size The size of the byte array.
 * \param[out] metadatum On successful initialization, this will point to a newly created
 *             \ref cardano_metadatum_t object. This object represents a "strong reference"
 *             to the metadatum, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the metadatum is no longer needed, the caller must release it
 *             by calling \ref cardano_metadatum_unref.
 *
 * \return \ref CARDANO_SUCCESS if the metadatum was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * const byte_t bytes[] = {0x01, 0x02, 0x03, 0x04}; // Example byte array
 * size_t size = sizeof(bytes) / sizeof(byte_t); // Calculate the size of the byte array
 * cardano_metadatum_t* metadatum = NULL;
 *
 * // Attempt to create a new metadatum from a byte array
 * cardano_error_t result = cardano_metadatum_new_bytes(bytes, size, &metadatum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the metadatum
 *
 *   // Once done, ensure to clean up and release the metadatum
 *   cardano_metadatum_unref(&metadatum);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_metadatum_new_bytes(
  const byte_t*         bytes,
  size_t                size,
  cardano_metadatum_t** metadatum);

/**
 * \brief Creates and initializes a new instance of a metadatum from a hexadecimal string.
 *
 * This function creates and initializes a new instance of a \ref cardano_metadatum_t object
 * from a given hexadecimal string. It essentially converts a hexadecimal string into a metadatum,
 * treating each pair of characters as a byte. It returns an error code to indicate the success or
 * failure of the operation.
 *
 * \param[in] hex A pointer to the hexadecimal string from which the metadatum will be created.
 *                The hexadecimal string must not be NULL.
 * \param[in] size The size of the hexadecimal string.
 * \param[out] metadatum On successful initialization, this will point to a newly created
 *             \ref cardano_metadatum_t object. This object represents a "strong reference"
 *             to the metadatum, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the metadatum is no longer needed, the caller must release it
 *             by calling \ref cardano_metadatum_unref.
 *
 * \return \ref CARDANO_SUCCESS if the metadatum was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * const char* hex_string = "0123456789abcdef"; // Example hexadecimal string
 * size_t size = strlen(hex_string); // Calculate the size of the string
 * cardano_metadatum_t* metadatum = NULL;
 *
 * // Attempt to create a new metadatum from a hexadecimal string
 * cardano_error_t result = cardano_metadatum_new_bytes_from_hex(hex_string, size, &metadatum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the metadatum
 *
 *   // Once done, ensure to clean up and release the metadatum
 *   cardano_metadatum_unref(&metadatum);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_metadatum_new_bytes_from_hex(
  const char*           hex,
  size_t                size,
  cardano_metadatum_t** metadatum);

/**
 * \brief Creates and initializes a new instance of a metadatum from a string.
 *
 * This function creates and initializes a new instance of a \ref cardano_metadatum_t object
 * from a given string.
 *
 * \param[in] string A pointer to the string from which the metadatum will be created.
 *                The string must not be NULL.
 * \param[in] size The size of the string.
 * \param[out] metadatum On successful initialization, this will point to a newly created
 *             \ref cardano_metadatum_t object. This object represents a "strong reference"
 *             to the metadatum, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the metadatum is no longer needed, the caller must release it
 *             by calling \ref cardano_metadatum_unref.
 *
 * \return \ref CARDANO_SUCCESS if the metadatum was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * const char* string = "hello world"; // Example string
 * size_t size = strlen(string); // Calculate the size of the string
 * cardano_metadatum_t* metadatum = NULL;
 *
 * // Attempt to create a new metadatum from a string
 * cardano_error_t result = cardano_metadatum_new_string(string, size, &metadatum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the metadatum
 *
 *   // Once done, ensure to clean up and release the metadatum
 *   cardano_metadatum_unref(&metadatum);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_metadatum_new_string(
  const char*           string,
  size_t                size,
  cardano_metadatum_t** metadatum);

/**
 * \brief Creates a metadatum from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_metadatum_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a metadatum.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded metadatum.
 * \param[out] metadatum A pointer to a pointer of \ref cardano_metadatum_t that will be set to the address
 *                        of the newly created metadatum object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the metadatum was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_metadatum_t object by calling
 *       \ref cardano_metadatum_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_metadatum_t* metadatum = NULL;
 *
 * cardano_error_t result = cardano_metadatum_from_cbor(reader, &metadatum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the metadatum
 *
 *   // Once done, ensure to clean up and release the metadatum
 *   cardano_metadatum_unref(&metadatum);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode metadatum: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_metadatum_from_cbor(cardano_cbor_reader_t* reader, cardano_metadatum_t** metadatum);

/**
 * \brief Creates a metadatum from a JSON string.
 *
 * This function parses JSON data using a provided JSON string and constructs a \ref cardano_metadatum_t object.
 * It assumes that the JSON data corresponds to the structure expected for a metadatum.
 *
 * \param[in] json A pointer to a character array containing the JSON-encoded metadatum data.
 * \param[in] json_size The size of the JSON data in bytes.
 * \param[out] metadatum A pointer to a pointer of \ref cardano_metadatum_t that will be set to the address
 *                           of the newly created metadatum object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the metadatum was successfully created, or an appropriate error code if an error occurred.
 *
 * Usage Example:
 * \code{.c}
 * const char* json_data = "{...}"; // Assume this contains valid JSON-encoded metadatum data
 * size_t data_size = strlen(json_data);
 * cardano_metadatum_t* metadatum = NULL;
 *
 * cardano_error_t result = cardano_metadatum_from_json(json_data, data_size, &metadatum);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the metadatum
 *
 *   // Once done, ensure to clean up and release the metadatum
 *   cardano_metadatum_unref(&metadatum);
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to decode metadatum: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_metadatum_from_json(const char* json, size_t json_size, cardano_metadatum_t** metadatum);

/**
 * \brief Computes the size needed for the JSON string representation of a metadatum.
 *
 * This function calculates the size in bytes required to represent a \ref cardano_metadatum_t
 * as a null-terminated JSON string, including the null terminator. This is useful for allocating
 * an appropriately sized buffer before converting the metadatum to JSON using
 * \ref cardano_metadatum_to_json.
 *
 * \param[in] metadatum A pointer to the \ref cardano_metadatum_t instance whose JSON size is being queried.
 *                      This parameter must not be NULL.
 *
 * \return The size in bytes required to represent the metadatum as a null-terminated JSON string,
 *         including the null terminator. Returns 0 if \p metadatum is NULL or if an error occurs.
 *
 * \note The size returned includes space for the null terminator.
 *
 * Usage Example:
 * \code{.c}
 * // Assume metadatum is already initialized
 * cardano_metadatum_t* metadatum = ...;
 *
 * size_t json_size = cardano_metadatum_get_json_size(metadatum);
 *
 * if (json_size > 0)
 * {
 *   // json_size now contains the size needed to store the JSON string, including null terminator
 *   // You can allocate a buffer of this size for conversion
 * }
 * else
 * {
 *   printf("Failed to get the JSON size.\n");
 *   // Handle error
 * }
 * \endcode
 *
 * \see cardano_metadatum_to_json
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_metadatum_get_json_size(cardano_metadatum_t* metadatum);

/**
 * \brief Converts a metadatum object to a JSON string.
 *
 * This function converts a \ref cardano_metadatum_t object to its JSON representation.
 * The resulting JSON string is stored in the provided buffer, which must be pre-allocated by the caller.
 * The buffer must be large enough to hold the entire JSON string, including the null terminator.
 *
 * To determine the required buffer size (including the null terminator), use
 * \ref cardano_metadatum_get_json_size before calling this function.
 *
 * \param[in] metadatum A pointer to the \ref cardano_metadatum_t object to be converted.
 *                      This parameter must not be NULL.
 * \param[out] json A pointer to a character array where the null-terminated JSON string will be stored.
 *                  The buffer must be allocated by the caller and have a size of at least \p json_size bytes.
 *                  This parameter must not be NULL.
 * \param[in] json_size The size of the \p json buffer in bytes, including space for the null terminator.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful.
 *         Returns \ref CARDANO_ERROR_POINTER_IS_NULL if \p metadatum or \p json is NULL.
 *         Returns \ref CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE if \p json_size is insufficient to hold the JSON string.
 *
 * \note The JSON string will be encoded in UTF-8.
 *
 * Usage Example:
 * \code{.c}
 * // Assume metadatum is already initialized
 * cardano_metadatum_t* metadatum = ...;
 *
 * // Get the size needed for the JSON string (including null terminator)
 * size_t json_size = cardano_metadatum_get_json_size(metadatum);
 * if (json_size == 0)
 * {
 *   printf("Failed to get the JSON size.\n");
 *   // Handle error
 * }
 * else
 * {
 *   // Allocate buffer for the JSON string
 *   char* json_buffer = (char*)malloc(json_size);
 *   if (json_buffer == NULL)
 *   {
 *     printf("Memory allocation failed.\n");
 *     // Handle memory allocation failure
 *   }
 *   else
 *   {
 *     // Convert the metadatum to JSON
 *     cardano_error_t result = cardano_metadatum_to_json(metadatum, json_buffer, json_size);
 *     if (result == CARDANO_SUCCESS)
 *     {
 *       printf("Metadatum as JSON: %s\n", json_buffer);
 *     }
 *     else
 *     {
 *       printf("Failed to convert metadatum to JSON: %s\n", cardano_error_to_string(result));
 *     }
 *     // Free the buffer when done
 *     free(json_buffer);
 *   }
 * }
 *
 * // Clean up the metadatum object once done
 * cardano_metadatum_unref(&metadatum);
 * \endcode
 *
 * \see cardano_metadatum_get_json_size
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_metadatum_to_json(
  cardano_metadatum_t* metadatum,
  char*                json,
  size_t               json_size);

/**
 * \brief Serializes a metadatum into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_metadatum_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] metadatum A constant pointer to the \ref cardano_metadatum_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p metadatum or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_metadatum_t* metadatum = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_metadatum_to_cbor(metadatum, writer);
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
 * cardano_metadatum_unref(&metadatum);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_metadatum_to_cbor(
  const cardano_metadatum_t* metadatum,
  cardano_cbor_writer_t*     writer);

/**
 * \brief Retrieves the kind of the metadatum.
 *
 * This function retrieves the kind of a given \ref cardano_metadatum_t object and stores it in the provided
 * output parameter. The metadatum kind is defined in the \ref cardano_metadatum_kind_t enumeration, which
 * specifies whether the metadatum represents an integer, byte array, or other types.
 *
 * \param[in] metadatum A constant pointer to the \ref cardano_metadatum_t object from which
 *                        the kind is to be retrieved. The object must not be NULL.
 * \param[out] kind Pointer to a variable where the metadatum kind will be stored. This variable will
 *                  be set to the value from the \ref cardano_metadatum_kind_t enumeration.
 *
 * \return \ref CARDANO_SUCCESS if the kind was successfully retrieved, or an appropriate error code if the
 *         input is NULL or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_metadatum_t* metadatum = cardano_metadatum_new_bytes_from_hex(...);
 * cardano_metadatum_kind_t kind;
 * cardano_error_t result = cardano_metadatum_get_kind(metadatum, &kind);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   if (kind == CARDANO_METADATUM_KIND_INTEGER)
 *   {
 *     // Handle integer type metadatum
 *   }
 *   else if (kind == CARDANO_METADATUM_KIND_BYTES)
 *   {
 *     // Handle byte array type metadatum
 *   }
 *   // Add more cases as needed for other kinds
 * }
 *
 * // Clean up the metadatum object once done
 * cardano_metadatum_unref(&metadatum);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_metadatum_get_kind(
  const cardano_metadatum_t* metadatum,
  cardano_metadatum_kind_t*  kind);

/**
 * \brief Converts a metadatum object to a metadatum map object.
 *
 * This function converts a \ref cardano_metadatum_t object to a \ref cardano_metadatum_map_t object.
 * It essentially creates a metadatum map object from the given metadatum, allowing the conversion
 * between different types of metadatum. It returns an error code to indicate the success or failure of
 * the operation.
 *
 * \param[in] metadatum A constant pointer to the \ref cardano_metadatum_t object to be converted.
 *                        The object must not be NULL.
 * \param[out] map On successful conversion, this will point to a newly created
 *                 \ref cardano_metadatum_map_t object. This object represents a "strong reference"
 *                 to the metadatum_map, meaning that it is fully initialized and ready for use.
 *                 The caller is responsible for managing the lifecycle of this object.
 *                 Specifically, once the metadatum_map is no longer needed, the caller must release it
 *                 by calling \ref cardano_metadatum_map_unref.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_metadatum_t* metadatum = ...; // Assume metadatum is a valid metadatum map object
 * cardano_metadatum_map_t* map = NULL;
 * cardano_error_t result = cardano_metadatum_to_map(metadatum, &map);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the metadatum_map
 *
 *   // Once done, ensure to clean up and release the metadatum_map
 *   cardano_metadatum_map_unref(&map);
 * }
 *
 * // Clean up the metadatum object once done
 * cardano_metadatum_unref(&metadatum);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_metadatum_to_map(
  cardano_metadatum_t*      metadatum,
  cardano_metadatum_map_t** map);

/**
 * \brief Converts a metadatum object to a metadatum list object.
 *
 * This function converts a \ref cardano_metadatum_t object to a \ref cardano_metadatum_list_t object.
 * It essentially creates a metadatum list object from the given metadatum, allowing the conversion
 * between different types of metadatum. It returns an error code to indicate the success or failure of
 * the operation.
 *
 * \param[in] metadatum A constant pointer to the \ref cardano_metadatum_t object to be converted.
 * \param[out] list On successful conversion, this will point to a newly created
 *                  \ref cardano_metadatum_list_t object. This object represents a "strong reference"
 *                  to the metadatum_list, meaning that it is fully initialized and ready for use.
 *                  The caller is responsible for managing the lifecycle of this object.
 *                  Specifically, once the metadatum_list is no longer needed, the caller must release it
 *                  by calling \ref cardano_metadatum_list_unref.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_metadatum_t* metadatum = ...; // Assume metadatum is a valid metadatum list object
 * cardano_metadatum_list_t* list = NULL;
 * cardano_error_t result = cardano_metadatum_to_list(metadatum, &list);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the metadatum_list
 *
 *   // Once done, ensure to clean up and release the metadatum_list
 *   cardano_metadatum_list_unref(&list);
 * }
 *
 * // Clean up the metadatum object once done
 * cardano_metadatum_unref(&metadatum);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_metadatum_to_list(
  cardano_metadatum_t*       metadatum,
  cardano_metadatum_list_t** list);

/**
 * \brief Converts a metadatum object to a bigint object.
 *
 * This function converts a \ref cardano_metadatum_t object to a \ref cardano_bigint_t object.
 *
 * \param[in] metadatum A constant pointer to the \ref cardano_metadatum_t object to be converted.
 * \param[out] integer On successful conversion, this will point to a newly created \ref cardano_bigint_t object
 *                     representing the extracted integer value. The caller is responsible for managing the memory
 *                     of this object, including releasing it when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_metadatum_t* metadatum = cardano_metadatum_new_integer_from_string("987456321478523698745");
 * cardano_bigint_t* bigint = NULL;
 * cardano_error_t result = cardano_metadatum_to_integer(metadatum, &bigint);
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
 * cardano_metadatum_unref(&metadatum);
 * cardano_bigint_unref(&bigint);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_metadatum_to_integer(
  const cardano_metadatum_t* metadatum,
  cardano_bigint_t**         integer);

/**
 * \brief Converts a metadatum object to bounded bytes.
 *
 * This function converts a \ref cardano_metadatum_t object to bounded bytes.
 * It extracts the bytes from the given metadatum, allowing the conversion
 * between different types of metadatum. The resulting bytes are stored in a bounded bytes buffer.
 * It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] metadatum A constant pointer to the \ref cardano_metadatum_t object to be converted.
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
 * cardano_metadatum_t* metadatum = cardano_metadatum_new_bytes_from_hex("DEADBEEF", 8);
 * cardano_buffer_t* bounded_bytes = NULL;
 * cardano_error_t result = cardano_metadatum_to_bounded_bytes(metadatum, &bounded_bytes);
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
 * // Clean up the metadatum object once done
 * cardano_metadatum_unref(&metadatum);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_metadatum_to_bounded_bytes(
  cardano_metadatum_t* metadatum,
  cardano_buffer_t**   bounded_bytes);

/**
 * \brief Computes the size needed for the null-terminated string representation of a metadatum.
 *
 * This function calculates the size in bytes required to represent a \ref cardano_metadatum_t
 * as a null-terminated string, including the null terminator. This is useful for allocating
 * an appropriately sized buffer before converting the metadatum to a string using
 * \ref cardano_metadatum_to_string.
 *
 * \param[in] metadatum A pointer to the \ref cardano_metadatum_t instance whose string size is being queried.
 *                      This parameter must not be NULL.
 *
 * \return The size in bytes required to represent the metadatum as a null-terminated string,
 *         including the null terminator. Returns 0 if \p metadatum is NULL or if an error occurs.
 *
 * Usage Example:
 * \code{.c}
 * // Assume metadatum is already initialized
 * cardano_metadatum_t* metadatum = ...;
 *
 * size_t size = cardano_metadatum_get_string_size(metadatum);
 *
 * if (size > 0)
 * {
 *     // size now contains the size needed to store the string, including null terminator
 *     // You can allocate a buffer of this size for conversion
 * }
 * else
 * {
 *     printf("Failed to get the string size.\n");
 *     // Handle error
 * }
 * \endcode
 *
 * \see cardano_metadatum_to_string
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_metadatum_get_string_size(cardano_metadatum_t* metadatum);

/**
 * \brief Converts a metadatum object to a null-terminated C string.
 *
 * This function converts a \ref cardano_metadatum_t object to its string representation.
 * The resulting string is stored in the provided buffer, which must be pre-allocated by the caller.
 * The buffer must be large enough to hold the entire string, including the null terminator.
 *
 * To determine the required buffer size (including the null terminator), use
 * \ref cardano_metadatum_get_string_size before calling this function.
 *
 * \param[in] metadatum A pointer to the \ref cardano_metadatum_t object to be converted.
 *                      This parameter must not be NULL.
 * \param[out] buffer A pointer to a character array where the null-terminated string will be stored.
 *                    The buffer must be allocated by the caller and have a size of at least \p buffer_size bytes.
 *                    This parameter must not be NULL.
 * \param[in] buffer_size The size of the \p buffer in bytes, including space for the null terminator.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful.
 *         Returns \ref CARDANO_ERROR_POINTER_IS_NULL if \p metadatum or \p buffer is NULL.
 *         Returns \ref CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE if \p buffer_size is insufficient to hold the string.
 *
 * Usage Example:
 * \code{.c}
 * // Assume metadatum is already initialized
 * cardano_metadatum_t* metadatum = ...;
 *
 * // Get the size needed for the string (including null terminator)
 * size_t string_size = cardano_metadatum_get_string_size(metadatum);
 * if (string_size == 0)
 * {
 *     printf("Failed to get the string size.\n");
 *     // Handle error
 * }
 * else
 * {
 *     // Allocate buffer for the string
 *     char* buffer = (char*)malloc(string_size);
 *     if (buffer == NULL)
 *     {
 *         printf("Memory allocation failed.\n");
 *         // Handle memory allocation failure
 *     }
 *     else
 *     {
 *         // Convert the metadatum to string
 *         cardano_error_t result = cardano_metadatum_to_string(metadatum, buffer, string_size);
 *         if (result == CARDANO_SUCCESS)
 *         {
 *             printf("Metadatum as string: %s\n", buffer);
 *         }
 *         else
 *         {
 *             printf("Failed to convert metadatum to string: %s\n", cardano_error_to_string(result));
 *         }
 *         // Free the buffer when done
 *         free(buffer);
 *     }
 * }
 *
 * // Clean up the metadatum object once done
 * cardano_metadatum_unref(&metadatum);
 * \endcode
 *
 * \see cardano_metadatum_get_string_size
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_metadatum_to_string(
  cardano_metadatum_t* metadatum,
  char*                buffer,
  size_t               buffer_size);

/**
 * \brief Checks if two metadatum objects are equal.
 *
 * This function compares two \ref cardano_metadatum_t objects for equality.
 * It checks if the contents of the two metadatum objects are identical.
 *
 * \param[in] lhs A constant pointer to the first \ref cardano_metadatum_t object to be compared.
 * \param[in] rhs A constant pointer to the second \ref cardano_metadatum_t object to be compared.
 *
 * \return \c true if the two metadatum objects are equal (have the same contents), \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_metadatum_t* metadatum1 = cardano_metadatum_new_bytes_from_hex("DEADBEEF", 8);
 * cardano_metadatum_t* metadatum2 = cardano_metadatum_new_bytes_from_hex("DEADBEEF", 8);
 *
 * if (cardano_metadatum_equals(metadatum1, metadatum2))
 * {
 *   printf("metadatum1 is equal to metadatum2\n");
 * }
 * else
 * {
 *   printf("metadatum1 is not equal to metadatum2\n");
 * }
 *
 * // Clean up the metadatum objects once done
 * cardano_metadatum_unref(&metadatum1);
 * cardano_metadatum_unref(&metadatum2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_metadatum_equals(const cardano_metadatum_t* lhs, const cardano_metadatum_t* rhs);

/**
 * \brief Decrements the reference count of a metadatum object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_metadatum_t object
 * by decreasing its reference count. When the reference count reaches zero, the metadatum is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] metadatum A pointer to the pointer of the metadatum object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_metadatum_t* metadatum = cardano_metadatum_new();
 *
 * // Perform operations with the metadatum...
 *
 * cardano_metadatum_unref(&metadatum);
 * // At this point, metadatum is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_metadatum_unref, the pointer to the \ref cardano_metadatum_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_metadatum_unref(cardano_metadatum_t** metadatum);

/**
 * \brief Increases the reference count of the cardano_metadatum_t object.
 *
 * This function is used to manually increment the reference count of a metadatum
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_metadatum_unref.
 *
 * \param metadatum A pointer to the metadatum object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming metadatum is a previously created metadatum object
 *
 * cardano_metadatum_ref(metadatum);
 *
 * // Now metadatum can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_metadatum_ref there is a corresponding
 * call to \ref cardano_metadatum_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_metadatum_ref(cardano_metadatum_t* metadatum);

/**
 * \brief Retrieves the current reference count of the cardano_metadatum_t object.
 *
 * This function returns the number of active references to a metadatum object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_metadatum_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param metadatum A pointer to the metadatum object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified metadatum object. If the object
 * is properly managed (i.e., every \ref cardano_metadatum_ref call is matched with a
 * \ref cardano_metadatum_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming metadatum is a previously created metadatum object
 *
 * size_t ref_count = cardano_metadatum_refcount(metadatum);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_metadatum_refcount(const cardano_metadatum_t* metadatum);

/**
 * \brief Sets the last error message for a given metadatum object.
 *
 * Records an error message in the metadatum's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] metadatum A pointer to the \ref cardano_metadatum_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the metadatum's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_metadatum_set_last_error(cardano_metadatum_t* metadatum, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific metadatum.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_metadatum_set_last_error for the given
 * metadatum. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] metadatum A pointer to the \ref cardano_metadatum_t instance whose last error
 *                   message is to be retrieved. If the metadatum is NULL, the function
 *                   returns a generic error message indicating the null metadatum.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified metadatum. If the metadatum is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_metadatum_set_last_error for the same metadatum, or until
 *       the metadatum is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_metadatum_get_last_error(const cardano_metadatum_t* metadatum);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_METADATUM_H