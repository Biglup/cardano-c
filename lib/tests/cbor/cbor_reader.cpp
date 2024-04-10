/**
 * \file cbor.cpp
 *
 * \author luisd.bianchi
 * \date   Mar 16, 2024
 *
 * \section LICENSE
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
#include <cardano/cbor/cbor_reader.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>
#include <math.h>

extern "C" {
#include "../src/cbor/cbor_reader/cbor_reader_collections.h"
#include "../src/cbor/cbor_reader/cbor_reader_numeric.h"
#include "../src/cbor/cbor_reader/cbor_reader_simple_values.h"
#include "../src/cbor/cbor_reader/cbor_reader_tags.h"
}

/* STATIC FUNCTIONS **********************************************************/

static void
verify_int(const char* hex, const int64_t expected_int, const cardano_cbor_reader_state_t expected_state)
{
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(hex, strlen(hex));

  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;
  cardano_error_t             result = cardano_cbor_reader_peek_state(reader, &state);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, expected_state);

  int64_t value = 0;
  result        = cardano_cbor_reader_read_int(reader, &value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(value, expected_int);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
}

static void
verify_float(const char* hex, const double expected_float, const cardano_cbor_reader_state_t expected_state)
{
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(hex, strlen(hex));

  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;
  cardano_error_t             result = cardano_cbor_reader_peek_state(reader, &state);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, expected_state);

  double value = 0;
  result       = cardano_cbor_reader_read_double(reader, &value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(value, expected_float);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
}

static void
verify_text(const char* hex, const char* expected_text, const cardano_cbor_reader_state_t expected_state)
{
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(hex, strlen(hex));
  cardano_buffer_t*      buffer   = nullptr;
  char                   text[30] = { 0 };

  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;
  cardano_error_t             result = cardano_cbor_reader_peek_state(reader, &state);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, expected_state);

  result = cardano_cbor_reader_read_textstring(reader, &buffer);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_buffer_to_str(buffer, text, 30);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  EXPECT_STREQ(text, expected_text);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
  cardano_buffer_unref(&buffer);
}

static std::string
get_json_val(cardano_cbor_reader_t* reader)
{
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;
  cardano_error_t             result = cardano_cbor_reader_peek_state(reader, &state);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  switch (state)
  {
    case CARDANO_CBOR_READER_STATE_BYTESTRING:
    {
      cardano_buffer_t* buffer = nullptr;
      result                   = cardano_cbor_reader_read_bytestring(reader, &buffer);
      EXPECT_EQ(result, CARDANO_SUCCESS);

      char text[128] = { 0 };
      result         = cardano_buffer_to_hex(buffer, text, 128);
      EXPECT_EQ(result, CARDANO_SUCCESS);

      cardano_buffer_unref(&buffer);
      return std::string(text);
    }
    case CARDANO_CBOR_READER_STATE_TEXTSTRING:
    {
      cardano_buffer_t* buffer = nullptr;
      result                   = cardano_cbor_reader_read_textstring(reader, &buffer);
      EXPECT_EQ(result, CARDANO_SUCCESS);

      char text[128] = { 0 };
      result         = cardano_buffer_to_str(buffer, text, 128);
      EXPECT_EQ(result, CARDANO_SUCCESS);

      cardano_buffer_unref(&buffer);
      return std::string(text);
    }
    case CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER:
    case CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER:
    {
      int64_t value = 0;
      result        = cardano_cbor_reader_read_int(reader, &value);
      EXPECT_EQ(result, CARDANO_SUCCESS);

      return std::to_string(value);
    }
    case CARDANO_CBOR_READER_STATE_START_MAP:
    {
      int64_t length = 0;
      result         = cardano_cbor_reader_read_start_map(reader, &length);
      EXPECT_EQ(result, CARDANO_SUCCESS);

      std::string json = "{";

      if (length > 0)
      {
        for (size_t i = 0; i < length; ++i)
        {
          std::string key = get_json_val(reader);
          std::string val = get_json_val(reader);
          json            += "\"" + key + "\":" + val;
          if (i < length - 1)
          {
            json += ",";
          }
        }
      }
      else
      {
        while (state != CARDANO_CBOR_READER_STATE_END_MAP)
        {
          std::string key = get_json_val(reader);
          std::string val = get_json_val(reader);
          json            += "\"" + key + "\":" + val;
          result          = cardano_cbor_reader_peek_state(reader, &state);
          EXPECT_EQ(result, CARDANO_SUCCESS);
          if (state != CARDANO_CBOR_READER_STATE_END_MAP)
          {
            json += ",";
          }
        }
      }

      result = cardano_cbor_reader_read_end_map(reader);
      EXPECT_EQ(result, CARDANO_SUCCESS);
      json += "}";

      return json;
    }
  }

  return "ERROR";
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_cbor_reader_new, returnsNullIfGivenNullPtr)
{
  // Act
  cardano_cbor_reader_t* reader = cardano_cbor_reader_new(nullptr, 0);

  // Assert
  EXPECT_EQ(reader, (cardano_cbor_reader_t*)nullptr);
}

TEST(cardano_cbor_reader_new, returnsNullIfGivenZeroSize)
{
  // Act
  cardano_cbor_reader_t* reader = cardano_cbor_reader_new((const byte_t*)"", 0);

  // Assert
  EXPECT_EQ(reader, (cardano_cbor_reader_t*)nullptr);
}

