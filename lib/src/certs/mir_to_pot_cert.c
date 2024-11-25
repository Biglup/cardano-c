/**
 * \file mir_to_pot_cert.c
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

#include <cardano/certs/mir_cert_type.h>
#include <cardano/certs/mir_to_pot_cert.h>
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
 * \brief This certificate move instantaneous rewards funds between accounting pots.
 */
typedef struct cardano_mir_to_pot_cert_t
{
    cardano_object_t            base;
    cardano_mir_cert_pot_type_t pot;
    uint64_t                    amount;
} cardano_mir_to_pot_cert_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a mir_to_pot_cert object.
 *
 * This function is responsible for properly deallocating a mir_to_pot_cert object (`cardano_mir_to_pot_cert_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the mir_to_pot_cert object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_mir_to_pot_cert_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the mir_to_pot_cert
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_mir_to_pot_cert_deallocate(void* object)
{
  assert(object != NULL);
  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_mir_to_pot_cert_new(
  cardano_mir_cert_pot_type_t pot_type,
  uint64_t                    amount,
  cardano_mir_to_pot_cert_t** mir_to_pot_cert)
{
  if (mir_to_pot_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_mir_to_pot_cert_t* data = _cardano_malloc(sizeof(cardano_mir_to_pot_cert_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_mir_to_pot_cert_deallocate;

  data->pot    = pot_type;
  data->amount = amount;

  *mir_to_pot_cert = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_mir_to_pot_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_mir_to_pot_cert_t** mir_to_pot_cert)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (mir_to_pot_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "mir_to_pot_cert";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    return expect_array_result;
  }

  uint64_t        pot      = 0U;
  cardano_error_t read_pot = cardano_cbor_validate_uint_in_range(
    validator_name,
    "pot",
    reader,
    &pot,
    CARDANO_MIR_CERT_TYPE_TO_POT,
    CARDANO_MIR_CERT_TYPE_TO_STAKE_CREDS);

  if (read_pot != CARDANO_SUCCESS)
  {
    return read_pot;
  }

  uint64_t amount = 0;

  cardano_error_t read_amount = cardano_cbor_reader_read_uint(reader, &amount);

  if (read_amount != CARDANO_SUCCESS)
  {
    return read_amount;
  }

  return cardano_mir_to_pot_cert_new(pot, amount, mir_to_pot_cert);
}

cardano_error_t
cardano_mir_to_pot_cert_to_cbor(
  const cardano_mir_to_pot_cert_t* mir_to_pot_cert,
  cardano_cbor_writer_t*           writer)
{
  if (mir_to_pot_cert == NULL)
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

  result = cardano_cbor_writer_write_uint(writer, mir_to_pot_cert->pot);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return cardano_cbor_writer_write_uint(writer, mir_to_pot_cert->amount);
}

cardano_error_t
cardano_mir_to_pot_cert_get_pot(const cardano_mir_to_pot_cert_t* mir_cert, cardano_mir_cert_pot_type_t* type)
{
  if (mir_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (type == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *type = mir_cert->pot;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_mir_to_pot_cert_set_pot(cardano_mir_to_pot_cert_t* mir_cert, cardano_mir_cert_pot_type_t type)
{
  if (mir_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  mir_cert->pot = type;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_mir_to_pot_cert_get_amount(const cardano_mir_to_pot_cert_t* mir_cert, uint64_t* amount)
{
  if (mir_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (amount == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *amount = mir_cert->amount;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_mir_to_pot_cert_set_amount(cardano_mir_to_pot_cert_t* mir_cert, uint64_t amount)
{
  if (mir_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  mir_cert->amount = amount;

  return CARDANO_SUCCESS;
}

void
cardano_mir_to_pot_cert_unref(cardano_mir_to_pot_cert_t** mir_to_pot_cert)
{
  if ((mir_to_pot_cert == NULL) || (*mir_to_pot_cert == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*mir_to_pot_cert)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *mir_to_pot_cert = NULL;
    return;
  }
}

void
cardano_mir_to_pot_cert_ref(cardano_mir_to_pot_cert_t* mir_to_pot_cert)
{
  if (mir_to_pot_cert == NULL)
  {
    return;
  }

  cardano_object_ref(&mir_to_pot_cert->base);
}

size_t
cardano_mir_to_pot_cert_refcount(const cardano_mir_to_pot_cert_t* mir_to_pot_cert)
{
  if (mir_to_pot_cert == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&mir_to_pot_cert->base);
}

void
cardano_mir_to_pot_cert_set_last_error(cardano_mir_to_pot_cert_t* mir_to_pot_cert, const char* message)
{
  cardano_object_set_last_error(&mir_to_pot_cert->base, message);
}

const char*
cardano_mir_to_pot_cert_get_last_error(const cardano_mir_to_pot_cert_t* mir_to_pot_cert)
{
  return cardano_object_get_last_error(&mir_to_pot_cert->base);
}
