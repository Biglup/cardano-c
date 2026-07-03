/**
 * \file deferred_redeemer_self_index_example.c
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

// Compiled Aiken validator `indexed.self_index` (Plutus V3, blueprint hash
// cb1d9a9421df4711b46535ccc0366b3b1df0f6401e1445a287f09078). Its spend handler
// expects the redeemer `Constr 0 [own_input_index]` and genuinely enforces the
// index on-chain: it looks up `inputs[own_input_index]` in the script context
// and checks that its output reference equals the validator's own output
// reference, failing otherwise.
static const char*    SELF_INDEX_VALIDATOR_V3      = "58f501010029800aba2aba1aba0aab9faab9eaab9dab9a488888896600264646644b30013370e900118031baa0018994c004c02c00660166018003370e90002444b30013001300a375400d1323259800980880144c9660026008601a6ea8006266ebcc044c038dd5180898071baa0010068b2018329800800cdd6180898071baa301100b9bad3011300e375401280088896600200514c103d87a80008acc004c018006266e9520003301230130024bd70466002007301400299b8048004005003201c40451640386eb4c03c004c02cdd50034590090c01cdd5000c59005180400098041804800980400098019baa0088a4d13656400401";
static const char*    SERIALIZED_BIP32_KEY_HANDLER = "0a0a0a0a01010000005c97db5e09b3a4919ec75ed1126056241a1e5278731c2e0b01bea0a5f42c22db4131e0a4bbe75633677eb0e60e2ecd3520178f85c7e0d4be77a449087fe9674ee52f946b07c1b56d228c496ec0d36dd44212ba8af0f6eed1a82194dd69f479c603";
static const uint64_t CONFIRM_TX_TIMEOUT_MS        = 240000U;
static const int64_t  LOVELACE_TO_SEND             = 2000000U;
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
 * \brief Builds the spend redeemer `Constr 0 [own_input_index]` from the balanced draft transaction.
 *
 * This callback implements the "witness-driven" redeemer pattern: many validators expect their
 * redeemer to carry the canonical index of the very input they are validating (their "own" input),
 * so they can cheaply look themselves up in the script context instead of scanning every input.
 * The Aiken validator used by this example really enforces it: it evaluates
 * `inputs[own_input_index].output_reference == own_ref` on-chain, so a wrong index makes the
 * script (and the transaction) fail.
 *
 * The catch is that the canonical input index is not known while the transaction is being declared:
 * inputs are sorted lexicographically (by transaction id and output index) during balancing, and
 * coin selection may add more inputs around ours. Deferred redeemer callbacks solve this chicken
 * and egg problem: the builder invokes this function on every balancing iteration, once the draft
 * transaction has its final canonical input order, change outputs and fee, and the payload it
 * produces replaces the redeemer in the witness set.
 *
 * The callback must be a pure function of its arguments (no captured mutable state, no side
 * effects, no randomness), otherwise the balancing loop may fail to converge. The script UTXO is
 * received through `user_context`; the caller keeps it alive until the transaction is built.
 *
 * \param[in]  user_context    The script UTXO being spent (a \ref cardano_utxo_t object).
 * \param[in]  draft_tx        The balanced draft transaction. Borrowed; must not be modified.
 * \param[in]  resolved_inputs The final selected inputs as resolved UTXOs. Borrowed.
 * \param[out] redeemer        The produced redeemer payload. The caller takes ownership.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code (which aborts balancing).
 */
