/**
 * \file json_parser.c
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

#include <cardano/typedefs.h>

#include "../../config.h"
#include "../../string_safe.h"
#include "json_object_common.h"
#include "json_parser.h"
#include "utf8.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const byte_t* ESC_QUOTE                  = (const byte_t*)"\""; // cppcheck-suppress misra-c2012-8.9
static const byte_t* ESC_BACKSLASH              = (const byte_t*)"\\"; // cppcheck-suppress misra-c2012-8.9
static const byte_t* ESC_SLASH                  = (const byte_t*)"/";  // cppcheck-suppress misra-c2012-8.9
static const byte_t* ESC_BACKSPACE              = (const byte_t*)"\b"; // cppcheck-suppress misra-c2012-8.9
static const byte_t* ESC_FORMFEED               = (const byte_t*)"\f"; // cppcheck-suppress misra-c2012-8.9
static const byte_t* ESC_NEWLINE                = (const byte_t*)"\n"; // cppcheck-suppress misra-c2012-8.9
static const byte_t* ESC_CARRIAGE               = (const byte_t*)"\r"; // cppcheck-suppress misra-c2012-8.9
static const byte_t* ESC_TAB                    = (const byte_t*)"\t"; // cppcheck-suppress misra-c2012-8.9
static const size_t  UNICODE_BASIC_ESCAPE_LEN   = 6U;                  // cppcheck-suppress misra-c2012-8.9
static const size_t  UNICODE_SURROGATE_PAIR_LEN = 12U;                 // cppcheck-suppress misra-c2012-8.9

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Sets the last error message in a JSON parsing context.
 *
 * This function records an error message in the `last_error` buffer of the specified
 * \ref cardano_json_parse_context_t instance. The message is truncated if it exceeds the
 * size of the buffer to ensure memory safety.
 *
 * \param[in,out] ctx     A pointer to the \ref cardano_json_parse_context_t instance where the error
 *                        message will be stored. This parameter must not be \c NULL.
 * \param[in]     message A null-terminated string containing the error message. This parameter
 *                        must not be \c NULL. If the message exceeds the buffer size, it will
 *                        be truncated to fit.
 */
static void
set_last_error(cardano_json_parse_context_t* ctx, const char* message)
{
  cardano_safe_memcpy(ctx->last_error, sizeof(ctx->last_error) - 1U, message, cardano_safe_strlen(message, sizeof(ctx->last_error) - 1U));
}

/* FUNCTIONS *****************************************************************/

void
cardano_skip_whitespace(cardano_json_parse_context_t* ctx)
{
  while (ctx->offset < ctx->length)
  {
    char c = ctx->input[ctx->offset];

    if ((c == ' ') || (c == '\n') || (c == '\r') || (c == '\t'))
    {
      ++ctx->offset;
    }
    else
    {
      break;
    }
  }
}

bool
cardano_has_char(const char to_match, const char* begin, const char* end)
{
  assert(begin != NULL);
  assert(end != NULL);
  assert(begin <= end);

  const char* current = begin;

  while (current < end)
  {
    if (*current == to_match)
    {
      return true;
    }

    ++current;
  }

  return false;
}

bool
cardano_handle_utf8_sequence(cardano_json_parse_context_t* ctx, cardano_buffer_t* buf)
{
  byte_t first_byte = (byte_t)ctx->input[ctx->offset];
  size_t seq_len    = cardano_utf8_sequence_length(first_byte);

  if ((seq_len == 0U) || ((ctx->offset + seq_len) > ctx->length))
  {
    set_last_error(ctx, "Invalid UTF-8 sequence");

    return false;
  }

  for (size_t i = 1U; i < seq_len; ++i)
  {
    if ((ctx->input[ctx->offset + i] & 0xC0) != 0x80)
    {
      set_last_error(ctx, "Invalid UTF-8 continuation byte");

      return false;
    }
  }

  cardano_error_t result = cardano_buffer_write(buf, (const byte_t*)&ctx->input[ctx->offset], seq_len);
  if (result != CARDANO_SUCCESS)
  {
    set_last_error(ctx, cardano_error_to_string(result));

    return false;
  }

  ctx->offset += seq_len;

  return true;
}

