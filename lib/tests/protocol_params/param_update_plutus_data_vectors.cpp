/**
 * \file param_update_plutus_data_vectors.cpp
 *
 * Golden-vector validation of the protocol-parameter-update Plutus-data
 * encoding (the V3 ParameterChange changed-parameters map). The expected CBOR
 * was produced by, and verified byte-for-byte against, Aiken's reference
 * ProtocolParamUpdate::to_plutus_data; it is frozen here so the check is
 * self-contained and needs no external reference at test time.
 *
 * Cost models are excluded: aiken 1.1.21 leaves their ToPlutusData unimplemented;
 * that encoder is covered by the unit tests in protocol_param_update.cpp.
 */

#include <cardano/cardano.h>

#include <gmock/gmock.h>
#include <string>

// changed-parameters CBOR for the update built below, from Aiken's
// ProtocolParamUpdate::to_plutus_data (uplc 1.1.21).
static const char* kAikenChangedParamsCbor =
  "ab00182c099f030aff0a9f0105ff0b9f0104ff139f9f010aff9f011832ffff149f1903e81907d0ff159f191388"
  "191770ff1619138818199f9f0102ff9f0103ff9f0104ff9f0105ff9f0106ffff181a9f9f0102ff9f0103ff9f01"
  "04ff9f0105ff9f0106ff9f0107ff9f0108ff9f0109ff9f010aff9f010bffff181e1a05f5e100";

static cardano_unit_interval_t*
ui(uint64_t n, uint64_t d)
{
  cardano_unit_interval_t* v = nullptr;
  EXPECT_EQ(cardano_unit_interval_new(n, d, &v), CARDANO_SUCCESS);
  return v;
}

