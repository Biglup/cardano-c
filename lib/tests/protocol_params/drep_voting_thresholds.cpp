/**
 * \file drep_voting_thresholds.cpp
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

#include <cardano/buffer.h>
#include <cardano/protocol_params/drep_voting_thresholds.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR = "8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909";

static cardano_drep_voting_thresholds_t*
init_drep_voting_thresholds()
{
  cardano_unit_interval_t* motion_no_confidence    = NULL;
  cardano_unit_interval_t* committee_normal        = NULL;
  cardano_unit_interval_t* committee_no_confidence = NULL;
  cardano_unit_interval_t* update_constitution     = NULL;
  cardano_unit_interval_t* hard_fork_initiation    = NULL;
  cardano_unit_interval_t* pp_network_group        = NULL;
  cardano_unit_interval_t* pp_economic_group       = NULL;
  cardano_unit_interval_t* pp_technical_group      = NULL;
  cardano_unit_interval_t* pp_governance_group     = NULL;
  cardano_unit_interval_t* treasury_withdrawal     = NULL;

  EXPECT_EQ(cardano_unit_interval_new(0, 0, &motion_no_confidence), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(1, 1, &committee_normal), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(2, 2, &committee_no_confidence), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(3, 3, &update_constitution), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(4, 4, &hard_fork_initiation), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(5, 5, &pp_network_group), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(6, 6, &pp_economic_group), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(7, 7, &pp_technical_group), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(8, 8, &pp_governance_group), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(9, 9, &treasury_withdrawal), CARDANO_SUCCESS);

  cardano_drep_voting_thresholds_t* drep_voting_thresholds = NULL;
  EXPECT_EQ(cardano_drep_voting_thresholds_new(motion_no_confidence, committee_normal, committee_no_confidence, update_constitution, hard_fork_initiation, pp_network_group, pp_economic_group, pp_technical_group, pp_governance_group, treasury_withdrawal, &drep_voting_thresholds), CARDANO_SUCCESS);

  cardano_unit_interval_unref(&motion_no_confidence);
  cardano_unit_interval_unref(&committee_normal);
  cardano_unit_interval_unref(&committee_no_confidence);
  cardano_unit_interval_unref(&update_constitution);
  cardano_unit_interval_unref(&hard_fork_initiation);
  cardano_unit_interval_unref(&pp_network_group);
  cardano_unit_interval_unref(&pp_economic_group);
  cardano_unit_interval_unref(&pp_technical_group);
  cardano_unit_interval_unref(&pp_governance_group);
  cardano_unit_interval_unref(&treasury_withdrawal);

  return drep_voting_thresholds;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_drep_voting_thresholds_new, canCreate)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Assert
  EXPECT_THAT(drep_voting_thresholds, testing::Not((cardano_drep_voting_thresholds_t*)nullptr));

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_new, returnErrorIfFirstParamIsNull)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;

  // Assert
  EXPECT_EQ(cardano_drep_voting_thresholds_new(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_drep_voting_thresholds_new(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &drep_voting_thresholds), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_drep_voting_thresholds_new((cardano_unit_interval_t*)"", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &drep_voting_thresholds), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_drep_voting_thresholds_new((cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &drep_voting_thresholds), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_drep_voting_thresholds_new((cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &drep_voting_thresholds), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_drep_voting_thresholds_new((cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &drep_voting_thresholds), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_drep_voting_thresholds_new((cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", nullptr, nullptr, nullptr, nullptr, nullptr, &drep_voting_thresholds), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_drep_voting_thresholds_new((cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", nullptr, nullptr, nullptr, nullptr, &drep_voting_thresholds), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_drep_voting_thresholds_new((cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", nullptr, nullptr, nullptr, &drep_voting_thresholds), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_drep_voting_thresholds_new((cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", nullptr, nullptr, &drep_voting_thresholds), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_drep_voting_thresholds_new((cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", (cardano_unit_interval_t*)"", nullptr, &drep_voting_thresholds), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_unit_interval_t* motion_no_confidence    = NULL;
  cardano_unit_interval_t* committee_normal        = NULL;
  cardano_unit_interval_t* committee_no_confidence = NULL;
  cardano_unit_interval_t* update_constitution     = NULL;
  cardano_unit_interval_t* hard_fork_initiation    = NULL;
  cardano_unit_interval_t* pp_network_group        = NULL;
  cardano_unit_interval_t* pp_economic_group       = NULL;
  cardano_unit_interval_t* pp_technical_group      = NULL;
  cardano_unit_interval_t* pp_governance_group     = NULL;
  cardano_unit_interval_t* treasury_withdrawal     = NULL;

  EXPECT_EQ(cardano_unit_interval_new(0, 0, &motion_no_confidence), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(1, 1, &committee_normal), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(2, 2, &committee_no_confidence), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(3, 3, &update_constitution), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(4, 4, &hard_fork_initiation), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(5, 5, &pp_network_group), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(6, 6, &pp_economic_group), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(7, 7, &pp_technical_group), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(8, 8, &pp_governance_group), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(9, 9, &treasury_withdrawal), CARDANO_SUCCESS);

  // Act
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);
  cardano_error_t error = cardano_drep_voting_thresholds_new(motion_no_confidence, committee_normal, committee_no_confidence, update_constitution, hard_fork_initiation, pp_network_group, pp_economic_group, pp_technical_group, pp_governance_group, treasury_withdrawal, &drep_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_unit_interval_unref(&motion_no_confidence);
  cardano_unit_interval_unref(&committee_normal);
  cardano_unit_interval_unref(&committee_no_confidence);
  cardano_unit_interval_unref(&update_constitution);
  cardano_unit_interval_unref(&hard_fork_initiation);
  cardano_unit_interval_unref(&pp_network_group);
  cardano_unit_interval_unref(&pp_economic_group);
  cardano_unit_interval_unref(&pp_technical_group);
  cardano_unit_interval_unref(&pp_governance_group);
  cardano_unit_interval_unref(&treasury_withdrawal);
}

TEST(cardano_drep_voting_thresholds_to_cbor, canSerializeCostModel)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();
  cardano_cbor_writer_t*            writer                 = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_to_cbor(drep_voting_thresholds, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  cbor_hex = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, cbor_hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, CBOR);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_cbor_writer_unref(&writer);
  free(cbor_hex);
}

TEST(cardano_drep_voting_thresholds_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_drep_voting_thresholds_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_to_cbor(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_from_cbor, canDeserializeCbor)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_from_cbor(reader, &drep_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(drep_voting_thresholds, testing::Not((cardano_drep_voting_thresholds_t*)nullptr));

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_voting_thresholds_from_cbor, returnsErrorIdInvalidArray)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("04", 2);

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_from_cbor(reader, &drep_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_voting_thresholds_from_cbor, returnErrorIfInvalidMotionNoConfidense)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("8ad81ea20000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909", strlen("8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909"));

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_from_cbor(reader, &drep_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_voting_thresholds_from_cbor, returnErrorIfInvalidComitteeNotmal)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("8ad81e820000d81ea20101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909", strlen("8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909"));

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_from_cbor(reader, &drep_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_voting_thresholds_from_cbor, returnErrorIfInvalidCommiteeNoConfidence)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("8ad81e820000d81e820101d81ea20202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909", strlen("8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909"));

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_from_cbor(reader, &drep_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_voting_thresholds_from_cbor, returnErrorIfInvalidUpdateConstitution)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("8ad81e820000d81e820101d81e820202d81ea20303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909", strlen("8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909"));

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_from_cbor(reader, &drep_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_voting_thresholds_from_cbor, returnErrorIfInvalidHardForkInitiation)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("8ad81e820000d81e820101d81e820202d81e820303d81ea20404d81e820505d81e820606d81e820707d81e820808d81e820909", strlen("8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909"));

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_from_cbor(reader, &drep_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_voting_thresholds_from_cbor, returnErrorIfInvalidPpNetworkGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("8ad81e820000d81e820101d81e820202d81e820303d81e820404d81ea20505d81e820606d81e820707d81e820808d81e820909", strlen("8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909"));

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_from_cbor(reader, &drep_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_voting_thresholds_from_cbor, returnErrorIfInvalidPpEconomicGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81ea20606d81e820707d81e820808d81e820909", strlen("8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909"));

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_from_cbor(reader, &drep_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_voting_thresholds_from_cbor, returnErrorIfInvalidPpTechnicalGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81ea20707d81e820808d81e820909", strlen("8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909"));

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_from_cbor(reader, &drep_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_voting_thresholds_from_cbor, returnErrorIfInvalidPpGovernanceGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81ea20808d81e820909", strlen("8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909"));

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_from_cbor(reader, &drep_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_voting_thresholds_from_cbor, returnErrorIfInvalidTreasuryWithdrawal)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81ea20909", strlen("8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909"));

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_from_cbor(reader, &drep_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_voting_thresholds_from_cbor, returnErrorIfGivenNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_voting_thresholds_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_from_cbor(nullptr, &drep_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_drep_voting_thresholds_from_cbor(reader, &drep_voting_thresholds);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_drep_voting_thresholds_ref(drep_voting_thresholds);

  // Assert
  EXPECT_THAT(drep_voting_thresholds, testing::Not((cardano_drep_voting_thresholds_t*)nullptr));
  EXPECT_EQ(cardano_drep_voting_thresholds_refcount(drep_voting_thresholds), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_voting_thresholds_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_drep_voting_thresholds_ref(nullptr);
}

TEST(cardano_drep_voting_thresholds_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;

  // Act
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_drep_voting_thresholds_unref((cardano_drep_voting_thresholds_t**)nullptr);
}

TEST(cardano_drep_voting_thresholds_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_drep_voting_thresholds_from_cbor(reader, &drep_voting_thresholds);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_drep_voting_thresholds_ref(drep_voting_thresholds);
  size_t ref_count = cardano_drep_voting_thresholds_refcount(drep_voting_thresholds);

  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  size_t updated_ref_count = cardano_drep_voting_thresholds_refcount(drep_voting_thresholds);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_voting_thresholds_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_drep_voting_thresholds_from_cbor(reader, &drep_voting_thresholds);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_drep_voting_thresholds_ref(drep_voting_thresholds);
  size_t ref_count = cardano_drep_voting_thresholds_refcount(drep_voting_thresholds);

  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  size_t updated_ref_count = cardano_drep_voting_thresholds_refcount(drep_voting_thresholds);

  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(drep_voting_thresholds, (cardano_drep_voting_thresholds_t*)nullptr);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_voting_thresholds_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_drep_voting_thresholds_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_drep_voting_thresholds_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  const char*                       message                = "This is a test message";

  // Act
  cardano_drep_voting_thresholds_set_last_error(drep_voting_thresholds, message);

  // Assert
  EXPECT_STREQ(cardano_drep_voting_thresholds_get_last_error(drep_voting_thresholds), "Object is NULL.");
}

TEST(cardano_drep_voting_thresholds_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t error = cardano_drep_voting_thresholds_from_cbor(reader, &drep_voting_thresholds);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_drep_voting_thresholds_set_last_error(drep_voting_thresholds, message);

  // Assert
  EXPECT_STREQ(cardano_drep_voting_thresholds_get_last_error(drep_voting_thresholds), "");

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_voting_thresholds_get_motion_no_confidence, canReturnTheMotion)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_unit_interval_t* motion_no_confidence = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_motion_no_confidence(drep_voting_thresholds, &motion_no_confidence), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(motion_no_confidence, testing::Not((cardano_unit_interval_t*)nullptr));

  uint64_t denominator = cardano_unit_interval_get_denominator(motion_no_confidence);
  uint64_t numerator   = cardano_unit_interval_get_numerator(motion_no_confidence);

  EXPECT_EQ(denominator, 0);
  EXPECT_EQ(numerator, 0);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&motion_no_confidence);
}

TEST(cardano_drep_voting_thresholds_get_motion_no_confidence, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;

  // Act
  cardano_unit_interval_t* motion_no_confidence = NULL;

  cardano_error_t error = cardano_drep_voting_thresholds_get_motion_no_confidence(drep_voting_thresholds, &motion_no_confidence);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_get_motion_no_confidence, returnsErrorIfGivenANullPtrForTheMotion)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_get_motion_no_confidence(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_get_committee_normal, canReturnTheCommitteeNormal)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_unit_interval_t* committee_normal = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_committee_normal(drep_voting_thresholds, &committee_normal), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(committee_normal, testing::Not((cardano_unit_interval_t*)nullptr));

  uint64_t denominator = cardano_unit_interval_get_denominator(committee_normal);
  uint64_t numerator   = cardano_unit_interval_get_numerator(committee_normal);

  EXPECT_EQ(denominator, 1);
  EXPECT_EQ(numerator, 1);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&committee_normal);
}

TEST(cardano_drep_voting_thresholds_get_committee_normal, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;

  // Act
  cardano_unit_interval_t* committee_normal = NULL;

  cardano_error_t error = cardano_drep_voting_thresholds_get_committee_normal(drep_voting_thresholds, &committee_normal);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_get_committee_normal, returnsErrorIfGivenANullPtrForTheCommitteeNormal)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_get_committee_normal(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_get_committee_no_confidence, canReturnTheCommitteeNoConfidence)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_unit_interval_t* committee_no_confidence = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_committee_no_confidence(drep_voting_thresholds, &committee_no_confidence), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(committee_no_confidence, testing::Not((cardano_unit_interval_t*)nullptr));

  uint64_t denominator = cardano_unit_interval_get_denominator(committee_no_confidence);
  uint64_t numerator   = cardano_unit_interval_get_numerator(committee_no_confidence);

  EXPECT_EQ(denominator, 2);
  EXPECT_EQ(numerator, 2);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&committee_no_confidence);
}

TEST(cardano_drep_voting_thresholds_get_committee_no_confidence, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;

  // Act
  cardano_unit_interval_t* committee_no_confidence = NULL;

  cardano_error_t error = cardano_drep_voting_thresholds_get_committee_no_confidence(drep_voting_thresholds, &committee_no_confidence);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_get_committee_no_confidence, returnsErrorIfGivenANullPtrForTheCommitteeNoConfidence)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_get_committee_no_confidence(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_get_update_constitution, canReturnTheUpdateConstitution)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_unit_interval_t* update_constitution = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_update_constitution(drep_voting_thresholds, &update_constitution), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(update_constitution, testing::Not((cardano_unit_interval_t*)nullptr));

  uint64_t denominator = cardano_unit_interval_get_denominator(update_constitution);
  uint64_t numerator   = cardano_unit_interval_get_numerator(update_constitution);

  EXPECT_EQ(denominator, 3);
  EXPECT_EQ(numerator, 3);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&update_constitution);
}

TEST(cardano_drep_voting_thresholds_get_update_constitution, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;

  // Act
  cardano_unit_interval_t* update_constitution = NULL;

  cardano_error_t error = cardano_drep_voting_thresholds_get_update_constitution(drep_voting_thresholds, &update_constitution);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_get_update_constitution, returnsErrorIfGivenANullPtrForTheUpdateConstitution)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_get_update_constitution(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_get_hard_fork_initiation, canReturnTheHardForkInitiation)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_unit_interval_t* hard_fork_initiation = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_hard_fork_initiation(drep_voting_thresholds, &hard_fork_initiation), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(hard_fork_initiation, testing::Not((cardano_unit_interval_t*)nullptr));

  uint64_t denominator = cardano_unit_interval_get_denominator(hard_fork_initiation);
  uint64_t numerator   = cardano_unit_interval_get_numerator(hard_fork_initiation);

  EXPECT_EQ(denominator, 4);
  EXPECT_EQ(numerator, 4);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&hard_fork_initiation);
}

TEST(cardano_drep_voting_thresholds_get_hard_fork_initiation, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;

  // Act
  cardano_unit_interval_t* hard_fork_initiation = NULL;

  cardano_error_t error = cardano_drep_voting_thresholds_get_hard_fork_initiation(drep_voting_thresholds, &hard_fork_initiation);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_get_hard_fork_initiation, returnsErrorIfGivenANullPtrForTheHardForkInitiation)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_get_hard_fork_initiation(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_get_pp_network_group, canReturnThePpNetworkGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_unit_interval_t* pp_network_group = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_pp_network_group(drep_voting_thresholds, &pp_network_group), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(pp_network_group, testing::Not((cardano_unit_interval_t*)nullptr));

  uint64_t denominator = cardano_unit_interval_get_denominator(pp_network_group);
  uint64_t numerator   = cardano_unit_interval_get_numerator(pp_network_group);

  EXPECT_EQ(denominator, 5);
  EXPECT_EQ(numerator, 5);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&pp_network_group);
}

TEST(cardano_drep_voting_thresholds_get_pp_network_group, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;

  // Act
  cardano_unit_interval_t* pp_network_group = NULL;

  cardano_error_t error = cardano_drep_voting_thresholds_get_pp_network_group(drep_voting_thresholds, &pp_network_group);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_get_pp_network_group, returnsErrorIfGivenANullPtrForThePpNetworkGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_get_pp_network_group(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_get_pp_economic_group, canReturnThePpEconomicGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_unit_interval_t* pp_economic_group = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_pp_economic_group(drep_voting_thresholds, &pp_economic_group), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(pp_economic_group, testing::Not((cardano_unit_interval_t*)nullptr));

  uint64_t denominator = cardano_unit_interval_get_denominator(pp_economic_group);
  uint64_t numerator   = cardano_unit_interval_get_numerator(pp_economic_group);

  EXPECT_EQ(denominator, 6);
  EXPECT_EQ(numerator, 6);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&pp_economic_group);
}

TEST(cardano_drep_voting_thresholds_get_pp_economic_group, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;

  // Act
  cardano_unit_interval_t* pp_economic_group = NULL;

  cardano_error_t error = cardano_drep_voting_thresholds_get_pp_economic_group(drep_voting_thresholds, &pp_economic_group);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_get_pp_economic_group, returnsErrorIfGivenANullPtrForThePpEconomicGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_get_pp_economic_group(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_get_pp_technical_group, canReturnThePpTechnicalGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_unit_interval_t* pp_technical_group = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_pp_technical_group(drep_voting_thresholds, &pp_technical_group), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(pp_technical_group, testing::Not((cardano_unit_interval_t*)nullptr));

  uint64_t denominator = cardano_unit_interval_get_denominator(pp_technical_group);
  uint64_t numerator   = cardano_unit_interval_get_numerator(pp_technical_group);

  EXPECT_EQ(denominator, 7);
  EXPECT_EQ(numerator, 7);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&pp_technical_group);
}

TEST(cardano_drep_voting_thresholds_get_pp_technical_group, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;

  // Act
  cardano_unit_interval_t* pp_technical_group = NULL;

  cardano_error_t error = cardano_drep_voting_thresholds_get_pp_technical_group(drep_voting_thresholds, &pp_technical_group);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_get_pp_technical_group, returnsErrorIfGivenANullPtrForThePpTechnicalGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_get_pp_technical_group(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_get_pp_governance_group, canReturnThePpGovernanceGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_unit_interval_t* pp_governance_group = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_pp_governance_group(drep_voting_thresholds, &pp_governance_group), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(pp_governance_group, testing::Not((cardano_unit_interval_t*)nullptr));

  uint64_t denominator = cardano_unit_interval_get_denominator(pp_governance_group);
  uint64_t numerator   = cardano_unit_interval_get_numerator(pp_governance_group);

  EXPECT_EQ(denominator, 8);
  EXPECT_EQ(numerator, 8);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&pp_governance_group);
}

TEST(cardano_drep_voting_thresholds_get_pp_governance_group, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;

  // Act
  cardano_unit_interval_t* pp_governance_group = NULL;

  cardano_error_t error = cardano_drep_voting_thresholds_get_pp_governance_group(drep_voting_thresholds, &pp_governance_group);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_get_pp_governance_group, returnsErrorIfGivenANullPtrForThePpGovernanceGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_get_pp_governance_group(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_get_treasury_withdrawal, canReturnTheTreasuryWithdrawal)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_unit_interval_t* treasury_withdrawal = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_treasury_withdrawal(drep_voting_thresholds, &treasury_withdrawal), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(treasury_withdrawal, testing::Not((cardano_unit_interval_t*)nullptr));

  uint64_t denominator = cardano_unit_interval_get_denominator(treasury_withdrawal);
  uint64_t numerator   = cardano_unit_interval_get_numerator(treasury_withdrawal);

  EXPECT_EQ(denominator, 9);
  EXPECT_EQ(numerator, 9);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&treasury_withdrawal);
}

TEST(cardano_drep_voting_thresholds_get_treasury_withdrawal, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;

  // Act
  cardano_unit_interval_t* treasury_withdrawal = NULL;

  cardano_error_t error = cardano_drep_voting_thresholds_get_treasury_withdrawal(drep_voting_thresholds, &treasury_withdrawal);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_get_treasury_withdrawal, returnsErrorIfGivenANullPtrForTheTreasuryWithdrawal)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_get_treasury_withdrawal(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_set_motion_no_confidence, canSetTheMotionNoConfidence)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();
  cardano_unit_interval_t*          motion_no_confidence   = NULL;

  EXPECT_EQ(cardano_unit_interval_new(99, 99, &motion_no_confidence), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_drep_voting_thresholds_set_motion_no_confidence(drep_voting_thresholds, motion_no_confidence), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* motion_no_confidence_result = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_motion_no_confidence(drep_voting_thresholds, &motion_no_confidence_result), CARDANO_SUCCESS);

  uint64_t denominator = cardano_unit_interval_get_denominator(motion_no_confidence_result);
  uint64_t numerator   = cardano_unit_interval_get_numerator(motion_no_confidence_result);

  EXPECT_EQ(denominator, 99);
  EXPECT_EQ(numerator, 99);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&motion_no_confidence);
  cardano_unit_interval_unref(&motion_no_confidence_result);
}

TEST(cardano_drep_voting_thresholds_set_motion_no_confidence, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_unit_interval_t*          motion_no_confidence   = NULL;

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_motion_no_confidence(drep_voting_thresholds, motion_no_confidence);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_set_motion_no_confidence, returnsErrorIfGivenANullPtrForTheMotionNoConfidence)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_motion_no_confidence(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_set_committee_normal, canSetTheCommitteeNormal)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();
  cardano_unit_interval_t*          committee_normal       = NULL;

  EXPECT_EQ(cardano_unit_interval_new(98, 98, &committee_normal), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_drep_voting_thresholds_set_committee_normal(drep_voting_thresholds, committee_normal), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* committee_normal_result = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_committee_normal(drep_voting_thresholds, &committee_normal_result), CARDANO_SUCCESS);

  uint64_t denominator = cardano_unit_interval_get_denominator(committee_normal_result);
  uint64_t numerator   = cardano_unit_interval_get_numerator(committee_normal_result);

  EXPECT_EQ(denominator, 98);
  EXPECT_EQ(numerator, 98);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&committee_normal);
  cardano_unit_interval_unref(&committee_normal_result);
}

TEST(cardano_drep_voting_thresholds_set_committee_normal, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_unit_interval_t*          committee_normal       = NULL;

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_committee_normal(drep_voting_thresholds, committee_normal);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_set_committee_normal, returnsErrorIfGivenANullPtrForTheCommitteeNormal)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_committee_normal(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_set_committee_no_confidence, canSetTheCommitteeNoConfidence)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds  = init_drep_voting_thresholds();
  cardano_unit_interval_t*          committee_no_confidence = NULL;

  EXPECT_EQ(cardano_unit_interval_new(97, 97, &committee_no_confidence), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_drep_voting_thresholds_set_committee_no_confidence(drep_voting_thresholds, committee_no_confidence), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* committee_no_confidence_result = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_committee_no_confidence(drep_voting_thresholds, &committee_no_confidence_result), CARDANO_SUCCESS);

  uint64_t denominator = cardano_unit_interval_get_denominator(committee_no_confidence_result);
  uint64_t numerator   = cardano_unit_interval_get_numerator(committee_no_confidence_result);

  EXPECT_EQ(denominator, 97);
  EXPECT_EQ(numerator, 97);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&committee_no_confidence);
  cardano_unit_interval_unref(&committee_no_confidence_result);
}

TEST(cardano_drep_voting_thresholds_set_committee_no_confidence, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds  = nullptr;
  cardano_unit_interval_t*          committee_no_confidence = NULL;

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_committee_no_confidence(drep_voting_thresholds, committee_no_confidence);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_set_committee_no_confidence, returnsErrorIfGivenANullPtrForTheCommitteeNoConfidence)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_committee_no_confidence(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_set_update_constitution, canSetTheUpdateConstitution)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();
  cardano_unit_interval_t*          update_constitution    = NULL;

  EXPECT_EQ(cardano_unit_interval_new(96, 96, &update_constitution), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_drep_voting_thresholds_set_update_constitution(drep_voting_thresholds, update_constitution), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* update_constitution_result = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_update_constitution(drep_voting_thresholds, &update_constitution_result), CARDANO_SUCCESS);

  uint64_t denominator = cardano_unit_interval_get_denominator(update_constitution_result);
  uint64_t numerator   = cardano_unit_interval_get_numerator(update_constitution_result);

  EXPECT_EQ(denominator, 96);
  EXPECT_EQ(numerator, 96);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&update_constitution);
  cardano_unit_interval_unref(&update_constitution_result);
}

TEST(cardano_drep_voting_thresholds_set_update_constitution, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_unit_interval_t*          update_constitution    = NULL;

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_update_constitution(drep_voting_thresholds, update_constitution);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_set_update_constitution, returnsErrorIfGivenANullPtrForTheUpdateConstitution)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_update_constitution(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_set_hard_fork_initiation, canSetTheHardForkInitiation)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();
  cardano_unit_interval_t*          hard_fork_initiation   = NULL;

  EXPECT_EQ(cardano_unit_interval_new(95, 95, &hard_fork_initiation), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_drep_voting_thresholds_set_hard_fork_initiation(drep_voting_thresholds, hard_fork_initiation), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* hard_fork_initiation_result = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_hard_fork_initiation(drep_voting_thresholds, &hard_fork_initiation_result), CARDANO_SUCCESS);

  uint64_t denominator = cardano_unit_interval_get_denominator(hard_fork_initiation_result);
  uint64_t numerator   = cardano_unit_interval_get_numerator(hard_fork_initiation_result);

  EXPECT_EQ(denominator, 95);
  EXPECT_EQ(numerator, 95);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&hard_fork_initiation);
  cardano_unit_interval_unref(&hard_fork_initiation_result);
}

TEST(cardano_drep_voting_thresholds_set_hard_fork_initiation, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_unit_interval_t*          hard_fork_initiation   = NULL;

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_hard_fork_initiation(drep_voting_thresholds, hard_fork_initiation);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_set_hard_fork_initiation, returnsErrorIfGivenANullPtrForTheHardForkInitiation)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_hard_fork_initiation(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_set_pp_network_group, canSetThePpNetworkGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();
  cardano_unit_interval_t*          pp_network_group       = NULL;

  EXPECT_EQ(cardano_unit_interval_new(94, 94, &pp_network_group), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_drep_voting_thresholds_set_pp_network_group(drep_voting_thresholds, pp_network_group), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* pp_network_group_result = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_pp_network_group(drep_voting_thresholds, &pp_network_group_result), CARDANO_SUCCESS);

  uint64_t denominator = cardano_unit_interval_get_denominator(pp_network_group_result);
  uint64_t numerator   = cardano_unit_interval_get_numerator(pp_network_group_result);

  EXPECT_EQ(denominator, 94);
  EXPECT_EQ(numerator, 94);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&pp_network_group);
  cardano_unit_interval_unref(&pp_network_group_result);
}

TEST(cardano_drep_voting_thresholds_set_pp_network_group, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_unit_interval_t*          pp_network_group       = NULL;

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_pp_network_group(drep_voting_thresholds, pp_network_group);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_set_pp_network_group, returnsErrorIfGivenANullPtrForThePpNetworkGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_pp_network_group(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_set_pp_economic_group, canSetThePpEconomicGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();
  cardano_unit_interval_t*          pp_economic_group      = NULL;

  EXPECT_EQ(cardano_unit_interval_new(93, 93, &pp_economic_group), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_drep_voting_thresholds_set_pp_economic_group(drep_voting_thresholds, pp_economic_group), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* pp_economic_group_result = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_pp_economic_group(drep_voting_thresholds, &pp_economic_group_result), CARDANO_SUCCESS);

  uint64_t denominator = cardano_unit_interval_get_denominator(pp_economic_group_result);
  uint64_t numerator   = cardano_unit_interval_get_numerator(pp_economic_group_result);

  EXPECT_EQ(denominator, 93);
  EXPECT_EQ(numerator, 93);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&pp_economic_group);
  cardano_unit_interval_unref(&pp_economic_group_result);
}

TEST(cardano_drep_voting_thresholds_set_pp_economic_group, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_unit_interval_t*          pp_economic_group      = NULL;

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_pp_economic_group(drep_voting_thresholds, pp_economic_group);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_set_pp_economic_group, returnsErrorIfGivenANullPtrForThePpEconomicGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_pp_economic_group(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_set_pp_technical_group, canSetThePpTechnicalGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();
  cardano_unit_interval_t*          pp_technical_group     = NULL;

  EXPECT_EQ(cardano_unit_interval_new(92, 92, &pp_technical_group), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_drep_voting_thresholds_set_pp_technical_group(drep_voting_thresholds, pp_technical_group), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* pp_technical_group_result = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_pp_technical_group(drep_voting_thresholds, &pp_technical_group_result), CARDANO_SUCCESS);

  uint64_t denominator = cardano_unit_interval_get_denominator(pp_technical_group_result);
  uint64_t numerator   = cardano_unit_interval_get_numerator(pp_technical_group_result);

  EXPECT_EQ(denominator, 92);
  EXPECT_EQ(numerator, 92);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&pp_technical_group);
  cardano_unit_interval_unref(&pp_technical_group_result);
}

TEST(cardano_drep_voting_thresholds_set_pp_technical_group, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_unit_interval_t*          pp_technical_group     = NULL;

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_pp_technical_group(drep_voting_thresholds, pp_technical_group);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_set_pp_technical_group, returnsErrorIfGivenANullPtrForThePpTechnicalGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_pp_technical_group(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_set_pp_governance_group, canSetThePpGovernanceGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();
  cardano_unit_interval_t*          pp_governance_group    = NULL;

  EXPECT_EQ(cardano_unit_interval_new(91, 91, &pp_governance_group), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_drep_voting_thresholds_set_pp_governance_group(drep_voting_thresholds, pp_governance_group), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* pp_governance_group_result = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_pp_governance_group(drep_voting_thresholds, &pp_governance_group_result), CARDANO_SUCCESS);

  uint64_t denominator = cardano_unit_interval_get_denominator(pp_governance_group_result);
  uint64_t numerator   = cardano_unit_interval_get_numerator(pp_governance_group_result);

  EXPECT_EQ(denominator, 91);
  EXPECT_EQ(numerator, 91);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&pp_governance_group);
  cardano_unit_interval_unref(&pp_governance_group_result);
}

TEST(cardano_drep_voting_thresholds_set_pp_governance_group, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_unit_interval_t*          pp_governance_group    = NULL;

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_pp_governance_group(drep_voting_thresholds, pp_governance_group);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_set_pp_governance_group, returnsErrorIfGivenANullPtrForThePpGovernanceGroup)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_pp_governance_group(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}

TEST(cardano_drep_voting_thresholds_set_treasury_withdrawal, canSetTheTreasuryWithdrawal)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();
  cardano_unit_interval_t*          treasury_withdrawal    = NULL;

  EXPECT_EQ(cardano_unit_interval_new(90, 90, &treasury_withdrawal), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_drep_voting_thresholds_set_treasury_withdrawal(drep_voting_thresholds, treasury_withdrawal), CARDANO_SUCCESS);

  // Assert
  cardano_unit_interval_t* treasury_withdrawal_result = NULL;

  EXPECT_EQ(cardano_drep_voting_thresholds_get_treasury_withdrawal(drep_voting_thresholds, &treasury_withdrawal_result), CARDANO_SUCCESS);

  uint64_t denominator = cardano_unit_interval_get_denominator(treasury_withdrawal_result);
  uint64_t numerator   = cardano_unit_interval_get_numerator(treasury_withdrawal_result);

  EXPECT_EQ(denominator, 90);
  EXPECT_EQ(numerator, 90);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_unit_interval_unref(&treasury_withdrawal);
  cardano_unit_interval_unref(&treasury_withdrawal_result);
}

TEST(cardano_drep_voting_thresholds_set_treasury_withdrawal, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;
  cardano_unit_interval_t*          treasury_withdrawal    = NULL;

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_treasury_withdrawal(drep_voting_thresholds, treasury_withdrawal);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_voting_thresholds_set_treasury_withdrawal, returnsErrorIfGivenANullPtrForTheTreasuryWithdrawal)
{
  // Arrange
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = init_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_drep_voting_thresholds_set_treasury_withdrawal(drep_voting_thresholds, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
}
