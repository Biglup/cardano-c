/**
 * \file pointer_addr_pack.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_pointer_ADDRESS_PACK_H
#define BIGLUP_LABS_INCLUDE_CARDANO_pointer_ADDRESS_PACK_H

/* INCLUDES ******************************************************************/

#include <cardano/address/address_type.h>
#include <cardano/address/pointer_address.h>
#include <cardano/common/credential.h>
#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Encodes an unsigned 32-bit integer using variable-length encoding.
 *
 * This function implements variable-length encoding which optimizes space for smaller values.
 * Each byte in the encoded output holds seven bits of the integer, with the eighth bit (most
 * significant bit) used as a continuation flag. The continuation flag is set (1) to indicate
 * that another byte follows or cleared (0) if it is the last byte of the sequence.
 *
 * \param value         The unsigned 32-bit integer to be encoded.
 * \param buffer        A pointer to the buffer where the encoded bytes are to be stored.
 * \param buffer_size   The size of the provided buffer, which must be at least 10 bytes to
 *                      accommodate the maximum possible encoded size.
 *
 * \return The number of bytes used in the buffer to store the encoded integer, or 0 if an error occurred
 *         (e.g., buffer size is insufficient).
 */
size_t
_cardano_pointer_address_variable_length_encode(uint32_t value, byte_t* buffer, size_t buffer_size);

/**
 * \brief Decodes a value that was encoded using a variable-length encoding scheme.
 *
 * This function reads bytes from the input array, reconstructing the original unsigned integer.
 * Each byte in the array contributes seven bits to the output value.
 *
 * The most significant bit of each byte acts as a continuation flag:
 *
 * if set (1), the function continues to read the next byte; if cleared (0), it
 * indicates that this is the last byte of the encoded value. The function assumes the bytes are
 * provided in little-endian order, with the least significant byte first.
 *
 * \param array Pointer to the byte array containing the encoded data. This pointer must not be NULL.
 * \param size The total size of the input array. This should be at least the number of bytes that
 *             the encoded value occupies.
 * \param total_bytes The number of bytes read so far into the array.
 * \param bytes_read Pointer to a size_t variable where the number of bytes read from the array will
 *                   be stored. This parameter cannot be NULL and will be set even if the function
 *                   returns an error, unless the error is due to a NULL pointer.
 *
 * \return Returns the decoded value or -1 if an error occurs.
 */
int64_t
_cardano_pointer_address_variable_length_decode(const byte_t* array, size_t total_bytes, size_t size, size_t* bytes_read);

/**
 * \brief Packs a Cardano pointer address into a byte array.
 *
 * This function takes a Cardano pointer address and encodes it into a byte array format.
 *
 * \param[in] address Pointer to the Cardano pointer address structure to be packed.
 * \param[out] data Pointer to the byte array where the packed address will be stored.
 * \param[in] size Size of the data byte array in bytes.
 *
 * \note This function assumes that the `data` buffer has enough space to hold the packed
 *       address. The caller is responsible for ensuring that there is sufficient space
 *       and for managing the memory of the `data` buffer.
 *
 * \return Returns the number of bytes written to the `data` buffer. If an error occurs,
 *        the return value will be less than zero.
 */
int64_t
_cardano_pack_pointer_address(const cardano_address_t* address, byte_t* data, size_t size);

/**
 * \brief Unpacks a byte array into a Cardano pointer address structure.
 *
 * This function takes a byte array containing the serialized form of a Cardano pointer address
 * and decodes it into a Cardano pointer address structure.
 *
 * \param[in] data Pointer to the byte array containing the serialized Cardano pointer address.
 * \param[in] size Size of the byte array in bytes.
 * \param[out] address Double pointer to where the newly created Cardano pointer address structure will be stored.
 *
 * \return Returns \ref CARDANO_SUCCESS if the unpacking is successful. Returns an error code if
 *         the unpacking fails due to invalid format, insufficient data provided, or memory allocation issues.
 *
 * \note The caller is responsible for managing the lifecycle of the created Cardano pointer address object.
 *       This includes deallocating the address structure using the appropriate function when it is no longer needed.
 */
cardano_error_t
_cardano_unpack_pointer_address(const byte_t* data, size_t size, cardano_pointer_address_t** address);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_pointer_ADDRESS_PACK_H