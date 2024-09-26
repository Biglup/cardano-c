/**
 * \file protocol_parameters.cpp
 *
 * \author angel.castillo
 * \date   Sep 26, 2024
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
#include <cardano/protocol_params/protocol_parameters.h>

#include "../allocators_helpers.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static cardano_protocol_parameters_t*
init_protocol_parameters()
{
  cardano_protocol_parameters_t* parameters = NULL;

  cardano_error_t error = cardano_protocol_parameters_new(&parameters);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  return parameters;
}

/**
 * Creates a unit interval with 0/0 values.
 *
 * @return The new unit interval.
 */
static cardano_unit_interval_t*
cardano_get_zero_interval()
{
  cardano_unit_interval_t* unit_interval = NULL;

  cardano_error_t result = cardano_unit_interval_new(1, 1, &unit_interval);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return unit_interval;
}

/**
 * Creates a protocol version with 0 0 values.
 *
 * @return The protocol version.
 */
static cardano_protocol_version_t*
cardano_get_protocol_version()
{
  cardano_protocol_version_t* protocol_version = NULL;

  cardano_error_t result = cardano_protocol_version_new(0, 0, &protocol_version);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return protocol_version;
}

/**
 * Creates empty costmdls.
 *
 * @return The costmdls.
 */
static cardano_costmdls_t*
cardano_get_costmdls()
{
  cardano_costmdls_t* costmdls = NULL;

  cardano_error_t result = cardano_costmdls_new(&costmdls);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return costmdls;
}

/**
 * Creates ex unit prices.
 *
 * @return The ex unit prices.
 */
static cardano_ex_unit_prices_t*
cardano_get_ex_unit_prices()
{
  cardano_ex_unit_prices_t* ex_unit_prices = NULL;

  cardano_unit_interval_t* memory_prices = cardano_get_zero_interval();
  cardano_unit_interval_t* steps_prices  = cardano_get_zero_interval();

  cardano_error_t result = cardano_ex_unit_prices_new(memory_prices, steps_prices, &ex_unit_prices);

  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return ex_unit_prices;
}

/**
 * Creates ex units.
 *
 * @return The ex units.
 */
static cardano_ex_units_t*
cardano_get_ex_unit()
{
  cardano_ex_units_t* ex_units = NULL;

  cardano_error_t result = cardano_ex_units_new(0, 0, &ex_units);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return ex_units;
}

/**
 * Creates voting thresholds.
 *
 * @return The voting thresholds.
 */
static cardano_pool_voting_thresholds_t*
cardano_get_pool_voting_thresholds()
{
  cardano_unit_interval_t* motion_no_confidence    = cardano_get_zero_interval();
  cardano_unit_interval_t* committee_normal        = cardano_get_zero_interval();
  cardano_unit_interval_t* committee_no_confidence = cardano_get_zero_interval();
  cardano_unit_interval_t* hard_fork_initiation    = cardano_get_zero_interval();
  cardano_unit_interval_t* security_relevant_param = cardano_get_zero_interval();

  cardano_pool_voting_thresholds_t* thresholds = NULL;

  cardano_error_t result = cardano_pool_voting_thresholds_new(
    motion_no_confidence,
    committee_normal,
    committee_no_confidence,
    hard_fork_initiation,
    security_relevant_param,
    &thresholds);

  cardano_unit_interval_unref(&motion_no_confidence);
  cardano_unit_interval_unref(&committee_normal);
  cardano_unit_interval_unref(&committee_no_confidence);
  cardano_unit_interval_unref(&hard_fork_initiation);
  cardano_unit_interval_unref(&security_relevant_param);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return thresholds;
}

/**
 * Creates voting thresholds.
 *
 * @return The voting thresholds.
 */
