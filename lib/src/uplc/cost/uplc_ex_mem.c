/**
 * \file uplc_ex_mem.c
 *
 * \author angel.castillo
 * \date   Jun 27, 2026
 *
 * Copyright 2026 Biglup Labs
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

#include "uplc_ex_mem.h"
#include "../ast/uplc_int.h"
#include "../data/uplc_data.h"
#include "../machine/uplc_value.h"
#include "uplc_cost_sat.h"

#include <cardano/common/bigint.h>

#include <stddef.h>
#include <stdint.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief The number of 64-bit words an integer occupies for one memory unit.
 */
static const int64_t CARDANO_UPLC_INTEGER_WORD_BITS = 64;

/**
 * \brief The byte-string chunk size: one memory unit per this many bytes.
 */
static const int64_t CARDANO_UPLC_BYTE_STRING_CHUNK = 8;

/**
 * \brief The utf-8 string chunk size: one memory unit per this many bytes.
 */
static const int64_t CARDANO_UPLC_STRING_UTF8_CHUNK = 4;

/**
 * \brief The ex-mem size of a BLS12-381 G1 element.
 *
 * Equal to the 144-byte element size divided by 8.
 */
static const int64_t CARDANO_UPLC_BLS_G1_EX_MEM = 18;

/**
 * \brief The ex-mem size of a BLS12-381 G2 element.
 *
 * Equal to the 288-byte element size divided by 8.
 */
static const int64_t CARDANO_UPLC_BLS_G2_EX_MEM = 36;

/**
 * \brief The ex-mem size of a BLS12-381 Miller-loop result.
 *
 * Equal to the 576-byte result size divided by 8.
 */
static const int64_t CARDANO_UPLC_BLS_ML_RESULT_EX_MEM = 72;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Counts the Unicode code points in a UTF-8 byte sequence.
 *
 * Code points are counted by skipping the continuation bytes (those whose top
 * two bits are 10); every other byte begins a new code point. The input is
 * assumed to be valid UTF-8, as string constants are validated on construction.
 *
 * \param[in] bytes The UTF-8 bytes, or NULL when \p size is 0.
 * \param[in] size The number of bytes.
 *
 * \return The code-point count.
 */
static int64_t
utf8_code_point_count(const byte_t* bytes, size_t size)
{
  int64_t count = 0;
  size_t  i     = 0U;

  if (bytes == NULL)
  {
    return 0;
  }

  for (i = 0U; i < size; ++i)
  {
    if ((bytes[i] & 0xC0U) != 0x80U)
    {
      ++count;
    }
  }

  return count;
}

/* DEFINITIONS ***************************************************************/

size_t
cardano_uplc_value_entry_token_count(const cardano_uplc_constant_t* entry)
{
  if ((entry == NULL) || (entry->kind != CARDANO_UPLC_TYPE_PAIR))
  {
    return 0U;
  }

  if ((entry->as.pair.snd == NULL) || (entry->as.pair.snd->kind != CARDANO_UPLC_TYPE_LIST))
  {
    return 0U;
  }

  return entry->as.pair.snd->as.list.count;
}

int64_t
cardano_uplc_value_token_count(const cardano_uplc_constant_t* constant)
{
  int64_t total = 0;
  size_t  i     = 0U;

  if ((constant == NULL) || (constant->kind != CARDANO_UPLC_TYPE_VALUE))
  {
    return 0;
  }

  for (i = 0U; i < constant->as.list.count; ++i)
  {
    total += (int64_t)cardano_uplc_value_entry_token_count(constant->as.list.items[i]);
  }

  return total;
}

int64_t
cardano_uplc_integer_ex_mem(const cardano_bigint_t* value)
{
  size_t  bits = 0U;
  int64_t log2 = 0;

  if ((value == NULL) || cardano_bigint_is_zero(value))
  {
    return 1;
  }

  bits = cardano_bigint_bit_length(value);
  log2 = (int64_t)bits - 1;

  return (log2 / CARDANO_UPLC_INTEGER_WORD_BITS) + 1;
}

int64_t
cardano_uplc_byte_string_ex_mem(size_t length)
{
  if (length == 0U)
  {
    return 1;
  }

  return (((int64_t)length - 1) / CARDANO_UPLC_BYTE_STRING_CHUNK) + 1;
}

