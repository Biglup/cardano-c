/**
 * \file sub_transaction_batch_example.c
 *
 * \author angel.castillo
 * \date   Sep 18, 2026
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

#include <stdlib.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

// Well known Ed25519 test keys (RFC 8032). They only exist to make this example reproducible,
// never use them to hold funds.
static const char* BUYER_PRIVATE_KEY   = "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60";
static const char* SELLER_PRIVATE_KEY  = "4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb";
static const char* BATCHER_PRIVATE_KEY = "c5aa8df43f9f837bedb7442f31dcb7b166d38535076f094b85ce3a2e0b4458f7";

// The passphrase that encrypts the keys of the parties inside their secure key handlers.
static const char* PASSPHRASE = "password";

// The UTXOs of this example are hardcoded, so it runs without a provider.
static const char* BUYER_UTXO_TX_ID   = "0000000000000000000000000000000000000000000000000000000000000001";
static const char* SELLER_UTXO_TX_ID  = "0000000000000000000000000000000000000000000000000000000000000002";
static const char* BATCHER_UTXO_TX_ID = "0000000000000000000000000000000000000000000000000000000000000003";

// Policy id followed by the asset name ("TSLA") of the token being traded.
static const char* TOKEN_ASSET_ID = "1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c960154534c41";

static const int64_t BUYER_LOVELACE    = 100000000;
static const int64_t SELLER_LOVELACE   = 10000000;
static const int64_t SELLER_TOKENS     = 5000;
static const int64_t BATCHER_LOVELACE  = 20000000;
static const int64_t PRICE_IN_LOVELACE = 50000000;
static const int64_t TOKENS_TO_TRADE   = 1000;

/* STRUCTS *******************************************************************/

/**
 * \brief A participant of the batch: the secure key handler that holds the key it signs with, the
 * credential and the address that key controls, and the UTXO it owns.
 */
typedef struct party_t
{
    cardano_secure_key_handler_t* key_handler;
    cardano_credential_t*         credential;
    cardano_address_t*            address;
    cardano_utxo_t*               utxo;
    cardano_utxo_list_t*          utxos;
} party_t;

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
 * \brief Retrieves the passphrase that decrypts the key of a party.
 *
 * A real application asks the user for the passphrase, this example hardcodes it so that it can run
 * unattended.
 *
 * \param[out] buffer The buffer where to write the passphrase.
 * \param[in] buffer_len The size of the buffer.
 *
 * \return The length of the passphrase (or -1 on error).
 */
static int32_t
get_passphrase(byte_t* buffer, const size_t buffer_len)
{
  const size_t passphrase_len = cardano_utils_safe_strlen(PASSPHRASE, 128);

  if (buffer_len < passphrase_len)
  {
    return -1;
  }

  cardano_utils_safe_memcpy(buffer, buffer_len, PASSPHRASE, passphrase_len);

  return (int32_t)passphrase_len;
}

/**
 * \brief Creates the protocol parameters used to price the batch.
 *
 * A real application gets them from a provider, this example hardcodes the ones that matter for a
 * transaction without Plutus scripts so that it can run offline.
 *
 * \return The protocol parameters. The caller is responsible for releasing them.
 */
static cardano_protocol_parameters_t*
create_protocol_parameters(void)
{
  cardano_protocol_parameters_t* params          = NULL;
  cardano_ex_unit_prices_t*      ex_unit_prices  = NULL;
  cardano_unit_interval_t*       memory_prices   = NULL;
  cardano_unit_interval_t*       steps_prices    = NULL;
  cardano_unit_interval_t*       script_ref_cost = NULL;

  exit_on_error(cardano_protocol_parameters_new(&params), "Failed to create protocol parameters");
  exit_on_error(cardano_unit_interval_from_double(0.0577, &memory_prices), "Failed to create memory prices");
  exit_on_error(cardano_unit_interval_from_double(0.0000721, &steps_prices), "Failed to create steps prices");
  exit_on_error(cardano_unit_interval_from_double(15.0, &script_ref_cost), "Failed to create reference script cost");
  exit_on_error(cardano_ex_unit_prices_new(memory_prices, steps_prices, &ex_unit_prices), "Failed to create execution unit prices");

  exit_on_error(cardano_protocol_parameters_set_min_fee_a(params, 44U), "Failed to set min fee A");
  exit_on_error(cardano_protocol_parameters_set_min_fee_b(params, 155381U), "Failed to set min fee B");
  exit_on_error(cardano_protocol_parameters_set_ada_per_utxo_byte(params, 4310U), "Failed to set ADA per UTXO byte");
  exit_on_error(cardano_protocol_parameters_set_execution_costs(params, ex_unit_prices), "Failed to set execution costs");
  exit_on_error(cardano_protocol_parameters_set_ref_script_cost_per_byte(params, script_ref_cost), "Failed to set reference script cost");

  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_ex_unit_prices_unref(&ex_unit_prices);

  return params;
}