bool
cardano_handle_unicode_sequence(cardano_json_parse_context_t* ctx, cardano_buffer_t* buf)
{
  if ((ctx->offset + 4U) > ctx->length)
  {
    set_last_error(ctx, "Not enough characters for Unicode escape sequence");
    return false;
  }

  for (size_t i = 0; i < 4U; ++i)
  {
    if (!isxdigit((unsigned char)ctx->input[ctx->offset + i]))
    {
      set_last_error(ctx, "Invalid character in Unicode escape sequence");
      return false;
    }
  }

  int32_t high = cardano_parse_unicode_escape(&ctx->input[ctx->offset]);

  if (high < 0)
  {
    set_last_error(ctx, "Invalid Unicode escape sequence");
    return false;
  }

  size_t consumed = UNICODE_BASIC_ESCAPE_LEN;

  if ((high >= 0xd800) && (high <= 0xdbff) && ((ctx->offset + 10U) <= ctx->length))
  {
    if (((char)ctx->input[ctx->offset + 4U] == (char)'\\') && ((char)ctx->input[ctx->offset + 5U] == (char)'u'))
    {
      for (size_t i = 6U; i < 10U; ++i)
      {
        if (!isxdigit((unsigned char)ctx->input[ctx->offset + i]))
        {
          set_last_error(ctx, "Invalid character in second Unicode escape sequence");
          return false;
        }
      }

      int32_t low = cardano_parse_unicode_escape(&ctx->input[ctx->offset + 6U]);

      if ((low >= 0xdc00) && (low <= 0xdfff))
      {
        uint32_t masked_high = (uint32_t)high & 0x3ffU;
        uint32_t masked_low  = (uint32_t)low & 0x3ffU;
        uint32_t codepoint   = 0x10000U + (masked_high << 10U) + masked_low;

        char   utf8_buf[4]   = { 0 };
        size_t bytes_written = cardano_encode_utf8((int32_t)codepoint, utf8_buf);

        if (bytes_written == 0U)
        {
          set_last_error(ctx, "Failed to encode surrogate pair as UTF-8");
          return false;
        }

        cardano_error_t result = cardano_buffer_write(buf, (byte_t*)&utf8_buf[0], bytes_written);

        if (result != CARDANO_SUCCESS)
        {
          set_last_error(ctx, cardano_error_to_string(result));
          return false;
        }

        consumed = UNICODE_SURROGATE_PAIR_LEN;
      }
      else
      {
        set_last_error(ctx, "Invalid surrogate pair in Unicode escape sequence");
        return false;
      }
    }
  }
  else
  {
    char   utf8_buf[4]   = { 0 };
    size_t bytes_written = cardano_encode_utf8(high, utf8_buf);

    if (bytes_written == 0U)
    {
      set_last_error(ctx, "Failed to encode Unicode character as UTF-8");
      return false;
    }

    cardano_error_t result = cardano_buffer_write(buf, (byte_t*)&utf8_buf[0], bytes_written);

    if (result != CARDANO_SUCCESS)
    {
      set_last_error(ctx, cardano_error_to_string(result));
      return false;
    }
  }

  ctx->offset += (consumed - 2U);
  return true;
}

