/**
 * \file auxiliary_data.cpp
 *
 * \author angel.castillo
 * \date   Sep 20, 2024
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

#include <cardano/auxiliary_data/auxiliary_data.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/crypto/blake2b_hash.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* AUXILIARY_DATA_CBOR               = "d90103a500a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b6501848204038205098202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f5402844746010000220010474601000022001147460100002200124746010000220013038447460100002200104746010000220011474601000022001247460100002200130483474601000022001047460100002200114746010000220012";
static const char* AUXILIARY_DATA_CBOR2              = "d90103a200a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b6501828202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f54";
static const char* AUXILIARY_DATA_CBOR3              = "d90103a100a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b65";
static const char* SHELLEY_AUXILIARY_DATA_CBOR       = "82a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b65828202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f54";
static const char* JUST_METADATA_AUXILIARY_DATA_CBOR = "a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b65";
static const char* TRANSACTION_METADATA_CBOR         = "a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b65";
static const char* NATIVE_SCRIPT_LIST_CBOR           = "9f8205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0ff";
static const char* PLUTUS_V1_LIST_CBOR               = "844e4d010000332222200512001200114e4d010001332222200512001200114e4d010002332222200512001200114e4d01000333222220051200120011";
static const char* PLUTUS_V2_LIST_CBOR               = "844e4d010000332222200512001200114e4d010001332222200512001200114e4d010002332222200512001200114e4d01000333222220051200120011";
static const char* PLUTUS_V3_LIST_CBOR               = "844e4d010000332222200512001200114e4d010001332222200512001200114e4d010002332222200512001200114e4d01000333222220051200120011";
static const char* AUXILIARY_DATA_HASH               = "d24e84d8dbf6f880b04f64ad919bb618bf66ce834b3c901b1efe2ce6b44beb7b";
static const char* SHELLEY_AUXILIARY_DATA_HASH       = "a02cace10f1fc93061cd0dcc31ccfafb9599eba245ae3f03a2ee69928f73d3ed";
static const char* JUST_METADATA_AUXILIARY_DATA_HASH = "3bed6c134ce51ea7cfccec5ae44acbcb995b568c6408f2a1302f0e1c76d4ae63";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the certificate.
 * @return A new instance of the certificate.
 */
static cardano_auxiliary_data_t*
new_default_auxiliary_data(const char* cbor)
{
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t           result         = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

  cardano_auxiliary_data_clear_cbor_cache(auxiliary_data);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return auxiliary_data;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_auxiliary_data_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);
  EXPECT_NE(auxiliary_data, nullptr);

  // Act
  cardano_auxiliary_data_ref(auxiliary_data);

  // Assert
  EXPECT_THAT(auxiliary_data, testing::Not((cardano_auxiliary_data_t*)nullptr));
  EXPECT_EQ(cardano_auxiliary_data_refcount(auxiliary_data), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_auxiliary_data_unref(&auxiliary_data);
}

TEST(cardano_auxiliary_data_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_auxiliary_data_ref(nullptr);
}

TEST(cardano_auxiliary_data_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = nullptr;

  // Act
  cardano_auxiliary_data_unref(&auxiliary_data);
}

TEST(cardano_auxiliary_data_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_auxiliary_data_unref((cardano_auxiliary_data_t**)nullptr);
}

TEST(cardano_auxiliary_data_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);
  EXPECT_NE(auxiliary_data, nullptr);

  // Act
  cardano_auxiliary_data_ref(auxiliary_data);
  size_t ref_count = cardano_auxiliary_data_refcount(auxiliary_data);

  cardano_auxiliary_data_unref(&auxiliary_data);
  size_t updated_ref_count = cardano_auxiliary_data_refcount(auxiliary_data);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
}

TEST(cardano_auxiliary_data_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);
  EXPECT_NE(auxiliary_data, nullptr);

  // Act
  cardano_auxiliary_data_ref(auxiliary_data);
  size_t ref_count = cardano_auxiliary_data_refcount(auxiliary_data);

  cardano_auxiliary_data_unref(&auxiliary_data);
  size_t updated_ref_count = cardano_auxiliary_data_refcount(auxiliary_data);

  cardano_auxiliary_data_unref(&auxiliary_data);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(auxiliary_data, (cardano_auxiliary_data_t*)nullptr);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
}

