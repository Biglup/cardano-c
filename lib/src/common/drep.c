/**
 * \file drep.c
 *
 * \author angel.castillo
 * \date   Jun 18, 2024
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

#include <cardano/common/drep.h>
#include <cardano/common/governance_key_type.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <cardano/encoding/bech32.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t DREP_ARRAY_SIZE          = 2;
static const size_t  DREP_CIP129_PAYLOAD_SIZE = 29U;
static const size_t  DREP_HEADER_SIZE         = 1U;
static const size_t  CREDENTIAL_HASH_SIZE     = 28U;
static const size_t  KEY_TYPE_OFFSET          = 2U;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano drep.
 */
typedef struct cardano_drep_t
{
    cardano_object_t      base;
    cardano_drep_type_t   type;
    cardano_credential_t* credential;
} cardano_drep_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a drep object.
 *
 * This function is responsible for properly deallocating a drep object (`cardano_drep_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the drep object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_drep_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the drep
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_drep_deallocate(void* object)
{
  assert(object != NULL);

  cardano_drep_t* data = (cardano_drep_t*)object;

  cardano_credential_unref(&data->credential);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_drep_new(const cardano_drep_type_t type, cardano_credential_t* credential, cardano_drep_t** drep)
{
  if (drep == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((type == CARDANO_DREP_TYPE_KEY_HASH) || (type == CARDANO_DREP_TYPE_SCRIPT_HASH))
  {
    if (credential == NULL)
    {
      *drep = NULL;
      return CARDANO_ERROR_POINTER_IS_NULL;
    }
  }
  else
  {
    if (credential != NULL)
    {
      *drep = NULL;
      return CARDANO_ERROR_INVALID_ARGUMENT;
    }
  }

  *drep = _cardano_malloc(sizeof(cardano_drep_t));

  if (*drep == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*drep)->base.deallocator   = cardano_drep_deallocate;
  (*drep)->base.ref_count     = 1;
  (*drep)->base.last_error[0] = '\0';
  (*drep)->type               = type;

  if (credential != NULL)
  {
    cardano_credential_ref(credential);
    (*drep)->credential = credential;
  }
  else
  {
    (*drep)->credential = NULL;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_from_string(
  const char*      bech32_string,
  const size_t     string_length,
  cardano_drep_t** drep)
{
  if (bech32_string == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (string_length == 0U)
  {
    return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
  }

  if (drep == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  size_t       hrp_size    = 0;
  const size_t data_length = cardano_encoding_bech32_get_decoded_length(bech32_string, string_length, &hrp_size);
  char*        hrp         = (char*)_cardano_malloc(hrp_size);

  if ((hrp_size == 0U) || (hrp == NULL))
  {
    _cardano_free(hrp);

    return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
  }

  byte_t* decoded_data = (byte_t*)_cardano_malloc(data_length);

  if ((data_length == 0U) || (decoded_data == NULL))
  {
    _cardano_free(hrp);
    _cardano_free(decoded_data);

    return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
  }

  cardano_error_t result = cardano_encoding_bech32_decode(bech32_string, string_length, hrp, hrp_size, decoded_data, data_length);

  if (result != CARDANO_SUCCESS)
  {
    _cardano_free(hrp);
    _cardano_free(decoded_data);

    return result;
  }

  if ((strncmp(hrp, "drep", 4) != 0) && (strncmp(hrp, "drep_script", 11)) != 0)
  {
    _cardano_free(hrp);
    _cardano_free(decoded_data);

    return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
  }

  if ((data_length != CREDENTIAL_HASH_SIZE) && (data_length != DREP_CIP129_PAYLOAD_SIZE)) // CIP-0105 or CIP-0129
  {
    _cardano_free(hrp);
    _cardano_free(decoded_data);

    return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
  }

  if (data_length == CREDENTIAL_HASH_SIZE)
  {
    // CIP-0105
    cardano_credential_t*           credential = NULL;
    const cardano_credential_type_t type       = (hrp_size == 5U) ? CARDANO_CREDENTIAL_TYPE_KEY_HASH : CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH;

    result = cardano_credential_from_hash_bytes(decoded_data, data_length, type, &credential);

    if (result != CARDANO_SUCCESS)
    {
      _cardano_free(hrp);
      _cardano_free(decoded_data);

      return result;
    }

    result = cardano_drep_new((cardano_drep_type_t)type, credential, drep);
    cardano_credential_unref(&credential);
  }
  else
  {
    // CIP-0129
    assert(data_length == DREP_CIP129_PAYLOAD_SIZE);
    byte_t header = decoded_data[0];

    // Values in header are offset by 2. The values 0 and 1 are reserved
    // to prevent accidental conflicts with Cardano Address Network tags, ensuring
    // that governance identifiers remain distinct and are not inadvertently processed as addresses.
    byte_t                        raw_type            = (byte_t)(header & 0x0FU);
    cardano_credential_type_t     type                = (cardano_credential_type_t)(byte_t)(raw_type - (byte_t)KEY_TYPE_OFFSET);
    cardano_governance_key_type_t governance_key_type = (cardano_governance_key_type_t)(byte_t)((header >> 4U) & 0x0FU);

    if (raw_type < KEY_TYPE_OFFSET)
    {
      _cardano_free(hrp);
      _cardano_free(decoded_data);

      return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
    }

    if (governance_key_type != CARDANO_GOVERNANCE_KEY_TYPE_DREP)
    {
      _cardano_free(hrp);
      _cardano_free(decoded_data);

      return CARDANO_ERROR_INVALID_ADDRESS_FORMAT;
    }

    cardano_credential_t* credential = NULL;
    result                           = cardano_credential_from_hash_bytes(&decoded_data[DREP_HEADER_SIZE], data_length - DREP_HEADER_SIZE, type, &credential);

    if (result != CARDANO_SUCCESS)
    {
      _cardano_free(hrp);
      _cardano_free(decoded_data);

      return result;
    }

    result = cardano_drep_new((cardano_drep_type_t)type, credential, drep);
    cardano_credential_unref(&credential);
  }

  _cardano_free(hrp);
  _cardano_free(decoded_data);

  return result;
}

cardano_error_t
cardano_drep_from_cbor(cardano_cbor_reader_t* reader, cardano_drep_t** drep)
{
  if (drep == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *drep = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "DRep";

  int64_t               array_size          = 0;
  const cardano_error_t expect_array_result = cardano_cbor_reader_read_start_array(reader, &array_size);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    *drep = NULL;
    return expect_array_result;
  }

  uint64_t              type             = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_uint_in_range(
    validator_name,
    "drep_type",
    reader,
    &type,
    CARDANO_DREP_TYPE_KEY_HASH,
    CARDANO_DREP_TYPE_NO_CONFIDENCE);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    *drep = NULL;
    return read_uint_result;
  }

  if ((type == (uint64_t)CARDANO_DREP_TYPE_KEY_HASH) || (type == (uint64_t)CARDANO_DREP_TYPE_SCRIPT_HASH))
  {
    if (array_size < DREP_ARRAY_SIZE)
    {
      cardano_cbor_reader_set_last_error(reader, "DRep: unexpected array size, expected 2");

      *drep = NULL;
      return CARDANO_ERROR_DECODING;
    }

    cardano_buffer_t*     credential_buffer        = NULL;
    const cardano_error_t credential_buffer_result = cardano_cbor_reader_read_bytestring(reader, &credential_buffer);

    if (credential_buffer_result != CARDANO_SUCCESS)
    {
      *drep = NULL;
      return credential_buffer_result;
    }

    cardano_credential_t* credential        = NULL;
    const cardano_error_t credential_result = cardano_credential_from_hash_bytes(
      cardano_buffer_get_data(credential_buffer),
      cardano_buffer_get_size(credential_buffer),
      (cardano_credential_type_t)type,
      &credential);

    cardano_buffer_unref(&credential_buffer);

    if (credential_result != CARDANO_SUCCESS)
    {
      *drep = NULL;
      return credential_result;
    }

    const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

    if (expect_end_array_result != CARDANO_SUCCESS)
    {
      cardano_credential_unref(&credential);
      *drep = NULL;
      return expect_end_array_result;
    }

    const cardano_error_t new_drep_result = cardano_drep_new((cardano_drep_type_t)type, credential, drep);
    cardano_credential_unref(&credential);

    if (new_drep_result != CARDANO_SUCCESS)
    {
      *drep = NULL;
      return new_drep_result;
    }

    return CARDANO_SUCCESS;
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    *drep = NULL;
    return expect_end_array_result;
  }

  return cardano_drep_new((cardano_drep_type_t)type, NULL, drep);
}

cardano_error_t
cardano_drep_to_cbor(
  const cardano_drep_t*  drep,
  cardano_cbor_writer_t* writer)
{
  if (drep == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  int64_t array_size = ((drep->type == CARDANO_DREP_TYPE_NO_CONFIDENCE) || (drep->type == CARDANO_DREP_TYPE_ABSTAIN)) ? 1 : DREP_ARRAY_SIZE;

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(writer, array_size);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  cardano_error_t write_uint_result = cardano_cbor_writer_write_uint(writer, drep->type);

  if (write_uint_result != CARDANO_SUCCESS)
  {
    return write_uint_result;
  }

  if (array_size == DREP_ARRAY_SIZE)
  {
    cardano_credential_t* credential = drep->credential;

    cardano_error_t credential_result = cardano_cbor_writer_write_bytestring(
      writer,
      cardano_credential_get_hash_bytes(credential),
      cardano_credential_get_hash_bytes_size(credential));

    if (credential_result != CARDANO_SUCCESS)
    {
      return credential_result;
    }
  }

  return CARDANO_SUCCESS;
}

size_t
cardano_drep_get_string_size(const cardano_drep_t* drep)
{
  if (drep == NULL)
  {
    return 0U;
  }

  if ((drep->type != CARDANO_DREP_TYPE_KEY_HASH) && (drep->type != CARDANO_DREP_TYPE_SCRIPT_HASH))
  {
    return 0U;
  }

  const byte_t gov_key_type = (byte_t)CARDANO_GOVERNANCE_KEY_TYPE_DREP;
  const byte_t type         = (byte_t)drep->type + (byte_t)KEY_TYPE_OFFSET;
  const byte_t header       = (gov_key_type << 4) | type;
  byte_t       payload[29]  = { 0 };

  payload[0] = header;

  cardano_credential_t* credential = drep->credential;

  cardano_safe_memcpy(&payload[DREP_HEADER_SIZE], CREDENTIAL_HASH_SIZE, cardano_credential_get_hash_bytes(credential), CREDENTIAL_HASH_SIZE);

  return cardano_encoding_bech32_get_encoded_length("drep", 4, payload, DREP_CIP129_PAYLOAD_SIZE);
}

cardano_error_t
cardano_drep_to_string(
  const cardano_drep_t* drep,
  char*                 data,
  size_t                size)
{
  if (drep == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size < cardano_drep_get_string_size(drep))
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  if ((drep->type != CARDANO_DREP_TYPE_KEY_HASH) && (drep->type != CARDANO_DREP_TYPE_SCRIPT_HASH))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  const byte_t gov_key_type = (byte_t)CARDANO_GOVERNANCE_KEY_TYPE_DREP;
  const byte_t type         = (byte_t)drep->type + (byte_t)KEY_TYPE_OFFSET;
  const byte_t header       = (gov_key_type << 4) | type;
  byte_t       payload[29]  = { 0 };

  payload[0] = header;

  cardano_credential_t* credential = drep->credential;

  cardano_safe_memcpy(&payload[DREP_HEADER_SIZE], CREDENTIAL_HASH_SIZE, cardano_credential_get_hash_bytes(credential), CREDENTIAL_HASH_SIZE);

  return cardano_encoding_bech32_encode("drep", 4, payload, DREP_CIP129_PAYLOAD_SIZE, data, size);
}

cardano_error_t
cardano_drep_get_credential(cardano_drep_t* drep, cardano_credential_t** credential)
{
  if (drep == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((drep->type != CARDANO_DREP_TYPE_KEY_HASH) && (drep->type != CARDANO_DREP_TYPE_SCRIPT_HASH))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  cardano_credential_ref(drep->credential);
  *credential = drep->credential;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_set_credential(cardano_drep_t* drep, cardano_credential_t* credential)
{
  if (drep == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((drep->type != CARDANO_DREP_TYPE_KEY_HASH) && (drep->type != CARDANO_DREP_TYPE_SCRIPT_HASH))
  {
    cardano_drep_set_last_error(drep, "DRep: only key hash and script hash DRep types can have a credential");
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  cardano_credential_ref(credential);
  cardano_credential_unref(&drep->credential);
  drep->credential = credential;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_get_type(const cardano_drep_t* drep, cardano_drep_type_t* type)
{
  if (drep == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (type == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *type = drep->type;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_set_type(cardano_drep_t* drep, const cardano_drep_type_t type)
{
  if (drep == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  drep->type = type;

  return CARDANO_SUCCESS;
}

void
cardano_drep_unref(cardano_drep_t** drep)
{
  if ((drep == NULL) || (*drep == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*drep)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *drep = NULL;
    return;
  }
}

void
cardano_drep_ref(cardano_drep_t* drep)
{
  if (drep == NULL)
  {
    return;
  }

  cardano_object_ref(&drep->base);
}

size_t
cardano_drep_refcount(const cardano_drep_t* drep)
{
  if (drep == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&drep->base);
}

void
cardano_drep_set_last_error(cardano_drep_t* drep, const char* message)
{
  cardano_object_set_last_error(&drep->base, message);
}

const char*
cardano_drep_get_last_error(const cardano_drep_t* drep)
{
  return cardano_object_get_last_error(&drep->base);
}