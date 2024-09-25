/**
 * \file redeemer.cpp
 *
 * \author angel.castillo
 * \date   Sep 21, 2024
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

#include <cardano/cbor/cbor_reader.h>
#include <cardano/common/ex_units.h>
#include <cardano/witness_set/redeemer.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR             = "840000d8799f0102030405ff821821182c";
static const char* PLUTUS_DATA_CBOR = "d8799f0102030405ff";
static const char* EX_UNITS         = "821821182C";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the redeemer.
 * @return A new instance of the redeemer.
 */
static cardano_redeemer_t*
new_default_redeemer()
{
  cardano_redeemer_t*    redeemer = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t        result   = cardano_redeemer_from_cbor(reader, &redeemer);

  cardano_redeemer_clear_cbor_cache(redeemer);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return redeemer;
};

/**
 * Creates a new default instance of the plutus_data.
 *
 * @return A new instance of the plutus_data.
 */
static cardano_plutus_data_t*
new_default_plutus_data()
{
  cardano_plutus_data_t* plutus_data = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(PLUTUS_DATA_CBOR, strlen(PLUTUS_DATA_CBOR));
  cardano_error_t        result      = cardano_plutus_data_from_cbor(reader, &plutus_data);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return plutus_data;
}

/**
 * Creates a new default instance of the ex_units.
 * @return A new instance of the ex_units.
 */
