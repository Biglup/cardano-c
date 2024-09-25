/**
 * \file mir_to_stake_creds_cert.cpp
 *
 * \author angel.castillo
 * \date   Jun 03, 2024
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
#include <cardano/certs/mir_to_stake_creds_cert.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR_USE_RESERVES_TO_CREDS = "8200a18200581c0101010101010101010101010101010101010101010101010101010100";
static const char* CBOR_USE_TREASURY_TO_CREDS = "8201a18200581c0101010101010101010101010101010101010101010101010101010100";
static const char* CREDENTIAL_HASH            = "01010101010101010101010101010101010101010101010101010101";
static const char* CREDENTIAL_HASH2           = "00010101010101010101010101010101010101010101010101010101";
static const char* CREDENTIAL_HASH3           = "ff010101010101010101010101010101010101010101010101010101";

/* UNIT TESTS ****************************************************************/

TEST(cardano_mir_to_stake_creds_cert_from_cbor, canDeserializeToPot)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_CREDS, strlen(CBOR_USE_RESERVES_TO_CREDS));

  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_from_cbor(reader, &mir_to_stake_creds_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(mir_to_stake_creds_cert, nullptr);

  cardano_mir_cert_pot_type_t type;

  EXPECT_EQ(cardano_mir_to_stake_creds_cert_get_pot(mir_to_stake_creds_cert, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_MIR_CERT_POT_TYPE_RESERVE);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_to_stake_creds_cert_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_CREDS, strlen(CBOR_USE_RESERVES_TO_CREDS));
  cardano_error_t                    result                  = cardano_mir_to_stake_creds_cert_from_cbor(reader, &mir_to_stake_creds_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  cardano_mir_to_stake_creds_cert_ref(mir_to_stake_creds_cert);

  // Assert
  EXPECT_THAT(mir_to_stake_creds_cert, testing::Not((cardano_mir_to_stake_creds_cert_t*)nullptr));
  EXPECT_EQ(cardano_mir_to_stake_creds_cert_refcount(mir_to_stake_creds_cert), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_to_stake_creds_cert_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_mir_to_stake_creds_cert_ref(nullptr);
}

TEST(cardano_mir_to_stake_creds_cert_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = nullptr;

  // Act
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
}

TEST(cardano_mir_to_stake_creds_cert_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_mir_to_stake_creds_cert_unref((cardano_mir_to_stake_creds_cert_t**)nullptr);
}

TEST(cardano_mir_to_stake_creds_cert_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_CREDS, strlen(CBOR_USE_RESERVES_TO_CREDS));
  cardano_error_t                    result                  = cardano_mir_to_stake_creds_cert_from_cbor(reader, &mir_to_stake_creds_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);
  // Act
  cardano_mir_to_stake_creds_cert_ref(mir_to_stake_creds_cert);
  size_t ref_count = cardano_mir_to_stake_creds_cert_refcount(mir_to_stake_creds_cert);

  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  size_t updated_ref_count = cardano_mir_to_stake_creds_cert_refcount(mir_to_stake_creds_cert);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_to_stake_creds_cert_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_CREDS, strlen(CBOR_USE_RESERVES_TO_CREDS));
  cardano_error_t                    result                  = cardano_mir_to_stake_creds_cert_from_cbor(reader, &mir_to_stake_creds_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  cardano_mir_to_stake_creds_cert_ref(mir_to_stake_creds_cert);
  size_t ref_count = cardano_mir_to_stake_creds_cert_refcount(mir_to_stake_creds_cert);

  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  size_t updated_ref_count = cardano_mir_to_stake_creds_cert_refcount(mir_to_stake_creds_cert);

  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(mir_to_stake_creds_cert, (cardano_mir_to_stake_creds_cert_t*)nullptr);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_to_stake_creds_cert_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_mir_to_stake_creds_cert_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_mir_to_stake_creds_cert_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = nullptr;
  const char*                        message                 = "This is a test message";

  // Act
  cardano_mir_to_stake_creds_cert_set_last_error(mir_to_stake_creds_cert, message);

  // Assert
  EXPECT_STREQ(cardano_mir_to_stake_creds_cert_get_last_error(mir_to_stake_creds_cert), "Object is NULL.");
}

TEST(cardano_mir_to_stake_creds_cert_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_CREDS, strlen(CBOR_USE_RESERVES_TO_CREDS));
  cardano_error_t                    result                  = cardano_mir_to_stake_creds_cert_from_cbor(reader, &mir_to_stake_creds_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_mir_to_stake_creds_cert_set_last_error(mir_to_stake_creds_cert, message);

  // Assert
  EXPECT_STREQ(cardano_mir_to_stake_creds_cert_get_last_error(mir_to_stake_creds_cert), "");

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_to_stake_creds_cert_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;

  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_from_cbor(nullptr, &mir_to_stake_creds_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_to_stake_creds_cert_from_cbor, returnsErrorIfMirCertIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_CREDS, strlen(CBOR_USE_RESERVES_TO_CREDS));

  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_to_stake_creds_cert_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_from_cbor(reader, &mir_to_stake_creds_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_to_stake_creds_cert_from_cbor, returnsErrorIfInvalidPotType)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("820900", strlen("820900"));

  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_from_cbor(reader, &mir_to_stake_creds_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_VALUE);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_to_stake_creds_cert_from_cbor, returnsErrorIfInvalidCerts)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("8200ef", strlen("8200ef"));

  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_from_cbor(reader, &mir_to_stake_creds_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_to_stake_creds_cert_to_cbor, canSerializeToCreds)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_credential_t*              credential              = NULL;
  cardano_cbor_writer_t*             writer                  = cardano_cbor_writer_new();

  cardano_error_t result = cardano_mir_to_stake_creds_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, &mir_to_stake_creds_cert);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_credential_from_hash_hex(CREDENTIAL_HASH, strlen(CREDENTIAL_HASH), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_EQ(cardano_mir_to_stake_creds_cert_insert(mir_to_stake_creds_cert, credential, 0), CARDANO_SUCCESS);

  // Act
  result = cardano_mir_to_stake_creds_cert_to_cbor(mir_to_stake_creds_cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_USE_TREASURY_TO_CREDS);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_cbor_writer_unref(&writer);
  cardano_credential_unref(&credential);
  free(hex);
}

