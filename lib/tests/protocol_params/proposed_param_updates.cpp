/**
 * \file proposed_param_updates.cpp
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

#include <cardano/plutus_data/plutus_data.h>
#include <cardano/protocol_params/proposed_param_updates.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR = "a3581c00000000000000000000000000000000000000000000000000000001b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000002b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000003b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba";

/* UNIT TESTS ****************************************************************/

TEST(cardano_proposed_param_updates_new, canCreateProposedParamUpdates)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;

  // Act
  cardano_error_t error = cardano_proposed_param_updates_new(&proposed_param_updates);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(proposed_param_updates, testing::Not((cardano_proposed_param_updates_t*)nullptr));

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
}

TEST(cardano_proposed_param_updates_new, returnsErrorIfProposedParamUpdatesIsNull)
{
  // Act
  cardano_error_t error = cardano_proposed_param_updates_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposed_param_updates_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;

  // Act
  cardano_error_t error = cardano_proposed_param_updates_new(&proposed_param_updates);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(proposed_param_updates, (cardano_proposed_param_updates_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_proposed_param_updates_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;

  // Act
  cardano_error_t error = cardano_proposed_param_updates_new(&proposed_param_updates);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(proposed_param_updates, (cardano_proposed_param_updates_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_proposed_param_updates_to_cbor, canSerializeAnEmptyProposedParamUpdates)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_cbor_writer_t*            writer                 = cardano_cbor_writer_new();

  cardano_error_t error = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_proposed_param_updates_to_cbor(proposed_param_updates, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 3);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "a0");

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_proposed_param_updates_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_proposed_param_updates_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_proposed_param_updates_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;

  cardano_error_t error = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_proposed_param_updates_to_cbor(proposed_param_updates, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
}

TEST(cardano_proposed_param_updates_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t*            writer                 = cardano_cbor_writer_new();

  cardano_error_t error = cardano_proposed_param_updates_from_cbor(reader, &proposed_param_updates);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_proposed_param_updates_to_cbor(proposed_param_updates, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_proposed_param_updates_from_cbor, returnErrorIfProposedParamUpdatesIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_proposed_param_updates_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposed_param_updates_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;

  // Act
  cardano_error_t error = cardano_proposed_param_updates_from_cbor(nullptr, &proposed_param_updates);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposed_param_updates_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_proposed_param_updates_from_cbor(reader, &proposed_param_updates);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(proposed_param_updates, (cardano_proposed_param_updates_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposed_param_updates_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_proposed_param_updates_t* list   = nullptr;
  cardano_cbor_reader_t*            reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_proposed_param_updates_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposed_param_updates_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_error_t                   error                  = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_proposed_param_updates_ref(proposed_param_updates);

  // Assert
  EXPECT_THAT(proposed_param_updates, testing::Not((cardano_proposed_param_updates_t*)nullptr));
  EXPECT_EQ(cardano_proposed_param_updates_refcount(proposed_param_updates), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_proposed_param_updates_unref(&proposed_param_updates);
  cardano_proposed_param_updates_unref(&proposed_param_updates);
}

TEST(cardano_proposed_param_updates_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_proposed_param_updates_ref(nullptr);
}

TEST(cardano_proposed_param_updates_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;

  // Act
  cardano_proposed_param_updates_unref(&proposed_param_updates);
}

TEST(cardano_proposed_param_updates_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_proposed_param_updates_unref((cardano_proposed_param_updates_t**)nullptr);
}

TEST(cardano_proposed_param_updates_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_error_t                   error                  = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_proposed_param_updates_ref(proposed_param_updates);
  size_t ref_count = cardano_proposed_param_updates_refcount(proposed_param_updates);

  cardano_proposed_param_updates_unref(&proposed_param_updates);
  size_t updated_ref_count = cardano_proposed_param_updates_refcount(proposed_param_updates);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
}

TEST(cardano_proposed_param_updates_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_error_t                   error                  = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_proposed_param_updates_ref(proposed_param_updates);
  size_t ref_count = cardano_proposed_param_updates_refcount(proposed_param_updates);

  cardano_proposed_param_updates_unref(&proposed_param_updates);
  size_t updated_ref_count = cardano_proposed_param_updates_refcount(proposed_param_updates);

  cardano_proposed_param_updates_unref(&proposed_param_updates);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(proposed_param_updates, (cardano_proposed_param_updates_t*)nullptr);

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
}

TEST(cardano_proposed_param_updates_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_proposed_param_updates_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_proposed_param_updates_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  const char*                       message                = "This is a test message";

  // Act
  cardano_proposed_param_updates_set_last_error(proposed_param_updates, message);

  // Assert
  EXPECT_STREQ(cardano_proposed_param_updates_get_last_error(proposed_param_updates), "Object is NULL.");
}

TEST(cardano_proposed_param_updates_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_error_t                   error                  = cardano_proposed_param_updates_new(&proposed_param_updates);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_proposed_param_updates_set_last_error(proposed_param_updates, message);

  // Assert
  EXPECT_STREQ(cardano_proposed_param_updates_get_last_error(proposed_param_updates), "");

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
}

TEST(cardano_proposed_param_updates_from_cbor, returnErrorIfInvalidMap)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("a100", 4);

  // Act
  cardano_error_t error = cardano_proposed_param_updates_from_cbor(reader, &proposed_param_updates);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposed_param_updates_from_cbor, returnErrorIfInvalidProtocolParameters)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("a3581c0000000000000000000000000000000000000000000000000000000fe60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000002b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000003b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba", strlen("a3581c0000000000000000000000000000000000000000000000000000000fe60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000002b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000003b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba"));

  // Act
  cardano_error_t error = cardano_proposed_param_updates_from_cbor(reader, &proposed_param_updates);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposed_param_updates_get_size, returnsZeroIfObjectIsNull)
{
  // Act
  size_t size = cardano_proposed_param_updates_get_size(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_proposed_param_updates_get_size, returnsTheNumberOfElementsInTheList)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_error_t                   error                  = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_protocol_param_update_t* update = nullptr;
  error                                   = cardano_protocol_param_update_new(&update);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  const char*             test = "test";
  cardano_blake2b_hash_t* hash = NULL;

  error = cardano_blake2b_compute_hash((uint8_t*)test, strlen(test), 32, &hash);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_proposed_param_updates_insert(proposed_param_updates, hash, update);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t size = cardano_proposed_param_updates_get_size(proposed_param_updates);

  // Assert
  EXPECT_EQ(size, 1);

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
  cardano_blake2b_hash_unref(&hash);
  cardano_protocol_param_update_unref(&update);
}

TEST(cardano_proposed_param_updates_insert, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* update = nullptr;
  cardano_blake2b_hash_t*          hash   = nullptr;

  // Act
  cardano_error_t error = cardano_proposed_param_updates_insert(nullptr, hash, update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposed_param_updates_insert, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_error_t                   error                  = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_protocol_param_update_t* update = nullptr;

  // Act
  error = cardano_proposed_param_updates_insert(proposed_param_updates, nullptr, update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
}

TEST(cardano_proposed_param_updates_insert, returnsErrorIfUpdateIsNull)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_error_t                   error                  = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  const char*             test = "test";
  cardano_blake2b_hash_t* hash = NULL;

  error = cardano_blake2b_compute_hash((uint8_t*)test, strlen(test), 32, &hash);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_proposed_param_updates_insert(proposed_param_updates, hash, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_proposed_param_updates_insert, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_error_t                   error                  = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  const char*             test = "test";
  cardano_blake2b_hash_t* hash = NULL;

  error = cardano_blake2b_compute_hash((uint8_t*)test, strlen(test), 32, &hash);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_protocol_param_update_t* update = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_proposed_param_updates_insert(proposed_param_updates, hash, (cardano_protocol_param_update_t*)"");

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
  cardano_blake2b_hash_unref(&hash);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_proposed_param_updates_insert, keepsElementsSortedByHash)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_error_t                   error                  = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_protocol_param_update_t* update = nullptr;
  error                                   = cardano_protocol_param_update_new(&update);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* hash1 = NULL;
  cardano_blake2b_hash_t* hash2 = NULL;
  cardano_blake2b_hash_t* hash3 = NULL;

  error = cardano_blake2b_hash_from_hex("00000000000000000000000000000000000000000000000000000001", strlen("00000000000000000000000000000000000000000000000000000001"), &hash1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_blake2b_hash_from_hex("00000000000000000000000000000000000000000000000000000002", strlen("00000000000000000000000000000000000000000000000000000002"), &hash2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_blake2b_hash_from_hex("00000000000000000000000000000000000000000000000000000003", strlen("00000000000000000000000000000000000000000000000000000003"), &hash3);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_proposed_param_updates_insert(proposed_param_updates, hash3, update);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_proposed_param_updates_insert(proposed_param_updates, hash2, update);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_proposed_param_updates_insert(proposed_param_updates, hash1, update);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  size_t size = cardano_proposed_param_updates_get_size(proposed_param_updates);

  EXPECT_EQ(size, 3);

  cardano_blake2b_hash_t* hash1_out = NULL;
  cardano_blake2b_hash_t* hash2_out = NULL;
  cardano_blake2b_hash_t* hash3_out = NULL;

  EXPECT_EQ(cardano_proposed_param_updates_get_key_at(proposed_param_updates, 0, &hash1_out), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_proposed_param_updates_get_key_at(proposed_param_updates, 1, &hash2_out), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_proposed_param_updates_get_key_at(proposed_param_updates, 2, &hash3_out), CARDANO_SUCCESS);

  EXPECT_TRUE(cardano_blake2b_hash_equals(hash1, hash1_out));
  EXPECT_TRUE(cardano_blake2b_hash_equals(hash2, hash2_out));
  EXPECT_TRUE(cardano_blake2b_hash_equals(hash3, hash3_out));

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
  cardano_blake2b_hash_unref(&hash1);
  cardano_blake2b_hash_unref(&hash2);
  cardano_blake2b_hash_unref(&hash3);
  cardano_blake2b_hash_unref(&hash1_out);
  cardano_blake2b_hash_unref(&hash2_out);
  cardano_blake2b_hash_unref(&hash3_out);
  cardano_protocol_param_update_unref(&update);
}

TEST(cardano_proposed_param_updates_get, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* update = nullptr;
  cardano_blake2b_hash_t*          hash   = nullptr;

  // Act
  cardano_error_t error = cardano_proposed_param_updates_get(nullptr, hash, &update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposed_param_updates_get, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_error_t                   error                  = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_protocol_param_update_t* update = nullptr;

  // Act
  error = cardano_proposed_param_updates_get(proposed_param_updates, nullptr, &update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
}

TEST(cardano_proposed_param_updates_get, returnsErrorIfUpdateIsNull)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_error_t                   error                  = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  const char*             test = "test";
  cardano_blake2b_hash_t* hash = NULL;

  error = cardano_blake2b_compute_hash((uint8_t*)test, strlen(test), 32, &hash);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_proposed_param_updates_get(proposed_param_updates, hash, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_proposed_param_updates_get, returnsErrorIfElementNotFound)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_error_t                   error                  = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  const char*             test = "test";
  cardano_blake2b_hash_t* hash = NULL;

  error = cardano_blake2b_compute_hash((uint8_t*)test, strlen(test), 32, &hash);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_protocol_param_update_t* update = nullptr;

  // Act
  error = cardano_proposed_param_updates_get(proposed_param_updates, hash, &update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_proposed_param_updates_get, returnsTheElement)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_error_t                   error                  = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_protocol_param_update_t* update = nullptr;
  error                                   = cardano_protocol_param_update_new(&update);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  const char*             test = "test";
  cardano_blake2b_hash_t* hash = NULL;

  error = cardano_blake2b_compute_hash((uint8_t*)test, strlen(test), 32, &hash);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_proposed_param_updates_insert(proposed_param_updates, hash, update);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_protocol_param_update_t* update_out = nullptr;
  error                                       = cardano_proposed_param_updates_get(proposed_param_updates, hash, &update_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(update, update_out);

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
  cardano_blake2b_hash_unref(&hash);
  cardano_protocol_param_update_unref(&update);
  cardano_protocol_param_update_unref(&update_out);
}

TEST(cardano_proposed_param_updates_get, returnsTheRightElementIfMoreThanOne)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_error_t                   error                  = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_protocol_param_update_t* update1 = nullptr;
  error                                    = cardano_protocol_param_update_new(&update1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_protocol_param_update_t* update2 = nullptr;
  error                                    = cardano_protocol_param_update_new(&update2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  const char*             test1 = "test1";
  cardano_blake2b_hash_t* hash1 = NULL;

  error = cardano_blake2b_compute_hash((uint8_t*)test1, strlen(test1), 32, &hash1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  const char*             test2 = "test2";
  cardano_blake2b_hash_t* hash2 = NULL;

  error = cardano_blake2b_compute_hash((uint8_t*)test2, strlen(test2), 32, &hash2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_proposed_param_updates_insert(proposed_param_updates, hash1, update1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_proposed_param_updates_insert(proposed_param_updates, hash2, update2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_protocol_param_update_t* update_out = nullptr;
  error                                       = cardano_proposed_param_updates_get(proposed_param_updates, hash1, &update_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(update1, update_out);

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
  cardano_blake2b_hash_unref(&hash1);
  cardano_blake2b_hash_unref(&hash2);
  cardano_protocol_param_update_unref(&update1);
  cardano_protocol_param_update_unref(&update2);
  cardano_protocol_param_update_unref(&update_out);
}

TEST(cardano_proposed_param_updates_get_key_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  // Act
  cardano_error_t error = cardano_proposed_param_updates_get_key_at(nullptr, 0, &hash);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposed_param_updates_get_key_at, returnsErrorIfOutIsNull)
{
  // Act
  cardano_error_t error = cardano_proposed_param_updates_get_key_at((cardano_proposed_param_updates_t*)"", 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposed_param_updates_get_key_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_error_t                   error                  = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* hash = nullptr;

  // Act
  error = cardano_proposed_param_updates_get_key_at(proposed_param_updates, 0, &hash);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
}

TEST(cardano_proposed_param_updates_get_value_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* update = nullptr;

  // Act
  cardano_error_t error = cardano_proposed_param_updates_get_value_at(nullptr, 0, &update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposed_param_updates_get_value_at, returnsErrorIfOutIsNull)
{
  // Act
  cardano_error_t error = cardano_proposed_param_updates_get_value_at((cardano_proposed_param_updates_t*)"", 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposed_param_updates_get_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_error_t                   error                  = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_protocol_param_update_t* update = nullptr;

  // Act
  error = cardano_proposed_param_updates_get_value_at(proposed_param_updates, 0, &update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
}

TEST(cardano_proposed_param_updates_get_value_at, returnsTheElement)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_error_t                   error                  = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_protocol_param_update_t* update = nullptr;
  error                                   = cardano_protocol_param_update_new(&update);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  const char*             test = "test";
  cardano_blake2b_hash_t* hash = NULL;

  error = cardano_blake2b_compute_hash((uint8_t*)test, strlen(test), 32, &hash);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_proposed_param_updates_insert(proposed_param_updates, hash, update);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_protocol_param_update_t* update_out = nullptr;
  error                                       = cardano_proposed_param_updates_get_value_at(proposed_param_updates, 0, &update_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(update, update_out);

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
  cardano_blake2b_hash_unref(&hash);
  cardano_protocol_param_update_unref(&update);
  cardano_protocol_param_update_unref(&update_out);
}

TEST(cardano_proposed_param_updates_get_key_value_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_blake2b_hash_t*          hash   = nullptr;
  cardano_protocol_param_update_t* update = nullptr;

  // Act
  cardano_error_t error = cardano_proposed_param_updates_get_key_value_at(nullptr, 0, &hash, &update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposed_param_updates_get_key_value_at, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* update = nullptr;

  // Act
  cardano_error_t error = cardano_proposed_param_updates_get_key_value_at((cardano_proposed_param_updates_t*)"", 0, nullptr, &update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposed_param_updates_get_key_value_at, returnsErrorIfUpdateIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash = nullptr;

  // Act
  cardano_error_t error = cardano_proposed_param_updates_get_key_value_at((cardano_proposed_param_updates_t*)"", 0, &hash, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposed_param_updates_get_key_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_error_t                   error                  = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_blake2b_hash_t*          hash   = nullptr;
  cardano_protocol_param_update_t* update = nullptr;

  // Act
  error = cardano_proposed_param_updates_get_key_value_at(proposed_param_updates, 0, &hash, &update);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
}

TEST(cardano_proposed_param_updates_get_key_value_at, returnsTheElement)
{
  // Arrange
  cardano_proposed_param_updates_t* proposed_param_updates = nullptr;
  cardano_error_t                   error                  = cardano_proposed_param_updates_new(&proposed_param_updates);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_protocol_param_update_t* update = nullptr;
  error                                   = cardano_protocol_param_update_new(&update);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  const char*             test = "test";
  cardano_blake2b_hash_t* hash = NULL;

  error = cardano_blake2b_compute_hash((uint8_t*)test, strlen(test), 32, &hash);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_proposed_param_updates_insert(proposed_param_updates, hash, update);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_t*          hash_out   = nullptr;
  cardano_protocol_param_update_t* update_out = nullptr;
  error                                       = cardano_proposed_param_updates_get_key_value_at(proposed_param_updates, 0, &hash_out, &update_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(hash, hash_out);
  EXPECT_EQ(update, update_out);

  // Cleanup
  cardano_proposed_param_updates_unref(&proposed_param_updates);
  cardano_blake2b_hash_unref(&hash);
  cardano_blake2b_hash_unref(&hash_out);
  cardano_protocol_param_update_unref(&update);
  cardano_protocol_param_update_unref(&update_out);
}