/**
 * \file batch_settlement.cpp
 *
 * \author angel.castillo
 * \date   Sep 18, 2026
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
#include <cardano/protocol_params/protocol_parameters.h>
#include <cardano/transaction/sub_transaction.h>
#include <cardano/transaction/transaction.h>
#include <cardano/transaction_body/required_guards_map.h>
#include <cardano/transaction_body/sub_transaction_set.h>
#include <cardano/transaction_body/transaction_input.h>
#include <cardano/transaction_body/transaction_output.h>
#include <cardano/transaction_body/value.h>
#include <cardano/transaction_builder/balancing/transaction_balancing.h>
#include <cardano/transaction_builder/coin_selection/random_improve_coin_selector.h>
#include <cardano/transaction_builder/fee.h>
#include <cardano/transaction_builder/sub_transaction_builder.h>
#include <cardano/transaction_builder/transaction_builder.h>
#include <cardano/witness_set/vkey_witness.h>
#include <cardano/witness_set/vkey_witness_set.h>

#include <gmock/gmock.h>

#include <array>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

/* CONSTANTS *****************************************************************/

/**
 * \brief Signing key seeds of the buyer, the seller and the batcher. They are the public RFC 8032 test vectors, known
 * to everyone, and must never hold funds.
 */
static const char* BUYER_KEY_HEX   = "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60";
static const char* SELLER_KEY_HEX  = "4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb";
static const char* BATCHER_KEY_HEX = "c5aa8df43f9f837bedb7442f31dcb7b166d38535076f094b85ce3a2e0b4458f7";

static const size_t MAX_PARTIES = 4U;

/**
 * \brief Signing key seeds of the parties of the property test. They are also public RFC 8032 test vectors and must
 * never hold funds.
 */
static const char* PARTY_KEYS_HEX[MAX_PARTIES] = {
  "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60",
  "4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb",
  "f5e5767cf153319517630f226876b86c8160cc583bc013744c6bf255f5cc0ee5",
  "833fe62409237b9d62ec77587520911e9a759cec1d19755b7da901b96dca3d42"
};

static const uint64_t PROPERTY_SEED = 0xC0FFEE42U;
static const size_t   ITERATIONS    = 1000U;

/**
 * \brief Upper bound in lovelace of the fee of a generated batch. It is one of the two slack bounds of the failure
 * oracle: a rejected batch is only accepted as honest when the batcher funds plus the net of the intents could not
 * cover MAX_BATCH_FEE plus MAX_CHANGE_COST.
 */
static const int64_t MAX_BATCH_FEE = 1000000;

/**
 * \brief Upper bound in lovelace of the cost of returning the change of a generated batch. It is the other slack bound
 * of the failure oracle.
 */
static const int64_t MAX_CHANGE_COST = 2000000;

static const size_t TOKEN           = 0U;
static const size_t ASSET_POOL_SIZE = 3U;

static const char* ASSET_POOL[ASSET_POOL_SIZE] = {
  "1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c960154534c41",
  "659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba8241",
  "7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc3734d414353"
};

/* STRUCTURES ****************************************************************/

/**
 * \brief Plain integer record of a value: lovelace plus the quantity of every asset of the pool. Expectations are
 * computed from these records with plain integer arithmetic, independently of the library arithmetic under test.
 */
struct amounts_t
{
    int64_t                              coin;
    std::array<int64_t, ASSET_POOL_SIZE> assets;

    amounts_t()
    : coin(0)
    {
      assets.fill(0);
    }

    amounts_t(const int64_t lovelace, const int64_t tokens)
    : coin(lovelace)
    {
      assets.fill(0);
      assets[TOKEN] = tokens;
    }
};

/**
 * \brief A participant of a settlement: the key it signs with, the address that key controls and the UTXOs it owns.
 */
struct party_t
{
    cardano_ed25519_private_key_t* private_key;
    cardano_ed25519_public_key_t*  public_key;
    cardano_credential_t*          credential;
    cardano_address_t*             address;
    cardano_utxo_list_t*           utxos;
};

/**
 * \brief Deterministic, platform-independent RNG (splitmix64).
 */
struct prop_rng_t
{
    uint64_t state;

    explicit prop_rng_t(const uint64_t seed)
    : state(seed)
    {
    }

    uint64_t
    next()
    {
      state += 0x9e3779b97f4a7c15ULL;

      uint64_t z = state;
      z          = (z ^ (z >> 30U)) * 0xbf58476d1ce4e5b9ULL;
      z          = (z ^ (z >> 27U)) * 0x94d049bb133111ebULL;

      return z ^ (z >> 31U);
    }

    int64_t
    range(const int64_t lo, const int64_t hi)
    {
      return lo + (int64_t)(next() % (uint64_t)(hi - lo + 1));
    }
};

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates the protocol parameters that price the batches of these tests.
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
 * Creates a value from its plain integer record.
 * \param amounts the lovelace and the asset quantities of the value.
 * \return A new instance of the value.
 */
static cardano_value_t*
new_value(const amounts_t& amounts)
{
  cardano_value_t* value = cardano_value_new_from_coin(amounts.coin);

  for (size_t i = 0U; i < ASSET_POOL_SIZE; ++i)
  {
    if (amounts.assets[i] != 0)
    {
      EXPECT_EQ(cardano_value_add_asset_with_id_ex(value, ASSET_POOL[i], strlen(ASSET_POOL[i]), amounts.assets[i]), CARDANO_SUCCESS);
    }
  }

  return value;
}

