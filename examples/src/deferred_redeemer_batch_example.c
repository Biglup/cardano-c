/**
 * \file deferred_redeemer_batch_example.c
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

// Compiled Aiken validator `indexed.order_pairing` (Plutus V3, blueprint hash
// 87b9c20485fa8050f32b1eee69823b09133dd1bcc86f47bfae8d69b2). Its spend handler
// expects the redeemer `Constr 0 [own_input_index, payout_output_index]` and
// genuinely enforces both indices on-chain: it checks that
// `inputs[own_input_index].output_reference` equals its own output reference AND
// that `outputs[payout_output_index]` pays at least 1_000_000 lovelace.
static const char*    ORDER_PAIRING_VALIDATOR_V3   = "59022c01010029800aba2aba1aba0aab9faab9eaab9dab9a488888896600264653001300800198041804800cdc3a400530080024888966002600460106ea800e2653001300d00198069807000cdc3a40009112cc004c004c030dd500444c8c8cc8966002602a007132323232598009804980a1baa001899192cc004c02cc058dd5000c56600266ebcc068c05cdd5180d180b9baa00300d899b894820225e8c9660026022602e6ea80062900044dd6980d980c1baa001405864b300130113017375400314c103d87a8000899198008009bab301c3019375400444b30010018a6103d87a8000899192cc004cdc8a45000018acc004cdc7a44100001898059980f180e00125eb82298103d87a80004069133004004302000340686eb8c068004c07400501b202c32330010013756600660306ea8c06cc060dd5001112cc004006298103d87a8000899192cc004cdc8a45000018acc004cdc7a44100001898051980e980d80125eb82298103d87a80004065133004004301f00340646eb8c064004c07000501a4528202a8b202a330033758603260346034602c6ea8038dd69800980b1baa01123019301a0018b2026330013758602e60286ea8030dd6980b980a1baa00f3001001222598008014530103d87a80008acc004c024006260086602e603000497ae08cc00400e6032005337009000800a006404c80b0dd2a40011640486eb4c048004dd69809001180900098069baa0088b201618049baa0038b200e180400098019baa0088a4d1365640041";
static const char*    SERIALIZED_BIP32_KEY_HANDLER = "0a0a0a0a01010000005c97db5e09b3a4919ec75ed1126056241a1e5278731c2e0b01bea0a5f42c22db4131e0a4bbe75633677eb0e60e2ecd3520178f85c7e0d4be77a449087fe9674ee52f946b07c1b56d228c496ec0d36dd44212ba8af0f6eed1a82194dd69f479c603";
static const uint64_t CONFIRM_TX_TIMEOUT_MS        = 240000U;
static const int64_t  LOVELACE_TO_SEND             = 2000000U;
static const uint64_t PAYOUT_LOVELACE_A            = 1200000U;
static const uint64_t PAYOUT_LOVELACE_B            = 1500000U;
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

/* STRUCTS *******************************************************************/

/**
 * \brief Context for one batched order: the script UTXO being spent and the payout output that
 * fulfills it.
 *
 * This is the classic "batcher" pairing pattern: a batcher transaction spends several script
 * UTXOs (orders) at once and each order's redeemer points at both its own input index and the
 * index of the output that pays it out, so the validator can check its own settlement in O(1)
 * without scanning the whole transaction.
 *
 * One instance of this struct is passed as `user_context` to each deferred redeemer callback.
 * The caller must keep it (and the objects it references) alive until the transaction is built.
 */
typedef struct batch_order_context_t
{
    cardano_utxo_t*    own_utxo;
    cardano_address_t* payout_address;
    uint64_t           payout_lovelace;
} batch_order_context_t;

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
 * \brief Builds the spend redeemer `Constr 0 [own_input_index, payout_output_index]` from the
 * balanced draft transaction.
 *
 * The Aiken validator used by this example really enforces both indices on-chain: it checks that
 * `inputs[own_input_index].output_reference` matches its own output reference and that
 * `outputs[payout_output_index]` carries at least 1_000_000 lovelace, failing otherwise.
 *
 * Neither index is known while the transaction is being declared: inputs are sorted into canonical
 * order during balancing (and coin selection may add fee inputs around ours), while outputs are
 * indexed in body order and followed by balancer-generated change. The builder therefore invokes
 * this callback on every balancing iteration, once the draft transaction has its final layout, and
 * uses the produced payload as the redeemer.
 *
 * The callback must be a pure function of its arguments (no captured mutable state, no side
 * effects, no randomness), otherwise the balancing loop may fail to converge.
 *
 * \param[in]  user_context    The \ref batch_order_context_t describing this order.
 * \param[in]  draft_tx        The balanced draft transaction. Borrowed; must not be modified.
 * \param[in]  resolved_inputs The final selected inputs as resolved UTXOs. Borrowed.
 * \param[out] redeemer        The produced redeemer payload. The caller takes ownership.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code (which aborts balancing).
 */
