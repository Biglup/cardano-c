/**
 * \file sub_transaction.cpp
 *
 * \author angel.castillo
 * \date   Jul 17, 2026
 *
 * Copyright 2026 Biglup Labs
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
#include <cardano/transaction/sub_transaction.h>
#include <cardano/transaction_body/sub_transaction_body.h>
#include <cardano/witness_set/witness_set.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* MINIMAL_BODY_CBOR = "a200d90102800180";
static const char* MINIMAL_BODY_HASH = "da4603f4488d798af667794ab542f130a4bd1c20c7ed950c4648aaa95d0be4f4";

static const char* BODY_WITH_FEE_CBOR = "a300d90102800180031864";
static const char* BODY_WITH_FEE_HASH = "8cbebc5de7b583095090a501bd7cd092fcebbfdb87a48adb03e8c00a8960a0ad";

static const char* AUXILIARY_DATA_CBOR = "a1016474657374";
static const char* WITNESS_SET_CBOR    = "a100d90102818258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a";

static const char* MINIMAL_CBOR                   = "83a200d90102800180a0f6";
static const char* WITH_AUX_DATA_CBOR             = "83a200d90102800180a0a1016474657374";
static const char* WITH_WITNESS_CBOR              = "83a200d90102800180a100d90102818258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40af6";
static const char* WITH_WITNESS_AND_AUX_DATA_CBOR = "83a200d90102800180a100d90102818258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40aa1016474657374";

static const char* TWO_ELEMENT_CBOR         = "82a200d90102800180a0";
static const char* FOUR_ELEMENT_CBOR        = "84a200d90102800180a0f5f6";
static const char* INVALID_BODY_CBOR        = "83a10180a0f6";
static const char* INVALID_WITNESS_SET_CBOR = "83a200d90102800180eff6";
static const char* INVALID_AUX_DATA_CBOR    = "83a200d90102800180a087";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the sub_transaction.
 * @return A new instance of the sub_transaction.
 */
static cardano_sub_transaction_t*
new_default_sub_transaction(const char* cbor)
{
  cardano_sub_transaction_t* sub_transaction = NULL;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t            result          = cardano_sub_transaction_from_cbor(reader, &sub_transaction);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return sub_transaction;
};

/**
 * Creates a new default instance of the sub_transaction_body.
 * @return A new instance of the sub_transaction_body.
 */
static cardano_sub_transaction_body_t*
new_default_sub_transaction_body(const char* cbor)
{
  cardano_sub_transaction_body_t* sub_transaction_body = NULL;
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t                 result               = cardano_sub_transaction_body_from_cbor(reader, &sub_transaction_body);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return sub_transaction_body;
};

/**
 * Creates a new default instance of the witness_set.
 * @return A new instance of the witness_set.
 */
static cardano_witness_set_t*
new_default_witness_set(const char* cbor)
{
  cardano_witness_set_t* witness_set = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result      = cardano_witness_set_from_cbor(reader, &witness_set);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return witness_set;
};

/**
 * Creates a new default instance of the auxiliary_data.
 * @return A new instance of the auxiliary_data.
 */
static cardano_auxiliary_data_t*
new_default_auxiliary_data(const char* cbor)
{
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t           result         = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return auxiliary_data;
};

/**
 * Decodes the given CBOR hex string and expects the decode to fail with the given error.
 *
 * @param cbor The CBOR hex string to decode.
 * @param expected_error The error the decode is expected to fail with.
 */
static void
expect_decode_failure(const char* cbor, const cardano_error_t expected_error)
{
  cardano_sub_transaction_t* sub_transaction = NULL;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_sub_transaction_from_cbor(reader, &sub_transaction);

  EXPECT_EQ(result, expected_error);
  EXPECT_EQ(sub_transaction, (cardano_sub_transaction_t*)nullptr);

  cardano_cbor_reader_unref(&reader);
};

/**
 * Serializes the given sub transaction and expects the encoding to match the given CBOR hex string.
 *
 * @param sub_transaction The sub transaction to serialize.
 * @param expected_cbor The CBOR hex string the encoding is expected to match.
 */