/**
 * Reads a value back into its plain integer record.
 * \param value the value to read.
 * \return The lovelace and the quantity of every asset of the pool held by the value.
 */
static amounts_t
read_value(cardano_value_t* value)
{
  amounts_t amounts;

  amounts.coin = cardano_value_get_coin(value);

  cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(value);
  cardano_multi_asset_unref(&multi_asset);

  if (multi_asset == NULL)
  {
    return amounts;
  }

  for (size_t i = 0U; i < ASSET_POOL_SIZE; ++i)
  {
    cardano_asset_id_t* asset_id = NULL;
    int64_t             quantity = 0;

    EXPECT_EQ(cardano_asset_id_from_hex(ASSET_POOL[i], strlen(ASSET_POOL[i]), &asset_id), CARDANO_SUCCESS);

    if (cardano_multi_asset_get_with_id(multi_asset, asset_id, &quantity) == CARDANO_SUCCESS)
    {
      amounts.assets[i] = quantity;
    }

    cardano_asset_id_unref(&asset_id);
  }

  return amounts;
}

/**
 * Renders a plain integer record, to describe a failing scenario.
 * \param amounts the record to render.
 * \return The rendered record.
 */
static std::string
amounts_to_string(const amounts_t& amounts)
{
  std::ostringstream stream;

  stream << "{coin:" << amounts.coin;

  for (size_t i = 0U; i < ASSET_POOL_SIZE; ++i)
  {
    if (amounts.assets[i] != 0)
    {
      stream << ", asset" << i << ":" << amounts.assets[i];
    }
  }

  stream << "}";

  return stream.str();
}

/**
 * Adds a record to a running total.
 * \param total the running total.
 * \param amounts the record to add.
 */
static void
add_amounts(amounts_t& total, const amounts_t& amounts)
{
  total.coin += amounts.coin;

  for (size_t i = 0U; i < ASSET_POOL_SIZE; ++i)
  {
    total.assets[i] += amounts.assets[i];
  }
}

/**
 * Subtracts a record from a running total.
 * \param total the running total.
 * \param amounts the record to subtract.
 */
static void
subtract_amounts(amounts_t& total, const amounts_t& amounts)
{
  total.coin -= amounts.coin;

  for (size_t i = 0U; i < ASSET_POOL_SIZE; ++i)
  {
    total.assets[i] -= amounts.assets[i];
  }
}

/**
 * Expects a value to hold exactly the given lovelace and asset quantities.
 * \param value the value to check.
 * \param expected the expected lovelace and asset quantities.
 */
static void
expect_amounts(cardano_value_t* value, const amounts_t& expected)
{
  const amounts_t actual = read_value(value);

  EXPECT_EQ(actual.coin, expected.coin);

  for (size_t i = 0U; i < ASSET_POOL_SIZE; ++i)
  {
    EXPECT_EQ(actual.assets[i], expected.assets[i]);
  }
}

/**
 * Creates a UTXO owned by an address.
 * \param ordinal a number that makes the transaction id of the UTXO unique.
 * \param address the address that owns the UTXO.
 * \param amounts the value held by the UTXO.
 * \return A new instance of the UTXO.
 */
