/**
 * \file json_writer.c
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
#include <cardano/json/json_writer.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../string_safe.h"
#include "internals/json_object_serialization.h"
#include "internals/json_writer_common.h"

#include <assert.h>
#include <cardano/json/json_format.h>
#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a JSON writer object.
 *
 * This function is responsible for properly deallocating a JSON writer object (`cardano_json_writer_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the JSON writer object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_json_writer_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the JSON writer
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_json_writer_deallocate(void* object)
{
  assert(object != NULL);

  cardano_json_writer_t* json_writer = (cardano_json_writer_t*)object;

  if (json_writer->buffer != NULL)
  {
    cardano_buffer_unref(&json_writer->buffer);
  }

  _cardano_free(json_writer);
}

/**
 * \brief Pushes a new JSON context onto the writer's context stack.
 *
 * This function adds a new JSON context (e.g., an object or array) to the writer's stack,
 * updating the depth and initializing the new context's state. It ensures the writer does
 * not exceed the maximum allowable nesting depth.
 *
 * \param[in,out] writer  A pointer to the JSON writer instance whose context stack is to be modified. Must not be NULL.
 * \param[in]     context The JSON context to be added to the stack. Valid values include:
 *                        - \c CARDANO_JSON_CONTEXT_OBJECT: Represents a JSON object.
 *                        - \c CARDANO_JSON_CONTEXT_ARRAY: Represents a JSON array.
 */
static void
push_context(cardano_json_writer_t* writer, const cardano_json_context_t context)
{
  assert(writer != NULL);
  assert(writer->last_error == CARDANO_SUCCESS);

  if (writer->depth >= ((size_t)LIB_CARDANO_C_MAX_JSON_DEPTH - 1U))
  {
    writer->last_error = CARDANO_ERROR_ENCODING;
    cardano_object_set_last_error(&writer->base, "JSON nesting depth exceeded maximum allowed.");
    return;
  }

  cardano_json_stack_frame_t* current_frame = &writer->current_frame[writer->depth];

  if ((current_frame->context == CARDANO_JSON_CONTEXT_OBJECT) && (!current_frame->expect_value))
  {
    writer->last_error = CARDANO_ERROR_ENCODING;
    cardano_object_set_last_error(&writer->base, "Cannot push a new context without writing a property value.");
    return;
  }

  const bool expect_value = (context == CARDANO_JSON_CONTEXT_ARRAY);

  ++writer->depth;

  writer->current_frame[writer->depth] = (cardano_json_stack_frame_t) {
    .context      = context,
    .item_count   = 0,
    .expect_value = expect_value
  };
}

/**
 * \brief Pops the current JSON context from the writer's context stack.
 *
 * This function removes the topmost JSON context from the writer's stack,
 * effectively signaling the end of the current JSON scope (such as an object
 * or an array). It also adjusts the writer's depth to reflect the new active
 * context. If the stack is already at the root context, an error is recorded.
 *
 * \param[in,out] writer A pointer to the JSON writer instance whose context
 *                       stack is to be modified. Must not be NULL.
 */
static void
pop_context(cardano_json_writer_t* writer)
{
  assert(writer != NULL);
  assert(writer->depth > 0U);

  --writer->depth;
}

/**
 * \brief Writes indentation based on the current depth.
 *
 * \param[in] writer The JSON writer instance.
 * \return CARDANO_SUCCESS on success, or an appropriate error code on failure.
 */
static cardano_error_t
write_indentation(cardano_json_writer_t* writer)
{
  if (writer->depth == 0U)
  {
    return CARDANO_SUCCESS;
  }

  const size_t indent_size = (writer->depth) * 2U;

  // cppcheck-suppress misra-c2012-18.8; Reason: LIB_CARDANO_C_MAX_JSON_DEPTH is injected by CMake and is a constant.
  static char buffer[(LIB_CARDANO_C_MAX_JSON_DEPTH * 2) + 1] = { 0 };

  CARDANO_UNUSED(memset((void*)buffer, ' ', indent_size));

  return cardano_buffer_write(writer->buffer, (byte_t*)buffer, indent_size);
}

