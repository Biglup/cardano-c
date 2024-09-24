/**
 * \file vote.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_VOTE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_VOTE_H

/* INCLUDES ******************************************************************/

#include <cardano/export.h>

/* DECLARATIONS *************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Enumeration representing possible voting choices in a Cardano voting procedure.
 */
typedef enum
{
  /**
   * \brief Represents a "No" vote.
   *
   * This value indicates that the voter is against the proposal being voted on.
   */
  CARDANO_VOTE_NO = 0,

  /**
   * \brief Represents a "Yes" vote.
   *
   * This value indicates that the voter supports the proposal being voted on.
   */
  CARDANO_VOTE_YES = 1,

  /**
   * \brief Represents an "Abstain" vote.
   *
   * This value is chosen when the voter decides to abstain, neither supporting nor opposing the proposal.
   */
  CARDANO_VOTE_ABSTAIN = 2,
} cardano_vote_t;

/**
 * \brief Converts voter types to their human readable form.
 *
 * \param[in] type The voter type to get the string representation for.
 * \return Human readable form of the given voter type.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_vote_to_string(cardano_vote_t type);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_VOTE_H