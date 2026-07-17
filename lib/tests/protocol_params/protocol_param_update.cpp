/**
 * \file protocol_param_update.cpp
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
#include <cardano/common/bigint.h>
#include <cardano/common/ex_units.h>
#include <cardano/common/unit_interval.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/plutus_data/plutus_map.h>
#include <cardano/protocol_params/protocol_param_update.h>

#include "../allocators_helpers.h"
#include "../json_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                        = "b8210018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d018201913881821d81e82185902";
static const char* COSTMDLS_CBOR               = "a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a";
static const char* EXECUTION_COSTS_CBOR        = "82d81e820102d81e820103";
static const char* POOL_VOTING_THRESHOLDS_CBOR = "85d81e820000d81e820101d81e820202d81e820303d81e820404";
static const char* DREP_VOTING_THRESHOLDS_CBOR = "8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909";
static const char* DIJKSTRA_PARAMS_CBOR        = "a418221affffffff18231a0140000018241964001825d81e820f0a";

/* UNIT TESTS ****************************************************************/

TEST(cardano_protocol_param_update_new, canCreateProtocolParamUpdate)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(protocol_param_update, testing::Not((cardano_protocol_param_update_t*)nullptr));

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_new, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Act
  cardano_error_t error = cardano_protocol_param_update_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_new, returnsErrorIfdenominatorAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(protocol_param_update, (cardano_protocol_param_update_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_to_cbor, canSerializeProtocolParamUpdate)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_writer_t*           writer                = cardano_cbor_writer_new();
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_protocol_param_update_to_cbor(protocol_param_update, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1); // +1 for the null terminator

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_writer_unref(&writer);
  cardano_cbor_reader_unref(&reader);
  free(actual_cbor);
}

