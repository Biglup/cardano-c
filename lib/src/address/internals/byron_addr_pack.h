/**
 * \file byron_addr_pack.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BYRON_ADDRESS_PACK_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BYRON_ADDRESS_PACK_H

/* INCLUDES ******************************************************************/

#include <cardano/address/byron_address.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Calculates the number of elements in the map of a Byron address based on its attributes.
 *
 * This function determines the size of the attributes map for a Byron address by checking the presence
 * of non-default attributes. It considers attributes such as the derivation path and the network magic number.
 * The size is used to construct the map in CBOR format during serialization of Byron addresses.
 *
 * \param[in] attributes The attributes of the Byron address to be examined.
 *
 * \return The calculated map size as an int64_t. If no attributes are present that require encoding, this function
 *         returns zero, indicating an empty map.
 */
int64_t
_cardano_byron_address_calculate_map_size(cardano_byron_address_attributes_t attributes);

/**
 * \brief Initializes a CBOR writer for serializing a Byron address.
 *
 * This function prepares a CBOR writer to serialize a Byron address by writing the necessary CBOR structure
 * headers, including the start of an array and the address root. It sets up the writer to accept the detailed
 * encoding of the address's attributes and type.
 *
 * \param[in,out] writer A pointer to a cardano_cbor_writer_t that will be used for writing the Byron address.
 * \param[in] address A pointer to a cardano_address_t containing the Byron address data to be serialized.
 *
 * \return A cardano_error_t indicating the result of the operation. Returns CARDANO_SUCCESS on successful
 *         initialization of the writer for the given Byron address. If an error occurs during the writing process,
 *         an appropriate error code is returned.
 */
cardano_error_t
_cardano_byron_address_initialize(cardano_cbor_writer_t* writer, const cardano_address_t* address);

/**
 * \brief Extracts the serialized CBOR data from a CBOR writer used to serialize a Byron address.
 *
 * \param[in] writer A pointer to the cardano_cbor_writer_t that has been used to serialize the Byron address.
 * \param[out] data A pointer to a byte pointer where the address of the allocated memory holding the CBOR data
 *                  will be stored. The caller is responsible for freeing this memory using an appropriate free
 *                  function, such as _cardano_free, to avoid memory leaks.
 * \param[out] size A pointer to a size_t variable where the size of the serialized data will be stored.
 *
 * \return A cardano_error_t indicating the result of the operation. Returns CARDANO_SUCCESS if the data
 *         was successfully retrieved and stored in the provided memory. Returns an error code if there was
 *         an issue with the extraction or memory allocation.
 */
cardano_error_t
_cardano_byron_address_extract_cbor_data(cardano_cbor_writer_t* writer, byte_t** data, size_t* size);

/**
 * \brief Encodes the "magic" attribute of a Byron address into CBOR format.
 *
 * This function encodes the network magic attribute (if present) of a Byron address into the provided
 * CBOR writer. The network magic is a numeric identifier used primarily in test networks to distinguish
 * between different network environments. If the magic attribute is valid (i.e., non-negative), it is
 * encoded; otherwise, it is ignored.
 *
 * \param[in] writer A pointer to the cardano_cbor_writer_t used for encoding the CBOR data.
 * \param[in] address A pointer to the cardano_address_t containing the Byron address whose magic
 *                    attribute is to be encoded.
 *
 * \return A cardano_error_t indicating the result of the operation. Returns CARDANO_SUCCESS if the magic
 *         was successfully encoded, or an error code if there was an issue during the encoding process.
 */
cardano_error_t
_cardano_byron_address_encode_magic(cardano_cbor_writer_t* writer, const cardano_address_t* address);

