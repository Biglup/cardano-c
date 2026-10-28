/**
 * \file plutus_v4_guarded_sub_transaction_example.c
 *
 * \author angel.castillo
 * \date   Oct 28, 2026
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

#include "utils/console.h"
#include "utils/utils.h"
#include <cardano/cardano.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

// The Plutus V4 validator: (program 1.0.0 (lam ctx (con unit ()))). It ignores its script context and
// returns unit, so it lets anyone spend the UTXOs it locks.
static const char* ALWAYS_SUCCEEDS_VALIDATOR_V4 = "450100002499";

// The Plutus V4 guard: (program 1.1.0 (lam ctx (con unit ()))). It is satisfied by any redeemer. Its
// program version differs from the validator, so it has its own script hash.
static const char* ALWAYS_SUCCEEDS_GUARD_V4 = "450101002499";

// The environment variables this example reads.
static const char* PRIVATE_KEY_ENV_VAR   = "PAYMENT_PRIVATE_KEY";
static const char* V4_COST_MODEL_ENV_VAR = "PLUTUS_V4_COST_MODEL";

static const uint64_t DIJKSTRA_PROTOCOL_MAJOR = 12U;
static const uint64_t LOVELACE_TO_LOCK        = 5000000U;
static const uint64_t LOVELACE_FOR_SUB_TX     = 5000000U;
static const size_t   TX_ID_HEX_SIZE          = 64U;

// The largest number of parameters a cost model given in the environment may have.
enum
{
  MAX_COST_MODEL_SIZE = 512
};

/* DECLARATIONS **************************************************************/

/**
 * \brief Exits the program if the given result is an error.
 *
 * \param[in] result The result to check.
 * \param[in] message The message to print if the result is an error.
 */
static void
exit_on_error(const cardano_error_t result, const char* message)
{
  if (result != CARDANO_SUCCESS)
  {
    console_error("%s", message);
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }
}

/**
 * \brief Parses the Plutus V4 cost model of the network.
 *
 * The cost model is a list of integers, separated by commas or spaces, optionally wrapped in brackets,
 * as printed by `cardano-cli query protocol-parameters | jq -c '.costModels.PlutusV4'`. It must be the
 * exact cost model of the network, since the script data hash of the transaction commits to it.
 *
 * \param[in] text The cost model as text.
 *
 * \return The cost model. The caller is responsible for releasing it.
 */
static cardano_cost_model_t*
parse_v4_cost_model(const char* text)
{
  int64_t     costs[MAX_COST_MODEL_SIZE] = { 0 };
  size_t      count                      = 0U;
  const char* cursor                     = text;

  while (*cursor != '\0')
  {
    if ((*cursor == '-') || ((*cursor >= '0') && (*cursor <= '9')))
    {
      char* end = NULL;

      errno = 0;

      const long long value = strtoll(cursor, &end, 10);

      if ((errno != 0) || (count >= (size_t)MAX_COST_MODEL_SIZE))
      {
        console_error("The %s environment variable is not a valid cost model.", V4_COST_MODEL_ENV_VAR);

        exit(EXIT_FAILURE);
      }

      costs[count] = (int64_t)value;
      ++count;
      cursor = end;
    }
    else
    {
      ++cursor;
    }
  }

  cardano_cost_model_t* cost_model = NULL;

  exit_on_error(cardano_cost_model_new(CARDANO_PLUTUS_LANGUAGE_VERSION_V4, costs, count, &cost_model), "Failed to create the Plutus V4 cost model");

  return cost_model;
}

/**
 * \brief Creates the protocol parameters used to build the transactions.
 *
 * A real application gets them from a provider. This example runs offline, so it uses fixed fee
 * parameters and takes the Plutus V4 cost model of the network from the environment. Adjust the fee
 * parameters if the network uses different values.
 *
 * \param[in] v4_cost_model The Plutus V4 cost model of the network.
 *
 * \return The protocol parameters. The caller is responsible for releasing them.
 */
