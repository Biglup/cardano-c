/**
 * \file vote.cpp
 *
 * \author angel.castillo
 * \date   Jun 03, 2024
 *
 * \section LICENSE
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
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_vote_to_string, canConvertNo)
{
  const char* message = cardano_vote_to_string(CARDANO_VOTE_NO);

  ASSERT_STREQ("Vote: No", message);
}

TEST(cardano_vote_to_string, canConvertYes)
{
  const char* message = cardano_vote_to_string(CARDANO_VOTE_YES);

  ASSERT_STREQ("Vote: Yes", message);
}

TEST(cardano_vote_to_string, canConvertAbstain)
{
  const char* message = cardano_vote_to_string(CARDANO_VOTE_ABSTAIN);

  ASSERT_STREQ("Vote: Abstain", message);
}

TEST(cardano_vote_to_string, canConvertUnknown)
{
  const char* message = cardano_vote_to_string((cardano_vote_t)100);

  ASSERT_STREQ("Vote: Unknown", message);
}
