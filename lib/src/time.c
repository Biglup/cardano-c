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

#include <cardano/slot_config.h>
#include <cardano/time.h>

/* CONSTANTS *****************************************************************/

const cardano_slot_config_t        CARDANO_MAINNET_SLOT_CONFIG = { 1596059091000, 4492800, 1000 };
const cardano_slot_config_t        CARDANO_PREVIEW_SLOT_CONFIG = { 1666656000000, 0, 1000 };
const cardano_slot_config_t        CARDANO_PREPROD_SLOT_CONFIG = { 1654041600000 + 1728000000, 86400, 1000 };
static const cardano_slot_config_t CARDANO_TESTNET_SLOT_CONFIG = { 0, 0, 0 };

/* DEFINITIONS ***************************************************************/

uint64_t
slot_to_begin_unix_time(const uint64_t slot, const cardano_slot_config_t* config)
{
  if (config == NULL)
  {
    return 0U;
  }

  const uint64_t ms_after_begin = (slot - config->zero_slot) * config->slot_length;

  return config->zero_time + ms_after_begin;
}

uint64_t
unix_time_to_enclosing_slot(const uint64_t unix_time, const cardano_slot_config_t* config)
{
  if (config == NULL)
  {
    return 0U;
  }

  if (config->slot_length == 0U)
  {
    return UINT64_MAX;
  }

  const uint64_t time_passed  = unix_time - config->zero_time;
  const uint64_t slots_passed = time_passed / config->slot_length;

  return slots_passed + config->zero_slot;
}

uint64_t
cardano_compute_slot_from_unix_time(const cardano_network_magic_t magic, const uint64_t unix_time)
{
  cardano_slot_config_t config = { 0U };

  switch (magic)
  {
    case CARDANO_NETWORK_MAGIC_MAINNET:
      config = CARDANO_MAINNET_SLOT_CONFIG;
      break;
    case CARDANO_NETWORK_MAGIC_PREVIEW:
      config = CARDANO_PREVIEW_SLOT_CONFIG;
      break;
    case CARDANO_NETWORK_MAGIC_PREPROD:
      config = CARDANO_PREPROD_SLOT_CONFIG;
      break;
    default:
    {
      config = CARDANO_TESTNET_SLOT_CONFIG;
    }
  }

  return unix_time_to_enclosing_slot(unix_time * 1000U, &config);
}

uint64_t
cardano_compute_unix_time_from_slot(const cardano_network_magic_t magic, const uint64_t slot)
{
  cardano_slot_config_t config = { 0U };

  switch (magic)
  {
    case CARDANO_NETWORK_MAGIC_MAINNET:
      config = CARDANO_MAINNET_SLOT_CONFIG;
      break;
    case CARDANO_NETWORK_MAGIC_PREVIEW:
      config = CARDANO_PREVIEW_SLOT_CONFIG;
      break;
    case CARDANO_NETWORK_MAGIC_PREPROD:
      config = CARDANO_PREPROD_SLOT_CONFIG;
      break;
    default:
    {
      config = CARDANO_TESTNET_SLOT_CONFIG;
    }
  }

  return slot_to_begin_unix_time(slot, &config) / 1000U;
}
