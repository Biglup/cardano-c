/**
 * \file voter_type.cpp
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

#include <cardano/voting_procedures/voter_type.h>
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_voter_type_to_string, canConvertConstitutionalCommitteeKeyHash)
{
  const char* message = cardano_voter_type_to_string(CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_KEY_HASH);

  ASSERT_STREQ("Voter Type: Constitutional Committee Key Hash", message);
}

TEST(cardano_voter_type_to_string, canConvertConstitutionalCommitteeScriptHash)
{
  const char* message = cardano_voter_type_to_string(CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_SCRIPT_HASH);

  ASSERT_STREQ("Voter Type: Constitutional Committee Script Hash", message);
}

TEST(cardano_voter_type_to_string, canConvertDRepKeyHash)
{
  const char* message = cardano_voter_type_to_string(CARDANO_VOTER_TYPE_DREP_KEY_HASH);

  ASSERT_STREQ("Voter Type: DRep Key Hash", message);
}

TEST(cardano_voter_type_to_string, canConvertDRepScriptHash)
{
  const char* message = cardano_voter_type_to_string(CARDANO_VOTER_TYPE_DREP_SCRIPT_HASH);

  ASSERT_STREQ("Voter Type: DRep Script Hash", message);
}

TEST(cardano_voter_type_to_string, canConvertStakePoolKeyHash)
{
  const char* message = cardano_voter_type_to_string(CARDANO_VOTER_TYPE_STAKE_POOL_KEY_HASH);

  ASSERT_STREQ("Voter Type: Stake Pool Key Hash", message);
}

TEST(cardano_voter_type_to_string, canConvertUnknown)
{
  const char* message = cardano_voter_type_to_string((cardano_voter_type_t)100);

  ASSERT_STREQ("Voter Type: Unknown", message);
}