static cardano_drep_voting_thresholds_t*
cardano_get_drep_voting_thresholds()
{
  cardano_unit_interval_t* motion_no_confidence    = cardano_get_zero_interval();
  cardano_unit_interval_t* committee_normal        = cardano_get_zero_interval();
  cardano_unit_interval_t* committee_no_confidence = cardano_get_zero_interval();
  cardano_unit_interval_t* update_constitution     = cardano_get_zero_interval();
  cardano_unit_interval_t* hard_fork_initiation    = cardano_get_zero_interval();
  cardano_unit_interval_t* pp_network_group        = cardano_get_zero_interval();
  cardano_unit_interval_t* pp_economic_group       = cardano_get_zero_interval();
  cardano_unit_interval_t* pp_technical_group      = cardano_get_zero_interval();
  cardano_unit_interval_t* pp_governance_group     = cardano_get_zero_interval();
  cardano_unit_interval_t* treasury_withdrawal     = cardano_get_zero_interval();

  cardano_drep_voting_thresholds_t* thresholds = NULL;

  cardano_error_t result = cardano_drep_voting_thresholds_new(
    motion_no_confidence,
    committee_normal,
    committee_no_confidence,
    update_constitution,
    hard_fork_initiation,
    pp_network_group,
    pp_economic_group,
    pp_technical_group,
    pp_governance_group,
    treasury_withdrawal,
    &thresholds);

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

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return thresholds;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_protocol_parameters_new, canCreate)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();

  // Assert
  EXPECT_THAT(protocol_parameters, testing::Not((cardano_protocol_parameters_t*)nullptr));

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();

  // Act
  cardano_protocol_parameters_ref(protocol_parameters);

  // Assert
  EXPECT_THAT(protocol_parameters, testing::Not((cardano_protocol_parameters_t*)nullptr));
  EXPECT_EQ(cardano_protocol_parameters_refcount(protocol_parameters), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_protocol_parameters_unref(&protocol_parameters);
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_protocol_parameters_ref(nullptr);
}

TEST(cardano_protocol_parameters_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_protocol_parameters_unref((cardano_protocol_parameters_t**)nullptr);
}

TEST(cardano_protocol_parameters_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();

  // Act
  cardano_protocol_parameters_ref(protocol_parameters);
  size_t ref_count = cardano_protocol_parameters_refcount(protocol_parameters);

  cardano_protocol_parameters_unref(&protocol_parameters);
  size_t updated_ref_count = cardano_protocol_parameters_refcount(protocol_parameters);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();

  // Act
  cardano_protocol_parameters_ref(protocol_parameters);
  size_t ref_count = cardano_protocol_parameters_refcount(protocol_parameters);

  cardano_protocol_parameters_unref(&protocol_parameters);
  size_t updated_ref_count = cardano_protocol_parameters_refcount(protocol_parameters);

  cardano_protocol_parameters_unref(&protocol_parameters);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(protocol_parameters, (cardano_protocol_parameters_t*)nullptr);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_protocol_parameters_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_protocol_parameters_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  const char*                    message             = "This is a test message";

  // Act
  cardano_protocol_parameters_set_last_error(protocol_parameters, message);

  // Assert
  EXPECT_STREQ(cardano_protocol_parameters_get_last_error(protocol_parameters), "Object is NULL.");
}

TEST(cardano_protocol_parameters_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();

  const char* message = nullptr;

  // Act
  cardano_protocol_parameters_set_last_error(protocol_parameters, message);

  // Assert
  EXPECT_STREQ(cardano_protocol_parameters_get_last_error(protocol_parameters), "");

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

// Getters and Setters

TEST(cardano_protocol_parameters_get_min_fee_a, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t min_fee_a = cardano_protocol_parameters_get_min_fee_a(protocol_parameters);

  // Assert
  EXPECT_EQ(min_fee_a, 0);
}

TEST(cardano_protocol_parameters_set_min_fee_a, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  uint64_t                       min_fee_a           = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_min_fee_a(protocol_parameters, min_fee_a);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_min_fee_a, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  uint64_t                       min_fee_a           = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_min_fee_a(protocol_parameters, min_fee_a);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_min_fee_a(protocol_parameters), min_fee_a);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_min_fee_b, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t min_fee_b = cardano_protocol_parameters_get_min_fee_b(protocol_parameters);

  // Assert
  EXPECT_EQ(min_fee_b, 0);
}

