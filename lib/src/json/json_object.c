/**
 * \file json_object.c
 *
 * \author angel.castillo
 * \date   Dec 06, 2024
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

#include <cardano/buffer.h>
#include <cardano/json/json_object.h>
#include <cardano/json/json_object_type.h>
#include <cardano/json/json_writer.h>
#include <cardano/object.h>

#include "../allocators.h"

#include <string.h>

#include "../collections/array.h"
#include "../string_safe.h"
#include "./internals/json_object_common.h"
#include "./internals/json_parser.h"

/* DECLARATIONS **************************************************************/

cardano_json_object_t*
cardano_json_object_parse(const char* json, const size_t size)
{
  if ((json == NULL) || (size == 0U))
  {
    return NULL;
  }

  cardano_json_parse_context_t ctx;

  ctx.input  = json;
  ctx.length = size;
  ctx.offset = 0U;
  ctx.depth  = 0U;

  cardano_json_object_t* root = cardano_parse_value(&ctx);

  if (root == NULL)
  {
    return NULL;
  }

  cardano_skip_whitespace(&ctx);

  if (ctx.offset < ctx.length)
  {
    cardano_json_object_unref(&root);

    return NULL;
  }

  return root;
}

const char*
cardano_json_object_to_json_string(
  cardano_json_object_t*      json_object,
  const cardano_json_format_t format,
  size_t*                     length)
{
  if (json_object == NULL)
  {
    return NULL;
  }

  if (json_object->json_string != NULL)
  {
    if (length != NULL)
    {
      *length = json_object->json_string_length;
    }

    return json_object->json_string;
  }

  cardano_json_writer_t* writer = cardano_json_writer_new(format);

  if (writer == NULL)
  {
    return NULL;
  }

  cardano_json_writer_write_object(writer, json_object);

  const size_t encoded_size = cardano_json_writer_get_encoded_size(writer);

  if (encoded_size == 0U)
  {
    cardano_json_writer_unref(&writer);
    return NULL;
  }

  char* buffer = _cardano_malloc(encoded_size);
  CARDANO_UNUSED(memset(buffer, 0, encoded_size));

  cardano_error_t result = cardano_json_writer_encode(writer, buffer, encoded_size);

  cardano_json_writer_unref(&writer);

  if (result != CARDANO_SUCCESS)
  {
    _cardano_free(buffer);

    return NULL;
  }

  json_object->json_string        = buffer;
  json_object->json_string_length = encoded_size;

  if (length != NULL)
  {
    *length = encoded_size;
  }

  return buffer;
}

cardano_json_object_type_t
cardano_json_object_get_type(
  const cardano_json_object_t* json_object)
{
  if (json_object == NULL)
  {
    return CARDANO_JSON_OBJECT_TYPE_NULL;
  }

  return json_object->type;
}

bool
cardano_json_object_has_property(
  const cardano_json_object_t* json_object,
  const char*                  key,
  const size_t                 size)
{
  if ((json_object == NULL) || (key == NULL) || (size == 0U))
  {
    return false;
  }

  if (json_object->type != CARDANO_JSON_OBJECT_TYPE_OBJECT)
  {
    return false;
  }

  for (size_t i = 0U; i < cardano_array_get_size(json_object->pairs); ++i)
  {
    cardano_json_kvp_t* kvp = (cardano_json_kvp_t*)((void*)cardano_array_get(json_object->pairs, i));
    cardano_object_unref((cardano_object_t**)((void*)&kvp));

    const size_t key_size = cardano_buffer_get_size(kvp->key) - 1U;

    if (key_size != size)
    {
      continue;
    }

    if (strncmp((const char*)cardano_buffer_get_data(kvp->key), key, key_size) == 0)
    {
      return true;
    }
  }

  return false;
}

size_t
cardano_json_object_get_property_count(
  const cardano_json_object_t* json_object)
{
  if ((json_object == NULL) || (json_object->type != CARDANO_JSON_OBJECT_TYPE_OBJECT))
  {
    return 0U;
  }

  return cardano_array_get_size(json_object->pairs);
}

