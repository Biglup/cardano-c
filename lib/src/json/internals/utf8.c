/**
 * \file utf8.c
 *
 * \author angel.castillo
 * \date   Dec 08, 2024
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

#include "utf8.h"
#include <cardano/typedefs.h>
#include <string.h>

/* FUNCTIONS *****************************************************************/

size_t
cardano_utf8_sequence_length(const byte_t first_byte)
{
  if (first_byte <= 0x7fU)
  {
    return 1U;
  }

  else if ((first_byte & 0xe0U) == 0xc0U)
  {
    return 2U;
  }

  else if ((first_byte & 0xf0U) == 0xe0U)
  {
    return 3U;
  }

  else if ((first_byte & 0xf8U) == 0xf0U)
  {
    return 4U;
  }
  else
  {
    return 0U;
  }
}

bool
cardano_is_valid_utf8_char(const unsigned char* data, const size_t length)
{
  if (length == 0U)
  {
    return false;
  }

  size_t seq_len = cardano_utf8_sequence_length(data[0]);

  if ((seq_len == 0U) || (seq_len > length))
  {
    return false;
  }

  for (size_t i = 1U; i < seq_len; i++)
  {
    if ((data[i] & 0xc0U) != 0x80U)
    {
      return false;
    }
  }

  return true;
}

int32_t
cardano_parse_hex_digit(const char c)
{
  if ((c >= '0') && (c <= '9'))
  {
    return c - '0';
  }
  if ((c >= 'A') && (c <= 'F'))
  {
    return c - 'A' + 10;
  }
  if ((c >= 'a') && (c <= 'f'))
  {
    return c - 'a' + 10;
  }

  return -1;
}

int32_t
cardano_parse_unicode_escape(const char* str)
{
  int32_t codepoint = 0;

  for (size_t i = 0U; i < 4U; ++i)
  {
    int32_t d = cardano_parse_hex_digit(str[i]);

    if (d < 0)
    {
      return -1;
    }

    uint32_t shifted_codepoint = (uint32_t)codepoint << 4U;
    uint32_t result            = shifted_codepoint | (uint32_t)d;

    codepoint = (int32_t)result;
  }

  return codepoint;
}

size_t
cardano_encode_utf8(const int32_t codepoint, char* out)
{
  if (codepoint < 0x80)
  {
    out[0] = (char)codepoint;

    return 1;
  }
  else if (codepoint < 0x800)
  {
    uint32_t shifted_codepoint = (uint32_t)codepoint;

    uint32_t part1 = 0xC0U | (shifted_codepoint >> 6U);
    uint32_t part2 = 0x80U | (shifted_codepoint & 0x3FU);

    out[0] = (char)part1;
    out[1] = (char)part2;

    return 2;
  }
  else if (codepoint < 0x10000)
  {
    uint32_t shifted_codepoint = (uint32_t)codepoint;

    uint32_t part1 = 0xE0U | ((shifted_codepoint >> 12U) & 0x0FU);
    uint32_t part2 = 0x80U | ((shifted_codepoint >> 6U) & 0x3FU);
    uint32_t part3 = 0x80U | (shifted_codepoint & 0x3FU);

    out[0] = (char)part1;
    out[1] = (char)part2;
    out[2] = (char)part3;

    return 3;
  }
  else if (codepoint <= 0x10FFFF)
  {
    uint32_t shifted_codepoint = (uint32_t)codepoint;

    uint32_t part1 = 0xF0U | ((shifted_codepoint >> 18U) & 0x07U);
    uint32_t part2 = 0x80U | ((shifted_codepoint >> 12U) & 0x3FU);
    uint32_t part3 = 0x80U | ((shifted_codepoint >> 6U) & 0x3FU);
    uint32_t part4 = 0x80U | (shifted_codepoint & 0x3FU);

    out[0] = (char)part1;
    out[1] = (char)part2;
    out[2] = (char)part3;
    out[3] = (char)part4;

    return 4;
  }
  else
  {
    return 0;
  }
}

size_t
cardano_decode_unicode_sequence(const char* str, const size_t len, char* out)
{
  if (len < 6U)
  {
    return 0U;
  }

  int32_t high = cardano_parse_unicode_escape(&str[2]);

  if (high < 0)
  {
    return 0U;
  }

  if ((high >= 0xd800) && (high <= 0xdbff))
  {
    if ((len < 12U) || (str[6] != '\\') || (str[7] != 'u'))
    {
      return 0U;
    }

    int32_t low = cardano_parse_unicode_escape(&str[8]);

    if ((low < 0xdc00) || (low > 0xdfff))
    {
      return 0U;
    }

    uint32_t masked_high = (uint32_t)high & 0x3FFU;
    uint32_t masked_low  = (uint32_t)low & 0x3FFU;
    uint32_t high_bits   = masked_high << 10U;
    uint32_t codepoint   = 0x10000U + high_bits + masked_low;

    return cardano_encode_utf8(codepoint, out);
  }
  else
  {
    return cardano_encode_utf8(high, out);
  }
}