TEST(cardano_protocol_parameters_set_min_fee_b, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  uint64_t                       min_fee_b           = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_min_fee_b(protocol_parameters, min_fee_b);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_min_fee_b, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  uint64_t                       min_fee_b           = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_min_fee_b(protocol_parameters, min_fee_b);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_min_fee_b(protocol_parameters), min_fee_b);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_max_block_body_size, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t max_block_body_size = cardano_protocol_parameters_get_max_block_body_size(protocol_parameters);

  // Assert
  EXPECT_EQ(max_block_body_size, 0);
}

TEST(cardano_protocol_parameters_set_max_block_body_size, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  uint64_t                       max_block_body_size = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_max_block_body_size(protocol_parameters, max_block_body_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_max_block_body_size, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  uint64_t                       max_block_body_size = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_max_block_body_size(protocol_parameters, max_block_body_size);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_max_block_body_size(protocol_parameters), max_block_body_size);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_max_tx_size, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t max_tx_size = cardano_protocol_parameters_get_max_tx_size(protocol_parameters);

  // Assert
  EXPECT_EQ(max_tx_size, 0);
}

TEST(cardano_protocol_parameters_set_max_tx_size, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  uint64_t                       max_tx_size         = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_max_tx_size(protocol_parameters, max_tx_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_max_tx_size, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  uint64_t                       max_tx_size         = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_max_tx_size(protocol_parameters, max_tx_size);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_max_tx_size(protocol_parameters), max_tx_size);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_max_block_header_size, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t max_block_header_size = cardano_protocol_parameters_get_max_block_header_size(protocol_parameters);

  // Assert
  EXPECT_EQ(max_block_header_size, 0);
}

TEST(cardano_protocol_parameters_set_max_block_header_size, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters   = nullptr;
  uint64_t                       max_block_header_size = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_max_block_header_size(protocol_parameters, max_block_header_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_max_block_header_size, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters   = init_protocol_parameters();
  uint64_t                       max_block_header_size = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_max_block_header_size(protocol_parameters, max_block_header_size);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_max_block_header_size(protocol_parameters), max_block_header_size);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_key_deposit, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t key_deposit = cardano_protocol_parameters_get_key_deposit(protocol_parameters);

  // Assert
  EXPECT_EQ(key_deposit, 0);
}

TEST(cardano_protocol_parameters_set_key_deposit, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  uint64_t                       key_deposit         = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_key_deposit(protocol_parameters, key_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_key_deposit, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  uint64_t                       key_deposit         = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_key_deposit(protocol_parameters, key_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_key_deposit(protocol_parameters), key_deposit);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_pool_deposit, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t pool_deposit = cardano_protocol_parameters_get_pool_deposit(protocol_parameters);

  // Assert
  EXPECT_EQ(pool_deposit, 0);
}

TEST(cardano_protocol_parameters_set_pool_deposit, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  uint64_t                       pool_deposit        = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_pool_deposit(protocol_parameters, pool_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_pool_deposit, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  uint64_t                       pool_deposit        = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_pool_deposit(protocol_parameters, pool_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_pool_deposit(protocol_parameters), pool_deposit);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_max_epoch, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t max_epoch = cardano_protocol_parameters_get_max_epoch(protocol_parameters);

  // Assert
  EXPECT_EQ(max_epoch, 0);
}

TEST(cardano_protocol_parameters_set_max_epoch, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  uint64_t                       max_epoch           = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_max_epoch(protocol_parameters, max_epoch);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_max_epoch, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  uint64_t                       max_epoch           = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_max_epoch(protocol_parameters, max_epoch);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_max_epoch(protocol_parameters), max_epoch);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_n_opt, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t n_opt = cardano_protocol_parameters_get_n_opt(protocol_parameters);

  // Assert
  EXPECT_EQ(n_opt, 0);
}

TEST(cardano_protocol_parameters_set_n_opt, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  uint64_t                       n_opt               = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_n_opt(protocol_parameters, n_opt);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_n_opt, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  uint64_t                       n_opt               = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_n_opt(protocol_parameters, n_opt);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_n_opt(protocol_parameters), n_opt);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_pool_pledge_influence, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  cardano_unit_interval_t* pool_pledge_influence = cardano_protocol_parameters_get_pool_pledge_influence(protocol_parameters);

  // Assert
  EXPECT_EQ(pool_pledge_influence, (cardano_unit_interval_t*)NULL);
}

