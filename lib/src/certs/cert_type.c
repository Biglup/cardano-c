/**
 * \file cert_type.c
 *
 * \author angel.castillo
 * \date   Jun 19, 2024
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

/* DEFINITIONS ***************************************************************/

const char*
cardano_cert_type_to_string(const cardano_cert_type_t cert_type)
{
  const char* message;

  switch (cert_type)
  {
    case CARDANO_CERT_TYPE_STAKE_REGISTRATION:
      message = "Certificate Type: Stake Registration";
      break;
    case CARDANO_CERT_TYPE_STAKE_DEREGISTRATION:
      message = "Certificate Type: Stake Deregistration";
      break;
    case CARDANO_CERT_TYPE_STAKE_DELEGATION:
      message = "Certificate Type: Stake Delegation";
      break;
    case CARDANO_CERT_TYPE_POOL_REGISTRATION:
      message = "Certificate Type: Pool Registration";
      break;
    case CARDANO_CERT_TYPE_POOL_RETIREMENT:
      message = "Certificate Type: Pool Retirement";
      break;
    case CARDANO_CERT_TYPE_GENESIS_KEY_DELEGATION:
      message = "Certificate Type: Genesis Key Delegation";
      break;
    case CARDANO_CERT_TYPE_MOVE_INSTANTANEOUS_REWARDS:
      message = "Certificate Type: Move Instantaneous Rewards";
      break;
    case CARDANO_CERT_TYPE_REGISTRATION:
      message = "Certificate Type: Registration";
      break;
    case CARDANO_CERT_TYPE_UNREGISTRATION:
      message = "Certificate Type: Unregistration";
      break;
    case CARDANO_CERT_TYPE_VOTE_DELEGATION:
      message = "Certificate Type: Vote Delegation";
      break;
    case CARDANO_CERT_TYPE_STAKE_VOTE_DELEGATION:
      message = "Certificate Type: Stake Vote Delegation";
      break;
    case CARDANO_CERT_TYPE_STAKE_REGISTRATION_DELEGATION:
      message = "Certificate Type: Stake Registration Delegation";
      break;
    case CARDANO_CERT_TYPE_VOTE_REGISTRATION_DELEGATION:
      message = "Certificate Type: Vote Registration Delegation";
      break;
    case CARDANO_CERT_TYPE_STAKE_VOTE_REGISTRATION_DELEGATION:
      message = "Certificate Type: Stake Vote Registration Delegation";
      break;
    case CARDANO_CERT_TYPE_AUTH_COMMITTEE_HOT:
      message = "Certificate Type: Auth Committee Hot";
      break;
    case CARDANO_CERT_TYPE_RESIGN_COMMITTEE_COLD:
      message = "Certificate Type: Resign Committee Cold";
      break;
    case CARDANO_CERT_TYPE_DREP_REGISTRATION:
      message = "Certificate Type: DRep Registration";
      break;
    case CARDANO_CERT_TYPE_DREP_UNREGISTRATION:
      message = "Certificate Type: DRep Unregistration";
      break;
    case CARDANO_CERT_TYPE_UPDATE_DREP:
      message = "Certificate Type: Update DRep";
      break;
    default:
      message = "Certificate Type: Unknown";
      break;
  }

  return message;
}