bool
cardano_handle_escape_sequence(cardano_json_parse_context_t* ctx, cardano_buffer_t* buf)
{
  if (ctx->offset >= ctx->length)
  {
    set_last_error(ctx, "Unexpected end of input in escape sequence");
    return false;
  }

  char e = ctx->input[ctx->offset];

  cardano_error_t result = CARDANO_SUCCESS;

  switch (e)
  {
    case '\"':
      result = cardano_buffer_write(buf, ESC_QUOTE, 1);
      break;
    case '\\':
      result = cardano_buffer_write(buf, ESC_BACKSLASH, 1);
      break;
    case '/':
      result = cardano_buffer_write(buf, ESC_SLASH, 1);
      break;
    case 'b':
      result = cardano_buffer_write(buf, ESC_BACKSPACE, 1);
      break;
    case 'f':
      result = cardano_buffer_write(buf, ESC_FORMFEED, 1);
      break;
    case 'n':
      result = cardano_buffer_write(buf, ESC_NEWLINE, 1);
      break;
    case 'r':
      result = cardano_buffer_write(buf, ESC_CARRIAGE, 1);
      break;
    case 't':
      result = cardano_buffer_write(buf, ESC_TAB, 1);
      break;
    case 'u':
      ++ctx->offset;
      if (!cardano_handle_unicode_sequence(ctx, buf))
      {
        return false;
      }
      return true;
    default:
      set_last_error(ctx, "Invalid escape sequence");
      return false;
  }

  if (result != CARDANO_SUCCESS)
  {
    set_last_error(ctx, cardano_error_to_string(result));
    return false;
  }

  ++ctx->offset;

  return true;
}

cardano_json_object_t*
cardano_parse_string_value(cardano_json_parse_context_t* ctx)
{
  cardano_skip_whitespace(ctx);

  if ((ctx->offset >= ctx->length) || ((char)ctx->input[ctx->offset] != (char)'\"'))
  {
    set_last_error(ctx, "Invalid JSON string start");

    return NULL;
  }

  ++ctx->offset;

  cardano_buffer_t* buf = cardano_buffer_new(128);

  if (buf == NULL)
  {
    set_last_error(ctx, "Memory allocation failed");

    return NULL;
  }

  while (ctx->offset < ctx->length)
  {
    char c = ctx->input[ctx->offset];

    if (c == '\"')
    {
      ++ctx->offset;
      break;
    }

    if (c == '\\')
    {
      ++ctx->offset;

      if (!cardano_handle_escape_sequence(ctx, buf))
      {
        cardano_buffer_unref(&buf);

        return NULL;
      }
    }
    else
    {
      if (!cardano_handle_utf8_sequence(ctx, buf))
      {
        cardano_buffer_unref(&buf);

        return NULL;
      }
    }
  }

  cardano_json_object_t* obj = cardano_json_object_new();

  if (obj == NULL)
  {
    cardano_buffer_unref(&buf);
    set_last_error(ctx, "Memory allocation failed");

    return NULL;
  }

  cardano_error_t result = cardano_buffer_write(buf, (const byte_t*)"\0", 1);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&buf);
    cardano_json_object_unref(&obj);
    set_last_error(ctx, cardano_error_to_string(result));

    return NULL;
  }

  obj->type   = CARDANO_JSON_OBJECT_TYPE_STRING;
  obj->string = buf;

  return obj;
}