TEST(param_update_plutus_data, matchesAikenVector)
{
  cardano_protocol_param_update_t* u = nullptr;
  ASSERT_EQ(cardano_protocol_param_update_new(&u), CARDANO_SUCCESS);

  const uint64_t fee_a = 44U;
  const uint64_t mvs   = 5000U;
  const uint64_t dep   = 100000000U;
  ASSERT_EQ(cardano_protocol_param_update_set_min_fee_a(u, &fee_a), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_max_value_size(u, &mvs), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_governance_action_deposit(u, &dep), CARDANO_SUCCESS);

  cardano_unit_interval_t* a0  = ui(3U, 10U);
  cardano_unit_interval_t* rho = ui(2U, 10U); // reduces to 1/5
  cardano_unit_interval_t* tau = ui(1U, 4U);
  ASSERT_EQ(cardano_protocol_param_update_set_pool_pledge_influence(u, a0), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_expansion_rate(u, rho), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_treasury_growth_rate(u, tau), CARDANO_SUCCESS);

  cardano_ex_units_t* tx_ex = nullptr;
  cardano_ex_units_t* bl_ex = nullptr;
  ASSERT_EQ(cardano_ex_units_new(1000U, 2000U, &tx_ex), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_ex_units_new(5000U, 6000U, &bl_ex), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_max_tx_ex_units(u, tx_ex), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_max_block_ex_units(u, bl_ex), CARDANO_SUCCESS);

  cardano_unit_interval_t*  pm     = ui(1U, 10U);
  cardano_unit_interval_t*  ps     = ui(2U, 100U); // reduces to 1/50
  cardano_ex_unit_prices_t* prices = nullptr;
  ASSERT_EQ(cardano_ex_unit_prices_new(pm, ps, &prices), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_execution_costs(u, prices), CARDANO_SUCCESS);

  cardano_pool_voting_thresholds_t* pvt = nullptr;
  cardano_unit_interval_t*          p1  = ui(1U, 2U);
  cardano_unit_interval_t*          p2  = ui(1U, 3U);
  cardano_unit_interval_t*          p3  = ui(1U, 4U);
  cardano_unit_interval_t*          p4  = ui(1U, 5U);
  cardano_unit_interval_t*          p5  = ui(1U, 6U);
  ASSERT_EQ(cardano_pool_voting_thresholds_new(p1, p2, p3, p4, p5, &pvt), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_pool_voting_thresholds(u, pvt), CARDANO_SUCCESS);

  cardano_drep_voting_thresholds_t* dvt = nullptr;
  cardano_unit_interval_t*          d1  = ui(1U, 2U);
  cardano_unit_interval_t*          d2  = ui(1U, 3U);
  cardano_unit_interval_t*          d3  = ui(1U, 4U);
  cardano_unit_interval_t*          d4  = ui(1U, 5U);
  cardano_unit_interval_t*          d5  = ui(1U, 6U);
  cardano_unit_interval_t*          d6  = ui(1U, 7U);
  cardano_unit_interval_t*          d7  = ui(1U, 8U);
  cardano_unit_interval_t*          d8  = ui(1U, 9U);
  cardano_unit_interval_t*          d9  = ui(1U, 10U);
  cardano_unit_interval_t*          d10 = ui(1U, 11U);
  ASSERT_EQ(cardano_drep_voting_thresholds_new(d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, &dvt), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_drep_voting_thresholds(u, dvt), CARDANO_SUCCESS);

  cardano_plutus_data_t* pd = nullptr;
  ASSERT_EQ(cardano_protocol_param_update_to_plutus_data(u, &pd), CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  ASSERT_EQ(cardano_plutus_data_to_cbor(pd, writer), CARDANO_SUCCESS);

  cardano_buffer_t* buffer = nullptr;
  ASSERT_EQ(cardano_cbor_writer_encode_in_buffer(writer, &buffer), CARDANO_SUCCESS);

  const size_t size           = cardano_buffer_get_hex_size(buffer);
  char         ours_hex[1024] = { 0 };
  ASSERT_LE(size, sizeof(ours_hex));
  ASSERT_EQ(cardano_buffer_to_hex(buffer, ours_hex, size), CARDANO_SUCCESS);

  EXPECT_EQ(std::string(ours_hex), std::string(kAikenChangedParamsCbor));

  cardano_buffer_unref(&buffer);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&pd);
  cardano_drep_voting_thresholds_unref(&dvt);
  cardano_pool_voting_thresholds_unref(&pvt);
  cardano_ex_unit_prices_unref(&prices);
  cardano_ex_units_unref(&bl_ex);
  cardano_ex_units_unref(&tx_ex);
  cardano_unit_interval_unref(&a0);
  cardano_unit_interval_unref(&rho);
  cardano_unit_interval_unref(&tau);
  cardano_unit_interval_unref(&pm);
  cardano_unit_interval_unref(&ps);
  cardano_unit_interval_unref(&p1);
  cardano_unit_interval_unref(&p2);
  cardano_unit_interval_unref(&p3);
  cardano_unit_interval_unref(&p4);
  cardano_unit_interval_unref(&p5);
  cardano_unit_interval_unref(&d1);
  cardano_unit_interval_unref(&d2);
  cardano_unit_interval_unref(&d3);
  cardano_unit_interval_unref(&d4);
  cardano_unit_interval_unref(&d5);
  cardano_unit_interval_unref(&d6);
  cardano_unit_interval_unref(&d7);
  cardano_unit_interval_unref(&d8);
  cardano_unit_interval_unref(&d9);
  cardano_unit_interval_unref(&d10);
  cardano_protocol_param_update_unref(&u);
}

// changed-parameters CBOR for the Dijkstra reference script parameters (tags
// 34-37), following the same encoding scheme: tag -> I value for the sizes and
// the stride, tag -> gcd-reduced rational for the multiplier (12/10 -> 6/5).
static const char* kDijkstraChangedParamsCbor =
  "a418221a00030d4018231a000186a018241961a818259f0605ff";

