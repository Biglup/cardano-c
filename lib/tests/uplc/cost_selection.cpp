/**
 * \file cost_selection.cpp
 *
 * \author angel.castillo
 * \date   Jun 20, 2026
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

#include <cardano/error.h>
#include <cardano/slot_config.h>

#include "../../src/uplc/cost/uplc_selected_cost_model.h"
#include "../../src/uplc/tx/script_context.h"

#include <cstdint>
#include <cstring>
#include <gmock/gmock.h>

/* COST MODEL PARAMETER FIXTURES *********************************************/

// The mainnet 2024-09-29 V1 flat parameter vector (reproduces builtin_costs_v1).
static const int64_t V1_DEFAULT_PARAMS[] = {
  100788,
  420,
  1,
  1,
  1000,
  173,
  0,
  1,
  1000,
  59957,
  4,
  1,
  11183,
  32,
  201305,
  8356,
  4,
  16000,
  100,
  16000,
  100,
  16000,
  100,
  16000,
  100,
  16000,
  100,
  16000,
  100,
  100,
  100,
  16000,
  100,
  94375,
  32,
  132994,
  32,
  61462,
  4,
  72010,
  178,
  0,
  1,
  22151,
  32,
  91189,
  769,
  4,
  2,
  85848,
  228465,
  122,
  0,
  1,
  1,
  1000,
  42921,
  4,
  2,
  24548,
  29498,
  38,
  1,
  898148,
  27279,
  1,
  51775,
  558,
  1,
  39184,
  1000,
  60594,
  1,
  141895,
  32,
  83150,
  32,
  15299,
  32,
  76049,
  1,
  13169,
  4,
  22100,
  10,
  28999,
  74,
  1,
  28999,
  74,
  1,
  43285,
  552,
  1,
  44749,
  541,
  1,
  33852,
  32,
  68246,
  32,
  72362,
  32,
  7243,
  32,
  7391,
  32,
  11546,
  32,
  85848,
  228465,
  122,
  0,
  1,
  1,
  90434,
  519,
  0,
  1,
  74433,
  32,
  85848,
  228465,
  122,
  0,
  1,
  1,
  85848,
  228465,
  122,
  0,
  1,
  1,
  270652,
  22588,
  4,
  1457325,
  64566,
  4,
  20467,
  1,
  4,
  0,
  141992,
  32,
  100788,
  420,
  1,
  1,
  81663,
  32,
  59498,
  32,
  20142,
  32,
  24588,
  32,
  20744,
  32,
  25933,
  32,
  24623,
  32,
  53384111,
  14333,
  10,
  955506,
  213312,
  0,
  2,
  43053543,
  10,
  43574283,
  26308,
  10,
  16000,
  100,
  16000,
  100,
  962335,
  18,
  2780678,
  6,
  442008,
  1,
  52538055,
  3756,
  18,
  267929,
  18,
  76433006,
  8868,
  18,
  52948122,
  18,
  1995836,
  36,
  3227919,
  12,
  901022,
  1,
  166917843,
  4307,
  36,
  284546,
  36,
  158221314,
  26549,
  36,
  74698472,
  36,
  333849714,
  1,
  254006273,
  72,
  2174038,
  72,
  2261318,
  64571,
  4,
  207616,
  8310,
  4,
  1293828,
  28716,
  63,
  0,
  1,
  1006041,
  43623,
  251,
  0,
  1,
  100181,
  726,
  719,
  0,
  1,
  100181,
  726,
  719,
  0,
  1,
  100181,
  726,
  719,
  0,
  1,
  107878,
  680,
  0,
  1,
  95336,
  1,
  281145,
  18848,
  0,
  1,
  180194,
  159,
  1,
  1,
  158519,
  8942,
  0,
  1,
  159378,
  8813,
  0,
  1,
  107490,
  3298,
  1,
  106057,
  655,
  1,
  1964219,
  24520,
  3,
  607153,
  231697,
  53144,
  0,
  1,
  116711,
  1957,
  4,
  231883,
  10,
  1000,
  24838,
  7,
  1,
  232010,
  32,
  321837444,
  25087669,
  18,
  617887431,
  67302824,
  36,
  356924,
  18413,
  45,
  21,
  219951,
  9444,
  1,
  1000,
  172116,
  183150,
  6,
  24,
  21,
  213283,
  618401,
  1998,
  28258,
  1,
  1000,
  38159,
  2,
  22,
  1000,
  95933,
  1,
  1,
  11,
  1000,
  277577,
  12,
  21
};