cardano_json_object_t*
cardano_parse_number_value(cardano_json_parse_context_t* ctx)
{
  cardano_skip_whitespace(ctx);

  const char* start = &ctx->input[ctx->offset];
  const char* end   = start;

  while (
    (end < &ctx->input[ctx->length]) &&
    (isdigit((unsigned char)*end) ||
     (*end == '.') ||
     (*end == 'e') ||
     (*end == 'E') ||
     (*end == '-') ||
     (*end == '+')))
  {
    ++end;
  }

  cardano_json_object_t* obj = cardano_json_object_new();

  if (obj == NULL)
  {
    set_last_error(ctx, "Memory allocation failed");

    return NULL;
  }

  obj->type         = CARDANO_JSON_OBJECT_TYPE_NUMBER;
  obj->uint_value   = 0;
  obj->int_value    = 0;
  obj->double_value = 0.0;

  const uintptr_t start_addr  = (uintptr_t)((const void*)start);
  const uintptr_t end_addr    = (uintptr_t)((const void*)end);
  const uintptr_t actual_size = (uintptr_t)(end_addr - start_addr);

  const size_t size = ((size_t)actual_size > 64U) ? 64U : (size_t)actual_size;

  char temp[65] = { 0 };
  cardano_safe_memcpy(temp, 64, start, size);
  temp[size] = '\0';

  bool is_out_of_range = false;
  errno                = 0;
  uint64_t uint_value  = strtoull(temp, NULL, 10);

  if (errno != 0)
  {
    is_out_of_range = true;
  }

  obj->is_negative = (temp[0] == '-');
  bool is_uint     = (end != start) && !is_out_of_range && !obj->is_negative;

  if (is_uint)
  {
    obj->uint_value = uint_value;
  }

  is_out_of_range   = false;
  errno             = 0;
  int64_t int_value = strtoll(temp, NULL, 10);

  if (errno != 0)
  {
    is_out_of_range = true;
  }

  bool is_int = ((end != start) && !is_out_of_range);

  if (is_int)
  {
    obj->int_value = int_value;
  }

  is_out_of_range     = false;
  errno               = 0;
  double double_value = strtod(temp, NULL);

  if (errno != 0)
  {
    is_out_of_range = true;
  }

  bool is_double = (end != start) && !is_out_of_range;

  if (is_double)
  {
    obj->double_value = double_value;
  }

  obj->is_real = cardano_has_char('.', start, end) || cardano_has_char('e', start, end) || cardano_has_char('E', start, end);

  if (!is_uint && !is_int && !is_double)
  {
    cardano_json_object_unref(&obj);
    set_last_error(ctx, "Invalid JSON number");

    return NULL;
  }

  if (end >= start)
  {
    ctx->offset += actual_size;
  }
  else
  {
    set_last_error(ctx, "Invalid JSON number");
    return NULL;
  }

  return obj;
}

