/**
 * \file multi_host_name_relay.c
 *
 * \author angel.castillo
 * \date   Jun 26, 2024
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
#include <cardano/pool_params/multi_host_name_relay.h>
#include <cardano/pool_params/relay.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents an multi host name relay.
 *
 * Each instance of `cardano_multi_host_name_relay_t` holds a single multi host name relay.
 */
typedef struct cardano_multi_host_name_relay_t
{
    cardano_object_t base;
    char             dns[65];
} cardano_multi_host_name_relay_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a multi_host_name_relay object.
 *
 * This function is responsible for properly deallocating a multi_host_name_relay object (`cardano_multi_host_name_relay_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the multi_host_name_relay object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_multi_host_name_relay_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the multi_host_name_relay
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_multi_host_name_relay_deallocate(void* object)
{
  assert(object != NULL);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_multi_host_name_relay_new(const char* dns, const size_t str_size, cardano_multi_host_name_relay_t** multi_host_name_relay)
{
  if (multi_host_name_relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (dns == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((str_size > 64U) || (str_size == 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  *multi_host_name_relay = _cardano_malloc(sizeof(cardano_multi_host_name_relay_t));

  if (*multi_host_name_relay == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*multi_host_name_relay)->base.deallocator   = cardano_multi_host_name_relay_deallocate;
  (*multi_host_name_relay)->base.ref_count     = 1;
  (*multi_host_name_relay)->base.last_error[0] = '\0';

  CARDANO_UNUSED(memset((*multi_host_name_relay)->dns, 0, 65));
  cardano_safe_memcpy((*multi_host_name_relay)->dns, 65, dns, str_size);

  const size_t dns_size                   = cardano_safe_strlen((*multi_host_name_relay)->dns, 64);
  (*multi_host_name_relay)->dns[dns_size] = '\0';

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_multi_host_name_relay_from_cbor(cardano_cbor_reader_t* reader, cardano_multi_host_name_relay_t** multi_host_name_relay)
{
  if (multi_host_name_relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *multi_host_name_relay = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "multi_host_name_relay";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    *multi_host_name_relay = NULL;
    return expect_array_result;
  }

  uint64_t              type             = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_enum_value(
    validator_name,
    "type",
    reader,
    CARDANO_RELAY_TYPE_MULTI_HOST_NAME,
    (enum_to_string_callback_t)((void*)&cardano_relay_type_to_string),
    &type);

  CARDANO_UNUSED(type);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  cardano_buffer_t* dns = NULL;

  const cardano_error_t read_string_result = cardano_cbor_reader_read_textstring(reader, &dns);

  if (read_string_result != CARDANO_SUCCESS)
  {
    return read_string_result;
  }

  const size_t dns_size = cardano_buffer_get_size(dns);
  const char*  dns_data = (char*)((void*)cardano_buffer_get_data(dns));

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&dns);
    *multi_host_name_relay = NULL;

    return expect_end_array_result;
  }

  const cardano_error_t result = cardano_multi_host_name_relay_new(dns_data, dns_size, multi_host_name_relay);

  cardano_buffer_unref(&dns);

  return result;
}

cardano_error_t
cardano_multi_host_name_relay_to_cbor(const cardano_multi_host_name_relay_t* multi_host_name_relay, cardano_cbor_writer_t* writer)
{
  if (multi_host_name_relay == NULL)
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

  cardano_error_t write_uint_result = cardano_cbor_writer_write_uint(writer, CARDANO_RELAY_TYPE_MULTI_HOST_NAME);

  if (write_uint_result != CARDANO_SUCCESS)
  {
    return write_uint_result;
  }

  return cardano_cbor_writer_write_textstring(writer, multi_host_name_relay->dns, cardano_safe_strlen(multi_host_name_relay->dns, 64));
}

size_t
cardano_multi_host_name_relay_get_dns_size(
  const cardano_multi_host_name_relay_t* multi_host_name_relay)
{
  if (multi_host_name_relay == NULL)
  {
    return 0;
  }

  return cardano_safe_strlen(multi_host_name_relay->dns, 64) + 1U;
}

const char*
cardano_multi_host_name_relay_get_dns(
  const cardano_multi_host_name_relay_t* multi_host_name_relay)
{
  if (multi_host_name_relay == NULL)
  {
    return NULL;
  }

  return multi_host_name_relay->dns;
}

cardano_error_t
cardano_multi_host_name_relay_set_dns(
  const char*                      dns,
  size_t                           dns_size,
  cardano_multi_host_name_relay_t* multi_host_name_relay)
{
  if (multi_host_name_relay == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (dns == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((dns_size > 64U) || (dns_size == 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  cardano_safe_memcpy((void*)&multi_host_name_relay->dns, 65U, dns, dns_size);

  return CARDANO_SUCCESS;
}

void
cardano_multi_host_name_relay_unref(cardano_multi_host_name_relay_t** multi_host_name_relay)
{
  if ((multi_host_name_relay == NULL) || (*multi_host_name_relay == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*multi_host_name_relay)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *multi_host_name_relay = NULL;
    return;
  }
}

void
cardano_multi_host_name_relay_ref(cardano_multi_host_name_relay_t* multi_host_name_relay)
{
  if (multi_host_name_relay == NULL)
  {
    return;
  }

  cardano_object_ref(&multi_host_name_relay->base);
}

size_t
cardano_multi_host_name_relay_refcount(const cardano_multi_host_name_relay_t* multi_host_name_relay)
{
  if (multi_host_name_relay == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&multi_host_name_relay->base);
}

void
cardano_multi_host_name_relay_set_last_error(cardano_multi_host_name_relay_t* multi_host_name_relay, const char* message)
{
  cardano_object_set_last_error(&multi_host_name_relay->base, message);
}

const char*
cardano_multi_host_name_relay_get_last_error(const cardano_multi_host_name_relay_t* multi_host_name_relay)
{
  return cardano_object_get_last_error(&multi_host_name_relay->base);
}