// The preprod 2024-11-22 V3 flat parameter vector (reproduces builtin_costs_v3).
static const int64_t V3_DEFAULT_PARAMS[] = {
  100788,
  420,
  1,
  1,
  1000,
  173,
  0,
  1,
  1000,
  59957,
  4,
  1,
  11183,
  32,
  201305,
  8356,
  4,
  16000,
  100,
  16000,
  100,
  16000,
  100,
  16000,
  100,
  16000,
  100,
  16000,
  100,
  100,
  100,
  16000,
  100,
  94375,
  32,
  132994,
  32,
  61462,
  4,
  72010,
  178,
  0,
  1,
  22151,
  32,
  91189,
  769,
  4,
  2,
  85848,
  123203,
  7305,
  -900,
  1716,
  549,
  57,
  85848,
  0,
  1,
  1,
  1000,
  42921,
  4,
  2,
  24548,
  29498,
  38,
  1,
  898148,
  27279,
  1,
  51775,
  558,
  1,
  39184,
  1000,
  60594,
  1,
  141895,
  32,
  83150,
  32,
  15299,
  32,
  76049,
  1,
  13169,
  4,
  22100,
  10,
  28999,
  74,
  1,
  28999,
  74,
  1,
  43285,
  552,
  1,
  44749,
  541,
  1,
  33852,
  32,
  68246,
  32,
  72362,
  32,
  7243,
  32,
  7391,
  32,
  11546,
  32,
  85848,
  123203,
  7305,
  -900,
  1716,
  549,
  57,
  85848,
  0,
  1,
  90434,
  519,
  0,
  1,
  74433,
  32,
  85848,
  123203,
  7305,
  -900,
  1716,
  549,
  57,
  85848,
  0,
  1,
  1,
  85848,
  123203,
  7305,
  -900,
  1716,
  549,
  57,
  85848,
  0,
  1,
  955506,
  213312,
  0,
  2,
  270652,
  22588,
  4,
  1457325,
  64566,
  4,
  20467,
  1,
  4,
  0,
  141992,
  32,
  100788,
  420,
  1,
  1,
  81663,
  32,
  59498,
  32,
  20142,
  32,
  24588,
  32,
  20744,
  32,
  25933,
  32,
  24623,
  32,
  43053543,
  10,
  53384111,
  14333,
  10,
  43574283,
  26308,
  10,
  16000,
  100,
  16000,
  100,
  962335,
  18,
  2780678,
  6,
  442008,
  1,
  52538055,
  3756,
  18,
  267929,
  18,
  76433006,
  8868,
  18,
  52948122,
  18,
  1995836,
  36,
  3227919,
  12,
  901022,
  1,
  166917843,
  4307,
  36,
  284546,
  36,
  158221314,
  26549,
  36,
  74698472,
  36,
  333849714,
  1,
  254006273,
  72,
  2174038,
  72,
  2261318,
  64571,
  4,
  207616,
  8310,
  4,
  1293828,
  28716,
  63,
  0,
  1,
  1006041,
  43623,
  251,
  0,
  1,
  100181,
  726,
  719,
  0,
  1,
  100181,
  726,
  719,
  0,
  1,
  100181,
  726,
  719,
  0,
  1,
  107878,
  680,
  0,
  1,
  95336,
  1,
  281145,
  18848,
  0,
  1,
  180194,
  159,
  1,
  1,
  158519,
  8942,
  0,
  1,
  159378,
  8813,
  0,
  1,
  107490,
  3298,
  1,
  106057,
  655,
  1,
  1964219,
  24520,
  3,
  607153,
  231697,
  53144,
  0,
  1,
  116711,
  1957,
  4,
  231883,
  10,
  1000,
  24838,
  7,
  1,
  232010,
  32,
  321837444,
  25087669,
  18,
  617887431,
  67302824,
  36,
  356924,
  18413,
  45,
  21,
  219951,
  9444,
  1,
  1000,
  172116,
  183150,
  6,
  24,
  21,
  213283,
  618401,
  1998,
  28258,
  1,
  1000,
  38159,
  2,
  22,
  1000,
  95933,
  1,
  1,
  11,
  1000,
  277577,
  12,
  21
};