TEST(cardano_protocol_parameters_set_pool_pledge_influence, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters   = nullptr;
  cardano_unit_interval_t*       pool_pledge_influence = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_pool_pledge_influence(protocol_parameters, pool_pledge_influence);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_pool_pledge_influence, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters   = init_protocol_parameters();
  cardano_unit_interval_t*       pool_pledge_influence = cardano_get_zero_interval();

  // Act
  cardano_error_t          error                        = cardano_protocol_parameters_set_pool_pledge_influence(protocol_parameters, pool_pledge_influence);
  cardano_unit_interval_t* pool_pledge_influence_result = cardano_protocol_parameters_get_pool_pledge_influence(protocol_parameters);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(pool_pledge_influence_result, pool_pledge_influence);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
  cardano_unit_interval_unref(&pool_pledge_influence);
  cardano_unit_interval_unref(&pool_pledge_influence_result);
}

TEST(cardano_protocol_parameters_get_expansion_rate, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  cardano_unit_interval_t* expansion_rate = cardano_protocol_parameters_get_expansion_rate(protocol_parameters);

  // Assert
  EXPECT_EQ(expansion_rate, (cardano_unit_interval_t*)NULL);
}

TEST(cardano_protocol_parameters_set_expansion_rate, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  cardano_unit_interval_t*       expansion_rate      = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_expansion_rate(protocol_parameters, expansion_rate);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_expansion_rate, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  cardano_unit_interval_t*       expansion_rate      = cardano_get_zero_interval();

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_expansion_rate(protocol_parameters, expansion_rate);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_unit_interval_t* expansion_rate_result = cardano_protocol_parameters_get_expansion_rate(protocol_parameters);

  // Assert
  EXPECT_EQ(expansion_rate_result, expansion_rate);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
  cardano_unit_interval_unref(&expansion_rate);
  cardano_unit_interval_unref(&expansion_rate_result);
}

TEST(cardano_protocol_parameters_get_treasury_growth_rate, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  cardano_unit_interval_t* treasury_growth_rate = cardano_protocol_parameters_get_treasury_growth_rate(protocol_parameters);

  // Assert
  EXPECT_EQ(treasury_growth_rate, (cardano_unit_interval_t*)NULL);
}

TEST(cardano_protocol_parameters_set_treasury_growth_rate, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters  = nullptr;
  cardano_unit_interval_t*       treasury_growth_rate = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_treasury_growth_rate(protocol_parameters, treasury_growth_rate);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_treasury_growth_rate, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters  = init_protocol_parameters();
  cardano_unit_interval_t*       treasury_growth_rate = cardano_get_zero_interval();

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_treasury_growth_rate(protocol_parameters, treasury_growth_rate);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_unit_interval_t* treasury_growth_rate_result = cardano_protocol_parameters_get_treasury_growth_rate(protocol_parameters);

  // Assert
  EXPECT_EQ(treasury_growth_rate_result, treasury_growth_rate);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
  cardano_unit_interval_unref(&treasury_growth_rate);
  cardano_unit_interval_unref(&treasury_growth_rate_result);
}

TEST(cardano_protocol_parameters_get_d, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  cardano_unit_interval_t* d = cardano_protocol_parameters_get_d(protocol_parameters);

  // Assert
  EXPECT_EQ(d, (cardano_unit_interval_t*)NULL);
}

TEST(cardano_protocol_parameters_set_d, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  cardano_unit_interval_t*       d                   = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_d(protocol_parameters, d);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_d, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  cardano_unit_interval_t*       d                   = cardano_get_zero_interval();

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_d(protocol_parameters, d);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_unit_interval_t* d_result = cardano_protocol_parameters_get_d(protocol_parameters);

  // Assert
  EXPECT_EQ(d_result, d);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
  cardano_unit_interval_unref(&d);
  cardano_unit_interval_unref(&d_result);
}

