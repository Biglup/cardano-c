/**
 * \file cost_model.cpp
 *
 * \author angel.castillo
 * \date   Jun 09, 2024
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

#include <cardano/buffer.h>
#include <cardano/protocol_params/cost_model.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* COST_MODEL_V1_HEX     = "98a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a";
static const char* COST_MODEL_V2_HEX     = "98af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a";
static const char* COST_MODEL_V3_HEX     = "98b31a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a01020304";
static const char* COST_MODE_V1_CBOR_HEX = "0098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a";
static const char* COST_MODE_V2_CBOR_HEX = "0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a";
static const char* COST_MODE_V3_CBOR_HEX = "0298b31a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a01020304";

static std::vector<int64_t>
hex_string_to_costs(const char* hex_string)
{
  std::vector<int64_t> costs;

  if (hex_string == NULL)
  {
    return costs;
  }

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(hex_string, strlen(hex_string));

  if (reader == NULL)
  {
    return costs;
  }

  int64_t array_size = 0;

  EXPECT_EQ(cardano_cbor_reader_read_start_array(reader, &array_size), CARDANO_SUCCESS);

  for (int64_t i = 0; i < array_size; ++i)
  {
    int64_t value = 0;

    EXPECT_EQ(cardano_cbor_reader_read_int(reader, &value), CARDANO_SUCCESS);
    costs.push_back(value);
  }

  cardano_cbor_reader_unref(&reader);

  return costs;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_cost_model_new, canCreateCostModelV1)
{
  // Arrange
  cardano_cost_model_t* cost_model = nullptr;
  std::vector<int64_t>  costs      = hex_string_to_costs(COST_MODEL_V1_HEX);

  // Act
  cardano_error_t error = cardano_cost_model_new(CARDANO_PLUTUS_LANGUAGE_VERSION_V1, &costs[0], costs.size(), &cost_model);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(cost_model, testing::Not((cardano_cost_model_t*)nullptr));

  // Cleanup
  cardano_cost_model_unref(&cost_model);
}

TEST(cardano_cost_model_new, canCreateCostModelV2)
{
  // Arrange
  cardano_cost_model_t* cost_model = nullptr;
  std::vector<int64_t>  costs      = hex_string_to_costs(COST_MODEL_V2_HEX);

  // Act
  cardano_error_t error = cardano_cost_model_new(CARDANO_PLUTUS_LANGUAGE_VERSION_V2, &costs[0], costs.size(), &cost_model);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(cost_model, testing::Not((cardano_cost_model_t*)nullptr));

  // Cleanup
  cardano_cost_model_unref(&cost_model);
}

TEST(cardano_cost_model_new, canCreateCostModelV3)
{
  // Arrange
  cardano_cost_model_t* cost_model = nullptr;
  std::vector<int64_t>  costs      = hex_string_to_costs(COST_MODEL_V3_HEX);

  // Act
  cardano_error_t error = cardano_cost_model_new(CARDANO_PLUTUS_LANGUAGE_VERSION_V3, &costs[0], costs.size(), &cost_model);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(cost_model, testing::Not((cardano_cost_model_t*)nullptr));

  // Cleanup
  cardano_cost_model_unref(&cost_model);
}

TEST(cardano_cost_model_new, returnErrorIfInvalidCostModel)
{
  // Arrange
  cardano_cost_model_t* cost_model = nullptr;
  std::vector<int64_t>  costs      = hex_string_to_costs(COST_MODEL_V3_HEX);

  // Act
  cardano_error_t error = cardano_cost_model_new(CARDANO_PLUTUS_LANGUAGE_VERSION_V1, &costs[0], 4096, &cost_model);
  EXPECT_EQ(error, CARDANO_INVALID_PLUTUS_COST_MODEL);

  error = cardano_cost_model_new(CARDANO_PLUTUS_LANGUAGE_VERSION_V2, &costs[0], 4096, &cost_model);
  EXPECT_EQ(error, CARDANO_INVALID_PLUTUS_COST_MODEL);

  error = cardano_cost_model_new(CARDANO_PLUTUS_LANGUAGE_VERSION_V3, &costs[0], 4096, &cost_model);
  EXPECT_EQ(error, CARDANO_INVALID_PLUTUS_COST_MODEL);

  error = cardano_cost_model_new((cardano_plutus_language_version_t)99, &costs[0], 4096, &cost_model);
  EXPECT_EQ(error, CARDANO_INVALID_PLUTUS_COST_MODEL);
}

TEST(cardano_cost_model_new, returnsErrorIfCostModelIsNull)
{
  // Arrange
  std::vector<int64_t> costs = hex_string_to_costs(COST_MODEL_V1_HEX);

  // Act
  cardano_error_t error = cardano_cost_model_new(CARDANO_PLUTUS_LANGUAGE_VERSION_V1, &costs[0], costs.size(), nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cost_model_new, returnsErrorIfAllocationFails)
{
  // Arrange
  cardano_cost_model_t* cost_model = nullptr;
  std::vector<int64_t>  costs      = hex_string_to_costs(COST_MODEL_V1_HEX);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_cost_model_new(CARDANO_PLUTUS_LANGUAGE_VERSION_V1, &costs[0], costs.size(), &cost_model);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
}

TEST(cardano_cost_model_to_cbor, canSerializeCostModel)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_writer_t* writer     = cardano_cbor_writer_new();
  std::vector<int64_t>   costs      = hex_string_to_costs(COST_MODEL_V1_HEX);

  cardano_error_t error = cardano_cost_model_new(CARDANO_PLUTUS_LANGUAGE_VERSION_V1, &costs[0], costs.size(), &cost_model);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_cost_model_to_cbor(cost_model, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  cbor_hex = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor_hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, COST_MODE_V1_CBOR_HEX);

  // Cleanup
  cardano_cost_model_unref(&cost_model);
  cardano_cbor_writer_unref(&writer);
  free(cbor_hex);
}

TEST(cardano_cost_model_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_cost_model_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_cost_model_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_cost_model_t* cost_model = nullptr;
  std::vector<int64_t>  costs      = hex_string_to_costs(COST_MODEL_V1_HEX);

  cardano_error_t error = cardano_cost_model_new(CARDANO_PLUTUS_LANGUAGE_VERSION_V1, &costs[0], costs.size(), &cost_model);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_cost_model_to_cbor(cost_model, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cost_model_unref(&cost_model);
}

TEST(cardano_cost_model_from_cbor, canDeserializeCostModelV1)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  // Act
  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(cost_model, testing::Not((cardano_cost_model_t*)nullptr));

  // Cleanup
  cardano_cost_model_unref(&cost_model);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_from_cbor, canDeserializeCostModelV2)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(COST_MODE_V2_CBOR_HEX, strlen(COST_MODE_V2_CBOR_HEX));

  // Act
  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(cost_model, testing::Not((cardano_cost_model_t*)nullptr));

  // Cleanup
  cardano_cost_model_unref(&cost_model);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_from_cbor, canDeserializeCostModelV3)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(COST_MODE_V3_CBOR_HEX, strlen(COST_MODE_V3_CBOR_HEX));

  // Act
  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(cost_model, testing::Not((cardano_cost_model_t*)nullptr));

  // Cleanup
  cardano_cost_model_unref(&cost_model);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_from_cbor, returnsErrorIdInvalidPlutusType)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex("04", 2);

  // Act
  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_VALUE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_from_cbor, returnsErrorIdInvalidCostsArray)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex("01fe", 4);

  // Act
  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_from_cbor, returnsErrorIdInvalidCostsInsideArray)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex("0198af", 6);

  // Act
  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_from_cbor, returnErrorIfCostModelIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  // Act
  cardano_error_t error = cardano_cost_model_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_cost_model_t* cost_model = nullptr;

  // Act
  cardano_error_t error = cardano_cost_model_from_cbor(nullptr, &cost_model);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cost_model_from_cbor, returnErrorIfCborDataStartWithAnInvalidArray)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex("81", 2);

  // Act
  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'cost_model', expected 'Reader State: Unsigned Integer' (1) but got 'Reader State: Start Array' (9).");
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_cost_model_ref(cost_model);

  // Assert
  EXPECT_THAT(cost_model, testing::Not((cardano_cost_model_t*)nullptr));
  EXPECT_EQ(cardano_cost_model_refcount(cost_model), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_cost_model_unref(&cost_model);
  cardano_cost_model_unref(&cost_model);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_cost_model_ref(nullptr);
}

TEST(cardano_cost_model_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_cost_model_t* cost_model = nullptr;

  // Act
  cardano_cost_model_unref(&cost_model);
}

TEST(cardano_cost_model_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_cost_model_unref((cardano_cost_model_t**)nullptr);
}

TEST(cardano_cost_model_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_cost_model_ref(cost_model);
  size_t ref_count = cardano_cost_model_refcount(cost_model);

  cardano_cost_model_unref(&cost_model);
  size_t updated_ref_count = cardano_cost_model_refcount(cost_model);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_cost_model_unref(&cost_model);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_cost_model_ref(cost_model);
  size_t ref_count = cardano_cost_model_refcount(cost_model);

  cardano_cost_model_unref(&cost_model);
  size_t updated_ref_count = cardano_cost_model_refcount(cost_model);

  cardano_cost_model_unref(&cost_model);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(cost_model, (cardano_cost_model_t*)nullptr);

  // Cleanup
  cardano_cost_model_unref(&cost_model);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_cost_model_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_cost_model_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_cost_model_t* cost_model = nullptr;
  const char*           message    = "This is a test message";

  // Act
  cardano_cost_model_set_last_error(cost_model, message);

  // Assert
  EXPECT_STREQ(cardano_cost_model_get_last_error(cost_model), "Object is NULL.");
}

TEST(cardano_cost_model_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_cost_model_set_last_error(cost_model, message);

  // Assert
  EXPECT_STREQ(cardano_cost_model_get_last_error(cost_model), "");

  // Cleanup
  cardano_cost_model_unref(&cost_model);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_set_cost, canSetCost)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_cost_model_set_cost(cost_model, 0, 100);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  int64_t c = 0;
  ASSERT_EQ(cardano_cost_model_get_cost(cost_model, 0, &c), CARDANO_SUCCESS);

  EXPECT_EQ(c, 100);

  // Cleanup
  cardano_cost_model_unref(&cost_model);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_set_cost, returnErrroIfGivenNull)
{
  // Arrange
  cardano_cost_model_t* cost_model = nullptr;

  // Act
  cardano_error_t error = cardano_cost_model_set_cost(cost_model, 0, 100);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cost_model_set_cost, returnErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_cost_model_set_cost(cost_model, 99999, 100);

  // Assert
  EXPECT_EQ(error, CARDANO_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_cost_model_unref(&cost_model);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_get_cost, canGetCost)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  int64_t c = 0;
  error     = cardano_cost_model_get_cost(cost_model, 0, &c);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(c, 205665);

  // Cleanup
  cardano_cost_model_unref(&cost_model);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_get_cost, returnErrorIfGivenNull)
{
  // Arrange
  cardano_cost_model_t* cost_model = nullptr;

  // Act
  int64_t         c     = 0;
  cardano_error_t error = cardano_cost_model_get_cost(cost_model, 0, &c);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cost_model_get_cost, returnErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  int64_t c = 0;
  error     = cardano_cost_model_get_cost(cost_model, 99999, &c);

  // Assert
  EXPECT_EQ(error, CARDANO_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_cost_model_unref(&cost_model);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_get_cost, returnErrorIfCostIsNull)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t get_error = cardano_cost_model_get_cost(cost_model, 0, nullptr);

  // Assert
  EXPECT_EQ(get_error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cost_model_unref(&cost_model);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_get_costs_size, canGetCostsSize)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t size = cardano_cost_model_get_costs_size(cost_model);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(size, 166);

  // Cleanup
  cardano_cost_model_unref(&cost_model);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_get_costs_size, returnZeroIfGivenNull)
{
  // Act
  size_t size = cardano_cost_model_get_costs_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_cost_model_get_costs, canGetCosts)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  const int64_t* costs = cardano_cost_model_get_costs(cost_model);

  // Assert
  EXPECT_THAT(costs, testing::Not((const int64_t*)nullptr));

  // Cleanup
  cardano_cost_model_unref(&cost_model);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_get_costs, returnNullIfGivenNull)
{
  // Act
  const int64_t* costs = cardano_cost_model_get_costs(nullptr);

  // Assert
  EXPECT_EQ(costs, (const int64_t*)nullptr);
}

TEST(cardano_cost_model_get_language, canGetLanguage)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_language_version_t language;
  ASSERT_EQ(cardano_cost_model_get_language(cost_model, &language), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(language, CARDANO_PLUTUS_LANGUAGE_VERSION_V1);

  // Cleanup
  cardano_cost_model_unref(&cost_model);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cost_model_get_language, returnErrorIfGivenNull)
{
  // Act
  cardano_plutus_language_version_t language;
  cardano_error_t                   error = cardano_cost_model_get_language(nullptr, &language);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_cost_model_get_language, returnErrorIfLanguageIsNull)
{
  // Arrange
  cardano_cost_model_t*  cost_model = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t get_error = cardano_cost_model_get_language(cost_model, nullptr);

  // Assert
  EXPECT_EQ(get_error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cost_model_unref(&cost_model);
  cardano_cbor_reader_unref(&reader);
}
