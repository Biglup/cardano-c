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
