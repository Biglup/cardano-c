/**
 * \file required_guards_map.cpp
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
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/transaction_body/required_guards_map.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* KEY_HASH_HEX    = "00112233445566778899aabbccddeeff00112233445566778899aabb";
static const char* SCRIPT_HASH_HEX = "aabbccddeeff00112233445566778899aabbccddeeff001122334455";

static const char* KEY_HASH_CREDENTIAL_CBOR    = "8200581c00112233445566778899aabbccddeeff00112233445566778899aabb";
static const char* SCRIPT_HASH_CREDENTIAL_CBOR = "8201581caabbccddeeff00112233445566778899aabbccddeeff001122334455";
static const char* PLUTUS_DATA_CBOR            = "d87980";

static const char* CBOR             = "a28200581c00112233445566778899aabbccddeeff00112233445566778899aabbf68201581caabbccddeeff00112233445566778899aabbccddeeff001122334455d87980";
static const char* REVERSED_CBOR    = "a28201581caabbccddeeff00112233445566778899aabbccddeeff001122334455d879808200581c00112233445566778899aabbccddeeff00112233445566778899aabbf6";
static const char* NIL_ENTRY_CBOR   = "a18200581c00112233445566778899aabbccddeeff00112233445566778899aabbf6";
static const char* DATUM_ENTRY_CBOR = "a18201581caabbccddeeff00112233445566778899aabbccddeeff001122334455d87980";
static const char* EMPTY_MAP_CBOR   = "a0";
static const char* DUPLICATE_CBOR   = "a28200581c00112233445566778899aabbccddeeff00112233445566778899aabbf68200581c00112233445566778899aabbccddeeff00112233445566778899aabbf6";

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
 * Creates a new default instance of the plutus data.
 * @return A new instance of the plutus data.
 */
static cardano_plutus_data_t*
new_default_plutus_data(const char* cbor)
{
  cardano_plutus_data_t* plutus_data = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_plutus_data_from_cbor(reader, &plutus_data);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_plutus_data_unref(&plutus_data);
    return nullptr;
  }

  return plutus_data;
}

/**
 * Decodes the given CBOR hex string into a required guards map.
 * @return A new instance of the required guards map.
 */
static cardano_required_guards_map_t*
new_default_required_guards_map(const char* cbor)
{
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_required_guards_map_from_cbor(reader, &required_guards_map);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_required_guards_map_unref(&required_guards_map);
    return nullptr;
  }

  return required_guards_map;
}

/**
 * Encodes the given required guards map to a CBOR hex string.
 * @return The CBOR hex string. The caller must free the returned string.
 */
static char*
encode_required_guards_map(cardano_required_guards_map_t* required_guards_map)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_required_guards_map_to_cbor(required_guards_map, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size    = cardano_cbor_writer_get_hex_size(writer);
  char*        actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return actual_cbor;
}

/**
 * Encodes the given plutus data to a CBOR hex string.
 * @return The CBOR hex string. The caller must free the returned string.
 */
