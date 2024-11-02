/**
 * \file address.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_ADDRESS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_ADDRESS_H

/* INCLUDES ******************************************************************/

#include <cardano/address/address_type.h>
#include <cardano/common/network_id.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* FORWARD DECLARATIONS *****************************************************/

/**
 * \brief Defines a pointer to a structure representing a Cardano base address.
 */
typedef struct cardano_base_address_t cardano_base_address_t;

/**
 * \brief Represents a Byron-era address in the Cardano blockchain.
 */
typedef struct cardano_byron_address_t cardano_byron_address_t;

/**
 * \brief Represents an enterprise address in the Cardano blockchain ecosystem.
 *
 * Enterprise addresses carry no stake rights, so using these addresses means that you are opting out of participation
 * in the proof-of-stake protocol.
 *
 * Note that using addresses with no stake rights effectively decreases the total amount of stake, which plays
 * into the hands of a potential adversary.
 */
typedef struct cardano_enterprise_address_t cardano_enterprise_address_t;

/**
 * \brief Represents a pointer address in the Cardano blockchain ecosystem.
 *
 * A pointer address indirectly specifies the stake key that should control the stake for the address. It references
 * a stake key by a stake key pointer, which is a location on the blockchain of the stake key registration certificate
 * for that key.
 */
typedef struct cardano_pointer_address_t cardano_pointer_address_t;

/**
 * \brief Represents a reward address in the Cardano blockchain ecosystem.
 */
typedef struct cardano_reward_address_t cardano_reward_address_t;

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a Cardano address.
 *
 * This structure encapsulates all the necessary information for a Cardano address.
 */
typedef struct cardano_address_t cardano_address_t;

