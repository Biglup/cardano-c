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

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>
#include <cardano/error.h>
#include <cardano/plutus_data/constr_plutus_data.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/plutus_data/plutus_map.h>
#include <cardano/slot_config.h>
#include <cardano/transaction/transaction.h>
#include <cardano/witness_set/redeemer.h>
#include <cardano/witness_set/redeemer_list.h>
#include <cardano/witness_set/witness_set.h>

#include "../../src/uplc/tx/script_context.h"

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
    "02ffa0d8799f582087aef86f0290a6cbcc910e3eccb2536e3b9d999b21b54b08e54d4fb49eefdbd0ffff";

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