static void
expect_encodes_to(cardano_sub_transaction_t* sub_transaction, const char* expected_cbor)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  EXPECT_THAT(cardano_sub_transaction_to_cbor(sub_transaction, writer), CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, expected_cbor);

  cardano_cbor_writer_unref(&writer);
  free(hex);
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_sub_transaction_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(MINIMAL_CBOR);
  EXPECT_NE(sub_transaction, nullptr);

  // Act
  cardano_sub_transaction_ref(sub_transaction);

  // Assert
  EXPECT_THAT(sub_transaction, testing::Not((cardano_sub_transaction_t*)nullptr));
  EXPECT_EQ(cardano_sub_transaction_refcount(sub_transaction), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_sub_transaction_unref(&sub_transaction);
}

TEST(cardano_sub_transaction_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_sub_transaction_ref(nullptr);
}

TEST(cardano_sub_transaction_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = nullptr;

  // Act
  cardano_sub_transaction_unref(&sub_transaction);
}

TEST(cardano_sub_transaction_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_sub_transaction_unref((cardano_sub_transaction_t**)nullptr);
}

TEST(cardano_sub_transaction_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(MINIMAL_CBOR);
  EXPECT_NE(sub_transaction, nullptr);

  // Act
  cardano_sub_transaction_ref(sub_transaction);
  size_t ref_count = cardano_sub_transaction_refcount(sub_transaction);

  cardano_sub_transaction_unref(&sub_transaction);
  size_t updated_ref_count = cardano_sub_transaction_refcount(sub_transaction);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
}

TEST(cardano_sub_transaction_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(MINIMAL_CBOR);
  EXPECT_NE(sub_transaction, nullptr);

  // Act
  cardano_sub_transaction_ref(sub_transaction);
  size_t ref_count = cardano_sub_transaction_refcount(sub_transaction);

  cardano_sub_transaction_unref(&sub_transaction);
  size_t updated_ref_count = cardano_sub_transaction_refcount(sub_transaction);

  cardano_sub_transaction_unref(&sub_transaction);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(sub_transaction, (cardano_sub_transaction_t*)nullptr);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
}

TEST(cardano_sub_transaction_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_sub_transaction_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_sub_transaction_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = nullptr;
  const char*                message         = "This is a test message";

  // Act
  cardano_sub_transaction_set_last_error(sub_transaction, message);

  // Assert
  EXPECT_STREQ(cardano_sub_transaction_get_last_error(sub_transaction), "Object is NULL.");
}

TEST(cardano_sub_transaction_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(MINIMAL_CBOR);
  EXPECT_NE(sub_transaction, nullptr);

  const char* message = nullptr;

  // Act
  cardano_sub_transaction_set_last_error(sub_transaction, message);

  // Assert
  EXPECT_STREQ(cardano_sub_transaction_get_last_error(sub_transaction), "");

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
}

TEST(cardano_sub_transaction_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = NULL;

  // Act
  cardano_error_t result = cardano_sub_transaction_from_cbor(nullptr, &sub_transaction);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_from_cbor, returnsErrorIfSubTransactionIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(MINIMAL_CBOR, strlen(MINIMAL_CBOR));

  // Act
  cardano_error_t result = cardano_sub_transaction_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Act & Assert
  expect_decode_failure("01", CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);
}

TEST(cardano_sub_transaction_from_cbor, rejectsTwoElementFrame)
{
  // Act & Assert
  expect_decode_failure(TWO_ELEMENT_CBOR, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);
}

TEST(cardano_sub_transaction_from_cbor, rejectsFourElementIsValidStyleFrame)
{
  // Act & Assert
  expect_decode_failure(FOUR_ELEMENT_CBOR, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);
}

TEST(cardano_sub_transaction_from_cbor, returnsErrorIfInvalidBody)
{
  // Act & Assert
  expect_decode_failure(INVALID_BODY_CBOR, CARDANO_ERROR_DECODING);
}

TEST(cardano_sub_transaction_from_cbor, returnsErrorIfInvalidWitnessSet)
{
  // Act & Assert
  expect_decode_failure(INVALID_WITNESS_SET_CBOR, CARDANO_ERROR_DECODING);
}

TEST(cardano_sub_transaction_from_cbor, returnsErrorIfInvalidAuxData)
{
  // Act & Assert
  expect_decode_failure(INVALID_AUX_DATA_CBOR, CARDANO_ERROR_DECODING);
}

