/**
 * \file cbor_validation.c
 *
 * \author angel.castillo
 * \date   Sep 12, 2024
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

#include "cbor_validation.h"

#include <assert.h>
#include <cardano/cbor/cbor_major_type.h>
#include <stdio.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Sets a detailed error message on a CBOR reader.
 *
 * This function constructs and sets a descriptive error message on a CBOR reader object
 * when a value does not meet the expected criteria during decoding.
 *
 * \param[in,out] reader The CBOR reader object where the error message will be stored.
 * \param[in] validator_name The name of the validator or field being checked.
 * \param[in] expected_value The expected value for comparison.
 * \param[in] expected_friendly_name A human-readable name or description of the expected value.
 * \param[in] actual_value The actual value retrieved from the CBOR data.
 * \param[in] actual_friendly_name A human-readable name or description of the actual value.
 *
 * \note This function constructs an error message indicating a mismatch between expected
 *       and actual values during CBOR decoding and updates the reader's error state.
 *       It assumes that all input pointers (reader, validator_name, expected_friendly_name,
 *       and actual_friendly_name) are valid and non-NULL.
 */
static void
set_invalid_type_error_message(
  cardano_cbor_reader_t* reader,
  const char*            validator_name,
  const uint64_t         expected_value,
  const char*            expected_friendly_name,
  const uint64_t         actual_value,
  const char*            actual_friendly_name)
{
  assert(reader != NULL);
  assert(validator_name != NULL);
  assert(expected_friendly_name != NULL);
  assert(actual_friendly_name != NULL);

  char buffer[1023] = { 0 };

  const int32_t written = snprintf(
    buffer,
    sizeof(buffer),
    "There was an error decoding the %s, expected %s (%lu) but got %s (%lu).",
    validator_name,
    expected_friendly_name,
    expected_value,
    actual_friendly_name,
    actual_value);

  assert(written > 0);
  CARDANO_UNUSED(written);

  cardano_cbor_reader_set_last_error(reader, buffer);
}

/**
 * \brief Sets an error message for size validation failures in CBOR decoding.
 *
 * This function generates and sets a descriptive error message on a CBOR reader object when the
 * decoded data does not match the expected size criteria, providing details about the expected
 * and actual sizes along with the type information.
 *
 * \param[in,out] reader The CBOR reader object where the error message will be stored.
 * \param[in] validator_name The name of the field or type validator where the error occurred.
 * \param[in] type_value The numerical value representing the type being validated.
 * \param[in] type_friendly_name A human-readable description of the type.
 * \param[in] expected_length The expected length or size of the type.
 * \param[in] actual_length The actual length or size encountered during decoding.
 *
 * \note This function assumes that all input pointers (reader, validator_name, type_friendly_name) are valid.
 */
static void
set_invalid_size_error_message(
  cardano_cbor_reader_t* reader,
  const char*            validator_name,
  const int64_t          type_value,
  const char*            type_friendly_name,
  const size_t           expected_length,
  const size_t           actual_length)
{
  assert(reader != NULL);
  assert(validator_name != NULL);
  assert(type_friendly_name != NULL);

  char buffer[1023] = { 0 };

  const int32_t written = snprintf(
    buffer,
    sizeof(buffer),
    "There was an error decoding the %s, expected a %s (%ld) of %zu element(s) but got a %s (%ld) of %zu element(s).",
    validator_name,
    type_friendly_name,
    type_value,
    expected_length,
    type_friendly_name,
    type_value,
    actual_length);

  assert(written > 0);
  CARDANO_UNUSED(written);

  cardano_cbor_reader_set_last_error(reader, buffer);
}

/**
 * \brief Sets an error message for range validation failures in CBOR decoding.
 *
 * This function generates and sets a descriptive error message on a CBOR reader object when the
 * decoded data does not match the expected range criteria, providing details about the expected
 * and actual values along with the type information.
 *
 * \param[in,out] reader The CBOR reader object where the error message will be stored.
 * \param[in] validator_name The name of the field or type validator where the error occurred.
 * \param[in] type_friendly_name A human-readable description of the type being validated.
 * \param[in] expected_min_value The minimum expected value for the type.
 * \param[in] expected_max_value The maximum expected value for the type.
 * \param[in] actual_value The actual value encountered during decoding.
 *
 * \note This function assumes that all input pointers (reader, validator_name, type_friendly_name) are valid.
 */
