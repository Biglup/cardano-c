/**
 * \file pointer_address.h
 *
 * \author angel.castillo
 * \date   Apr 15, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_POINTER_ADDRESS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_POINTER_ADDRESS_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

#include <cardano/address/stake_pointer.h>
#include <cardano/common/credential.h>
#include <cardano/common/network_id.h>

/* FORWARD DECLARATIONS *****************************************************/

/**
 * \brief Represents a Cardano address.
 *
 * This structure encapsulates all the necessary information for a Cardano address.
 */
typedef struct cardano_address_t cardano_address_t;

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a pointer address in the Cardano blockchain ecosystem.
 *
 * A pointer address indirectly specifies the stake key that should control the stake for the address. It references
 * a stake key by a stake key pointer, which is a location on the blockchain of the stake key registration certificate
 * for that key.
 */
typedef struct cardano_pointer_address_t cardano_pointer_address_t;

/**
 * \brief Creates a pointer address from network and payment credentials and a stake pointer.
 *
 * This function constructs a \ref cardano_pointer_address_t object by using a network identifier,
 * payment credentials, and a stake pointer. The stake pointer directs to a stake key registration
 * certificate on the blockchain, allowing the address to indirectly specify staking rights without
 * embedding a stake key.
 *
 * \param[in] network_id The network identifier, which specifies the Cardano network (e.g., mainnet, testnet).
 * \param[in] payment A pointer to a \ref cardano_credential_t object representing the payment credentials.
 * \param[in] pointer A \ref cardano_stake_pointer_t structure that specifies the location of the stake key
 *                    registration certificate on the blockchain.
 * \param[out] pointer_address A pointer to a pointer to \ref cardano_pointer_address_t that will be set to
 *                              the address of the newly created pointer address object upon successful
 *                              address creation.
 *
 * \return Returns \ref CARDANO_SUCCESS if the pointer address was successfully created. Returns
 *         \ref CARDANO_ERROR_POINTER_IS_NULL if any of the pointers (\p payment or \p pointer_address) are NULL.
 *
 * \note The caller is responsible for managing the lifecycle of the created \ref cardano_pointer_address_t
 *       object, including ensuring that `cardano_pointer_address_unref` is called to free the address object
 *       when it is no longer needed, to prevent memory leaks.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* payment_credential = ...;     // Initialized elsewhere
 * cardano_stake_pointer_t stake_pointer = { 5756214, 1, 0 };  // Example pointer to a stake key registration
 * cardano_pointer_address_t* pointer_address = NULL;
 * cardano_error_t result = cardano_pointer_address_from_credentials(
 *      CARDANO_NETWORK_ID_MAIN_NET,
 *      payment_credentials,
 *      stake_pointer,
 *      &pointer_address);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the pointer address as needed
 *   // Once done, ensure to clean up and release the pointer address
 *   cardano_pointer_address_unref(&pointer_address);
 * }
 * else
 * {
 *   printf("Failed to create pointer address: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pointer_address_from_credentials(
  cardano_network_id_t        network_id,
  cardano_credential_t*       payment,
  cardano_stake_pointer_t     pointer,
  cardano_pointer_address_t** pointer_address);

/**
 * \brief Converts a general Cardano address to a pointer address.
 *
 * This function takes an existing \ref cardano_address_t object and attempts to create a corresponding
 * \ref cardano_pointer_address_t object.
 *
 * \param[in] address A pointer to the \ref cardano_address_t object that is to be converted into a pointer address.
 * \param[out] pointer_address A pointer to a pointer to \ref cardano_pointer_address_t that will be set
 *                              to the address of the newly created pointer address object upon successful
 *                              conversion.
 *
 * \return Returns \ref CARDANO_SUCCESS if the conversion was successful. Returns \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         the \p address or \p pointer_address pointer is NULL. Returns \ref CARDANO_ERROR_INVALID_ADDRESS_FORMAT if the
 *         provided address cannot be converted to a pointer address.
 *
 * \note It is the caller's responsibility to manage the lifecycle of the created \ref cardano_pointer_address_t object,
 *       including calling `cardano_pointer_address_unref` to free the address object when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_address_t* general_address = NULL;
 * // Assume general_address is previously created and represents a valid Cardano address
 * cardano_pointer_address_t* pointer_address = NULL;
 * cardano_error_t result = cardano_pointer_address_from_address(general_address, &pointer_address);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The address has been successfully converted to a pointer address
 *   // Use the pointer address as needed
 *
 *   // Once done, ensure to clean up and release the pointer address
 *   cardano_pointer_address_unref(&pointer_address);
 * }
 * else
 * {
 *   printf("Failed to convert to pointer address: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pointer_address_from_address(
  const cardano_address_t*    address,
  cardano_pointer_address_t** pointer_address);

/**
 * \brief Converts a pointer address to a general Cardano address.
 *
 * This function takes a \ref cardano_pointer_address_t object representing a pointer address and converts
 * it back to a general \ref cardano_address_t object. This is useful for operations that require the use of
 * a generic address format instead of a pointer-specific format, particularly in interfaces that do not support
 * the direct use of pointer addresses.
 *
 * \param[in] pointer_address A pointer to the \ref cardano_pointer_address_t object that is to be converted
 *                            into a general Cardano address.
 *
 * \return A pointer to a newly created \ref cardano_address_t object representing the general address. Returns NULL
 *         if the \p pointer_address is NULL.
 *
 * \note The caller is responsible for managing the lifecycle of the returned \ref cardano_address_t object, including
 *       calling `cardano_address_unref` to free the object when it is no longer needed. Failure to do so can result in
 *       memory leaks.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pointer_address_t* pointer_address = NULL;
 * // Assume pointer_address is previously created and represents a valid pointer address
 * cardano_address_t* general_address = cardano_pointer_address_to_address(pointer_address);
 *
 * if (general_address != NULL)
 * {
 *   // The pointer address has been successfully converted to a general Cardano address
 *   // Use the general address as needed
 *
 *   // Once done, ensure to clean up and release the general address
 *   cardano_address_unref(&general_address);
 * }
 * else
 * {
 *   printf("Failed to convert pointer address to a general address\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_address_t* cardano_pointer_address_to_address(
  const cardano_pointer_address_t* pointer_address);

/**
 * \brief Retrieves the payment credential from a pointer address.
 *
 * This function extracts the payment credential from a \ref cardano_pointer_address_t object.
 *
 * \param[in] pointer_address A pointer to the \ref cardano_pointer_address_t object from which the payment
 *                            credential is to be retrieved.
 *
 * \return A pointer to a \ref cardano_credential_t object representing the payment credential. Returns NULL if the
 *         \p pointer_address is NULL.
 *
 * \note The lifecycle of the returned \ref cardano_credential_t object must be managed by the caller. It is important to
 *       call `cardano_credential_unref` to free this object when it is no longer needed to prevent memory leaks.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pointer_address_t* pointer_address = NULL;
 * // Assume pointer_address is previously created and represents a valid pointer address
 * const cardano_credential_t* payment_credential = cardano_pointer_address_get_payment_credential(pointer_address);
 *
 * if (payment_credential != NULL)
 * {
 *   // The payment credential can be used to initiate transactions or for other purposes
 *
 *   cardano_credential_unref(&payment_credential);
 * }
 * else
 * {
 *   printf("Failed to retrieve payment credential from pointer address\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_credential_t* cardano_pointer_address_get_payment_credential(
  cardano_pointer_address_t* pointer_address);

/**
 * \brief Retrieves the stake pointer from a pointer address.
 *
 * This function extracts the stake pointer from a \ref cardano_pointer_address_t object.
 *
 * \param[in] address A pointer to the \ref cardano_pointer_address_t object from which the stake pointer
 *                    is to be retrieved.
 * \param[out] pointer A pointer to a \ref cardano_stake_pointer_t structure that will be populated with the
 *                     stake pointer information if the retrieval is successful.
 *
 * \return Returns \ref CARDANO_SUCCESS if the stake pointer was successfully retrieved. Returns
 *         \ref CARDANO_ERROR_POINTER_IS_NULL if the \p address or \p pointer argument is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pointer_address_t* pointer_address = NULL;
 * // Assume pointer_address is previously created and represents a valid pointer address
 * cardano_stake_pointer_t stake_pointer;
 * cardano_error_t result = cardano_pointer_address_get_stake_pointer(pointer_address, &stake_pointer);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Stake Pointer - Slot: %llu, Tx Index: %llu, Cert Index: %llu\n",
 *          stake_pointer.slot, stake_pointer.tx_index, stake_pointer.cert_index);
 * }
 * else
 * {
 *   printf("Failed to retrieve stake pointer from address: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pointer_address_get_stake_pointer(
  const cardano_pointer_address_t* address,
  cardano_stake_pointer_t*         pointer);

/**
 * \brief Creates a pointer address from a byte array.
 *
 * This function constructs a \ref cardano_pointer_address_t object by decoding a byte array that
 * represents a pointer address in serialized form.
 *
 * \param[in] data A pointer to the byte array containing the serialized pointer address data.
 * \param[in] size The size of the byte array in bytes.
 * \param[out] address A pointer to a pointer to \ref cardano_pointer_address_t that will be set to the address
 *                     of the newly created pointer address object upon successful decoding.
 *
 * \return Returns \ref CARDANO_SUCCESS if the address was successfully created. Returns \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the \p data or \p address pointer is NULL. Returns \ref CARDANO_ERROR_INVALID_ADDRESS_FORMAT if the byte array data
 *         could not be decoded into a valid pointer address.
 *
 * \note It is the caller's responsibility to manage the lifecycle of the created \ref cardano_pointer_address_t object,
 *       including ensuring that `cardano_pointer_address_unref` is called to free the address object when it is no
 *       longer needed, to prevent memory leaks.
 *
 * Usage Example:
 * \code{.c}
 * byte_t serialized_address[] = { ... };
 * size_t serialized_size = sizeof(serialized_address);
 * cardano_pointer_address_t* pointer_address = NULL;
 *
 * cardano_error_t result = cardano_pointer_address_from_bytes(serialized_address, serialized_size, &pointer_address);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the pointer address as needed
 *   // Once done, ensure to clean up and release the pointer address
 *   cardano_pointer_address_unref(&pointer_address);
 * }
 * else
 * {
 *   printf("Failed to create pointer address from bytes: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pointer_address_from_bytes(
  const byte_t*               data,
  size_t                      size,
  cardano_pointer_address_t** address);

/**
 * \brief Retrieves the size in bytes required to serialize a pointer address.
 *
 * This function calculates the size necessary to store the serialized byte array representation of a given
 * \ref cardano_pointer_address_t object.
 *
 * \param[in] address A pointer to the \ref cardano_pointer_address_t object whose serialized size is to be retrieved.
 *
 * \return The size in bytes required to serialize the pointer address. Returns 0 if the \p address pointer is NULL
 *         or if the address is invalid.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pointer_address_t* pointer_address = NULL;
 * // Assume pointer_address is already created and valid
 * size_t required_size = cardano_pointer_address_get_bytes_size(pointer_address);
 *
 * if (required_size > 0)
 * {
 *   byte_t* buffer = (byte_t*)malloc(required_size);
 *
 *   if (buffer)
 *   {
 *     // Proceed to serialize the pointer address into buffer
 *   }
 *
 *  free(buffer);
 * }
 * else
 * {
 *   printf("Invalid address or unable to determine size\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_pointer_address_get_bytes_size(const cardano_pointer_address_t* address);

/**
 * \brief Retrieves the byte array representation of a pointer address.
 *
 * This function provides access to the serialized byte array form of a \ref cardano_pointer_address_t object.
 *
 * \param[in] address A pointer to the \ref cardano_pointer_address_t object whose byte array is to be retrieved.
 *
 * \return A pointer to the constant byte array representing the pointer address. Returns NULL if the \p address
 *         pointer is NULL.
 *
 * \note The returned byte array is managed internally and should not be freed or modified by the caller. It remains
 *       valid as long as the \ref cardano_pointer_address_t object is not modified or freed. Use
 *       \ref cardano_pointer_address_get_bytes_size to determine the size of the byte array.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pointer_address_t* pointer_address = NULL;
 * // Assume pointer_address is already created and valid
 * const byte_t* address_bytes = cardano_pointer_address_get_bytes(pointer_address);
 * size_t bytes_size = cardano_pointer_address_get_bytes_size(pointer_address);
 *
 * if (address_bytes != NULL)
 * {
 *   // Process or transmit the byte array
 * }
 * else
 * {
 *   printf("Failed to retrieve byte array from pointer address\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const byte_t* cardano_pointer_address_get_bytes(const cardano_pointer_address_t* address);

/**
 * \brief Serializes a pointer address into a byte array.
 *
 * This function takes a \ref cardano_pointer_address_t object and serializes it into a binary format.
 * The serialized bytes are written into the provided \p data buffer, which must be large enough to hold
 * the entire binary representation of the address.
 *
 * \param[in] address A pointer to the \ref cardano_pointer_address_t object to be serialized.
 * \param[out] data A pointer to a byte array where the serialized data will be written.
 * \param[in] size The size of the provided buffer in bytes. This size should be at least the value returned
 *                 by \ref cardano_pointer_address_get_bytes_size to ensure it can hold the serialized data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the address was successfully serialized into the byte array.
 *         Returns \ref CARDANO_ERROR_POINTER_IS_NULL if the \p address or \p data pointer is NULL. Returns
 *         \ref CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE if the \p size is too small to hold the serialized data.
 *
 * \note The buffer provided in \p data must be sufficiently large to accommodate
 *       the serialized data to prevent buffer overflow errors.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pointer_address_t* pointer_address = NULL;
 * // Assume pointer_address is previously created and valid
 * size_t required_size = cardano_pointer_address_get_bytes_size(pointer_address);
 * byte_t* buffer = (byte_t*)malloc(required_size);
 *
 * if (buffer)
 * {
 *   cardano_error_t result = cardano_pointer_address_to_bytes(pointer_address, buffer, required_size);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     // The buffer now contains the serialized address
 *     // Proceed with using the buffer for storage, transmission, etc.
 *   }
 *   else
 *   {
 *     printf("Failed to serialize pointer address: %d\n", result);
 *   }
 *
 *  free(buffer);
 * }
 * else
 * {
 *   printf("Memory allocation for buffer failed\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pointer_address_to_bytes(
  const cardano_pointer_address_t* address,
  byte_t*                          data,
  size_t                           size);

/**
 * \brief Creates an enterprise address from a Bech32-encoded string.
 *
 * This function constructs a \ref cardano_pointer_address_t object by decoding the provided
 * Bech32-encoded string that represents the address data.
 *
 * \param[in] data A pointer to a character array containing the Bech32-encoded representation of the address.
 * \param[in] size The size of the Bech32 string in bytes.
 * \param[out] address A pointer to a pointer to \ref cardano_pointer_address_t that will be set to the address
 *                     of the newly created enterprise address object upon successful decoding.
 *
 * \return Returns \ref CARDANO_SUCCESS if the address was successfully created. Returns \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the \p data or \p address pointer is NULL. Returns \ref CARDANO_ERROR_INVALID_ADDRESS_FORMAT if the Bech32 data
 *         could not be decoded into a valid enterprise address.
 *
 * Usage Example:
 * \code{.c}
 * const char* bech32_data = "addr1qw508d6qejxtdg4y5r3zarvary0c5xw7kxpjzsx...";
 * size_t bech32_size = strlen(bech32_data);
 * cardano_pointer_address_t* cardano_pointer = NULL;
 *
 * cardano_error_t result = cardano_pointer_address_from_bech32(bech32_data, bech32_size, &cardano_pointer);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the enterprise address
 *
 *   // Once done, ensure to clean up and release the enterprise address
 *   cardano_pointer_address_unref(&cardano_pointer);
 * }
 * else
 * {
 *   printf("Failed to decode enterprise address from Bech32: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pointer_address_from_bech32(
  const char*                 data,
  size_t                      size,
  cardano_pointer_address_t** address);

/**
 * \brief Retrieves the size in bytes of the Bech32-encoded representation of an enterprise address.
 *
 * This function calculates the size necessary to store the Bech32-encoded representation of a given
 * \ref cardano_pointer_address_t address object. This size includes the characters needed to represent
 * the Bech32 string but does not include the null-termination character.
 *
 * \param[in] address A pointer to the \ref cardano_pointer_address_t object whose Bech32 size is to be retrieved.
 *
 * \return The size in bytes of the Bech32-encoded representation of the address, including the null terminator.
 *         Returns 0 if the \p address pointer is NULL or if the address is invalid.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pointer_address_t* cardano_pointer = NULL;
 * // Assume cardano_pointer is already created and valid
 * size_t bech32_size = cardano_pointer_address_get_bech32_size(cardano_pointer);
 *
 * if (bech32_size > 0)
 * {
 *   char* bech32_string = (char*) malloc(bech32_size);
 *
 *   if (bech32_string)
 *   {
 *     // Proceed to convert address to Bech32 string
 *   }
 *
 *   free(bech32_string);
 * }
 * else
 * {
 *   printf("Invalid address or unable to determine size\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_pointer_address_get_bech32_size(const cardano_pointer_address_t* address);

/**
 * \brief Converts an enterprise address to a Bech32-encoded string.
 *
 * This function takes a \ref cardano_pointer_address_t object and converts it into a Bech32 string,
 * writing the result into the provided \p data buffer. The buffer must be large enough to hold the entire
 * Bech32 string including the null-termination character.
 *
 * \param[in] address A pointer to the \ref cardano_pointer_address_t object to be converted.
 * \param[out] data A pointer to a character buffer where the Bech32-encoded string will be stored.
 * \param[in] size The size of the provided buffer in bytes. This size must be at least the value returned
 *                 by \ref cardano_pointer_address_get_bech32_size to ensure it can hold the Bech32 string
 *                 and the null terminator.
 *
 * \return Returns \ref CARDANO_SUCCESS if the conversion was successful. Returns \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         the \p address or \p data pointer is NULL. Returns \ref CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE if the \p size is too
 *         small to hold the Bech32 representation including the null terminator.
 *
 * \note It is crucial to ensure that the buffer provided in \p data is sufficiently large to accommodate the
 *       Bech32 string and the null-termination character to prevent buffer overflow errors.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pointer_address_t* cardano_pointer = NULL;
 * // Assume cardano_pointer is previously created and valid
 * size_t required_size = cardano_pointer_address_get_bech32_size(cardano_pointer) + 1; // +1 for null-termination
 * char* bech32_string = (char*) malloc(required_size);
 * if (bech32_string)
 * {
 *   cardano_error_t result = cardano_pointer_address_to_bech32(cardano_pointer, bech32_string, required_size);
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     printf("Bech32 representation: %s\n", bech32_string);
 *   }
 *   else
 *   {
 *     printf("Failed to convert enterprise address to Bech32: %d\n", result);
 *   }
 *   free(bech32_string);
 * }
 * else
 * {
 *   printf("Memory allocation failed for Bech32 buffer\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pointer_address_to_bech32(
  const cardano_pointer_address_t* address,
  char*                            data,
  size_t                           size);

/**
 * \brief Retrieves the string representation of an enterprise address.
 *
 * This function provides access to the string form of a \ref cardano_pointer_address_t object. It allows for
 * easy display and logging of the address in a human-readable format.
 *
 * \param[in] address A pointer to the \ref cardano_pointer_address_t object whose string representation is to be retrieved.
 *
 * \return A pointer to a constant character string representing the enterprise address. Returns NULL if the \p address
 *         pointer is NULL or if the address cannot be properly serialized into a string form.
 *
 * \note The returned string is managed internally and should not be freed or modified by the caller. It remains
 *       valid as long as the \ref cardano_pointer_address_t object is not modified or freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pointer_address_t* cardano_pointer = NULL;
 * // Assume cardano_pointer is previously created and valid
 * const char* address_string = cardano_pointer_address_get_string(cardano_pointer);
 *
 * if (address_string != NULL)
 * {
 *   printf("Enterprise Address: %s\n", address_string);
 * }
 * else
 * {
 *   printf("Failed to retrieve string representation of the enterprise address\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_pointer_address_get_string(const cardano_pointer_address_t* address);

/**
 * \brief Retrieves the network ID from a given Cardano pointer address.
 *
 * This function extracts the network identifier from the provided \ref cardano_pointer_address_t object.
 * The network ID indicates whether the address belongs to the test network or the main network.
 *
 * \param[in] address A constant pointer to the \ref cardano_pointer_address_t object from which the network ID is to be retrieved.
 * \param[out] network_id A pointer to \ref cardano_network_id_t where the network ID will be stored upon successful
 *                        extraction. This parameter cannot be NULL.
 *
 * \return Returns \ref CARDANO_SUCCESS if the network ID was successfully retrieved.
 *         Returns \ref CARDANO_ERROR_POINTER_IS_NULL if the input address or network_id pointer is NULL.
 *         Returns other error codes as defined in \ref cardano_error_t if the network ID cannot be retrieved due to
 *         malformed or unrecognized address formats.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pointer_address_t* address = NULL;
 * cardano_network_id_t network_id;
 *
 * cardano_error_t get_network_id = cardano_pointer_address_get_network_id(address, &network_id);
 *
 * if (get_network_id != CARDANO_SUCCESS)
 * {
 *   printf("Failed to determine the network id.\n");
 * }
 * else
 * {
 *   printf("Network ID: %d\n", network_id);
 * }
 *
 * cardano_pointer_address_unref(&address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pointer_address_get_network_id(
  const cardano_pointer_address_t* address,
  cardano_network_id_t*            network_id);

/**
 * \brief Decrements the pointer address's reference count.
 *
 * If the reference count reaches zero, the pointer address memory is deallocated.
 *
 * \param[in] address Pointer to the pointer address whose reference count is to be decremented.
 */
