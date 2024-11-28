/**
 * \file byron_addr_pack.c
 *
 * \author angel.castillo
 * \date   Apr 15, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
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

/* INCLUDES ******************************************************************/

#include <cardano/crypto/crc32.h>

#include "../../allocators.h"
#include "addr_common.h"
#include "byron_addr_pack.h"

#include "../../string_safe.h"

#include <assert.h>
#include <string.h>

/* IMPLEMENTATION ************************************************************/

int64_t
_cardano_byron_address_calculate_map_size(cardano_byron_address_attributes_t attributes)
{
  int64_t map_size = 0;

  if (attributes.magic >= 0)
  {
    map_size++;
  }

  if (attributes.derivation_path_size > 0U)
  {
    map_size++;
  }

  return map_size;
}

cardano_error_t
_cardano_byron_address_initialize(cardano_cbor_writer_t* writer, const cardano_address_t* address)
{
  assert(writer != NULL);
  assert(address != NULL);

  cardano_error_t result = cardano_cbor_writer_write_start_array(writer, 3);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_cbor_writer_write_bytestring(writer, address->byron_content->root, sizeof(address->byron_content->root));

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  int64_t map_size = _cardano_byron_address_calculate_map_size(address->byron_content->attributes);

  return cardano_cbor_writer_write_start_map(writer, map_size);
}

cardano_error_t
_cardano_byron_address_extract_cbor_data(cardano_cbor_writer_t* writer, byte_t** data, size_t* size)
{
  assert(writer != NULL);
  assert(data != NULL);
  assert(size != NULL);

  *size = cardano_cbor_writer_get_encode_size(writer);
  *data = (byte_t*)_cardano_malloc(*size);

  if (*data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  return cardano_cbor_writer_encode(writer, *data, *size);
}

cardano_error_t
_cardano_byron_address_encode_magic(cardano_cbor_writer_t* writer, const cardano_address_t* address)
{
  assert(writer != NULL);
  assert(address != NULL);

  cardano_cbor_writer_t* magic_writer = cardano_cbor_writer_new();
  cardano_error_t        result       = cardano_cbor_writer_write_uint(magic_writer, address->byron_content->attributes.magic);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&magic_writer);
    return result;
  }

  size_t  magic_size;
  byte_t* magic_data;
  result = _cardano_byron_address_extract_cbor_data(magic_writer, &magic_data, &magic_size);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_cbor_writer_write_uint(writer, 2);
    if (result == CARDANO_SUCCESS)
    {
      result = cardano_cbor_writer_write_bytestring(writer, magic_data, magic_size);
    }
    _cardano_free(magic_data);
  }

  cardano_cbor_writer_unref(&magic_writer);

  return result;
}

cardano_error_t
_cardano_byron_address_encode_derivation_path(cardano_cbor_writer_t* writer, const cardano_address_t* address)
{
  assert(writer != NULL);
  assert(address != NULL);

  cardano_cbor_writer_t* attributes_writer = cardano_cbor_writer_new();
  cardano_error_t        result            = cardano_cbor_writer_write_bytestring(
    attributes_writer,
    address->byron_content->attributes.derivation_path,
    address->byron_content->attributes.derivation_path_size);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&attributes_writer);

    return result;
  }

  size_t  attributes_size;
  byte_t* attributes_data;
  result = _cardano_byron_address_extract_cbor_data(attributes_writer, &attributes_data, &attributes_size);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_cbor_writer_write_uint(writer, 1);

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_cbor_writer_write_bytestring(writer, attributes_data, attributes_size);
    }
    _cardano_free(attributes_data);
  }

  cardano_cbor_writer_unref(&attributes_writer);

  return result;
}

cardano_error_t
_cardano_byron_address_encode_attributes(cardano_cbor_writer_t* writer, const cardano_address_t* address)
{
  assert(writer != NULL);
  assert(address != NULL);

  if (address->byron_content->attributes.derivation_path_size > 0U)
  {
    cardano_error_t result = _cardano_byron_address_encode_derivation_path(writer, address);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  if (address->byron_content->attributes.magic >= 0)
  {
    cardano_error_t result = _cardano_byron_address_encode_magic(writer, address);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }
  }

  return cardano_cbor_writer_write_uint(writer, address->byron_content->type);
}

