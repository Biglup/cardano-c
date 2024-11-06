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

TEST(cardano_compute_epoch_from_unix_time, canConvertUnixTimeToEpoch)
{
  // Arrange
  uint64_t unix_time = 1730901968;

  // Act
  EXPECT_EQ(cardano_compute_epoch_from_unix_time(CARDANO_NETWORK_MAGIC_MAINNET, unix_time), 520);
  EXPECT_EQ(cardano_compute_epoch_from_unix_time(CARDANO_NETWORK_MAGIC_PREVIEW, unix_time), 743);
  EXPECT_EQ(cardano_compute_epoch_from_unix_time(CARDANO_NETWORK_MAGIC_PREPROD, unix_time), 177);
}