TEST(cardano_auxiliary_data_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_auxiliary_data_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_auxiliary_data_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = nullptr;
  const char*               message        = "This is a test message";

  // Act
  cardano_auxiliary_data_set_last_error(auxiliary_data, message);

  // Assert
  EXPECT_STREQ(cardano_auxiliary_data_get_last_error(auxiliary_data), "Object is NULL.");
}

TEST(cardano_auxiliary_data_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);
  EXPECT_NE(auxiliary_data, nullptr);

  const char* message = nullptr;

  // Act
  cardano_auxiliary_data_set_last_error(auxiliary_data, message);

  // Assert
  EXPECT_STREQ(cardano_auxiliary_data_get_last_error(auxiliary_data), "");

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
}

TEST(cardano_auxiliary_data_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = NULL;

  // Act
  cardano_error_t result = cardano_auxiliary_data_from_cbor(nullptr, &auxiliary_data);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_auxiliary_data_from_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(AUXILIARY_DATA_CBOR, strlen(AUXILIARY_DATA_CBOR));

  // Act
  cardano_error_t result = cardano_auxiliary_data_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auxiliary_data_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t*    writer         = cardano_cbor_writer_new();
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);
  EXPECT_NE(auxiliary_data, nullptr);

  // Act
  cardano_error_t result = cardano_auxiliary_data_to_cbor(auxiliary_data, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, AUXILIARY_DATA_CBOR);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_auxiliary_data_to_cbor, canSerializeShelleyEra)
{
  // Arrange
  cardano_cbor_writer_t*    writer         = cardano_cbor_writer_new();
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(SHELLEY_AUXILIARY_DATA_CBOR);
  EXPECT_NE(auxiliary_data, nullptr);

  // Act
  cardano_error_t result = cardano_auxiliary_data_to_cbor(auxiliary_data, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, AUXILIARY_DATA_CBOR2);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_auxiliary_data_to_cbor, canSerializeJustMetadata)
{
  // Arrange
  cardano_cbor_writer_t*    writer         = cardano_cbor_writer_new();
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(JUST_METADATA_AUXILIARY_DATA_CBOR);
  EXPECT_NE(auxiliary_data, nullptr);

  // Act
  cardano_error_t result = cardano_auxiliary_data_to_cbor(auxiliary_data, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, AUXILIARY_DATA_CBOR3);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_auxiliary_data_to_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_auxiliary_data_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_auxiliary_data_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_auxiliary_data_to_cbor((cardano_auxiliary_data_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

// Cert specific tests

TEST(cardano_auxiliary_data_new, canCreateNewInstance)
{
  // Act
  cardano_auxiliary_data_t* auxiliary_data = NULL;

  cardano_error_t result = cardano_auxiliary_data_new(&auxiliary_data);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(auxiliary_data, nullptr);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
}

TEST(cardano_auxiliary_data_new, returnsErrorIfCertIsNull)
{
  // Act

  cardano_error_t result = cardano_auxiliary_data_new(nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
}

TEST(cardano_auxiliary_data_new, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_auxiliary_data_t* auxiliary_data = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_auxiliary_data_new(&auxiliary_data);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_auxiliary_data_from_cbor, returnsErrorIfInvalidCbor)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auxiliary_data_from_cbor, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(AUXILIARY_DATA_CBOR, strlen(AUXILIARY_DATA_CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auxiliary_data_from_cbor, returnsErrorIfMemoryAllocationFails2)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(AUXILIARY_DATA_CBOR, strlen(AUXILIARY_DATA_CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auxiliary_data_from_cbor, returnsErrorIfMemoryAllocationFails3)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(AUXILIARY_DATA_CBOR, strlen(AUXILIARY_DATA_CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_thirty_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auxiliary_data_from_cbor, returnsErrorIfInvalidMetadata)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex("a100ef", strlen("a100ef"));

  // Act
  cardano_error_t result = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auxiliary_data_from_cbor, returnsErrorIfInvalidTag)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  const char*               cbor           = "d90113a500a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b6501848204038205098202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f5402844746010000220010474601000022001147460100002200124746010000220013038447460100002200104746010000220011474601000022001247460100002200130483474601000022001047460100002200114746010000220012";
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_VALUE);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auxiliary_data_from_cbor, returnsErrorIfInvalidMapAlonzo)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  const char*               cbor           = "d90103ef00a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b6501848204038205098202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f5402844746010000220010474601000022001147460100002200124746010000220013038447460100002200104746010000220011474601000022001247460100002200130483474601000022001047460100002200114746010000220012";
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auxiliary_data_from_cbor, returnsErrorIfInvalidMapKeyAlonzo)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  const char*               cbor           = "d90103a5efa11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b6501848204038205098202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f5402844746010000220010474601000022001147460100002200124746010000220013038447460100002200104746010000220011474601000022001247460100002200130483474601000022001047460100002200114746010000220012";
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auxiliary_data_from_cbor, returnsErrorIfWrongMapKeyAlonzo)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  const char*               cbor           = "d90103a509a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b6501848204038205098202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f5402844746010000220010474601000022001147460100002200124746010000220013038447460100002200104746010000220011474601000022001247460100002200130483474601000022001047460100002200114746010000220012";
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_MAP_KEY);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auxiliary_data_from_cbor, returnsErrorIfInvalidMetadataAlonzo)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  const char*               cbor           = "d90103a500ef1902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b6501848204038205098202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f5402844746010000220010474601000022001147460100002200124746010000220013038447460100002200104746010000220011474601000022001247460100002200130483474601000022001047460100002200114746010000220012";
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auxiliary_data_from_cbor, returnsErrorIfInvalidNativeScriptsAlonzo)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  const char*               cbor           = "d90103a500a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b6501ef8204038205098202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f5402844746010000220010474601000022001147460100002200124746010000220013038447460100002200104746010000220011474601000022001247460100002200130483474601000022001047460100002200114746010000220012";
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auxiliary_data_from_cbor, returnsErrorIfInvalidPlutusV1Alonzo)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  const char*               cbor           = "d90103a500a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b6501848204038205098202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f5402ef4746010000220010474601000022001147460100002200124746010000220013038447460100002200104746010000220011474601000022001247460100002200130483474601000022001047460100002200114746010000220012";
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auxiliary_data_from_cbor, returnsErrorIfInvalidPlutusV2Alonzo)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  const char*               cbor           = "d90103a500a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b6501848204038205098202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f540284474601000022001047460100002200114746010000220012474601000022001303ef47460100002200104746010000220011474601000022001247460100002200130483474601000022001047460100002200114746010000220012";
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auxiliary_data_from_cbor, returnsErrorIfInvalidPlutusV3Alonzo)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  const char*               cbor           = "d90103a500a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b6501848204038205098202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f54028447460100002200104746010000220011474601000022001247460100002200130384474601000022001047460100002200114746010000220012474601000022001304ef474601000022001047460100002200114746010000220012";
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auxiliary_data_from_cbor, returnsErrorIfInvalidMetadataShelley)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  const char*               cbor           = "82ef1902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b65828202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f54";
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auxiliary_data_from_cbor, returnsErrorIfInvalidNativeScripts)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  const char*               cbor           = "82a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b6582ef02818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f54";
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auxiliary_data_to_cbor, preservesOriginalCbor)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(SHELLEY_AUXILIARY_DATA_CBOR, strlen(SHELLEY_AUXILIARY_DATA_CBOR));
  cardano_error_t           result         = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);
  cardano_cbor_writer_t*    writer         = cardano_cbor_writer_new();

  EXPECT_THAT(result, CARDANO_SUCCESS);

  // Act
  result = cardano_auxiliary_data_to_cbor(auxiliary_data, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, SHELLEY_AUXILIARY_DATA_CBOR);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_auxiliary_data_get_transaction_metadata, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_transaction_metadata_t* metadata = cardano_auxiliary_data_get_transaction_metadata(nullptr);

  // Assert
  EXPECT_EQ(metadata, nullptr);
}