CARDANO_EXPORT void cardano_pointer_address_unref(cardano_pointer_address_t** address);

/**
 * \brief Increments the pointer address's reference count.
 *
 * Ensures that the pointer address remains allocated until the last reference is released.
 *
 * \param[in] address pointer address whose reference count is to be incremented.
 */
CARDANO_EXPORT void cardano_pointer_address_ref(cardano_pointer_address_t* address);

/**
 * \brief Retrieves the pointer address's current reference count.
 *
 * \warning Does not account for transitive references.
 *
 * \param[in] address Target pointer address.
 * \return Current reference count of the pointer address.
 */
CARDANO_EXPORT size_t cardano_pointer_address_refcount(const cardano_pointer_address_t* address);

/**
 * \brief Sets the last error message for a given pointer address.
 *
 * This function records an error message in the pointer address's last_error buffer,
 * overwriting any previous message. The message is truncated if it exceeds
 * the buffer size. This function is typically used to store descriptive
 * error information that can be retrieved later with
 * cardano_pointer_address_get_last_error.
 *
 * \param[in,out] address A pointer to the cardano_pointer_address_t instance whose last error
 *               message is to be set. If the pointer address is NULL, the function
 *               has no effect.
 * \param[in] message A null-terminated string containing the error message to be
 *                recorded. If the message is NULL, the pointer address's last_error
 *                will be set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters due to the fixed size
 *       of the last_error buffer (1024 characters), including the null
 *       terminator. Messages longer than this limit will be truncated.
 */
CARDANO_EXPORT void cardano_pointer_address_set_last_error(cardano_pointer_address_t* address, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific pointer address.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_pointer_address_set_last_error for the given
 * pointer address. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in,out] address A pointer to the \ref cardano_pointer_address_t instance whose last error
 *               message is to be retrieved. If the pointer address is \c NULL, the function
 *               returns a generic error message indicating the null pointer address.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified pointer address. If the pointer address is \c NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the pointer address and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_pointer_address_set_last_error for the same pointer address, or until
 *       the pointer address is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_pointer_address_get_last_error(const cardano_pointer_address_t* address);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_POINTER_ADDRESS_H