TEST(cardano_cbor_reader_new, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t* reader = nullptr;
  const byte_t           cbor[] = { 0x81, 0x18, 0x2a };

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  reader = cardano_cbor_reader_new(&cbor[0], 3);

  // Assert
  EXPECT_EQ(reader, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_cbor_reader_new, returnsNullIfMemoryAllocationEventuallyFails)
{
  // Arrange
  cardano_cbor_reader_t* reader = nullptr;
  const byte_t           cbor[] = { 0x81, 0x18, 0x2a };

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  reader = cardano_cbor_reader_new(&cbor[0], 3);

  // Assert
  EXPECT_EQ(reader, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_cbor_reader_new, createsANewObjectWithRefCountOne)
{
  // Arrange
  const char*  cbor_hex = "81182a";
  const byte_t cbor[]   = { 0x81, 0x18, 0x2a };

  // Act
  cardano_cbor_reader_t* reader_hex = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_new(&cbor[0], 3);

  // Assert
  EXPECT_THAT(reader_hex, testing::Not((cardano_cbor_reader_t*)nullptr));
  EXPECT_EQ(cardano_cbor_reader_refcount(reader_hex), 1);

  EXPECT_THAT(reader, testing::Not((cardano_cbor_reader_t*)nullptr));
  EXPECT_EQ(cardano_cbor_reader_refcount(reader), 1);

  // Cleanup
  cardano_cbor_reader_unref(&reader_hex);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_ref, increasesTheReferenceCount)
{
  // Arrange
  const char*            cbor_hex = "81182a";
  cardano_cbor_reader_t* reader   = nullptr;

  // Act
  reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_cbor_reader_ref(reader);

  // Assert
  EXPECT_THAT(reader, testing::Not((cardano_cbor_reader_t*)nullptr));
  EXPECT_EQ(cardano_cbor_reader_refcount(reader), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_from_hex, returnsNullIfGivenNullPtr)
{
  // Act
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(nullptr, 0);

  // Assert
  EXPECT_EQ(reader, (cardano_cbor_reader_t*)nullptr);
}

TEST(cardano_cbor_reader_from_hex, returnsNullIfGivenZeroSize)
{
  // Act
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex((const char*)"", 0);

  // Assert
  EXPECT_EQ(reader, (cardano_cbor_reader_t*)nullptr);
}

TEST(cardano_cbor_reader_from_hex, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t* reader   = nullptr;
  const char*            cbor_hex = "81182a";

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  // Assert
  EXPECT_EQ(reader, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_cbor_reader_from_hex, returnsNullIfMemoryAllocationEventuallyFails)
{
  // Arrange
  cardano_cbor_reader_t* reader   = nullptr;
  const char*            cbor_hex = "81182a";

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  // Assert
  EXPECT_EQ(reader, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_cbor_reader_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_cbor_reader_ref(nullptr);
}

TEST(cardano_cbor_reader_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_cbor_reader_t* reader = nullptr;

  // Act
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_cbor_reader_unref((cardano_cbor_reader_t**)nullptr);
}

TEST(cardano_cbor_reader_unref, decreasesTheReferenceCount)
{
  // Arrange
  const char*            cbor_hex = "81182a";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  // Act
  cardano_cbor_reader_ref(reader);
  size_t ref_count = cardano_cbor_reader_refcount(reader);

  cardano_cbor_reader_unref(&reader);
  size_t updated_ref_count = cardano_cbor_reader_refcount(reader);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  const char*            cbor_hex = "81182a";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  // Act
  cardano_cbor_reader_ref(reader);
  size_t ref_count = cardano_cbor_reader_refcount(reader);

  cardano_cbor_reader_unref(&reader);
  size_t updated_ref_count = cardano_cbor_reader_refcount(reader);

  cardano_cbor_reader_unref(&reader);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(reader, (cardano_cbor_reader_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_move, decreasesTheReferenceCountWithoutDeletingTheObject)
{
  // Arrange
  const char*            cbor_hex = "81182a";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  // Act
  EXPECT_THAT(cardano_cbor_reader_move(reader), testing::Not((cardano_cbor_reader_t*)nullptr));
  size_t ref_count = cardano_cbor_reader_refcount(reader);

  // Assert
  EXPECT_EQ(ref_count, 0);
  EXPECT_THAT(reader, testing::Not((cardano_cbor_reader_t*)nullptr));

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_cbor_reader_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_cbor_reader_move, returnsNullIfGivenANullPtr)
{
  // Act
  cardano_cbor_reader_t* reader = cardano_cbor_reader_move(nullptr);

  // Assert
  EXPECT_EQ(reader, (cardano_cbor_reader_t*)nullptr);
}

TEST(cardano_cbor_reader_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader  = nullptr;
  const char*            message = "This is a test message";

  // Act
  cardano_cbor_reader_set_last_error(reader, message);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Object is NULL.");
}

TEST(cardano_cbor_reader_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  const char*            cbor_hex = "81182a";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  const char*            message  = nullptr;

  // Act
  cardano_cbor_reader_set_last_error(reader, message);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_peek_state, returnsTheStateOfTheReader)
{
  // Arrange
  const char*                 cbor_hex = "81182a";
  cardano_cbor_reader_t*      reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_cbor_reader_state_t state    = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);

  // Assert
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_ARRAY);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_start_array, canReadAnEmptyFixedArray)
{
  // Arrange
  const char*                 cbor_hex = "80";
  cardano_cbor_reader_t*      reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  int64_t                     length   = 0;
  cardano_cbor_reader_state_t state    = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_ARRAY);

  result = cardano_cbor_reader_read_start_array(reader, &length);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(length, 0);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_END_ARRAY);

  result = cardano_cbor_reader_read_end_array(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_start_array, canReadFixedsizeArrayWithAndUnsignedNumber)
{
  // Arrange
  const char*                 cbor_hex = "81182a";
  cardano_cbor_reader_t*      reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  int64_t                     length   = 0;
  cardano_cbor_reader_state_t state    = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_ARRAY);

  result = cardano_cbor_reader_read_start_array(reader, &length);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(length, 1);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);

  uint64_t value = 0;
  result         = cardano_cbor_reader_read_uint(reader, &value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(value, 42);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_END_ARRAY);

  result = cardano_cbor_reader_read_end_array(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_start_array, canReadAwwayWithSevelraUnsignedNumbers)
{
  // Arrange
  const char*                 cbor_hex = "98190102030405060708090a0b0c0d0e0f101112131415161718181819";
  cardano_cbor_reader_t*      reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  int64_t                     length   = 0;
  cardano_cbor_reader_state_t state    = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_ARRAY);

  result = cardano_cbor_reader_read_start_array(reader, &length);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(length, 25);

  for (size_t i = 0; i < length; ++i)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);
    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);

    uint64_t value = 0;
    result         = cardano_cbor_reader_read_uint(reader, &value);
    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_EQ(value, i + 1);
  }

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_END_ARRAY);

  result = cardano_cbor_reader_read_end_array(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_start_array, canReadAFixedSizeArrayWithUnsigned64bitsNumbers)
{
  // Arrange
  const char* cbor_hex = "831BCD2FB6B45D4CF7B01BCD2FB6B45D4CF7B11BCD2FB6B45D4CF7B2";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  int64_t                     length = 0;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_ARRAY);

  result = cardano_cbor_reader_read_start_array(reader, &length);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(length, 3);

  for (size_t i = 0; i < length; ++i)
  {
    result = cardano_cbor_reader_peek_state(reader, &state);
    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);

    uint64_t value = 0;
    result         = cardano_cbor_reader_read_uint(reader, &value);
    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_EQ(value, 14785236987456321456U + i);
  }

  result = cardano_cbor_reader_read_end_array(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_start_array, canReadArrayWithMixedTypes)
{
  // Arrange
  const char* cbor_hex = "8301204107";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  int64_t                     length = 0;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_ARRAY);

  result = cardano_cbor_reader_read_start_array(reader, &length);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(length, 3);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);

  uint64_t uint_value = 0;
  result              = cardano_cbor_reader_read_uint(reader, &uint_value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(uint_value, 1);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER);

  int64_t int_value = 0;
  result            = cardano_cbor_reader_read_int(reader, &int_value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(int_value, -1);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_BYTESTRING);

  cardano_buffer_t* byte_string = nullptr;
  result                        = cardano_cbor_reader_read_bytestring(reader, &byte_string);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_buffer_get_size(byte_string), 1);
  EXPECT_EQ(cardano_buffer_get_data(byte_string)[0], 7);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_END_ARRAY);

  result = cardano_cbor_reader_read_end_array(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_buffer_unref(&byte_string);
}

