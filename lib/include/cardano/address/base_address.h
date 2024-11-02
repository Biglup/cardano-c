/**
 * \file base_address.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BASE_ADDRESS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BASE_ADDRESS_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

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
 * \brief Defines a pointer to a structure representing a Cardano base address.
 */
typedef struct cardano_base_address_t cardano_base_address_t;

/**
 * \brief Constructs a Cardano base address from payment and stake credentials.
 *
 * This function creates a base address by combining payment and stake credentials, along with specifying
 * the network ID (either mainnet or testnet).
 *
 * \param[in]  network_id The network identifier, specifying whether the address is for mainnet or testnet.
 * \param[in]  payment The payment credential, which is a pointer to a \ref cardano_credential_t that defines
 *                     who controls the funds.
 * \param[in]  stake The stake credential, which is a pointer to a \ref cardano_credential_t that who controls
 *                   staking rewards are directed.
 * \param[out] base_address A pointer to a pointer to \ref cardano_base_address_t that will hold the address
 *                           if the function succeeds.
 *
 * \return Returns \ref CARDANO_SUCCESS if the address was successfully created, otherwise it returns an
 *         error code indicating what went wrong (e.g., CARDANO_ERROR_POINTER_IS_NULL if any input pointers are NULL).
 *
 * \note It is the caller's responsibility to manage the lifecycle of the created \ref cardano_base_address_t object,
 *       which includes freeing it when it is no longer needed by calling \ref cardano_base_address_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* payment_credential = ...; // Assume this is initialized
 * cardano_credential_t* stake_credential = ...; // Assume this is initialized
 * cardano_base_address_t* base_address = NULL;
 * cardano_error_t result = cardano_base_address_from_credentials(
 *     CARDANO_NETWORK_ID_MAIN_NET, payment_credential, stake_credential, &base_address);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the base_address
 *   cardano_base_address_unref(&base_address);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_base_address_from_credentials(
  cardano_network_id_t     network_id,
  cardano_credential_t*    payment,
  cardano_credential_t*    stake,
  cardano_base_address_t** base_address);

/**
 * \brief Converts a general Cardano address to a base address.
 *
 * This function takes a general Cardano address and attempts to interpret it as a base address,
 * which is a specific type of Cardano address that includes separate payment and stake credentials.
 * If the conversion is successful, a new \ref cardano_base_address_t object is created.
 *
 * \param[in]  address A constant pointer to the \ref cardano_address_t object representing the general
 *                     Cardano address to be converted.
 * \param[out] base_address A pointer to a pointer to \ref cardano_base_address_t that will hold the
 *                           base address object if the conversion is successful.
 *
 * \return Returns \ref CARDANO_SUCCESS if the conversion is successful. If the address is not of a
 *         base address type, returns an appropriate error code such as \ref CARDANO_ERROR_INVALID_ADDRESS_TYPE.
 *         If the \p address is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * \note The newly created \ref cardano_base_address_t object is fully managed by the caller, who is responsible
 *       for freeing it when it is no longer needed by calling \ref cardano_base_address_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_address_t* general_address = ...; // Assume this is initialized as a general Cardano address
 * cardano_base_address_t* base_address = NULL;
 * cardano_error_t result = cardano_base_address_from_address(general_address, &base_address);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The conversion was successful, and base_address can now be used
 *   cardano_base_address_unref(&base_address);
 * }
 * else
 * {
 *   printf("Failed to convert to base address: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_base_address_from_address(
  const cardano_address_t* address,
  cardano_base_address_t** base_address);

/**
 * \brief Converts a base address to a general Cardano address.
 *
 * This function takes a \ref cardano_base_address_t object and converts it into a general
 * \ref cardano_address_t object. This allows for the use of the base address in any context
 * that requires a generic address format.
 *
 * \param[in] base_address A constant pointer to the \ref cardano_base_address_t object that
 *                         is to be converted into a general Cardano address.
 *
 * \return A pointer to a new \ref cardano_address_t object that represents the general Cardano
 *         address. If the conversion fails due to memory allocation issues, returns NULL.
 *
 * \note The newly created \ref cardano_address_t object is fully managed by the caller, who is
 *       responsible for freeing it when it is no longer needed by calling \ref cardano_address_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_base_address_t* base_address = ...; // Assume this is initialized
 * cardano_address_t* general_address = cardano_base_address_to_address(base_address);
 *
 * if (general_address)
 * {
 *   // The conversion was successful, and general_address can now be used
 *   cardano_address_unref(&general_address);
 * }
 * else
 * {
 *   printf("Failed to convert base address to general address\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_address_t* cardano_base_address_to_address(
  const cardano_base_address_t* base_address);

/**
 * \brief Retrieves the payment credential from a base address.
 *
 * This function extracts the payment credential from a \ref cardano_base_address_t object.
 * The payment credential is a hash of a public key or a script that is authorized
 * to spend funds from the address.
 *
 * \param[in] base_address A constant pointer to the \ref cardano_base_address_t object from which
 *                         the payment credential is to be retrieved.
 *
 * \return A pointer to a \ref cardano_credential_t object that holds the payment credential. If
 *         the base address does not have a payment credential or if an error occurs, returns NULL.
 *
 * \note The newly created \ref cardano_credential_t object is fully managed by the caller, who is
 *       responsible for freeing it when it is no longer needed by calling \ref cardano_credential_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_base_address_t* base_address = ...; // Assume this is initialized
 * cardano_credential_t* payment_credential = cardano_base_address_get_payment_credential(base_address);
 *
 * if (payment_credential)
 * {
 *   // Use the payment credential
 *   cardano_credential_unref(&payment_credential);
 * }
 * else
 * {
 *    printf("Failed to retrieve payment credential from base address\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_credential_t* cardano_base_address_get_payment_credential(
  cardano_base_address_t* base_address);

/**
 * \brief Retrieves the stake credential from a base address.
 *
 * This function extracts the stake credential from a \ref cardano_base_address_t object.
 * The stake credential is a hash of a public key or a script that is in control
 * of the staking rewards from the address.
 *
 * \param[in] base_address A constant pointer to the \ref cardano_base_address_t object from which
 *                         the stake credential is to be retrieved.
 *
 * \return A pointer to a \ref cardano_credential_t object that holds the stake credential. If
 *         the base address does not have a stake credential or if an error occurs, returns NULL.
 *
 * \note The newly created \ref cardano_credential_t object is fully managed by the caller, who is
 *       responsible for freeing it when it is no longer needed by calling \ref cardano_credential_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_base_address_t* base_address = ...; // Assume this is initialized
 * cardano_credential_t* stake_credential = cardano_base_address_get_stake_credential(base_address);
 *
 * if (stake_credential)
 * {
 *   // Use the stake credential
 *   cardano_credential_unref(&stake_credential);
 * }
 * else
 * {
 *    printf("Failed to retrieve stake credential from base address\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_credential_t* cardano_base_address_get_stake_credential(
  cardano_base_address_t* base_address);

/**
 * \brief Creates a base address from a serialized byte array.
 *
 * This function constructs a \ref cardano_base_address_t object from a byte array that
 * contains serialized address data. It attempts to deserialize the data and create
 * a base address object from it.
 *
 * \param[in] data A pointer to the byte array containing the serialized base address data.
 * \param[in] size The size of the byte array in bytes.
 * \param[out] address A pointer to a pointer to \ref cardano_base_address_t. On successful
 *                     deserialization, this will point to a newly allocated base address object.
 *
 * \return A \ref cardano_error_t indicating the result of the function call. This will be
 *         \ref CARDANO_SUCCESS if the address was successfully created from the provided data.
 *         If an error occurs, a corresponding error code will be returned.
 *
 * \note The newly created \ref cardano_base_address_t object is fully managed by the caller,
 *       who is responsible for freeing it when it is no longer needed by calling \ref cardano_address_unref.
 *
 * Usage Example:
 * \code{.c}
 * const byte_t serialized_data[] = {0x00, 0x01, 0x02, ...}; // example serialized data
 * size_t size = sizeof(serialized_data);
 * cardano_base_address_t* base_address = NULL;
 * cardano_error_t result = cardano_base_address_from_bytes(serialized_data, size, &base_address);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the base_address
 *
 *   // Once done, ensure to clean up and release the base address
 *   cardano_address_unref(&base_address);
 * }
 * else
 * {
 *   printf("Failed to create base address from bytes: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_base_address_from_bytes(
  const byte_t*            data,
  size_t                   size,
  cardano_base_address_t** address);

/**
 * \brief Retrieves the size of the byte array needed to serialize a base address.
 *
 * This function calculates the size in bytes that a serialized form of the specified
 * \ref cardano_base_address_t object would occupy. This size is necessary to allocate a buffer
 * of appropriate size for serialization.
 *
 * \param[in] address A constant pointer to the \ref cardano_base_address_t object whose
 *                    serialized size is to be calculated. The object must not be NULL.
 *
 * \return The size in bytes required to serialize the base address. If the input address is NULL,
 *         the function returns 0.
 *
 * Usage Example:
 * \code{.c}
 * cardano_base_address_t* base_address = ...; // Assume this is initialized
 * size_t required_size = cardano_base_address_get_bytes_size(base_address);
 *
 * if (required_size > 0)
 * {
 *   byte_t* buffer = (byte_t*)malloc(required_size);
 *
 *   if (buffer)
 *   {
 *     // Proceed to serialize the base address
 *   }
 *   free(buffer);
 * }
 * else
 * {
 *   printf("Failed to determine the size needed for serialization\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_base_address_get_bytes_size(const cardano_base_address_t* address);

/**
 * \brief Retrieves a pointer to the internal byte array representation of a base address.
 *
 * This function provides access to the internal byte array that represents the serialized form
 * of a \ref cardano_base_address_t object. The data returned by this function is useful for
 * operations that require direct access to the serialized address, such as hashing or networking.
 *
 * \param[in] address A constant pointer to the \ref cardano_base_address_t object from which
 *                    the byte array is to be retrieved. The object must not be NULL.
 *
 * \return A pointer to the constant byte array that represents the serialized base address.
 *         If the input address is NULL, returns NULL. The data returned by this function should
 *         not be modified or freed by the caller, as it is managed internally by the Cardano library.
 *
 * Usage Example:
 * \code{.c}
 * cardano_base_address_t* base_address = ...; // Assume this is initialized
 * const byte_t* byte_data = cardano_base_address_get_bytes(base_address);
 *
 * if (byte_data)
 * {
 *   // Use the byte array for operations like sending over a network
 * }
 * else
 * {
 *   printf("Failed to retrieve byte data from base address\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const byte_t* cardano_base_address_get_bytes(const cardano_base_address_t* address);

/**
 * \brief Serializes a base address into a provided byte array.
 *
 * This function serializes the specified \ref cardano_base_address_t object into a byte array
 * provided by the caller. The size of the buffer must be sufficient to hold the serialized data;
 * the required size can be obtained by calling \ref cardano_base_address_get_bytes_size.
 *
 * \param[in] address A constant pointer to the \ref cardano_base_address_t object to be serialized.
 * \param[out] data A pointer to the buffer where the serialized data will be written. This buffer
 *                  must be pre-allocated by the caller.
 * \param[in] size The size of the buffer pointed to by \p data. It must be at least as large as the value
 *                 returned by \ref cardano_base_address_get_bytes_size to ensure successful serialization.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the buffer is too small,
 *         returns \ref CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE. If the \p address or \p data is NULL, returns
 *         \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_base_address_t* base_address = ...; // Assume this is initialized
 * size_t required_size = cardano_base_address_get_bytes_size(base_address);
 * byte_t* buffer = (byte_t*)malloc(required_size);
 *
 * if (buffer)
 * {
 *   cardano_error_t result = cardano_base_address_to_bytes(base_address, buffer, required_size);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     // Use the serialized data
 *   }
 *
 *   free(buffer);
 * }
 * else
 * {
 *   printf("Memory allocation failed\n");
 * }
 *
 * cardano_base_address_unref(&base_address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_base_address_to_bytes(
  const cardano_base_address_t* address,
  byte_t*                       data,
  size_t                        size);

/**
 * \brief Creates a base address from a Bech32 encoded string.
 *
 * This function decodes a Bech32 encoded string and constructs a \ref cardano_base_address_t object.
 * The Bech32 string should correctly represent a Cardano base address according to the
 * Cardano address specifications. This function will parse the string and if valid, will allocate
 * and initialize a new base address object.
 *
 * \param[in] data A pointer to the character array containing the Bech32 encoded address.
 * \param[in] size The length of the character array pointed to by \p data.
 * \param[out] address A pointer to a pointer to \ref cardano_base_address_t. If the decoding is
 *                     successful, this will be set to point to the newly created base address object.
 *
 * \return Returns \ref CARDANO_SUCCESS if the base address is successfully created from the Bech32 string.
 *         Returns \ref CARDANO_ERROR_INVALID_ADDRESS_FORMAT if the Bech32 string does not conform to the expected format or
 *         if the decoding fails. Returns \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * \note The caller is responsible for freeing the created \ref cardano_base_address_t object using the
 *       \ref cardano_base_address_unref function when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * const char* bech32_data = "addr1qxy...";
 * size_t bech32_size = strlen(bech32_data);
 * cardano_base_address_t* base_address = NULL;
 *
 * cardano_error_t result = cardano_base_address_from_bech32(bech32_data, bech32_size, &base_address);
 *
 * if (result == CARDANO_SUCCESS && base_address != NULL)
 * {
 *   // The base address has been successfully created
 *   // Use the base_address for operations
 * }
 * else
 * {
 *   printf("Failed to create base address from Bech32 data: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_base_address_unref(base_address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_base_address_from_bech32(
  const char*              data,
  size_t                   size,
  cardano_base_address_t** address);

/**
 * \brief Calculates the size of the Bech32 encoded string of a base address.
 *
 * This function calculates the length of the Bech32 string that would be generated
 * from the given \ref cardano_base_address_t object. This size includes the null
 * terminator needed for string termination in C.
 *
 * \param[in] address A constant pointer to the \ref cardano_base_address_t object
 *                    whose Bech32 string size is to be calculated.
 *
 * \return The size in bytes of the Bech32 string representation of the address,
 *         including the null terminator. If the input address is NULL, returns 0.
 *
 * Usage Example:
 * \code{.c}
 * cardano_base_address_t* base_address = ...; // Assume this is initialized
 * size_t bech32_size = cardano_base_address_get_bech32_size(base_address);
 *
 * if (bech32_size > 0)
 * {
 *
 *   char* bech32_string = (char*)malloc(bech32_size);
 *
 *   if (bech32_string)
 *   {
 *     // Proceed to encode or use the bech32_string as needed
 *     free(bech32_string);
 *   }
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_base_address_get_bech32_size(const cardano_base_address_t* address);

/**
 * \brief Encodes a base address into a Bech32 formatted string.
 *
 * This function converts a \ref cardano_base_address_t object into its corresponding
 * Bech32 string representation. The caller must provide a buffer of sufficient size to hold
 * the Bech32 encoded string, which can be determined by \ref cardano_base_address_get_bech32_size.
 *
 * \param[in] address A constant pointer to the \ref cardano_base_address_t object to be encoded.
 * \param[out] data A pointer to a buffer where the Bech32 string will be written.
 * \param[in] size The size of the buffer provided in \p data. This size must be at least as large
 *                 as the value returned by \ref cardano_base_address_get_bech32_size to ensure
 *                 successful encoding.
 *
 * \return Returns \ref CARDANO_SUCCESS if the address was successfully encoded into Bech32 format.
 *         If the provided buffer is too small, returns \ref CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE.
 *         If \p address or \p data is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_base_address_t* base_address = ...; // Assume this is initialized
 * size_t bech32_size = cardano_base_address_get_bech32_size(base_address);
 *
 * char* bech32_string = (char*)malloc(bech32_size);
 *
 * if (bech32_string)
 * {
 *   cardano_error_t result = cardano_base_address_to_bech32(base_address, bech32_string, bech32_size);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     printf("Bech32 Address: %s\n", bech32_string);
 *   }
 *   else
 *   {
 *     printf("Failed to encode base address to Bech32: %d\n", result);
 *   }
 *
 *   free(bech32_string);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_base_address_to_bech32(
  const cardano_base_address_t* address,
  char*                         data,
  size_t                        size);

/**
 * \brief Retrieves the string representation of a base address.
 *
 * This function provides access to the string representation of a \ref cardano_base_address_t object.
 * The string is in Bech32 or another. This function does not allocate new memory for the string; instead,
 * it returns a pointer to the internal representation within the object.
 *
 * \param[in] address A constant pointer to the \ref cardano_base_address_t object from which
 *                    the string representation is to be retrieved.
 *
 * \return A constant pointer to the internal string representation of the address. If the address
 *         is NULL, returns NULL. The returned string should not be modified or freed by the caller,
 *         as it is managed internally by the \ref cardano_base_address_t object.
 *
 * Usage Example:
 * \code{.c}
 * cardano_base_address_t* base_address = ...; // Assume this is initialized
 * const char* address_str = cardano_base_address_get_string(base_address);
 *
 * if (address_str)
 * {
 *   printf("Base Address: %s\n", address_str);
 * }
 * else
 * {
 *   printf("Failed to retrieve string representation of the base address.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_base_address_get_string(const cardano_base_address_t* address);

/**
 * \brief Retrieves the network ID from a given Cardano base address.
 *
 * This function extracts the network identifier from the provided \ref cardano_base_address_t object.
 * The network ID indicates whether the address belongs to the test network or the main network.
 *
 * \param[in] address A constant pointer to the \ref cardano_base_address_t object from which the network ID is to be retrieved.
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
 * cardano_base_address_t* address = NULL;
 * cardano_network_id_t network_id;
 *
 * cardano_error_t get_network_id = cardano_base_address_get_network_id(address, &network_id);
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
 * cardano_base_address_unref(&address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_base_address_get_network_id(
  const cardano_base_address_t* address,
  cardano_network_id_t*         network_id);

/**
 * \brief Decrements the base address's reference count.
 *
 * If the reference count reaches zero, the base address memory is deallocated.
 *
 * \param[in] address Pointer to the base address whose reference count is to be decremented.
 */
