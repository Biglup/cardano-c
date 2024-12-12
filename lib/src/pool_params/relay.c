/**
 * \file relay.c
 *
 * \author angel.castillo
 * \date   Jun 26, 2024
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

#include <cardano/error.h>
#include <cardano/pool_params/multi_host_name_relay.h>
#include <cardano/pool_params/relay.h>
#include <cardano/pool_params/relay_type.h>
#include <cardano/pool_params/single_host_addr_relay.h>
#include <cardano/pool_params/single_host_name_relay.h>

#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief A relay is a type of node that acts as intermediaries between core nodes
 * (which produce blocks) and the wider internet. They help in passing along
 * transactions and blocks, ensuring that data is propagated throughout the
 * network.
 */
typedef struct cardano_relay_t
{
    cardano_object_t                  base;
    cardano_relay_type_t              type;
    cardano_multi_host_name_relay_t*  multi_host_name_relay;
    cardano_single_host_addr_relay_t* single_host_addr_relay;
    cardano_single_host_name_relay_t* single_host_name_relay;
} cardano_relay_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a relay object.
 *
 * This function is responsible for properly deallocating a relay object (`cardano_relay_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the relay object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_relay_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the relay
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_relay_deallocate(void* object)
{
  assert(object != NULL);

  cardano_relay_t* data = (cardano_relay_t*)object;

  cardano_multi_host_name_relay_unref(&data->multi_host_name_relay);
  cardano_single_host_addr_relay_unref(&data->single_host_addr_relay);
  cardano_single_host_name_relay_unref(&data->single_host_name_relay);

  _cardano_free(data);
}

/**
 * \brief Creates a new relay object.
 *
 * \return A pointer to the newly created relay object, or `NULL` if the operation failed.
 */
static cardano_relay_t*
cardano_relay_new(void)
{
  cardano_relay_t* data = _cardano_malloc(sizeof(cardano_relay_t));

  if (data == NULL)
  {
    return NULL;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_relay_deallocate;

  data->type                   = CARDANO_RELAY_TYPE_SINGLE_HOST_ADDRESS;
  data->multi_host_name_relay  = NULL;
  data->single_host_addr_relay = NULL;
  data->single_host_name_relay = NULL;

  return data;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_relay_new_single_host_addr(
  cardano_single_host_addr_relay_t* single_host_addr_relay,
  cardano_relay_t**                 relay)
{
  if (single_host_addr_relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_relay_t* data = cardano_relay_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->type                   = CARDANO_RELAY_TYPE_SINGLE_HOST_ADDRESS;
  data->single_host_addr_relay = single_host_addr_relay;

  cardano_single_host_addr_relay_ref(single_host_addr_relay);

  *relay = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_relay_new_single_host_name(
  cardano_single_host_name_relay_t* single_host_name,
  cardano_relay_t**                 relay)
{
  if (single_host_name == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_relay_t* data = cardano_relay_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->type                   = CARDANO_RELAY_TYPE_SINGLE_HOST_NAME;
  data->single_host_name_relay = single_host_name;

  cardano_single_host_name_relay_ref(single_host_name);

  *relay = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_relay_new_multi_host_name(
  cardano_multi_host_name_relay_t* multi_host_name_relay,
  cardano_relay_t**                relay)
{
  if (multi_host_name_relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_relay_t* data = cardano_relay_new();

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->type                  = CARDANO_RELAY_TYPE_MULTI_HOST_NAME;
  data->multi_host_name_relay = multi_host_name_relay;

  cardano_multi_host_name_relay_ref(multi_host_name_relay);

  *relay = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_relay_from_cbor(cardano_cbor_reader_t* reader, cardano_relay_t** relay)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_cbor_reader_t* reader_clone = NULL;

  cardano_error_t result = cardano_cbor_reader_clone(reader, &reader_clone);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  static const char* validator_name = "relay";

  int64_t array_size = 0;
  result             = cardano_cbor_reader_read_start_array(reader_clone, &array_size);

  CARDANO_UNUSED(array_size);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_reader_unref(&reader_clone);
    return result;
  }

  uint64_t              type             = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_uint_in_range(
    validator_name,
    "type",
    reader_clone,
    &type,
    CARDANO_RELAY_TYPE_SINGLE_HOST_ADDRESS,
    CARDANO_RELAY_TYPE_MULTI_HOST_NAME);

  cardano_cbor_reader_unref(&reader_clone);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  switch (type)
  {
    case CARDANO_RELAY_TYPE_SINGLE_HOST_ADDRESS:
    {
      cardano_single_host_addr_relay_t* single_host_addr_relay = NULL;
      result                                                   = cardano_single_host_addr_relay_from_cbor(reader, &single_host_addr_relay);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_relay_new_single_host_addr(single_host_addr_relay, relay);
      cardano_single_host_addr_relay_unref(&single_host_addr_relay);

      break;
    }
    case CARDANO_RELAY_TYPE_SINGLE_HOST_NAME:
    {
      cardano_single_host_name_relay_t* single_host_name_relay = NULL;
      result                                                   = cardano_single_host_name_relay_from_cbor(reader, &single_host_name_relay);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_relay_new_single_host_name(single_host_name_relay, relay);
      cardano_single_host_name_relay_unref(&single_host_name_relay);

      break;
    }
    case CARDANO_RELAY_TYPE_MULTI_HOST_NAME:
    {
      cardano_multi_host_name_relay_t* multi_host_name_relay = NULL;
      result                                                 = cardano_multi_host_name_relay_from_cbor(reader, &multi_host_name_relay);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_relay_new_multi_host_name(multi_host_name_relay, relay);
      cardano_multi_host_name_relay_unref(&multi_host_name_relay);

      break;
    }

    default:
      result = CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE;
      break;
  }

  return result;
}

cardano_error_t
cardano_relay_to_cbor(
  const cardano_relay_t* relay,
  cardano_cbor_writer_t* writer)
{
  if (relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  switch (relay->type)
  {
    case CARDANO_RELAY_TYPE_SINGLE_HOST_ADDRESS:
    {
      result = cardano_single_host_addr_relay_to_cbor(relay->single_host_addr_relay, writer);
      break;
    }
    case CARDANO_RELAY_TYPE_SINGLE_HOST_NAME:
    {
      result = cardano_single_host_name_relay_to_cbor(relay->single_host_name_relay, writer);
      break;
    }
    case CARDANO_RELAY_TYPE_MULTI_HOST_NAME:
    {
      result = cardano_multi_host_name_relay_to_cbor(relay->multi_host_name_relay, writer);
      break;
    }

    default:
      result = CARDANO_ERROR_INVALID_NATIVE_SCRIPT_TYPE;
      break;
  }

  return result;
}

cardano_error_t
cardano_relay_get_type(
  const cardano_relay_t* relay,
  cardano_relay_type_t*  type)
{
  if (relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (type == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *type = relay->type;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_relay_to_single_host_addr(
  cardano_relay_t*                   relay,
  cardano_single_host_addr_relay_t** single_host_addr)
{
  if (relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (single_host_addr == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (relay->type != CARDANO_RELAY_TYPE_SINGLE_HOST_ADDRESS)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  cardano_single_host_addr_relay_ref(relay->single_host_addr_relay);

  *single_host_addr = relay->single_host_addr_relay;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_relay_to_single_host_name(
  cardano_relay_t*                   relay,
  cardano_single_host_name_relay_t** single_host_name)
{
  if (relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (single_host_name == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (relay->type != CARDANO_RELAY_TYPE_SINGLE_HOST_NAME)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  cardano_single_host_name_relay_ref(relay->single_host_name_relay);

  *single_host_name = relay->single_host_name_relay;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_relay_to_multi_host_name(
  cardano_relay_t*                  relay,
  cardano_multi_host_name_relay_t** multi_host_name)
{
  if (relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (multi_host_name == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (relay->type != CARDANO_RELAY_TYPE_MULTI_HOST_NAME)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  cardano_multi_host_name_relay_ref(relay->multi_host_name_relay);

  *multi_host_name = relay->multi_host_name_relay;

  return CARDANO_SUCCESS;
}

void
cardano_relay_unref(cardano_relay_t** relay)
{
  if ((relay == NULL) || (*relay == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*relay)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *relay = NULL;
    return;
  }
}

void
cardano_relay_ref(cardano_relay_t* relay)
{
  if (relay == NULL)
  {
    return;
  }

  cardano_object_ref(&relay->base);
}

size_t
cardano_relay_refcount(const cardano_relay_t* relay)
{
  if (relay == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&relay->base);
}

void
cardano_relay_set_last_error(cardano_relay_t* relay, const char* message)
{
  cardano_object_set_last_error(&relay->base, message);
}

const char*
cardano_relay_get_last_error(const cardano_relay_t* relay)
{
  return cardano_object_get_last_error(&relay->base);
}