TEST(cardano_auxiliary_data_get_transaction_metadata, returnsNullIfMetadataIsNotPresent)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  EXPECT_EQ(cardano_auxiliary_data_set_transaction_metadata(auxiliary_data, nullptr), CARDANO_SUCCESS);

  // Act
  cardano_transaction_metadata_t* metadata = cardano_auxiliary_data_get_transaction_metadata(auxiliary_data);

  // Assert
  EXPECT_EQ(metadata, nullptr);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_transaction_metadata_unref(&metadata);
}

TEST(cardano_auxiliary_data_get_transaction_metadata, returnsMetadataIfPresent)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  // Act
  cardano_transaction_metadata_t* metadata = cardano_auxiliary_data_get_transaction_metadata(auxiliary_data);

  // Assert
  EXPECT_NE(metadata, nullptr);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_transaction_metadata_unref(&metadata);
}

TEST(cardano_auxiliary_data_set_transaction_metadata, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_error_t result = cardano_auxiliary_data_set_transaction_metadata(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_auxiliary_data_set_transaction_metadata, canSetMetadataToNull)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  // Act
  cardano_error_t result = cardano_auxiliary_data_set_transaction_metadata(auxiliary_data, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
}

TEST(cardano_auxiliary_data_set_transaction_metadata, canSetMetadata)
{
  // Arrange
  cardano_auxiliary_data_t*       auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);
  cardano_transaction_metadata_t* metadata       = NULL;
  cardano_cbor_reader_t*          cbor_reader    = cardano_cbor_reader_from_hex(TRANSACTION_METADATA_CBOR, strlen(TRANSACTION_METADATA_CBOR));

  cardano_error_t result = cardano_transaction_metadata_from_cbor(cbor_reader, &metadata);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  // Act
  result = cardano_auxiliary_data_set_transaction_metadata(auxiliary_data, metadata);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_transaction_metadata_t* metadata2 = cardano_auxiliary_data_get_transaction_metadata(auxiliary_data);

  EXPECT_EQ(metadata2, metadata);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_transaction_metadata_unref(&metadata);
  cardano_transaction_metadata_unref(&metadata2);
  cardano_cbor_reader_unref(&cbor_reader);
}

