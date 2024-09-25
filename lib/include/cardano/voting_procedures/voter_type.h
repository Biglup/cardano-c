/**
 * \file voter_type.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_VOTER_TYPE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_VOTER_TYPE_H

/* INCLUDES ******************************************************************/

#include <cardano/export.h>

/* DECLARATIONS *************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Enumeration to represent different kinds of voters within the Cardano governance system.
 */
typedef enum
{
  /**
   * \brief Represents a constitutional committee member identified by a key hash.
   **/
  CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_KEY_HASH = 0,

  /**
   * \brief Represents a constitutional committee member identified by a script hash.
   **/
  CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_SCRIPT_HASH = 1,

  /**
   * \brief Represents a DRep (Delegation Representative) identified by a key hash.
   **/
  CARDANO_VOTER_TYPE_DREP_KEY_HASH = 2,

  /**
   * \brief Represents a DRep (Delegation Representative) identified by a script hash.
   **/
  CARDANO_VOTER_TYPE_DREP_SCRIPT_HASH = 3,

  /**
   * \brief Represents a Stake Pool Operator (SPO) identified by a key hash.
   **/
  CARDANO_VOTER_TYPE_STAKE_POOL_KEY_HASH = 4
} cardano_voter_type_t;

/**
 * \brief Converts voter types to their human readable form.
 *
 * \param[in] type The voter type to get the string representation for.
 * \return Human readable form of the given voter type.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_voter_type_to_string(cardano_voter_type_t type);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_VOTER_TYPE_H