/**
 * \brief Encodes the derivation path attribute of a Byron address into CBOR format.
 *
 * This function is responsible for encoding the derivation path attribute of a Byron address into the
 * provided CBOR writer.
 *
 * \param[in] writer A pointer to a cardano_cbor_writer_t that will be used for encoding the CBOR data.
 * \param[in] address A pointer to the cardano_address_t containing the Byron address with the derivation
 *                    path to be encoded.
 *
 * \return A cardano_error_t indicating the result of the operation. Returns CARDANO_SUCCESS if the derivation
 *         path was successfully encoded, or an error code if there was an issue during the encoding process.
 */
cardano_error_t
_cardano_byron_address_encode_derivation_path(cardano_cbor_writer_t* writer, const cardano_address_t* address);

/**
 * \brief Encodes the attributes of a Byron address into CBOR format using the provided CBOR writer.
 *
 * This function takes a Byron address and encodes its attributes, such as the derivation path and magic value,
 * into a CBOR map. The function dynamically calculates the size of the map based on which attributes are
 * present and valid within the address, encoding only those attributes that are necessary.
 *
 * \param[in] writer A pointer to a cardano_cbor_writer_t that will be used to encode the attributes into CBOR.
 * \param[in] address A pointer to the cardano_address_t containing the Byron address attributes to be encoded.
 *
 * \return A cardano_error_t indicating the result of the operation. Returns CARDANO_SUCCESS if the attributes
 *         were successfully encoded into CBOR format.
 */
cardano_error_t
_cardano_byron_address_encode_attributes(cardano_cbor_writer_t* writer, const cardano_address_t* address);

/**
 * \brief Finalizes the CBOR writer and retrieves the encoded Byron address data.
 *
 * This function completes the CBOR encoding process for a Byron address by finalizing the CBOR writer.
 * It retrieves the encoded data and the size of this data. The function ensures that all data encoded
 * up to this point is properly compiled into a contiguous byte array which it returns via pointer
 * parameters.
 *
 * \param[in] writer A pointer to a cardano_cbor_writer_t that has been used to encode a Byron address.
 * \param[out] data A pointer to a byte array where the address of the encoded data will be stored.
 *                  The caller is responsible for freeing this memory using _cardano_free.
 * \param[out] size A pointer to a size_t variable where the size of the encoded data will be stored.
 *
 * \return A cardano_error_t indicating the result of the operation. Returns CARDANO_SUCCESS if the
 *         data was successfully retrieved.
 */
cardano_error_t
_cardano_byron_address_finalize_writer(cardano_cbor_writer_t* writer, byte_t** data, size_t* size);

/**
 * \brief Writes the final CBOR structure for a Byron address, including the encoded data and its checksum.
 *
 * This function writes the encoded Byron address data along with its checksum into a CBOR structure
 * using a CBOR writer. The encoded data is wrapped in a CBOR array that includes a self-describe tag
 * to indicate the format and a checksum to validate the integrity of the data upon decoding.
 *
 * \param[in] writer A pointer to a cardano_cbor_writer_t that is used to write the final CBOR structure.
 * \param[in] encoded_data A pointer to the byte array containing the previously encoded Byron address data.
 * \param[in] encoded_size The size of the encoded data in bytes.
 * \param[in] crc The CRC32 checksum calculated for the encoded data, used for data integrity verification.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. Returns CARDANO_SUCCESS if
 *         the final structure is written successfully.
 */
cardano_error_t
_cardano_byron_address_write_final_structure(
  cardano_cbor_writer_t* writer,
  const byte_t*          encoded_data,
  size_t                 encoded_size,
  uint32_t               crc);

/**
 * \brief Finalizes the encoding of a Byron address by extracting the data written to a CBOR writer.
 *
 * This function finalizes the encoding process for a Byron address by retrieving the encoded data
 * from a CBOR writer.
 *
 * \param[in] writer A pointer to a cardano_cbor_writer_t that has been used to encode a Byron address.
 * \param[out] data A pointer to a byte pointer where the address of the newly allocated buffer containing
 *                  the encoded data will be stored.
 * \param[out] size A pointer to a size_t variable where the size of the encoded data will be stored.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. Returns CARDANO_SUCCESS
 *         if the data is successfully retrieved and stored in the provided pointers.
 */
