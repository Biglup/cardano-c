/**
 * \file multi_asset.cpp
 *
 * \author angel.castillo
 * \date   Sep 16, 2024
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

#include <cardano/assets/multi_asset.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR               = "a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a";
static const char* CBOR_MIXED2        = "a2581c00000000000000000000000000000000000000002200000000000000a34430313232186444333435361863444041424229581c11111111111111111111111111111111111111111111111111111111a34430313232386344333435361863444041424229";
static const char* CBOR_MIXED         = "a2581c00000000000000000000000000000000000000000000000000000000a34430313232186444333435361863444041424229581c11111111111111111111111111111111111111111111111111111111a34430313232386344333435361863444041424229";
static const char* ASSET_NAME_CBOR_1  = "49736b7977616c6b6571";
static const char* ASSET_NAME_CBOR_2  = "49736b7977616c6b6572";
static const char* ASSET_NAME_CBOR_3  = "49736b7977616c6b6573";
static const char* ASSET_NAME_CBOR_1B = "4430313232";
static const char* ASSET_NAME_CBOR_2B = "4433343536";
static const char* ASSET_NAME_CBOR_3B = "4440414242";
static const char* POLICY_ID_HEX_1B   = "00000000000000000000000000000000000000000000000000000000";
static const char* POLICY_ID_HEX_2B   = "11111111111111111111111111111111111111111111111111111111";
static const char* POLICY_ID_HEX_1    = "f0ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9a";
static const char* POLICY_ID_HEX_2    = "f1ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9a";
static const char* POLICY_ID_HEX_3    = "f2ff48bbb7bbe9d59a40f1ce90e9e9d0ff5002ec48f232b49ca0fb9a";
static const char* ASSET_MAP_CBOR     = "a349736b7977616c6b65710149736b7977616c6b65720249736b7977616c6b657303";

/* STATIC FUNCTIONS **********************************************************/

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

/* UNIT TESTS ****************************************************************/

TEST(cardano_multi_asset_new, canCreateAssetMultiAssets)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;

  // Act
  cardano_error_t error = cardano_multi_asset_new(&multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(multi_asset, testing::Not((cardano_multi_asset_t*)nullptr));

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
}