const char*
cardano_json_object_get_key_at(
  const cardano_json_object_t* json_object,
  const size_t                 index,
  size_t*                      key_length)
{
  if ((json_object == NULL) || (json_object->type != CARDANO_JSON_OBJECT_TYPE_OBJECT))
  {
    return NULL;
  }

  cardano_json_kvp_t* kvp = (cardano_json_kvp_t*)((void*)cardano_array_get(json_object->pairs, index));
  cardano_object_unref((cardano_object_t**)((void*)&kvp));

  if (key_length != NULL)
  {
    *key_length = cardano_buffer_get_size(kvp->key) - 1U;
  }

  return (const char*)((const void*)cardano_buffer_get_data(kvp->key));
}

cardano_json_object_t*
cardano_json_object_get_value_at(
  const cardano_json_object_t* json_object,
  const size_t                 index)
{
  if ((json_object == NULL) || (json_object->type != CARDANO_JSON_OBJECT_TYPE_OBJECT))
  {
    return NULL;
  }

  if (index >= cardano_array_get_size(json_object->pairs))
  {
    return NULL;
  }

  cardano_json_kvp_t* kvp = (cardano_json_kvp_t*)((void*)cardano_array_get(json_object->pairs, index));
  cardano_object_unref((cardano_object_t**)((void*)&kvp));

  cardano_object_ref((cardano_object_t*)((void*)kvp->value));

  return kvp->value;
}

cardano_json_object_t*
cardano_json_object_get_value_at_ex(
  const cardano_json_object_t* json_object,
  const size_t                 index)
{
  cardano_json_object_t* value = cardano_json_object_get_value_at(json_object, index);
  cardano_json_object_unref(&value);

  return value;
}

bool
cardano_json_object_get(
  const cardano_json_object_t* json_object,
  const char*                  key,
  const size_t                 size,
  cardano_json_object_t**      value)
{
  if ((json_object == NULL) || (key == NULL) || (size == 0U) || (value == NULL))
  {
    return false;
  }

  if (json_object->type != CARDANO_JSON_OBJECT_TYPE_OBJECT)
  {
    return false;
  }

  for (size_t i = 0U; i < cardano_array_get_size(json_object->pairs); ++i)
  {
    cardano_json_kvp_t* kvp = (cardano_json_kvp_t*)((void*)cardano_array_get(json_object->pairs, i));
    cardano_object_unref((cardano_object_t**)((void*)&kvp));

    const size_t key_size = cardano_buffer_get_size(kvp->key) - 1U;

    if (key_size != size)
    {
      continue;
    }

    if (strncmp((const char*)cardano_buffer_get_data(kvp->key), key, key_size) == 0)
    {
      cardano_json_object_ref(kvp->value);

      *value = kvp->value;

      return true;
    }
  }

  return false;
}

bool
cardano_json_object_get_ex(
  const cardano_json_object_t* json_object,
  const char*                  key,
  size_t                       size,
  cardano_json_object_t**      value)
{
  if (cardano_json_object_get(json_object, key, size, value))
  {
    cardano_json_object_unref(value);
    return true;
  }

  return false;
}

size_t
cardano_json_object_array_get_length(
  const cardano_json_object_t* json_object)
{
  if ((json_object == NULL) || (json_object->type != CARDANO_JSON_OBJECT_TYPE_ARRAY))
  {
    return 0U;
  }

  return cardano_array_get_size(json_object->array);
}

cardano_json_object_t*
cardano_json_object_array_get(
  const cardano_json_object_t* json_array,
  const size_t                 index)
{
  if ((json_array == NULL) || (json_array->type != CARDANO_JSON_OBJECT_TYPE_ARRAY))
  {
    return NULL;
  }

  if (index >= cardano_array_get_size(json_array->array))
  {
    return NULL;
  }

  cardano_json_object_t* element = (cardano_json_object_t*)((void*)cardano_array_get(json_array->array, index));

  return element;
}

cardano_json_object_t*
cardano_json_object_array_get_ex(
  const cardano_json_object_t* json_array,
  const size_t                 index)
{
  cardano_json_object_t* element = cardano_json_object_array_get(json_array, index);
  cardano_json_object_unref(&element);

  return element;
}

