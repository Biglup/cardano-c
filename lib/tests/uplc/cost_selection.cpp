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

#include "cost_model_params_vectors.h"

#include <cstdint>
#include <cstring>
#include <gmock/gmock.h>

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

TEST(cardano_uplc_cost_model_version_for_language, v3_maps_to_cost_v3)
{
  EXPECT_EQ(
    cardano_uplc_cost_model_version_for_language(CARDANO_UPLC_LANG_VERSION_V3),
    CARDANO_UPLC_COST_MODEL_VERSION_V3);
}

TEST(cardano_uplc_cost_model_version_for_language, v4_maps_to_cost_v4)
{
  EXPECT_EQ(
    cardano_uplc_cost_model_version_for_language(CARDANO_UPLC_LANG_VERSION_V4),
    CARDANO_UPLC_COST_MODEL_VERSION_V4);
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
    cardano_uplc_cost_model_from_params(CARDANO_UPLC_COST_MODEL_VERSION_V1, V1_DEFAULT_PARAMS, CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V1, CARDANO_UPLC_SEMANTICS_A, &expected),
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
    cardano_uplc_cost_model_from_params(CARDANO_UPLC_COST_MODEL_VERSION_V3, V3_DEFAULT_PARAMS, CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3, CARDANO_UPLC_SEMANTICS_C, &expected),
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

TEST(cardano_uplc_select_cost_model, v1_dijkstra_selects_f)
{
  cardano_uplc_selected_cost_model_t selected;
  std::memset(&selected, 0, sizeof(selected));

  cardano_error_t result = cardano_uplc_select_cost_model(
    CARDANO_UPLC_LANG_VERSION_V1,
    12U,
    V1_DEFAULT_PARAMS,
    CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V1,
    &selected);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(selected.semantics, CARDANO_UPLC_SEMANTICS_F);
}

TEST(cardano_uplc_select_cost_model, v3_dijkstra_selects_g)
{
  cardano_uplc_selected_cost_model_t selected;
  std::memset(&selected, 0, sizeof(selected));

  cardano_error_t result = cardano_uplc_select_cost_model(
    CARDANO_UPLC_LANG_VERSION_V3,
    12U,
    V3_DEFAULT_PARAMS,
    CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3,
    &selected);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(selected.semantics, CARDANO_UPLC_SEMANTICS_G);
}

TEST(cardano_uplc_select_cost_model, v4_dijkstra_selects_g)
{
  cardano_uplc_selected_cost_model_t selected;
  std::memset(&selected, 0, sizeof(selected));

  cardano_error_t result = cardano_uplc_select_cost_model(
    CARDANO_UPLC_LANG_VERSION_V4,
    12U,
    V3_DEFAULT_PARAMS,
    CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3,
    &selected);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(selected.semantics, CARDANO_UPLC_SEMANTICS_G);
}

TEST(cardano_uplc_select_cost_model, v4_reads_the_v4_cost_model)
{
  cardano_uplc_selected_cost_model_t selected;
  std::memset(&selected, 0, sizeof(selected));

  ASSERT_EQ(
    cardano_uplc_select_cost_model(CARDANO_UPLC_LANG_VERSION_V4, 12U, V3_DEFAULT_PARAMS, CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V4, &selected),
    CARDANO_SUCCESS);

  cardano_uplc_cost_model_t expected;
  ASSERT_EQ(
    cardano_uplc_cost_model_from_params(CARDANO_UPLC_COST_MODEL_VERSION_V4, V3_DEFAULT_PARAMS, CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V4, CARDANO_UPLC_SEMANTICS_G, &expected),
    CARDANO_SUCCESS);
  EXPECT_EQ(std::memcmp(&selected.model, &expected, sizeof(expected)), 0);
}

TEST(cardano_uplc_select_cost_model, dijkstra_ledger_list_lengths_are_accepted)
{
  const struct
  {
      cardano_uplc_lang_version_t       language;
      cardano_uplc_cost_model_version_t version;
      const int64_t*                    params;
      size_t                            count;
  } cases[] = {
    { CARDANO_UPLC_LANG_VERSION_V1, CARDANO_UPLC_COST_MODEL_VERSION_V1, V1_DEFAULT_PARAMS, PARAM_COUNT_V1_WITHOUT_BATCH_7 },
    { CARDANO_UPLC_LANG_VERSION_V2, CARDANO_UPLC_COST_MODEL_VERSION_V2, V2_DEFAULT_PARAMS, PARAM_COUNT_V2_WITHOUT_BATCH_7 },
    { CARDANO_UPLC_LANG_VERSION_V3, CARDANO_UPLC_COST_MODEL_VERSION_V3, V3_DEFAULT_PARAMS, PARAM_COUNT_V3_WITHOUT_BATCH_7 },
    { CARDANO_UPLC_LANG_VERSION_V4, CARDANO_UPLC_COST_MODEL_VERSION_V4, V3_DEFAULT_PARAMS, PARAM_COUNT_V4_DIJKSTRA_GENESIS }
  };

  for (const auto& c: cases)
  {
    cardano_uplc_selected_cost_model_t selected;
    std::memset(&selected, 0, sizeof(selected));

    ASSERT_EQ(cardano_uplc_select_cost_model(c.language, 12U, c.params, c.count, &selected), CARDANO_SUCCESS);

    cardano_uplc_cost_model_t expected;
    ASSERT_EQ(cardano_uplc_cost_model_from_params(c.version, c.params, c.count, selected.semantics, &expected), CARDANO_SUCCESS);
    EXPECT_EQ(std::memcmp(&selected.model, &expected, sizeof(expected)), 0) << "language " << c.language;
  }
}

TEST(cardano_uplc_select_cost_model, v1_chang_keeps_the_cost_model_b_division_shapes)
{
  cardano_uplc_selected_cost_model_t selected;
  std::memset(&selected, 0, sizeof(selected));

  ASSERT_EQ(
    cardano_uplc_select_cost_model(CARDANO_UPLC_LANG_VERSION_V1, 10U, V1_DEFAULT_PARAMS, CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V1, &selected),
    CARDANO_SUCCESS);

  EXPECT_EQ(selected.model.builtins.entries[CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER].cpu.two.kind, CARDANO_UPLC_TWO_ARG_CONST_ABOVE_DIAGONAL);
  EXPECT_EQ(selected.model.builtins.entries[CARDANO_UPLC_BUILTIN_MOD_INTEGER].mem.two.kind, CARDANO_UPLC_TWO_ARG_SUBTRACTED_SIZES);
}

TEST(cardano_uplc_select_cost_model, v1_van_rossem_reads_division_with_cost_model_d_shapes)
{
  cardano_uplc_selected_cost_model_t selected;
  std::memset(&selected, 0, sizeof(selected));

  ASSERT_EQ(
    cardano_uplc_select_cost_model(CARDANO_UPLC_LANG_VERSION_V1, 11U, V1_DEFAULT_PARAMS, CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V1, &selected),
    CARDANO_SUCCESS);

  cardano_uplc_cost_model_t expected;
  ASSERT_EQ(
    cardano_uplc_cost_model_from_params(CARDANO_UPLC_COST_MODEL_VERSION_V1, V1_DEFAULT_PARAMS, CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V1, CARDANO_UPLC_SEMANTICS_D, &expected),
    CARDANO_SUCCESS);
  EXPECT_EQ(std::memcmp(&selected.model, &expected, sizeof(expected)), 0);
  EXPECT_EQ(selected.model.builtins.entries[CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER].cpu.two.kind, CARDANO_UPLC_TWO_ARG_ABOVE_AND_BELOW_DIAGONAL);
  EXPECT_EQ(selected.model.builtins.entries[CARDANO_UPLC_BUILTIN_MOD_INTEGER].mem.two.kind, CARDANO_UPLC_TWO_ARG_LINEAR_IN_Y2);
}

TEST(cardano_uplc_select_cost_model, v3_pre_van_rossem_keeps_the_cost_model_c_division_shapes)
{
  cardano_uplc_selected_cost_model_t selected;
  std::memset(&selected, 0, sizeof(selected));

  ASSERT_EQ(
    cardano_uplc_select_cost_model(CARDANO_UPLC_LANG_VERSION_V3, 10U, V3_DEFAULT_PARAMS, CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3, &selected),
    CARDANO_SUCCESS);

  EXPECT_EQ(selected.model.builtins.entries[CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER].cpu.two.kind, CARDANO_UPLC_TWO_ARG_CONST_ABOVE_DIAGONAL_INTO_QUADRATIC);
}

TEST(cardano_uplc_select_cost_model, v3_and_v4_from_van_rossem_read_division_with_cost_model_e_shapes)
{
  const cardano_uplc_lang_version_t languages[2] = { CARDANO_UPLC_LANG_VERSION_V3, CARDANO_UPLC_LANG_VERSION_V4 };
  const uint64_t                    protocols[2] = { 11U, 12U };

  for (size_t i = 0U; i < 2U; ++i)
  {
    for (size_t j = 0U; j < 2U; ++j)
    {
      cardano_uplc_selected_cost_model_t selected;
      std::memset(&selected, 0, sizeof(selected));

      ASSERT_EQ(
        cardano_uplc_select_cost_model(languages[i], protocols[j], V3_DEFAULT_PARAMS, CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3, &selected),
        CARDANO_SUCCESS);

      EXPECT_EQ(selected.model.builtins.entries[CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER].cpu.two.kind, CARDANO_UPLC_TWO_ARG_ABOVE_AND_BELOW_DIAGONAL);
      EXPECT_EQ(selected.model.builtins.entries[CARDANO_UPLC_BUILTIN_MOD_INTEGER].cpu.two.kind, CARDANO_UPLC_TWO_ARG_ABOVE_AND_BELOW_DIAGONAL);
      EXPECT_EQ(selected.model.builtins.entries[CARDANO_UPLC_BUILTIN_QUOTIENT_INTEGER].cpu.two.kind, CARDANO_UPLC_TWO_ARG_CONST_ABOVE_DIAGONAL_INTO_QUADRATIC);
      EXPECT_EQ(selected.model.builtins.entries[CARDANO_UPLC_BUILTIN_MOD_INTEGER].mem.two.kind, CARDANO_UPLC_TWO_ARG_LINEAR_IN_Y);
    }
  }
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
    cardano_uplc_cost_model_from_params(CARDANO_UPLC_COST_MODEL_VERSION_V3, V3_DEFAULT_PARAMS, CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3, CARDANO_UPLC_SEMANTICS_E, &expected),
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