static void
set_invalid_range_error_message(
  cardano_cbor_reader_t* reader,
  const char*            validator_name,
  const char*            type_friendly_name,
  const uint64_t         expected_min_value,
  const uint64_t         expected_max_value,
  const uint64_t         actual_value)
{
  assert(reader != NULL);
  assert(validator_name != NULL);
  assert(type_friendly_name != NULL);

  char buffer[1023] = { 0 };

  const int32_t written = snprintf(
    buffer,
    sizeof(buffer),
    "There was an error decoding the %s, %s must have a value between %lu and %lu, but got %lu.",
    validator_name,
    type_friendly_name,
    expected_min_value,
    expected_max_value,
    actual_value);

  assert(written > 0);
  CARDANO_UNUSED(written);

  cardano_cbor_reader_set_last_error(reader, buffer);
}

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_cbor_validate_array_of_n_elements(const char* validator_name, cardano_cbor_reader_t* reader, const uint32_t n)
{
  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  cardano_error_t peek_result = cardano_cbor_reader_peek_state(reader, &state);

  if (peek_result != CARDANO_SUCCESS)
  {
    return peek_result;
  }

  if (state != CARDANO_CBOR_READER_STATE_START_ARRAY)
  {
    set_invalid_type_error_message(
      reader,
      validator_name,
      CARDANO_CBOR_READER_STATE_START_ARRAY,
      cardano_cbor_reader_state_to_string(CARDANO_CBOR_READER_STATE_START_ARRAY),
      state,
      cardano_cbor_reader_state_to_string(state));

    return CARDANO_ERROR_UNEXPECTED_CBOR_TYPE;
  }

  int64_t array_size = 0U;

  cardano_error_t read_start_array_result = cardano_cbor_reader_read_start_array(reader, &array_size);

  if (read_start_array_result != CARDANO_SUCCESS)
  {
    return read_start_array_result; /* LCOV_EXCL_LINE */
  }

  if ((uint64_t)array_size != n)
  {
    set_invalid_size_error_message(
      reader,
      validator_name,
      CARDANO_CBOR_MAJOR_TYPE_BYTE_STRING,
      cardano_cbor_major_type_to_string(CARDANO_CBOR_MAJOR_TYPE_BYTE_STRING),
      n,
      array_size);

    return CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_cbor_validate_uint_in_range(const char* validator_name, const char* type_name, cardano_cbor_reader_t* reader, uint64_t* type, const uint64_t min, const uint64_t max)
{
  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  cardano_error_t peek_result = cardano_cbor_reader_peek_state(reader, &state);

  if (peek_result != CARDANO_SUCCESS)
  {
    return peek_result;
  }

  if (state != CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER)
  {
    set_invalid_type_error_message(
      reader,
      validator_name,
      CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER,
      cardano_cbor_reader_state_to_string(CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER),
      state,
      cardano_cbor_reader_state_to_string(state));

    return CARDANO_ERROR_UNEXPECTED_CBOR_TYPE;
  }

  cardano_error_t read_uint_result = cardano_cbor_reader_read_uint(reader, type);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result; /* LCOV_EXCL_LINE */
  }

  if ((*type < min) || (*type > max))
  {
    set_invalid_range_error_message(
      reader,
      validator_name,
      type_name,
      min,
      max,
      *type);

    return CARDANO_ERROR_INVALID_CBOR_VALUE;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_cbor_validate_byte_string_of_size(const char* validator_name, cardano_cbor_reader_t* reader, cardano_buffer_t** byte_string, const uint32_t size)
{
  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  cardano_error_t peek_result = cardano_cbor_reader_peek_state(reader, &state);

  if (peek_result != CARDANO_SUCCESS)
  {
    return peek_result;
  }

  if (state != CARDANO_CBOR_READER_STATE_BYTESTRING)
  {
    set_invalid_type_error_message(
      reader,
      validator_name,
      CARDANO_CBOR_READER_STATE_BYTESTRING,
      cardano_cbor_reader_state_to_string(CARDANO_CBOR_READER_STATE_BYTESTRING),
      state,
      cardano_cbor_reader_state_to_string(state));

    return CARDANO_ERROR_UNEXPECTED_CBOR_TYPE;
  }

  cardano_error_t read_byte_string_result = cardano_cbor_reader_read_bytestring(reader, byte_string);

  if (read_byte_string_result != CARDANO_SUCCESS)
  {
    return read_byte_string_result;
  }

  const size_t byte_string_size = cardano_buffer_get_size(*byte_string);

  if (cardano_buffer_get_size(*byte_string) != size)
  {
    cardano_buffer_unref(byte_string);

    set_invalid_size_error_message(
      reader,
      validator_name,
      CARDANO_CBOR_READER_STATE_BYTESTRING,
      cardano_cbor_reader_state_to_string(CARDANO_CBOR_READER_STATE_BYTESTRING),
      size,
      byte_string_size);

    return CARDANO_ERROR_INVALID_CBOR_VALUE;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_cbor_validate_end_array(const char* validator_name, cardano_cbor_reader_t* reader)
{
  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  cardano_error_t peek_result = cardano_cbor_reader_peek_state(reader, &state);

  if (peek_result != CARDANO_SUCCESS)
  {
    return peek_result;
  }

  if (state != CARDANO_CBOR_READER_STATE_END_ARRAY)
  {
    set_invalid_type_error_message(
      reader,
      validator_name,
      CARDANO_CBOR_READER_STATE_END_ARRAY,
      cardano_cbor_reader_state_to_string(CARDANO_CBOR_READER_STATE_END_ARRAY),
      state,
      cardano_cbor_reader_state_to_string(state));

    return CARDANO_ERROR_UNEXPECTED_CBOR_TYPE;
  }

  cardano_error_t read_end_array_result = cardano_cbor_reader_read_end_array(reader);

  if (read_end_array_result != CARDANO_SUCCESS)
  {
    return read_end_array_result; /* LCOV_EXCL_LINE */
  }

  return CARDANO_SUCCESS;
}
