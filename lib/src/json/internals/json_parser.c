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
 * \param[in]     error The parsing error code; allocation failures remain sticky.
 * \param[in]     message A null-terminated string containing the error message. This parameter
 *                        must not be \c NULL. If the message exceeds the buffer size, it will
 *                        be truncated to fit.
 */
static void
set_last_error(cardano_json_parse_context_t* ctx, const cardano_error_t error, const char* message)
{
  if (ctx->error != CARDANO_ERROR_MEMORY_ALLOCATION_FAILED)
  {
    ctx->error = error;
  }

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
cardano_handle_utf8_sequence(cardano_json_parse_context_t* ctx, cardano_buffer_t* buf)
{
  byte_t first_byte = (byte_t)ctx->input[ctx->offset];
  size_t seq_len    = cardano_utf8_sequence_length(first_byte);

  if ((seq_len == 0U) || ((ctx->offset + seq_len) > ctx->length))
  {
    set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid UTF-8 sequence");

    return false;
  }

  for (size_t i = 1U; i < seq_len; ++i)
  {
    if ((ctx->input[ctx->offset + i] & 0xC0) != 0x80)
    {
      set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid UTF-8 continuation byte");

      return false;
    }
  }

  if ((seq_len > 1U) && ((first_byte < 0xc2U) || (first_byte > 0xf4U)))
  {
    set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid UTF-8 leading byte");
    return false;
  }

  if (seq_len >= 3U)
  {
    const byte_t second = (byte_t)ctx->input[ctx->offset + 1U];

    if (((first_byte == 0xe0U) && (second < 0xa0U)) || ((first_byte == 0xedU) && (second >= 0xa0U)) || ((first_byte == 0xf0U) && (second < 0x90U)) || ((first_byte == 0xf4U) && (second >= 0x90U)))
    {
      set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid UTF-8 codepoint");
      return false;
    }
  }

  cardano_error_t result = cardano_buffer_write(buf, (const byte_t*)&ctx->input[ctx->offset], seq_len);
  if (result != CARDANO_SUCCESS)
  {
    set_last_error(ctx, result, cardano_error_to_string(result));

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
    set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Not enough characters for Unicode escape sequence");
    return false;
  }

  for (size_t i = 0; i < 4U; ++i)
  {
    if (!isxdigit((unsigned char)ctx->input[ctx->offset + i]))
    {
      set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid character in Unicode escape sequence");
      return false;
    }
  }

  int32_t high = cardano_parse_unicode_escape(&ctx->input[ctx->offset]);

  if (high < 0)
  {
    set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid Unicode escape sequence");
    return false;
  }

  size_t consumed = UNICODE_BASIC_ESCAPE_LEN;

  if ((high >= 0xdc00) && (high <= 0xdfff))
  {
    set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Unpaired low surrogate");
    return false;
  }

  if ((high >= 0xd800) && (high <= 0xdbff))
  {
    if (((ctx->offset + 10U) > ctx->length) || (ctx->input[ctx->offset + 4U] != '\\') || (ctx->input[ctx->offset + 5U] != 'u'))
    {
      set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Missing low surrogate");
      return false;
    }

    for (size_t i = 6U; i < 10U; ++i)
    {
      if (!isxdigit((unsigned char)ctx->input[ctx->offset + i]))
      {
        set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid character in second Unicode escape sequence");
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
        set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Failed to encode surrogate pair as UTF-8");
        return false;
      }

      cardano_error_t result = cardano_buffer_write(buf, (byte_t*)&utf8_buf[0], bytes_written);

      if (result != CARDANO_SUCCESS)
      {
        set_last_error(ctx, result, cardano_error_to_string(result));
        return false;
      }

      consumed = UNICODE_SURROGATE_PAIR_LEN;
    }
    else
    {
      set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid surrogate pair in Unicode escape sequence");
      return false;
    }
  }
  else
  {
    char   utf8_buf[4]   = { 0 };
    size_t bytes_written = cardano_encode_utf8(high, utf8_buf);

    if (bytes_written == 0U)
    {
      set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Failed to encode Unicode character as UTF-8");
      return false;
    }

    cardano_error_t result = cardano_buffer_write(buf, (byte_t*)&utf8_buf[0], bytes_written);

    if (result != CARDANO_SUCCESS)
    {
      set_last_error(ctx, result, cardano_error_to_string(result));
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
    set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Unexpected end of input in escape sequence");
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
      set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid escape sequence");
      return false;
  }

  if (result != CARDANO_SUCCESS)
  {
    set_last_error(ctx, result, cardano_error_to_string(result));
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
    set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid JSON string start");

    return NULL;
  }

  ++ctx->offset;

  cardano_buffer_t* buf = cardano_buffer_new(128);

  if (buf == NULL)
  {
    set_last_error(ctx, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED, "Memory allocation failed");

    return NULL;
  }

  bool closed = false;

  while (ctx->offset < ctx->length)
  {
    char c = ctx->input[ctx->offset];

    if (c == '\"')
    {
      ++ctx->offset;
      closed = true;
      break;
    }

    if ((unsigned char)c < 0x20U)
    {
      cardano_buffer_unref(&buf);
      set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Unescaped control character in JSON string");
      return NULL;
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

  if (!closed)
  {
    cardano_buffer_unref(&buf);
    set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Unterminated JSON string");
    return NULL;
  }

  cardano_json_object_t* obj = cardano_json_object_new();

  if (obj == NULL)
  {
    cardano_buffer_unref(&buf);
    set_last_error(ctx, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED, "Memory allocation failed");

    return NULL;
  }

  cardano_error_t result = cardano_buffer_write(buf, (const byte_t*)"\0", 1);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&buf);
    cardano_json_object_unref(&obj);
    set_last_error(ctx, result, cardano_error_to_string(result));

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

  const size_t start = ctx->offset;
  size_t       end   = start;

  if ((end < ctx->length) && (ctx->input[end] == '-'))
  {
    ++end;
  }

  if ((end >= ctx->length) || (ctx->input[end] < '0') || (ctx->input[end] > '9'))
  {
    set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid JSON number");
    return NULL;
  }

  if (ctx->input[end] == '0')
  {
    ++end;
  }
  else
  {
    while ((end < ctx->length) && (ctx->input[end] >= '0') && (ctx->input[end] <= '9'))
    {
      ++end;
    }
  }

  bool is_real = false;

  if ((end < ctx->length) && (ctx->input[end] == '.'))
  {
    is_real = true;
    ++end;
    const size_t fraction_start = end;

    while ((end < ctx->length) && (ctx->input[end] >= '0') && (ctx->input[end] <= '9'))
    {
      ++end;
    }

    if (end == fraction_start)
    {
      set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid JSON fraction");
      return NULL;
    }
  }

  if ((end < ctx->length) && ((ctx->input[end] == 'e') || (ctx->input[end] == 'E')))
  {
    is_real = true;
    ++end;

    if ((end < ctx->length) && ((ctx->input[end] == '+') || (ctx->input[end] == '-')))
    {
      ++end;
    }

    const size_t exponent_start = end;

    while ((end < ctx->length) && (ctx->input[end] >= '0') && (ctx->input[end] <= '9'))
    {
      ++end;
    }

    if (end == exponent_start)
    {
      set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid JSON exponent");
      return NULL;
    }
  }

  if ((end < ctx->length) && (ctx->input[end] != ',') && (ctx->input[end] != ']') && (ctx->input[end] != '}') && (ctx->input[end] != ' ') && (ctx->input[end] != '\t') && (ctx->input[end] != '\r') && (ctx->input[end] != '\n'))
  {
    set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid JSON number delimiter");
    return NULL;
  }

  cardano_json_object_t* obj = cardano_json_object_new();

  if (obj == NULL)
  {
    set_last_error(ctx, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED, "Memory allocation failed");
    return NULL;
  }

  obj->type        = CARDANO_JSON_OBJECT_TYPE_NUMBER;
  obj->is_negative = ctx->input[start] == '-';
  obj->is_real     = is_real;
  obj->string      = cardano_buffer_new_from((const byte_t*)&ctx->input[start], end - start);

  if (obj->string == NULL)
  {
    cardano_json_object_unref(&obj);
    set_last_error(ctx, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED, "Memory allocation failed");
    return NULL;
  }

  cardano_error_t result = cardano_buffer_write(obj->string, (const byte_t*)"\0", 1U);

  if (result != CARDANO_SUCCESS)
  {
    cardano_json_object_unref(&obj);
    set_last_error(ctx, result, cardano_error_to_string(result));
    return NULL;
  }

  ctx->offset = end;

  return obj;
}

cardano_json_object_t*
cardano_parse_object_value(cardano_json_parse_context_t* ctx)
{
  if ((ctx->offset >= ctx->length) || ((char)ctx->input[ctx->offset] != (char)'{'))
  {
    set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid JSON object start");

    return NULL;
  }

  ++ctx->offset;
  ++ctx->depth;

  if (ctx->depth >= (size_t)LIB_CARDANO_C_MAX_JSON_DEPTH)
  {
    set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Maximum object depth exceeded");
    return NULL;
  }

  cardano_skip_whitespace(ctx);

  if (ctx->offset >= ctx->length)
  {
    set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid JSON object start");

    return NULL;
  }

  cardano_array_t* pairs = cardano_array_new(32);

  if (pairs == NULL)
  {
    set_last_error(ctx, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED, "Memory allocation failed");

    return NULL;
  }

  cardano_skip_whitespace(ctx);

  if ((ctx->offset < ctx->length) && ((char)ctx->input[ctx->offset] == (char)'}'))
  {
    ++ctx->offset;
    --ctx->depth;

    cardano_json_object_t* obj = cardano_json_object_new();

    if (obj == NULL)
    {
      cardano_array_unref(&pairs);
      set_last_error(ctx, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED, "Memory allocation failed");

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
      set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Unexpected end of input");
      cardano_array_unref(&pairs);

      return NULL;
    }

    cardano_json_object_t* key = cardano_parse_string_value(ctx);

    if (!key || (key->type != CARDANO_JSON_OBJECT_TYPE_STRING))
    {
      set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid JSON object key");
      cardano_json_object_unref(&key);
      cardano_array_unref(&pairs);

      return NULL;
    }

    cardano_skip_whitespace(ctx);

    if ((ctx->offset >= ctx->length) || ((char)ctx->input[ctx->offset] != (char)':'))
    {
      set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid JSON object key-value separator");
      cardano_json_object_unref(&key);
      cardano_array_unref(&pairs);

      return NULL;
    }

    ++ctx->offset;

    if (ctx->depth >= (size_t)LIB_CARDANO_C_MAX_JSON_DEPTH)
    {
      set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Maximum object depth exceeded");
      cardano_json_object_unref(&key);
      cardano_array_unref(&pairs);

      return NULL;
    }

    // cppcheck-suppress misra-c2012-17.2; Reason: We need to use recursion to parse nested objects.
    cardano_json_object_t* val = cardano_parse_value(ctx);

    if (!val)
    {
      set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid JSON object value");
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

      set_last_error(ctx, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED, "Memory allocation failed");

      return NULL;
    }

    cardano_buffer_ref(key->string);

    kvp->key = key->string;

    cardano_json_object_unref(&key);

    kvp->value = val;

    const size_t old_size = cardano_array_get_size(pairs);
    const size_t new_size = cardano_array_push(pairs, (cardano_object_t*)((void*)kvp));

    cardano_object_unref((cardano_object_t**)((void*)&kvp));

    if (new_size != (old_size + 1U))
    {
      cardano_array_unref(&pairs);

      set_last_error(ctx, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED, "Memory allocation failed");

      return NULL;
    }

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
      set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Expected ',' or '}'");
      cardano_array_unref(&pairs);

      return NULL;
    }
  }

  cardano_json_object_t* obj = cardano_json_object_new();

  if (obj == NULL)
  {
    cardano_array_unref(&pairs);
    set_last_error(ctx, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED, "Memory allocation failed");

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
    set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid JSON array start");

    return NULL;
  }

  ++ctx->depth;
  ++ctx->offset;

  if (ctx->depth >= (size_t)LIB_CARDANO_C_MAX_JSON_DEPTH)
  {
    set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Maximum object depth exceeded");

    return NULL;
  }

  cardano_array_t* arr = cardano_array_new(32);

  if (arr == NULL)
  {
    set_last_error(ctx, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED, "Memory allocation failed");

    return NULL;
  }

  cardano_skip_whitespace(ctx);

  if ((ctx->offset < ctx->length) && ((char)ctx->input[ctx->offset] == (char)']'))
  {
    ctx->offset++;
    --ctx->depth;
    cardano_json_object_t* obj = cardano_json_object_new();

    if (obj == NULL)
    {
      cardano_array_unref(&arr);
      set_last_error(ctx, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED, "Memory allocation failed");

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
      set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Unexpected end of input");
      cardano_array_unref(&arr);

      return NULL;
    }

    // cppcheck-suppress misra-c2012-17.2; Reason: We need to use recursion to parse nested objects.
    cardano_json_object_t* val = cardano_parse_value(ctx);

    if (!val)
    {
      set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Invalid JSON array value");
      cardano_array_unref(&arr);

      return NULL;
    }

    const size_t old_size = cardano_array_get_size(arr);
    const size_t new_size = cardano_array_push(arr, ((cardano_object_t*)(void*)val));

    cardano_json_object_unref(&val);

    if (new_size != (old_size + 1U))
    {
      cardano_array_unref(&arr);

      set_last_error(ctx, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED, "Memory allocation failed");

      return NULL;
    }

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
      set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Expected ',' or ']'");
      cardano_array_unref(&arr);

      return NULL;
    }
  }

  cardano_json_object_t* obj = cardano_json_object_new();

  if (obj == NULL)
  {
    cardano_array_unref(&arr);
    set_last_error(ctx, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED, "Memory allocation failed");

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
      set_last_error(ctx, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED, "Memory allocation failed");
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
    set_last_error(ctx, CARDANO_ERROR_INVALID_JSON, "Unexpected end of input");
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