/**
 * \file deferred_redeemer_withdraw_zero_example.c
 *
 * \author angel.castillo
 * \date   Jul 03, 2026
 *
 * Copyright 2026 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
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

#include "providers/provider_factory.h"

#include "utils/console.h"
#include "utils/utils.h"
#include <cardano/cardano.h>

#include <stdlib.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

// Compiled Aiken validator `indexed.withdraw_zero` (Plutus V3, blueprint hash
// 0b995c3b26e9c3e74f07499f5676d61f157e0739771cf114cb71ea2d). Its withdraw handler
// expects the redeemer `Constr 0 [[indices...]]` and genuinely enforces the payload
// on-chain: the list must enumerate ALL transaction inputs (same length as the input
// list) in STRICTLY ASCENDING order, with every index resolving to an existing input.
// Its publish handler accepts anything, so the stake registration/deregistration
// certificates in this example pass with a void redeemer.
static const char*    WITHDRAW_ZERO_VALIDATOR_V3   = "5901ad01010029800aba2aba1aba0aab9faab9eaab9dab9a4888888966002646465300130053754003300900398048012444b30013370e9002001c4cc8966002600260146ea800e264b30013010001899198008009bac30100022259800800c56600266e1cc8cc004004dd6180918079baa0072259800800c52000899b8048008cc008008c05000501119198008009bac3012300f375401044b30010018a4001133700900119801001180a000a022899199119914c00400a003480050021112cc00400a2946264b300133710004003159800acc004c02cc050dd519803002800c528c52820268cc0040126032007001401114a0809a29410131bad301700240546eb0c050c044dd500498008009bac3012300f3754010444b30010028a6103d87a80008acc004c01c006266e9520003301330140024bd70466002007301500299b8048004005003201e404914a080622646600600660260046eb4c04400500f45900d18059baa0038b2012300c300937540086e1d20008acc004cdc3a400c00713233224a26eb4c034004c034c038004c024dd5002459007200e18041804800980400098019baa0088a4d13656400401";
static const char*    SERIALIZED_BIP32_KEY_HANDLER = "0a0a0a0a01010000005c97db5e09b3a4919ec75ed1126056241a1e5278731c2e0b01bea0a5f42c22db4131e0a4bbe75633677eb0e60e2ecd3520178f85c7e0d4be77a449087fe9674ee52f946b07c1b56d228c496ec0d36dd44212ba8af0f6eed1a82194dd69f479c603";
static const uint64_t CONFIRM_TX_TIMEOUT_MS        = 240000U;
static const uint64_t PAYMENT_CRED_INDEX           = 0U;
static const uint64_t STAKE_CRED_INDEX             = 0U;
static const uint64_t SECONDS_IN_TWO_HOURS         = 60UL * 60UL * 2UL;

static const cardano_account_derivation_path_t ACCOUNT_DERIVATION_PATH = {
  .purpose   = 1852U | 0x80000000,
  .coin_type = 1815U | 0x80000000,
  .account   = 0U
};

static const cardano_derivation_path_t SIGNER_DERIVATION_PATH = {
  .purpose   = 1852U | 0x80000000,
  .coin_type = 1815U | 0x80000000,
  .account   = 0U,
  .role      = 0U,
  .index     = 0U
};

/* DECLARATIONS **************************************************************/

/**
 * \brief Retrieves the password for the secure key handler.
 *
 * \param buffer The buffer where to write the password.
 * \param buffer_len The size of the buffer.
 *
 * \return The length of the password (or -1 on error).
 */
static int32_t
get_passphrase(byte_t* buffer, const size_t buffer_len)
{
  console_warn("Enter passphrase: ");
  char          password[128] = { 0 };
  const int32_t password_len  = console_read_password(password, sizeof(password));

  if (buffer_len < password_len)
  {
    return -1;
  }

  cardano_utils_safe_memcpy(buffer, buffer_len, &password[0], password_len);

  // Clear local password from memory
  cardano_memzero(password, sizeof(password));

  return password_len;
}