TEST(cardano_multi_asset_new, returnsErrorIfAssetMultiAssetsIsNull)
{
  // Act
  cardano_error_t error = cardano_multi_asset_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_multi_asset_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_multi_asset_t* multi_asset = nullptr;

  // Act
  cardano_error_t error = cardano_multi_asset_new(&multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(multi_asset, (cardano_multi_asset_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_multi_asset_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_multi_asset_t* multi_asset = nullptr;

  // Act
  cardano_error_t error = cardano_multi_asset_new(&multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(multi_asset, (cardano_multi_asset_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_multi_asset_to_cbor, canSerializeAnEmptyAssetMultiAssets)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  cardano_cbor_writer_t* writer      = cardano_cbor_writer_new();

  cardano_error_t error = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_to_cbor(multi_asset, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 3);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "a0");

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_multi_asset_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_multi_asset_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_multi_asset_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;

  cardano_error_t error = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_to_cbor(multi_asset, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
}

TEST(cardano_multi_asset_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t* writer      = cardano_cbor_writer_new();

  cardano_error_t error = cardano_multi_asset_from_cbor(reader, &multi_asset);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_to_cbor(multi_asset, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_multi_asset_from_cbor, returnErrorIfAssetMultiAssetsIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_multi_asset_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_multi_asset_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;

  // Act
  cardano_error_t error = cardano_multi_asset_from_cbor(nullptr, &multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_multi_asset_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_multi_asset_from_cbor(reader, &multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(multi_asset, (cardano_multi_asset_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_multi_asset_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_multi_asset_t* list   = nullptr;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_multi_asset_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_multi_asset_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  cardano_error_t        error       = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_multi_asset_ref(multi_asset);

  // Assert
  EXPECT_THAT(multi_asset, testing::Not((cardano_multi_asset_t*)nullptr));
  EXPECT_EQ(cardano_multi_asset_refcount(multi_asset), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_multi_asset_unref(&multi_asset);
  cardano_multi_asset_unref(&multi_asset);
}

TEST(cardano_multi_asset_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_multi_asset_ref(nullptr);
}

TEST(cardano_multi_asset_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;

  // Act
  cardano_multi_asset_unref(&multi_asset);
}

TEST(cardano_multi_asset_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_multi_asset_unref((cardano_multi_asset_t**)nullptr);
}

TEST(cardano_multi_asset_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  cardano_error_t        error       = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_multi_asset_ref(multi_asset);
  size_t ref_count = cardano_multi_asset_refcount(multi_asset);

  cardano_multi_asset_unref(&multi_asset);
  size_t valued_ref_count = cardano_multi_asset_refcount(multi_asset);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(valued_ref_count, 1);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
}

TEST(cardano_multi_asset_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  cardano_error_t        error       = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_multi_asset_ref(multi_asset);
  size_t ref_count = cardano_multi_asset_refcount(multi_asset);

  cardano_multi_asset_unref(&multi_asset);
  size_t valued_ref_count = cardano_multi_asset_refcount(multi_asset);

  cardano_multi_asset_unref(&multi_asset);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(valued_ref_count, 1);
  EXPECT_EQ(multi_asset, (cardano_multi_asset_t*)nullptr);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
}

TEST(cardano_multi_asset_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_multi_asset_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_multi_asset_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  const char*            message     = "This is a test message";

  // Act
  cardano_multi_asset_set_last_error(multi_asset, message);

  // Assert
  EXPECT_STREQ(cardano_multi_asset_get_last_error(multi_asset), "Object is NULL.");
}

TEST(cardano_multi_asset_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  cardano_error_t        error       = cardano_multi_asset_new(&multi_asset);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_multi_asset_set_last_error(multi_asset, message);

  // Assert
  EXPECT_STREQ(cardano_multi_asset_get_last_error(multi_asset), "");

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
}

TEST(cardano_multi_asset_from_cbor, returnErrorIfInvalidMap)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("a100", 4);

  // Act
  cardano_error_t error = cardano_multi_asset_from_cbor(reader, &multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_multi_asset_from_cbor, returnErrorIfInvalidValue)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("a349736b7977616c6b6571ef49736b7977616c6b65720249736b7977616c6b657303", strlen("a349736b7977616c6b65710149736b7977616c6b65720249736b7977616c6b657303"));

  // Act
  cardano_error_t error = cardano_multi_asset_from_cbor(reader, &multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_multi_asset_get_policy_count, returnsZeroIfObjectIsNull)
{
  // Act
  size_t size = cardano_multi_asset_get_policy_count(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_multi_asset_get_policy_count, returnsTheNumberOfElementsInTheList)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  cardano_error_t        error       = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* policy_id  = new_default_blake2b_hash(POLICY_ID_HEX_1);
  cardano_asset_name_t*   asset_name = new_default_asset_name(ASSET_NAME_CBOR_1);

  error = cardano_multi_asset_set(multi_asset, policy_id, asset_name, 1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t size = cardano_multi_asset_get_policy_count(multi_asset);

  // Assert
  EXPECT_EQ(size, 1);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_multi_asset_insert_assets, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_error_t error = cardano_multi_asset_insert_assets(nullptr, (cardano_blake2b_hash_t*)"", (cardano_asset_name_map_t*)"");

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_multi_asset_insert_assets, returnsErrorIfAssetNameIsNull)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  cardano_error_t        error       = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_insert_assets(multi_asset, nullptr, (cardano_asset_name_map_t*)"");

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
}

TEST(cardano_multi_asset_insert_assets, returnsErrorIfAssetNameMapIsNull)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  cardano_error_t        error       = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* policy_id = new_default_blake2b_hash(POLICY_ID_HEX_1);

  // Act
  error = cardano_multi_asset_insert_assets(multi_asset, policy_id, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_multi_asset_insert_assets, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  cardano_error_t        error       = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_map_t* asset_name_map = new_default_asset_name_map(ASSET_MAP_CBOR);
  cardano_blake2b_hash_t*   policy_id      = new_default_blake2b_hash(POLICY_ID_HEX_1);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_multi_asset_insert_assets(multi_asset, policy_id, asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_multi_asset_insert_assets, keepsElementsSortedByAssetName)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = NULL;
  cardano_error_t        error       = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_map_t* asset_name_map = new_default_asset_name_map(ASSET_MAP_CBOR);

  cardano_blake2b_hash_t* policy_id1 = new_default_blake2b_hash(POLICY_ID_HEX_1);
  cardano_blake2b_hash_t* policy_id2 = new_default_blake2b_hash(POLICY_ID_HEX_2);
  cardano_blake2b_hash_t* policy_id3 = new_default_blake2b_hash(POLICY_ID_HEX_3);

  // Act
  error = cardano_multi_asset_insert_assets(multi_asset, policy_id1, asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_insert_assets(multi_asset, policy_id2, asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_insert_assets(multi_asset, policy_id3, asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  size_t size = cardano_multi_asset_get_policy_count(multi_asset);

  EXPECT_EQ(size, 3);

  int64_t value1 = 0;
  int64_t value2 = 0;
  int64_t value3 = 0;

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);
  cardano_asset_name_t* asset_name2 = new_default_asset_name(ASSET_NAME_CBOR_2);
  cardano_asset_name_t* asset_name3 = new_default_asset_name(ASSET_NAME_CBOR_3);

  EXPECT_EQ(cardano_multi_asset_get(multi_asset, policy_id1, asset_name1, &value1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_get(multi_asset, policy_id2, asset_name2, &value2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_get(multi_asset, policy_id3, asset_name3, &value3), CARDANO_SUCCESS);

  EXPECT_EQ(value1, 1);
  EXPECT_EQ(value2, 2);
  EXPECT_EQ(value3, 3);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_blake2b_hash_unref(&policy_id1);
  cardano_blake2b_hash_unref(&policy_id2);
  cardano_blake2b_hash_unref(&policy_id3);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
  cardano_asset_name_unref(&asset_name3);
}

TEST(cardano_multi_asset_insert_assets, canOverrideValueIfAlreadyPresent)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  cardano_error_t        error       = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_map_t* asset_name_map = new_default_asset_name_map(ASSET_MAP_CBOR);

  cardano_blake2b_hash_t* policy_id1 = new_default_blake2b_hash(POLICY_ID_HEX_1);
  cardano_blake2b_hash_t* policy_id2 = new_default_blake2b_hash(POLICY_ID_HEX_2);
  cardano_blake2b_hash_t* policy_id3 = new_default_blake2b_hash(POLICY_ID_HEX_3);

  // Act
  error = cardano_multi_asset_insert_assets(multi_asset, policy_id1, asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_insert_assets(multi_asset, policy_id2, asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_insert_assets(multi_asset, policy_id3, asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_insert_assets(multi_asset, policy_id1, asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  size_t size = cardano_multi_asset_get_policy_count(multi_asset);

  EXPECT_EQ(size, 3);

  int64_t value1 = 0;
  int64_t value2 = 0;
  int64_t value3 = 0;

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);
  cardano_asset_name_t* asset_name2 = new_default_asset_name(ASSET_NAME_CBOR_2);
  cardano_asset_name_t* asset_name3 = new_default_asset_name(ASSET_NAME_CBOR_3);

  EXPECT_EQ(cardano_multi_asset_get(multi_asset, policy_id1, asset_name1, &value1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_get(multi_asset, policy_id2, asset_name2, &value2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_get(multi_asset, policy_id3, asset_name3, &value3), CARDANO_SUCCESS);

  EXPECT_EQ(value1, 1);
  EXPECT_EQ(value2, 2);
  EXPECT_EQ(value3, 3);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_blake2b_hash_unref(&policy_id1);
  cardano_blake2b_hash_unref(&policy_id2);
  cardano_blake2b_hash_unref(&policy_id3);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
  cardano_asset_name_unref(&asset_name3);
}

TEST(cardano_multi_asset_get, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_error_t error = cardano_multi_asset_get(nullptr, (cardano_blake2b_hash_t*)"", (cardano_asset_name_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_multi_asset_get, returnsErrorIfElementIsNull)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  cardano_error_t        error       = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_get(multi_asset, nullptr, nullptr, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
}

TEST(cardano_multi_asset_get, returnsErrorIfAssetNameIsNull)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  cardano_error_t        error       = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* policy_id = new_default_blake2b_hash(POLICY_ID_HEX_1);

  // Act
  error = cardano_multi_asset_get(multi_asset, policy_id, nullptr, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_multi_asset_get, returnsErrorIfElementNotFound)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  cardano_error_t        error       = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* policy_id  = new_default_blake2b_hash(POLICY_ID_HEX_1);
  cardano_asset_name_t*   asset_name = new_default_asset_name(ASSET_NAME_CBOR_1);

  // Act
  int64_t value = 0;
  error         = cardano_multi_asset_get(multi_asset, policy_id, asset_name, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_multi_asset_get, returnsTheElement)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  cardano_error_t        error       = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_map_t* asset_name_map = new_default_asset_name_map(ASSET_MAP_CBOR);
  cardano_asset_name_t*     asset_name     = new_default_asset_name(ASSET_NAME_CBOR_1);
  cardano_blake2b_hash_t*   policy_id      = new_default_blake2b_hash(POLICY_ID_HEX_1);

  error = cardano_multi_asset_insert_assets(multi_asset, policy_id, asset_name_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  int64_t value_out = 0;
  error             = cardano_multi_asset_get(multi_asset, policy_id, asset_name, &value_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value_out, 1);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_multi_asset_get, returnsTheRightElementIfMoreThanOne)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;
  cardano_error_t        error       = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_map_t* asset_name_map = new_default_asset_name_map(ASSET_MAP_CBOR);
  cardano_asset_name_t*     asset_name1    = new_default_asset_name(ASSET_NAME_CBOR_1);
  cardano_asset_name_t*     asset_name2    = new_default_asset_name(ASSET_NAME_CBOR_2);
  cardano_blake2b_hash_t*   policy_id1     = new_default_blake2b_hash(POLICY_ID_HEX_1);
  cardano_blake2b_hash_t*   policy_id2     = new_default_blake2b_hash(POLICY_ID_HEX_2);

  error = cardano_multi_asset_insert_assets(multi_asset, policy_id1, asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_insert_assets(multi_asset, policy_id2, asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  int64_t value_out1 = 0;
  int64_t value_out2 = 0;

  error = cardano_multi_asset_get(multi_asset, policy_id1, asset_name1, &value_out1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_get(multi_asset, policy_id2, asset_name2, &value_out2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(value_out1, 1);
  EXPECT_EQ(value_out2, 2);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_blake2b_hash_unref(&policy_id1);
  cardano_blake2b_hash_unref(&policy_id2);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
}

TEST(cardano_multi_asset_get_keys, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_policy_id_list_t* policies = nullptr;

  // Act
  cardano_error_t error = cardano_multi_asset_get_keys(nullptr, &policies);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_multi_asset_get_keys, returnsErrorIfOutIsNull)
{
  // Act
  cardano_error_t error = cardano_multi_asset_get_keys((cardano_multi_asset_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_multi_asset_get_keys, returnsTheListOfKeys)
{
  // Arrange
  cardano_multi_asset_t*    multi_asset = nullptr;
  cardano_policy_id_list_t* policies    = nullptr;
  cardano_error_t           error       = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_map_t* asset_name_map = new_default_asset_name_map(ASSET_MAP_CBOR);
  cardano_blake2b_hash_t*   policy_id1     = new_default_blake2b_hash(POLICY_ID_HEX_1);
  cardano_blake2b_hash_t*   policy_id2     = new_default_blake2b_hash(POLICY_ID_HEX_2);
  cardano_blake2b_hash_t*   policy_id3     = new_default_blake2b_hash(POLICY_ID_HEX_3);

  error = cardano_multi_asset_insert_assets(multi_asset, policy_id1, asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_insert_assets(multi_asset, policy_id2, asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_insert_assets(multi_asset, policy_id3, asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_get_keys(multi_asset, &policies);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_policy_id_list_get_length(policies), 3);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_blake2b_hash_unref(&policy_id1);
  cardano_blake2b_hash_unref(&policy_id2);
  cardano_blake2b_hash_unref(&policy_id3);
  cardano_policy_id_list_unref(&policies);
}

TEST(cardano_multi_asset_add, returnsErrorIfLhsIsNull)
{
  // Arrange
  cardano_multi_asset_t* lhs_multi_asset    = nullptr;
  cardano_multi_asset_t* rhs_multi_asset    = nullptr;
  cardano_multi_asset_t* result_multi_asset = nullptr;

  // Act
  cardano_error_t error = cardano_multi_asset_add(lhs_multi_asset, rhs_multi_asset, &result_multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_multi_asset_add, returnsErrorIfRhsIsNull)
{
  // Arrange
  cardano_multi_asset_t* rhs_multi_asset    = nullptr;
  cardano_multi_asset_t* result_multi_asset = nullptr;

  // Act
  cardano_error_t error = cardano_multi_asset_add((cardano_multi_asset_t*)"", rhs_multi_asset, &result_multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_multi_asset_add, returnsErrorIfOutIsNull)
{
  // Arrange
  cardano_multi_asset_t* lhs_multi_asset = nullptr;
  cardano_multi_asset_t* rhs_multi_asset = nullptr;

  // Act
  cardano_error_t error = cardano_multi_asset_add(lhs_multi_asset, rhs_multi_asset, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_multi_asset_add, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_multi_asset_t* lhs_multi_asset    = nullptr;
  cardano_multi_asset_t* rhs_multi_asset    = nullptr;
  cardano_multi_asset_t* result_multi_asset = nullptr;

  cardano_error_t error = cardano_multi_asset_new(&lhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_new(&rhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_multi_asset_add(lhs_multi_asset, rhs_multi_asset, &result_multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);
  cardano_multi_asset_unref(&result_multi_asset);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_multi_asset_add, canAddTwoEmptyMultiAssets)
{
  // Arrange
  cardano_multi_asset_t* lhs_multi_asset    = nullptr;
  cardano_multi_asset_t* rhs_multi_asset    = nullptr;
  cardano_multi_asset_t* result_multi_asset = nullptr;

  cardano_error_t error = cardano_multi_asset_new(&lhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_new(&rhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_add(lhs_multi_asset, rhs_multi_asset, &result_multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_get_policy_count(result_multi_asset), 0);

  // Cleanup
  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);
  cardano_multi_asset_unref(&result_multi_asset);
}

TEST(cardano_multi_asset_add, canAddTwoMultiAssets)
{
  // Arrange
  cardano_multi_asset_t* lhs_multi_asset    = nullptr;
  cardano_multi_asset_t* rhs_multi_asset    = nullptr;
  cardano_multi_asset_t* result_multi_asset = nullptr;

  cardano_error_t error = cardano_multi_asset_new(&lhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_new(&rhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_map_t* asset_name_map = new_default_asset_name_map(ASSET_MAP_CBOR);
  cardano_blake2b_hash_t*   policy_id      = new_default_blake2b_hash(POLICY_ID_HEX_1);

  error = cardano_multi_asset_insert_assets(lhs_multi_asset, policy_id, asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_insert_assets(rhs_multi_asset, policy_id, asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_add(lhs_multi_asset, rhs_multi_asset, &result_multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_get_policy_count(result_multi_asset), 1);

  int64_t value = 0;

  cardano_asset_name_t* asset_name = new_default_asset_name(ASSET_NAME_CBOR_1);

  error = cardano_multi_asset_get(result_multi_asset, policy_id, asset_name, &value);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(value, 2);

  // Cleanup
  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);
  cardano_multi_asset_unref(&result_multi_asset);
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_multi_asset_add, canAddTwoMultiAssets2)
{
  // Arrange
  cardano_multi_asset_t* lhs_multi_asset    = nullptr;
  cardano_multi_asset_t* rhs_multi_asset    = nullptr;
  cardano_multi_asset_t* result_multi_asset = nullptr;

  cardano_error_t error = cardano_multi_asset_new(&lhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_new(&rhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_map_t* asset_name_map = new_default_asset_name_map(ASSET_MAP_CBOR);
  cardano_blake2b_hash_t*   policy_id1     = new_default_blake2b_hash(POLICY_ID_HEX_1);
  cardano_blake2b_hash_t*   policy_id2     = new_default_blake2b_hash(POLICY_ID_HEX_2);

  error = cardano_multi_asset_insert_assets(lhs_multi_asset, policy_id1, asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_insert_assets(rhs_multi_asset, policy_id2, asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_add(lhs_multi_asset, rhs_multi_asset, &result_multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_get_policy_count(result_multi_asset), 2);

  int64_t value = 0;

  cardano_asset_name_t* asset_name = new_default_asset_name(ASSET_NAME_CBOR_1);

  error = cardano_multi_asset_get(result_multi_asset, policy_id1, asset_name, &value);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(value, 1);

  error = cardano_multi_asset_get(result_multi_asset, policy_id2, asset_name, &value);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(value, 1);

  // Cleanup
  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);
  cardano_multi_asset_unref(&result_multi_asset);
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_blake2b_hash_unref(&policy_id1);
  cardano_blake2b_hash_unref(&policy_id2);
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_multi_asset_add, canAddTwoMultiAssetsAndAddsPositiveValuesForSameAssetName)
{
  // Arrange
  cardano_multi_asset_t*  lhs_multi_asset    = nullptr;
  cardano_multi_asset_t*  rhs_multi_asset    = nullptr;
  cardano_multi_asset_t*  result_multi_asset = nullptr;
  cardano_blake2b_hash_t* policy_id          = new_default_blake2b_hash(POLICY_ID_HEX_1);

  cardano_error_t error = cardano_multi_asset_new(&lhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_new(&rhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);
  cardano_asset_name_t* asset_name2 = new_default_asset_name(ASSET_NAME_CBOR_2);

  error = cardano_multi_asset_set(lhs_multi_asset, policy_id, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_set(rhs_multi_asset, policy_id, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_set(rhs_multi_asset, policy_id, asset_name2, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_add(lhs_multi_asset, rhs_multi_asset, &result_multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_get_policy_count(result_multi_asset), 1);

  int64_t value1 = 0;
  int64_t value2 = 0;

  error = cardano_multi_asset_get(result_multi_asset, policy_id, asset_name1, &value1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_get(result_multi_asset, policy_id, asset_name2, &value2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(value1, 2);
  EXPECT_EQ(value2, 1);

  // Cleanup
  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);
  cardano_multi_asset_unref(&result_multi_asset);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_multi_asset_add, canAddTwoMultiAssetsAndAddsNegativeValuesForSameAssetName)
{
  // Arrange
  cardano_multi_asset_t*  lhs_multi_asset    = nullptr;
  cardano_multi_asset_t*  rhs_multi_asset    = nullptr;
  cardano_multi_asset_t*  result_multi_asset = nullptr;
  cardano_blake2b_hash_t* policy_id          = new_default_blake2b_hash(POLICY_ID_HEX_1);

  cardano_error_t error = cardano_multi_asset_new(&lhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_new(&rhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);
  cardano_asset_name_t* asset_name2 = new_default_asset_name(ASSET_NAME_CBOR_2);

  error = cardano_multi_asset_set(lhs_multi_asset, policy_id, asset_name1, -1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_set(rhs_multi_asset, policy_id, asset_name1, -1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_set(rhs_multi_asset, policy_id, asset_name2, -1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_add(lhs_multi_asset, rhs_multi_asset, &result_multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_get_policy_count(result_multi_asset), 1);

  int64_t value1 = 0;
  int64_t value2 = 0;

  error = cardano_multi_asset_get(result_multi_asset, policy_id, asset_name1, &value1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_get(result_multi_asset, policy_id, asset_name2, &value2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(value1, -2);
  EXPECT_EQ(value2, -1);

  // Cleanup
  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);
  cardano_multi_asset_unref(&result_multi_asset);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_multi_asset_add, returnsErrorIfElementIsNull)
{
  // Arrange
  cardano_multi_asset_t* lhs_multi_asset = nullptr;
  cardano_multi_asset_t* rhs_multi_asset = nullptr;

  cardano_error_t error = cardano_multi_asset_new(&lhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_new(&rhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_add(lhs_multi_asset, rhs_multi_asset, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);
}

TEST(cardano_multi_asset_subtract, returnsErrorIfLhsIsNull)
{
  // Arrange
  cardano_multi_asset_t* lhs_multi_asset    = nullptr;
  cardano_multi_asset_t* rhs_multi_asset    = nullptr;
  cardano_multi_asset_t* result_multi_asset = nullptr;

  // Act
  cardano_error_t error = cardano_multi_asset_subtract(lhs_multi_asset, rhs_multi_asset, &result_multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_multi_asset_subtract, returnsErrorIfRhsIsNull)
{
  // Arrange
  cardano_multi_asset_t* rhs_multi_asset    = nullptr;
  cardano_multi_asset_t* result_multi_asset = nullptr;

  // Act
  cardano_error_t error = cardano_multi_asset_subtract((cardano_multi_asset_t*)"", rhs_multi_asset, &result_multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_multi_asset_subtract, returnsErrorIfOutIsNull)
{
  // Arrange
  cardano_multi_asset_t* lhs_multi_asset = nullptr;
  cardano_multi_asset_t* rhs_multi_asset = nullptr;

  // Act
  cardano_error_t error = cardano_multi_asset_subtract(lhs_multi_asset, rhs_multi_asset, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_multi_asset_subtract, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_multi_asset_t* lhs_multi_asset    = nullptr;
  cardano_multi_asset_t* rhs_multi_asset    = nullptr;
  cardano_multi_asset_t* result_multi_asset = nullptr;

  cardano_error_t error = cardano_multi_asset_new(&lhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_new(&rhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_multi_asset_subtract(lhs_multi_asset, rhs_multi_asset, &result_multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);
  cardano_multi_asset_unref(&result_multi_asset);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_multi_asset_subtract, canSubtractTwoEmptyMultiAssets)
{
  // Arrange
  cardano_multi_asset_t* lhs_multi_asset    = nullptr;
  cardano_multi_asset_t* rhs_multi_asset    = nullptr;
  cardano_multi_asset_t* result_multi_asset = nullptr;

  cardano_error_t error = cardano_multi_asset_new(&lhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_new(&rhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_subtract(lhs_multi_asset, rhs_multi_asset, &result_multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_get_policy_count(result_multi_asset), 0);

  // Cleanup
  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);
  cardano_multi_asset_unref(&result_multi_asset);
}

TEST(cardano_multi_asset_subtract, canSubtractTwoMultiAssets)
{
  // Arrange
  cardano_multi_asset_t*  lhs_multi_asset    = nullptr;
  cardano_multi_asset_t*  rhs_multi_asset    = nullptr;
  cardano_multi_asset_t*  result_multi_asset = nullptr;
  cardano_blake2b_hash_t* policy_id          = new_default_blake2b_hash(POLICY_ID_HEX_1);

  cardano_error_t error = cardano_multi_asset_new(&lhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_new(&rhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);
  cardano_asset_name_t* asset_name2 = new_default_asset_name(ASSET_NAME_CBOR_2);

  error = cardano_multi_asset_set(lhs_multi_asset, policy_id, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_set(rhs_multi_asset, policy_id, asset_name2, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_subtract(lhs_multi_asset, rhs_multi_asset, &result_multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_get_policy_count(result_multi_asset), 1);

  int64_t value1 = 0;
  int64_t value2 = 0;

  error = cardano_multi_asset_get(result_multi_asset, policy_id, asset_name1, &value1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_get(result_multi_asset, policy_id, asset_name2, &value2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(value1, 1);
  EXPECT_EQ(value2, -1);

  // Cleanup
  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);
  cardano_multi_asset_unref(&result_multi_asset);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_multi_asset_subtract, canSubtractTwoMultiAssets2)
{
  // Arrange
  cardano_multi_asset_t*  lhs_multi_asset    = nullptr;
  cardano_multi_asset_t*  rhs_multi_asset    = nullptr;
  cardano_multi_asset_t*  result_multi_asset = nullptr;
  cardano_blake2b_hash_t* policy_id1         = new_default_blake2b_hash(POLICY_ID_HEX_1);
  cardano_blake2b_hash_t* policy_id2         = new_default_blake2b_hash(POLICY_ID_HEX_2);

  cardano_error_t error = cardano_multi_asset_new(&lhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_new(&rhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);
  cardano_asset_name_t* asset_name2 = new_default_asset_name(ASSET_NAME_CBOR_2);

  error = cardano_multi_asset_set(lhs_multi_asset, policy_id1, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_set(rhs_multi_asset, policy_id2, asset_name2, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_subtract(lhs_multi_asset, rhs_multi_asset, &result_multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_get_policy_count(result_multi_asset), 2);

  int64_t value1 = 0;
  int64_t value2 = 0;

  error = cardano_multi_asset_get(result_multi_asset, policy_id1, asset_name1, &value1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_get(result_multi_asset, policy_id2, asset_name2, &value2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(value1, 1);
  EXPECT_EQ(value2, -1);

  // Cleanup
  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);
  cardano_multi_asset_unref(&result_multi_asset);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
  cardano_blake2b_hash_unref(&policy_id1);
  cardano_blake2b_hash_unref(&policy_id2);
}

TEST(cardano_multi_asset_subtract, canSubtractTwoMultiAssetsAndSubtractsPositiveValuesForSameAssetName)
{
  // Arrange
  cardano_multi_asset_t*  lhs_multi_asset    = nullptr;
  cardano_multi_asset_t*  rhs_multi_asset    = nullptr;
  cardano_multi_asset_t*  result_multi_asset = nullptr;
  cardano_blake2b_hash_t* policy_id          = new_default_blake2b_hash(POLICY_ID_HEX_1);

  cardano_error_t error = cardano_multi_asset_new(&lhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_new(&rhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);
  cardano_asset_name_t* asset_name2 = new_default_asset_name(ASSET_NAME_CBOR_2);

  error = cardano_multi_asset_set(lhs_multi_asset, policy_id, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_set(rhs_multi_asset, policy_id, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_set(rhs_multi_asset, policy_id, asset_name2, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_subtract(lhs_multi_asset, rhs_multi_asset, &result_multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_get_policy_count(result_multi_asset), 1);

  int64_t value1 = 0;
  int64_t value2 = 0;

  error = cardano_multi_asset_get(result_multi_asset, policy_id, asset_name1, &value1);
  ASSERT_EQ(error, CARDANO_ELEMENT_NOT_FOUND);

  error = cardano_multi_asset_get(result_multi_asset, policy_id, asset_name2, &value2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(value2, -1);

  // Cleanup
  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);
  cardano_multi_asset_unref(&result_multi_asset);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_multi_asset_subtract, canSubtractTwoMultiAssetsAndSubtractsNegativeValuesForSameAssetName)
{
  // Arrange
  cardano_multi_asset_t*  lhs_multi_asset    = nullptr;
  cardano_multi_asset_t*  rhs_multi_asset    = nullptr;
  cardano_multi_asset_t*  result_multi_asset = nullptr;
  cardano_blake2b_hash_t* policy_id          = new_default_blake2b_hash(POLICY_ID_HEX_1);

  cardano_error_t error = cardano_multi_asset_new(&lhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_new(&rhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);
  cardano_asset_name_t* asset_name2 = new_default_asset_name(ASSET_NAME_CBOR_2);

  error = cardano_multi_asset_set(lhs_multi_asset, policy_id, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_set(rhs_multi_asset, policy_id, asset_name1, 4);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_set(rhs_multi_asset, policy_id, asset_name2, -1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_subtract(lhs_multi_asset, rhs_multi_asset, &result_multi_asset);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_get_policy_count(result_multi_asset), 1);

  int64_t value1 = 0;
  int64_t value2 = 0;

  error = cardano_multi_asset_get(result_multi_asset, policy_id, asset_name1, &value1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_get(result_multi_asset, policy_id, asset_name2, &value2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(value1, -3);
  EXPECT_EQ(value2, 1);

  // Cleanup
  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);
  cardano_multi_asset_unref(&result_multi_asset);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_multi_asset_subtract, returnsErrorIfElementIsNull)
{
  // Arrange
  cardano_multi_asset_t* lhs_multi_asset = nullptr;
  cardano_multi_asset_t* rhs_multi_asset = nullptr;

  cardano_error_t error = cardano_multi_asset_new(&lhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_new(&rhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_subtract(lhs_multi_asset, rhs_multi_asset, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);
}

TEST(cardano_multi_asset_equals, returnsErrorIfLhsIsNull)
{
  // Arrange
  cardano_multi_asset_t* lhs_multi_asset = nullptr;
  cardano_multi_asset_t* rhs_multi_asset = (cardano_multi_asset_t*)"";

  // Act
  bool result = cardano_multi_asset_equals(lhs_multi_asset, rhs_multi_asset);

  // Assert
  EXPECT_FALSE(result);
}

TEST(cardano_multi_asset_equals, returnsErrorIfRhsIsNull)
{
  // Arrange
  cardano_multi_asset_t* lhs_multi_asset = (cardano_multi_asset_t*)"";
  cardano_multi_asset_t* rhs_multi_asset = nullptr;

  // Act
  bool result = cardano_multi_asset_equals(lhs_multi_asset, rhs_multi_asset);

  // Assert
  EXPECT_FALSE(result);
}

TEST(cardano_multi_asset_equals, returnsTrueIfBothAreNull)
{
  // Arrange
  cardano_multi_asset_t* lhs_multi_asset = nullptr;
  cardano_multi_asset_t* rhs_multi_asset = nullptr;

  // Act
  bool result = cardano_multi_asset_equals(lhs_multi_asset, rhs_multi_asset);

  // Assert
  EXPECT_TRUE(result);
}

TEST(cardano_multi_asset_equals, returnsTrueIfBothAreEmpty)
{
  // Arrange
  cardano_multi_asset_t* lhs_multi_asset = nullptr;
  cardano_multi_asset_t* rhs_multi_asset = nullptr;

  cardano_error_t error = cardano_multi_asset_new(&lhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_new(&rhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_multi_asset_equals(lhs_multi_asset, rhs_multi_asset);

  // Assert
  EXPECT_TRUE(result);

  // Cleanup
  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);
}

TEST(cardano_multi_asset_equals, returnsFalseIfLengthsAreDifferent)
{
  // Arrange
  cardano_multi_asset_t*  lhs_multi_asset = nullptr;
  cardano_multi_asset_t*  rhs_multi_asset = nullptr;
  cardano_blake2b_hash_t* policy_id       = new_default_blake2b_hash(POLICY_ID_HEX_1);

  cardano_error_t error = cardano_multi_asset_new(&lhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_new(&rhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name = new_default_asset_name(ASSET_NAME_CBOR_1);

  error = cardano_multi_asset_set(lhs_multi_asset, policy_id, asset_name, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_multi_asset_equals(lhs_multi_asset, rhs_multi_asset);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_multi_asset_equals, returnsFalseIfValuesAreDifferent)
{
  // Arrange
  cardano_multi_asset_t*  lhs_multi_asset = nullptr;
  cardano_multi_asset_t*  rhs_multi_asset = nullptr;
  cardano_blake2b_hash_t* policy_id       = new_default_blake2b_hash(POLICY_ID_HEX_1);

  cardano_error_t error = cardano_multi_asset_new(&lhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_new(&rhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);

  error = cardano_multi_asset_set(lhs_multi_asset, policy_id, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_set(rhs_multi_asset, policy_id, asset_name1, 2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_multi_asset_equals(lhs_multi_asset, rhs_multi_asset);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);
  cardano_asset_name_unref(&asset_name1);
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_multi_asset_equals, returnsTrueIfMultiAssetsAreEqual)
{
  // Arrange
  cardano_multi_asset_t*  lhs_multi_asset = nullptr;
  cardano_multi_asset_t*  rhs_multi_asset = nullptr;
  cardano_blake2b_hash_t* policy_id       = new_default_blake2b_hash(POLICY_ID_HEX_1);

  cardano_error_t error = cardano_multi_asset_new(&lhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_new(&rhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_asset_name_t* asset_name1 = new_default_asset_name(ASSET_NAME_CBOR_1);

  error = cardano_multi_asset_set(lhs_multi_asset, policy_id, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_set(rhs_multi_asset, policy_id, asset_name1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_multi_asset_equals(lhs_multi_asset, rhs_multi_asset);

  // Assert
  EXPECT_TRUE(result);

  // Cleanup
  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);
  cardano_asset_name_unref(&asset_name1);
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_multi_asset_equals, retursFalseIfNotEquals)
{
  // Arrange
  cardano_multi_asset_t* lhs_multi_asset = nullptr;
  cardano_multi_asset_t* rhs_multi_asset = nullptr;

  cardano_cbor_reader_t* lhs_reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_reader_t* rhs_reader = cardano_cbor_reader_from_hex(CBOR_MIXED2, strlen(CBOR_MIXED2));

  cardano_error_t error = cardano_multi_asset_from_cbor(lhs_reader, &lhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_from_cbor(rhs_reader, &rhs_multi_asset);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool result = cardano_multi_asset_equals(lhs_multi_asset, rhs_multi_asset);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_multi_asset_unref(&lhs_multi_asset);
  cardano_multi_asset_unref(&rhs_multi_asset);
  cardano_cbor_reader_unref(&lhs_reader);
  cardano_cbor_reader_unref(&rhs_reader);
}

TEST(cardano_multi_asset_insert_assets, returnsErrorIfMultiAssetIsNull)
{
  // Arrange
  cardano_multi_asset_t*    multi_asset    = nullptr;
  cardano_blake2b_hash_t*   policy_id      = new_default_blake2b_hash(POLICY_ID_HEX_1);
  cardano_asset_name_map_t* asset_name_map = new_default_asset_name_map(ASSET_MAP_CBOR);

  // Act
  cardano_error_t error = cardano_multi_asset_insert_assets(multi_asset, policy_id, asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_map_unref(&asset_name_map);
}

TEST(cardano_multi_asset_get_assets, returnsErrorIfMultiAssetIsNull)
{
  // Arrange
  cardano_multi_asset_t*    multi_asset    = nullptr;
  cardano_blake2b_hash_t*   policy_id      = new_default_blake2b_hash(POLICY_ID_HEX_1);
  cardano_asset_name_map_t* asset_name_map = nullptr;

  // Act
  cardano_error_t error = cardano_multi_asset_get_assets(multi_asset, policy_id, &asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_multi_asset_get_assets, returnsErrorIfPolicyIdIsNull)
{
  // Arrange
  cardano_multi_asset_t*  multi_asset = nullptr;
  cardano_blake2b_hash_t* policy_id   = nullptr;

  cardano_error_t error = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_asset_name_map_t* asset_name_map = nullptr;
  error                                    = cardano_multi_asset_get_assets(multi_asset, policy_id, &asset_name_map);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
}

TEST(cardano_multi_asset_get_assets, returnsErrorIfAssetNameMapIsNull)
{
  // Arrange
  cardano_multi_asset_t*    multi_asset    = nullptr;
  cardano_blake2b_hash_t*   policy_id      = new_default_blake2b_hash(POLICY_ID_HEX_1);
  cardano_asset_name_map_t* asset_name_map = nullptr;
  cardano_asset_name_map_t* new_asset_map  = new_default_asset_name_map(ASSET_MAP_CBOR);

  cardano_error_t error = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_insert_assets(multi_asset, policy_id, new_asset_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_get_assets(multi_asset, policy_id, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_map_unref(&asset_name_map);
  cardano_asset_name_map_unref(&new_asset_map);
}

TEST(cardano_multi_asset_get, returnsErrorIfMultiAssetIsNull)
{
  // Arrange
  cardano_multi_asset_t*  multi_asset = nullptr;
  cardano_blake2b_hash_t* policy_id   = new_default_blake2b_hash(POLICY_ID_HEX_1);
  cardano_asset_name_t*   asset_name  = new_default_asset_name(ASSET_NAME_CBOR_1);

  // Act
  int64_t         value_out = 0;
  cardano_error_t error     = cardano_multi_asset_get(multi_asset, policy_id, asset_name, &value_out);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_multi_asset_get, returnsErrorIfPolicyIdIsNull)
{
  // Arrange
  cardano_multi_asset_t*  multi_asset = nullptr;
  cardano_blake2b_hash_t* policy_id   = nullptr;
  cardano_asset_name_t*   asset_name  = new_default_asset_name(ASSET_NAME_CBOR_1);

  cardano_error_t error = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  int64_t value_out = 0;
  error             = cardano_multi_asset_get(multi_asset, policy_id, asset_name, &value_out);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_multi_asset_get, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_multi_asset_t*    multi_asset    = nullptr;
  cardano_blake2b_hash_t*   policy_id      = new_default_blake2b_hash(POLICY_ID_HEX_1);
  cardano_asset_name_t*     asset_name     = new_default_asset_name(ASSET_NAME_CBOR_1);
  cardano_asset_name_map_t* asset_name_map = new_default_asset_name_map(ASSET_MAP_CBOR);

  cardano_error_t error = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_multi_asset_insert_assets(multi_asset, policy_id, asset_name_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_get(multi_asset, policy_id, asset_name, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_map_unref(&asset_name_map);
}

TEST(cardano_multi_asset_set, returnsErrorIfMultiAssetIsNull)
{
  // Arrange
  cardano_multi_asset_t*  multi_asset = nullptr;
  cardano_blake2b_hash_t* policy_id   = new_default_blake2b_hash(POLICY_ID_HEX_1);
  cardano_asset_name_t*   asset_name  = new_default_asset_name(ASSET_NAME_CBOR_1);

  // Act
  cardano_error_t error = cardano_multi_asset_set(multi_asset, policy_id, asset_name, 1);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_multi_asset_set, returnsErrorIfPolicyIdIsNull)
{
  // Arrange
  cardano_multi_asset_t*  multi_asset = nullptr;
  cardano_blake2b_hash_t* policy_id   = nullptr;
  cardano_asset_name_t*   asset_name  = new_default_asset_name(ASSET_NAME_CBOR_1);

  cardano_error_t error = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_set(multi_asset, policy_id, asset_name, 1);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
  cardano_asset_name_unref(&asset_name);
}

TEST(cardano_multi_asset_set, returnsErrorIfAssetNameIsNull)
{
  // Arrange
  cardano_multi_asset_t*  multi_asset = nullptr;
  cardano_blake2b_hash_t* policy_id   = new_default_blake2b_hash(POLICY_ID_HEX_1);
  cardano_asset_name_t*   asset_name  = nullptr;

  cardano_error_t error = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_set(multi_asset, policy_id, asset_name, 1);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_multi_asset_get_positive, returnsThePositiveAssets)
{
  // Arrange
  cardano_multi_asset_t*  multi_asset     = nullptr;
  cardano_multi_asset_t*  multi_asset_out = nullptr;
  cardano_blake2b_hash_t* policy_id1      = new_default_blake2b_hash(POLICY_ID_HEX_1B);
  cardano_blake2b_hash_t* policy_id2      = new_default_blake2b_hash(POLICY_ID_HEX_2B);
  cardano_asset_name_t*   asset_name1     = new_default_asset_name(ASSET_NAME_CBOR_1B);
  cardano_asset_name_t*   asset_name2     = new_default_asset_name(ASSET_NAME_CBOR_2B);
  cardano_asset_name_t*   asset_name3     = new_default_asset_name(ASSET_NAME_CBOR_3B);
  cardano_cbor_reader_t*  reader          = cardano_cbor_reader_from_hex(CBOR_MIXED, strlen(CBOR_MIXED));
  cardano_error_t         error           = cardano_multi_asset_from_cbor(reader, &multi_asset);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_get_positive(multi_asset, &multi_asset_out);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_policy_id_list_t* policy_ids = nullptr;

  error = cardano_multi_asset_get_keys(multi_asset, &policy_ids);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  size_t policy_count = cardano_policy_id_list_get_length(policy_ids);

  EXPECT_EQ(policy_count, 2);

  int64_t val1 = 0;
  int64_t val2 = 0;
  int64_t val3 = 0;

  error = cardano_multi_asset_get(multi_asset_out, policy_id1, asset_name1, &val1);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(val1, 100);

  error = cardano_multi_asset_get(multi_asset_out, policy_id1, asset_name2, &val2);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(val2, 99);

  error = cardano_multi_asset_get(multi_asset_out, policy_id1, asset_name3, &val3);

  EXPECT_EQ(error, CARDANO_ELEMENT_NOT_FOUND);
  EXPECT_EQ(val3, 0);

  val1 = 0;
  val2 = 0;
  val3 = 0;

  error = cardano_multi_asset_get(multi_asset_out, policy_id2, asset_name1, &val1);

  EXPECT_EQ(error, CARDANO_ELEMENT_NOT_FOUND);
  EXPECT_EQ(val1, 0);

  error = cardano_multi_asset_get(multi_asset_out, policy_id2, asset_name2, &val2);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(val2, 99);

  error = cardano_multi_asset_get(multi_asset_out, policy_id2, asset_name3, &val3);

  EXPECT_EQ(error, CARDANO_ELEMENT_NOT_FOUND);
  EXPECT_EQ(val3, 0);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
  cardano_multi_asset_unref(&multi_asset_out);
  cardano_policy_id_list_unref(&policy_ids);
  cardano_blake2b_hash_unref(&policy_id1);
  cardano_blake2b_hash_unref(&policy_id2);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
  cardano_asset_name_unref(&asset_name3);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_multi_asset_get_negative, returnsTheNegativeAssets)
{
  // Arrange
  cardano_multi_asset_t*  multi_asset     = nullptr;
  cardano_multi_asset_t*  multi_asset_out = nullptr;
  cardano_blake2b_hash_t* policy_id1      = new_default_blake2b_hash(POLICY_ID_HEX_1B);
  cardano_blake2b_hash_t* policy_id2      = new_default_blake2b_hash(POLICY_ID_HEX_2B);
  cardano_asset_name_t*   asset_name1     = new_default_asset_name(ASSET_NAME_CBOR_1B);
  cardano_asset_name_t*   asset_name2     = new_default_asset_name(ASSET_NAME_CBOR_2B);
  cardano_asset_name_t*   asset_name3     = new_default_asset_name(ASSET_NAME_CBOR_3B);
  cardano_cbor_reader_t*  reader          = cardano_cbor_reader_from_hex(CBOR_MIXED, strlen(CBOR_MIXED));
  cardano_error_t         error           = cardano_multi_asset_from_cbor(reader, &multi_asset);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_get_negative(multi_asset, &multi_asset_out);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_policy_id_list_t* policy_ids = nullptr;

  error = cardano_multi_asset_get_keys(multi_asset, &policy_ids);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  size_t policy_count = cardano_policy_id_list_get_length(policy_ids);

  EXPECT_EQ(policy_count, 2);

  int64_t val1 = 0;
  int64_t val2 = 0;
  int64_t val3 = 0;

  error = cardano_multi_asset_get(multi_asset_out, policy_id1, asset_name1, &val1);

  EXPECT_EQ(error, CARDANO_ELEMENT_NOT_FOUND);
  EXPECT_EQ(val1, 0);

  error = cardano_multi_asset_get(multi_asset_out, policy_id1, asset_name2, &val2);

  EXPECT_EQ(error, CARDANO_ELEMENT_NOT_FOUND);
  EXPECT_EQ(val2, 0);

  error = cardano_multi_asset_get(multi_asset_out, policy_id1, asset_name3, &val3);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(val3, -10);

  val1 = 0;
  val2 = 0;
  val3 = 0;

  error = cardano_multi_asset_get(multi_asset_out, policy_id2, asset_name1, &val1);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(val1, -100);

  error = cardano_multi_asset_get(multi_asset_out, policy_id2, asset_name2, &val2);

  EXPECT_EQ(error, CARDANO_ELEMENT_NOT_FOUND);
  EXPECT_EQ(val2, 0);

  error = cardano_multi_asset_get(multi_asset_out, policy_id2, asset_name3, &val3);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(val3, -10);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
  cardano_multi_asset_unref(&multi_asset_out);
  cardano_cbor_reader_unref(&reader);
  cardano_policy_id_list_unref(&policy_ids);
  cardano_blake2b_hash_unref(&policy_id1);
  cardano_blake2b_hash_unref(&policy_id2);
  cardano_asset_name_unref(&asset_name1);
  cardano_asset_name_unref(&asset_name2);
  cardano_asset_name_unref(&asset_name3);
}

TEST(cardano_multi_asset_get_positive, returnsErrorIfFirstArgumentIsNull)
{
  // Arrange
  cardano_multi_asset_t* multi_asset_out = nullptr;

  // Act
  cardano_error_t error = cardano_multi_asset_get_positive(nullptr, &multi_asset_out);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_multi_asset_get_positive, returnsErrorIfSecondArgumentIsNull)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;

  cardano_error_t error = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_get_positive(multi_asset, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
}

TEST(cardano_multi_asset_get_negative, returnsErrorIfFirstArgumentIsNull)
{
  // Arrange
  cardano_multi_asset_t* multi_asset_out = nullptr;

  // Act
  cardano_error_t error = cardano_multi_asset_get_negative(nullptr, &multi_asset_out);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_multi_asset_get_negative, returnsErrorIfSecondArgumentIsNull)
{
  // Arrange
  cardano_multi_asset_t* multi_asset = nullptr;

  cardano_error_t error = cardano_multi_asset_new(&multi_asset);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_multi_asset_get_negative(multi_asset, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_multi_asset_unref(&multi_asset);
}