/**
 * \brief Creates a value with some lovelace and, optionally, some tokens.
 *
 * \param[in] lovelace The lovelace of the value.
 * \param[in] tokens The quantity of the traded token, can be zero.
 *
 * \return The value. The caller is responsible for releasing it.
 */
static cardano_value_t*
create_value(const int64_t lovelace, const int64_t tokens)
{
  cardano_value_t* value = cardano_value_new_from_coin(lovelace);

  if (tokens != 0)
  {
    exit_on_error(cardano_value_add_asset_with_id_ex(value, TOKEN_ASSET_ID, cardano_utils_safe_strlen(TOKEN_ASSET_ID, 128), tokens), "Failed to add tokens to value");
  }

  return value;
}

/**
 * \brief Creates a party from its private key and gives it a single UTXO.
 *
 * The private key is handed to a software secure key handler, which keeps it encrypted and only
 * decrypts it while signing. The party controls the enterprise address derived from its key hash.
 *
 * \param[in] private_key_hex The Ed25519 private key of the party.
 * \param[in] utxo_tx_id_hex The id of the transaction that created the UTXO of the party.
 * \param[in] lovelace The lovelace held by the UTXO.
 * \param[in] tokens The quantity of the traded token held by the UTXO, can be zero.
 *
 * \return The party. The caller is responsible for releasing it with \ref release_party.
 */
static party_t
create_party(const char* private_key_hex, const char* utxo_tx_id_hex, const int64_t lovelace, const int64_t tokens)
{
  party_t party = { 0 };

  cardano_ed25519_private_key_t* private_key        = NULL;
  cardano_ed25519_public_key_t*  public_key         = NULL;
  cardano_blake2b_hash_t*        key_hash           = NULL;
  cardano_enterprise_address_t*  enterprise_address = NULL;
  cardano_blake2b_hash_t*        utxo_tx_id         = NULL;
  cardano_transaction_output_t*  output             = NULL;
  cardano_value_t*               value              = create_value(lovelace, tokens);

  exit_on_error(cardano_ed25519_private_key_from_normal_hex(private_key_hex, cardano_utils_safe_strlen(private_key_hex, 128), &private_key), "Failed to create private key");
  exit_on_error(cardano_software_secure_key_handler_ed25519_new(private_key, (const byte_t*)PASSPHRASE, cardano_utils_safe_strlen(PASSPHRASE, 128), get_passphrase, &party.key_handler), "Failed to create secure key handler");
  exit_on_error(cardano_secure_key_handler_ed25519_get_public_key(party.key_handler, &public_key), "Failed to get public key");

  exit_on_error(cardano_ed25519_public_key_to_hash(public_key, &key_hash), "Failed to hash public key");
  exit_on_error(cardano_credential_new(key_hash, CARDANO_CREDENTIAL_TYPE_KEY_HASH, &party.credential), "Failed to create credential");
  exit_on_error(cardano_enterprise_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, party.credential, &enterprise_address), "Failed to create address");

  party.address = cardano_enterprise_address_to_address(enterprise_address);

  exit_on_error(cardano_blake2b_hash_from_hex(utxo_tx_id_hex, cardano_utils_safe_strlen(utxo_tx_id_hex, 128), &utxo_tx_id), "Failed to create transaction id");
  exit_on_error(cardano_transaction_output_new(party.address, 0U, &output), "Failed to create output");
  exit_on_error(cardano_transaction_output_set_value(output, value), "Failed to set output value");

  party.utxo = create_utxo(utxo_tx_id, 0U, output);

  exit_on_error(cardano_utxo_list_new(&party.utxos), "Failed to create UTXO list");
  exit_on_error(cardano_utxo_list_add(party.utxos, party.utxo), "Failed to add UTXO to list");

  cardano_ed25519_private_key_unref(&private_key);
  cardano_ed25519_public_key_unref(&public_key);
  cardano_blake2b_hash_unref(&key_hash);
  cardano_enterprise_address_unref(&enterprise_address);
  cardano_blake2b_hash_unref(&utxo_tx_id);
  cardano_transaction_output_unref(&output);
  cardano_value_unref(&value);

  return party;
}

/**
 * \brief Releases everything a party holds.
 *
 * \param[in] party The party to release.
 */
static void
release_party(party_t* party)
{
  cardano_secure_key_handler_unref(&party->key_handler);
  cardano_credential_unref(&party->credential);
  cardano_address_unref(&party->address);
  cardano_utxo_unref(&party->utxo);
  cardano_utxo_list_unref(&party->utxos);
}

/**
 * \brief Gets the quantity of the traded token held by a value.
 *
 * \param[in] value The value to read.
 *
 * \return The quantity of the traded token, negative when the value owes tokens.
 */
