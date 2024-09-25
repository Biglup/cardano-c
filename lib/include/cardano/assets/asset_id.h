/**
 * \file asset_id.h
 *
 * \author angel.castillo
 * \date   Sep 14, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_ASSET_ID_H
#define BIGLUP_LABS_INCLUDE_CARDANO_ASSET_ID_H

/* INCLUDES ******************************************************************/

#include <cardano/assets/asset_name.h>
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
 * \brief Represents an asset identifier in the Cardano blockchain.
 *
 * The `cardano_asset_id_t` structure encapsulates the unique identification of a native asset on the Cardano blockchain.
 * Each asset is uniquely identified by a combination of its policy ID and asset name.
 */
typedef struct cardano_asset_id_t cardano_asset_id_t;

/**
 * \brief Creates and initializes a new instance of a Cardano asset identifier.
 *
 * This function allocates and initializes a new instance of a \ref cardano_asset_id_t object,
 * which represents a unique asset identifier in the Cardano blockchain.
 * The asset ID is composed of a policy ID and an asset name.
 *
 * - **Policy ID**: A cryptographic hash (Blake2b hash) that identifies the minting policy.
 * - **Asset Name**: A user-defined name (max 32 bytes) associated with the asset.
 *
 * The function ensures that a valid asset identifier is created by linking the provided policy ID and asset name.
 *
 * \param[in] policy_id A pointer to an initialized \ref cardano_blake2b_hash_t object representing the minting policy ID.
 *                      This value must not be NULL.
 * \param[in] asset_name A pointer to an initialized \ref cardano_asset_name_t object representing the name of the asset.
 *                       This value must not be NULL.
 * \param[out] asset_id On successful creation, this will point to a newly created \ref cardano_asset_id_t object
 *                      representing the asset. The caller is responsible for managing the lifecycle of this object by calling
 *                      \ref cardano_asset_id_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the asset ID was
 *         successfully created, or an appropriate error code indicating the failure reason, such as:
 *         - \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *         - \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if memory allocation for the asset ID failed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_t* policy_id = ...; // Assume policy ID is initialized
 * cardano_asset_name_t* asset_name = ...;  // Assume asset name is initialized
 * cardano_asset_id_t* asset_id = NULL;
 *
 * // Create a new asset ID
 * cardano_error_t result = cardano_asset_id_new(policy_id, asset_name, &asset_id);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the asset ID
 *
 *   // Once done, release the asset ID
 *   cardano_asset_id_unref(&asset_id);
 * }
 * else
 * {
 *   printf("Failed to create asset ID: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_asset_id_new(
  cardano_blake2b_hash_t* policy_id,
  cardano_asset_name_t*   asset_name,
  cardano_asset_id_t**    asset_id);

/**
 * \brief Creates a new instance of the Cardano native asset identifier for Lovelace.
 *
 * This function allocates and initializes a new instance of a \ref cardano_asset_id_t object that represents
 * the Cardano native coin, Lovelace. Lovelace is the smallest denomination of the ADA cryptocurrency, and it
 * does not have an associated asset name or a policy ID like other assets. Instead, Lovelace is identified
 * by the special identifier "lovelace."
 *
 * \param[out] asset_id On successful creation, this will point to a newly created \ref cardano_asset_id_t object
 *                      representing the Lovelace asset. The caller is responsible for managing the lifecycle of this object
 *                      and must release it by calling \ref cardano_asset_id_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the Lovelace asset ID
 *         was successfully created, or an appropriate error code indicating the failure reason, such as:
 *         - \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if memory allocation for the asset ID failed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_asset_id_t* lovelace_asset_id = NULL;
 *
 * // Create a new asset ID for Lovelace
 * cardano_error_t result = cardano_asset_id_new_lovelace(&lovelace_asset_id);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the Lovelace asset ID
 *
 *   // Once done, release the asset ID
 *   cardano_asset_id_unref(&lovelace_asset_id);
 * }
 * else
 * {
 *   printf("Failed to create Lovelace asset ID: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_asset_id_new_lovelace(cardano_asset_id_t** asset_id);

/**
 * \brief Creates and initializes a new instance of a Cardano asset ID from byte data.
 *
 * This function allocates and initializes a new instance of a \ref cardano_asset_id_t object,
 * which represents a Cardano native asset. The asset ID is formed by concatenating the bytes
 * of the policy ID and the asset name. The asset name can be an empty byte sequence, but the
 * policy ID must always be present.
 *
 * \param[in] data A pointer to an array of bytes representing the concatenation of the policy ID
 *                 and the asset name. The policy ID typically occupies the first 28 bytes, followed
 *                 by the bytes of the asset name. The data pointer must not be NULL.
 * \param[in] size The size of the byte array, which includes both the policy ID and the asset name.
 *                 The size must be at least 28 bytes (the size of the policy ID), but it can be larger
 *                 depending on the length of the asset name.
 * \param[out] asset_id On successful initialization, this will point to a newly created \ref cardano_asset_id_t object.
 *             This object represents a "strong reference" to the asset ID, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object. Specifically, once the asset ID is no longer needed,
 *             the caller must release it by calling \ref cardano_asset_id_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the asset ID was successfully created from the provided bytes, or an appropriate error code
 *         indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the input data pointer is NULL,
 *         or \ref CARDANO_ERROR_INVALID_ARGUMENT if the size is less than 28 bytes or violates other constraints.
 *
 * Usage Example:
 * \code{.c}
 * const byte_t* asset_data = ...; // Asset data should contain policy ID followed by asset name
 * size_t asset_size = ...; // Size of asset_data (minimum 28 bytes)
 * cardano_asset_id_t* asset_id = NULL;
 *
 * cardano_error_t result = cardano_asset_id_from_bytes(asset_data, asset_size, &asset_id);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the asset_id
 *   // Once done, ensure to clean up and release the asset_id
 *   cardano_asset_id_unref(&asset_id);
 * }
 * else
 * {
 *   printf("Failed to create asset ID: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 *
 * \note The byte data must include the policy ID followed by the asset name, which could be empty.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_asset_id_from_bytes(
  const byte_t*        data,
  size_t               size,
  cardano_asset_id_t** asset_id);

/**
 * \brief Creates and initializes a new instance of a Cardano asset ID from a hexadecimal string.
 *
 * This function allocates and initializes a new instance of a \ref cardano_asset_id_t object,
 * which represents the ID of a Cardano native asset. The asset ID is derived from a hexadecimal-encoded
 * string, typically representing a policy ID or asset name in hexadecimal format.
 *
 * \param[in] hex_string A pointer to a null-terminated string containing the asset ID in hexadecimal format.
 *                       The string must only contain valid hexadecimal characters (0-9, a-f, A-F) and must not be NULL.
 * \param[in] size The size of the hexadecimal string, not including the null terminator. This allows for flexible
 *                 string lengths while excluding the null terminator from the conversion process.
 * \param[out] asset_id On successful initialization, this will point to a newly created \ref cardano_asset_id_t object.
 *             This object represents a "strong reference" to the asset ID, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object. Specifically, once the asset ID is no longer needed,
 *             the caller must release it by calling \ref cardano_asset_id_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the asset ID was successfully created from the provided hexadecimal string, or an appropriate error code
 *         indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the input string pointer is NULL,
 *         or \ref CARDANO_ERROR_INVALID_ARGUMENT if the string contains invalid characters or does not meet other constraints.
 *
 * Usage Example:
 * \code{.c}
 * const char* hex_string = "616263646566..."; // Hexadecimal representation of some asset data
 * size_t hex_size = strlen(hex_string); // Size of the hexadecimal string
 * cardano_asset_id_t* asset_id = NULL;
 *
 * cardano_error_t result = cardano_asset_id_from_hex(hex_string, hex_size, &asset_id);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the asset_id
 *   // Once done, ensure to clean up and release the asset_id
 *   cardano_asset_id_unref(&asset_id);
 * }
 * else
 * {
 *   printf("Failed to create asset ID from hex: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_asset_id_from_hex(
  const char*          hex_string,
  size_t               size,
  cardano_asset_id_t** asset_id);

/**
 * \brief Retrieves the byte representation of a Cardano asset id.
 *
 * This function provides access to the underlying byte array of a \ref cardano_asset_id_t object.
 * The byte array represents the asset id as it is stored and used within the Cardano ecosystem.
 *
 * \param[in] asset_id A pointer of an initialized \ref cardano_asset_id_t object.
 *                       The pointer must not be NULL and should point to a valid asset id object.
 *
 * \return A pointer to the constant byte array representing the asset id. This pointer points to an
 *         internal buffer and must not be modified or freed by the caller. If the input is NULL or an
 *         error occurs, the function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_asset_id_t* asset_id = ...; // Assume asset_id is already initialized
 * const byte_t* name_bytes = cardano_asset_id_get_bytes(asset_id);
 *
 * if (name_bytes != NULL)
 * {
 *   // Process the bytes of the asset id
 *   // Note: The length of the byte array should be known or determined separately
 * }
 * else
 * {
 *   printf("Failed to retrieve the bytes of the asset id.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const byte_t*
cardano_asset_id_get_bytes(const cardano_asset_id_t* asset_id);

/**
 * \brief Retrieves the size of the byte array for a Cardano asset id.
 *
 * This function returns the size of the byte array representing the asset id contained within
 * a \ref cardano_asset_id_t object.
 *
 * \param[in] asset_id A pointer of an initialized \ref cardano_asset_id_t object.
 *                       The pointer must not be NULL and should point to a valid asset id object.
 *
 * \return The size of the byte array representing the asset id. If the input is NULL or an
 *         error occurs, the function returns 0. This size does not include a null terminator, as
 *         asset ids can contain arbitrary binary data.
 *
 * Usage Example:
 * \code{.c}
 * cardano_asset_id_t* asset_id = ...; // Assume asset_id is already initialized
 * size_t name_size = cardano_asset_id_get_bytes_size(asset_id);
 *
 * if (name_size > 0)
 * {
 *   const byte_t* name_bytes = cardano_asset_id_get_bytes(asset_id);
 *   // Process the bytes knowing the exact size of the asset id
 * }
 * else
 * {
 *   printf("Failed to retrieve the size of the asset id or asset id is empty.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t
cardano_asset_id_get_bytes_size(const cardano_asset_id_t* asset_id);

/**
 * \brief Retrieves the hexadecimal string representation of a Cardano asset ID.
 *
 * This function returns the hexadecimal string representation of a \ref cardano_asset_id_t object.
 * The string encodes the asset ID, which consists of the concatenated bytes of the policy ID and asset name.
 *
 * \param[in] asset_id A pointer to an initialized \ref cardano_asset_id_t object.
 *                     The pointer must not be NULL and should point to a valid asset ID object.
 *
 * \return A pointer to a null-terminated string representing the asset ID in hexadecimal format.
 *         This string is owned by the \ref cardano_asset_id_t object and must not be modified or freed by the caller.
 *         If the input is NULL or an error occurs, the function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_asset_id_t* asset_id = ...; // Assume asset_id is already initialized
 * const char* hex_string = cardano_asset_id_get_hex(asset_id);
 *
 * if (hex_string != NULL)
 * {
 *   // Process the hexadecimal string of the asset ID
 *   printf("Asset ID in hex: %s\n", hex_string);
 * }
 * else
 * {
 *   printf("Failed to retrieve the hexadecimal string of the asset ID.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char*
cardano_asset_id_get_hex(const cardano_asset_id_t* asset_id);

/**
 * \brief Retrieves the size of the hexadecimal string for a Cardano asset ID.
 *
 * This function returns the size of the hexadecimal string representation of a \ref cardano_asset_id_t object.
 * The size includes the number of characters in the hexadecimal string, not including the null terminator.
 *
 * \param[in] asset_id A pointer to an initialized \ref cardano_asset_id_t object.
 *                     The pointer must not be NULL and should point to a valid asset ID object.
 *
 * \return The size of the hexadecimal string representing the asset ID. If the input is NULL or an
 *         error occurs, the function returns 0. The size does not include a null terminator.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_asset_id_t* asset_id = ...; // Assume asset_id is already initialized
 * size_t hex_size = cardano_asset_id_get_hex_size(asset_id);
 *
 * if (hex_size > 0)
 * {
 *   const char* hex_string = cardano_asset_id_get_hex(asset_id);
 *   // Process the hex string knowing the exact size of the asset ID
 *   printf("Hexadecimal string size: %zu\n", hex_size);
 * }
 * else
 * {
 *   printf("Failed to retrieve the size of the asset ID in hex or asset ID is empty.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t
cardano_asset_id_get_hex_size(const cardano_asset_id_t* asset_id);

/**
 * \brief Checks if the given asset ID represents the native Cardano currency, Lovelace.
 *
 * This function determines if a \ref cardano_asset_id_t object corresponds to the Lovelace currency.
 * Lovelace is the smallest unit of ADA, the native cryptocurrency of the Cardano network. A Lovelace
 * asset ID is identified by the absence of an asset name and a specific policy ID.
 *
 * \param[in] asset_id A constant pointer to an initialized \ref cardano_asset_id_t object.
 *
 * \return \c true if the asset ID represents Lovelace, or \c false otherwise. If the input \c asset_id is NULL,
 *         the function returns false.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_asset_id_t* asset_id = ...; // Assume asset_id is already initialized
 * bool is_lovelace = cardano_asset_id_is_lovelace(asset_id);
 *
 * if (is_lovelace)
 * {
 *   printf("The asset is Lovelace.\n");
 * }
 * else
 * {
 *   printf("The asset is not Lovelace.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool
cardano_asset_id_is_lovelace(const cardano_asset_id_t* asset_id);

/**
 * \brief Retrieves the policy ID from a Cardano asset ID.
 *
 * This function extracts the policy ID from a given \ref cardano_asset_id_t object. The policy ID
 * is a \ref cardano_blake2b_hash_t object representing the unique identifier of the policy under which
 * the asset was issued.
 *
 * \param[in] asset_id A pointer to an initialized \ref cardano_asset_id_t object.
 *
 * \return A pointer to the \ref cardano_blake2b_hash_t object representing the policy ID. The returned
 *         policy ID is a new reference, and the caller is responsible for releasing it with \ref cardano_blake2b_hash_unref
 *         when it is no longer needed. If the asset ID is NULL, or if the asset represents Lovelace, the function returns NULL.
 *
 * \note For assets that represent the native currency Lovelace, there is no associated policy ID. Clients must first call
 *       \ref cardano_asset_id_is_lovelace to check if the asset is Lovelace before attempting to retrieve the policy ID.
 *       If the asset is Lovelace, this function will return NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_asset_id_t* asset_id = ...; // Assume asset_id is already initialized
 * if (!cardano_asset_id_is_lovelace(asset_id))
 * {
 *   cardano_blake2b_hash_t* policy_id = cardano_asset_id_get_policy_id(asset_id);
 *
 *   if (policy_id != NULL)
 *   {
 *     // Use the policy ID
 *     // Ensure to release the policy ID once done
 *     cardano_blake2b_hash_unref(&policy_id);
 *   }
 *   else
 *   {
 *     printf("Failed to retrieve the policy ID or asset_id is NULL.\n");
 *   }
 * }
 * else
 * {
 *   printf("This asset is Lovelace and has no policy ID.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t*
cardano_asset_id_get_policy_id(cardano_asset_id_t* asset_id);

/**
 * \brief Retrieves the asset name from a Cardano asset ID.
 *
 * This function extracts the asset name from a given \ref cardano_asset_id_t object. The asset name
 * is a \ref cardano_asset_name_t object representing the name of the asset within the specified policy.
 *
 * \param[in] asset_id A pointer to an initialized \ref cardano_asset_id_t object.
 *
 * \return A pointer to the \ref cardano_asset_name_t object representing the asset name. The returned
 *         asset name is a new reference, and the caller is responsible for releasing it with \ref cardano_asset_name_unref
 *         when it is no longer needed. If the asset ID is NULL or if the asset represents Lovelace, the function returns NULL.
 *
 * \note For assets that represent the native currency Lovelace, there is no associated asset name. Clients must first call
 *       \ref cardano_asset_id_is_lovelace to check if the asset is Lovelace before attempting to retrieve the asset name.
 *       If the asset is Lovelace, this function will return NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_asset_id_t* asset_id = ...; // Assume asset_id is already initialized
 *
 * if (!cardano_asset_id_is_lovelace(asset_id))
 * {
 *   cardano_asset_name_t* asset_name = cardano_asset_id_get_asset_name(asset_id);
 *
 *   if (asset_name != NULL)
 *   {
 *     // Use the asset name
 *     // Ensure to release the asset name once done
 *     cardano_asset_name_unref(&asset_name);
 *   }
 *   else
 *   {
 *     printf("Failed to retrieve the asset name or asset_id is NULL.\n");
 *   }
 * }
 * else
 * {
 *   printf("This asset is Lovelace and has no asset name.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_asset_name_t*
cardano_asset_id_get_asset_name(cardano_asset_id_t* asset_id);

/**
 * \brief Decrements the reference count of a cardano_asset_id_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_asset_id_t object
 * by decreasing its reference count. When the reference count reaches zero, the asset_id is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] asset_id A pointer to the pointer of the asset_id object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_asset_id_t* asset_id = cardano_asset_id_new(major, minor);
 *
 * // Perform operations with the asset_id...
 *
 * cardano_asset_id_unref(&asset_id);
 * // At this point, asset_id is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_asset_id_unref, the pointer to the \ref cardano_asset_id_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_asset_id_unref(cardano_asset_id_t** asset_id);

/**
 * \brief Increases the reference count of the cardano_asset_id_t object.
 *
 * This function is used to manually increment the reference count of an cardano_asset_id_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_asset_id_unref.
 *
 * \param asset_id A pointer to the cardano_asset_id_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming asset_id is a previously created asset_id object
 *
 * cardano_asset_id_ref(asset_id);
 *
 * // Now asset_id can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_asset_id_ref there is a corresponding
 * call to \ref cardano_asset_id_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_asset_id_ref(cardano_asset_id_t* asset_id);

/**
 * \brief Retrieves the current reference count of the cardano_asset_id_t object.
 *
 * This function returns the number of active references to an cardano_asset_id_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_asset_id_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param asset_id A pointer to the cardano_asset_id_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_asset_id_t object. If the object
 * is properly managed (i.e., every \ref cardano_asset_id_ref call is matched with a
 * \ref cardano_asset_id_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming asset_id is a previously created asset_id object
 *
 * size_t ref_count = cardano_asset_id_refcount(asset_id);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_asset_id_refcount(const cardano_asset_id_t* asset_id);

/**
 * \brief Sets the last error message for a given cardano_asset_id_t object.
 *
 * Records an error message in the asset_id's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] asset_id A pointer to the \ref cardano_asset_id_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the asset_id's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_asset_id_set_last_error(
  cardano_asset_id_t* asset_id,
  const char*         message);

/**
 * \brief Retrieves the last error message recorded for a specific asset_id.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_asset_id_set_last_error for the given
 * asset_id. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] asset_id A pointer to the \ref cardano_asset_id_t instance whose last error
 *                   message is to be retrieved. If the asset_id is NULL, the function
 *                   returns a generic error message indicating the null asset_id.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified asset_id. If the asset_id is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_asset_id_set_last_error for the same asset_id, or until
 *       the asset_id is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_asset_id_get_last_error(
  const cardano_asset_id_t* asset_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_ASSET_ID_H