static cardano_utxo_t*
new_utxo(const uint64_t ordinal, cardano_address_t* address, const amounts_t& amounts)
{
  char hex[65] = { 0 };

  EXPECT_EQ(snprintf(hex, sizeof(hex), "%064llx", (unsigned long long)ordinal), 64);

  cardano_blake2b_hash_t*       id     = NULL;
  cardano_transaction_input_t*  input  = NULL;
  cardano_transaction_output_t* output = NULL;
  cardano_utxo_t*               utxo   = NULL;
  cardano_value_t*              value  = new_value(amounts);

  EXPECT_EQ(cardano_blake2b_hash_from_hex(hex, 64, &id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_new(id, 0, &input), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_new(address, 0, &output), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_set_value(output, value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_new(input, output, &utxo), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&id);
  cardano_transaction_input_unref(&input);
  cardano_transaction_output_unref(&output);
  cardano_value_unref(&value);

  return utxo;
}

/**
 * Creates a party from its private key. The party controls the enterprise address of its key hash and owns no UTXOs.
 * \param private_key_hex the Ed25519 private key of the party.
 * \return The new party. The caller must release it with \ref free_party.
 */
static party_t
new_party(const char* private_key_hex)
{
  party_t party = { NULL, NULL, NULL, NULL, NULL };

  cardano_blake2b_hash_t*       key_hash           = NULL;
  cardano_enterprise_address_t* enterprise_address = NULL;

  EXPECT_EQ(cardano_ed25519_private_key_from_normal_hex(private_key_hex, strlen(private_key_hex), &party.private_key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ed25519_private_key_get_public_key(party.private_key, &party.public_key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ed25519_public_key_to_hash(party.public_key, &key_hash), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_credential_new(key_hash, CARDANO_CREDENTIAL_TYPE_KEY_HASH, &party.credential), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_enterprise_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, party.credential, &enterprise_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_new(&party.utxos), CARDANO_SUCCESS);

  party.address = cardano_enterprise_address_to_address(enterprise_address);

  cardano_blake2b_hash_unref(&key_hash);
  cardano_enterprise_address_unref(&enterprise_address);

  return party;
}

/**
 * Gives a party a new UTXO.
 * \param party the party that receives the UTXO.
 * \param ordinal a number that makes the transaction id of the UTXO unique.
 * \param amounts the value held by the UTXO.
 */
static void
fund_party(party_t& party, const uint64_t ordinal, const amounts_t& amounts)
{
  cardano_utxo_t* utxo = new_utxo(ordinal, party.address, amounts);

  EXPECT_EQ(cardano_utxo_list_add(party.utxos, utxo), CARDANO_SUCCESS);

  cardano_utxo_unref(&utxo);
}

/**
 * Takes every UTXO away from a party, so it can be funded again.
 * \param party the party to clear.
 */
static void
clear_party_utxos(party_t& party)
{
  cardano_utxo_list_unref(&party.utxos);

  EXPECT_EQ(cardano_utxo_list_new(&party.utxos), CARDANO_SUCCESS);
}

/**
 * Releases everything a party holds.
 * \param party the party to release.
 */
static void
free_party(party_t& party)
{
  cardano_ed25519_private_key_unref(&party.private_key);
  cardano_ed25519_public_key_unref(&party.public_key);
  cardano_credential_unref(&party.credential);
  cardano_address_unref(&party.address);
  cardano_utxo_list_unref(&party.utxos);
}

/**
 * Joins the UTXOs of several parties, which is what resolves every input of a batch.
 * \param parties the parties whose UTXOs are joined.
 * \return A new instance of the UTXO list.
 */
static cardano_utxo_list_t*
join_utxos(const std::vector<const party_t*>& parties)
{
  cardano_utxo_list_t* joined = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&joined), CARDANO_SUCCESS);

  for (const party_t* party: parties)
  {
    for (size_t i = 0U; i < cardano_utxo_list_get_length(party->utxos); ++i)
    {
      cardano_utxo_t* utxo = NULL;

      EXPECT_EQ(cardano_utxo_list_get(party->utxos, i, &utxo), CARDANO_SUCCESS);
      EXPECT_EQ(cardano_utxo_list_add(joined, utxo), CARDANO_SUCCESS);

      cardano_utxo_unref(&utxo);
    }
  }

  return joined;
}

/**
 * Signs a message with the key of a party.
 * \param party the party that signs.
 * \param message the hash to sign, a transaction id or a sub transaction id.
 * \return A new vkey witness set with the witness of the party.
 */
static cardano_vkey_witness_set_t*
sign_with_party(const party_t& party, cardano_blake2b_hash_t* message)
{
  cardano_ed25519_signature_t* signature = NULL;
  cardano_vkey_witness_t*      witness   = NULL;
  cardano_vkey_witness_set_t*  witnesses = NULL;

  EXPECT_EQ(cardano_ed25519_private_key_sign(party.private_key, cardano_blake2b_hash_get_data(message), cardano_blake2b_hash_get_bytes_size(message), &signature), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_vkey_witness_new(party.public_key, signature, &witness), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_vkey_witness_set_new(&witnesses), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_vkey_witness_set_add(witnesses, witness), CARDANO_SUCCESS);

  cardano_ed25519_signature_unref(&signature);
  cardano_vkey_witness_unref(&witness);

  return witnesses;
}

/**
 * Builds and signs the intent of a party as a sub transaction. The party spends all its UTXOs, pays a value back to
 * itself and only lets a transaction guarded by the batcher carry the intent. Whatever the party does not pay back to
 * itself is what it offers, and whatever it pays back above what it spends is what it asks for.
 * \param params the protocol parameters.
 * \param party the party that expresses the intent.
 * \param batcher the batcher the intent is pinned to.
 * \param kept the value the party pays back to itself.
 * \return A new instance of the signed sub transaction.
 */
static cardano_sub_transaction_t*
build_signed_intent(
  cardano_protocol_parameters_t* params,
  const party_t&                 party,
  const party_t&                 batcher,
  const amounts_t&               kept)
{
  cardano_sub_tx_builder_t*  builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_value_t*           value           = new_value(kept);
  cardano_sub_transaction_t* sub_transaction = NULL;

  for (size_t i = 0U; i < cardano_utxo_list_get_length(party.utxos); ++i)
  {
    cardano_utxo_t* utxo = NULL;

    EXPECT_EQ(cardano_utxo_list_get(party.utxos, i, &utxo), CARDANO_SUCCESS);

    cardano_sub_tx_builder_add_input(builder, utxo);
    cardano_utxo_unref(&utxo);
  }

  cardano_sub_tx_builder_send_value(builder, party.address, value);
  cardano_sub_tx_builder_require_top_level_guard(builder, batcher.credential, NULL);

  EXPECT_EQ(cardano_sub_tx_builder_build(builder, &sub_transaction), CARDANO_SUCCESS);

  cardano_blake2b_hash_t*     unsigned_id = cardano_sub_transaction_get_id(sub_transaction);
  cardano_vkey_witness_set_t* witnesses   = sign_with_party(party, unsigned_id);

  EXPECT_EQ(cardano_sub_transaction_apply_vkey_witnesses(sub_transaction, witnesses), CARDANO_SUCCESS);

  cardano_blake2b_hash_t* signed_id = cardano_sub_transaction_get_id(sub_transaction);

  EXPECT_TRUE(cardano_blake2b_hash_equals(unsigned_id, signed_id));

  cardano_blake2b_hash_unref(&unsigned_id);
  cardano_blake2b_hash_unref(&signed_id);
  cardano_vkey_witness_set_unref(&witnesses);
  cardano_value_unref(&value);
  cardano_sub_tx_builder_unref(&builder);

  return sub_transaction;
}

/**
 * Expects the imbalance of a sub transaction, the value it consumes minus the value it produces, to be the given intent.
 * \param params the protocol parameters.
 * \param party the party that owns the UTXOs spent by the sub transaction.
 * \param sub_transaction the sub transaction of the party.
 * \param intent the expected imbalance.
 */
static void
expect_intent(
  cardano_protocol_parameters_t* params,
  const party_t&                 party,
  cardano_sub_transaction_t*     sub_transaction,
  const amounts_t&               intent)
{
  cardano_value_t* imbalance = NULL;

  EXPECT_EQ(cardano_compute_sub_transaction_imbalance(sub_transaction, party.utxos, params, &imbalance), CARDANO_SUCCESS);

  expect_amounts(imbalance, intent);

  cardano_value_unref(&imbalance);
}

/**
 * Encodes a sub transaction to a CBOR hex string.
 * \param sub_transaction the sub transaction to encode.
 * \return The CBOR hex string.
 */
static std::string
encode_sub_transaction(cardano_sub_transaction_t* sub_transaction)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  EXPECT_EQ(cardano_sub_transaction_to_cbor(sub_transaction, writer), CARDANO_SUCCESS);

  std::string hex(cardano_cbor_writer_get_hex_size(writer), '\0');

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, &hex[0], hex.size()), CARDANO_SUCCESS);

  hex.resize(hex.size() - 1U);

  cardano_cbor_writer_unref(&writer);

  return hex;
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
 * Gets a borrowed reference to the sub transactions carried by a transaction.
 * \param tx the transaction.
 * \return The sub transaction set of the transaction body, or NULL when it has none.
 */