TEST(param_update_plutus_data, matchesDijkstraRefScriptVector)
{
  cardano_protocol_param_update_t* u = nullptr;
  ASSERT_EQ(cardano_protocol_param_update_new(&u), CARDANO_SUCCESS);

  const uint64_t per_block = 200000U;
  const uint64_t per_tx    = 100000U;
  const uint64_t stride    = 25000U;
  ASSERT_EQ(cardano_protocol_param_update_set_max_ref_script_size_per_block(u, &per_block), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_max_ref_script_size_per_tx(u, &per_tx), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_ref_script_cost_stride(u, &stride), CARDANO_SUCCESS);

  cardano_unit_interval_t* multiplier = ui(12U, 10U); // reduces to 6/5
  ASSERT_EQ(cardano_protocol_param_update_set_ref_script_cost_multiplier(u, multiplier), CARDANO_SUCCESS);

  cardano_plutus_data_t* pd = nullptr;
  ASSERT_EQ(cardano_protocol_param_update_to_plutus_data(u, &pd), CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  ASSERT_EQ(cardano_plutus_data_to_cbor(pd, writer), CARDANO_SUCCESS);

  cardano_buffer_t* buffer = nullptr;
  ASSERT_EQ(cardano_cbor_writer_encode_in_buffer(writer, &buffer), CARDANO_SUCCESS);

  const size_t size          = cardano_buffer_get_hex_size(buffer);
  char         ours_hex[256] = { 0 };
  ASSERT_LE(size, sizeof(ours_hex));
  ASSERT_EQ(cardano_buffer_to_hex(buffer, ours_hex, size), CARDANO_SUCCESS);

  EXPECT_EQ(std::string(ours_hex), std::string(kDijkstraChangedParamsCbor));

  cardano_buffer_unref(&buffer);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&pd);
  cardano_unit_interval_unref(&multiplier);
  cardano_protocol_param_update_unref(&u);
}

// changed-parameters CBOR for the Dijkstra pledge leverage, pool margin and
// Leios parameters (tags 38-48), following the same encoding scheme and the
// ledger's StrictMaybe encoding for the nullable tag 38 (Constr 0 [x] when a
// bound is proposed, Constr 1 [] when the bound is lifted). Derived by hand:
//
//   ab                                map(11)
//   1826 d879 9f 9f 03 02 ff ff       38 -> Constr 0 [[3, 2]]     (6/4 -> 3/2)
//   1827 9f 01 14 ff                  39 -> [1, 20]               (5/100 -> 1/20)
//   1828 19 03e8                      40 -> 1000
//   1829 19 07d0                      41 -> 2000
//   182a 19 0bb8                      42 -> 3000
//   182b 19 01f4                      43 -> 500
//   182c 9f 03 05 ff                  44 -> [3, 5]                (6/10 -> 3/5)
//   182d 1a 00010000                  45 -> 65536
//   182e 1a 000186a0                  46 -> 100000
//   182f 9f 1a 00d59f80 1b 00000002540be400 ff
//                                     47 -> [14000000, 10000000000]
//   1830 1a 00032000                  48 -> 204800
//
// Constr alternatives 0 and 1 use the compact CBOR tags 121 (d879) and 122
// (d87a); lists are indefinite length (9f ... ff) except the empty field list
// of Constr 1, which is written as the definite empty array 80.
static const char* kDijkstraLeiosChangedParamsCbor =
  "ab1826d8799f9f0302ffff18279f0114ff18281903e818291907d0182a190bb8182b1901f4182c9f0305ff"
  "182d1a00010000182e1a000186a0182f9f1a00d59f801b00000002540be400ff18301a00032000";

// Same update with tag 38 proposing an unbounded pledge leverage:
//
//   1826 d87a 80                      38 -> Constr 1 []
static const char* kDijkstraLeiosUnboundedChangedParamsCbor =
  "ab1826d87a8018279f0114ff18281903e818291907d0182a190bb8182b1901f4182c9f0305ff"
  "182d1a00010000182e1a000186a0182f9f1a00d59f801b00000002540be400ff18301a00032000";