TEST(cardano_sub_transaction_from_cbor, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = NULL;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex(WITH_WITNESS_AND_AUX_DATA_CBOR, strlen(WITH_WITNESS_AND_AUX_DATA_CBOR));

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_sub_transaction_from_cbor(reader, &sub_transaction);

  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  result = cardano_sub_transaction_from_cbor(reader, &sub_transaction);

  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_two_malloc, realloc, free);

  result = cardano_sub_transaction_from_cbor(reader, &sub_transaction);

  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  result = cardano_sub_transaction_from_cbor(reader, &sub_transaction);

  cardano_set_allocators(malloc, realloc, free);

  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_sub_transaction_to_cbor, preservesMinimalOriginalCborWhenEncoding)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(MINIMAL_CBOR);
  EXPECT_NE(sub_transaction, nullptr);

  // Act & Assert
  expect_encodes_to(sub_transaction, MINIMAL_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
}

TEST(cardano_sub_transaction_to_cbor, preservesOriginalCborWithAuxDataWhenEncoding)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(WITH_AUX_DATA_CBOR);
  EXPECT_NE(sub_transaction, nullptr);

  // Act & Assert
  expect_encodes_to(sub_transaction, WITH_AUX_DATA_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
}

TEST(cardano_sub_transaction_to_cbor, preservesOriginalCborWithWitnessSetWhenEncoding)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(WITH_WITNESS_CBOR);
  EXPECT_NE(sub_transaction, nullptr);

  // Act & Assert
  expect_encodes_to(sub_transaction, WITH_WITNESS_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
}

TEST(cardano_sub_transaction_to_cbor, preservesOriginalCborWithWitnessSetAndAuxDataWhenEncoding)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(WITH_WITNESS_AND_AUX_DATA_CBOR);
  EXPECT_NE(sub_transaction, nullptr);

  // Act & Assert
  expect_encodes_to(sub_transaction, WITH_WITNESS_AND_AUX_DATA_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
}

TEST(cardano_sub_transaction_to_cbor, canSerializeAfterClearingTheCache)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(MINIMAL_CBOR);
  EXPECT_NE(sub_transaction, nullptr);

  // Act
  cardano_sub_transaction_clear_cbor_cache(sub_transaction);

  // Assert
  expect_encodes_to(sub_transaction, MINIMAL_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
}