TEST(cardano_cbor_reader_read_start_array, canReadArrayOfSimpleValues)
{
  // Arrange
  const char* cbor_hex = "84f4f6faffc00000fb7ff0000000000000";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  int64_t                     length = 0;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_ARRAY);

  result = cardano_cbor_reader_read_start_array(reader, &length);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(length, 4);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_BOOLEAN);

  bool bool_value = true;
  result          = cardano_cbor_reader_read_boolean(reader, &bool_value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(bool_value, false);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_NULL);

  result = cardano_cbor_reader_read_null(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_SINGLE_PRECISION_FLOAT);

  double single_precition_value = 0.0;
  result                        = cardano_cbor_reader_read_double(reader, &single_precition_value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(isnan(single_precition_value), true);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_DOUBLE_PRECISION_FLOAT);

  double double_precition_value = 0.0;
  result                        = cardano_cbor_reader_read_double(reader, &double_precition_value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(isinf(double_precition_value), true);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_END_ARRAY);

  result = cardano_cbor_reader_read_end_array(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_start_array, canReadFixedSizeArrayWithNestedValues)
{
  // Arrange
  const char* cbor_hex = "8301820203820405";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  int64_t                     length = 0;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_ARRAY);

  result = cardano_cbor_reader_read_start_array(reader, &length);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(length, 3);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);

  int64_t int_value = 0;
  result            = cardano_cbor_reader_read_int(reader, &int_value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(int_value, 1);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_ARRAY);

  result = cardano_cbor_reader_read_start_array(reader, &length);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(length, 2);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);

  int_value = 0;
  result    = cardano_cbor_reader_read_int(reader, &int_value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(int_value, 2);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);

  int_value = 0;
  result    = cardano_cbor_reader_read_int(reader, &int_value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(int_value, 3);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_END_ARRAY);

  result = cardano_cbor_reader_read_end_array(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_ARRAY);

  result = cardano_cbor_reader_read_start_array(reader, &length);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(length, 2);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);

  int_value = 0;
  result    = cardano_cbor_reader_read_int(reader, &int_value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(int_value, 4);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);

  int_value = 0;
  result    = cardano_cbor_reader_read_int(reader, &int_value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(int_value, 5);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_END_ARRAY);

  result = cardano_cbor_reader_read_end_array(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_END_ARRAY);

  result = cardano_cbor_reader_read_end_array(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_start_array, canReadAnEmptyIndefiniteLengthArray)
{
  // Arrange
  const char* cbor_hex = "9fff";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  int64_t                     length = 0;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_ARRAY);

  result = cardano_cbor_reader_read_start_array(reader, &length);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(length, -1);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_END_ARRAY);

  result = cardano_cbor_reader_read_end_array(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_start_array, canReadIndefiniteLengthArrayWithAndUnsignedNumber)
{
  // Arrange
  const char* cbor_hex = "9f182aff";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  int64_t                     length = 0;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_ARRAY);

  result = cardano_cbor_reader_read_start_array(reader, &length);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(length, -1);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);

  uint64_t uint_value = 0;
  result              = cardano_cbor_reader_read_uint(reader, &uint_value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(uint_value, 42);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_END_ARRAY);

  result = cardano_cbor_reader_read_end_array(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_start_array, canReadIndefiniteLengthArrayWithSeveralUnsignedNumbers)
{
  // Arrange
  const char* cbor_hex = "9f0102030405060708090a0b0c0d0e0f101112131415161718181819ff";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  int64_t                     length = 0;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_ARRAY);

  result = cardano_cbor_reader_read_start_array(reader, &length);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(length, -1);

  int64_t count = 0;
  while (state != CARDANO_CBOR_READER_STATE_END_ARRAY)
  {
    count++;
    result = cardano_cbor_reader_peek_state(reader, &state);
    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);

    uint64_t value = 0;
    result         = cardano_cbor_reader_read_uint(reader, &value);
    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_EQ(value, count);

    result = cardano_cbor_reader_peek_state(reader, &state);
    EXPECT_EQ(result, CARDANO_SUCCESS);
  }

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_END_ARRAY);

  result = cardano_cbor_reader_read_end_array(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_bytestring, canReadAnEmptyFixedSizeBytestring)
{
  // Arrange
  const char* cbor_hex = "40";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*           buffer = nullptr;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_BYTESTRING);

  result = cardano_cbor_reader_read_bytestring(reader, &buffer);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_buffer_get_size(buffer), 0);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_cbor_reader_read_bytestring, canReadFixedSizeBytestring)
{
  // Arrange
  const char* cbor_hex = "4401020304";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*           buffer = nullptr;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_BYTESTRING);

  result = cardano_cbor_reader_read_bytestring(reader, &buffer);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_buffer_get_size(buffer), 4);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[0], 1);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[1], 2);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[2], 3);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[3], 4);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_cbor_reader_read_bytestring, canReadFixedSizeBytestringWithAllFF)
{
  // Arrange
  const char* cbor_hex = "4effffffffffffffffffffffffffff";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*           buffer = nullptr;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_BYTESTRING);

  result = cardano_cbor_reader_read_bytestring(reader, &buffer);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_buffer_get_size(buffer), 14);

  for (size_t i = 0; i < 14; ++i)
  {
    EXPECT_EQ(cardano_buffer_get_data(buffer)[i], 0xff);
  }

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_cbor_reader_read_bytestring, canReadEmptyNoArrayIndefiniteArray)
{
  // Arrange
  const char* cbor_hex = "5fff";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*           buffer = nullptr;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_BYTESTRING);

  result = cardano_cbor_reader_read_bytestring(reader, &buffer);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_buffer_get_size(buffer), 0);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_cbor_reader_read_bytestring, canReadEmptyIndefiniteArray)
{
  // Arrange
  const char* cbor_hex = "5f40ff";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*           buffer = nullptr;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_BYTESTRING);

  result = cardano_cbor_reader_read_bytestring(reader, &buffer);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_buffer_get_size(buffer), 0);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_cbor_reader_read_bytestring, canReadNonEmptyIndefiniteSizeByteString)
{
  // Arrange
  const char* cbor_hex = "5f41ab40ff";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*           buffer = nullptr;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_BYTESTRING);

  result = cardano_cbor_reader_read_bytestring(reader, &buffer);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_buffer_get_size(buffer), 1);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[0], 0xab);
  cardano_buffer_unref(&buffer);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);

  // Arrange
  cbor_hex = "5f41ab41bc40ff";

  reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  buffer = nullptr;

  // Act
  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_BYTESTRING);

  result = cardano_cbor_reader_read_bytestring(reader, &buffer);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_buffer_get_size(buffer), 2);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[0], 0xab);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[1], 0xbc);
  cardano_buffer_unref(&buffer);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);

  // Arrange
  cbor_hex = "5f584064676273786767746f6768646a7074657476746b636f6376796669647171676775726a687268716169697370717275656c687679707178656577707279667677584064676273786767746f6768646a7074657476746b636f6376796669647171676775726a687268716169697370717275656c687679707178656577707279667677584064676273786767746f6768646a7074657476746b636f6376796669647171676775726a687268716169697370717275656c687679707178656577707279667677584064676273786767746f6768646a7074657476746b636f6376796669647171676775726a687268716169697370717275656c687679707178656577707279667677ff";

  reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  buffer = nullptr;

  // Act
  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_BYTESTRING);

  result = cardano_cbor_reader_read_bytestring(reader, &buffer);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_buffer_get_size(buffer), 256);
  size_t hex_size = cardano_buffer_get_hex_size(buffer);
  char*  hex      = (char*)malloc(hex_size);

  cardano_error_t to_hex_result = cardano_buffer_to_hex(buffer, hex, hex_size);

  EXPECT_EQ(to_hex_result, CARDANO_SUCCESS);

  EXPECT_STREQ(hex, "64676273786767746f6768646a7074657476746b636f6376796669647171676775726a687268716169697370717275656c68767970717865657770727966767764676273786767746f6768646a7074657476746b636f6376796669647171676775726a687268716169697370717275656c68767970717865657770727966767764676273786767746f6768646a7074657476746b636f6376796669647171676775726a687268716169697370717275656c68767970717865657770727966767764676273786767746f6768646a7074657476746b636f6376796669647171676775726a687268716169697370717275656c687679707178656577707279667677");

  cardano_buffer_unref(&buffer);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  free(hex);
}

TEST(cardano_cbor_reader_read_uint, canReadUnsignedIntegers)
{
  verify_int("00", 0, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);
  verify_int("01", 1, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);
  verify_int("0a", 10, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);
  verify_int("17", 23, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);
  verify_int("1818", 24, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);
  verify_int("1819", 25, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);
  verify_int("1864", 100, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);
  verify_int("1903e8", 1000, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);
  verify_int("1a000f4240", 1000000, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);
  verify_int("1b000000e8d4a51000", 1000000000000, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);
  verify_int("18ff", 255, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);
  verify_int("190100", 256, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);
  verify_int("1affffffff", 4294967295, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);
  verify_int("1b7fffffffffffffff", 9223372036854775807, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);
  verify_int("1b0000000100000000", 4294967296, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);
  verify_int("19ffff", 65535, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);
  verify_int("1a00010000", 65536, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);
}

TEST(cardano_cbor_reader_read_uint, canReadNegativeIntegers)
{
  verify_int("20", -1, CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER);
  verify_int("29", -10, CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER);
  verify_int("37", -24, CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER);
  verify_int("3863", -100, CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER);
  verify_int("3903e7", -1000, CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER);
  verify_int("38ff", -256, CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER);
  verify_int("390100", -257, CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER);
  verify_int("39ffff", -65536, CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER);
  verify_int("3a00010000", -65537, CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER);
  verify_int("3affffffff", -4294967296, CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER);
  verify_int("3b0000000100000000", -4294967297, CARDANO_CBOR_READER_STATE_NEGATIVE_INTEGER);
}