static int64_t
get_tokens(cardano_value_t* value)
{
  cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(value);
  cardano_asset_id_t*    asset_id    = NULL;
  int64_t                tokens      = 0;

  exit_on_error(cardano_asset_id_from_hex(TOKEN_ASSET_ID, cardano_utils_safe_strlen(TOKEN_ASSET_ID, 128), &asset_id), "Failed to create asset id");

  if (cardano_multi_asset_get_with_id(multi_asset, asset_id, &tokens) != CARDANO_SUCCESS)
  {
    tokens = 0;
  }

  cardano_asset_id_unref(&asset_id);
  cardano_multi_asset_unref(&multi_asset);

  return tokens;
}

/**
 * \brief Builds and signs the sub transaction of a party.
 *
 * A sub transaction is an intent. The party spends its UTXO and pays itself what it wants to end up
 * with, the builder does not balance anything: it selects no inputs, adds no change and pays no fee.
 * The resulting imbalance is the offer of the party. It is only valid inside a transaction guarded by
 * the batcher it trusts, which is expressed with a required top level guard.
 *
 * \param[in] name The name of the party, for display purposes.
 * \param[in] params The protocol parameters.
 * \param[in] party The party that expresses the intent.
 * \param[in] batcher The batcher the intent is pinned to.
 * \param[in] lovelace The lovelace the party pays back to itself.
 * \param[in] tokens The quantity of the traded token the party pays back to itself.
 *
 * \return The signed sub transaction. The caller is responsible for releasing it.
 */
static cardano_sub_transaction_t*
build_signed_sub_transaction(
  const char*                    name,
  cardano_protocol_parameters_t* params,
  const party_t*                 party,
  const party_t*                 batcher,
  const int64_t                  lovelace,
  const int64_t                  tokens)
{
  cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_PREPROD_SLOT_CONFIG);
  cardano_value_t*          value   = create_value(lovelace, tokens);

  cardano_sub_tx_builder_add_input(builder, party->utxo);
  cardano_sub_tx_builder_send_value(builder, party->address, value);
  cardano_sub_tx_builder_require_top_level_guard(builder, batcher->credential, NULL);

  cardano_sub_transaction_t* sub_transaction = NULL;
  cardano_error_t            result          = cardano_sub_tx_builder_build(builder, &sub_transaction);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to build sub transaction");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_sub_tx_builder_get_last_error(builder));

    exit(result);
  }

  // A party signs the id of its sub transaction. The id is the hash of the body, so attaching the
  // witnesses does not change it.
  cardano_blake2b_hash_t*     id        = cardano_sub_transaction_get_id(sub_transaction);
  cardano_vkey_witness_set_t* witnesses = NULL;

  exit_on_error(cardano_secure_key_handler_ed25519_sign_sub_transaction(party->key_handler, sub_transaction, &witnesses), "Failed to sign sub transaction");
  exit_on_error(cardano_sub_transaction_apply_vkey_witnesses(sub_transaction, witnesses), "Failed to apply vkey witnesses");

  // The imbalance is what the sub transaction consumes minus what it produces: a positive amount is
  // offered to the batch and a negative amount is asked from it.
  cardano_value_t* imbalance = NULL;

  exit_on_error(cardano_compute_sub_transaction_imbalance(sub_transaction, party->utxos, params, &imbalance), "Failed to compute imbalance");

  console_info("%s signed its sub transaction, imbalance: %lld lovelace, %lld tokens", name, (long long)cardano_value_get_coin(imbalance), (long long)get_tokens(imbalance));
  print_hash("Sub transaction id", id);

  cardano_value_unref(&imbalance);
  cardano_vkey_witness_set_unref(&witnesses);
  cardano_blake2b_hash_unref(&id);
  cardano_value_unref(&value);
  cardano_sub_tx_builder_unref(&builder);

  return sub_transaction;
}

/**
 * \brief Prints a transaction as a CBOR hex string.
 *
 * \param[in] transaction The transaction to print.
 */
