/**
 * \file account_derivation_path.h
 *
 * \author angel.castillo
 * \date   Oct 06, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_ACCOUNT_DERIVATION_PATH_H
#define BIGLUP_LABS_INCLUDE_CARDANO_ACCOUNT_DERIVATION_PATH_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief BIP44 account derivation path.
 */
typedef struct cardano_account_derivation_path_t
{
    /**
     * \brief Purpose is a constant set to either 1852' (standard wallets) or 1854' (multisig wallets).
     *
     * Hardened derivation is used at this level.
     */
    uint64_t purpose;

    /**
     * \brief One master node (seed) can be used for unlimited number of independent cryptocoins such as Bitcoin, Litecoin or Namecoin.
     * However, sharing the same space for various cryptocoins has some disadvantages.
     *
     * This level creates a separate subtree for every cryptocoin, avoiding reusing addresses across cryptocoins and improving privacy issues.
     *
     * Coin type is a constant, set for each cryptocoin. Cryptocoin developers may ask for registering unused number for their project.
     *
     * \remark The coin value for Cardano is 1815 (Ada Lovelace year of birth).
     *
     * Hardened derivation is used at this level.
     */
    uint64_t coin_type;

    /**
     * \brief Accounts are numbered from index 0 in sequentially increasing manner.
     *
     * This number is used as a child index in BIP32 derivation.
     *
     * Hardened derivation is used at this level.
     */
    uint64_t account;
} cardano_account_derivation_path_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_ACCOUNT_DERIVATION_PATH_H
