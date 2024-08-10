/**
 * \file voter_type.c
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

/* INCLUDES ******************************************************************/

#include <cardano/voting_procedures/voter_type.h>

/* DEFINITIONS ***************************************************************/

const char*
cardano_voter_type_to_string(const cardano_voter_type_t voter_type)
{
  const char* message;

  switch (voter_type)
  {
    case CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_KEY_HASH:
      message = "Voter Type: Constitutional Committee Key Hash";
      break;
    case CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_SCRIPT_HASH:
      message = "Voter Type: Constitutional Committee Script Hash";
      break;
    case CARDANO_VOTER_TYPE_DREP_KEY_HASH:
      message = "Voter Type: DRep Key Hash";
      break;
    case CARDANO_VOTER_TYPE_DREP_SCRIPT_HASH:
      message = "Voter Type: DRep Script Hash";
      break;
    case CARDANO_VOTER_TYPE_STAKE_POOL_KEY_HASH:
      message = "Voter Type: Stake Pool Key Hash";
      break;
    default:
      message = "Voter Type: Unknown";
      break;
  }

  return message;
}