static cardano_sub_transaction_set_t*
get_sub_transactions(cardano_transaction_t* tx)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  cardano_sub_transaction_set_t* sub_transactions = cardano_transaction_body_get_sub_transactions(body);
  cardano_sub_transaction_set_unref(&sub_transactions);

  return sub_transactions;
}

/**
 * Checks whether one of the witnesses is a valid signature of the message by the key of the party.
 * \param witnesses the vkey witnesses to search.
 * \param party the party that is expected to have signed.
 * \param message the hash that was signed, a transaction id or a sub transaction id.
 * \return true if the party signed the message.
 */
static bool
is_signed_by(cardano_vkey_witness_set_t* witnesses, const party_t& party, cardano_blake2b_hash_t* message)
{
  bool is_signed = false;

  for (size_t i = 0U; i < cardano_vkey_witness_set_get_length(witnesses); ++i)
  {
    cardano_vkey_witness_t* witness = NULL;

    EXPECT_EQ(cardano_vkey_witness_set_get(witnesses, i, &witness), CARDANO_SUCCESS);
    cardano_vkey_witness_unref(&witness);

    if (!cardano_vkey_witness_has_public_key(witness, party.public_key))
    {
      continue;
    }

    cardano_ed25519_signature_t* signature = cardano_vkey_witness_get_signature(witness);

    is_signed = is_signed || cardano_ed25519_public_verify(party.public_key, signature, cardano_blake2b_hash_get_data(message), cardano_blake2b_hash_get_bytes_size(message));

    cardano_ed25519_signature_unref(&signature);
  }

  return is_signed;
}

/**
 * Expects a transaction to carry a sub transaction exactly as its party signed it: same id, same bytes and a
 * signature of the party that still verifies against the id.
 * \param tx the transaction that carries the sub transaction.
 * \param party the party that signed the sub transaction.
 * \param signed_sub_transaction the sub transaction the party signed.
 * \param signed_cbor the CBOR hex string of the sub transaction at the time it was signed.
 */
static void
expect_carries_as_signed(
  cardano_transaction_t*     tx,
  const party_t&             party,
  cardano_sub_transaction_t* signed_sub_transaction,
  const std::string&         signed_cbor)
{
  cardano_blake2b_hash_t*    signed_id = cardano_sub_transaction_get_id(signed_sub_transaction);
  cardano_sub_transaction_t* carried   = NULL;

  ASSERT_EQ(cardano_sub_transaction_set_find_by_id(get_sub_transactions(tx), signed_id, &carried), CARDANO_SUCCESS);

  cardano_blake2b_hash_t* carried_id  = cardano_sub_transaction_get_id(carried);
  cardano_witness_set_t*  witness_set = cardano_sub_transaction_get_witness_set(carried);

  cardano_vkey_witness_set_t* witnesses = cardano_witness_set_get_vkeys(witness_set);

  EXPECT_TRUE(cardano_blake2b_hash_equals(carried_id, signed_id));
  EXPECT_EQ(encode_sub_transaction(carried), signed_cbor);
  EXPECT_TRUE(is_signed_by(witnesses, party, carried_id));

  cardano_vkey_witness_set_unref(&witnesses);
  cardano_witness_set_unref(&witness_set);
  cardano_blake2b_hash_unref(&carried_id);
  cardano_blake2b_hash_unref(&signed_id);
  cardano_sub_transaction_unref(&carried);
}

/**
 * Expects the guards of a transaction to contain every top level guard that its sub transactions require.
 * \param tx the transaction that carries the sub transactions.
 * \param expected_guard the credential that every sub transaction is expected to require.
 */
