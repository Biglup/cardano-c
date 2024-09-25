/**
 * \file value.cpp
 *
 * \author angel.castillo
 * \date   Sep 17, 2024
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

#include <cardano/error.h>

#include <cardano/transaction_body/value.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                             = "821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a";
static const char* CBOR2                            = "821a000f4240a2581c00000000000000000000000000001100000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a";
static const char* CBOR_VALUE_WITH_TWICE_THE_ASSETS = "821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218c8443334353618c6444041424214581c11111111111111111111111111111111111111111111111111111111a3443031323218c8443334353618c6444041424214";
static const char* CBOR_WITH_TWICE_THE_ASSETS       = "a2581c00000000000000000000000000000000000000000000000000000000a3443031323218c8443334353618c6444041424214581c11111111111111111111111111111111111111111111111111111111a3443031323218c8443334353618c6444041424214";
static const char* CBOR_WITH_NEGATIVE_THE_ASSETS    = "a2581c00000000000000000000000000000000000000000000000000000000a34430313232386344333435363862444041424229581c11111111111111111111111111111111111111111111111111111111a34430313232386344333435363862444041424229";
static const char* MULTI_ASSET_CBOR                 = "a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a";
static const char* MULTI_ASSET_CBOR_MIXED2          = "a2581c00000000000000000000000000000000000000002200000000000000a34430313232186444333435361863444041424229581c11111111111111111111111111111111111111111111111111111111a34430313232386344333435361863444041424229";
static const char* MULTI_ASSET_CBOR_MIXED           = "a2581c00000000000000000000000000000000000000000000000000000000a34430313232186444333435361863444041424229581c11111111111111111111111111111111111111111111111111111111a34430313232386344333435361863444041424229";
static const char* ASSET_NAME_CBOR_1                = "49736b7977616c6b6571";
static const char* ASSET_NAME_CBOR_2                = "49736b7977616c6b6572";
static const char* ASSET_NAME_CBOR_3                = "49736b7977616c6b6573";
static const char* ASSET_NAME_CBOR_1B               = "4430313232";
static const char* ASSET_NAME_CBOR_2B               = "4433343536";
static const char* ASSET_NAME_CBOR_3B               = "4440414242";
static const char* POLICY_ID_HEX_1B                 = "00000000000000000000000000000000000000000000000000000000";
static const char* POLICY_ID_HEX_2B                 = "11111111111111111111111111111111111111111111111111111111";
static const char* POLICY_ID_HEX_1                  = "f0ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9a";
static const char* POLICY_ID_HEX_2                  = "f1ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9a";
static const char* POLICY_ID_HEX_3                  = "f2ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9a";
static const char* ASSET_MAP_CBOR                   = "a349736b7977616c6b65710149736b7977616c6b65720249736b7977616c6b657303";

static const char** ASSET_IDS = (const char*[]) {
  "lovelace",
  "0000000000000000000000000000000000000000000000000000000030313232",
  "0000000000000000000000000000000000000000000000000000000033343536",
  "0000000000000000000000000000000000000000000000000000000040414242",
  "1111111111111111111111111111111111111111111111111111111130313232",
  "1111111111111111111111111111111111111111111111111111111133343536",
  "1111111111111111111111111111111111111111111111111111111140414242",
};

static const char** ASSET_IDS_2 = (const char*[]) {
  "lovelace",
  "1111111111111111111111111111111111111111111111111111111130313232",
  "1111111111111111111111111111111111111111111111111111111133343536",
  "1111111111111111111111111111111111111111111111111111111140414242",
};

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the value.
 * @return A new instance of the value.
 */
static cardano_value_t*
new_default_value(const char* value_cbor)
{
  cardano_value_t*       value  = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(value_cbor, strlen(value_cbor));
  cardano_error_t        result = cardano_value_from_cbor(reader, &value);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return value;
};

/**
 * Creates a new default instance of the governance action id.
 * @return A new instance of the governance action id.
 */
static cardano_asset_name_t*
new_default_asset_name(const char* name)
{
  cardano_asset_name_t*  asset_name = NULL;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(name, strlen(name));
  cardano_error_t        result     = cardano_asset_name_from_cbor(reader, &asset_name);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return asset_name;
};

/**
 * Creates a new default instance of the blake2b_hash.
 * @return A new instance of the blake2b_hash.
 */
static cardano_blake2b_hash_t*
new_default_blake2b_hash(const char* hash)
{
  cardano_blake2b_hash_t* blake2b_hash = NULL;
  cardano_error_t         result       = cardano_blake2b_hash_from_hex(hash, strlen(hash), &blake2b_hash);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  return blake2b_hash;
};

/**
 * Creates a new default instance of the asset name map.
 * @return A new instance of the asset name map.
 */
static cardano_asset_name_map_t*
new_default_asset_name_map(const char* cbor)
{
  cardano_asset_name_map_t* asset_name_map = NULL;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t           result         = cardano_asset_name_map_from_cbor(reader, &asset_name_map);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return asset_name_map;
};

/**
 * Creates a new default instance of the multi asset.
 * @return A new instance of the multi asset.
 */
