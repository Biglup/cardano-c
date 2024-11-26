/**
 * \file cip_1852_constants.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CIP_1852_CONSTANTS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CIP_1852_CONSTANTS_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* COIN TYPE *****************************************************************/

/**
 * \brief The coin_type value for Cardano is 1815 (Ada Lovelace year of birth).
 */
#define CARDANO_CIP_1852_COIN_TYPE (1815U)

/* PURPOSE *******************************************************************/

/**
 * \brief To associate standard wallet key derivation purpose.
 */
#define CARDANO_CIP_1852_PURPOSE_STANDARD (1852U)

/**
 * \brief To associate multi-signature keys to a wallet, CIP-1854 reserves
 * purpose=1854' to distinguish multisig wallets from standard wallets.
 */
#define CARDANO_CIP_1852_PURPOSE_MULTI_SIG (1854U)

/* ROLES *********************************************************************/

/**
 * \brief External chain role.
 */
#define CARDANO_CIP_1852_ROLE_EXTERNAL (0U)

/**
 * \brief Internal chain role.
 */
#define CARDANO_CIP_1852_ROLE_INTERNAL (1U)

/**
 * \brief Staking Key role.
 */
#define CARDANO_CIP_1852_ROLE_STAKING (2U)

/**
 * \brief DRep Key (CIP-0105) role.
 */
#define CARDANO_CIP_1852_ROLE_DREP (3U)

/**
 * \brief Constitutional Committee Cold Key (CIP-0105) role.
 */
#define CARDANO_CIP_1852_ROLE_CC_COLD (4U)

/**
 * \brief Constitutional Committee Hot Key (CIP-0105) role.
 */
#define CARDANO_CIP_1852_ROLE_CC_HOT (5U)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CIP_1852_CONSTANTS_H