TEST(cardano_auxiliary_data_get_native_scripts, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_native_script_list_t* scripts = cardano_auxiliary_data_get_native_scripts(nullptr);

  // Assert
  EXPECT_EQ(scripts, nullptr);
}

TEST(cardano_auxiliary_data_get_native_scripts, returnsNullIfScriptsAreNotPresent)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  EXPECT_EQ(cardano_auxiliary_data_set_native_scripts(auxiliary_data, nullptr), CARDANO_SUCCESS);

  // Act
  cardano_native_script_list_t* scripts = cardano_auxiliary_data_get_native_scripts(auxiliary_data);

  // Assert
  EXPECT_EQ(scripts, nullptr);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_native_script_list_unref(&scripts);
}

TEST(cardano_auxiliary_data_get_native_scripts, returnsScriptsIfPresent)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  // Act
  cardano_native_script_list_t* scripts = cardano_auxiliary_data_get_native_scripts(auxiliary_data);

  // Assert
  EXPECT_NE(scripts, nullptr);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_native_script_list_unref(&scripts);
}

TEST(cardano_auxiliary_data_set_native_scripts, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_error_t result = cardano_auxiliary_data_set_native_scripts(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_auxiliary_data_set_native_scripts, canSetScriptsToNull)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  // Act
  cardano_error_t result = cardano_auxiliary_data_set_native_scripts(auxiliary_data, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
}

TEST(cardano_auxiliary_data_set_native_scripts, canSetScripts)
{
  // Arrange
  cardano_auxiliary_data_t*     auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);
  cardano_native_script_list_t* scripts        = NULL;
  cardano_cbor_reader_t*        cbor_reader    = cardano_cbor_reader_from_hex(NATIVE_SCRIPT_LIST_CBOR, strlen(NATIVE_SCRIPT_LIST_CBOR));

  cardano_error_t result = cardano_native_script_list_from_cbor(cbor_reader, &scripts);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  // Act
  result = cardano_auxiliary_data_set_native_scripts(auxiliary_data, scripts);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_native_script_list_t* scripts2 = cardano_auxiliary_data_get_native_scripts(auxiliary_data);

  EXPECT_EQ(scripts2, scripts);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_native_script_list_unref(&scripts);
  cardano_native_script_list_unref(&scripts2);
  cardano_cbor_reader_unref(&cbor_reader);
}

TEST(cardano_auxiliary_data_get_plutus_v1_scripts, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_plutus_v1_script_list_t* plutus_data = cardano_auxiliary_data_get_plutus_v1_scripts(nullptr);

  // Assert
  EXPECT_EQ(plutus_data, nullptr);
}