TEST(cardano_mir_to_stake_creds_cert_to_cbor, returnsErrorIfMirCertIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_mir_to_stake_creds_cert_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_to_cbor((cardano_mir_to_stake_creds_cert_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_to_stake_creds_cert_new, returnsErrorIfMirCertIsNull)
{
  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_to_stake_creds_cert_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, &mir_to_stake_creds_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_mir_to_stake_creds_cert_new, returnsErrorIfMemoryAllocationFails2)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_two_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, &mir_to_stake_creds_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_mir_to_stake_creds_cert_get_pot, returnsErrorIfMirCertIsNull)
{
  // Arrange
  cardano_mir_cert_pot_type_t type;

  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_get_pot(nullptr, &type);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_to_stake_creds_cert_get_pot, returnsErrorIfPotTypeIsNull)
{
  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_get_pot((cardano_mir_to_stake_creds_cert_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_to_stake_creds_cert_from_cbor, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = nullptr;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_CREDS, strlen(CBOR_USE_RESERVES_TO_CREDS));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);
  cardano_error_t result = cardano_mir_to_stake_creds_cert_from_cbor(reader, &mir_to_stake_creds_cert);
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  cardano_cbor_reader_unref(&reader);

  cardano_set_allocators(malloc, realloc, free);
  reader = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_CREDS, strlen(CBOR_USE_RESERVES_TO_CREDS));
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_nine_malloc, realloc, free);
  result = cardano_mir_to_stake_creds_cert_from_cbor(reader, &mir_to_stake_creds_cert);
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  cardano_cbor_reader_unref(&reader);

  cardano_set_allocators(malloc, realloc, free);
  reader = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_CREDS, strlen(CBOR_USE_RESERVES_TO_CREDS));
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_fourteen_malloc, realloc, free);
  result = cardano_mir_to_stake_creds_cert_from_cbor(reader, &mir_to_stake_creds_cert);
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  cardano_cbor_reader_unref(&reader);

  cardano_set_allocators(malloc, realloc, free);
  reader = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_CREDS, strlen(CBOR_USE_RESERVES_TO_CREDS));
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_twenty_nine_malloc, realloc, free);
  result = cardano_mir_to_stake_creds_cert_from_cbor(reader, &mir_to_stake_creds_cert);
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  cardano_cbor_reader_unref(&reader);

  cardano_set_allocators(malloc, realloc, free);
  reader = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_CREDS, strlen(CBOR_USE_RESERVES_TO_CREDS));
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_thirty_malloc, realloc, free);
  result = cardano_mir_to_stake_creds_cert_from_cbor(reader, &mir_to_stake_creds_cert);
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  cardano_cbor_reader_unref(&reader);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_mir_to_stake_creds_cert_set_pot, returnsErrorIfMirCertIsNull)
{
  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_set_pot(nullptr, CARDANO_MIR_CERT_POT_TYPE_TREASURY);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_to_stake_creds_cert_set_pot, canReturnPot)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;

  cardano_error_t result = cardano_mir_to_stake_creds_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, &mir_to_stake_creds_cert);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  cardano_mir_cert_pot_type_t type;
  result = cardano_mir_to_stake_creds_cert_get_pot(mir_to_stake_creds_cert, &type);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(type, CARDANO_MIR_CERT_POT_TYPE_TREASURY);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
}