static void
expect_required_guards_are_present(cardano_transaction_t* tx, cardano_credential_t* expected_guard)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  cardano_guard_set_t* guards = cardano_transaction_body_get_guards(body);
  cardano_guard_set_unref(&guards);

  cardano_sub_transaction_set_t* sub_transactions = get_sub_transactions(tx);

  for (size_t i = 0U; i < cardano_sub_transaction_set_get_length(sub_transactions); ++i)
  {
    cardano_sub_transaction_t* sub_transaction = NULL;

    EXPECT_EQ(cardano_sub_transaction_set_get(sub_transactions, i, &sub_transaction), CARDANO_SUCCESS);
    cardano_sub_transaction_unref(&sub_transaction);

    cardano_sub_transaction_body_t* sub_body = cardano_sub_transaction_get_body(sub_transaction);
    cardano_sub_transaction_body_unref(&sub_body);

    cardano_required_guards_map_t* required = cardano_sub_transaction_body_get_required_top_level_guards(sub_body);
    cardano_required_guards_map_unref(&required);

    ASSERT_EQ(cardano_required_guards_map_get_length(required), 1U);

    cardano_credential_t* required_guard = NULL;

    EXPECT_EQ(cardano_required_guards_map_get_key_at(required, 0U, &required_guard), CARDANO_SUCCESS);
    cardano_credential_unref(&required_guard);

    EXPECT_TRUE(cardano_credential_equals(required_guard, expected_guard));

    bool is_present = false;

    for (size_t j = 0U; j < cardano_guard_set_get_length(guards); ++j)
    {
      cardano_credential_t* guard = NULL;

      EXPECT_EQ(cardano_guard_set_get(guards, j, &guard), CARDANO_SUCCESS);
      cardano_credential_unref(&guard);

      is_present = is_present || cardano_credential_equals(guard, required_guard);
    }

    EXPECT_TRUE(is_present);
  }
}

/**
 * Adds up what the top level body of a transaction spends from a party and what it pays to the party.
 * \param tx the transaction.
 * \param party the party whose UTXOs and address are looked up.
 * \param spent on return, the value of the UTXOs of the party that the body spends.
 * \param received on return, the value of the outputs of the body that pay to the address of the party.
 * \return true if every input and every output of the body belongs to the party.
 */
static bool
read_top_level_flows(cardano_transaction_t* tx, const party_t& party, amounts_t& spent, amounts_t& received)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  cardano_transaction_input_set_t* inputs = cardano_transaction_body_get_inputs(body);
  cardano_transaction_input_set_unref(&inputs);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  bool is_owned_by_party = true;

  for (size_t i = 0U; i < cardano_transaction_input_set_get_length(inputs); ++i)
  {
    cardano_transaction_input_t* input = NULL;

    EXPECT_EQ(cardano_transaction_input_set_get(inputs, i, &input), CARDANO_SUCCESS);
    cardano_transaction_input_unref(&input);

    bool is_resolved = false;

    for (size_t j = 0U; j < cardano_utxo_list_get_length(party.utxos); ++j)
    {
      cardano_utxo_t* utxo = NULL;

      EXPECT_EQ(cardano_utxo_list_get(party.utxos, j, &utxo), CARDANO_SUCCESS);
      cardano_utxo_unref(&utxo);

      cardano_transaction_input_t* utxo_input = cardano_utxo_get_input(utxo);
      cardano_transaction_input_unref(&utxo_input);

      if (!cardano_transaction_input_equals(input, utxo_input))
      {
        continue;
      }

      cardano_transaction_output_t* utxo_output = cardano_utxo_get_output(utxo);
      cardano_transaction_output_unref(&utxo_output);

      cardano_value_t* value = cardano_transaction_output_get_value(utxo_output);
      cardano_value_unref(&value);

      add_amounts(spent, read_value(value));

      is_resolved = true;
    }

    is_owned_by_party = is_owned_by_party && is_resolved;
  }

  for (size_t i = 0U; i < cardano_transaction_output_list_get_length(outputs); ++i)
  {
    cardano_transaction_output_t* output = NULL;

    EXPECT_EQ(cardano_transaction_output_list_get(outputs, i, &output), CARDANO_SUCCESS);
    cardano_transaction_output_unref(&output);

    cardano_address_t* address = cardano_transaction_output_get_address(output);
    cardano_address_unref(&address);

    cardano_value_t* value = cardano_transaction_output_get_value(output);
    cardano_value_unref(&value);

    add_amounts(received, read_value(value));

    is_owned_by_party = is_owned_by_party && cardano_address_equals(address, party.address);
  }

  return is_owned_by_party;
}

/**
 * Settles the intents of a buyer and a seller the way a batcher does: it adds both signed sub transactions and its
 * own guard to a transaction funded by its UTXOs, builds it and signs it. It then expects the result to be a valid
 * settlement: the batch is balanced, the fee pays for more than the size of the batch before the batcher signs, the
 * sub transactions are carried exactly as they were signed, the batcher guards the transaction as the intents
 * require, the transaction round trips byte exact, and the batcher pays the fee plus the given contribution out of
 * its own UTXOs and nothing else.
 * \param buyer_kept the value the buyer pays back to itself.
 * \param seller_kept the value the seller pays back to itself.
 * \param buyer_intent the imbalance the sub transaction of the buyer is expected to have.
 * \param seller_intent the imbalance the sub transaction of the seller is expected to have.
 * \param batcher_funds the value of the UTXO the batcher owns.
 * \param batcher_contribution the value the batcher is expected to put into the settlement on top of the fee, negative
 *                             amounts are what the batcher takes out of it.
 */