static void
print_transaction_cbor(cardano_transaction_t* transaction)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  exit_on_error(cardano_transaction_to_cbor(transaction, writer), "Failed to serialize transaction");

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  if (hex == NULL)
  {
    console_error("Failed to allocate memory for the transaction CBOR");

    exit(EXIT_FAILURE);
  }

  exit_on_error(cardano_cbor_writer_encode_hex(writer, hex, hex_size), "Failed to encode transaction");

  console_info("Transaction CBOR:");
  console_write_line("%s", hex);

  free(hex);
  cardano_cbor_writer_unref(&writer);
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
  console_info("Sub Transaction Batch Example");
  console_info("libcardano-c:  V-%s\n", cardano_get_lib_version());

  console_info("This example settles a trade between two parties with nested transactions (CIP-118).");
  console_info("The buyer offers %lld lovelace for %lld tokens and the seller offers the opposite.", (long long)PRICE_IN_LOVELACE, (long long)TOKENS_TO_TRADE);
  console_info("Each party signs a sub transaction and a batcher aggregates them, paying the fee of the batch.\n");

  // 0.- Initialize dependencies
  cardano_protocol_parameters_t* protocol_params = create_protocol_parameters();

  party_t buyer   = create_party(BUYER_PRIVATE_KEY, BUYER_UTXO_TX_ID, BUYER_LOVELACE, 0);
  party_t seller  = create_party(SELLER_PRIVATE_KEY, SELLER_UTXO_TX_ID, SELLER_LOVELACE, SELLER_TOKENS);
  party_t batcher = create_party(BATCHER_PRIVATE_KEY, BATCHER_UTXO_TX_ID, BATCHER_LOVELACE, 0);

  // 1.- Each party builds and signs its sub transaction on its own
  console_info("Building sub transactions...");

  cardano_sub_transaction_t* buyer_sub_tx  = build_signed_sub_transaction("Buyer", protocol_params, &buyer, &batcher, BUYER_LOVELACE - PRICE_IN_LOVELACE, TOKENS_TO_TRADE);
  cardano_sub_transaction_t* seller_sub_tx = build_signed_sub_transaction("Seller", protocol_params, &seller, &batcher, SELLER_LOVELACE + PRICE_IN_LOVELACE, SELLER_TOKENS - TOKENS_TO_TRADE);

  // 2.- The batcher aggregates the sub transactions. It must resolve the inputs they spend, adds the
  // guard that both parties require and lets the builder balance the whole batch: a deficit of the sub
  // transactions would be funded from the UTXOs of the batcher and a surplus would end up in its change.
  console_info("Building transaction...");

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(protocol_params, &CARDANO_PREPROD_SLOT_CONFIG);

  cardano_tx_builder_set_utxos(tx_builder, batcher.utxos);
  cardano_tx_builder_set_change_address(tx_builder, batcher.address);
  cardano_tx_builder_add_sub_transaction(tx_builder, buyer_sub_tx, buyer.utxos);
  cardano_tx_builder_add_sub_transaction(tx_builder, seller_sub_tx, seller.utxos);
  cardano_tx_builder_add_guard(tx_builder, batcher.credential);

  cardano_transaction_t* transaction = NULL;
  cardano_error_t        result      = cardano_tx_builder_build(tx_builder, &transaction);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to build transaction");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_tx_builder_get_last_error(tx_builder));

    return result;
  }

  // 3.- The batcher signs the transaction. The sub transactions are carried untouched, so the
  // signatures of the parties remain valid.
  cardano_blake2b_hash_t*     transaction_id    = cardano_transaction_get_id(transaction);
  cardano_vkey_witness_set_t* batcher_witnesses = NULL;

  exit_on_error(cardano_secure_key_handler_ed25519_sign_transaction(batcher.key_handler, transaction, &batcher_witnesses), "Failed to sign transaction");
  exit_on_error(cardano_transaction_apply_vkey_witnesses(transaction, batcher_witnesses), "Failed to apply vkey witnesses");

  // 4.- Verify that the batch conserves value. The check needs the UTXOs that resolve the inputs of
  // the transaction and the inputs of its sub transactions.
  cardano_utxo_list_t* party_utxos = cardano_utxo_list_concat(buyer.utxos, seller.utxos);
  cardano_utxo_list_t* all_utxos   = cardano_utxo_list_concat(party_utxos, batcher.utxos);
  bool                 is_balanced = false;

  exit_on_error(cardano_is_transaction_balanced(transaction, all_utxos, protocol_params, &is_balanced), "Failed to check the balance of the batch");

  cardano_transaction_body_t* body = cardano_transaction_get_body(transaction);

  console_info("The batch is %s, the batcher pays a fee of %llu lovelace", is_balanced ? "balanced" : "NOT balanced", (unsigned long long)cardano_transaction_body_get_fee(body));
  print_hash("Transaction id", transaction_id);
  print_transaction_cbor(transaction);

  // Cleanup
  cardano_transaction_body_unref(&body);
  cardano_utxo_list_unref(&party_utxos);
  cardano_utxo_list_unref(&all_utxos);
  cardano_vkey_witness_set_unref(&batcher_witnesses);
  cardano_blake2b_hash_unref(&transaction_id);
  cardano_transaction_unref(&transaction);
  cardano_tx_builder_unref(&tx_builder);
  cardano_sub_transaction_unref(&buyer_sub_tx);
  cardano_sub_transaction_unref(&seller_sub_tx);
  cardano_protocol_parameters_unref(&protocol_params);
  release_party(&buyer);
  release_party(&seller);
  release_party(&batcher);

  return is_balanced ? EXIT_SUCCESS : EXIT_FAILURE;
}
