/**
 * \file vote.c
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

#include <cardano/voting_procedures/vote.h>

/* DEFINITIONS ***************************************************************/

const char*
cardano_vote_to_string(const cardano_vote_t vote)
{
  const char* message;

  switch (vote)
  {
    case CARDANO_VOTE_NO:
      message = "Vote: No";
      break;
    case CARDANO_VOTE_YES:
      message = "Vote: Yes";
      break;
    case CARDANO_VOTE_ABSTAIN:
      message = "Vote: Abstain";
      break;
    default:
      message = "Vote: Unknown";
      break;
  }

  return message;
}