static void
expect_settlement(
  const amounts_t& buyer_kept,
  const amounts_t& seller_kept,
  const amounts_t& buyer_intent,
  const amounts_t& seller_intent,
  const amounts_t& batcher_funds,
  const amounts_t& batcher_contribution)
{
  // Arrange
  cardano_protocol_parameters_t* params  = new_protocol_parameters();
  party_t                        buyer   = new_party(BUYER_KEY_HEX);
  party_t                        seller  = new_party(SELLER_KEY_HEX);
  party_t                        batcher = new_party(BATCHER_KEY_HEX);

  fund_party(buyer, 1U, amounts_t(100000000, 0));
  fund_party(seller, 2U, amounts_t(10000000, 5000));
  fund_party(batcher, 3U, batcher_funds);

  cardano_utxo_list_t*       all_utxos     = join_utxos({ &buyer, &seller, &batcher });
  cardano_sub_transaction_t* buyer_sub_tx  = build_signed_intent(params, buyer, batcher, buyer_kept);
  cardano_sub_transaction_t* seller_sub_tx = build_signed_intent(params, seller, batcher, seller_kept);

  const std::string buyer_cbor  = encode_sub_transaction(buyer_sub_tx);
  const std::string seller_cbor = encode_sub_transaction(seller_sub_tx);

  expect_intent(params, buyer, buyer_sub_tx, buyer_intent);
  expect_intent(params, seller, seller_sub_tx, seller_intent);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, batcher.address);
  cardano_tx_builder_set_utxos(tx_builder, batcher.utxos);

  // Act
  cardano_tx_builder_add_sub_transaction(tx_builder, buyer_sub_tx, buyer.utxos);
  cardano_tx_builder_add_sub_transaction(tx_builder, seller_sub_tx, seller.utxos);
  cardano_tx_builder_add_guard(tx_builder, batcher.credential);

  cardano_transaction_t* tx = NULL;

  ASSERT_EQ(cardano_tx_builder_build(tx_builder, &tx), CARDANO_SUCCESS) << cardano_tx_builder_get_last_error(tx_builder);

  uint64_t unsigned_min_fee = 0U;

  EXPECT_EQ(cardano_compute_transaction_fee(tx, all_utxos, params, &unsigned_min_fee), CARDANO_SUCCESS);

  cardano_blake2b_hash_t*     tx_id             = cardano_transaction_get_id(tx);
  cardano_vkey_witness_set_t* batcher_witnesses = sign_with_party(batcher, tx_id);

  EXPECT_EQ(cardano_transaction_apply_vkey_witnesses(tx, batcher_witnesses), CARDANO_SUCCESS);

  // Assert
  bool             is_balanced     = false;
  cardano_value_t* batch_imbalance = NULL;

  EXPECT_EQ(cardano_is_transaction_balanced(tx, all_utxos, params, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  EXPECT_EQ(cardano_compute_transaction_batch_imbalance(tx, all_utxos, params, &batch_imbalance), CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_value_is_zero(batch_imbalance));

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  const int64_t fee = (int64_t)cardano_transaction_body_get_fee(body);

  EXPECT_GT(fee, (int64_t)unsigned_min_fee);

  EXPECT_EQ(cardano_sub_transaction_set_get_length(get_sub_transactions(tx)), 2U);

  expect_carries_as_signed(tx, buyer, buyer_sub_tx, buyer_cbor);
  expect_carries_as_signed(tx, seller, seller_sub_tx, seller_cbor);
  expect_required_guards_are_present(tx, batcher.credential);

  const std::string      tx_cbor = encode_transaction(tx);
  cardano_transaction_t* decoded = decode_transaction(tx_cbor);

  ASSERT_NE(decoded, nullptr);
  EXPECT_EQ(encode_transaction(decoded), tx_cbor);

  cardano_blake2b_hash_t*     decoded_id          = cardano_transaction_get_id(decoded);
  cardano_witness_set_t*      decoded_witness_set = cardano_transaction_get_witness_set(decoded);
  cardano_vkey_witness_set_t* decoded_witnesses   = cardano_witness_set_get_vkeys(decoded_witness_set);

  EXPECT_TRUE(cardano_blake2b_hash_equals(decoded_id, tx_id));
  EXPECT_TRUE(is_signed_by(decoded_witnesses, batcher, decoded_id));

  expect_carries_as_signed(decoded, buyer, buyer_sub_tx, buyer_cbor);
  expect_carries_as_signed(decoded, seller, seller_sub_tx, seller_cbor);
  expect_required_guards_are_present(decoded, batcher.credential);

  cardano_transaction_clear_cbor_cache(decoded);

  EXPECT_EQ(encode_transaction(decoded), tx_cbor);

  amounts_t spent;
  amounts_t received;

  EXPECT_TRUE(read_top_level_flows(tx, batcher, spent, received));
  EXPECT_EQ(spent.coin - received.coin, fee + batcher_contribution.coin);

  for (size_t i = 0U; i < ASSET_POOL_SIZE; ++i)
  {
    EXPECT_EQ(spent.assets[i] - received.assets[i], batcher_contribution.assets[i]);
  }

  cardano_value_t* top_level_imbalance = NULL;

  EXPECT_EQ(cardano_compute_transaction_imbalance(tx, batcher.utxos, params, &top_level_imbalance), CARDANO_SUCCESS);

  expect_amounts(top_level_imbalance, batcher_contribution);

  // Cleanup
  cardano_value_unref(&top_level_imbalance);
  cardano_vkey_witness_set_unref(&decoded_witnesses);
  cardano_witness_set_unref(&decoded_witness_set);
  cardano_blake2b_hash_unref(&decoded_id);
  cardano_transaction_unref(&decoded);
  cardano_value_unref(&batch_imbalance);
  cardano_vkey_witness_set_unref(&batcher_witnesses);
  cardano_blake2b_hash_unref(&tx_id);
  cardano_transaction_unref(&tx);
  cardano_tx_builder_unref(&tx_builder);
  cardano_sub_transaction_unref(&buyer_sub_tx);
  cardano_sub_transaction_unref(&seller_sub_tx);
  cardano_utxo_list_unref(&all_utxos);
  cardano_protocol_parameters_unref(&params);
  free_party(buyer);
  free_party(seller);
  free_party(batcher);
}

/**
 * Generates the value of a UTXO: some lovelace and up to two assets of the pool.
 * \param rng the random number generator.
 * \param min_coin the minimum lovelace of the UTXO.
 * \param max_coin the maximum lovelace of the UTXO.
 * \return The generated value.
 */
static amounts_t
gen_utxo_amounts(prop_rng_t& rng, const int64_t min_coin, const int64_t max_coin)
{
  amounts_t amounts;

  amounts.coin = rng.range(min_coin, max_coin);

  const size_t asset_count = rng.next() % 3U;

  for (size_t i = 0U; i < asset_count; ++i)
  {
    amounts.assets[rng.next() % ASSET_POOL_SIZE] = rng.range(1, 1000000);
  }

  return amounts;
}

/**
 * Generates what a party pays back to itself out of the value it spends. For lovelace and for every asset of the pool
 * the party either keeps what it has, offers part of it or asks for more than it has.
 * \param rng the random number generator.
 * \param spent the value the party spends.
 * \return The generated value.
 */
static amounts_t
gen_kept_amounts(prop_rng_t& rng, const amounts_t& spent)
{
  amounts_t kept = spent;

  switch (rng.next() % 3U)
  {
    case 0U:
      kept.coin = rng.range(2000000, spent.coin);
      break;
    case 1U:
      kept.coin = spent.coin + rng.range(1, 50000000);
      break;
    default:
      break;
  }

  for (size_t i = 0U; i < ASSET_POOL_SIZE; ++i)
  {
    switch (rng.next() % 6U)
    {
      case 0U:
      case 1U:
        kept.assets[i] = rng.range(0, spent.assets[i]);
        break;
      case 2U:
        kept.assets[i] = spent.assets[i] + rng.range(1, 1000);
        break;
      default:
        break;
    }
  }

  return kept;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_batch_settlement, settlesACoincidenceOfWantsAndTheBatcherOnlyPaysTheFee)
{
  const amounts_t buyer_kept(50000000, 1000);
  const amounts_t seller_kept(60000000, 4000);
  const amounts_t buyer_intent(50000000, -1000);
  const amounts_t seller_intent(-50000000, 1000);
  const amounts_t batcher_funds(20000000, 0);
  const amounts_t batcher_contribution(0, 0);

  expect_settlement(buyer_kept, seller_kept, buyer_intent, seller_intent, batcher_funds, batcher_contribution);
}

TEST(cardano_batch_settlement, settlesIntentsThatFallShortAndTheBatcherFundsTheDifference)
{
  const amounts_t buyer_kept(50000000, 1000);
  const amounts_t seller_kept(62000000, 4100);
  const amounts_t buyer_intent(50000000, -1000);
  const amounts_t seller_intent(-52000000, 900);
  const amounts_t batcher_funds(20000000, 500);
  const amounts_t batcher_contribution(2000000, 100);

  expect_settlement(buyer_kept, seller_kept, buyer_intent, seller_intent, batcher_funds, batcher_contribution);
}

TEST(cardano_batch_settlement, settlesIntentsThatLeaveASurplusAndTheBatcherKeepsItAsChange)
{
  const amounts_t buyer_kept(50000000, 1000);
  const amounts_t seller_kept(58000000, 3900);
  const amounts_t buyer_intent(50000000, -1000);
  const amounts_t seller_intent(-48000000, 1100);
  const amounts_t batcher_funds(20000000, 0);
  const amounts_t batcher_contribution(-2000000, -100);

  expect_settlement(buyer_kept, seller_kept, buyer_intent, seller_intent, batcher_funds, batcher_contribution);
}

TEST(cardano_batch_settlement_properties, randomIntentsBuildIntoABalancedBatchOrFailForLackOfFunds)
{
  cardano_protocol_parameters_t* params   = new_protocol_parameters();
  cardano_coin_selector_t*       selector = NULL;
  party_t                        batcher  = new_party(BATCHER_KEY_HEX);
  std::vector<party_t>           parties;

  ASSERT_EQ(cardano_random_improve_coin_selector_new_with_seed(PROPERTY_SEED, &selector), CARDANO_SUCCESS);

  for (size_t i = 0U; i < MAX_PARTIES; ++i)
  {
    parties.push_back(new_party(PARTY_KEYS_HEX[i]));
  }

  prop_rng_t rng(PROPERTY_SEED);
  uint64_t   utxo_ordinal = 0U;
  size_t     built        = 0U;
  size_t     rejected     = 0U;

  for (size_t iteration = 0U; iteration < ITERATIONS; ++iteration)
  {
    const size_t party_count        = 1U + (rng.next() % MAX_PARTIES);
    const size_t batcher_utxo_count = 1U + (rng.next() % 3U);

    amounts_t          net_intent;
    amounts_t          batcher_funds;
    std::ostringstream scenario;

    scenario << "seed=" << PROPERTY_SEED << " iteration=" << iteration << " intents=[";

    std::vector<const party_t*>             owners;
    std::vector<cardano_sub_transaction_t*> sub_transactions;
    std::vector<std::string>                signed_cbors;

    for (size_t i = 0U; i < party_count; ++i)
    {
      const size_t utxo_count = 1U + (rng.next() % 2U);

      amounts_t spent;

      clear_party_utxos(parties[i]);

      for (size_t j = 0U; j < utxo_count; ++j)
      {
        const amounts_t amounts = gen_utxo_amounts(rng, 5000000, 500000000);

        fund_party(parties[i], ++utxo_ordinal, amounts);
        add_amounts(spent, amounts);
      }

      const amounts_t kept   = gen_kept_amounts(rng, spent);
      amounts_t       intent = spent;

      subtract_amounts(intent, kept);
      add_amounts(net_intent, intent);

      scenario << amounts_to_string(intent);

      cardano_sub_transaction_t* sub_transaction = build_signed_intent(params, parties[i], batcher, kept);

      expect_intent(params, parties[i], sub_transaction, intent);

      owners.push_back(&parties[i]);
      sub_transactions.push_back(sub_transaction);
      signed_cbors.push_back(encode_sub_transaction(sub_transaction));
    }

    clear_party_utxos(batcher);

    for (size_t i = 0U; i < batcher_utxo_count; ++i)
    {
      const amounts_t amounts = gen_utxo_amounts(rng, 1000000, 100000000);

      fund_party(batcher, ++utxo_ordinal, amounts);
      add_amounts(batcher_funds, amounts);
    }

    owners.push_back(&batcher);

    scenario << "] net=" << amounts_to_string(net_intent) << " batcher=" << amounts_to_string(batcher_funds);
    SCOPED_TRACE(scenario.str());

    cardano_utxo_list_t*  all_utxos  = join_utxos(owners);
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    cardano_tx_builder_set_coin_selector(tx_builder, selector);
    cardano_tx_builder_set_change_address(tx_builder, batcher.address);
    cardano_tx_builder_set_utxos(tx_builder, batcher.utxos);
    cardano_tx_builder_add_guard(tx_builder, batcher.credential);

    for (size_t i = 0U; i < party_count; ++i)
    {
      cardano_tx_builder_add_sub_transaction(tx_builder, sub_transactions[i], parties[i].utxos);
    }

    cardano_transaction_t* tx     = NULL;
    const cardano_error_t  result = cardano_tx_builder_build(tx_builder, &tx);

    if (result == CARDANO_SUCCESS)
    {
      ++built;

      bool             is_balanced     = false;
      cardano_value_t* batch_imbalance = NULL;

      EXPECT_EQ(cardano_is_transaction_balanced(tx, all_utxos, params, &is_balanced), CARDANO_SUCCESS);
      EXPECT_TRUE(is_balanced);

      EXPECT_EQ(cardano_compute_transaction_batch_imbalance(tx, all_utxos, params, &batch_imbalance), CARDANO_SUCCESS);
      EXPECT_TRUE(cardano_value_is_zero(batch_imbalance));

      EXPECT_EQ(cardano_sub_transaction_set_get_length(get_sub_transactions(tx)), party_count);

      for (size_t i = 0U; i < party_count; ++i)
      {
        expect_carries_as_signed(tx, parties[i], sub_transactions[i], signed_cbors[i]);
      }

      expect_required_guards_are_present(tx, batcher.credential);

      cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
      cardano_transaction_body_unref(&body);

      amounts_t spent;
      amounts_t received;

      EXPECT_TRUE(read_top_level_flows(tx, batcher, spent, received));
      EXPECT_EQ(spent.coin - received.coin, (int64_t)cardano_transaction_body_get_fee(body) - net_intent.coin);

      for (size_t i = 0U; i < ASSET_POOL_SIZE; ++i)
      {
        EXPECT_EQ(spent.assets[i] - received.assets[i], -net_intent.assets[i]);
      }

      cardano_value_unref(&batch_imbalance);
    }
    else
    {
      ++rejected;

      EXPECT_EQ(result, CARDANO_ERROR_BALANCE_INSUFFICIENT) << cardano_tx_builder_get_last_error(tx_builder);

      amounts_t funds = batcher_funds;

      add_amounts(funds, net_intent);

      bool is_fundable = funds.coin >= (MAX_BATCH_FEE + MAX_CHANGE_COST);

      for (size_t i = 0U; i < ASSET_POOL_SIZE; ++i)
      {
        is_fundable = is_fundable && (funds.assets[i] >= 0);
      }

      EXPECT_FALSE(is_fundable) << "the build failed even though the batcher can fund the batch; funds=" << amounts_to_string(funds);
    }

    for (cardano_sub_transaction_t* sub_transaction: sub_transactions)
    {
      cardano_sub_transaction_unref(&sub_transaction);
    }

    cardano_transaction_unref(&tx);
    cardano_tx_builder_unref(&tx_builder);
    cardano_utxo_list_unref(&all_utxos);

    if (::testing::Test::HasFailure())
    {
      break;
    }
  }

  if (!::testing::Test::HasFailure())
  {
    EXPECT_GT(built, ITERATIONS / 4U);
    EXPECT_GT(rejected, ITERATIONS / 10U);
  }

  for (party_t& party: parties)
  {
    free_party(party);
  }

  free_party(batcher);
  cardano_coin_selector_unref(&selector);
  cardano_protocol_parameters_unref(&params);
}