cardano_error_t
_cardano_byron_address_finalize_writer(cardano_cbor_writer_t* writer, byte_t** data, size_t* size)
{
  assert(writer != NULL);
  assert(data != NULL);
  assert(size != NULL);

  *size = cardano_cbor_writer_get_encode_size(writer);
  *data = (byte_t*)_cardano_malloc(*size);

  if (*data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  return cardano_cbor_writer_encode(writer, *data, *size);
}

cardano_error_t
_cardano_byron_address_write_final_structure(
  cardano_cbor_writer_t* writer,
  const byte_t*          encoded_data,
  size_t                 encoded_size,
  uint32_t               crc)
{
  assert(writer != NULL);

  cardano_error_t result = cardano_cbor_writer_write_start_array(writer, 2);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_cbor_writer_write_tag(writer, CARDANO_ENCODED_CBOR_DATA_ITEM);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_cbor_writer_write_bytestring(writer, encoded_data, encoded_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return cardano_cbor_writer_write_uint(writer, crc);
}

cardano_error_t
_cardano_byron_address_finalize_encoding(cardano_cbor_writer_t* writer, byte_t** data, size_t* size)
{
  assert(writer != NULL);
  assert(data != NULL);
  assert(size != NULL);

  size_t          encoded_size;
  byte_t*         encoded_data;
  cardano_error_t result = _cardano_byron_address_finalize_writer(writer, &encoded_data, &encoded_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  uint32_t crc = cardano_checksum_crc32(encoded_data, encoded_size);

  result = cardano_cbor_writer_reset(writer);

  if (result != CARDANO_SUCCESS)
  {
    _cardano_free(encoded_data);
    return result;
  }

  result = _cardano_byron_address_write_final_structure(writer, encoded_data, encoded_size, crc);

  if (result == CARDANO_SUCCESS)
  {
    result = _cardano_byron_address_extract_cbor_data(writer, data, size);
  }

  _cardano_free(encoded_data);

  return result;
}

cardano_error_t
_cardano_pack_byron_address(const cardano_address_t* address, byte_t* data, const size_t data_size, size_t* size)
{
  assert(address != NULL);
  assert(data != NULL);
  assert(size != NULL);
  assert(address->byron_content != NULL);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (!writer)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = _cardano_byron_address_initialize(writer, address);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  result = _cardano_byron_address_encode_attributes(writer, address);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  byte_t* result_data = NULL;
  result              = _cardano_byron_address_finalize_encoding(writer, &result_data, size);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  cardano_safe_memcpy(data, data_size, result_data, *size);

  _cardano_free(result_data);
  cardano_cbor_writer_unref(&writer);

  return result;
}

cardano_error_t
_cardano_byron_address_initialize_cbor_reader(const byte_t* data, size_t size, cardano_cbor_reader_t** reader)
{
  assert(data != NULL);
  assert(reader != NULL);

  *reader = cardano_cbor_reader_new(data, size);

  if (*reader == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_byron_address_verify_cbor_structure(
  cardano_cbor_reader_t* reader,
  uint32_t*              crc_calculated,
  uint64_t*              crc_expected,
  cardano_buffer_t**     address_data_encoded)
{
  assert(reader != NULL);
  assert(crc_calculated != NULL);
  assert(crc_expected != NULL);
  assert(address_data_encoded != NULL);

  int64_t         array_size = 0;
  cardano_error_t result     = cardano_cbor_reader_read_start_array(reader, &array_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_tag_t tag;
  result = cardano_cbor_reader_read_tag(reader, &tag);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_cbor_reader_read_bytestring(reader, address_data_encoded);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_cbor_reader_read_uint(reader, crc_expected);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(address_data_encoded);
    *address_data_encoded = NULL;

    return result;
  }

  *crc_calculated = cardano_checksum_crc32(
    cardano_buffer_get_data(*address_data_encoded),
    cardano_buffer_get_size(*address_data_encoded));

  if (*crc_calculated != *crc_expected)
  {
    cardano_buffer_unref(address_data_encoded);
    return CARDANO_ERROR_CHECKSUM_MISMATCH;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_byron_address_unpack_inner_cbor_content(
  cardano_buffer_t*       address_data_encoded,
  uint32_t                crc_calculated,
  uint64_t                crc_expected,
  cardano_cbor_reader_t** inner_reader)
{
  assert(address_data_encoded != NULL);
  assert(inner_reader != NULL);

  if (crc_calculated != crc_expected)
  {
    return CARDANO_ERROR_CHECKSUM_MISMATCH;
  }

  *inner_reader = cardano_cbor_reader_new(cardano_buffer_get_data(address_data_encoded), cardano_buffer_get_size(address_data_encoded));

  if (!*inner_reader)
  {
    cardano_buffer_unref(&address_data_encoded);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_buffer_unref(&address_data_encoded);

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_byron_address_process_derivation_path(
  cardano_cbor_reader_t*              inner_reader,
  cardano_byron_address_attributes_t* attributes)
{
  assert(inner_reader != NULL);
  assert(attributes != NULL);

  cardano_buffer_t* encoded_derivation_path = NULL;
  cardano_error_t   result                  = cardano_cbor_reader_read_bytestring(inner_reader, &encoded_derivation_path);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_reader_t* path_reader = cardano_cbor_reader_new(cardano_buffer_get_data(encoded_derivation_path), cardano_buffer_get_size(encoded_derivation_path));

  if (!path_reader)
  {
    cardano_buffer_unref(&encoded_derivation_path);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_buffer_t* derivation_path = NULL;
  result                            = cardano_cbor_reader_read_bytestring(path_reader, &derivation_path);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&encoded_derivation_path);
    cardano_cbor_reader_unref(&path_reader);

    return result;
  }

  attributes->derivation_path_size = cardano_buffer_get_size(derivation_path);

  cardano_safe_memcpy(
    attributes->derivation_path,
    sizeof(attributes->derivation_path),
    cardano_buffer_get_data(derivation_path),
    attributes->derivation_path_size);

  cardano_buffer_unref(&derivation_path);
  cardano_buffer_unref(&encoded_derivation_path);
  cardano_cbor_reader_unref(&path_reader);

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_byron_address_process_magic(cardano_cbor_reader_t* inner_reader, cardano_byron_address_attributes_t* attributes)
{
  assert(inner_reader != NULL);
  assert(attributes != NULL);

  cardano_buffer_t* encoded_magic = NULL;
  cardano_error_t   result        = cardano_cbor_reader_read_bytestring(inner_reader, &encoded_magic);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_reader_t* magic_reader = cardano_cbor_reader_new(cardano_buffer_get_data(encoded_magic), cardano_buffer_get_size(encoded_magic));

  if (!magic_reader)
  {
    cardano_buffer_unref(&encoded_magic);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  int64_t magic;
  result = cardano_cbor_reader_read_int(magic_reader, &magic);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&encoded_magic);
    cardano_cbor_reader_unref(&magic_reader);
    return result;
  }

  attributes->magic = magic;

  cardano_buffer_unref(&encoded_magic);
  cardano_cbor_reader_unref(&magic_reader);

  return CARDANO_SUCCESS;
}

cardano_error_t
_cardano_byron_address_extract_address_components(cardano_cbor_reader_t* inner_reader, cardano_byron_address_t** address)
{
  assert(inner_reader != NULL);
  assert(address != NULL);

  int64_t         array_size = 0;
  cardano_error_t result     = cardano_cbor_reader_read_start_array(inner_reader, &array_size);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_buffer_t* root;
  result = cardano_cbor_reader_read_bytestring(inner_reader, &root);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  int64_t map_size = 0;
  result           = cardano_cbor_reader_read_start_map(inner_reader, &map_size);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&root);
    return result;
  }

  cardano_byron_address_attributes_t attributes = { 0 };
  attributes.magic                              = -1;

  while (map_size > 0)
  {
    int64_t key;
    result = cardano_cbor_reader_read_int(inner_reader, &key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_buffer_unref(&root);
      return result;
    }

    switch (key)
    {
      case 1:
        result = _cardano_byron_address_process_derivation_path(inner_reader, &attributes);
        break;
      case 2:
        result = _cardano_byron_address_process_magic(inner_reader, &attributes);
        break;
      default:
        result = CARDANO_ERROR_DECODING;
        break;
    }

    if (result != CARDANO_SUCCESS)
    {
      cardano_buffer_unref(&root);
      return result;
    }

    --map_size;
  }

  result = cardano_cbor_reader_read_end_map(inner_reader);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&root);
    return result;
  }

  int64_t byron_address_type;
  result = cardano_cbor_reader_read_int(inner_reader, &byron_address_type);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&root);
    return result;
  }

  cardano_blake2b_hash_t* hash = NULL;
  result                       = cardano_blake2b_hash_from_bytes(cardano_buffer_get_data(root), cardano_buffer_get_size(root), &hash);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&root);
    cardano_blake2b_hash_unref(&hash);

    return result;
  }

  result = cardano_byron_address_from_credentials(hash, attributes, byron_address_type, address);

  cardano_buffer_unref(&root);
  cardano_blake2b_hash_unref(&hash);

  return result;
}

cardano_error_t
_cardano_unpack_byron_address(const byte_t* data, const size_t size, cardano_byron_address_t** cardano_byron)
{
  assert(data != NULL);
  assert(cardano_byron != NULL);

  cardano_cbor_reader_t* reader;
  cardano_error_t        result = _cardano_byron_address_initialize_cbor_reader(data, size, &reader);
  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  uint32_t          crc_calculated;
  uint64_t          crc_expected;
  cardano_buffer_t* address_data_encoded = NULL;
  result                                 = _cardano_byron_address_verify_cbor_structure(reader, &crc_calculated, &crc_expected, &address_data_encoded);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_reader_unref(&reader);

    return result;
  }

  cardano_cbor_reader_t* inner_reader;
  result = _cardano_byron_address_unpack_inner_cbor_content(address_data_encoded, crc_calculated, crc_expected, &inner_reader);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_reader_unref(&reader);
    return result;
  }

  result = _cardano_byron_address_extract_address_components(inner_reader, cardano_byron);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_reader_unref(&reader);
    cardano_cbor_reader_unref(&inner_reader);

    return result;
  }

  cardano_cbor_reader_unref(&reader);
  cardano_cbor_reader_unref(&inner_reader);

  return CARDANO_SUCCESS;
}