int64_t
cardano_uplc_string_ex_mem(const byte_t* bytes, size_t size, bool costs_by_utf8_bytes)
{
  if (costs_by_utf8_bytes)
  {
    if (size == 0U)
    {
      return 0;
    }

    return (((int64_t)size - 1) / CARDANO_UPLC_STRING_UTF8_CHUNK) + 1;
  }

  return utf8_code_point_count(bytes, size);
}

int64_t
cardano_uplc_data_ex_mem(const cardano_uplc_data_t* data)
{
  return cardano_uplc_data_node_ex_mem(data);
}

int64_t
cardano_uplc_constant_ex_mem(
  const cardano_uplc_constant_t* constant,
  bool                           costs_strings_by_utf8_bytes)
{
  int64_t total = 0;

  if (constant == NULL)
  {
    return 0;
  }

  switch (constant->kind)
  {
    case CARDANO_UPLC_TYPE_INTEGER:
    {
      if (cardano_uplc_constant_int_is_small(constant))
      {
        total = 1;
      }
      else
      {
        total = cardano_uplc_integer_ex_mem(constant->as.integer.big);
      }

      break;
    }
    case CARDANO_UPLC_TYPE_BYTE_STRING:
    {
      total = cardano_uplc_byte_string_ex_mem(constant->as.bytes.size);
      break;
    }
    case CARDANO_UPLC_TYPE_STRING:
    {
      total = cardano_uplc_string_ex_mem(
        constant->as.string.data,
        constant->as.string.size,
        costs_strings_by_utf8_bytes);
      break;
    }
    case CARDANO_UPLC_TYPE_UNIT:
    case CARDANO_UPLC_TYPE_BOOL:
    {
      total = 1;
      break;
    }
    case CARDANO_UPLC_TYPE_LIST:
    case CARDANO_UPLC_TYPE_ARRAY:
    {
      size_t i = 0U;

      for (i = 0U; i < constant->as.list.count; ++i)
      {
        total = sat_add(total, cardano_uplc_constant_ex_mem(constant->as.list.items[i], costs_strings_by_utf8_bytes));
      }
      break;
    }
    case CARDANO_UPLC_TYPE_VALUE:
    {
      total = cardano_uplc_value_token_count(constant);
      break;
    }
    case CARDANO_UPLC_TYPE_PAIR:
    {
      total = sat_add(
        cardano_uplc_constant_ex_mem(constant->as.pair.fst, costs_strings_by_utf8_bytes),
        cardano_uplc_constant_ex_mem(constant->as.pair.snd, costs_strings_by_utf8_bytes));
      break;
    }
    case CARDANO_UPLC_TYPE_DATA:
    {
      total = cardano_uplc_data_ex_mem(constant->as.data);
      break;
    }
    case CARDANO_UPLC_TYPE_BLS_G1:
    {
      total = CARDANO_UPLC_BLS_G1_EX_MEM;
      break;
    }
    case CARDANO_UPLC_TYPE_BLS_G2:
    {
      total = CARDANO_UPLC_BLS_G2_EX_MEM;
      break;
    }
    case CARDANO_UPLC_TYPE_BLS_ML_RESULT:
    {
      total = CARDANO_UPLC_BLS_ML_RESULT_EX_MEM;
      break;
    }
    default:
    {
      break;
    }
  }

  return total;
}

int64_t
cardano_uplc_value_ex_mem(
  const cardano_uplc_value_t* value,
  bool                        costs_strings_by_utf8_bytes)
{
  int64_t total = 0;

  if (value == NULL)
  {
    return 0;
  }

  if (value->ex_mem != CARDANO_UPLC_VALUE_EX_MEM_UNCOMPUTED)
  {
    return value->ex_mem;
  }

  switch (value->kind)
  {
    case CARDANO_UPLC_VALUE_CONSTANT:
    {
      total = cardano_uplc_constant_ex_mem(value->as.constant, costs_strings_by_utf8_bytes);
      break;
    }
    case CARDANO_UPLC_VALUE_DELAY:
    case CARDANO_UPLC_VALUE_LAMBDA:
    case CARDANO_UPLC_VALUE_BUILTIN:
    case CARDANO_UPLC_VALUE_CONSTR:
    {
      total = 1;
      break;
    }
    default:
    {
      break;
    }
  }

  ((cardano_uplc_value_t*)((void*)value))->ex_mem = total;

  return total;
}