TEST(cardano_mir_to_stake_creds_cert_get_size, returnsZeroIfMirCertIsNull)
{
  // Act
  size_t size = cardano_mir_to_stake_creds_cert_get_size(nullptr);

  // Assert
  ASSERT_EQ(size, 0);
}

TEST(cardano_mir_to_stake_creds_cert_get_size, returnsTheNumberOfCerts)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_credential_t*              credential              = NULL;

  cardano_error_t result = cardano_mir_to_stake_creds_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, &mir_to_stake_creds_cert);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_credential_from_hash_hex(CREDENTIAL_HASH, strlen(CREDENTIAL_HASH), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_EQ(cardano_mir_to_stake_creds_cert_insert(mir_to_stake_creds_cert, credential, 0), CARDANO_SUCCESS);

  // Act
  size_t size = cardano_mir_to_stake_creds_cert_get_size(mir_to_stake_creds_cert);

  // Assert
  ASSERT_EQ(size, 1);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_credential_unref(&credential);
}

TEST(cardano_mir_to_stake_creds_cert_insert, returnsErrorIfMirCertIsNull)
{
  // Arrange
  cardano_credential_t* credential = NULL;

  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_insert(nullptr, credential, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_to_stake_creds_cert_insert, returnsErrorIfCredentialIsNull)
{
  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_insert((cardano_mir_to_stake_creds_cert_t*)"", nullptr, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_to_stake_creds_cert_insert, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_credential_t*              credential              = NULL;

  cardano_error_t result = cardano_mir_to_stake_creds_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, &mir_to_stake_creds_cert);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_credential_from_hash_hex(CREDENTIAL_HASH, strlen(CREDENTIAL_HASH), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_mir_to_stake_creds_cert_insert(mir_to_stake_creds_cert, credential, 0);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_credential_unref(&credential);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_mir_to_stake_creds_cert_get_key_at, returnsErrorIfMirCertIsNull)
{
  // Arrange
  cardano_credential_t* credential = NULL;

  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_get_key_at(nullptr, 0, &credential);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_to_stake_creds_cert_get_key_at, returnsErrorIfCredentialIsNull)
{
  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_get_key_at((cardano_mir_to_stake_creds_cert_t*)"", 0, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_to_stake_creds_cert_get_key_at, returnsErrorIfIndexOutOfBounds)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_credential_t*              credential              = NULL;

  cardano_error_t result = cardano_mir_to_stake_creds_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, &mir_to_stake_creds_cert);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_credential_from_hash_hex(CREDENTIAL_HASH, strlen(CREDENTIAL_HASH), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_EQ(cardano_mir_to_stake_creds_cert_insert(mir_to_stake_creds_cert, credential, 0), CARDANO_SUCCESS);

  // Act
  cardano_credential_t* key = NULL;
  result                    = cardano_mir_to_stake_creds_cert_get_key_at(mir_to_stake_creds_cert, 1, &key);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_credential_unref(&credential);
  cardano_credential_unref(&key);
}

TEST(cardano_mir_to_stake_creds_cert_get_key_at, canGetTheKey)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_credential_t*              credential              = NULL;

  cardano_error_t result = cardano_mir_to_stake_creds_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, &mir_to_stake_creds_cert);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_credential_from_hash_hex(CREDENTIAL_HASH, strlen(CREDENTIAL_HASH), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_EQ(cardano_mir_to_stake_creds_cert_insert(mir_to_stake_creds_cert, credential, 0), CARDANO_SUCCESS);

  // Act
  cardano_credential_t* key = NULL;
  result                    = cardano_mir_to_stake_creds_cert_get_key_at(mir_to_stake_creds_cert, 0, &key);

  const char* hash = cardano_credential_get_hash_hex(key);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  EXPECT_STREQ(hash, CREDENTIAL_HASH);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_credential_unref(&credential);
  cardano_credential_unref(&key);
}

TEST(cardano_mir_to_stake_creds_cert_get_value_at, returnsErrorIfMirCertIsNull)
{
  // Arrange
  uint64_t val;

  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_get_value_at(nullptr, 0, &val);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_to_stake_creds_cert_get_value_at, returnsErrorIfValueIsNull)
{
  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_get_value_at((cardano_mir_to_stake_creds_cert_t*)"", 0, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_to_stake_creds_cert_get_value_at, returnsErrorIfIndexOutOfBounds)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_credential_t*              credential              = NULL;

  cardano_error_t result = cardano_mir_to_stake_creds_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, &mir_to_stake_creds_cert);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_credential_from_hash_hex(CREDENTIAL_HASH, strlen(CREDENTIAL_HASH), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_EQ(cardano_mir_to_stake_creds_cert_insert(mir_to_stake_creds_cert, credential, 0), CARDANO_SUCCESS);

  // Act
  uint64_t val;
  result = cardano_mir_to_stake_creds_cert_get_value_at(mir_to_stake_creds_cert, 1, &val);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_credential_unref(&credential);
}

TEST(cardano_mir_to_stake_creds_cert_get_value_at, canGetValue)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_credential_t*              credential              = NULL;

  cardano_error_t result = cardano_mir_to_stake_creds_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, &mir_to_stake_creds_cert);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_credential_from_hash_hex(CREDENTIAL_HASH, strlen(CREDENTIAL_HASH), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_EQ(cardano_mir_to_stake_creds_cert_insert(mir_to_stake_creds_cert, credential, 100), CARDANO_SUCCESS);

  // Act
  uint64_t val;
  result = cardano_mir_to_stake_creds_cert_get_value_at(mir_to_stake_creds_cert, 0, &val);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(val, 100);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_credential_unref(&credential);
}

