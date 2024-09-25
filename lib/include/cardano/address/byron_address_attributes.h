/**
 * \file byron_address_attributes.h
 *
 * \author angel.castillo
 * \date   Apr 12, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BYRON_ADDRESS_ATTRIBUTES_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BYRON_ADDRESS_ATTRIBUTES_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Attributes specific to Byron addresses in the Cardano blockchain.
 *
 * This structure holds optional attributes associated with Byron addresses. Byron addresses were used during
 * the Byron era of the Cardano blockchain. The `derivation_path` attribute was utilized by legacy random wallets
 * for key derivation, a practice that has since been replaced by more modern wallet structures such as Yoroi and
 * Icarus. The `magic` attribute serves as a network tag, used primarily on test networks to distinguish between
 * different network environments.
 */
typedef struct cardano_byron_address_attributes_t
{
    /**
     * \brief  Holds the derivation path used by legacy wallets.
     */
    byte_t derivation_path[64];

    /**
     * \brief  The size of the derivation path encrypted data.
     */
    size_t derivation_path_size;

    /**
     * \brief * Network magic identifier used for network discrimination, primarily relevant in test network scenarios. This
     * int32_t value helps ensure that transactions are broadcast on the correct Cardano network. It will be set to -1
     * if the address is not associated with a specific network.
     */
    int64_t magic;
} cardano_byron_address_attributes_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BYRON_ADDRESS_ATTRIBUTES_H