/* SLOT TO POSIX TIME ********************************************************/

TEST(cardano_uplc_int_slot_to_posix_time, mainnet_zero_slot_maps_to_zero_time)
{
  uint64_t        time   = 0U;
  cardano_error_t result = cardano_uplc_int_slot_to_posix_time(&CARDANO_MAINNET_SLOT_CONFIG, 4492800U, &time);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(time, 1596059091000ULL);
}

TEST(cardano_uplc_int_slot_to_posix_time, mainnet_one_slot_after_origin)
{
  uint64_t        time   = 0U;
  cardano_error_t result = cardano_uplc_int_slot_to_posix_time(&CARDANO_MAINNET_SLOT_CONFIG, 4492801U, &time);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(time, 1596059092000ULL);
}

TEST(cardano_uplc_int_slot_to_posix_time, mainnet_far_slot)
{
  uint64_t        time   = 0U;
  cardano_error_t result = cardano_uplc_int_slot_to_posix_time(&CARDANO_MAINNET_SLOT_CONFIG, 5000000U, &time);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(time, 1596566291000ULL);
}

TEST(cardano_uplc_int_slot_to_posix_time, preview_origin_and_offset)
{
  uint64_t        time   = 0U;
  cardano_error_t origin = cardano_uplc_int_slot_to_posix_time(&CARDANO_PREVIEW_SLOT_CONFIG, 0U, &time);

  EXPECT_EQ(origin, CARDANO_SUCCESS);
  EXPECT_EQ(time, 1666656000000ULL);

  cardano_error_t offset = cardano_uplc_int_slot_to_posix_time(&CARDANO_PREVIEW_SLOT_CONFIG, 100U, &time);

  EXPECT_EQ(offset, CARDANO_SUCCESS);
  EXPECT_EQ(time, 1666656100000ULL);
}

TEST(cardano_uplc_int_slot_to_posix_time, preprod_origin_and_offset)
{
  uint64_t        time   = 0U;
  cardano_error_t origin = cardano_uplc_int_slot_to_posix_time(&CARDANO_PREPROD_SLOT_CONFIG, 86400U, &time);

  EXPECT_EQ(origin, CARDANO_SUCCESS);
  EXPECT_EQ(time, 1655769600000ULL);

  cardano_error_t offset = cardano_uplc_int_slot_to_posix_time(&CARDANO_PREPROD_SLOT_CONFIG, 86500U, &time);

  EXPECT_EQ(offset, CARDANO_SUCCESS);
  EXPECT_EQ(time, 1655769700000ULL);
}

TEST(cardano_uplc_int_slot_to_posix_time, matches_aiken_test_slot_config)
{
  // The slot config used across aiken/crates/uplc/src/tx/tests.rs:
  // zero_time 1660003200000, zero_slot 0, slot_length 1000.
  cardano_slot_config_t cfg  = { 1660003200000ULL, 0U, 1000U };
  uint64_t              time = 0U;

  cardano_error_t result = cardano_uplc_int_slot_to_posix_time(&cfg, 1000U, &time);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(time, 1660004200000ULL);
}

