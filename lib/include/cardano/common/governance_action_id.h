/**
 * \file governance_action_id.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_GOVERNANCE_ACTION_ID_H
#define BIGLUP_LABS_INCLUDE_CARDANO_GOVERNANCE_ACTION_ID_H

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
 * \brief Each governance action that is accepted on the chain will be assigned a unique
 * identifier, consisting of the transaction hash that created it and the index within
 * the transaction body that points to it.
 */
typedef struct cardano_governance_action_id_t cardano_governance_action_id_t;

/**
 * \brief Creates and initializes a new instance of a governance action id.
 *
 * This function allocates and initializes a new instance of \ref cardano_governance_action_id_t,
 * using the provided transaction hash and index. It returns an error code to indicate success
 * or failure of the operation.
 *
 * \param[in] transaction_hash A pointer to \ref cardano_blake2b_hash_t representing the transaction hash associated
 *             with this governance action id. The hash must be properly initialized before being
 *             passed to this function.
 * \param[in] index The index within the transaction body that points this governance action.
 * \param[out] governance_action_id On successful initialization, this will point to a newly created
 *             \ref cardano_governance_action_id_t object. This object represents a "strong reference"
 *             to the governance_action_id, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the governance_action_id is no longer needed, the caller must release it
 *             by calling \ref cardano_governance_action_id_unref.
 *
 * \return \ref CARDANO_SUCCESS if the governance_action_id was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_t* hash = { ... };  // Assume hash is initialized here
 * cardano_governance_action_id_t* governance_action_id = NULL;
 *
 * // Attempt to create a new governance_action_id
 * cardano_error_t result = cardano_governance_action_id_new(hash, 1, &governance_action_id);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the governance_action_id
 *
 *   // Once done, ensure to clean up and release the governance_action_id
 *   cardano_governance_action_id_unref(&governance_action_id);
 * }
 *
 * cardano_blake2b_hash_unref(&hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_governance_action_id_new(
  const cardano_blake2b_hash_t*    transaction_hash,
  uint64_t                         index,
  cardano_governance_action_id_t** governance_action_id);

/**
 * \brief Parses a Bech32-encoded governance action ID and initializes a corresponding governance action ID object.
 *
 * This function takes a Bech32-encoded string representation of a governance action ID, as specified in CIP-129,
 * and creates a \ref cardano_governance_action_id_t object.
 *
 * For example:
 * - Bech32-encoded input: `gov_action1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqpzklpgpf`
 * - Resulting governance action ID: Transaction ID (`0000000000000000000000000000000000000000000000000000000000000000`),
 *   Index (`11`).
 *
 * \param[in] data A pointer to the Bech32-encoded string. This parameter must not be NULL.
 * \param[in] size The size of the Bech32-encoded string in bytes.
 * \param[out] action_id A pointer to a \ref cardano_governance_action_id_t pointer. On successful parsing, this pointer
 *             will point to a newly created governance action ID object. The caller is responsible for managing the
 *             lifecycle of this object and must release it using \ref cardano_governance_action_id_unref.
 *
 * \return \ref CARDANO_SUCCESS if the parsing was successful and the governance action ID object was created.
 *         Returns an appropriate error code otherwise:
 *         - \ref CARDANO_ERROR_POINTER_IS_NULL if any input pointer is NULL.
 *         - \ref CARDANO_ERROR_INVALID_ARGUMENT if the input string is not a valid Bech32-encoded governance action ID.
 *
 * Usage Example
 * \code{.c}
 * const char* bech32_string = "gov_action1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqpzklpgpf";
 * cardano_governance_action_id_t* action_id = NULL;
 *
 * cardano_error_t result = cardano_governance_action_id_from_bech32(bech32_string, strlen(bech32_string), &action_id);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Governance Action ID successfully parsed.\n");
 *   // Use the action_id as needed
 *
 *   // Free the governance action ID object when done
 *   cardano_governance_action_id_unref(&action_id);
 * }
 * else
 * {
 *   printf("Failed to parse governance action ID from Bech32: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_governance_action_id_from_bech32(
  const char*                      data,
  size_t                           size,
  cardano_governance_action_id_t** action_id);

/**
 * \brief Creates a governance action id from a hexadecimal transaction hash string.
 *
 * This function constructs a \ref cardano_governance_action_id_t object by interpreting the provided
 * hexadecimal string as a hash value and associating it with a specified index. It
 * returns an error code indicating the success or failure of the operation.
 *
 * \param[in] hex A pointer to a character array containing the hexadecimal representation of the transaction hash.
 * \param[in] hex_size The size of the hexadecimal string in bytes.
 * \param[in] index The index within the transaction body that points this governance action.
 * \param[out] governance_action_id On successful initialization, this will point to a newly created
 *                 \ref cardano_governance_action_id_t object. This object represents a "strong reference"
 *                 to the governance_action_id, meaning that it is fully initialized and ready for use.
 *                 The caller is responsible for managing the lifecycle of this object,
 *                 specifically, once the governance_action_id is no longer needed, the caller must release it
 *                 by calling \ref cardano_governance_action_id_unref.
 *
 * \return \ref CARDANO_SUCCESS if the governance_action_id was successfully created, or an appropriate error code
 *         indicating the reason for failure.
 *
 * \note The function assumes that the hexadecimal string is valid and correctly formatted.
 *
 * Usage Example:
 * \code{.c}
 * const char* hash_hex = "abcdef1234567890abcdef1234567890abcdef12....";
 * size_t hex_size = strlen(hash_hex);
 * cardano_governance_action_id_t* governance_action_id = NULL;
 *
 * // Attempt to create a new governance_action_id from a hexadecimal hash
 * cardano_error_t result = cardano_governance_action_id_from_hash_hex(hash_hex, hex_size, 1, &governance_action_id);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the governance_action_id
 *
 *   // Once done, ensure to clean up and release the governance_action_id
 *   cardano_governance_action_id_unref(&governance_action_id);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_governance_action_id_from_hash_hex(
  const char*                      hex,
  size_t                           hex_size,
  uint64_t                         index,
  cardano_governance_action_id_t** governance_action_id);

/**
 * \brief Creates a governance action id from a byte array representing a transaction hash.
 *
 * This function constructs a \ref cardano_governance_action_id_t object by using the provided
 * byte array as a hash value and associating it with a specified index. It returns an error
 * code indicating the success or failure of the operation.
 *
 * \param[in] data A pointer to the byte array containing the hash data.
 * \param[in] data_size The size of the byte array in bytes.
 * \param[in] index The index within the transaction body that points this governance action.
 * \param[out] governance_action_id On successful initialization, this will point to a newly created
 *                 \ref cardano_governance_action_id_t object. This object represents a "strong reference"
 *                 to the governance_action_id, meaning that it is fully initialized and ready for use.
 *                 The caller is responsible for managing the lifecycle of this object,
 *                 specifically, once the governance_action_id is no longer needed, the caller must release it
 *                 by calling \ref cardano_governance_action_id_unref.
 *
 * \return \ref CARDANO_SUCCESS if the governance_action_id was successfully created, or an appropriate error code
 *         indicating the reason for failure.
 *
 * Usage Example:
 * \code{.c}
 * const byte_t hash_data[] = { 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef ... };
 * size_t data_size = sizeof(hash_data);
 * cardano_governance_action_id_t* governance_action_id = NULL;
 *
 * // Attempt to create a new governance_action_id from byte array hash
 * cardano_error_t result = cardano_governance_action_id_from_hash_bytes(hash_data, data_size, 1, &governance_action_id);
 *
 * if (result == CARDANO_SUCCESS && governance_action_id)
 * {
 *   // Use the governance_action_id
 *
 *   // Once done, ensure to clean up and release the governance_action_id
 *   cardano_governance_action_id_unref(&governance_action_id);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_governance_action_id_from_hash_bytes(
  const byte_t*                    data,
  size_t                           data_size,
  uint64_t                         index,
  cardano_governance_action_id_t** governance_action_id);

/**
 * \brief Creates a governance_action_id from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_governance_action_id_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a governance action id.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded governance action id data.
 * \param[out] governance_action_id A pointer to a pointer of \ref cardano_governance_action_id_t that will be set to the address
 *                        of the newly created governance action id object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the governance_action_id was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_governance_action_id_t object by calling
 *       \ref cardano_governance_action_id_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_governance_action_id_t* governance_action_id = NULL;
 *
 * cardano_error_t result = cardano_governance_action_id_from_cbor(reader, &governance_action_id);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the governance_action_id
 *
 *   // Once done, ensure to clean up and release the governance_action_id
 *   cardano_governance_action_id_unref(&governance_action_id);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode governance_action_id: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_governance_action_id_from_cbor(cardano_cbor_reader_t* reader, cardano_governance_action_id_t** governance_action_id);

/**
 * \brief Serializes a governance action id into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_governance_action_id_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] governance_action_id A constant pointer to the \ref cardano_governance_action_id_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p governance_action_id or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_governance_action_id_t* governance_action_id = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_governance_action_id_to_cbor(governance_action_id, writer);
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
 * cardano_governance_action_id_unref(&governance_action_id);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_governance_action_id_to_cbor(
  const cardano_governance_action_id_t* governance_action_id,
  cardano_cbor_writer_t*                writer);

/**
 * \brief Computes the required buffer size for a CIP-29 compliant Bech32 representation of a governance action ID.
 *
 * This function calculates the minimum buffer size, including null termination, needed to store the Bech32 string representation
 * of a governance action ID. The Bech32 representation combines a transaction ID (32 bytes) and an index (1 byte) into a single
 * string with the prefix `gov_action`.
 *
 * \param[in] governance_action_id A pointer to an initialized \ref cardano_governance_action_id_t object representing the
 *                                 governance action ID. This parameter must not be NULL.
 *
 * \return The required buffer size in bytes, including space for the null-terminator.
 *
 * Usage Example
 * \code{.c}
 * cardano_governance_action_id_t* gov_action_id = ...; // Assume initialized
 * size_t required_size = cardano_governance_action_id_get_bech32_size(gov_action_id);
 *
 * char* bech32_str = (char*)malloc(required_size);
 *
 * if (bech32_str != NULL)
 * {
 *   // Use the buffer for encoding the governance action ID into Bech32 format
 *   free(bech32_str);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t
cardano_governance_action_id_get_bech32_size(
  const cardano_governance_action_id_t* governance_action_id);

/**
 * \brief Converts a governance action ID to its CIP-29 compliant Bech32 representation.
 *
 * This function encodes a governance action ID, which combines a transaction ID (32 bytes) and an index (1 byte), into its
 * Bech32 string representation. The resulting Bech32 string uses the prefix `gov_action` as defined in CIP-29 and is stored
 * in the provided buffer.
 *
 * \param[in] governance_action_id A pointer to an initialized \ref cardano_governance_action_id_t object representing the
 *                                 governance action ID to be converted. This parameter must not be NULL.
 * \param[out] data A pointer to the buffer where the resulting Bech32 string will be stored. The buffer must have sufficient
 *                  size, as determined by \ref cardano_governance_action_id_get_bech32_size.
 * \param[in] size The size of the provided buffer in bytes, including space for the null-terminator.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful and the Bech32 string is stored in `data`. Returns an appropriate error code if the operation fails.
 *
 * Usage Example
 * \code{.c}
 * cardano_governance_action_id_t* gov_action_id = ...; // Assume initialized
 * size_t required_size = cardano_governance_action_id_get_bech32_size(gov_action_id);
 *
 * char* bech32_str = (char*)malloc(required_size);
 *
 * if (bech32_str != NULL)
 * {
 *   cardano_error_t result = cardano_governance_action_id_to_bech32(gov_action_id, bech32_str, required_size);
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     printf("Governance Action Bech32: %s\n", bech32_str);
 *   }
 *   else
 *   {
 *     printf("Error converting governance action ID to Bech32: %s\n", cardano_error_to_string(result));
 *   }
 *   free(bech32_str);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_governance_action_id_to_bech32(
  const cardano_governance_action_id_t* governance_action_id,
  char*                                 data,
  size_t                                size);

/**
 * \brief Retrieves the Bech32 string representation of a governance action ID.
 *
 * This function returns the Bech32-encoded string representation of a governance action ID as specified in CIP-0129.
 * The governance action ID is derived from the transaction ID and index, and the Bech32 encoding includes the appropriate
 * prefix (`"gov_action"`) followed by the encoded ID.
 *
 * For example:
 * - Transaction ID: `0000000000000000000000000000000000000000000000000000000000000000`
 * - Index: `11`
 * - Bech32-encoded governance action ID: `gov_action1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqpzklpgpf`
 *
 * \param[in] governance_action_id A pointer to an initialized \ref cardano_governance_action_id_t object representing the governance action ID.
 *                                 This parameter must not be NULL.
 *
 * \return A constant pointer to a null-terminated string containing the Bech32-encoded governance action ID.
 *         The returned string is managed internally by the library and must not be modified or freed by the caller.
 *         Returns `NULL` if the input pointer is invalid.
 *
 * ### Usage Example
 * \code{.c}
 * const cardano_governance_action_id_t* gov_action_id = ...; // Assume initialized
 * const char* bech32_string = cardano_governance_action_id_get_string(gov_action_id);
 *
 * if (bech32_string != NULL)
 * {
 *   printf("Governance Action ID (Bech32): %s\n", bech32_string);
 * }
 * else
 * {
 *   printf("Failed to retrieve governance action ID in Bech32 format.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char*
cardano_governance_action_id_get_string(
  const cardano_governance_action_id_t* governance_action_id);

/**
 * \brief Retrieves the transaction hash associated with a governance action id.
 *
 * This function provides access to the hash part of a \ref cardano_governance_action_id_t object.
 * It returns a new reference to a \ref cardano_blake2b_hash_t object representing the hash.
 * This allows the hash to be used independently of the original governance_action_id object. The
 * reference count of the hash object is increased by one, making it the caller's responsibility
 * to release it by calling \ref cardano_blake2b_hash_unref when it is no longer needed.
 *
 * \param governance_action_id A constant pointer to the \ref cardano_governance_action_id_t object from which
 *                   the hash is to be retrieved.
 *
 * \return A pointer to a new \ref cardano_blake2b_hash_t object containing the hash.
 *         If the input governance_action_id is NULL, returns NULL. The caller is responsible for
 *         managing the lifecycle of this object, including releasing it with
 *         \ref cardano_blake2b_hash_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_governance_action_id_t* original_governance_action_id = cardano_governance_action_id_new(...);
 * cardano_blake2b_hash_t* hash_governance_action_id = cardano_governance_action_id_get_hash(original_governance_action_id);
 *
 * if (hash_governance_action_id)
 * {
 *   // Use the hash governance_action_id
 *
 *   // Once done, ensure to clean up and release the hash governance_action_id
 *   cardano_blake2b_hash_unref(&hash_governance_action_id);
 * }
 *
 * // Release the original governance_action_id after use
 * cardano_governance_action_id_unref(&original_governance_action_id);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t* cardano_governance_action_id_get_hash(const cardano_governance_action_id_t* governance_action_id);

/**
 * \brief Retrieves the size of the hash bytes stored in the governance action id.
 *
 * This function computes the size of the hash bytes stored within a \ref cardano_governance_action_id_t object.
 * It is particularly useful for determining the buffer size needed to store the hash bytes when
 * retrieving them via \ref cardano_governance_action_id_get_hash_bytes.
 *
 * \param[in] governance_action_id A constant pointer to the \ref cardano_governance_action_id_t object from which
 *                       the size of the hash bytes is to be retrieved.
 *
 * \return The size of the hash bytes. If the governance_action_id is NULL the function returns zero.
 *
 * Usage Example:
 * \code{.c}
 * cardano_governance_action_id_t* governance_action_id = cardano_governance_action_id_new(...);
 * size_t hash_size = cardano_governance_action_id_get_hash_bytes_size(governance_action_id);
 *
 * if (hash_size > 0)
 * {
 *   byte_t* hash_bytes = malloc(hash_size);
 *
 *   if (hash_bytes)
 *   {
 *     // Proceed to get the hash bytes
 *   }
 * }
 * cardano_governance_action_id_unref(&governance_action_id);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_governance_action_id_get_hash_bytes_size(const cardano_governance_action_id_t* governance_action_id);

/**
 * \brief Retrieves the byte array representation of the transaction hash from a governance action id.
 *
 * This function accesses the byte representation of the hash associated with a given
 * \ref cardano_governance_action_id_t object. It provides a direct pointer to the internal byte array
 * representing the hash, which should not be modified or freed by the caller.
 *
 * \param governance_action_id A constant pointer to the \ref cardano_governance_action_id_t object from which
 *                   the hash bytes are to be retrieved.
 *
 * \return A pointer to a constant byte array containing the hash data. If the input governance_action_id
 *         is NULL, returns NULL. The data remains valid as long as the governance_action_id object is not
 *         freed or modified.
 *
 * Usage Example:
 * \code{.c}
 * cardano_governance_action_id_t* governance_action_id = cardano_governance_action_id_new(...);
 * const byte_t* hash_bytes = cardano_governance_action_id_get_hash_bytes(governance_action_id);
 *
 * if (hash_bytes)
 * {
 *   // Use the hash bytes for operations like comparison or display
 * }
 *
 * // Clean up the governance_action_id object once done
 * cardano_governance_action_id_unref(&governance_action_id);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const byte_t* cardano_governance_action_id_get_hash_bytes(const cardano_governance_action_id_t* governance_action_id);

/**
 * \brief Retrieves the size needed for the hexadecimal string representation of the governance_action_id's hash.
 *
 * This function calculates the size required to store the hexadecimal string representation of the hash
 * associated with a \ref cardano_governance_action_id_t object. This size includes the space needed for the null-terminator.
 *
 * \param[in] governance_action_id A constant pointer to the \ref cardano_governance_action_id_t object whose hash size is to be determined.
 *
 * \return The size in bytes required to store the hexadecimal representation of the hash, including the null terminator.
 *
 * Usage Example:
 * \code{.c}
 * cardano_governance_action_id_t* governance_action_id = cardano_governance_action_id_new(...);
 * size_t hex_size = cardano_governance_action_id_get_hash_hex_size(governance_action_id);
 * char* hex_string = malloc(hex_size);
 *
 * if (hex_string)
 * {
 *   // Now use hex_string to get the hash or do other operations
 *   free(hex_string);
 * }
 *
 * cardano_governance_action_id_unref(&governance_action_id);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_governance_action_id_get_hash_hex_size(const cardano_governance_action_id_t* governance_action_id);

/**
 * \brief Retrieves the hexadecimal string representation of the hash from a governance action id.
 *
 * This function provides access to the hexadecimal (hex) string representation of the hash
 * associated with a given \ref cardano_governance_action_id_t object. It returns a direct pointer to the
 * internal hex string which should not be modified or freed by the caller.
 *
 * \param governance_action_id A constant pointer to the \ref cardano_governance_action_id_t object from which
 *                   the hex string of the hash is to be retrieved. The object must not be NULL.
 *
 * \return A pointer to a constant character array containing the hex representation of the hash.
 *         If the input governance_action_id is NULL, returns NULL. The data remains valid as long as the
 *         governance_action_id object is not freed or modified.
 *
 * Usage Example:
 * \code{.c}
 * cardano_governance_action_id_t* governance_action_id = cardano_governance_action_id_new(...);
 * const char* hash_hex = cardano_governance_action_id_get_hash_hex(governance_action_id);
 *
 * if (hash_hex)
 * {
 *   // Use the hash hex for operations like logging, display, or comparison
 * }
 *
 * // Clean up the governance_action_id object once done
 * cardano_governance_action_id_unref(&governance_action_id);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_governance_action_id_get_hash_hex(const cardano_governance_action_id_t* governance_action_id);

/**
 * \brief Retrieves the index of the governance action id.
 *
 * This function retrieves the index of a given \ref cardano_governance_action_id_t object and stores it in the provided
 * output parameter.
 *
 * \param[in] governance_action_id A constant pointer to the \ref cardano_governance_action_id_t object from which
 *                                 the index is to be retrieved. The object must not be NULL.
 * \param[out] index Pointer to a variable where the governance action id index will be stored.
 *
 * \return \ref CARDANO_SUCCESS if the index was successfully retrieved, or an appropriate error code if the
 *         input is NULL or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_governance_action_id_t* governance_action_id = cardano_governance_action_id_new(...);
 * uint64_t index = 0;
 * cardano_error_t result = cardano_governance_action_id_get_index(governance_action_id, &index);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *  // Handle governance action id index
 * }
 *
 * // Clean up the governance_action_id object once done
 * cardano_governance_action_id_unref(&governance_action_id);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_governance_action_id_get_index(const cardano_governance_action_id_t* governance_action_id, uint64_t* index);

/**
 * \brief Sets the index of the governance action id.
 *
 * This function assigns a new index to an existing \ref cardano_governance_action_id_t object.
 *
 * \param[in,out] governance_action_id A pointer to the \ref cardano_governance_action_id_t object whose index is to be set.
 * \param[in] index The new index of the governance action id.
 *
 * \return Returns \ref CARDANO_SUCCESS if the type was successfully set. Returns \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the \p governance_action_id pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_governance_action_id_t* governance_action_id = cardano_governance_action_id_new(...);
 * cardano_error_t result = cardano_governance_action_id_set_index(governance_action_id, 1);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // governance_action_id index updated successfully
 * }
 *
 * cardano_governance_action_id_unref(&governance_action_id);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_governance_action_id_set_index(cardano_governance_action_id_t* governance_action_id, uint64_t index);

/**
 * \brief Sets the hash for a governance action id.
 *
 * This function assigns a new hash to an existing \ref cardano_governance_action_id_t object. The function copies the
 * provided hash into the governance_action_id, so the original hash object may be modified or freed after this operation without
 * affecting the governance_action_id's hash.
 *
 * \param[in,out] governance_action_id A pointer to the \ref cardano_governance_action_id_t object whose hash is to be set.
 *                           This object must have been previously created and not yet freed.
 * \param[in] hash A pointer to a \ref cardano_blake2b_hash_t object containing the new hash to be set.
 *                 This parameter must not be NULL.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the hash was successfully set. If the \p governance_action_id or \p hash is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_governance_action_id_t* governance_action_id = ...;
 * cardano_blake2b_hash_t* new_hash = cardano_blake2b_compute_hash(...);
 *
 * cardano_error_t result = cardano_governance_action_id_set_hash(governance_action_id, new_hash);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The hash was successfully set
 * }
 *
 * // Clean up
 * cardano_governance_action_id_unref(&governance_action_id);
 * cardano_blake2b_hash_unref(&new_hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_governance_action_id_set_hash(cardano_governance_action_id_t* governance_action_id, const cardano_blake2b_hash_t* hash);

/**
 * \brief Checks if two governance_action_id objects are equal.
 *
 * This function compares two \ref cardano_governance_action_id_t objects for equality.
 * It checks if the contents of the two governance_action_id objects are identical.
 *
 * \param[in] lhs A constant pointer to the first \ref cardano_governance_action_id_t object to be compared.
 * \param[in] rhs A constant pointer to the second \ref cardano_governance_action_id_t object to be compared.
 *
 * \return \c true if the two governance_action_id objects are equal (have the same contents), \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_governance_action_id_t* governance_action_id1 = ...;
 * cardano_governance_action_id_t* governance_action_id2 = ...;
 *
 * if (cardano_governance_action_id_equals(governance_action_id1, governance_action_id2))
 * {
 *   printf("governance_action_id1 is equal to governance_action_id2\n");
 * }
 * else
 * {
 *   printf("governance_action_id1 is not equal to governance_action_id2\n");
 * }
 *
 * // Clean up the governance_action_id objects once done
 * cardano_governance_action_id_unref(&governance_action_id1);
 * cardano_governance_action_id_unref(&governance_action_id2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_governance_action_id_equals(const cardano_governance_action_id_t* lhs, const cardano_governance_action_id_t* rhs);

/**
 * \brief Decrements the reference count of a governance_action_id object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_governance_action_id_t object
 * by decreasing its reference count. When the reference count reaches zero, the governance_action_id is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] governance_action_id A pointer to the pointer of the governance_action_id object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_governance_action_id_t* governance_action_id = cardano_governance_action_id_new();
 *
 * // Perform operations with the governance action id...
 *
 * cardano_governance_action_id_unref(&governance_action_id);
 * // At this point, governance_action_id is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_governance_action_id_unref, the pointer to the \ref cardano_governance_action_id_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_governance_action_id_unref(cardano_governance_action_id_t** governance_action_id);

/**
 * \brief Increases the reference count of the cardano_governance_action_id_t object.
 *
 * This function is used to manually increment the reference count of a governance_action_id
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_governance_action_id_unref.
 *
 * \param governance_action_id A pointer to the governance_action_id object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming governance_action_id is a previously created governance_action_id object
 *
 * cardano_governance_action_id_ref(governance_action_id);
 *
 * // Now governance_action_id can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_governance_action_id_ref there is a corresponding
 * call to \ref cardano_governance_action_id_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_governance_action_id_ref(cardano_governance_action_id_t* governance_action_id);

/**
 * \brief Retrieves the current reference count of the cardano_governance_action_id_t object.
 *
 * This function returns the number of active references to a governance_action_id object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_governance_action_id_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param governance_action_id A pointer to the governance_action_id object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified governance_action_id object. If the object
 * is properly managed (i.e., every \ref cardano_governance_action_id_ref call is matched with a
 * \ref cardano_governance_action_id_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming governance_action_id is a previously created governance_action_id object
 *
 * size_t ref_count = cardano_governance_action_id_refcount(governance_action_id);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_governance_action_id_refcount(const cardano_governance_action_id_t* governance_action_id);

/**
 * \brief Sets the last error message for a given governance_action_id object.
 *
 * Records an error message in the governance_action_id's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] governance_action_id A pointer to the \ref cardano_governance_action_id_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the governance_action_id's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_governance_action_id_set_last_error(cardano_governance_action_id_t* governance_action_id, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific governance action id.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_governance_action_id_set_last_error for the given
 * governance action id. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] governance_action_id A pointer to the \ref cardano_governance_action_id_t instance whose last error
 *                   message is to be retrieved. If the governance_action_id is NULL, the function
 *                   returns a generic error message indicating the null governance action id.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified governance action id. If the governance_action_id is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_governance_action_id_set_last_error for the same governance_action_id, or until
 *       the governance_action_id is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_governance_action_id_get_last_error(const cardano_governance_action_id_t* governance_action_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_GOVERNANCE_ACTION_ID_H