static char*
encode_plutus_data(cardano_plutus_data_t* plutus_data)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_data_to_cbor(plutus_data, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size    = cardano_cbor_writer_get_hex_size(writer);
  char*        actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return actual_cbor;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_required_guards_map_new, canCreateRequiredGuardsMap)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;

  // Act
  cardano_error_t error = cardano_required_guards_map_new(&required_guards_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(required_guards_map, testing::Not((cardano_required_guards_map_t*)nullptr));

  // Cleanup
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_new, returnsErrorIfRequiredGuardsMapIsNull)
{
  // Act
  cardano_error_t error = cardano_required_guards_map_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_required_guards_map_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_required_guards_map_t* required_guards_map = nullptr;

  // Act
  cardano_error_t error = cardano_required_guards_map_new(&required_guards_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(required_guards_map, (cardano_required_guards_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_required_guards_map_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_required_guards_map_t* required_guards_map = nullptr;

  // Act
  cardano_error_t error = cardano_required_guards_map_new(&required_guards_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(required_guards_map, (cardano_required_guards_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_required_guards_map_from_cbor, canDeserializeRequiredGuardsMap)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_required_guards_map_from_cbor(reader, &required_guards_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(required_guards_map, testing::Not((cardano_required_guards_map_t*)nullptr));
  EXPECT_EQ(cardano_required_guards_map_get_length(required_guards_map), 2);

  cardano_credential_t*  key1   = nullptr;
  cardano_credential_t*  key2   = nullptr;
  cardano_plutus_data_t* value1 = nullptr;
  cardano_plutus_data_t* value2 = nullptr;

  EXPECT_EQ(cardano_required_guards_map_get_key_value_at(required_guards_map, 0, &key1, &value1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_required_guards_map_get_key_value_at(required_guards_map, 1, &key2, &value2), CARDANO_SUCCESS);

  cardano_credential_type_t type = CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH;
  EXPECT_EQ(cardano_credential_get_type(key1, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  EXPECT_STREQ(cardano_credential_get_hash_hex(key1), KEY_HASH_HEX);
  EXPECT_EQ(value1, (cardano_plutus_data_t*)nullptr);

  EXPECT_EQ(cardano_credential_get_type(key2, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);
  EXPECT_STREQ(cardano_credential_get_hash_hex(key2), SCRIPT_HASH_HEX);
  EXPECT_THAT(value2, testing::Not((cardano_plutus_data_t*)nullptr));

  char* datum_cbor = encode_plutus_data(value2);
  EXPECT_STREQ(datum_cbor, PLUTUS_DATA_CBOR);

  // Cleanup
  free(datum_cbor);
  cardano_credential_unref(&key1);
  cardano_credential_unref(&key2);
  cardano_plutus_data_unref(&value2);
  cardano_required_guards_map_unref(&required_guards_map);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_required_guards_map_from_cbor, returnErrorIfEmptyMap)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex(EMPTY_MAP_CBOR, strlen(EMPTY_MAP_CBOR));

  // Act
  cardano_error_t error = cardano_required_guards_map_from_cbor(reader, &required_guards_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_MAP_SIZE);
  EXPECT_EQ(required_guards_map, (cardano_required_guards_map_t*)nullptr);
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'required_guards_map', the map must not be empty.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_required_guards_map_from_cbor, returnErrorIfDuplicatedKey)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex(DUPLICATE_CBOR, strlen(DUPLICATE_CBOR));

  // Act
  cardano_error_t error = cardano_required_guards_map_from_cbor(reader, &required_guards_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_EQ(required_guards_map, (cardano_required_guards_map_t*)nullptr);
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'required_guards_map', the map must not contain duplicated keys.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_required_guards_map_from_cbor, returnErrorIfNotAMap)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_required_guards_map_from_cbor(reader, &required_guards_map);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_required_guards_map_from_cbor, returnErrorIfInvalidKey)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex("a101f6", 6);

  // Act
  cardano_error_t error = cardano_required_guards_map_from_cbor(reader, &required_guards_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);
  EXPECT_EQ(required_guards_map, (cardano_required_guards_map_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_required_guards_map_from_cbor, returnErrorIfInvalidValue)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex("a18200581c00112233445566778899aabbccddeeff00112233445566778899aabbff", 68);

  // Act
  cardano_error_t error = cardano_required_guards_map_from_cbor(reader, &required_guards_map);

  // Assert
  EXPECT_THAT(error, testing::Not(CARDANO_SUCCESS));
  EXPECT_EQ(required_guards_map, (cardano_required_guards_map_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_required_guards_map_from_cbor, returnErrorIfRequiredGuardsMapIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_required_guards_map_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_required_guards_map_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;

  // Act
  cardano_error_t error = cardano_required_guards_map_from_cbor(nullptr, &required_guards_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_required_guards_map_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_required_guards_map_from_cbor(reader, &required_guards_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(required_guards_map, (cardano_required_guards_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_required_guards_map_to_cbor, canRoundTripDatumAndNilEntries)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = new_default_required_guards_map(CBOR);
  ASSERT_THAT(required_guards_map, testing::Not((cardano_required_guards_map_t*)nullptr));

  // Act
  char* actual_cbor = encode_required_guards_map(required_guards_map);

  // Assert
  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_required_guards_map_unref(&required_guards_map);
  free(actual_cbor);
}

TEST(cardano_required_guards_map_to_cbor, canRoundTripNilEntry)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = new_default_required_guards_map(NIL_ENTRY_CBOR);
  ASSERT_THAT(required_guards_map, testing::Not((cardano_required_guards_map_t*)nullptr));

  // Act
  char* actual_cbor = encode_required_guards_map(required_guards_map);

  // Assert
  EXPECT_STREQ(actual_cbor, NIL_ENTRY_CBOR);

  // Cleanup
  cardano_required_guards_map_unref(&required_guards_map);
  free(actual_cbor);
}

TEST(cardano_required_guards_map_to_cbor, canRoundTripDatumEntry)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = new_default_required_guards_map(DATUM_ENTRY_CBOR);
  ASSERT_THAT(required_guards_map, testing::Not((cardano_required_guards_map_t*)nullptr));

  // Act
  char* actual_cbor = encode_required_guards_map(required_guards_map);

  // Assert
  EXPECT_STREQ(actual_cbor, DATUM_ENTRY_CBOR);

  // Cleanup
  cardano_required_guards_map_unref(&required_guards_map);
  free(actual_cbor);
}

TEST(cardano_required_guards_map_to_cbor, preservesKeyInsertionOrder)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = new_default_required_guards_map(REVERSED_CBOR);
  ASSERT_THAT(required_guards_map, testing::Not((cardano_required_guards_map_t*)nullptr));

  cardano_credential_t* key = nullptr;
  EXPECT_EQ(cardano_required_guards_map_get_key_at(required_guards_map, 0, &key), CARDANO_SUCCESS);

  cardano_credential_type_t type = CARDANO_CREDENTIAL_TYPE_KEY_HASH;
  EXPECT_EQ(cardano_credential_get_type(key, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);

  // Act
  char* actual_cbor = encode_required_guards_map(required_guards_map);

  // Assert
  EXPECT_STREQ(actual_cbor, REVERSED_CBOR);

  // Cleanup
  cardano_credential_unref(&key);
  cardano_required_guards_map_unref(&required_guards_map);
  free(actual_cbor);
}

TEST(cardano_required_guards_map_to_cbor, canSerializeManuallyBuiltMap)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;

  cardano_error_t error = cardano_required_guards_map_new(&required_guards_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t*  key_hash_credential    = new_default_credential(KEY_HASH_CREDENTIAL_CBOR);
  cardano_credential_t*  script_hash_credential = new_default_credential(SCRIPT_HASH_CREDENTIAL_CBOR);
  cardano_plutus_data_t* datum                  = new_default_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_required_guards_map_insert(required_guards_map, key_hash_credential, nullptr), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_required_guards_map_insert(required_guards_map, script_hash_credential, datum), CARDANO_SUCCESS);

  // Act
  char* actual_cbor = encode_required_guards_map(required_guards_map);

  // Assert
  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_credential_unref(&key_hash_credential);
  cardano_credential_unref(&script_hash_credential);
  cardano_plutus_data_unref(&datum);
  cardano_required_guards_map_unref(&required_guards_map);
  free(actual_cbor);
}

TEST(cardano_required_guards_map_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_required_guards_map_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_required_guards_map_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;

  cardano_error_t error = cardano_required_guards_map_new(&required_guards_map);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_required_guards_map_to_cbor(required_guards_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_get_length, returnsZeroIfRequiredGuardsMapIsNull)
{
  // Act
  size_t length = cardano_required_guards_map_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_required_guards_map_get_length, returnsZeroIfRequiredGuardsMapIsEmpty)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_error_t                error               = cardano_required_guards_map_new(&required_guards_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_required_guards_map_get_length(required_guards_map);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_get, returnsErrorIfRequiredGuardsMapIsNull)
{
  // Arrange
  cardano_credential_t*  key   = new_default_credential(KEY_HASH_CREDENTIAL_CBOR);
  cardano_plutus_data_t* datum = nullptr;

  // Act
  cardano_error_t error = cardano_required_guards_map_get(nullptr, key, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&key);
}

TEST(cardano_required_guards_map_get, returnsErrorIfKeyIsNull)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_error_t                error               = cardano_required_guards_map_new(&required_guards_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* datum = nullptr;

  // Act
  error = cardano_required_guards_map_get(required_guards_map, nullptr, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_get, returnsErrorIfElementIsNull)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_error_t                error               = cardano_required_guards_map_new(&required_guards_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* key = new_default_credential(KEY_HASH_CREDENTIAL_CBOR);

  // Act
  error = cardano_required_guards_map_get(required_guards_map, key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&key);
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_get, returnsErrorIfElementNotFound)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = new_default_required_guards_map(NIL_ENTRY_CBOR);
  ASSERT_THAT(required_guards_map, testing::Not((cardano_required_guards_map_t*)nullptr));

  cardano_credential_t*  key   = new_default_credential(SCRIPT_HASH_CREDENTIAL_CBOR);
  cardano_plutus_data_t* datum = nullptr;

  // Act
  cardano_error_t error = cardano_required_guards_map_get(required_guards_map, key, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_credential_unref(&key);
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_get, returnsNullDatumForNilEntry)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = new_default_required_guards_map(CBOR);
  ASSERT_THAT(required_guards_map, testing::Not((cardano_required_guards_map_t*)nullptr));

  cardano_credential_t*  key   = new_default_credential(KEY_HASH_CREDENTIAL_CBOR);
  cardano_plutus_data_t* datum = nullptr;

  // Act
  cardano_error_t error = cardano_required_guards_map_get(required_guards_map, key, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(datum, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_credential_unref(&key);
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_get, returnsDatumForDatumEntry)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = new_default_required_guards_map(CBOR);
  ASSERT_THAT(required_guards_map, testing::Not((cardano_required_guards_map_t*)nullptr));

  cardano_credential_t*  key   = new_default_credential(SCRIPT_HASH_CREDENTIAL_CBOR);
  cardano_plutus_data_t* datum = nullptr;

  // Act
  cardano_error_t error = cardano_required_guards_map_get(required_guards_map, key, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(datum, testing::Not((cardano_plutus_data_t*)nullptr));

  char* datum_cbor = encode_plutus_data(datum);
  EXPECT_STREQ(datum_cbor, PLUTUS_DATA_CBOR);

  // Cleanup
  free(datum_cbor);
  cardano_credential_unref(&key);
  cardano_plutus_data_unref(&datum);
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_insert, returnsErrorIfRequiredGuardsMapIsNull)
{
  // Arrange
  cardano_credential_t* key = new_default_credential(KEY_HASH_CREDENTIAL_CBOR);

  // Act
  cardano_error_t error = cardano_required_guards_map_insert(nullptr, key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&key);
}

TEST(cardano_required_guards_map_insert, returnsErrorIfKeyIsNull)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_error_t                error               = cardano_required_guards_map_new(&required_guards_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_required_guards_map_insert(required_guards_map, nullptr, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_insert, returnsErrorIfKeyIsDuplicated)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_error_t                error               = cardano_required_guards_map_new(&required_guards_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* key = new_default_credential(KEY_HASH_CREDENTIAL_CBOR);

  // Act
  EXPECT_EQ(cardano_required_guards_map_insert(required_guards_map, key, nullptr), CARDANO_SUCCESS);
  error = cardano_required_guards_map_insert(required_guards_map, key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_EQ(cardano_required_guards_map_get_length(required_guards_map), 1);

  // Cleanup
  cardano_credential_unref(&key);
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_insert, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_error_t                error               = cardano_required_guards_map_new(&required_guards_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* key = new_default_credential(KEY_HASH_CREDENTIAL_CBOR);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_required_guards_map_insert(required_guards_map, key, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_credential_unref(&key);
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_get_key_at, returnsErrorIfRequiredGuardsMapIsNull)
{
  // Arrange
  cardano_credential_t* credential = nullptr;

  // Act
  cardano_error_t error = cardano_required_guards_map_get_key_at(nullptr, 0, &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_required_guards_map_get_key_at, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_error_t                error               = cardano_required_guards_map_new(&required_guards_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_required_guards_map_get_key_at(required_guards_map, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_get_key_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_error_t                error               = cardano_required_guards_map_new(&required_guards_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential = nullptr;

  // Act
  error = cardano_required_guards_map_get_key_at(required_guards_map, 0, &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_get_key_at, canReturnTheRightKey)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = new_default_required_guards_map(CBOR);
  ASSERT_THAT(required_guards_map, testing::Not((cardano_required_guards_map_t*)nullptr));

  // Act
  cardano_credential_t* credential = nullptr;
  cardano_error_t       error      = cardano_required_guards_map_get_key_at(required_guards_map, 1, &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_credential_get_hash_hex(credential), SCRIPT_HASH_HEX);

  // Cleanup
  cardano_credential_unref(&credential);
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_get_value_at, returnsErrorIfRequiredGuardsMapIsNull)
{
  // Arrange
  cardano_plutus_data_t* datum = nullptr;

  // Act
  cardano_error_t error = cardano_required_guards_map_get_value_at(nullptr, 0, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_required_guards_map_get_value_at, returnsErrorIfDatumIsNull)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_error_t                error               = cardano_required_guards_map_new(&required_guards_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_required_guards_map_get_value_at(required_guards_map, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_get_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_error_t                error               = cardano_required_guards_map_new(&required_guards_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* datum = nullptr;

  // Act
  error = cardano_required_guards_map_get_value_at(required_guards_map, 0, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_get_value_at, canReturnTheRightValue)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = new_default_required_guards_map(CBOR);
  ASSERT_THAT(required_guards_map, testing::Not((cardano_required_guards_map_t*)nullptr));

  // Act
  cardano_plutus_data_t* nil_datum = nullptr;
  cardano_plutus_data_t* datum     = nullptr;

  EXPECT_EQ(cardano_required_guards_map_get_value_at(required_guards_map, 0, &nil_datum), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_required_guards_map_get_value_at(required_guards_map, 1, &datum), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(nil_datum, (cardano_plutus_data_t*)nullptr);
  EXPECT_THAT(datum, testing::Not((cardano_plutus_data_t*)nullptr));

  char* datum_cbor = encode_plutus_data(datum);
  EXPECT_STREQ(datum_cbor, PLUTUS_DATA_CBOR);

  // Cleanup
  free(datum_cbor);
  cardano_plutus_data_unref(&datum);
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_get_key_value_at, returnsErrorIfRequiredGuardsMapIsNull)
{
  // Arrange
  cardano_credential_t*  credential = nullptr;
  cardano_plutus_data_t* datum      = nullptr;

  // Act
  cardano_error_t error = cardano_required_guards_map_get_key_value_at(nullptr, 0, &credential, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_required_guards_map_get_key_value_at, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_error_t                error               = cardano_required_guards_map_new(&required_guards_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_plutus_data_t* datum = nullptr;

  // Act
  error = cardano_required_guards_map_get_key_value_at(required_guards_map, 0, nullptr, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_get_key_value_at, returnsErrorIfDatumIsNull)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_error_t                error               = cardano_required_guards_map_new(&required_guards_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential = nullptr;

  // Act
  error = cardano_required_guards_map_get_key_value_at(required_guards_map, 0, &credential, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_get_key_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_error_t                error               = cardano_required_guards_map_new(&required_guards_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t*  credential = nullptr;
  cardano_plutus_data_t* datum      = nullptr;

  // Act
  error = cardano_required_guards_map_get_key_value_at(required_guards_map, 0, &credential, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_get_key_value_at, canReturnTheRightKeyValuePair)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = new_default_required_guards_map(CBOR);
  ASSERT_THAT(required_guards_map, testing::Not((cardano_required_guards_map_t*)nullptr));

  // Act
  cardano_credential_t*  credential = nullptr;
  cardano_plutus_data_t* datum      = nullptr;

  cardano_error_t error = cardano_required_guards_map_get_key_value_at(required_guards_map, 0, &credential, &datum);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_credential_get_hash_hex(credential), KEY_HASH_HEX);
  EXPECT_EQ(datum, (cardano_plutus_data_t*)nullptr);

  // Cleanup
  cardano_credential_unref(&credential);
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_error_t                error               = cardano_required_guards_map_new(&required_guards_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_required_guards_map_ref(required_guards_map);

  // Assert
  EXPECT_THAT(required_guards_map, testing::Not((cardano_required_guards_map_t*)nullptr));
  EXPECT_EQ(cardano_required_guards_map_refcount(required_guards_map), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_required_guards_map_unref(&required_guards_map);
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_required_guards_map_ref(nullptr);
}

TEST(cardano_required_guards_map_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;

  // Act
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_required_guards_map_unref((cardano_required_guards_map_t**)nullptr);
}

TEST(cardano_required_guards_map_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_error_t                error               = cardano_required_guards_map_new(&required_guards_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_required_guards_map_ref(required_guards_map);
  size_t ref_count = cardano_required_guards_map_refcount(required_guards_map);

  cardano_required_guards_map_unref(&required_guards_map);
  size_t updated_ref_count = cardano_required_guards_map_refcount(required_guards_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_error_t                error               = cardano_required_guards_map_new(&required_guards_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_required_guards_map_ref(required_guards_map);
  size_t ref_count = cardano_required_guards_map_refcount(required_guards_map);

  cardano_required_guards_map_unref(&required_guards_map);
  size_t updated_ref_count = cardano_required_guards_map_refcount(required_guards_map);

  cardano_required_guards_map_unref(&required_guards_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(required_guards_map, (cardano_required_guards_map_t*)nullptr);

  // Cleanup
  cardano_required_guards_map_unref(&required_guards_map);
}

TEST(cardano_required_guards_map_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_required_guards_map_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_required_guards_map_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  const char*                    message             = "This is a test message";

  // Act
  cardano_required_guards_map_set_last_error(required_guards_map, message);

  // Assert
  EXPECT_STREQ(cardano_required_guards_map_get_last_error(required_guards_map), "Object is NULL.");
}

TEST(cardano_required_guards_map_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_required_guards_map_t* required_guards_map = nullptr;
  cardano_error_t                error               = cardano_required_guards_map_new(&required_guards_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_required_guards_map_set_last_error(required_guards_map, message);

  // Assert
  EXPECT_STREQ(cardano_required_guards_map_get_last_error(required_guards_map), "");

  // Cleanup
  cardano_required_guards_map_unref(&required_guards_map);
}
