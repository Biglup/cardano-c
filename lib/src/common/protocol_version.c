/**
 * \file protocol_version.c
 *
 * \author angel.castillo
 * \date   May 06, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License") {}
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

#include <cardano/common/protocol_version.h>
#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t PROTOCOL_VERSION_EMBEDDED_GROUP_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano protocol version.
 */
typedef struct cardano_protocol_version_t
{
    cardano_object_t base;
    uint64_t         major;
    uint64_t         minor;
} cardano_protocol_version_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a protocol version object.
 *
 * This function is responsible for properly deallocating a protocol version object (`cardano_protocol_version_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the protocol_version object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_protocol_version_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the protocol_version
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_protocol_version_deallocate(void* object)
{
  assert(object != NULL);
  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_protocol_version_new(
  const uint64_t               major,
  const uint64_t               minor,
  cardano_protocol_version_t** protocol_version)
{
  if (protocol_version == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *protocol_version = _cardano_malloc(sizeof(cardano_protocol_version_t));

  if (*protocol_version == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*protocol_version)->base.deallocator   = cardano_protocol_version_deallocate;
  (*protocol_version)->base.ref_count     = 1;
  (*protocol_version)->base.last_error[0] = '\0';

  (*protocol_version)->major = major;
  (*protocol_version)->minor = minor;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_protocol_version_from_cbor(cardano_cbor_reader_t* reader, cardano_protocol_version_t** protocol_version)
{
  if (protocol_version == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *protocol_version = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "protocol_version";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)PROTOCOL_VERSION_EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    *protocol_version = NULL;
    return expect_array_result;
  }

  uint64_t              major            = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_uint_in_range(
    validator_name,
    "major",
    reader,
    &major,
    0,
    UINT64_MAX);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    *protocol_version = NULL;
    return read_uint_result;
  }

  uint64_t              minor             = 0U;
  const cardano_error_t read_minor_result = cardano_cbor_validate_uint_in_range(
    validator_name,
    "minor",
    reader,
    &minor,
    0,
    UINT64_MAX);

  if (read_minor_result != CARDANO_SUCCESS)
  {
    *protocol_version = NULL;
    return read_minor_result;
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    *protocol_version = NULL;

    return expect_end_array_result;
  }

  return cardano_protocol_version_new(major, minor, protocol_version);
}

cardano_error_t
cardano_protocol_version_to_cbor(const cardano_protocol_version_t* protocol_version, cardano_cbor_writer_t* writer)
{
  if (protocol_version == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(
    writer,
    PROTOCOL_VERSION_EMBEDDED_GROUP_SIZE);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  cardano_error_t write_uint_result = cardano_cbor_writer_write_uint(writer, protocol_version->major);

  if (write_uint_result != CARDANO_SUCCESS)
  {
    return write_uint_result;
  }

  return cardano_cbor_writer_write_uint(writer, protocol_version->minor);
}

uint64_t
cardano_protocol_version_get_major(const cardano_protocol_version_t* protocol_version)
{
  if (protocol_version == NULL)
  {
    return 0;
  }

  return protocol_version->major;
}

cardano_error_t
cardano_protocol_version_set_major(cardano_protocol_version_t* protocol_version, const uint64_t major)
{
  if (protocol_version == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_version->major = major;

  return CARDANO_SUCCESS;
}

uint64_t
cardano_protocol_version_get_minor(const cardano_protocol_version_t* protocol_version)
{
  if (protocol_version == NULL)
  {
    return 0;
  }

  return protocol_version->minor;
}

cardano_error_t
cardano_protocol_version_set_minor(cardano_protocol_version_t* protocol_version, const uint64_t minor)
{
  if (protocol_version == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  protocol_version->minor = minor;

  return CARDANO_SUCCESS;
}

void
cardano_protocol_version_unref(cardano_protocol_version_t** protocol_version)
{
  if ((protocol_version == NULL) || (*protocol_version == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*protocol_version)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *protocol_version = NULL;
    return;
  }
}

void
cardano_protocol_version_ref(cardano_protocol_version_t* protocol_version)
{
  if (protocol_version == NULL)
  {
    return;
  }

  cardano_object_ref(&protocol_version->base);
}

size_t
cardano_protocol_version_refcount(const cardano_protocol_version_t* protocol_version)
{
  if (protocol_version == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&protocol_version->base);
}

void
cardano_protocol_version_set_last_error(cardano_protocol_version_t* protocol_version, const char* message)
{
  cardano_object_set_last_error(&protocol_version->base, message);
}

const char*
cardano_protocol_version_get_last_error(const cardano_protocol_version_t* protocol_version)
{
  return cardano_object_get_last_error(&protocol_version->base);
}