/**
 * \brief Constructs a Cardano address from a serialized byte array.
 *
 * This function decodes a byte array into a \ref cardano_address_t object. The function ensures that the
 * byte array provided conforms to the expected format for a Cardano address before creating the
 * address object.
 *
 * \param[in] data A pointer to the byte array containing the serialized address data.
 * \param[in] size The size of the byte array.
 * \param[out] address A pointer to a pointer of \ref cardano_address_t that will be set to the
 *                     address of the newly created address object upon successful decoding.
 *
 * \return A \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the address was successfully created, or an appropriate error code if an error occurred,
 *         such as invalid format or insufficient data for decoding.
 *
 * \note The caller is responsible for freeing the created \ref cardano_address_t object by calling
 *       \ref cardano_address_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * const byte_t serialized_address[] = { ... };
 * size_t size = sizeof(serialized_address);
 * cardano_address_t* address = NULL;
 *
 * cardano_error_t result = cardano_address_from_bytes(serialized_address, size, &address);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the address
 *
 *   // Clean up
 *   cardano_address_unref(&address);
 * }
 * else
 * {
 *   printf("Failed to decode address: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_address_from_bytes(
  const byte_t*       data,
  size_t              size,
  cardano_address_t** address);

/**
 * \brief Retrieves the size of the byte array required to serialize a Cardano address.
 *
 * This function calculates the number of bytes necessary to serialize the specified
 * \ref cardano_address_t object. It is useful for allocating the appropriate amount of memory
 * before serializing the address using \ref cardano_address_to_bytes.
 *
 * \param[in] address A constant pointer to the \ref cardano_address_t object whose serialized
 *                     size is to be determined. The object must not be NULL.
 *
 * \return The size in bytes required to store the serialized form of the address. If the input
 *         address is NULL, the function returns 0, indicating an error in usage.
 *
 * Usage Example:
 * \code{.c}
 * cardano_address_t* address = cardano_address_from_bytes(...);
 *
 * if (address)
 * {
 *   size_t required_size = cardano_address_get_bytes_size(address);
 *   byte_t* buffer = (byte_t*)malloc(required_size);
 *
 *   if (buffer)
 *   {
 *     // Serialize the address into buffer
 *     cardano_address_to_bytes(address, buffer, required_size);
 *     // Use the buffer ...
 *     free(buffer);
 *   }
 *
 *   // Clean up the address object
 *   cardano_address_unref(&address);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_address_get_bytes_size(
  const cardano_address_t* address);

/**
 * \brief Serializes a Cardano address into a byte array.
 *
 * This function serializes the given \ref cardano_address_t object into a preallocated buffer.
 * The buffer must be large enough to hold the serialized data, which should be determined
 * by calling \ref cardano_address_get_bytes_size prior to this function.
 *
 * \param[in] address  A constant pointer to the \ref cardano_address_t object that is to be serialized.
 *                     The object must not be NULL.
 * \param[out] data  A pointer to the buffer where the serialized data will be written.
 * \param[in] size  The size of the buffer pointed to by \p data. It must be at least as large
 *                  as the value returned by \ref cardano_address_get_bytes_size for successful serialization.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the buffer \p data
 *         is too small, returns \ref CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE. If \p address or \p data is NULL,
 *         returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_address_t* address = cardano_address_from_bytes(...);
 * size_t required_size = cardano_address_get_bytes_size(address);
 * byte_t* buffer = (byte_t*)malloc(required_size);
 *
 * if (buffer)
 * {
 *   cardano_error_t result = cardano_address_to_bytes(address, buffer, required_size);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     // Use the serialized data
 *   }
 *
 *   free(buffer);
 * }
 *
 * cardano_address_unref(&address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_address_to_bytes(
  const cardano_address_t* address,
  byte_t*                  data,
  size_t                   size);

/**
 * \brief Retrieves a pointer to the internal byte array of a Cardano address.
 *
 * This function provides read-only access to the internal byte representation of a Cardano address.
 * The returned pointer points directly to the internal data structure within the \ref cardano_address_t object.
 * This approach avoids copying data and allows efficient access to the address's byte representation.
 *
 * \param[in] address  A constant pointer to the \ref cardano_address_t object whose byte array is to be accessed.
 *
 * \return A constant pointer to the internal byte array of the Cardano address. This pointer should not be modified
 *         or freed by the caller, as it is managed internally by the Cardano library. The content pointed to remains
 *         valid as long as the \ref cardano_address_t object itself is not freed or modified.
 *
 * \note Modifying or freeing the data pointed to by the returned pointer will lead to undefined behavior. Treat this pointer
 * as read-only.
 *
 * Usage Example:
 * \code{.c}
 * cardano_address_t* address = cardano_address_from_bytes(...);
 * const byte_t* byte_ptr = cardano_address_get_bytes(address);
 *
 * if (byte_ptr)
 * {
 *   // Read the bytes but do not modify them
 * }
 *
 * cardano_address_unref(&address);  // This also invalidates byte_ptr
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const byte_t* cardano_address_get_bytes(
  const cardano_address_t* address);

/**
 * \brief Creates a Cardano address from a string representation.
 *
 * This function constructs a \ref cardano_address_t object by parsing the provided string. The string
 * should contain a valid representation of a Cardano address, either in base58 or Bech32 format.
 *
 * \param[in] data A pointer to the character array containing the string representation of the address.
 * \param[in] size The length of the string pointed to by \p data, not including the null terminator.
 * \param[out] address A pointer to a pointer to \ref cardano_address_t that will be set to the address
 *                     of the newly created Cardano address object upon successful parsing. The caller
 *                     is responsible for managing the lifecycle of this object, including freeing it
 *                     when it is no longer needed using \ref cardano_address_unref.
 *
 * \return Returns \ref CARDANO_SUCCESS if the address is successfully created from the string. Returns
 *         \ref CARDANO_ERROR_INVALID_ADDRESS_FORMAT if the string is not a valid address format. If the \p data is
 *         NULL or \p address is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * const char* address_str = "addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgse35a3x";
 * size_t size = strlen(address_str);
 * cardano_address_t* address = NULL;
 *
 * cardano_error_t result = cardano_address_from_string(address_str, size, &address);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the address
 * }
 * else
 * {
 *   printf("Failed to parse address: %d\n", result);
 * }
 *
 * // Clean up the address object once done
 * cardano_address_unref(&address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_address_from_string(
  const char*         data,
  size_t              size,
  cardano_address_t** address);

/**
 * \brief Retrieves the size needed for the string representation of a Cardano address.
 *
 * This function calculates the size of the buffer required to hold the string representation
 * of a \ref cardano_address_t object, including the null terminator. This size is necessary
 * to ensure that the buffer allocated for converting the address to a string is sufficient.
 *
 * \param[in] address A constant pointer to the \ref cardano_address_t object for which the string
 *                    size is being calculated. The object must not be NULL.
 *
 * \return The size in bytes needed to store the string representation of the address, including
 *         the null terminator. If the input \p address is NULL, the behavior is undefined.
 *
 * Usage Example:
 * \code{.c}
 * cardano_address_t* address = cardano_address_new(...);
 * size_t required_size = cardano_address_get_string_size(address);
 *
 * char* address_str = (char*)malloc(required_size);
 *
 * if (address_str)
 * {
 *   cardano_error_t result = cardano_address_to_string(address, address_str, required_size);
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     printf("Address: %s\n", address_str);
 *   }
 *   free(address_str);
 * }
 *
 * // Clean up the address object once done
 * cardano_address_unref(&address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_address_get_string_size(const cardano_address_t* address);

/**
 * \brief Converts a Cardano address into its string representation.
 *
 * This function serializes the given \ref cardano_address_t object into a string format.
 * The string is written to a user-provided buffer, and the size of this buffer must be
 * adequate to hold the entire string, including the null terminator. The required size
 * can be determined by calling \ref cardano_address_get_string_size.
 *
 * \param[in] address A constant pointer to the \ref cardano_address_t object that is to be converted to a string.
 *                    The object must not be NULL.
 * \param[out] data A pointer to the buffer where the string representation of the address will be written.
 * \param[in] size The size of the buffer pointed to by \p data. This size should be at least as large as the value
 *                 returned by \ref cardano_address_get_string_size to ensure successful serialization.
 *
 * \return Returns \ref CARDANO_SUCCESS if the conversion is successful. If the buffer is too small, returns
 *         \ref CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE. If the \p address or \p data is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_address_t* address = cardano_address_new(...);
 * size_t required_size = cardano_address_get_string_size(address);
 * char* address_str = (char*)malloc(required_size);
 *
 * if (address_str)
 * {
 *   cardano_error_t result = cardano_address_to_string(address, address_str, required_size);
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     printf("Address: %s\n", address_str);
 *   }
 *
 *   free(address_str);
 * }
 *
 * // Clean up the address object once done
 * cardano_address_unref(&address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_address_to_string(
  const cardano_address_t* address,
  char*                    data,
  size_t                   size);

/**
 * \brief Retrieves the string representation of a Cardano address.
 *
 * This function provides access to the string form of the \ref cardano_address_t object. The function
 * returns a pointer to a constant string which represents the address. This string should not be modified
 * or freed by the user, as it is managed internally by the address object.
 *
 * \param[in] address A constant pointer to the \ref cardano_address_t object from which
 *                    the string is to be retrieved.
 *
 * \return A constant pointer to the internal string representation of the address. If the input
 *         \p address is NULL, the return value is NULL, indicating an error.
 *
 * \note The returned string will remain valid as long as the \ref cardano_address_t object is not modified
 *       or freed. Once \ref cardano_address_unref is called on the address object, the string pointer
 *       should be considered invalid.
 *
 * Usage Example:
 * \code{.c}
 * cardano_address_t* address = cardano_address_new(...);
 * const char* address_str = cardano_address_get_string(address);
 *
 * if (address_str)
 * {
 *   printf("Address: %s\n", address_str);
 * }
 *
 * // Clean up the address object once done
 * cardano_address_unref(&address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_address_get_string(const cardano_address_t* address);

/**
 * \brief Checks if the provided string is a valid Bech32-encoded Cardano address.
 *
 * This function verifies if the given string conforms to the Bech32 encoding format specific to Cardano addresses.
 *
 * \param[in] data A pointer to the character array containing the potential Bech32 address.
 * \param[in] size The length of the string pointed to by \p data.
 *
 * \return \c true if the string is a valid Bech32-encoded Cardano address; otherwise, \c false.
 *
 * Usage Example:
 * \code{.c}
 * const char* address = "addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsqqxw8x";
 * size_t address_length = strlen(address);
 *
 * bool is_valid = cardano_address_is_valid_bech32(address, address_length);
 *
 * if (is_valid)
 * {
 *   printf("The address is a valid Bech32 Cardano address.\n");
 * }
 * else
 * {
 *   printf("The address is not a valid Bech32 Cardano address.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_address_is_valid_bech32(const char* data, size_t size);

/**
 * \brief Checks if the provided string is a valid Byron-encoded Cardano address.
 *
 * This function verifies if the given string conforms to the Byron address encoding format used in earlier versions of the Cardano blockchain. It performs checks on the structure and integrity of the address, ensuring it follows the expected format and contains a valid checksum.
 *
 * \param[in] data A pointer to the character array containing the potential Byron address.
 * \param[in] size The length of the string pointed to by \p data.
 *
 * \return \c true if the string is a valid Byron-encoded Cardano address; otherwise, \c false.
 *
 * \note This function does not modify the input string and does not allocate any dynamic memory.
 *
 * Usage Example:
 * \code{.c}
 * const char* address = "Ae2tdPwUPEZFRbyhz3cpfC2CumGzNkFBN2L42rcUc2yjQpEkxDbkPodpMAi";
 * size_t address_length = strlen(address);
 *
 * bool is_valid = cardano_address_is_valid_byron(address, address_length);
 *
 * if (is_valid)
 * {
 *   printf("The address is a valid Byron Cardano address.\n");
 * }
 * else
 * {
 *   printf("The address is not a valid Byron Cardano address.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_address_is_valid_byron(const char* data, size_t size);

/**
 * \brief Checks if the provided string is a valid Cardano address.
 *
 * This function verifies if the given string conforms to the encoding formats used for Cardano addresses,
 * including both Byron and Shelley address formats. It checks the structure and integrity of the address, ensuring
 * it follows the expected format.
 *
 * \param[in] data A pointer to the character array containing the potential Cardano address.
 * \param[in] size The length of the string pointed to by \p data.
 *
 * \return \c true if the string is a valid Cardano address; otherwise, \c false.
 *
 * Usage Example:
 * \code{.c}
 * const char* address = "addr1qxy2cnx2l2s54at80au205tswxpnuvsgsn9rqlat4zuyghqp6v3hj4t4c7szlevu9hxy6w2scv0hecqlq5v3x6jts0sqqx5scn";
 * size_t address_length = strlen(address);
 *
 * bool is_valid = cardano_address_is_valid(address, address_length);
 *
 * if (is_valid)
 * {
 *   printf("The address is a valid Cardano address.\n");
 * }
 * else
 * {
 *   printf("The address is not a valid Cardano address.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_address_is_valid(const char* data, size_t size);

/**
 * \brief Retrieves the type of a given Cardano address.
 *
 * This function examines the provided \ref cardano_address_t object and determines its address type,
 * such as whether it is a base, pointer, enterprise, reward, or Byron address. The address type is determined
 * based on the structure and prefix of the address.
 *
 * \param[in] address A constant pointer to the \ref cardano_address_t object whose type is to be determined.
 *                    The object must not be NULL.
 * \param[out] type A pointer to \ref cardano_address_type_t where the address type will be stored upon successful
 *                  determination. This parameter cannot be NULL.
 *
 * \return Returns \ref CARDANO_SUCCESS if the address type was successfully determined.
 *         Returns \ref CARDANO_ERROR_POINTER_IS_NULL if the input address or type pointer is NULL.
 *         Returns other error codes as defined in \ref cardano_error_t if the address type cannot be determined
 *         due to the address not conforming to any known Cardano address format.
 *
 * Usage Example:
 * \code{.c}
 * cardano_address_t* address = NULL;
 * cardano_address_type_t address_type;
 *
 * cardano_error_t result = cardano_address_from_string("addr1...", strlen("addr1..."), &address);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   cardano_error_t get_type_result = cardano_address_get_type(address, &address_type);
 *
 *   if (get_type_result != CARDANO_SUCCESS)
 *   {
 *     printf("Failed to determine the address type.\n");
 *   }
 *   else
 *   {
 *     printf("Address type: %d\n", address_type);
 *   }
 * }
 * else
 * {
 *   printf("Failed to determine the address type.\n");
 * }
 *
 * cardano_address_unref(&address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_address_get_type(
  const cardano_address_t* address,
  cardano_address_type_t*  type);

/**
 * \brief Retrieves the network ID from a given Cardano address.
 *
 * This function extracts the network identifier from the provided \ref cardano_address_t object.
 * The network ID indicates whether the address belongs to the test network or the main network.
 *
 * \param[in] address A constant pointer to the \ref cardano_address_t object from which the network ID is to be retrieved.
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
 * cardano_address_t* address = NULL;
 * cardano_network_id_t network_id;
 *
 * cardano_error_t result = cardano_address_from_string("addr1...", strlen("addr1..."), &address);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   cardano_error_t get_network_id = cardano_address_get_network_id(address, &network_id);
 *
 *   if (get_network_id != CARDANO_SUCCESS)
 *   {
 *     printf("Failed to determine the address network id.\n");
 *   }
 *   else
 *   {
 *     printf("Network ID: %d\n", network_id);
 *   }
 * }
 * else
 * {
 *   printf("Failed to retrieve the network ID.\n");
 * }
 *
 * cardano_address_unref(&address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_address_get_network_id(
  const cardano_address_t* address,
  cardano_network_id_t*    network_id);

/**
 * \brief Converts a general Cardano address to a Byron-specific address format.
 *
 * This function creates a \ref cardano_byron_address_t object from a given \ref cardano_address_t object.
 *
 * \param[in] address A constant pointer to the \ref cardano_address_t object to be converted.
 *                    The object must not be NULL and must represent a valid Byron address.
 *
 * \return A pointer to a newly created \ref cardano_byron_address_t object if the conversion is successful.
 *         Returns NULL if the address cannot be converted to a Byron address due to an invalid format or
 *         if the provided address does not correspond to a Byron address. The caller is responsible for managing
 *         the lifecycle of the returned object, including freeing it when it is no longer needed using
 *         \ref cardano_byron_address_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_byron_address_t* byron_address = cardano_address_to_byron_address(address);
 *
 * if (byron_address)
 * {
 *   // Byron address is successfully converted
 *   // Proceed with operations that require a Byron address
 *
 *   cardano_byron_address_unref(&byron_address);
 * }
 * else
 * {
 *   printf("Conversion to Byron address format failed.\n");
 * }
 *
 * cardano_address_unref(&shelley_address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_byron_address_t* cardano_address_to_byron_address(const cardano_address_t* address);

/**
 * \brief Converts a general Cardano address to a reward-specific address format.
 *
 * This function creates a \ref cardano_reward_address_t object from a given \ref cardano_address_t object.
 *
 * \param[in] address A constant pointer to the \ref cardano_address_t object to be converted.
 *                    The object must not be NULL and must represent a valid reward address.
 *
 * \return A pointer to a newly created \ref cardano_reward_address_t object if the conversion is successful.
 *         Returns NULL if the address cannot be converted to a reward address due to an invalid format or
 *         if the provided address does not correspond to a reward address. The caller is responsible for managing
 *         the lifecycle of the returned object, including freeing it when it is no longer needed using
 *         \ref cardano_reward_address_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_reward_address_t* reward_address = cardano_address_to_reward_address(address);
 *
 * if (reward_address)
 * {
 *   // Reward address is successfully converted
 *   // Proceed with operations that require a reward address
 *
 *   cardano_reward_address_unref(&reward_address);
 * }
 * else
 * {
 *   printf("Conversion to reward address format failed.\n");
 * }
 *
 * cardano_address_unref(&general_address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_reward_address_t* cardano_address_to_reward_address(const cardano_address_t* address);

/**
 * \brief Converts a general Cardano address to a pointer address format.
 *
 * This function creates a \ref cardano_pointer_address_t object from a given \ref cardano_address_t object.
 *
 * \param[in] address A constant pointer to the \ref cardano_address_t object to be converted.
 *                    The object must not be NULL and must represent a valid pointer address.
 *
 * \return A pointer to a newly created \ref cardano_pointer_address_t object if the conversion is successful.
 *         Returns NULL if the address cannot be converted to a pointer address due to an invalid format or
 *         if the provided address does not correspond to a pointer address. The caller is responsible for managing
 *         the lifecycle of the returned object, including freeing it when it is no longer needed using
 *         \ref cardano_pointer_address_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_address_t* address = ...;
 * cardano_pointer_address_t* pointer_address = cardano_address_to_pointer_address(address);
 *
 * if (pointer_address)
 * {
 *   // Pointer address is successfully converted
 *   // Proceed with operations that require a pointer address
 *
 *   cardano_pointer_address_unref(&pointer_address);
 * }
 * else
 * {
 *   printf("Conversion to pointer address format failed.\n");
 * }
 *
 * cardano_address_unref(&general_address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_pointer_address_t* cardano_address_to_pointer_address(const cardano_address_t* address);

/**
 * \brief Converts a general Cardano address to an enterprise address format.
 *
 * This function creates a \ref cardano_enterprise_address_t object from a given \ref cardano_address_t object.
 *
 * \param[in] address A constant pointer to the \ref cardano_address_t object to be converted.
 *                    The object must not be NULL and must represent a valid enterprise address.
 *
 * \return A pointer to a newly created \ref cardano_enterprise_address_t object if the conversion is successful.
 *         Returns NULL if the address cannot be converted to an enterprise address due to an invalid format or
 *         if the provided address does not correspond to an enterprise address. The caller is responsible for managing
 *         the lifecycle of the returned object, including freeing it when it is no longer needed using
 *         \ref cardano_enterprise_address_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_address_t* address = ...;
 * cardano_enterprise_address_t* enterprise_address = cardano_address_to_enterprise_address(address);
 *
 * if (enterprise_address)
 * {
 *   // Enterprise address is successfully converted
 *   // Proceed with operations that require an enterprise address
 *
 *   cardano_enterprise_address_unref(&enterprise_address);
 * }
 * else
 * {
 *   printf("Conversion to enterprise address format failed.\n");
 * }
 *
 * cardano_address_unref(&general_address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_enterprise_address_t* cardano_address_to_enterprise_address(const cardano_address_t* address);

/**
 * \brief Converts a general Cardano address to a base address format.
 *
 * This function creates a \ref cardano_base_address_t object from a given \ref cardano_address_t object.
 *
 * \param[in] address A constant pointer to the \ref cardano_address_t object to be converted.
 *                    The object must not be NULL and must represent a valid base address.
 *
 * \return A pointer to a newly created \ref cardano_base_address_t object if the conversion is successful.
 *         Returns NULL if the address cannot be converted to a base address due to an invalid format or
 *         if the provided address does not correspond to a base address. The caller is responsible for managing
 *         the lifecycle of the returned object, including freeing it when it is no longer needed using
 *         \ref cardano_base_address_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_address_t* address = ...;
 * cardano_base_address_t* base_address = cardano_address_to_base_address(address);
 *
 * if (base_address)
 * {
 *   // Base address is successfully converted
 *   // Proceed with operations that require a base address
 *
 *   cardano_base_address_unref(&base_address);
 * }
 * else
 * {
 *   printf("Conversion to base address format failed.\n");
 * }
 *
 * cardano_address_unref(&general_address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_base_address_t* cardano_address_to_base_address(const cardano_address_t* address);

/**
 * \brief Compares two address objects for equality.
 *
 * This function compares two address objects to determine if they are equal.
 *
 * \param[in] lhs Pointer to the first address object.
 * \param[in] rhs Pointer to the second address object.
 *
 * \return \c true if the address objects are equal, \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_address_t* address1 = NULL;
 * cardano_address_t* address2 = NULL;
 *
 * // Assume address1 and address2 are initialized properly
 *
 * bool are_equal = cardano_address_equals(address1, address2);
 *
 * if (are_equal)
 * {
 *   printf("The address objects are equal.\n");
 * }
 * else
 * {
 *   printf("The address objects are not equal.\n");
 * }
 *
 * // Clean up
 * cardano_address_unref(&address1);
 * cardano_address_unref(&address2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_address_equals(const cardano_address_t* lhs, const cardano_address_t* rhs);

/**
 * \brief Decrements the address's reference count.
 *
 * If the reference count reaches zero, the address memory is deallocated.
 *
 * \param[in] address Pointer to the address whose reference count is to be decremented.
 */
