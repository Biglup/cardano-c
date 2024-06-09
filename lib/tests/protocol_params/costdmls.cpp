/**
 * \file costmdls.cpp
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
#include <cardano/protocol_params/costmdls.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* COST_MODEL_V1_HEX          = "98a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a";
static const char* COST_MODEL_V2_HEX          = "98af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a";
static const char* COST_MODEL_V3_HEX          = "98b31a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a01020304";
static const char* COST_MODE_V1_CBOR_HEX      = "0098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a";
static const char* COST_MODE_V2_CBOR_HEX      = "0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a";
static const char* COST_MODE_V3_CBOR_HEX      = "0298b31a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a01020304";
static const char* COSTMDLS_CBOR              = "a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a";
static const char* COSTMDLS_ALL_CBOR          = "a30098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a0298b31a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a01020304";
static const char* PLUTUS_VASIL_LANGUAGE_VIEW = "a20198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a41005901b69f1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0aff";

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

TEST(cardano_costmdls_new, canCreateCostmdls)
{
  // Arrange
  cardano_costmdls_t* costmdls = nullptr;

  // Act
  cardano_error_t error = cardano_costmdls_new(&costmdls);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(costmdls, testing::Not((cardano_costmdls_t*)nullptr));

  // Cleanup
  cardano_costmdls_unref(&costmdls);
}

TEST(cardano_costmdls_new, returnsErrorIfCostmdlsIsNull)
{
  // Act
  cardano_error_t error = cardano_costmdls_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_costmdls_new, returnsErrorIfAllocationFails)
{
  // Arrange
  cardano_costmdls_t* costmdls = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_costmdls_new(&costmdls);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
}

TEST(cardano_costmdls_to_cbor, canSerializeCostmdls)
{
  // Arrange
  cardano_costmdls_t*    costmdls      = nullptr;
  cardano_cost_model_t*  cost_model_v1 = nullptr;
  cardano_cost_model_t*  cost_model_v2 = nullptr;
  cardano_cbor_reader_t* reader_v1     = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));
  cardano_cbor_reader_t* reader_v2     = cardano_cbor_reader_from_hex(COST_MODE_V2_CBOR_HEX, strlen(COST_MODE_V2_CBOR_HEX));
  cardano_cbor_writer_t* writer        = cardano_cbor_writer_new();

  cardano_error_t error = cardano_costmdls_new(&costmdls);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_cost_model_from_cbor(reader_v1, &cost_model_v1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_cost_model_from_cbor(reader_v2, &cost_model_v2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_costmdls_insert(costmdls, cost_model_v1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_costmdls_insert(costmdls, cost_model_v2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_costmdls_to_cbor(costmdls, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  cbor_hex = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor_hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, COSTMDLS_CBOR);

  // Cleanup
  cardano_costmdls_unref(&costmdls);
  cardano_cbor_writer_unref(&writer);
  cardano_cbor_reader_unref(&reader_v1);
  cardano_cbor_reader_unref(&reader_v2);
  cardano_cost_model_unref(&cost_model_v1);
  cardano_cost_model_unref(&cost_model_v2);
  free(cbor_hex);
}

TEST(cardano_costmdls_to_cbor, canSerializeCostmdlsWithAllThreeVersion)
{
  // Arrange
  cardano_costmdls_t*    costmdls      = nullptr;
  cardano_cost_model_t*  cost_model_v1 = nullptr;
  cardano_cost_model_t*  cost_model_v2 = nullptr;
  cardano_cost_model_t*  cost_model_v3 = nullptr;
  cardano_cbor_reader_t* reader_v1     = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));
  cardano_cbor_reader_t* reader_v2     = cardano_cbor_reader_from_hex(COST_MODE_V2_CBOR_HEX, strlen(COST_MODE_V2_CBOR_HEX));
  cardano_cbor_reader_t* reader_v3     = cardano_cbor_reader_from_hex(COST_MODE_V3_CBOR_HEX, strlen(COST_MODE_V3_CBOR_HEX));
  cardano_cbor_writer_t* writer        = cardano_cbor_writer_new();

  cardano_error_t error = cardano_costmdls_new(&costmdls);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_cost_model_from_cbor(reader_v1, &cost_model_v1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_cost_model_from_cbor(reader_v2, &cost_model_v2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_cost_model_from_cbor(reader_v3, &cost_model_v3);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_costmdls_insert(costmdls, cost_model_v1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_costmdls_insert(costmdls, cost_model_v2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_costmdls_insert(costmdls, cost_model_v3);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_costmdls_to_cbor(costmdls, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  cbor_hex = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor_hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, COSTMDLS_ALL_CBOR);

  // Cleanup
  cardano_costmdls_unref(&costmdls);
  cardano_cbor_writer_unref(&writer);
  cardano_cbor_reader_unref(&reader_v1);
  cardano_cbor_reader_unref(&reader_v2);
  cardano_cbor_reader_unref(&reader_v3);
  cardano_cost_model_unref(&cost_model_v1);
  cardano_cost_model_unref(&cost_model_v2);
  cardano_cost_model_unref(&cost_model_v3);
  free(cbor_hex);
}

TEST(cardano_costmdls_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_costmdls_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_costmdls_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_costmdls_t* costmdls = nullptr;

  cardano_error_t error = cardano_costmdls_new(&costmdls);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_costmdls_to_cbor(costmdls, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_costmdls_unref(&costmdls);
}

TEST(cardano_costmdls_from_cbor, canDeserializeCostmdls)
{
  // Arrange
  cardano_costmdls_t*    costmdls = nullptr;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(COSTMDLS_CBOR, strlen(COSTMDLS_CBOR));

  // Act
  cardano_error_t error = cardano_costmdls_from_cbor(reader, &costmdls);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(costmdls, testing::Not((cardano_costmdls_t*)nullptr));

  // Cleanup
  cardano_costmdls_unref(&costmdls);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_costmdls_from_cbor, returnErrorIfCostmdlsIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  // Act
  cardano_error_t error = cardano_costmdls_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_costmdls_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_costmdls_t* costmdls = nullptr;

  // Act
  cardano_error_t error = cardano_costmdls_from_cbor(nullptr, &costmdls);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_costmdls_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_costmdls_t*    costmdls = nullptr;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(COSTMDLS_CBOR, strlen(COSTMDLS_CBOR));

  cardano_error_t error = cardano_costmdls_from_cbor(reader, &costmdls);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_costmdls_ref(costmdls);

  // Assert
  EXPECT_THAT(costmdls, testing::Not((cardano_costmdls_t*)nullptr));
  EXPECT_EQ(cardano_costmdls_refcount(costmdls), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_costmdls_unref(&costmdls);
  cardano_costmdls_unref(&costmdls);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_costmdls_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_costmdls_ref(nullptr);
}

TEST(cardano_costmdls_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_costmdls_t* costmdls = nullptr;

  // Act
  cardano_costmdls_unref(&costmdls);
}

TEST(cardano_costmdls_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_costmdls_unref((cardano_costmdls_t**)nullptr);
}

TEST(cardano_costmdls_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_costmdls_t*    costmdls = nullptr;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(COSTMDLS_CBOR, strlen(COSTMDLS_CBOR));

  cardano_error_t error = cardano_costmdls_from_cbor(reader, &costmdls);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_costmdls_ref(costmdls);
  size_t ref_count = cardano_costmdls_refcount(costmdls);

  cardano_costmdls_unref(&costmdls);
  size_t updated_ref_count = cardano_costmdls_refcount(costmdls);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_costmdls_unref(&costmdls);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_costmdls_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_costmdls_t*    costmdls = nullptr;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(COSTMDLS_CBOR, strlen(COSTMDLS_CBOR));

  cardano_error_t error = cardano_costmdls_from_cbor(reader, &costmdls);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_costmdls_ref(costmdls);
  size_t ref_count = cardano_costmdls_refcount(costmdls);

  cardano_costmdls_unref(&costmdls);
  size_t updated_ref_count = cardano_costmdls_refcount(costmdls);

  cardano_costmdls_unref(&costmdls);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(costmdls, (cardano_costmdls_t*)nullptr);

  // Cleanup
  cardano_costmdls_unref(&costmdls);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_costmdls_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_costmdls_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_costmdls_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_costmdls_t* costmdls = nullptr;
  const char*         message  = "This is a test message";

  // Act
  cardano_costmdls_set_last_error(costmdls, message);

  // Assert
  EXPECT_STREQ(cardano_costmdls_get_last_error(costmdls), "Object is NULL.");
}

TEST(cardano_costmdls_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_costmdls_t*    costmdls = nullptr;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(COSTMDLS_CBOR, strlen(COSTMDLS_CBOR));

  cardano_error_t error = cardano_costmdls_from_cbor(reader, &costmdls);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_costmdls_set_last_error(costmdls, message);

  // Assert
  EXPECT_STREQ(cardano_costmdls_get_last_error(costmdls), "");

  // Cleanup
  cardano_costmdls_unref(&costmdls);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_costmdls_from_cbor, caDeserializeAnEmptyMap)
{
  // Arrange
  cardano_costmdls_t*    costmdls = nullptr;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex("a0", 2);

  // Act
  cardano_error_t error = cardano_costmdls_from_cbor(reader, &costmdls);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(costmdls, testing::Not((cardano_costmdls_t*)nullptr));
  EXPECT_EQ(cardano_costmdls_has(costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V1), false);
  EXPECT_EQ(cardano_costmdls_has(costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V2), false);
  EXPECT_EQ(cardano_costmdls_has(costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V3), false);

  // Cleanup
  cardano_costmdls_unref(&costmdls);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_costmdls_from_cbor, errorIfDoesntStartWithAMap)
{
  // Arrange
  cardano_costmdls_t*    costmdls = nullptr;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex("80", 2);

  // Act
  cardano_error_t error = cardano_costmdls_from_cbor(reader, &costmdls);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
  EXPECT_THAT(costmdls, testing::IsNull());

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_costmdls_from_cbor, errorIfMemoryAllocationFails)
{
  // Arrange
  cardano_costmdls_t*    costmdls = nullptr;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex("a0", 2);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_costmdls_from_cbor(reader, &costmdls);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_THAT(costmdls, testing::IsNull());

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_costmdls_from_cbor, returnErrorIfInvalidModel)
{
  // Arrange
  cardano_costmdls_t*    costmdls = nullptr;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex("a10000", 6);

  // Act
  cardano_error_t error = cardano_costmdls_from_cbor(reader, &costmdls);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);
  EXPECT_THAT(costmdls, testing::IsNull());

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_costmdls_from_cbor, canDeserializeCostmdlsWithAllVersions)
{
  // Arrange
  cardano_costmdls_t*    costmdls = nullptr;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(COSTMDLS_ALL_CBOR, strlen(COSTMDLS_ALL_CBOR));

  // Act
  cardano_error_t error = cardano_costmdls_from_cbor(reader, &costmdls);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(costmdls, testing::Not((cardano_costmdls_t*)nullptr));
  EXPECT_EQ(cardano_costmdls_has(costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V1), true);
  EXPECT_EQ(cardano_costmdls_has(costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V2), true);
  EXPECT_EQ(cardano_costmdls_has(costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V3), true);

  // Cleanup
  cardano_costmdls_unref(&costmdls);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_costmdls_insert, returnErrorIfCostmdlsIsNull)
{
  // Arrange
  cardano_costmdls_t*    costmdls      = nullptr;
  cardano_cost_model_t*  cost_model_v1 = nullptr;
  cardano_cbor_reader_t* reader        = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  cardano_error_t error = cardano_cost_model_from_cbor(reader, &cost_model_v1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_costmdls_insert(costmdls, cost_model_v1);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cost_model_unref(&cost_model_v1);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_costmdls_insert, returnErrorIfCostModelIsNull)
{
  // Arrange
  cardano_costmdls_t* costmdls = nullptr;

  cardano_error_t error = cardano_costmdls_new(&costmdls);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_costmdls_insert(costmdls, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_costmdls_unref(&costmdls);
}

TEST(cardano_costmdls_get, canGetCostModelV1)
{
  // Arrange
  cardano_costmdls_t*    costmdls      = nullptr;
  cardano_cost_model_t*  cost_model_v1 = nullptr;
  cardano_cbor_reader_t* reader        = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  cardano_error_t error = cardano_costmdls_new(&costmdls);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_cost_model_from_cbor(reader, &cost_model_v1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_costmdls_insert(costmdls, cost_model_v1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_cost_model_t* model = NULL;

  ASSERT_EQ(cardano_costmdls_get(costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V1, &model), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(model, testing::Not((cardano_cost_model_t*)nullptr));

  // Cleanup
  cardano_costmdls_unref(&costmdls);
  cardano_cost_model_unref(&model);
  cardano_cost_model_unref(&cost_model_v1);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_costmdls_get, canGetCostModelV2)
{
  // Arrange
  cardano_costmdls_t*    costmdls      = nullptr;
  cardano_cost_model_t*  cost_model_v2 = nullptr;
  cardano_cbor_reader_t* reader        = cardano_cbor_reader_from_hex(COST_MODE_V2_CBOR_HEX, strlen(COST_MODE_V2_CBOR_HEX));

  cardano_error_t error = cardano_costmdls_new(&costmdls);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_cost_model_from_cbor(reader, &cost_model_v2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_costmdls_insert(costmdls, cost_model_v2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_cost_model_t* model = NULL;

  ASSERT_EQ(cardano_costmdls_get(costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V2, &model), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(model, testing::Not((cardano_cost_model_t*)nullptr));

  // Cleanup
  cardano_costmdls_unref(&costmdls);
  cardano_cost_model_unref(&model);
  cardano_cost_model_unref(&cost_model_v2);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_costmdls_get, canGetCostModelV3)
{
  // Arrange
  cardano_costmdls_t*    costmdls      = nullptr;
  cardano_cost_model_t*  cost_model_v3 = nullptr;
  cardano_cbor_reader_t* reader        = cardano_cbor_reader_from_hex(COST_MODE_V3_CBOR_HEX, strlen(COST_MODE_V3_CBOR_HEX));

  cardano_error_t error = cardano_costmdls_new(&costmdls);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_cost_model_from_cbor(reader, &cost_model_v3);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_costmdls_insert(costmdls, cost_model_v3);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_cost_model_t* model = NULL;

  ASSERT_EQ(cardano_costmdls_get(costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V3, &model), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(model, testing::Not((cardano_cost_model_t*)nullptr));

  // Cleanup
  cardano_costmdls_unref(&costmdls);
  cardano_cost_model_unref(&model);
  cardano_cost_model_unref(&cost_model_v3);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_costmdls_get, returnsErrorIfCostmdlsIsNull)
{
  // Arrange
  cardano_cost_model_t* model = NULL;

  // Act
  cardano_error_t error = cardano_costmdls_get(nullptr, CARDANO_PLUTUS_LANGUAGE_VERSION_V1, &model);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_costmdls_get, returnsErrorIfModelIsNull)
{
  // Arrange
  cardano_costmdls_t* costmdls = nullptr;

  cardano_error_t error = cardano_costmdls_new(&costmdls);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_costmdls_get(costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V1, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_costmdls_unref(&costmdls);
}

TEST(cardano_costmdls_get, returnErrorIfGivenInvalidLanguage)
{
  // Arrange
  cardano_costmdls_t*    costmdls      = nullptr;
  cardano_cost_model_t*  cost_model_v1 = nullptr;
  cardano_cbor_reader_t* reader        = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  cardano_error_t error = cardano_costmdls_new(&costmdls);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_cost_model_from_cbor(reader, &cost_model_v1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_costmdls_insert(costmdls, cost_model_v1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_cost_model_t* model = NULL;

  ASSERT_EQ(cardano_costmdls_get(costmdls, (cardano_plutus_language_version_t)999, &model), CARDANO_INVALID_PLUTUS_COST_MODEL);

  // Assert
  EXPECT_THAT(model, testing::IsNull());

  // Cleanup
  cardano_costmdls_unref(&costmdls);
  cardano_cost_model_unref(&model);
  cardano_cbor_reader_unref(&reader);
  cardano_cost_model_unref(&cost_model_v1);
}

TEST(cardano_costmdls_has, returnsFalseIfCostmdlsIsNull)
{
  // Act
  bool has = cardano_costmdls_has(nullptr, CARDANO_PLUTUS_LANGUAGE_VERSION_V1);

  // Assert
  EXPECT_EQ(has, false);
}

TEST(cardano_costmdls_has, returnsFalseIfModelDoesntExist)
{
  // Arrange
  cardano_costmdls_t* costmdls = nullptr;

  cardano_error_t error = cardano_costmdls_new(&costmdls);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool has = cardano_costmdls_has(costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V1);

  // Assert
  EXPECT_EQ(has, false);

  // Cleanup
  cardano_costmdls_unref(&costmdls);
}

TEST(cardano_costmdls_has, returnsTrueIfModelExists)
{
  // Arrange
  cardano_costmdls_t*    costmdls      = nullptr;
  cardano_cost_model_t*  cost_model_v1 = nullptr;
  cardano_cbor_reader_t* reader        = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  cardano_error_t error = cardano_costmdls_new(&costmdls);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_cost_model_from_cbor(reader, &cost_model_v1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_costmdls_insert(costmdls, cost_model_v1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool has = cardano_costmdls_has(costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V1);

  // Assert
  EXPECT_EQ(has, true);

  // Cleanup
  cardano_costmdls_unref(&costmdls);
  cardano_cost_model_unref(&cost_model_v1);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_costmdls_has, returnsErrorIfGivenInvalidLanguage)
{
  // Arrange
  cardano_costmdls_t*    costmdls      = nullptr;
  cardano_cost_model_t*  cost_model_v1 = nullptr;
  cardano_cbor_reader_t* reader        = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));

  cardano_error_t error = cardano_costmdls_new(&costmdls);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_cost_model_from_cbor(reader, &cost_model_v1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_costmdls_insert(costmdls, cost_model_v1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool has = cardano_costmdls_has(costmdls, (cardano_plutus_language_version_t)999);

  // Assert
  EXPECT_EQ(has, false);

  // Cleanup
  cardano_costmdls_unref(&costmdls);
  cardano_cost_model_unref(&cost_model_v1);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_costmdls_get_language_views_encoding, canComputeLanguageViews)
{
  // Arrange
  cardano_costmdls_t*    costmdls      = nullptr;
  cardano_cost_model_t*  cost_model_v1 = nullptr;
  cardano_cost_model_t*  cost_model_v2 = nullptr;
  cardano_cbor_reader_t* reader_v1     = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));
  cardano_cbor_reader_t* reader_v2     = cardano_cbor_reader_from_hex(COST_MODE_V2_CBOR_HEX, strlen(COST_MODE_V2_CBOR_HEX));

  cardano_error_t error = cardano_costmdls_new(&costmdls);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_cost_model_from_cbor(reader_v1, &cost_model_v1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_cost_model_from_cbor(reader_v2, &cost_model_v2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_costmdls_insert(costmdls, cost_model_v1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_costmdls_insert(costmdls, cost_model_v2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_buffer_t* views = NULL;
  ASSERT_EQ(cardano_costmdls_get_language_views_encoding(costmdls, &views), CARDANO_SUCCESS);

  size_t views_size    = cardano_buffer_get_hex_size(views);
  char*  language_view = (char*)malloc(views_size);

  ASSERT_EQ(cardano_buffer_to_hex(views, language_view, views_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(language_view, PLUTUS_VASIL_LANGUAGE_VIEW);

  // Cleanup
  cardano_costmdls_unref(&costmdls);
  cardano_cbor_reader_unref(&reader_v1);
  cardano_cbor_reader_unref(&reader_v2);
  cardano_cost_model_unref(&cost_model_v1);
  cardano_cost_model_unref(&cost_model_v2);
  free(language_view);
  cardano_buffer_unref(&views);
}

TEST(cardano_costmdls_get_language_views_encoding, canComputeLanguageViewsWithAllModels)
{
  // Arrange
  cardano_costmdls_t*    costmdls      = nullptr;
  cardano_cost_model_t*  cost_model_v1 = nullptr;
  cardano_cost_model_t*  cost_model_v2 = nullptr;
  cardano_cost_model_t*  cost_model_v3 = nullptr;
  cardano_cbor_reader_t* reader_v1     = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));
  cardano_cbor_reader_t* reader_v2     = cardano_cbor_reader_from_hex(COST_MODE_V2_CBOR_HEX, strlen(COST_MODE_V2_CBOR_HEX));
  cardano_cbor_reader_t* reader_v3     = cardano_cbor_reader_from_hex(COST_MODE_V3_CBOR_HEX, strlen(COST_MODE_V3_CBOR_HEX));

  cardano_error_t error = cardano_costmdls_new(&costmdls);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_cost_model_from_cbor(reader_v1, &cost_model_v1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_cost_model_from_cbor(reader_v2, &cost_model_v2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_cost_model_from_cbor(reader_v3, &cost_model_v3);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_costmdls_insert(costmdls, cost_model_v1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_costmdls_insert(costmdls, cost_model_v2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_costmdls_insert(costmdls, cost_model_v3);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_buffer_t* views = NULL;
  ASSERT_EQ(cardano_costmdls_get_language_views_encoding(costmdls, &views), CARDANO_SUCCESS);

  size_t views_size    = cardano_buffer_get_hex_size(views);
  char*  language_view = (char*)malloc(views_size);

  ASSERT_EQ(cardano_buffer_to_hex(views, language_view, views_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(language_view, "a30198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a0298b31a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a0102030441005901b69f1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0aff");

  // Cleanup
  cardano_costmdls_unref(&costmdls);
  cardano_cbor_reader_unref(&reader_v1);
  cardano_cbor_reader_unref(&reader_v2);
  cardano_cbor_reader_unref(&reader_v3);
  cardano_cost_model_unref(&cost_model_v1);
  cardano_cost_model_unref(&cost_model_v2);
  cardano_cost_model_unref(&cost_model_v3);
  free(language_view);
  cardano_buffer_unref(&views);
}

TEST(cardano_costmdls_get_language_views_encoding, returnErrorWhenMemoryAllocationFails)
{
  // Arrange
  cardano_costmdls_t*    costmdls      = nullptr;
  cardano_cost_model_t*  cost_model_v1 = nullptr;
  cardano_cost_model_t*  cost_model_v2 = nullptr;
  cardano_cbor_reader_t* reader_v1     = cardano_cbor_reader_from_hex(COST_MODE_V1_CBOR_HEX, strlen(COST_MODE_V1_CBOR_HEX));
  cardano_cbor_reader_t* reader_v2     = cardano_cbor_reader_from_hex(COST_MODE_V2_CBOR_HEX, strlen(COST_MODE_V2_CBOR_HEX));

  cardano_error_t error = cardano_costmdls_new(&costmdls);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_cost_model_from_cbor(reader_v1, &cost_model_v1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_cost_model_from_cbor(reader_v2, &cost_model_v2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_costmdls_insert(costmdls, cost_model_v1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_costmdls_insert(costmdls, cost_model_v2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_buffer_t* views = NULL;
  ASSERT_EQ(cardano_costmdls_get_language_views_encoding(costmdls, &views), CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_costmdls_unref(&costmdls);
  cardano_cbor_reader_unref(&reader_v1);
  cardano_cbor_reader_unref(&reader_v2);
  cardano_cost_model_unref(&cost_model_v1);
  cardano_cost_model_unref(&cost_model_v2);
  cardano_set_allocators(malloc, realloc, free);
}