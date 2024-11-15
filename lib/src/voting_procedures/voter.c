/**
 * \file voter.c
 *
 * \author angel.castillo
 * \date   Aug 08, 2024
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

#include <cardano/object.h>
#include <cardano/voting_procedures/voter.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t VOTER_ARRAY_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano voter.
 */
typedef struct cardano_voter_t
{
    cardano_object_t      base;
    cardano_voter_type_t  type;
    cardano_credential_t* credential;
} cardano_voter_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a voter object.
 *
 * This function is responsible for properly deallocating a voter object (`cardano_voter_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the voter object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_voter_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the voter
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_voter_deallocate(void* object)
{
  assert(object != NULL);

  cardano_voter_t* data = (cardano_voter_t*)object;

  cardano_credential_unref(&data->credential);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_voter_new(const cardano_voter_type_t type, cardano_credential_t* credential, cardano_voter_t** voter)
{
  if (voter == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    *voter = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *voter = _cardano_malloc(sizeof(cardano_voter_t));

  if (*voter == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*voter)->base.deallocator   = cardano_voter_deallocate;
  (*voter)->base.ref_count     = 1;
  (*voter)->base.last_error[0] = '\0';
  (*voter)->type               = type;

  cardano_credential_ref(credential);
  (*voter)->credential = credential;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_voter_from_cbor(cardano_cbor_reader_t* reader, cardano_voter_t** voter)
{
  if (voter == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *voter = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "Voter";

  int64_t               array_size          = 0;
  const cardano_error_t expect_array_result = cardano_cbor_reader_read_start_array(reader, &array_size);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    *voter = NULL;
    return expect_array_result;
  }

  uint64_t              type             = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_uint_in_range(
    validator_name,
    "voter_type",
    reader,
    &type,
    CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_KEY_HASH,
    CARDANO_VOTER_TYPE_STAKE_POOL_KEY_HASH);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    *voter = NULL;
    return read_uint_result;
  }

  if (array_size < VOTER_ARRAY_SIZE)
  {
    cardano_cbor_reader_set_last_error(reader, "Voter: unexpected array size, expected 2");

    *voter = NULL;
    return CARDANO_ERROR_DECODING;
  }

  cardano_buffer_t*     credential_buffer        = NULL;
  const cardano_error_t credential_buffer_result = cardano_cbor_reader_read_bytestring(reader, &credential_buffer);

  if (credential_buffer_result != CARDANO_SUCCESS)
  {
    *voter = NULL;
    return credential_buffer_result;
  }

  cardano_credential_t*     credential = NULL;
  cardano_credential_type_t credential_type;

  switch (type)
  {
    case CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_KEY_HASH:
    case CARDANO_VOTER_TYPE_DREP_KEY_HASH:
    case CARDANO_VOTER_TYPE_STAKE_POOL_KEY_HASH:
      credential_type = CARDANO_CREDENTIAL_TYPE_KEY_HASH;
      break;
    case CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_SCRIPT_HASH:
    case CARDANO_VOTER_TYPE_DREP_SCRIPT_HASH:
      credential_type = CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH;
      break;

    default:
      cardano_buffer_unref(&credential_buffer);
      *voter = NULL;
      return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  const cardano_error_t credential_result = cardano_credential_from_hash_bytes(
    cardano_buffer_get_data(credential_buffer),
    cardano_buffer_get_size(credential_buffer),
    credential_type,
    &credential);

  cardano_buffer_unref(&credential_buffer);

  if (credential_result != CARDANO_SUCCESS)
  {
    *voter = NULL;
    return credential_result;
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    cardano_credential_unref(&credential);
    *voter = NULL;
    return expect_end_array_result;
  }

  const cardano_error_t new_voter_result = cardano_voter_new((cardano_voter_type_t)type, credential, voter);
  cardano_credential_unref(&credential);

  if (new_voter_result != CARDANO_SUCCESS)
  {
    *voter = NULL;
    return new_voter_result;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_voter_to_cbor(
  const cardano_voter_t* voter,
  cardano_cbor_writer_t* writer)
{
  if (voter == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(writer, VOTER_ARRAY_SIZE);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  cardano_error_t write_uint_result = cardano_cbor_writer_write_uint(writer, voter->type);

  if (write_uint_result != CARDANO_SUCCESS)
  {
    return write_uint_result;
  }

  cardano_credential_t* credential = voter->credential;

  cardano_error_t credential_result = cardano_cbor_writer_write_bytestring(
    writer,
    cardano_credential_get_hash_bytes(credential),
    cardano_credential_get_hash_bytes_size(credential));

  if (credential_result != CARDANO_SUCCESS)
  {
    return credential_result;
  }

  return CARDANO_SUCCESS;
}

cardano_credential_t*
cardano_voter_get_credential(cardano_voter_t* voter)
{
  if (voter == NULL)
  {
    return NULL;
  }

  cardano_credential_ref(voter->credential);

  return voter->credential;
}

cardano_error_t
cardano_voter_set_credential(cardano_voter_t* voter, cardano_credential_t* credential)
{
  if (voter == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_ref(credential);
  cardano_credential_unref(&voter->credential);
  voter->credential = credential;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_voter_get_type(const cardano_voter_t* voter, cardano_voter_type_t* type)
{
  if (voter == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (type == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *type = voter->type;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_voter_set_type(cardano_voter_t* voter, const cardano_voter_type_t type)
{
  if (voter == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  voter->type = type;

  return CARDANO_SUCCESS;
}

bool
cardano_voter_equals(const cardano_voter_t* lhs, const cardano_voter_t* rhs)
{
  if (lhs == rhs)
  {
    return true;
  }

  if (lhs == NULL)
  {
    return false;
  }

  if (rhs == NULL)
  {
    return false;
  }

  if (lhs->type != rhs->type)
  {
    return false;
  }

  return cardano_credential_equals(lhs->credential, rhs->credential);
}

void
cardano_voter_unref(cardano_voter_t** voter)
{
  if ((voter == NULL) || (*voter == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*voter)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *voter = NULL;
    return;
  }
}

void
cardano_voter_ref(cardano_voter_t* voter)
{
  if (voter == NULL)
  {
    return;
  }

  cardano_object_ref(&voter->base);
}

size_t
cardano_voter_refcount(const cardano_voter_t* voter)
{
  if (voter == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&voter->base);
}

void
cardano_voter_set_last_error(cardano_voter_t* voter, const char* message)
{
  cardano_object_set_last_error(&voter->base, message);
}

const char*
cardano_voter_get_last_error(const cardano_voter_t* voter)
{
  return cardano_object_get_last_error(&voter->base);
}