/**
 * \brief Get the escape character of a non-printable.
 * \param ch Character source.
 *
 * \return The escape character or null character if error.
 * */
static int32_t
escape(const int32_t ch)
{
  static struct
  {
      char code;
      char ch;
  } const pair[] = {
    { '\"', '\"' },
    { '\\', '\\' },
    { 'b', '\b' },
    { 'f', '\f' },
    { 'n', '\n' },
    { 'r', '\r' },
    { 't', '\t' },
  };

  for (size_t i = 0U; i < (sizeof(pair) / sizeof(*pair)); ++i)
  {
    if (ch == pair[i].ch)
    {
      return pair[i].code;
    }
  }

  return '\0';
}

/* DECLARATIONS **************************************************************/

cardano_json_writer_t*
cardano_json_writer_new(const cardano_json_format_t format)
{
  cardano_json_writer_t* obj = (cardano_json_writer_t*)_cardano_malloc(sizeof(cardano_json_writer_t));

  if (obj == NULL)
  {
    return NULL;
  }

  obj->base.ref_count     = 1;
  obj->base.deallocator   = cardano_json_writer_deallocate;
  obj->base.last_error[0] = '\0';
  obj->buffer             = cardano_buffer_new(128);
  obj->last_error         = CARDANO_SUCCESS;
  obj->depth              = 0;
  obj->format             = format;
  obj->current_frame[0]   = (cardano_json_stack_frame_t) {
      .context      = CARDANO_JSON_CONTEXT_ROOT,
      .item_count   = 0,
      .expect_value = false
  };

  if (obj->buffer == NULL)
  {
    _cardano_free(obj);
    return NULL;
  }

  return obj;
}

