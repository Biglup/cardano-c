/**
 * \file account_balance_intervals_map.cpp
 *
 * \author angel.castillo
 * \date   Jul 16, 2026
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

#include <cardano/error.h>

#include <cardano/common/credential.h>
#include <cardano/transaction_body/account_balance_interval.h>
#include <cardano/transaction_body/account_balance_intervals_map.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* KEY_HASH_A_HEX  = "00112233445566778899aabbccddeeff00112233445566778899aabb";
static const char* KEY_HASH_B_HEX  = "ffeeddccbbaa99887766554433221100ffeeddccbbaa998877665544";
static const char* SCRIPT_HASH_HEX = "aabbccddeeff00112233445566778899aabbccddeeff001122334455";

static const char* KEY_HASH_A_CREDENTIAL_CBOR  = "8200581c00112233445566778899aabbccddeeff00112233445566778899aabb";
static const char* KEY_HASH_B_CREDENTIAL_CBOR  = "8200581cffeeddccbbaa99887766554433221100ffeeddccbbaa998877665544";
static const char* SCRIPT_HASH_CREDENTIAL_CBOR = "8201581caabbccddeeff00112233445566778899aabbccddeeff001122334455";

static const char* BOTH_BOUNDS_INTERVAL_CBOR = "821864191388";
static const char* LOWER_ONLY_INTERVAL_CBOR  = "821901f4f6";
static const char* UPPER_ONLY_INTERVAL_CBOR  = "82f6192710";

static const char* CBOR           = "a38200581c00112233445566778899aabbccddeeff00112233445566778899aabb8218641913888200581cffeeddccbbaa99887766554433221100ffeeddccbbaa998877665544821901f4f68201581caabbccddeeff00112233445566778899aabbccddeeff00112233445582f6192710";
static const char* REVERSED_CBOR  = "a28200581cffeeddccbbaa99887766554433221100ffeeddccbbaa998877665544821901f4f68200581c00112233445566778899aabbccddeeff00112233445566778899aabb821864191388";
static const char* EMPTY_MAP_CBOR = "a0";
static const char* DUPLICATE_CBOR = "a28200581c00112233445566778899aabbccddeeff00112233445566778899aabb8218641913888200581c00112233445566778899aabbccddeeff00112233445566778899aabb821901f4f6";

/**
 * Creates a new default instance of the credential.
 * @return A new instance of the credential.
 */
static cardano_credential_t*
new_default_credential(const char* cbor)
{
  cardano_credential_t*  credential = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_credential_from_cbor(reader, &credential);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_credential_unref(&credential);
    return nullptr;
  }

  return credential;
}

/**
 * Creates a new default instance of the account balance interval.
 * @return A new instance of the account balance interval.
 */
static cardano_account_balance_interval_t*
new_default_account_balance_interval(const char* cbor)
{
  cardano_account_balance_interval_t* account_balance_interval = nullptr;
  cardano_cbor_reader_t*              reader                   = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_account_balance_interval_from_cbor(reader, &account_balance_interval);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_account_balance_interval_unref(&account_balance_interval);
    return nullptr;
  }

  return account_balance_interval;
}

/**
 * Decodes the given CBOR hex string into an account balance intervals map.
 * @return A new instance of the account balance intervals map.
 */
static cardano_account_balance_intervals_map_t*
new_default_account_balance_intervals_map(const char* cbor)
{
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_cbor_reader_t*                   reader                        = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
    return nullptr;
  }

  return account_balance_intervals_map;
}

/**
 * Encodes the given account balance intervals map to a CBOR hex string.
 * @return The CBOR hex string. The caller must free the returned string.
 */
static char*
encode_account_balance_intervals_map(cardano_account_balance_intervals_map_t* account_balance_intervals_map)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_account_balance_intervals_map_to_cbor(account_balance_intervals_map, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size    = cardano_cbor_writer_get_hex_size(writer);
  char*        actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return actual_cbor;
}

/**
 * Encodes the given account balance interval to a CBOR hex string.
 * @return The CBOR hex string. The caller must free the returned string.
 */