cardano_error_t
_cardano_byron_address_finalize_encoding(cardano_cbor_writer_t* writer, byte_t** data, size_t* size);

/**
 * \brief Initializes a CBOR reader for parsing Byron address data from a given byte buffer.
 *
 * This function sets up a cardano_cbor_reader_t to parse Byron address data that has been
 * encoded in CBOR format. It prepares the reader to parse data from a specified memory buffer
 * containing the CBOR-encoded data.
 *
 * \param[in] data A pointer to the buffer containing the CBOR-encoded Byron address data.
 * \param[in] size The size of the buffer in bytes.
 * \param[out] reader A pointer to a pointer of cardano_cbor_reader_t where the initialized
 *                    CBOR reader object will be stored.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. Returns CARDANO_SUCCESS
 *         if the reader is successfully initialized.
 */
cardano_error_t
_cardano_byron_address_initialize_cbor_reader(const byte_t* data, size_t size, cardano_cbor_reader_t** reader);

/**
 * \brief Verifies the structure of a CBOR-encoded Byron address and reads the encoded address data and expected CRC.
 *
 * This function reads from a cardano_cbor_reader_t initialized with Byron address data to verify the correct
 * structure of the CBOR data according to expected formats.
 *
 * \param[in] reader A pointer to an initialized cardano_cbor_reader_t that is set to read the Byron address data.
 * \param[out] crc_calculated A pointer to a uint32_t where the calculated CRC of the encoded data will be stored.
 * \param[out] crc_expected A pointer to a uint64_t where the expected CRC value read from the CBOR data will be stored.
 * \param[out] address_data_encoded A pointer to a pointer of cardano_buffer_t where the extracted address data
 *                  will be stored. The data is stored as a dynamically allocated buffer that the caller must manage.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. Returns CARDANO_SUCCESS if the
 *         structure is verified and the data is successfully read.
 */
cardano_error_t
_cardano_byron_address_verify_cbor_structure(
  cardano_cbor_reader_t* reader,
  uint32_t*              crc_calculated,
  uint64_t*              crc_expected,
  cardano_buffer_t**     address_data_encoded);

/**
 * \brief Unpacks the inner CBOR content of a Byron address and initializes a CBOR reader for it.
 *
 * This function checks the CRC of the encoded Byron address data against the expected CRC to ensure data integrity.
 * If the CRC matches, it initializes a new CBOR reader for the inner content of the address, allowing further
 * decoding and analysis of the encoded data.
 *
 * \param[in] address_data_encoded A pointer to a cardano_buffer_t containing the CBOR-encoded Byron address data.
 * \param[in] crc_calculated The CRC that was calculated from the encoded data.
 * \param[in] crc_expected The expected CRC read from the CBOR data stream, used for verification.
 * \param[out] inner_reader A pointer to a pointer of cardano_cbor_reader_t that will be initialized to read
 *              from the address_data_encoded buffer.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. Returns CARDANO_SUCCESS if the
 *         structure is verified and the data is successfully read.
 */
cardano_error_t
_cardano_byron_address_unpack_inner_cbor_content(
  cardano_buffer_t*       address_data_encoded,
  uint32_t                crc_calculated,
  uint64_t                crc_expected,
  cardano_cbor_reader_t** inner_reader);

/**
 * \brief Processes the derivation path attribute from the inner CBOR content of a Byron address.
 *
 * This function reads the derivation path attribute encoded within the Byron address's CBOR content.
 * It assumes that the CBOR reader is positioned at the correct point to read the derivation path.
 * The function extracts the derivation path and stores it in the provided attributes structure.
 *
 * \param[in] inner_reader A pointer to a cardano_cbor_reader_t already set up to read the inner CBOR content.
 * \param[out] attributes A pointer to a byron_address_attributes_t structure where the decoded derivation path
 *                        will be stored. The function updates the derivation_path field of this structure.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. Returns CARDANO_SUCCESS if the
 *         structure is verified and the data is successfully read.
 */