TEST(cardano_mir_to_stake_creds_cert_get_key_value_at, returnsErrorIfMirCertIsNull)
{
  // Arrange
  cardano_credential_t* credential = NULL;
  uint64_t              val;

  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_get_key_value_at(nullptr, 0, &credential, &val);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_to_stake_creds_cert_get_key_value_at, returnsErrorIfCredentialIsNull)
{
  // Act
  uint64_t        val;
  cardano_error_t result = cardano_mir_to_stake_creds_cert_get_key_value_at((cardano_mir_to_stake_creds_cert_t*)"", 0, nullptr, &val);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_to_stake_creds_cert_get_key_value_at, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_credential_t* credential = NULL;

  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_get_key_value_at((cardano_mir_to_stake_creds_cert_t*)"", 0, &credential, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_to_stake_creds_cert_get_key_value_at, returnsErrorIfIndexOutOfBounds)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_credential_t*              credential              = NULL;

  cardano_error_t result = cardano_mir_to_stake_creds_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, &mir_to_stake_creds_cert);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_credential_from_hash_hex(CREDENTIAL_HASH, strlen(CREDENTIAL_HASH), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_EQ(cardano_mir_to_stake_creds_cert_insert(mir_to_stake_creds_cert, credential, 100), CARDANO_SUCCESS);

  // Act
  cardano_credential_t* key = NULL;
  uint64_t              val;
  result = cardano_mir_to_stake_creds_cert_get_key_value_at(mir_to_stake_creds_cert, 1, &key, &val);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_credential_unref(&credential);
  cardano_credential_unref(&key);
}