TEST(cardano_protocol_param_update_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_protocol_param_update_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_protocol_param_update_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_protocol_param_update_to_cbor(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_from_cbor, canDeserializeProtocolParamUpdate)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(protocol_param_update, testing::Not((cardano_protocol_param_update_t*)nullptr));

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_from_cbor, returnErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(nullptr, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_protocol_param_update_ref(protocol_param_update);

  // Assert
  EXPECT_THAT(protocol_param_update, testing::Not((cardano_protocol_param_update_t*)nullptr));
  EXPECT_EQ(cardano_protocol_param_update_refcount(protocol_param_update), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_protocol_param_update_ref(nullptr);
}

TEST(cardano_protocol_param_update_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_protocol_param_update_unref((cardano_protocol_param_update_t**)nullptr);
}

TEST(cardano_protocol_param_update_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_protocol_param_update_ref(protocol_param_update);
  size_t ref_count = cardano_protocol_param_update_refcount(protocol_param_update);

  cardano_protocol_param_update_unref(&protocol_param_update);
  size_t updated_ref_count = cardano_protocol_param_update_refcount(protocol_param_update);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_protocol_param_update_ref(protocol_param_update);
  size_t ref_count = cardano_protocol_param_update_refcount(protocol_param_update);

  cardano_protocol_param_update_unref(&protocol_param_update);
  size_t updated_ref_count = cardano_protocol_param_update_refcount(protocol_param_update);

  cardano_protocol_param_update_unref(&protocol_param_update);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(protocol_param_update, (cardano_protocol_param_update_t*)nullptr);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_protocol_param_update_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_protocol_param_update_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  const char*                      message               = "This is a test message";

  // Act
  cardano_protocol_param_update_set_last_error(protocol_param_update, message);

  // Assert
  EXPECT_STREQ(cardano_protocol_param_update_get_last_error(protocol_param_update), "Object is NULL.");
}

TEST(cardano_protocol_param_update_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_protocol_param_update_set_last_error(protocol_param_update, message);

  // Assert
  EXPECT_STREQ(cardano_protocol_param_update_get_last_error(protocol_param_update), "");

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfDuplicatedKeyForUint)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a200000000", strlen("a200000000"));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfMemoryAllocationErrorWhenReadingUint)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a200000000", strlen("a200000000"));

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_six_malloc, realloc, free);

  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfDuplicatedUnitInterval)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a209d81e82010509d81e820105", strlen("a209d81e82010509d81e820105"));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfDuplicatedEntropy)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a20d8201582000000000000000000000000000000000000000000000000000000000000000000d820158200000000000000000000000000000000000000000000000000000000000000000", strlen("a20d8201582000000000000000000000000000000000000000000000000000000000000000000d820158200000000000000000000000000000000000000000000000000000000000000000"));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfInvalidArrayInEntropy)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a10d830158200000000000000000000000000000000000000000000000000000000000000000", strlen("a10d820158200000000000000000000000000000000000000000000000000000000000000000"));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_from_cbor, canReadEmptyEntropy)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a10d8100", strlen("a10d8100"));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfInvalidUintInEntropy)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a10d82fe58200000000000000000000000000000000000000000000000000000000000000000", strlen("a10d820158200000000000000000000000000000000000000000000000000000000000000000"));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfInvalidBytestringInEntropy)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a10d8201fe200000000000000000000000000000000000000000000000000000000000000000", strlen("a10d820158200000000000000000000000000000000000000000000000000000000000000000"));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfDuplicatedProtocolVersion)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a20e8201030e820103", strlen("a20e8201030e820103"));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfDuplicatedCostModels)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a212a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a12a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a", strlen("a212a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a12a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a"));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfDuplicatedExPrices)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a21382d81e820102d81e8201031382d81e820102d81e820103", strlen("a21382d81e820102d81e8201031382d81e820102d81e820103"));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfDuplicatedExUnit)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a214821b000086788ffc4e831b00015060e9e4645114821b000086788ffc4e831b00015060e9e46451", strlen("a214821b000086788ffc4e831b00015060e9e4645114821b000086788ffc4e831b00015060e9e46451"));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfDuplicatedPoolVotingThresholds)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a2181985d81e820000d81e820101d81e820202d81e820303d81e820404181985d81e820000d81e820101d81e820202d81e820303d81e820404", strlen("a2181985d81e820000d81e820101d81e820202d81e820303d81e820404181985d81e820000d81e820101d81e820202d81e820303d81e820404"));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfDuplicatedDrepVotingThresholds)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a2181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909", strlen("a2181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909"));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfInvalidKey)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a1198a8a8a8a", strlen("a1188a8a8a8a"));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_MAP_KEY);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfKeyIsPastDijkstraRange)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a1182600", strlen("a1182600"));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_MAP_KEY);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_from_cbor, returnErrorIfMemoryAllocationFailS)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a20e8201030e820103", strlen("a20e8201030e820103"));

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfNotAMap)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("820103", strlen("820103"));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfMapKeyIsNotAnInteger)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a1a1a1a1", strlen("a1a1a1a1"));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_min_fee_a, returnsTheMinFeeA)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  uint64_t min_fee_a = 0;

  EXPECT_EQ(cardano_protocol_param_update_get_min_fee_a(protocol_param_update, &min_fee_a), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(min_fee_a, 100);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_min_fee_a, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t min_fee_a = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_min_fee_a(nullptr, &min_fee_a);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_min_fee_a, returnsErrorIfMinFeeAIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_min_fee_a(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_min_fee_b, returnsTheMinFeeB)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  uint64_t min_fee_b = 0;

  EXPECT_EQ(cardano_protocol_param_update_get_min_fee_b(protocol_param_update, &min_fee_b), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(min_fee_b, 200);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_min_fee_b, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t min_fee_b = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_min_fee_b(nullptr, &min_fee_b);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_min_fee_b, returnsErrorIfMinFeeBIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_min_fee_b(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_max_block_body_size, returnsTheMaxBlockBodySize)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  uint64_t max_block_body_size = 0;

  EXPECT_EQ(cardano_protocol_param_update_get_max_block_body_size(protocol_param_update, &max_block_body_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(max_block_body_size, 300);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_max_block_body_size, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t max_block_body_size = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_block_body_size(nullptr, &max_block_body_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_max_block_body_size, returnsErrorIfMaxBlockBodySizeIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_block_body_size(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_max_tx_size, returnsTheMaxTxSize)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  uint64_t max_tx_size = 0;

  EXPECT_EQ(cardano_protocol_param_update_get_max_tx_size(protocol_param_update, &max_tx_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(max_tx_size, 400);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_max_tx_size, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t max_tx_size = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_tx_size(nullptr, &max_tx_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_max_tx_size, returnsErrorIfMaxTxSizeIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_tx_size(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_max_block_header_size, returnsTheMaxBlockHeaderSize)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  uint64_t max_block_header_size = 0;

  EXPECT_EQ(cardano_protocol_param_update_get_max_block_header_size(protocol_param_update, &max_block_header_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(max_block_header_size, 500);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_max_block_header_size, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t max_block_header_size = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_block_header_size(nullptr, &max_block_header_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_max_block_header_size, returnsErrorIfMaxBlockHeaderSizeIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_block_header_size(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_key_deposit, returnsTheKeyDeposit)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  uint64_t key_deposit = 0;

  EXPECT_EQ(cardano_protocol_param_update_get_key_deposit(protocol_param_update, &key_deposit), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(key_deposit, 2000000);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_key_deposit, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t key_deposit = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_key_deposit(nullptr, &key_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_key_deposit, returnsErrorIfKeyDepositIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_key_deposit(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_pool_deposit, returnsThePoolDeposit)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  uint64_t pool_deposit = 0;

  EXPECT_EQ(cardano_protocol_param_update_get_pool_deposit(protocol_param_update, &pool_deposit), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(pool_deposit, 200000000);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_pool_deposit, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t pool_deposit = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_pool_deposit(nullptr, &pool_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_pool_deposit, returnsErrorIfPoolDepositIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_pool_deposit(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_e_max, returnsTheEMax)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  uint64_t e_max = 0;

  EXPECT_EQ(cardano_protocol_param_update_get_max_epoch(protocol_param_update, &e_max), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(e_max, 800);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_e_max, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t e_max = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_epoch(nullptr, &e_max);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_e_max, returnsErrorIfEMaxIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_epoch(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_n_opt, returnsTheNOpt)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  uint64_t n_opt = 0;

  EXPECT_EQ(cardano_protocol_param_update_get_n_opt(protocol_param_update, &n_opt), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(n_opt, 900);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_n_opt, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t n_opt = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_n_opt(nullptr, &n_opt);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_n_opt, returnsErrorIfNOptIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_n_opt(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_pool_pledge_influence, returnsTheA0)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_unit_interval_t*         a0                    = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_pool_pledge_influence(protocol_param_update, &a0), CARDANO_SUCCESS);

  // Assert
  EXPECT_NEAR(cardano_unit_interval_to_double(a0), 0.5, 0.1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
  cardano_unit_interval_unref(&a0);
}

TEST(cardano_protocol_param_update_get_pool_pledge_influence, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_unit_interval_t* a0 = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_pool_pledge_influence(nullptr, &a0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_pool_pledge_influence, returnsErrorIfA0IsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_pool_pledge_influence(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_expansion_rate, returnsTheExpansionRate)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_unit_interval_t*         expansion_rate        = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_expansion_rate(protocol_param_update, &expansion_rate), CARDANO_SUCCESS);

  // Assert
  EXPECT_NEAR(cardano_unit_interval_to_double(expansion_rate), 0.3333333333333333, 0.1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
  cardano_unit_interval_unref(&expansion_rate);
}

TEST(cardano_protocol_param_update_get_expansion_rate, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_unit_interval_t* expansion_rate = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_expansion_rate(nullptr, &expansion_rate);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_expansion_rate, returnsErrorIfExpansionRateIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_expansion_rate(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_treasury_growth_rate, returnsTheTreasuryGrowthRate)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_unit_interval_t*         treasury_growth_rate  = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_treasury_growth_rate(protocol_param_update, &treasury_growth_rate), CARDANO_SUCCESS);

  // Assert
  EXPECT_NEAR(cardano_unit_interval_to_double(treasury_growth_rate), 0.25, 0.1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
  cardano_unit_interval_unref(&treasury_growth_rate);
}

TEST(cardano_protocol_param_update_get_treasury_growth_rate, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_unit_interval_t* treasury_growth_rate = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_treasury_growth_rate(nullptr, &treasury_growth_rate);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_treasury_growth_rate, returnsErrorIfTreasuryGrowthRateIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_treasury_growth_rate(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_d, returnsTheDecentralization)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_unit_interval_t*         decentralization      = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_d(protocol_param_update, &decentralization), CARDANO_SUCCESS);

  // Assert
  EXPECT_NEAR(cardano_unit_interval_to_double(decentralization), 0.2, 0.01);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
  cardano_unit_interval_unref(&decentralization);
}

TEST(cardano_protocol_param_update_get_d, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_unit_interval_t* decentralization = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_d(nullptr, &decentralization);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_d, returnsErrorIfDecentralizationIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_d(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_extra_entropy, returnsTheExtraEntropy)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_buffer_t*                extra_entropy         = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_extra_entropy(protocol_param_update, &extra_entropy), CARDANO_SUCCESS);
  size_t extra_entropy_size = cardano_buffer_get_hex_size(extra_entropy);
  char*  extra_entropy_hex  = (char*)malloc(extra_entropy_size);

  EXPECT_EQ(cardano_buffer_to_hex(extra_entropy, extra_entropy_hex, extra_entropy_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(extra_entropy_hex, "0000000000000000000000000000000000000000000000000000000000000000");

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
  cardano_buffer_unref(&extra_entropy);
  free(extra_entropy_hex);
}

TEST(cardano_protocol_param_update_get_extra_entropy, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_buffer_t* extra_entropy = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_extra_entropy(nullptr, &extra_entropy);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_extra_entropy, returnsErrorIfExtraEntropyIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_extra_entropy(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_protocol_version, returnsTheProtocolVersion)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_protocol_version_t*      protocol_version      = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_protocol_version(protocol_param_update, &protocol_version), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_protocol_version_get_major(protocol_version), 1);
  EXPECT_EQ(cardano_protocol_version_get_minor(protocol_version), 3);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
  cardano_protocol_version_unref(&protocol_version);
}

TEST(cardano_protocol_param_update_get_protocol_version, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_protocol_version(nullptr, &protocol_version);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_protocol_version, returnsErrorIfProtocolVersionIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_protocol_version(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_min_pool_cost, returnsTheMinPoolCost)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  uint64_t                         min_pool_cost         = 0;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_min_pool_cost(protocol_param_update, &min_pool_cost), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(min_pool_cost, 1000);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_min_pool_cost, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t min_pool_cost = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_min_pool_cost(nullptr, &min_pool_cost);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_min_pool_cost, returnsErrorIfMinPoolCostIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_min_pool_cost(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_ada_per_utxo_byte, returnsTheMaxValueSize)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  uint64_t                         ada_per_utxo_byte     = 0;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_ada_per_utxo_byte(protocol_param_update, &ada_per_utxo_byte), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(ada_per_utxo_byte, 35000);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_ada_per_utxo_byte, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t ada_per_utxo_byte = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_ada_per_utxo_byte(nullptr, &ada_per_utxo_byte);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_ada_per_utxo_byte, returnsErrorIfAdaPerUtxoByteIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_ada_per_utxo_byte(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_cost_models, returnsTheCostModels)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_costmdls_t*              cost_models           = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_cost_models(protocol_param_update, &cost_models), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_costmdls_has(cost_models, CARDANO_PLUTUS_LANGUAGE_VERSION_V1), true);
  EXPECT_EQ(cardano_costmdls_has(cost_models, CARDANO_PLUTUS_LANGUAGE_VERSION_V2), true);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
  cardano_costmdls_unref(&cost_models);
}

TEST(cardano_protocol_param_update_get_cost_models, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_costmdls_t* cost_models = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_cost_models(nullptr, &cost_models);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_cost_models, returnsErrorIfCostModelsIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_cost_models(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_execution_costs, returnsTheExecutionCosts)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_ex_unit_prices_t*        execution_costs       = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_execution_costs(protocol_param_update, &execution_costs), CARDANO_SUCCESS);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
  cardano_ex_unit_prices_unref(&execution_costs);
}

TEST(cardano_protocol_param_update_get_execution_costs, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_ex_unit_prices_t* execution_costs = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_execution_costs(nullptr, &execution_costs);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_execution_costs, returnsErrorIfExecutionCostsIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_execution_costs(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_max_tx_execution_units, returnsTheMaxTxExecutionUnits)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_ex_units_t* max_tx_execution_units = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_max_tx_ex_units(protocol_param_update, &max_tx_execution_units), CARDANO_SUCCESS);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
  cardano_ex_units_unref(&max_tx_execution_units);
}

TEST(cardano_protocol_param_update_get_max_tx_execution_units, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_ex_units_t* max_tx_execution_units = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_tx_ex_units(nullptr, &max_tx_execution_units);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_max_tx_execution_units, returnsErrorIfMaxTxExecutionUnitsIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_tx_ex_units(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_max_block_execution_units, returnsTheMaxBlockExecutionUnits)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_ex_units_t* max_block_execution_units = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_max_block_ex_units(protocol_param_update, &max_block_execution_units), CARDANO_SUCCESS);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
  cardano_ex_units_unref(&max_block_execution_units);
}

TEST(cardano_protocol_param_update_get_max_block_execution_units, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_ex_units_t* max_block_execution_units = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_block_ex_units(nullptr, &max_block_execution_units);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_max_block_execution_units, returnsErrorIfMaxBlockExecutionUnitsIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_block_ex_units(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_max_value_size, returnsTheMaxValueSize)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  uint64_t                         max_value_size        = 0;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_max_value_size(protocol_param_update, &max_value_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(max_value_size, 954);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_max_value_size, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t max_value_size = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_value_size(nullptr, &max_value_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_max_value_size, returnsErrorIfMaxValueSizeIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_value_size(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_collateral_percentage, returnsTheCollateralPercentage)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  uint64_t collateral_percentage = 0;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_collateral_percentage(protocol_param_update, &collateral_percentage), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(collateral_percentage, 852);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_collateral_percentage, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t collateral_percentage = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_collateral_percentage(nullptr, &collateral_percentage);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_collateral_percentage, returnsErrorIfCollateralPercentageIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_collateral_percentage(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_max_collateral_inputs, returnsTheMaxCollateralInputs)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  uint64_t max_collateral_inputs = 0;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_max_collateral_inputs(protocol_param_update, &max_collateral_inputs), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(max_collateral_inputs, 100);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_max_collateral_inputs, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t max_collateral_inputs = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_collateral_inputs(nullptr, &max_collateral_inputs);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_max_collateral_inputs, returnsErrorIfMaxCollateralInputsIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_collateral_inputs(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_pool_voting_thresholds, returnsThePoolVotingThresholds)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_pool_voting_thresholds(protocol_param_update, &pool_voting_thresholds), CARDANO_SUCCESS);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
}

TEST(cardano_protocol_param_update_get_pool_voting_thresholds, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_pool_voting_thresholds(nullptr, &pool_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_pool_voting_thresholds, returnsErrorIfPoolVotingThresholdsIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_pool_voting_thresholds(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_drep_voting_thresholds, returnsTheDRepVotingThresholds)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_drep_voting_thresholds_t* d_rep_voting_thresholds = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_drep_voting_thresholds(protocol_param_update, &d_rep_voting_thresholds), CARDANO_SUCCESS);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
  cardano_drep_voting_thresholds_unref(&d_rep_voting_thresholds);
}

TEST(cardano_protocol_param_update_get_drep_voting_thresholds, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_drep_voting_thresholds_t* d_rep_voting_thresholds = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_drep_voting_thresholds(nullptr, &d_rep_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_drep_voting_thresholds, returnsErrorIfDRepVotingThresholdsIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_drep_voting_thresholds(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_min_committee_size, returnsTheMinCommitteeSize)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  uint64_t min_committee_size = 0;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_min_committee_size(protocol_param_update, &min_committee_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(min_committee_size, 100);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_min_committee_size, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t min_committee_size = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_min_committee_size(nullptr, &min_committee_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_min_committee_size, returnsErrorIfMinCommitteeSizeIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_min_committee_size(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_committee_term_limit, returnsTheCommitteeTermLimit)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  uint64_t committee_term_limit = 0;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_committee_term_limit(protocol_param_update, &committee_term_limit), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(committee_term_limit, 200);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_committee_term_limit, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t committee_term_limit = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_committee_term_limit(nullptr, &committee_term_limit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_committee_term_limit, returnsErrorIfCommitteeTermLimitIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_committee_term_limit(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_governance_action_validity_period, returnsTheGovernanceActionValidityPeriod)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  uint64_t governance_action_validity_period = 0;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_governance_action_validity_period(protocol_param_update, &governance_action_validity_period), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(governance_action_validity_period, 300);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_governance_action_validity_period, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t governance_action_validity_period = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_governance_action_validity_period(nullptr, &governance_action_validity_period);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_governance_action_validity_period, returnsErrorIfGovernanceActionValidityPeriodIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_governance_action_validity_period(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_governance_action_deposit, returnsTheGovernanceActionDeposit)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  uint64_t governance_action_deposit = 0;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_governance_action_deposit(protocol_param_update, &governance_action_deposit), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(governance_action_deposit, 1000);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_governance_action_deposit, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t governance_action_deposit = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_governance_action_deposit(nullptr, &governance_action_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_governance_action_deposit, returnsErrorIfGovernanceActionDepositIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_governance_action_deposit(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_drep_deposit, returnsTheDRepDeposit)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  uint64_t d_rep_deposit = 0;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_drep_deposit(protocol_param_update, &d_rep_deposit), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(d_rep_deposit, 2000);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_drep_deposit, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t d_rep_deposit = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_drep_deposit(nullptr, &d_rep_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_drep_deposit, returnsErrorIfDRepDepositIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_drep_deposit(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_drep_inactivity_period, returnsTheDRepInactivityPeriod)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  uint64_t d_rep_inactivity_period = 0;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_drep_inactivity_period(protocol_param_update, &d_rep_inactivity_period), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(d_rep_inactivity_period, 5000);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_drep_inactivity_period, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t d_rep_inactivity_period = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_drep_inactivity_period(nullptr, &d_rep_inactivity_period);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_drep_inactivity_period, returnsErrorIfDRepInactivityPeriodIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_drep_inactivity_period(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_ref_script_cost_per_byte, returnsTheRefScriptCostPerByte)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_unit_interval_t* ref_script_cost_per_byte = NULL;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_ref_script_cost_per_byte(protocol_param_update, &ref_script_cost_per_byte), CARDANO_SUCCESS);

  // Assert
  EXPECT_NEAR(cardano_unit_interval_to_double(ref_script_cost_per_byte), 44.5, 0.1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_unit_interval_unref(&ref_script_cost_per_byte);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_ref_script_cost_per_byte, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_unit_interval_t* ref_script_cost_per_byte = NULL;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_ref_script_cost_per_byte(nullptr, &ref_script_cost_per_byte);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_ref_script_cost_per_byte, returnsErrorIfRefScriptCostPerByteIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_ref_script_cost_per_byte(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_, returnsElementNotFoundIfMissingField)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  uint64_t min_fee_a = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_min_fee_a(protocol_param_update, &min_fee_a), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t min_fee_b = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_min_fee_b(protocol_param_update, &min_fee_b), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t max_block_body_size = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_block_body_size(protocol_param_update, &max_block_body_size), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t max_tx_size = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_tx_size(protocol_param_update, &max_tx_size), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t max_block_header_size = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_block_header_size(protocol_param_update, &max_block_header_size), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t key_deposit = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_key_deposit(protocol_param_update, &key_deposit), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t pool_deposit = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_pool_deposit(protocol_param_update, &pool_deposit), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t e_max = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_epoch(protocol_param_update, &e_max), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t n_opt = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_n_opt(protocol_param_update, &n_opt), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  cardano_unit_interval_t* rho = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_pool_pledge_influence(protocol_param_update, &rho), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  cardano_unit_interval_t* tau = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_expansion_rate(protocol_param_update, &tau), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  cardano_unit_interval_t* a_0 = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_treasury_growth_rate(protocol_param_update, &a_0), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  cardano_unit_interval_t* d = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_d(protocol_param_update, &d), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  cardano_buffer_t* extra_entropy = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_extra_entropy(protocol_param_update, &extra_entropy), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  cardano_protocol_version_t* protocol_version = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_protocol_version(protocol_param_update, &protocol_version), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t min_pool_cost = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_min_pool_cost(protocol_param_update, &min_pool_cost), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t ada_per_utxo_byte = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_ada_per_utxo_byte(protocol_param_update, &ada_per_utxo_byte), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  cardano_costmdls_t* cost_models = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_cost_models(protocol_param_update, &cost_models), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  cardano_ex_unit_prices_t* execution_costs = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_execution_costs(protocol_param_update, &execution_costs), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  cardano_ex_units_t* max_tx_execution_units = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_max_tx_ex_units(protocol_param_update, &max_tx_execution_units), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  cardano_ex_units_t* max_block_execution_units = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_max_block_ex_units(protocol_param_update, &max_block_execution_units), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t max_value_size = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_value_size(protocol_param_update, &max_value_size), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t collateral_percentage = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_collateral_percentage(protocol_param_update, &collateral_percentage), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t max_collateral_inputs = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_collateral_inputs(protocol_param_update, &max_collateral_inputs), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_pool_voting_thresholds(protocol_param_update, &pool_voting_thresholds), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  cardano_drep_voting_thresholds_t* d_rep_voting_thresholds = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_drep_voting_thresholds(protocol_param_update, &d_rep_voting_thresholds), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t min_committee_size = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_min_committee_size(protocol_param_update, &min_committee_size), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t committee_term_limit = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_committee_term_limit(protocol_param_update, &committee_term_limit), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t governance_action_validity_period = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_governance_action_validity_period(protocol_param_update, &governance_action_validity_period), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t governance_action_deposit = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_governance_action_deposit(protocol_param_update, &governance_action_deposit), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t d_rep_deposit = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_drep_deposit(protocol_param_update, &d_rep_deposit), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t d_rep_inactivity_period = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_drep_inactivity_period(protocol_param_update, &d_rep_inactivity_period), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  cardano_unit_interval_t* ref_script_cost_per_byte = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_ref_script_cost_per_byte(protocol_param_update, &ref_script_cost_per_byte), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_min_fee_a, setsTheMinFeeA)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t min_fee_a = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_min_fee_a(protocol_param_update, &min_fee_a), CARDANO_SUCCESS);

  // Assert
  min_fee_a = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_min_fee_a(protocol_param_update, &min_fee_a), CARDANO_SUCCESS);
  EXPECT_EQ(min_fee_a, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_min_fee_a, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t min_fee_a = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_min_fee_a(nullptr, &min_fee_a);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_min_fee_a, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t min_fee_a = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_min_fee_a(protocol_param_update, &min_fee_a), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_min_fee_a(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  min_fee_a = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_min_fee_a(protocol_param_update, &min_fee_a), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_min_fee_a, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         min_fee_a             = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_min_fee_a(protocol_param_update, &min_fee_a);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_min_fee_b, setsTheMinFeeB)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t min_fee_b = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_min_fee_b(protocol_param_update, &min_fee_b), CARDANO_SUCCESS);

  // Assert
  min_fee_b = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_min_fee_b(protocol_param_update, &min_fee_b), CARDANO_SUCCESS);
  EXPECT_EQ(min_fee_b, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_min_fee_b, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t min_fee_b = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_min_fee_b(nullptr, &min_fee_b);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_min_fee_b, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t min_fee_b = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_min_fee_b(protocol_param_update, &min_fee_b), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_min_fee_b(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  min_fee_b = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_min_fee_b(protocol_param_update, &min_fee_b), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_min_fee_b, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         min_fee_b             = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_min_fee_b(protocol_param_update, &min_fee_b);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_max_block_body_size, setsTheMaxBlockBodySize)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t max_block_body_size = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_block_body_size(protocol_param_update, &max_block_body_size), CARDANO_SUCCESS);

  // Assert
  max_block_body_size = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_block_body_size(protocol_param_update, &max_block_body_size), CARDANO_SUCCESS);
  EXPECT_EQ(max_block_body_size, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_block_body_size, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t max_block_body_size = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_max_block_body_size(nullptr, &max_block_body_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_max_block_body_size, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t max_block_body_size = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_block_body_size(protocol_param_update, &max_block_body_size), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_max_block_body_size(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  max_block_body_size = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_block_body_size(protocol_param_update, &max_block_body_size), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_block_body_size, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         max_block_body_size   = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_max_block_body_size(protocol_param_update, &max_block_body_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_max_tx_size, setsTheMaxTxSize)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t max_tx_size = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_tx_size(protocol_param_update, &max_tx_size), CARDANO_SUCCESS);

  // Assert
  max_tx_size = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_tx_size(protocol_param_update, &max_tx_size), CARDANO_SUCCESS);
  EXPECT_EQ(max_tx_size, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_tx_size, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t max_tx_size = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_max_tx_size(nullptr, &max_tx_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_max_tx_size, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t max_tx_size = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_tx_size(protocol_param_update, &max_tx_size), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_max_tx_size(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  max_tx_size = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_tx_size(protocol_param_update, &max_tx_size), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_tx_size, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         max_tx_size           = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_max_tx_size(protocol_param_update, &max_tx_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_max_block_header_size, setsTheMaxBlockHeaderSize)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t max_block_header_size = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_block_header_size(protocol_param_update, &max_block_header_size), CARDANO_SUCCESS);

  // Assert
  max_block_header_size = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_block_header_size(protocol_param_update, &max_block_header_size), CARDANO_SUCCESS);
  EXPECT_EQ(max_block_header_size, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_block_header_size, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t max_block_header_size = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_max_block_header_size(nullptr, &max_block_header_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_max_block_header_size, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t max_block_header_size = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_block_header_size(protocol_param_update, &max_block_header_size), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_max_block_header_size(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  max_block_header_size = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_block_header_size(protocol_param_update, &max_block_header_size), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_block_header_size, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         max_block_header_size = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_max_block_header_size(protocol_param_update, &max_block_header_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_key_deposit, setsTheKeyDeposit)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t key_deposit = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_key_deposit(protocol_param_update, &key_deposit), CARDANO_SUCCESS);

  // Assert
  key_deposit = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_key_deposit(protocol_param_update, &key_deposit), CARDANO_SUCCESS);
  EXPECT_EQ(key_deposit, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_key_deposit, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t key_deposit = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_key_deposit(nullptr, &key_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_key_deposit, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t key_deposit = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_key_deposit(protocol_param_update, &key_deposit), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_key_deposit(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  key_deposit = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_key_deposit(protocol_param_update, &key_deposit), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_key_deposit, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         key_deposit           = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_key_deposit(protocol_param_update, &key_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_pool_deposit, setsThePoolDeposit)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t pool_deposit = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_pool_deposit(protocol_param_update, &pool_deposit), CARDANO_SUCCESS);

  // Assert
  pool_deposit = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_pool_deposit(protocol_param_update, &pool_deposit), CARDANO_SUCCESS);
  EXPECT_EQ(pool_deposit, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_pool_deposit, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t pool_deposit = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_pool_deposit(nullptr, &pool_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_pool_deposit, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t pool_deposit = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_pool_deposit(protocol_param_update, &pool_deposit), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_pool_deposit(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  pool_deposit = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_pool_deposit(protocol_param_update, &pool_deposit), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_pool_deposit, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         pool_deposit          = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_pool_deposit(protocol_param_update, &pool_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_max_epoch, setsTheMaxEpoch)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t e_max = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_epoch(protocol_param_update, &e_max), CARDANO_SUCCESS);

  // Assert
  e_max = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_epoch(protocol_param_update, &e_max), CARDANO_SUCCESS);
  EXPECT_EQ(e_max, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_epoch, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t e_max = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_max_epoch(nullptr, &e_max);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_max_epoch, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t e_max = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_epoch(protocol_param_update, &e_max), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_max_epoch(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  e_max = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_epoch(protocol_param_update, &e_max), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_epoch, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         e_max                 = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_max_epoch(protocol_param_update, &e_max);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_n_opt, setsTheNOpt)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t n_opt = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_n_opt(protocol_param_update, &n_opt), CARDANO_SUCCESS);

  // Assert
  n_opt = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_n_opt(protocol_param_update, &n_opt), CARDANO_SUCCESS);
  EXPECT_EQ(n_opt, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_n_opt, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t n_opt = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_n_opt(nullptr, &n_opt);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_n_opt, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t n_opt = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_n_opt(protocol_param_update, &n_opt), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_n_opt(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  n_opt = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_n_opt(protocol_param_update, &n_opt), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_n_opt, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         n_opt                 = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_n_opt(protocol_param_update, &n_opt);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_pool_pledge_influence, setsThePoolPledgeInfluence)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_unit_interval_t* rho = NULL;
  EXPECT_EQ(cardano_unit_interval_new(1, 10, &rho), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_pool_pledge_influence(protocol_param_update, rho), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* rho_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_pool_pledge_influence(protocol_param_update, &rho_out), CARDANO_SUCCESS);
  EXPECT_NEAR(cardano_unit_interval_to_double(rho_out), 0.1, 0.01);

  // Cleanup
  cardano_unit_interval_unref(&rho);
  cardano_unit_interval_unref(&rho_out);
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_pool_pledge_influence, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_unit_interval_t* rho = NULL;
  EXPECT_EQ(cardano_unit_interval_new(1, 10, &rho), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_pool_pledge_influence(nullptr, rho);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_unit_interval_unref(&rho);
}

TEST(cardano_protocol_param_update_set_pool_pledge_influence, returnsErrorIfPoolPledgeInfluenceIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_pool_pledge_influence(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_pool_pledge_influence, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_unit_interval_t*         rho                   = NULL;
  EXPECT_EQ(cardano_unit_interval_new(1, 10, &rho), CARDANO_SUCCESS);

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_pool_pledge_influence(protocol_param_update, rho), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_pool_pledge_influence(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* rho_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_pool_pledge_influence(protocol_param_update, &rho_out), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_unit_interval_unref(&rho);
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_expansion_rate, setsTheExpansionRate)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_unit_interval_t* tau = NULL;
  EXPECT_EQ(cardano_unit_interval_new(1, 10, &tau), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_expansion_rate(protocol_param_update, tau), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* tau_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_expansion_rate(protocol_param_update, &tau_out), CARDANO_SUCCESS);
  EXPECT_NEAR(cardano_unit_interval_to_double(tau_out), 0.1, 0.01);

  // Cleanup
  cardano_unit_interval_unref(&tau);
  cardano_unit_interval_unref(&tau_out);
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_expansion_rate, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_unit_interval_t* tau = NULL;
  EXPECT_EQ(cardano_unit_interval_new(1, 10, &tau), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_expansion_rate(nullptr, tau);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_unit_interval_unref(&tau);
}

TEST(cardano_protocol_param_update_set_expansion_rate, returnsErrorIfExpansionRateIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_expansion_rate(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_expansion_rate, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_unit_interval_t*         tau                   = NULL;
  EXPECT_EQ(cardano_unit_interval_new(1, 10, &tau), CARDANO_SUCCESS);

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_expansion_rate(protocol_param_update, tau), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_expansion_rate(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* tau_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_expansion_rate(protocol_param_update, &tau_out), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_unit_interval_unref(&tau);
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_treasury_growth_rate, setsTheTreasuryGrowthRate)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_unit_interval_t* tau = NULL;
  EXPECT_EQ(cardano_unit_interval_new(1, 10, &tau), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_treasury_growth_rate(protocol_param_update, tau), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* tau_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_treasury_growth_rate(protocol_param_update, &tau_out), CARDANO_SUCCESS);
  EXPECT_NEAR(cardano_unit_interval_to_double(tau_out), 0.1, 0.01);

  // Cleanup
  cardano_unit_interval_unref(&tau);
  cardano_unit_interval_unref(&tau_out);
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_treasury_growth_rate, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_unit_interval_t* tau = NULL;
  EXPECT_EQ(cardano_unit_interval_new(1, 10, &tau), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_treasury_growth_rate(nullptr, tau);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_unit_interval_unref(&tau);
}

TEST(cardano_protocol_param_update_set_treasury_growth_rate, returnsErrorIfTreasuryGrowthRateIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_treasury_growth_rate(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_treasury_growth_rate, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_unit_interval_t*         tau                   = NULL;
  EXPECT_EQ(cardano_unit_interval_new(1, 10, &tau), CARDANO_SUCCESS);

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_treasury_growth_rate(protocol_param_update, tau), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_treasury_growth_rate(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* tau_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_treasury_growth_rate(protocol_param_update, &tau_out), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_unit_interval_unref(&tau);
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_d, setsTheDecentralisationParam)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_unit_interval_t* d = NULL;
  EXPECT_EQ(cardano_unit_interval_new(1, 10, &d), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_d(protocol_param_update, d), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* d_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_d(protocol_param_update, &d_out), CARDANO_SUCCESS);
  EXPECT_NEAR(cardano_unit_interval_to_double(d_out), 0.1, 0.01);

  // Cleanup
  cardano_unit_interval_unref(&d);
  cardano_unit_interval_unref(&d_out);
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_d, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_unit_interval_t* d = NULL;
  EXPECT_EQ(cardano_unit_interval_new(1, 10, &d), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_d(nullptr, d);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_unit_interval_unref(&d);
}

TEST(cardano_protocol_param_update_set_d, returnsErrorIfDecentralisationParamIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_d(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_d, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_unit_interval_t*         d                     = NULL;
  EXPECT_EQ(cardano_unit_interval_new(1, 10, &d), CARDANO_SUCCESS);

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_d(protocol_param_update, d), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_d(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* d_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_d(protocol_param_update, &d_out), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_unit_interval_unref(&d);
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_extra_entropy, setsTheExtraEntropyParam)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_buffer_t* extra_entropy = cardano_buffer_from_hex("1234567890", 10);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_extra_entropy(protocol_param_update, extra_entropy), CARDANO_SUCCESS);

  // Assert
  cardano_buffer_t* extra_entropy_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_extra_entropy(protocol_param_update, &extra_entropy_out), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_buffer_equals(extra_entropy, extra_entropy_out), true);

  // Cleanup
  cardano_buffer_unref(&extra_entropy);
  cardano_buffer_unref(&extra_entropy_out);
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_extra_entropy, setsEmptyExtraEntropyParam)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_buffer_t* extra_entropy = cardano_buffer_new(0);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_extra_entropy(protocol_param_update, extra_entropy), CARDANO_SUCCESS);

  // Assert
  // serialize to CBOR
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  EXPECT_EQ(cardano_protocol_param_update_to_cbor(protocol_param_update, writer), CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  // Check if the extra_entropy is empty
  EXPECT_STREQ(hex, "a10d8100");

  // Cleanup
  cardano_buffer_unref(&extra_entropy);
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_protocol_param_update_set_extra_entropy, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_buffer_t* extra_entropy = cardano_buffer_from_hex("1234567890", 10);

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_extra_entropy(nullptr, extra_entropy);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_buffer_unref(&extra_entropy);
}

TEST(cardano_protocol_param_update_set_extra_entropy, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_buffer_t*                extra_entropy         = cardano_buffer_from_hex("1234567890", 10);

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_extra_entropy(protocol_param_update, extra_entropy), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_extra_entropy(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  cardano_buffer_t* extra_entropy_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_extra_entropy(protocol_param_update, &extra_entropy_out), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_buffer_unref(&extra_entropy);
  cardano_buffer_unref(&extra_entropy_out);
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_protocol_version, setsTheProtocolVersion)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_protocol_version_t* protocol_version = NULL;
  EXPECT_EQ(cardano_protocol_version_new(1, 2, &protocol_version), CARDANO_SUCCESS);
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_protocol_version(protocol_param_update, protocol_version), CARDANO_SUCCESS);

  // Assert
  cardano_protocol_version_t* protocol_version_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_protocol_version(protocol_param_update, &protocol_version_out), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_version_get_major(protocol_version_out), 1);
  EXPECT_EQ(cardano_protocol_version_get_minor(protocol_version_out), 2);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_protocol_version_unref(&protocol_version);
  cardano_protocol_version_unref(&protocol_version_out);
}

TEST(cardano_protocol_param_update_set_protocol_version, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_protocol_version_t* protocol_version = NULL;
  EXPECT_EQ(cardano_protocol_version_new(1, 2, &protocol_version), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_protocol_version(nullptr, protocol_version);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_version_unref(&protocol_version);
}

TEST(cardano_protocol_param_update_set_protocol_version, returnsErrorIfProtocolVersionIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_protocol_version(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_protocol_version, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_protocol_version_t*      protocol_version      = NULL;
  EXPECT_EQ(cardano_protocol_version_new(1, 2, &protocol_version), CARDANO_SUCCESS);

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_protocol_version(protocol_param_update, protocol_version), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_protocol_version(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  cardano_protocol_version_t* protocol_version_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_protocol_version(protocol_param_update, &protocol_version_out), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_version_unref(&protocol_version);
  cardano_protocol_version_unref(&protocol_version_out);
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_min_pool_cost, setsTheCost)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t min_pool_cost = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_min_pool_cost(protocol_param_update, &min_pool_cost), CARDANO_SUCCESS);

  // Assert
  min_pool_cost = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_min_pool_cost(protocol_param_update, &min_pool_cost), CARDANO_SUCCESS);
  EXPECT_EQ(min_pool_cost, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_min_pool_cost, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t min_pool_cost = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_min_pool_cost(nullptr, &min_pool_cost);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_min_pool_cost, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         min_pool_cost         = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_min_pool_cost(protocol_param_update, &min_pool_cost), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_min_pool_cost(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  min_pool_cost = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_min_pool_cost(protocol_param_update, &min_pool_cost), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_min_pool_cost, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         min_pool_cost         = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_min_pool_cost(protocol_param_update, &min_pool_cost);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_ada_per_utxo_byte, setsValue)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t ada_per_utxo_byte = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_ada_per_utxo_byte(protocol_param_update, &ada_per_utxo_byte), CARDANO_SUCCESS);

  // Assert
  ada_per_utxo_byte = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_ada_per_utxo_byte(protocol_param_update, &ada_per_utxo_byte), CARDANO_SUCCESS);
  EXPECT_EQ(ada_per_utxo_byte, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_ada_per_utxo_byte, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t ada_per_utxo_byte = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_ada_per_utxo_byte(nullptr, &ada_per_utxo_byte);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_ada_per_utxo_byte, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         ada_per_utxo_byte     = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_ada_per_utxo_byte(protocol_param_update, &ada_per_utxo_byte), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_ada_per_utxo_byte(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  ada_per_utxo_byte = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_ada_per_utxo_byte(protocol_param_update, &ada_per_utxo_byte), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_ada_per_utxo_byte, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         ada_per_utxo_byte     = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_ada_per_utxo_byte(protocol_param_update, &ada_per_utxo_byte);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_cost_models, setsValue)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           cbor_reader           = cardano_cbor_reader_from_hex(COSTMDLS_CBOR, strlen(COSTMDLS_CBOR));
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_costmdls_t* cost_model = nullptr;
  EXPECT_EQ(cardano_costmdls_from_cbor(cbor_reader, &cost_model), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_cost_models(protocol_param_update, cost_model), CARDANO_SUCCESS);

  // Assert
  cardano_costmdls_t* cost_model_out = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_get_cost_models(protocol_param_update, &cost_model_out), CARDANO_SUCCESS);

  // Cleanup
  cardano_costmdls_unref(&cost_model);
  cardano_costmdls_unref(&cost_model_out);
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&cbor_reader);
}

TEST(cardano_protocol_param_update_set_cost_models, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_cbor_reader_t* cbor_reader = cardano_cbor_reader_from_hex(COSTMDLS_CBOR, strlen(COSTMDLS_CBOR));
  cardano_costmdls_t*    cost_model  = nullptr;
  EXPECT_EQ(cardano_costmdls_from_cbor(cbor_reader, &cost_model), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_cost_models(nullptr, cost_model);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_costmdls_unref(&cost_model);
  cardano_cbor_reader_unref(&cbor_reader);
}

TEST(cardano_protocol_param_update_set_cost_models, returnsErrorIfCostModelsIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_cost_models(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_cost_models, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_costmdls_t*              cost_model            = nullptr;
  cardano_cbor_reader_t*           cbor_reader           = cardano_cbor_reader_from_hex(COSTMDLS_CBOR, strlen(COSTMDLS_CBOR));
  EXPECT_EQ(cardano_costmdls_from_cbor(cbor_reader, &cost_model), CARDANO_SUCCESS);

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_cost_models(protocol_param_update, cost_model), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_cost_models(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  cardano_costmdls_t* cost_model_out = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_get_cost_models(protocol_param_update, &cost_model_out), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_costmdls_unref(&cost_model);
  cardano_costmdls_unref(&cost_model_out);
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&cbor_reader);
}

TEST(cardano_protocol_param_update_set_execution_costs, setsValue)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           cbor_reader           = cardano_cbor_reader_from_hex(EXECUTION_COSTS_CBOR, strlen(EXECUTION_COSTS_CBOR));
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_ex_unit_prices_t* execution_costs = nullptr;
  EXPECT_EQ(cardano_ex_unit_prices_from_cbor(cbor_reader, &execution_costs), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_execution_costs(protocol_param_update, execution_costs), CARDANO_SUCCESS);

  // Assert
  cardano_ex_unit_prices_t* execution_costs_out = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_get_execution_costs(protocol_param_update, &execution_costs_out), CARDANO_SUCCESS);

  // Cleanup
  cardano_ex_unit_prices_unref(&execution_costs);
  cardano_ex_unit_prices_unref(&execution_costs_out);
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&cbor_reader);
}

TEST(cardano_protocol_param_update_set_execution_costs, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_cbor_reader_t*    cbor_reader     = cardano_cbor_reader_from_hex(EXECUTION_COSTS_CBOR, strlen(EXECUTION_COSTS_CBOR));
  cardano_ex_unit_prices_t* execution_costs = nullptr;
  EXPECT_EQ(cardano_ex_unit_prices_from_cbor(cbor_reader, &execution_costs), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_execution_costs(nullptr, execution_costs);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_ex_unit_prices_unref(&execution_costs);
  cardano_cbor_reader_unref(&cbor_reader);
}

TEST(cardano_protocol_param_update_set_execution_costs, returnsErrorIfExecutionCostsIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_execution_costs(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_execution_costs, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_ex_unit_prices_t*        execution_costs       = nullptr;
  cardano_cbor_reader_t*           cbor_reader           = cardano_cbor_reader_from_hex(EXECUTION_COSTS_CBOR, strlen(EXECUTION_COSTS_CBOR));
  EXPECT_EQ(cardano_ex_unit_prices_from_cbor(cbor_reader, &execution_costs), CARDANO_SUCCESS);

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_execution_costs(protocol_param_update, execution_costs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_execution_costs(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  cardano_ex_unit_prices_t* execution_costs_out = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_get_execution_costs(protocol_param_update, &execution_costs_out), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_ex_unit_prices_unref(&execution_costs);
  cardano_ex_unit_prices_unref(&execution_costs_out);
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&cbor_reader);
}

TEST(cardano_protocol_param_update_set_max_tx_ex_units, setsValue)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_ex_units_t* max_tx_ex_units = NULL;
  EXPECT_EQ(cardano_ex_units_new(1, 2, &max_tx_ex_units), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_tx_ex_units(protocol_param_update, max_tx_ex_units), CARDANO_SUCCESS);

  // Assert
  cardano_ex_units_t* max_tx_ex_units_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_max_tx_ex_units(protocol_param_update, &max_tx_ex_units_out), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ex_units_get_memory(max_tx_ex_units_out), 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_ex_units_unref(&max_tx_ex_units);
  cardano_ex_units_unref(&max_tx_ex_units_out);
}

TEST(cardano_protocol_param_update_set_max_tx_ex_units, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_ex_units_t* max_tx_ex_units = NULL;
  EXPECT_EQ(cardano_ex_units_new(1, 2, &max_tx_ex_units), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_max_tx_ex_units(nullptr, max_tx_ex_units);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_ex_units_unref(&max_tx_ex_units);
}

TEST(cardano_protocol_param_update_set_max_tx_ex_units, returnsErrorIfMaxTxExUnitsIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_max_tx_ex_units(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_max_tx_ex_units, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_ex_units_t*              max_tx_ex_units       = NULL;
  EXPECT_EQ(cardano_ex_units_new(1, 2, &max_tx_ex_units), CARDANO_SUCCESS);

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_tx_ex_units(protocol_param_update, max_tx_ex_units), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_max_tx_ex_units(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  cardano_ex_units_t* max_tx_ex_units_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_max_tx_ex_units(protocol_param_update, &max_tx_ex_units_out), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_ex_units_unref(&max_tx_ex_units);
  cardano_ex_units_unref(&max_tx_ex_units_out);
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_block_ex_units, setsValue)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_ex_units_t* max_block_ex_units = NULL;
  EXPECT_EQ(cardano_ex_units_new(1, 2, &max_block_ex_units), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_block_ex_units(protocol_param_update, max_block_ex_units), CARDANO_SUCCESS);

  // Assert
  cardano_ex_units_t* max_block_ex_units_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_max_block_ex_units(protocol_param_update, &max_block_ex_units_out), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ex_units_get_memory(max_block_ex_units_out), 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_ex_units_unref(&max_block_ex_units);
  cardano_ex_units_unref(&max_block_ex_units_out);
}

TEST(cardano_protocol_param_update_set_max_block_ex_units, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_ex_units_t* max_block_ex_units = NULL;
  EXPECT_EQ(cardano_ex_units_new(1, 2, &max_block_ex_units), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_max_block_ex_units(nullptr, max_block_ex_units);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_ex_units_unref(&max_block_ex_units);
}

TEST(cardano_protocol_param_update_set_max_block_ex_units, returnsErrorIfMaxBlockExUnitsIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_max_block_ex_units(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_max_block_ex_units, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_ex_units_t*              max_block_ex_units    = NULL;
  EXPECT_EQ(cardano_ex_units_new(1, 2, &max_block_ex_units), CARDANO_SUCCESS);

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_block_ex_units(protocol_param_update, max_block_ex_units), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_max_block_ex_units(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  cardano_ex_units_t* max_block_ex_units_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_max_block_ex_units(protocol_param_update, &max_block_ex_units_out), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_ex_units_unref(&max_block_ex_units);
  cardano_ex_units_unref(&max_block_ex_units_out);
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_value_size, setsValue)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t max_value_size = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_value_size(protocol_param_update, &max_value_size), CARDANO_SUCCESS);

  // Assert
  max_value_size = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_value_size(protocol_param_update, &max_value_size), CARDANO_SUCCESS);
  EXPECT_EQ(max_value_size, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_value_size, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t max_value_size = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_max_value_size(nullptr, &max_value_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_max_value_size, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         max_value_size        = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_value_size(protocol_param_update, &max_value_size), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_max_value_size(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  max_value_size = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_value_size(protocol_param_update, &max_value_size), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_value_size, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         max_value_size        = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_max_value_size(protocol_param_update, &max_value_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_collateral_percentage, setsValue)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t collateral_percentage = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_collateral_percentage(protocol_param_update, &collateral_percentage), CARDANO_SUCCESS);

  // Assert
  collateral_percentage = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_collateral_percentage(protocol_param_update, &collateral_percentage), CARDANO_SUCCESS);
  EXPECT_EQ(collateral_percentage, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_collateral_percentage, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t collateral_percentage = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_collateral_percentage(nullptr, &collateral_percentage);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_collateral_percentage, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         collateral_percentage = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_collateral_percentage(protocol_param_update, &collateral_percentage), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_collateral_percentage(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  collateral_percentage = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_collateral_percentage(protocol_param_update, &collateral_percentage), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_collateral_percentage, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         collateral_percentage = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_collateral_percentage(protocol_param_update, &collateral_percentage);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_max_collateral_inputs, setsValue)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t max_collateral_inputs = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_collateral_inputs(protocol_param_update, &max_collateral_inputs), CARDANO_SUCCESS);

  // Assert
  max_collateral_inputs = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_collateral_inputs(protocol_param_update, &max_collateral_inputs), CARDANO_SUCCESS);
  EXPECT_EQ(max_collateral_inputs, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_collateral_inputs, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t max_collateral_inputs = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_max_collateral_inputs(nullptr, &max_collateral_inputs);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_max_collateral_inputs, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         max_collateral_inputs = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_collateral_inputs(protocol_param_update, &max_collateral_inputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_max_collateral_inputs(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  max_collateral_inputs = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_collateral_inputs(protocol_param_update, &max_collateral_inputs), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_collateral_inputs, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         max_collateral_inputs = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_max_collateral_inputs(protocol_param_update, &max_collateral_inputs);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_pool_voting_thresholds, setsValue)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           cbor_reader           = cardano_cbor_reader_from_hex(POOL_VOTING_THRESHOLDS_CBOR, strlen(POOL_VOTING_THRESHOLDS_CBOR));
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;
  EXPECT_EQ(cardano_pool_voting_thresholds_from_cbor(cbor_reader, &pool_voting_thresholds), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_pool_voting_thresholds(protocol_param_update, pool_voting_thresholds), CARDANO_SUCCESS);

  // Assert
  cardano_pool_voting_thresholds_t* pool_voting_thresholds_out = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_get_pool_voting_thresholds(protocol_param_update, &pool_voting_thresholds_out), CARDANO_SUCCESS);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds_out);
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&cbor_reader);
}

TEST(cardano_protocol_param_update_set_pool_voting_thresholds, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_cbor_reader_t*            cbor_reader            = cardano_cbor_reader_from_hex(POOL_VOTING_THRESHOLDS_CBOR, strlen(POOL_VOTING_THRESHOLDS_CBOR));
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;
  EXPECT_EQ(cardano_pool_voting_thresholds_from_cbor(cbor_reader, &pool_voting_thresholds), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_pool_voting_thresholds(nullptr, pool_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_cbor_reader_unref(&cbor_reader);
}

TEST(cardano_protocol_param_update_set_pool_voting_thresholds, returnsErrorIfPoolVotingThresholdsIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_pool_voting_thresholds(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_pool_voting_thresholds, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t*  protocol_param_update  = nullptr;
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            cbor_reader            = cardano_cbor_reader_from_hex(POOL_VOTING_THRESHOLDS_CBOR, strlen(POOL_VOTING_THRESHOLDS_CBOR));
  EXPECT_EQ(cardano_pool_voting_thresholds_from_cbor(cbor_reader, &pool_voting_thresholds), CARDANO_SUCCESS);

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_pool_voting_thresholds(protocol_param_update, pool_voting_thresholds), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_pool_voting_thresholds(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  cardano_pool_voting_thresholds_t* pool_voting_thresholds_out = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_get_pool_voting_thresholds(protocol_param_update, &pool_voting_thresholds_out), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds_out);
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&cbor_reader);
}

TEST(cardano_protocol_param_update_set_drep_voting_thresholds, setsValue)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           cbor_reader           = cardano_cbor_reader_from_hex(DREP_VOTING_THRESHOLDS_CBOR, strlen(DREP_VOTING_THRESHOLDS_CBOR));
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  EXPECT_EQ(cardano_drep_voting_thresholds_from_cbor(cbor_reader, &drep_voting_thresholds), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_drep_voting_thresholds(protocol_param_update, drep_voting_thresholds), CARDANO_SUCCESS);

  // Assert
  cardano_drep_voting_thresholds_t* drep_voting_thresholds_out = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_get_drep_voting_thresholds(protocol_param_update, &drep_voting_thresholds_out), CARDANO_SUCCESS);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds_out);
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&cbor_reader);
}

TEST(cardano_protocol_param_update_set_drep_voting_thresholds, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_cbor_reader_t*            cbor_reader            = cardano_cbor_reader_from_hex(DREP_VOTING_THRESHOLDS_CBOR, strlen(DREP_VOTING_THRESHOLDS_CBOR));
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  EXPECT_EQ(cardano_drep_voting_thresholds_from_cbor(cbor_reader, &drep_voting_thresholds), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_drep_voting_thresholds(nullptr, drep_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_cbor_reader_unref(&cbor_reader);
}

TEST(cardano_protocol_param_update_set_drep_voting_thresholds, returnsErrorIfDrepVotingThresholdsIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_drep_voting_thresholds(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_drep_voting_thresholds, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t*  protocol_param_update  = nullptr;
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            cbor_reader            = cardano_cbor_reader_from_hex(DREP_VOTING_THRESHOLDS_CBOR, strlen(DREP_VOTING_THRESHOLDS_CBOR));
  EXPECT_EQ(cardano_drep_voting_thresholds_from_cbor(cbor_reader, &drep_voting_thresholds), CARDANO_SUCCESS);

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_drep_voting_thresholds(protocol_param_update, drep_voting_thresholds), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_drep_voting_thresholds(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  cardano_drep_voting_thresholds_t* drep_voting_thresholds_out = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_get_drep_voting_thresholds(protocol_param_update, &drep_voting_thresholds_out), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds_out);
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&cbor_reader);
}

TEST(cardano_protocol_param_update_set_min_committee_size, setsValue)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t min_committee_size = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_min_committee_size(protocol_param_update, &min_committee_size), CARDANO_SUCCESS);

  // Assert
  min_committee_size = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_min_committee_size(protocol_param_update, &min_committee_size), CARDANO_SUCCESS);
  EXPECT_EQ(min_committee_size, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_min_committee_size, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t min_committee_size = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_min_committee_size(nullptr, &min_committee_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_min_committee_size, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         min_committee_size    = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_min_committee_size(protocol_param_update, &min_committee_size), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_min_committee_size(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  min_committee_size = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_min_committee_size(protocol_param_update, &min_committee_size), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_min_committee_size, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         min_committee_size    = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_min_committee_size(protocol_param_update, &min_committee_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_committee_term_limit, setsValue)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t committee_term_limit = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_committee_term_limit(protocol_param_update, &committee_term_limit), CARDANO_SUCCESS);

  // Assert
  committee_term_limit = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_committee_term_limit(protocol_param_update, &committee_term_limit), CARDANO_SUCCESS);
  EXPECT_EQ(committee_term_limit, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_committee_term_limit, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t committee_term_limit = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_committee_term_limit(nullptr, &committee_term_limit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_committee_term_limit, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         committee_term_limit  = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_committee_term_limit(protocol_param_update, &committee_term_limit), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_committee_term_limit(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  committee_term_limit = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_committee_term_limit(protocol_param_update, &committee_term_limit), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_committee_term_limit, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         committee_term_limit  = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_committee_term_limit(protocol_param_update, &committee_term_limit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_governance_action_validity_period, setsValue)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t governance_action_validity_period = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_governance_action_validity_period(protocol_param_update, &governance_action_validity_period), CARDANO_SUCCESS);

  // Assert
  governance_action_validity_period = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_governance_action_validity_period(protocol_param_update, &governance_action_validity_period), CARDANO_SUCCESS);
  EXPECT_EQ(governance_action_validity_period, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_governance_action_validity_period, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t governance_action_validity_period = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_governance_action_validity_period(nullptr, &governance_action_validity_period);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_governance_action_validity_period, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update             = nullptr;
  uint64_t                         governance_action_validity_period = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_governance_action_validity_period(protocol_param_update, &governance_action_validity_period), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_governance_action_validity_period(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  governance_action_validity_period = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_governance_action_validity_period(protocol_param_update, &governance_action_validity_period), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_governance_action_validity_period, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update             = nullptr;
  uint64_t                         governance_action_validity_period = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_governance_action_validity_period(protocol_param_update, &governance_action_validity_period);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_governance_action_deposit, setsValue)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t governance_action_deposit = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_governance_action_deposit(protocol_param_update, &governance_action_deposit), CARDANO_SUCCESS);

  // Assert
  governance_action_deposit = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_governance_action_deposit(protocol_param_update, &governance_action_deposit), CARDANO_SUCCESS);
  EXPECT_EQ(governance_action_deposit, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_governance_action_deposit, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t governance_action_deposit = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_governance_action_deposit(nullptr, &governance_action_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_governance_action_deposit, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update     = nullptr;
  uint64_t                         governance_action_deposit = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_governance_action_deposit(protocol_param_update, &governance_action_deposit), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_governance_action_deposit(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  governance_action_deposit = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_governance_action_deposit(protocol_param_update, &governance_action_deposit), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_governance_action_deposit, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update     = nullptr;
  uint64_t                         governance_action_deposit = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_governance_action_deposit(protocol_param_update, &governance_action_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_drep_deposit, setsValue)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t drep_deposit = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_drep_deposit(protocol_param_update, &drep_deposit), CARDANO_SUCCESS);

  // Assert
  drep_deposit = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_drep_deposit(protocol_param_update, &drep_deposit), CARDANO_SUCCESS);
  EXPECT_EQ(drep_deposit, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_drep_deposit, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t drep_deposit = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_drep_deposit(nullptr, &drep_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_drep_deposit, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         drep_deposit          = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_drep_deposit(protocol_param_update, &drep_deposit), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_drep_deposit(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  drep_deposit = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_drep_deposit(protocol_param_update, &drep_deposit), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_drep_deposit, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  uint64_t                         drep_deposit          = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_drep_deposit(protocol_param_update, &drep_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_drep_inactivity_period, setsValue)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t drep_inactivity_period = 1;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_drep_inactivity_period(protocol_param_update, &drep_inactivity_period), CARDANO_SUCCESS);

  // Assert
  drep_inactivity_period = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_drep_inactivity_period(protocol_param_update, &drep_inactivity_period), CARDANO_SUCCESS);
  EXPECT_EQ(drep_inactivity_period, 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_drep_inactivity_period, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t drep_inactivity_period = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_drep_inactivity_period(nullptr, &drep_inactivity_period);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_drep_inactivity_period, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update  = nullptr;
  uint64_t                         drep_inactivity_period = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_drep_inactivity_period(protocol_param_update, &drep_inactivity_period), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_drep_inactivity_period(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  drep_inactivity_period = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_drep_inactivity_period(protocol_param_update, &drep_inactivity_period), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_drep_inactivity_period, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update  = nullptr;
  uint64_t                         drep_inactivity_period = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_drep_inactivity_period(protocol_param_update, &drep_inactivity_period);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_ref_script_cost_per_byte, setsValue)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_unit_interval_t* ref_script_cost_per_byte = NULL;
  error                                             = cardano_unit_interval_new(1, 1, &ref_script_cost_per_byte);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_ref_script_cost_per_byte(protocol_param_update, ref_script_cost_per_byte), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* ref_script_cost_per_byte_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_ref_script_cost_per_byte(protocol_param_update, &ref_script_cost_per_byte_out), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_get_denominator(ref_script_cost_per_byte_out), 1);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_unit_interval_unref(&ref_script_cost_per_byte);
  cardano_unit_interval_unref(&ref_script_cost_per_byte_out);
}

TEST(cardano_protocol_param_update_set_ref_script_cost_per_byte, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_unit_interval_t* ref_script_cost_per_byte = NULL;
  cardano_error_t          error                    = cardano_unit_interval_new(1, 1, &ref_script_cost_per_byte);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t error_set = cardano_protocol_param_update_set_ref_script_cost_per_byte(nullptr, ref_script_cost_per_byte);

  // Assert
  EXPECT_EQ(error_set, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_unit_interval_unref(&ref_script_cost_per_byte);
}

TEST(cardano_protocol_param_update_set_ref_script_cost_per_byte, returnsErrorIfRefScriptCostPerByteIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_ref_script_cost_per_byte(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_ref_script_cost_per_byte, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update    = nullptr;
  cardano_unit_interval_t*         ref_script_cost_per_byte = NULL;
  cardano_error_t                  error                    = cardano_unit_interval_new(1, 1, &ref_script_cost_per_byte);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_ref_script_cost_per_byte(protocol_param_update, ref_script_cost_per_byte), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_ref_script_cost_per_byte(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* ref_script_cost_per_byte_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_ref_script_cost_per_byte(protocol_param_update, &ref_script_cost_per_byte_out), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_unit_interval_unref(&ref_script_cost_per_byte);
  cardano_unit_interval_unref(&ref_script_cost_per_byte_out);
}

TEST(cardano_protocol_param_update_from_cbor, roundTripsDijkstraParamsByteExactly)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_writer_t*           writer                = cardano_cbor_writer_new();
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(DIJKSTRA_PARAMS_CBOR, strlen(DIJKSTRA_PARAMS_CBOR));

  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_protocol_param_update_to_cbor(protocol_param_update, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size    = cardano_cbor_writer_get_hex_size(writer);
  char*        actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, DIJKSTRA_PARAMS_CBOR);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_writer_unref(&writer);
  cardano_cbor_reader_unref(&reader);
  free(actual_cbor);
}

TEST(cardano_protocol_param_update_from_cbor, roundTripsEachDijkstraParamByteExactly)
{
  // Arrange
  const char* vectors[] = {
    "a118221a000f4240",
    "a118231a00020000",
    "a1182419c350",
    "a11825d81e820f0a"
  };

  for (size_t i = 0U; i < (sizeof(vectors) / sizeof(vectors[0])); ++i)
  {
    cardano_protocol_param_update_t* protocol_param_update = nullptr;
    cardano_cbor_writer_t*           writer                = cardano_cbor_writer_new();
    cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(vectors[i], strlen(vectors[i]));

    // Act
    ASSERT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);
    ASSERT_EQ(cardano_protocol_param_update_to_cbor(protocol_param_update, writer), CARDANO_SUCCESS);

    // Assert
    const size_t hex_size    = cardano_cbor_writer_get_hex_size(writer);
    char*        actual_cbor = (char*)malloc(hex_size);

    ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size), CARDANO_SUCCESS);
    EXPECT_STREQ(actual_cbor, vectors[i]);

    // Cleanup
    cardano_protocol_param_update_unref(&protocol_param_update);
    cardano_cbor_writer_unref(&writer);
    cardano_cbor_reader_unref(&reader);
    free(actual_cbor);
  }
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfDijkstraUintParamExceedsUint32Max)
{
  // Arrange
  const char* vectors[] = {
    "a118221b0000000100000000",
    "a118231b0000000100000000",
    "a118241b0000000100000000"
  };

  for (size_t i = 0U; i < (sizeof(vectors) / sizeof(vectors[0])); ++i)
  {
    cardano_protocol_param_update_t* protocol_param_update = nullptr;
    cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(vectors[i], strlen(vectors[i]));

    // Act
    cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

    // Assert
    EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_VALUE);

    // Cleanup
    cardano_cbor_reader_unref(&reader);
  }
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfRefScriptCostStrideIsZero)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a1182400", strlen("a1182400"));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_VALUE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfDuplicatedDijkstraUintParam)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a2182200182200", strlen("a2182200182200"));

  // Act
  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_from_cbor, returnsErrorIfMemoryAllocationFailsWhenReadingDijkstraUintParam)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a118221a000f4240", strlen("a118221a000f4240"));

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_error_t error = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_get_max_ref_script_size_per_block, returnsTheMaxRefScriptSizePerBlock)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(DIJKSTRA_PARAMS_CBOR, strlen(DIJKSTRA_PARAMS_CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  uint64_t max_ref_script_size_per_block = 0;

  EXPECT_EQ(cardano_protocol_param_update_get_max_ref_script_size_per_block(protocol_param_update, &max_ref_script_size_per_block), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(max_ref_script_size_per_block, 4294967295U);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_max_ref_script_size_per_block, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t max_ref_script_size_per_block = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_ref_script_size_per_block(nullptr, &max_ref_script_size_per_block);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_max_ref_script_size_per_block, returnsErrorIfMaxRefScriptSizePerBlockIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_new(&protocol_param_update), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_ref_script_size_per_block(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_get_max_ref_script_size_per_tx, returnsTheMaxRefScriptSizePerTx)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(DIJKSTRA_PARAMS_CBOR, strlen(DIJKSTRA_PARAMS_CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  uint64_t max_ref_script_size_per_tx = 0;

  EXPECT_EQ(cardano_protocol_param_update_get_max_ref_script_size_per_tx(protocol_param_update, &max_ref_script_size_per_tx), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(max_ref_script_size_per_tx, 20971520U);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_max_ref_script_size_per_tx, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t max_ref_script_size_per_tx = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_ref_script_size_per_tx(nullptr, &max_ref_script_size_per_tx);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_max_ref_script_size_per_tx, returnsErrorIfMaxRefScriptSizePerTxIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_new(&protocol_param_update), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_max_ref_script_size_per_tx(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_get_ref_script_cost_stride, returnsTheRefScriptCostStride)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(DIJKSTRA_PARAMS_CBOR, strlen(DIJKSTRA_PARAMS_CBOR));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  uint64_t ref_script_cost_stride = 0;

  EXPECT_EQ(cardano_protocol_param_update_get_ref_script_cost_stride(protocol_param_update, &ref_script_cost_stride), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(ref_script_cost_stride, 25600U);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_ref_script_cost_stride, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t ref_script_cost_stride = 0;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_ref_script_cost_stride(nullptr, &ref_script_cost_stride);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_ref_script_cost_stride, returnsErrorIfRefScriptCostStrideIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_new(&protocol_param_update), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_ref_script_cost_stride(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_get_ref_script_cost_multiplier, returnsTheRefScriptCostMultiplier)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(DIJKSTRA_PARAMS_CBOR, strlen(DIJKSTRA_PARAMS_CBOR));

  cardano_unit_interval_t* ref_script_cost_multiplier = NULL;
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &protocol_param_update), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_get_ref_script_cost_multiplier(protocol_param_update, &ref_script_cost_multiplier), CARDANO_SUCCESS);

  // Assert
  EXPECT_NEAR(cardano_unit_interval_to_double(ref_script_cost_multiplier), 1.5, 0.01);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_unit_interval_unref(&ref_script_cost_multiplier);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_protocol_param_update_get_ref_script_cost_multiplier, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_unit_interval_t* ref_script_cost_multiplier = NULL;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_ref_script_cost_multiplier(nullptr, &ref_script_cost_multiplier);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_ref_script_cost_multiplier, returnsErrorIfRefScriptCostMultiplierIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_get_ref_script_cost_multiplier(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_get_, returnsElementNotFoundIfMissingDijkstraField)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  uint64_t max_ref_script_size_per_block = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_ref_script_size_per_block(protocol_param_update, &max_ref_script_size_per_block), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t max_ref_script_size_per_tx = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_ref_script_size_per_tx(protocol_param_update, &max_ref_script_size_per_tx), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  uint64_t ref_script_cost_stride = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_ref_script_cost_stride(protocol_param_update, &ref_script_cost_stride), CARDANO_ERROR_ELEMENT_NOT_FOUND);
  cardano_unit_interval_t* ref_script_cost_multiplier = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_ref_script_cost_multiplier(protocol_param_update, &ref_script_cost_multiplier), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_ref_script_size_per_block, setsTheMaxRefScriptSizePerBlock)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t max_ref_script_size_per_block = 1048576;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_ref_script_size_per_block(protocol_param_update, &max_ref_script_size_per_block), CARDANO_SUCCESS);

  // Assert
  max_ref_script_size_per_block = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_ref_script_size_per_block(protocol_param_update, &max_ref_script_size_per_block), CARDANO_SUCCESS);
  EXPECT_EQ(max_ref_script_size_per_block, 1048576);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_ref_script_size_per_block, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t max_ref_script_size_per_block = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_max_ref_script_size_per_block(nullptr, &max_ref_script_size_per_block);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_max_ref_script_size_per_block, returnsErrorIfValueExceedsUint32Max)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t max_ref_script_size_per_block = 4294967296U;

  // Act
  error = cardano_protocol_param_update_set_max_ref_script_size_per_block(protocol_param_update, &max_ref_script_size_per_block);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_ref_script_size_per_block, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update         = nullptr;
  uint64_t                         max_ref_script_size_per_block = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_ref_script_size_per_block(protocol_param_update, &max_ref_script_size_per_block), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_max_ref_script_size_per_block(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  max_ref_script_size_per_block = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_ref_script_size_per_block(protocol_param_update, &max_ref_script_size_per_block), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_ref_script_size_per_block, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update         = nullptr;
  uint64_t                         max_ref_script_size_per_block = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_max_ref_script_size_per_block(protocol_param_update, &max_ref_script_size_per_block);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_max_ref_script_size_per_tx, setsTheMaxRefScriptSizePerTx)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t max_ref_script_size_per_tx = 204800;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_ref_script_size_per_tx(protocol_param_update, &max_ref_script_size_per_tx), CARDANO_SUCCESS);

  // Assert
  max_ref_script_size_per_tx = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_ref_script_size_per_tx(protocol_param_update, &max_ref_script_size_per_tx), CARDANO_SUCCESS);
  EXPECT_EQ(max_ref_script_size_per_tx, 204800);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_ref_script_size_per_tx, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t max_ref_script_size_per_tx = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_max_ref_script_size_per_tx(nullptr, &max_ref_script_size_per_tx);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_max_ref_script_size_per_tx, returnsErrorIfValueExceedsUint32Max)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t max_ref_script_size_per_tx = 4294967296U;

  // Act
  error = cardano_protocol_param_update_set_max_ref_script_size_per_tx(protocol_param_update, &max_ref_script_size_per_tx);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_ref_script_size_per_tx, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update      = nullptr;
  uint64_t                         max_ref_script_size_per_tx = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_max_ref_script_size_per_tx(protocol_param_update, &max_ref_script_size_per_tx), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_max_ref_script_size_per_tx(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  max_ref_script_size_per_tx = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_max_ref_script_size_per_tx(protocol_param_update, &max_ref_script_size_per_tx), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_max_ref_script_size_per_tx, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update      = nullptr;
  uint64_t                         max_ref_script_size_per_tx = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_max_ref_script_size_per_tx(protocol_param_update, &max_ref_script_size_per_tx);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_ref_script_cost_stride, setsTheRefScriptCostStride)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t ref_script_cost_stride = 25600;
  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_ref_script_cost_stride(protocol_param_update, &ref_script_cost_stride), CARDANO_SUCCESS);

  // Assert
  ref_script_cost_stride = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_ref_script_cost_stride(protocol_param_update, &ref_script_cost_stride), CARDANO_SUCCESS);
  EXPECT_EQ(ref_script_cost_stride, 25600);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_ref_script_cost_stride, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  uint64_t ref_script_cost_stride = 1;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_ref_script_cost_stride(nullptr, &ref_script_cost_stride);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_ref_script_cost_stride, returnsErrorIfValueIsZero)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t ref_script_cost_stride = 0;

  // Act
  error = cardano_protocol_param_update_set_ref_script_cost_stride(protocol_param_update, &ref_script_cost_stride);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_ref_script_cost_stride, returnsErrorIfValueExceedsUint32Max)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  uint64_t ref_script_cost_stride = 4294967296U;

  // Act
  error = cardano_protocol_param_update_set_ref_script_cost_stride(protocol_param_update, &ref_script_cost_stride);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_ref_script_cost_stride, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update  = nullptr;
  uint64_t                         ref_script_cost_stride = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_ref_script_cost_stride(protocol_param_update, &ref_script_cost_stride), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_ref_script_cost_stride(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  ref_script_cost_stride = 0;
  EXPECT_EQ(cardano_protocol_param_update_get_ref_script_cost_stride(protocol_param_update, &ref_script_cost_stride), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_protocol_param_update_set_ref_script_cost_stride, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update  = nullptr;
  uint64_t                         ref_script_cost_stride = 1;

  cardano_error_t error = cardano_protocol_param_update_new(&protocol_param_update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_protocol_param_update_set_ref_script_cost_stride(protocol_param_update, &ref_script_cost_stride);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_protocol_param_update_set_ref_script_cost_multiplier, setsValue)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;
  cardano_error_t                  error                 = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_unit_interval_t* ref_script_cost_multiplier = NULL;
  error                                               = cardano_unit_interval_new(15, 10, &ref_script_cost_multiplier);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_ref_script_cost_multiplier(protocol_param_update, ref_script_cost_multiplier), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* ref_script_cost_multiplier_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_ref_script_cost_multiplier(protocol_param_update, &ref_script_cost_multiplier_out), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_get_numerator(ref_script_cost_multiplier_out), 15);
  EXPECT_EQ(cardano_unit_interval_get_denominator(ref_script_cost_multiplier_out), 10);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_unit_interval_unref(&ref_script_cost_multiplier);
  cardano_unit_interval_unref(&ref_script_cost_multiplier_out);
}

TEST(cardano_protocol_param_update_set_ref_script_cost_multiplier, returnsErrorIfProtocolParamUpdateIsNull)
{
  // Arrange
  cardano_unit_interval_t* ref_script_cost_multiplier = NULL;
  cardano_error_t          error                      = cardano_unit_interval_new(15, 10, &ref_script_cost_multiplier);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_error_t error_set = cardano_protocol_param_update_set_ref_script_cost_multiplier(nullptr, ref_script_cost_multiplier);

  // Assert
  EXPECT_EQ(error_set, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_unit_interval_unref(&ref_script_cost_multiplier);
}

TEST(cardano_protocol_param_update_set_ref_script_cost_multiplier, returnsErrorIfRefScriptCostMultiplierIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_param_update_set_ref_script_cost_multiplier(protocol_param_update, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_param_update_set_ref_script_cost_multiplier, canUnsetParameterByPassingNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update      = nullptr;
  cardano_unit_interval_t*         ref_script_cost_multiplier = NULL;
  cardano_error_t                  error                      = cardano_unit_interval_new(15, 10, &ref_script_cost_multiplier);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_protocol_param_update_new(&protocol_param_update);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_protocol_param_update_set_ref_script_cost_multiplier(protocol_param_update, ref_script_cost_multiplier), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_ref_script_cost_multiplier(protocol_param_update, nullptr), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* ref_script_cost_multiplier_out = NULL;
  EXPECT_EQ(cardano_protocol_param_update_get_ref_script_cost_multiplier(protocol_param_update, &ref_script_cost_multiplier_out), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_unit_interval_unref(&ref_script_cost_multiplier);
  cardano_unit_interval_unref(&ref_script_cost_multiplier_out);
}

TEST(cardano_protocol_param_update_to_cip116_json, canConvertDijkstraParamsToCip116Json)
{
  // Arrange
  cardano_protocol_param_update_t* update = nullptr;
  cardano_cbor_reader_t*           reader = cardano_cbor_reader_from_hex(DIJKSTRA_PARAMS_CBOR, strlen(DIJKSTRA_PARAMS_CBOR));
  cardano_error_t                  error  = cardano_protocol_param_update_from_cbor(reader, &update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Serialize
  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  error          = cardano_protocol_param_update_to_cip116_json(update, json);
  char* json_str = encode_json(json);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(json_str, R"({"max_ref_script_size_per_block":"4294967295","max_ref_script_size_per_tx":"20971520","ref_script_cost_stride":"25600","ref_script_cost_multiplier":{"numerator":"15","denominator":"10"}})");

  // Cleanup
  cardano_json_writer_unref(&json);
  cardano_cbor_reader_unref(&reader);
  cardano_protocol_param_update_unref(&update);
  free(json_str);
}

TEST(cardano_protocol_param_update_to_cip116_json, canConvertToCip116Json)
{
  // Arrange
  cardano_protocol_param_update_t* update = nullptr;
  cardano_cbor_reader_t*           reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t                  error  = cardano_protocol_param_update_from_cbor(reader, &update);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Serialize
  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  error          = cardano_protocol_param_update_to_cip116_json(update, json);
  char* json_str = encode_json(json);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(json_str, R"({"min_fee_a":"100","min_fee_b":"200","max_block_body_size":"300","max_tx_size":"400","max_block_header_size":"500","key_deposit":"2000000","pool_deposit":"200000000","max_epoch":"800","n_opt":"900","pool_pledge_influence":{"numerator":"1","denominator":"2"},"expansion_rate":{"numerator":"1","denominator":"3"},"treasury_growth_rate":{"numerator":"1","denominator":"4"},"d":{"numerator":"1","denominator":"5"},"extra_entropy":"0000000000000000000000000000000000000000000000000000000000000000","protocol_version":{"major":1,"minor":3},"min_pool_cost":"1000","ada_per_utxo_byte":"35000","cost_models":{"plutus_v1":["205665","812","1","1","1000","571","0","1","1000","24177","4","1","1000","32","117366","10475","4","23000","100","23000","100","23000","100","23000","100","23000","100","23000","100","100","100","23000","100","19537","32","175354","32","46417","4","221973","511","0","1","89141","32","497525","14068","4","2","196500","453240","220","0","1","1","1000","28662","4","2","245000","216773","62","1","1060367","12586","1","208512","421","1","187000","1000","52998","1","80436","32","43249","32","1000","32","80556","1","57667","4","1000","10","197145","156","1","197145","156","1","204924","473","1","208896","511","1","52467","32","64832","32","65493","32","22558","32","16563","32","76511","32","196500","453240","220","0","1","1","69522","11687","0","1","60091","32","196500","453240","220","0","1","1","196500","453240","220","0","1","1","806990","30482","4","1927926","82523","4","265318","0","4","0","85931","32","205665","812","1","1","41182","32","212342","32","31220","32","32696","32","43357","32","32247","32","38314","32","57996947","18975","10"],"plutus_v2":["205665","812","1","1","1000","571","0","1","1000","24177","4","1","1000","32","117366","10475","4","23000","100","23000","100","23000","100","23000","100","23000","100","23000","100","100","100","23000","100","19537","32","175354","32","46417","4","221973","511","0","1","89141","32","497525","14068","4","2","196500","453240","220","0","1","1","1000","28662","4","2","245000","216773","62","1","1060367","12586","1","208512","421","1","187000","1000","52998","1","80436","32","43249","32","1000","32","80556","1","57667","4","1000","10","197145","156","1","197145","156","1","204924","473","1","208896","511","1","52467","32","64832","32","65493","32","22558","32","16563","32","76511","32","196500","453240","220","0","1","1","69522","11687","0","1","60091","32","196500","453240","220","0","1","1","196500","453240","220","0","1","1","1159724","392670","0","2","806990","30482","4","1927926","82523","4","265318","0","4","0","85931","32","205665","812","1","1","41182","32","212342","32","31220","32","32696","32","43357","32","32247","32","38314","32","35892428","10","57996947","18975","10","38887044","32947","10"]},"execution_costs":{"mem_price":{"numerator":"1","denominator":"2"},"step_price":{"numerator":"1","denominator":"2"}},"max_tx_ex_units":{"mem":"4294967296","steps":"4294967296"},"max_block_ex_units":{"mem":"4294967296","steps":"4294967296"},"max_value_size":"954","collateral_percentage":"852","max_collateral_inputs":"100","pool_voting_thresholds":{"motion_no_confidence":{"numerator":"0","denominator":"0"},"committee_normal":{"numerator":"1","denominator":"1"},"committee_no_confidence":{"numerator":"2","denominator":"2"},"hard_fork_initiation":{"numerator":"3","denominator":"3"},"security_relevant_param":{"numerator":"1","denominator":"1"}},"drep_voting_thresholds":{"motion_no_confidence":{"numerator":"0","denominator":"0"},"committee_normal":{"numerator":"1","denominator":"1"},"committee_no_confidence":{"numerator":"2","denominator":"2"},"update_constitution":{"numerator":"3","denominator":"3"},"hard_fork_initiation":{"numerator":"4","denominator":"4"},"pp_network_group":{"numerator":"5","denominator":"5"},"pp_economic_group":{"numerator":"6","denominator":"6"},"pp_technical_group":{"numerator":"7","denominator":"7"},"pp_gov_group":{"numerator":"8","denominator":"8"},"treasury_withdrawal":{"numerator":"9","denominator":"9"}},"min_committee_size":"100","committee_term_limit":"200","governance_action_validity_period":"300","governance_action_deposit":"1000","drep_deposit":"2000","drep_inactivity_period":"5000","ref_script_cost_per_byte":{"numerator":"89","denominator":"2"}})");

  // Cleanup
  cardano_json_writer_unref(&json);
  cardano_cbor_reader_unref(&reader);
  cardano_protocol_param_update_unref(&update);
  free(json_str);
}

TEST(cardano_protocol_param_update_to_cip116_json, canConvertEmptyUpdate)
{
  // Arrange
  cardano_protocol_param_update_t* update = NULL;
  EXPECT_EQ(cardano_protocol_param_update_new(&update), CARDANO_SUCCESS);

  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  cardano_error_t error    = cardano_protocol_param_update_to_cip116_json(update, json);
  char*           json_str = encode_json(json);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(json_str, "{}");

  // Cleanup
  cardano_json_writer_unref(&json);
  cardano_protocol_param_update_unref(&update);
  free(json_str);
}

TEST(cardano_protocol_param_update_to_cip116_json, returnsErrorIfUpdateIsNull)
{
  cardano_json_writer_t* json  = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);
  cardano_error_t        error = cardano_protocol_param_update_to_cip116_json(nullptr, json);
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_json_writer_unref(&json);
}

TEST(cardano_protocol_param_update_to_cip116_json, returnsErrorIfWriterIsNull)
{
  cardano_protocol_param_update_t* update = NULL;
  EXPECT_EQ(cardano_protocol_param_update_new(&update), CARDANO_SUCCESS);
  cardano_error_t error = cardano_protocol_param_update_to_cip116_json(update, nullptr);
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_protocol_param_update_unref(&update);
}

/* PLUTUS-DATA ENCODING ******************************************************/

// Reads the integer value of a plutus-data integer as int64.
static int64_t
pd_to_i64(cardano_plutus_data_t* pd)
{
  cardano_bigint_t* big = nullptr;
  EXPECT_EQ(cardano_plutus_data_to_integer(pd, &big), CARDANO_SUCCESS);
  const int64_t v = cardano_bigint_to_int(big);
  cardano_bigint_unref(&big);
  return v;
}

// Looks up the value for an integer key (tag) in a plutus-data map.
static cardano_plutus_data_t*
pd_map_get_tag(cardano_plutus_data_t* map_pd, uint64_t tag)
{
  cardano_plutus_map_t* map = nullptr;
  EXPECT_EQ(cardano_plutus_data_to_map(map_pd, &map), CARDANO_SUCCESS);

  cardano_plutus_data_t* key = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_integer_from_uint(tag, &key), CARDANO_SUCCESS);

  cardano_plutus_data_t* value = nullptr;
  EXPECT_EQ(cardano_plutus_map_get(map, key, &value), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&key);
  cardano_plutus_map_unref(&map);
  return value;
}

// Returns true if a plutus-data map contains an entry for the given integer key (tag).
static bool
pd_map_has_tag(cardano_plutus_data_t* map_pd, uint64_t tag)
{
  cardano_plutus_map_t* map = nullptr;
  EXPECT_EQ(cardano_plutus_data_to_map(map_pd, &map), CARDANO_SUCCESS);

  cardano_plutus_data_t* key = nullptr;
  EXPECT_EQ(cardano_plutus_data_new_integer_from_uint(tag, &key), CARDANO_SUCCESS);

  cardano_plutus_data_t* value  = nullptr;
  const bool             exists = cardano_plutus_map_get(map, key, &value) == CARDANO_SUCCESS;

  cardano_plutus_data_unref(&value);
  cardano_plutus_data_unref(&key);
  cardano_plutus_map_unref(&map);
  return exists;
}

// Returns the index-th integer of a plutus-data list.
static int64_t
pd_list_i64(cardano_plutus_data_t* list_pd, size_t index)
{
  cardano_plutus_list_t* list = nullptr;
  EXPECT_EQ(cardano_plutus_data_to_list(list_pd, &list), CARDANO_SUCCESS);
  cardano_plutus_data_t* item = nullptr;
  EXPECT_EQ(cardano_plutus_list_get(list, index, &item), CARDANO_SUCCESS);
  const int64_t v = pd_to_i64(item);
  cardano_plutus_data_unref(&item);
  cardano_plutus_list_unref(&list);
  return v;
}

TEST(cardano_protocol_param_update_to_plutus_data, encodesTagsValuesAndReducesRationals)
{
  cardano_protocol_param_update_t* update = nullptr;
  ASSERT_EQ(cardano_protocol_param_update_new(&update), CARDANO_SUCCESS);

  const uint64_t min_fee_a = 44U;
  ASSERT_EQ(cardano_protocol_param_update_set_min_fee_a(update, &min_fee_a), CARDANO_SUCCESS);

  cardano_unit_interval_t* pledge = nullptr;
  ASSERT_EQ(cardano_unit_interval_new(3U, 10U, &pledge), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_pool_pledge_influence(update, pledge), CARDANO_SUCCESS);

  cardano_unit_interval_t* tau = nullptr; // 2/10 must reduce to 1/5
  ASSERT_EQ(cardano_unit_interval_new(2U, 10U, &tau), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_treasury_growth_rate(update, tau), CARDANO_SUCCESS);

  cardano_ex_units_t* ex = nullptr;
  ASSERT_EQ(cardano_ex_units_new(1000U, 2000U, &ex), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_max_tx_ex_units(update, ex), CARDANO_SUCCESS);

  cardano_plutus_data_t* pd = nullptr;
  ASSERT_EQ(cardano_protocol_param_update_to_plutus_data(update, &pd), CARDANO_SUCCESS);

  cardano_plutus_map_t* map = nullptr;
  ASSERT_EQ(cardano_plutus_data_to_map(pd, &map), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_map_get_length(map), 4U);
  cardano_plutus_map_unref(&map);

  // tag 0: min_fee_a -> I 44
  cardano_plutus_data_t* fee = pd_map_get_tag(pd, 0U);
  EXPECT_EQ(pd_to_i64(fee), 44);
  cardano_plutus_data_unref(&fee);

  // tag 9: pool_pledge_influence -> [3, 10]
  cardano_plutus_data_t* a0 = pd_map_get_tag(pd, 9U);
  EXPECT_EQ(pd_list_i64(a0, 0U), 3);
  EXPECT_EQ(pd_list_i64(a0, 1U), 10);
  cardano_plutus_data_unref(&a0);

  // tag 11: treasury_growth_rate 2/10 -> reduced [1, 5]
  cardano_plutus_data_t* reduced = pd_map_get_tag(pd, 11U);
  EXPECT_EQ(pd_list_i64(reduced, 0U), 1);
  EXPECT_EQ(pd_list_i64(reduced, 1U), 5);
  cardano_plutus_data_unref(&reduced);

  // tag 20: max_tx_ex_units -> [mem, steps] = [1000, 2000]
  cardano_plutus_data_t* units = pd_map_get_tag(pd, 20U);
  EXPECT_EQ(pd_list_i64(units, 0U), 1000);
  EXPECT_EQ(pd_list_i64(units, 1U), 2000);
  cardano_plutus_data_unref(&units);

  cardano_plutus_data_unref(&pd);
  cardano_ex_units_unref(&ex);
  cardano_unit_interval_unref(&tau);
  cardano_unit_interval_unref(&pledge);
  cardano_protocol_param_update_unref(&update);
}

TEST(cardano_protocol_param_update_to_plutus_data, encodesAFullUpdateWithEveryParamType)
{
  cardano_cbor_reader_t*           reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_protocol_param_update_t* update = nullptr;
  ASSERT_EQ(cardano_protocol_param_update_from_cbor(reader, &update), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  cardano_plutus_data_t* pd = nullptr;
  ASSERT_EQ(cardano_protocol_param_update_to_plutus_data(update, &pd), CARDANO_SUCCESS);

  cardano_plutus_map_t* map = nullptr;
  ASSERT_EQ(cardano_plutus_data_to_map(pd, &map), CARDANO_SUCCESS);
  EXPECT_GT(cardano_plutus_map_get_length(map), 25U);
  cardano_plutus_map_unref(&map);

  // cost models (tag 18): the fixture sets V1 and V2 only, so the map has exactly two entries.
  cardano_plutus_data_t* cost_models = pd_map_get_tag(pd, 18U);
  cardano_plutus_map_t*  cm_map      = nullptr;
  ASSERT_EQ(cardano_plutus_data_to_map(cost_models, &cm_map), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_map_get_length(cm_map), 2U);

  cardano_plutus_map_unref(&cm_map);
  cardano_plutus_data_unref(&cost_models);
  cardano_plutus_data_unref(&pd);
  cardano_protocol_param_update_unref(&update);
}

TEST(cardano_protocol_param_update_to_plutus_data, encodesThePlutusV4CostModel)
{
  cardano_protocol_param_update_t* update = nullptr;
  ASSERT_EQ(cardano_protocol_param_update_new(&update), CARDANO_SUCCESS);

  const int64_t v3_costs[] = { 1, 2, 3 };
  const int64_t v4_costs[] = { 4, 5, 6 };

  cardano_cost_model_t* v3_model = nullptr;
  cardano_cost_model_t* v4_model = nullptr;
  ASSERT_EQ(cardano_cost_model_new(CARDANO_PLUTUS_LANGUAGE_VERSION_V3, v3_costs, 3U, &v3_model), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_cost_model_new(CARDANO_PLUTUS_LANGUAGE_VERSION_V4, v4_costs, 3U, &v4_model), CARDANO_SUCCESS);

  cardano_costmdls_t* costmdls = nullptr;
  ASSERT_EQ(cardano_costmdls_new(&costmdls), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_costmdls_insert(costmdls, v3_model), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_costmdls_insert(costmdls, v4_model), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_cost_models(update, costmdls), CARDANO_SUCCESS);

  cardano_plutus_data_t* pd = nullptr;
  ASSERT_EQ(cardano_protocol_param_update_to_plutus_data(update, &pd), CARDANO_SUCCESS);

  cardano_plutus_data_t* cost_models = pd_map_get_tag(pd, 18U);

  // only the two present languages are encoded.
  cardano_plutus_map_t* cm_map = nullptr;
  ASSERT_EQ(cardano_plutus_data_to_map(cost_models, &cm_map), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_map_get_length(cm_map), 2U);
  cardano_plutus_map_unref(&cm_map);

  EXPECT_FALSE(pd_map_has_tag(cost_models, 0U));
  EXPECT_FALSE(pd_map_has_tag(cost_models, 1U));

  // language 2 (PlutusV3) -> [1, 2, 3]
  cardano_plutus_data_t* v3_list = pd_map_get_tag(cost_models, 2U);
  EXPECT_EQ(pd_list_i64(v3_list, 0U), 1);
  EXPECT_EQ(pd_list_i64(v3_list, 1U), 2);
  EXPECT_EQ(pd_list_i64(v3_list, 2U), 3);
  cardano_plutus_data_unref(&v3_list);

  // language 3 (PlutusV4) -> [4, 5, 6]
  cardano_plutus_data_t* v4_list = pd_map_get_tag(cost_models, 3U);
  EXPECT_EQ(pd_list_i64(v4_list, 0U), 4);
  EXPECT_EQ(pd_list_i64(v4_list, 1U), 5);
  EXPECT_EQ(pd_list_i64(v4_list, 2U), 6);
  cardano_plutus_data_unref(&v4_list);

  cardano_plutus_data_unref(&cost_models);
  cardano_plutus_data_unref(&pd);
  cardano_costmdls_unref(&costmdls);
  cardano_cost_model_unref(&v3_model);
  cardano_cost_model_unref(&v4_model);
  cardano_protocol_param_update_unref(&update);
}

TEST(cardano_protocol_param_update_to_plutus_data, omitsAbsentCostModelLanguages)
{
  cardano_protocol_param_update_t* update = nullptr;
  ASSERT_EQ(cardano_protocol_param_update_new(&update), CARDANO_SUCCESS);

  const int64_t v1_costs[] = { 1, 2, 3 };

  cardano_cost_model_t* v1_model = nullptr;
  ASSERT_EQ(cardano_cost_model_new(CARDANO_PLUTUS_LANGUAGE_VERSION_V1, v1_costs, 3U, &v1_model), CARDANO_SUCCESS);

  cardano_costmdls_t* costmdls = nullptr;
  ASSERT_EQ(cardano_costmdls_new(&costmdls), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_costmdls_insert(costmdls, v1_model), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_cost_models(update, costmdls), CARDANO_SUCCESS);

  cardano_plutus_data_t* pd = nullptr;
  ASSERT_EQ(cardano_protocol_param_update_to_plutus_data(update, &pd), CARDANO_SUCCESS);

  cardano_plutus_data_t* cost_models = pd_map_get_tag(pd, 18U);

  // only PlutusV1 is present, so the map has exactly one entry.
  cardano_plutus_map_t* cm_map = nullptr;
  ASSERT_EQ(cardano_plutus_data_to_map(cost_models, &cm_map), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_map_get_length(cm_map), 1U);
  cardano_plutus_map_unref(&cm_map);

  // language 0 (PlutusV1) -> [1, 2, 3]
  cardano_plutus_data_t* v1_list = pd_map_get_tag(cost_models, 0U);
  EXPECT_EQ(pd_list_i64(v1_list, 0U), 1);
  EXPECT_EQ(pd_list_i64(v1_list, 1U), 2);
  EXPECT_EQ(pd_list_i64(v1_list, 2U), 3);
  cardano_plutus_data_unref(&v1_list);

  EXPECT_FALSE(pd_map_has_tag(cost_models, 1U));
  EXPECT_FALSE(pd_map_has_tag(cost_models, 2U));
  EXPECT_FALSE(pd_map_has_tag(cost_models, 3U));

  cardano_plutus_data_unref(&cost_models);
  cardano_plutus_data_unref(&pd);
  cardano_costmdls_unref(&costmdls);
  cardano_cost_model_unref(&v1_model);
  cardano_protocol_param_update_unref(&update);
}

TEST(cardano_protocol_param_update_to_plutus_data, encodesTheDijkstraRefScriptParams)
{
  cardano_cbor_reader_t*           reader = cardano_cbor_reader_from_hex(DIJKSTRA_PARAMS_CBOR, strlen(DIJKSTRA_PARAMS_CBOR));
  cardano_protocol_param_update_t* update = nullptr;
  ASSERT_EQ(cardano_protocol_param_update_from_cbor(reader, &update), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  cardano_plutus_data_t* pd = nullptr;
  ASSERT_EQ(cardano_protocol_param_update_to_plutus_data(update, &pd), CARDANO_SUCCESS);

  cardano_plutus_map_t* map = nullptr;
  ASSERT_EQ(cardano_plutus_data_to_map(pd, &map), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_map_get_length(map), 4U);
  cardano_plutus_map_unref(&map);

  // tag 34: max_ref_script_size_per_block -> I 4294967295
  cardano_plutus_data_t* per_block = pd_map_get_tag(pd, 34U);
  EXPECT_EQ(pd_to_i64(per_block), 4294967295);
  cardano_plutus_data_unref(&per_block);

  // tag 35: max_ref_script_size_per_tx -> I 20971520
  cardano_plutus_data_t* per_tx = pd_map_get_tag(pd, 35U);
  EXPECT_EQ(pd_to_i64(per_tx), 20971520);
  cardano_plutus_data_unref(&per_tx);

  // tag 36: ref_script_cost_stride -> I 25600
  cardano_plutus_data_t* stride = pd_map_get_tag(pd, 36U);
  EXPECT_EQ(pd_to_i64(stride), 25600);
  cardano_plutus_data_unref(&stride);

  // tag 37: ref_script_cost_multiplier 15/10 -> reduced [3, 2]
  cardano_plutus_data_t* multiplier = pd_map_get_tag(pd, 37U);
  EXPECT_EQ(pd_list_i64(multiplier, 0U), 3);
  EXPECT_EQ(pd_list_i64(multiplier, 1U), 2);
  cardano_plutus_data_unref(&multiplier);

  cardano_plutus_data_unref(&pd);
  cardano_protocol_param_update_unref(&update);
}
