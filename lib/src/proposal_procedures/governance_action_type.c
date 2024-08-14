/**
 * \file governance_action_type.c
 *
 * \author angel.castillo
 * \date   Aug 15, 2024
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

#include <cardano/proposal_procedures/governance_action_type.h>

/* DEFINITIONS ***************************************************************/

const char*
cardano_governance_action_type_to_string(const cardano_governance_action_type_t governance_action_type)
{
  const char* message;

  switch (governance_action_type)
  {
    case CARDANO_GOVERNANCE_ACTION_TYPE_PARAMETER_CHANGE:
      message = "Governance Action Type: Parameter Change";
      break;
    case CARDANO_GOVERNANCE_ACTION_TYPE_HARD_FORK_INITIATION:
      message = "Governance Action Type: Hard Fork Initiation";
      break;
    case CARDANO_GOVERNANCE_ACTION_TYPE_TREASURY_WITHDRAWALS:
      message = "Governance Action Type: Treasury Withdrawals";
      break;
    case CARDANO_GOVERNANCE_ACTION_TYPE_NO_CONFIDENCE:
      message = "Governance Action Type: No Confidence";
      break;
    case CARDANO_GOVERNANCE_ACTION_TYPE_UPDATE_COMMITTEE:
      message = "Governance Action Type: Update Committee";
      break;
    case CARDANO_GOVERNANCE_ACTION_TYPE_NEW_CONSTITUTION:
      message = "Governance Action Type: New Constitution";
      break;
    case CARDANO_GOVERNANCE_ACTION_TYPE_INFO:
      message = "Governance Action Type: Info";
      break;
    default:
      message = "Governance Action Type: Unknown";
      break;
  }

  return message;
}