CARDANO_EXPORT void cardano_base_address_unref(cardano_base_address_t** address);

/**
 * \brief Increments the base address's reference count.
 *
 * Ensures that the base address remains allocated until the last reference is released.
 *
 * \param[in] address base address whose reference count is to be incremented.
 */
CARDANO_EXPORT void cardano_base_address_ref(cardano_base_address_t* address);

/**
 * \brief Retrieves the base address's current reference count.
 *
 * \warning Does not account for transitive references.
 *
 * \param[in] address Target base address.
 * \return Current reference count of the base address.
 */
CARDANO_EXPORT size_t cardano_base_address_refcount(const cardano_base_address_t* address);

/**
 * \brief Sets the last error message for a given base address.
 *
 * This function records an error message in the base address's last_error buffer,
 * overwriting any previous message. The message is truncated if it exceeds
 * the buffer size. This function is typically used to store descriptive
 * error information that can be retrieved later with
 * cardano_base_address_get_last_error.
 *
 * \param[in,out] address A pointer to the cardano_base_address_t instance whose last error
 *               message is to be set. If the base address is NULL, the function
 *               has no effect.
 * \param[in] message A null-terminated string containing the error message to be
 *                recorded. If the message is NULL, the base address's last_error
 *                will be set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters due to the fixed size
 *       of the last_error buffer (1024 characters), including the null
 *       terminator. Messages longer than this limit will be truncated.
 */
CARDANO_EXPORT void cardano_base_address_set_last_error(cardano_base_address_t* address, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific base address.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_base_address_set_last_error for the given
 * base address. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in,out] address A pointer to the \ref cardano_base_address_t instance whose last error
 *               message is to be retrieved. If the base address is \c NULL, the function
 *               returns a generic error message indicating the null base address.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified base address. If the base address is \c NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the base address and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_base_address_set_last_error for the same base address, or until
 *       the base address is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_base_address_get_last_error(const cardano_base_address_t* address);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BASE_ADDRESS_H