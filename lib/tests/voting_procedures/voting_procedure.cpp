/**
 * \file voting_procedure.cpp
 *
 * \author angel.castillo
 * \date   Aug 05, 2024
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
#include <cardano/common/anchor.h>
#include <cardano/voting_procedures/voting_procedure.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR_NO_WITHOUT_ANCHOR      = "8200f6";
static const char* CBOR_YES_WITHOUT_ANCHOR     = "8201f6";
static const char* CBOR_ABSTAIN_WITHOUT_ANCHOR = "8202f6";
static const char* CBOR_NO_WITH_ANCHOR         = "8200827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_YES_WITH_ANCHOR        = "8201827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_ABSTAIN_WITH_ANCHOR    = "8202827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_ANCHOR                 = "827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the voting procedure.
 * @return A new instance of the voting procedure.
 */
static cardano_voting_procedure_t*
new_default_voting_procedure()
{
  cardano_voting_procedure_t* voting_procedure = NULL;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex(CBOR_YES_WITHOUT_ANCHOR, strlen(CBOR_YES_WITHOUT_ANCHOR));
  cardano_error_t             result           = cardano_voting_procedure_from_cbor(reader, &voting_procedure);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return voting_procedure;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_voting_procedure_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_voting_procedure_t* voting_procedure = new_default_voting_procedure();
  EXPECT_NE(voting_procedure, nullptr);

  // Act
  cardano_voting_procedure_ref(voting_procedure);

  // Assert
  EXPECT_THAT(voting_procedure, testing::Not((cardano_voting_procedure_t*)nullptr));
  EXPECT_EQ(cardano_voting_procedure_refcount(voting_procedure), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_voting_procedure_unref(&voting_procedure);
  cardano_voting_procedure_unref(&voting_procedure);
}

TEST(cardano_voting_procedure_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_voting_procedure_ref(nullptr);
}

TEST(cardano_voting_procedure_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_voting_procedure_t* voting_procedure = nullptr;

  // Act
  cardano_voting_procedure_unref(&voting_procedure);
}

TEST(cardano_voting_procedure_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_voting_procedure_unref((cardano_voting_procedure_t**)nullptr);
}

TEST(cardano_voting_procedure_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_voting_procedure_t* voting_procedure = new_default_voting_procedure();
  EXPECT_NE(voting_procedure, nullptr);

  // Act
  cardano_voting_procedure_ref(voting_procedure);
  size_t ref_count = cardano_voting_procedure_refcount(voting_procedure);

  cardano_voting_procedure_unref(&voting_procedure);
  size_t updated_ref_count = cardano_voting_procedure_refcount(voting_procedure);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_voting_procedure_unref(&voting_procedure);
}

TEST(cardano_voting_procedure_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_voting_procedure_t* voting_procedure = new_default_voting_procedure();
  EXPECT_NE(voting_procedure, nullptr);

  // Act
  cardano_voting_procedure_ref(voting_procedure);
  size_t ref_count = cardano_voting_procedure_refcount(voting_procedure);

  cardano_voting_procedure_unref(&voting_procedure);
  size_t updated_ref_count = cardano_voting_procedure_refcount(voting_procedure);

  cardano_voting_procedure_unref(&voting_procedure);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(voting_procedure, (cardano_voting_procedure_t*)nullptr);

  // Cleanup
  cardano_voting_procedure_unref(&voting_procedure);
}

TEST(cardano_voting_procedure_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_voting_procedure_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_voting_procedure_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_voting_procedure_t* voting_procedure = nullptr;
  const char*                 message          = "This is a test message";

  // Act
  cardano_voting_procedure_set_last_error(voting_procedure, message);

  // Assert
  EXPECT_STREQ(cardano_voting_procedure_get_last_error(voting_procedure), "Object is NULL.");
}

TEST(cardano_voting_procedure_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_voting_procedure_t* voting_procedure = new_default_voting_procedure();
  EXPECT_NE(voting_procedure, nullptr);

  const char* message = nullptr;

  // Act
  cardano_voting_procedure_set_last_error(voting_procedure, message);

  // Assert
  EXPECT_STREQ(cardano_voting_procedure_get_last_error(voting_procedure), "");

  // Cleanup
  cardano_voting_procedure_unref(&voting_procedure);
}

TEST(cardano_voting_procedure_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_voting_procedure_t* voting_procedure = NULL;

  // Act
  cardano_error_t result = cardano_voting_procedure_from_cbor(nullptr, &voting_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voting_procedure_from_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_YES_WITHOUT_ANCHOR, strlen(CBOR_YES_WITHOUT_ANCHOR));

  // Act
  cardano_error_t result = cardano_voting_procedure_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_voting_procedure_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t*      writer = cardano_cbor_writer_new();
  cardano_voting_procedure_t* cert   = new_default_voting_procedure();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_voting_procedure_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_YES_WITHOUT_ANCHOR);

  // Cleanup
  cardano_voting_procedure_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_voting_procedure_to_cbor, canSerializeWithAnchor)
{
  // Arrange
  cardano_cbor_writer_t*      writer = cardano_cbor_writer_new();
  cardano_voting_procedure_t* cert   = new_default_voting_procedure();
  EXPECT_NE(cert, nullptr);

  cardano_anchor_t*      anchor = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_ANCHOR, strlen(CBOR_ANCHOR));

  cardano_error_t result = cardano_anchor_from_cbor(reader, &anchor);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  EXPECT_EQ(cardano_voting_procedure_set_anchor(cert, anchor), CARDANO_SUCCESS);

  // Act
  result = cardano_voting_procedure_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_YES_WITH_ANCHOR);

  // Cleanup
  cardano_voting_procedure_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  cardano_anchor_unref(&anchor);
  cardano_cbor_reader_unref(&reader);
  free(hex);
}