/**
 * \brief Builds the withdrawal redeemer `Constr 0 [spend_input_indices]` from the balanced draft
 * transaction.
 *
 * This callback implements the redeemer side of the "withdraw zero" forwarding pattern: instead of
 * validating each spent UTXO with its own expensive spend script execution, the spend validators
 * merely check that the transaction withdraws (zero lovelace is enough) from a well-known script
 * reward account. That withdrawal triggers a single execution of the staking validator, which
 * performs the real business validation for the whole batch. To do its job in O(1) per input, the
 * staking validator is handed the canonical indices of all the inputs it must validate through its
 * redeemer.
 *
 * Those indices only exist after balancing: coin selection picks the fee inputs and the final
 * input set is sorted into canonical order. The builder therefore invokes this callback on every
 * balancing iteration with the balanced draft transaction and the final resolved inputs, and uses
 * the produced payload as the withdrawal redeemer.
 *
 * The Aiken staking validator used by this example genuinely enforces the payload on-chain: the
 * redeemer list must enumerate ALL transaction inputs (its length must equal the input count) in
 * STRICTLY ASCENDING order, and every index must resolve to an existing input. `resolved_inputs`
 * is not in canonical order, so this callback maps each entry to its canonical index and then
 * sorts the collected indices ascending; because every input is mapped exactly once, the sorted
 * result is exactly the full enumeration the validator demands.
 *
 * The callback must be a pure function of its arguments (no captured mutable state, no side
 * effects, no randomness), otherwise the balancing loop may fail to converge.
 *
 * \param[in]  user_context    Unused. No extra context is needed; everything comes from the arguments.
 * \param[in]  draft_tx        The balanced draft transaction. Borrowed; must not be modified.
 * \param[in]  resolved_inputs The final selected inputs as resolved UTXOs. Borrowed.
 * \param[out] redeemer        The produced redeemer payload. The caller takes ownership.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code (which aborts balancing).
 */