static void
set_leios_params(cardano_protocol_param_update_t* u, cardano_unit_interval_t* margin, cardano_unit_interval_t* quorum, cardano_ex_units_t* units)
{
  const uint64_t announcement = 1000U;
  const uint64_t vote         = 2000U;
  const uint64_t diffusion    = 3000U;
  const uint64_t committee    = 500U;
  const uint64_t references   = 65536U;
  const uint64_t txs          = 100000U;
  const uint64_t ref_scripts  = 204800U;

  ASSERT_EQ(cardano_protocol_param_update_set_min_pool_margin(u, margin), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_leios_announcement_period_length(u, &announcement), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_leios_vote_period_length(u, &vote), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_leios_diffusion_period_length(u, &diffusion), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_leios_committee_size(u, &committee), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_leios_quorum_stake_threshold(u, quorum), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_max_endorser_block_references_size(u, &references), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_max_endorser_block_txs_size(u, &txs), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_max_endorser_block_execution_units(u, units), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_protocol_param_update_set_max_ref_script_size_per_endorser_block(u, &ref_scripts), CARDANO_SUCCESS);
}

static std::string
changed_params_hex(cardano_protocol_param_update_t* u)
{
  cardano_plutus_data_t* pd = nullptr;
  EXPECT_EQ(cardano_protocol_param_update_to_plutus_data(u, &pd), CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_EQ(cardano_plutus_data_to_cbor(pd, writer), CARDANO_SUCCESS);

  cardano_buffer_t* buffer = nullptr;
  EXPECT_EQ(cardano_cbor_writer_encode_in_buffer(writer, &buffer), CARDANO_SUCCESS);

  const size_t size          = cardano_buffer_get_hex_size(buffer);
  char         ours_hex[256] = { 0 };
  EXPECT_LE(size, sizeof(ours_hex));
  EXPECT_EQ(cardano_buffer_to_hex(buffer, ours_hex, size), CARDANO_SUCCESS);

  cardano_buffer_unref(&buffer);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_data_unref(&pd);

  return std::string(ours_hex);
}

TEST(param_update_plutus_data, matchesDijkstraLeiosVector)
{
  cardano_protocol_param_update_t* u = nullptr;
  ASSERT_EQ(cardano_protocol_param_update_new(&u), CARDANO_SUCCESS);

  cardano_unit_interval_t* leverage = ui(6U, 4U);   // reduces to 3/2
  cardano_unit_interval_t* margin   = ui(5U, 100U); // reduces to 1/20
  cardano_unit_interval_t* quorum   = ui(6U, 10U);  // reduces to 3/5
  cardano_ex_units_t*      units    = nullptr;
  ASSERT_EQ(cardano_ex_units_new(14000000U, 10000000000U, &units), CARDANO_SUCCESS);

  ASSERT_EQ(cardano_protocol_param_update_set_max_pledge_leverage(u, leverage), CARDANO_SUCCESS);
  set_leios_params(u, margin, quorum, units);

  EXPECT_EQ(changed_params_hex(u), std::string(kDijkstraLeiosChangedParamsCbor));

  cardano_ex_units_unref(&units);
  cardano_unit_interval_unref(&quorum);
  cardano_unit_interval_unref(&margin);
  cardano_unit_interval_unref(&leverage);
  cardano_protocol_param_update_unref(&u);
}

TEST(param_update_plutus_data, matchesDijkstraLeiosUnboundedLeverageVector)
{
  cardano_protocol_param_update_t* u = nullptr;
  ASSERT_EQ(cardano_protocol_param_update_new(&u), CARDANO_SUCCESS);

  cardano_unit_interval_t* margin = ui(5U, 100U); // reduces to 1/20
  cardano_unit_interval_t* quorum = ui(6U, 10U);  // reduces to 3/5
  cardano_ex_units_t*      units  = nullptr;
  ASSERT_EQ(cardano_ex_units_new(14000000U, 10000000000U, &units), CARDANO_SUCCESS);

  ASSERT_EQ(cardano_protocol_param_update_set_max_pledge_leverage_unbounded(u), CARDANO_SUCCESS);
  set_leios_params(u, margin, quorum, units);

  EXPECT_EQ(changed_params_hex(u), std::string(kDijkstraLeiosUnboundedChangedParamsCbor));

  cardano_ex_units_unref(&units);
  cardano_unit_interval_unref(&quorum);
  cardano_unit_interval_unref(&margin);
  cardano_protocol_param_update_unref(&u);
}
