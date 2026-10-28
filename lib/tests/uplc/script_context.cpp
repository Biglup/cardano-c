/**
 * \file script_context.cpp
 *
 * \author angel.castillo
 * \date   Jun 19, 2026
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

#include <cardano/address/reward_address.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/credential.h>
#include <cardano/common/guard_set.h>
#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>
#include <cardano/common/withdrawal_map.h>
#include <cardano/error.h>
#include <cardano/plutus_data/constr_plutus_data.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/plutus_data/plutus_map.h>
#include <cardano/slot_config.h>
#include <cardano/transaction/transaction.h>
#include <cardano/transaction_body/account_balance_interval.h>
#include <cardano/transaction_body/account_balance_intervals_map.h>
#include <cardano/transaction_body/direct_deposit_map.h>
#include <cardano/transaction_body/required_guards_map.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/voting_procedures/voter.h>
#include <cardano/voting_procedures/voting_procedures.h>
#include <cardano/witness_set/redeemer.h>
#include <cardano/witness_set/redeemer_list.h>
#include <cardano/witness_set/witness_set.h>

#include "../../src/uplc/tx/script_context.h"
#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <cstring>
#include <gmock/gmock.h>
#include <string>

/* CONSTANTS *****************************************************************/

/**
 * \brief A Conway transaction with a single spend input, one output, a fee, a
 *        single signer and a single spend redeemer.
 *
 * body = { 0: inputs[ (id=00*32, ix=0) ],
 *          1: outputs[ enterprise-key addr, 1000000 ],
 *          2: fee 200000,
 *          0x0e: required signers[ 02*28 ] }
 * witness set = { 5: redeemers[ (Spend, 0, Constr0[], [1000000, 100000000]) ] }
 */
static const char* kTxCbor =
  "84a40081825820000000000000000000000000000000000000000000000000000000000000000000018182"
  "581d61111111111111111111111111111111111111111111111111111111111a000f4240021a00030d400e"
  "81581c02020202020202020202020202020202020202020202020202020202a10581840000d87980821a00"
  "0f42401a05f5e100f5f6";

/**
 * \brief A Conway transaction as kTxCbor but with a validity interval.
 *
 * Adds 3: ttl 4600000 and 8: validity start 4500000, both at or past the slot
 * origin used by kSlotConfig so the slot-to-POSIX conversion succeeds.
 */
static const char* kTxWithIntervalCbor =
  "84a60081825820000000000000000000000000000000000000000000000000000000000000000000018182"
  "581d61111111111111111111111111111111111111111111111111111111111a000f4240021a00030d4003"
  "1a004630c0081a0044aa200e81581c02020202020202020202020202020202020202020202020202020202"
  "a10581840000d87980821a000f42401a05f5e100f5f6";

/**
 * \brief A slot config whose origin precedes the interval slots of kTxWithIntervalCbor.
 */
static const cardano_slot_config_t kSlotConfig = { 1596059091000U, 4492800U, 1000U };

/**
 * \brief A UTxO resolving the spend input to a plain enterprise-key output with
 *        no datum and no script reference (valid for V1 and V2).
 *
 * utxo = [ (id=00*32, ix=0), { 0: enterprise-key addr, 1: 5000000 } ]
 */
static const char* kUtxoCbor =
  "82825820000000000000000000000000000000000000000000000000000000000000000000a200581d6133"
  "333333333333333333333333333333333333333333333333333333011a004c4b40";

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Decodes a hex-encoded CBOR transaction.
 */
static cardano_transaction_t*
decode_tx(const char* hex)
{
  std::string clean;
  for (const char* p = hex; *p != '\0'; ++p)
  {
    if ((*p != ' '))
    {
      clean.push_back(*p);
    }
  }

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(clean.c_str(), clean.size());
  cardano_transaction_t* tx     = NULL;
  cardano_error_t        result = cardano_transaction_from_cbor(reader, &tx);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return tx;
}

/**
 * \brief Builds a one-element UTxO list from a hex-encoded CBOR utxo.
 */
static cardano_utxo_list_t*
decode_utxos(const char* hex)
{
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(hex, strlen(hex));
  cardano_utxo_t*        utxo   = NULL;
  cardano_error_t        result = cardano_utxo_from_cbor(reader, &utxo);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_utxo_list_t* list = NULL;
  EXPECT_EQ(cardano_utxo_list_new(&list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(list, utxo), CARDANO_SUCCESS);

  cardano_utxo_unref(&utxo);
  cardano_cbor_reader_unref(&reader);

  return list;
}

/**
 * \brief Returns the first spend redeemer of a transaction.
 */
static cardano_redeemer_t*
first_redeemer(cardano_transaction_t* tx)
{
  cardano_witness_set_t*   witness   = cardano_transaction_get_witness_set(tx);
  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness);
  cardano_redeemer_t*      redeemer  = NULL;

  EXPECT_EQ(cardano_redeemer_list_get(redeemers, 0, &redeemer), CARDANO_SUCCESS);

  cardano_redeemer_list_unref(&redeemers);
  cardano_witness_set_unref(&witness);

  return redeemer;
}

/**
 * \brief Returns the constructor alternative of a plutus-data value.
 */
static uint64_t
constr_alt(cardano_plutus_data_t* data)
{
  cardano_constr_plutus_data_t* constr = NULL;
  EXPECT_EQ(cardano_plutus_data_to_constr(data, &constr), CARDANO_SUCCESS);

  uint64_t alt = 0U;
  EXPECT_EQ(cardano_constr_plutus_data_get_alternative(constr, &alt), CARDANO_SUCCESS);

  cardano_constr_plutus_data_unref(&constr);

  return alt;
}

/**
 * \brief Returns the field count of a constructor plutus-data value.
 */
static size_t
constr_len(cardano_plutus_data_t* data)
{
  cardano_constr_plutus_data_t* constr = NULL;
  EXPECT_EQ(cardano_plutus_data_to_constr(data, &constr), CARDANO_SUCCESS);

  cardano_plutus_list_t* fields = NULL;
  EXPECT_EQ(cardano_constr_plutus_data_get_data(constr, &fields), CARDANO_SUCCESS);

  const size_t len = cardano_plutus_list_get_length(fields);

  cardano_plutus_list_unref(&fields);
  cardano_constr_plutus_data_unref(&constr);

  return len;
}

/**
 * \brief Returns the field at the given index of a constructor plutus-data value.
 */
static cardano_plutus_data_t*
constr_field(cardano_plutus_data_t* data, size_t index)
{
  cardano_constr_plutus_data_t* constr = NULL;
  EXPECT_EQ(cardano_plutus_data_to_constr(data, &constr), CARDANO_SUCCESS);

  cardano_plutus_list_t* fields = NULL;
  EXPECT_EQ(cardano_constr_plutus_data_get_data(constr, &fields), CARDANO_SUCCESS);

  cardano_plutus_data_t* field = NULL;
  EXPECT_EQ(cardano_plutus_list_get(fields, index, &field), CARDANO_SUCCESS);

  cardano_plutus_list_unref(&fields);
  cardano_constr_plutus_data_unref(&constr);

  return field;
}

/* TESTS *********************************************************************/