TEST(cardano_protocol_parameters_get_extra_entropy, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  cardano_buffer_t* extra_entropy = cardano_protocol_parameters_get_extra_entropy(protocol_parameters);

  // Assert
  EXPECT_EQ(extra_entropy, (cardano_buffer_t*)0);
}

TEST(cardano_protocol_parameters_set_extra_entropy, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  cardano_buffer_t*              extra_entropy       = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_extra_entropy(protocol_parameters, extra_entropy);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_extra_entropy, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  cardano_buffer_t*              extra_entropy       = cardano_buffer_new(10);

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_extra_entropy(protocol_parameters, extra_entropy);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_buffer_t* extra_entropy_result = cardano_protocol_parameters_get_extra_entropy(protocol_parameters);

  // Assert
  EXPECT_EQ(extra_entropy_result, extra_entropy);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
  cardano_buffer_unref(&extra_entropy);
  cardano_buffer_unref(&extra_entropy_result);
}

TEST(cardano_protocol_parameters_get_protocol_version, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  cardano_protocol_version_t* protocol_version = cardano_protocol_parameters_get_protocol_version(protocol_parameters);

  // Assert
  EXPECT_EQ(protocol_version, (cardano_protocol_version_t*)0);
}

TEST(cardano_protocol_parameters_set_protocol_version, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  cardano_protocol_version_t*    protocol_version    = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_protocol_version(protocol_parameters, protocol_version);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_protocol_version, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  cardano_protocol_version_t*    protocol_version    = cardano_get_protocol_version();

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_protocol_version(protocol_parameters, protocol_version);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_protocol_version_t* protocol_version_result = cardano_protocol_parameters_get_protocol_version(protocol_parameters);

  // Assert
  EXPECT_EQ(protocol_version_result, protocol_version);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
  cardano_protocol_version_unref(&protocol_version);
  cardano_protocol_version_unref(&protocol_version_result);
}

TEST(cardano_protocol_parameters_get_cost_models, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  cardano_costmdls_t* costmdls = cardano_protocol_parameters_get_cost_models(protocol_parameters);

  // Assert
  EXPECT_EQ(costmdls, (cardano_costmdls_t*)0);
}

TEST(cardano_protocol_parameters_set_cost_models, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  cardano_costmdls_t*            costmdls            = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_cost_models(protocol_parameters, costmdls);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_cost_models, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  cardano_costmdls_t*            costmdls            = cardano_get_costmdls();

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_cost_models(protocol_parameters, costmdls);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_costmdls_t* costmdls_result = cardano_protocol_parameters_get_cost_models(protocol_parameters);

  // Assert
  EXPECT_EQ(costmdls_result, costmdls);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
  cardano_costmdls_unref(&costmdls);
  cardano_costmdls_unref(&costmdls_result);
}

TEST(cardano_protocol_parameters_get_min_pool_cost, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t min_pool_cost = cardano_protocol_parameters_get_min_pool_cost(protocol_parameters);

  // Assert
  EXPECT_EQ(min_pool_cost, 0);
}

TEST(cardano_protocol_parameters_set_min_pool_cost, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  uint64_t                       min_pool_cost       = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_min_pool_cost(protocol_parameters, min_pool_cost);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_min_pool_cost, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  uint64_t                       min_pool_cost       = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_min_pool_cost(protocol_parameters, min_pool_cost);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_min_pool_cost(protocol_parameters), min_pool_cost);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_max_value_size, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t max_value_size = cardano_protocol_parameters_get_max_value_size(protocol_parameters);

  // Assert
  EXPECT_EQ(max_value_size, 0);
}

TEST(cardano_protocol_parameters_set_max_value_size, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  uint64_t                       max_value_size      = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_max_value_size(protocol_parameters, max_value_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_max_value_size, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  uint64_t                       max_value_size      = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_max_value_size(protocol_parameters, max_value_size);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_max_value_size(protocol_parameters), max_value_size);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_ada_per_utxo_byte, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t ada_per_utxo_byte = cardano_protocol_parameters_get_ada_per_utxo_byte(protocol_parameters);

  // Assert
  EXPECT_EQ(ada_per_utxo_byte, 0);
}

