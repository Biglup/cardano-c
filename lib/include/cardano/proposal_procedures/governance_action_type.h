/**
 * \file governance_action_type.h
 *
 * \author angel.castillo
 * \date   Aug 08, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_GOVERNANCE_ACTION_TYPE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_GOVERNANCE_ACTION_TYPE_H

/* INCLUDES ******************************************************************/

#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents the different types of governance actions within the Cardano blockchain ecosystem.
 */
typedef enum
{
  /**
   * \brief Updates one or more updatable protocol parameters, excluding changes to major protocol versions (i.e., "hard forks").
   **/
  CARDANO_GOVERNANCE_ACTION_TYPE_PARAMETER_CHANGE = 0,

  /**
   * \brief Initiates a non-backwards compatible upgrade of the network. This action necessitates a preceding software update.
   **/
  CARDANO_GOVERNANCE_ACTION_TYPE_HARD_FORK_INITIATION = 1,

  /**
   * \brief Withdraws funds from the treasury.
   **/
  CARDANO_GOVERNANCE_ACTION_TYPE_TREASURY_WITHDRAWALS = 2,

  /**
   * \brief Propose a state of no-confidence in the current constitutional committee.
   * Allows Ada holders to challenge the authority granted to the existing committee.
   */
  CARDANO_GOVERNANCE_ACTION_TYPE_NO_CONFIDENCE = 3,

  /**
   * \brief Modifies the composition of the constitutional committee, its signature threshold, or its terms of operation.
   **/
  CARDANO_GOVERNANCE_ACTION_TYPE_UPDATE_COMMITTEE = 4,

  /**
   * \brief Changes or amendments the Constitution.
   * */
  CARDANO_GOVERNANCE_ACTION_TYPE_NEW_CONSTITUTION = 5,

  /**
   * \brief Represents an action that has no direct effect on the blockchain,
   * but serves as an on-chain record or informative notice.
   */
  CARDANO_GOVERNANCE_ACTION_TYPE_INFO = 6
} cardano_governance_action_type_t;

/**
 * \brief Converts governance action types to their human readable form.
 *
 * \param[in] type The governance action type to get the string representation for.
 * \return Human readable form of the given governance action type.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_governance_action_type_to_string(cardano_governance_action_type_t type);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_GOVERNANCE_ACTION_TYPE_H