/**
 * \file withdrawal_map.cpp
 *
 * \author angel.castillo
 * \date   May 21, 2024
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

#include <cardano/address/reward_address.h>
#include <cardano/common/withdrawal_map.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* rewardKey    = "stake1uyehkck0lajq8gr28t9uxnuvgcqrc6070x3k9r8048z8y5gh6ffgw";
static const char* rewardScript = "stake178phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcccycj5";
static const char* CBOR         = "a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d005";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the reward address.
 * @return A new instance of the reward address.
 */
static cardano_reward_address_t*
new_default_reward_address(const char* reward_address)
{
  cardano_reward_address_t* reward_address_obj = NULL;
  cardano_error_t           result             = cardano_reward_address_from_bech32(reward_address, strlen(reward_address), &reward_address_obj);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  return reward_address_obj;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_withdrawal_map_new, canCreateProposedParamUpdates)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;

  // Act
  cardano_error_t error = cardano_withdrawal_map_new(&withdrawal_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(withdrawal_map, testing::Not((cardano_withdrawal_map_t*)nullptr));

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
}

TEST(cardano_withdrawal_map_new, returnsErrorIfProposedParamUpdatesIsNull)
{
  // Act
  cardano_error_t error = cardano_withdrawal_map_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_withdrawal_map_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_withdrawal_map_t* withdrawal_map = nullptr;

  // Act
  cardano_error_t error = cardano_withdrawal_map_new(&withdrawal_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(withdrawal_map, (cardano_withdrawal_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_withdrawal_map_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_withdrawal_map_t* withdrawal_map = nullptr;

  // Act
  cardano_error_t error = cardano_withdrawal_map_new(&withdrawal_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(withdrawal_map, (cardano_withdrawal_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_withdrawal_map_to_cbor, canSerializeAnEmptyProposedParamUpdates)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_cbor_writer_t*    writer         = cardano_cbor_writer_new();

  cardano_error_t error = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_withdrawal_map_to_cbor(withdrawal_map, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 3);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "a0");

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_withdrawal_map_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_withdrawal_map_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_withdrawal_map_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;

  cardano_error_t error = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_withdrawal_map_to_cbor(withdrawal_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
}

TEST(cardano_withdrawal_map_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t*    writer         = cardano_cbor_writer_new();

  cardano_error_t error = cardano_withdrawal_map_from_cbor(reader, &withdrawal_map);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_withdrawal_map_to_cbor(withdrawal_map, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_withdrawal_map_from_cbor, returnErrorIfProposedParamUpdatesIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_withdrawal_map_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_withdrawal_map_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;

  // Act
  cardano_error_t error = cardano_withdrawal_map_from_cbor(nullptr, &withdrawal_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_withdrawal_map_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_withdrawal_map_from_cbor(reader, &withdrawal_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(withdrawal_map, (cardano_withdrawal_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_withdrawal_map_from_cbor, returnErrorIfNotAnMap)
{
  // Arrange
  cardano_withdrawal_map_t* list   = nullptr;
  cardano_cbor_reader_t*    reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_withdrawal_map_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_withdrawal_map_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_withdrawal_map_ref(withdrawal_map);

  // Assert
  EXPECT_THAT(withdrawal_map, testing::Not((cardano_withdrawal_map_t*)nullptr));
  EXPECT_EQ(cardano_withdrawal_map_refcount(withdrawal_map), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_withdrawal_map_unref(&withdrawal_map);
}

TEST(cardano_withdrawal_map_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_withdrawal_map_ref(nullptr);
}

TEST(cardano_withdrawal_map_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;

  // Act
  cardano_withdrawal_map_unref(&withdrawal_map);
}

TEST(cardano_withdrawal_map_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_withdrawal_map_unref((cardano_withdrawal_map_t**)nullptr);
}

TEST(cardano_withdrawal_map_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_withdrawal_map_ref(withdrawal_map);
  size_t ref_count = cardano_withdrawal_map_refcount(withdrawal_map);

  cardano_withdrawal_map_unref(&withdrawal_map);
  size_t updated_ref_count = cardano_withdrawal_map_refcount(withdrawal_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
}

TEST(cardano_withdrawal_map_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_withdrawal_map_ref(withdrawal_map);
  size_t ref_count = cardano_withdrawal_map_refcount(withdrawal_map);

  cardano_withdrawal_map_unref(&withdrawal_map);
  size_t updated_ref_count = cardano_withdrawal_map_refcount(withdrawal_map);

  cardano_withdrawal_map_unref(&withdrawal_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(withdrawal_map, (cardano_withdrawal_map_t*)nullptr);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
}

TEST(cardano_withdrawal_map_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_withdrawal_map_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_withdrawal_map_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  const char*               message        = "This is a test message";

  // Act
  cardano_withdrawal_map_set_last_error(withdrawal_map, message);

  // Assert
  EXPECT_STREQ(cardano_withdrawal_map_get_last_error(withdrawal_map), "Object is NULL.");
}

TEST(cardano_withdrawal_map_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_withdrawal_map_set_last_error(withdrawal_map, message);

  // Assert
  EXPECT_STREQ(cardano_withdrawal_map_get_last_error(withdrawal_map), "");

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
}

TEST(cardano_withdrawal_map_from_cbor, returnErrorIfInvalidMap)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex("a100", 4);

  // Act
  cardano_error_t error = cardano_withdrawal_map_from_cbor(reader, &withdrawal_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_withdrawal_map_from_cbor, returnErrorIfInvalidWithdrawl)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex("a3581c0000000000000000000000000000000000000000000000000000000fe60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000002b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000003b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba", strlen("a3581c0000000000000000000000000000000000000000000000000000000fe60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000002b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000003b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba"));

  // Act
  cardano_error_t error = cardano_withdrawal_map_from_cbor(reader, &withdrawal_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ADDRESS_FORMAT);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_withdrawal_map_from_cbor, returnErrorIfInvalidWithdrawlAmount)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex("a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0ef", strlen("a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d005"));

  // Act
  cardano_error_t error = cardano_withdrawal_map_from_cbor(reader, &withdrawal_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_withdrawal_map_get_size, returnsZeroIfObjectIsNull)
{
  // Act
  size_t size = cardano_withdrawal_map_get_length(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_withdrawal_map_get_size, returnsTheNumberOfElementsInTheList)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* address = new_default_reward_address(rewardKey);

  error = cardano_withdrawal_map_insert(withdrawal_map, address, 5);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t size = cardano_withdrawal_map_get_length(withdrawal_map);

  // Assert
  EXPECT_EQ(size, 1);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_reward_address_unref(&address);
}

TEST(cardano_withdrawal_map_insert, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_reward_address_t* address = nullptr;

  // Act
  cardano_error_t error = cardano_withdrawal_map_insert(nullptr, address, 5);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_withdrawal_map_insert, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_withdrawal_map_insert(withdrawal_map, nullptr, 5);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
}

TEST(cardano_withdrawal_map_insert, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* address = new_default_reward_address(rewardKey);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_withdrawal_map_insert(withdrawal_map, address, 5);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_reward_address_unref(&address);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_withdrawal_map_insert, keepsElementsSortedByAddress)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* address1 = new_default_reward_address(rewardKey);
  cardano_reward_address_t* address2 = new_default_reward_address(rewardScript);

  // Act
  error = cardano_withdrawal_map_insert(withdrawal_map, address1, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_withdrawal_map_insert(withdrawal_map, address2, 2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  size_t size = cardano_withdrawal_map_get_length(withdrawal_map);

  EXPECT_EQ(size, 2);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  error = cardano_withdrawal_map_to_cbor(withdrawal_map, writer);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  const char* expected = "a2581de1337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c4725101581df1c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f02";

  EXPECT_STREQ(hex, expected);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_reward_address_unref(&address1);
  cardano_reward_address_unref(&address2);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_withdrawal_map_get, returnsErrorIfObjectIsNull)
{
  // Arrange
  // Act
  cardano_error_t error = cardano_withdrawal_map_get(nullptr, nullptr, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_withdrawal_map_get, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_withdrawal_map_get(withdrawal_map, nullptr, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
}

TEST(cardano_withdrawal_map_get, returnsErrorIfAmountIsNull)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_withdrawal_map_get(withdrawal_map, (cardano_reward_address_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
}

TEST(cardano_withdrawal_map_get, returnsErrorIfElementNotFound)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t                  value   = 0;
  cardano_reward_address_t* address = new_default_reward_address(rewardKey);

  // Act
  error = cardano_withdrawal_map_get(withdrawal_map, address, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_reward_address_unref(&address);
}

TEST(cardano_withdrawal_map_get, returnsTheElement)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* address = new_default_reward_address(rewardKey);

  error = cardano_withdrawal_map_insert(withdrawal_map, address, 65);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  uint64_t value = 0;
  error          = cardano_withdrawal_map_get(withdrawal_map, address, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, 65);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_reward_address_unref(&address);
}

TEST(cardano_withdrawal_map_get, returnsTheRightElementIfMoreThanOne)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* address1 = new_default_reward_address(rewardKey);
  cardano_reward_address_t* address2 = new_default_reward_address(rewardScript);

  error = cardano_withdrawal_map_insert(withdrawal_map, address1, 1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_withdrawal_map_insert(withdrawal_map, address2, 2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  uint64_t value = 0;
  error          = cardano_withdrawal_map_get(withdrawal_map, address1, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, 1);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_reward_address_unref(&address1);
  cardano_reward_address_unref(&address2);
}

TEST(cardano_withdrawal_map_get, returnsTheRightElementIfMoreThanOne2)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* address1 = new_default_reward_address(rewardKey);
  cardano_reward_address_t* address2 = new_default_reward_address(rewardScript);

  error = cardano_withdrawal_map_insert(withdrawal_map, address1, 1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_withdrawal_map_insert(withdrawal_map, address2, 2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  uint64_t value = 0;
  error          = cardano_withdrawal_map_get(withdrawal_map, address2, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, 2);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_reward_address_unref(&address1);
  cardano_reward_address_unref(&address2);
}

TEST(cardano_withdrawal_map_get_keys, returnsNullIfObjectIsNull)
{
  // Assert
  EXPECT_EQ(cardano_withdrawal_map_get_keys(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_withdrawal_map_get_keys, returnsNullIfKeysIsNull)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_withdrawal_map_get_keys(withdrawal_map, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
}

TEST(cardano_withdrawal_map_get_keys, returnsEmptyArrayIfNoElements)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_list_t* keys = nullptr;

  // Act
  error = cardano_withdrawal_map_get_keys(withdrawal_map, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_reward_address_list_get_length(keys), 0);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_reward_address_list_unref(&keys);
}

TEST(cardano_withdrawal_map_get_keys, returnsTheKeys)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* address1 = new_default_reward_address(rewardKey);
  cardano_reward_address_t* address2 = new_default_reward_address(rewardScript);

  error = cardano_withdrawal_map_insert(withdrawal_map, address1, 1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_withdrawal_map_insert(withdrawal_map, address2, 2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_list_t* keys = nullptr;

  // Act
  error = cardano_withdrawal_map_get_keys(withdrawal_map, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_reward_address_list_get_length(keys), 2);

  cardano_reward_address_t* key = nullptr;

  error = cardano_reward_address_list_get(keys, 0, &key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  char bech32[120] = { 0 };

  EXPECT_EQ(cardano_reward_address_to_bech32(key, bech32, 120), CARDANO_SUCCESS);

  EXPECT_STREQ(rewardKey, bech32);

  cardano_reward_address_unref(&key);

  error = cardano_reward_address_list_get(keys, 1, &key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(cardano_reward_address_to_bech32(key, bech32, 120), CARDANO_SUCCESS);

  EXPECT_STREQ(rewardScript, bech32);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_reward_address_list_unref(&keys);
  cardano_reward_address_unref(&address1);
  cardano_reward_address_unref(&address2);
  cardano_reward_address_unref(&key);
}

TEST(cardano_withdrawal_map_get_key_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = nullptr;

  // Act
  cardano_error_t error = cardano_withdrawal_map_get_key_at(nullptr, 0, &reward_address);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_withdrawal_map_get_key_at, returnsErrorIfOutIsNull)
{
  // Act
  cardano_error_t error = cardano_withdrawal_map_get_key_at((cardano_withdrawal_map_t*)"", 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_withdrawal_map_get_key_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = nullptr;

  // Act
  error = cardano_withdrawal_map_get_key_at(withdrawal_map, 0, &reward_address);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
}

TEST(cardano_withdrawal_map_get_key_at, canReturnTheRightKey)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* address1 = new_default_reward_address(rewardKey);
  cardano_reward_address_t* address2 = new_default_reward_address(rewardScript);

  error = cardano_withdrawal_map_insert(withdrawal_map, address1, 1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_withdrawal_map_insert(withdrawal_map, address2, 2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_reward_address_t* reward_address = nullptr;
  error                                    = cardano_withdrawal_map_get_key_at(withdrawal_map, 0, &reward_address);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(reward_address, address1);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_reward_address_unref(&reward_address);
  cardano_reward_address_unref(&address1);
  cardano_reward_address_unref(&address2);
}

TEST(cardano_withdrawal_map_get_value_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  uint64_t value = 0;

  // Act
  cardano_error_t error = cardano_withdrawal_map_get_value_at(nullptr, 0, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_withdrawal_map_get_value_at, returnsErrorIfOutIsNull)
{
  // Act
  cardano_error_t error = cardano_withdrawal_map_get_value_at((cardano_withdrawal_map_t*)"", 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_withdrawal_map_get_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t value = 0;

  // Act
  error = cardano_withdrawal_map_get_value_at(withdrawal_map, 0, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
}

TEST(cardano_withdrawal_map_get_value_at, returnsTheElement)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t value = 2;

  cardano_reward_address_t* reward_address = new_default_reward_address(rewardKey);

  error = cardano_withdrawal_map_insert(withdrawal_map, reward_address, value);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  uint64_t value_out = 2;
  error              = cardano_withdrawal_map_get_value_at(withdrawal_map, 0, &value_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, value_out);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_withdrawal_map_get_key_value_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = nullptr;
  uint64_t                  value          = 0;

  // Act
  cardano_error_t error = cardano_withdrawal_map_get_key_value_at(nullptr, 0, &reward_address, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_withdrawal_map_get_key_value_at, returnsErrorIfHashIsNull)
{
  // Arrange
  uint64_t value = 0;

  // Act
  cardano_error_t error = cardano_withdrawal_map_get_key_value_at((cardano_withdrawal_map_t*)"", 0, nullptr, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_withdrawal_map_get_key_value_at, returnsErrorIfUpdateIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = nullptr;

  // Act
  cardano_error_t error = cardano_withdrawal_map_get_key_value_at((cardano_withdrawal_map_t*)"", 0, &reward_address, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_withdrawal_map_get_key_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = nullptr;
  uint64_t                  value          = 0;

  // Act
  error = cardano_withdrawal_map_get_key_value_at(withdrawal_map, 0, &reward_address, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
}

TEST(cardano_withdrawal_map_get_key_value_at, returnsTheElement)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t value = 10;

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_reward_address_t* reward_address = new_default_reward_address(rewardKey);

  error = cardano_withdrawal_map_insert(withdrawal_map, reward_address, value);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_reward_address_t* reward_address_out = nullptr;
  uint64_t                  value_out          = 0;
  error                                        = cardano_withdrawal_map_get_key_value_at(withdrawal_map, 0, &reward_address_out, &value_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(reward_address, reward_address_out);
  EXPECT_EQ(value, value_out);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_reward_address_unref(&reward_address);
  cardano_reward_address_unref(&reward_address_out);
}

TEST(cardano_withdrawal_map_insert_ex, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_error_t error = cardano_withdrawal_map_insert_ex(nullptr, "xxxxx", 5, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_withdrawal_map_insert_ex, canAddInsertElement)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_withdrawal_map_insert_ex(withdrawal_map, rewardScript, strlen(rewardScript), 100);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_withdrawal_map_get_length(withdrawal_map), 1);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
}

TEST(cardano_withdrawal_map_insert_ex, returnsErrorIfTheAddresIsInvalid)
{
  // Arrange
  cardano_withdrawal_map_t* withdrawal_map = nullptr;
  cardano_error_t           error          = cardano_withdrawal_map_new(&withdrawal_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_withdrawal_map_insert_ex(withdrawal_map, "xxxxx", 5, 0);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ADDRESS_FORMAT);

  // Cleanup
  cardano_withdrawal_map_unref(&withdrawal_map);
}