static cardano_protocol_parameters_t*
create_protocol_parameters(cardano_cost_model_t* v4_cost_model)
{
  cardano_protocol_parameters_t* params          = NULL;
  cardano_ex_unit_prices_t*      ex_unit_prices  = NULL;
  cardano_unit_interval_t*       memory_prices   = NULL;
  cardano_unit_interval_t*       steps_prices    = NULL;
  cardano_unit_interval_t*       script_ref_cost = NULL;
  cardano_ex_units_t*            max_ex_units    = NULL;
  cardano_protocol_version_t*    version         = NULL;
  cardano_costmdls_t*            cost_models     = NULL;

  exit_on_error(cardano_protocol_parameters_new(&params), "Failed to create protocol parameters");
  exit_on_error(cardano_unit_interval_from_double(0.0577, &memory_prices), "Failed to create memory prices");
  exit_on_error(cardano_unit_interval_from_double(0.0000721, &steps_prices), "Failed to create steps prices");
  exit_on_error(cardano_unit_interval_from_double(15.0, &script_ref_cost), "Failed to create reference script cost");
  exit_on_error(cardano_ex_unit_prices_new(memory_prices, steps_prices, &ex_unit_prices), "Failed to create execution unit prices");
  exit_on_error(cardano_ex_units_new(14000000U, 10000000000U, &max_ex_units), "Failed to create the maximum execution units");
  exit_on_error(cardano_protocol_version_new(DIJKSTRA_PROTOCOL_MAJOR, 0U, &version), "Failed to create the protocol version");
  exit_on_error(cardano_costmdls_new(&cost_models), "Failed to create the cost models");
  exit_on_error(cardano_costmdls_insert(cost_models, v4_cost_model), "Failed to add the Plutus V4 cost model");

  exit_on_error(cardano_protocol_parameters_set_min_fee_a(params, 44U), "Failed to set min fee A");
  exit_on_error(cardano_protocol_parameters_set_min_fee_b(params, 155381U), "Failed to set min fee B");
  exit_on_error(cardano_protocol_parameters_set_ada_per_utxo_byte(params, 4310U), "Failed to set ADA per UTXO byte");
  exit_on_error(cardano_protocol_parameters_set_execution_costs(params, ex_unit_prices), "Failed to set execution costs");
  exit_on_error(cardano_protocol_parameters_set_ref_script_cost_per_byte(params, script_ref_cost), "Failed to set reference script cost");
  exit_on_error(cardano_protocol_parameters_set_max_tx_ex_units(params, max_ex_units), "Failed to set the maximum execution units");
  exit_on_error(cardano_protocol_parameters_set_collateral_percentage(params, 150U), "Failed to set the collateral percentage");
  exit_on_error(cardano_protocol_parameters_set_max_collateral_inputs(params, 3U), "Failed to set the maximum collateral inputs");
  exit_on_error(cardano_protocol_parameters_set_protocol_version(params, version), "Failed to set the protocol version");
  exit_on_error(cardano_protocol_parameters_set_cost_models(params, cost_models), "Failed to set the cost models");

  cardano_costmdls_unref(&cost_models);
  cardano_protocol_version_unref(&version);
  cardano_ex_units_unref(&max_ex_units);
  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_ex_unit_prices_unref(&ex_unit_prices);

  return params;
}

/**
 * \brief Creates a Plutus V4 script from the hex of its CBOR wrapped flat program.
 *
 * \param[in] hex The script bytes in hex.
 *
 * \return The script. The caller is responsible for releasing it.
 */
static cardano_script_t*
create_plutus_v4_script(const char* hex)
{
  cardano_plutus_v4_script_t* plutus_script = NULL;
  cardano_script_t*           script        = NULL;

  exit_on_error(cardano_plutus_v4_script_new_bytes_from_hex(hex, cardano_utils_safe_strlen(hex, 128), &plutus_script), "Failed to create the Plutus V4 script");
  exit_on_error(cardano_script_new_plutus_v4(plutus_script, &script), "Failed to wrap the Plutus V4 script");

  cardano_plutus_v4_script_unref(&plutus_script);

  return script;
}

/**
 * \brief Creates the script hash credential of a script.
 *
 * \param[in] script The script.
 *
 * \return The credential. The caller is responsible for releasing it.
 */
static cardano_credential_t*
create_script_credential(cardano_script_t* script)
{
  cardano_blake2b_hash_t* hash       = cardano_script_get_hash(script);
  cardano_credential_t*   credential = NULL;

  exit_on_error(cardano_credential_new(hash, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, &credential), "Failed to create the script credential");

  cardano_blake2b_hash_unref(&hash);

  return credential;
}

/**
 * \brief Creates the enterprise address controlled by a private key.
 *
 * \param[in] private_key The private key.
 *
 * \return The address. The caller is responsible for releasing it.
 */
