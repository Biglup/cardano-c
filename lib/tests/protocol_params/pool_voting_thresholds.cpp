/**
 * \file pool_voting_thresholds.cpp
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

#include <cardano/protocol_params/pool_voting_thresholds.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR = "85d81e820000d81e820101d81e820202d81e820303d81e820404";

static cardano_pool_voting_thresholds_t*
init_pool_voting_thresholds()
{
  cardano_unit_interval_t* motion_no_confidence    = NULL;
  cardano_unit_interval_t* committee_normal        = NULL;
  cardano_unit_interval_t* committee_no_confidence = NULL;
  cardano_unit_interval_t* hard_fork_initiation    = NULL;
  cardano_unit_interval_t* security_relevant_param = NULL;

  EXPECT_EQ(cardano_unit_interval_new(0, 0, &motion_no_confidence), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(1, 1, &committee_normal), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(2, 2, &committee_no_confidence), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(3, 3, &hard_fork_initiation), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(4, 4, &security_relevant_param), CARDANO_SUCCESS);

  cardano_pool_voting_thresholds_t* pool_voting_thresholds = NULL;
  EXPECT_EQ(cardano_pool_voting_thresholds_new(motion_no_confidence, committee_normal, committee_no_confidence, hard_fork_initiation, security_relevant_param, &pool_voting_thresholds), CARDANO_SUCCESS);

  cardano_unit_interval_unref(&motion_no_confidence);
  cardano_unit_interval_unref(&committee_normal);
  cardano_unit_interval_unref(&committee_no_confidence);
  cardano_unit_interval_unref(&hard_fork_initiation);
  cardano_unit_interval_unref(&security_relevant_param);

  return pool_voting_thresholds;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_pool_voting_thresholds_new, canCreate)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();

  // Assert
  EXPECT_THAT(pool_voting_thresholds, testing::Not((cardano_pool_voting_thresholds_t*)nullptr));

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
}

TEST(cardano_pool_voting_thresholds_new, returnErrorIfFirstParamIsNull)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;

  // Assert
  EXPECT_EQ(cardano_pool_voting_thresholds_new(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr), CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(cardano_pool_voting_thresholds_new(nullptr, nullptr, nullptr, nullptr, nullptr, &pool_voting_thresholds), CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(cardano_pool_voting_thresholds_new((cardano_unit_interval_t*)"", nullptr, nullptr, nullptr, nullptr, &pool_voting_thresholds), CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(cardano_pool_voting_thresholds_new((cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", nullptr, nullptr, nullptr, &pool_voting_thresholds), CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(cardano_pool_voting_thresholds_new((cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", nullptr, nullptr, &pool_voting_thresholds), CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(cardano_pool_voting_thresholds_new((cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", nullptr, &pool_voting_thresholds), CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_voting_thresholds_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_unit_interval_t* motion_no_confidence    = NULL;
  cardano_unit_interval_t* committee_normal        = NULL;
  cardano_unit_interval_t* committee_no_confidence = NULL;
  cardano_unit_interval_t* hard_fork_initiation    = NULL;
  cardano_unit_interval_t* security_relevant_param = NULL;

  EXPECT_EQ(cardano_unit_interval_new(0, 0, &motion_no_confidence), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(1, 1, &committee_normal), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(2, 2, &committee_no_confidence), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(3, 3, &hard_fork_initiation), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(4, 4, &security_relevant_param), CARDANO_SUCCESS);

  // Act
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);
  cardano_error_t error = cardano_pool_voting_thresholds_new(motion_no_confidence, committee_normal, committee_no_confidence, hard_fork_initiation, security_relevant_param, &pool_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_unit_interval_unref(&motion_no_confidence);
  cardano_unit_interval_unref(&committee_normal);
  cardano_unit_interval_unref(&committee_no_confidence);
  cardano_unit_interval_unref(&hard_fork_initiation);
  cardano_unit_interval_unref(&security_relevant_param);
}

TEST(cardano_pool_voting_thresholds_to_cbor, canSerialize)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();
  cardano_cbor_writer_t*            writer                 = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_to_cbor(pool_voting_thresholds, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  cbor_hex = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor_hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, CBOR);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_cbor_writer_unref(&writer);
  free(cbor_hex);
}

TEST(cardano_pool_voting_thresholds_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_pool_voting_thresholds_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_to_cbor(pool_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
}

TEST(cardano_pool_voting_thresholds_from_cbor, canDeserializeCbor)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_from_cbor(reader, &pool_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(pool_voting_thresholds, testing::Not((cardano_pool_voting_thresholds_t*)nullptr));

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_voting_thresholds_from_cbor, returnsErrorIdInvalidArray)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("04", 2);

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_from_cbor(reader, &pool_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_voting_thresholds_from_cbor, returnErrorIfInvalidMotionNoConfidence)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("85d81ea20000d81e820101d81e820202d81e820303d81e820404", strlen("85d81ea20000d81e820101d81e820202d81e820303d81e820404"));

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_from_cbor(reader, &pool_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_voting_thresholds_from_cbor, returnErrorIfInvalidComitteeNotmal)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("85d81e820000d81ea20101d81e820202d81e820303d81e820404", strlen("85d81e820000d81e820101d81e820202d81e820303d81e820404"));

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_from_cbor(reader, &pool_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_voting_thresholds_from_cbor, returnErrorIfInvalidCommiteeNoConfidence)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("85d81e820000d81e820101d81ea20202d81e820303d81e820404", strlen("85d81e820000d81e820101d81e820202d81e820303d81e820404"));

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_from_cbor(reader, &pool_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_voting_thresholds_from_cbor, returnErrorIfInvalidHardForkInitiation)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("85d81e820000d81e820101d81e820202d81ea20303d81e820404", strlen("85d81e820000d81e820101d81e820202d81e820303d81e820404"));

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_from_cbor(reader, &pool_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_voting_thresholds_from_cbor, returnErrorIfInvalidSecurityParam)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("85d81e820000d81e820101d81e820202d81e820303d81ea20404", strlen("85d81e820000d81e820101d81e820202d81e820303d81e820404"));

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_from_cbor(reader, &pool_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_voting_thresholds_from_cbor, returnErrorIfGivenNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_voting_thresholds_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_from_cbor(nullptr, &pool_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_voting_thresholds_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_voting_thresholds_from_cbor(reader, &pool_voting_thresholds);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_pool_voting_thresholds_ref(pool_voting_thresholds);

  // Assert
  EXPECT_THAT(pool_voting_thresholds, testing::Not((cardano_pool_voting_thresholds_t*)nullptr));
  EXPECT_EQ(cardano_pool_voting_thresholds_refcount(pool_voting_thresholds), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_voting_thresholds_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_pool_voting_thresholds_ref(nullptr);
}

TEST(cardano_pool_voting_thresholds_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;

  // Act
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
}

TEST(cardano_pool_voting_thresholds_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_pool_voting_thresholds_unref((cardano_pool_voting_thresholds_t**)nullptr);
}

TEST(cardano_pool_voting_thresholds_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_voting_thresholds_from_cbor(reader, &pool_voting_thresholds);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_pool_voting_thresholds_ref(pool_voting_thresholds);
  size_t ref_count = cardano_pool_voting_thresholds_refcount(pool_voting_thresholds);

  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  size_t updated_ref_count = cardano_pool_voting_thresholds_refcount(pool_voting_thresholds);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_voting_thresholds_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_voting_thresholds_from_cbor(reader, &pool_voting_thresholds);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_pool_voting_thresholds_ref(pool_voting_thresholds);
  size_t ref_count = cardano_pool_voting_thresholds_refcount(pool_voting_thresholds);

  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  size_t updated_ref_count = cardano_pool_voting_thresholds_refcount(pool_voting_thresholds);

  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(pool_voting_thresholds, (cardano_pool_voting_thresholds_t*)nullptr);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_voting_thresholds_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_pool_voting_thresholds_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_pool_voting_thresholds_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;
  const char*                       message                = "This is a test message";

  // Act
  cardano_pool_voting_thresholds_set_last_error(pool_voting_thresholds, message);

  // Assert
  EXPECT_STREQ(cardano_pool_voting_thresholds_get_last_error(pool_voting_thresholds), "Object is NULL.");
}

TEST(cardano_pool_voting_thresholds_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_pool_voting_thresholds_from_cbor(reader, &pool_voting_thresholds);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_pool_voting_thresholds_set_last_error(pool_voting_thresholds, message);

  // Assert
  EXPECT_STREQ(cardano_pool_voting_thresholds_get_last_error(pool_voting_thresholds), "");

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_voting_thresholds_get_motion_no_confidence, canReturnTheMotion)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();

  // Act
  cardano_unit_interval_t* motion_no_confidence = NULL;

  EXPECT_EQ(cardano_pool_voting_thresholds_get_motion_no_confidence(pool_voting_thresholds, &motion_no_confidence), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(motion_no_confidence, testing::Not((cardano_unit_interval_t*)nullptr));

  uint64_t denominator = cardano_unit_interval_get_denominator(motion_no_confidence);
  uint64_t numerator   = cardano_unit_interval_get_numerator(motion_no_confidence);

  EXPECT_EQ(denominator, 0);
  EXPECT_EQ(numerator, 0);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_unit_interval_unref(&motion_no_confidence);
}

TEST(cardano_pool_voting_thresholds_get_motion_no_confidence, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;

  // Act
  cardano_unit_interval_t* motion_no_confidence = NULL;

  cardano_error_t error = cardano_pool_voting_thresholds_get_motion_no_confidence(pool_voting_thresholds, &motion_no_confidence);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_voting_thresholds_get_motion_no_confidence, returnsErrorIfGivenANullPtrForTheMotion)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_get_motion_no_confidence(pool_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
}

TEST(cardano_pool_voting_thresholds_get_committee_normal, canReturnTheCommitteeNormal)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();

  // Act
  cardano_unit_interval_t* committee_normal = NULL;

  EXPECT_EQ(cardano_pool_voting_thresholds_get_committee_normal(pool_voting_thresholds, &committee_normal), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(committee_normal, testing::Not((cardano_unit_interval_t*)nullptr));

  uint64_t denominator = cardano_unit_interval_get_denominator(committee_normal);
  uint64_t numerator   = cardano_unit_interval_get_numerator(committee_normal);

  EXPECT_EQ(denominator, 1);
  EXPECT_EQ(numerator, 1);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_unit_interval_unref(&committee_normal);
}

TEST(cardano_pool_voting_thresholds_get_committee_normal, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;

  // Act
  cardano_unit_interval_t* committee_normal = NULL;

  cardano_error_t error = cardano_pool_voting_thresholds_get_committee_normal(pool_voting_thresholds, &committee_normal);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_voting_thresholds_get_committee_normal, returnsErrorIfGivenANullPtrForTheCommitteeNormal)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_get_committee_normal(pool_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
}

TEST(cardano_pool_voting_thresholds_get_committee_no_confidence, canReturnTheCommitteeNoConfidence)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();

  // Act
  cardano_unit_interval_t* committee_no_confidence = NULL;

  EXPECT_EQ(cardano_pool_voting_thresholds_get_committee_no_confidence(pool_voting_thresholds, &committee_no_confidence), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(committee_no_confidence, testing::Not((cardano_unit_interval_t*)nullptr));

  uint64_t denominator = cardano_unit_interval_get_denominator(committee_no_confidence);
  uint64_t numerator   = cardano_unit_interval_get_numerator(committee_no_confidence);

  EXPECT_EQ(denominator, 2);
  EXPECT_EQ(numerator, 2);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_unit_interval_unref(&committee_no_confidence);
}

TEST(cardano_pool_voting_thresholds_get_committee_no_confidence, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;

  // Act
  cardano_unit_interval_t* committee_no_confidence = NULL;

  cardano_error_t error = cardano_pool_voting_thresholds_get_committee_no_confidence(pool_voting_thresholds, &committee_no_confidence);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_voting_thresholds_get_committee_no_confidence, returnsErrorIfGivenANullPtrForTheCommitteeNoConfidence)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_get_committee_no_confidence(pool_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
}

TEST(cardano_pool_voting_thresholds_get_hard_fork_initiation, canReturnTheHardForkInitiation)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();

  // Act
  cardano_unit_interval_t* hard_fork_initiation = NULL;

  EXPECT_EQ(cardano_pool_voting_thresholds_get_hard_fork_initiation(pool_voting_thresholds, &hard_fork_initiation), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(hard_fork_initiation, testing::Not((cardano_unit_interval_t*)nullptr));

  uint64_t denominator = cardano_unit_interval_get_denominator(hard_fork_initiation);
  uint64_t numerator   = cardano_unit_interval_get_numerator(hard_fork_initiation);

  EXPECT_EQ(denominator, 3);
  EXPECT_EQ(numerator, 3);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_unit_interval_unref(&hard_fork_initiation);
}

TEST(cardano_pool_voting_thresholds_get_hard_fork_initiation, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;

  // Act
  cardano_unit_interval_t* hard_fork_initiation = NULL;

  cardano_error_t error = cardano_pool_voting_thresholds_get_hard_fork_initiation(pool_voting_thresholds, &hard_fork_initiation);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_voting_thresholds_get_hard_fork_initiation, returnsErrorIfGivenANullPtrForTheHardForkInitiation)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_get_hard_fork_initiation(pool_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
}

TEST(cardano_pool_voting_thresholds_get_security_relevant_param, canReturnThePpNetworkGroup)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();

  // Act
  cardano_unit_interval_t* security_relevant_param = NULL;

  EXPECT_EQ(cardano_pool_voting_thresholds_get_security_relevant_param(pool_voting_thresholds, &security_relevant_param), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(security_relevant_param, testing::Not((cardano_unit_interval_t*)nullptr));

  uint64_t denominator = cardano_unit_interval_get_denominator(security_relevant_param);
  uint64_t numerator   = cardano_unit_interval_get_numerator(security_relevant_param);

  EXPECT_EQ(denominator, 4);
  EXPECT_EQ(numerator, 4);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_unit_interval_unref(&security_relevant_param);
}

TEST(cardano_pool_voting_thresholds_get_security_relevant_param, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;

  // Act
  cardano_unit_interval_t* security_relevant_param = NULL;

  cardano_error_t error = cardano_pool_voting_thresholds_get_security_relevant_param(pool_voting_thresholds, &security_relevant_param);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_voting_thresholds_get_security_relevant_param, returnsErrorIfGivenANullPtrForThePpNetworkGroup)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_get_security_relevant_param(pool_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
}

TEST(cardano_pool_voting_thresholds_set_motion_no_confidence, canSetTheMotionNoConfidence)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();
  cardano_unit_interval_t*          motion_no_confidence   = NULL;

  EXPECT_EQ(cardano_unit_interval_new(99, 99, &motion_no_confidence), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_pool_voting_thresholds_set_motion_no_confidence(pool_voting_thresholds, motion_no_confidence), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* motion_no_confidence_result = NULL;

  EXPECT_EQ(cardano_pool_voting_thresholds_get_motion_no_confidence(pool_voting_thresholds, &motion_no_confidence_result), CARDANO_SUCCESS);

  uint64_t denominator = cardano_unit_interval_get_denominator(motion_no_confidence_result);
  uint64_t numerator   = cardano_unit_interval_get_numerator(motion_no_confidence_result);

  EXPECT_EQ(denominator, 99);
  EXPECT_EQ(numerator, 99);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_unit_interval_unref(&motion_no_confidence);
  cardano_unit_interval_unref(&motion_no_confidence_result);
}

TEST(cardano_pool_voting_thresholds_set_motion_no_confidence, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;
  cardano_unit_interval_t*          motion_no_confidence   = NULL;

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_set_motion_no_confidence(pool_voting_thresholds, motion_no_confidence);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_voting_thresholds_set_motion_no_confidence, returnsErrorIfGivenANullPtrForTheMotionNoConfidence)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_set_motion_no_confidence(pool_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
}

TEST(cardano_pool_voting_thresholds_set_committee_normal, canSetTheCommitteeNormal)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();
  cardano_unit_interval_t*          committee_normal       = NULL;

  EXPECT_EQ(cardano_unit_interval_new(98, 98, &committee_normal), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_pool_voting_thresholds_set_committee_normal(pool_voting_thresholds, committee_normal), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* committee_normal_result = NULL;

  EXPECT_EQ(cardano_pool_voting_thresholds_get_committee_normal(pool_voting_thresholds, &committee_normal_result), CARDANO_SUCCESS);

  uint64_t denominator = cardano_unit_interval_get_denominator(committee_normal_result);
  uint64_t numerator   = cardano_unit_interval_get_numerator(committee_normal_result);

  EXPECT_EQ(denominator, 98);
  EXPECT_EQ(numerator, 98);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_unit_interval_unref(&committee_normal);
  cardano_unit_interval_unref(&committee_normal_result);
}

TEST(cardano_pool_voting_thresholds_set_committee_normal, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;
  cardano_unit_interval_t*          committee_normal       = NULL;

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_set_committee_normal(pool_voting_thresholds, committee_normal);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_voting_thresholds_set_committee_normal, returnsErrorIfGivenANullPtrForTheCommitteeNormal)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_set_committee_normal(pool_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
}

TEST(cardano_pool_voting_thresholds_set_committee_no_confidence, canSetTheCommitteeNoConfidence)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds  = init_pool_voting_thresholds();
  cardano_unit_interval_t*          committee_no_confidence = NULL;

  EXPECT_EQ(cardano_unit_interval_new(97, 97, &committee_no_confidence), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_pool_voting_thresholds_set_committee_no_confidence(pool_voting_thresholds, committee_no_confidence), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* committee_no_confidence_result = NULL;

  EXPECT_EQ(cardano_pool_voting_thresholds_get_committee_no_confidence(pool_voting_thresholds, &committee_no_confidence_result), CARDANO_SUCCESS);

  uint64_t denominator = cardano_unit_interval_get_denominator(committee_no_confidence_result);
  uint64_t numerator   = cardano_unit_interval_get_numerator(committee_no_confidence_result);

  EXPECT_EQ(denominator, 97);
  EXPECT_EQ(numerator, 97);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_unit_interval_unref(&committee_no_confidence);
  cardano_unit_interval_unref(&committee_no_confidence_result);
}

TEST(cardano_pool_voting_thresholds_set_committee_no_confidence, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds  = nullptr;
  cardano_unit_interval_t*          committee_no_confidence = NULL;

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_set_committee_no_confidence(pool_voting_thresholds, committee_no_confidence);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_voting_thresholds_set_committee_no_confidence, returnsErrorIfGivenANullPtrForTheCommitteeNoConfidence)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_set_committee_no_confidence(pool_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
}

TEST(cardano_pool_voting_thresholds_set_hard_fork_initiation, canSetTheHardForkInitiation)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();
  cardano_unit_interval_t*          hard_fork_initiation   = NULL;

  EXPECT_EQ(cardano_unit_interval_new(95, 95, &hard_fork_initiation), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_pool_voting_thresholds_set_hard_fork_initiation(pool_voting_thresholds, hard_fork_initiation), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* hard_fork_initiation_result = NULL;

  EXPECT_EQ(cardano_pool_voting_thresholds_get_hard_fork_initiation(pool_voting_thresholds, &hard_fork_initiation_result), CARDANO_SUCCESS);

  uint64_t denominator = cardano_unit_interval_get_denominator(hard_fork_initiation_result);
  uint64_t numerator   = cardano_unit_interval_get_numerator(hard_fork_initiation_result);

  EXPECT_EQ(denominator, 95);
  EXPECT_EQ(numerator, 95);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_unit_interval_unref(&hard_fork_initiation);
  cardano_unit_interval_unref(&hard_fork_initiation_result);
}

TEST(cardano_pool_voting_thresholds_set_hard_fork_initiation, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;
  cardano_unit_interval_t*          hard_fork_initiation   = NULL;

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_set_hard_fork_initiation(pool_voting_thresholds, hard_fork_initiation);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_voting_thresholds_set_hard_fork_initiation, returnsErrorIfGivenANullPtrForTheHardForkInitiation)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_set_hard_fork_initiation(pool_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
}

TEST(cardano_pool_voting_thresholds_set_security_relevant_param, canSetThePpNetworkGroup)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds  = init_pool_voting_thresholds();
  cardano_unit_interval_t*          security_relevant_param = NULL;

  EXPECT_EQ(cardano_unit_interval_new(94, 94, &security_relevant_param), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_pool_voting_thresholds_set_security_relevant_param(pool_voting_thresholds, security_relevant_param), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* security_relevant_param_result = NULL;

  EXPECT_EQ(cardano_pool_voting_thresholds_get_security_relevant_param(pool_voting_thresholds, &security_relevant_param_result), CARDANO_SUCCESS);

  uint64_t denominator = cardano_unit_interval_get_denominator(security_relevant_param_result);
  uint64_t numerator   = cardano_unit_interval_get_numerator(security_relevant_param_result);

  EXPECT_EQ(denominator, 94);
  EXPECT_EQ(numerator, 94);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_unit_interval_unref(&security_relevant_param);
  cardano_unit_interval_unref(&security_relevant_param_result);
}

TEST(cardano_pool_voting_thresholds_set_security_relevant_param, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds  = nullptr;
  cardano_unit_interval_t*          security_relevant_param = NULL;

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_set_security_relevant_param(pool_voting_thresholds, security_relevant_param);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_voting_thresholds_set_security_relevant_param, returnsErrorIfGivenANullPtrForThePpNetworkGroup)
{
  // Arrange
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = init_pool_voting_thresholds();

  // Act
  cardano_error_t error = cardano_pool_voting_thresholds_set_security_relevant_param(pool_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
}