TEST(cardano_mir_to_stake_creds_cert_get_key_value_at, canGetKeyAndValue)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_credential_t*              credential              = NULL;

  cardano_error_t result = cardano_mir_to_stake_creds_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, &mir_to_stake_creds_cert);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_credential_from_hash_hex(CREDENTIAL_HASH, strlen(CREDENTIAL_HASH), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_EQ(cardano_mir_to_stake_creds_cert_insert(mir_to_stake_creds_cert, credential, 100), CARDANO_SUCCESS);

  // Act
  cardano_credential_t* key = NULL;
  uint64_t              val;
  result = cardano_mir_to_stake_creds_cert_get_key_value_at(mir_to_stake_creds_cert, 0, &key, &val);

  const char* hash = cardano_credential_get_hash_hex(key);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  EXPECT_STREQ(hash, CREDENTIAL_HASH);
  ASSERT_EQ(val, 100);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_credential_unref(&credential);
  cardano_credential_unref(&key);
}

TEST(cardano_mir_to_stake_creds_cert_set_pot, canSetPot)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;

  cardano_error_t result = cardano_mir_to_stake_creds_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, &mir_to_stake_creds_cert);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_mir_to_stake_creds_cert_set_pot(mir_to_stake_creds_cert, CARDANO_MIR_CERT_POT_TYPE_RESERVE);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_mir_cert_pot_type_t type;
  result = cardano_mir_to_stake_creds_cert_get_pot(mir_to_stake_creds_cert, &type);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_EQ(type, CARDANO_MIR_CERT_POT_TYPE_RESERVE);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
}

TEST(cardano_mir_to_stake_creds_cert_insert, keysAreKeptSortedAtInsertion)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_credential_t*              credential1             = NULL;
  cardano_credential_t*              credential2             = NULL;
  cardano_credential_t*              credential3             = NULL;

  cardano_error_t result = cardano_mir_to_stake_creds_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, &mir_to_stake_creds_cert);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_credential_from_hash_hex(CREDENTIAL_HASH, strlen(CREDENTIAL_HASH), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential1);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_credential_from_hash_hex(CREDENTIAL_HASH2, strlen(CREDENTIAL_HASH2), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential2);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_credential_from_hash_hex(CREDENTIAL_HASH3, strlen(CREDENTIAL_HASH3), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential3);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_mir_to_stake_creds_cert_insert(mir_to_stake_creds_cert, credential1, 100), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_mir_to_stake_creds_cert_insert(mir_to_stake_creds_cert, credential2, 200), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_mir_to_stake_creds_cert_insert(mir_to_stake_creds_cert, credential3, 300), CARDANO_SUCCESS);

  // Assert
  cardano_credential_t* key = NULL;
  uint64_t              val;
  result = cardano_mir_to_stake_creds_cert_get_key_value_at(mir_to_stake_creds_cert, 0, &key, &val);
  ASSERT_EQ(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_credential_get_hash_hex(key), CREDENTIAL_HASH2);
  ASSERT_EQ(val, 200);
  cardano_credential_unref(&key);

  result = cardano_mir_to_stake_creds_cert_get_key_value_at(mir_to_stake_creds_cert, 1, &key, &val);
  ASSERT_EQ(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_credential_get_hash_hex(key), CREDENTIAL_HASH);
  ASSERT_EQ(val, 100);
  cardano_credential_unref(&key);

  result = cardano_mir_to_stake_creds_cert_get_key_value_at(mir_to_stake_creds_cert, 2, &key, &val);
  ASSERT_EQ(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_credential_get_hash_hex(key), CREDENTIAL_HASH3);
  ASSERT_EQ(val, 300);
  cardano_credential_unref(&key);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_credential_unref(&credential1);
  cardano_credential_unref(&credential2);
  cardano_credential_unref(&credential3);
}
