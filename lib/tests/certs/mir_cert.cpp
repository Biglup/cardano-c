/**
 * \file mir_cert.cpp
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
#include <cardano/certs/mir_cert.h>

#include "../json_helpers.h"
#include "tests/allocators_helpers.h"

#include <allocators.h>
#include <cardano/certs/mir_cert_type.h>
#include <cardano/certs/mir_to_pot_cert.h>
#include <cardano/certs/mir_to_stake_creds_cert.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR_USE_RESERVES_TO_POT   = "820682001a000f4240";
static const char* CBOR_USE_TREASURY_TO_POT   = "820682011a000f4240";
static const char* CBOR_USE_RESERVES_TO_CREDS = "82068200a18200581c0101010101010101010101010101010101010101010101010101010100";
static const char* CBOR_USE_TREASURY_TO_CREDS = "82068201a18200581c0101010101010101010101010101010101010101010101010101010100";
static const char* CREDENTIAL_HASH            = "01010101010101010101010101010101010101010101010101010101";

/* UNIT TESTS ****************************************************************/

TEST(cardano_mir_cert_from_cbor, canDeserializeToPot)
{
  // Arrange
  cardano_mir_cert_t*    mir_cert = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_POT, strlen(CBOR_USE_RESERVES_TO_POT));

  // Act
  cardano_error_t result = cardano_mir_cert_from_cbor(reader, &mir_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(mir_cert, nullptr);

  cardano_mir_cert_type_t type;

  EXPECT_EQ(cardano_mir_cert_get_type(mir_cert, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_MIR_CERT_TYPE_TO_POT);

  // Cleanup
  cardano_mir_cert_unref(&mir_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_cert_from_cbor, canDeserializeToCreds)
{
  // Arrange
  cardano_mir_cert_t*    mir_cert = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_CREDS, strlen(CBOR_USE_RESERVES_TO_CREDS));

  // Act
  cardano_error_t result = cardano_mir_cert_from_cbor(reader, &mir_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(mir_cert, nullptr);

  cardano_mir_cert_type_t type;

  EXPECT_EQ(cardano_mir_cert_get_type(mir_cert, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_MIR_CERT_TYPE_TO_STAKE_CREDS);

  // Cleanup
  cardano_mir_cert_unref(&mir_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_cert_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_mir_cert_t*    mir_cert = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_POT, strlen(CBOR_USE_RESERVES_TO_POT));
  cardano_error_t        result   = cardano_mir_cert_from_cbor(reader, &mir_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  cardano_mir_cert_ref(mir_cert);

  // Assert
  EXPECT_THAT(mir_cert, testing::Not((cardano_mir_cert_t*)nullptr));
  EXPECT_EQ(cardano_mir_cert_refcount(mir_cert), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_mir_cert_unref(&mir_cert);
  cardano_mir_cert_unref(&mir_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_cert_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_mir_cert_ref(nullptr);
}

TEST(cardano_mir_cert_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_mir_cert_t* mir_cert = nullptr;

  // Act
  cardano_mir_cert_unref(&mir_cert);
}

TEST(cardano_mir_cert_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_mir_cert_unref((cardano_mir_cert_t**)nullptr);
}

TEST(cardano_mir_cert_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_mir_cert_t*    mir_cert = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_POT, strlen(CBOR_USE_RESERVES_TO_POT));
  cardano_error_t        result   = cardano_mir_cert_from_cbor(reader, &mir_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);
  // Act
  cardano_mir_cert_ref(mir_cert);
  size_t ref_count = cardano_mir_cert_refcount(mir_cert);

  cardano_mir_cert_unref(&mir_cert);
  size_t updated_ref_count = cardano_mir_cert_refcount(mir_cert);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_mir_cert_unref(&mir_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_cert_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_mir_cert_t*    mir_cert = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_POT, strlen(CBOR_USE_RESERVES_TO_POT));
  cardano_error_t        result   = cardano_mir_cert_from_cbor(reader, &mir_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  cardano_mir_cert_ref(mir_cert);
  size_t ref_count = cardano_mir_cert_refcount(mir_cert);

  cardano_mir_cert_unref(&mir_cert);
  size_t updated_ref_count = cardano_mir_cert_refcount(mir_cert);

  cardano_mir_cert_unref(&mir_cert);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(mir_cert, (cardano_mir_cert_t*)nullptr);

  // Cleanup
  cardano_mir_cert_unref(&mir_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_cert_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_mir_cert_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_mir_cert_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_mir_cert_t* mir_cert = nullptr;
  const char*         message  = "This is a test message";

  // Act
  cardano_mir_cert_set_last_error(mir_cert, message);

  // Assert
  EXPECT_STREQ(cardano_mir_cert_get_last_error(mir_cert), "Object is NULL.");
}

TEST(cardano_mir_cert_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_mir_cert_t*    mir_cert = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_POT, strlen(CBOR_USE_RESERVES_TO_POT));
  cardano_error_t        result   = cardano_mir_cert_from_cbor(reader, &mir_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_mir_cert_set_last_error(mir_cert, message);

  // Assert
  EXPECT_STREQ(cardano_mir_cert_get_last_error(mir_cert), "");

  // Cleanup
  cardano_mir_cert_unref(&mir_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_cert_new_to_other_pot, canCreateNewToPot)
{
  // Arrange
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = NULL;
  cardano_mir_cert_t*        mir_cert        = NULL;
  cardano_error_t            result          = cardano_mir_to_pot_cert_new(CARDANO_MIR_CERT_POT_TYPE_RESERVE, 1000000000, &mir_to_pot_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_mir_cert_new_to_other_pot(mir_to_pot_cert, &mir_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(mir_to_pot_cert, nullptr);

  // Cleanup
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
  cardano_mir_cert_unref(&mir_cert);
}

TEST(cardano_mir_cert_new_to_other_pot, returnsErrorIfToOtherPotCertIsNull)
{
  // Arrange
  cardano_mir_cert_t* mir_cert = NULL;

  // Act
  cardano_error_t result = cardano_mir_cert_new_to_other_pot(nullptr, &mir_cert);
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  result = cardano_mir_cert_new_to_other_pot((cardano_mir_to_pot_cert_t*)"", NULL);
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_cert_new_to_stake_creds, canCreateNewToCreds)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_mir_cert_t*                mir_cert                = NULL;
  cardano_error_t                    result                  = cardano_mir_to_stake_creds_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, &mir_to_stake_creds_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_mir_cert_new_to_stake_creds(mir_to_stake_creds_cert, &mir_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(mir_to_stake_creds_cert, nullptr);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_mir_cert_unref(&mir_cert);
}

TEST(cardano_mir_cert_new_to_stake_creds, returnsErrorIfToStakeCredsCertIsNull)
{
  // Arrange
  cardano_mir_cert_t* mir_cert = NULL;

  // Act
  cardano_error_t result = cardano_mir_cert_new_to_stake_creds(nullptr, &mir_cert);
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  result = cardano_mir_cert_new_to_stake_creds((cardano_mir_to_stake_creds_cert_t*)"", NULL);
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_cert_new_to_other_pot, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = NULL;
  cardano_mir_cert_t*        mir_cert        = NULL;

  // Act
  cardano_error_t result = cardano_mir_to_pot_cert_new(CARDANO_MIR_CERT_POT_TYPE_RESERVE, 1000000000, &mir_to_pot_cert);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_mir_cert_new_to_other_pot(mir_to_pot_cert, &mir_cert);
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
  cardano_mir_cert_unref(&mir_cert);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_mir_cert_new_to_stake_creds, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_mir_cert_t*                mir_cert                = NULL;

  // Act
  cardano_error_t result = cardano_mir_to_stake_creds_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, &mir_to_stake_creds_cert);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_mir_cert_new_to_stake_creds(mir_to_stake_creds_cert, &mir_cert);
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_mir_cert_unref(&mir_cert);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_mir_cert_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_mir_cert_t* mir_cert = NULL;

  // Act
  cardano_error_t result = cardano_mir_cert_from_cbor(nullptr, &mir_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_cert_from_cbor, returnsErrorIfMirCertIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_POT, strlen(CBOR_USE_RESERVES_TO_POT));

  // Act
  cardano_error_t result = cardano_mir_cert_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_cert_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_mir_cert_t*    mir_cert = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_mir_cert_from_cbor(reader, &mir_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_mir_cert_unref(&mir_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_cert_from_cbor, returnsErrorIfCertTypeIsNotMir)
{
  // Arrange
  cardano_mir_cert_t*    mir_cert = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex("820900", strlen("820900"));

  // Act
  cardano_error_t result = cardano_mir_cert_from_cbor(reader, &mir_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_VALUE);

  // Cleanup
  cardano_mir_cert_unref(&mir_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_cert_from_cbor, returnsErrorIfCertContentIsNotArray)
{
  // Arrange
  cardano_mir_cert_t*    mir_cert = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex("820600", strlen("820600"));

  // Act
  cardano_error_t result = cardano_mir_cert_from_cbor(reader, &mir_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_mir_cert_unref(&mir_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_cert_from_cbor, returnsErrorIfInvalidPotType)
{
  // Arrange
  cardano_mir_cert_t*    mir_cert = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex("8206820909", strlen("8206820909"));

  // Act
  cardano_error_t result = cardano_mir_cert_from_cbor(reader, &mir_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_VALUE);

  // Cleanup
  cardano_mir_cert_unref(&mir_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_cert_from_cbor, returnsErrorIfInvalidToPotCert)
{
  // Arrange
  cardano_mir_cert_t*    mir_cert = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex("82068200", strlen("82068200"));

  // Act
  cardano_error_t result = cardano_mir_cert_from_cbor(reader, &mir_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_mir_cert_unref(&mir_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_cert_from_cbor, returnsErrorIfInvalidToPotCert2)
{
  // Arrange
  cardano_mir_cert_t*    mir_cert = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex("82068200fe", strlen("82068200fe"));

  // Act
  cardano_error_t result = cardano_mir_cert_from_cbor(reader, &mir_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_mir_cert_unref(&mir_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_cert_from_cbor, returnsErrorIfInvalidToCreds)
{
  // Arrange
  cardano_mir_cert_t*    mir_cert = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex("8206820182", strlen("8206820182"));

  // Act
  cardano_error_t result = cardano_mir_cert_from_cbor(reader, &mir_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_mir_cert_unref(&mir_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_cert_to_cbor, canSerializeToPot)
{
  // Arrange
  cardano_mir_cert_t*    mir_cert = NULL;
  cardano_cbor_writer_t* writer   = cardano_cbor_writer_new();

  cardano_mir_to_pot_cert_t* mir_to_pot_cert = NULL;
  cardano_error_t            result          = cardano_mir_to_pot_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, 1000000, &mir_to_pot_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_mir_cert_new_to_other_pot(mir_to_pot_cert, &mir_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_mir_cert_to_cbor(mir_cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_USE_TREASURY_TO_POT);

  // Cleanup
  cardano_mir_cert_unref(&mir_cert);
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_mir_cert_to_cbor, canSerializeToCreds)
{
  // Arrange
  cardano_mir_cert_t*                mir_cert                = NULL;
  cardano_cbor_writer_t*             writer                  = cardano_cbor_writer_new();
  cardano_credential_t*              credential              = NULL;
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_error_t                    result                  = cardano_mir_to_stake_creds_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, &mir_to_stake_creds_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  ASSERT_EQ(cardano_credential_from_hash_hex(CREDENTIAL_HASH, strlen(CREDENTIAL_HASH), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential), CARDANO_SUCCESS);

  ASSERT_EQ(cardano_mir_to_stake_creds_cert_insert(mir_to_stake_creds_cert, credential, 0), CARDANO_SUCCESS);

  result = cardano_mir_cert_new_to_stake_creds(mir_to_stake_creds_cert, &mir_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_mir_cert_to_cbor(mir_cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_USE_TREASURY_TO_CREDS);

  // Cleanup
  cardano_mir_cert_unref(&mir_cert);
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_cbor_writer_unref(&writer);
  cardano_credential_unref(&credential);
  free(hex);
}

TEST(cardano_mir_cert_to_cbor, returnsErrorIfMirCertIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_mir_cert_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_mir_cert_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_mir_cert_to_cbor((cardano_mir_cert_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_cert_get_type, returnsErrorIfMirCertIsNull)
{
  // Arrange
  cardano_mir_cert_type_t type;

  // Act
  cardano_error_t result = cardano_mir_cert_get_type(nullptr, &type);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_cert_get_type, returnsErrorIfTypeIsNull)
{
  // Act
  cardano_error_t result = cardano_mir_cert_get_type((cardano_mir_cert_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_cert_as_to_other_pot, returnsErrorIfMirCertIsNull)
{
  // Arrange
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = NULL;

  // Act
  cardano_error_t result = cardano_mir_cert_as_to_other_pot(nullptr, &mir_to_pot_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  result = cardano_mir_cert_as_to_other_pot((cardano_mir_cert_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_cert_as_to_other_pot, returnsErrorIfMirCertIsNotToPot)
{
  // Arrange
  cardano_mir_cert_t*        mir_cert        = NULL;
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = NULL;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_CREDS, strlen(CBOR_USE_RESERVES_TO_CREDS));

  cardano_error_t result = cardano_mir_cert_from_cbor(reader, &mir_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_mir_cert_as_to_other_pot(mir_cert, &mir_to_pot_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_mir_cert_unref(&mir_cert);
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_cert_as_to_stake_creds, returnsErrorIfMirCertIsNull)
{
  // Arrange
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;

  // Act
  cardano_error_t result = cardano_mir_cert_as_to_stake_creds(nullptr, &mir_to_stake_creds_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  result = cardano_mir_cert_as_to_stake_creds((cardano_mir_cert_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_mir_cert_as_to_stake_creds, returnsErrorIfMirCertIsNotToCreds)
{
  // Arrange
  cardano_mir_cert_t*                mir_cert                = NULL;
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_POT, strlen(CBOR_USE_RESERVES_TO_POT));

  cardano_error_t result = cardano_mir_cert_from_cbor(reader, &mir_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_mir_cert_as_to_stake_creds(mir_cert, &mir_to_stake_creds_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_mir_cert_unref(&mir_cert);
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_cert_as_to_other_pot, canGetToOtherPotCert)
{
  // Arrange
  cardano_mir_cert_t*        mir_cert        = NULL;
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = NULL;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_POT, strlen(CBOR_USE_RESERVES_TO_POT));

  cardano_error_t result = cardano_mir_cert_from_cbor(reader, &mir_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_mir_cert_as_to_other_pot(mir_cert, &mir_to_pot_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(mir_to_pot_cert, nullptr);

  // Cleanup
  cardano_mir_cert_unref(&mir_cert);
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_cert_as_to_stake_creds, canGetToStakeCredsCert)
{
  // Arrange
  cardano_mir_cert_t*                mir_cert                = NULL;
  cardano_mir_to_stake_creds_cert_t* mir_to_stake_creds_cert = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_CREDS, strlen(CBOR_USE_RESERVES_TO_CREDS));

  cardano_error_t result = cardano_mir_cert_from_cbor(reader, &mir_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_mir_cert_as_to_stake_creds(mir_cert, &mir_to_stake_creds_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(mir_to_stake_creds_cert, nullptr);

  // Cleanup
  cardano_mir_cert_unref(&mir_cert);
  cardano_mir_to_stake_creds_cert_unref(&mir_to_stake_creds_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_cert_to_cip116_json, canConvertToCredsToCip116Json)
{
  // Arrange
  cardano_mir_cert_t*    mir_cert = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_CREDS, strlen(CBOR_USE_RESERVES_TO_CREDS));

  cardano_error_t result = cardano_mir_cert_from_cbor(reader, &mir_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  cardano_error_t error    = cardano_mir_cert_to_cip116_json(mir_cert, json);
  char*           json_str = encode_json(json);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(json_str, R"({"tag":"to_stake_creds","pot":"reserves","rewards":[{"key":{"tag":"pubkey_hash","value":"01010101010101010101010101010101010101010101010101010101"},"value":"0"}]})");

  // Cleanup
  cardano_json_writer_unref(&json);
  cardano_mir_cert_unref(&mir_cert);
  cardano_cbor_reader_unref(&reader);
  free(json_str);
}

TEST(cardano_mir_cert_to_cip116_json, canConvertToPotToCip116Json)
{
  // Arrange
  cardano_mir_cert_t*    mir_cert = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_POT, strlen(CBOR_USE_RESERVES_TO_POT));

  cardano_error_t result = cardano_mir_cert_from_cbor(reader, &mir_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  cardano_error_t error    = cardano_mir_cert_to_cip116_json(mir_cert, json);
  char*           json_str = encode_json(json);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(json_str, R"({"tag":"to_other_pot","pot":"reserves","amount":"1000000"})");

  // Cleanup
  cardano_json_writer_unref(&json);
  cardano_mir_cert_unref(&mir_cert);
  cardano_cbor_reader_unref(&reader);
  free(json_str);
}

TEST(cardano_mir_cert_to_cip116_json, returnsErrorIfMirCertIsNull)
{
  // Arrange
  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  cardano_error_t result = cardano_mir_cert_to_cip116_json(nullptr, json);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_json_writer_unref(&json);
}

TEST(cardano_mir_cert_to_cip116_json, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_mir_cert_to_cip116_json((cardano_mir_cert_t*)"", nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}