CARDANO_EXPORT void cardano_address_unref(cardano_address_t** address);

/**
 * \brief Increments the address's reference count.
 *
 * Ensures that the address remains allocated until the last reference is released.
 *
 * \param[in] address address whose reference count is to be incremented.
 */
CARDANO_EXPORT void cardano_address_ref(cardano_address_t* address);

/**
 * \brief Retrieves the address's current reference count.
 *
 * \warning Does not account for transitive references.
 *
 * \param[in] address Target address.
 * \return Current reference count of the address.
 */
CARDANO_EXPORT size_t cardano_address_refcount(const cardano_address_t* address);

/**
 * \brief Sets the last error message for a given address.
 *
 * This function records an error message in the address's last_error buffer,
 * overwriting any previous message. The message is truncated if it exceeds
 * the buffer size. This function is typically used to store descriptive
 * error information that can be retrieved later with
 * cardano_address_get_last_error.
 *
 * \param[in,out] address A pointer to the cardano_address_t instance whose last error
 *               message is to be set. If the address is NULL, the function
 *               has no effect.
 * \param[in] message A null-terminated string containing the error message to be
 *                recorded. If the message is NULL, the address's last_error
 *                will be set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters due to the fixed size
 *       of the last_error buffer (1024 characters), including the null
 *       terminator. Messages longer than this limit will be truncated.
 */
CARDANO_EXPORT void cardano_address_set_last_error(cardano_address_t* address, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific address.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_address_set_last_error for the given
 * address. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in,out] address A pointer to the \ref cardano_address_t instance whose last error
 *               message is to be retrieved. If the address is \c NULL, the function
 *               returns a generic error message indicating the null address.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified address. If the address is \c NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the address and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_address_set_last_error for the same address, or until
 *       the address is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_address_get_last_error(const cardano_address_t* address);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_ADDRESS_H