TEST(cardano_sub_transaction_to_cbor, returnsErrorIfSubTransactionIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_sub_transaction_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_sub_transaction_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_sub_transaction_to_cbor((cardano_sub_transaction_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_new, canCreateNewInstance)
{
  // Arrange
  cardano_sub_transaction_body_t* body           = new_default_sub_transaction_body(MINIMAL_BODY_CBOR);
  cardano_witness_set_t*          witness_set    = new_default_witness_set(WITNESS_SET_CBOR);
  cardano_auxiliary_data_t*       auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  // Act
  cardano_sub_transaction_t* sub_transaction = NULL;

  cardano_error_t result = cardano_sub_transaction_new(body, witness_set, auxiliary_data, &sub_transaction);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(sub_transaction, nullptr);
  expect_encodes_to(sub_transaction, WITH_WITNESS_AND_AUX_DATA_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_sub_transaction_body_unref(&body);
  cardano_witness_set_unref(&witness_set);
  cardano_auxiliary_data_unref(&auxiliary_data);
}

TEST(cardano_sub_transaction_new, canCreateNewInstanceWithoutAuxiliaryData)
{
  // Arrange
  cardano_sub_transaction_body_t* body        = new_default_sub_transaction_body(MINIMAL_BODY_CBOR);
  cardano_witness_set_t*          witness_set = new_default_witness_set(WITNESS_SET_CBOR);

  // Act
  cardano_sub_transaction_t* sub_transaction = NULL;

  cardano_error_t result = cardano_sub_transaction_new(body, witness_set, NULL, &sub_transaction);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(sub_transaction, nullptr);
  expect_encodes_to(sub_transaction, WITH_WITNESS_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_sub_transaction_body_unref(&body);
  cardano_witness_set_unref(&witness_set);
}

TEST(cardano_sub_transaction_new, returnsErrorIfBodyIsNull)
{
  // Act
  cardano_sub_transaction_t* sub_transaction = NULL;

  cardano_error_t result = cardano_sub_transaction_new(nullptr, nullptr, nullptr, &sub_transaction);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_new, returnsErrorIfWitnessSetIsNull)
{
  // Act
  cardano_sub_transaction_t* sub_transaction = NULL;

  cardano_error_t result = cardano_sub_transaction_new((cardano_sub_transaction_body_t*)"", nullptr, nullptr, &sub_transaction);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_new, returnsErrorIfSubTransactionIsNull)
{
  // Act
  cardano_error_t result = cardano_sub_transaction_new((cardano_sub_transaction_body_t*)"", (cardano_witness_set_t*)"", (cardano_auxiliary_data_t*)"", nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_sub_transaction_body_t* body           = new_default_sub_transaction_body(MINIMAL_BODY_CBOR);
  cardano_witness_set_t*          witness_set    = new_default_witness_set(WITNESS_SET_CBOR);
  cardano_auxiliary_data_t*       auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  // Act
  cardano_sub_transaction_t* sub_transaction = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_sub_transaction_new(body, witness_set, auxiliary_data, &sub_transaction);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_sub_transaction_body_unref(&body);
  cardano_witness_set_unref(&witness_set);
  cardano_auxiliary_data_unref(&auxiliary_data);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_sub_transaction_get_body, canGetBody)
{
  // Arrange
  cardano_sub_transaction_t*      sub_transaction = new_default_sub_transaction(MINIMAL_CBOR);
  cardano_sub_transaction_body_t* body            = new_default_sub_transaction_body(MINIMAL_BODY_CBOR);

  EXPECT_EQ(cardano_sub_transaction_set_body(sub_transaction, body), CARDANO_SUCCESS);

  // Act
  cardano_sub_transaction_body_t* body2 = cardano_sub_transaction_get_body(sub_transaction);

  // Assert
  EXPECT_EQ(body2, body);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_sub_transaction_body_unref(&body);
  cardano_sub_transaction_body_unref(&body2);
}

TEST(cardano_sub_transaction_get_body, returnsNullIfSubTransactionIsNull)
{
  // Act
  cardano_sub_transaction_body_t* body = cardano_sub_transaction_get_body(nullptr);

  // Assert
  EXPECT_EQ(body, nullptr);
}

TEST(cardano_sub_transaction_set_body, canSetBody)
{
  // Arrange
  cardano_sub_transaction_t*      sub_transaction = new_default_sub_transaction(MINIMAL_CBOR);
  cardano_sub_transaction_body_t* body            = new_default_sub_transaction_body(MINIMAL_BODY_CBOR);

  // Act
  cardano_error_t result = cardano_sub_transaction_set_body(sub_transaction, body);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_sub_transaction_body_unref(&body);
}

TEST(cardano_sub_transaction_set_body, returnsErrorIfSubTransactionIsNull)
{
  // Arrange
  cardano_sub_transaction_body_t* body = new_default_sub_transaction_body(MINIMAL_BODY_CBOR);

  // Act
  cardano_error_t result = cardano_sub_transaction_set_body(nullptr, body);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_transaction_body_unref(&body);
}

TEST(cardano_sub_transaction_set_body, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(MINIMAL_CBOR);

  // Act
  cardano_error_t result = cardano_sub_transaction_set_body(sub_transaction, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
}

TEST(cardano_sub_transaction_get_witness_set, canGetWitnessSet)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(MINIMAL_CBOR);
  cardano_witness_set_t*     witness_set     = new_default_witness_set(WITNESS_SET_CBOR);

  EXPECT_EQ(cardano_sub_transaction_set_witness_set(sub_transaction, witness_set), CARDANO_SUCCESS);

  // Act
  cardano_witness_set_t* witness_set2 = cardano_sub_transaction_get_witness_set(sub_transaction);

  // Assert
  EXPECT_EQ(witness_set2, witness_set);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_witness_set_unref(&witness_set);
  cardano_witness_set_unref(&witness_set2);
}

TEST(cardano_sub_transaction_get_witness_set, returnsNullIfSubTransactionIsNull)
{
  // Act
  cardano_witness_set_t* witness_set = cardano_sub_transaction_get_witness_set(nullptr);

  // Assert
  EXPECT_EQ(witness_set, nullptr);
}

TEST(cardano_sub_transaction_set_witness_set, canSetWitnessSet)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(MINIMAL_CBOR);
  cardano_witness_set_t*     witness_set     = new_default_witness_set(WITNESS_SET_CBOR);

  // Act
  cardano_error_t result = cardano_sub_transaction_set_witness_set(sub_transaction, witness_set);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_witness_set_unref(&witness_set);
}

TEST(cardano_sub_transaction_set_witness_set, returnsErrorIfSubTransactionIsNull)
{
  // Arrange
  cardano_witness_set_t* witness_set = new_default_witness_set(WITNESS_SET_CBOR);

  // Act
  cardano_error_t result = cardano_sub_transaction_set_witness_set(nullptr, witness_set);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_witness_set_unref(&witness_set);
}

TEST(cardano_sub_transaction_set_witness_set, returnsErrorIfWitnessSetIsNull)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(MINIMAL_CBOR);

  // Act
  cardano_error_t result = cardano_sub_transaction_set_witness_set(sub_transaction, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
}

TEST(cardano_sub_transaction_get_auxiliary_data, canGetAuxiliaryData)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(WITH_AUX_DATA_CBOR);

  // Act
  cardano_auxiliary_data_t* auxiliary_data = cardano_sub_transaction_get_auxiliary_data(sub_transaction);

  // Assert
  EXPECT_NE(auxiliary_data, nullptr);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_auxiliary_data_unref(&auxiliary_data);
}

TEST(cardano_sub_transaction_get_auxiliary_data, returnsNullIfAuxiliaryDataIsAbsent)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(MINIMAL_CBOR);

  // Act
  cardano_auxiliary_data_t* auxiliary_data = cardano_sub_transaction_get_auxiliary_data(sub_transaction);

  // Assert
  EXPECT_EQ(auxiliary_data, nullptr);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
}

TEST(cardano_sub_transaction_get_auxiliary_data, returnsNullIfSubTransactionIsNull)
{
  // Act
  cardano_auxiliary_data_t* auxiliary_data = cardano_sub_transaction_get_auxiliary_data(nullptr);

  // Assert
  EXPECT_EQ(auxiliary_data, nullptr);
}

TEST(cardano_sub_transaction_set_auxiliary_data, canSetAuxiliaryData)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(MINIMAL_CBOR);
  cardano_auxiliary_data_t*  auxiliary_data  = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  // Act
  cardano_error_t result = cardano_sub_transaction_set_auxiliary_data(sub_transaction, auxiliary_data);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  expect_encodes_to(sub_transaction, WITH_AUX_DATA_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_auxiliary_data_unref(&auxiliary_data);
}

TEST(cardano_sub_transaction_set_auxiliary_data, canSetNullAuxiliaryData)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(WITH_AUX_DATA_CBOR);

  // Act
  cardano_error_t result = cardano_sub_transaction_set_auxiliary_data(sub_transaction, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  expect_encodes_to(sub_transaction, MINIMAL_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
}

TEST(cardano_sub_transaction_set_auxiliary_data, returnsErrorIfSubTransactionIsNull)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  // Act
  cardano_error_t result = cardano_sub_transaction_set_auxiliary_data(nullptr, auxiliary_data);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
}

TEST(cardano_sub_transaction_get_id, canGetId)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(MINIMAL_CBOR);

  // Act
  cardano_blake2b_hash_t* id = cardano_sub_transaction_get_id(sub_transaction);

  size_t size = cardano_blake2b_hash_get_hex_size(id);
  char*  hex  = (char*)malloc(size);

  cardano_error_t result = cardano_blake2b_hash_to_hex(id, hex, size);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(hex, MINIMAL_BODY_HASH);

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_blake2b_hash_unref(&id);
  free(hex);
}

TEST(cardano_sub_transaction_get_id, matchesTheBodyHash)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(WITH_WITNESS_AND_AUX_DATA_CBOR);

  cardano_sub_transaction_body_t* body = cardano_sub_transaction_get_body(sub_transaction);

  // Act
  cardano_blake2b_hash_t* id        = cardano_sub_transaction_get_id(sub_transaction);
  cardano_blake2b_hash_t* body_hash = cardano_sub_transaction_body_get_hash(body);

  // Assert
  EXPECT_NE(id, nullptr);
  EXPECT_NE(body_hash, nullptr);
  EXPECT_TRUE(cardano_blake2b_hash_equals(id, body_hash));

  // Cleanup
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_sub_transaction_body_unref(&body);
  cardano_blake2b_hash_unref(&id);
  cardano_blake2b_hash_unref(&body_hash);
}