TEST(uplc_script_context, v1_context_has_top_level_shape)
{
  // Arrange
  cardano_transaction_t* tx    = decode_tx(kTxCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);
  cardano_redeemer_t*    rdmr  = first_redeemer(tx);

  // Act
  cardano_plutus_data_t* ctx    = NULL;
  cardano_error_t        result = cardano_uplc_int_build_script_context_v1(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, rdmr, &ctx);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(constr_alt(ctx), 0U);
  ASSERT_EQ(constr_len(ctx), 2U);

  cardano_plutus_data_t* tx_info = constr_field(ctx, 0);
  cardano_plutus_data_t* purpose = constr_field(ctx, 1);

  EXPECT_EQ(constr_alt(tx_info), 0U);
  EXPECT_EQ(constr_len(tx_info), 10U);

  // Spending purpose is Constr 1.
  EXPECT_EQ(constr_alt(purpose), 1U);

  cardano_plutus_data_unref(&tx_info);
  cardano_plutus_data_unref(&purpose);
  cardano_plutus_data_unref(&ctx);
  cardano_redeemer_unref(&rdmr);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, v2_context_has_top_level_shape)
{
  // Arrange
  cardano_transaction_t* tx    = decode_tx(kTxCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);
  cardano_redeemer_t*    rdmr  = first_redeemer(tx);

  // Act
  cardano_plutus_data_t* ctx    = NULL;
  cardano_error_t        result = cardano_uplc_int_build_script_context_v2(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, rdmr, &ctx);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(constr_alt(ctx), 0U);
  ASSERT_EQ(constr_len(ctx), 2U);

  cardano_plutus_data_t* tx_info = constr_field(ctx, 0);

  EXPECT_EQ(constr_alt(tx_info), 0U);
  EXPECT_EQ(constr_len(tx_info), 12U);

  cardano_plutus_data_unref(&tx_info);
  cardano_plutus_data_unref(&ctx);
  cardano_redeemer_unref(&rdmr);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, tx_info_v1_round_trips_through_cbor)
{
  // Arrange
  cardano_transaction_t* tx    = decode_tx(kTxCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);

  // Act
  cardano_plutus_data_t* tx_info = NULL;
  cardano_error_t        result  = cardano_uplc_int_build_tx_info_v1(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  ASSERT_EQ(cardano_plutus_data_to_cbor(tx_info, writer), CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  std::string  hex(hex_size, '\0');
  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, &hex[0], hex_size), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(hex.c_str(), hex.size() - 1U);
  cardano_plutus_data_t* reparsed = NULL;
  ASSERT_EQ(cardano_plutus_data_from_cbor(reader, &reparsed), CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_plutus_data_equals(tx_info, reparsed));

  cardano_plutus_data_unref(&reparsed);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&tx_info);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, spend_purpose_wraps_wrapped_out_ref)
{
  // Arrange
  cardano_transaction_t* tx    = decode_tx(kTxCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);
  cardano_redeemer_t*    rdmr  = first_redeemer(tx);

  // Act
  cardano_plutus_data_t* purpose = NULL;
  cardano_error_t        result  = cardano_uplc_int_build_script_purpose_v1v2(tx, utxos, rdmr, &purpose);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(constr_alt(purpose), 1U);
  ASSERT_EQ(constr_len(purpose), 1U);

  cardano_plutus_data_t* out_ref = constr_field(purpose, 0);
  EXPECT_EQ(constr_alt(out_ref), 0U);
  ASSERT_EQ(constr_len(out_ref), 2U);

  // Wrapped transaction id: Constr 0 [ id ].
  cardano_plutus_data_t* wrapped_id = constr_field(out_ref, 0);
  EXPECT_EQ(constr_alt(wrapped_id), 0U);

  cardano_plutus_data_unref(&wrapped_id);
  cardano_plutus_data_unref(&out_ref);
  cardano_plutus_data_unref(&purpose);
  cardano_redeemer_unref(&rdmr);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, null_arguments_are_rejected)
{
  cardano_transaction_t* tx    = decode_tx(kTxCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);
  cardano_redeemer_t*    rdmr  = first_redeemer(tx);
  cardano_plutus_data_t* ctx   = NULL;

  EXPECT_EQ(cardano_uplc_int_build_tx_info_v1(NULL, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &ctx), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_int_build_tx_info_v2(tx, NULL, &CARDANO_MAINNET_SLOT_CONFIG, &ctx), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_int_build_tx_info_v1(tx, utxos, NULL, &ctx), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_int_build_tx_info_v2(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, NULL), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_int_build_script_purpose_v1v2(NULL, utxos, rdmr, &ctx), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_int_build_script_context_v1(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, NULL, &ctx), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_int_build_script_context_v2(NULL, utxos, &CARDANO_MAINNET_SLOT_CONFIG, rdmr, &ctx), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_redeemer_unref(&rdmr);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

/**
 * \brief Serializes a plutus-data value to lowercase hex CBOR.
 */
static std::string
to_hex(cardano_plutus_data_t* data)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_EQ(cardano_plutus_data_to_cbor(data, writer), CARDANO_SUCCESS);

  const size_t n = cardano_cbor_writer_get_hex_size(writer);
  std::string  s(n, '\0');
  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, s.data(), n), CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  if (!s.empty() && (s.back() == '\0'))
  {
    s.pop_back();
  }

  return s;
}