TEST(cardano_uplc_int_slot_to_posix_time, slot_before_zero_slot_is_rejected)
{
  uint64_t        time   = 123U;
  cardano_error_t result = cardano_uplc_int_slot_to_posix_time(&CARDANO_MAINNET_SLOT_CONFIG, 4492799U, &time);

  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(time, 123U);
}

TEST(cardano_uplc_int_slot_to_posix_time, null_slot_config_is_rejected)
{
  uint64_t        time   = 0U;
  cardano_error_t result = cardano_uplc_int_slot_to_posix_time(nullptr, 0U, &time);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_uplc_int_slot_to_posix_time, null_out_is_rejected)
{
  cardano_error_t result = cardano_uplc_int_slot_to_posix_time(&CARDANO_MAINNET_SLOT_CONFIG, 4492800U, nullptr);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

/* COST MODEL VERSION FOR LANGUAGE *******************************************/

TEST(cardano_uplc_cost_model_version_for_language, v1_maps_to_cost_v1)
{
  EXPECT_EQ(
    cardano_uplc_cost_model_version_for_language(CARDANO_UPLC_LANG_VERSION_V1),
    CARDANO_UPLC_COST_MODEL_VERSION_V1);
}

TEST(cardano_uplc_cost_model_version_for_language, v2_maps_to_cost_v2)
{
  EXPECT_EQ(
    cardano_uplc_cost_model_version_for_language(CARDANO_UPLC_LANG_VERSION_V2),
    CARDANO_UPLC_COST_MODEL_VERSION_V2);
}

TEST(cardano_uplc_cost_model_version_for_language, v3_and_v4_map_to_cost_v3)
{
  EXPECT_EQ(
    cardano_uplc_cost_model_version_for_language(CARDANO_UPLC_LANG_VERSION_V3),
    CARDANO_UPLC_COST_MODEL_VERSION_V3);
  EXPECT_EQ(
    cardano_uplc_cost_model_version_for_language(CARDANO_UPLC_LANG_VERSION_V4),
    CARDANO_UPLC_COST_MODEL_VERSION_V3);
}

/* COMBINED COST MODEL AND SEMANTICS SELECTION *******************************/

/* The semantics-selection boundaries are covered directly in cost_model.cpp;
 * the select-cost-model cases below assert that the combined selector returns
 * the same variant alongside the right cost model. */

TEST(cardano_uplc_select_cost_model, v1_pre_chang_selects_a_and_v1_model)
{
  cardano_uplc_selected_cost_model_t selected;
  std::memset(&selected, 0, sizeof(selected));

  cardano_error_t result = cardano_uplc_select_cost_model(
    CARDANO_UPLC_LANG_VERSION_V1,
    8U,
    V1_DEFAULT_PARAMS,
    CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V1,
    &selected);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(selected.semantics, CARDANO_UPLC_SEMANTICS_A);

  cardano_uplc_cost_model_t expected;
  ASSERT_EQ(
    cardano_uplc_cost_model_from_params(CARDANO_UPLC_COST_MODEL_VERSION_V1, V1_DEFAULT_PARAMS, CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V1, &expected),
    CARDANO_SUCCESS);
  EXPECT_EQ(std::memcmp(&selected.model, &expected, sizeof(expected)), 0);
}

TEST(cardano_uplc_select_cost_model, v1_chang_selects_b)
{
  cardano_uplc_selected_cost_model_t selected;
  std::memset(&selected, 0, sizeof(selected));

  cardano_error_t result = cardano_uplc_select_cost_model(
    CARDANO_UPLC_LANG_VERSION_V1,
    9U,
    V1_DEFAULT_PARAMS,
    CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V1,
    &selected);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(selected.semantics, CARDANO_UPLC_SEMANTICS_B);
}

TEST(cardano_uplc_select_cost_model, v1_van_rossem_selects_d)
{
  cardano_uplc_selected_cost_model_t selected;
  std::memset(&selected, 0, sizeof(selected));

  cardano_error_t result = cardano_uplc_select_cost_model(
    CARDANO_UPLC_LANG_VERSION_V1,
    11U,
    V1_DEFAULT_PARAMS,
    CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V1,
    &selected);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(selected.semantics, CARDANO_UPLC_SEMANTICS_D);
}

TEST(cardano_uplc_select_cost_model, v3_pre_van_rossem_selects_c_and_v3_model)
{
  cardano_uplc_selected_cost_model_t selected;
  std::memset(&selected, 0, sizeof(selected));

  cardano_error_t result = cardano_uplc_select_cost_model(
    CARDANO_UPLC_LANG_VERSION_V3,
    10U,
    V3_DEFAULT_PARAMS,
    CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3,
    &selected);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(selected.semantics, CARDANO_UPLC_SEMANTICS_C);

  cardano_uplc_cost_model_t expected;
  ASSERT_EQ(
    cardano_uplc_cost_model_from_params(CARDANO_UPLC_COST_MODEL_VERSION_V3, V3_DEFAULT_PARAMS, CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3, &expected),
    CARDANO_SUCCESS);
  EXPECT_EQ(std::memcmp(&selected.model, &expected, sizeof(expected)), 0);
}

TEST(cardano_uplc_select_cost_model, v3_van_rossem_selects_e)
{
  cardano_uplc_selected_cost_model_t selected;
  std::memset(&selected, 0, sizeof(selected));

  cardano_error_t result = cardano_uplc_select_cost_model(
    CARDANO_UPLC_LANG_VERSION_V3,
    11U,
    V3_DEFAULT_PARAMS,
    CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3,
    &selected);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(selected.semantics, CARDANO_UPLC_SEMANTICS_E);
}

TEST(cardano_uplc_select_cost_model, extra_trailing_params_are_ignored)
{
  int64_t longer[CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3 + 2U];
  std::memcpy(longer, V3_DEFAULT_PARAMS, CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3 * sizeof(int64_t));
  longer[CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3]      = 12345;
  longer[CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3 + 1U] = 67890;

  cardano_uplc_selected_cost_model_t selected;
  std::memset(&selected, 0, sizeof(selected));

  cardano_error_t result = cardano_uplc_select_cost_model(
    CARDANO_UPLC_LANG_VERSION_V3,
    11U,
    longer,
    CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3 + 2U,
    &selected);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_uplc_cost_model_t expected;
  ASSERT_EQ(
    cardano_uplc_cost_model_from_params(CARDANO_UPLC_COST_MODEL_VERSION_V3, V3_DEFAULT_PARAMS, CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3, &expected),
    CARDANO_SUCCESS);
  EXPECT_EQ(std::memcmp(&selected.model, &expected, sizeof(expected)), 0);
}

TEST(cardano_uplc_select_cost_model, fewer_params_than_known_is_tolerated)
{
  cardano_uplc_selected_cost_model_t selected;
  std::memset(&selected, 0, sizeof(selected));

  cardano_error_t result = cardano_uplc_select_cost_model(
    CARDANO_UPLC_LANG_VERSION_V1,
    9U,
    V1_DEFAULT_PARAMS,
    CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V1 - 1U,
    &selected);

  EXPECT_EQ(result, CARDANO_SUCCESS);
}

TEST(cardano_uplc_select_cost_model, null_params_is_rejected)
{
  cardano_uplc_selected_cost_model_t selected;

  cardano_error_t result = cardano_uplc_select_cost_model(
    CARDANO_UPLC_LANG_VERSION_V3,
    11U,
    nullptr,
    CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3,
    &selected);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_uplc_select_cost_model, null_out_is_rejected)
{
  cardano_error_t result = cardano_uplc_select_cost_model(
    CARDANO_UPLC_LANG_VERSION_V3,
    11U,
    V3_DEFAULT_PARAMS,
    CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3,
    nullptr);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}