static cardano_address_t*
create_key_address(cardano_ed25519_private_key_t* private_key)
{
  cardano_ed25519_public_key_t* public_key = NULL;
  cardano_blake2b_hash_t*       key_hash   = NULL;
  cardano_credential_t*         credential = NULL;
  cardano_enterprise_address_t* enterprise = NULL;

  exit_on_error(cardano_ed25519_private_key_get_public_key(private_key, &public_key), "Failed to get the public key");
  exit_on_error(cardano_ed25519_public_key_to_hash(public_key, &key_hash), "Failed to hash the public key");
  exit_on_error(cardano_credential_new(key_hash, CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential), "Failed to create the key credential");
  exit_on_error(cardano_enterprise_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, credential, &enterprise), "Failed to create the address");

  cardano_address_t* address = cardano_enterprise_address_to_address(enterprise);

  cardano_enterprise_address_unref(&enterprise);
  cardano_credential_unref(&credential);
  cardano_blake2b_hash_unref(&key_hash);
  cardano_ed25519_public_key_unref(&public_key);

  return address;
}

/**
 * \brief Signs a transaction or sub transaction id with a private key.
 *
 * \param[in] private_key The private key.
 * \param[in] id The id to sign.
 *
 * \return A vkey witness set with the signature. The caller is responsible for releasing it.
 */
static cardano_vkey_witness_set_t*
sign_id(cardano_ed25519_private_key_t* private_key, cardano_blake2b_hash_t* id)
{
  cardano_ed25519_public_key_t* public_key = NULL;
  cardano_ed25519_signature_t*  signature  = NULL;
  cardano_vkey_witness_t*       witness    = NULL;
  cardano_vkey_witness_set_t*   witnesses  = NULL;

  exit_on_error(cardano_ed25519_private_key_get_public_key(private_key, &public_key), "Failed to get the public key");
  exit_on_error(cardano_ed25519_private_key_sign(private_key, cardano_blake2b_hash_get_data(id), cardano_blake2b_hash_get_bytes_size(id), &signature), "Failed to sign");
  exit_on_error(cardano_vkey_witness_new(public_key, signature, &witness), "Failed to create the vkey witness");
  exit_on_error(cardano_vkey_witness_set_new(&witnesses), "Failed to create the vkey witness set");
  exit_on_error(cardano_vkey_witness_set_add(witnesses, witness), "Failed to add the vkey witness");

  cardano_vkey_witness_unref(&witness);
  cardano_ed25519_signature_unref(&signature);
  cardano_ed25519_public_key_unref(&public_key);

  return witnesses;
}

/**
 * \brief Creates the UTXO that funds the example from the command line arguments.
 *
 * \param[in] out_ref The output reference of the UTXO, as `<transaction id>#<index>`.
 * \param[in] lovelace The lovelace held by the UTXO, as text.
 * \param[in] address The address that holds the UTXO.
 *
 * \return The UTXO. The caller is responsible for releasing it.
 */
static cardano_utxo_t*
create_funding_utxo(const char* out_ref, const char* lovelace, cardano_address_t* address)
{
  const char* separator = strchr(out_ref, '#');

  if ((separator == NULL) || ((size_t)(separator - out_ref) != TX_ID_HEX_SIZE))
  {
    console_error("The funding UTXO must be given as <transaction id>#<index>.");

    exit(EXIT_FAILURE);
  }

  cardano_blake2b_hash_t*       tx_id  = NULL;
  cardano_transaction_output_t* output = NULL;
  const uint64_t                index  = strtoull(separator + 1, NULL, 10);
  const uint64_t                amount = strtoull(lovelace, NULL, 10);

  exit_on_error(cardano_blake2b_hash_from_hex(out_ref, TX_ID_HEX_SIZE, &tx_id), "Failed to parse the funding transaction id");
  exit_on_error(cardano_transaction_output_new(address, amount, &output), "Failed to create the funding output");

  cardano_utxo_t* utxo = create_utxo(tx_id, (uint32_t)index, output);

  cardano_transaction_output_unref(&output);
  cardano_blake2b_hash_unref(&tx_id);

  return utxo;
}

/**
 * \brief Builds a transaction and exits with the builder error when it fails.
 *
 * \param[in] tx_builder The transaction builder.
 *
 * \return The transaction. The caller is responsible for releasing it.
 */
static cardano_transaction_t*
build_transaction(cardano_tx_builder_t* tx_builder)
{
  cardano_transaction_t* transaction = NULL;
  const cardano_error_t  result      = cardano_tx_builder_build(tx_builder, &transaction);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to build transaction");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_tx_builder_get_last_error(tx_builder));

    exit(result);
  }

  return transaction;
}