static cardano_ex_units_t*
new_default_ex_units()
{
  cardano_ex_units_t*    ex_units = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(EX_UNITS, strlen(EX_UNITS));
  cardano_error_t        result   = cardano_ex_units_from_cbor(reader, &ex_units);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return ex_units;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_redeemer_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_redeemer_t* redeemer = new_default_redeemer();
  EXPECT_NE(redeemer, nullptr);

  // Act
  cardano_redeemer_ref(redeemer);

  // Assert
  EXPECT_THAT(redeemer, testing::Not((cardano_redeemer_t*)nullptr));
  EXPECT_EQ(cardano_redeemer_refcount(redeemer), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_redeemer_unref(&redeemer);
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_redeemer_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_redeemer_ref(nullptr);
}

TEST(cardano_redeemer_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_redeemer_t* redeemer = nullptr;

  // Act
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_redeemer_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_redeemer_unref((cardano_redeemer_t**)nullptr);
}

TEST(cardano_redeemer_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_redeemer_t* redeemer = new_default_redeemer();
  EXPECT_NE(redeemer, nullptr);

  // Act
  cardano_redeemer_ref(redeemer);
  size_t ref_count = cardano_redeemer_refcount(redeemer);

  cardano_redeemer_unref(&redeemer);
  size_t updated_ref_count = cardano_redeemer_refcount(redeemer);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_redeemer_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_redeemer_t* redeemer = new_default_redeemer();
  EXPECT_NE(redeemer, nullptr);

  // Act
  cardano_redeemer_ref(redeemer);
  size_t ref_count = cardano_redeemer_refcount(redeemer);

  cardano_redeemer_unref(&redeemer);
  size_t updated_ref_count = cardano_redeemer_refcount(redeemer);

  cardano_redeemer_unref(&redeemer);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(redeemer, (cardano_redeemer_t*)nullptr);

  // Cleanup
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_redeemer_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_redeemer_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_redeemer_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_redeemer_t* redeemer = nullptr;
  const char*         message  = "This is a test message";

  // Act
  cardano_redeemer_set_last_error(redeemer, message);

  // Assert
  EXPECT_STREQ(cardano_redeemer_get_last_error(redeemer), "Object is NULL.");
}

TEST(cardano_redeemer_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_redeemer_t* redeemer = new_default_redeemer();
  EXPECT_NE(redeemer, nullptr);

  const char* message = nullptr;

  // Act
  cardano_redeemer_set_last_error(redeemer, message);

  // Assert
  EXPECT_STREQ(cardano_redeemer_get_last_error(redeemer), "");

  // Cleanup
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_redeemer_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_redeemer_t* redeemer = NULL;

  // Act
  cardano_error_t result = cardano_redeemer_from_cbor(nullptr, &redeemer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_redeemer_from_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_redeemer_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_redeemer_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t* writer   = cardano_cbor_writer_new();
  cardano_redeemer_t*    redeemer = new_default_redeemer();
  EXPECT_NE(redeemer, nullptr);

  // Act
  cardano_error_t result = cardano_redeemer_to_cbor(redeemer, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_redeemer_unref(&redeemer);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_redeemer_to_cbor, canSerializeFromCache)
{
  // Arrange
  cardano_cbor_writer_t* writer   = cardano_cbor_writer_new();
  cardano_redeemer_t*    redeemer = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t        result   = cardano_redeemer_from_cbor(reader, &redeemer);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  // Act
  result = cardano_redeemer_to_cbor(redeemer, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_redeemer_unref(&redeemer);
  cardano_cbor_writer_unref(&writer);
  cardano_cbor_reader_unref(&reader);
  free(hex);
}

TEST(cardano_redeemer_to_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_redeemer_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_redeemer_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_redeemer_to_cbor((cardano_redeemer_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

// Cert specific tests

TEST(cardano_redeemer_new, canCreateNewInstance)
{
  // Act
  cardano_plutus_data_t* plutus_data = new_default_plutus_data();
  cardano_ex_units_t*    ex_units    = new_default_ex_units();

  cardano_redeemer_t* redeemer = NULL;

  cardano_error_t result = cardano_redeemer_new(CARDANO_REDEEMER_TAG_SPEND, 0, plutus_data, ex_units, &redeemer);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(redeemer, nullptr);

  // Cleanup
  cardano_redeemer_unref(&redeemer);
  cardano_plutus_data_unref(&plutus_data);
  cardano_ex_units_unref(&ex_units);
}

TEST(cardano_redeemer_new, returnsErrorIfFirstArgIsNull)
{
  // Act
  cardano_redeemer_t* redeemer = NULL;

  cardano_error_t result = cardano_redeemer_new(CARDANO_REDEEMER_TAG_SPEND, 0, nullptr, nullptr, &redeemer);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_redeemer_new, returnsErrorIfSecondArgIsNull)
{
  // Act
  cardano_redeemer_t* redeemer = NULL;

  cardano_error_t result = cardano_redeemer_new(CARDANO_REDEEMER_TAG_SPEND, 0, (cardano_plutus_data_t*)"", nullptr, &redeemer);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_redeemer_new, returnsErrorIfCertIsNull)
{
  // Act

  cardano_error_t result = cardano_redeemer_new(CARDANO_REDEEMER_TAG_SPEND, 0, (cardano_plutus_data_t*)"", (cardano_ex_units_t*)"", nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
}

TEST(cardano_redeemer_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = new_default_plutus_data();
  cardano_ex_units_t*    ex_units    = new_default_ex_units();

  // Act
  cardano_redeemer_t* redeemer = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_redeemer_new(CARDANO_REDEEMER_TAG_SPEND, 0, plutus_data, ex_units, &redeemer);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
  cardano_ex_units_unref(&ex_units);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_redeemer_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_redeemer_t*    redeemer = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_redeemer_from_cbor(reader, &redeemer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_redeemer_unref(&redeemer);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_redeemer_from_cbor, returnsErrorIfInvalidCbor)
{
  // Arrange
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex("84ef", strlen("84ef"));
  cardano_redeemer_t*    redeemer = NULL;

  // Act
  cardano_error_t result = cardano_redeemer_from_cbor(reader, &redeemer);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_redeemer_from_cbor, returnsErrorIfInvalidTag)
{
  // Arrange
  const char*            cbor     = "84ef00d8799f0102030405ff821821182c";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_redeemer_t*    redeemer = NULL;

  // Act
  cardano_error_t result = cardano_redeemer_from_cbor(reader, &redeemer);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_redeemer_from_cbor, returnsErrorIfInvalidIndex)
{
  // Arrange
  const char*            cbor     = "8400efd8799f0102030405ff821821182c";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_redeemer_t*    redeemer = NULL;

  // Act
  cardano_error_t result = cardano_redeemer_from_cbor(reader, &redeemer);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_redeemer_from_cbor, returnsErrorIfInvalidPlutusData)
{
  // Arrange
  const char*            cbor     = "840000ef821821182c";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_redeemer_t*    redeemer = NULL;

  // Act
  cardano_error_t result = cardano_redeemer_from_cbor(reader, &redeemer);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_redeemer_from_cbor, returnsErrorIfInvalidExUnits)
{
  // Arrange
  const char*            cbor     = "84000000ef";
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_redeemer_t*    redeemer = NULL;

  // Act
  cardano_error_t result = cardano_redeemer_from_cbor(reader, &redeemer);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

// Getters and Setters

TEST(cardano_redeemer_set_data, canSetPlutusData)
{
  // Arrange
  cardano_redeemer_t*    redeemer    = new_default_redeemer();
  cardano_plutus_data_t* plutus_data = new_default_plutus_data();

  // Act
  cardano_error_t result = cardano_redeemer_set_data(redeemer, plutus_data);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_redeemer_unref(&redeemer);
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_redeemer_set_data, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = new_default_plutus_data();

  // Act
  cardano_error_t result = cardano_redeemer_set_data(nullptr, plutus_data);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_data_unref(&plutus_data);
}

TEST(cardano_redeemer_set_data, returnsErrorIfPlutusDataIsNull)
{
  // Arrange
  cardano_redeemer_t* redeemer = new_default_redeemer();

  // Act
  cardano_error_t result = cardano_redeemer_set_data(redeemer, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_redeemer_get_data, canGetPlutusData)
{
  // Arrange
  cardano_redeemer_t*    redeemer    = new_default_redeemer();
  cardano_plutus_data_t* plutus_data = new_default_plutus_data();

  EXPECT_EQ(cardano_redeemer_set_data(redeemer, plutus_data), CARDANO_SUCCESS);

  // Act
  cardano_plutus_data_t* plutus_data2 = cardano_redeemer_get_data(redeemer);

  // Assert
  EXPECT_NE(plutus_data2, nullptr);

  // Cleanup
  cardano_redeemer_unref(&redeemer);
  cardano_plutus_data_unref(&plutus_data);
  cardano_plutus_data_unref(&plutus_data2);
}

TEST(cardano_redeemer_get_data, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_plutus_data_t* plutus_data = cardano_redeemer_get_data(nullptr);

  // Assert
  EXPECT_EQ(plutus_data, nullptr);
}

TEST(cardano_redeemer_get_ex_units, canGetExUnits)
{
  // Arrange
  cardano_redeemer_t* redeemer = new_default_redeemer();
  cardano_ex_units_t* ex_units = new_default_ex_units();

  EXPECT_EQ(cardano_redeemer_set_ex_units(redeemer, ex_units), CARDANO_SUCCESS);

  // Act
  cardano_ex_units_t* ex_units2 = cardano_redeemer_get_ex_units(redeemer);

  // Assert
  EXPECT_NE(ex_units2, nullptr);

  // Cleanup
  cardano_redeemer_unref(&redeemer);
  cardano_ex_units_unref(&ex_units);
  cardano_ex_units_unref(&ex_units2);
}

TEST(cardano_redeemer_get_ex_units, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_ex_units_t* ex_units = cardano_redeemer_get_ex_units(nullptr);

  // Assert
  EXPECT_EQ(ex_units, nullptr);
}

TEST(cardano_redeemer_set_ex_units, canSetExUnits)
{
  // Arrange
  cardano_redeemer_t* redeemer = new_default_redeemer();
  cardano_ex_units_t* ex_units = new_default_ex_units();

  // Act
  cardano_error_t result = cardano_redeemer_set_ex_units(redeemer, ex_units);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_redeemer_unref(&redeemer);
  cardano_ex_units_unref(&ex_units);
}

TEST(cardano_redeemer_set_ex_units, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_ex_units_t* ex_units = new_default_ex_units();

  // Act
  cardano_error_t result = cardano_redeemer_set_ex_units(nullptr, ex_units);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_ex_units_unref(&ex_units);
}

TEST(cardano_redeemer_set_ex_units, returnsErrorIfExUnitsIsNull)
{
  // Arrange
  cardano_redeemer_t* redeemer = new_default_redeemer();

  // Act
  cardano_error_t result = cardano_redeemer_set_ex_units(redeemer, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_redeemer_from_cbor, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_redeemer_t*    redeemer = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_redeemer_from_cbor(reader, &redeemer);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_redeemer_get_tag, canGetTag)
{
  // Arrange
  cardano_redeemer_t* redeemer = new_default_redeemer();

  // Act
  cardano_redeemer_tag_t tag = cardano_redeemer_get_tag(redeemer);

  // Assert
  EXPECT_EQ(tag, CARDANO_REDEEMER_TAG_SPEND);

  // Cleanup
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_redeemer_get_tag, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_redeemer_tag_t tag = cardano_redeemer_get_tag(nullptr);

  // Assert
  EXPECT_EQ(tag, CARDANO_REDEEMER_TAG_SPEND);
}

TEST(cardano_redeemer_set_tag, canSetTag)
{
  // Arrange
  cardano_redeemer_t* redeemer = new_default_redeemer();

  // Act
  cardano_error_t result = cardano_redeemer_set_tag(redeemer, CARDANO_REDEEMER_TAG_MINT);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_redeemer_set_tag, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_error_t result = cardano_redeemer_set_tag(nullptr, CARDANO_REDEEMER_TAG_MINT);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_redeemer_get_index, canGetIndex)
{
  // Arrange
  cardano_redeemer_t* redeemer = new_default_redeemer();

  // Act
  uint32_t index = cardano_redeemer_get_index(redeemer);

  // Assert
  EXPECT_EQ(index, 0);

  // Cleanup
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_redeemer_get_index, returnsErrorIfObjectIsNull)
{
  // Act
  uint32_t index = cardano_redeemer_get_index(nullptr);

  // Assert
  EXPECT_EQ(index, 0);
}

TEST(cardano_redeemer_set_index, canSetIndex)
{
  // Arrange
  cardano_redeemer_t* redeemer = new_default_redeemer();

  // Act
  cardano_error_t result = cardano_redeemer_set_index(redeemer, 1);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_redeemer_set_index, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_error_t result = cardano_redeemer_set_index(nullptr, 1);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_redeemer_clear_cbor_cache, doesNothingIfObjectIsNull)
{
  // Act
  cardano_redeemer_clear_cbor_cache(nullptr);
}