cardano_json_object_t*
cardano_parse_object_value(cardano_json_parse_context_t* ctx)
{
  if ((ctx->offset >= ctx->length) || ((char)ctx->input[ctx->offset] != (char)'{'))
  {
    set_last_error(ctx, "Invalid JSON object start");

    return NULL;
  }

  ++ctx->offset;
  ++ctx->depth;

  cardano_skip_whitespace(ctx);

  if (ctx->offset >= ctx->length)
  {
    set_last_error(ctx, "Invalid JSON object start");

    return NULL;
  }

  cardano_array_t* pairs = cardano_array_new(32);

  if (pairs == NULL)
  {
    set_last_error(ctx, "Memory allocation failed");

    return NULL;
  }

  cardano_skip_whitespace(ctx);

  if ((ctx->offset < ctx->length) && ((char)ctx->input[ctx->offset] == (char)'}'))
  {
    ++ctx->offset;

    cardano_json_object_t* obj = cardano_json_object_new();

    if (obj == NULL)
    {
      cardano_array_unref(&pairs);
      set_last_error(ctx, "Memory allocation failed");

      return NULL;
    }

    obj->type  = CARDANO_JSON_OBJECT_TYPE_OBJECT;
    obj->pairs = pairs;

    return obj;
  }

  bool end_of_object = false;
  while (!end_of_object)
  {
    if (ctx->offset >= ctx->length)
    {
      set_last_error(ctx, "Unexpected end of input");
      cardano_array_unref(&pairs);

      return NULL;
    }

    if ((char)ctx->input[ctx->offset] == (char)'}')
    {
      ++ctx->offset;
      --ctx->depth;

      end_of_object = true;
      continue;
    }

    cardano_json_object_t* key = cardano_parse_string_value(ctx);

    if (!key || (key->type != CARDANO_JSON_OBJECT_TYPE_STRING))
    {
      set_last_error(ctx, "Invalid JSON object key");
      cardano_json_object_unref(&key);
      cardano_array_unref(&pairs);

      return NULL;
    }

    cardano_skip_whitespace(ctx);

    if ((ctx->offset >= ctx->length) || ((char)ctx->input[ctx->offset] != (char)':'))
    {
      set_last_error(ctx, "Invalid JSON object key-value separator");
      cardano_json_object_unref(&key);
      cardano_array_unref(&pairs);

      return NULL;
    }

    ++ctx->offset;

    if (ctx->depth >= (size_t)LIB_CARDANO_C_MAX_JSON_DEPTH)
    {
      set_last_error(ctx, "Maximum object depth exceeded");
      cardano_json_object_unref(&key);
      cardano_array_unref(&pairs);

      return NULL;
    }

    // cppcheck-suppress misra-c2012-17.2; Reason: We need to use recursion to parse nested objects.
    cardano_json_object_t* val = cardano_parse_value(ctx);

    if (!val)
    {
      set_last_error(ctx, "Invalid JSON object value");
      cardano_json_object_unref(&key);
      cardano_array_unref(&pairs);

      return NULL;
    }

    cardano_json_kvp_t* kvp = cardano_json_kvp_new();

    if (kvp == NULL)
    {
      cardano_json_object_unref(&key);
      cardano_json_object_unref(&val);
      cardano_array_unref(&pairs);

      set_last_error(ctx, "Memory allocation failed");

      return NULL;
    }

    cardano_buffer_ref(key->string);

    kvp->key = key->string;

    cardano_json_object_unref(&key);

    kvp->value            = val;
    const size_t new_size = cardano_array_push(pairs, (cardano_object_t*)((void*)kvp));
    CARDANO_UNUSED(new_size);

    cardano_object_unref((cardano_object_t**)((void*)&kvp));

    cardano_skip_whitespace(ctx);

    if ((ctx->offset < ctx->length) && ((char)ctx->input[ctx->offset] == (char)','))
    {
      ++ctx->offset;
      cardano_skip_whitespace(ctx);

      continue;
    }
    else if ((ctx->offset < ctx->length) && ((char)ctx->input[ctx->offset] == (char)'}'))
    {
      ++ctx->offset;
      --ctx->depth;

      end_of_object = true;
      continue;
    }
    else
    {
      set_last_error(ctx, "Expected ',' or '}'");
      cardano_array_unref(&pairs);

      return NULL;
    }
  }

  cardano_json_object_t* obj = cardano_json_object_new();

  if (obj == NULL)
  {
    cardano_array_unref(&pairs);
    set_last_error(ctx, "Memory allocation failed");

    return NULL;
  }

  obj->type  = CARDANO_JSON_OBJECT_TYPE_OBJECT;
  obj->pairs = pairs;

  return obj;
}

