/**
 * \file time.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_TIMES_H
#define BIGLUP_LABS_INCLUDE_CARDANO_TIMES_H

/* INCLUDES ******************************************************************/

#include <cardano/common/network_magic.h>
#include <cardano/export.h>
#include <cardano/slot_config.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Computes the Unix start time (in milliseconds) of a given slot.
 *
 * This function converts a slot number into the Unix timestamp (milliseconds)
 * representing the exact beginning of that slot. The calculation is performed
 * using the linear slot progression model defined by the supplied
 * \ref cardano_slot_config_t structure, which specifies:
 *
 * - `zero_time`   — the Unix time (ms) corresponding to `zero_slot`
 * - `zero_slot`   — the reference slot number
 * - `slot_length` — duration of a single slot in milliseconds
 *
 * The formula used is:
 * \code
 * unix_time = config->zero_time
 *           + (slot - config->zero_slot) * config->slot_length;
 * \endcode
 *
 * \param[in] slot
 *   The absolute slot number whose starting Unix time should be computed.
 *
 * \param[in] config
 *   Pointer to a valid \ref cardano_slot_config_t providing slot timing rules.
 *   Must not be `NULL`.
 *
 * \return
 *   The Unix start time of the given slot, expressed in milliseconds since
 *   the Unix epoch. If `config` is `NULL`, the returned value is unspecified.
 *
 * \note
 *   This function assumes constant slot duration and does not model historical
 *   hard-fork boundaries or era transitions where slot lengths changed.
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t
slot_to_begin_unix_time(uint64_t slot, const cardano_slot_config_t* config);

/**
 * \brief Converts a Unix timestamp (in milliseconds) to the enclosing slot number.
 *
 * This function calculates the slot that *contains* the specified Unix time using
 * the linear relation defined by \ref cardano_slot_config_t. The operation
 * determines how many full slot intervals have elapsed since `zero_time`, and
 * returns the slot at or immediately before the provided timestamp.
 *
 * The formula used is:
 * \code
 * slot = config->zero_slot
 *      + (unix_time - config->zero_time) / config->slot_length;
 * \endcode
 *
 * \param[in] unix_time
 *   The Unix timestamp in milliseconds for which the corresponding slot should
 *   be determined.
 *
 * \param[in] config
 *   Pointer to a valid \ref cardano_slot_config_t supplying slot timing rules.
 *   Must not be `NULL`.
 *
 * \return
 *   The slot number that encloses the given timestamp. If `config` is `NULL`,
 *   the returned value is unspecified.
 *
 * \note
 *   This function uses truncation toward zero to determine the enclosing slot.
 *   It assumes a fixed slot length and does not account for historical chain
 *   eras with variable slot durations.
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t
unix_time_to_enclosing_slot(uint64_t unix_time, const cardano_slot_config_t* config);

/**
 * \brief Computes the Cardano network slot for a given Unix time.
 *
 * This function calculates the slot number on the Cardano network corresponding to a specified Unix timestamp.
 * Since slot duration may vary across different networks and over time, this computation requires both the
 * network magic and the Unix time.
 *
 * \param[in] magic     The network magic (\ref cardano_network_magic_t) identifying the specific Cardano network.
 * \param[in] unix_time The Unix timestamp in seconds for which to compute the slot.
 *
 * \returns The computed slot number as a uint64_t, representing the slot at the specified Unix time.
 *
 * Usage Example:
 * \code{.c}
 * cardano_network_magic_t magic = CARDANO_NETWORK_MAGIC_MAINNET; // Obtain network magic
 * uint64_t unix_time = 1700000000; // Example Unix timestamp
 * uint64_t slot = cardano_compute_slot_from_unix_time(magic, unix_time);
 *
 * printf("Computed slot: %llu\n", (unsigned long long)slot);
 * \endcode
 *
 * \note Slot duration and epoch boundaries may vary across networks and can change over time. This function
 *       takes these network-specific configurations into account.
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_compute_slot_from_unix_time(cardano_network_magic_t magic, uint64_t unix_time);

/**
 * \brief Computes the Unix time corresponding to a given Cardano network slot.
 *
 * This function calculates the Unix timestamp for a specified slot number on the Cardano network.
 * Slot-to-time mapping depends on the network's specific slot duration and other time-related parameters
 * that may vary across different Cardano networks and epochs.
 *
 * \param[in] magic The network magic (\ref cardano_network_magic_t) identifying the specific Cardano network.
 * \param[in] slot  The slot number for which to compute the Unix time.
 *
 * \returns The computed Unix timestamp in seconds as a uint64_t, representing the time at the specified slot.
 *
 * Usage Example:
 * \code{.c}
 * cardano_network_magic_t magic = ...; // Obtain network magic
 * uint64_t slot = 500000; // Example slot number
 * uint64_t unix_time = cardano_compute_unix_time_from_slot(magic, slot);
 *
 * printf("Computed Unix time: %llu\n", (unsigned long long)unix_time);
 * \endcode
 *
 * \note The conversion relies on the network's configuration, as different networks and epochs
 *       may have varying slot durations, affecting the calculated Unix time.
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_compute_unix_time_from_slot(cardano_network_magic_t magic, uint64_t slot);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_TIMES_H