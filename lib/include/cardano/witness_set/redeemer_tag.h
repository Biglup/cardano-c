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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_REDEEMER_TAG_H
#define BIGLUP_LABS_INCLUDE_CARDANO_REDEEMER_TAG_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * The redeemer tags act as an enumeration to signify the purpose or context of the
 * redeemer in a transaction. When a Plutus script is executed, the specific action
 * type related to the redeemer can be identified using these tags, allowing the
 * script to respond appropriately.
 */
typedef enum
{
  /**
   * \brief Indicates the redeemer is associated with a `Spending` script.
   *
   * A spending script validates the spending of a UTXO. A UTXO can have either a public key address or a
   * script address. To spend a UTXO with a script address, the script whose hash matches the script address
   * must be included in the transaction (either directly, or as a reference script), and is executed to validate it.
   */
  CARDANO_REDEEMER_TAG_SPEND = 0,

  /**
   * \brief Indicates the redeemer is associated with a `Minting` script.
   *
   * Minting scripts, sometimes also referred to as minting policies, are used to approve or reject minting of new assets.
   *
   * In Cardano, we can uniquely identify an asset by its `CurrencySymbol` and `TokenName`. If a transaction attempts to
   * mint some new assets, then for each unique `CurrencySymbol` in the new assets, the script whose hash matches the
   * `CurrencySymbol` must be included in the transaction, and is executed to validate the minting of that `CurrencySymbol`.
   */
  CARDANO_REDEEMER_TAG_MINT = 1,

  /**
   * \brief Indicates the redeemer is associated with a `Certifying` script.
   *
   * A certifying script can validate a number of certificate-related transactions,
   * such as: (1) registering a staking credential, and in doing so, creating a reward account
   * associated with the staking credential; (2) de-registering a staking credential, and in doing so,
   * terminating the reward account; (3) delegating a staking credential to a particular delegatee.
   *
   * In all these cases except registration, if the staking credential in question contains a script
   * hash (as opposed to a public key hash), the script with that hash must be included in the transaction,
   * and is executed to validate the action.
   */
  CARDANO_REDEEMER_TAG_CERTIFYING = 2,

  /**
   * \brief Indicates the redeemer is associated with a `Rewarding` script.
   *
   * As previously stated, a UTXO's address can be either a public key address or a script address.
   * Both kinds of addresses can optionally have a staking credential. A UTXO may contain Ada, and the Ada
   * it contains can be delegated to an SPO to earn staking rewards. Staking rewards are deposited into a
   * reward account corresponding to the staking credential.
   *
   * A staking credential can contain either a public key hash or a script hash. To withdraw rewards from a
   * reward account corresponding to a staking credential that contains a script hash, the script with that
   * particular hash must be included in the transaction, and is executed to validate the withdrawal.
   */
  CARDANO_REDEEMER_TAG_REWARD = 3,

  /**
   * \brief Indicates the redeemer is associated with a `Voting` script.
   *
   * A voting script validates votes cast by a Delegated Representative (DRep) or a constitutional committee member (CCM)
   * in a transaction. A DRep or a CCM can be associated with a script hash. If a transaction contains one or more votes
   * from a DRep or a constitution committee member associated with a script hash, the script with that hash must be
   * included in the transaction, and is executed to approve or reject the vote.
   */
  CARDANO_REDEEMER_TAG_VOTING = 4,

  /**
   * \brief Indicates the redeemer is associated with a `Proposing` script.
   *
   * A proposing script, also known as constitution script or guardrail script, validates
   * two kinds of governance actions: \ref cardano_parameter_change_action_t and \ref cardano_treasury_withdrawals_action_t`.
   *
   * There is a key distinction between proposing scripts and other kinds of scripts: proposing scripts
   * are not written by regular users. At any given point in time, there is exactly one active proposing script
   * being used by the entire chain. This proposing script must be included in transactions that propose
   * \ref cardano_parameter_change_action_t and \ref cardano_treasury_withdrawals_action_t`. The ledger enforces that no
   * other proposing script is accepted.
   *
   * The proposing script is updated only when there is a change to the constitution, via the
   * \ref cardano_new_constitution_action_t governance action.
   *
   * \note The proposing script decides whether the proposal of a governance action is allowed to go
   * through, rather than whether the governance action will be enacted. After a proposal goes through, it
   * will need to meet the appropriate criteria (such as gathering enough votes from constitution committee members,
   * DReps and/or SPOs) in order to be enacted.
   */
  CARDANO_REDEEMER_TAG_PROPOSING = 5,
} cardano_redeemer_tag_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_REDEEMER_TAG_H