void
cardano_json_writer_write_property_name(
  cardano_json_writer_t* writer,
  const char*            name,
  const size_t           name_size)
{
  if ((writer == NULL) || (writer->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if (name == NULL)
  {
    writer->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    cardano_object_set_last_error(&writer->base, "Property name cannot be NULL or empty.");
    return;
  }

  if ((writer->depth == 0U) || (writer->current_frame[writer->depth].context != CARDANO_JSON_CONTEXT_OBJECT))
  {
    writer->last_error = CARDANO_ERROR_ENCODING;
    cardano_object_set_last_error(&writer->base, "Property name can only be written within an object context.");
    return;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  if (writer->current_frame[writer->depth].item_count > 0U)
  {
    result = cardano_buffer_write(writer->buffer, COMMA, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write comma before property name.");
  }

  if (writer->format == CARDANO_JSON_FORMAT_PRETTY)
  {
    result = cardano_buffer_write(writer->buffer, &NEW_LINE[0], 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write new line before property name.");

    result = write_indentation(writer);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write indentation before property name.");
  }

  result = cardano_buffer_write(writer->buffer, QUOTES, 1);
  cardano_json_writer_set_message_if_error(writer, result, "Failed to write quotes before property name.");

  for (size_t i = 0; i < name_size; ++i)
  {
    int32_t esc = escape(name[i]);

    if (esc != 0)
    {
      result = cardano_buffer_write(writer->buffer, ESCAPE, 1);
      cardano_json_writer_set_message_if_error(writer, result, "Failed to write escape character before string value.");

      result = cardano_buffer_write(writer->buffer, (const byte_t*)&esc, 1);
      cardano_json_writer_set_message_if_error(writer, result, "Failed to write escape character value.");
    }
    else
    {
      result = cardano_buffer_write(writer->buffer, (const byte_t*)&name[i], 1);
      cardano_json_writer_set_message_if_error(writer, result, "Failed to write string value.");
    }
  }

  result = cardano_buffer_write(writer->buffer, QUOTES, 1);
  cardano_json_writer_set_message_if_error(writer, result, "Failed to write quotes after property name.");

  result = cardano_buffer_write(writer->buffer, COLON, 1);
  cardano_json_writer_set_message_if_error(writer, result, "Failed to write colon after property name.");

  if (writer->format == CARDANO_JSON_FORMAT_PRETTY)
  {
    result = cardano_buffer_write(writer->buffer, &SPACE[0], 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write space after colon.");
  }

  writer->current_frame[writer->depth].expect_value = true;
}

void
cardano_json_writer_write_bool(
  cardano_json_writer_t* writer,
  const bool             value)
{
  if ((writer == NULL) || (writer->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  cardano_json_stack_frame_t* current_context = &writer->current_frame[writer->depth];

  const bool is_root       = (current_context->context == CARDANO_JSON_CONTEXT_ROOT);
  const bool is_root_first = (is_root && (current_context->item_count == 0U));
  const bool can_write     = (is_root_first || current_context->expect_value);

  if (!can_write)
  {
    writer->last_error = CARDANO_ERROR_ENCODING;
    cardano_object_set_last_error(&writer->base, "Unexpected boolean value.");

    return;
  }

  if ((current_context->item_count > 0U) && (current_context->context == CARDANO_JSON_CONTEXT_ARRAY))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, COMMA, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write comma before boolean value.");
  }

  if ((writer->format == CARDANO_JSON_FORMAT_PRETTY) && (current_context->context == CARDANO_JSON_CONTEXT_ARRAY))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, NEW_LINE, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write new line before boolean value.");

    result = write_indentation(writer);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write indentation before boolean value.");
  }

  cardano_error_t result = cardano_buffer_write(writer->buffer, value ? TRUE : FALSE, value ? 4 : 5);
  cardano_json_writer_set_message_if_error(writer, result, "Failed to write boolean value.");

  current_context->item_count++;
  current_context->expect_value = current_context->context == CARDANO_JSON_CONTEXT_ARRAY;
}

void
cardano_json_writer_write_null(cardano_json_writer_t* writer)
{
  if ((writer == NULL) || (writer->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  cardano_json_stack_frame_t* current_context = &writer->current_frame[writer->depth];

  const bool is_root       = (current_context->context == CARDANO_JSON_CONTEXT_ROOT);
  const bool is_root_first = (is_root && (current_context->item_count == 0U));
  const bool can_write     = (is_root_first || current_context->expect_value);

  if (!can_write)
  {
    writer->last_error = CARDANO_ERROR_ENCODING;
    cardano_object_set_last_error(&writer->base, "Unexpected null value.");
    return;
  }

  if ((current_context->item_count > 0U) && (current_context->context == CARDANO_JSON_CONTEXT_ARRAY))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, COMMA, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write comma before null value.");
  }

  if ((writer->format == CARDANO_JSON_FORMAT_PRETTY) && (current_context->context == CARDANO_JSON_CONTEXT_ARRAY))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, NEW_LINE, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write new line before null value.");

    result = write_indentation(writer);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write indentation before null value.");
  }

  cardano_error_t result = cardano_buffer_write(writer->buffer, NULL_VALUE, 4);
  cardano_json_writer_set_message_if_error(writer, result, "Failed to write null value.");

  current_context->item_count++;
  current_context->expect_value = current_context->context == CARDANO_JSON_CONTEXT_ARRAY;
}

void
cardano_json_writer_write_bigint(
  cardano_json_writer_t* writer,
  cardano_bigint_t*      bigint)
{
  if ((writer == NULL) || (writer->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if (bigint == NULL)
  {
    writer->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    cardano_object_set_last_error(&writer->base, "Bigint value cannot be NULL.");
    return;
  }

  cardano_json_stack_frame_t* current_context = &writer->current_frame[writer->depth];

  const bool is_root       = (current_context->context == CARDANO_JSON_CONTEXT_ROOT);
  const bool is_root_first = (is_root && (current_context->item_count == 0U));
  const bool can_write     = (is_root_first || current_context->expect_value);

  if (!can_write)
  {
    writer->last_error = CARDANO_ERROR_ENCODING;
    cardano_object_set_last_error(&writer->base, "Unexpected bigint value.");
    return;
  }

  if (current_context->item_count > 0U)
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, COMMA, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write comma before bigint value.");
  }

  if ((writer->format == CARDANO_JSON_FORMAT_PRETTY) && (current_context->context == CARDANO_JSON_CONTEXT_ARRAY))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, NEW_LINE, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write new line before bigint value.");

    result = write_indentation(writer);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write indentation before bigint value.");
  }

  const size_t size = cardano_bigint_get_string_size(bigint, 10);
  byte_t*      data = (byte_t*)_cardano_malloc(size);
  if (data == NULL)
  {
    writer->last_error = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    cardano_object_set_last_error(&writer->base, "Failed to allocate memory for bigint value.");
    return;
  }

  cardano_error_t result = cardano_bigint_to_string(bigint, (char*)data, size, 10);
  if (result != CARDANO_SUCCESS)
  {
    _cardano_free(data);
    writer->last_error = result;
    cardano_object_set_last_error(&writer->base, "Failed to convert bigint value to string.");
    return;
  }

  result = cardano_buffer_write(writer->buffer, QUOTES, 1);
  cardano_json_writer_set_message_if_error(writer, result, "Failed to write quotes before bigint value.");

  result = cardano_buffer_write(writer->buffer, data, size - 1U);
  _cardano_free(data);

  cardano_json_writer_set_message_if_error(writer, result, "Failed to write bigint value.");

  result = cardano_buffer_write(writer->buffer, QUOTES, 1);
  cardano_json_writer_set_message_if_error(writer, result, "Failed to write quotes after bigint value.");

  current_context->item_count++;
  current_context->expect_value = current_context->context == CARDANO_JSON_CONTEXT_ARRAY;
}

void
cardano_json_writer_write_start_array(
  cardano_json_writer_t* writer)
{
  if ((writer == NULL) || (writer->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  cardano_json_stack_frame_t* current_context = &writer->current_frame[writer->depth];

  const bool is_root       = (current_context->context == CARDANO_JSON_CONTEXT_ROOT);
  const bool is_root_first = (is_root && (current_context->item_count == 0U));
  const bool can_write     = (is_root_first || current_context->expect_value);

  if (!can_write)
  {
    writer->last_error = CARDANO_ERROR_ENCODING;
    cardano_object_set_last_error(&writer->base, "Unexpected start of array.");
    return;
  }

  if ((current_context->item_count > 0U) && (current_context->context == CARDANO_JSON_CONTEXT_ARRAY))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, COMMA, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write comma before starting array.");
  }

  if ((writer->format == CARDANO_JSON_FORMAT_PRETTY) && (current_context->context == CARDANO_JSON_CONTEXT_ARRAY))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, NEW_LINE, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write new line before starting array.");

    result = write_indentation(writer);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write indentation before starting array.");
  }

  cardano_error_t result = cardano_buffer_write(writer->buffer, OPEN_ARRAY, 1);
  cardano_json_writer_set_message_if_error(writer, result, "Failed to write opening array.");

  push_context(writer, CARDANO_JSON_CONTEXT_ARRAY);
}

void
cardano_json_writer_write_end_array(cardano_json_writer_t* writer)
{
  if ((writer == NULL) || (writer->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if ((writer->depth == 0U) || (writer->current_frame[writer->depth].context != CARDANO_JSON_CONTEXT_ARRAY))
  {
    writer->last_error = CARDANO_ERROR_ENCODING;
    cardano_object_set_last_error(&writer->base, "Cannot write end of array: Not in an array context.");
    return;
  }

  const size_t item_count = writer->current_frame[writer->depth].item_count;
  pop_context(writer);

  if ((writer->format == CARDANO_JSON_FORMAT_PRETTY) && (item_count > 0U))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, NEW_LINE, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write new line before closing array.");

    result = write_indentation(writer);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write indentation before closing array.");
  }

  cardano_error_t result = cardano_buffer_write(writer->buffer, CLOSE_ARRAY, 1);
  cardano_json_writer_set_message_if_error(writer, result, "Failed to write closing array.");

  writer->current_frame[writer->depth].item_count++;
}

void
cardano_json_writer_write_start_object(cardano_json_writer_t* writer)
{
  if ((writer == NULL) || (writer->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  cardano_json_stack_frame_t* current_context = &writer->current_frame[writer->depth];

  if ((!current_context->expect_value) && (writer->depth > 0U))
  {
    writer->last_error = CARDANO_ERROR_ENCODING;
    cardano_object_set_last_error(&writer->base, "Object cannot be written as a property name.");
    return;
  }

  if ((current_context->item_count > 0U) && (current_context->context == CARDANO_JSON_CONTEXT_ARRAY))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, COMMA, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write comma before starting object.");
  }

  if ((writer->format == CARDANO_JSON_FORMAT_PRETTY) && (current_context->context == CARDANO_JSON_CONTEXT_ARRAY))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, NEW_LINE, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write new line before starting object.");

    result = write_indentation(writer);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write indentation before starting object.");
  }

  cardano_error_t result = cardano_buffer_write(writer->buffer, OPEN_OBJECT, 1);
  cardano_json_writer_set_message_if_error(writer, result, "Failed to write opening object.");

  push_context(writer, CARDANO_JSON_CONTEXT_OBJECT);
}

void
cardano_json_writer_write_end_object(cardano_json_writer_t* writer)
{
  if ((writer == NULL) || (writer->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if ((writer->depth == 0U) || (writer->current_frame[writer->depth].context != CARDANO_JSON_CONTEXT_OBJECT))
  {
    writer->last_error = CARDANO_ERROR_ENCODING;
    cardano_object_set_last_error(&writer->base, "Cannot write end of object: Not in an object context.");
    return;
  }

  const size_t item_count = writer->current_frame[writer->depth].item_count;
  pop_context(writer);

  if ((writer->format == CARDANO_JSON_FORMAT_PRETTY) && (item_count > 0U))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, NEW_LINE, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write new line before closing object.");

    result = write_indentation(writer);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write indentation before closing object.");
  }

  cardano_error_t result = cardano_buffer_write(writer->buffer, CLOSE_OBJECT, 1);
  cardano_json_writer_set_message_if_error(writer, result, "Failed to write closing object.");

  writer->current_frame[writer->depth].item_count++;
}

void
cardano_json_writer_write_raw_value(
  cardano_json_writer_t* writer,
  const char*            data,
  size_t                 size)
{
  if ((writer == NULL) || (writer->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if (data == NULL)
  {
    writer->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    cardano_object_set_last_error(&writer->base, "Raw value data cannot be NULL.");
    return;
  }

  if (size == 0U)
  {
    writer->last_error = CARDANO_ERROR_INVALID_ARGUMENT;
    cardano_object_set_last_error(&writer->base, "Raw value size cannot be zero.");
    return;
  }

  cardano_json_stack_frame_t* current_context = &writer->current_frame[writer->depth];

  const bool is_root       = (current_context->context == CARDANO_JSON_CONTEXT_ROOT);
  const bool is_root_first = (is_root && (current_context->item_count == 0U));
  const bool can_write     = (is_root_first || current_context->expect_value);

  if (!can_write)
  {
    writer->last_error = CARDANO_ERROR_ENCODING;
    cardano_object_set_last_error(&writer->base, "Unexpected raw value.");
    return;
  }

  if (current_context->item_count > 0U)
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, COMMA, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write comma before raw value.");
  }

  if ((writer->format == CARDANO_JSON_FORMAT_PRETTY) && (current_context->context == CARDANO_JSON_CONTEXT_ARRAY))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, NEW_LINE, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write new line before raw value.");

    result = write_indentation(writer);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write indentation before raw value.");
  }

  cardano_error_t result = cardano_buffer_write(writer->buffer, (const byte_t*)data, size);
  cardano_json_writer_set_message_if_error(writer, result, "Failed to write raw value.");

  current_context->item_count++;
  current_context->expect_value = current_context->context == CARDANO_JSON_CONTEXT_ARRAY;
}

void
cardano_json_writer_write_object(
  cardano_json_writer_t* writer,
  cardano_json_object_t* object)
{
  if ((writer == NULL) || (writer->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if (object == NULL)
  {
    writer->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    cardano_object_set_last_error(&writer->base, "Object cannot be NULL.");
    return;
  }

  cardano_json_stack_frame_t* current_context = &writer->current_frame[writer->depth];

  const bool is_root       = (current_context->context == CARDANO_JSON_CONTEXT_ROOT);
  const bool is_root_first = (is_root && (current_context->item_count == 0U));
  const bool can_write     = (is_root_first || current_context->expect_value);

  if (!can_write)
  {
    writer->last_error = CARDANO_ERROR_ENCODING;
    cardano_object_set_last_error(&writer->base, "Unexpected object value.");
    return;
  }

  if (current_context->item_count > 0U)
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, COMMA, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write comma before object value.");
  }

  if ((writer->format == CARDANO_JSON_FORMAT_PRETTY) && (current_context->context == CARDANO_JSON_CONTEXT_ARRAY))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, NEW_LINE, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write new line before object value.");

    result = write_indentation(writer);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write indentation before object value.");
  }

  cardano_error_t result = cardano_write_json_object(writer, object);
  cardano_json_writer_set_message_if_error(writer, result, "Failed to write object value.");

  current_context->item_count++;
  current_context->expect_value = current_context->context == CARDANO_JSON_CONTEXT_ARRAY;
}

void
cardano_json_writer_write_uint(
  cardano_json_writer_t* writer,
  const uint64_t         value)
{
  if ((writer == NULL) || (writer->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  cardano_json_stack_frame_t* current_context = &writer->current_frame[writer->depth];

  const bool is_root       = (current_context->context == CARDANO_JSON_CONTEXT_ROOT);
  const bool is_root_first = (is_root && (current_context->item_count == 0U));
  const bool can_write     = (is_root_first || current_context->expect_value);

  if (!can_write)
  {
    writer->last_error = CARDANO_ERROR_ENCODING;
    cardano_object_set_last_error(&writer->base, "Unexpected unsigned integer value.");
    return;
  }

  if ((current_context->item_count > 0U) && (current_context->context == CARDANO_JSON_CONTEXT_ARRAY))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, COMMA, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write comma before unsigned integer value.");
  }

  if ((writer->format == CARDANO_JSON_FORMAT_PRETTY) && (current_context->context == CARDANO_JSON_CONTEXT_ARRAY))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, NEW_LINE, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write new line before unsigned integer value.");

    result = write_indentation(writer);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write indentation before unsigned integer value.");
  }

  char   buffer[32]  = { 0 };
  size_t buffer_size = cardano_safe_uint64_to_string(value, buffer, sizeof(buffer));
  if (buffer_size == 0U)
  {
    writer->last_error = CARDANO_ERROR_ENCODING;
    cardano_object_set_last_error(&writer->base, "Failed to convert unsigned integer value to string.");
    return;
  }

  cardano_error_t result = cardano_buffer_write(writer->buffer, (const byte_t*)buffer, buffer_size);
  cardano_json_writer_set_message_if_error(writer, result, "Failed to write unsigned integer value.");

  current_context->item_count++;
  current_context->expect_value = current_context->context == CARDANO_JSON_CONTEXT_ARRAY;
}

void
cardano_json_writer_write_signed_int(
  cardano_json_writer_t* writer,
  int64_t                value)
{
  if ((writer == NULL) || (writer->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  cardano_json_stack_frame_t* current_context = &writer->current_frame[writer->depth];

  const bool is_root       = (current_context->context == CARDANO_JSON_CONTEXT_ROOT);
  const bool is_root_first = (is_root && (current_context->item_count == 0U));
  const bool can_write     = (is_root_first || current_context->expect_value);

  if (!can_write)
  {
    writer->last_error = CARDANO_ERROR_ENCODING;
    cardano_object_set_last_error(&writer->base, "Unexpected signed integer value.");
    return;
  }

  if ((current_context->item_count > 0U) && (current_context->context == CARDANO_JSON_CONTEXT_ARRAY))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, COMMA, 1);
    CARDANO_UNUSED(result);
  }

  if ((writer->format == CARDANO_JSON_FORMAT_PRETTY) && (current_context->context == CARDANO_JSON_CONTEXT_ARRAY))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, NEW_LINE, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write new line before signed integer value.");

    result = write_indentation(writer);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write indentation before signed integer value.");
  }

  char   buffer[32]  = { 0 };
  size_t buffer_size = cardano_safe_int64_to_string(value, buffer, sizeof(buffer));
  if (buffer_size == 0U)
  {
    writer->last_error = CARDANO_ERROR_ENCODING;
    cardano_object_set_last_error(&writer->base, "Failed to convert signed integer value to string.");
    return;
  }

  cardano_error_t result = cardano_buffer_write(writer->buffer, (const byte_t*)buffer, buffer_size);
  cardano_json_writer_set_message_if_error(writer, result, "Failed to write signed integer value.");

  current_context->item_count++;
  current_context->expect_value = current_context->context == CARDANO_JSON_CONTEXT_ARRAY;
}

void
cardano_json_writer_write_double(
  cardano_json_writer_t* writer,
  double                 value)
{
  if ((writer == NULL) || (writer->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  cardano_json_stack_frame_t* current_context = &writer->current_frame[writer->depth];

  const bool is_root       = (current_context->context == CARDANO_JSON_CONTEXT_ROOT);
  const bool is_root_first = (is_root && (current_context->item_count == 0U));
  const bool can_write     = (is_root_first || current_context->expect_value);

  if (!can_write)
  {
    writer->last_error = CARDANO_ERROR_ENCODING;
    cardano_object_set_last_error(&writer->base, "Unexpected double value.");
    return;
  }

  if ((current_context->item_count > 0U) && (current_context->context == CARDANO_JSON_CONTEXT_ARRAY))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, COMMA, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write comma before double value.");
  }

  if ((writer->format == CARDANO_JSON_FORMAT_PRETTY) && (current_context->context == CARDANO_JSON_CONTEXT_ARRAY))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, NEW_LINE, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write new line before double value.");

    result = write_indentation(writer);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write indentation before double value.");
  }

  char   buffer[32]  = { 0 };
  size_t buffer_size = cardano_safe_double_to_string(value, buffer, sizeof(buffer));
  if (buffer_size == 0U)
  {
    writer->last_error = CARDANO_ERROR_ENCODING;
    cardano_object_set_last_error(&writer->base, "Failed to convert double value to string.");
    return;
  }

  cardano_error_t result = cardano_buffer_write(writer->buffer, (const byte_t*)buffer, buffer_size);
  cardano_json_writer_set_message_if_error(writer, result, "Failed to write double value.");

  current_context->item_count++;
  current_context->expect_value = current_context->context == CARDANO_JSON_CONTEXT_ARRAY;
}

void
cardano_json_writer_write_string(
  cardano_json_writer_t* writer,
  const char*            value,
  size_t                 value_size)
{
  if ((writer == NULL) || (writer->last_error != CARDANO_SUCCESS))
  {
    return;
  }

  if (value == NULL)
  {
    writer->last_error = CARDANO_ERROR_POINTER_IS_NULL;
    cardano_object_set_last_error(&writer->base, "String value cannot be NULL or empty.");
    return;
  }

  cardano_json_stack_frame_t* current_context = &writer->current_frame[writer->depth];

  const bool is_root       = (current_context->context == CARDANO_JSON_CONTEXT_ROOT);
  const bool is_root_first = (is_root && (current_context->item_count == 0U));
  const bool can_write     = (is_root_first || current_context->expect_value);

  if (!can_write)
  {
    writer->last_error = CARDANO_ERROR_ENCODING;
    cardano_object_set_last_error(&writer->base, "Unexpected string value.");
    return;
  }

  if ((current_context->item_count > 0U) && (current_context->context == CARDANO_JSON_CONTEXT_ARRAY))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, COMMA, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write comma before string value.");
  }

  if ((writer->format == CARDANO_JSON_FORMAT_PRETTY) && (current_context->context == CARDANO_JSON_CONTEXT_ARRAY))
  {
    cardano_error_t result = cardano_buffer_write(writer->buffer, NEW_LINE, 1);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write new line before string value.");

    result = write_indentation(writer);
    cardano_json_writer_set_message_if_error(writer, result, "Failed to write indentation before string value.");
  }

  cardano_error_t result = cardano_buffer_write(writer->buffer, QUOTES, 1);
  cardano_json_writer_set_message_if_error(writer, result, "Failed to write quotes before string value.");

  for (size_t i = 0; i < value_size; ++i)
  {
    int32_t esc = escape(value[i]);

    if (esc != 0)
    {
      result = cardano_buffer_write(writer->buffer, ESCAPE, 1);
      cardano_json_writer_set_message_if_error(writer, result, "Failed to write escape character before string value.");

      result = cardano_buffer_write(writer->buffer, (const byte_t*)&esc, 1);
      cardano_json_writer_set_message_if_error(writer, result, "Failed to write escape character value.");
    }
    else
    {
      result = cardano_buffer_write(writer->buffer, (const byte_t*)&value[i], 1);
      cardano_json_writer_set_message_if_error(writer, result, "Failed to write string value.");
    }
  }

  result = cardano_buffer_write(writer->buffer, QUOTES, 1);
  cardano_json_writer_set_message_if_error(writer, result, "Failed to write quotes after string value.");

  current_context->item_count++;
  current_context->expect_value = current_context->context == CARDANO_JSON_CONTEXT_ARRAY;
}

cardano_json_context_t
cardano_json_writer_get_context(cardano_json_writer_t* writer)
{
  if (writer == NULL)
  {
    return CARDANO_JSON_CONTEXT_ROOT;
  }

  if (writer->depth == 0U)
  {
    return CARDANO_JSON_CONTEXT_ROOT;
  }

  return writer->current_frame[writer->depth].context;
}

size_t
cardano_json_writer_get_encoded_size(cardano_json_writer_t* writer)
{
  if ((writer == NULL) || (writer->last_error != CARDANO_SUCCESS))
  {
    return 0U;
  }

  return cardano_buffer_get_size(writer->buffer) + 1U;
}

cardano_error_t
cardano_json_writer_encode(cardano_json_writer_t* writer, char* data, const size_t size)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }
  if (writer->last_error != CARDANO_SUCCESS)
  {
    return writer->last_error;
  }

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((writer->current_frame[writer->depth].context != CARDANO_JSON_CONTEXT_ROOT) || (writer->current_frame[writer->depth].expect_value))
  {
    cardano_json_writer_set_last_error(writer, "JSON document is incomplete.");
    return CARDANO_ERROR_ENCODING;
  }

  const size_t buffer_size = cardano_buffer_get_size(writer->buffer);

  if ((buffer_size + 1U) > size)
  {
    return CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE;
  }

  CARDANO_UNUSED(memset(data, 0, buffer_size + 1U));
  cardano_safe_memcpy(data, size, cardano_buffer_get_data(writer->buffer), buffer_size);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_json_writer_encode_in_buffer(cardano_json_writer_t* writer, cardano_buffer_t** buffer)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (buffer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *buffer = cardano_buffer_new(cardano_buffer_get_size(writer->buffer));

  if (*buffer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_buffer_write(*buffer, cardano_buffer_get_data(writer->buffer), cardano_buffer_get_size(writer->buffer));

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(buffer);
  }

  return result;
}

cardano_error_t
cardano_json_writer_reset(cardano_json_writer_t* writer)
{
  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_buffer_unref(&writer->buffer);

  writer->buffer                        = cardano_buffer_new(128);
  writer->depth                         = 0U;
  writer->last_error                    = CARDANO_SUCCESS;
  writer->current_frame[0].context      = CARDANO_JSON_CONTEXT_ROOT;
  writer->current_frame[0].item_count   = 0U;
  writer->current_frame[0].expect_value = false;

  return CARDANO_SUCCESS;
}

void
cardano_json_writer_unref(cardano_json_writer_t** json_writer)
{
  if ((json_writer == NULL) || (*json_writer == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*json_writer)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *json_writer = NULL;
    return;
  }
}

void
cardano_json_writer_ref(cardano_json_writer_t* json_writer)
{
  if (json_writer == NULL)
  {
    return;
  }

  cardano_object_ref(&json_writer->base);
}

size_t
cardano_json_writer_refcount(const cardano_json_writer_t* json_writer)
{
  if (json_writer == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&json_writer->base);
}

void
cardano_json_writer_set_last_error(cardano_json_writer_t* writer, const char* message)
{
  cardano_object_set_last_error(&writer->base, message);
}

const char*
cardano_json_writer_get_last_error(const cardano_json_writer_t* writer)
{
  return cardano_object_get_last_error(&writer->base);
}