static cardano_error_t
build_withdraw_zero_redeemer(
  void*                   user_context,
  cardano_transaction_t*  draft_tx,
  cardano_utxo_list_t*    resolved_inputs,
  cardano_plutus_data_t** redeemer)
{
  CARDANO_UNUSED(user_context);

  const size_t input_count = cardano_utxo_list_get_length(resolved_inputs);

  uint64_t* canonical_indices = (uint64_t*)malloc(input_count * sizeof(uint64_t));

  if (canonical_indices == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  // Map every resolved input to its canonical position in the draft transaction. Since
  // `resolved_inputs` covers the whole input set, this yields every canonical index exactly once.
  for (size_t i = 0U; i < input_count; ++i)
  {
    cardano_utxo_t* utxo = NULL;

    cardano_error_t get_result = cardano_utxo_list_get(resolved_inputs, i, &utxo);

    if (get_result != CARDANO_SUCCESS)
    {
      free(canonical_indices);

      return get_result;
    }

    cardano_transaction_input_t* input      = cardano_utxo_get_input(utxo);
    cardano_blake2b_hash_t*      tx_id      = cardano_transaction_input_get_id(input);
    const uint64_t               utxo_index = cardano_transaction_input_get_index(input);

    uint64_t canonical_index = 0U;

    get_result = cardano_transaction_find_input_index(draft_tx, tx_id, utxo_index, &canonical_index);

    cardano_blake2b_hash_unref(&tx_id);
    cardano_transaction_input_unref(&input);
    cardano_utxo_unref(&utxo);

    if (get_result != CARDANO_SUCCESS)
    {
      free(canonical_indices);

      return get_result;
    }

    canonical_indices[i] = canonical_index;
  }

  // The validator requires the indices in STRICTLY ASCENDING order, but `resolved_inputs` is not
  // in canonical order, so the collected indices must be sorted before building the plutus list.
  // A small insertion sort keeps the callback self-contained and pure.
  for (size_t i = 1U; i < input_count; ++i)
  {
    const uint64_t key = canonical_indices[i];
    size_t         j   = i;

    while ((j > 0U) && (canonical_indices[j - 1U] > key))
    {
      canonical_indices[j] = canonical_indices[j - 1U];
      --j;
    }

    canonical_indices[j] = key;
  }

  cardano_plutus_list_t* input_indices = NULL;
  cardano_error_t        result        = cardano_plutus_list_new(&input_indices);

  if (result != CARDANO_SUCCESS)
  {
    free(canonical_indices);

    return result;
  }

  for (size_t i = 0U; i < input_count; ++i)
  {
    cardano_plutus_data_t* index_data = NULL;

    result = cardano_plutus_data_new_integer_from_uint(canonical_indices[i], &index_data);

    if (result != CARDANO_SUCCESS)
    {
      free(canonical_indices);
      cardano_plutus_list_unref(&input_indices);

      return result;
    }

    result = cardano_plutus_list_add(input_indices, index_data);
    cardano_plutus_data_unref(&index_data);

    if (result != CARDANO_SUCCESS)
    {
      free(canonical_indices);
      cardano_plutus_list_unref(&input_indices);

      return result;
    }
  }

  free(canonical_indices);

  cardano_plutus_data_t* indices_data = NULL;

  result = cardano_plutus_data_new_list(input_indices, &indices_data);
  cardano_plutus_list_unref(&input_indices);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_plutus_list_t* fields = NULL;

  result = cardano_plutus_list_new(&fields);

  if (result != CARDANO_SUCCESS)
  {
    cardano_plutus_data_unref(&indices_data);

    return result;
  }

  result = cardano_plutus_list_add(fields, indices_data);
  cardano_plutus_data_unref(&indices_data);

  if (result != CARDANO_SUCCESS)
  {
    cardano_plutus_list_unref(&fields);

    return result;
  }

  cardano_constr_plutus_data_t* constr = NULL;

  result = cardano_constr_plutus_data_new(0U, fields, &constr);
  cardano_plutus_list_unref(&fields);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_plutus_data_new_constr(constr, redeemer);
  cardano_constr_plutus_data_unref(&constr);

  return result;
}

/**
 * \brief Registers a reward address and delegates it to a specified stake pool.
 *
 * This function performs two operations:
 * 1. Registers a reward address on the blockchain.
 * 2. Delegates the registered reward address to a specified stake pool using the provided stake pool ID.
 *
 * \param[in] provider A pointer to a \ref cardano_provider_t object used for accessing blockchain state.
 * \param[in] key_handler A pointer to a \ref cardano_secure_key_handler_t object for signing and key management.
 * \param[in] pparams A pointer to a \ref cardano_protocol_parameters_t object containing the protocol parameters required for transaction creation.
 * \param[in] script A pointer to a \ref cardano_script_t The script used in the reward address credential.
 * \param[in] funding_address A pointer to a \ref cardano_address_t object representing the funding source for transaction fees.
 * \param[in] reward_address A pointer to a \ref cardano_reward_address_t object representing the reward address to be registered and delegated.
 * \param[in] pool_id A pointer to a \ref cardano_blake2b_hash_t object representing the stake pool ID to which the delegation will be made.
 */
static void
register_and_delegate(
  cardano_provider_t*            provider,
  cardano_secure_key_handler_t*  key_handler,
  cardano_protocol_parameters_t* pparams,
  cardano_script_t*              script,
  cardano_address_t*             funding_address,
  cardano_reward_address_t*      reward_address,
  cardano_blake2b_hash_t*        pool_id)
{
  cardano_utxo_list_t*   utxo_list  = get_unspent_utxos(provider, funding_address);
  cardano_tx_builder_t*  tx_builder = cardano_tx_builder_new(pparams, &CARDANO_PREPROD_SLOT_CONFIG);
  cardano_plutus_data_t* redeemer   = create_void_plutus_data();

  const uint64_t invalid_after = cardano_utils_get_time() + SECONDS_IN_TWO_HOURS;

  cardano_tx_builder_set_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_change_address(tx_builder, funding_address);
  cardano_tx_builder_set_collateral_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_collateral_change_address(tx_builder, funding_address);
  cardano_tx_builder_set_invalid_after_ex(tx_builder, invalid_after);
  cardano_tx_builder_add_script(tx_builder, script);
  cardano_tx_builder_register_reward_address(tx_builder, reward_address, redeemer);
  cardano_tx_builder_delegate_stake(tx_builder, reward_address, pool_id, redeemer);

  cardano_transaction_t* transaction = NULL;
  cardano_error_t        result      = cardano_tx_builder_build(tx_builder, &transaction);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to build transaction");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_tx_builder_get_last_error(tx_builder));

    exit(result);
  }

  sign_transaction(key_handler, SIGNER_DERIVATION_PATH, transaction);
  submit_transaction(provider, CONFIRM_TX_TIMEOUT_MS, transaction);

  cardano_utxo_list_unref(&utxo_list);
  cardano_tx_builder_unref(&tx_builder);
  cardano_transaction_unref(&transaction);
  cardano_plutus_data_unref(&redeemer);

  console_info("Stake key registered and delegated successfully.");
}

