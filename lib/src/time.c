/**
 * \file time.c
 *
 * \author angel.castillo
 * \date   Nov 06, 2024
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

/* STRUCTURES ****************************************************************/

/**
 * \brief Configuration structure for slot and epoch timing on the Cardano network.
 *
 * The `cardano_slot_config_t` structure encapsulates configuration parameters for
 * converting between Unix time, slots, and epochs in the Cardano network. This structure
 * provides essential timing information to correctly interpret and compute slot numbers and
 * epochs based on network-specific parameters.
 *
 * \var zero_time
 *   The start time in milliseconds since the Unix epoch. This time aligns with `zero_slot` and
 *   acts as a baseline for slot and epoch calculations.
 *
 * \var zero_slot
 *   The initial slot number corresponding to `zero_time`. It is the reference slot number that
 *   maps directly to `zero_time`, allowing accurate calculation of other slot numbers relative to
 *   this baseline.
 *
 * \var slot_length
 *   The duration of each slot in milliseconds. This value is crucial for time-to-slot
 *   conversions, providing the length of time between consecutive slots.
 *
 * \var start_epoch
 *   The epoch number at `zero_time`. This is the baseline epoch number and typically marks the
 *   starting point or genesis epoch of the network. It helps in calculating the current epoch
 *   based on elapsed time and slot progress.
 *
 * \var epoch_length
 *   The number of slots in a single epoch. This parameter is essential for slot-to-epoch
 *   conversions and determines the duration of each epoch in terms of slot count.
 */
typedef struct cardano_slot_config_t
{
    uint64_t zero_time;
    uint64_t zero_slot;
    uint64_t slot_length;
    uint64_t start_epoch;
    uint64_t epoch_length;
} cardano_slot_config_t;

/* CONSTANTS *****************************************************************/

static const cardano_slot_config_t MAINNET_SLOT_CONFIG = { 1596059091000, 4492800, 1000, 208, 432000 };
static const cardano_slot_config_t PREVIEW_SLOT_CONFIG = { 1666656000000, 0, 1000, 0, 86400 };
static const cardano_slot_config_t PREPROD_SLOT_CONFIG = { 1654041600000 + 1728000000, 86400, 1000, 4, 432000 };
static const cardano_slot_config_t TESTNET_SLOT_CONFIG = { 0, 0, 0, 0, 0 };

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Converts a slot number to its corresponding Unix start time in milliseconds.
 *
 * This function computes the Unix time (in milliseconds) at the beginning of a given slot number,
 * using the provided slot configuration parameters.
 *
 * \param[in] slot The slot number to convert.
 * \param[in] config The \ref cardano_slot_config_t structure containing slot timing configuration
 *                   values, such as the zero time, zero slot, and slot length.
 *
 * \return The Unix time in milliseconds representing the start of the specified slot.
 *
 * \note This calculation assumes a linear slot progression without accounting for any epoch-based
 *       adjustments to slot duration. The result is derived by calculating the elapsed time since
 *       the baseline `zero_slot` in `config`, with each slot taking the fixed length of time
 *       specified by `config.slot_length`.
 */
static uint64_t
slot_to_begin_unix_time(const uint64_t slot, const cardano_slot_config_t config)
{
  const uint64_t ms_after_begin = (slot - config.zero_slot) * config.slot_length;

  return config.zero_time + ms_after_begin;
}

/**
 * \brief Converts a Unix time (in milliseconds) to the corresponding enclosing slot number.
 *
 * This function calculates the slot number that would encompass a given Unix time, taking into account
 * the start time, slot length, and other slot configuration parameters provided.
 *
 * \param[in] unix_time The Unix time in milliseconds to convert to a slot number.
 * \param[in] config The \ref cardano_slot_config_t structure containing slot configuration
 *                   parameters, such as the `zero_time`, `zero_slot`, and `slot_length`.
 *
 * \return The slot number corresponding to the provided Unix time.
 *
 * \note This calculation assumes a consistent slot length across all epochs. It finds the elapsed time
 *       from `zero_time` to `unix_time`, divides by `slot_length` to determine the number of slots passed,
 *       and adds the result to `zero_slot`.
 */
static uint64_t
unix_time_to_enclosing_slot(const uint64_t unix_time, const cardano_slot_config_t config)
{
  if (config.slot_length == 0U)
  {
    return UINT64_MAX;
  }

  const uint64_t time_passed  = unix_time - config.zero_time;
  const uint64_t slots_passed = time_passed / config.slot_length;

  return slots_passed + config.zero_slot;
}

/* DEFINITIONS ***************************************************************/

uint64_t
cardano_compute_slot_from_unix_time(const cardano_network_magic_t magic, const uint64_t unix_time)
{
  cardano_slot_config_t config = { 0U };

  switch (magic)
  {
    case CARDANO_NETWORK_MAGIC_MAINNET:
      config = MAINNET_SLOT_CONFIG;
      break;
    case CARDANO_NETWORK_MAGIC_PREVIEW:
      config = PREVIEW_SLOT_CONFIG;
      break;
    case CARDANO_NETWORK_MAGIC_PREPROD:
      config = PREPROD_SLOT_CONFIG;
      break;
    default:
    {
      config = TESTNET_SLOT_CONFIG;
    }
  }

  return unix_time_to_enclosing_slot(unix_time * 1000U, config);
}

uint64_t
cardano_compute_unix_time_from_slot(const cardano_network_magic_t magic, const uint64_t slot)
{
  cardano_slot_config_t config = { 0U };

  switch (magic)
  {
    case CARDANO_NETWORK_MAGIC_MAINNET:
      config = MAINNET_SLOT_CONFIG;
      break;
    case CARDANO_NETWORK_MAGIC_PREVIEW:
      config = PREVIEW_SLOT_CONFIG;
      break;
    case CARDANO_NETWORK_MAGIC_PREPROD:
      config = PREPROD_SLOT_CONFIG;
      break;
    default:
    {
      config = TESTNET_SLOT_CONFIG;
    }
  }

  return slot_to_begin_unix_time(slot, config) / 1000U;
}

uint64_t
cardano_compute_epoch_from_unix_time(const cardano_network_magic_t magic, uint64_t unix_time)
{
  cardano_slot_config_t config = { 0U };

  switch (magic)
  {
    case CARDANO_NETWORK_MAGIC_MAINNET:
      config = MAINNET_SLOT_CONFIG;
      break;
    case CARDANO_NETWORK_MAGIC_PREVIEW:
      config = PREVIEW_SLOT_CONFIG;
      break;
    case CARDANO_NETWORK_MAGIC_PREPROD:
      config = PREPROD_SLOT_CONFIG;
      break;
    default:
    {
      config = TESTNET_SLOT_CONFIG;
    }
  }

  if (config.epoch_length == 0U)
  {
    return UINT64_MAX;
  }

  const uint64_t time_passed = (unix_time * 1000U) - config.zero_time;
  const uint64_t epoch       = (time_passed / 1000U / config.epoch_length) + config.start_epoch;

  return epoch;
}