TEST(cardano_cbor_reader_read_double, canReadHalfPresitionValues)
{
  verify_float("f90000", 0, CARDANO_CBOR_READER_STATE_HALF_PRECISION_FLOAT);
  verify_float("f93c00", 1, CARDANO_CBOR_READER_STATE_HALF_PRECISION_FLOAT);
  verify_float("f93e00", 1.5, CARDANO_CBOR_READER_STATE_HALF_PRECISION_FLOAT);
  verify_float("f98000", -0, CARDANO_CBOR_READER_STATE_HALF_PRECISION_FLOAT);
  verify_float("f93c00", 1, CARDANO_CBOR_READER_STATE_HALF_PRECISION_FLOAT);
  verify_float("f97bff", 65504, CARDANO_CBOR_READER_STATE_HALF_PRECISION_FLOAT);
  verify_float("f90001", 5.960464477539063e-8, CARDANO_CBOR_READER_STATE_HALF_PRECISION_FLOAT);
  verify_float("f90400", 0.00006103515625, CARDANO_CBOR_READER_STATE_HALF_PRECISION_FLOAT);
  verify_float("f9c400", -4, CARDANO_CBOR_READER_STATE_HALF_PRECISION_FLOAT);
  verify_float("f97c00", INFINITY, CARDANO_CBOR_READER_STATE_HALF_PRECISION_FLOAT);
  verify_float("f9fc00", -INFINITY, CARDANO_CBOR_READER_STATE_HALF_PRECISION_FLOAT);
}

TEST(cardano_cbor_reader_read_double, canSinglePresitionValues)
{
  verify_float("fa47c35000", 100000, CARDANO_CBOR_READER_STATE_SINGLE_PRECISION_FLOAT);
  verify_float("fa7f7fffff", 3.4028234663852886e+38, CARDANO_CBOR_READER_STATE_SINGLE_PRECISION_FLOAT);
  verify_float("fa7f800000", INFINITY, CARDANO_CBOR_READER_STATE_SINGLE_PRECISION_FLOAT);
  verify_float("faff800000", -INFINITY, CARDANO_CBOR_READER_STATE_SINGLE_PRECISION_FLOAT);
}

TEST(cardano_cbor_reader_read_double, canDoublePresitionValues)
{
  verify_float("fb3ff199999999999a", 1.1, CARDANO_CBOR_READER_STATE_DOUBLE_PRECISION_FLOAT);
  verify_float("fb7e37e43c8800759c", 1e300, CARDANO_CBOR_READER_STATE_DOUBLE_PRECISION_FLOAT);
  verify_float("fbc010666666666666", -4.1, CARDANO_CBOR_READER_STATE_DOUBLE_PRECISION_FLOAT);
  verify_float("fb7ff0000000000000", INFINITY, CARDANO_CBOR_READER_STATE_DOUBLE_PRECISION_FLOAT);
  verify_float("fbfff0000000000000", -INFINITY, CARDANO_CBOR_READER_STATE_DOUBLE_PRECISION_FLOAT);
}