TEST(cardano_auxiliary_data_get_plutus_v1_scripts, returnsNullIfScriptsAreNotPresent)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  EXPECT_EQ(cardano_auxiliary_data_set_plutus_v1_scripts(auxiliary_data, nullptr), CARDANO_SUCCESS);

  // Act
  cardano_plutus_v1_script_list_t* plutus_data = cardano_auxiliary_data_get_plutus_v1_scripts(auxiliary_data);

  // Assert
  EXPECT_EQ(plutus_data, nullptr);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_plutus_v1_script_list_unref(&plutus_data);
}

TEST(cardano_auxiliary_data_get_plutus_v1_scripts, returnsScriptsIfPresent)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  // Act
  cardano_plutus_v1_script_list_t* plutus_data = cardano_auxiliary_data_get_plutus_v1_scripts(auxiliary_data);

  // Assert
  EXPECT_NE(plutus_data, nullptr);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_plutus_v1_script_list_unref(&plutus_data);
}

TEST(cardano_auxiliary_data_set_plutus_v1_scripts, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_error_t result = cardano_auxiliary_data_set_plutus_v1_scripts(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_auxiliary_data_set_plutus_v1_scripts, canSetScriptsToNull)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  // Act
  cardano_error_t result = cardano_auxiliary_data_set_plutus_v1_scripts(auxiliary_data, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
}

TEST(cardano_auxiliary_data_set_plutus_v1_scripts, canSetScripts)
{
  // Arrange
  cardano_auxiliary_data_t*        auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);
  cardano_plutus_v1_script_list_t* plutus_data    = NULL;
  cardano_cbor_reader_t*           cbor_reader    = cardano_cbor_reader_from_hex(PLUTUS_V1_LIST_CBOR, strlen(PLUTUS_V1_LIST_CBOR));

  cardano_error_t result = cardano_plutus_v1_script_list_from_cbor(cbor_reader, &plutus_data);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  // Act
  result = cardano_auxiliary_data_set_plutus_v1_scripts(auxiliary_data, plutus_data);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_plutus_v1_script_list_t* plutus_data2 = cardano_auxiliary_data_get_plutus_v1_scripts(auxiliary_data);

  EXPECT_EQ(plutus_data2, plutus_data);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_plutus_v1_script_list_unref(&plutus_data);
  cardano_plutus_v1_script_list_unref(&plutus_data2);
  cardano_cbor_reader_unref(&cbor_reader);
}

TEST(cardano_auxiliary_data_get_plutus_v2_scripts, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_plutus_v2_script_list_t* plutus_data = cardano_auxiliary_data_get_plutus_v2_scripts(nullptr);

  // Assert
  EXPECT_EQ(plutus_data, nullptr);
}

TEST(cardano_auxiliary_data_get_plutus_v2_scripts, returnsNullIfScriptsAreNotPresent)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  EXPECT_EQ(cardano_auxiliary_data_set_plutus_v2_scripts(auxiliary_data, nullptr), CARDANO_SUCCESS);

  // Act
  cardano_plutus_v2_script_list_t* plutus_data = cardano_auxiliary_data_get_plutus_v2_scripts(auxiliary_data);

  // Assert
  EXPECT_EQ(plutus_data, nullptr);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_plutus_v2_script_list_unref(&plutus_data);
}

TEST(cardano_auxiliary_data_get_plutus_v2_scripts, returnsScriptsIfPresent)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  // Act
  cardano_plutus_v2_script_list_t* plutus_data = cardano_auxiliary_data_get_plutus_v2_scripts(auxiliary_data);

  // Assert
  EXPECT_NE(plutus_data, nullptr);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_plutus_v2_script_list_unref(&plutus_data);
}

TEST(cardano_auxiliary_data_set_plutus_v2_scripts, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_error_t result = cardano_auxiliary_data_set_plutus_v2_scripts(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_auxiliary_data_set_plutus_v2_scripts, canSetScriptsToNull)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  // Act
  cardano_error_t result = cardano_auxiliary_data_set_plutus_v2_scripts(auxiliary_data, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
}

TEST(cardano_auxiliary_data_set_plutus_v2_scripts, canSetScripts)
{
  // Arrange
  cardano_auxiliary_data_t*        auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);
  cardano_plutus_v2_script_list_t* plutus_data    = NULL;
  cardano_cbor_reader_t*           cbor_reader    = cardano_cbor_reader_from_hex(PLUTUS_V2_LIST_CBOR, strlen(PLUTUS_V2_LIST_CBOR));

  cardano_error_t result = cardano_plutus_v2_script_list_from_cbor(cbor_reader, &plutus_data);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  // Act
  result = cardano_auxiliary_data_set_plutus_v2_scripts(auxiliary_data, plutus_data);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_plutus_v2_script_list_t* plutus_data2 = cardano_auxiliary_data_get_plutus_v2_scripts(auxiliary_data);

  EXPECT_EQ(plutus_data2, plutus_data);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_plutus_v2_script_list_unref(&plutus_data);
  cardano_plutus_v2_script_list_unref(&plutus_data2);
  cardano_cbor_reader_unref(&cbor_reader);
}

