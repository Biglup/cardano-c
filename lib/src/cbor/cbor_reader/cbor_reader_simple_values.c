/**
 * \file cbor_reader_simple_values.c
 *
 * \author luisd.bianchi
 * \date   Mar 14, 2024
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

#include "cbor_reader_simple_values.h"
#include "cbor_reader_core.h"

#include <cardano/cbor/cbor_reader.h>
#include <cardano/error.h>

#include "../cbor_initial_byte.h"

#include <assert.h>

/* IMPLEMENTATION ************************************************************/

cardano_error_t
_cbor_reader_read_boolean(cardano_cbor_reader_t* reader, bool* value)
{
  byte_t          header = 0;
  cardano_error_t result = _cbor_reader_peek_initial_byte(reader, CARDANO_CBOR_MAJOR_TYPE_SIMPLE, &header);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_additional_info_t additional_info = cardano_cbor_initial_byte_get_additional_info(header);

  if ((additional_info != CARDANO_CBOR_ADDITIONAL_INFO_TRUE) && (additional_info != CARDANO_CBOR_ADDITIONAL_INFO_FALSE))
  {
    cardano_cbor_reader_set_last_error(reader, "Not a boolean encoding");
    return CARDANO_ERROR_DECODING;
  }

  *value = additional_info == CARDANO_CBOR_ADDITIONAL_INFO_TRUE;

  CARDANO_UNUSED(_cbor_reader_advance_buffer(reader, 1));

  _cbor_reader_advance_data_item_counters(reader);

  return CARDANO_SUCCESS;
}

cardano_error_t
_cbor_reader_read_null(cardano_cbor_reader_t* reader)
{
  byte_t header = 0;

  cardano_error_t result = _cbor_reader_peek_initial_byte(reader, CARDANO_CBOR_MAJOR_TYPE_SIMPLE, &header);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_additional_info_t additional_info = cardano_cbor_initial_byte_get_additional_info(header);

  if (additional_info != CARDANO_CBOR_ADDITIONAL_INFO_NULL)
  {
    cardano_cbor_reader_set_last_error(reader, "Not a null encoding");
    return CARDANO_ERROR_DECODING;
  }

  CARDANO_UNUSED(_cbor_reader_advance_buffer(reader, 1));

  _cbor_reader_advance_data_item_counters(reader);

  return CARDANO_SUCCESS;
}

cardano_error_t
_cbor_reader_read_simple_value(cardano_cbor_reader_t* reader, cardano_cbor_simple_value_t* value)
{
  byte_t          header = 0;
  cardano_error_t result = _cbor_reader_peek_initial_byte(reader, CARDANO_CBOR_MAJOR_TYPE_SIMPLE, &header);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_additional_info_t additional_info = cardano_cbor_initial_byte_get_additional_info(header);

  if (additional_info < CARDANO_CBOR_ADDITIONAL_INFO_8BIT_DATA)
  {
    CARDANO_UNUSED(_cbor_reader_advance_buffer(reader, 1));

    _cbor_reader_advance_data_item_counters(reader);
    *value = (cardano_cbor_simple_value_t)additional_info;
    return CARDANO_SUCCESS;
  }

  if (additional_info == CARDANO_CBOR_ADDITIONAL_INFO_8BIT_DATA)
  {
    if (cardano_buffer_get_size(reader->buffer) <= (reader->offset + 1U))
    {
      return CARDANO_ERROR_DECODING;
    }

    byte_t simple_value = cardano_buffer_get_data(reader->buffer)[reader->offset + 1U];

    CARDANO_UNUSED(_cbor_reader_advance_buffer(reader, 2));

    _cbor_reader_advance_data_item_counters(reader);
    *value = (cardano_cbor_simple_value_t)simple_value;

    return CARDANO_SUCCESS;
  }

  cardano_cbor_reader_set_last_error(reader, "Not a simple value encoding");
  return CARDANO_ERROR_DECODING;
}