TEST(cardano_cbor_reader_read_null, canReadNullValues)
{
  const char* cbor_hex = "f6";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*           buffer = nullptr;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_NULL);

  result = cardano_cbor_reader_read_null(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_boolean, canReadBooleanValues)
{
  const char* cbor_hex = "f4f5";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*           buffer = nullptr;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_BOOLEAN);

  bool value = false;
  result     = cardano_cbor_reader_read_boolean(reader, &value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(value, false);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_BOOLEAN);

  result = cardano_cbor_reader_read_boolean(reader, &value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(value, true);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_simple_value, canReadSimpleValues)
{
  const char* cbor_hex = "e0f4f5f6f7f820f8ff";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*           buffer = nullptr;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_SIMPLE_VALUE);

  cardano_cbor_simple_value_t value = CARDANO_CBOR_SIMPLE_VALUE_FALSE;
  result                            = cardano_cbor_reader_read_simple_value(reader, &value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(value, 0);

  result = cardano_cbor_reader_read_simple_value(reader, &value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(value, CARDANO_CBOR_SIMPLE_VALUE_FALSE);

  result = cardano_cbor_reader_read_simple_value(reader, &value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(value, CARDANO_CBOR_SIMPLE_VALUE_TRUE);

  result = cardano_cbor_reader_read_simple_value(reader, &value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(value, CARDANO_CBOR_SIMPLE_VALUE_NULL);

  result = cardano_cbor_reader_read_simple_value(reader, &value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(value, CARDANO_CBOR_SIMPLE_VALUE_UNDEFINED);

  result = cardano_cbor_reader_read_simple_value(reader, &value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(value, 32);

  result = cardano_cbor_reader_read_simple_value(reader, &value);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(value, 255);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_skip_value, canSkipAIndefiniteLengthWithoutDecoding)
{
  const char* cbor_hex = "845f41ab40ff456C6F72656D45697073756D45646F6C6F72";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*           buffer = nullptr;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_ARRAY);

  int64_t size = 0;
  result       = cardano_cbor_reader_read_start_array(reader, &size);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(size, 4);

  result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_read_encoded_value(reader, &buffer);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_buffer_get_size(buffer), 6);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[0], 0x45);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[1], 0x64);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[2], 0x6f);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[3], 0x6c);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[4], 0x6f);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[5], 0x72);
  cardano_buffer_unref(&buffer);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_END_ARRAY);

  result = cardano_cbor_reader_read_end_array(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_skip_value, canSkipAIndefiniteLengthElements)
{
  const char* cbor_hex = "8a9f182aff5f40ffa201020304bf6161614161626142616361436164614461656145ffc11a514b67b07f62616262626360ff1a00010000f9040038fff4";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*           buffer = nullptr;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_ARRAY);

  int64_t size = 0;
  result       = cardano_cbor_reader_read_start_array(reader, &size);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(size, 10);

  result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_END_ARRAY);

  result = cardano_cbor_reader_read_end_array(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_skip_value, canSkipAValueWithoutDecoding)
{
  const char* cbor_hex = "83656c6f72656d65697073756d65646f6c6f72";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*           buffer = nullptr;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_ARRAY);

  int64_t size = 0;
  result       = cardano_cbor_reader_read_start_array(reader, &size);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(size, 3);

  result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_read_textstring(reader, &buffer);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  char text[6] = { 0 };
  result       = cardano_buffer_to_str(buffer, text, 7);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_buffer_get_size(buffer), 5);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[0], 0x64);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[1], 0x6f);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[2], 0x6c);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[3], 0x6f);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[4], 0x72);
  EXPECT_STREQ(text, "dolor");

  cardano_buffer_unref(&buffer);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_END_ARRAY);

  result = cardano_cbor_reader_read_end_array(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_skip_value, canGetAValueWithoutDecoding)
{
  const char* cbor_hex = "83456C6F72656D45697073756D45646F6C6F72";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*           buffer = nullptr;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_ARRAY);

  int64_t size = 0;
  result       = cardano_cbor_reader_read_start_array(reader, &size);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(size, 3);

  result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_read_encoded_value(reader, &buffer);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_buffer_get_size(buffer), 6);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[0], 0x45);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[1], 0x64);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[2], 0x6f);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[3], 0x6c);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[4], 0x6f);
  EXPECT_EQ(cardano_buffer_get_data(buffer)[5], 0x72);
  cardano_buffer_unref(&buffer);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_END_ARRAY);

  result = cardano_cbor_reader_read_end_array(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_tag, canReadSingleTaggedValues)
{
  const char* cbor_hex = "c074323031332d30332d32315432303a30343a30305a";

  cardano_cbor_reader_t*      reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*           buffer   = nullptr;
  cardano_cbor_reader_state_t state    = CARDANO_CBOR_READER_STATE_UNDEFINED;
  char                        text[30] = { 0 };
  cardano_cbor_tag_t          tag      = CARDANO_CBOR_TAG_SELF_DESCRIBE_CBOR;

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_TAG);

  result = cardano_cbor_reader_read_tag(reader, &tag);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(tag, CARDANO_CBOR_TAG_DATE_TIME_STRING);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_TEXTSTRING);

  result = cardano_cbor_reader_read_textstring(reader, &buffer);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_buffer_to_str(buffer, text, 30);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  EXPECT_STREQ(text, "2013-03-21T20:04:00Z");

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_cbor_reader_read_tag, canReadSingleTaggedUnixValues)
{
  const char* cbor_hex = "c11a514b67b0";

  cardano_cbor_reader_t*      reader  = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  uint64_t                    seconds = 0;
  cardano_cbor_reader_state_t state   = CARDANO_CBOR_READER_STATE_UNDEFINED;
  cardano_cbor_tag_t          tag     = CARDANO_CBOR_TAG_SELF_DESCRIBE_CBOR;

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_TAG);

  result = cardano_cbor_reader_read_tag(reader, &tag);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(tag, CARDANO_CBOR_TAG_UNIX_TIME_SECONDS);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);

  result = cardano_cbor_reader_read_uint(reader, &seconds);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  EXPECT_EQ(seconds, 1363896240);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_tag, canReadUnsignedBignumValues)
{
  const char* cbor_hex = "c202";

  cardano_cbor_reader_t*      reader  = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  uint64_t                    seconds = 0;
  cardano_cbor_reader_state_t state   = CARDANO_CBOR_READER_STATE_UNDEFINED;
  cardano_cbor_tag_t          tag     = CARDANO_CBOR_TAG_SELF_DESCRIBE_CBOR;

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_TAG);

  result = cardano_cbor_reader_read_tag(reader, &tag);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(tag, CARDANO_CBOR_TAG_UNSIGNED_BIG_NUM);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER);

  result = cardano_cbor_reader_read_uint(reader, &seconds);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  EXPECT_EQ(seconds, 2);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_tag, canReadBase16Values)
{
  const char* cbor_hex = "d74401020304";

  cardano_cbor_reader_t*      reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*           buffer   = nullptr;
  cardano_cbor_reader_state_t state    = CARDANO_CBOR_READER_STATE_UNDEFINED;
  char                        text[30] = { 0 };
  cardano_cbor_tag_t          tag      = CARDANO_CBOR_TAG_SELF_DESCRIBE_CBOR;

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_TAG);

  result = cardano_cbor_reader_read_tag(reader, &tag);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(tag, 23);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_BYTESTRING);

  result = cardano_cbor_reader_read_bytestring(reader, &buffer);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_buffer_to_hex(buffer, text, 30);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  EXPECT_STREQ(text, "01020304");

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_cbor_reader_read_tag, canReadSingleTaggedUriValue)
{
  const char* cbor_hex = "d82076687474703a2f2f7777772e6578616d706c652e636f6d";

  cardano_cbor_reader_t*      reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*           buffer   = nullptr;
  cardano_cbor_reader_state_t state    = CARDANO_CBOR_READER_STATE_UNDEFINED;
  char                        text[30] = { 0 };
  cardano_cbor_tag_t          tag      = CARDANO_CBOR_TAG_SELF_DESCRIBE_CBOR;

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_TAG);

  result = cardano_cbor_reader_read_tag(reader, &tag);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(tag, 32);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_TEXTSTRING);

  result = cardano_cbor_reader_read_textstring(reader, &buffer);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_buffer_to_str(buffer, text, 30);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  EXPECT_STREQ(text, "http://www.example.com");

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_cbor_reader_get_bytes_remaining, canGetBytesRemaining)
{
  const char* cbor_hex = "d82076687474703a2f2f7777772e6578616d706c652e636f6d";

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  size_t                 size   = 0;

  cardano_error_t result = cardano_cbor_reader_get_bytes_remaining(reader, &size);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(size, 25);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_get_bytes_remaining, returnsNullIfReaderIsNull)
{
  size_t size = 0;

  cardano_error_t result = cardano_cbor_reader_get_bytes_remaining(nullptr, &size);
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_reader_get_bytes_remaining, returnsNullIfSizeIsNull)
{
  const char* cbor_hex = "d82076687474703a2f2f7777772e6578616d706c652e636f6d";

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  cardano_error_t result = cardano_cbor_reader_get_bytes_remaining(reader, nullptr);
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_get_remainder_bytes, canGetRemainderBytes)
{
  const char* cbor_hex = "d82076687474703a2f2f7777772e6578616d706c652e636f6d";

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*      buffer = nullptr;

  cardano_error_t result = cardano_cbor_reader_get_remainder_bytes(reader, &buffer);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_buffer_get_size(buffer), 25);

  cardano_cbor_reader_unref(&reader);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_cbor_reader_get_remainder_bytes, returnsNullIfReaderIsNull)
{
  cardano_buffer_t* buffer = nullptr;

  cardano_error_t result = cardano_cbor_reader_get_remainder_bytes(nullptr, &buffer);
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_reader_read_encoded_value, returnNullIfReaderIsNull)
{
  cardano_buffer_t* buffer = nullptr;

  cardano_error_t result = cardano_cbor_reader_read_encoded_value(nullptr, &buffer);
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_reader_read_encoded_value, returnNullIfBufferIsNull)
{
  const char* cbor_hex = "d82076687474703a2f2f7777772e6578616d706c652e636f6d";

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  cardano_error_t result = cardano_cbor_reader_read_encoded_value(reader, nullptr);
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_skip_value, returnsNullIfReaderIsNull)
{
  cardano_error_t result = cardano_cbor_reader_skip_value(nullptr);
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cbor_reader_get_remainder_bytes, returnsNullIfBufferIsNull)
{
  const char* cbor_hex = "d82076687474703a2f2f7777772e6578616d706c652e636f6d";

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  cardano_error_t result = cardano_cbor_reader_get_remainder_bytes(reader, nullptr);
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_tag, canReadNestedTaggedValues)
{
  const char* cbor_hex = "c0c0c074323031332d30332d32315432303a30343a30305a";

  cardano_cbor_reader_t*      reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*           buffer   = nullptr;
  char                        text[30] = { 0 };
  cardano_cbor_tag_t          tag      = CARDANO_CBOR_TAG_SELF_DESCRIBE_CBOR;
  cardano_cbor_reader_state_t state    = CARDANO_CBOR_READER_STATE_UNDEFINED;

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_TAG);

  result = cardano_cbor_reader_peek_tag(reader, &tag);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(tag, CARDANO_CBOR_TAG_DATE_TIME_STRING);

  result = cardano_cbor_reader_read_tag(reader, &tag);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(tag, CARDANO_CBOR_TAG_DATE_TIME_STRING);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_TAG);

  result = cardano_cbor_reader_read_tag(reader, &tag);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(tag, CARDANO_CBOR_TAG_DATE_TIME_STRING);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_TAG);

  result = cardano_cbor_reader_read_tag(reader, &tag);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(tag, CARDANO_CBOR_TAG_DATE_TIME_STRING);

  result = cardano_cbor_reader_read_textstring(reader, &buffer);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_buffer_to_str(buffer, text, 30);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  EXPECT_STREQ(text, "2013-03-21T20:04:00Z");

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_cbor_reader_read_textstring, canReadFixedLengthTextStrings)
{
  verify_text("60", "", CARDANO_CBOR_READER_STATE_TEXTSTRING);
  verify_text("6161", "a", CARDANO_CBOR_READER_STATE_TEXTSTRING);
  verify_text("6449455446", "IETF", CARDANO_CBOR_READER_STATE_TEXTSTRING);
  verify_text("62225c", "\"\\", CARDANO_CBOR_READER_STATE_TEXTSTRING);
  verify_text("62c3bc", "\u00FC", CARDANO_CBOR_READER_STATE_TEXTSTRING);
  verify_text("63e6b0b4", "\u6C34", CARDANO_CBOR_READER_STATE_TEXTSTRING);
  verify_text("62cebb", "\u03BB", CARDANO_CBOR_READER_STATE_TEXTSTRING);
}

TEST(cardano_cbor_reader_read_textstring, canReadIndefiniteLengthTextStrings)
{
  verify_text("7fff", "", CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_TEXTSTRING);
  verify_text("7f60ff", "", CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_TEXTSTRING);
  verify_text("7f62616260ff", "ab", CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_TEXTSTRING);
  verify_text("7f62616262626360ff", "abbc", CARDANO_CBOR_READER_STATE_START_INDEFINITE_LENGTH_TEXTSTRING);
}

