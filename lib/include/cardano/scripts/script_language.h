/**
 * \file script_language.h
 *
 * \author angel.castillo
 * \date   Mar 20, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_LANGUAGE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_LANGUAGE_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Script language.
 */
typedef enum
{
  /**
   * \brief The Native scripts form an expression tree, the evaluation of the script produces either true or false.
   *
   * Note that it is recursive. There are no constraints on the nesting or size, except that imposed by the overall
   * transaction size limit (given that the script must be included in the transaction in a script witnesses).
   */
  CARDANO_SCRIPT_LANGUAGE_NATIVE,

  /**
   * \brief V1 was the initial version of Plutus, introduced in the Alonzo hard fork.
   */
  CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1,

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
  CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2,

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
  CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3,

} cardano_script_language_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SCRIPT_LANGUAGE_H
