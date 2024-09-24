/**
 * \file cert_type.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CERT_TYPE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CERT_TYPE_H

/* INCLUDES ******************************************************************/

#include <cardano/export.h>

/* DECLARATIONS *************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Certificates are used to register, update, or deregister stake pools, and delegate stake.
 */
typedef enum
{
  /**
   * \brief This certificate is used when an individual wants to register as a stakeholder.
   * It allows the holder to participate in the staking process by delegating their
   * stake or creating a stake pool.
   */
  CARDANO_CERT_TYPE_STAKE_REGISTRATION = 0,

  /**
   * \brief This certificate is used when a stakeholder no longer wants to participate in
   * staking. It revokes the stake registration and the associated stake is no
   * longer counted when calculating stake pool rewards.
   */
  CARDANO_CERT_TYPE_STAKE_DEREGISTRATION = 1,

  /**
   * \brief This certificate is used when a stakeholder wants to delegate their stake to a
   * specific stake pool. It includes the stake pool id to which the stake is delegated.
   */
  CARDANO_CERT_TYPE_STAKE_DELEGATION = 2,

  /**
   * \brief This certificate is used to register a new stake pool. It includes various details
   * about the pool such as the pledge, costs, margin, reward account, and the pool's owners and relays.
   */
  CARDANO_CERT_TYPE_POOL_REGISTRATION = 3,

  /**
   * \brief This certificate is used to retire a stake pool. It includes an epoch number indicating when the pool will be retired.
   */
  CARDANO_CERT_TYPE_POOL_RETIREMENT = 4,

  /**
   * \brief This certificate is used to delegate from a Genesis key to a set of keys. This was primarily used in the early
   * phases of the Cardano network's existence during the transition from the Byron to the Shelley era.
   */
  CARDANO_CERT_TYPE_GENESIS_KEY_DELEGATION = 5,

  /**
   * \brief Certificate used to facilitate an instantaneous transfer of rewards within the system.
   */
  CARDANO_CERT_TYPE_MOVE_INSTANTANEOUS_REWARDS = 6,

  /**
   * \brief This certificate is used when an individual wants to register as a stakeholder.
   * It allows the holder to participate in the staking process by delegating their
   * stake or creating a stake pool.
   *
   * Deposit must match the expected deposit amount specified by `ppKeyDepositL` in
   * the protocol parameters.
   *
   * Remark: Replaces the deprecated `StakeRegistration` in after Conway era.
   */
  CARDANO_CERT_TYPE_REGISTRATION = 7,

  /**
   * \brief This certificate is used when a stakeholder no longer wants to participate in
   * staking. It revokes the stake registration and the associated stake is no
   * longer counted when calculating stake pool rewards.
   *
   * Deposit must match the expected deposit amount specified by `ppKeyDepositL` in
   * the protocol parameters.
   *
   * Remark: Replaces the deprecated `StakeDeregistration` in after Conway era.
   */
  CARDANO_CERT_TYPE_UNREGISTRATION = 8,

  /**
   * \brief This certificate is used when an individual wants to delegate their voting rights to any other DRep.
   */
  CARDANO_CERT_TYPE_VOTE_DELEGATION = 9,

  /**
   * \brief This certificate is used when an individual wants to delegate their voting
   * rights to any other DRep and simultaneously wants to delegate their stake to a
   * specific stake pool.
   */
  CARDANO_CERT_TYPE_STAKE_VOTE_DELEGATION = 10,

  /**
   * \brief This certificate Register the stake key and delegate with a single certificate to a stake pool.
   */
  CARDANO_CERT_TYPE_STAKE_REGISTRATION_DELEGATION = 11,

  /**
   * \brief This certificate Register the stake key and delegate with a single certificate to a DRep.
   */
  CARDANO_CERT_TYPE_VOTE_REGISTRATION_DELEGATION = 12,

  /**
   * \brief This certificate is used when an individual wants to register its stake key,
   * delegate their voting rights to any other DRep and simultaneously wants to delegate
   * their stake to a specific stake pool.
   */
  CARDANO_CERT_TYPE_STAKE_VOTE_REGISTRATION_DELEGATION = 13,

  /**
   * \brief This certificate registers the Hot and Cold credentials of a committee member.
   */
  CARDANO_CERT_TYPE_AUTH_COMMITTEE_HOT = 14,

  /**
   * \brief This certificate is used then a committee member wants to resign early (will be marked on-chain as an expired member).
   */
  CARDANO_CERT_TYPE_RESIGN_COMMITTEE_COLD = 15,

  /**
   * \brief In Voltaire, existing stake credentials will be able to delegate their stake to DReps for voting
   * purposes, in addition to the current delegation to stake pools for block production.
   * DRep delegation will mimic the existing stake delegation mechanisms (via on-chain certificates).
   *
   * This certificate register a stake key as a DRep.
   */
  CARDANO_CERT_TYPE_DREP_REGISTRATION = 16,

  /**
   * \brief This certificate unregister an individual as a DRep.
   *
   * Note that a DRep is retired immediately upon the chain accepting a retirement certificate, and
   * the deposit is returned as part of the transaction that submits the retirement certificate
   * (the same way that stake credential registration deposits are returned).
   */
  CARDANO_CERT_TYPE_DREP_UNREGISTRATION = 17,

  /**
   * \brief Updates the DRep anchored metadata.
   */
  CARDANO_CERT_TYPE_UPDATE_DREP = 18
} cardano_cert_type_t;

/**
 * \brief Converts certificate types to their human readable form.
 *
 * \param[in] type The certificate type to get the string representation for.
 * \return Human readable form of the given certificate type.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_cert_type_to_string(cardano_cert_type_t type);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CERT_TYPE_H