TEST(cardano_auxiliary_data_get_plutus_v3_scripts, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_plutus_v3_script_list_t* plutus_data = cardano_auxiliary_data_get_plutus_v3_scripts(nullptr);

  // Assert
  EXPECT_EQ(plutus_data, nullptr);
}

TEST(cardano_auxiliary_data_get_plutus_v3_scripts, returnsNullIfScriptsAreNotPresent)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  EXPECT_EQ(cardano_auxiliary_data_set_plutus_v3_scripts(auxiliary_data, nullptr), CARDANO_SUCCESS);

  // Act
  cardano_plutus_v3_script_list_t* plutus_data = cardano_auxiliary_data_get_plutus_v3_scripts(auxiliary_data);

  // Assert
  EXPECT_EQ(plutus_data, nullptr);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_plutus_v3_script_list_unref(&plutus_data);
}

TEST(cardano_auxiliary_data_get_plutus_v3_scripts, returnsScriptsIfPresent)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  // Act
  cardano_plutus_v3_script_list_t* plutus_data = cardano_auxiliary_data_get_plutus_v3_scripts(auxiliary_data);

  // Assert
  EXPECT_NE(plutus_data, nullptr);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_plutus_v3_script_list_unref(&plutus_data);
}

TEST(cardano_auxiliary_data_set_plutus_v3_scripts, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_error_t result = cardano_auxiliary_data_set_plutus_v3_scripts(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_auxiliary_data_set_plutus_v3_scripts, canSetScriptsToNull)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  // Act
  cardano_error_t result = cardano_auxiliary_data_set_plutus_v3_scripts(auxiliary_data, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
}

TEST(cardano_auxiliary_data_set_plutus_v3_scripts, canSetScripts)
{
  // Arrange
  cardano_auxiliary_data_t*        auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);
  cardano_plutus_v3_script_list_t* plutus_data    = NULL;
  cardano_cbor_reader_t*           cbor_reader    = cardano_cbor_reader_from_hex(PLUTUS_V3_LIST_CBOR, strlen(PLUTUS_V3_LIST_CBOR));

  cardano_error_t result = cardano_plutus_v3_script_list_from_cbor(cbor_reader, &plutus_data);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  // Act
  result = cardano_auxiliary_data_set_plutus_v3_scripts(auxiliary_data, plutus_data);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_plutus_v3_script_list_t* plutus_data3 = cardano_auxiliary_data_get_plutus_v3_scripts(auxiliary_data);

  EXPECT_EQ(plutus_data3, plutus_data);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_plutus_v3_script_list_unref(&plutus_data);
  cardano_plutus_v3_script_list_unref(&plutus_data3);
  cardano_cbor_reader_unref(&cbor_reader);
}

TEST(cardano_auxiliary_data_get_hash, returnsNullIfTransactionBodyIsNull)
{
  // Act
  const cardano_blake2b_hash_t* hash = cardano_auxiliary_data_get_hash(nullptr);

  // Assert
  EXPECT_EQ(hash, nullptr);
}

