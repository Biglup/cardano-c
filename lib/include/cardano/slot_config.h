/**
 * \file slot_config.h
 *
 * \author angel.castillo
 * \date   Dec 21, 2025
 *
 * Copyright 2025 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SLOT_CONFIG_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SLOT_CONFIG_H

/* INCLUDES ******************************************************************/

#include <cardano/common/network_magic.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Configuration structure for slot timing on the Cardano network.
 *
 * The `cardano_slot_config_t` structure encapsulates configuration parameters for
 * converting between Unix time and slots in the Cardano network. This structure
 * provides essential timing information to correctly interpret and compute slot numbers
 * based on network-specific parameters.
 */
typedef struct cardano_slot_config_t
{
    /**
     * \brief The start zero_time in milliseconds since the Unix epoch. This zero_time aligns with `zero_slot` and
     * acts as a baseline for slot and epoch calculations.
     */
    uint64_t zero_time;

    /**
     * \brief The initial slot number corresponding to `zero_time`. It is the reference slot number that
     * maps directly to `zero_time`, allowing accurate calculation of other slot numbers relative to
     * this baseline.
     */
    uint64_t zero_slot;

    /**
     * \brief The duration of each slot in milliseconds. This value is crucial for slot_config-to-slot
     * conversions, providing the length of slot_config between consecutive slots.
     */
    uint64_t slot_length;
} cardano_slot_config_t;

/**
 * \brief Predefined slot configuration for the Cardano Mainnet.
 */
extern const cardano_slot_config_t CARDANO_MAINNET_SLOT_CONFIG;

/**
 * \brief Predefined slot configuration for the Cardano Preview network.
 */
extern const cardano_slot_config_t CARDANO_PREVIEW_SLOT_CONFIG;

/**
 * \brief Predefined slot configuration for the Cardano Preprod network.
 */
extern const cardano_slot_config_t CARDANO_PREPROD_SLOT_CONFIG;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SLOT_CONFIG_H