static char*
encode_account_balance_interval(cardano_account_balance_interval_t* account_balance_interval)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_account_balance_interval_to_cbor(account_balance_interval, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size    = cardano_cbor_writer_get_hex_size(writer);
  char*        actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return actual_cbor;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_account_balance_intervals_map_new, canCreateAccountBalanceIntervalsMap)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_new, returnsErrorIfAccountBalanceIntervalsMapIsNull)
{
  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_account_balance_intervals_map_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(account_balance_intervals_map, (cardano_account_balance_intervals_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_account_balance_intervals_map_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(account_balance_intervals_map, (cardano_account_balance_intervals_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_account_balance_intervals_map_from_cbor, canDeserializeAccountBalanceIntervalsMap)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_cbor_reader_t*                   reader                        = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));
  EXPECT_EQ(cardano_account_balance_intervals_map_get_length(account_balance_intervals_map), 3);

  cardano_credential_t*               key1   = nullptr;
  cardano_credential_t*               key3   = nullptr;
  cardano_account_balance_interval_t* value1 = nullptr;
  cardano_account_balance_interval_t* value3 = nullptr;

  EXPECT_EQ(cardano_account_balance_intervals_map_get_key_value_at(account_balance_intervals_map, 0, &key1, &value1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_account_balance_intervals_map_get_key_value_at(account_balance_intervals_map, 2, &key3, &value3), CARDANO_SUCCESS);

  cardano_credential_type_t type = CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH;
  EXPECT_EQ(cardano_credential_get_type(key1, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  EXPECT_STREQ(cardano_credential_get_hash_hex(key1), KEY_HASH_A_HEX);

  const uint64_t* inclusive_lower_bound = cardano_account_balance_interval_get_inclusive_lower_bound(value1);
  const uint64_t* exclusive_upper_bound = cardano_account_balance_interval_get_exclusive_upper_bound(value1);

  ASSERT_THAT(inclusive_lower_bound, testing::Not((const uint64_t*)nullptr));
  ASSERT_THAT(exclusive_upper_bound, testing::Not((const uint64_t*)nullptr));
  EXPECT_EQ(*inclusive_lower_bound, 100);
  EXPECT_EQ(*exclusive_upper_bound, 5000);

  EXPECT_EQ(cardano_credential_get_type(key3, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);
  EXPECT_STREQ(cardano_credential_get_hash_hex(key3), SCRIPT_HASH_HEX);

  char* interval_cbor = encode_account_balance_interval(value3);
  EXPECT_STREQ(interval_cbor, UPPER_ONLY_INTERVAL_CBOR);

  // Cleanup
  free(interval_cbor);
  cardano_credential_unref(&key1);
  cardano_credential_unref(&key3);
  cardano_account_balance_interval_unref(&value1);
  cardano_account_balance_interval_unref(&value3);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_intervals_map_from_cbor, returnErrorIfEmptyMap)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_cbor_reader_t*                   reader                        = cardano_cbor_reader_from_hex(EMPTY_MAP_CBOR, strlen(EMPTY_MAP_CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_MAP_SIZE);
  EXPECT_EQ(account_balance_intervals_map, (cardano_account_balance_intervals_map_t*)nullptr);
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'account_balance_intervals_map', the map must not be empty.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_intervals_map_from_cbor, returnErrorIfDuplicatedKey)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_cbor_reader_t*                   reader                        = cardano_cbor_reader_from_hex(DUPLICATE_CBOR, strlen(DUPLICATE_CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_EQ(account_balance_intervals_map, (cardano_account_balance_intervals_map_t*)nullptr);
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'account_balance_intervals_map', the map must not contain duplicated keys.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_intervals_map_from_cbor, returnErrorIfNotAMap)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_cbor_reader_t*                   reader                        = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_intervals_map_from_cbor, returnErrorIfInvalidKey)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_cbor_reader_t*                   reader                        = cardano_cbor_reader_from_hex("a101821864191388", 16);

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);
  EXPECT_EQ(account_balance_intervals_map, (cardano_account_balance_intervals_map_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_intervals_map_from_cbor, returnErrorIfInvalidValue)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_cbor_reader_t*                   reader                        = cardano_cbor_reader_from_hex("a18200581c00112233445566778899aabbccddeeff00112233445566778899aabbff", 68);

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);

  // Assert
  EXPECT_THAT(error, testing::Not(CARDANO_SUCCESS));
  EXPECT_EQ(account_balance_intervals_map, (cardano_account_balance_intervals_map_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_intervals_map_from_cbor, returnErrorIfValueHasBothBoundsNil)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_cbor_reader_t*                   reader                        = cardano_cbor_reader_from_hex("a18200581c00112233445566778899aabbccddeeff00112233445566778899aabb82f6f6", 72);

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(account_balance_intervals_map, (cardano_account_balance_intervals_map_t*)nullptr);
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Both interval bounds cannot be nil.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_intervals_map_from_cbor, returnErrorIfAccountBalanceIntervalsMapIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_intervals_map_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(nullptr, &account_balance_intervals_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_account_balance_intervals_map_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_cbor_reader_t*                   reader                        = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(account_balance_intervals_map, (cardano_account_balance_intervals_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_account_balance_intervals_map_to_cbor, canRoundTripAllIntervalShapes)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = new_default_account_balance_intervals_map(CBOR);
  ASSERT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  // Act
  char* actual_cbor = encode_account_balance_intervals_map(account_balance_intervals_map);

  // Assert
  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  free(actual_cbor);
}

TEST(cardano_account_balance_intervals_map_to_cbor, preservesKeyInsertionOrder)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = new_default_account_balance_intervals_map(REVERSED_CBOR);
  ASSERT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  cardano_credential_t* key = nullptr;
  EXPECT_EQ(cardano_account_balance_intervals_map_get_key_at(account_balance_intervals_map, 0, &key), CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_credential_get_hash_hex(key), KEY_HASH_B_HEX);

  // Act
  char* actual_cbor = encode_account_balance_intervals_map(account_balance_intervals_map);

  // Assert
  EXPECT_STREQ(actual_cbor, REVERSED_CBOR);

  // Cleanup
  cardano_credential_unref(&key);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  free(actual_cbor);
}

TEST(cardano_account_balance_intervals_map_to_cbor, canSerializeManuallyBuiltMap)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;

  cardano_error_t error = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* key_a = new_default_credential(KEY_HASH_A_CREDENTIAL_CBOR);
  cardano_credential_t* key_b = new_default_credential(KEY_HASH_B_CREDENTIAL_CBOR);
  cardano_credential_t* key_s = new_default_credential(SCRIPT_HASH_CREDENTIAL_CBOR);

  cardano_account_balance_interval_t* both_bounds_interval = new_default_account_balance_interval(BOTH_BOUNDS_INTERVAL_CBOR);
  cardano_account_balance_interval_t* lower_only_interval  = new_default_account_balance_interval(LOWER_ONLY_INTERVAL_CBOR);
  cardano_account_balance_interval_t* upper_only_interval  = new_default_account_balance_interval(UPPER_ONLY_INTERVAL_CBOR);

  EXPECT_EQ(cardano_account_balance_intervals_map_insert(account_balance_intervals_map, key_a, both_bounds_interval), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_account_balance_intervals_map_insert(account_balance_intervals_map, key_b, lower_only_interval), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_account_balance_intervals_map_insert(account_balance_intervals_map, key_s, upper_only_interval), CARDANO_SUCCESS);

  // Act
  char* actual_cbor = encode_account_balance_intervals_map(account_balance_intervals_map);

  // Assert
  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_credential_unref(&key_a);
  cardano_credential_unref(&key_b);
  cardano_credential_unref(&key_s);
  cardano_account_balance_interval_unref(&both_bounds_interval);
  cardano_account_balance_interval_unref(&lower_only_interval);
  cardano_account_balance_interval_unref(&upper_only_interval);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  free(actual_cbor);
}

TEST(cardano_account_balance_intervals_map_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_account_balance_intervals_map_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;

  cardano_error_t error = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_account_balance_intervals_map_to_cbor(account_balance_intervals_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_length, returnsZeroIfAccountBalanceIntervalsMapIsNull)
{
  // Act
  size_t length = cardano_account_balance_intervals_map_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_account_balance_intervals_map_get_length, returnsZeroIfAccountBalanceIntervalsMapIsEmpty)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_account_balance_intervals_map_get_length(account_balance_intervals_map);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get, returnsErrorIfAccountBalanceIntervalsMapIsNull)
{
  // Arrange
  cardano_credential_t*               key      = new_default_credential(KEY_HASH_A_CREDENTIAL_CBOR);
  cardano_account_balance_interval_t* interval = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_get(nullptr, key, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&key);
}

TEST(cardano_account_balance_intervals_map_get, returnsErrorIfKeyIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval = nullptr;

  // Act
  error = cardano_account_balance_intervals_map_get(account_balance_intervals_map, nullptr, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get, returnsErrorIfElementIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* key = new_default_credential(KEY_HASH_A_CREDENTIAL_CBOR);

  // Act
  error = cardano_account_balance_intervals_map_get(account_balance_intervals_map, key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&key);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get, returnsErrorIfElementNotFound)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = new_default_account_balance_intervals_map(REVERSED_CBOR);
  ASSERT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  cardano_credential_t*               key      = new_default_credential(SCRIPT_HASH_CREDENTIAL_CBOR);
  cardano_account_balance_interval_t* interval = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_get(account_balance_intervals_map, key, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_credential_unref(&key);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get, returnsIntervalForCredential)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = new_default_account_balance_intervals_map(CBOR);
  ASSERT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  cardano_credential_t*               key      = new_default_credential(KEY_HASH_B_CREDENTIAL_CBOR);
  cardano_account_balance_interval_t* interval = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_get(account_balance_intervals_map, key, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(interval, testing::Not((cardano_account_balance_interval_t*)nullptr));

  char* interval_cbor = encode_account_balance_interval(interval);
  EXPECT_STREQ(interval_cbor, LOWER_ONLY_INTERVAL_CBOR);

  // Cleanup
  free(interval_cbor);
  cardano_credential_unref(&key);
  cardano_account_balance_interval_unref(&interval);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_insert, returnsErrorIfAccountBalanceIntervalsMapIsNull)
{
  // Arrange
  cardano_credential_t*               key      = new_default_credential(KEY_HASH_A_CREDENTIAL_CBOR);
  cardano_account_balance_interval_t* interval = new_default_account_balance_interval(BOTH_BOUNDS_INTERVAL_CBOR);

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_insert(nullptr, key, interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&key);
  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_account_balance_intervals_map_insert, returnsErrorIfKeyIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval = new_default_account_balance_interval(BOTH_BOUNDS_INTERVAL_CBOR);

  // Act
  error = cardano_account_balance_intervals_map_insert(account_balance_intervals_map, nullptr, interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_account_balance_interval_unref(&interval);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_insert, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* key = new_default_credential(KEY_HASH_A_CREDENTIAL_CBOR);

  // Act
  error = cardano_account_balance_intervals_map_insert(account_balance_intervals_map, key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&key);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_insert, returnsErrorIfKeyIsDuplicated)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t*               key      = new_default_credential(KEY_HASH_A_CREDENTIAL_CBOR);
  cardano_account_balance_interval_t* interval = new_default_account_balance_interval(BOTH_BOUNDS_INTERVAL_CBOR);

  // Act
  EXPECT_EQ(cardano_account_balance_intervals_map_insert(account_balance_intervals_map, key, interval), CARDANO_SUCCESS);
  error = cardano_account_balance_intervals_map_insert(account_balance_intervals_map, key, interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_EQ(cardano_account_balance_intervals_map_get_length(account_balance_intervals_map), 1);

  // Cleanup
  cardano_credential_unref(&key);
  cardano_account_balance_interval_unref(&interval);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_insert, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t*               key      = new_default_credential(KEY_HASH_A_CREDENTIAL_CBOR);
  cardano_account_balance_interval_t* interval = new_default_account_balance_interval(BOTH_BOUNDS_INTERVAL_CBOR);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_account_balance_intervals_map_insert(account_balance_intervals_map, key, interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_credential_unref(&key);
  cardano_account_balance_interval_unref(&interval);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_keys, returnsErrorIfAccountBalanceIntervalsMapIsNull)
{
  // Arrange
  cardano_credential_set_t* keys = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_get_keys(nullptr, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_account_balance_intervals_map_get_keys, returnsErrorIfKeysIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_account_balance_intervals_map_get_keys(account_balance_intervals_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_keys, returnsEmptySetIfNoElements)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_set_t* keys = nullptr;

  // Act
  error = cardano_account_balance_intervals_map_get_keys(account_balance_intervals_map, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_credential_set_get_length(keys), 0);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  cardano_credential_set_unref(&keys);
}

TEST(cardano_account_balance_intervals_map_get_keys, returnsTheKeys)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = new_default_account_balance_intervals_map(CBOR);
  ASSERT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  cardano_credential_set_t* keys = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_get_keys(account_balance_intervals_map, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_credential_set_get_length(keys), 3);

  const char* expected_credentials[] = { KEY_HASH_A_CREDENTIAL_CBOR, KEY_HASH_B_CREDENTIAL_CBOR, SCRIPT_HASH_CREDENTIAL_CBOR };

  for (size_t i = 0; i < 3; ++i)
  {
    cardano_credential_t* key = nullptr;
    EXPECT_EQ(cardano_credential_set_get(keys, i, &key), CARDANO_SUCCESS);

    cardano_credential_t* expected = new_default_credential(expected_credentials[i]);

    EXPECT_TRUE(cardano_credential_equals(key, expected));

    cardano_credential_unref(&expected);
    cardano_credential_unref(&key);
  }

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  cardano_credential_set_unref(&keys);
}

TEST(cardano_account_balance_intervals_map_get_key_at, returnsErrorIfAccountBalanceIntervalsMapIsNull)
{
  // Arrange
  cardano_credential_t* credential = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_get_key_at(nullptr, 0, &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_account_balance_intervals_map_get_key_at, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_account_balance_intervals_map_get_key_at(account_balance_intervals_map, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_key_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential = nullptr;

  // Act
  error = cardano_account_balance_intervals_map_get_key_at(account_balance_intervals_map, 0, &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_key_at, canReturnTheRightKey)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = new_default_account_balance_intervals_map(CBOR);
  ASSERT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  // Act
  cardano_credential_t* credential = nullptr;
  cardano_error_t       error      = cardano_account_balance_intervals_map_get_key_at(account_balance_intervals_map, 2, &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_credential_get_hash_hex(credential), SCRIPT_HASH_HEX);

  // Cleanup
  cardano_credential_unref(&credential);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_value_at, returnsErrorIfAccountBalanceIntervalsMapIsNull)
{
  // Arrange
  cardano_account_balance_interval_t* interval = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_get_value_at(nullptr, 0, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_account_balance_intervals_map_get_value_at, returnsErrorIfIntervalIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_account_balance_intervals_map_get_value_at(account_balance_intervals_map, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval = nullptr;

  // Act
  error = cardano_account_balance_intervals_map_get_value_at(account_balance_intervals_map, 0, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_value_at, canReturnTheRightValue)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = new_default_account_balance_intervals_map(CBOR);
  ASSERT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  // Act
  cardano_account_balance_interval_t* interval = nullptr;

  cardano_error_t error = cardano_account_balance_intervals_map_get_value_at(account_balance_intervals_map, 1, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(interval, testing::Not((cardano_account_balance_interval_t*)nullptr));

  char* interval_cbor = encode_account_balance_interval(interval);
  EXPECT_STREQ(interval_cbor, LOWER_ONLY_INTERVAL_CBOR);

  // Cleanup
  free(interval_cbor);
  cardano_account_balance_interval_unref(&interval);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_key_value_at, returnsErrorIfAccountBalanceIntervalsMapIsNull)
{
  // Arrange
  cardano_credential_t*               credential = nullptr;
  cardano_account_balance_interval_t* interval   = nullptr;

  // Act
  cardano_error_t error = cardano_account_balance_intervals_map_get_key_value_at(nullptr, 0, &credential, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_account_balance_intervals_map_get_key_value_at, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_account_balance_interval_t* interval = nullptr;

  // Act
  error = cardano_account_balance_intervals_map_get_key_value_at(account_balance_intervals_map, 0, nullptr, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_key_value_at, returnsErrorIfIntervalIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential = nullptr;

  // Act
  error = cardano_account_balance_intervals_map_get_key_value_at(account_balance_intervals_map, 0, &credential, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_key_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t*               credential = nullptr;
  cardano_account_balance_interval_t* interval   = nullptr;

  // Act
  error = cardano_account_balance_intervals_map_get_key_value_at(account_balance_intervals_map, 0, &credential, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_get_key_value_at, canReturnTheRightKeyValuePair)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = new_default_account_balance_intervals_map(CBOR);
  ASSERT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));

  // Act
  cardano_credential_t*               credential = nullptr;
  cardano_account_balance_interval_t* interval   = nullptr;

  cardano_error_t error = cardano_account_balance_intervals_map_get_key_value_at(account_balance_intervals_map, 0, &credential, &interval);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_credential_get_hash_hex(credential), KEY_HASH_A_HEX);

  char* interval_cbor = encode_account_balance_interval(interval);
  EXPECT_STREQ(interval_cbor, BOTH_BOUNDS_INTERVAL_CBOR);

  // Cleanup
  free(interval_cbor);
  cardano_credential_unref(&credential);
  cardano_account_balance_interval_unref(&interval);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_account_balance_intervals_map_ref(account_balance_intervals_map);

  // Assert
  EXPECT_THAT(account_balance_intervals_map, testing::Not((cardano_account_balance_intervals_map_t*)nullptr));
  EXPECT_EQ(cardano_account_balance_intervals_map_refcount(account_balance_intervals_map), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_account_balance_intervals_map_ref(nullptr);
}

TEST(cardano_account_balance_intervals_map_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;

  // Act
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_account_balance_intervals_map_unref((cardano_account_balance_intervals_map_t**)nullptr);
}

TEST(cardano_account_balance_intervals_map_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_account_balance_intervals_map_ref(account_balance_intervals_map);
  size_t ref_count = cardano_account_balance_intervals_map_refcount(account_balance_intervals_map);

  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  size_t updated_ref_count = cardano_account_balance_intervals_map_refcount(account_balance_intervals_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_account_balance_intervals_map_ref(account_balance_intervals_map);
  size_t ref_count = cardano_account_balance_intervals_map_refcount(account_balance_intervals_map);

  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
  size_t updated_ref_count = cardano_account_balance_intervals_map_refcount(account_balance_intervals_map);

  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(account_balance_intervals_map, (cardano_account_balance_intervals_map_t*)nullptr);

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}

TEST(cardano_account_balance_intervals_map_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_account_balance_intervals_map_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_account_balance_intervals_map_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  const char*                              message                       = "This is a test message";

  // Act
  cardano_account_balance_intervals_map_set_last_error(account_balance_intervals_map, message);

  // Assert
  EXPECT_STREQ(cardano_account_balance_intervals_map_get_last_error(account_balance_intervals_map), "Object is NULL.");
}

TEST(cardano_account_balance_intervals_map_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_account_balance_intervals_map_t* account_balance_intervals_map = nullptr;
  cardano_error_t                          error                         = cardano_account_balance_intervals_map_new(&account_balance_intervals_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_account_balance_intervals_map_set_last_error(account_balance_intervals_map, message);

  // Assert
  EXPECT_STREQ(cardano_account_balance_intervals_map_get_last_error(account_balance_intervals_map), "");

  // Cleanup
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_map);
}