static cardano_error_t
build_batch_order_redeemer(
  void*                   user_context,
  cardano_transaction_t*  draft_tx,
  cardano_utxo_list_t*    resolved_inputs,
  cardano_plutus_data_t** redeemer)
{
  CARDANO_UNUSED(resolved_inputs);

  const batch_order_context_t* context = (const batch_order_context_t*)user_context;

  cardano_transaction_input_t* input      = cardano_utxo_get_input(context->own_utxo);
  cardano_blake2b_hash_t*      tx_id      = cardano_transaction_input_get_id(input);
  const uint64_t               utxo_index = cardano_transaction_input_get_index(input);

  uint64_t        own_input_index = 0U;
  cardano_error_t result          = cardano_transaction_find_input_index(draft_tx, tx_id, utxo_index, &own_input_index);

  cardano_blake2b_hash_unref(&tx_id);
  cardano_transaction_input_unref(&input);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  // Finds the first output paying at least `payout_lovelace` to `payout_address`. User-declared
  // outputs come before balancer-generated change outputs, so this matches the payout we added
  // with `cardano_tx_builder_send_lovelace`. Since both payouts here go to the same address, we
  // added the smaller payout first so that each minimum-lovelace lookup finds its own output.
  uint64_t payout_output_index = 0U;

  result = cardano_transaction_find_output_index(draft_tx, context->payout_address, context->payout_lovelace, &payout_output_index);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_plutus_list_t* fields = NULL;

  result = cardano_plutus_list_new(&fields);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  const uint64_t indices[] = { own_input_index, payout_output_index };

  for (size_t i = 0U; i < (sizeof(indices) / sizeof(indices[0])); ++i)
  {
    cardano_plutus_data_t* index_data = NULL;

    result = cardano_plutus_data_new_integer_from_uint(indices[i], &index_data);

    if (result != CARDANO_SUCCESS)
    {
      cardano_plutus_list_unref(&fields);

      return result;
    }

    result = cardano_plutus_list_add(fields, index_data);
    cardano_plutus_data_unref(&index_data);

    if (result != CARDANO_SUCCESS)
    {
      cardano_plutus_list_unref(&fields);

      return result;
    }
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
 * \brief Funds a script address with the specified amount.
 *
 * This function creates and submits a transaction that transfers the specified amount from the
 * `funding_address` to the `script_address`. It leverages the provided `provider` to interact
 * with the Cardano network, and uses the `key_handler` to sign the transaction.
 *
 * \param[in] provider The Cardano provider object for network access.
 * \param[in] key_handler The secure key handler object for signing the transaction.
 * \param[in] pparams The protocol parameters object required for transaction building and fee calculations.
 * \param[in] funding_address The address from which funds are withdrawn to fund the script address.
 * \param[in] script_address The destination script address to receive the specified amount.
 * \param[in] amount The amount of lovelace to fund the script address.
 *
 * \return A pointer to the \ref cardano_utxo_t object created at the script address if successful.
 *         Returns NULL if the funding transaction fails.
 */
static cardano_utxo_t*
fund_script_address(
  cardano_provider_t*            provider,
  cardano_secure_key_handler_t*  key_handler,
  cardano_protocol_parameters_t* pparams,
  cardano_address_t*             funding_address,
  cardano_address_t*             script_address,
  const uint32_t                 amount)
{
  console_info("Funding script address: %s", cardano_address_get_string(script_address));

  cardano_utxo_list_t*  utxo_list  = get_unspent_utxos(provider, funding_address);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(pparams, &CARDANO_PREPROD_SLOT_CONFIG);
  cardano_datum_t*      datum      = create_void_datum();

  const uint64_t invalid_after = cardano_utils_get_time() + SECONDS_IN_TWO_HOURS;

  cardano_tx_builder_set_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_change_address(tx_builder, funding_address);
  cardano_tx_builder_set_invalid_after_ex(tx_builder, invalid_after);
  cardano_tx_builder_lock_lovelace(tx_builder, script_address, amount, datum);

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

  cardano_utxo_t* utxo = get_utxo_at_index(transaction, 0);

  cardano_utxo_list_unref(&utxo_list);
  cardano_tx_builder_unref(&tx_builder);
  cardano_transaction_unref(&transaction);
  cardano_datum_unref(&datum);

  console_info("Script address funded successfully.");

  return utxo;
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
  console_info("Deferred Redeemer (Batch) Example");
  console_info("libcardano-c:  V-%s\n", cardano_get_lib_version());

  console_info("This example will spend two UTxOs from a plutus script in a single transaction, pairing each input with its payout output via deferred redeemers. The validator verifies both indices on-chain.");

  console_set_foreground_color(CONSOLE_COLOR_GREEN);
  console_write("\nUse passphrase: 'password'\n\n");
  console_reset_color();

  const char* api_key = getenv("BLOCKFROST_API_KEY");

  if (api_key == NULL)
  {
    console_error("BLOCKFROST_API_KEY environment variable is not set.\n");

    return EXIT_FAILURE;
  }

  cardano_script_t*              order_pairing_script = create_plutus_v3_script_from_hex(ORDER_PAIRING_VALIDATOR_V3);
  cardano_address_t*             script_address       = get_script_address(order_pairing_script);
  cardano_secure_key_handler_t*  key_handler          = create_secure_key_handler(SERIALIZED_BIP32_KEY_HANDLER, cardano_utils_safe_strlen(SERIALIZED_BIP32_KEY_HANDLER, 256), get_passphrase);
  cardano_provider_t*            provider             = create_provider(CARDANO_NETWORK_MAGIC_PREPROD, api_key);
  cardano_address_t*             payment_address      = create_address_from_derivation_paths(key_handler, ACCOUNT_DERIVATION_PATH, PAYMENT_CRED_INDEX, STAKE_CRED_INDEX);
  cardano_protocol_parameters_t* protocol_params      = get_protocol_parameters(provider);

  // Create the two "orders" to batch: two UTxOs locked at the script address.
  cardano_utxo_t* script_utxo_a = fund_script_address(provider, key_handler, protocol_params, payment_address, script_address, LOVELACE_TO_SEND);
  cardano_utxo_t* script_utxo_b = fund_script_address(provider, key_handler, protocol_params, payment_address, script_address, LOVELACE_TO_SEND);

  cardano_utxo_list_t* utxo_list = get_unspent_utxos(provider, payment_address);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(protocol_params, &CARDANO_PREPROD_SLOT_CONFIG);

  const uint64_t invalid_after = cardano_utils_get_time() + SECONDS_IN_TWO_HOURS;

  cardano_tx_builder_set_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_collateral_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_collateral_change_address(tx_builder, payment_address);
  cardano_tx_builder_set_change_address(tx_builder, payment_address);
  cardano_tx_builder_set_invalid_after_ex(tx_builder, invalid_after);
  cardano_tx_builder_add_script(tx_builder, order_pairing_script);

  // Payout outputs. They are declared before building, so their body indices come before any
  // balancer-generated change outputs. The smaller payout is added first so each callback's
  // minimum-lovelace lookup resolves to its own output (both pay to the same address here).
  // Both payouts exceed the 1_000_000 lovelace minimum that the validator enforces on-chain
  // for `outputs[payout_output_index]`.
  cardano_tx_builder_send_lovelace(tx_builder, payment_address, PAYOUT_LOVELACE_A);
  cardano_tx_builder_send_lovelace(tx_builder, payment_address, PAYOUT_LOVELACE_B);

  // The contexts must stay alive until `cardano_tx_builder_build` returns; they live on the
  // stack of `main`, and the UTXOs and address they reference are unrefed only after building.
  batch_order_context_t order_a_context = { .own_utxo = script_utxo_a, .payout_address = payment_address, .payout_lovelace = PAYOUT_LOVELACE_A };
  batch_order_context_t order_b_context = { .own_utxo = script_utxo_b, .payout_address = payment_address, .payout_lovelace = PAYOUT_LOVELACE_B };

  // Each deferred callback builds `Constr 0 [own_input_index, payout_output_index]` once the
  // canonical input order and the final output list are known.
  cardano_tx_builder_add_input_with_deferred_redeemer(tx_builder, script_utxo_a, build_batch_order_redeemer, &order_a_context, NULL); // Datum is inlined in the UTXO
  cardano_tx_builder_add_input_with_deferred_redeemer(tx_builder, script_utxo_b, build_batch_order_redeemer, &order_b_context, NULL); // Datum is inlined in the UTXO

  cardano_transaction_t* transaction = NULL;

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &transaction);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to build transaction");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_tx_builder_get_last_error(tx_builder));

    exit(result);
  }

  sign_transaction(key_handler, SIGNER_DERIVATION_PATH, transaction);
  submit_transaction(provider, CONFIRM_TX_TIMEOUT_MS, transaction);

  // Cleanup
  cardano_script_unref(&order_pairing_script);
  cardano_address_unref(&script_address);
  cardano_provider_unref(&provider);
  cardano_address_unref(&payment_address);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_secure_key_handler_unref(&key_handler);
  cardano_utxo_list_unref(&utxo_list);
  cardano_tx_builder_unref(&tx_builder);
  cardano_transaction_unref(&transaction);
  cardano_utxo_unref(&script_utxo_a);
  cardano_utxo_unref(&script_utxo_b);

  return EXIT_SUCCESS;
}