cardano_error_t
_cardano_byron_address_process_derivation_path(
  cardano_cbor_reader_t*              inner_reader,
  cardano_byron_address_attributes_t* attributes);

/**
 * \brief Processes the network magic attribute from the inner CBOR content of a Byron address.
 *
 * This function is designed to extract and decode the network magic attribute encoded within the CBOR
 * structure of a Byron address. It assumes that the CBOR reader is appropriately positioned to read
 * this attribute directly.
 *
 * \param[in] inner_reader A pointer to a cardano_cbor_reader_t that is set to read the inner CBOR content
 *                         of a Byron address. The reader should be positioned at the start of the network
 *                         magic attribute.
 * \param[out] attributes A pointer to a byron_address_attributes_t structure where the network magic
 *                        will be stored upon successful extraction. The `magic` field of this structure
 *                        is updated.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. Returns CARDANO_SUCCESS if the
 *         structure is verified and the data is successfully read.
 */
cardano_error_t
_cardano_byron_address_process_magic(
  cardano_cbor_reader_t*              inner_reader,
  cardano_byron_address_attributes_t* attributes);

/**
 * \brief Extracts address components from the CBOR content read from a Byron address.
 *
 * This function interprets the decoded CBOR content to extract essential components of a Byron address,
 * including the address root, attributes, and the address type. It constructs a Byron address structure
 * from the extracted data.
 *
 * \param[in] inner_reader A pointer to a cardano_cbor_reader_t that has been initialized and positioned
 *                         to read the inner content of a Byron address CBOR stream.
 * \param[out] address A pointer to a pointer of cardano_byron_address_t where the newly created Byron address
 *                     structure will be stored upon successful extraction.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. Returns CARDANO_SUCCESS if the
 *         structure is verified and the data is successfully read.
 */
cardano_error_t
_cardano_byron_address_extract_address_components(
  cardano_cbor_reader_t*    inner_reader,
  cardano_byron_address_t** address);

/**
 * \brief Serializes a Byron address into a byte array using Concise Binary Object Representation (CBOR).
 *
 * This function takes a Byron address structure and serializes it into a CBOR format. The serialized data
 * is then wrapped with a self-describe CBOR tag and appended with a CRC32 checksum for integrity verification.
 *
 * \param[in] address A pointer to the cardano_address_t structure that represents a Byron address to be serialized.
 * \param[out] data A pointer to a byte array where the serialized data will be stored.
 * \param[in,out] size A pointer to a size_t that initially contains the size of the array pointed to by data.
 *                     After successful execution, it will contain the actual size of the serialized data.
 * \param[in] data_size The size of the data array in bytes.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. Returns CARDANO_SUCCESS if the
 *         structure is verified and the data is successfully read.
 */
cardano_error_t
_cardano_pack_byron_address(const cardano_address_t* address, byte_t* data, size_t data_size, size_t* size);

/**
 * \brief Deserializes a byte array containing a serialized Byron address into a cardano_byron_address_t structure.
 *
 * This function takes a byte array containing a serialized Byron address in CBOR format, and deserializes it
 * into a cardano_byron_address_t structure. It performs validation checks such as CRC32 checksum verification
 * to ensure the integrity of the serialized data.
 *
 * \param[in] data A pointer to a byte array containing the serialized Byron address data in CBOR format.
 * \param[in] size The size of the serialized data in bytes.
 * \param[out] cardano_byron A pointer to a pointer to a cardano_byron_address_t structure, where the deserialized
 *                            Byron address will be stored upon successful execution. The caller is responsible for
 *                            freeing the memory allocated for this structure.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. Returns CARDANO_SUCCESS if the
 *         structure is verified and the data is successfully read.
 */
cardano_error_t
_cardano_unpack_byron_address(const byte_t* data, size_t size, cardano_byron_address_t** cardano_byron);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BYRON_ADDRESS_PACK_H