/**
 * \brief Builds a sub transaction and exits with the builder error when it fails.
 *
 * \param[in] sub_tx_builder The sub transaction builder.
 *
 * \return The sub transaction. The caller is responsible for releasing it.
 */
static cardano_sub_transaction_t*
build_sub_transaction(cardano_sub_tx_builder_t* sub_tx_builder)
{
  cardano_sub_transaction_t* sub_transaction = NULL;
  const cardano_error_t      result          = cardano_sub_tx_builder_build(sub_tx_builder, &sub_transaction);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to build the sub transaction");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_sub_tx_builder_get_last_error(sub_tx_builder));

    exit(result);
  }

  return sub_transaction;
}

/**
 * \brief Prints a transaction as a CBOR hex string.
 *
 * \param[in] name The name of the transaction.
 * \param[in] transaction The transaction to print.
 */
static void
print_transaction_cbor(const char* name, cardano_transaction_t* transaction)
{
  cardano_cbor_writer_t*  writer = cardano_cbor_writer_new();
  cardano_blake2b_hash_t* id     = cardano_transaction_get_id(transaction);

  exit_on_error(cardano_transaction_to_cbor(transaction, writer), "Failed to serialize transaction");

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  if (hex == NULL)
  {
    console_error("Failed to allocate memory for the transaction CBOR");

    exit(EXIT_FAILURE);
  }

  exit_on_error(cardano_cbor_writer_encode_hex(writer, hex, hex_size), "Failed to encode transaction");

  print_hash(name, id);
  console_write_line("%s", hex);

  free(hex);
  cardano_blake2b_hash_unref(&id);
  cardano_cbor_writer_unref(&writer);
}

/**
 * \brief Prints the execution units of the redeemers of a witness set.
 *
 * \param[in] level The transaction level the witness set belongs to, for display purposes.
 * \param[in] witness_set The witness set.
 */
static void
print_redeemers(const char* level, cardano_witness_set_t* witness_set)
{
  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness_set);

  for (size_t i = 0U; i < cardano_redeemer_list_get_length(redeemers); ++i)
  {
    cardano_redeemer_t* redeemer = NULL;

    exit_on_error(cardano_redeemer_list_get(redeemers, i, &redeemer), "Failed to read a redeemer");

    cardano_ex_units_t* units = cardano_redeemer_get_ex_units(redeemer);

    console_info(
      "%s redeemer (tag %d, index %llu): %llu memory, %llu cpu steps",
      level,
      (int)cardano_redeemer_get_tag(redeemer),
      (unsigned long long)cardano_redeemer_get_index(redeemer),
      (unsigned long long)cardano_ex_units_get_memory(units),
      (unsigned long long)cardano_ex_units_get_cpu_steps(units));

    cardano_ex_units_unref(&units);
    cardano_redeemer_unref(&redeemer);
  }

  cardano_redeemer_list_unref(&redeemers);
}

/* MAIN **********************************************************************/

/**
 * \brief Entry point of the program.
 *
 * \param[in] argc The number of command line arguments.
 * \param[in] argv The command line arguments: the funding UTXO as `<transaction id>#<index>` and the
 *                 lovelace it holds.
 *
 * \return Returns `0` on successful execution, or a non-zero value if there is an error.
 */
