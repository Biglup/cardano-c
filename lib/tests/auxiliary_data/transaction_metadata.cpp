/**
 * \file transaction_metadata.cpp
 *
 * \author angel.castillo
 * \date   Sep 19, 2024
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

#include <cardano/auxiliary_data/metadatum.h>
#include <cardano/auxiliary_data/transaction_metadata.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR            = "a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b65";
static const char* METADATUM_CBOR  = "a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b65";
static const char* METADATUM_CBOR2 = "a4187b1904d2636b65796576616c7565646b65793246000102034405a1190237656569676874a119029a6463616b65";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the metadatum.
 * @return A new instance of the metadatum.
 */
static cardano_metadatum_t*
new_default_metadatum(const char* cbor)
{
  cardano_metadatum_t*   metadatum_obj = NULL;
  cardano_cbor_reader_t* reader        = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result        = cardano_metadatum_from_cbor(reader, &metadatum_obj);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return metadatum_obj;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_transaction_metadata_new, canCreateProposedParamUpdates)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;

  // Act
  cardano_error_t error = cardano_transaction_metadata_new(&transaction_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(transaction_metadata, testing::Not((cardano_transaction_metadata_t*)nullptr));

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
}

TEST(cardano_transaction_metadata_new, returnsErrorIfProposedParamUpdatesIsNull)
{
  // Act
  cardano_error_t error = cardano_transaction_metadata_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_metadata_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_transaction_metadata_t* transaction_metadata = nullptr;

  // Act
  cardano_error_t error = cardano_transaction_metadata_new(&transaction_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(transaction_metadata, (cardano_transaction_metadata_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_transaction_metadata_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_transaction_metadata_t* transaction_metadata = nullptr;

  // Act
  cardano_error_t error = cardano_transaction_metadata_new(&transaction_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(transaction_metadata, (cardano_transaction_metadata_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_transaction_metadata_to_cbor, canSerializeAnEmptyProposedParamUpdates)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_cbor_writer_t*          writer               = cardano_cbor_writer_new();

  cardano_error_t error = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_transaction_metadata_to_cbor(transaction_metadata, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, 3);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, "a0");

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_transaction_metadata_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_transaction_metadata_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_transaction_metadata_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;

  cardano_error_t error = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_transaction_metadata_to_cbor(transaction_metadata, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
}

TEST(cardano_transaction_metadata_to_cbor, canDeserializeAndReserializeCbor)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_cbor_writer_t*          writer               = cardano_cbor_writer_new();

  cardano_error_t error = cardano_transaction_metadata_from_cbor(reader, &transaction_metadata);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_transaction_metadata_to_cbor(transaction_metadata, writer);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  EXPECT_EQ(hex_size, strlen(CBOR) + 1);

  char* actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(actual_cbor, CBOR);

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(actual_cbor);
}

TEST(cardano_transaction_metadata_from_cbor, returnErrorIfProposedParamUpdatesIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_transaction_metadata_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_metadata_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;

  // Act
  cardano_error_t error = cardano_transaction_metadata_from_cbor(nullptr, &transaction_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_metadata_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_transaction_metadata_from_cbor(reader, &transaction_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(transaction_metadata, (cardano_transaction_metadata_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_metadata_from_cbor, returnErrorIfNotAnMap)
{
  // Arrange
  cardano_transaction_metadata_t* list   = nullptr;
  cardano_cbor_reader_t*          reader = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_transaction_metadata_from_cbor(reader, &list);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_metadata_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_transaction_metadata_ref(transaction_metadata);

  // Assert
  EXPECT_THAT(transaction_metadata, testing::Not((cardano_transaction_metadata_t*)nullptr));
  EXPECT_EQ(cardano_transaction_metadata_refcount(transaction_metadata), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_transaction_metadata_unref(&transaction_metadata);
  cardano_transaction_metadata_unref(&transaction_metadata);
}

TEST(cardano_transaction_metadata_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_transaction_metadata_ref(nullptr);
}

TEST(cardano_transaction_metadata_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;

  // Act
  cardano_transaction_metadata_unref(&transaction_metadata);
}

TEST(cardano_transaction_metadata_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_transaction_metadata_unref((cardano_transaction_metadata_t**)nullptr);
}

TEST(cardano_transaction_metadata_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_transaction_metadata_ref(transaction_metadata);
  size_t ref_count = cardano_transaction_metadata_refcount(transaction_metadata);

  cardano_transaction_metadata_unref(&transaction_metadata);
  size_t updated_ref_count = cardano_transaction_metadata_refcount(transaction_metadata);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
}

TEST(cardano_transaction_metadata_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_transaction_metadata_ref(transaction_metadata);
  size_t ref_count = cardano_transaction_metadata_refcount(transaction_metadata);

  cardano_transaction_metadata_unref(&transaction_metadata);
  size_t updated_ref_count = cardano_transaction_metadata_refcount(transaction_metadata);

  cardano_transaction_metadata_unref(&transaction_metadata);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(transaction_metadata, (cardano_transaction_metadata_t*)nullptr);

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
}

TEST(cardano_transaction_metadata_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_transaction_metadata_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_transaction_metadata_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  const char*                     message              = "This is a test message";

  // Act
  cardano_transaction_metadata_set_last_error(transaction_metadata, message);

  // Assert
  EXPECT_STREQ(cardano_transaction_metadata_get_last_error(transaction_metadata), "Object is NULL.");
}

TEST(cardano_transaction_metadata_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_transaction_metadata_set_last_error(transaction_metadata, message);

  // Assert
  EXPECT_STREQ(cardano_transaction_metadata_get_last_error(transaction_metadata), "");

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
}

TEST(cardano_transaction_metadata_from_cbor, returnErrorIfInvalidMap)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex("a100", 4);

  // Act
  cardano_error_t error = cardano_transaction_metadata_from_cbor(reader, &transaction_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_metadata_from_cbor, returnErrorIfInvalidTransactionMetadata)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex("a3581c0000000000000000000000000000000000000000000000000000000fe60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000002b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000003b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba", strlen("a3581c0000000000000000000000000000000000000000000000000000000fe60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000002b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000003b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba"));

  // Act
  cardano_error_t error = cardano_transaction_metadata_from_cbor(reader, &transaction_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_metadata_from_cbor, returnErrorIfInvalidTransactionMetadataAmount)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex("a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0ef", strlen("a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d005"));

  // Act
  cardano_error_t error = cardano_transaction_metadata_from_cbor(reader, &transaction_metadata);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_metadata_get_size, returnsZeroIfObjectIsNull)
{
  // Act
  size_t size = cardano_transaction_metadata_get_length(nullptr);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_transaction_metadata_get_size, returnsTheNumberOfElementsInTheList)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* metadatum = new_default_metadatum(METADATUM_CBOR);

  error = cardano_transaction_metadata_insert(transaction_metadata, 5, metadatum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t size = cardano_transaction_metadata_get_length(transaction_metadata);

  // Assert
  EXPECT_EQ(size, 1);

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_transaction_metadata_insert, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_metadatum_t* metadatum = nullptr;

  // Act
  cardano_error_t error = cardano_transaction_metadata_insert(nullptr, 5, metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_metadata_insert, returnsErrorIfMetadatumIsNull)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_transaction_metadata_insert(transaction_metadata, 5, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
}

TEST(cardano_transaction_metadata_insert, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* metadatum = new_default_metadatum(METADATUM_CBOR);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  error = cardano_transaction_metadata_insert(transaction_metadata, 5, metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
  cardano_metadatum_unref(&metadatum);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_transaction_metadata_insert, keepsElementsSortedByLabel)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* metadatum1 = new_default_metadatum(METADATUM_CBOR);
  cardano_metadatum_t* metadatum2 = new_default_metadatum(METADATUM_CBOR2);

  // Act
  error = cardano_transaction_metadata_insert(transaction_metadata, 99, metadatum1);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_transaction_metadata_insert(transaction_metadata, 2, metadatum2);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  size_t size = cardano_transaction_metadata_get_length(transaction_metadata);

  EXPECT_EQ(size, 2);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  error = cardano_transaction_metadata_to_cbor(transaction_metadata, writer);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  const char* expected = "a202a4187b1904d2636b65796576616c7565646b65793246000102034405a1190237656569676874a119029a6463616b651863a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b65";

  EXPECT_STREQ(hex, expected);

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
  cardano_metadatum_unref(&metadatum1);
  cardano_metadatum_unref(&metadatum2);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_transaction_metadata_get, returnsErrorIfObjectIsNull)
{
  // Arrange
  // Act
  cardano_error_t error = cardano_transaction_metadata_get(nullptr, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_metadata_get, returnsErrorIfMetadatumIsNull)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_transaction_metadata_get(transaction_metadata, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
}

TEST(cardano_transaction_metadata_get, returnsErrorIfElementNotFound)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* metadatum = nullptr;

  // Act
  error = cardano_transaction_metadata_get(transaction_metadata, 0, &metadatum);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
  cardano_metadatum_unref(&metadatum);
}

TEST(cardano_transaction_metadata_get, returnsTheElement)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* metadatum = new_default_metadatum(METADATUM_CBOR);

  error = cardano_transaction_metadata_insert(transaction_metadata, 65, metadatum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_metadatum_t* value = 0;
  error                      = cardano_transaction_metadata_get(transaction_metadata, 65, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_metadatum_equals(metadatum, value));

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
  cardano_metadatum_unref(&metadatum);
  cardano_metadatum_unref(&value);
}

TEST(cardano_transaction_metadata_get, returnsTheRightElementIfMoreThanOne)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* metadatum1 = new_default_metadatum(METADATUM_CBOR);
  cardano_metadatum_t* metadatum2 = new_default_metadatum(METADATUM_CBOR2);

  error = cardano_transaction_metadata_insert(transaction_metadata, 1, metadatum1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_transaction_metadata_insert(transaction_metadata, 2, metadatum2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_metadatum_t* value = NULL;
  error                      = cardano_transaction_metadata_get(transaction_metadata, 2, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_metadatum_equals(metadatum2, value));

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
  cardano_metadatum_unref(&metadatum1);
  cardano_metadatum_unref(&metadatum2);
  cardano_metadatum_unref(&value);
}

TEST(cardano_transaction_metadata_get, returnsTheRightElementIfMoreThanOne2)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* metadatum1 = new_default_metadatum(METADATUM_CBOR);
  cardano_metadatum_t* metadatum2 = new_default_metadatum(METADATUM_CBOR2);

  error = cardano_transaction_metadata_insert(transaction_metadata, 1, metadatum1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_transaction_metadata_insert(transaction_metadata, 2, metadatum2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_metadatum_t* value = NULL;
  error                      = cardano_transaction_metadata_get(transaction_metadata, 1, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_metadatum_equals(metadatum1, value));

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
  cardano_metadatum_unref(&metadatum1);
  cardano_metadatum_unref(&metadatum2);
  cardano_metadatum_unref(&value);
}

TEST(cardano_transaction_metadata_get_keys, returnsNullIfObjectIsNull)
{
  // Assert
  EXPECT_EQ(cardano_transaction_metadata_get_keys(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_metadata_get_keys, returnsNullIfKeysIsNull)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_transaction_metadata_get_keys(transaction_metadata, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
}

TEST(cardano_transaction_metadata_get_keys, returnsEmptyArrayIfNoElements)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_label_list_t* keys = nullptr;

  // Act
  error = cardano_transaction_metadata_get_keys(transaction_metadata, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_label_list_get_length(keys), 0);

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
  cardano_metadatum_label_list_unref(&keys);
}

TEST(cardano_transaction_metadata_get_keys, returnsTheKeys)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* metadatum1 = new_default_metadatum(METADATUM_CBOR);
  cardano_metadatum_t* metadatum2 = new_default_metadatum(METADATUM_CBOR2);

  error = cardano_transaction_metadata_insert(transaction_metadata, 1, metadatum1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_transaction_metadata_insert(transaction_metadata, 2, metadatum2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_label_list_t* keys = nullptr;

  // Act
  error = cardano_transaction_metadata_get_keys(transaction_metadata, &keys);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_metadatum_label_list_get_length(keys), 2);

  uint64_t key = 0;

  error = cardano_metadatum_label_list_get(keys, 0, &key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_metadatum_label_list_get(keys, 1, &key);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
  cardano_metadatum_unref(&metadatum1);
  cardano_metadatum_unref(&metadatum2);
  cardano_metadatum_label_list_unref(&keys);
}

TEST(cardano_transaction_metadata_get_key_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  uint64_t label = 0;

  // Act
  cardano_error_t error = cardano_transaction_metadata_get_key_at(nullptr, 0, &label);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_metadata_get_key_at, returnsErrorIfOutIsNull)
{
  // Act
  cardano_error_t error = cardano_transaction_metadata_get_key_at((cardano_transaction_metadata_t*)"", 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_metadata_get_key_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t label = 0;

  // Act
  error = cardano_transaction_metadata_get_key_at(transaction_metadata, 0, &label);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
}

TEST(cardano_transaction_metadata_get_key_at, canReturnTheRightKey)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* metadatum1 = new_default_metadatum(METADATUM_CBOR);
  cardano_metadatum_t* metadatum2 = new_default_metadatum(METADATUM_CBOR2);

  error = cardano_transaction_metadata_insert(transaction_metadata, 1, metadatum1);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_transaction_metadata_insert(transaction_metadata, 2, metadatum2);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  uint64_t label = 0;
  error          = cardano_transaction_metadata_get_key_at(transaction_metadata, 0, &label);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(label, 1);

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
  cardano_metadatum_unref(&metadatum1);
  cardano_metadatum_unref(&metadatum2);
}

TEST(cardano_transaction_metadata_get_value_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_metadatum_t* value = NULL;

  // Act
  cardano_error_t error = cardano_transaction_metadata_get_value_at(nullptr, 0, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_metadata_get_value_at, returnsErrorIfOutIsNull)
{
  // Act
  cardano_error_t error = cardano_transaction_metadata_get_value_at((cardano_transaction_metadata_t*)"", 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_metadata_get_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* value = NULL;

  // Act
  error = cardano_transaction_metadata_get_value_at(transaction_metadata, 0, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
}

TEST(cardano_transaction_metadata_get_value_at, returnsTheElement)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t key = 2;

  cardano_metadatum_t* metadatum = new_default_metadatum(METADATUM_CBOR);

  error = cardano_transaction_metadata_insert(transaction_metadata, key, metadatum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_metadatum_t* value_out = nullptr;
  error                          = cardano_transaction_metadata_get_value_at(transaction_metadata, 0, &value_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_metadatum_equals(metadatum, value_out));

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
  cardano_metadatum_unref(&metadatum);
  cardano_metadatum_unref(&value_out);
}

TEST(cardano_transaction_metadata_get_key_value_at, returnsErrorIfObjectIsNull)
{
  // Arrange
  uint64_t             key   = 0;
  cardano_metadatum_t* value = nullptr;

  // Act
  cardano_error_t error = cardano_transaction_metadata_get_key_value_at(nullptr, 0, &key, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_metadata_get_key_value_at, returnsErrorIfMEtadatumIsNull)
{
  // Arrange
  cardano_metadatum_t* value = NULL;

  // Act
  cardano_error_t error = cardano_transaction_metadata_get_key_value_at((cardano_transaction_metadata_t*)"", 0, nullptr, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_metadata_get_key_value_at, returnsErrorIfUpdateIsNull)
{
  // Arrange
  uint64_t val = 0;

  // Act
  cardano_error_t error = cardano_transaction_metadata_get_key_value_at((cardano_transaction_metadata_t*)"", 0, &val, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_metadata_get_key_value_at, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t             key   = 0;
  cardano_metadatum_t* value = nullptr;

  // Act
  error = cardano_transaction_metadata_get_key_value_at(transaction_metadata, 0, &key, &value);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
  cardano_metadatum_unref(&value);
}

TEST(cardano_transaction_metadata_get_key_value_at, returnsTheElement)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t key = 10;

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* metadatum = new_default_metadatum(METADATUM_CBOR);

  error = cardano_transaction_metadata_insert(transaction_metadata, key, metadatum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  uint64_t             key_out   = 0;
  cardano_metadatum_t* value_out = NULL;
  error                          = cardano_transaction_metadata_get_key_value_at(transaction_metadata, 0, &key_out, &value_out);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(key_out, key);
  EXPECT_TRUE(cardano_metadatum_equals(metadatum, value_out));

  // Cleanup
  cardano_transaction_metadata_unref(&transaction_metadata);
  cardano_metadatum_unref(&metadatum);
  cardano_metadatum_unref(&value_out);
}

TEST(cardano_transaction_metadata_to_cip116_json, canEncodeMetadata)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  uint64_t key = 10;

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_metadatum_t* metadatum = new_default_metadatum(METADATUM_CBOR);

  error = cardano_transaction_metadata_insert(transaction_metadata, key, metadatum);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);
  error                         = cardano_transaction_metadata_to_cip116_json(transaction_metadata, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t json_size = cardano_json_writer_get_encoded_size(writer);
  char*        json      = (char*)malloc(json_size);

  error = cardano_json_writer_encode(writer, json, json_size);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(json, "[{\"key\":\"10\",\"value\":{\"tag\":\"map\",\"contents\":[{\"key\":{\"tag\":\"int\",\"value\":,\"123\"},\"value\":{\"tag\":\"int\",\"value\":,\"1234\"}},{\"key\":{\"tag\":\"string\",\"value\":\"key\"},\"value\":{\"tag\":\"string\",\"value\":\"value\"}},{\"key\":{\"tag\":\"string\",\"value\":\"key2\"},\"value\":{\"tag\":\"bytes\",\"value\":\"000102030405\"}},{\"key\":{\"tag\":\"map\",\"contents\":[{\"key\":{\"tag\":\"int\",\"value\":,\"567\"},\"value\":{\"tag\":\"string\",\"value\":\"eight\"}}]},\"value\":{\"tag\":\"map\",\"contents\":[{\"key\":{\"tag\":\"int\",\"value\":,\"666\"},\"value\":{\"tag\":\"string\",\"value\":\"cake\"}}]}}]}}]");

  // Cleanup
  cardano_metadatum_unref(&metadatum);
  free(json);
  cardano_json_writer_unref(&writer);
  cardano_transaction_metadata_unref(&transaction_metadata);
}

TEST(cardano_transaction_metadata_to_cip116_json, returnErrorIfNullPointer)
{
  // Arrange
  cardano_transaction_metadata_t* transaction_metadata = nullptr;
  cardano_error_t                 error                = cardano_transaction_metadata_new(&transaction_metadata);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_json_writer_t* writer = cardano_json_writer_new(CARDANO_JSON_FORMAT_PRETTY);

  EXPECT_EQ(cardano_transaction_metadata_to_cip116_json(nullptr, writer), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_transaction_metadata_to_cip116_json(transaction_metadata, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_json_writer_unref(&writer);
  cardano_transaction_metadata_unref(&transaction_metadata);
}
