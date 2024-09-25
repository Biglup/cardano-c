/**
 * \file ipv4.c
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
#include <cardano/pool_params/ipv4.h>

#include "../allocators.h"
#include "../string_safe.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents an IPv4 address.
 *
 * Each instance of `cardano_ipv4_t` holds a single IPv4 address in network byte order (big endian).
 */
typedef struct cardano_ipv4_t
{
    cardano_object_t base;
    byte_t           ip_bytes[4];
    char             ip_str[16];
} cardano_ipv4_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a ipv4 object.
 *
 * This function is responsible for properly deallocating a ipv4 object (`cardano_ipv4_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the ipv4 object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_ipv4_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the ipv4
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_ipv4_deallocate(void* object)
{
  assert(object != NULL);

  _cardano_free(object);
}

/**
 * Converts a 4-byte array representing an IPv4 address into a dotted decimal string.
 *
 * \param[in] ip_data Pointer to an array containing the 4 bytes of the IPv4 address.
 * \param[in] ip_data_size The size of the array pointed to by ip_data. Should be 4 for valid IPv4 addresses.
 * \param[out] destination Pointer to the buffer where the resulting string representation of the IP address will be stored.
 * \param[in] destination_size The size of the buffer pointed to by destination. Must be at least 16 bytes to fit the entire string.
 *
 * \warning The function assumes ip_data_size is correctly 4. If not, the behavior is undefined.
 * Make sure destination is large enough to hold the IP address string ("xxx.xxx.xxx.xxx\0").
 */
static void
ip_to_string(const byte_t* ip_data, const size_t ip_data_size, char* destination, const size_t destination_size)
{
  assert(ip_data != NULL);
  assert(ip_data_size == 4U);
  assert(destination != NULL);
  assert(destination_size == 16U);

  CARDANO_UNUSED(ip_data_size);

  const int32_t result = snprintf(destination, destination_size, "%d.%d.%d.%d", ip_data[0], ip_data[1], ip_data[2], ip_data[3]);

  assert(result > 0);
  CARDANO_UNUSED(result);
}

/**
 * Converts a string representing an IPv4 address in dotted decimal format into a 4-byte array.
 *
 * \param ip_string Pointer to the string containing the IPv4 address in dotted decimal format.
 * \param ip_data Pointer to the array where the bytes of the IPv4 address will be stored.
 * \param ip_data_size The size of the array pointed to by ip_data. Should be at least 4.
 *
 * \return int Returns 0 if the conversion is successful, \ref cardano_error_t if the string format is invalid or ip_data_size is less than 4.
 */
static cardano_error_t
ip_from_string(const char* ip_string, byte_t* ip_data, const size_t ip_data_size)
{
  assert(ip_string != NULL);
  assert(ip_data != NULL);
  assert(ip_data_size == 4U);

  CARDANO_UNUSED(ip_data_size);

  int32_t bytes[4] = { 0 };
  int32_t result   = sscanf(ip_string, "%d.%d.%d.%d", &bytes[0], &bytes[1], &bytes[2], &bytes[3]); // nosemgrep

  if (result != 4)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  for (size_t i = 0U; i < 4U; i++)
  {
    if ((bytes[i] < 0) || (bytes[i] > 255))
    {
      return CARDANO_ERROR_INVALID_ARGUMENT;
    }

    ip_data[i] = (byte_t)bytes[i];
  }

  return CARDANO_SUCCESS;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_ipv4_new(const byte_t* data, const size_t size, cardano_ipv4_t** ipv4)
{
  if (ipv4 == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size != 4U)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  *ipv4 = _cardano_malloc(sizeof(cardano_ipv4_t));

  if (*ipv4 == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*ipv4)->base.deallocator   = cardano_ipv4_deallocate;
  (*ipv4)->base.ref_count     = 1;
  (*ipv4)->base.last_error[0] = '\0';

  cardano_safe_memcpy((*ipv4)->ip_bytes, 4, data, 4);
  ip_to_string((*ipv4)->ip_bytes, 4, (*ipv4)->ip_str, 16);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_ipv4_from_string(const char* string, const size_t size, cardano_ipv4_t** ipv4)
{
  if (ipv4 == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (string == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((size < 7U) || (size > 15U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  *ipv4 = _cardano_malloc(sizeof(cardano_ipv4_t));

  if (*ipv4 == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*ipv4)->base.deallocator   = cardano_ipv4_deallocate;
  (*ipv4)->base.ref_count     = 1;
  (*ipv4)->base.last_error[0] = '\0';

  cardano_error_t result = ip_from_string(string, (*ipv4)->ip_bytes, 4);

  if (result != CARDANO_SUCCESS)
  {
    cardano_ipv4_deallocate(*ipv4);
    *ipv4 = NULL;
    return result;
  }

  CARDANO_UNUSED(memset((*ipv4)->ip_str, 0, 16));

  cardano_safe_memcpy((*ipv4)->ip_str, 16, string, size);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_ipv4_from_cbor(cardano_cbor_reader_t* reader, cardano_ipv4_t** ipv4)
{
  if (ipv4 == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *ipv4 = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_buffer_t* buffer           = NULL;
  cardano_error_t   read_byte_string = cardano_cbor_reader_read_bytestring(reader, &buffer);

  if (read_byte_string != CARDANO_SUCCESS)
  {
    return read_byte_string;
  }

  cardano_error_t result = cardano_ipv4_new(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer), ipv4);

  cardano_buffer_unref(&buffer);

  return result;
}

cardano_error_t
cardano_ipv4_to_cbor(const cardano_ipv4_t* ipv4, cardano_cbor_writer_t* writer)
{
  if (ipv4 == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_cbor_writer_write_bytestring(writer, ipv4->ip_bytes, sizeof(ipv4->ip_bytes));
}

size_t
cardano_ipv4_get_bytes_size(const cardano_ipv4_t* ipv4)
{
  if (ipv4 == NULL)
  {
    return 0;
  }

  return 4;
}

const byte_t*
cardano_ipv4_get_bytes(const cardano_ipv4_t* ipv4)
{
  if (ipv4 == NULL)
  {
    return NULL;
  }

  return ipv4->ip_bytes;
}

size_t
cardano_ipv4_get_string_size(const cardano_ipv4_t* ipv4)
{
  if (ipv4 == NULL)
  {
    return 0;
  }

  return cardano_safe_strlen(ipv4->ip_str, 16);
}

const char*
cardano_ipv4_get_string(const cardano_ipv4_t* ipv4)
{
  if (ipv4 == NULL)
  {
    return NULL;
  }

  return ipv4->ip_str;
}

void
cardano_ipv4_unref(cardano_ipv4_t** ipv4)
{
  if ((ipv4 == NULL) || (*ipv4 == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*ipv4)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *ipv4 = NULL;
    return;
  }
}

void
cardano_ipv4_ref(cardano_ipv4_t* ipv4)
{
  if (ipv4 == NULL)
  {
    return;
  }

  cardano_object_ref(&ipv4->base);
}

size_t
cardano_ipv4_refcount(const cardano_ipv4_t* ipv4)
{
  if (ipv4 == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&ipv4->base);
}

void
cardano_ipv4_set_last_error(cardano_ipv4_t* ipv4, const char* message)
{
  cardano_object_set_last_error(&ipv4->base, message);
}

const char*
cardano_ipv4_get_last_error(const cardano_ipv4_t* ipv4)
{
  return cardano_object_get_last_error(&ipv4->base);
}