/**
 * \brief Withdraws zero lovelace from the script reward account with a deferred redeemer and
 * deregisters the reward address.
 *
 * The withdrawal is added with \ref cardano_tx_builder_withdraw_rewards_with_deferred_redeemer: instead of a
 * literal redeemer, a callback builds `Constr 0 [spend_input_indices]` once coin selection and
 * balancing have fixed the final canonical input set. This is the "withdraw zero" forwarding
 * pattern trigger: the mere presence of the withdrawal runs the staking validator once for the
 * whole transaction, and its redeemer points it at every input it must validate. The validator
 * rejects the transaction unless the indices enumerate all inputs in strictly ascending order.
 *
 * \param[in] provider A pointer to a \ref cardano_provider_t object used for accessing blockchain state.
 * \param[in] key_handler A pointer to a \ref cardano_secure_key_handler_t object for signing and key management.
 * \param[in] pparams A pointer to a \ref cardano_protocol_parameters_t object containing the protocol parameters required for transaction creation.
 * \param[in] script A pointer to a \ref cardano_script_t The script used in the reward address credential.
 * \param[in] funding_address A pointer to a \ref cardano_address_t object representing the funding source for transaction fees.
 * \param[in] reward_address A pointer to a \ref cardano_reward_address_t object representing the reward address to be unregistered and from which rewards will be withdrawn.
 */
static void
unregister_and_withdraw_rewards(
  cardano_provider_t*            provider,
  cardano_secure_key_handler_t*  key_handler,
  cardano_protocol_parameters_t* pparams,
  cardano_script_t*              script,
  cardano_address_t*             funding_address,
  cardano_reward_address_t*      reward_address)
{
  cardano_utxo_list_t*   utxo_list  = get_unspent_utxos(provider, funding_address);
  cardano_tx_builder_t*  tx_builder = cardano_tx_builder_new(pparams, &CARDANO_PREPROD_SLOT_CONFIG);
  cardano_plutus_data_t* redeemer   = create_void_plutus_data();

  const uint64_t invalid_after = cardano_utils_get_time() + SECONDS_IN_TWO_HOURS;

  cardano_tx_builder_set_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_change_address(tx_builder, funding_address);
  cardano_tx_builder_set_collateral_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_collateral_change_address(tx_builder, funding_address);
  cardano_tx_builder_set_invalid_after_ex(tx_builder, invalid_after);
  cardano_tx_builder_add_script(tx_builder, script);

  // Withdraw 0 since we just registered this account, so there are no rewards to withdraw. The
  // redeemer is deferred: the callback receives the balanced draft transaction and the final
  // resolved inputs, and builds `Constr 0 [spend_input_indices]` from them. No user context is
  // needed, so NULL is passed.
  cardano_tx_builder_withdraw_rewards_with_deferred_redeemer(tx_builder, reward_address, 0, build_withdraw_zero_redeemer, NULL);
  cardano_tx_builder_deregister_reward_address(tx_builder, reward_address, redeemer);

  cardano_transaction_t* transaction = NULL;
  cardano_error_t        result      = cardano_tx_builder_build(tx_builder, &transaction);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to build transaction");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_tx_builder_get_last_error(tx_builder));

    exit(result);
  }

  sign_transaction(key_handler, SIGNER_DERIVATION_PATH, transaction);
  submit_transaction(provider, CONFIRM_TX_TIMEOUT_MS, transaction);

  cardano_utxo_list_unref(&utxo_list);
  cardano_tx_builder_unref(&tx_builder);
  cardano_transaction_unref(&transaction);
  cardano_plutus_data_unref(&redeemer);

  console_info("Rewards withdrawn and stake key deregistered successfully.");
}