TEST(cardano_sub_transaction_get_id, returnsNullIfSubTransactionIsNull)
{
  // Act
  cardano_blake2b_hash_t* id = cardano_sub_transaction_get_id(nullptr);

  // Assert
  EXPECT_EQ(id, nullptr);
}

TEST(cardano_sub_transaction_get_id, returnsTheCachedHashOnSubsequentCalls)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(MINIMAL_CBOR);

  // Act
  cardano_blake2b_hash_t* first_id  = cardano_sub_transaction_get_id(sub_transaction);
  cardano_blake2b_hash_t* second_id = cardano_sub_transaction_get_id(sub_transaction);

  // Assert
  EXPECT_EQ(first_id, second_id);
  EXPECT_TRUE(cardano_blake2b_hash_equals(first_id, second_id));
  EXPECT_EQ(cardano_blake2b_hash_refcount(first_id), 3);

  // Cleanup
  cardano_blake2b_hash_unref(&first_id);
  cardano_blake2b_hash_unref(&second_id);
  cardano_sub_transaction_unref(&sub_transaction);
}

TEST(cardano_sub_transaction_get_id, changesAfterSetBody)
{
  // Arrange
  cardano_sub_transaction_t*      sub_transaction = new_default_sub_transaction(MINIMAL_CBOR);
  cardano_sub_transaction_body_t* body            = new_default_sub_transaction_body(BODY_WITH_FEE_CBOR);

  cardano_blake2b_hash_t* first_id = cardano_sub_transaction_get_id(sub_transaction);

  // Act
  EXPECT_EQ(cardano_sub_transaction_set_body(sub_transaction, body), CARDANO_SUCCESS);

  cardano_blake2b_hash_t* second_id = cardano_sub_transaction_get_id(sub_transaction);

  size_t size = cardano_blake2b_hash_get_hex_size(second_id);
  char*  hex  = (char*)malloc(size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(second_id, hex, size), CARDANO_SUCCESS);

  // Assert
  EXPECT_FALSE(cardano_blake2b_hash_equals(first_id, second_id));
  EXPECT_STREQ(hex, BODY_WITH_FEE_HASH);

  // Cleanup
  cardano_blake2b_hash_unref(&first_id);
  cardano_blake2b_hash_unref(&second_id);
  cardano_sub_transaction_body_unref(&body);
  cardano_sub_transaction_unref(&sub_transaction);
  free(hex);
}