TEST(cardano_auxiliary_data_get_hash, returnsHash)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data1 = NULL;
  cardano_auxiliary_data_t* auxiliary_data2 = NULL;
  cardano_auxiliary_data_t* auxiliary_data3 = NULL;

  cardano_cbor_reader_t* reader1 = cardano_cbor_reader_from_hex(AUXILIARY_DATA_CBOR, strlen(AUXILIARY_DATA_CBOR));
  cardano_cbor_reader_t* reader2 = cardano_cbor_reader_from_hex(SHELLEY_AUXILIARY_DATA_CBOR, strlen(SHELLEY_AUXILIARY_DATA_CBOR));
  cardano_cbor_reader_t* reader3 = cardano_cbor_reader_from_hex(JUST_METADATA_AUXILIARY_DATA_CBOR, strlen(JUST_METADATA_AUXILIARY_DATA_CBOR));

  EXPECT_THAT(cardano_auxiliary_data_from_cbor(reader1, &auxiliary_data1), CARDANO_SUCCESS);
  EXPECT_THAT(cardano_auxiliary_data_from_cbor(reader2, &auxiliary_data2), CARDANO_SUCCESS);
  EXPECT_THAT(cardano_auxiliary_data_from_cbor(reader3, &auxiliary_data3), CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_t* hash1 = cardano_auxiliary_data_get_hash(auxiliary_data1);
  cardano_blake2b_hash_t* hash2 = cardano_auxiliary_data_get_hash(auxiliary_data2);
  cardano_blake2b_hash_t* hash3 = cardano_auxiliary_data_get_hash(auxiliary_data3);

  size_t hex_size1 = cardano_blake2b_hash_get_hex_size(hash1);
  char*  hex1      = (char*)malloc(hex_size1);

  size_t hex_size2 = cardano_blake2b_hash_get_hex_size(hash2);
  char*  hex2      = (char*)malloc(hex_size2);

  size_t hex_size3 = cardano_blake2b_hash_get_hex_size(hash3);
  char*  hex3      = (char*)malloc(hex_size3);

  EXPECT_THAT(cardano_blake2b_hash_to_hex(hash1, hex1, hex_size1), CARDANO_SUCCESS);
  EXPECT_THAT(cardano_blake2b_hash_to_hex(hash2, hex2, hex_size2), CARDANO_SUCCESS);
  EXPECT_THAT(cardano_blake2b_hash_to_hex(hash3, hex3, hex_size3), CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(hex1, AUXILIARY_DATA_HASH);
  EXPECT_STREQ(hex2, SHELLEY_AUXILIARY_DATA_HASH);
  EXPECT_STREQ(hex3, JUST_METADATA_AUXILIARY_DATA_HASH);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data1);
  cardano_auxiliary_data_unref(&auxiliary_data2);
  cardano_auxiliary_data_unref(&auxiliary_data3);
  cardano_blake2b_hash_unref(&hash1);
  cardano_blake2b_hash_unref(&hash2);
  cardano_blake2b_hash_unref(&hash3);
  cardano_cbor_reader_unref(&reader1);
  cardano_cbor_reader_unref(&reader2);
  cardano_cbor_reader_unref(&reader3);
  free(hex1);
  free(hex2);
  free(hex3);
}

TEST(cardano_auxiliary_data_clear_cbor_cache, returnsErrorIfTransactionBodyIsNull)
{
  // Act
  cardano_auxiliary_data_clear_cbor_cache(nullptr);

  // Assert
  EXPECT_TRUE(true);
}

TEST(cardano_auxiliary_data_clear_cbor_cache, clearsTheCache)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(SHELLEY_AUXILIARY_DATA_CBOR, strlen(SHELLEY_AUXILIARY_DATA_CBOR));

  EXPECT_THAT(cardano_auxiliary_data_from_cbor(reader, &auxiliary_data), CARDANO_SUCCESS);

  // Act
  // Encode to CBOR and compare
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  EXPECT_THAT(cardano_auxiliary_data_to_cbor(auxiliary_data, writer), CARDANO_SUCCESS);

  size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*  cbor      = (char*)malloc(cbor_size);

  EXPECT_THAT(cardano_cbor_writer_encode_hex(writer, cbor, cbor_size), CARDANO_SUCCESS);
  EXPECT_STREQ(cbor, SHELLEY_AUXILIARY_DATA_CBOR);

  // Clear the cache
  cardano_auxiliary_data_clear_cbor_cache(auxiliary_data);

  // Encode to CBOR and compare
  cardano_cbor_writer_t* writer2 = cardano_cbor_writer_new();

  EXPECT_THAT(cardano_auxiliary_data_to_cbor(auxiliary_data, writer2), CARDANO_SUCCESS);

  size_t cbor_size2 = cardano_cbor_writer_get_hex_size(writer2);
  char*  cbor2      = (char*)malloc(cbor_size2);

  EXPECT_THAT(cardano_cbor_writer_encode_hex(writer2, cbor2, cbor_size2), CARDANO_SUCCESS);

  EXPECT_STREQ(cbor2, AUXILIARY_DATA_CBOR2);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  cardano_cbor_writer_unref(&writer2);
  free(cbor);
  free(cbor2);
}