const char*
cardano_json_object_get_string(
  const cardano_json_object_t* json_object,
  size_t*                      string_length)
{
  if ((json_object == NULL) || (json_object->type != CARDANO_JSON_OBJECT_TYPE_STRING))
  {
    return NULL;
  }

  if (string_length != NULL)
  {
    *string_length = cardano_buffer_get_size(json_object->string);
  }

  return (const char*)((const void*)cardano_buffer_get_data(json_object->string));
}

bool
cardano_json_object_get_is_negative_number(const cardano_json_object_t* json_object)
{
  if ((json_object == NULL) || (json_object->type != CARDANO_JSON_OBJECT_TYPE_NUMBER))
  {
    return false;
  }

  return json_object->is_negative;
}

bool
cardano_json_object_get_is_real_number(const cardano_json_object_t* json_object)
{
  if ((json_object == NULL) || (json_object->type != CARDANO_JSON_OBJECT_TYPE_NUMBER))
  {
    return false;
  }

  return json_object->is_real;
}

cardano_error_t
cardano_json_object_get_uint(
  const cardano_json_object_t* json_object,
  uint64_t*                    value)
{
  if ((json_object == NULL) || ((json_object->type != CARDANO_JSON_OBJECT_TYPE_NUMBER) && (json_object->type != CARDANO_JSON_OBJECT_TYPE_STRING)))
  {
    return CARDANO_ERROR_JSON_TYPE_MISMATCH;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (json_object->type == CARDANO_JSON_OBJECT_TYPE_STRING)
  {
    return cardano_safe_string_to_uint64((const char*)cardano_buffer_get_data(json_object->string), cardano_buffer_get_size(json_object->string) - 1U, value);
  }

  *value = json_object->uint_value;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_json_object_get_signed_int(
  const cardano_json_object_t* json_object,
  int64_t*                     value)
{
  if ((json_object == NULL) || ((json_object->type != CARDANO_JSON_OBJECT_TYPE_NUMBER) && (json_object->type != CARDANO_JSON_OBJECT_TYPE_STRING)))
  {
    return CARDANO_ERROR_JSON_TYPE_MISMATCH;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (json_object->type == CARDANO_JSON_OBJECT_TYPE_STRING)
  {
    return cardano_safe_string_to_int64((const char*)cardano_buffer_get_data(json_object->string), cardano_buffer_get_size(json_object->string) - 1U, value);
  }

  *value = json_object->int_value;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_json_object_get_double(
  const cardano_json_object_t* json_object,
  double*                      value)
{
  if ((json_object == NULL) || ((json_object->type != CARDANO_JSON_OBJECT_TYPE_NUMBER) && (json_object->type != CARDANO_JSON_OBJECT_TYPE_STRING)))
  {
    return CARDANO_ERROR_JSON_TYPE_MISMATCH;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (json_object->type == CARDANO_JSON_OBJECT_TYPE_STRING)
  {
    return cardano_safe_string_to_double((const char*)cardano_buffer_get_data(json_object->string), cardano_buffer_get_size(json_object->string) - 1U, value);
  }

  *value = json_object->double_value;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_json_object_get_boolean(
  const cardano_json_object_t* json_object,
  bool*                        value)
{
  if ((json_object == NULL) || (json_object->type != CARDANO_JSON_OBJECT_TYPE_BOOLEAN))
  {
    return CARDANO_ERROR_JSON_TYPE_MISMATCH;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *value = json_object->bool_value;

  return CARDANO_SUCCESS;
}

void
cardano_json_object_unref(cardano_json_object_t** json_object)
{
  if ((json_object == NULL) || (*json_object == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*json_object)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *json_object = NULL;
    return;
  }
}

void
cardano_json_object_ref(cardano_json_object_t* json_object)
{
  if (json_object == NULL)
  {
    return;
  }

  cardano_object_ref(&json_object->base);
}

size_t
cardano_json_object_refcount(const cardano_json_object_t* json_object)
{
  if (json_object == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&json_object->base);
}

void
cardano_json_object_set_last_error(cardano_json_object_t* writer, const char* message)
{
  cardano_object_set_last_error(&writer->base, message);
}

const char*
cardano_json_object_get_last_error(const cardano_json_object_t* writer)
{
  return cardano_object_get_last_error(&writer->base);
}
