/**
 * \file byron_address.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BYRON_ADDRESS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BYRON_ADDRESS_H

/* INCLUDES ******************************************************************/

#include <cardano/address/byron_address_attributes.h>
#include <cardano/address/byron_address_type.h>
#include <cardano/common/network_id.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

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
 * \brief Represents a Byron-era address in the Cardano blockchain.
 */
typedef struct cardano_byron_address_t cardano_byron_address_t;

/**
 * \brief Creates a Byron-era address from given credentials.
 *
 * This function generates a \ref cardano_byron_address_t object based on the provided root hash, address attributes,
 * and address type.
 *
 * \param[in] root A pointer to \ref cardano_blake2b_hash_t representing the root hash of the address.
 * \param[in] attributes A \ref cardano_byron_address_attributes_t structure containing optional address attributes
 *                       such as derivation path and network magic.
 * \param[in] type The type of Byron address, defined by \ref cardano_byron_address_type_t, which indicates
 *                 whether the address is based on a public key, a script, or a redeem key.
 * \param[out] address A pointer to a pointer of \ref cardano_byron_address_t where the newly created Byron
 *                     address object will be stored.
 *
 * \return A \ref cardano_error_t indicating the result of the operation. Returns \ref CARDANO_SUCCESS on success
 *         or an appropriate error code on failure.
 *
 * \note It is the caller's responsibility to manage the lifecycle of the created \ref cardano_byron_address_t object,
 *       including freeing it when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_t* root_hash = ...; // Assume root_hash is initialized
 * cardano_byron_address_attributes_t attributes = { .derivation_path = "...", .magic = 0x1234 };
 * cardano_byron_address_t* byron_address = NULL;
 *
 * cardano_error_t result = cardano_byron_address_from_credentials(
 *      root_hash, attributes,
 *      CARDANO_BYRON_ADDRESS_TYPE_PUBKEY,
 *      &byron_address);
 *
 * if (result == CARDANO_SUCCESS && byron_address)
 * {
 *   // Byron address has been successfully created and can be used
 *   // Free the Byron address when done
 *   cardano_byron_address_unref(byron_address);
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_byron_address_from_credentials(
  cardano_blake2b_hash_t*            root,
  cardano_byron_address_attributes_t attributes,
  cardano_byron_address_type_t       type,
  cardano_byron_address_t**          address);

/**
 * \brief Converts a generic Cardano address to a Byron-era address.
 *
 * This function attempts to convert a \ref cardano_address_t object into a \ref cardano_byron_address_t object.
 *
 * \param[in] address A constant pointer to the generic \ref cardano_address_t object to be converted.
 * \param[out] byron_address A pointer to a pointer of \ref cardano_byron_address_t where the newly created
 *                           Byron address object will be stored if the conversion is successful.
 *
 * \return A \ref cardano_error_t indicating the result of the operation. Returns \ref CARDANO_SUCCESS on successful
 *         conversion, \ref CARDANO_ERROR_INVALID_ADDRESS_TYPE if the input address is not a valid Byron address, or other
 *         appropriate error codes for different failures.
 *
 * \note The caller is responsible for managing the lifecycle of the created \ref cardano_byron_address_t object,
 *       including freeing it when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_address_t* generic_address = ...; // Assume this is initialized and points to a Byron address
 * cardano_byron_address_t* byron_address = NULL;
 *
 * cardano_error_t result = cardano_byron_address_from_address(generic_address, &byron_address);
 *
 * if (result == CARDANO_SUCCESS && byron_address)
 * {
 *   // Byron address has been successfully converted and can be used
 *   // Free the Byron address when done
 *   cardano_byron_address_unref(byron_address);
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_byron_address_from_address(
  const cardano_address_t*  address,
  cardano_byron_address_t** byron_address);

/**
 * \brief Converts a Byron-era address to a generic Cardano address.
 *
 * This function converts a \ref cardano_byron_address_t object to a \ref cardano_address_t object.
 *
 * \param[in] byron_address A constant pointer to the \ref cardano_byron_address_t object that represents the
 *                          Byron-era address to be converted.
 *
 * \return A pointer to a new \ref cardano_address_t object representing the generic Cardano address. This object
 *         represents a "strong reference" to the address, meaning that it is fully initialized and ready for use.
 *         The caller is responsible for managing the lifecycle of this object, including freeing it when it is
 *         no longer needed.
 *
 * \note If the conversion fails, NULL is returned. It is the caller's responsibility to check the return value
 *       before using it.
 *
 * Usage Example:
 * \code{.c}
 * cardano_byron_address_t* byron_address = ...; // Assume this is initialized
 * cardano_address_t* cardano_address = cardano_byron_address_to_address(byron_address);
 *
 * if (cardano_address)
 * {
 *   // Use the generic Cardano address
 *   // Remember to free the Cardano address when done
 *   cardano_address_unref(&cardano_address);
 * }
 * else
 * {
 *   // Handle conversion failure
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_address_t* cardano_byron_address_to_address(
  const cardano_byron_address_t* byron_address);

/**
 * \brief Retrieves attributes from a Byron-era address.
 *
 * This function extracts the attributes associated with a Byron-era address encapsulated in a \ref cardano_byron_address_t object.
 * These attributes might include optional fields such as a derivation path or a network magic identifier, which were specific to
 * the Byron address format.
 *
 * \param[in] address A constant pointer to the \ref cardano_byron_address_t object from which the attributes are to be retrieved.
 * \param[out] attributes A pointer to a \ref cardano_byron_address_attributes_t structure where the attributes of the address will be stored.
 *
 * \return \ref CARDANO_SUCCESS if the attributes were successfully retrieved, or an appropriate error code if an error occurred.
 *
 * \note The function assumes the \p address pointer and the \p attributes pointer are not NULL. If either is NULL, the function
 *       returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_byron_address_t* byron_address = ...; // Assume this is initialized
 * cardano_byron_address_attributes_t attributes;
 * cardano_error_t result = cardano_byron_address_get_attributes(byron_address, &attributes);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the attributes
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to retrieve attributes: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_byron_address_get_attributes(
  const cardano_byron_address_t*      address,
  cardano_byron_address_attributes_t* attributes);

/**
 * \brief Retrieves the type of a Byron-era address.
 *
 * This function extracts the type from a Byron-era address encapsulated in a \ref cardano_byron_address_t object.
 * The type determines the nature of the credentials used in the address, such as a public key, a script, or a redeem key.
 *
 * \param[in] address A constant pointer to the \ref cardano_byron_address_t object from which the type is to be retrieved.
 * \param[out] type A pointer to a \ref cardano_byron_address_type_t variable where the type of the address will be stored.
 *
 * \return \ref CARDANO_SUCCESS if the type was successfully retrieved, or an appropriate error code if an error occurred.
 *
 * \note The function assumes the \p address pointer and the \p type pointer are not NULL. If either is NULL, the function
 *       returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_byron_address_t* byron_address = ...; // Assume this is initialized
 * cardano_byron_address_type_t type;
 * cardano_error_t result = cardano_byron_address_get_type(byron_address, &type);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the retrieved type
 *   switch (type)
 *   {
 *     case CARDANO_BYRON_ADDRESS_TYPE_PUBKEY:
 *       printf("Address type is Public Key\n");
 *       break;
 *     case CARDANO_BYRON_ADDRESS_TYPE_SCRIPT:
 *       printf("Address type is Script\n");
 *       break;
 *     case CARDANO_BYRON_ADDRESS_TYPE_REDEEM:
 *       printf("Address type is Redeem Key\n");
 *       break;
 *     default:
 *       printf("Unknown address type\n");
 *   }
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to retrieve address type: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_byron_address_get_type(
  const cardano_byron_address_t* address,
  cardano_byron_address_type_t*  type);

/**
 * \brief Retrieves the root hash from a Byron-era address.
 *
 * This function extracts the root hash associated with a Byron-era address, which is encapsulated in a \ref cardano_byron_address_t object.
 *
 * \param[in] address A constant pointer to the \ref cardano_byron_address_t object from which the root hash is to be retrieved.
 * \param[out] root A pointer to a pointer of \ref cardano_blake2b_hash_t that will be set to point to the root hash of the address.
 *                  The function will allocate memory for the root hash, and it is the caller's responsibility to free it using
 *                  \ref cardano_blake2b_hash_unref when it is no longer needed.
 *
 * \return \ref CARDANO_SUCCESS if the root hash was successfully retrieved, or an appropriate error code if an error occurred.
 *
 * \note The function assumes the \p address pointer is not NULL and that \p root is a valid double pointer. If either is NULL,
 *       the function returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_byron_address_t* byron_address = ...; // Assume this is initialized
 * cardano_blake2b_hash_t* root_hash = NULL;
 * cardano_error_t result = cardano_byron_address_get_root(byron_address, &root_hash);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the root hash
 *   // ...
 *
 *   // Clean up
 *   cardano_blake2b_hash_unref(&root_hash);
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to retrieve root hash: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_byron_address_get_root(
  const cardano_byron_address_t* address,
  cardano_blake2b_hash_t**       root);

/**
 * \brief Constructs a Byron-era address from a byte array.
 *
 * This function creates a \ref cardano_byron_address_t object from raw byte data, interpreting it as a serialized Byron address.
 * It verifies the structure and validity of the data before constructing the address object.
 *
 * \param[in] data A pointer to the byte array containing the serialized Byron address data.
 * \param[in] size The size of the byte array in bytes.
 * \param[out] address A pointer to a pointer of \ref cardano_byron_address_t that will be set to the address of the newly created
 *                     Byron address object upon successful decoding.
 *
 * \return \ref CARDANO_SUCCESS if the Byron address is successfully created from the provided bytes, \ref CARDANO_ERROR_INVALID_ADDRESS_FORMAT
 *         if the byte array does not represent a valid Byron address, or \ref CARDANO_ERROR_POINTER_IS_NULL if any pointer parameter is NULL.
 *
 * \note The caller is responsible for managing the lifecycle of the created \ref cardano_byron_address_t object. Specifically,
 *       once the address is no longer needed, the caller must release it by calling \ref cardano_address_unref.
 *
 * Usage Example:
 * \code{.c}
 * const byte_t address_data[] = {...}; // Assume this contains valid Byron address bytes
 * size_t address_data_size = sizeof(address_data);
 * cardano_byron_address_t* byron_address = NULL;
 * cardano_error_t result = cardano_byron_address_from_bytes(address_data, address_data_size, &byron_address);
 * if (result == CARDANO_SUCCESS) {
 *   // Byron address is successfully created and can be used
 *   // ...
 *
 *   // Once done, clean up
 *   cardano_address_unref(&byron_address);
 * } else {
 *   printf("Failed to create Byron address from bytes: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_byron_address_from_bytes(
  const byte_t*             data,
  size_t                    size,
  cardano_byron_address_t** address);

/**
 * \brief Retrieves the size in bytes of the serialized form of a Byron-era address.
 *
 * This function calculates the size of the byte array needed to store the serialized form of a given
 * \ref cardano_byron_address_t object. This size is useful for allocating the correct amount of memory
 * for serialization.
 *
 * \param[in] address A constant pointer to the \ref cardano_byron_address_t object whose serialized size is to be determined.
 *
 * \return The size in bytes required to store the serialized form of the Byron address. Returns zero if the input address
 *         is NULL or if any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_byron_address_t* byron_address = ...; // Assume this is a valid Byron address object
 * size_t required_size = cardano_byron_address_get_bytes_size(byron_address);
 *
 * if (required_size > 0)
 * {
 *   byte_t* buffer = (byte_t*)malloc(required_size);
 *
 *   if (buffer)
 *   {
 *     // Proceed to serialize the Byron address into the buffer
 *     ...
 *     free(buffer);
 *   }
 * }
 * else
 * {
 *   printf("Error determining the required size for the serialized Byron address.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_byron_address_get_bytes_size(const cardano_byron_address_t* address);

/**
 * \brief Retrieves a pointer to the internal byte array representation of a Byron-era address.
 *
 * This function provides direct read-only access to the internal byte array that represents
 * a Byron-era address in its serialized form. This is useful for operations that require direct
 * access to the raw data of the address, such as hashing or verification.
 *
 * \param[in] address A constant pointer to the \ref cardano_byron_address_t object from which
 *                    the byte array is to be accessed.
 *
 * \return A pointer to the constant byte array representing the serialized Byron address. If the
 *         input address is NULL, returns NULL. The data should not be modified, and the lifetime
 *         of the returned pointer is tied to the lifetime of the \ref cardano_byron_address_t object.
 *
 * Usage Example:
 * \code{.c}
 * cardano_byron_address_t* byron_address = ...; // Assume this is a valid Byron address object
 * const byte_t* byte_data = cardano_byron_address_get_bytes(byron_address);
 *
 * if (byte_data)
 * {
 *   // Use the byte data for reading purposes
 *   ...
 * }
 * else
 * {
 *   printf("Failed to access the byte data of the Byron address.\n");
 * }
 * \endcode
 *
 * \note Modifying the data pointed to by the returned pointer is undefined behavior.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const byte_t* cardano_byron_address_get_bytes(const cardano_byron_address_t* address);

/**
 * \brief Serializes a Byron-era address into a byte array.
 *
 * This function serializes the given \ref cardano_byron_address_t object into a preallocated buffer in its raw byte form.
 * The buffer size can be determined by calling \ref cardano_byron_address_get_bytes_size prior to this function.
 *
 * \param[in] address A constant pointer to the \ref cardano_byron_address_t object that is to be serialized.
 * \param[out] data A pointer to the buffer where the serialized byte data will be written.
 * \param[in] size The size of the buffer pointed to by \p data. It must be at least as large as the value
 *                 returned by \ref cardano_byron_address_get_bytes_size to ensure successful serialization.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the buffer is too small, returns
 *         \ref CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE. If the \p address is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_byron_address_t* byron_address = cardano_byron_address_new(...);
 * size_t required_size = cardano_byron_address_get_bytes_size(byron_address);
 * byte_t* buffer = (byte_t*)malloc(required_size);
 *
 * if (buffer)
 * {
 *   cardano_error_t result = cardano_byron_address_to_bytes(byron_address, buffer, required_size);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     // Use the serialized data
 *   }
 *   free(buffer);
 * }
 *
 * cardano_byron_address_unref(&byron_address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_byron_address_to_bytes(
  const cardano_byron_address_t* address,
  byte_t*                        data,
  size_t                         size);

/**
 * \brief Creates a Byron-era address from a Base58-encoded string.
 *
 * This function constructs a \ref cardano_byron_address_t object by decoding the provided
 * Base58-encoded string that represents the address data.
 *
 * \param[in] data A pointer to a character array containing the Base58-encoded representation of the address.
 * \param[in] size The size of the Base58 string in bytes.
 * \param[out] address A pointer to a pointer to \ref cardano_byron_address_t that will be set to the address
 *                     of the newly created Byron address object upon successful decoding.
 *
 * \return Returns \ref CARDANO_SUCCESS if the address was successfully created. Returns \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the \p data or \p address pointer is NULL. Returns \ref CARDANO_ERROR_INVALID_ADDRESS_FORMAT if the Base58 data
 *         could not be decoded into a valid Byron address.
 *
 * \note The function checks the format and validity of the Base58 string. It is the caller's responsibility to
 *       ensure that the string is correctly formatted and of appropriate length. Malformed or incorrect Base58
 *       data may lead to decoding errors.
 *
 * Usage Example:
 * \code{.c}
 * const char* base58_address = "3P3QsMVK8...";
 * size_t base58_size = strlen(base58_address);
 * cardano_byron_address_t* byron_address = NULL;
 *
 * cardano_error_t result = cardano_byron_address_from_base58(base58_address, base58_size, &byron_address);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the Byron address
 *
 *   // Once done, ensure to clean up and release the Byron address
 *   cardano_byron_address_unref(&byron_address);
 * }
 * else
 * {
 *   printf("Failed to decode Byron address from Base58: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_byron_address_from_base58(
  const char*               data,
  size_t                    size,
  cardano_byron_address_t** address);

/**
 * \brief Retrieves the size in bytes of the Base58-encoded representation of a Byron-era address.
 *
 * This function calculates the size necessary to store the Base58-encoded representation of a given
 * \ref cardano_byron_address_t address object, including the null-termination character.
 *
 * \param[in] address A pointer to a \ref cardano_byron_address_t object whose Base58 size is to be retrieved.
 *
 * \return The size in bytes of the Base58-encoded representation of the address. Returns 0 if the \p address
 *         pointer is NULL or if the address is invalid.
 *
 * Usage Example:
 * \code{.c}
 * cardano_byron_address_t* byron_address = NULL;
 * // Assume byron_address is already created and valid
 * size_t base58_size = cardano_byron_address_get_base58_size(byron_address);
 *
 * if (base58_size > 0)
 * {
 *   char* base58_string = (char*)malloc(base58_size);
 *
 *   if (base58_string)
 *   {
 *     // Proceed to convert address to Base58 string
 *   }
 *
 *   free(base58_string);
 * }
 * else
 * {
 *   printf("Invalid address or unable to determine size\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_byron_address_get_base58_size(const cardano_byron_address_t* address);

/**
 * \brief Converts a Byron-era address into a Base58-encoded string.
 *
 * This function takes a \ref cardano_byron_address_t object and converts it to its Base58 string
 * representation. The resulting string is written into the provided \p data buffer, which must be large
 * enough to hold the Base58 string and the null-termination character.
 *
 * \param[in] address A pointer to the \ref cardano_byron_address_t object to be converted.
 * \param[out] data A pointer to a character buffer where the Base58-encoded string will be stored.
 * \param[in] size The size of the provided buffer in bytes. This size should be at least the value returned
 *                 by \ref cardano_byron_address_get_base58_size for this address.
 *
 * \return Returns \ref CARDANO_SUCCESS if the conversion was successful. Returns \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         the \p address or \p data pointer is NULL. Returns \ref CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE if the \p size is too
 *         small to hold the Base58 representation including the null terminator.
 *
 * Usage Example:
 * \code{.c}
 * cardano_byron_address_t* byron_address = NULL;
 * // Assume byron_address is previously created and valid
 * size_t base58_size = cardano_byron_address_get_base58_size(byron_address);
 * char* base58_string = (char*) malloc(base58_size);
 *
 * if (base58_string)
 * {
 *   cardano_error_t result = cardano_byron_address_to_base58(byron_address, base58_string, base58_size);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     printf("Base58 representation: %s\n", base58_string);
 *   }
 *   else
 *   {
 *     printf("Failed to convert Byron address to Base58: %d\n", result);
 *   }
 *
 *   free(base58_string);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_byron_address_to_base58(
  const cardano_byron_address_t* address,
  char*                          data,
  size_t                         size);

/**
 * \brief Retrieves the string representation of a byron era address.
 *
 * This function provides access to the string representation of a \ref cardano_byron_address_t object.
 * The string is in Base58. This function does not allocate new memory for the string; instead,
 * it returns a pointer to the internal representation within the object.
 *
 * \param[in] address A constant pointer to the \ref cardano_byron_address_t object from which
 *                    the string representation is to be retrieved.
 *
 * \return A constant pointer to the internal string representation of the address. If the address
 *         is NULL, returns NULL. The returned string should not be modified or freed by the caller,
 *         as it is managed internally by the \ref cardano_byron_address_t object.
 *
 * Usage Example:
 * \code{.c}
 * cardano_byron_address_t* byron_address = ...; // Assume this is initialized
 * const char* address_str = cardano_byron_address_get_string(byron_address);
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
CARDANO_EXPORT const char* cardano_byron_address_get_string(const cardano_byron_address_t* address);

/**
 * \brief Retrieves the network ID from a given Cardano byron address.
 *
 * This function extracts the network identifier from the provided \ref cardano_byron_address_t object.
 * The network ID indicates whether the address belongs to the test network or the main network.
 *
 * \param[in] address A constant pointer to the \ref cardano_byron_address_t object from which the network ID is to be retrieved.
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
 * cardano_byron_address_t* address = NULL;
 * cardano_network_id_t network_id;
 *
 * cardano_error_t get_network_id = cardano_byron_address_get_network_id(address, &network_id);
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
 * cardano_byron_address_unref(&address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_byron_address_get_network_id(
  const cardano_byron_address_t* address,
  cardano_network_id_t*          network_id);

/**
 * \brief Decrements the byron address's reference count.
 *
 * If the reference count reaches zero, the byron address memory is deallocated.
 *
 * \param[in] address Pointer to the byron address whose reference count is to be decremented.
 */