TEST(cardano_protocol_parameters_set_ada_per_utxo_byte, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  uint64_t                       ada_per_utxo_byte   = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_ada_per_utxo_byte(protocol_parameters, ada_per_utxo_byte);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_ada_per_utxo_byte, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  uint64_t                       ada_per_utxo_byte   = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_ada_per_utxo_byte(protocol_parameters, ada_per_utxo_byte);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_ada_per_utxo_byte(protocol_parameters), ada_per_utxo_byte);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_execution_costs, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  cardano_ex_unit_prices_t* execution_costs = cardano_protocol_parameters_get_execution_costs(protocol_parameters);

  // Assert
  EXPECT_EQ(execution_costs, (cardano_ex_unit_prices_t*)0);
}

TEST(cardano_protocol_parameters_set_execution_costs, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  cardano_ex_unit_prices_t*      execution_costs     = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_execution_costs(protocol_parameters, execution_costs);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_execution_costs, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  cardano_ex_unit_prices_t*      execution_costs     = cardano_get_ex_unit_prices();

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_execution_costs(protocol_parameters, execution_costs);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_ex_unit_prices_t* execution_costs_result = cardano_protocol_parameters_get_execution_costs(protocol_parameters);

  EXPECT_EQ(execution_costs_result, execution_costs);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
  cardano_ex_unit_prices_unref(&execution_costs);
  cardano_ex_unit_prices_unref(&execution_costs_result);
}

TEST(cardano_protocol_parameters_get_max_tx_ex_units, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  cardano_ex_units_t* max_tx_ex_units = cardano_protocol_parameters_get_max_tx_ex_units(protocol_parameters);

  // Assert
  EXPECT_EQ(max_tx_ex_units, (cardano_ex_units_t*)0);
}

TEST(cardano_protocol_parameters_set_max_tx_ex_units, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  cardano_ex_units_t*            max_tx_ex_units     = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_max_tx_ex_units(protocol_parameters, max_tx_ex_units);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_max_tx_ex_units, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  cardano_ex_units_t*            max_tx_ex_units     = cardano_get_ex_unit();

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_max_tx_ex_units(protocol_parameters, max_tx_ex_units);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_ex_units_t* res = cardano_protocol_parameters_get_max_tx_ex_units(protocol_parameters);

  EXPECT_EQ(res, max_tx_ex_units);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
  cardano_ex_units_unref(&max_tx_ex_units);
  cardano_ex_units_unref(&res);
}

TEST(cardano_protocol_parameters_get_max_block_ex_units, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  cardano_ex_units_t* max_block_ex_units = cardano_protocol_parameters_get_max_block_ex_units(protocol_parameters);

  // Assert
  EXPECT_EQ(max_block_ex_units, (cardano_ex_units_t*)0);
}

TEST(cardano_protocol_parameters_set_max_block_ex_units, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  cardano_ex_units_t*            max_block_ex_units  = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_max_block_ex_units(protocol_parameters, max_block_ex_units);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_max_block_ex_units, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  cardano_ex_units_t*            max_block_ex_units  = cardano_get_ex_unit();

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_max_block_ex_units(protocol_parameters, max_block_ex_units);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_ex_units_t* res = cardano_protocol_parameters_get_max_block_ex_units(protocol_parameters);

  EXPECT_EQ(res, max_block_ex_units);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
  cardano_ex_units_unref(&max_block_ex_units);
  cardano_ex_units_unref(&res);
}

TEST(cardano_protocol_parameters_get_collateral_percentage, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t collateral_percentage = cardano_protocol_parameters_get_collateral_percentage(protocol_parameters);

  // Assert
  EXPECT_EQ(collateral_percentage, 0);
}

TEST(cardano_protocol_parameters_set_collateral_percentage, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters   = nullptr;
  uint64_t                       collateral_percentage = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_collateral_percentage(protocol_parameters, collateral_percentage);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_collateral_percentage, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters   = init_protocol_parameters();
  uint64_t                       collateral_percentage = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_collateral_percentage(protocol_parameters, collateral_percentage);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_collateral_percentage(protocol_parameters), collateral_percentage);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_max_collateral_inputs, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t max_collateral_inputs = cardano_protocol_parameters_get_max_collateral_inputs(protocol_parameters);

  // Assert
  EXPECT_EQ(max_collateral_inputs, 0);
}

