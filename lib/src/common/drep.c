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
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t DREP_ARRAY_SIZE = 2;

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
    return CARDANO_POINTER_IS_NULL;
  }

  if ((type == CARDANO_DREP_TYPE_KEY_HASH) || (type == CARDANO_DREP_TYPE_SCRIPT_HASH))
  {
    if (credential == NULL)
    {
      *drep = NULL;
      return CARDANO_POINTER_IS_NULL;
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
    return CARDANO_MEMORY_ALLOCATION_FAILED;
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
cardano_drep_from_cbor(cardano_cbor_reader_t* reader, cardano_drep_t** drep)
{
  if (drep == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *drep = NULL;
    return CARDANO_POINTER_IS_NULL;
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
      // LCOV_EXCL_START
      cardano_credential_unref(&credential);
      *drep = NULL;
      return expect_end_array_result;
      // LCOV_EXCL_STOP
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
    // LCOV_EXCL_START
    *drep = NULL;
    return expect_end_array_result;
    // LCOV_EXCL_STOP
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
    return CARDANO_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  int64_t array_size = ((drep->type == CARDANO_DREP_TYPE_NO_CONFIDENCE) || (drep->type == CARDANO_DREP_TYPE_ABSTAIN)) ? 1 : DREP_ARRAY_SIZE;

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(writer, array_size);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result; /* LCOV_EXCL_LINE */
  }

  cardano_error_t write_uint_result = cardano_cbor_writer_write_uint(writer, drep->type);

  if (write_uint_result != CARDANO_SUCCESS)
  {
    return write_uint_result; /* LCOV_EXCL_LINE */
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
      return credential_result; /* LCOV_EXCL_LINE */
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_get_credential(cardano_drep_t* drep, cardano_credential_t** credential)
{
  if (drep == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if ((drep->type != CARDANO_DREP_TYPE_KEY_HASH) && (drep->type != CARDANO_DREP_TYPE_SCRIPT_HASH))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  *credential = drep->credential;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_set_credential(cardano_drep_t* drep, cardano_credential_t* credential)
{
  if (drep == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  if ((drep->type != CARDANO_DREP_TYPE_KEY_HASH) && (drep->type != CARDANO_DREP_TYPE_SCRIPT_HASH))
  {
    cardano_drep_set_last_error(drep, "DRep: only key hash and script hash DRep types can have a credential");
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  if (credential == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
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
    return CARDANO_POINTER_IS_NULL;
  }

  if (type == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
  }

  *type = drep->type;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_drep_set_type(cardano_drep_t* drep, const cardano_drep_type_t type)
{
  if (drep == NULL)
  {
    return CARDANO_POINTER_IS_NULL;
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