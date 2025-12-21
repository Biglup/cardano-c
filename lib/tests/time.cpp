/**
 * \file time.cpp
 *
 * \author angel.castillo
 * \date   Nov 06, 2024
 *
 * \section LICENSE
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

#include <cardano/time.h>
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_compute_slot_from_unix_time, canConvertUnixTimeToSlot)
{
  // Arrange
  uint64_t unix_time = 1730901968;

  // Act
  EXPECT_EQ(cardano_compute_slot_from_unix_time(CARDANO_NETWORK_MAGIC_MAINNET, unix_time), 139335677);
  EXPECT_EQ(cardano_compute_slot_from_unix_time(CARDANO_NETWORK_MAGIC_PREVIEW, unix_time), 64245968);
  EXPECT_EQ(cardano_compute_slot_from_unix_time(CARDANO_NETWORK_MAGIC_PREPROD, unix_time), 75218768);
}

TEST(cardano_compute_unix_time_from_slot, canConvertSlotToUnixTime)
{
  // Act
  EXPECT_EQ(cardano_compute_unix_time_from_slot(CARDANO_NETWORK_MAGIC_MAINNET, 139335677), 1730901968);
  EXPECT_EQ(cardano_compute_unix_time_from_slot(CARDANO_NETWORK_MAGIC_PREVIEW, 64245968), 1730901968);
  EXPECT_EQ(cardano_compute_unix_time_from_slot(CARDANO_NETWORK_MAGIC_PREPROD, 75218768), 1730901968);
}

TEST(unix_time_to_enclosing_slot, returnsMaxWhenSlotLengthIsZero)
{
  cardano_slot_config_t config {};
  config.zero_time   = 0ULL;
  config.zero_slot   = 0ULL;
  config.slot_length = 0ULL;

  EXPECT_EQ(unix_time_to_enclosing_slot(1730901968000ULL, &config), UINT64_MAX);
}

TEST(slot_to_begin_unix_time, computesExpectedMainnetBoundaryTime)
{
  // This mirrors MAINNET_SLOT_CONFIG from time.c
  cardano_slot_config_t config {};
  config.zero_time   = 1596059091000ULL;
  config.zero_slot   = 4492800ULL;
  config.slot_length = 1000ULL;

  const uint64_t slot    = 139335677ULL;
  const uint64_t unix_ms = slot_to_begin_unix_time(slot, &config);

  // cardano_compute_unix_time_from_slot(...) == 1730901968,
  // so in milliseconds this must be * 1000.
  EXPECT_EQ(unix_ms, 1730901968000ULL);
}

TEST(unix_time_to_enclosing_slot, computesExpectedMainnetBoundarySlot)
{
  // Mirrors MAINNET_SLOT_CONFIG from time.c
  cardano_slot_config_t config {};
  config.zero_time   = 1596059091000ULL;
  config.zero_slot   = 4492800ULL;
  config.slot_length = 1000ULL;

  const uint64_t unix_ms = 1730901968000ULL;
  const uint64_t slot    = unix_time_to_enclosing_slot(unix_ms, &config);

  EXPECT_EQ(slot, 139335677ULL);
}

TEST(unix_time_to_enclosing_slot, roundsDownToEnclosingSlot)
{
  // Mirrors MAINNET_SLOT_CONFIG from time.c
  cardano_slot_config_t config {};
  config.zero_time   = 1596059091000ULL;
  config.zero_slot   = 4492800ULL;
  config.slot_length = 1000ULL;

  // 1 ms before the boundary of slot 139335677 should still be in slot 139335676.
  const uint64_t unix_ms = 1730901968000ULL - 1ULL;
  const uint64_t slot    = unix_time_to_enclosing_slot(unix_ms, &config);

  EXPECT_EQ(slot, 139335676ULL);
}

TEST(slot_and_time_conversion, roundTripWithCustomConfig)
{
  cardano_slot_config_t config {};
  config.zero_time   = 1'700'000'000'000ULL; // arbitrary baseline (ms)
  config.zero_slot   = 1'000'000ULL;         // arbitrary baseline slot
  config.slot_length = 2000ULL;              // 2 seconds per slot

  const uint64_t original_slot = 1'234'567ULL;

  const uint64_t unix_ms   = slot_to_begin_unix_time(original_slot, &config);
  const uint64_t roundtrip = unix_time_to_enclosing_slot(unix_ms, &config);

  EXPECT_EQ(roundtrip, original_slot);
}