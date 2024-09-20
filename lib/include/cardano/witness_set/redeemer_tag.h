/**
 * \file redeemer_tag.h
 *
 * \author angel.castillo
 * \date   Sep 20, 2024
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

#ifndef CARDANO_REDEEMER_TAG_H
#define CARDANO_REDEEMER_TAG_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * The redeemer tags act as an enumeration to signify the purpose or context of the
 * redeemer in a transaction. When a Plutus script is executed, the specific action
 * type related to the redeemer can be identified using these tags, allowing the
 * script to respond appropriately.
 *
 * redeemer_tag =
 *     0 ; Spending
 *   / 1 ; Minting
 *   / 2 ; Certifying
 *   / 3 ; Rewarding
 *   / 4 ; Voting
 *   / 5 ; Proposing
 */
typedef enum
{
  /**
   * \brief Indicates the redeemer is for spending a UTxO.
   */
  CARDANO_REDEEMER_TAG_SPEND = 0,

  /**
   * \brief Indicates the redeemer is associated with a minting action.
   */
  CARDANO_REDEEMER_TAG_MINT = 1,

  /**
   * \brief Indicates the redeemer is related to a certificate action within a transaction.
   */
  CARDANO_REDEEMER_TAG_CERTIFYING = 2,

  /**
   * \brief Indicates the redeemer is for withdrawing rewards from staking.
   */
  CARDANO_REDEEMER_TAG_REWARD = 3,

  /**
   * \brief Indicates the redeemer is for voting.
   */
  CARDANO_REDEEMER_TAG_VOTING = 4,

  /**
   * \brief Indicates the redeemer is for proposing.
   */
  CARDANO_REDEEMER_TAG_PROPOSING = 5,
} cardano_redeemer_tag_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CARDANO_REDEEMER_TAG_H