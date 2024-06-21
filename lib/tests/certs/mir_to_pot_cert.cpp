/**
 * \file mir_to_pot_cert.cpp
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
#include <cardano/certs/mir_to_pot_cert.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>
#include <cardano/certs/mir_to_pot_cert.h>
#include <cardano/certs/mir_to_stake_creds_cert.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR_USE_RESERVES_TO_POT = "82001a000f4240";
static const char* CBOR_USE_TREASURY_TO_POT = "82011a000f4240";

/* UNIT TESTS ****************************************************************/

TEST(cardano_mir_to_pot_cert_from_cbor, canDeserializeToPot)
{
  // Arrange
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = NULL;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_POT, strlen(CBOR_USE_RESERVES_TO_POT));

  // Act
  cardano_error_t result = cardano_mir_to_pot_cert_from_cbor(reader, &mir_to_pot_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(mir_to_pot_cert, nullptr);

  cardano_mir_cert_pot_type_t type;

  EXPECT_EQ(cardano_mir_to_pot_cert_get_pot(mir_to_pot_cert, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_MIR_CERT_POT_TYPE_RESERVE);

  // Cleanup
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_to_pot_cert_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = NULL;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_POT, strlen(CBOR_USE_RESERVES_TO_POT));
  cardano_error_t            result          = cardano_mir_to_pot_cert_from_cbor(reader, &mir_to_pot_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  cardano_mir_to_pot_cert_ref(mir_to_pot_cert);

  // Assert
  EXPECT_THAT(mir_to_pot_cert, testing::Not((cardano_mir_to_pot_cert_t*)nullptr));
  EXPECT_EQ(cardano_mir_to_pot_cert_refcount(mir_to_pot_cert), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_to_pot_cert_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_mir_to_pot_cert_ref(nullptr);
}

TEST(cardano_mir_to_pot_cert_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = nullptr;

  // Act
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
}

TEST(cardano_mir_to_pot_cert_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_mir_to_pot_cert_unref((cardano_mir_to_pot_cert_t**)nullptr);
}

TEST(cardano_mir_to_pot_cert_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = NULL;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_POT, strlen(CBOR_USE_RESERVES_TO_POT));
  cardano_error_t            result          = cardano_mir_to_pot_cert_from_cbor(reader, &mir_to_pot_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);
  // Act
  cardano_mir_to_pot_cert_ref(mir_to_pot_cert);
  size_t ref_count = cardano_mir_to_pot_cert_refcount(mir_to_pot_cert);

  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
  size_t updated_ref_count = cardano_mir_to_pot_cert_refcount(mir_to_pot_cert);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_to_pot_cert_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = NULL;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_POT, strlen(CBOR_USE_RESERVES_TO_POT));
  cardano_error_t            result          = cardano_mir_to_pot_cert_from_cbor(reader, &mir_to_pot_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  cardano_mir_to_pot_cert_ref(mir_to_pot_cert);
  size_t ref_count = cardano_mir_to_pot_cert_refcount(mir_to_pot_cert);

  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
  size_t updated_ref_count = cardano_mir_to_pot_cert_refcount(mir_to_pot_cert);

  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(mir_to_pot_cert, (cardano_mir_to_pot_cert_t*)nullptr);

  // Cleanup
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_to_pot_cert_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_mir_to_pot_cert_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_mir_to_pot_cert_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = nullptr;
  const char*                message         = "This is a test message";

  // Act
  cardano_mir_to_pot_cert_set_last_error(mir_to_pot_cert, message);

  // Assert
  EXPECT_STREQ(cardano_mir_to_pot_cert_get_last_error(mir_to_pot_cert), "Object is NULL.");
}

TEST(cardano_mir_to_pot_cert_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = NULL;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_POT, strlen(CBOR_USE_RESERVES_TO_POT));
  cardano_error_t            result          = cardano_mir_to_pot_cert_from_cbor(reader, &mir_to_pot_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_mir_to_pot_cert_set_last_error(mir_to_pot_cert, message);

  // Assert
  EXPECT_STREQ(cardano_mir_to_pot_cert_get_last_error(mir_to_pot_cert), "");

  // Cleanup
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_to_pot_cert_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = NULL;

  // Act
  cardano_error_t result = cardano_mir_to_pot_cert_from_cbor(nullptr, &mir_to_pot_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_mir_to_pot_cert_from_cbor, returnsErrorIfMirCertIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_USE_RESERVES_TO_POT, strlen(CBOR_USE_RESERVES_TO_POT));

  // Act
  cardano_error_t result = cardano_mir_to_pot_cert_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_to_pot_cert_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = NULL;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_mir_to_pot_cert_from_cbor(reader, &mir_to_pot_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_to_pot_cert_from_cbor, returnsErrorIfInvalidPotType)
{
  // Arrange
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = NULL;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex("820900", strlen("820900"));

  // Act
  cardano_error_t result = cardano_mir_to_pot_cert_from_cbor(reader, &mir_to_pot_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_VALUE);

  // Cleanup
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_to_pot_cert_from_cbor, returnsErrorIfInvalidAmount)
{
  // Arrange
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = NULL;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex("8200ef", strlen("8200ef"));

  // Act
  cardano_error_t result = cardano_mir_to_pot_cert_from_cbor(reader, &mir_to_pot_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_mir_to_pot_cert_to_cbor, canSerializeToPot)
{
  // Arrange
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = NULL;
  cardano_cbor_writer_t*     writer          = cardano_cbor_writer_new();

  cardano_error_t result = cardano_mir_to_pot_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, 1000000, &mir_to_pot_cert);
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_mir_to_pot_cert_to_cbor(mir_to_pot_cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_USE_TREASURY_TO_POT);

  // Cleanup
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_mir_to_pot_cert_to_cbor, returnsErrorIfMirCertIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_mir_to_pot_cert_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_mir_to_pot_cert_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_mir_to_pot_cert_to_cbor((cardano_mir_to_pot_cert_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_mir_to_pot_cert_new, returnsErrorIfMirCertIsNull)
{
  // Act
  cardano_error_t result = cardano_mir_to_pot_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, 1000000, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_mir_to_pot_cert_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_mir_to_pot_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, 1000000, &mir_to_pot_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_mir_to_pot_cert_get_pot, returnsErrorIfMirCertIsNull)
{
  // Arrange
  cardano_mir_cert_pot_type_t type;

  // Act
  cardano_error_t result = cardano_mir_to_pot_cert_get_pot(nullptr, &type);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_mir_to_pot_cert_get_pot, returnsErrorIfPotTypeIsNull)
{
  // Act
  cardano_error_t result = cardano_mir_to_pot_cert_get_pot((cardano_mir_to_pot_cert_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_mir_to_pot_cert_get_amount, returnsErrorIfMirCertIsNull)
{
  // Arrange
  uint64_t amount;

  // Act
  cardano_error_t result = cardano_mir_to_pot_cert_get_amount(nullptr, &amount);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_mir_to_pot_cert_get_amount, returnsErrorIfAmountIsNull)
{
  // Act
  cardano_error_t result = cardano_mir_to_pot_cert_get_amount((cardano_mir_to_pot_cert_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_mir_to_pot_cert_get_amount, canGetAmount)
{
  // Arrange
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = nullptr;
  cardano_error_t            result          = cardano_mir_to_pot_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, 1000000, &mir_to_pot_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  uint64_t amount;

  // Act
  result = cardano_mir_to_pot_cert_get_amount(mir_to_pot_cert, &amount);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(amount, 1000000);

  // Cleanup
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
}

TEST(cardano_mir_to_pot_cert_set_pot, returnsErrorIfMirCertIsNull)
{
  // Act
  cardano_error_t result = cardano_mir_to_pot_cert_set_pot(nullptr, CARDANO_MIR_CERT_POT_TYPE_TREASURY);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_mir_to_pot_cert_set_amount, returnsErrorIfMirCertIsNull)
{
  // Act
  cardano_error_t result = cardano_mir_to_pot_cert_set_amount(nullptr, 1000000);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_mir_to_pot_cert_set_amount, canSetAmount)
{
  // Arrange
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = nullptr;
  cardano_error_t            result          = cardano_mir_to_pot_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, 1000000, &mir_to_pot_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_mir_to_pot_cert_set_amount(mir_to_pot_cert, 2000000);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  uint64_t amount;

  EXPECT_EQ(cardano_mir_to_pot_cert_get_amount(mir_to_pot_cert, &amount), CARDANO_SUCCESS);
  EXPECT_EQ(amount, 2000000);

  // Cleanup
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
}

TEST(cardano_mir_to_pot_cert_set_pot, canSetPot)
{
  // Arrange
  cardano_mir_to_pot_cert_t* mir_to_pot_cert = nullptr;
  cardano_error_t            result          = cardano_mir_to_pot_cert_new(CARDANO_MIR_CERT_POT_TYPE_TREASURY, 1000000, &mir_to_pot_cert);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_mir_to_pot_cert_set_pot(mir_to_pot_cert, CARDANO_MIR_CERT_POT_TYPE_RESERVE);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_mir_cert_pot_type_t type;

  EXPECT_EQ(cardano_mir_to_pot_cert_get_pot(mir_to_pot_cert, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_MIR_CERT_POT_TYPE_RESERVE);

  // Cleanup
  cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
}