int
main(const int argc, char** argv)
{
  console_info("Plutus V4 Guarded Sub Transaction Example");
  console_info("libcardano-c:  V-%s\n", cardano_get_lib_version());

  console_info("This example spends from a Plutus V4 validator in a transaction that carries a sub transaction");
  console_info("guarded by a Plutus V4 script (protocol version 12). It builds two transactions offline and");
  console_info("prints them, it does not submit anything: submit the first one, then the second one.\n");

  const char* private_key_hex = getenv(PRIVATE_KEY_ENV_VAR);
  const char* cost_model_text = getenv(V4_COST_MODEL_ENV_VAR);

  if ((argc != 3) || (private_key_hex == NULL) || (cost_model_text == NULL))
  {
    console_error("Usage: %s <funding transaction id>#<index> <funding lovelace>", argv[0]);
    console_error("The %s environment variable must hold the hex of the Ed25519 private key that owns the funding UTXO.", PRIVATE_KEY_ENV_VAR);
    console_error("The %s environment variable must hold the Plutus V4 cost model of the network.", V4_COST_MODEL_ENV_VAR);

    return EXIT_FAILURE;
  }

  // 0.- Initialize dependencies
  cardano_ed25519_private_key_t* private_key = NULL;

  exit_on_error(cardano_ed25519_private_key_from_normal_hex(private_key_hex, cardano_utils_safe_strlen(private_key_hex, 128), &private_key), "Failed to read the private key");

  cardano_cost_model_t*          v4_cost_model   = parse_v4_cost_model(cost_model_text);
  cardano_protocol_parameters_t* protocol_params = create_protocol_parameters(v4_cost_model);
  cardano_address_t*             payment_address = create_key_address(private_key);
  cardano_utxo_t*                funding_utxo    = create_funding_utxo(argv[1], argv[2], payment_address);
  cardano_script_t*              validator       = create_plutus_v4_script(ALWAYS_SUCCEEDS_VALIDATOR_V4);
  cardano_script_t*              guard_script    = create_plutus_v4_script(ALWAYS_SUCCEEDS_GUARD_V4);
  cardano_credential_t*          guard           = create_script_credential(guard_script);
  cardano_address_t*             script_address  = get_script_address(validator);
  cardano_datum_t*               datum           = create_void_datum();
  cardano_plutus_data_t*         redeemer        = create_void_plutus_data();
  cardano_utxo_list_t*           funding_utxos   = NULL;

  exit_on_error(cardano_utxo_list_new(&funding_utxos), "Failed to create the UTXO list");
  exit_on_error(cardano_utxo_list_add(funding_utxos, funding_utxo), "Failed to add the funding UTXO");

  console_info("Payment address: %s", cardano_address_get_string(payment_address));
  console_info("Validator address: %s\n", cardano_address_get_string(script_address));

  // 1.- The first transaction locks lovelace at the validator (output 0) and creates the UTXO the sub
  // transaction spends (output 1). Every output from index 2 on is change.
  cardano_tx_builder_t* lock_builder = cardano_tx_builder_new(protocol_params, &CARDANO_PREPROD_SLOT_CONFIG);

  cardano_tx_builder_set_utxos(lock_builder, funding_utxos);
  cardano_tx_builder_set_change_address(lock_builder, payment_address);
  cardano_tx_builder_lock_lovelace(lock_builder, script_address, LOVELACE_TO_LOCK, datum);
  cardano_tx_builder_send_lovelace(lock_builder, payment_address, LOVELACE_FOR_SUB_TX);

  cardano_transaction_t*      lock_tx        = build_transaction(lock_builder);
  cardano_blake2b_hash_t*     lock_tx_id     = cardano_transaction_get_id(lock_tx);
  cardano_vkey_witness_set_t* lock_witnesses = sign_id(private_key, lock_tx_id);

  exit_on_error(cardano_transaction_apply_vkey_witnesses(lock_tx, lock_witnesses), "Failed to apply the vkey witnesses");

  cardano_transaction_body_t*        lock_tx_body    = cardano_transaction_get_body(lock_tx);
  cardano_transaction_output_list_t* lock_tx_outputs = cardano_transaction_body_get_outputs(lock_tx_body);
  const size_t                       output_count    = cardano_transaction_output_list_get_length(lock_tx_outputs);
  cardano_utxo_t*                    script_utxo     = get_utxo_at_index(lock_tx, 0U);
  cardano_utxo_t*                    sub_tx_utxo     = get_utxo_at_index(lock_tx, 1U);
  cardano_utxo_list_t*               sub_tx_utxos    = NULL;
  cardano_utxo_list_t*               change_utxos    = NULL;

  exit_on_error(cardano_utxo_list_new(&sub_tx_utxos), "Failed to create the UTXO list");
  exit_on_error(cardano_utxo_list_add(sub_tx_utxos, sub_tx_utxo), "Failed to add the sub transaction UTXO");
  exit_on_error(cardano_utxo_list_new(&change_utxos), "Failed to create the UTXO list");

  for (size_t i = 2U; i < output_count; ++i)
  {
    cardano_utxo_t* change_utxo = get_utxo_at_index(lock_tx, (uint32_t)i);

    exit_on_error(cardano_utxo_list_add(change_utxos, change_utxo), "Failed to add a change UTXO");

    cardano_utxo_unref(&change_utxo);
  }

  // 2.- The sub transaction spends output 1 and pays it back. It is guarded by the V4 guard script,
  // satisfied by a guarding redeemer. The builder evaluates the guard natively, so the sub transaction
  // carries its final execution units and script data hash before it is signed.
  cardano_sub_tx_builder_t* sub_builder = cardano_sub_tx_builder_new(protocol_params, &CARDANO_PREPROD_SLOT_CONFIG);

  cardano_sub_tx_builder_add_input(sub_builder, sub_tx_utxo);
  cardano_sub_tx_builder_send_lovelace(sub_builder, payment_address, LOVELACE_FOR_SUB_TX);
  cardano_sub_tx_builder_add_script(sub_builder, guard_script);
  cardano_sub_tx_builder_add_guard_with_redeemer(sub_builder, guard, redeemer);

  cardano_sub_transaction_t*  sub_tx           = build_sub_transaction(sub_builder);
  cardano_blake2b_hash_t*     sub_tx_id        = cardano_sub_transaction_get_id(sub_tx);
  cardano_vkey_witness_set_t* sub_tx_witnesses = sign_id(private_key, sub_tx_id);

  exit_on_error(cardano_sub_transaction_apply_vkey_witnesses(sub_tx, sub_tx_witnesses), "Failed to apply the sub transaction vkey witnesses");

  // 3.- The second transaction spends the UTXO locked at the validator with a V4 spend redeemer and
  // carries the sub transaction. The builder evaluates the validator together with the guard of the
  // sub transaction, as the ledger does, and pays the fee and collateral from the change of step 1.
  cardano_tx_builder_t* spend_builder = cardano_tx_builder_new(protocol_params, &CARDANO_PREPROD_SLOT_CONFIG);

  cardano_tx_builder_set_utxos(spend_builder, change_utxos);
  cardano_tx_builder_set_collateral_utxos(spend_builder, change_utxos);
  cardano_tx_builder_set_collateral_change_address(spend_builder, payment_address);
  cardano_tx_builder_set_change_address(spend_builder, payment_address);
  cardano_tx_builder_add_script(spend_builder, validator);
  cardano_tx_builder_add_input(spend_builder, script_utxo, redeemer, NULL);
  cardano_tx_builder_add_sub_transaction(spend_builder, sub_tx, sub_tx_utxos);

  cardano_transaction_t*      spend_tx        = build_transaction(spend_builder);
  cardano_blake2b_hash_t*     spend_tx_id     = cardano_transaction_get_id(spend_tx);
  cardano_vkey_witness_set_t* spend_witnesses = sign_id(private_key, spend_tx_id);

  exit_on_error(cardano_transaction_apply_vkey_witnesses(spend_tx, spend_witnesses), "Failed to apply the vkey witnesses");

  cardano_witness_set_t* spend_witness_set  = cardano_transaction_get_witness_set(spend_tx);
  cardano_witness_set_t* sub_tx_witness_set = cardano_sub_transaction_get_witness_set(sub_tx);

  print_redeemers("Top level", spend_witness_set);
  print_redeemers("Sub transaction", sub_tx_witness_set);

  console_write_line("");
  print_transaction_cbor("Lock transaction id", lock_tx);
  console_write_line("");
  print_transaction_cbor("Spend transaction id", spend_tx);

  // Cleanup
  cardano_witness_set_unref(&sub_tx_witness_set);
  cardano_witness_set_unref(&spend_witness_set);
  cardano_vkey_witness_set_unref(&spend_witnesses);
  cardano_blake2b_hash_unref(&spend_tx_id);
  cardano_transaction_unref(&spend_tx);
  cardano_tx_builder_unref(&spend_builder);
  cardano_vkey_witness_set_unref(&sub_tx_witnesses);
  cardano_blake2b_hash_unref(&sub_tx_id);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&sub_builder);
  cardano_utxo_list_unref(&change_utxos);
  cardano_utxo_list_unref(&sub_tx_utxos);
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&script_utxo);
  cardano_transaction_output_list_unref(&lock_tx_outputs);
  cardano_transaction_body_unref(&lock_tx_body);
  cardano_vkey_witness_set_unref(&lock_witnesses);
  cardano_blake2b_hash_unref(&lock_tx_id);
  cardano_transaction_unref(&lock_tx);
  cardano_tx_builder_unref(&lock_builder);
  cardano_utxo_list_unref(&funding_utxos);
  cardano_plutus_data_unref(&redeemer);
  cardano_datum_unref(&datum);
  cardano_address_unref(&script_address);
  cardano_credential_unref(&guard);
  cardano_script_unref(&guard_script);
  cardano_script_unref(&validator);
  cardano_utxo_unref(&funding_utxo);
  cardano_address_unref(&payment_address);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_cost_model_unref(&v4_cost_model);
  cardano_ed25519_private_key_unref(&private_key);

  return EXIT_SUCCESS;
}