TEST(uplc_script_context, tx_info_v1_byte_exact)
{
  // Expected V1 TxInfo CBOR generated from the ledger encoding mirrored from
  // aiken crates/uplc/src/tx/to_plutus_data.rs (TxInfo::V1 to_plutus_data).
  static const char* kExpectedV1 =
    "d8799f9fd8799fd8799fd8799f582000000000000000000000000000000000000000000000000000000000"
    "00000000ff00ffd8799fd8799fd8799f581c33333333333333333333333333333333333333333333333333"
    "333333ffd87a80ffa140a1401a004c4b40d87a80ffffff9fd8799fd8799fd8799f581c1111111111111111"
    "1111111111111111111111111111111111111111ffd87a80ffa140a1401a000f4240d87a80ffffa140a140"
    "1a00030d40a140a140008080d8799fd8799fd87a9f1b000001739cf6dd38ffd87a80ffd8799fd87a9f1b00"
    "000173a2ecbe38ffd87980ffff9f581c020202020202020202020202020202020202020202020202020202"
    "02ff80d8799f582087aef86f0290a6cbcc910e3eccb2536e3b9d999b21b54b08e54d4fb49eefdbd0ffff";

  cardano_transaction_t* tx    = decode_tx(kTxWithIntervalCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);

  cardano_plutus_data_t* v1 = NULL;
  ASSERT_EQ(cardano_uplc_int_build_tx_info_v1(tx, utxos, &kSlotConfig, &v1), CARDANO_SUCCESS);
  EXPECT_EQ(to_hex(v1), kExpectedV1);

  cardano_plutus_data_unref(&v1);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, tx_info_v2_byte_exact)
{
  // Expected V2 TxInfo CBOR mirrored from aiken TxInfo::V2 to_plutus_data: it
  // adds reference inputs, output datum option and script ref, the redeemer map
  // and the full datum map.
  static const char* kExpectedV2 =
    "d8799f9fd8799fd8799fd8799f582000000000000000000000000000000000000000000000000000000000"
    "00000000ff00ffd8799fd8799fd8799f581c33333333333333333333333333333333333333333333333333"
    "333333ffd87a80ffa140a1401a004c4b40d87980d87a80ffffff809fd8799fd8799fd8799f581c11111111"
    "111111111111111111111111111111111111111111111111ffd87a80ffa140a1401a000f4240d87980d87a"
    "80ffffa140a1401a00030d40a140a1400080a0d8799fd8799fd87a9f1b000001739cf6dd38ffd87a80ffd8"
    "799fd87a9f1b00000173a2ecbe38ffd87980ffff9f581c0202020202020202020202020202020202020202"
    "0202020202020202ffa1d87a9fd8799fd8799f582000000000000000000000000000000000000000000000"
    "00000000000000000000ff00ffffd87980a0d8799f582087aef86f0290a6cbcc910e3eccb2536e3b9d999b"
    "21b54b08e54d4fb49eefdbd0ffff";

  cardano_transaction_t* tx    = decode_tx(kTxWithIntervalCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);

  cardano_plutus_data_t* v2 = NULL;
  ASSERT_EQ(cardano_uplc_int_build_tx_info_v2(tx, utxos, &kSlotConfig, &v2), CARDANO_SUCCESS);
  EXPECT_EQ(to_hex(v2), kExpectedV2);

  cardano_plutus_data_unref(&v2);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, slot_before_origin_is_rejected)
{
  // The default mainnet slot origin (4492800) is past the interval slots
  // (4500000 is fine, but the preview origin 0 differs); a slot config whose
  // origin is past the interval slots makes the conversion underflow, which
  // must surface as an error rather than a wrapped value.
  static const cardano_slot_config_t late_origin = { 0U, 9000000U, 1000U };

  cardano_transaction_t* tx    = decode_tx(kTxWithIntervalCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);

  cardano_plutus_data_t* v1 = NULL;
  EXPECT_NE(cardano_uplc_int_build_tx_info_v1(tx, utxos, &late_origin, &v1), CARDANO_SUCCESS);
  EXPECT_EQ(v1, nullptr);

  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, missing_resolved_input_is_an_error)
{
  // Arrange: empty UTxO set cannot resolve the spend input.
  cardano_transaction_t* tx    = decode_tx(kTxCbor);
  cardano_utxo_list_t*   utxos = NULL;
  ASSERT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);

  // Act
  cardano_plutus_data_t* tx_info = NULL;
  cardano_error_t        result  = cardano_uplc_int_build_tx_info_v1(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_EQ(tx_info, nullptr);

  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

/* V3 TESTS ******************************************************************/

/**
 * \brief Returns the key at the given index of a plutus map.
 */
static cardano_plutus_data_t*
map_key_at(cardano_plutus_data_t* data, size_t index)
{
  cardano_plutus_map_t* map = NULL;
  EXPECT_EQ(cardano_plutus_data_to_map(data, &map), CARDANO_SUCCESS);

  cardano_plutus_list_t* keys = NULL;
  EXPECT_EQ(cardano_plutus_map_get_keys(map, &keys), CARDANO_SUCCESS);

  cardano_plutus_data_t* key = NULL;
  EXPECT_EQ(cardano_plutus_list_get(keys, index, &key), CARDANO_SUCCESS);

  cardano_plutus_list_unref(&keys);
  cardano_plutus_map_unref(&map);

  return key;
}

/**
 * \brief Builds a spending datum (Constr 0 []) used by the V3 spending tests.
 */
static cardano_plutus_data_t*
make_unit_datum()
{
  cardano_plutus_list_t* empty = NULL;
  EXPECT_EQ(cardano_plutus_list_new(&empty), CARDANO_SUCCESS);

  cardano_constr_plutus_data_t* constr = NULL;
  EXPECT_EQ(cardano_constr_plutus_data_new(0U, empty, &constr), CARDANO_SUCCESS);

  cardano_plutus_data_t* datum = NULL;
  EXPECT_EQ(cardano_plutus_data_new_constr(constr, &datum), CARDANO_SUCCESS);

  cardano_constr_plutus_data_unref(&constr);
  cardano_plutus_list_unref(&empty);

  return datum;
}

TEST(uplc_script_context, v3_context_has_top_level_shape)
{
  // Arrange
  cardano_transaction_t* tx    = decode_tx(kTxCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);
  cardano_redeemer_t*    rdmr  = first_redeemer(tx);

  // Act
  cardano_plutus_data_t* ctx    = NULL;
  cardano_error_t        result = cardano_uplc_int_build_script_context_v3(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, rdmr, NULL, &ctx);

  // Assert: ScriptContext::V3 is Constr 0 [tx_info, redeemer, script_info].
  ASSERT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(constr_alt(ctx), 0U);
  ASSERT_EQ(constr_len(ctx), 3U);

  cardano_plutus_data_t* tx_info = constr_field(ctx, 0);
  cardano_plutus_data_t* info    = constr_field(ctx, 2);

  // V3 TxInfo has 16 fields.
  EXPECT_EQ(constr_alt(tx_info), 0U);
  EXPECT_EQ(constr_len(tx_info), 16U);

  // The spending ScriptInfo is Constr 1 [out ref, Maybe datum].
  EXPECT_EQ(constr_alt(info), 1U);
  ASSERT_EQ(constr_len(info), 2U);

  cardano_plutus_data_unref(&tx_info);
  cardano_plutus_data_unref(&info);
  cardano_plutus_data_unref(&ctx);
  cardano_redeemer_unref(&rdmr);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, v3_tx_info_field_shapes)
{
  // Arrange
  cardano_transaction_t* tx    = decode_tx(kTxCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);

  // Act
  cardano_plutus_data_t* tx_info = NULL;
  ASSERT_EQ(cardano_uplc_int_build_tx_info_v3(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info), CARDANO_SUCCESS);

  // The fee (field 3) is a bare integer in V3, not a value map.
  cardano_plutus_data_t*     fee  = constr_field(tx_info, 3);
  cardano_plutus_data_kind_t kind = CARDANO_PLUTUS_DATA_KIND_CONSTR;
  EXPECT_EQ(cardano_plutus_data_get_kind(fee, &kind), CARDANO_SUCCESS);
  EXPECT_EQ(kind, CARDANO_PLUTUS_DATA_KIND_INTEGER);

  // The transaction id (field 11) is a bare bytestring, not wrapped.
  cardano_plutus_data_t* id = constr_field(tx_info, 11);
  EXPECT_EQ(cardano_plutus_data_get_kind(id, &kind), CARDANO_SUCCESS);
  EXPECT_EQ(kind, CARDANO_PLUTUS_DATA_KIND_BYTES);

  // The current treasury amount (field 14) and donation (field 15) are Nothing.
  cardano_plutus_data_t* treasury = constr_field(tx_info, 14);
  cardano_plutus_data_t* donation = constr_field(tx_info, 15);
  EXPECT_EQ(constr_alt(treasury), 1U);
  EXPECT_EQ(constr_alt(donation), 1U);

  cardano_plutus_data_unref(&fee);
  cardano_plutus_data_unref(&id);
  cardano_plutus_data_unref(&treasury);
  cardano_plutus_data_unref(&donation);
  cardano_plutus_data_unref(&tx_info);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, v3_spending_script_info_carries_datum)
{
  // Arrange
  cardano_transaction_t* tx    = decode_tx(kTxCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);
  cardano_redeemer_t*    rdmr  = first_redeemer(tx);
  cardano_plutus_data_t* datum = make_unit_datum();

  // Act: with a datum.
  cardano_plutus_data_t* info = NULL;
  ASSERT_EQ(cardano_uplc_int_build_script_info_v3(tx, utxos, rdmr, datum, &info), CARDANO_SUCCESS);

  // Assert: Constr 1 [out ref, Some datum].
  EXPECT_EQ(constr_alt(info), 1U);
  ASSERT_EQ(constr_len(info), 2U);

  cardano_plutus_data_t* out_ref = constr_field(info, 0);
  cardano_plutus_data_t* maybe   = constr_field(info, 1);

  // The out ref uses the unwrapped transaction id: Constr 0 [id_bytes, index].
  EXPECT_EQ(constr_alt(out_ref), 0U);
  ASSERT_EQ(constr_len(out_ref), 2U);
  cardano_plutus_data_t*     id   = constr_field(out_ref, 0);
  cardano_plutus_data_kind_t kind = CARDANO_PLUTUS_DATA_KIND_CONSTR;
  EXPECT_EQ(cardano_plutus_data_get_kind(id, &kind), CARDANO_SUCCESS);
  EXPECT_EQ(kind, CARDANO_PLUTUS_DATA_KIND_BYTES);

  // The datum is Some (Constr 0 [datum]).
  EXPECT_EQ(constr_alt(maybe), 0U);

  cardano_plutus_data_unref(&id);
  cardano_plutus_data_unref(&out_ref);
  cardano_plutus_data_unref(&maybe);
  cardano_plutus_data_unref(&info);

  // Act: without a datum, the Maybe is Nothing (Constr 1).
  cardano_plutus_data_t* info2 = NULL;
  ASSERT_EQ(cardano_uplc_int_build_script_info_v3(tx, utxos, rdmr, NULL, &info2), CARDANO_SUCCESS);
  cardano_plutus_data_t* maybe2 = constr_field(info2, 1);
  EXPECT_EQ(constr_alt(maybe2), 1U);

  cardano_plutus_data_unref(&maybe2);
  cardano_plutus_data_unref(&info2);
  cardano_plutus_data_unref(&datum);
  cardano_redeemer_unref(&rdmr);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, v3_tx_info_round_trips_through_cbor)
{
  cardano_transaction_t* tx    = decode_tx(kTxWithIntervalCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);

  cardano_plutus_data_t* tx_info = NULL;
  ASSERT_EQ(cardano_uplc_int_build_tx_info_v3(tx, utxos, &kSlotConfig, &tx_info), CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  ASSERT_EQ(cardano_plutus_data_to_cbor(tx_info, writer), CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  std::string  hex(hex_size, '\0');
  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, &hex[0], hex_size), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(hex.c_str(), hex.size() - 1U);
  cardano_plutus_data_t* reparsed = NULL;
  ASSERT_EQ(cardano_plutus_data_from_cbor(reader, &reparsed), CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_plutus_data_equals(tx_info, reparsed));

  cardano_plutus_data_unref(&reparsed);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&tx_info);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, v3_mint_script_info)
{
  // A Conway transaction minting a single asset under one policy, with a Mint
  // redeemer at index 0.
  static const char* kMintTx =
    "84a50081825820000000000000000000000000000000000000000000000000000000000000000000018182"
    "581d61111111111111111111111111111111111111111111111111111111111a000f4240021a00030d4009"
    "a1581c12121212121212121212121212121212121212121212121212121212a144746573741801"
    "0e81581c02020202020202020202020202020202020202020202020202020202a10581840100d87980821a"
    "000f42401a05f5e100f5f6";

  cardano_transaction_t* tx    = decode_tx(kMintTx);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);
  cardano_redeemer_t*    rdmr  = first_redeemer(tx);

  cardano_plutus_data_t* info = NULL;
  ASSERT_EQ(cardano_uplc_int_build_script_info_v3(tx, utxos, rdmr, NULL, &info), CARDANO_SUCCESS);

  // Minting ScriptInfo is Constr 0 [policy id].
  EXPECT_EQ(constr_alt(info), 0U);
  ASSERT_EQ(constr_len(info), 1U);

  // The V3 mint field excludes the synthetic ada entry: its first key is the
  // single minting policy, a 28-byte bytestring (not the empty ada policy).
  cardano_plutus_data_t* tx_info = NULL;
  ASSERT_EQ(cardano_uplc_int_build_tx_info_v3(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info), CARDANO_SUCCESS);
  cardano_plutus_data_t* mint     = constr_field(tx_info, 4);
  cardano_plutus_map_t*  mint_map = NULL;
  ASSERT_EQ(cardano_plutus_data_to_map(mint, &mint_map), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_map_get_length(mint_map), 1U);

  cardano_plutus_map_unref(&mint_map);
  cardano_plutus_data_unref(&mint);
  cardano_plutus_data_unref(&tx_info);
  cardano_plutus_data_unref(&info);
  cardano_redeemer_unref(&rdmr);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

/**
 * \brief Returns the kind of a plutus-data value.
 */
static cardano_plutus_data_kind_t
data_kind(cardano_plutus_data_t* data)
{
  cardano_plutus_data_kind_t kind = CARDANO_PLUTUS_DATA_KIND_CONSTR;
  EXPECT_EQ(cardano_plutus_data_get_kind(data, &kind), CARDANO_SUCCESS);

  return kind;
}

/**
 * \brief A Conway transaction whose body carries a single withdrawal keyed by a
 *        key-hash stake credential (reward address e1 + 28-byte key hash).
 */
static const char* kTxWdrlKeyCbor =
  "84a50081825820000000000000000000000000000000000000000000000000000000000000000000018182"
  "581d61111111111111111111111111111111111111111111111111111111111a000f4240021a00030d4005"
  "a1581de1333333333333333333333333333333333333333333333333333333331a000f42400e81581c0202"
  "0202020202020202020202020202020202020202020202020202a10581840000d87980821a000f42401a05"
  "f5e100f5f6";

/**
 * \brief As kTxWdrlKeyCbor but the withdrawal is keyed by a script-hash stake
 *        credential (reward address f1 + 28-byte script hash).
 */
static const char* kTxWdrlScriptCbor =
  "84a50081825820000000000000000000000000000000000000000000000000000000000000000000018182"
  "581d61111111111111111111111111111111111111111111111111111111111a000f4240021a00030d4005"
  "a1581df1444444444444444444444444444444444444444444444444444444441a000f42400e81581c0202"
  "0202020202020202020202020202020202020202020202020202a10581840000d87980821a000f42401a05"
  "f5e100f5f6";

/**
 * \brief A Conway transaction whose body carries a treasury-withdrawals proposal
 *        procedure; the withdrawal is keyed by a key-hash stake credential.
 */
static const char* kTxTreasuryCbor =
  "84a5008182582000000000000000000000000000000000000000000000000000000000000000000001818258"
  "1d61111111111111111111111111111111111111111111111111111111111a000f4240021a00030d400e8158"
  "1c020202020202020202020202020202020202020202020202020202021481841a000f4240581de1cb0ec269"
  "2497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8302a1581de1cb0ec2692497b458e46812c8a5bf"
  "a2931d1a2d965a99893828ec810f01581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b8"
  "0d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000"
  "000000000000000000000000000000a10581840000d87980821a000f42401a05f5e100f5f6";

/**
 * \brief A Conway transaction whose body carries a single pool-registration
 *        certificate (operator d85087.., pool VRF 8dd154..).
 */
static const char* kTxPoolRegCbor =
  "84a5008182582000000000000000000000000000000000000000000000000000000000000000000001818258"
  "1d61111111111111111111111111111111111111111111111111111111111a000f4240021a00030d4004818a"
  "03581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12"
  "bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e4"
  "6812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d"
  "965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b657861"
  "6d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf"
  "8a466e6e754833a08a62a6c56fe0e78f19d9d50e81581c020202020202020202020202020202020202020202"
  "02020202020202a10581840000d87980821a000f42401a05f5e100f5f6";

/**
 * \brief A Conway transaction whose body carries a single pool-retirement
 *        certificate (pool d85087.., epoch 1000).
 */
static const char* kTxPoolRetCbor =
  "84a5008182582000000000000000000000000000000000000000000000000000000000000000000001818258"
  "1d61111111111111111111111111111111111111111111111111111111111a000f4240021a00030d40048183"
  "04581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef921903e80e81581c020202020202"
  "02020202020202020202020202020202020202020202a10581840000d87980821a000f42401a05f5e100f5f6";

/**
 * \brief A Conway transaction whose certificate set is a script-credentialed
 *        REGISTRATION (key 7, with deposit) followed by an UNREGISTRATION (key 8).
 *
 * The V1/V2 context must translate these to the legacy DCertDelegRegKey /
 * DCertDelegDeRegKey (deposit dropped), matching the ledger's transTxCertV1V2.
 */
static const char* kTxRegDepositCbor =
  "84a40081825820000000000000000000000000000000000000000000000000000000000000000000018182581d"
  "61111111111111111111111111111111111111111111111111111111111a000f4240021a00030d400482830782"
  "01581cabababababababababababababababababababababababababababab1a001e848083088201581cababab"
  "ababababababababababababababababababababababababab1a001e8480a0f5f6";

TEST(uplc_script_context, v2_conway_registration_dcert_drops_deposit)
{
  // transTxCertV1V2: RegDepositTxCert -> DCertDelegRegKey (Constr 0 [StakingHash cred]);
  // UnRegDepositTxCert -> DCertDelegDeRegKey (Constr 1 [StakingHash cred]).
  cardano_transaction_t* tx    = decode_tx(kTxRegDepositCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);

  cardano_plutus_data_t* tx_info = NULL;
  ASSERT_EQ(cardano_uplc_int_build_tx_info_v2(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info), CARDANO_SUCCESS);

  cardano_plutus_data_t* certs = constr_field(tx_info, 5U);
  ASSERT_EQ(data_kind(certs), CARDANO_PLUTUS_DATA_KIND_LIST);

  cardano_plutus_list_t* list = NULL;
  ASSERT_EQ(cardano_plutus_data_to_list(certs, &list), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_plutus_list_get_length(list), 2U);

  cardano_plutus_data_t* reg   = NULL;
  cardano_plutus_data_t* unreg = NULL;
  ASSERT_EQ(cardano_plutus_list_get(list, 0U, &reg), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_plutus_list_get(list, 1U, &unreg), CARDANO_SUCCESS);

  EXPECT_EQ(constr_alt(reg), 0U);
  ASSERT_EQ(constr_len(reg), 1U);
  EXPECT_EQ(constr_alt(unreg), 1U);
  ASSERT_EQ(constr_len(unreg), 1U);

  cardano_plutus_data_t* staking_hash = constr_field(reg, 0);
  EXPECT_EQ(constr_alt(staking_hash), 0U);
  ASSERT_EQ(constr_len(staking_hash), 1U);

  cardano_plutus_data_t* credential = constr_field(staking_hash, 0);
  EXPECT_EQ(constr_alt(credential), 1U);

  cardano_plutus_data_unref(&credential);
  cardano_plutus_data_unref(&staking_hash);
  cardano_plutus_data_unref(&unreg);
  cardano_plutus_data_unref(&reg);
  cardano_plutus_list_unref(&list);
  cardano_plutus_data_unref(&certs);
  cardano_plutus_data_unref(&tx_info);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

/**
 * \brief A V1/V2 transaction carrying a Conway treasury-donation field, which a
 *        V1/V2 script context cannot represent (guardConwayFeaturesForPlutusV1V2).
 */
static const char* kTxDonationCbor =
  "84a40081825820000000000000000000000000000000000000000000000000000000000000000000018182581d"
  "61111111111111111111111111111111111111111111111111111111111a000f4240021a00030d40161a000f42"
  "40a0f5f6";

TEST(uplc_script_context, v3_conway_registration_dcert_carries_deposit)
{
  // transTxCert (post bootstrap): RegDepositTxCert -> TxCertRegStaking cred (Just deposit)
  // = Constr 0 [cred, Constr 0 [deposit]]; UnRegDepositTxCert -> Constr 1 [cred, Just refund].
  cardano_transaction_t* tx    = decode_tx(kTxRegDepositCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);

  cardano_plutus_data_t* tx_info = NULL;
  ASSERT_EQ(cardano_uplc_int_build_tx_info_v3(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info), CARDANO_SUCCESS);

  cardano_plutus_data_t* certs = constr_field(tx_info, 5U);
  ASSERT_EQ(data_kind(certs), CARDANO_PLUTUS_DATA_KIND_LIST);

  cardano_plutus_list_t* list = NULL;
  ASSERT_EQ(cardano_plutus_data_to_list(certs, &list), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_plutus_list_get_length(list), 2U);

  cardano_plutus_data_t* reg   = NULL;
  cardano_plutus_data_t* unreg = NULL;
  ASSERT_EQ(cardano_plutus_list_get(list, 0U, &reg), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_plutus_list_get(list, 1U, &unreg), CARDANO_SUCCESS);

  EXPECT_EQ(constr_alt(reg), 0U);
  ASSERT_EQ(constr_len(reg), 2U);
  EXPECT_EQ(constr_alt(unreg), 1U);
  ASSERT_EQ(constr_len(unreg), 2U);

  // Field 1 of each is the Maybe deposit/refund: Just x = Constr 0 [x].
  cardano_plutus_data_t* reg_deposit = constr_field(reg, 1);
  EXPECT_EQ(constr_alt(reg_deposit), 0U);
  ASSERT_EQ(constr_len(reg_deposit), 1U);

  cardano_plutus_data_t* deposit_value = constr_field(reg_deposit, 0);
  EXPECT_EQ(data_kind(deposit_value), CARDANO_PLUTUS_DATA_KIND_INTEGER);

  cardano_plutus_data_t* unreg_refund = constr_field(unreg, 1);
  EXPECT_EQ(constr_alt(unreg_refund), 0U);

  cardano_plutus_data_unref(&unreg_refund);
  cardano_plutus_data_unref(&deposit_value);
  cardano_plutus_data_unref(&reg_deposit);
  cardano_plutus_data_unref(&unreg);
  cardano_plutus_data_unref(&reg);
  cardano_plutus_list_unref(&list);
  cardano_plutus_data_unref(&certs);
  cardano_plutus_data_unref(&tx_info);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, v1v2_context_rejects_conway_treasury_fields)
{
  cardano_transaction_t* tx    = decode_tx(kTxDonationCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);

  cardano_plutus_data_t* tx_info_v1 = NULL;
  cardano_plutus_data_t* tx_info_v2 = NULL;

  EXPECT_NE(cardano_uplc_int_build_tx_info_v1(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info_v1), CARDANO_SUCCESS);
  EXPECT_NE(cardano_uplc_int_build_tx_info_v2(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info_v2), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&tx_info_v1);
  cardano_plutus_data_unref(&tx_info_v2);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

/**
 * \brief The hash of the script credential the Dijkstra field tests attach.
 */
static const char* kGuardScriptHash = "abababababababababababababababababababababababababababab";

/**
 * \brief Builds a credential of the given type over kGuardScriptHash.
 */
static cardano_credential_t*
guard_credential(cardano_credential_type_t type)
{
  cardano_credential_t* credential = NULL;
  EXPECT_EQ(cardano_credential_from_hash_hex(kGuardScriptHash, strlen(kGuardScriptHash), type, &credential), CARDANO_SUCCESS);

  return credential;
}

/**
 * \brief Builds a testnet reward address over a script credential.
 */
static cardano_reward_address_t*
guard_reward_address()
{
  cardano_credential_t*     credential = guard_credential(CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);
  cardano_reward_address_t* address    = NULL;
  EXPECT_EQ(cardano_reward_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, credential, &address), CARDANO_SUCCESS);

  cardano_credential_unref(&credential);

  return address;
}

/**
 * \brief Replaces the guard set of kTxCbor with a single guard of the given type.
 */
static cardano_transaction_t*
tx_with_guard(cardano_credential_type_t type)
{
  cardano_transaction_t*      tx         = decode_tx(kTxCbor);
  cardano_transaction_body_t* body       = cardano_transaction_get_body(tx);
  cardano_credential_t*       credential = guard_credential(type);
  cardano_guard_set_t*        guards     = NULL;

  EXPECT_EQ(cardano_guard_set_new(&guards), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_guard_set_add(guards, credential), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_guards(body, guards), CARDANO_SUCCESS);

  cardano_guard_set_unref(&guards);
  cardano_credential_unref(&credential);
  cardano_transaction_body_unref(&body);

  return tx;
}

/**
 * \brief Expects the V1, V2 and V3 TxInfo builders to reject \p tx as the ledger's
 *        guardDijkstraFeaturesForPlutusV1toV3 does.
 */
static void
expect_v1_to_v3_rejected(cardano_transaction_t* tx)
{
  cardano_utxo_list_t*   utxos   = decode_utxos(kUtxoCbor);
  cardano_plutus_data_t* tx_info = NULL;

  EXPECT_EQ(cardano_uplc_int_build_tx_info_v1(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info), CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(tx_info, nullptr);
  EXPECT_EQ(cardano_uplc_int_build_tx_info_v2(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info), CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(tx_info, nullptr);
  EXPECT_EQ(cardano_uplc_int_build_tx_info_v3(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info), CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(tx_info, nullptr);

  cardano_utxo_list_unref(&utxos);
}

TEST(uplc_script_context, v1_to_v3_context_rejects_direct_deposits)
{
  cardano_transaction_t*        tx       = decode_tx(kTxCbor);
  cardano_transaction_body_t*   body     = cardano_transaction_get_body(tx);
  cardano_reward_address_t*     address  = guard_reward_address();
  cardano_direct_deposit_map_t* deposits = NULL;

  ASSERT_EQ(cardano_direct_deposit_map_new(&deposits), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_direct_deposit_map_insert(deposits, address, 1000000U), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_transaction_body_set_direct_deposits(body, deposits), CARDANO_SUCCESS);

  expect_v1_to_v3_rejected(tx);

  cardano_direct_deposit_map_unref(&deposits);
  cardano_reward_address_unref(&address);
  cardano_transaction_body_unref(&body);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, v1_to_v3_context_rejects_account_balance_intervals)
{
  const uint64_t                           lower     = 1000000U;
  cardano_transaction_t*                   tx        = decode_tx(kTxCbor);
  cardano_transaction_body_t*              body      = cardano_transaction_get_body(tx);
  cardano_reward_address_t*                address   = guard_reward_address();
  cardano_account_balance_interval_t*      interval  = NULL;
  cardano_account_balance_intervals_map_t* intervals = NULL;

  ASSERT_EQ(cardano_account_balance_interval_new(&lower, NULL, &interval), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_account_balance_intervals_map_new(&intervals), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_account_balance_intervals_map_insert(intervals, address, interval), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_transaction_body_set_account_balance_intervals(body, intervals), CARDANO_SUCCESS);

  expect_v1_to_v3_rejected(tx);

  cardano_account_balance_intervals_map_unref(&intervals);
  cardano_account_balance_interval_unref(&interval);
  cardano_reward_address_unref(&address);
  cardano_transaction_body_unref(&body);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, v1_to_v3_context_rejects_required_top_level_guards)
{
  cardano_transaction_t*         tx         = decode_tx(kTxCbor);
  cardano_transaction_body_t*    body       = cardano_transaction_get_body(tx);
  cardano_credential_t*          credential = guard_credential(CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  cardano_required_guards_map_t* required   = NULL;

  ASSERT_EQ(cardano_required_guards_map_new(&required), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_required_guards_map_insert(required, credential, NULL), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_transaction_body_set_required_top_level_guards(body, required), CARDANO_SUCCESS);

  expect_v1_to_v3_rejected(tx);

  cardano_required_guards_map_unref(&required);
  cardano_credential_unref(&credential);
  cardano_transaction_body_unref(&body);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, v1_to_v3_context_rejects_a_script_hash_guard)
{
  cardano_transaction_t* tx = tx_with_guard(CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);

  expect_v1_to_v3_rejected(tx);

  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, v1_to_v3_context_accepts_a_key_hash_guard)
{
  cardano_transaction_t* tx      = tx_with_guard(CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  cardano_utxo_list_t*   utxos   = decode_utxos(kUtxoCbor);
  cardano_plutus_data_t* tx_info = NULL;

  EXPECT_EQ(cardano_uplc_int_build_tx_info_v1(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&tx_info);
  EXPECT_EQ(cardano_uplc_int_build_tx_info_v2(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&tx_info);
  EXPECT_EQ(cardano_uplc_int_build_tx_info_v3(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&tx_info);

  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, v3_withdrawal_key_is_a_bare_key_credential)
{
  // C1: V3 txInfoWdrl is Map Credential Lovelace, so the key must be a bare
  // Credential (Constr 0 [keyHash]), not a 2-field Address constructor.
  cardano_transaction_t* tx    = decode_tx(kTxWdrlKeyCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);

  cardano_plutus_data_t* tx_info = NULL;
  ASSERT_EQ(cardano_uplc_int_build_tx_info_v3(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info), CARDANO_SUCCESS);

  cardano_plutus_data_t* wdrl = constr_field(tx_info, 6);
  EXPECT_EQ(data_kind(wdrl), CARDANO_PLUTUS_DATA_KIND_MAP);

  cardano_plutus_data_t* key = map_key_at(wdrl, 0);
  EXPECT_EQ(constr_alt(key), 0U);
  ASSERT_EQ(constr_len(key), 1U);

  cardano_plutus_data_t* hash = constr_field(key, 0);
  EXPECT_EQ(data_kind(hash), CARDANO_PLUTUS_DATA_KIND_BYTES);

  cardano_plutus_data_unref(&hash);
  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&wdrl);
  cardano_plutus_data_unref(&tx_info);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, v3_withdrawal_key_is_a_bare_script_credential)
{
  // C1: a script-cred withdrawal must key to Constr 1 [scriptHash].
  cardano_transaction_t* tx    = decode_tx(kTxWdrlScriptCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);

  cardano_plutus_data_t* tx_info = NULL;
  ASSERT_EQ(cardano_uplc_int_build_tx_info_v3(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info), CARDANO_SUCCESS);

  cardano_plutus_data_t* wdrl = constr_field(tx_info, 6);
  cardano_plutus_data_t* key  = map_key_at(wdrl, 0);
  EXPECT_EQ(constr_alt(key), 1U);
  ASSERT_EQ(constr_len(key), 1U);

  cardano_plutus_data_t* hash = constr_field(key, 0);
  EXPECT_EQ(data_kind(hash), CARDANO_PLUTUS_DATA_KIND_BYTES);

  cardano_plutus_data_unref(&hash);
  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&wdrl);
  cardano_plutus_data_unref(&tx_info);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, v3_treasury_withdrawals_key_is_a_bare_credential)
{
  // C2: TreasuryWithdrawals (Map Credential Lovelace) (Maybe ScriptHash) uses
  // the same bare-Credential key. The gov action is Constr 2 [wdrl, maybe sh].
  cardano_transaction_t* tx    = decode_tx(kTxTreasuryCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);

  cardano_plutus_data_t* tx_info = NULL;
  ASSERT_EQ(cardano_uplc_int_build_tx_info_v3(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info), CARDANO_SUCCESS);

  // Proposals are field 13: a list of ProposalProcedure values.
  cardano_plutus_data_t* proposals = constr_field(tx_info, 13);
  EXPECT_EQ(data_kind(proposals), CARDANO_PLUTUS_DATA_KIND_LIST);

  cardano_plutus_list_t* prop_list = NULL;
  ASSERT_EQ(cardano_plutus_data_to_list(proposals, &prop_list), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_plutus_list_get_length(prop_list), 1U);

  cardano_plutus_data_t* proposal = NULL;
  ASSERT_EQ(cardano_plutus_list_get(prop_list, 0, &proposal), CARDANO_SUCCESS);

  // ProposalProcedure is Constr 0 [deposit, returnAddr, govAction].
  cardano_plutus_data_t* gov_action = constr_field(proposal, 2);
  EXPECT_EQ(constr_alt(gov_action), 2U);

  cardano_plutus_data_t* wdrl = constr_field(gov_action, 0);
  EXPECT_EQ(data_kind(wdrl), CARDANO_PLUTUS_DATA_KIND_MAP);

  cardano_plutus_data_t* key = map_key_at(wdrl, 0);
  EXPECT_EQ(constr_alt(key), 0U);
  ASSERT_EQ(constr_len(key), 1U);

  cardano_plutus_data_t* hash = constr_field(key, 0);
  EXPECT_EQ(data_kind(hash), CARDANO_PLUTUS_DATA_KIND_BYTES);

  cardano_plutus_data_unref(&hash);
  cardano_plutus_data_unref(&key);
  cardano_plutus_data_unref(&wdrl);
  cardano_plutus_data_unref(&gov_action);
  cardano_plutus_data_unref(&proposal);
  cardano_plutus_list_unref(&prop_list);
  cardano_plutus_data_unref(&proposals);
  cardano_plutus_data_unref(&tx_info);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

/**
 * \brief Returns the only DCert from a V1 or V2 TxInfo.
 *
 * The certificate list sits at field 4 in V1 and field 5 in V2 (V2 inserts the
 * reference inputs at index 1, shifting the later fields by one).
 */
static cardano_plutus_data_t*
single_dcert(cardano_plutus_data_t* tx_info, size_t certs_index)
{
  cardano_plutus_data_t* certs = constr_field(tx_info, certs_index);
  EXPECT_EQ(data_kind(certs), CARDANO_PLUTUS_DATA_KIND_LIST);

  cardano_plutus_list_t* list = NULL;
  EXPECT_EQ(cardano_plutus_data_to_list(certs, &list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_list_get_length(list), 1U);

  cardano_plutus_data_t* cert = NULL;
  EXPECT_EQ(cardano_plutus_list_get(list, 0, &cert), CARDANO_SUCCESS);

  cardano_plutus_list_unref(&list);
  cardano_plutus_data_unref(&certs);

  return cert;
}

TEST(uplc_script_context, v1_pool_registration_dcert_is_constr_3)
{
  // M1: DCertPoolRegister PubKeyHash PubKeyHash => Constr 3 [poolId, poolVRF].
  cardano_transaction_t* tx    = decode_tx(kTxPoolRegCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);

  cardano_plutus_data_t* tx_info = NULL;
  ASSERT_EQ(cardano_uplc_int_build_tx_info_v1(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info), CARDANO_SUCCESS);

  cardano_plutus_data_t* cert = single_dcert(tx_info, 4U);
  EXPECT_EQ(constr_alt(cert), 3U);
  ASSERT_EQ(constr_len(cert), 2U);

  cardano_plutus_data_t* pool_id  = constr_field(cert, 0);
  cardano_plutus_data_t* pool_vrf = constr_field(cert, 1);
  EXPECT_EQ(data_kind(pool_id), CARDANO_PLUTUS_DATA_KIND_BYTES);
  EXPECT_EQ(data_kind(pool_vrf), CARDANO_PLUTUS_DATA_KIND_BYTES);

  cardano_plutus_data_unref(&pool_vrf);
  cardano_plutus_data_unref(&pool_id);
  cardano_plutus_data_unref(&cert);
  cardano_plutus_data_unref(&tx_info);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, v2_pool_registration_dcert_is_constr_3)
{
  cardano_transaction_t* tx    = decode_tx(kTxPoolRegCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);

  cardano_plutus_data_t* tx_info = NULL;
  ASSERT_EQ(cardano_uplc_int_build_tx_info_v2(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info), CARDANO_SUCCESS);

  cardano_plutus_data_t* cert = single_dcert(tx_info, 5U);
  EXPECT_EQ(constr_alt(cert), 3U);
  ASSERT_EQ(constr_len(cert), 2U);

  cardano_plutus_data_unref(&cert);
  cardano_plutus_data_unref(&tx_info);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, v1_pool_retirement_dcert_is_constr_4)
{
  // M1: DCertPoolRetire PubKeyHash Integer => Constr 4 [poolId, epoch].
  cardano_transaction_t* tx    = decode_tx(kTxPoolRetCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);

  cardano_plutus_data_t* tx_info = NULL;
  ASSERT_EQ(cardano_uplc_int_build_tx_info_v1(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info), CARDANO_SUCCESS);

  cardano_plutus_data_t* cert = single_dcert(tx_info, 4U);
  EXPECT_EQ(constr_alt(cert), 4U);
  ASSERT_EQ(constr_len(cert), 2U);

  cardano_plutus_data_t* pool_id = constr_field(cert, 0);
  cardano_plutus_data_t* epoch   = constr_field(cert, 1);
  EXPECT_EQ(data_kind(pool_id), CARDANO_PLUTUS_DATA_KIND_BYTES);
  EXPECT_EQ(data_kind(epoch), CARDANO_PLUTUS_DATA_KIND_INTEGER);

  cardano_plutus_data_unref(&epoch);
  cardano_plutus_data_unref(&pool_id);
  cardano_plutus_data_unref(&cert);
  cardano_plutus_data_unref(&tx_info);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, v2_pool_retirement_dcert_is_constr_4)
{
  cardano_transaction_t* tx    = decode_tx(kTxPoolRetCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);

  cardano_plutus_data_t* tx_info = NULL;
  ASSERT_EQ(cardano_uplc_int_build_tx_info_v2(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info), CARDANO_SUCCESS);

  cardano_plutus_data_t* cert = single_dcert(tx_info, 5U);
  EXPECT_EQ(constr_alt(cert), 4U);
  ASSERT_EQ(constr_len(cert), 2U);

  cardano_plutus_data_unref(&cert);
  cardano_plutus_data_unref(&tx_info);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, v3_null_arguments_are_rejected)
{
  cardano_transaction_t* tx    = decode_tx(kTxCbor);
  cardano_utxo_list_t*   utxos = decode_utxos(kUtxoCbor);
  cardano_redeemer_t*    rdmr  = first_redeemer(tx);
  cardano_plutus_data_t* ctx   = NULL;

  EXPECT_EQ(cardano_uplc_int_build_tx_info_v3(NULL, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &ctx), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_int_build_tx_info_v3(tx, NULL, &CARDANO_MAINNET_SLOT_CONFIG, &ctx), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_int_build_tx_info_v3(tx, utxos, NULL, &ctx), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_int_build_tx_info_v3(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, NULL), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_int_build_script_info_v3(NULL, utxos, rdmr, NULL, &ctx), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_int_build_script_info_v3(tx, utxos, NULL, NULL, &ctx), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_int_build_script_context_v3(NULL, utxos, &CARDANO_MAINNET_SLOT_CONFIG, rdmr, NULL, &ctx), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_uplc_int_build_script_context_v3(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, NULL, NULL, &ctx), CARDANO_ERROR_POINTER_IS_NULL);

  cardano_redeemer_unref(&rdmr);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

/**
 * \brief A transaction whose collections are ordered differently by the library
 *        and by the ledger.
 *
 * body = { 0: inputs[ (id=00*32, ix=0) ],
 *          1: outputs[ (enterprise-key 11*28, [1000000, { 50*28: { 02: 2, 0101: 1 } }]),
 *                      (enterprise-key 11*28, 0) ],
 *          2: fee 200000,
 *          5: withdrawals{ e1 ++ 33*28: 1000000, f1 ++ 44*28: 2000000 },
 *          9: mint{ 50*28: { 02: -1, 0101: 1 } } }
 * witness set = { 4: datums[ 1, 3 ] (descending hash order),
 *                 5: redeemers[ (Spend, 0), (Mint, 0), (Reward, 0) ] }
 *
 * The ledger orders V3 withdrawals as its Map AccountAddress: by network, then
 * script credentials before key credentials. V1 and V2 keep the Plutus
 * StakingCredential order, key credentials first. Asset name 0101 sorts before
 * 02 byte by byte, and the zero-coin output keeps its ada entry. The V1
 * txInfoData is a list [(DatumHash, Datum)]; V2 and V3 use Map DatumHash Datum.
 */
static const char* kTxLedgerOrderCbor =
  "84a5008182582000000000000000000000000000000000000000000000000000000000000000000001828258"
  "1d6111111111111111111111111111111111111111111111111111111111821a000f4240a1581c5050505050"
  "5050505050505050505050505050505050505050505050a24102024201010182581d61111111111111111111"
  "1111111111111111111111111111111111111100021a00030d4005a2581de133333333333333333333333333"
  "3333333333333333333333333333331a000f4240581df1444444444444444444444444444444444444444444"
  "444444444444441a001e848009a1581c50505050505050505050505050505050505050505050505050505050"
  "a241022042010101a2048201030583840000d87980821a000f42401a05f5e100840100d87980821a000f4240"
  "1a05f5e100840300d87980821a000f42401a05f5e100f5f6";

/**
 * \brief A transaction with a DRep key voter and a DRep script voter, and an
 *        UpdateCommittee proposal with key and script members and a 2/4 quorum.
 *
 * body = { 19: votes{ DRep key 55*28: { (cc*32, 0): Yes },
 *                     DRep script 66*28: { (cc*32, 0): No } },
 *          20: proposals[ (1000000, e1 ++ 77*28,
 *                          UpdateCommittee(None, { key 88*28, script 99*28 },
 *                                          { key aa*28: 10, script bb*28: 20 }, 2/4),
 *                          anchor) ] }
 * witness set = { 5: redeemers[ (Spend, 0), (Voting, 0) ] }
 */
static const char* kTxGovernanceOrderCbor =
  "84a5008182582000000000000000000000000000000000000000000000000000000000000000000001818258"
  "1d61111111111111111111111111111111111111111111111111111111111a000f4240021a00030d4013a282"
  "02581c55555555555555555555555555555555555555555555555555555555a1825820cccccccccccccccccc"
  "cccccccccccccccccccccccccccccccccccccccccccccc008201f68203581c66666666666666666666666666"
  "666666666666666666666666666666a1825820cccccccccccccccccccccccccccccccccccccccccccccccccc"
  "cccccccccccccc008200f61481841a000f4240581de177777777777777777777777777777777777777777777"
  "7777777777778504f6d90102828200581c888888888888888888888888888888888888888888888888888888"
  "888201581c99999999999999999999999999999999999999999999999999999999a28200581caaaaaaaaaaaa"
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa0a8201581cbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
  "bbbbbbbbbbbbbbbbbbbbbb14d81e820204827368747470733a2f2f6578616d706c652e636f6d582000000000"
  "00000000000000000000000000000000000000000000000000000000a10582840000d87980821a000f42401a"
  "05f5e100840400d87980821a000f42401a05f5e100f5f6";

/**
 * \brief A transaction with a TreasuryWithdrawals proposal paying a key account
 *        (e1 ++ 33*28: 1) and a script account (f1 ++ 44*28: 2).
 */
static const char* kTxTreasuryOrderCbor =
  "84a4008182582000000000000000000000000000000000000000000000000000000000000000000001818258"
  "1d61111111111111111111111111111111111111111111111111111111111a000f4240021a00030d40148184"
  "1a000f4240581de1777777777777777777777777777777777777777777777777777777778302a2581de13333"
  "333333333333333333333333333333333333333333333333333301581df14444444444444444444444444444"
  "444444444444444444444444444402f6827368747470733a2f2f6578616d706c652e636f6d58200000000000"
  "000000000000000000000000000000000000000000000000000000a10581840000d87980821a000f42401a05"
  "f5e100f5f6";

/**
 * \brief A transaction whose witness set carries the same datum twice.
 *
 * body = { 0: inputs[ (id=00*32, ix=0) ],
 *          1: outputs[ (enterprise-key 11*28, 1000000) ],
 *          2: fee 200000 }
 * witness set = { 4: datums[ 1, 3, 1 ], 5: redeemers[ (Spend, 0) ] }
 *
 * The ledger keeps the witness datums in a map keyed by hash, so the repeated
 * datum appears once in txInfoData.
 */
static const char* kTxRepeatedDatumCbor =
  "84a3008182582000000000000000000000000000000000000000000000000000000000000000000001818258"
  "1d61111111111111111111111111111111111111111111111111111111111a000f4240021a00030d40a20483"
  "0103010581840000d87980821a000f42401a05f5e100f5f6";

/**
 * \brief Returns the redeemer at a position of the witness set of a transaction.
 */
static cardano_redeemer_t*
redeemer_at(cardano_transaction_t* tx, const size_t position)
{
  cardano_witness_set_t*   witness   = cardano_transaction_get_witness_set(tx);
  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness);
  cardano_redeemer_t*      redeemer  = NULL;

  EXPECT_EQ(cardano_redeemer_list_get(redeemers, position, &redeemer), CARDANO_SUCCESS);

  cardano_redeemer_list_unref(&redeemers);
  cardano_witness_set_unref(&witness);

  return redeemer;
}

/**
 * \brief Serializes a field of a constructor plutus-data value to hex CBOR.
 */
static std::string
field_hex(cardano_plutus_data_t* data, const size_t index)
{
  cardano_plutus_data_t* field = constr_field(data, index);
  std::string            hex   = to_hex(field);

  cardano_plutus_data_unref(&field);

  return hex;
}

/**
 * \brief Builds the TxInfo of a transaction for a Plutus version (1, 2 or 3).
 */
static cardano_plutus_data_t*
build_tx_info(cardano_transaction_t* tx, cardano_utxo_list_t* utxos, const int version)
{
  cardano_plutus_data_t* tx_info = NULL;
  cardano_error_t        result  = CARDANO_ERROR_GENERIC;

  if (version == 1)
  {
    result = cardano_uplc_int_build_tx_info_v1(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info);
  }
  else if (version == 2)
  {
    result = cardano_uplc_int_build_tx_info_v2(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info);
  }
  else
  {
    result = cardano_uplc_int_build_tx_info_v3(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, &tx_info);
  }

  EXPECT_EQ(result, CARDANO_SUCCESS);

  return tx_info;
}

/**
 * \brief Serializes one field of the TxInfo of a transaction for a Plutus version.
 */
static std::string
tx_info_field_hex(const char* tx_hex, const int version, const size_t index)
{
  cardano_transaction_t* tx      = decode_tx(tx_hex);
  cardano_utxo_list_t*   utxos   = decode_utxos(kUtxoCbor);
  cardano_plutus_data_t* tx_info = build_tx_info(tx, utxos, version);
  std::string            hex     = (tx_info != NULL) ? field_hex(tx_info, index) : std::string();

  cardano_plutus_data_unref(&tx_info);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);

  return hex;
}

/**
 * \brief Builds a V1, V2 or V3 script context with every allocation failing in turn until it succeeds.
 *
 * \return The number of failed attempts.
 */
static int
sweep_context_allocations(const char* tx_hex, const size_t position, const int version)
{
  cardano_transaction_t* tx       = decode_tx(tx_hex);
  cardano_utxo_list_t*   utxos    = decode_utxos(kUtxoCbor);
  cardano_redeemer_t*    redeemer = redeemer_at(tx, position);
  cardano_error_t        result   = CARDANO_ERROR_GENERIC;
  int                    limit    = 0;

  for (; result != CARDANO_SUCCESS; ++limit)
  {
    cardano_plutus_data_t* context = NULL;

    reset_allocators_run_count();
    set_malloc_limit(limit);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    if (version == 1)
    {
      result = cardano_uplc_int_build_script_context_v1(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, redeemer, &context);
    }
    else if (version == 2)
    {
      result = cardano_uplc_int_build_script_context_v2(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, redeemer, &context);
    }
    else
    {
      result = cardano_uplc_int_build_script_context_v3(tx, utxos, &CARDANO_MAINNET_SLOT_CONFIG, redeemer, NULL, &context);
    }

    cardano_set_allocators(malloc, realloc, free);
    reset_limited_malloc();

    EXPECT_EQ(context == nullptr, result != CARDANO_SUCCESS);

    cardano_plutus_data_unref(&context);
  }

  cardano_redeemer_unref(&redeemer);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);

  return limit - 1;
}

TEST(uplc_script_context, v3_withdrawals_put_script_accounts_before_key_accounts)
{
  static const char* kExpected =
    "a2d87a9f581c44444444444444444444444444444444444444444444444444444444ff1a001e8480d8799f58"
    "1c33333333333333333333333333333333333333333333333333333333ff1a000f4240";

  EXPECT_EQ(tx_info_field_hex(kTxLedgerOrderCbor, 3, 6), kExpected);
}

TEST(uplc_script_context, v1_v2_withdrawals_keep_key_credentials_first)
{
  static const char* kExpectedV1 =
    "9fd8799fd8799fd8799f581c33333333333333333333333333333333333333333333333333333333ffff1a00"
    "0f4240ffd8799fd8799fd87a9f581c44444444444444444444444444444444444444444444444444444444ff"
    "ff1a001e8480ffff";
  static const char* kExpectedV2 =
    "a2d8799fd8799f581c33333333333333333333333333333333333333333333333333333333ffff1a000f4240"
    "d8799fd87a9f581c44444444444444444444444444444444444444444444444444444444ffff1a001e8480";

  EXPECT_EQ(tx_info_field_hex(kTxLedgerOrderCbor, 1, 5), kExpectedV1);
  EXPECT_EQ(tx_info_field_hex(kTxLedgerOrderCbor, 2, 6), kExpectedV2);
}

TEST(uplc_script_context, v3_treasury_withdrawals_put_script_accounts_first)
{
  static const char* kExpected =
    "9fd8799f1a000f4240d8799f581c77777777777777777777777777777777777777777777777777777777ffd8"
    "7b9fa2d87a9f581c44444444444444444444444444444444444444444444444444444444ff02d8799f581c33"
    "333333333333333333333333333333333333333333333333333333ff01d87a80ffffff";

  EXPECT_EQ(tx_info_field_hex(kTxTreasuryOrderCbor, 3, 13), kExpected);
}

TEST(uplc_script_context, output_asset_names_follow_byte_order)
{
  static const char* kExpectedV1 =
    "9fd8799fd8799fd8799f581c11111111111111111111111111111111111111111111111111111111ffd87a80"
    "ffa240a1401a000f4240581c50505050505050505050505050505050505050505050505050505050a2420101"
    "01410202d87a80ffd8799fd8799fd8799f581c11111111111111111111111111111111111111111111111111"
    "111111ffd87a80ffa140a14000d87a80ffff";
  static const char* kExpectedV2V3 =
    "9fd8799fd8799fd8799f581c11111111111111111111111111111111111111111111111111111111ffd87a80"
    "ffa240a1401a000f4240581c50505050505050505050505050505050505050505050505050505050a2420101"
    "01410202d87980d87a80ffd8799fd8799fd8799f581c11111111111111111111111111111111111111111111"
    "111111111111ffd87a80ffa140a14000d87980d87a80ffff";

  EXPECT_EQ(tx_info_field_hex(kTxLedgerOrderCbor, 1, 1), kExpectedV1);
  EXPECT_EQ(tx_info_field_hex(kTxLedgerOrderCbor, 2, 2), kExpectedV2V3);
  EXPECT_EQ(tx_info_field_hex(kTxLedgerOrderCbor, 3, 2), kExpectedV2V3);
}

TEST(uplc_script_context, mint_asset_names_follow_byte_order)
{
  static const char* kExpectedV1V2 =
    "a240a14000581c50505050505050505050505050505050505050505050505050505050a242010101410220";
  static const char* kExpectedV3 =
    "a1581c50505050505050505050505050505050505050505050505050505050a242010101410220";

  EXPECT_EQ(tx_info_field_hex(kTxLedgerOrderCbor, 1, 3), kExpectedV1V2);
  EXPECT_EQ(tx_info_field_hex(kTxLedgerOrderCbor, 2, 4), kExpectedV1V2);
  EXPECT_EQ(tx_info_field_hex(kTxLedgerOrderCbor, 3, 4), kExpectedV3);
}

TEST(uplc_script_context, datums_follow_hash_order)
{
  static const char* kExpectedV1 =
    "9fd8799f5820e88bd757ad5b9bedf372d8d3f0cf6c962a469db61a265f6418e1ffed86da29ec03ffd8799f58"
    "20ee155ace9c40292074cb6aff8c9ccdd273c81648ff1149ef36bcea6ebb8a3e2501ffff";
  static const char* kExpectedV2V3 =
    "a25820e88bd757ad5b9bedf372d8d3f0cf6c962a469db61a265f6418e1ffed86da29ec035820ee155ace9c40"
    "292074cb6aff8c9ccdd273c81648ff1149ef36bcea6ebb8a3e2501";

  EXPECT_EQ(tx_info_field_hex(kTxLedgerOrderCbor, 1, 8), kExpectedV1);
  EXPECT_EQ(tx_info_field_hex(kTxLedgerOrderCbor, 2, 10), kExpectedV2V3);
  EXPECT_EQ(tx_info_field_hex(kTxLedgerOrderCbor, 3, 10), kExpectedV2V3);
}

TEST(uplc_script_context, repeated_datums_appear_once)
{
  static const char* kExpectedV1 =
    "9fd8799f5820e88bd757ad5b9bedf372d8d3f0cf6c962a469db61a265f6418e1ffed86da29ec03ffd8799f58"
    "20ee155ace9c40292074cb6aff8c9ccdd273c81648ff1149ef36bcea6ebb8a3e2501ffff";
  static const char* kExpectedV2V3 =
    "a25820e88bd757ad5b9bedf372d8d3f0cf6c962a469db61a265f6418e1ffed86da29ec035820ee155ace9c40"
    "292074cb6aff8c9ccdd273c81648ff1149ef36bcea6ebb8a3e2501";

  EXPECT_EQ(tx_info_field_hex(kTxRepeatedDatumCbor, 1, 8), kExpectedV1);
  EXPECT_EQ(tx_info_field_hex(kTxRepeatedDatumCbor, 2, 10), kExpectedV2V3);
  EXPECT_EQ(tx_info_field_hex(kTxRepeatedDatumCbor, 3, 10), kExpectedV2V3);
}

TEST(uplc_script_context, v3_votes_put_script_voters_before_key_voters)
{
  static const char* kExpected =
    "a2d87a9fd87a9f581c66666666666666666666666666666666666666666666666666666666ffffa1d8799f58"
    "20cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc00ffd87980d87a9fd8799f"
    "581c55555555555555555555555555555555555555555555555555555555ffffa1d8799f5820cccccccccccc"
    "cccccccccccccccccccccccccccccccccccccccccccccccccccc00ffd87a80";

  EXPECT_EQ(tx_info_field_hex(kTxGovernanceOrderCbor, 3, 12), kExpected);
}

TEST(uplc_script_context, v3_update_committee_orders_members_and_reduces_quorum)
{
  static const char* kExpected =
    "9fd8799f1a000f4240d8799f581c77777777777777777777777777777777777777777777777777777777ffd8"
    "7d9fd87a809fd87a9f581c99999999999999999999999999999999999999999999999999999999ffd8799f58"
    "1c88888888888888888888888888888888888888888888888888888888ffffa2d87a9f581cbbbbbbbbbbbbbb"
    "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbff14d8799f581caaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaff0ad8799f0102ffffffff";

  EXPECT_EQ(tx_info_field_hex(kTxGovernanceOrderCbor, 3, 13), kExpected);
}

TEST(uplc_script_context, reward_redeemer_resolves_in_ledger_account_order)
{
  static const char* kExpectedV1V2 =
    "d87b9fd8799fd87a9f581c44444444444444444444444444444444444444444444444444444444ffffff";
  static const char* kExpectedV3 =
    "d87b9fd87a9f581c44444444444444444444444444444444444444444444444444444444ffff";

  cardano_transaction_t* tx       = decode_tx(kTxLedgerOrderCbor);
  cardano_utxo_list_t*   utxos    = decode_utxos(kUtxoCbor);
  cardano_redeemer_t*    redeemer = redeemer_at(tx, 2U);
  cardano_plutus_data_t* purpose  = NULL;
  cardano_plutus_data_t* info     = NULL;

  ASSERT_EQ(cardano_redeemer_get_tag(redeemer), CARDANO_REDEEMER_TAG_REWARD);
  ASSERT_EQ(cardano_uplc_int_build_script_purpose_v1v2(tx, utxos, redeemer, &purpose), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_uplc_int_build_script_info_v3(tx, utxos, redeemer, NULL, &info), CARDANO_SUCCESS);

  EXPECT_EQ(to_hex(purpose), kExpectedV1V2);
  EXPECT_EQ(to_hex(info), kExpectedV3);

  cardano_plutus_data_unref(&info);
  cardano_plutus_data_unref(&purpose);
  cardano_redeemer_unref(&redeemer);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, voting_redeemer_resolves_in_ledger_voter_order)
{
  static const char* kExpected =
    "d87d9fd87a9fd87a9f581c66666666666666666666666666666666666666666666666666666666ffffff";

  cardano_transaction_t* tx       = decode_tx(kTxGovernanceOrderCbor);
  cardano_utxo_list_t*   utxos    = decode_utxos(kUtxoCbor);
  cardano_redeemer_t*    redeemer = redeemer_at(tx, 1U);
  cardano_plutus_data_t* info     = NULL;

  ASSERT_EQ(cardano_redeemer_get_tag(redeemer), CARDANO_REDEEMER_TAG_VOTING);
  ASSERT_EQ(cardano_uplc_int_build_script_info_v3(tx, utxos, redeemer, NULL, &info), CARDANO_SUCCESS);

  EXPECT_EQ(to_hex(info), kExpected);

  cardano_plutus_data_unref(&info);
  cardano_redeemer_unref(&redeemer);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, withdrawal_at_ledger_index_puts_script_accounts_first)
{
  // Arrange
  cardano_transaction_t*      tx          = decode_tx(kTxLedgerOrderCbor);
  cardano_transaction_body_t* body        = cardano_transaction_get_body(tx);
  cardano_withdrawal_map_t*   withdrawals = cardano_transaction_body_get_withdrawals(body);
  cardano_reward_address_t*   first       = NULL;
  cardano_reward_address_t*   second      = NULL;
  cardano_reward_address_t*   missing     = NULL;

  // Act
  ASSERT_EQ(cardano_uplc_int_withdrawal_at_ledger_index(withdrawals, 0U, &first), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_uplc_int_withdrawal_at_ledger_index(withdrawals, 1U, &second), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_reward_address_get_bytes(first)[0], 0xf1U);
  EXPECT_EQ(cardano_reward_address_get_bytes(second)[0], 0xe1U);
  EXPECT_EQ(cardano_uplc_int_withdrawal_at_ledger_index(withdrawals, 2U, &missing), CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);
  EXPECT_EQ(cardano_uplc_int_withdrawal_at_ledger_index(NULL, 0U, &missing), CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);
  EXPECT_EQ(cardano_uplc_int_withdrawal_at_ledger_index(withdrawals, 0U, NULL), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(missing, nullptr);

  // Cleanup
  cardano_reward_address_unref(&second);
  cardano_reward_address_unref(&first);
  cardano_withdrawal_map_unref(&withdrawals);
  cardano_transaction_body_unref(&body);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, voter_at_ledger_index_puts_script_voters_first)
{
  // Arrange
  cardano_transaction_t*       tx         = decode_tx(kTxGovernanceOrderCbor);
  cardano_transaction_body_t*  body       = cardano_transaction_get_body(tx);
  cardano_voting_procedures_t* procedures = cardano_transaction_body_get_voting_procedures(body);
  cardano_voter_t*             first      = NULL;
  cardano_voter_t*             second     = NULL;
  cardano_voter_t*             missing    = NULL;
  cardano_voter_type_t         type       = CARDANO_VOTER_TYPE_STAKE_POOL_KEY_HASH;

  // Act
  ASSERT_EQ(cardano_uplc_int_voter_at_ledger_index(procedures, 0U, &first), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_uplc_int_voter_at_ledger_index(procedures, 1U, &second), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_voter_get_type(first, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_VOTER_TYPE_DREP_SCRIPT_HASH);
  EXPECT_EQ(cardano_voter_get_type(second, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_VOTER_TYPE_DREP_KEY_HASH);
  EXPECT_EQ(cardano_uplc_int_voter_at_ledger_index(procedures, 2U, &missing), CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);
  EXPECT_EQ(cardano_uplc_int_voter_at_ledger_index(NULL, 0U, &missing), CARDANO_ERROR_INDEX_OUT_OF_BOUNDS);
  EXPECT_EQ(cardano_uplc_int_voter_at_ledger_index(procedures, 0U, NULL), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(missing, nullptr);

  // Cleanup
  cardano_voter_unref(&second);
  cardano_voter_unref(&first);
  cardano_voting_procedures_unref(&procedures);
  cardano_transaction_body_unref(&body);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, voter_at_ledger_index_fails_when_listing_voters_fails)
{
  // Arrange
  cardano_transaction_t*       tx         = decode_tx(kTxGovernanceOrderCbor);
  cardano_transaction_body_t*  body       = cardano_transaction_get_body(tx);
  cardano_voting_procedures_t* procedures = cardano_transaction_body_get_voting_procedures(body);
  cardano_voter_t*             voter      = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_uplc_int_voter_at_ledger_index(procedures, 0U, &voter);

  cardano_set_allocators(malloc, realloc, free);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(voter, nullptr);

  // Cleanup
  cardano_voting_procedures_unref(&procedures);
  cardano_transaction_body_unref(&body);
  cardano_transaction_unref(&tx);
}

TEST(uplc_script_context, ledger_order_contexts_free_everything_on_allocation_failure)
{
  for (size_t position = 0U; position < 3U; ++position)
  {
    EXPECT_GT(sweep_context_allocations(kTxLedgerOrderCbor, position, 1), 0);
    EXPECT_GT(sweep_context_allocations(kTxLedgerOrderCbor, position, 2), 0);
    EXPECT_GT(sweep_context_allocations(kTxLedgerOrderCbor, position, 3), 0);
  }

  EXPECT_GT(sweep_context_allocations(kTxGovernanceOrderCbor, 1U, 3), 0);
  EXPECT_GT(sweep_context_allocations(kTxTreasuryOrderCbor, 0U, 3), 0);
}
