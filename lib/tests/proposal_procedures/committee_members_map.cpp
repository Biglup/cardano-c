/**
 * \file committee_members_map.cpp
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
#include <cardano/proposal_procedures/committee_members_map.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR             = "a48200581c00000000000000000000000000000000000000000000000000000000008200581c10000000000000000000000000000000000000000000000000000000018200581c20000000000000000000000000000000000000000000000000000000028200581c3000000000000000000000000000000000000000000000000000000003";
static const char* CREDENTIAL1_CBOR = "8200581c00000000000000000000000000000000000000000000000000000000";
static const char* CREDENTIAL2_CBOR = "8200581c10000000000000000000000000000000000000000000000000000000";
static const char* CREDENTIAL3_CBOR = "8200581c20000000000000000000000000000000000000000000000000000000";
static const char* CREDENTIAL4_CBOR = "8200581c30000000000000000000000000000000000000000000000000000000";

/* STATIC FUNCTIONS **********************************************************/

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

/* UNIT TESTS ****************************************************************/

TEST(cardano_committee_members_map_new, canCreateProposedParamUpdates)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;

  // Act
  cardano_error_t error = cardano_committee_members_map_new(&committee_members_map);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(committee_members_map, testing::Not((cardano_committee_members_map_t*)nullptr));

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
}