TEST(cardano_sub_transaction_get_id, unchangedAfterSetAuxiliaryData)
{
  // Arrange
  cardano_sub_transaction_t* sub_transaction = new_default_sub_transaction(MINIMAL_CBOR);
  cardano_auxiliary_data_t*  auxiliary_data  = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  cardano_blake2b_hash_t* first_id = cardano_sub_transaction_get_id(sub_transaction);

  // Act
  EXPECT_EQ(cardano_sub_transaction_set_auxiliary_data(sub_transaction, auxiliary_data), CARDANO_SUCCESS);

  cardano_blake2b_hash_t* second_id = cardano_sub_transaction_get_id(sub_transaction);

  size_t size = cardano_blake2b_hash_get_hex_size(second_id);
  char*  hex  = (char*)malloc(size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(second_id, hex, size), CARDANO_SUCCESS);

  // Assert
  EXPECT_TRUE(cardano_blake2b_hash_equals(first_id, second_id));
  EXPECT_STREQ(hex, MINIMAL_BODY_HASH);

  // Cleanup
  cardano_blake2b_hash_unref(&first_id);
  cardano_blake2b_hash_unref(&second_id);
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_sub_transaction_unref(&sub_transaction);
  free(hex);
}

TEST(cardano_sub_transaction_clear_cbor_cache, doesNothingIfGivenNullPtr)
{
  // Act
  cardano_sub_transaction_clear_cbor_cache(nullptr);
}
