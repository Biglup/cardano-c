/**
 * \file governance_action_type.cpp
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

#include <cardano/proposal_procedures/governance_action_type.h>
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_governance_action_type_to_string, canConvertParameterChange)
{
  const char* message = cardano_governance_action_type_to_string(CARDANO_GOVERNANCE_ACTION_TYPE_PARAMETER_CHANGE);

  ASSERT_STREQ("Governance Action Type: Parameter Change", message);
}

TEST(cardano_governance_action_type_to_string, canConvertHardForkInitiation)
{
  const char* message = cardano_governance_action_type_to_string(CARDANO_GOVERNANCE_ACTION_TYPE_HARD_FORK_INITIATION);

  ASSERT_STREQ("Governance Action Type: Hard Fork Initiation", message);
}

TEST(cardano_governance_action_type_to_string, canConvertTreasuryWithdrawals)
{
  const char* message = cardano_governance_action_type_to_string(CARDANO_GOVERNANCE_ACTION_TYPE_TREASURY_WITHDRAWALS);

  ASSERT_STREQ("Governance Action Type: Treasury Withdrawals", message);
}

TEST(cardano_governance_action_type_to_string, canConvertNoConfidence)
{
  const char* message = cardano_governance_action_type_to_string(CARDANO_GOVERNANCE_ACTION_TYPE_NO_CONFIDENCE);

  ASSERT_STREQ("Governance Action Type: No Confidence", message);
}

TEST(cardano_governance_action_type_to_string, canConvertUpdateCommittee)
{
  const char* message = cardano_governance_action_type_to_string(CARDANO_GOVERNANCE_ACTION_TYPE_UPDATE_COMMITTEE);

  ASSERT_STREQ("Governance Action Type: Update Committee", message);
}

TEST(cardano_governance_action_type_to_string, canConvertNewConstitution)
{
  const char* message = cardano_governance_action_type_to_string(CARDANO_GOVERNANCE_ACTION_TYPE_NEW_CONSTITUTION);

  ASSERT_STREQ("Governance Action Type: New Constitution", message);
}

TEST(cardano_governance_action_type_to_string, canConvertInfo)
{
  const char* message = cardano_governance_action_type_to_string(CARDANO_GOVERNANCE_ACTION_TYPE_INFO);

  ASSERT_STREQ("Governance Action Type: Info", message);
}

TEST(cardano_governance_action_type_to_string, canConvertUnknown)
{
  const char* message = cardano_governance_action_type_to_string((cardano_governance_action_type_t)100);

  ASSERT_STREQ("Governance Action Type: Unknown", message);
}