TEST(cardano_protocol_parameters_set_max_collateral_inputs, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters   = nullptr;
  uint64_t                       max_collateral_inputs = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_max_collateral_inputs(protocol_parameters, max_collateral_inputs);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_max_collateral_inputs, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters   = init_protocol_parameters();
  uint64_t                       max_collateral_inputs = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_max_collateral_inputs(protocol_parameters, max_collateral_inputs);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_max_collateral_inputs(protocol_parameters), max_collateral_inputs);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_pool_voting_thresholds, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = cardano_protocol_parameters_get_pool_voting_thresholds(protocol_parameters);

  // Assert
  EXPECT_EQ(pool_voting_thresholds, (cardano_pool_voting_thresholds_t*)0);
}

TEST(cardano_protocol_parameters_set_pool_voting_thresholds, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t*    protocol_parameters    = nullptr;
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_pool_voting_thresholds(protocol_parameters, pool_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_pool_voting_thresholds, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t*    protocol_parameters    = init_protocol_parameters();
  cardano_pool_voting_thresholds_t* pool_voting_thresholds = cardano_get_pool_voting_thresholds();

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_pool_voting_thresholds(protocol_parameters, pool_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_pool_voting_thresholds_t* res = cardano_protocol_parameters_get_pool_voting_thresholds(protocol_parameters);

  EXPECT_EQ(res, pool_voting_thresholds);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
  cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
  cardano_pool_voting_thresholds_unref(&res);
}

TEST(cardano_protocol_parameters_get_drep_voting_thresholds, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = cardano_protocol_parameters_get_drep_voting_thresholds(protocol_parameters);

  // Assert
  EXPECT_EQ(drep_voting_thresholds, (cardano_drep_voting_thresholds_t*)0);
}

TEST(cardano_protocol_parameters_set_drep_voting_thresholds, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t*    protocol_parameters    = nullptr;
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_drep_voting_thresholds(protocol_parameters, drep_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_drep_voting_thresholds, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t*    protocol_parameters    = init_protocol_parameters();
  cardano_drep_voting_thresholds_t* drep_voting_thresholds = cardano_get_drep_voting_thresholds();

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_drep_voting_thresholds(protocol_parameters, drep_voting_thresholds);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_drep_voting_thresholds_t* res = cardano_protocol_parameters_get_drep_voting_thresholds(protocol_parameters);

  EXPECT_EQ(res, drep_voting_thresholds);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
  cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
  cardano_drep_voting_thresholds_unref(&res);
}

TEST(cardano_protocol_parameters_get_min_committee_size, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t min_committee_size = cardano_protocol_parameters_get_min_committee_size(protocol_parameters);

  // Assert
  EXPECT_EQ(min_committee_size, 0);
}

TEST(cardano_protocol_parameters_set_min_committee_size, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  uint64_t                       min_committee_size  = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_min_committee_size(protocol_parameters, min_committee_size);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_min_committee_size, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  uint64_t                       min_committee_size  = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_min_committee_size(protocol_parameters, min_committee_size);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_min_committee_size(protocol_parameters), min_committee_size);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_committee_term_limit, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t committee_term_limit = cardano_protocol_parameters_get_committee_term_limit(protocol_parameters);

  // Assert
  EXPECT_EQ(committee_term_limit, 0);
}

TEST(cardano_protocol_parameters_set_committee_term_limit, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters  = nullptr;
  uint64_t                       committee_term_limit = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_committee_term_limit(protocol_parameters, committee_term_limit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_committee_term_limit, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters  = init_protocol_parameters();
  uint64_t                       committee_term_limit = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_committee_term_limit(protocol_parameters, committee_term_limit);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_committee_term_limit(protocol_parameters), committee_term_limit);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_governance_action_validity_period, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t governance_action_validity_period = cardano_protocol_parameters_get_governance_action_validity_period(protocol_parameters);

  // Assert
  EXPECT_EQ(governance_action_validity_period, 0);
}

TEST(cardano_protocol_parameters_set_governance_action_validity_period, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters               = nullptr;
  uint64_t                       governance_action_validity_period = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_governance_action_validity_period(protocol_parameters, governance_action_validity_period);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_governance_action_validity_period, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters               = init_protocol_parameters();
  uint64_t                       governance_action_validity_period = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_governance_action_validity_period(protocol_parameters, governance_action_validity_period);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_governance_action_validity_period(protocol_parameters), governance_action_validity_period);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_governance_action_deposit, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t governance_action_deposit = cardano_protocol_parameters_get_governance_action_deposit(protocol_parameters);

  // Assert
  EXPECT_EQ(governance_action_deposit, 0);
}

TEST(cardano_protocol_parameters_set_governance_action_deposit, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters       = nullptr;
  uint64_t                       governance_action_deposit = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_governance_action_deposit(protocol_parameters, governance_action_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_governance_action_deposit, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters       = init_protocol_parameters();
  uint64_t                       governance_action_deposit = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_governance_action_deposit(protocol_parameters, governance_action_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_governance_action_deposit(protocol_parameters), governance_action_deposit);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_drep_deposit, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t drep_deposit = cardano_protocol_parameters_get_drep_deposit(protocol_parameters);

  // Assert
  EXPECT_EQ(drep_deposit, 0);
}

TEST(cardano_protocol_parameters_set_drep_deposit, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;
  uint64_t                       drep_deposit        = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_drep_deposit(protocol_parameters, drep_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_drep_deposit, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = init_protocol_parameters();
  uint64_t                       drep_deposit        = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_drep_deposit(protocol_parameters, drep_deposit);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_drep_deposit(protocol_parameters), drep_deposit);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_drep_inactivity_period, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  uint64_t drep_inactivity_period = cardano_protocol_parameters_get_drep_inactivity_period(protocol_parameters);

  // Assert
  EXPECT_EQ(drep_inactivity_period, 0);
}

TEST(cardano_protocol_parameters_set_drep_inactivity_period, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters    = nullptr;
  uint64_t                       drep_inactivity_period = 0;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_drep_inactivity_period(protocol_parameters, drep_inactivity_period);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_drep_inactivity_period, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters    = init_protocol_parameters();
  uint64_t                       drep_inactivity_period = 1000;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_drep_inactivity_period(protocol_parameters, drep_inactivity_period);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_get_drep_inactivity_period(protocol_parameters), drep_inactivity_period);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
}

TEST(cardano_protocol_parameters_get_ref_script_cost_per_byte, returnsZeroWhenObjectIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters = nullptr;

  // Act
  cardano_unit_interval_t* ref_script_cost_per_byte = cardano_protocol_parameters_get_ref_script_cost_per_byte(protocol_parameters);

  // Assert
  EXPECT_EQ(ref_script_cost_per_byte, (cardano_unit_interval_t*)0);
}

TEST(cardano_protocol_parameters_set_ref_script_cost_per_byte, returnsErrorIfPointerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters      = nullptr;
  cardano_unit_interval_t*       ref_script_cost_per_byte = nullptr;

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_ref_script_cost_per_byte(protocol_parameters, ref_script_cost_per_byte);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_protocol_parameters_set_ref_script_cost_per_byte, setsTheValue)
{
  // Arrange
  cardano_protocol_parameters_t* protocol_parameters      = init_protocol_parameters();
  cardano_unit_interval_t*       ref_script_cost_per_byte = cardano_get_zero_interval();

  // Act
  cardano_error_t error = cardano_protocol_parameters_set_ref_script_cost_per_byte(protocol_parameters, ref_script_cost_per_byte);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  cardano_unit_interval_t* res = cardano_protocol_parameters_get_ref_script_cost_per_byte(protocol_parameters);
  EXPECT_EQ(res, ref_script_cost_per_byte);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol_parameters);
  cardano_unit_interval_unref(&ref_script_cost_per_byte);
  cardano_unit_interval_unref(&res);
}

TEST(cardano_protocol_parameters_new, returnErrorIfGivenNull)
{
  // Act
  cardano_error_t error = cardano_protocol_parameters_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}
