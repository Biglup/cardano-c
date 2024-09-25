/**
 * \file plutus_language_version.h
 *
 * \author angel.castillo
 * \date   Jun 09, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_PLUTUS_LANGUAGE_VERSION_H
#define BIGLUP_LABS_INCLUDE_CARDANO_PLUTUS_LANGUAGE_VERSION_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The Cardano ledger tags scripts with a language that determines what the ledger will do with the script.
 *
 * In most cases this language will be very similar to the ones that came before, we refer to these as
 * 'Plutus language versions'. However, from the ledger’s perspective they are entirely unrelated and there
 * is generally no requirement that they be similar or compatible in any way.
 */
typedef enum
{
  /**
   * \brief V1 was the initial version of Plutus, introduced in the Alonzo hard fork.
   */
  CARDANO_PLUTUS_LANGUAGE_VERSION_V1 = 0,

  /**
   * \brief V2 was introduced in the Vasil hard fork.
   *
   * The main changes in V2 of Plutus were to the interface to scripts. The ScriptContext was extended
   * to include the following information:
   *
   *  - The full “redeemers” structure, which contains all the redeemers used in the transaction
   *  - Reference inputs in the transaction (proposed in CIP-31)
   *  - Inline datums in the transaction (proposed in CIP-32)
   *  - Reference scripts in the transaction (proposed in CIP-33)
   */
  CARDANO_PLUTUS_LANGUAGE_VERSION_V2 = 1,

  /**
   * \brief V3 was introduced in the Conway hard fork.
   *
   * The main changes in V3 of Plutus were to the interface to scripts. The ScriptContext was extended
   * to include the following information:
   *
   *  - A Map with all the votes that were included in the transaction.
   *  - A list with Proposals that will be turned into GovernanceActions, that everyone can vote on
   *  - Optional amount for the current treasury. If included it will be checked to be equal the current amount in the treasury.
   *  - Optional amount for donating to the current treasury. If included, specified amount will go into the treasury.
   */
  CARDANO_PLUTUS_LANGUAGE_VERSION_V3 = 2,

} cardano_plutus_language_version_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_PLUTUS_LANGUAGE_VERSION_H
