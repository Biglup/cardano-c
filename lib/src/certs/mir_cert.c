/**
 * \file mir_cert.c
 *
 * \author angel.castillo
 * \date   Jun 19, 2024
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

/* INCLUDES ******************************************************************/

#include <cardano/certs/cert_type.h>
#include <cardano/certs/mir_cert.h>
#include <cardano/certs/mir_cert_type.h>
#include <cardano/certs/mir_to_pot_cert.h>
#include <cardano/certs/mir_to_stake_creds_cert.h>
#include <cardano/error.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief Certificate used to facilitate an instantaneous transfer of rewards within the system.
 */
typedef struct cardano_mir_cert_t
{
    cardano_object_t                   base;
    cardano_mir_cert_type_t            type;
    cardano_mir_to_pot_cert_t*         mir_to_pot_cert;
    cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert;
} cardano_mir_cert_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a mir_cert object.
 *
 * This function is responsible for properly deallocating a mir_cert object (`cardano_mir_cert_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the mir_cert object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_mir_cert_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the mir_cert
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_mir_cert_deallocate(void* object)
{
  assert(object != NULL);

  cardano_mir_cert_t* data = (cardano_mir_cert_t*)object;

  cardano_mir_to_pot_cert_unref(&data->mir_to_pot_cert);
  cardano_mir_to_stake_creds_cert_unref(&data->mir_to_stake_creds_cert);

  _cardano_free(data);
}

/**
 * \brief Creates a new mir_cert object.
 *
 * \return A pointer to the newly created mir_cert object, or `NULL` if the operation failed.
 */
static cardano_mir_cert_t*
cardano_mir_cert_new(void)
{
  cardano_mir_cert_t* data = _cardano_malloc(sizeof(cardano_mir_cert_t));

  if (data == NULL)
  {
    return NULL;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_mir_cert_deallocate;

  data->mir_to_pot_cert         = NULL;
  data->mir_to_stake_creds_cert = NULL;
  data->type                    = CARDANO_MIR_CERT_TYPE_TO_POT;

  return data;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_mir_cert_new_to_other_pot(
  cardano_mir_to_pot_cert_t* to_other_pot_cert,
  cardano_mir_cert_t**       mir_cert)
{
  if (to_other_pot_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (mir_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_mir_cert_t* data = cardano_mir_cert_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_mir_to_pot_cert_ref(to_other_pot_cert);

  data->mir_to_pot_cert = to_other_pot_cert;
  data->type            = CARDANO_MIR_CERT_TYPE_TO_POT;

  *mir_cert = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_mir_cert_new_to_stake_creds(
  cardano_mir_to_stake_creds_cert_t* to_stake_creds_cert,
  cardano_mir_cert_t**               mir_cert)
{
  if (to_stake_creds_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (mir_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_mir_cert_t* data = cardano_mir_cert_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_mir_to_stake_creds_cert_ref(to_stake_creds_cert);

  data->mir_to_stake_creds_cert = to_stake_creds_cert;
  data->type                    = CARDANO_MIR_CERT_TYPE_TO_STAKE_CREDS;

  *mir_cert = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_mir_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_mir_cert_t** mir_cert)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (mir_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "mir_cert";

  cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    return expect_array_result;
  }

  uint64_t              type             = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_enum_value(
    validator_name,
    "type",
    reader,
    CARDANO_CERT_TYPE_MOVE_INSTANTANEOUS_REWARDS,
    (enum_to_string_callback_t)((void*)&cardano_cert_type_to_string),
    &type);

  CARDANO_UNUSED(type);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  cardano_cbor_reader_t* embedded_reader = NULL;

  cardano_error_t clone_reader = cardano_cbor_reader_clone(reader, &embedded_reader);

  if (clone_reader != CARDANO_SUCCESS)
  {
    return clone_reader;
  }

  expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, embedded_reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    cardano_cbor_reader_unref(&embedded_reader);

    return expect_array_result;
  }

  uint64_t        pot      = 0U;
  cardano_error_t read_pot = cardano_cbor_validate_uint_in_range(
    validator_name,
    "pot",
    embedded_reader,
    &pot,
    CARDANO_MIR_CERT_POT_TYPE_RESERVE,
    CARDANO_MIR_CERT_POT_TYPE_TREASURY);

  if (read_pot != CARDANO_SUCCESS)
  {
    cardano_cbor_reader_unref(&embedded_reader);

    return read_pot;
  }

  CARDANO_UNUSED(pot);

  cardano_cbor_reader_state_t state;
  cardano_error_t             peek_result = cardano_cbor_reader_peek_state(embedded_reader, &state);

  cardano_cbor_reader_unref(&embedded_reader);

  if (peek_result != CARDANO_SUCCESS)
  {
    return peek_result;
  }

  if (state == CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER)
  {
    cardano_mir_to_pot_cert_t* mir_to_pot_cert = NULL;
    cardano_error_t            to_pot_result   = cardano_mir_to_pot_cert_from_cbor(reader, &mir_to_pot_cert);

    if (to_pot_result != CARDANO_SUCCESS)
    {
      return to_pot_result;
    }

    cardano_error_t create_mir_cert_result = cardano_mir_cert_new_to_other_pot(mir_to_pot_cert, mir_cert);

    cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);

    return create_mir_cert_result;
  }

  if ((state == CARDANO_CBOR_READER_STATE_START_ARRAY) || (state == CARDANO_CBOR_READER_STATE_START_MAP))
  {
    cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
    cardano_error_t                    to_stake_creds_result   = cardano_mir_to_stake_creds_cert_from_cbor(reader, &mir_to_stake_creds_cert);

    if (to_stake_creds_result != CARDANO_SUCCESS)
    {
      return to_stake_creds_result;
    }

    cardano_error_t create_mir_cert_result = cardano_mir_cert_new_to_stake_creds(mir_to_stake_creds_cert, mir_cert);

    cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);

    return create_mir_cert_result;
  }

  return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
}

cardano_error_t
cardano_mir_cert_to_cbor(
  const cardano_mir_cert_t* mir_cert,
  cardano_cbor_writer_t*    writer)
{
  if (mir_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = cardano_cbor_writer_write_start_array(writer, EMBEDDED_GROUP_SIZE);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_cbor_writer_write_uint(writer, CARDANO_CERT_TYPE_MOVE_INSTANTANEOUS_REWARDS);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  switch (mir_cert->type)
  {
    case CARDANO_MIR_CERT_TYPE_TO_POT:
    {
      result = cardano_mir_to_pot_cert_to_cbor(mir_cert->mir_to_pot_cert, writer);
      break;
    }
    case CARDANO_MIR_CERT_TYPE_TO_STAKE_CREDS:
    {
      result = cardano_mir_to_stake_creds_cert_to_cbor(mir_cert->mir_to_stake_creds_cert, writer);
      break;
    }

    default:
      return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  return result;
}

cardano_error_t
cardano_mir_cert_get_type(const cardano_mir_cert_t* mir_cert, cardano_mir_cert_type_t* type)
{
  if (mir_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (type == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *type = mir_cert->type;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_mir_cert_as_to_other_pot(
  cardano_mir_cert_t*         mir_cert,
  cardano_mir_to_pot_cert_t** to_other_pot_cert)
{
  if (mir_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (to_other_pot_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (mir_cert->type != CARDANO_MIR_CERT_TYPE_TO_POT)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_mir_to_pot_cert_ref(mir_cert->mir_to_pot_cert);

  *to_other_pot_cert = mir_cert->mir_to_pot_cert;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_mir_cert_as_to_stake_creds(
  cardano_mir_cert_t*                 mir_cert,
  cardano_mir_to_stake_creds_cert_t** to_stake_creds_cert)
{
  if (mir_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (to_stake_creds_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (mir_cert->type != CARDANO_MIR_CERT_TYPE_TO_STAKE_CREDS)
  {
    return CARDANO_ERROR_INVALID_CERTIFICATE_TYPE;
  }

  cardano_mir_to_stake_creds_cert_ref(mir_cert->mir_to_stake_creds_cert);

  *to_stake_creds_cert = mir_cert->mir_to_stake_creds_cert;

  return CARDANO_SUCCESS;
}

void
cardano_mir_cert_unref(cardano_mir_cert_t** mir_cert)
{
  if ((mir_cert == NULL) || (*mir_cert == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*mir_cert)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *mir_cert = NULL;
    return;
  }
}

void
cardano_mir_cert_ref(cardano_mir_cert_t* mir_cert)
{
  if (mir_cert == NULL)
  {
    return;
  }

  cardano_object_ref(&mir_cert->base);
}

size_t
cardano_mir_cert_refcount(const cardano_mir_cert_t* mir_cert)
{
  if (mir_cert == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&mir_cert->base);
}

void
cardano_mir_cert_set_last_error(cardano_mir_cert_t* mir_cert, const char* message)
{
  cardano_object_set_last_error(&mir_cert->base, message);
}

const char*
cardano_mir_cert_get_last_error(const cardano_mir_cert_t* mir_cert)
{
  return cardano_object_get_last_error(&mir_cert->base);
}
