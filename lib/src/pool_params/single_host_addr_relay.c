/**
 * \file single_host_addr_relay.c
 *
 * \author angel.castillo
 * \date   jun 26, 2024
 *
 * copyright 2024 biglup labs
 *
 * licensed under the apache license, version 2.0 (the "license") {}
 * you may not use this file except in compliance with the license.
 * you may obtain a copy of the license at
 *
 *     www.apache.org/licenses/license-2.0
 *
 * unless required by applicable law or agreed to in writing, software
 * distributed under the license is distributed on an "as is" basis,
 * without warranties or conditions of any kind, either express or implied.
 * see the license for the specific language governing permissions and
 * limitations under the license.
 */

/* INCLUDES ******************************************************************/

#include <cardano/object.h>
#include <cardano/pool_params/ipv4.h>
#include <cardano/pool_params/ipv6.h>
#include <cardano/pool_params/relay.h>
#include <cardano/pool_params/single_host_addr_relay.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 4;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents an single host addr relay.
 *
 * Each instance of `cardano_single_host_addr_relay_t` holds a single host addr relay.
 */
typedef struct cardano_single_host_addr_relay_t
{
    cardano_object_t base;
    uint16_t*        port;
    cardano_ipv4_t*  ipv4;
    cardano_ipv6_t*  ipv6;
} cardano_single_host_addr_relay_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a single_host_addr_relay object.
 *
 * This function is responsible for properly deallocating a single_host_addr_relay object (`cardano_single_host_addr_relay_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the single_host_addr_relay object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_single_host_addr_relay_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the single_host_addr_relay
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_single_host_addr_relay_deallocate(void* object)
{
  assert(object != NULL);

  cardano_single_host_addr_relay_t* single_host_addr_relay = (cardano_single_host_addr_relay_t*)object;

  _cardano_free(single_host_addr_relay->port);
  cardano_ipv4_unref(&single_host_addr_relay->ipv4);
  cardano_ipv6_unref(&single_host_addr_relay->ipv6);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_single_host_addr_relay_new(
  const uint16_t*                    port,
  cardano_ipv4_t*                    ipv4,
  cardano_ipv6_t*                    ipv6,
  cardano_single_host_addr_relay_t** single_host_addr_relay)
{
  if (single_host_addr_relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *single_host_addr_relay = _cardano_malloc(sizeof(cardano_single_host_addr_relay_t));

  if (*single_host_addr_relay == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*single_host_addr_relay)->base.deallocator   = cardano_single_host_addr_relay_deallocate;
  (*single_host_addr_relay)->base.ref_count     = 1;
  (*single_host_addr_relay)->base.last_error[0] = '\0';
  (*single_host_addr_relay)->port               = NULL;
  (*single_host_addr_relay)->ipv4               = NULL;
  (*single_host_addr_relay)->ipv6               = NULL;

  if (port != NULL)
  {
    (*single_host_addr_relay)->port = _cardano_malloc(sizeof(uint16_t));

    if ((*single_host_addr_relay)->port == NULL)
    {
      cardano_single_host_addr_relay_unref(single_host_addr_relay);
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    *(*single_host_addr_relay)->port = *port;
  }

  if (ipv4 != NULL)
  {
    cardano_ipv4_ref(ipv4);
    (*single_host_addr_relay)->ipv4 = ipv4;
  }

  if (ipv6 != NULL)
  {
    cardano_ipv6_ref(ipv6);
    (*single_host_addr_relay)->ipv6 = ipv6;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_single_host_addr_relay_from_cbor(cardano_cbor_reader_t* reader, cardano_single_host_addr_relay_t** single_host_addr_relay)
{
  if (single_host_addr_relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *single_host_addr_relay = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "single_host_addr_relay";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    *single_host_addr_relay = NULL;
    return expect_array_result;
  }

  uint64_t              type             = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_enum_value(
    validator_name,
    "type",
    reader,
    CARDANO_RELAY_TYPE_SINGLE_HOST_ADDRESS,
    (enum_to_string_callback_t)((void*)&cardano_relay_type_to_string),
    &type);

  CARDANO_UNUSED(type);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  bool            has_port = false;
  uint64_t        port64   = 0U;
  uint16_t        port     = 0U;
  cardano_ipv4_t* ipv4     = NULL;
  cardano_ipv6_t* ipv6     = NULL;

  cardano_cbor_reader_state_t state;
  cardano_error_t             peek_state = cardano_cbor_reader_peek_state(reader, &state);

  if (peek_state != CARDANO_SUCCESS)
  {
    return peek_state;
  }

  if (state == CARDANO_CBOR_READER_STATE_NULL)
  {
    const cardano_error_t read_null_result = cardano_cbor_reader_read_null(reader);

    if (read_null_result != CARDANO_SUCCESS)
    {
      return read_null_result;
    }
  }
  else
  {
    const cardano_error_t read_port_result = cardano_cbor_reader_read_uint(reader, &port64);

    if (read_port_result != CARDANO_SUCCESS)
    {
      return read_port_result;
    }

    port     = (uint16_t)port64;
    has_port = true;
  }

  peek_state = cardano_cbor_reader_peek_state(reader, &state);

  if (peek_state != CARDANO_SUCCESS)
  {
    return peek_state;
  }

  if (state == CARDANO_CBOR_READER_STATE_NULL)
  {
    const cardano_error_t read_null_result = cardano_cbor_reader_read_null(reader);

    if (read_null_result != CARDANO_SUCCESS)
    {
      return read_null_result;
    }
  }
  else
  {
    cardano_error_t read_ipv4_result = cardano_ipv4_from_cbor(reader, &ipv4);

    if (read_ipv4_result != CARDANO_SUCCESS)
    {
      return read_ipv4_result;
    }
  }

  peek_state = cardano_cbor_reader_peek_state(reader, &state);

  if (peek_state != CARDANO_SUCCESS)
  {
    return peek_state;
  }

  if (state == CARDANO_CBOR_READER_STATE_NULL)
  {
    const cardano_error_t read_null_result = cardano_cbor_reader_read_null(reader);

    if (read_null_result != CARDANO_SUCCESS)
    {
      return read_null_result;
    }
  }
  else
  {
    cardano_error_t read_ipv6_result = cardano_ipv6_from_cbor(reader, &ipv6);

    if (read_ipv6_result != CARDANO_SUCCESS)
    {
      cardano_ipv4_unref(&ipv4);
      return read_ipv6_result;
    }
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    *single_host_addr_relay = NULL;

    cardano_ipv4_unref(&ipv4);
    cardano_ipv6_unref(&ipv6);

    return expect_end_array_result;
  }

  cardano_error_t new_result = cardano_single_host_addr_relay_new(
    has_port ? &port : NULL,
    ipv4,
    ipv6,
    single_host_addr_relay);

  cardano_ipv4_unref(&ipv4);
  cardano_ipv6_unref(&ipv6);

  return new_result;
}

cardano_error_t
cardano_single_host_addr_relay_to_cbor(const cardano_single_host_addr_relay_t* single_host_addr_relay, cardano_cbor_writer_t* writer)
{
  if (single_host_addr_relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(
    writer,
    EMBEDDED_GROUP_SIZE);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  cardano_error_t write_uint_result = cardano_cbor_writer_write_uint(writer, CARDANO_RELAY_TYPE_SINGLE_HOST_ADDRESS);

  if (write_uint_result != CARDANO_SUCCESS)
  {
    return write_uint_result;
  }

  if (single_host_addr_relay->port == NULL)
  {
    cardano_error_t write_null_result = cardano_cbor_writer_write_null(writer);

    if (write_null_result != CARDANO_SUCCESS)
    {
      return write_null_result;
    }
  }
  else
  {
    cardano_error_t write_port_result = cardano_cbor_writer_write_uint(writer, *single_host_addr_relay->port);

    if (write_port_result != CARDANO_SUCCESS)
    {
      return write_port_result;
    }
  }

  if (single_host_addr_relay->ipv4 == NULL)
  {
    cardano_error_t write_null_result = cardano_cbor_writer_write_null(writer);

    if (write_null_result != CARDANO_SUCCESS)
    {
      return write_null_result;
    }
  }
  else
  {
    cardano_error_t write_ipv4_result = cardano_ipv4_to_cbor(single_host_addr_relay->ipv4, writer);

    if (write_ipv4_result != CARDANO_SUCCESS)
    {
      return write_ipv4_result;
    }
  }

  if (single_host_addr_relay->ipv6 == NULL)
  {
    cardano_error_t write_null_result = cardano_cbor_writer_write_null(writer);

    if (write_null_result != CARDANO_SUCCESS)
    {
      return write_null_result;
    }
  }
  else
  {
    cardano_error_t write_ipv6_result = cardano_ipv6_to_cbor(single_host_addr_relay->ipv6, writer);

    if (write_ipv6_result != CARDANO_SUCCESS)
    {
      return write_ipv6_result;
    }
  }

  return CARDANO_SUCCESS;
}

const uint16_t*
cardano_single_host_addr_relay_get_port(
  const cardano_single_host_addr_relay_t* relay)
{
  if (relay == NULL)
  {
    return NULL;
  }

  return relay->port;
}

cardano_error_t
cardano_single_host_addr_relay_set_port(
  cardano_single_host_addr_relay_t* relay,
  const uint16_t*                   port)
{
  if (relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (port == NULL)
  {
    if (relay->port != NULL)
    {
      _cardano_free(relay->port);
      relay->port = NULL;
    }

    return CARDANO_SUCCESS;
  }

  if (relay->port == NULL)
  {
    relay->port = _cardano_malloc(sizeof(uint16_t));

    if (relay->port == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }
  }

  *relay->port = *port;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_single_host_addr_relay_get_ipv4(
  cardano_single_host_addr_relay_t* single_host_addr_relay,
  cardano_ipv4_t**                  ipv4)
{
  if (single_host_addr_relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (ipv4 == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_ipv4_ref(single_host_addr_relay->ipv4);
  *ipv4 = single_host_addr_relay->ipv4;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_single_host_addr_relay_set_ipv4(
  cardano_single_host_addr_relay_t* single_host_addr_relay,
  cardano_ipv4_t*                   ipv4)
{
  if (single_host_addr_relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (ipv4 == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_ipv4_ref(ipv4);
  cardano_ipv4_unref(&single_host_addr_relay->ipv4);
  single_host_addr_relay->ipv4 = ipv4;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_single_host_addr_relay_get_ipv6(
  cardano_single_host_addr_relay_t* single_host_addr_relay,
  cardano_ipv6_t**                  ipv6)
{
  if (single_host_addr_relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (ipv6 == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_ipv6_ref(single_host_addr_relay->ipv6);
  *ipv6 = single_host_addr_relay->ipv6;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_single_host_addr_relay_set_ipv6(
  cardano_single_host_addr_relay_t* single_host_addr_relay,
  cardano_ipv6_t*                   ipv6)
{
  if (single_host_addr_relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (ipv6 == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_ipv6_ref(ipv6);
  cardano_ipv6_unref(&single_host_addr_relay->ipv6);
  single_host_addr_relay->ipv6 = ipv6;

  return CARDANO_SUCCESS;
}

void
cardano_single_host_addr_relay_unref(cardano_single_host_addr_relay_t** single_host_addr_relay)
{
  if ((single_host_addr_relay == NULL) || (*single_host_addr_relay == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*single_host_addr_relay)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *single_host_addr_relay = NULL;
    return;
  }
}

void
cardano_single_host_addr_relay_ref(cardano_single_host_addr_relay_t* single_host_addr_relay)
{
  if (single_host_addr_relay == NULL)
  {
    return;
  }

  cardano_object_ref(&single_host_addr_relay->base);
}

size_t
cardano_single_host_addr_relay_refcount(const cardano_single_host_addr_relay_t* single_host_addr_relay)
{
  if (single_host_addr_relay == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&single_host_addr_relay->base);
}

void
cardano_single_host_addr_relay_set_last_error(cardano_single_host_addr_relay_t* single_host_addr_relay, const char* message)
{
  cardano_object_set_last_error(&single_host_addr_relay->base, message);
}

const char*
cardano_single_host_addr_relay_get_last_error(const cardano_single_host_addr_relay_t* single_host_addr_relay)
{
  return cardano_object_get_last_error(&single_host_addr_relay->base);
}