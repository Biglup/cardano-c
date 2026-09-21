/**
 * \file golden_transactions.cpp
 *
 * \author angel.castillo
 * \date   Sep 22, 2026
 *
 * \section LICENSE
 *
 * Copyright 2026 Biglup Labs
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

#include <cardano/address/address.h>
#include <cardano/address/enterprise_address.h>
#include <cardano/address/reward_address.h>
#include <cardano/assets/asset_name.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/credential.h>
#include <cardano/common/guard_set.h>
#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/crypto/ed25519_private_key.h>
#include <cardano/crypto/ed25519_public_key.h>
#include <cardano/crypto/ed25519_signature.h>
#include <cardano/error.h>
#include <cardano/key_handlers/secure_key_handler.h>
#include <cardano/key_handlers/software_secure_key_handler.h>
#include <cardano/protocol_params/protocol_parameters.h>
#include <cardano/scripts/native_scripts/native_script.h>
#include <cardano/scripts/native_scripts/script_pubkey.h>
#include <cardano/scripts/script.h>
#include <cardano/transaction/sub_transaction.h>
#include <cardano/transaction/transaction.h>
#include <cardano/transaction_body/account_balance_interval.h>
#include <cardano/transaction_body/account_balance_intervals_map.h>
#include <cardano/transaction_body/direct_deposit_map.h>
#include <cardano/transaction_body/required_guards_map.h>
#include <cardano/transaction_body/sub_transaction_body.h>
#include <cardano/transaction_body/sub_transaction_set.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/transaction_body/transaction_input.h>
#include <cardano/transaction_body/transaction_input_set.h>
#include <cardano/transaction_body/transaction_output.h>
#include <cardano/transaction_body/transaction_output_list.h>
#include <cardano/transaction_body/value.h>
#include <cardano/transaction_builder/balancing/transaction_balancing.h>
#include <cardano/transaction_builder/coin_selection/coin_selector.h>
#include <cardano/transaction_builder/coin_selection/random_improve_coin_selector.h>
#include <cardano/transaction_builder/fee.h>
#include <cardano/transaction_builder/sub_transaction_builder.h>
#include <cardano/transaction_builder/transaction_builder.h>
#include <cardano/witness_set/vkey_witness.h>
#include <cardano/witness_set/vkey_witness_set.h>
#include <cardano/witness_set/witness_set.h>

#include <gmock/gmock.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

/* CONSTANTS *****************************************************************/

/**
 * \brief Signing key seeds of the payer, the counterparty and the batcher. They are the public RFC 8032 test vectors,
 * known to everyone, and must never hold funds.
 */
static const char* PAYER_KEY_HEX        = "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60";
static const char* COUNTERPARTY_KEY_HEX = "4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb";
static const char* BATCHER_KEY_HEX      = "c5aa8df43f9f837bedb7442f31dcb7b166d38535076f094b85ce3a2e0b4458f7";

/**
 * \brief Passphrase the software key handlers of these tests encrypt their key with.
 */
static const char* PASSWORD = "password";

/**
 * \brief Seed of the random improve coin selector. Every transaction of this file is built with a selector created
 * from this seed, so the inputs it picks never change between runs.
 */
static const uint64_t SELECTOR_SEED = 0x60D1DE5EU;

/**
 * \brief Slot after which every transaction of this file is invalid. A fixed slot keeps the wall clock out of the
 * transaction body.
 */
static const uint64_t INVALID_AFTER_SLOT = 150000000U;

/**
 * \brief The most a fee may exceed the minimum fee of the signed transaction, in bytes of fee.
 */
static const int64_t MAX_FEE_EXCESS_IN_BYTES = 3;

/**
 * \brief Asset traded in the batch: a policy id followed by an asset name, in hex.
 */
static const char* TRADED_ASSET_ID = "1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c960154534c41";

/**
 * \brief Name of the token minted under the native script.
 */
static const char* MINTED_ASSET_NAME = "GOLD";

/**
 * \brief The most UTXOs a golden transaction spends from its top level body.
 */
static const size_t MAX_GOLDEN_INPUTS = 4U;

/* STRUCTURES ****************************************************************/

/**
 * \brief A fully built and signed transaction pinned by its bytes, its id, its fee, the size of the reference scripts
 * it carries and the UTXOs it spends.
 */
struct golden_t
{
    const char* cbor;
    const char* id;
    uint64_t    fee;
    size_t      reference_script_size;
    size_t      output_count;
    size_t      input_count;
    uint64_t    inputs[MAX_GOLDEN_INPUTS];
};

/**
 * \brief A participant of a golden transaction: the key handler it signs with, the address that key controls and the
 * UTXOs it owns.
 */
struct wallet_t
{
    cardano_secure_key_handler_t* key_handler;
    cardano_ed25519_public_key_t* public_key;
    cardano_blake2b_hash_t*       key_hash;
    cardano_credential_t*         credential;
    cardano_address_t*            address;
    cardano_utxo_list_t*          utxos;
};

/* GOLDEN VECTORS ************************************************************/

/**
 * \brief A plain payment: the payer, funded with UTXOs of 5, 30 and 80 ADA, pays 20 ADA to the counterparty and takes
 * the change back to its own address. It pins the selection of the seeded random improve selector, the change output,
 * the fee of a transaction with a single witness and the encoding of the signed transaction.
 * Built by the pinsAPlainPayment test.
 */
static const golden_t PLAIN_PAYMENT = {
  "84a400d90102828258200000000000000000000000000000000000000000000000000000000000000001008258200000000000000000000000000000000000000000000000000000000000000003000182a200581d60977efb35ab621d39dbeb7274ec7795a34708ff4d25a01a1df04c1f27011a01312d00a200581d6035dedd2982a03cf39e7dce03c839994ffdec2ec6b04f1cf2d40e61a3011a03dd432b021a00028f15031a08f0d180a100d9010281825820d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a58408031982b625220891d0d86583a3fd88338593f663675a6aa610b4c48d0341bb55830af591418c39a2ee03e195b714a1dbfb3060865d3b51e70f37e993e141005f5f6",
  "e05aed45c9a4a71c11e4a329bb888a5064192bdb21bb20d79b01631dea5804d2",
  167701U,
  0U,
  2U,
  2U,
  { 1U, 3U, 0U, 0U }
};

