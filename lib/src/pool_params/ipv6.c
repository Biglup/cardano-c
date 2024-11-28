/**
 * \file ipv6.c
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
#include <cardano/pool_params/ipv6.h>

#include "../allocators.h"
#include "../endian.h"
#include "../string_safe.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents an IPv6 address.
 *
 * Each instance of `cardano_ipv6_t` holds a single IPv6 address in network byte order (big endian).
 */
typedef struct cardano_ipv6_t
{
    cardano_object_t base;
    byte_t           ip_bytes[16];
    char             ip_str[40];
} cardano_ipv6_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a ipv6 object.
 *
 * This function is responsible for properly deallocating a ipv6 object (`cardano_ipv6_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the ipv6 object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_ipv6_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the ipv6
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_ipv6_deallocate(void* object)
{
  assert(object != NULL);

  _cardano_free(object);
}

/**
 * Converts a 16-byte array representing an IPv6 address into a colon-separated hexadecimal string.
 *
 * \param[in] ip_data Pointer to an array containing the 16 bytes of the IPv6 address.
 * \param[in] ip_data_size The size of the array pointed to by ip_data. Should be 16 for valid IPv6 addresses.
 * \param[out] destination Pointer to the buffer where the resulting string representation of the IP address will be stored.
 * \param[in] destination_size The size of the buffer pointed to by destination. Must be at least 40 bytes to fit the entire string plus the null terminator.
 *
 * \warning The function assumes ip_data_size is correctly 16. If not, the behavior is undefined.
 * Make sure the destination is large enough to hold the IP address string (up to "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx\0").
 */
static void
ip_to_string(const byte_t* ip_data, const size_t ip_data_size, char* destination, const size_t destination_size)
{
  assert(ip_data != NULL);
  assert(ip_data_size == 16U);
  assert(destination != NULL);
  assert(destination_size >= 40U);

  CARDANO_UNUSED(ip_data_size);

  int32_t result = snprintf(destination, destination_size, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x", ip_data[0], ip_data[1], ip_data[2], ip_data[3], ip_data[4], ip_data[5], ip_data[6], ip_data[7], ip_data[8], ip_data[9], ip_data[10], ip_data[11], ip_data[12], ip_data[13], ip_data[14], ip_data[15]);

  assert(result > 0);
  CARDANO_UNUSED(result);
}

/**
 * Converts a string representing an IPv6 address in colon-separated hexadecimal format into a 16-byte array.
 *
 * \param ip_string Pointer to the string containing the IPv6 address in colon-separated hexadecimal format.
 * \param ip_data Pointer to the array where the bytes of the IPv6 address will be stored.
 * \param ip_data_size The size of the array pointed to by ip_data. Should be at least 16.
 *
 * \return cardano_error_t Returns CARDANO_SUCCESS if the conversion is successful,
 *         CARDANO_ERROR_INVALID_ARGUMENT if the string format is invalid or ip_data_size is less than 16.
 */
static cardano_error_t
ip_from_string(const char* ip_string, byte_t* ip_data, const size_t ip_data_size)
{
  assert(ip_string != NULL);
  assert(ip_data != NULL);
  assert(ip_data_size >= 16U);

  uint16_t temp[8];
  int32_t  result = sscanf(ip_string, "%4hx:%4hx:%4hx:%4hx:%4hx:%4hx:%4hx:%4hx", &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5], &temp[6], &temp[7]); // nosemgrep

  if (result == 8)
  {
    for (size_t i = 0U; i < 8U; i++)
    {
      size_t          offset       = 2U * i;
      cardano_error_t write_result = cardano_write_uint16_be(temp[i], ip_data, ip_data_size, offset);

      if (write_result != CARDANO_SUCCESS)
      {
        return write_result;
      }
    }
  }
  else
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  return CARDANO_SUCCESS;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_ipv6_new(const byte_t* data, const size_t size, cardano_ipv6_t** ipv6)
{
  if (ipv6 == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size != 16U)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  *ipv6 = _cardano_malloc(sizeof(cardano_ipv6_t));

  if (*ipv6 == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*ipv6)->base.deallocator   = cardano_ipv6_deallocate;
  (*ipv6)->base.ref_count     = 1;
  (*ipv6)->base.last_error[0] = '\0';

  cardano_safe_memcpy((*ipv6)->ip_bytes, 16, data, 16);
  ip_to_string((*ipv6)->ip_bytes, 16, (*ipv6)->ip_str, 40);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_ipv6_from_string(const char* string, const size_t size, cardano_ipv6_t** ipv6)
{
  if (ipv6 == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (string == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (size != 39U)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  *ipv6 = _cardano_malloc(sizeof(cardano_ipv6_t));

  if (*ipv6 == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*ipv6)->base.deallocator   = cardano_ipv6_deallocate;
  (*ipv6)->base.ref_count     = 1;
  (*ipv6)->base.last_error[0] = '\0';

  cardano_error_t result = ip_from_string(string, (*ipv6)->ip_bytes, 16);

  if (result != CARDANO_SUCCESS)
  {
    cardano_ipv6_deallocate(*ipv6);
    *ipv6 = NULL;
    return result;
  }

  CARDANO_UNUSED(memset((*ipv6)->ip_str, 0, 40));

  cardano_safe_memcpy((*ipv6)->ip_str, 40, string, size);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_ipv6_from_cbor(cardano_cbor_reader_t* reader, cardano_ipv6_t** ipv6)
{
  if (ipv6 == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *ipv6 = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_buffer_t* buffer           = NULL;
  cardano_error_t   read_byte_string = cardano_cbor_reader_read_bytestring(reader, &buffer);

  if (read_byte_string != CARDANO_SUCCESS)
  {
    return read_byte_string;
  }

  cardano_error_t result = cardano_ipv6_new(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer), ipv6);

  cardano_buffer_unref(&buffer);

  return result;
}

cardano_error_t
cardano_ipv6_to_cbor(const cardano_ipv6_t* ipv6, cardano_cbor_writer_t* writer)
{
  if (ipv6 == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_cbor_writer_write_bytestring(writer, ipv6->ip_bytes, sizeof(ipv6->ip_bytes));
}

size_t
cardano_ipv6_get_bytes_size(const cardano_ipv6_t* ipv6)
{
  if (ipv6 == NULL)
  {
    return 0;
  }

  return 16;
}

const byte_t*
cardano_ipv6_get_bytes(const cardano_ipv6_t* ipv6)
{
  if (ipv6 == NULL)
  {
    return NULL;
  }

  return ipv6->ip_bytes;
}

size_t
cardano_ipv6_get_string_size(const cardano_ipv6_t* ipv6)
{
  if (ipv6 == NULL)
  {
    return 0;
  }

  return cardano_safe_strlen(ipv6->ip_str, 40);
}

const char*
cardano_ipv6_get_string(const cardano_ipv6_t* ipv6)
{
  if (ipv6 == NULL)
  {
    return NULL;
  }

  return ipv6->ip_str;
}

void
cardano_ipv6_unref(cardano_ipv6_t** ipv6)
{
  if ((ipv6 == NULL) || (*ipv6 == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*ipv6)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *ipv6 = NULL;
    return;
  }
}

void
cardano_ipv6_ref(cardano_ipv6_t* ipv6)
{
  if (ipv6 == NULL)
  {
    return;
  }

  cardano_object_ref(&ipv6->base);
}

size_t
cardano_ipv6_refcount(const cardano_ipv6_t* ipv6)
{
  if (ipv6 == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&ipv6->base);
}

void
cardano_ipv6_set_last_error(cardano_ipv6_t* ipv6, const char* message)
{
  cardano_object_set_last_error(&ipv6->base, message);
}

const char*
cardano_ipv6_get_last_error(const cardano_ipv6_t* ipv6)
{
  return cardano_object_get_last_error(&ipv6->base);
}