TEST(cardano_cbor_reader_read_start_map, canReadEmptyMap)
{
  const char* cbor_hex = "a0";

  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  int64_t                     size   = 0;
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_MAP);

  result = cardano_cbor_reader_read_start_map(reader, &size);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(size, 0);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_END_MAP);

  result = cardano_cbor_reader_read_end_map(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_start_map, canReadFixedLengthMapsWithNumbers)
{
  cardano_cbor_reader_state_t state    = CARDANO_CBOR_READER_STATE_UNDEFINED;
  const char*                 cbor_hex = "a201020304";

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  std::string            json   = get_json_val(reader);

  EXPECT_STREQ(json.c_str(), "{\"1\":2,\"3\":4}");

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_start_map, canReadFixedLengthMapsWithStrings)
{
  cardano_cbor_reader_state_t state    = CARDANO_CBOR_READER_STATE_UNDEFINED;
  const char*                 cbor_hex = "a56161614161626142616361436164614461656145";

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  std::string            json   = get_json_val(reader);

  EXPECT_STREQ(json.c_str(), "{\"a\":A,\"b\":B,\"c\":C,\"d\":D,\"e\":E}");

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_start_map, canReadFixedLengthMapsWithMixedTypes)
{
  cardano_cbor_reader_state_t state    = CARDANO_CBOR_READER_STATE_UNDEFINED;
  const char*                 cbor_hex = "a3616161412002404101";

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  std::string            json   = get_json_val(reader);

  EXPECT_STREQ(json.c_str(), "{\"a\":A,\"-1\":2,\"\":01}");

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_start_map, canReadFixedLengthMapsWithNestedTypes)
{
  cardano_cbor_reader_state_t state    = CARDANO_CBOR_READER_STATE_UNDEFINED;
  const char*                 cbor_hex = "a26161a102036162a26178206179a1617a00";

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  std::string            json   = get_json_val(reader);

  EXPECT_STREQ(json.c_str(), "{\"a\":{\"2\":3},\"b\":{\"x\":-1,\"y\":{\"z\":0}}}");

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_start_map, canReadEmptyIndefiniteLengthMaps)
{
  cardano_cbor_reader_state_t state    = CARDANO_CBOR_READER_STATE_UNDEFINED;
  const char*                 cbor_hex = "bfff";

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  int64_t                size   = 0;

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_START_MAP);

  result = cardano_cbor_reader_read_start_map(reader, &size);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(size, -1);

  result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_END_MAP);

  result = cardano_cbor_reader_read_end_map(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_start_map, canReadIndefiniteLengthMapsWithStrings)
{
  cardano_cbor_reader_state_t state    = CARDANO_CBOR_READER_STATE_UNDEFINED;
  const char*                 cbor_hex = "bf6161614161626142616361436164614461656145ff";

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  std::string            json   = get_json_val(reader);

  EXPECT_STREQ(json.c_str(), "{\"a\":A,\"b\":B,\"c\":C,\"d\":D,\"e\":E}");

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_start_map, canReadIndefiniteLengthMapsWithMixedTypes)
{
  cardano_cbor_reader_state_t state    = CARDANO_CBOR_READER_STATE_UNDEFINED;
  const char*                 cbor_hex = "bf616161412002404101ff";

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  std::string            json   = get_json_val(reader);

  EXPECT_STREQ(json.c_str(), "{\"a\":A,\"-1\":2,\"\":01}");

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  cardano_cbor_reader_unref(&reader);
}

// UNIT TEST INTERNALS