TEST(cardano_voting_procedure_to_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_voting_procedure_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_voting_procedure_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_voting_procedure_to_cbor((cardano_voting_procedure_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

// Cert specific tests

TEST(cardano_voting_procedure_new, canCreateNewInstance)
{
  // Act
  cardano_voting_procedure_t* voting_procedure = NULL;

  cardano_error_t result = cardano_voting_procedure_new(CARDANO_VOTE_NO, NULL, &voting_procedure);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(voting_procedure, nullptr);

  // Cleanup
  cardano_voting_procedure_unref(&voting_procedure);
}

TEST(cardano_voting_procedure_new, canCreateNewInstanceWithAnchor)
{
  // Act
  cardano_anchor_t*      anchor = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_ANCHOR, strlen(CBOR_ANCHOR));

  cardano_voting_procedure_t* voting_procedure = NULL;

  cardano_error_t result = cardano_anchor_from_cbor(reader, &anchor);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_voting_procedure_new(CARDANO_VOTE_NO, anchor, &voting_procedure);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(voting_procedure, nullptr);

  // Cleanup
  cardano_voting_procedure_unref(&voting_procedure);
  cardano_anchor_unref(&anchor);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_voting_procedure_new, returnsErrorIfFirstArgIsNull)
{
  // Act
  cardano_error_t result = cardano_voting_procedure_new(CARDANO_VOTE_NO, NULL, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voting_procedure_new, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_voting_procedure_t* voting_procedure = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_voting_procedure_new(CARDANO_VOTE_NO, NULL, &voting_procedure);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_voting_procedure_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_voting_procedure_t* voting_procedure = NULL;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_voting_procedure_from_cbor(reader, &voting_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_voting_procedure_unref(&voting_procedure);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_voting_procedure_from_cbor, returnsErrorIfInvalidUintAsType)
{
  // Arrange
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex("82ef", strlen("82ef"));
  cardano_voting_procedure_t* voting_procedure = NULL;

  // Act
  cardano_error_t result = cardano_voting_procedure_from_cbor(reader, &voting_procedure);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_voting_procedure_from_cbor, returnsErrorIfInvalidAnchor)
{
  // Arrange
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex("8200ef7668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000", strlen("8200827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000"));
  cardano_voting_procedure_t* voting_procedure = NULL;

  // Act
  cardano_error_t result = cardano_voting_procedure_from_cbor(reader, &voting_procedure);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

// Getters and Setters

TEST(cardano_voting_procedure_get_anchor, canGetAnchor)
{
  // Arrange
  cardano_voting_procedure_t* voting_procedure = new_default_voting_procedure();

  // Act
  cardano_anchor_t* anchor = cardano_voting_procedure_get_anchor(voting_procedure);

  // Assert
  EXPECT_EQ(anchor, nullptr);

  // Cleanup
  cardano_voting_procedure_unref(&voting_procedure);
}

TEST(cardano_voting_procedure_set_anchor, canSetAnchor)
{
  // Arrange
  cardano_voting_procedure_t* voting_procedure = new_default_voting_procedure();
  cardano_anchor_t*           anchor           = NULL;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex(CBOR_ANCHOR, strlen(CBOR_ANCHOR));

  cardano_error_t result = cardano_anchor_from_cbor(reader, &anchor);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_voting_procedure_set_anchor(voting_procedure, anchor);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_voting_procedure_unref(&voting_procedure);
  cardano_anchor_unref(&anchor);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_voting_procedure_set_anchor, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_anchor_t*      anchor = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_ANCHOR, strlen(CBOR_ANCHOR));

  cardano_error_t result = cardano_anchor_from_cbor(reader, &anchor);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_voting_procedure_set_anchor(nullptr, anchor);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_anchor_unref(&anchor);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_voting_procedure_set_anchor, returnsErrorIfAnchorIsNull)
{
  // Arrange
  cardano_voting_procedure_t* voting_procedure = new_default_voting_procedure();

  // Act
  cardano_error_t result = cardano_voting_procedure_set_anchor(voting_procedure, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_voting_procedure_unref(&voting_procedure);
}

TEST(cardano_voting_procedure_get_anchor, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_anchor_t* anchor = cardano_voting_procedure_get_anchor(nullptr);

  // Assert
  EXPECT_EQ(anchor, nullptr);
}

TEST(cardano_voting_procedure_get_vote, canGetVote)
{
  // Arrange
  cardano_voting_procedure_t* voting_procedure = new_default_voting_procedure();

  // Act
  cardano_vote_t vote = cardano_voting_procedure_get_vote(voting_procedure);

  // Assert
  EXPECT_EQ(vote, CARDANO_VOTE_YES);

  // Cleanup
  cardano_voting_procedure_unref(&voting_procedure);
}

TEST(cardano_voting_procedure_set_vote, canSetVote)
{
  // Arrange
  cardano_voting_procedure_t* voting_procedure = new_default_voting_procedure();

  // Act
  cardano_error_t result = cardano_voting_procedure_set_vote(voting_procedure, CARDANO_VOTE_NO);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_voting_procedure_get_vote(voting_procedure), CARDANO_VOTE_NO);

  // Cleanup
  cardano_voting_procedure_unref(&voting_procedure);
}

TEST(cardano_voting_procedure_set_vote, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_error_t result = cardano_voting_procedure_set_vote(nullptr, CARDANO_VOTE_NO);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_voting_procedure_get_vote, returnsNoIfObjectIsNull)
{
  // Act
  cardano_vote_t vote = cardano_voting_procedure_get_vote(nullptr);

  // Assert
  EXPECT_EQ(vote, CARDANO_VOTE_NO);
}
