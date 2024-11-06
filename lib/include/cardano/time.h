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
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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

/**
 * \brief Computes the epoch number corresponding to a given Unix timestamp.
 *
 * This function determines the epoch number for a specified Unix time (in seconds) based on the network configuration,
 * which is determined by the `magic` parameter. Different networks may have distinct epoch configurations, so this
 * calculation takes the network's settings into account.
 *
 * \param[in] magic The \ref cardano_network_magic_t that specifies the network identifier, which may influence epoch length
 *                  and other configuration details.
 * \param[in] unix_time The Unix timestamp in seconds for which to compute the epoch number.
 *
 * \return The epoch number corresponding to the specified Unix time.
 *
 * \note This function leverages network-specific configurations, such as epoch length and initial slot settings, to
 *       compute the epoch accurately. It is useful for converting timestamps to Cardano's epoch-based timekeeping system.
 *
 * Usage Example:
 * \code{.c}
 * uint64_t unix_time = 1609459200000; // Unix time in seconds (e.g., Jan 1, 2021)
 * cardano_network_magic_t network_magic = CARDANO_NETWORK_MAGIC_MAINNET; // Example network magic identifier
 *
 * uint64_t epoch = cardano_compute_epoch_from_unix_time(network_magic, unix_time);
 * printf("Unix time %llu seconds corresponds to epoch %llu\n", unix_time, epoch);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_compute_epoch_from_unix_time(cardano_network_magic_t magic, uint64_t unix_time);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_TIMES_H