cardano_json_object_t*
cardano_parse_array_value(cardano_json_parse_context_t* ctx)
{
  cardano_skip_whitespace(ctx);

  if ((ctx->offset >= ctx->length) || ((char)ctx->input[ctx->offset] != (char)'['))
  {
    set_last_error(ctx, "Invalid JSON array start");

    return NULL;
  }

  ++ctx->depth;
  ++ctx->offset;

  if (ctx->depth >= (size_t)LIB_CARDANO_C_MAX_JSON_DEPTH)
  {
    set_last_error(ctx, "Maximum object depth exceeded");

    return NULL;
  }

  cardano_array_t* arr = cardano_array_new(32);

  if (arr == NULL)
  {
    set_last_error(ctx, "Memory allocation failed");

    return NULL;
  }

  cardano_skip_whitespace(ctx);

  if ((ctx->offset < ctx->length) && ((char)ctx->input[ctx->offset] == (char)']'))
  {
    ctx->offset++;
    cardano_json_object_t* obj = cardano_json_object_new();

    if (obj == NULL)
    {
      cardano_array_unref(&arr);
      set_last_error(ctx, "Memory allocation failed");

      return NULL;
    }

    obj->type  = CARDANO_JSON_OBJECT_TYPE_ARRAY;
    obj->array = arr;

    return obj;
  }

  bool end_of_array = false;
  while (!end_of_array)
  {
    if (ctx->offset >= ctx->length)
    {
      set_last_error(ctx, "Unexpected end of input");
      cardano_array_unref(&arr);

      return NULL;
    }

    if (((char)ctx->input[ctx->offset] == (char)']'))
    {
      ++ctx->offset;
      --ctx->depth;

      end_of_array = true;
      continue;
    }

    // cppcheck-suppress misra-c2012-17.2; Reason: We need to use recursion to parse nested objects.
    cardano_json_object_t* val = cardano_parse_value(ctx);

    if (!val)
    {
      set_last_error(ctx, "Invalid JSON array value");
      cardano_array_unref(&arr);

      return NULL;
    }

    size_t size = cardano_array_push(arr, ((cardano_object_t*)(void*)val));
    cardano_json_object_unref(&val);

    CARDANO_UNUSED(size);

    cardano_skip_whitespace(ctx);

    if ((ctx->offset < ctx->length) && ((char)ctx->input[ctx->offset] == (char)','))
    {
      ++ctx->offset;
      cardano_skip_whitespace(ctx);

      continue;
    }
    else if ((ctx->offset < ctx->length) && ((char)ctx->input[ctx->offset] == (char)']'))
    {
      ++ctx->offset;
      --ctx->depth;

      end_of_array = true;
      continue;
    }
    else
    {
      set_last_error(ctx, "Expected ',' or ']'");
      cardano_array_unref(&arr);

      return NULL;
    }
  }

  cardano_json_object_t* obj = cardano_json_object_new();

  if (obj == NULL)
  {
    cardano_array_unref(&arr);
    set_last_error(ctx, "Memory allocation failed");

    return NULL;
  }

  obj->type  = CARDANO_JSON_OBJECT_TYPE_ARRAY;
  obj->array = arr;

  return obj;
}

cardano_json_object_t*
cardano_parse_literal(
  cardano_json_parse_context_t*    ctx,
  const char*                      literal,
  const size_t                     literal_size,
  const cardano_json_object_type_t type)
{
  if (((ctx->offset + literal_size) <= ctx->length) && (strncmp(&ctx->input[ctx->offset], literal, literal_size) == 0))
  {
    ctx->offset                += literal_size;
    cardano_json_object_t* obj = cardano_json_object_new();

    if (obj == NULL)
    {
      set_last_error(ctx, "Memory allocation failed");
      return NULL;
    }

    obj->type = type;

    if (type == CARDANO_JSON_OBJECT_TYPE_BOOLEAN)
    {
      obj->bool_value = (strcmp(literal, "true") == 0);
    }

    return obj;
  }

  return NULL;
}

cardano_json_object_t*
cardano_parse_value(cardano_json_parse_context_t* ctx)
{
  cardano_skip_whitespace(ctx);

  if (ctx->offset >= ctx->length)
  {
    set_last_error(ctx, "Unexpected end of input");
    return NULL;
  }

  char c = ctx->input[ctx->offset];

  if (c == '{')
  {
    // cppcheck-suppress misra-c2012-17.2; Reason: We need to use recursion to parse nested objects.
    return cardano_parse_object_value(ctx);
  }

  if (c == '[')
  {
    // cppcheck-suppress misra-c2012-17.2; Reason: We need to use recursion to parse nested objects.
    return cardano_parse_array_value(ctx);
  }

  if (c == '\"')
  {
    return cardano_parse_string_value(ctx);
  }

  if (c == 't')
  {
    return cardano_parse_literal(ctx, "true", 4, CARDANO_JSON_OBJECT_TYPE_BOOLEAN);
  }

  if (c == 'f')
  {
    return cardano_parse_literal(ctx, "false", 5, CARDANO_JSON_OBJECT_TYPE_BOOLEAN);
  }

  if (c == 'n')
  {
    return cardano_parse_literal(ctx, "null", 4, CARDANO_JSON_OBJECT_TYPE_NULL);
  }

  if ((c == '-') || ((c >= '0') && (c <= '9')))
  {
    return cardano_parse_number_value(ctx);
  }

  return NULL;
}