TEST(_cbor_reader_peek_tag, canPeekTag)
{
  const char* cbor_hex = "c074323031332d30332d32315432303a30343a30305a";

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_cbor_tag_t     tag    = CARDANO_CBOR_TAG_SELF_DESCRIBE_CBOR;

  // Act
  cardano_error_t result = _cbor_reader_peek_tag(reader, &tag);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(tag, CARDANO_CBOR_TAG_DATE_TIME_STRING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_peek_tag, returnErrorIfReaderIsNull)
{
  cardano_cbor_tag_t tag = CARDANO_CBOR_TAG_SELF_DESCRIBE_CBOR;

  // Act
  cardano_error_t result = _cbor_reader_peek_tag(nullptr, &tag);
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(_cbor_reader_read_tag, canReadTag)
{
  const char* cbor_hex = "c074323031332d30332d32315432303a30343a30305a";

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_cbor_tag_t     tag    = CARDANO_CBOR_TAG_SELF_DESCRIBE_CBOR;

  // Act
  cardano_error_t result = _cbor_reader_read_tag(reader, &tag);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(tag, CARDANO_CBOR_TAG_DATE_TIME_STRING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_tag, returnErrorIfReaderIsNull)
{
  cardano_cbor_tag_t tag = CARDANO_CBOR_TAG_SELF_DESCRIBE_CBOR;

  // Act
  cardano_error_t result = _cbor_reader_read_tag(nullptr, &tag);
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(_cbor_reader_read_boolean, returnErrorIfInvalidInitialByte)
{
  const char*            cbor_hex = "40";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  bool value = false;

  // Act
  cardano_error_t result = _cbor_reader_read_boolean(reader, &value);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_boolean, returnErrorSimpleValueButAdditionalInfoIsNotBoolean)
{
  const char*            cbor_hex = "f8";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  bool value = false;

  // Act
  cardano_error_t result = _cbor_reader_read_boolean(reader, &value);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Not a boolean encoding");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_null, returnErrorIfInvalidInitialByte)
{
  const char*            cbor_hex = "40";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  // Act
  cardano_error_t result = _cbor_reader_read_null(reader);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_null, returnErrorSimpleValueButAdditionalInfoIsNotNull)
{
  const char*            cbor_hex = "f8";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  // Act
  cardano_error_t result = _cbor_reader_read_null(reader);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Not a null encoding");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_simple_value, returnErrorIfInvalidInitialByte)
{
  const char*                 cbor_hex = "40";
  cardano_cbor_reader_t*      reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_cbor_simple_value_t value    = CARDANO_CBOR_SIMPLE_VALUE_FALSE;

  // Act
  cardano_error_t result = _cbor_reader_read_simple_value(reader, &value);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_simple_value, returnErrorIfNotAValidSimpleValue)
{
  const char*                 cbor_hex = "ff";
  cardano_cbor_reader_t*      reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_cbor_simple_value_t value    = CARDANO_CBOR_SIMPLE_VALUE_FALSE;

  // Act
  cardano_error_t result = _cbor_reader_read_simple_value(reader, &value);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Not a simple value encoding");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_decode_unsigned_integer, returnErrorIfBufferIsNull)
{
  cardano_buffer_t* buffer     = nullptr;
  uint64_t          value      = 0;
  uint64_t          bytes_read = 0;

  // Act
  cardano_error_t result = _cbor_reader_decode_unsigned_integer(buffer, 0, &value, &bytes_read);
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(_cbor_reader_decode_unsigned_integer, returnErrorIfAdditionalInfoIs8BitsAndBufferTooSmall)
{
  const char*       cbor_hex   = "18";
  cardano_buffer_t* buffer     = cardano_buffer_from_hex(cbor_hex, strlen(cbor_hex));
  uint64_t          value      = 0;
  uint64_t          bytes_read = 0;

  // Act
  cardano_error_t result = _cbor_reader_decode_unsigned_integer(buffer, 0xf8, &value, &bytes_read);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(_cbor_reader_decode_unsigned_integer, returnErrorIfAdditionalInfoIs16BitsAndBufferTooSmall)
{
  const char*       cbor_hex   = "18";
  cardano_buffer_t* buffer     = cardano_buffer_from_hex(cbor_hex, strlen(cbor_hex));
  uint64_t          value      = 0;
  uint64_t          bytes_read = 0;

  // Act
  cardano_error_t result = _cbor_reader_decode_unsigned_integer(buffer, 0xf9, &value, &bytes_read);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(_cbor_reader_decode_unsigned_integer, returnErrorIfAdditionalInfoIs32BitsAndBufferTooSmall)
{
  const char*       cbor_hex   = "18";
  cardano_buffer_t* buffer     = cardano_buffer_from_hex(cbor_hex, strlen(cbor_hex));
  uint64_t          value      = 0;
  uint64_t          bytes_read = 0;

  // Act
  cardano_error_t result = _cbor_reader_decode_unsigned_integer(buffer, 0xfa, &value, &bytes_read);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(_cbor_reader_decode_unsigned_integer, returnErrorIfAdditionalInfoIs64BitsAndBufferTooSmall)
{
  const char*       cbor_hex   = "18";
  cardano_buffer_t* buffer     = cardano_buffer_from_hex(cbor_hex, strlen(cbor_hex));
  uint64_t          value      = 0;
  uint64_t          bytes_read = 0;

  // Act
  cardano_error_t result = _cbor_reader_decode_unsigned_integer(buffer, 0xfb, &value, &bytes_read);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(_cbor_reader_decode_unsigned_integer, returnErrorIfAdditionalInfoIsUnknown)
{
  const char*       cbor_hex   = "18";
  cardano_buffer_t* buffer     = cardano_buffer_from_hex(cbor_hex, strlen(cbor_hex));
  uint64_t          value      = 0;
  uint64_t          bytes_read = 0;

  // Act
  cardano_error_t result = _cbor_reader_decode_unsigned_integer(buffer, 0xff, &value, &bytes_read);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(_cbor_reader_read_double, returnErrorIfReaderIsNull)
{
  cardano_cbor_reader_t* reader = nullptr;
  double                 value  = 0;

  // Act
  cardano_error_t result = _cbor_reader_read_double(reader, &value);
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(_cbor_reader_read_double, returnErrorIfValueIsNull)
{
  const char*            cbor_hex = "f97e00";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  // Act
  cardano_error_t result = _cbor_reader_read_double(reader, nullptr);
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_double, returnErrorIfAdditionalInfoIs16BitsAndBufferTooSmall)
{
  const char*            cbor_hex = "f9";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  double                 value    = 0;

  // Act
  cardano_error_t result = _cbor_reader_read_double(reader, &value);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_double, returnErrorIfAdditionalInfoIs32BitsAndBufferTooSmall)
{
  const char*            cbor_hex = "fa";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  double                 value    = 0;

  // Act
  cardano_error_t result = _cbor_reader_read_double(reader, &value);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_double, returnErrorIfAdditionalInfoIs64BitsAndBufferTooSmall)
{
  const char*            cbor_hex = "fb";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  double                 value    = 0;

  // Act
  cardano_error_t result = _cbor_reader_read_double(reader, &value);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_double, returnErrorIfAdditionalInfoIsUnknown)
{
  const char*            cbor_hex = "ff";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  double                 value    = 0;

  // Act
  cardano_error_t result = _cbor_reader_read_double(reader, &value);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_int, returnErrorIfReaderIsNull)
{
  cardano_cbor_reader_t* reader = nullptr;
  int64_t                value  = 0;

  // Act
  cardano_error_t result = _cbor_reader_read_int(reader, &value);
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(_cbor_reader_read_int, returnErrorIfMajorTypeIsNotAnInt)
{
  const char*            cbor_hex = "f8";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  int64_t                value    = 0;

  // Act
  cardano_error_t result = _cbor_reader_read_int(reader, &value);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Reader type mismatch, expected 0 or 1 but got 7.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_uint, returnErrorIfReaderIsNull)
{
  cardano_cbor_reader_t* reader = nullptr;
  uint64_t               value  = 0;

  // Act
  cardano_error_t result = _cbor_reader_read_uint(reader, &value);
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(_cbor_reader_read_uint, returnErrorIfMajorTypeIsNotAnInt)
{
  const char*            cbor_hex = "f8";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  uint64_t               value    = 0;

  // Act
  cardano_error_t result = _cbor_reader_read_uint(reader, &value);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Reader type mismatch, expected 0 but got 7.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_uint, returnErrorIfMajorTypeIsSignedInt)
{
  const char*            cbor_hex = "20";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  uint64_t               value    = 0;

  // Act
  cardano_error_t result = _cbor_reader_read_uint(reader, &value);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Integer overflow.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_start_indefinite_length_string, returnErrorIfValueIsNull)
{
  // Act
  cardano_error_t result = _cbor_reader_read_start_indefinite_length_string(nullptr, CARDANO_CBOR_MAJOR_TYPE_UTF8_STRING);
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(_cbor_reader_read_start_indefinite_length_string, returnsErrorIfReaderIsEmpty)
{
  const char*            cbor_hex = "ff";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  reader->offset                  = 1;

  // Act
  cardano_error_t result = _cbor_reader_read_start_indefinite_length_string(reader, CARDANO_CBOR_MAJOR_TYPE_UTF8_STRING);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_start_indefinite_length_string, returnsErrorIfInvalidInitialByte)
{
  const char*            cbor_hex = "F9";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  // Act
  cardano_error_t result = _cbor_reader_read_start_indefinite_length_string(reader, CARDANO_CBOR_MAJOR_TYPE_TAG);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_end_indefinite_length_string, returnsErrorIfInvalidInitialByte)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  // Act
  cardano_error_t result = _cbor_reader_read_end_indefinite_length_string(reader, CARDANO_CBOR_MAJOR_TYPE_TAG);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_end_indefinite_length_string, returnsErrorIfInvalidIndefiniteLengthBreak)
{
  const char*            cbor_hex = "F2";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  // Act
  cardano_error_t result = _cbor_reader_read_end_indefinite_length_string(reader, CARDANO_CBOR_MAJOR_TYPE_TAG);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_end_indefinite_length_string, returnsErrorIfThereAreNoMoreDatatoRead)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  reader->current_frame.definite_length = 0;

  // Act
  cardano_error_t result = _cbor_reader_read_end_indefinite_length_string(reader, CARDANO_CBOR_MAJOR_TYPE_UTF8_STRING);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_indefinite_length_concatenated, returnsErrorIfEventualMemoryAllocationFails)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*      buffer   = nullptr;
  size_t                 size     = 0;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = _cbor_reader_read_indefinite_length_concatenated(reader, &buffer, &size);
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(_cbor_reader_read_indefinite_length_concatenated, returnsErrorIfMemoryAllocationFails)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*      buffer   = nullptr;
  size_t                 size     = 0;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_two_malloc, realloc, free);

  // Act
  cardano_error_t result = _cbor_reader_read_indefinite_length_concatenated(reader, &buffer, &size);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(_cbor_reader_read_start_array, returnsErrorIfInvalidInitialByte)
{
  const char*            cbor_hex = "4101";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  // Act
  int64_t         size   = 0;
  cardano_error_t result = _cbor_reader_read_start_array(reader, &size);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_start_array, returnsErrorIfMemoryAllocationFails)
{
  const char*            cbor_hex = "8101";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  int64_t         size   = 0;
  cardano_error_t result = _cbor_reader_read_start_array(reader, &size);
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(_cbor_reader_read_start_array, returnsErrorIfEventualMemoryAllocationFails)
{
  const char*            cbor_hex = "8101";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  int64_t         size   = 0;
  cardano_error_t result = _cbor_reader_read_start_array(reader, &size);
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(_cbor_reader_read_start_array, returnsErrorIfMemoryAllocationFailsReadingIndfiniteArray)
{
  const char*            cbor_hex = "9fff";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  int64_t         size   = 0;
  cardano_error_t result = _cbor_reader_read_start_array(reader, &size);
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(_cbor_reader_read_end_array, returnsErrorIfInvalidInitialByte)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  // Act
  cardano_error_t result = _cbor_reader_read_end_array(reader);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_start_map, returnsErrorIfInvalidInitialByte)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  // Act
  int64_t         size   = 0;
  cardano_error_t result = _cbor_reader_read_start_map(reader, &size);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_start_map, returnsErrorIfMemoryAllocationFails)
{
  const char*            cbor_hex = "bfff";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  int64_t         size   = 0;
  cardano_error_t result = _cbor_reader_read_start_map(reader, &size);
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(_cbor_reader_read_end_map, returnsErrorIfUnevenKeyValuePairs)
{
  const char*            cbor_hex = "ff";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  reader->current_frame.items_read = 1;

  // Act
  cardano_error_t result = _cbor_reader_read_end_map(reader);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_start_map, returnsErrorIfEventualMemoryAllocationFails)
{
  const char*            cbor_hex = "a26161a102036162a26178206179a1617a00";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);
  reader->current_frame.definite_length = 1;

  // Act
  int64_t         size   = 0;
  cardano_error_t result = _cbor_reader_read_start_map(reader, &size);
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(_cbor_reader_read_end_map, returnsErrorIfInvalidInitialByte)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  // Act
  cardano_error_t result = _cbor_reader_read_end_map(reader);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_read_string, returnsErrorIfInvalidInitialByte)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  // Act
  cardano_buffer_t* buffer = NULL;
  cardano_error_t   result = _cbor_reader_read_string(reader, CARDANO_CBOR_MAJOR_TYPE_TAG, &buffer);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_pop_data_item, returnsErrorIfCurrentTypeDoesntMatchPopExpectedType)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, 1);

  // Act
  cardano_error_t result = _cbor_reader_pop_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_TAG);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_pop_data_item, returnsErrorIfInvalidLenght)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, 1);

  // Act
  cardano_error_t result = _cbor_reader_pop_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_pop_data_item, returnsErrorIfIsTagContext)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, -1);
  reader->is_tag_context = true;

  // Act
  cardano_error_t result = _cbor_reader_pop_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_peek_initial_byte, returnsErrorIfAlreadyAtEndOfBuffer)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, -1);
  reader->offset = 1;

  // Act
  uint8_t         initial_byte = 0;
  cardano_error_t result       = _cbor_reader_peek_initial_byte(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, &initial_byte);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_peek_initial_byte, returnsErrorIfAlreadyAtEndOfIndefiniteArray)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, -1);
  reader->offset                        = 1;
  reader->current_frame.type            = CARDANO_CBOR_MAJOR_TYPE_UNDEFINED;
  reader->current_frame.definite_length = -1;

  // Act
  uint8_t         initial_byte = 0;
  cardano_error_t result       = _cbor_reader_peek_initial_byte(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, &initial_byte);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_peek_initial_byte, returnsErrorIfIndefiniteLengthStringContainsInvalidItems)
{
  const char*            cbor_hex = "F9";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, -1);
  reader->current_frame.type            = CARDANO_CBOR_MAJOR_TYPE_UTF8_STRING;
  reader->current_frame.definite_length = -1;

  // Act
  uint8_t         initial_byte = 0;
  cardano_error_t result       = _cbor_reader_peek_initial_byte(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, &initial_byte);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_advance_buffer, returnsErrorIfBufferOutOfBounds)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  // Act
  cardano_error_t result = _cbor_reader_advance_buffer(reader, 10);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_skip_next_node, returnsErrorIfInvalidState)
{
  const char*            cbor_hex = "F9";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, -1);
  reader->current_frame.type            = CARDANO_CBOR_MAJOR_TYPE_BYTE_STRING;
  reader->current_frame.definite_length = 1;

  // Act
  size_t          depth  = 0;
  cardano_error_t result = _cbor_reader_skip_next_node(reader, &depth);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_peek_state, returnErrorIfReaderIsNull)
{
  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  // Act
  cardano_error_t result = _cbor_reader_peek_state(nullptr, &state);
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(_cbor_reader_peek_state, returnErrorIfStateIsNull)
{
  const char*            cbor_hex = "F9";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  // Act
  cardano_error_t result = _cbor_reader_peek_state(reader, nullptr);
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_peek_state, returnStateFinishesIfNoMoreItems)
{
  const char*            cbor_hex = "F9";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, -1);
  reader->current_frame.type            = CARDANO_CBOR_MAJOR_TYPE_UNDEFINED;
  reader->current_frame.definite_length = 1;
  reader->current_frame.items_read      = 1;

  // Act
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;
  cardano_error_t             result = _cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(state, CARDANO_CBOR_READER_STATE_FINISHED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_peek_state, returnErrorIfInvalidEndMarker)
{
  const char*            cbor_hex = "F9";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, -1);
  reader->current_frame.type            = CARDANO_CBOR_MAJOR_TYPE_TAG;
  reader->current_frame.definite_length = 1;
  reader->current_frame.items_read      = 1;

  // Act
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;
  cardano_error_t             result = _cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_peek_state, returnErrorIfTagNotFollowedByValue)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, -1);
  reader->current_frame.type = CARDANO_CBOR_MAJOR_TYPE_TAG;
  reader->is_tag_context     = true;

  // Act
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;
  cardano_error_t             result = _cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_peek_state, returnErrorIfUnexpectedBreakByte)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, -1);
  reader->current_frame.type = CARDANO_CBOR_MAJOR_TYPE_UNDEFINED;

  // Act
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;
  cardano_error_t             result = _cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_peek_state, returnErrorIfUnexpectedMapSize)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, -1);
  reader->current_frame.type       = CARDANO_CBOR_MAJOR_TYPE_MAP;
  reader->current_frame.items_read = 1;

  // Act
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;
  cardano_error_t             result = _cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_peek_state, returnErrorIfUnexpectedEndOfIndefiniteSizeElement)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, -1);
  reader->current_frame.type = CARDANO_CBOR_MAJOR_TYPE_TAG;

  // Act
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;
  cardano_error_t             result = _cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_peek_state, returnErrorIfUnexpectedBreakByteInIndefiniteLengthItem)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, -1);
  reader->current_frame.definite_length = 1;

  // Act
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;
  cardano_error_t             result = _cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_peek_state, returnErrorIfUnexpectedEndOfBufferDueToBufferOverflow)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, -1);
  reader->current_frame.type = CARDANO_CBOR_MAJOR_TYPE_TAG;
  reader->offset             = 5;

  // Act
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;
  cardano_error_t             result = _cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(_cbor_reader_peek_state, returnErrorIfUnexpectedEndOfBuffer)
{
  const char*            cbor_hex = "FF";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  _cbor_reader_push_data_item(reader, CARDANO_CBOR_MAJOR_TYPE_ARRAY, -1);
  reader->current_frame.type = CARDANO_CBOR_MAJOR_TYPE_TAG;
  reader->offset             = 1;

  // Act
  cardano_cbor_reader_state_t state  = CARDANO_CBOR_READER_STATE_UNDEFINED;
  cardano_error_t             result = _cbor_reader_peek_state(reader, &state);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_bytestring, returnsErrorIfBufferOverflow)
{
  // Arrange
  const char*            cbor_hex   = "4240";
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*      bytestring = NULL;

  // Act
  cardano_error_t result = cardano_cbor_reader_read_bytestring(reader, &bytestring);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_read_simple_value, returnsErrorIfBufferOverflow)
{
  // Arrange
  const char*            cbor_hex   = "f8";
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));
  cardano_buffer_t*      bytestring = NULL;

  // Act
  cardano_cbor_simple_value_t value  = CARDANO_CBOR_SIMPLE_VALUE_FALSE;
  cardano_error_t             result = cardano_cbor_reader_read_simple_value(reader, &value);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cbor_reader_skip_value, returnErrorIfInvalidDefiniteLenghtArrayLength)
{
  const char* cbor_hex = "68d8618543a10a0fa054541a69735f09305f5f5f5f5f5f5f5f5f5f5f605f5f5f5f5bfffffffffffffff4ff5f5f5fffffffffffffff5bffffffffffffffffffffffff3dffff78610015c0";

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor_hex, strlen(cbor_hex));

  cardano_error_t result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_cbor_reader_skip_value(reader);
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  cardano_cbor_reader_unref(&reader);
}