static cardano_multi_asset_t*
new_default_multi_asset(const char* cbor)
{
  cardano_multi_asset_t* multi_asset = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result      = cardano_multi_asset_from_cbor(reader, &multi_asset);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return multi_asset;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_value_new, canCreateValue)
{
  // Arrange
  cardano_value_t* value = nullptr;

  // Act
  cardano_error_t error = cardano_value_new(0, NULL, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(value, testing::Not((cardano_value_t*)nullptr));

  // Cleanup
  cardano_value_unref(&value);
}

TEST(cardano_value_new, returnsErrorIfValueIsNull)
{
  // Act
  cardano_error_t error = cardano_value_new(0, NULL, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_value_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_value_t* value = nullptr;

  // Act
  cardano_error_t error = cardano_value_new(0, NULL, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(value, (cardano_value_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_value_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_value_t* value = nullptr;

  // Act
  cardano_error_t error = cardano_value_new(0, NULL, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(value, (cardano_value_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_value_to_cbor, canSerializeAnEmptyValue)
{
  // Arrange
  cardano_value_t*       value  = nullptr;
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_value_new(0, NULL, &value);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_value_to_cbor(value, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 3);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "00");

  // Cleanup
  cardano_value_unref(&value);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_value_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_value_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_value_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_value_t* value = nullptr;

  cardano_error_t error = cardano_value_new(0, NULL, &value);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_value_to_cbor(value, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_value_unref(&value);
}

TEST(cardano_value_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_value_t*       value  = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_value_from_cbor(reader, &value);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_value_to_cbor(value, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_value_unref(&value);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_value_from_cbor, returnErrorIfValueIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_value_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_value_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_value_t* value = nullptr;

  // Act
  cardano_error_t error = cardano_value_from_cbor(nullptr, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_value_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_value_t*       value  = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_value_from_cbor(reader, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(value, (cardano_value_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_value_from_cbor, canReadInteger)
{
  // Arrange
  cardano_value_t*       value  = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("09", 2);

  // Act
  cardano_error_t error = cardano_value_from_cbor(reader, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(value, testing::Not((cardano_value_t*)nullptr));
  EXPECT_EQ(cardano_value_get_coin(value), 9);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_value_unref(&value);
}

TEST(cardano_value_from_cbor, returnErrorIfNotAnArrayNorInt)
{
  // Arrange
  cardano_value_t*       list   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("ef", 2);

  // Act
  cardano_error_t error = cardano_value_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'value', expected 'Reader State: Start Array' (9) but got 'Reader State: Simple Value' (14).");
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_value_from_cbor, returnErrorIfInvalidArraySize)
{
  // Arrange
  cardano_value_t*       list   = nullptr;
  const char*            cbor   = "85";
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t error = cardano_value_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'value', expected a 'Major Type: Byte String' (2) of 2 element(s) but got a 'Major Type: Byte String' (2) of 5 element(s).");
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_value_from_cbor, returnErrorIfInvalidInt)
{
  // Arrange
  cardano_value_t*       list   = nullptr;
  const char*            cbor   = "82efa2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a";
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t error = cardano_value_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_value_from_cbor, returnErrorIfInvalidMultiAsset)
{
  // Arrange
  cardano_value_t*       list   = nullptr;
  const char*            cbor   = "821a00ef581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a";
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t error = cardano_value_from_cbor(reader, &list);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_value_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_value_t* value = nullptr;
  cardano_error_t  error = cardano_value_new(0, NULL, &value);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_value_ref(value);

  // Assert
  EXPECT_THAT(value, testing::Not((cardano_value_t*)nullptr));
  EXPECT_EQ(cardano_value_refcount(value), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_value_unref(&value);
  cardano_value_unref(&value);
}

TEST(cardano_value_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_value_ref(nullptr);
}

TEST(cardano_value_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_value_t* value = nullptr;

  // Act
  cardano_value_unref(&value);
}

TEST(cardano_value_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_value_unref((cardano_value_t**)nullptr);
}

TEST(cardano_value_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_value_t* value = nullptr;
  cardano_error_t  error = cardano_value_new(0, NULL, &value);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_value_ref(value);
  size_t ref_count = cardano_value_refcount(value);

  cardano_value_unref(&value);
  size_t valued_ref_count = cardano_value_refcount(value);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(valued_ref_count, 1);

  // Cleanup
  cardano_value_unref(&value);
}

TEST(cardano_value_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_value_t* value = nullptr;
  cardano_error_t  error = cardano_value_new(0, NULL, &value);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_value_ref(value);
  size_t ref_count = cardano_value_refcount(value);

  cardano_value_unref(&value);
  size_t valued_ref_count = cardano_value_refcount(value);

  cardano_value_unref(&value);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(valued_ref_count, 1);
  EXPECT_EQ(value, (cardano_value_t*)nullptr);

  // Cleanup
  cardano_value_unref(&value);
}

TEST(cardano_value_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_value_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_value_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_value_t* value   = nullptr;
  const char*      message = "This is a test message";

  // Act
  cardano_value_set_last_error(value, message);

  // Assert
  EXPECT_STREQ(cardano_value_get_last_error(value), "Object is NULL.");
}

TEST(cardano_value_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_value_t* value = nullptr;
  cardano_error_t  error = cardano_value_new(0, NULL, &value);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_value_set_last_error(value, message);

  // Assert
  EXPECT_STREQ(cardano_value_get_last_error(value), "");

  // Cleanup
  cardano_value_unref(&value);
}

TEST(cardano_value_get_multi_asset, canGetMultiAsset)
{
  // Arrange
  cardano_value_t* value = new_default_value(CBOR);

  // Act
  cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(value);

  // Assert
  EXPECT_THAT(multi_asset, testing::Not((cardano_multi_asset_t*)nullptr));

  // check asset CBOR
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_error_t        error  = cardano_multi_asset_to_cbor(multi_asset, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(MULTI_ASSET_CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(actual_cbor, MULTI_ASSET_CBOR);

  // Cleanup
  cardano_value_unref(&value);
  cardano_multi_asset_unref(&multi_asset);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_value_get_multi_asset, returnsNullIfValueIsNull)
{
  // Act
  cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(nullptr);

  // Assert
  EXPECT_THAT(multi_asset, testing::IsNull());
}

TEST(cardano_value_set_multi_asset, canSetMultiAsset)
{
  // Arrange
  cardano_value_t*       value       = new_default_value(CBOR);
  cardano_multi_asset_t* multi_asset = new_default_multi_asset(MULTI_ASSET_CBOR_MIXED2);

  // Act
  cardano_error_t error = cardano_value_set_multi_asset(value, multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // check asset CBOR
  cardano_multi_asset_t* multi_asset2 = cardano_value_get_multi_asset(value);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  error                         = cardano_multi_asset_to_cbor(multi_asset2, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(MULTI_ASSET_CBOR_MIXED2) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, MULTI_ASSET_CBOR_MIXED2);

  // Cleanup
  cardano_value_unref(&value);
  cardano_multi_asset_unref(&multi_asset);
  cardano_multi_asset_unref(&multi_asset2);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_value_set_multi_asset, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = new_default_multi_asset(MULTI_ASSET_CBOR);

  // Act
  cardano_error_t error = cardano_value_set_multi_asset(nullptr, multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
}

TEST(cardano_value_get_coin, canGetCoin)
{
  // Arrange
  cardano_value_t* value = new_default_value(CBOR);

  // Act
  uint64_t coin = cardano_value_get_coin(value);

  // Assert
  EXPECT_EQ(coin, 1000000);

  // Cleanup
  cardano_value_unref(&value);
}

TEST(cardano_value_get_coin, returnsZeroIfValueIsNull)
{
  // Act
  uint64_t coin = cardano_value_get_coin(nullptr);

  // Assert
  EXPECT_EQ(coin, 0);
}

TEST(cardano_value_set_coin, canSetCoin)
{
  // Arrange
  cardano_value_t* value = new_default_value(CBOR);

  // Act
  cardano_error_t error = cardano_value_set_coin(value, 2000000);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(value), 2000000);

  // Cleanup
  cardano_value_unref(&value);
}

TEST(cardano_value_set_coin, returnsErrorIfValueIsNull)
{
  // Act
  cardano_error_t error = cardano_value_set_coin(nullptr, 2000000);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_value_add_coin, canAddCoin)
{
  // Arrange
  cardano_value_t* value = new_default_value(CBOR);

  // Act
  cardano_error_t error = cardano_value_add_coin(value, 2000000);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(value), 3000000);

  // Cleanup
  cardano_value_unref(&value);
}

TEST(cardano_value_add_coin, returnsErrorIfValueIsNull)
{
  // Act
  cardano_error_t error = cardano_value_add_coin(nullptr, 2000000);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_value_subtract_coin, canSubtractCoin)
{
  // Arrange
  cardano_value_t* value = new_default_value(CBOR);

  // Act
  cardano_error_t error = cardano_value_subtract_coin(value, 1000000);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(value), 0);

  // Cleanup
  cardano_value_unref(&value);
}

TEST(cardano_value_subtract_coin, returnsUnderflowIfTooBigValue)
{
  // Arrange
  cardano_value_t* value = new_default_value(CBOR);

  // Act
  cardano_error_t error = cardano_value_subtract_coin(value, 2000000);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INTEGER_UNDERFLOW);
  EXPECT_EQ(cardano_value_get_coin(value), 1000000);

  // Cleanup
  cardano_value_unref(&value);
}

TEST(cardano_value_subtract_coin, returnsErrorIfValueIsNull)
{
  // Act
  cardano_error_t error = cardano_value_subtract_coin(nullptr, 2000000);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_value_add_multi_asset, canAddMultiAsset)
{
  // Arrange
  cardano_value_t*       value       = new_default_value(CBOR);
  cardano_multi_asset_t* multi_asset = new_default_multi_asset(MULTI_ASSET_CBOR);

  // Act
  cardano_error_t error = cardano_value_add_multi_asset(value, multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // check asset CBOR
  cardano_multi_asset_t* multi_asset2 = cardano_value_get_multi_asset(value);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  error                         = cardano_multi_asset_to_cbor(multi_asset2, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(MULTI_ASSET_CBOR_MIXED) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR_WITH_TWICE_THE_ASSETS);

  // Cleanup
  cardano_value_unref(&value);
  cardano_multi_asset_unref(&multi_asset);
  cardano_multi_asset_unref(&multi_asset2);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_value_add_multi_asset, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = new_default_multi_asset(MULTI_ASSET_CBOR);

  // Act
  cardano_error_t error = cardano_value_add_multi_asset(nullptr, multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
}

TEST(cardano_value_subtract_multi_asset, canSubtractMultiAsset)
{
  // Arrange
  cardano_value_t*       value       = new_default_value(CBOR);
  cardano_multi_asset_t* multi_asset = new_default_multi_asset(CBOR_WITH_TWICE_THE_ASSETS);

  // Act
  cardano_error_t error = cardano_value_subtract_multi_asset(value, multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // check asset CBOR
  cardano_multi_asset_t* multi_asset2 = cardano_value_get_multi_asset(value);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  error                         = cardano_multi_asset_to_cbor(multi_asset2, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(MULTI_ASSET_CBOR_MIXED2) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR_WITH_NEGATIVE_THE_ASSETS);

  // Cleanup
  cardano_value_unref(&value);
  cardano_multi_asset_unref(&multi_asset);
  cardano_multi_asset_unref(&multi_asset2);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_value_add_multi_asset, returnsErrorIfMultiAssetIsNull)
{
  // Act
  cardano_error_t error = cardano_value_add_multi_asset((cardano_value_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_value_subtract_multi_asset, returnsErrorIfMultiAssetIsNull)
{
  // Act
  cardano_error_t error = cardano_value_subtract_multi_asset((cardano_value_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_value_subtract_multi_asset, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = new_default_multi_asset(MULTI_ASSET_CBOR);

  // Act
  cardano_error_t error = cardano_value_subtract_multi_asset(nullptr, multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
}

TEST(cardano_value_add, canAddValues)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(CBOR);
  cardano_value_t* value2 = new_default_value(CBOR);
  cardano_value_t* result = NULL;
  // Act
  cardano_error_t error = cardano_value_add(value1, value2, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(result), 2000000);

  // check asset CBOR
  cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(result);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  error = cardano_multi_asset_to_cbor(multi_asset, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR_WITH_TWICE_THE_ASSETS) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR_WITH_TWICE_THE_ASSETS);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
  cardano_value_unref(&result);
  cardano_multi_asset_unref(&multi_asset);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_value_add, returnsErrorIfValue1IsNull)
{
  // Arrange
  cardano_value_t* value2 = new_default_value(CBOR);
  cardano_value_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_add(nullptr, value2, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_value_unref(&value2);
}

TEST(cardano_value_add, returnsErrorIfValue2IsNull)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(CBOR);
  cardano_value_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_add(value1, nullptr, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_value_unref(&value1);
}

TEST(cardano_value_add, returnsErrorIfResultIsNull)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(CBOR);
  cardano_value_t* value2 = new_default_value(CBOR);

  // Act
  cardano_error_t error = cardano_value_add(value1, value2, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
}

TEST(cardano_value_add, canAddTwoValuesWithoutAssets)
{
  // Arrange
  cardano_value_t* value1 = new_default_value("01");
  cardano_value_t* value2 = new_default_value("02");
  cardano_value_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_add(value1, value2, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(result), 3);

  // check value CBOR
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  error                         = cardano_value_to_cbor(result, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 3);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "03");

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
  cardano_value_unref(&result);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_value_add, canAddTwoValuesLhsHasAssetsRhsOnlyCoin)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(CBOR);
  cardano_value_t* value2 = new_default_value("01");
  cardano_value_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_add(value1, value2, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(result), 1000001);

  // check asset CBOR
  cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(result);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  error                         = cardano_multi_asset_to_cbor(multi_asset, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(MULTI_ASSET_CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, MULTI_ASSET_CBOR);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
  cardano_value_unref(&result);
  cardano_multi_asset_unref(&multi_asset);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_value_add, canAddTwoValuesRhsHasAssetsLhsOnlyCoin)
{
  // Arrange
  cardano_value_t* value1 = new_default_value("01");
  cardano_value_t* value2 = new_default_value(CBOR);
  cardano_value_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_add(value1, value2, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(result), 1000001);

  // check asset CBOR
  cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(result);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  error                         = cardano_multi_asset_to_cbor(multi_asset, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(MULTI_ASSET_CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, MULTI_ASSET_CBOR);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
  cardano_value_unref(&result);
  cardano_multi_asset_unref(&multi_asset);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_value_subtract, canSubtractValues)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(CBOR);
  cardano_value_t* value2 = new_default_value(CBOR_VALUE_WITH_TWICE_THE_ASSETS);
  cardano_value_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_subtract(value1, value2, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(result), 0);

  // check asset CBOR
  cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(result);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  error                         = cardano_multi_asset_to_cbor(multi_asset, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(MULTI_ASSET_CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR_WITH_NEGATIVE_THE_ASSETS);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
  cardano_value_unref(&result);
  cardano_multi_asset_unref(&multi_asset);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_value_subtract, returnsErrorIfValue1IsNull)
{
  // Arrange
  cardano_value_t* value2 = new_default_value(CBOR);
  cardano_value_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_subtract(nullptr, value2, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_value_unref(&value2);
}

TEST(cardano_value_subtract, returnsErrorIfValue2IsNull)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(CBOR);
  cardano_value_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_subtract(value1, nullptr, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_value_unref(&value1);
}

TEST(cardano_value_subtract, returnsErrorIfResultIsNull)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(CBOR);
  cardano_value_t* value2 = new_default_value(CBOR);

  // Act
  cardano_error_t error = cardano_value_subtract(value1, value2, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
}

TEST(cardano_value_subtract, canSubtractTwoValuesWithoutAssets)
{
  // Arrange
  cardano_value_t* value1 = new_default_value("03");
  cardano_value_t* value2 = new_default_value("01");
  cardano_value_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_subtract(value1, value2, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(result), 2);

  // check value CBOR
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  error                         = cardano_value_to_cbor(result, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 3);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "02");

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
  cardano_value_unref(&result);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_value_subtract, canSubtractTwoValuesLhsHasAssetsRhsOnlyCoin)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(CBOR);
  cardano_value_t* value2 = new_default_value("01");
  cardano_value_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_subtract(value1, value2, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(result), 999999);

  // check asset CBOR
  cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(result);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  error                         = cardano_multi_asset_to_cbor(multi_asset, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(MULTI_ASSET_CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, MULTI_ASSET_CBOR);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
  cardano_value_unref(&result);
  cardano_multi_asset_unref(&multi_asset);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_value_subtract, canSubtractTwoValuesRhsHasAssetsLhsOnlyCoin)
{
  // Arrange
  cardano_value_t* value1 = new_default_value("1a000f4242");
  cardano_value_t* value2 = new_default_value(CBOR);
  cardano_value_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_subtract(value1, value2, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(result), 2);

  // check asset CBOR
  cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(result);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  error                         = cardano_multi_asset_to_cbor(multi_asset, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(MULTI_ASSET_CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR_WITH_NEGATIVE_THE_ASSETS);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
  cardano_value_unref(&result);
  cardano_multi_asset_unref(&multi_asset);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_value_subtract, returnsUnderflowIfRhsCoinIsGreaterThanLhsCoinNoAssets)
{
  // Arrange
  cardano_value_t* value1 = new_default_value("01");
  cardano_value_t* value2 = new_default_value("02");
  cardano_value_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_subtract(value1, value2, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INTEGER_UNDERFLOW);
  EXPECT_EQ(cardano_value_get_coin(result), 0);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
  cardano_value_unref(&result);
}

TEST(cardano_value_subtract, returnsUnderflowIfRhsCoinIsGreaterThanLhsCoinLhsHasAssets)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(CBOR);
  cardano_value_t* value2 = new_default_value("1a000f4242");
  cardano_value_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_subtract(value1, value2, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INTEGER_UNDERFLOW);
  EXPECT_EQ(cardano_value_get_coin(result), 0);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
  cardano_value_unref(&result);
}

TEST(cardano_value_subtract, returnsUnderflowIfRhsCoinIsGreaterThanLhsCoinRhsHasAssets)
{
  // Arrange
  cardano_value_t* value1 = new_default_value("01");
  cardano_value_t* value2 = new_default_value(CBOR);
  cardano_value_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_subtract(value1, value2, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INTEGER_UNDERFLOW);
  EXPECT_EQ(cardano_value_get_coin(result), 0);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
  cardano_value_unref(&result);
}

TEST(cardano_value_subtract, returnsUnderflowIfRhsCoinIsGreaterThanLhsCoinBothHasAssets)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(CBOR);
  cardano_value_t* value2 = new_default_value(CBOR);
  cardano_value_t* result = NULL;

  EXPECT_EQ(cardano_value_set_coin(value2, 20000000000), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_value_subtract(value1, value2, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INTEGER_UNDERFLOW);
  EXPECT_EQ(cardano_value_get_coin(result), 0);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
  cardano_value_unref(&result);
}

TEST(cardano_value_get_intersection, canGetIntersection)
{
  // Arrange
  cardano_value_t*         value1 = new_default_value(CBOR);
  cardano_value_t*         value2 = new_default_value(CBOR_VALUE_WITH_TWICE_THE_ASSETS);
  cardano_asset_id_list_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_get_intersection(value1, value2, &result);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Assert
  const size_t size = cardano_asset_id_list_get_length(result);

  EXPECT_EQ(size, 7);

  for (size_t i = 0; i < size; i++)
  {
    cardano_asset_id_t* asset_id = NULL;

    cardano_error_t get_id_result = cardano_asset_id_list_get(result, i, &asset_id);

    EXPECT_EQ(get_id_result, CARDANO_SUCCESS);

    const char* asset_id_str = cardano_asset_id_get_hex(asset_id);

    if (i == 0)
    {
      EXPECT_TRUE(cardano_asset_id_is_lovelace(asset_id));
    }
    else
    {
      EXPECT_STREQ(ASSET_IDS[i], asset_id_str);
    }

    cardano_asset_id_unref(&asset_id);
  }

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
  cardano_asset_id_list_unref(&result);
}

TEST(cardano_value_get_intersection, canGetIntersection2)
{
  // Arrange
  cardano_value_t*         value1 = new_default_value(CBOR);
  cardano_value_t*         value2 = new_default_value(CBOR2);
  cardano_asset_id_list_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_get_intersection(value1, value2, &result);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Assert
  const size_t size = cardano_asset_id_list_get_length(result);

  EXPECT_EQ(size, 4);

  for (size_t i = 0; i < size; i++)
  {
    cardano_asset_id_t* asset_id = NULL;

    cardano_error_t get_id_result = cardano_asset_id_list_get(result, i, &asset_id);

    EXPECT_EQ(get_id_result, CARDANO_SUCCESS);

    const char* asset_id_str = cardano_asset_id_get_hex(asset_id);

    if (i == 0)
    {
      EXPECT_TRUE(cardano_asset_id_is_lovelace(asset_id));
    }
    else
    {
      EXPECT_STREQ(ASSET_IDS_2[i], asset_id_str);
    }

    cardano_asset_id_unref(&asset_id);
  }

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
  cardano_asset_id_list_unref(&result);
}

TEST(cardano_value_get_intersection, canGetIntersectionOfOnlyAda)
{
  // Arrange
  cardano_value_t*         value1 = new_default_value(CBOR);
  cardano_value_t*         value2 = new_default_value("01");
  cardano_asset_id_list_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_get_intersection(value1, value2, &result);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Assert
  const size_t size = cardano_asset_id_list_get_length(result);

  EXPECT_EQ(size, 1);

  for (size_t i = 0; i < size; i++)
  {
    cardano_asset_id_t* asset_id = NULL;

    cardano_error_t get_id_result = cardano_asset_id_list_get(result, i, &asset_id);

    EXPECT_EQ(get_id_result, CARDANO_SUCCESS);

    const char* asset_id_str = cardano_asset_id_get_hex(asset_id);

    if (i == 0)
    {
      EXPECT_TRUE(cardano_asset_id_is_lovelace(asset_id));
    }
    else
    {
      EXPECT_STREQ(ASSET_IDS[i], asset_id_str);
    }

    cardano_asset_id_unref(&asset_id);
  }

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
  cardano_asset_id_list_unref(&result);
}

TEST(cardano_value_get_intersection, canGetIntersectionOfOnlyAda2)
{
  // Arrange
  cardano_value_t*         value1 = new_default_value("02");
  cardano_value_t*         value2 = new_default_value("01");
  cardano_asset_id_list_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_get_intersection(value1, value2, &result);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Assert
  const size_t size = cardano_asset_id_list_get_length(result);

  EXPECT_EQ(size, 1);

  for (size_t i = 0; i < size; i++)
  {
    cardano_asset_id_t* asset_id = NULL;

    cardano_error_t get_id_result = cardano_asset_id_list_get(result, i, &asset_id);

    EXPECT_EQ(get_id_result, CARDANO_SUCCESS);

    const char* asset_id_str = cardano_asset_id_get_hex(asset_id);

    if (i == 0)
    {
      EXPECT_TRUE(cardano_asset_id_is_lovelace(asset_id));
    }
    else
    {
      EXPECT_STREQ(ASSET_IDS[i], asset_id_str);
    }

    cardano_asset_id_unref(&asset_id);
  }

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
  cardano_asset_id_list_unref(&result);
}

TEST(cardano_value_get_intersection, canGetIntersectionOfOnlyAda3)
{
  // Arrange
  cardano_value_t*         value1 = new_default_value("03");
  cardano_value_t*         value2 = new_default_value(CBOR);
  cardano_asset_id_list_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_get_intersection(value1, value2, &result);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Assert
  const size_t size = cardano_asset_id_list_get_length(result);

  EXPECT_EQ(size, 1);

  for (size_t i = 0; i < size; i++)
  {
    cardano_asset_id_t* asset_id = NULL;

    cardano_error_t get_id_result = cardano_asset_id_list_get(result, i, &asset_id);

    EXPECT_EQ(get_id_result, CARDANO_SUCCESS);

    const char* asset_id_str = cardano_asset_id_get_hex(asset_id);

    if (i == 0)
    {
      EXPECT_TRUE(cardano_asset_id_is_lovelace(asset_id));
    }
    else
    {
      EXPECT_STREQ(ASSET_IDS[i], asset_id_str);
    }

    cardano_asset_id_unref(&asset_id);
  }

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
  cardano_asset_id_list_unref(&result);
}

TEST(cardano_value_get_intersection, returnsErrorIfLhsIsNull)
{
  // Arrange
  cardano_value_t*         value2 = new_default_value(CBOR);
  cardano_asset_id_list_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_get_intersection(nullptr, value2, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_value_unref(&value2);
}

TEST(cardano_value_get_intersection, returnsErrorIfRhsIsNull)
{
  // Arrange
  cardano_value_t*         value1 = new_default_value(CBOR);
  cardano_asset_id_list_t* result = NULL;

  // Act
  cardano_error_t error = cardano_value_get_intersection(value1, nullptr, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_value_unref(&value1);
}

TEST(cardano_value_get_intersection_count, canGetIntersectionCount)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(CBOR);
  cardano_value_t* value2 = new_default_value(CBOR_VALUE_WITH_TWICE_THE_ASSETS);
  size_t           result = 0;

  // Act
  cardano_error_t error = cardano_value_get_intersection_count(value1, value2, &result);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(result, 7);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
}

TEST(cardano_value_get_intersection_count, canGetIntersectionCount2)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(CBOR);
  cardano_value_t* value2 = new_default_value(CBOR2);
  size_t           result = 0;

  // Act
  cardano_error_t error = cardano_value_get_intersection_count(value1, value2, &result);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(result, 4);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
}

TEST(cardano_value_get_intersection_count, canGetIntersectionCountOfOnlyAda)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(CBOR);
  cardano_value_t* value2 = new_default_value("01");
  size_t           result = 0;

  // Act
  cardano_error_t error = cardano_value_get_intersection_count(value1, value2, &result);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(result, 1);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
}

TEST(cardano_value_get_intersection_count, returnsErrorIfLshIsNull)
{
  // Arrange
  cardano_value_t* value2 = new_default_value(CBOR);
  size_t           result = 0;

  // Act
  cardano_error_t error = cardano_value_get_intersection_count(nullptr, value2, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_value_unref(&value2);
}

TEST(cardano_value_get_intersection_count, returnsErrorIfRhsIsNull)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(CBOR);
  size_t           result = 0;

  // Act
  cardano_error_t error = cardano_value_get_intersection_count(value1, nullptr, &result);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_value_unref(&value1);
}

TEST(cardano_value_get_intersection_count, returnsErrorIfResultIsNull)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(CBOR);
  cardano_value_t* value2 = new_default_value(CBOR);

  // Act
  cardano_error_t error = cardano_value_get_intersection_count(value1, value2, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
}

TEST(cardano_value_as_assets_map, canConvertValueToAssetsMap)
{
  // Arrange
  cardano_value_t* value = new_default_value(CBOR);

  // Act
  cardano_asset_id_map_t* result = cardano_value_as_assets_map(value);

  // Assert
  const size_t             size = cardano_asset_id_map_get_length(result);
  cardano_asset_id_list_t* keys = NULL;

  cardano_error_t get_keys_result = cardano_asset_id_map_get_keys(result, &keys);

  EXPECT_EQ(get_keys_result, CARDANO_SUCCESS);
  EXPECT_EQ(size, 7);

  for (size_t i = 0; i < size; i++)
  {
    cardano_asset_id_t* asset_id = NULL;

    cardano_error_t get_id_result = cardano_asset_id_list_get(keys, i, &asset_id);

    EXPECT_EQ(get_id_result, CARDANO_SUCCESS);

    const char* asset_id_str = cardano_asset_id_get_hex(asset_id);

    if (i == 0)
    {
      EXPECT_TRUE(cardano_asset_id_is_lovelace(asset_id));
    }
    else
    {
      EXPECT_STREQ(ASSET_IDS[i], asset_id_str);
    }

    cardano_asset_id_unref(&asset_id);
  }

  // Cleanup
  cardano_value_unref(&value);
  cardano_asset_id_map_unref(&result);
  cardano_asset_id_list_unref(&keys);
}

TEST(cardano_value_as_assets_map, canCovnertOnlyAda)
{
  // Arrange
  cardano_value_t* value = new_default_value("01");

  // Act
  cardano_asset_id_map_t* result = cardano_value_as_assets_map(value);

  // Assert
  const size_t             size = cardano_asset_id_map_get_length(result);
  cardano_asset_id_list_t* keys = NULL;

  cardano_error_t get_keys_result = cardano_asset_id_map_get_keys(result, &keys);

  EXPECT_EQ(get_keys_result, CARDANO_SUCCESS);
  EXPECT_EQ(size, 1);

  for (size_t i = 0; i < size; i++)
  {
    cardano_asset_id_t* asset_id = NULL;

    cardano_error_t get_id_result = cardano_asset_id_list_get(keys, i, &asset_id);

    EXPECT_EQ(get_id_result, CARDANO_SUCCESS);

    const char* asset_id_str = cardano_asset_id_get_hex(asset_id);

    if (i == 0)
    {
      EXPECT_TRUE(cardano_asset_id_is_lovelace(asset_id));
    }
    else
    {
      EXPECT_STREQ(ASSET_IDS[i], asset_id_str);
    }

    cardano_asset_id_unref(&asset_id);
  }

  // Cleanup
  cardano_value_unref(&value);
  cardano_asset_id_map_unref(&result);
  cardano_asset_id_list_unref(&keys);
}

TEST(cardano_value_as_assets_map, canCovnertEmptyValue)
{
  // Arrange
  cardano_value_t* value = new_default_value("00");

  // Act
  cardano_asset_id_map_t* result = cardano_value_as_assets_map(value);

  // Assert
  const size_t             size = cardano_asset_id_map_get_length(result);
  cardano_asset_id_list_t* keys = NULL;

  cardano_error_t get_keys_result = cardano_asset_id_map_get_keys(result, &keys);

  EXPECT_EQ(get_keys_result, CARDANO_SUCCESS);
  EXPECT_EQ(size, 0);

  // Cleanup
  cardano_value_unref(&value);
  cardano_asset_id_map_unref(&result);
  cardano_asset_id_list_unref(&keys);
}

TEST(cardano_value_as_assets_map, returnsErrorIfValueIsNull)
{
  // Act
  cardano_asset_id_map_t* result = cardano_value_as_assets_map(nullptr);

  // Assert
  EXPECT_EQ(result, nullptr);
}

TEST(cardano_value_get_asset_count, canGetAssetCount)
{
  // Arrange
  cardano_value_t* value = new_default_value(CBOR);

  // Act
  size_t result = cardano_value_get_asset_count(value);

  // Assert
  EXPECT_EQ(result, 7);

  // Cleanup
  cardano_value_unref(&value);
}

TEST(cardano_value_get_asset_count, canGetAssetCountOfOnlyAda)
{
  // Arrange
  cardano_value_t* value = new_default_value("01");

  // Act
  size_t result = cardano_value_get_asset_count(value);

  // Assert
  EXPECT_EQ(result, 1);

  // Cleanup
  cardano_value_unref(&value);
}

TEST(cardano_value_get_asset_count, canGetAssetCountOfEmptyValue)
{
  // Arrange
  cardano_value_t* value = new_default_value("00");

  // Act
  size_t result = cardano_value_get_asset_count(value);

  // Assert
  EXPECT_EQ(result, 0);

  // Cleanup
  cardano_value_unref(&value);
}

TEST(cardano_value_get_asset_count, returnsZeroIfValueIsNull)
{
  // Act
  size_t result = cardano_value_get_asset_count(nullptr);

  // Assert
  EXPECT_EQ(result, 0);
}

TEST(cardano_value_is_zero, returnsTrueIfValueIsZero)
{
  // Arrange
  cardano_value_t* value = new_default_value("00");

  // Act
  bool result = cardano_value_is_zero(value);

  // Assert
  EXPECT_TRUE(result);

  // Cleanup
  cardano_value_unref(&value);
}

TEST(cardano_value_is_zero, returnsFalseIfValueIsNotZero)
{
  // Arrange
  cardano_value_t* value = new_default_value(CBOR);

  // Act
  bool result = cardano_value_is_zero(value);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_value_unref(&value);
}

TEST(cardano_value_is_zero, returnsFalseIfValueIsOnlyAda)
{
  // Arrange
  cardano_value_t* value = new_default_value("01");

  // Act
  bool result = cardano_value_is_zero(value);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_value_unref(&value);
}

TEST(cardano_value_is_zero, returnsTrueIfValueIsNull)
{
  // Act
  bool result = cardano_value_is_zero(nullptr);

  // Assert
  EXPECT_TRUE(result);
}

TEST(cardano_value_equals, returnsTrueIfValuesAreEqual)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(CBOR);
  cardano_value_t* value2 = new_default_value(CBOR);

  // Act
  bool result = cardano_value_equals(value1, value2);

  // Assert
  EXPECT_TRUE(result);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
}

TEST(cardano_value_equals, returnsFalseIfValuesAreNotEqual)
{
  // Arrange
  cardano_value_t* value1 = new_default_value(CBOR);
  cardano_value_t* value2 = new_default_value(CBOR_VALUE_WITH_TWICE_THE_ASSETS);

  // Act
  bool result = cardano_value_equals(value1, value2);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
}

TEST(cardano_value_equals, returnsFalseIfDifferentAdaAmount)
{
  // Arrange
  cardano_value_t* value1 = new_default_value("01");
  cardano_value_t* value2 = new_default_value("02");

  // Act
  bool result = cardano_value_equals(value1, value2);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
}

TEST(cardano_value_equals, returnsTrueIfSameAdaAmount)
{
  // Arrange
  cardano_value_t* value1 = new_default_value("01");
  cardano_value_t* value2 = new_default_value("01");

  // Act
  bool result = cardano_value_equals(value1, value2);

  // Assert
  EXPECT_TRUE(result);

  // Cleanup
  cardano_value_unref(&value1);
  cardano_value_unref(&value2);
}

TEST(cardano_value_equals, returnTrueIfBothNull)
{
  // Act
  bool result = cardano_value_equals(nullptr, nullptr);

  // Assert
  EXPECT_TRUE(result);
}

TEST(cardano_value_equals, returnFalseIfOneIsNull)
{
  // Arrange
  cardano_value_t* value = new_default_value(CBOR);

  // Act
  bool result = cardano_value_equals(value, nullptr);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_value_unref(&value);
}