/**
 * \brief A mint under a native script supplied by reference: the payer, funded with UTXOs of 5 and 30 ADA, mints one
 * GOLD token under the script that requires its own signature, which a reference input carries, and takes the token
 * back with the change. There are no redeemers, so it pins the pricing of the reference script by the fee, the mint
 * field and the multi asset change output.
 * Built by the pinsAMintUnderANativeScriptSuppliedByReference test.
 */
static const golden_t MINT_BY_REFERENCE = {
  "84a600d90102818258200000000000000000000000000000000000000000000000000000000000000002000181a200581d6035dedd2982a03cf39e7dce03c839994ffdec2ec6b04f1cf2d40e61a301821a01c72adda1581cb5c02fe2b3cd5339561bb9b9fbb2b88295bbd1848116d53d99ccfe0ca144474f4c4401021a000298a3031a08f0d18009a1581cb5c02fe2b3cd5339561bb9b9fbb2b88295bbd1848116d53d99ccfe0ca144474f4c440112d9010281825820000000000000000000000000000000000000000000000000000000000000000a00a100d9010281825820d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a58408e22c815dc8d9583c9c78d02b411b4e1b0faa3ca32d6e729c20538d472ebe6bfcd7a4d7e7ebbe78c60573d0bd29b18cbb69abf5fac3f54b382eaf90a43b05b01f5f6",
  "c8c846426f6127633fdbefc1e86111b1068c78c37f1259507ebb53b49efde0f1",
  170147U,
  34U,
  1U,
  1U,
  { 2U, 0U, 0U, 0U }
};

/**
 * \brief A batch of two sub transactions: the payer offers 50 ADA for 1000 tokens and the counterparty offers 1000
 * tokens for 50 ADA, both pinned to the guard of the batcher, which funds the fee out of its own 20 ADA UTXO. It pins
 * the encoding of the signed sub transactions inside the signed batch, the guard set and the fee of the batch.
 * Built by the pinsABatchOfTwoIntentsGuardedByTheBatcher test.
 */
static const golden_t BATCH_SETTLEMENT = {
  "84a600d90102818258200000000000000000000000000000000000000000000000000000000000000003000181a200581d607f8a76c0ebaa4ad20dfdcd51a5de070ab771f4bf377f2c41e6b71c0a011a012e4777021a0002e589031a08f0d1800ed9010281581c7f8a76c0ebaa4ad20dfdcd51a5de070ab771f4bf377f2c41e6b71c0a17d901028283a300d90102818258200000000000000000000000000000000000000000000000000000000000000001000181a200581d6035dedd2982a03cf39e7dce03c839994ffdec2ec6b04f1cf2d40e61a301821a02faf080a1581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14454534c411903e81818a18200581c7f8a76c0ebaa4ad20dfdcd51a5de070ab771f4bf377f2c41e6b71c0af6a100d9010281825820d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a584034699a97301c3964bd2e723e92e800d7d4d6656dd58923980b1712ebb2b9aaacf707dda4025b121206412a321eb3cfd9dd86c8c98cd69f3835c6b0baf6e4a907f683a300d90102818258200000000000000000000000000000000000000000000000000000000000000002000181a200581d60977efb35ab621d39dbeb7274ec7795a34708ff4d25a01a1df04c1f2701821a03938700a1581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14454534c41190fa01818a18200581c7f8a76c0ebaa4ad20dfdcd51a5de070ab771f4bf377f2c41e6b71c0af6a100d90102818258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58402228d8903f8476384e7278f3cc5b9ee5ec3a325bda004c6570529a0401d7f245f08f26f281c6e3b890ec2a61edf310a2c11dc68c178624841804ae3045c4ae0df6a100d9010281825820fc51cd8e6218a1a38da47ed00230f0580816ed13ba3303ac5deb9115489080255840b0e03c483a2fee5d72a8ed2d357353f32e7d5e60a047d32b6ff3e04b320f3e13eb131d363aae0cc22034b2e12190ea1086ef327d61d65097faeb7fbfc7910a03f5f6",
  "d1a56706bf1843ab60f852f7ae464cf6ce17976b826146ec8fdbbd81380d6313",
  189833U,
  0U,
  1U,
  1U,
  { 3U, 0U, 0U, 0U }
};

/**
 * \brief A direct deposit with an account balance interval: the payer, funded with UTXOs of 5 and 30 ADA, deposits
 * 3 ADA straight into its reward account and requires that account to hold at least 1 ADA, taking the rest back as
 * change. It pins the direct deposit map, the account balance intervals map and the deposit as a term of the balance.
 * Built by the pinsADirectDepositWithAnAccountBalanceInterval test.
 */
static const golden_t DIRECT_DEPOSIT = {
  "84a600d90102818258200000000000000000000000000000000000000000000000000000000000000002000181a200581d6035dedd2982a03cf39e7dce03c839994ffdec2ec6b04f1cf2d40e61a3011a01996ccf021a00028ff1031a08f0d1801819a1581de035dedd2982a03cf39e7dce03c839994ffdec2ec6b04f1cf2d40e61a31a002dc6c0181aa1581de035dedd2982a03cf39e7dce03c839994ffdec2ec6b04f1cf2d40e61a3821a000f4240f6a100d9010281825820d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a5840204032bfe5d4f713bc798657a7fb30ae71291512311b0b83d53ebadbe371e523da579f326345693df26eca81cf17708101a2793bd942d0d9c41d3563a9883902f5f6",
  "fd057e48533c502e138b45e04fac4818c500a56aa19fae6c47c7e785fe2d3f13",
  167921U,
  0U,
  1U,
  1U,
  { 2U, 0U, 0U, 0U }
};

/* STATIC FUNCTIONS **********************************************************/

/**
 * Gives the key handlers the passphrase they encrypt their key with.
 * \param buffer the buffer where the passphrase is written.
 * \param buffer_len the size of the buffer.
 * \return The length of the passphrase, or -1 if the buffer is too small.
 */
static int32_t
get_passphrase(byte_t* buffer, const size_t buffer_len)
{
  if (buffer_len < strlen(PASSWORD))
  {
    return -1;
  }

  (void)memcpy(buffer, PASSWORD, strlen(PASSWORD));

  return (int32_t)strlen(PASSWORD);
}

/**
 * Creates the protocol parameters that price the transactions of these tests.
 * \return A new instance of the protocol parameters.
 */