/* MAIN **********************************************************************/

/**
 * \brief Entry point of the program.
 *
 * \return Returns `0` on successful execution, or a non-zero value if there is an error.
 */
int
main(void)
{
  console_info("Deferred Redeemer (Withdraw Zero) Example");
  console_info("libcardano-c:  V-%s\n", cardano_get_lib_version());

  console_info("This example registers a script stake key and then withdraws zero lovelace from it using a deferred redeemer that carries the spend input indices. The validator verifies the indices on-chain.");

  console_set_foreground_color(CONSOLE_COLOR_GREEN);
  console_write("\nUse passphrase: 'password'\n\n");
  console_reset_color();

  const char* api_key = getenv("BLOCKFROST_API_KEY");

  if (api_key == NULL)
  {
    console_error("BLOCKFROST_API_KEY environment variable is not set.\n");

    return EXIT_FAILURE;
  }

  const char*             POOL_ID_HEX = "089a06986c7dbd50d411890a74ab9e60ba22d32bf6e59cb658491f2c"; // SMAUG
  cardano_blake2b_hash_t* policy_id   = NULL;

  cardano_error_t result = cardano_blake2b_hash_from_hex(POOL_ID_HEX, strlen(POOL_ID_HEX), &policy_id);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to convert pool ID to hash");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    return EXIT_FAILURE;
  }

  cardano_script_t*              withdraw_zero_script = create_plutus_v3_script_from_hex(WITHDRAW_ZERO_VALIDATOR_V3);
  cardano_address_t*             script_address       = get_script_address(withdraw_zero_script);
  cardano_reward_address_t*      reward_address       = get_script_stake_address(withdraw_zero_script);
  cardano_secure_key_handler_t*  key_handler          = create_secure_key_handler(SERIALIZED_BIP32_KEY_HANDLER, cardano_utils_safe_strlen(SERIALIZED_BIP32_KEY_HANDLER, 256), get_passphrase);
  cardano_provider_t*            provider             = create_provider(CARDANO_NETWORK_MAGIC_PREPROD, api_key);
  cardano_address_t*             payment_address      = create_address_from_derivation_paths(key_handler, ACCOUNT_DERIVATION_PATH, PAYMENT_CRED_INDEX, STAKE_CRED_INDEX);
  cardano_protocol_parameters_t* protocol_params      = get_protocol_parameters(provider);
  cardano_utxo_list_t*           utxo_list            = get_unspent_utxos(provider, payment_address);

  // Registers and delegates the stake key to the pool
  register_and_delegate(provider, key_handler, protocol_params, withdraw_zero_script, payment_address, reward_address, policy_id);

  // Withdraws zero lovelace from the script reward account with a deferred redeemer carrying the
  // spend input indices, and deregisters the stake key
  unregister_and_withdraw_rewards(provider, key_handler, protocol_params, withdraw_zero_script, payment_address, reward_address);

  // Cleanup
  cardano_script_unref(&withdraw_zero_script);
  cardano_address_unref(&script_address);
  cardano_reward_address_unref(&reward_address);
  cardano_provider_unref(&provider);
  cardano_address_unref(&payment_address);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_secure_key_handler_unref(&key_handler);
  cardano_utxo_list_unref(&utxo_list);
  cardano_blake2b_hash_unref(&policy_id);

  return EXIT_SUCCESS;
}
