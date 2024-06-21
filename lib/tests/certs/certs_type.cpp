/**
 * \file certs_type.cpp
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

#include <cardano/certs/cert_type.h>
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_certs_type_to_string, canConvertStakeRegistration)
{
  const char* message = cardano_cert_type_to_string(CARDANO_CERT_TYPE_STAKE_REGISTRATION);

  ASSERT_STREQ("Certificate Type: Stake Registration", message);
}

TEST(cardano_certs_type_to_string, canConvertStakeDeregistration)
{
  const char* message = cardano_cert_type_to_string(CARDANO_CERT_TYPE_STAKE_DEREGISTRATION);

  ASSERT_STREQ("Certificate Type: Stake Deregistration", message);
}

TEST(cardano_certs_type_to_string, canConvertStakeDelegation)
{
  const char* message = cardano_cert_type_to_string(CARDANO_CERT_TYPE_STAKE_DELEGATION);

  ASSERT_STREQ("Certificate Type: Stake Delegation", message);
}

TEST(cardano_certs_type_to_string, canConvertPoolRegistration)
{
  const char* message = cardano_cert_type_to_string(CARDANO_CERT_TYPE_POOL_REGISTRATION);

  ASSERT_STREQ("Certificate Type: Pool Registration", message);
}

TEST(cardano_certs_type_to_string, canConvertPoolRetirement)
{
  const char* message = cardano_cert_type_to_string(CARDANO_CERT_TYPE_POOL_RETIREMENT);

  ASSERT_STREQ("Certificate Type: Pool Retirement", message);
}

TEST(cardano_certs_type_to_string, canConvertGenesisKeyDelegation)
{
  const char* message = cardano_cert_type_to_string(CARDANO_CERT_TYPE_GENESIS_KEY_DELEGATION);

  ASSERT_STREQ("Certificate Type: Genesis Key Delegation", message);
}

TEST(cardano_certs_type_to_string, canConvertMoveInstantaneousRewards)
{
  const char* message = cardano_cert_type_to_string(CARDANO_CERT_TYPE_MOVE_INSTANTANEOUS_REWARDS);

  ASSERT_STREQ("Certificate Type: Move Instantaneous Rewards", message);
}

TEST(cardano_certs_type_to_string, canConvertRegistration)
{
  const char* message = cardano_cert_type_to_string(CARDANO_CERT_TYPE_REGISTRATION);

  ASSERT_STREQ("Certificate Type: Registration", message);
}

TEST(cardano_certs_type_to_string, canConvertUnregistration)
{
  const char* message = cardano_cert_type_to_string(CARDANO_CERT_TYPE_UNREGISTRATION);

  ASSERT_STREQ("Certificate Type: Unregistration", message);
}

TEST(cardano_certs_type_to_string, canConvertVoteDelegation)
{
  const char* message = cardano_cert_type_to_string(CARDANO_CERT_TYPE_VOTE_DELEGATION);

  ASSERT_STREQ("Certificate Type: Vote Delegation", message);
}

TEST(cardano_certs_type_to_string, canConvertStakeVoteDelegation)
{
  const char* message = cardano_cert_type_to_string(CARDANO_CERT_TYPE_STAKE_VOTE_DELEGATION);

  ASSERT_STREQ("Certificate Type: Stake Vote Delegation", message);
}

TEST(cardano_certs_type_to_string, canConvertStakeRegistrationDelegation)
{
  const char* message = cardano_cert_type_to_string(CARDANO_CERT_TYPE_STAKE_REGISTRATION_DELEGATION);

  ASSERT_STREQ("Certificate Type: Stake Registration Delegation", message);
}

TEST(cardano_certs_type_to_string, canConvertVoteRegistrationDelegation)
{
  const char* message = cardano_cert_type_to_string(CARDANO_CERT_TYPE_VOTE_REGISTRATION_DELEGATION);

  ASSERT_STREQ("Certificate Type: Vote Registration Delegation", message);
}

TEST(cardano_certs_type_to_string, canConvertStakeVoteRegistrationDelegation)
{
  const char* message = cardano_cert_type_to_string(CARDANO_CERT_TYPE_STAKE_VOTE_REGISTRATION_DELEGATION);

  ASSERT_STREQ("Certificate Type: Stake Vote Registration Delegation", message);
}

TEST(cardano_certs_type_to_string, canConvertAuthCommitteeHot)
{
  const char* message = cardano_cert_type_to_string(CARDANO_CERT_TYPE_AUTH_COMMITTEE_HOT);

  ASSERT_STREQ("Certificate Type: Auth Committee Hot", message);
}

TEST(cardano_certs_type_to_string, canConvertResignCommitteeCold)
{
  const char* message = cardano_cert_type_to_string(CARDANO_CERT_TYPE_RESIGN_COMMITTEE_COLD);

  ASSERT_STREQ("Certificate Type: Resign Committee Cold", message);
}

TEST(cardano_certs_type_to_string, canConvertDRepRegistration)
{
  const char* message = cardano_cert_type_to_string(CARDANO_CERT_TYPE_DREP_REGISTRATION);

  ASSERT_STREQ("Certificate Type: DRep Registration", message);
}

TEST(cardano_certs_type_to_string, canConvertDRepUnregistration)
{
  const char* message = cardano_cert_type_to_string(CARDANO_CERT_TYPE_DREP_UNREGISTRATION);

  ASSERT_STREQ("Certificate Type: DRep Unregistration", message);
}

TEST(cardano_certs_type_to_string, canConvertUpdateDRep)
{
  const char* message = cardano_cert_type_to_string(CARDANO_CERT_TYPE_UPDATE_DREP);

  ASSERT_STREQ("Certificate Type: Update DRep", message);
}

TEST(cardano_certs_type_to_string, canConvertUnknown)
{
  const char* message = cardano_cert_type_to_string((cardano_cert_type_t)100);

  ASSERT_STREQ("Certificate Type: Unknown", message);
}