static cardano_protocol_parameters_t*
new_protocol_parameters()
{
  cardano_protocol_parameters_t* params          = NULL;
  cardano_ex_unit_prices_t*      ex_unit_prices  = NULL;
  cardano_unit_interval_t*       memory_prices   = NULL;
  cardano_unit_interval_t*       steps_prices    = NULL;
  cardano_unit_interval_t*       script_ref_cost = NULL;

  EXPECT_EQ(cardano_protocol_parameters_new(&params), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_from_double(0.0577, &memory_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_from_double(0.0000721, &steps_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_from_double(15.0, &script_ref_cost), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ex_unit_prices_new(memory_prices, steps_prices, &ex_unit_prices), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_protocol_parameters_set_min_fee_a(params, 44U), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_min_fee_b(params, 155381U), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_execution_costs(params, ex_unit_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_ref_script_cost_per_byte(params, script_ref_cost), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_ada_per_utxo_byte(params, 4310U), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_key_deposit(params, 2000000U), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_pool_deposit(params, 500000000U), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_drep_deposit(params, 500000000U), CARDANO_SUCCESS);

  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_ex_unit_prices_unref(&ex_unit_prices);

  return params;
}

/**
 * Creates the seeded coin selector that makes the input selection of these tests deterministic.
 * \return A new instance of the coin selector.
 */
static cardano_coin_selector_t*
new_seeded_selector()
{
  cardano_coin_selector_t* selector = NULL;

  EXPECT_EQ(cardano_random_improve_coin_selector_new_with_seed(SELECTOR_SEED, &selector), CARDANO_SUCCESS);

  return selector;
}

/**
 * Creates a transaction builder with the fixed terms shared by every golden transaction: the seeded selector, the
 * fixed expiry slot and the change address of a wallet, whose UTXOs fund the transaction.
 * \param params the protocol parameters.
 * \param wallet the wallet that funds the transaction and receives the change.
 * \return A new instance of the transaction builder.
 */
static cardano_tx_builder_t*
new_golden_builder(cardano_protocol_parameters_t* params, const wallet_t& wallet)
{
  cardano_tx_builder_t*    builder  = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_coin_selector_t* selector = new_seeded_selector();

  cardano_tx_builder_set_coin_selector(builder, selector);
  cardano_tx_builder_set_change_address(builder, wallet.address);
  cardano_tx_builder_set_utxos(builder, wallet.utxos);
  cardano_tx_builder_set_invalid_after(builder, INVALID_AFTER_SLOT);

  cardano_coin_selector_unref(&selector);

  return builder;
}

/**
 * Creates the id of the UTXO with the given ordinal.
 * \param ordinal the number that makes the transaction id of the UTXO unique.
 * \return A new instance of the hash.
 */
static cardano_blake2b_hash_t*
new_utxo_id(const uint64_t ordinal)
{
  char                    hex[65] = { 0 };
  cardano_blake2b_hash_t* id      = NULL;

  EXPECT_EQ(snprintf(hex, sizeof(hex), "%064llx", (unsigned long long)ordinal), 64);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(hex, 64, &id), CARDANO_SUCCESS);

  return id;
}

/**
 * Creates a UTXO owned by an address.
 * \param ordinal a number that makes the transaction id of the UTXO unique.
 * \param address the address that owns the UTXO.
 * \param coin the lovelace held by the UTXO.
 * \param tokens the quantity of the traded asset held by the UTXO, or 0 if it holds none.
 * \param script the reference script carried by the UTXO, or NULL if it carries none.
 * \return A new instance of the UTXO.
 */
static cardano_utxo_t*
new_utxo(const uint64_t ordinal, cardano_address_t* address, const uint64_t coin, const int64_t tokens, cardano_script_t* script)
{
  cardano_blake2b_hash_t*       id     = new_utxo_id(ordinal);
  cardano_transaction_input_t*  input  = NULL;
  cardano_transaction_output_t* output = NULL;
  cardano_utxo_t*               utxo   = NULL;
  cardano_value_t*              value  = cardano_value_new_from_coin((int64_t)coin);

  if (tokens != 0)
  {
    EXPECT_EQ(cardano_value_add_asset_with_id_ex(value, TRADED_ASSET_ID, strlen(TRADED_ASSET_ID), tokens), CARDANO_SUCCESS);
  }

  EXPECT_EQ(cardano_transaction_input_new(id, 0, &input), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_new(address, 0, &output), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_set_value(output, value), CARDANO_SUCCESS);

  if (script != NULL)
  {
    EXPECT_EQ(cardano_transaction_output_set_script_ref(output, script), CARDANO_SUCCESS);
  }

  EXPECT_EQ(cardano_utxo_new(input, output, &utxo), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&id);
  cardano_transaction_input_unref(&input);
  cardano_transaction_output_unref(&output);
  cardano_value_unref(&value);

  return utxo;
}

/**
 * Creates a wallet from its private key. The key is handed to a software key handler, which is the only holder of it
 * from then on, and the wallet controls the enterprise address of its key hash and owns no UTXOs.
 * \param private_key_hex the Ed25519 private key of the wallet.
 * \return The new wallet. The caller must release it with \ref free_wallet.
 */
static wallet_t
new_wallet(const char* private_key_hex)
{
  wallet_t wallet = { NULL, NULL, NULL, NULL, NULL, NULL };

  cardano_ed25519_private_key_t* private_key        = NULL;
  cardano_enterprise_address_t*  enterprise_address = NULL;

  EXPECT_EQ(cardano_ed25519_private_key_from_normal_hex(private_key_hex, strlen(private_key_hex), &private_key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_software_secure_key_handler_ed25519_new(private_key, (const byte_t*)&PASSWORD[0], strlen(PASSWORD), &get_passphrase, &wallet.key_handler), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_secure_key_handler_ed25519_get_public_key(wallet.key_handler, &wallet.public_key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ed25519_public_key_to_hash(wallet.public_key, &wallet.key_hash), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_credential_new(wallet.key_hash, CARDANO_CREDENTIAL_TYPE_KEY_HASH, &wallet.credential), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_enterprise_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, wallet.credential, &enterprise_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_new(&wallet.utxos), CARDANO_SUCCESS);

  wallet.address = cardano_enterprise_address_to_address(enterprise_address);

  cardano_ed25519_private_key_unref(&private_key);
  cardano_enterprise_address_unref(&enterprise_address);

  return wallet;
}

/**
 * Gives a wallet a new UTXO.
 * \param wallet the wallet that receives the UTXO.
 * \param ordinal a number that makes the transaction id of the UTXO unique.
 * \param coin the lovelace held by the UTXO.
 * \param tokens the quantity of the traded asset held by the UTXO, or 0 if it holds none.
 */
static void
fund_wallet(wallet_t& wallet, const uint64_t ordinal, const uint64_t coin, const int64_t tokens)
{
  cardano_utxo_t* utxo = new_utxo(ordinal, wallet.address, coin, tokens, NULL);

  EXPECT_EQ(cardano_utxo_list_add(wallet.utxos, utxo), CARDANO_SUCCESS);

  cardano_utxo_unref(&utxo);
}

/**
 * Releases everything a wallet holds.
 * \param wallet the wallet to release.
 */
static void
free_wallet(wallet_t& wallet)
{
  cardano_secure_key_handler_unref(&wallet.key_handler);
  cardano_ed25519_public_key_unref(&wallet.public_key);
  cardano_blake2b_hash_unref(&wallet.key_hash);
  cardano_credential_unref(&wallet.credential);
  cardano_address_unref(&wallet.address);
  cardano_utxo_list_unref(&wallet.utxos);
}

/**
 * Creates the native script that requires the signature of a wallet, as a script reference carries it.
 * \param wallet the wallet the native script requires.
 * \return A new instance of the script.
 */
static cardano_script_t*
new_native_reference_script(const wallet_t& wallet)
{
  cardano_script_pubkey_t* script_pubkey = NULL;
  cardano_native_script_t* native_script = NULL;
  cardano_script_t*        script        = NULL;

  EXPECT_EQ(cardano_script_pubkey_new(wallet.key_hash, &script_pubkey), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_native_script_new_pubkey(script_pubkey, &native_script), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_script_new_native(native_script, &script), CARDANO_SUCCESS);

  cardano_script_pubkey_unref(&script_pubkey);
  cardano_native_script_unref(&native_script);

  return script;
}

/**
 * Adds every UTXO of a list to another list, which is how the UTXOs that resolve every input of a transaction are
 * gathered.
 * \param target the list that receives the UTXOs.
 * \param source the list whose UTXOs are added.
 */
static void
add_utxos(cardano_utxo_list_t* target, cardano_utxo_list_t* source)
{
  for (size_t i = 0U; i < cardano_utxo_list_get_length(source); ++i)
  {
    cardano_utxo_t* utxo = NULL;

    EXPECT_EQ(cardano_utxo_list_get(source, i, &utxo), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_utxo_list_add(target, utxo), CARDANO_SUCCESS);

    cardano_utxo_unref(&utxo);
  }
}

/**
 * Signs a transaction through the key handler of a wallet and attaches the witness to it.
 * \param wallet the wallet that signs.
 * \param tx the transaction to sign.
 */
static void
sign_transaction(const wallet_t& wallet, cardano_transaction_t* tx)
{
  cardano_vkey_witness_set_t* witnesses = NULL;

  EXPECT_EQ(cardano_secure_key_handler_ed25519_sign_transaction(wallet.key_handler, tx, &witnesses), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_apply_vkey_witnesses(tx, witnesses), CARDANO_SUCCESS);

  cardano_vkey_witness_set_unref(&witnesses);
}

/**
 * Builds the intent of a wallet as a sub transaction and signs it through the key handler of the wallet. The wallet
 * spends all its UTXOs, pays a value back to itself and only lets a transaction guarded by the batcher carry the
 * intent. Whatever the wallet does not pay back to itself is what it offers, and whatever it pays back above what it
 * spends is what it asks for.
 * \param params the protocol parameters.
 * \param wallet the wallet that expresses the intent.
 * \param batcher the batcher the intent is pinned to.
 * \param kept_coin the lovelace the wallet pays back to itself.
 * \param kept_tokens the quantity of the traded asset the wallet pays back to itself.
 * \return A new instance of the signed sub transaction.
 */
static cardano_sub_transaction_t*
build_signed_intent(
  cardano_protocol_parameters_t* params,
  const wallet_t&                wallet,
  const wallet_t&                batcher,
  const uint64_t                 kept_coin,
  const int64_t                  kept_tokens)
{
  cardano_sub_tx_builder_t*   builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_value_t*            value           = cardano_value_new_from_coin((int64_t)kept_coin);
  cardano_sub_transaction_t*  sub_transaction = NULL;
  cardano_vkey_witness_set_t* witnesses       = NULL;

  EXPECT_EQ(cardano_value_add_asset_with_id_ex(value, TRADED_ASSET_ID, strlen(TRADED_ASSET_ID), kept_tokens), CARDANO_SUCCESS);

  for (size_t i = 0U; i < cardano_utxo_list_get_length(wallet.utxos); ++i)
  {
    cardano_utxo_t* utxo = NULL;

    EXPECT_EQ(cardano_utxo_list_get(wallet.utxos, i, &utxo), CARDANO_SUCCESS);

    cardano_sub_tx_builder_add_input(builder, utxo);
    cardano_utxo_unref(&utxo);
  }

  cardano_sub_tx_builder_send_value(builder, wallet.address, value);
  cardano_sub_tx_builder_require_top_level_guard(builder, batcher.credential, NULL);

  EXPECT_EQ(cardano_sub_tx_builder_build(builder, &sub_transaction), CARDANO_SUCCESS) << cardano_sub_tx_builder_get_last_error(builder);
  EXPECT_EQ(cardano_secure_key_handler_ed25519_sign_sub_transaction(wallet.key_handler, sub_transaction, &witnesses), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_apply_vkey_witnesses(sub_transaction, witnesses), CARDANO_SUCCESS);

  cardano_vkey_witness_set_unref(&witnesses);
  cardano_value_unref(&value);
  cardano_sub_tx_builder_unref(&builder);

  return sub_transaction;
}

/**
 * Encodes a transaction to a CBOR hex string.
 * \param tx the transaction to encode.
 * \return The CBOR hex string.
 */
static std::string
encode_transaction(cardano_transaction_t* tx)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  EXPECT_EQ(cardano_transaction_to_cbor(tx, writer), CARDANO_SUCCESS);

  std::string hex(cardano_cbor_writer_get_hex_size(writer), '\0');

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, &hex[0], hex.size()), CARDANO_SUCCESS);

  hex.resize(hex.size() - 1U);

  cardano_cbor_writer_unref(&writer);

  return hex;
}

/**
 * Decodes a transaction from a CBOR hex string.
 * \param hex the CBOR hex string.
 * \return A new instance of the transaction.
 */
static cardano_transaction_t*
decode_transaction(const std::string& hex)
{
  cardano_transaction_t* tx     = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(hex.c_str(), hex.size());

  EXPECT_EQ(cardano_transaction_from_cbor(reader, &tx), CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return tx;
}

/**
 * Renders the id of a transaction as hex.
 * \param tx the transaction.
 * \return The hex string of the id.
 */
static std::string
get_transaction_id(cardano_transaction_t* tx)
{
  cardano_blake2b_hash_t* id = cardano_transaction_get_id(tx);

  std::string hex(cardano_blake2b_hash_get_hex_size(id), '\0');

  EXPECT_EQ(cardano_blake2b_hash_to_hex(id, &hex[0], hex.size()), CARDANO_SUCCESS);

  hex.resize(hex.size() - 1U);

  cardano_blake2b_hash_unref(&id);

  return hex;
}

/**
 * Gets a borrowed reference to the body of a transaction.
 * \param tx the transaction.
 * \return The body of the transaction.
 */
static cardano_transaction_body_t*
get_body(cardano_transaction_t* tx)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  return body;
}

/**
 * Checks whether one of the witnesses of a set is a valid signature of an id by the key of a wallet.
 * \param witnesses the vkey witnesses.
 * \param wallet the wallet that is expected to have signed.
 * \param id the id the wallet is expected to have signed.
 * \return true if the wallet signed the id.
 */
static bool
is_signature_of(cardano_vkey_witness_set_t* witnesses, const wallet_t& wallet, cardano_blake2b_hash_t* id)
{
  bool is_signed = false;

  for (size_t i = 0U; i < cardano_vkey_witness_set_get_length(witnesses); ++i)
  {
    cardano_vkey_witness_t* witness = NULL;

    EXPECT_EQ(cardano_vkey_witness_set_get(witnesses, i, &witness), CARDANO_SUCCESS);
    cardano_vkey_witness_unref(&witness);

    if (!cardano_vkey_witness_has_public_key(witness, wallet.public_key))
    {
      continue;
    }

    cardano_ed25519_signature_t* signature = cardano_vkey_witness_get_signature(witness);

    is_signed = is_signed || cardano_ed25519_public_verify(wallet.public_key, signature, cardano_blake2b_hash_get_data(id), cardano_blake2b_hash_get_bytes_size(id));

    cardano_ed25519_signature_unref(&signature);
  }

  return is_signed;
}

/**
 * Checks whether one of the witnesses of a transaction is a valid signature of its id by the key of a wallet.
 * \param tx the transaction.
 * \param wallet the wallet that is expected to have signed.
 * \return true if the wallet signed the transaction.
 */
static bool
is_signed_by(cardano_transaction_t* tx, const wallet_t& wallet)
{
  cardano_blake2b_hash_t*     id          = cardano_transaction_get_id(tx);
  cardano_witness_set_t*      witness_set = cardano_transaction_get_witness_set(tx);
  cardano_vkey_witness_set_t* witnesses   = cardano_witness_set_get_vkeys(witness_set);

  const bool is_signed = is_signature_of(witnesses, wallet, id);

  cardano_vkey_witness_set_unref(&witnesses);
  cardano_witness_set_unref(&witness_set);
  cardano_blake2b_hash_unref(&id);

  return is_signed;
}

/**
 * Expects the top level body of a transaction to spend exactly the UTXOs with the given ordinals, which is the choice
 * of the seeded selector the golden vector pins.
 * \param tx the transaction.
 * \param golden the golden vector that names the expected UTXOs.
 */
static void
expect_selected_inputs(cardano_transaction_t* tx, const golden_t& golden)
{
  cardano_transaction_input_set_t* inputs = cardano_transaction_body_get_inputs(get_body(tx));
  cardano_transaction_input_set_unref(&inputs);

  ASSERT_EQ(cardano_transaction_input_set_get_length(inputs), golden.input_count);

  for (size_t i = 0U; i < golden.input_count; ++i)
  {
    cardano_blake2b_hash_t* expected_id = new_utxo_id(golden.inputs[i]);
    bool                    is_spent    = false;

    for (size_t j = 0U; j < cardano_transaction_input_set_get_length(inputs); ++j)
    {
      cardano_transaction_input_t* input = NULL;

      EXPECT_EQ(cardano_transaction_input_set_get(inputs, j, &input), CARDANO_SUCCESS);
      cardano_transaction_input_unref(&input);

      cardano_blake2b_hash_t* id = cardano_transaction_input_get_id(input);
      cardano_blake2b_hash_unref(&id);

      is_spent = is_spent || (cardano_blake2b_hash_equals(id, expected_id) && (cardano_transaction_input_get_index(input) == 0U));
    }

    EXPECT_TRUE(is_spent) << "UTXO " << golden.inputs[i] << " is not spent";

    cardano_blake2b_hash_unref(&expected_id);
  }
}

/**
 * Expects the outputs of a transaction to be as many as the golden vector pins and the last of them, the change, to
 * pay the change address.
 * \param tx the transaction.
 * \param change_address the address the change is expected to be paid to.
 * \param golden the golden vector that pins the number of outputs.
 */
static void
expect_change_output(cardano_transaction_t* tx, cardano_address_t* change_address, const golden_t& golden)
{
  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(get_body(tx));
  cardano_transaction_output_list_unref(&outputs);

  ASSERT_EQ(cardano_transaction_output_list_get_length(outputs), golden.output_count);

  cardano_transaction_output_t* change = NULL;

  EXPECT_EQ(cardano_transaction_output_list_get(outputs, golden.output_count - 1U, &change), CARDANO_SUCCESS);
  cardano_transaction_output_unref(&change);

  cardano_address_t* address = cardano_transaction_output_get_address(change);
  cardano_address_unref(&address);

  EXPECT_TRUE(cardano_address_equals(address, change_address));
}

/**
 * Expects the fee of a signed transaction to be the one the golden vector pins and to be the ledger minimum for the
 * signed bytes: the size fee of the signed transaction plus the fee of the reference scripts it carries, exceeded by
 * no more than a few bytes of fee. Both terms are checked against the ledger arithmetic over the pinned quantities:
 * the size fee is the coefficient times the number of pinned bytes plus the constant, and the reference script fee
 * is the cost per reference script byte times the pinned size of the reference scripts.
 * \param params the protocol parameters.
 * \param tx the signed transaction.
 * \param all_utxos the UTXOs that resolve every input and every reference input of the transaction.
 * \param reference_utxos the UTXOs the transaction takes as reference inputs, which price the reference scripts.
 * \param golden the golden vector that pins the fee, the bytes and the size of the reference scripts.
 */
static void
expect_ledger_minimum_fee(
  cardano_protocol_parameters_t* params,
  cardano_transaction_t*         tx,
  cardano_utxo_list_t*           all_utxos,
  cardano_utxo_list_t*           reference_utxos,
  const golden_t&                golden)
{
  cardano_unit_interval_t* script_ref_cost = cardano_protocol_parameters_get_ref_script_cost_per_byte(params);
  cardano_unit_interval_unref(&script_ref_cost);

  const uint64_t min_fee_a       = cardano_protocol_parameters_get_min_fee_a(params);
  const uint64_t min_fee_b       = cardano_protocol_parameters_get_min_fee_b(params);
  const uint64_t fee             = cardano_transaction_body_get_fee(get_body(tx));
  const size_t   signed_size     = strlen(golden.cbor) / 2U;
  uint64_t       signed_min_fee  = 0U;
  uint64_t       signed_size_fee = 0U;
  uint64_t       script_ref_fee  = 0U;

  EXPECT_EQ(cardano_compute_transaction_fee(tx, all_utxos, params, &signed_min_fee), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_compute_min_fee_without_scripts(tx, min_fee_b, min_fee_a, &signed_size_fee), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_compute_script_ref_fee(reference_utxos, script_ref_cost, &script_ref_fee), CARDANO_SUCCESS);

  const uint64_t expected_script_ref_fee = (uint64_t)ceil(cardano_unit_interval_to_double(script_ref_cost) * (double)golden.reference_script_size);
  const int64_t  fee_excess              = (int64_t)fee - (int64_t)signed_min_fee;

  EXPECT_EQ(fee, golden.fee);
  EXPECT_EQ(signed_size_fee, (min_fee_a * signed_size) + min_fee_b);
  EXPECT_EQ(script_ref_fee, expected_script_ref_fee);
  EXPECT_EQ(signed_min_fee, signed_size_fee + script_ref_fee);
  EXPECT_GE(fee_excess, 0);
  EXPECT_LT(fee_excess, MAX_FEE_EXCESS_IN_BYTES * (int64_t)min_fee_a);
}

/**
 * Expects a transaction built and signed by a wallet to be the golden transaction: it encodes to the pinned bytes and
 * has the pinned id, the pinned bytes decode to a transaction that encodes back to the same bytes with and without its
 * cached encoding, and the decoded transaction satisfies the invariants a legitimate re-pin must keep: it is signed by
 * the wallet, balanced against the resolved UTXOs, spends the pinned UTXOs, pays the change to the change address and
 * its fee is the ledger minimum for its signed size.
 * \param params the protocol parameters.
 * \param tx the transaction that was built and signed.
 * \param signer the wallet that signed the transaction.
 * \param all_utxos the UTXOs that resolve every input and every reference input of the transaction.
 * \param reference_utxos the UTXOs the transaction takes as reference inputs.
 * \param golden the golden vector.
 */
static void
expect_golden(
  cardano_protocol_parameters_t* params,
  cardano_transaction_t*         tx,
  const wallet_t&                signer,
  cardano_utxo_list_t*           all_utxos,
  cardano_utxo_list_t*           reference_utxos,
  const golden_t&                golden)
{
  EXPECT_EQ(encode_transaction(tx), golden.cbor);
  EXPECT_EQ(get_transaction_id(tx), golden.id);

  cardano_transaction_t* decoded = decode_transaction(golden.cbor);

  ASSERT_NE(decoded, nullptr);
  EXPECT_EQ(encode_transaction(decoded), golden.cbor);
  EXPECT_EQ(get_transaction_id(decoded), golden.id);

  cardano_transaction_clear_cbor_cache(decoded);

  EXPECT_EQ(encode_transaction(decoded), golden.cbor);
  EXPECT_TRUE(is_signed_by(decoded, signer));

  bool is_balanced = false;

  EXPECT_EQ(cardano_is_transaction_balanced(decoded, all_utxos, params, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  expect_selected_inputs(decoded, golden);
  expect_change_output(decoded, signer.address, golden);
  expect_ledger_minimum_fee(params, decoded, all_utxos, reference_utxos, golden);

  cardano_transaction_unref(&decoded);
}

/**
 * Expects a sub transaction carried by the decoded golden batch to be the signed intent of a wallet: it carries a
 * witness of the wallet that is a valid signature of the id of the sub transaction, and the only top level guard it
 * requires is the credential of the batcher.
 * \param decoded the transaction the pinned bytes decode to.
 * \param index the position of the sub transaction in the batch.
 * \param signer the wallet that expressed and signed the intent.
 * \param batcher the batcher the intent is pinned to.
 */
static void
expect_signed_intent(cardano_transaction_t* decoded, const size_t index, const wallet_t& signer, const wallet_t& batcher)
{
  cardano_sub_transaction_set_t* sub_transactions = cardano_transaction_body_get_sub_transactions(get_body(decoded));
  cardano_sub_transaction_set_unref(&sub_transactions);

  ASSERT_LT(index, cardano_sub_transaction_set_get_length(sub_transactions));

  cardano_sub_transaction_t* sub_transaction = NULL;

  ASSERT_EQ(cardano_sub_transaction_set_get(sub_transactions, index, &sub_transaction), CARDANO_SUCCESS);
  cardano_sub_transaction_unref(&sub_transaction);

  cardano_blake2b_hash_t*     id          = cardano_sub_transaction_get_id(sub_transaction);
  cardano_witness_set_t*      witness_set = cardano_sub_transaction_get_witness_set(sub_transaction);
  cardano_vkey_witness_set_t* witnesses   = cardano_witness_set_get_vkeys(witness_set);

  EXPECT_TRUE(is_signature_of(witnesses, signer, id));

  cardano_vkey_witness_set_unref(&witnesses);
  cardano_witness_set_unref(&witness_set);
  cardano_blake2b_hash_unref(&id);

  cardano_sub_transaction_body_t* sub_body = cardano_sub_transaction_get_body(sub_transaction);
  cardano_sub_transaction_body_unref(&sub_body);

  cardano_required_guards_map_t* required = cardano_sub_transaction_body_get_required_top_level_guards(sub_body);
  cardano_required_guards_map_unref(&required);

  ASSERT_EQ(cardano_required_guards_map_get_length(required), 1U);

  cardano_credential_t* required_guard = NULL;

  ASSERT_EQ(cardano_required_guards_map_get_key_at(required, 0U, &required_guard), CARDANO_SUCCESS);
  cardano_credential_unref(&required_guard);

  EXPECT_TRUE(cardano_credential_equals(required_guard, batcher.credential));
}

/**
 * Expects the guards of the top level body of the decoded golden batch to contain the credential of the batcher, which
 * every sub transaction it carries requires.
 * \param decoded the transaction the pinned bytes decode to.
 * \param batcher the batcher whose credential is expected among the guards.
 */
static void
expect_guarded_by(cardano_transaction_t* decoded, const wallet_t& batcher)
{
  cardano_guard_set_t* guards = cardano_transaction_body_get_guards(get_body(decoded));
  cardano_guard_set_unref(&guards);

  ASSERT_NE(guards, nullptr);

  bool is_present = false;

  for (size_t i = 0U; i < cardano_guard_set_get_length(guards); ++i)
  {
    cardano_credential_t* guard = NULL;

    EXPECT_EQ(cardano_guard_set_get(guards, i, &guard), CARDANO_SUCCESS);
    cardano_credential_unref(&guard);

    is_present = is_present || cardano_credential_equals(guard, batcher.credential);
  }

  EXPECT_TRUE(is_present);
}

/**
 * Expects the decoded golden deposit to deposit a single amount into an account and to require a single balance of
 * that account, bounded below and not above.
 * \param decoded the transaction the pinned bytes decode to.
 * \param account the reward account the deposit is paid into.
 * \param deposit the lovelace expected to be deposited.
 * \param lower_bound the inclusive lower bound the balance of the account is expected to be held to.
 */
static void
expect_direct_deposit(cardano_transaction_t* decoded, cardano_reward_address_t* account, const uint64_t deposit, const uint64_t lower_bound)
{
  cardano_direct_deposit_map_t* deposits = cardano_transaction_body_get_direct_deposits(get_body(decoded));
  cardano_direct_deposit_map_unref(&deposits);

  cardano_account_balance_intervals_map_t* intervals = cardano_transaction_body_get_account_balance_intervals(get_body(decoded));
  cardano_account_balance_intervals_map_unref(&intervals);

  ASSERT_EQ(cardano_direct_deposit_map_get_length(deposits), 1U);
  ASSERT_EQ(cardano_account_balance_intervals_map_get_length(intervals), 1U);

  uint64_t                            deposited = 0U;
  cardano_account_balance_interval_t* interval  = NULL;

  EXPECT_EQ(cardano_direct_deposit_map_get(deposits, account, &deposited), CARDANO_SUCCESS);
  EXPECT_EQ(deposited, deposit);

  ASSERT_EQ(cardano_account_balance_intervals_map_get(intervals, account, &interval), CARDANO_SUCCESS);
  cardano_account_balance_interval_unref(&interval);

  const uint64_t* inclusive_lower_bound = cardano_account_balance_interval_get_inclusive_lower_bound(interval);

  ASSERT_NE(inclusive_lower_bound, nullptr);
  EXPECT_EQ(*inclusive_lower_bound, lower_bound);
  EXPECT_EQ(cardano_account_balance_interval_get_exclusive_upper_bound(interval), nullptr);
  EXPECT_FALSE(cardano_account_balance_interval_is_exact(interval));
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_golden_transactions, pinsAPlainPayment)
{
  // Arrange
  cardano_protocol_parameters_t* params          = new_protocol_parameters();
  wallet_t                       payer           = new_wallet(PAYER_KEY_HEX);
  wallet_t                       counterparty    = new_wallet(COUNTERPARTY_KEY_HEX);
  cardano_utxo_list_t*           reference_utxos = NULL;

  ASSERT_EQ(cardano_utxo_list_new(&reference_utxos), CARDANO_SUCCESS);

  fund_wallet(payer, 1U, 5000000U, 0);
  fund_wallet(payer, 2U, 30000000U, 0);
  fund_wallet(payer, 3U, 80000000U, 0);

  cardano_tx_builder_t* builder = new_golden_builder(params, payer);

  // Act
  cardano_tx_builder_send_lovelace(builder, counterparty.address, 20000000U);

  cardano_transaction_t* tx = NULL;

  ASSERT_EQ(cardano_tx_builder_build(builder, &tx), CARDANO_SUCCESS) << cardano_tx_builder_get_last_error(builder);

  sign_transaction(payer, tx);

  // Assert
  expect_golden(params, tx, payer, payer.utxos, reference_utxos, PLAIN_PAYMENT);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_tx_builder_unref(&builder);
  cardano_utxo_list_unref(&reference_utxos);
  cardano_protocol_parameters_unref(&params);
  free_wallet(payer);
  free_wallet(counterparty);
}

TEST(cardano_golden_transactions, pinsAMintUnderANativeScriptSuppliedByReference)
{
  // Arrange
  cardano_protocol_parameters_t* params          = new_protocol_parameters();
  wallet_t                       payer           = new_wallet(PAYER_KEY_HEX);
  cardano_script_t*              script          = new_native_reference_script(payer);
  cardano_utxo_t*                reference_utxo  = new_utxo(10U, payer.address, 2000000U, 0, script);
  cardano_blake2b_hash_t*        policy_id       = cardano_script_get_hash(script);
  cardano_asset_name_t*          asset_name      = NULL;
  cardano_utxo_list_t*           reference_utxos = NULL;
  cardano_utxo_list_t*           all_utxos       = NULL;

  ASSERT_EQ(cardano_asset_name_from_string(MINTED_ASSET_NAME, strlen(MINTED_ASSET_NAME), &asset_name), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_utxo_list_new(&reference_utxos), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_utxo_list_new(&all_utxos), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_utxo_list_add(reference_utxos, reference_utxo), CARDANO_SUCCESS);

  fund_wallet(payer, 1U, 5000000U, 0);
  fund_wallet(payer, 2U, 30000000U, 0);

  add_utxos(all_utxos, payer.utxos);
  add_utxos(all_utxos, reference_utxos);

  cardano_tx_builder_t* builder = new_golden_builder(params, payer);

  // Act
  cardano_tx_builder_add_reference_input(builder, reference_utxo);
  cardano_tx_builder_mint_token(builder, policy_id, asset_name, 1, NULL);

  cardano_transaction_t* tx = NULL;

  ASSERT_EQ(cardano_tx_builder_build(builder, &tx), CARDANO_SUCCESS) << cardano_tx_builder_get_last_error(builder);

  sign_transaction(payer, tx);

  // Assert
  expect_golden(params, tx, payer, all_utxos, reference_utxos, MINT_BY_REFERENCE);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_tx_builder_unref(&builder);
  cardano_utxo_list_unref(&all_utxos);
  cardano_utxo_list_unref(&reference_utxos);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_utxo_unref(&reference_utxo);
  cardano_script_unref(&script);
  cardano_protocol_parameters_unref(&params);
  free_wallet(payer);
}

TEST(cardano_golden_transactions, pinsABatchOfTwoIntentsGuardedByTheBatcher)
{
  // Arrange
  cardano_protocol_parameters_t* params          = new_protocol_parameters();
  wallet_t                       payer           = new_wallet(PAYER_KEY_HEX);
  wallet_t                       counterparty    = new_wallet(COUNTERPARTY_KEY_HEX);
  wallet_t                       batcher         = new_wallet(BATCHER_KEY_HEX);
  cardano_utxo_list_t*           reference_utxos = NULL;
  cardano_utxo_list_t*           all_utxos       = NULL;

  ASSERT_EQ(cardano_utxo_list_new(&reference_utxos), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_utxo_list_new(&all_utxos), CARDANO_SUCCESS);

  fund_wallet(payer, 1U, 100000000U, 0);
  fund_wallet(counterparty, 2U, 10000000U, 5000);
  fund_wallet(batcher, 3U, 20000000U, 0);

  add_utxos(all_utxos, payer.utxos);
  add_utxos(all_utxos, counterparty.utxos);
  add_utxos(all_utxos, batcher.utxos);

  cardano_sub_transaction_t* payer_intent        = build_signed_intent(params, payer, batcher, 50000000U, 1000);
  cardano_sub_transaction_t* counterparty_intent = build_signed_intent(params, counterparty, batcher, 60000000U, 4000);
  cardano_tx_builder_t*      builder             = new_golden_builder(params, batcher);

  // Act
  cardano_tx_builder_add_sub_transaction(builder, payer_intent, payer.utxos);
  cardano_tx_builder_add_sub_transaction(builder, counterparty_intent, counterparty.utxos);
  cardano_tx_builder_add_guard(builder, batcher.credential);

  cardano_transaction_t* tx = NULL;

  ASSERT_EQ(cardano_tx_builder_build(builder, &tx), CARDANO_SUCCESS) << cardano_tx_builder_get_last_error(builder);

  sign_transaction(batcher, tx);

  // Assert
  expect_golden(params, tx, batcher, all_utxos, reference_utxos, BATCH_SETTLEMENT);

  cardano_transaction_t*         decoded          = decode_transaction(BATCH_SETTLEMENT.cbor);
  cardano_sub_transaction_set_t* sub_transactions = cardano_transaction_body_get_sub_transactions(get_body(decoded));
  cardano_sub_transaction_set_unref(&sub_transactions);

  EXPECT_EQ(cardano_sub_transaction_set_get_length(sub_transactions), 2U);

  expect_signed_intent(decoded, 0U, payer, batcher);
  expect_signed_intent(decoded, 1U, counterparty, batcher);
  expect_guarded_by(decoded, batcher);

  // Cleanup
  cardano_transaction_unref(&decoded);
  cardano_transaction_unref(&tx);
  cardano_tx_builder_unref(&builder);
  cardano_sub_transaction_unref(&payer_intent);
  cardano_sub_transaction_unref(&counterparty_intent);
  cardano_utxo_list_unref(&all_utxos);
  cardano_utxo_list_unref(&reference_utxos);
  cardano_protocol_parameters_unref(&params);
  free_wallet(payer);
  free_wallet(counterparty);
  free_wallet(batcher);
}

TEST(cardano_golden_transactions, pinsADirectDepositWithAnAccountBalanceInterval)
{
  // Arrange
  cardano_protocol_parameters_t*      params          = new_protocol_parameters();
  wallet_t                            payer           = new_wallet(PAYER_KEY_HEX);
  cardano_reward_address_t*           reward_address  = NULL;
  cardano_account_balance_interval_t* interval        = NULL;
  cardano_utxo_list_t*                reference_utxos = NULL;
  const uint64_t                      lower_bound     = 1000000U;

  ASSERT_EQ(cardano_reward_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, payer.credential, &reward_address), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_account_balance_interval_new(&lower_bound, NULL, &interval), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_utxo_list_new(&reference_utxos), CARDANO_SUCCESS);

  fund_wallet(payer, 1U, 5000000U, 0);
  fund_wallet(payer, 2U, 30000000U, 0);

  cardano_tx_builder_t* builder = new_golden_builder(params, payer);

  // Act
  cardano_tx_builder_add_direct_deposit(builder, reward_address, 3000000U);
  cardano_tx_builder_add_account_balance_interval(builder, reward_address, interval);

  cardano_transaction_t* tx = NULL;

  ASSERT_EQ(cardano_tx_builder_build(builder, &tx), CARDANO_SUCCESS) << cardano_tx_builder_get_last_error(builder);

  sign_transaction(payer, tx);

  // Assert
  expect_golden(params, tx, payer, payer.utxos, reference_utxos, DIRECT_DEPOSIT);

  cardano_transaction_t* decoded = decode_transaction(DIRECT_DEPOSIT.cbor);

  expect_direct_deposit(decoded, reward_address, 3000000U, lower_bound);

  // Cleanup
  cardano_transaction_unref(&decoded);
  cardano_transaction_unref(&tx);
  cardano_tx_builder_unref(&builder);
  cardano_utxo_list_unref(&reference_utxos);
  cardano_account_balance_interval_unref(&interval);
  cardano_reward_address_unref(&reward_address);
  cardano_protocol_parameters_unref(&params);
  free_wallet(payer);
}