static cardano_error_t
build_self_index_redeemer(
  void*                   user_context,
  cardano_transaction_t*  draft_tx,
  cardano_utxo_list_t*    resolved_inputs,
  cardano_plutus_data_t** redeemer)
{
  CARDANO_UNUSED(resolved_inputs);

  cardano_utxo_t*              script_utxo = (cardano_utxo_t*)user_context;
  cardano_transaction_input_t* input       = cardano_utxo_get_input(script_utxo);
  cardano_blake2b_hash_t*      tx_id       = cardano_transaction_input_get_id(input);
  const uint64_t               utxo_index  = cardano_transaction_input_get_index(input);

  uint64_t        own_input_index = 0U;
  cardano_error_t result          = cardano_transaction_find_input_index(draft_tx, tx_id, utxo_index, &own_input_index);

  cardano_blake2b_hash_unref(&tx_id);
  cardano_transaction_input_unref(&input);

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

  cardano_plutus_data_t* index_data = NULL;

  result = cardano_plutus_data_new_integer_from_uint(own_input_index, &index_data);

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
  console_info("Deferred Redeemer (Self Index) Example");
  console_info("libcardano-c:  V-%s\n", cardano_get_lib_version());

  console_info("This example will spend balance from a plutus script using a deferred redeemer that carries the input's own canonical index. The validator verifies the index on-chain.");

  console_set_foreground_color(CONSOLE_COLOR_GREEN);
  console_write("\nUse passphrase: 'password'\n\n");
  console_reset_color();

  const char* api_key = getenv("BLOCKFROST_API_KEY");

  if (api_key == NULL)
  {
    console_error("BLOCKFROST_API_KEY environment variable is not set.\n");

    return EXIT_FAILURE;
  }

  cardano_script_t*              self_index_script = create_plutus_v3_script_from_hex(SELF_INDEX_VALIDATOR_V3);
  cardano_address_t*             script_address    = get_script_address(self_index_script);
  cardano_secure_key_handler_t*  key_handler       = create_secure_key_handler(SERIALIZED_BIP32_KEY_HANDLER, cardano_utils_safe_strlen(SERIALIZED_BIP32_KEY_HANDLER, 256), get_passphrase);
  cardano_provider_t*            provider          = create_provider(CARDANO_NETWORK_MAGIC_PREPROD, api_key);
  cardano_address_t*             payment_address   = create_address_from_derivation_paths(key_handler, ACCOUNT_DERIVATION_PATH, PAYMENT_CRED_INDEX, STAKE_CRED_INDEX);
  cardano_protocol_parameters_t* protocol_params   = get_protocol_parameters(provider);

  cardano_utxo_t* script_utxo = fund_script_address(provider, key_handler, protocol_params, payment_address, script_address, LOVELACE_TO_SEND);

  cardano_utxo_list_t* utxo_list = get_unspent_utxos(provider, payment_address);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(protocol_params, &CARDANO_PREPROD_SLOT_CONFIG);

  const uint64_t invalid_after = cardano_utils_get_time() + SECONDS_IN_TWO_HOURS;

  cardano_tx_builder_set_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_collateral_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_collateral_change_address(tx_builder, payment_address);
  cardano_tx_builder_set_change_address(tx_builder, payment_address);
  cardano_tx_builder_set_invalid_after_ex(tx_builder, invalid_after);
  cardano_tx_builder_add_script(tx_builder, self_index_script);
  cardano_tx_builder_send_lovelace(tx_builder, payment_address, 1000000);

  // Instead of a literal redeemer, register a callback that builds `Constr 0 [own_input_index]`
  // once the canonical input order is known. The script UTXO is passed as `user_context`; it must
  // stay alive until `cardano_tx_builder_build` returns (we unref it after building).
  cardano_tx_builder_add_input_with_deferred_redeemer(tx_builder, script_utxo, build_self_index_redeemer, script_utxo, NULL); // Datum is inlined in the UTXO

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
  cardano_script_unref(&self_index_script);
  cardano_address_unref(&script_address);
  cardano_provider_unref(&provider);
  cardano_address_unref(&payment_address);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_secure_key_handler_unref(&key_handler);
  cardano_utxo_list_unref(&utxo_list);
  cardano_tx_builder_unref(&tx_builder);
  cardano_transaction_unref(&transaction);
  cardano_utxo_unref(&script_utxo);

  return EXIT_SUCCESS;
}