CARDANO_EXPORT void cardano_byron_address_unref(cardano_byron_address_t** address);

/**
 * \brief Increments the byron address's reference count.
 *
 * Ensures that the byron address remains allocated until the last reference is released.
 *
 * \param[in] address byron address whose reference count is to be incremented.
 */
CARDANO_EXPORT void cardano_byron_address_ref(cardano_byron_address_t* address);

/**
 * \brief Retrieves the byron address's current reference count.
 *
 * \warning Does not account for transitive references.
 *
 * \param[in] address Target byron address.
 * \return Current reference count of the byron address.
 */
CARDANO_EXPORT size_t cardano_byron_address_refcount(const cardano_byron_address_t* address);

/**
 * \brief Sets the last error message for a given byron address.
 *
 * This function records an error message in the byron address's last_error buffer,
 * overwriting any previous message. The message is truncated if it exceeds
 * the buffer size. This function is typically used to store descriptive
 * error information that can be retrieved later with
 * cardano_byron_address_get_last_error.
 *
 * \param[in,out] address A pointer to the cardano_byron_address_t instance whose last error
 *               message is to be set. If the byron address is NULL, the function
 *               has no effect.
 * \param[in] message A null-terminated string containing the error message to be
 *                recorded. If the message is NULL, the byron address's last_error
 *                will be set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters due to the fixed size
 *       of the last_error buffer (1024 characters), including the null
 *       terminator. Messages longer than this limit will be truncated.
 */
CARDANO_EXPORT void cardano_byron_address_set_last_error(cardano_byron_address_t* address, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific byron address.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_byron_address_set_last_error for the given
 * byron address. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in,out] address A pointer to the \ref cardano_byron_address_t instance whose last error
 *               message is to be retrieved. If the byron address is \c NULL, the function
 *               returns a generic error message indicating the null byron address.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified byron address. If the byron address is \c NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the byron address and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_byron_address_set_last_error for the same byron address, or until
 *       the byron address is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_byron_address_get_last_error(const cardano_byron_address_t* address);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BYRON_ADDRESS_H