TEST(cardano_committee_members_map_new, returnsErrorIfProposedParamUpdatesIsNull)
{
  // Act
  cardano_error_t error = cardano_committee_members_map_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_members_map_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_committee_members_map_t* committee_members_map = nullptr;

  // Act
  cardano_error_t error = cardano_committee_members_map_new(&committee_members_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(committee_members_map, (cardano_committee_members_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_committee_members_map_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_committee_members_map_t* committee_members_map = nullptr;

  // Act
  cardano_error_t error = cardano_committee_members_map_new(&committee_members_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(committee_members_map, (cardano_committee_members_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_committee_members_map_to_cbor, canSerializeAnEmptyProposedParamUpdates)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_cbor_writer_t*           writer                = cardano_cbor_writer_new();

  cardano_error_t error = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_committee_members_map_to_cbor(committee_members_map, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 3);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "a0");

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_committee_members_map_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_committee_members_map_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_committee_members_map_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;

  cardano_error_t error = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_committee_members_map_to_cbor(committee_members_map, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
}

TEST(cardano_committee_members_map_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t*           writer                = cardano_cbor_writer_new();

  cardano_error_t error = cardano_committee_members_map_from_cbor(reader, &committee_members_map);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_committee_members_map_to_cbor(committee_members_map, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_committee_members_map_from_cbor, returnErrorIfProposedParamUpdatesIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_committee_members_map_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_committee_members_map_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;

  // Act
  cardano_error_t error = cardano_committee_members_map_from_cbor(nullptr, &committee_members_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_members_map_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_committee_members_map_from_cbor(reader, &committee_members_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(committee_members_map, (cardano_committee_members_map_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_committee_members_map_from_cbor, returnErrorIfNotAnMap)
{
  // Arrange
  cardano_committee_members_map_t* list   = nullptr;
  cardano_cbor_reader_t*           reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_committee_members_map_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_committee_members_map_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_committee_members_map_ref(committee_members_map);

  // Assert
  EXPECT_THAT(committee_members_map, testing::Not((cardano_committee_members_map_t*)nullptr));
  EXPECT_EQ(cardano_committee_members_map_refcount(committee_members_map), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_committee_members_map_unref(&committee_members_map);
  cardano_committee_members_map_unref(&committee_members_map);
}

TEST(cardano_committee_members_map_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_committee_members_map_ref(nullptr);
}

TEST(cardano_committee_members_map_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;

  // Act
  cardano_committee_members_map_unref(&committee_members_map);
}

TEST(cardano_committee_members_map_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_committee_members_map_unref((cardano_committee_members_map_t**)nullptr);
}

TEST(cardano_committee_members_map_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_committee_members_map_ref(committee_members_map);
  size_t ref_count = cardano_committee_members_map_refcount(committee_members_map);

  cardano_committee_members_map_unref(&committee_members_map);
  size_t updated_ref_count = cardano_committee_members_map_refcount(committee_members_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
}

TEST(cardano_committee_members_map_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_committee_members_map_ref(committee_members_map);
  size_t ref_count = cardano_committee_members_map_refcount(committee_members_map);

  cardano_committee_members_map_unref(&committee_members_map);
  size_t updated_ref_count = cardano_committee_members_map_refcount(committee_members_map);

  cardano_committee_members_map_unref(&committee_members_map);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(committee_members_map, (cardano_committee_members_map_t*)nullptr);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
}

TEST(cardano_committee_members_map_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_committee_members_map_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_committee_members_map_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  const char*                      message               = "This is a test message";

  // Act
  cardano_committee_members_map_set_last_error(committee_members_map, message);

  // Assert
  EXPECT_STREQ(cardano_committee_members_map_get_last_error(committee_members_map), "Object is NULL.");
}

TEST(cardano_committee_members_map_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_committee_members_map_set_last_error(committee_members_map, message);

  // Assert
  EXPECT_STREQ(cardano_committee_members_map_get_last_error(committee_members_map), "");

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
}

TEST(cardano_committee_members_map_from_cbor, returnErrorIfInvalidMap)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a100", 4);

  // Act
  cardano_error_t error = cardano_committee_members_map_from_cbor(reader, &committee_members_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_committee_members_map_from_cbor, returnErrorIfInvalidMember)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a3581c0000000000000000000000000000000000000000000000000000000fe60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000002b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000003b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba", strlen("a3581c0000000000000000000000000000000000000000000000000000000fe60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000002b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000003b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba"));

  // Act
  cardano_error_t error = cardano_committee_members_map_from_cbor(reader, &committee_members_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_committee_members_map_from_cbor, returnErrorIfInvalidEpoch)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex("a48200581c00000000000000000000000000000000000000000000000000000000fe8200581c10000000000000000000000000000000000000000000000000000000018200581c20000000000000000000000000000000000000000000000000000000028200581c3000000000000000000000000000000000000000000000000000000003", strlen("a48200581c00000000000000000000000000000000000000000000000000000000008200581c10000000000000000000000000000000000000000000000000000000018200581c20000000000000000000000000000000000000000000000000000000028200581c3000000000000000000000000000000000000000000000000000000003"));

  // Act
  cardano_error_t error = cardano_committee_members_map_from_cbor(reader, &committee_members_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_committee_members_map_get_size, returnsZeroIfObjectIsNull)
{
  // Act
  size_t size = cardano_committee_members_map_get_length(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_committee_members_map_get_size, returnsTheNumberOfElementsInTheList)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential1 = new_default_credential(CREDENTIAL1_CBOR);
  cardano_credential_t* credential2 = new_default_credential(CREDENTIAL2_CBOR);
  cardano_credential_t* credential3 = new_default_credential(CREDENTIAL3_CBOR);
  cardano_credential_t* credential4 = new_default_credential(CREDENTIAL3_CBOR);

  error = cardano_committee_members_map_insert(committee_members_map, credential1, 5);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_committee_members_map_insert(committee_members_map, credential2, 5);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_committee_members_map_insert(committee_members_map, credential3, 5);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_committee_members_map_insert(committee_members_map, credential4, 5);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t size = cardano_committee_members_map_get_length(committee_members_map);

  // Assert
  EXPECT_EQ(size, 4);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
  cardano_credential_unref(&credential1);
  cardano_credential_unref(&credential2);
  cardano_credential_unref(&credential3);
  cardano_credential_unref(&credential4);
}

TEST(cardano_committee_members_map_insert, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_credential_t* credential = nullptr;

  // Act
  cardano_error_t error = cardano_committee_members_map_insert(nullptr, credential, 5);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_members_map_insert, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_committee_members_map_insert(committee_members_map, nullptr, 5);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
}

TEST(cardano_committee_members_map_insert, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential = new_default_credential(CREDENTIAL1_CBOR);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_committee_members_map_insert(committee_members_map, credential, 5);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
  cardano_credential_unref(&credential);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_committee_members_map_insert, keepsElementsSortedByCredential)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential1 = new_default_credential(CREDENTIAL1_CBOR);
  cardano_credential_t* credential2 = new_default_credential(CREDENTIAL2_CBOR);
  cardano_credential_t* credential3 = new_default_credential(CREDENTIAL3_CBOR);
  cardano_credential_t* credential4 = new_default_credential(CREDENTIAL4_CBOR);

  // Act
  error = cardano_committee_members_map_insert(committee_members_map, credential3, 2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_committee_members_map_insert(committee_members_map, credential1, 0);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_committee_members_map_insert(committee_members_map, credential4, 3);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_committee_members_map_insert(committee_members_map, credential2, 1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  size_t size = cardano_committee_members_map_get_length(committee_members_map);

  EXPECT_EQ(size, 4);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  error = cardano_committee_members_map_to_cbor(committee_members_map, writer);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  const char* expected = "a48200581c00000000000000000000000000000000000000000000000000000000008200581c10000000000000000000000000000000000000000000000000000000018200581c20000000000000000000000000000000000000000000000000000000028200581c3000000000000000000000000000000000000000000000000000000003";

  EXPECT_STREQ(hex, expected);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
  cardano_credential_unref(&credential1);
  cardano_credential_unref(&credential2);
  cardano_credential_unref(&credential3);
  cardano_credential_unref(&credential4);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_committee_members_map_get, returnsErrorIfObjectIsNull)
{
  // Arrange
  // Act
  cardano_error_t error = cardano_committee_members_map_get(nullptr, nullptr, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_members_map_get, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_committee_members_map_get(committee_members_map, nullptr, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
}

TEST(cardano_committee_members_map_get, returnsErrorIfEpochIsNull)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_committee_members_map_get(committee_members_map, (cardano_credential_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
}

TEST(cardano_committee_members_map_get, returnsErrorIfElementNotFound)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t              value      = 0;
  cardano_credential_t* credential = new_default_credential(CREDENTIAL1_CBOR);

  // Act
  error = cardano_committee_members_map_get(committee_members_map, credential, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
  cardano_credential_unref(&credential);
}

TEST(cardano_committee_members_map_get, returnsTheElement)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential = new_default_credential(CREDENTIAL1_CBOR);

  error = cardano_committee_members_map_insert(committee_members_map, credential, 65);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  uint64_t value = 0;
  error          = cardano_committee_members_map_get(committee_members_map, credential, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, 65);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
  cardano_credential_unref(&credential);
}

TEST(cardano_committee_members_map_get, returnsTheRightElementIfMoreThanOne)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential1 = new_default_credential(CREDENTIAL1_CBOR);
  cardano_credential_t* credential2 = new_default_credential(CREDENTIAL2_CBOR);

  error = cardano_committee_members_map_insert(committee_members_map, credential1, 1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_committee_members_map_insert(committee_members_map, credential2, 2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  uint64_t value = 0;
  error          = cardano_committee_members_map_get(committee_members_map, credential1, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, 1);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
  cardano_credential_unref(&credential1);
  cardano_credential_unref(&credential2);
}

TEST(cardano_committee_members_map_get, returnsTheRightElementIfMoreThanOne2)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential1 = new_default_credential(CREDENTIAL1_CBOR);
  cardano_credential_t* credential2 = new_default_credential(CREDENTIAL2_CBOR);

  error = cardano_committee_members_map_insert(committee_members_map, credential1, 1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_committee_members_map_insert(committee_members_map, credential2, 2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  uint64_t value = 0;
  error          = cardano_committee_members_map_get(committee_members_map, credential2, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, 2);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
  cardano_credential_unref(&credential1);
  cardano_credential_unref(&credential2);
}

TEST(cardano_committee_members_map_get_keys, returnsNullIfObjectIsNull)
{
  // Assert
  EXPECT_EQ(cardano_committee_members_map_get_keys(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_members_map_get_keys, returnsNullIfKeysIsNull)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_committee_members_map_get_keys(committee_members_map, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
}

TEST(cardano_committee_members_map_get_keys, returnsEmptyArrayIfNoElements)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_set_t* keys = nullptr;

  // Act
  error = cardano_committee_members_map_get_keys(committee_members_map, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_credential_set_get_length(keys), 0);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
  cardano_credential_set_unref(&keys);
}

TEST(cardano_committee_members_map_get_keys, returnsTheKeys)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential1 = new_default_credential(CREDENTIAL1_CBOR);
  cardano_credential_t* credential2 = new_default_credential(CREDENTIAL2_CBOR);

  error = cardano_committee_members_map_insert(committee_members_map, credential1, 1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_committee_members_map_insert(committee_members_map, credential2, 2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_set_t* keys = nullptr;

  // Act
  error = cardano_committee_members_map_get_keys(committee_members_map, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_credential_set_get_length(keys), 2);

  cardano_credential_t* key = nullptr;

  error = cardano_credential_set_get(keys, 0, &key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(cardano_credential_compare(credential1, key), 0);

  cardano_credential_unref(&key);

  error = cardano_credential_set_get(keys, 1, &key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(cardano_credential_compare(credential2, key), 0);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
  cardano_credential_unref(&credential1);
  cardano_credential_unref(&credential2);
  cardano_credential_set_unref(&keys);
  cardano_credential_unref(&key);
}

TEST(cardano_committee_members_map_get_key_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_credential_t* committee_members_map = nullptr;

  // Act
  cardano_error_t error = cardano_committee_members_map_get_key_at(nullptr, 0, &committee_members_map);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_members_map_get_key_at, returnsErrorIfOutIsNull)
{
  // Act
  cardano_error_t error = cardano_committee_members_map_get_key_at((cardano_committee_members_map_t*)"", 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_members_map_get_key_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential = nullptr;

  // Act
  error = cardano_committee_members_map_get_key_at(committee_members_map, 0, &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
}

TEST(cardano_committee_members_map_get_key_at, canReturnTheRightKey)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential1 = new_default_credential(CREDENTIAL1_CBOR);
  cardano_credential_t* credential2 = new_default_credential(CREDENTIAL2_CBOR);

  error = cardano_committee_members_map_insert(committee_members_map, credential1, 1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_committee_members_map_insert(committee_members_map, credential2, 2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_credential_t* credential = nullptr;
  error                            = cardano_committee_members_map_get_key_at(committee_members_map, 0, &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_credential_compare(credential1, credential), 0);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
  cardano_committee_members_map_unref(&committee_members_map);
  cardano_credential_unref(&credential1);
  cardano_credential_unref(&credential2);
  cardano_credential_unref(&credential);
}

TEST(cardano_committee_members_map_get_value_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  uint64_t value = 0;

  // Act
  cardano_error_t error = cardano_committee_members_map_get_value_at(nullptr, 0, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_members_map_get_value_at, returnsErrorIfOutIsNull)
{
  // Act
  cardano_error_t error = cardano_committee_members_map_get_value_at((cardano_committee_members_map_t*)"", 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_members_map_get_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t value = 0;

  // Act
  error = cardano_committee_members_map_get_value_at(committee_members_map, 0, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
}

TEST(cardano_committee_members_map_get_value_at, returnsTheElement)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t value = 2;

  cardano_credential_t* credential = new_default_credential(CREDENTIAL1_CBOR);

  error = cardano_committee_members_map_insert(committee_members_map, credential, value);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  uint64_t value_out = 2;
  error              = cardano_committee_members_map_get_value_at(committee_members_map, 0, &value_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(value, value_out);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
  cardano_credential_unref(&credential);
}

TEST(cardano_committee_members_map_get_key_value_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_credential_t* credential = nullptr;
  uint64_t              value      = 0;

  // Act
  cardano_error_t error = cardano_committee_members_map_get_key_value_at(nullptr, 0, &credential, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_members_map_get_key_value_at, returnsErrorIfHashIsNull)
{
  // Arrange
  uint64_t value = 0;

  // Act
  cardano_error_t error = cardano_committee_members_map_get_key_value_at((cardano_committee_members_map_t*)"", 0, nullptr, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_members_map_get_key_value_at, returnsErrorIfUpdateIsNull)
{
  // Arrange
  cardano_credential_t* credential = nullptr;

  // Act
  cardano_error_t error = cardano_committee_members_map_get_key_value_at((cardano_committee_members_map_t*)"", 0, &credential, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_committee_members_map_get_key_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential = nullptr;
  uint64_t              value      = 0;

  // Act
  error = cardano_committee_members_map_get_key_value_at(committee_members_map, 0, &credential, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
}

TEST(cardano_committee_members_map_get_key_value_at, returnsTheElement)
{
  // Arrange
  cardano_committee_members_map_t* committee_members_map = nullptr;
  cardano_error_t                  error                 = cardano_committee_members_map_new(&committee_members_map);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t value = 10;

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential = new_default_credential(CREDENTIAL1_CBOR);

  error = cardano_committee_members_map_insert(committee_members_map, credential, value);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_credential_t* credential_out = nullptr;
  uint64_t              value_out      = 0;
  error                                = cardano_committee_members_map_get_key_value_at(committee_members_map, 0, &credential_out, &value_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_credential_compare(credential, credential_out), 0);
  EXPECT_EQ(value, value_out);

  // Cleanup
  cardano_committee_members_map_unref(&committee_members_map);
  cardano_credential_unref(&credential);
  cardano_credential_unref(&credential_out);
}