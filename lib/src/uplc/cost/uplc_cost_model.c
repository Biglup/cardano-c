/**
 * \file uplc_cost_model.c
 *
 * \author angel.castillo
 * \date   Jun 27, 2026
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

#include "uplc_cost_model.h"
#include "../builtins/uplc_builtin.h"
#include "uplc_builtin_costs.h"
#include "uplc_machine_costs.h"

#include <stddef.h>
#include <stdint.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Overwrites the two coefficients of a one-argument linear cpu and mem.
 *
 * \param[in,out] e The entry whose one-argument linear functions are set.
 * \param[in] p The parameter array.
 * \param[in] cpu_i The intercept index of the cpu function.
 * \param[in] cpu_s The slope index of the cpu function.
 * \param[in] mem_i The intercept index of the mem function.
 * \param[in] mem_s The slope index of the mem function.
 */
static void
set_one_lin(cardano_uplc_builtin_cost_t* e, const int64_t* p, size_t cpu_i, size_t cpu_s, size_t mem_i, size_t mem_s)
{
  e->cpu.one.params.linear.intercept = p[cpu_i];
  e->cpu.one.params.linear.slope     = p[cpu_s];
  e->mem.one.params.linear.intercept = p[mem_i];
  e->mem.one.params.linear.slope     = p[mem_s];
}

/**
 * \brief Overwrites a one-argument linear cpu and a constant mem.
 *
 * \param[in,out] e The entry to set.
 * \param[in] p The parameter array.
 * \param[in] cpu_i The cpu intercept index.
 * \param[in] cpu_s The cpu slope index.
 * \param[in] mem_c The mem constant index.
 */
static void
set_one_lin_cpu_const_mem(cardano_uplc_builtin_cost_t* e, const int64_t* p, size_t cpu_i, size_t cpu_s, size_t mem_c)
{
  e->cpu.one.params.linear.intercept = p[cpu_i];
  e->cpu.one.params.linear.slope     = p[cpu_s];
  e->mem.one.params.constant         = p[mem_c];
}

/**
 * \brief Overwrites a one-argument constant cpu and mem.
 *
 * \param[in,out] e The entry to set.
 * \param[in] p The parameter array.
 * \param[in] cpu_c The cpu constant index.
 * \param[in] mem_c The mem constant index.
 */
static void
set_one_const(cardano_uplc_builtin_cost_t* e, const int64_t* p, size_t cpu_c, size_t mem_c)
{
  e->cpu.one.params.constant = p[cpu_c];
  e->mem.one.params.constant = p[mem_c];
}

/**
 * \brief Overwrites a two-argument linear cpu and mem (both plain linear arms).
 *
 * \param[in,out] e The entry to set.
 * \param[in] p The parameter array.
 * \param[in] cpu_i The cpu intercept index.
 * \param[in] cpu_s The cpu slope index.
 * \param[in] mem_i The mem intercept index.
 * \param[in] mem_s The mem slope index.
 */
static void
set_two_lin(cardano_uplc_builtin_cost_t* e, const int64_t* p, size_t cpu_i, size_t cpu_s, size_t mem_i, size_t mem_s)
{
  e->cpu.two.params.linear.intercept = p[cpu_i];
  e->cpu.two.params.linear.slope     = p[cpu_s];
  e->mem.two.params.linear.intercept = p[mem_i];
  e->mem.two.params.linear.slope     = p[mem_s];
}

/**
 * \brief Overwrites a two-argument constant cpu and mem.
 *
 * \param[in,out] e The entry to set.
 * \param[in] p The parameter array.
 * \param[in] cpu_c The cpu constant index.
 * \param[in] mem_c The mem constant index.
 */
static void
set_two_const(cardano_uplc_builtin_cost_t* e, const int64_t* p, size_t cpu_c, size_t mem_c)
{
  e->cpu.two.params.constant = p[cpu_c];
  e->mem.two.params.constant = p[mem_c];
}

/**
 * \brief Sets the budget of a machine step kind from a cpu and mem index.
 *
 * \param[out] b The budget to set.
 * \param[in] p The parameter array.
 * \param[in] cpu_i The cpu index.
 * \param[in] mem_i The mem index.
 */
static void
set_budget(cardano_uplc_budget_t* b, const int64_t* p, size_t cpu_i, size_t mem_i)
{
  b->cpu = p[cpu_i];
  b->mem = p[mem_i];
}

/**
 * \brief Fills the machine-step costs shared by V1, V2 and V3 from params.
 *
 * The machine-step indices are identical across the three versions; constr and
 * case are V3-only and are set by the V3 path.
 *
 * \param[out] m The machine costs to populate.
 * \param[in] p The parameter array.
 */
static void
fill_machine_common(cardano_uplc_machine_costs_t* m, const int64_t* p)
{
  set_budget(&m->apply, p, 17U, 18U);
  set_budget(&m->builtin, p, 19U, 20U);
  set_budget(&m->constant, p, 21U, 22U);
  set_budget(&m->delay, p, 23U, 24U);
  set_budget(&m->force, p, 25U, 26U);
  set_budget(&m->lambda, p, 27U, 28U);
  set_budget(&m->startup, p, 29U, 30U);
  set_budget(&m->var_step, p, 31U, 32U);
}

/**
 * \brief Fills the integer-division entries shared by V1 and V2 from params.
 *
 * Divide, quotient, remainder and mod all carry the same const-above-diagonal
 * over multiplied-sizes cpu shape and subtracted-sizes mem shape under the V1/V2
 * semantics, and each reads its own constant/intercept/slope/minimum indices.
 *
 * \param[in,out] e The entry to set.
 * \param[in] p The parameter array.
 * \param[in] cpu_c The cpu off-diagonal constant index.
 * \param[in] cpu_i The cpu model intercept index.
 * \param[in] cpu_s The cpu model slope index.
 * \param[in] mem_i The mem intercept index.
 * \param[in] mem_min The mem minimum index.
 * \param[in] mem_s The mem slope index.
 */
static void
set_v12_div(
  cardano_uplc_builtin_cost_t* e,
  const int64_t*               p,
  size_t                       cpu_c,
  size_t                       cpu_i,
  size_t                       cpu_s,
  size_t                       mem_i,
  size_t                       mem_min,
  size_t                       mem_s)
{
  e->cpu.two.params.const_diagonal.constant                         = p[cpu_c];
  e->cpu.two.params.const_diagonal.model.multiplied_sizes.intercept = p[cpu_i];
  e->cpu.two.params.const_diagonal.model.multiplied_sizes.slope     = p[cpu_s];
  e->mem.two.params.subtracted_sizes.intercept                      = p[mem_i];
  e->mem.two.params.subtracted_sizes.minimum                        = p[mem_min];
  e->mem.two.params.subtracted_sizes.slope                          = p[mem_s];
}

/**
 * \brief Fills the BLS12-381 builtin block from a contiguous run of 38 params.
 *
 * The run is ordered g1_add, g1_compress, g1_equal, g1_hash_to_group, g1_neg,
 * g1_scalar_mul, g1_uncompress, the same seven for g2, then final_verify,
 * miller_loop and mul_ml_result; the order is identical in every version's list.
 *
 * \param[in,out] b The builtin costs to fill.
 * \param[in] p The parameter array.
 * \param[in] base The index of the first parameter of the run.
 */
static void
fill_bls(cardano_uplc_builtin_costs_t* b, const int64_t* p, size_t base)
{
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_ADD], p, base + 0U, base + 1U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_COMPRESS], p, base + 2U, base + 3U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_EQUAL], p, base + 4U, base + 5U);

  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_HASH_TO_GROUP].cpu.two.params.linear.intercept = p[base + 6U];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_HASH_TO_GROUP].cpu.two.params.linear.slope     = p[base + 7U];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_HASH_TO_GROUP].mem.two.params.constant         = p[base + 8U];

  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_NEG], p, base + 9U, base + 10U);

  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_SCALAR_MUL].cpu.two.params.linear.intercept = p[base + 11U];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_SCALAR_MUL].cpu.two.params.linear.slope     = p[base + 12U];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_SCALAR_MUL].mem.two.params.constant         = p[base + 13U];

  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_UNCOMPRESS], p, base + 14U, base + 15U);

  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_ADD], p, base + 16U, base + 17U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_COMPRESS], p, base + 18U, base + 19U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_EQUAL], p, base + 20U, base + 21U);

  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_HASH_TO_GROUP].cpu.two.params.linear.intercept = p[base + 22U];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_HASH_TO_GROUP].cpu.two.params.linear.slope     = p[base + 23U];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_HASH_TO_GROUP].mem.two.params.constant         = p[base + 24U];

  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_NEG], p, base + 25U, base + 26U);

  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_SCALAR_MUL].cpu.two.params.linear.intercept = p[base + 27U];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_SCALAR_MUL].cpu.two.params.linear.slope     = p[base + 28U];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_SCALAR_MUL].mem.two.params.constant         = p[base + 29U];

  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_UNCOMPRESS], p, base + 30U, base + 31U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_FINAL_VERIFY], p, base + 32U, base + 33U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_MILLER_LOOP], p, base + 34U, base + 35U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_MUL_ML_RESULT], p, base + 36U, base + 37U);
}

/**
 * \brief Fills keccak_256 and blake2b_224 from a contiguous run of 6 params.
 *
 * \param[in,out] b The builtin costs to fill.
 * \param[in] p The parameter array.
 * \param[in] base The index of the first parameter of the run.
 */
static void
fill_keccak_blake224(cardano_uplc_builtin_costs_t* b, const int64_t* p, size_t base)
{
  set_one_lin_cpu_const_mem(&b->entries[CARDANO_UPLC_BUILTIN_KECCAK_256], p, base + 0U, base + 1U, base + 2U);
  set_one_lin_cpu_const_mem(&b->entries[CARDANO_UPLC_BUILTIN_BLAKE2B_224], p, base + 3U, base + 4U, base + 5U);
}

/**
 * \brief Fills the integer/byte-string conversions from a contiguous run of 10 params.
 *
 * \param[in,out] b The builtin costs to fill.
 * \param[in] p The parameter array.
 * \param[in] base The index of the first parameter of the run.
 */
static void
fill_int_bytestring_conversions(cardano_uplc_builtin_costs_t* b, const int64_t* p, size_t base)
{
  b->entries[CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING].cpu.three.params.quadratic_in_z.coeff_0 = p[base + 0U];
  b->entries[CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING].cpu.three.params.quadratic_in_z.coeff_1 = p[base + 1U];
  b->entries[CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING].cpu.three.params.quadratic_in_z.coeff_2 = p[base + 2U];
  b->entries[CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING].mem.three.params.linear.intercept       = p[base + 3U];
  b->entries[CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING].mem.three.params.linear.slope           = p[base + 4U];

  b->entries[CARDANO_UPLC_BUILTIN_BYTE_STRING_TO_INTEGER].cpu.two.params.quadratic_in_y.coeff_0 = p[base + 5U];
  b->entries[CARDANO_UPLC_BUILTIN_BYTE_STRING_TO_INTEGER].cpu.two.params.quadratic_in_y.coeff_1 = p[base + 6U];
  b->entries[CARDANO_UPLC_BUILTIN_BYTE_STRING_TO_INTEGER].cpu.two.params.quadratic_in_y.coeff_2 = p[base + 7U];
  b->entries[CARDANO_UPLC_BUILTIN_BYTE_STRING_TO_INTEGER].mem.two.params.linear.intercept       = p[base + 8U];
  b->entries[CARDANO_UPLC_BUILTIN_BYTE_STRING_TO_INTEGER].mem.two.params.linear.slope           = p[base + 9U];
}

/**
 * \brief Fills the bitwise builtin block from a contiguous run of 46 params.
 *
 * The run is ordered and/or/xor, complement, read_bit, write_bits, replicate_byte,
 * shift, rotate, count_set_bits, find_first_set_bit and ripemd_160; the order is
 * identical in every version's list.
 *
 * \param[in,out] b The builtin costs to fill.
 * \param[in] p The parameter array.
 * \param[in] base The index of the first parameter of the run.
 */
static void
fill_bitwise(cardano_uplc_builtin_costs_t* b, const int64_t* p, size_t base)
{
  b->entries[CARDANO_UPLC_BUILTIN_AND_BYTE_STRING].cpu.three.params.linear_in_y_and_z.intercept = p[base + 0U];
  b->entries[CARDANO_UPLC_BUILTIN_AND_BYTE_STRING].cpu.three.params.linear_in_y_and_z.slope1    = p[base + 1U];
  b->entries[CARDANO_UPLC_BUILTIN_AND_BYTE_STRING].cpu.three.params.linear_in_y_and_z.slope2    = p[base + 2U];
  b->entries[CARDANO_UPLC_BUILTIN_AND_BYTE_STRING].mem.three.params.linear.intercept            = p[base + 3U];
  b->entries[CARDANO_UPLC_BUILTIN_AND_BYTE_STRING].mem.three.params.linear.slope                = p[base + 4U];

  b->entries[CARDANO_UPLC_BUILTIN_OR_BYTE_STRING].cpu.three.params.linear_in_y_and_z.intercept = p[base + 5U];
  b->entries[CARDANO_UPLC_BUILTIN_OR_BYTE_STRING].cpu.three.params.linear_in_y_and_z.slope1    = p[base + 6U];
  b->entries[CARDANO_UPLC_BUILTIN_OR_BYTE_STRING].cpu.three.params.linear_in_y_and_z.slope2    = p[base + 7U];
  b->entries[CARDANO_UPLC_BUILTIN_OR_BYTE_STRING].mem.three.params.linear.intercept            = p[base + 8U];
  b->entries[CARDANO_UPLC_BUILTIN_OR_BYTE_STRING].mem.three.params.linear.slope                = p[base + 9U];

  b->entries[CARDANO_UPLC_BUILTIN_XOR_BYTE_STRING].cpu.three.params.linear_in_y_and_z.intercept = p[base + 10U];
  b->entries[CARDANO_UPLC_BUILTIN_XOR_BYTE_STRING].cpu.three.params.linear_in_y_and_z.slope1    = p[base + 11U];
  b->entries[CARDANO_UPLC_BUILTIN_XOR_BYTE_STRING].cpu.three.params.linear_in_y_and_z.slope2    = p[base + 12U];
  b->entries[CARDANO_UPLC_BUILTIN_XOR_BYTE_STRING].mem.three.params.linear.intercept            = p[base + 13U];
  b->entries[CARDANO_UPLC_BUILTIN_XOR_BYTE_STRING].mem.three.params.linear.slope                = p[base + 14U];

  set_one_lin(&b->entries[CARDANO_UPLC_BUILTIN_COMPLEMENT_BYTE_STRING], p, base + 15U, base + 16U, base + 17U, base + 18U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_READ_BIT], p, base + 19U, base + 20U);

  b->entries[CARDANO_UPLC_BUILTIN_WRITE_BITS].cpu.three.params.linear.intercept = p[base + 21U];
  b->entries[CARDANO_UPLC_BUILTIN_WRITE_BITS].cpu.three.params.linear.slope     = p[base + 22U];
  b->entries[CARDANO_UPLC_BUILTIN_WRITE_BITS].mem.three.params.linear.intercept = p[base + 23U];
  b->entries[CARDANO_UPLC_BUILTIN_WRITE_BITS].mem.three.params.linear.slope     = p[base + 24U];

  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_REPLICATE_BYTE], p, base + 25U, base + 26U, base + 27U, base + 28U);
  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_SHIFT_BYTE_STRING], p, base + 29U, base + 30U, base + 31U, base + 32U);
  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_ROTATE_BYTE_STRING], p, base + 33U, base + 34U, base + 35U, base + 36U);
  set_one_lin_cpu_const_mem(&b->entries[CARDANO_UPLC_BUILTIN_COUNT_SET_BITS], p, base + 37U, base + 38U, base + 39U);
  set_one_lin_cpu_const_mem(&b->entries[CARDANO_UPLC_BUILTIN_FIND_FIRST_SET_BIT], p, base + 40U, base + 41U, base + 42U);
  set_one_lin_cpu_const_mem(&b->entries[CARDANO_UPLC_BUILTIN_RIPEMD_160], p, base + 43U, base + 44U, base + 45U);
}

/**
 * \brief Fills exp_mod_integer from a contiguous run of 5 params.
 *
 * \param[in,out] b The builtin costs to fill.
 * \param[in] p The parameter array.
 * \param[in] base The index of the first parameter of the run.
 */
static void
fill_exp_mod(cardano_uplc_builtin_costs_t* b, const int64_t* p, size_t base)
{
  b->entries[CARDANO_UPLC_BUILTIN_EXP_MOD_INTEGER].cpu.three.params.exp_mod.coeff_00 = p[base + 0U];
  b->entries[CARDANO_UPLC_BUILTIN_EXP_MOD_INTEGER].cpu.three.params.exp_mod.coeff_11 = p[base + 1U];
  b->entries[CARDANO_UPLC_BUILTIN_EXP_MOD_INTEGER].cpu.three.params.exp_mod.coeff_12 = p[base + 2U];
  b->entries[CARDANO_UPLC_BUILTIN_EXP_MOD_INTEGER].mem.three.params.linear.intercept = p[base + 3U];
  b->entries[CARDANO_UPLC_BUILTIN_EXP_MOD_INTEGER].mem.three.params.linear.slope     = p[base + 4U];
}

/**
 * \brief Fills the batch-6 builtins from a contiguous run of 48 params.
 *
 * The run is ordered drop_list, length_of_array, list_to_array, index_array, the
 * two multi-scalar multiplications, then the builtin Value family insert_coin,
 * lookup_coin, union_value, value_contains, value_data, un_value_data and
 * scale_value; the order is identical in every version's list.
 *
 * \param[in,out] b The builtin costs to fill.
 * \param[in] p The parameter array.
 * \param[in] base The index of the first parameter of the run.
 */
static void
fill_batch6(cardano_uplc_builtin_costs_t* b, const int64_t* p, size_t base)
{
  b->entries[CARDANO_UPLC_BUILTIN_DROP_LIST].cpu.two.params.linear.intercept = p[base + 0U];
  b->entries[CARDANO_UPLC_BUILTIN_DROP_LIST].cpu.two.params.linear.slope     = p[base + 1U];
  b->entries[CARDANO_UPLC_BUILTIN_DROP_LIST].mem.two.params.constant         = p[base + 2U];

  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_LENGTH_OF_ARRAY], p, base + 3U, base + 4U);
  set_one_lin(&b->entries[CARDANO_UPLC_BUILTIN_LIST_TO_ARRAY], p, base + 5U, base + 6U, base + 7U, base + 8U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_INDEX_ARRAY], p, base + 9U, base + 10U);

  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_MULTI_SCALAR_MUL].cpu.two.params.linear.intercept = p[base + 11U];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_MULTI_SCALAR_MUL].cpu.two.params.linear.slope     = p[base + 12U];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_MULTI_SCALAR_MUL].mem.two.params.constant         = p[base + 13U];

  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_MULTI_SCALAR_MUL].cpu.two.params.linear.intercept = p[base + 14U];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_MULTI_SCALAR_MUL].cpu.two.params.linear.slope     = p[base + 15U];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_MULTI_SCALAR_MUL].mem.two.params.constant         = p[base + 16U];

  set_one_lin(&b->entries[CARDANO_UPLC_BUILTIN_INSERT_COIN], p, base + 17U, base + 18U, base + 19U, base + 20U);

  b->entries[CARDANO_UPLC_BUILTIN_LOOKUP_COIN].cpu.three.params.linear.intercept = p[base + 21U];
  b->entries[CARDANO_UPLC_BUILTIN_LOOKUP_COIN].cpu.three.params.linear.slope     = p[base + 22U];
  b->entries[CARDANO_UPLC_BUILTIN_LOOKUP_COIN].mem.three.params.constant         = p[base + 23U];

  b->entries[CARDANO_UPLC_BUILTIN_UNION_VALUE].cpu.two.params.with_interaction.c00 = p[base + 24U];
  b->entries[CARDANO_UPLC_BUILTIN_UNION_VALUE].cpu.two.params.with_interaction.c10 = p[base + 25U];
  b->entries[CARDANO_UPLC_BUILTIN_UNION_VALUE].cpu.two.params.with_interaction.c01 = p[base + 26U];
  b->entries[CARDANO_UPLC_BUILTIN_UNION_VALUE].cpu.two.params.with_interaction.c11 = p[base + 27U];
  b->entries[CARDANO_UPLC_BUILTIN_UNION_VALUE].mem.two.params.linear.intercept     = p[base + 28U];
  b->entries[CARDANO_UPLC_BUILTIN_UNION_VALUE].mem.two.params.linear.slope         = p[base + 29U];

  b->entries[CARDANO_UPLC_BUILTIN_VALUE_CONTAINS].cpu.two.params.const_diagonal.constant                          = p[base + 30U];
  b->entries[CARDANO_UPLC_BUILTIN_VALUE_CONTAINS].cpu.two.params.const_diagonal.model.linear_in_x_and_y.intercept = p[base + 31U];
  b->entries[CARDANO_UPLC_BUILTIN_VALUE_CONTAINS].cpu.two.params.const_diagonal.model.linear_in_x_and_y.slope1    = p[base + 32U];
  b->entries[CARDANO_UPLC_BUILTIN_VALUE_CONTAINS].cpu.two.params.const_diagonal.model.linear_in_x_and_y.slope2    = p[base + 33U];
  b->entries[CARDANO_UPLC_BUILTIN_VALUE_CONTAINS].mem.two.params.constant                                         = p[base + 34U];

  set_one_lin(&b->entries[CARDANO_UPLC_BUILTIN_VALUE_DATA], p, base + 35U, base + 36U, base + 37U, base + 38U);

  b->entries[CARDANO_UPLC_BUILTIN_UN_VALUE_DATA].cpu.one.params.quadratic.coeff_0 = p[base + 39U];
  b->entries[CARDANO_UPLC_BUILTIN_UN_VALUE_DATA].cpu.one.params.quadratic.coeff_1 = p[base + 40U];
  b->entries[CARDANO_UPLC_BUILTIN_UN_VALUE_DATA].cpu.one.params.quadratic.coeff_2 = p[base + 41U];
  b->entries[CARDANO_UPLC_BUILTIN_UN_VALUE_DATA].mem.one.params.linear.intercept  = p[base + 42U];
  b->entries[CARDANO_UPLC_BUILTIN_UN_VALUE_DATA].mem.one.params.linear.slope      = p[base + 43U];

  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_SCALE_VALUE], p, base + 44U, base + 45U, base + 46U, base + 47U);
}

/**
 * \brief Maps the V1 flat parameter list onto a default V1 cost model.
 *
 * The default shapes come from \ref cardano_uplc_builtin_costs_v1; this
 * overwrites their coefficients. Protocol version 11 (van Rossem) unified the
 * builtin set across the three languages, so the V1 list appends the previously
 * V2/V3-only builtins, the constr and case machine steps and the batch-6
 * builtins after the original 166 parameters.
 *
 * \param[out] out The cost model to populate.
 * \param[in] p The V1 parameter array.
 */
static void
from_params_v1(cardano_uplc_cost_model_t* out, const int64_t* p)
{
  cardano_uplc_builtin_costs_t* b = &out->builtins;

  out->machine = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V1);
  *b           = cardano_uplc_builtin_costs_v1();

  fill_machine_common(&out->machine, p);

  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_ADD_INTEGER], p, 0U, 1U, 2U, 3U);
  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_APPEND_BYTE_STRING], p, 4U, 5U, 6U, 7U);
  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_APPEND_STRING], p, 8U, 9U, 10U, 11U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_B_DATA], p, 12U, 13U);
  set_one_lin_cpu_const_mem(&b->entries[CARDANO_UPLC_BUILTIN_BLAKE2B_256], p, 14U, 15U, 16U);

  b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_DATA].cpu.six.params.constant   = p[33];
  b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_DATA].mem.six.params.constant   = p[34];
  b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_LIST].cpu.three.params.constant = p[35];
  b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_LIST].mem.three.params.constant = p[36];
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_UNIT], p, 37U, 38U);
  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_CONS_BYTE_STRING], p, 39U, 40U, 41U, 42U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_CONSTR_DATA], p, 43U, 44U);
  set_one_lin(&b->entries[CARDANO_UPLC_BUILTIN_DECODE_UTF8], p, 45U, 46U, 47U, 48U);

  set_v12_div(&b->entries[CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER], p, 49U, 50U, 51U, 52U, 53U, 54U);
  set_one_lin(&b->entries[CARDANO_UPLC_BUILTIN_ENCODE_UTF8], p, 55U, 56U, 57U, 58U);

  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING].cpu.two.params.linear_on_diagonal.constant  = p[59];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING].cpu.two.params.linear_on_diagonal.intercept = p[60];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING].cpu.two.params.linear_on_diagonal.slope     = p[61];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING].mem.two.params.constant                     = p[62];

  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_DATA].cpu.two.params.linear.intercept = p[63];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_DATA].cpu.two.params.linear.slope     = p[64];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_DATA].mem.two.params.constant         = p[65];

  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_INTEGER].cpu.two.params.linear.intercept = p[66];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_INTEGER].cpu.two.params.linear.slope     = p[67];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_INTEGER].mem.two.params.constant         = p[68];

  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_STRING].cpu.two.params.linear_on_diagonal.constant  = p[69];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_STRING].cpu.two.params.linear_on_diagonal.intercept = p[70];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_STRING].cpu.two.params.linear_on_diagonal.slope     = p[71];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_STRING].mem.two.params.constant                     = p[72];

  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_FST_PAIR], p, 73U, 74U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_HEAD_LIST], p, 75U, 76U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_I_DATA], p, 77U, 78U);
  b->entries[CARDANO_UPLC_BUILTIN_IF_THEN_ELSE].cpu.three.params.constant = p[79];
  b->entries[CARDANO_UPLC_BUILTIN_IF_THEN_ELSE].mem.three.params.constant = p[80];
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_INDEX_BYTE_STRING], p, 81U, 82U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_LENGTH_OF_BYTE_STRING], p, 83U, 84U);

  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_BYTE_STRING].cpu.two.params.linear.intercept = p[85];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_BYTE_STRING].cpu.two.params.linear.slope     = p[86];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_BYTE_STRING].mem.two.params.constant         = p[87];

  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_BYTE_STRING].cpu.two.params.linear.intercept = p[88];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_BYTE_STRING].cpu.two.params.linear.slope     = p[89];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_BYTE_STRING].mem.two.params.constant         = p[90];

  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_INTEGER].cpu.two.params.linear.intercept = p[91];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_INTEGER].cpu.two.params.linear.slope     = p[92];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_INTEGER].mem.two.params.constant         = p[93];

  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_INTEGER].cpu.two.params.linear.intercept = p[94];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_INTEGER].cpu.two.params.linear.slope     = p[95];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_INTEGER].mem.two.params.constant         = p[96];

  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_LIST_DATA], p, 97U, 98U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_MAP_DATA], p, 99U, 100U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_MK_CONS], p, 101U, 102U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_MK_NIL_DATA], p, 103U, 104U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_MK_NIL_PAIR_DATA], p, 105U, 106U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_MK_PAIR_DATA], p, 107U, 108U);

  set_v12_div(&b->entries[CARDANO_UPLC_BUILTIN_MOD_INTEGER], p, 109U, 110U, 111U, 112U, 113U, 114U);
  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_MULTIPLY_INTEGER], p, 115U, 116U, 117U, 118U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_NULL_LIST], p, 119U, 120U);
  set_v12_div(&b->entries[CARDANO_UPLC_BUILTIN_QUOTIENT_INTEGER], p, 121U, 122U, 123U, 124U, 125U, 126U);
  set_v12_div(&b->entries[CARDANO_UPLC_BUILTIN_REMAINDER_INTEGER], p, 127U, 128U, 129U, 130U, 131U, 132U);
  set_one_lin_cpu_const_mem(&b->entries[CARDANO_UPLC_BUILTIN_SHA2_256], p, 133U, 134U, 135U);
  set_one_lin_cpu_const_mem(&b->entries[CARDANO_UPLC_BUILTIN_SHA3_256], p, 136U, 137U, 138U);

  b->entries[CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING].cpu.three.params.linear.intercept = p[139];
  b->entries[CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING].cpu.three.params.linear.slope     = p[140];
  b->entries[CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING].mem.three.params.linear.intercept = p[141];
  b->entries[CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING].mem.three.params.linear.slope     = p[142];

  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_SND_PAIR], p, 143U, 144U);
  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_SUBTRACT_INTEGER], p, 145U, 146U, 147U, 148U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_TAIL_LIST], p, 149U, 150U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_TRACE], p, 151U, 152U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_UN_B_DATA], p, 153U, 154U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_UN_CONSTR_DATA], p, 155U, 156U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_UN_I_DATA], p, 157U, 158U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_UN_LIST_DATA], p, 159U, 160U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_UN_MAP_DATA], p, 161U, 162U);

  b->entries[CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE].cpu.three.params.linear.intercept = p[163];
  b->entries[CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE].cpu.three.params.linear.slope     = p[164];
  b->entries[CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE].mem.three.params.constant         = p[165];

  set_one_lin(&b->entries[CARDANO_UPLC_BUILTIN_SERIALISE_DATA], p, 166U, 167U, 168U, 169U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_VERIFY_ECDSA_SECP256K1_SIGNATURE], p, 170U, 171U);

  b->entries[CARDANO_UPLC_BUILTIN_VERIFY_SCHNORR_SECP256K1_SIGNATURE].cpu.three.params.linear.intercept = p[172];
  b->entries[CARDANO_UPLC_BUILTIN_VERIFY_SCHNORR_SECP256K1_SIGNATURE].cpu.three.params.linear.slope     = p[173];
  b->entries[CARDANO_UPLC_BUILTIN_VERIFY_SCHNORR_SECP256K1_SIGNATURE].mem.three.params.constant         = p[174];

  set_budget(&out->machine.constr, p, 175U, 176U);
  set_budget(&out->machine.case_step, p, 177U, 178U);

  fill_bls(b, p, 179U);
  fill_keccak_blake224(b, p, 217U);
  fill_int_bytestring_conversions(b, p, 223U);
  fill_bitwise(b, p, 233U);
  fill_exp_mod(b, p, 279U);
  fill_batch6(b, p, 284U);
}

/**
 * \brief Maps the V2 flat parameter list onto a default V2 cost model.
 *
 * The default shapes come from \ref cardano_uplc_builtin_costs_v2; this
 * overwrites their coefficients. The integer/byte-string conversions were
 * appended to the V2 list at protocol version 10 (Plomin); protocol version 11
 * (van Rossem) then unified the builtin set across the three languages,
 * appending the previously V3-only builtins, the constr and case machine steps
 * and the batch-6 builtins.
 *
 * \param[out] out The cost model to populate.
 * \param[in] p The V2 parameter array.
 */
static void
from_params_v2(cardano_uplc_cost_model_t* out, const int64_t* p)
{
  cardano_uplc_builtin_costs_t* b = &out->builtins;

  out->machine = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V2);
  *b           = cardano_uplc_builtin_costs_v2();

  fill_machine_common(&out->machine, p);

  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_ADD_INTEGER], p, 0U, 1U, 2U, 3U);
  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_APPEND_BYTE_STRING], p, 4U, 5U, 6U, 7U);
  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_APPEND_STRING], p, 8U, 9U, 10U, 11U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_B_DATA], p, 12U, 13U);
  set_one_lin_cpu_const_mem(&b->entries[CARDANO_UPLC_BUILTIN_BLAKE2B_256], p, 14U, 15U, 16U);

  b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_DATA].cpu.six.params.constant   = p[33];
  b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_DATA].mem.six.params.constant   = p[34];
  b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_LIST].cpu.three.params.constant = p[35];
  b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_LIST].mem.three.params.constant = p[36];
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_UNIT], p, 37U, 38U);
  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_CONS_BYTE_STRING], p, 39U, 40U, 41U, 42U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_CONSTR_DATA], p, 43U, 44U);
  set_one_lin(&b->entries[CARDANO_UPLC_BUILTIN_DECODE_UTF8], p, 45U, 46U, 47U, 48U);
  set_v12_div(&b->entries[CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER], p, 49U, 50U, 51U, 52U, 53U, 54U);
  set_one_lin(&b->entries[CARDANO_UPLC_BUILTIN_ENCODE_UTF8], p, 55U, 56U, 57U, 58U);

  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING].cpu.two.params.linear_on_diagonal.constant  = p[59];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING].cpu.two.params.linear_on_diagonal.intercept = p[60];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING].cpu.two.params.linear_on_diagonal.slope     = p[61];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING].mem.two.params.constant                     = p[62];

  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_DATA].cpu.two.params.linear.intercept = p[63];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_DATA].cpu.two.params.linear.slope     = p[64];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_DATA].mem.two.params.constant         = p[65];

  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_INTEGER].cpu.two.params.linear.intercept = p[66];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_INTEGER].cpu.two.params.linear.slope     = p[67];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_INTEGER].mem.two.params.constant         = p[68];

  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_STRING].cpu.two.params.linear_on_diagonal.constant  = p[69];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_STRING].cpu.two.params.linear_on_diagonal.intercept = p[70];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_STRING].cpu.two.params.linear_on_diagonal.slope     = p[71];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_STRING].mem.two.params.constant                     = p[72];

  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_FST_PAIR], p, 73U, 74U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_HEAD_LIST], p, 75U, 76U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_I_DATA], p, 77U, 78U);
  b->entries[CARDANO_UPLC_BUILTIN_IF_THEN_ELSE].cpu.three.params.constant = p[79];
  b->entries[CARDANO_UPLC_BUILTIN_IF_THEN_ELSE].mem.three.params.constant = p[80];
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_INDEX_BYTE_STRING], p, 81U, 82U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_LENGTH_OF_BYTE_STRING], p, 83U, 84U);

  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_BYTE_STRING].cpu.two.params.linear.intercept = p[85];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_BYTE_STRING].cpu.two.params.linear.slope     = p[86];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_BYTE_STRING].mem.two.params.constant         = p[87];

  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_BYTE_STRING].cpu.two.params.linear.intercept = p[88];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_BYTE_STRING].cpu.two.params.linear.slope     = p[89];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_BYTE_STRING].mem.two.params.constant         = p[90];

  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_INTEGER].cpu.two.params.linear.intercept = p[91];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_INTEGER].cpu.two.params.linear.slope     = p[92];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_INTEGER].mem.two.params.constant         = p[93];

  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_INTEGER].cpu.two.params.linear.intercept = p[94];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_INTEGER].cpu.two.params.linear.slope     = p[95];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_INTEGER].mem.two.params.constant         = p[96];

  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_LIST_DATA], p, 97U, 98U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_MAP_DATA], p, 99U, 100U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_MK_CONS], p, 101U, 102U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_MK_NIL_DATA], p, 103U, 104U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_MK_NIL_PAIR_DATA], p, 105U, 106U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_MK_PAIR_DATA], p, 107U, 108U);
  set_v12_div(&b->entries[CARDANO_UPLC_BUILTIN_MOD_INTEGER], p, 109U, 110U, 111U, 112U, 113U, 114U);
  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_MULTIPLY_INTEGER], p, 115U, 116U, 117U, 118U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_NULL_LIST], p, 119U, 120U);
  set_v12_div(&b->entries[CARDANO_UPLC_BUILTIN_QUOTIENT_INTEGER], p, 121U, 122U, 123U, 124U, 125U, 126U);
  set_v12_div(&b->entries[CARDANO_UPLC_BUILTIN_REMAINDER_INTEGER], p, 127U, 128U, 129U, 130U, 131U, 132U);

  set_one_lin(&b->entries[CARDANO_UPLC_BUILTIN_SERIALISE_DATA], p, 133U, 134U, 135U, 136U);
  set_one_lin_cpu_const_mem(&b->entries[CARDANO_UPLC_BUILTIN_SHA2_256], p, 137U, 138U, 139U);
  set_one_lin_cpu_const_mem(&b->entries[CARDANO_UPLC_BUILTIN_SHA3_256], p, 140U, 141U, 142U);

  b->entries[CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING].cpu.three.params.linear.intercept = p[143];
  b->entries[CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING].cpu.three.params.linear.slope     = p[144];
  b->entries[CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING].mem.three.params.linear.intercept = p[145];
  b->entries[CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING].mem.three.params.linear.slope     = p[146];

  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_SND_PAIR], p, 147U, 148U);
  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_SUBTRACT_INTEGER], p, 149U, 150U, 151U, 152U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_TAIL_LIST], p, 153U, 154U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_TRACE], p, 155U, 156U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_UN_B_DATA], p, 157U, 158U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_UN_CONSTR_DATA], p, 159U, 160U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_UN_I_DATA], p, 161U, 162U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_UN_LIST_DATA], p, 163U, 164U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_UN_MAP_DATA], p, 165U, 166U);

  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_VERIFY_ECDSA_SECP256K1_SIGNATURE], p, 167U, 168U);

  b->entries[CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE].cpu.three.params.linear.intercept = p[169];
  b->entries[CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE].cpu.three.params.linear.slope     = p[170];
  b->entries[CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE].mem.three.params.constant         = p[171];

  b->entries[CARDANO_UPLC_BUILTIN_VERIFY_SCHNORR_SECP256K1_SIGNATURE].cpu.three.params.linear.intercept = p[172];
  b->entries[CARDANO_UPLC_BUILTIN_VERIFY_SCHNORR_SECP256K1_SIGNATURE].cpu.three.params.linear.slope     = p[173];
  b->entries[CARDANO_UPLC_BUILTIN_VERIFY_SCHNORR_SECP256K1_SIGNATURE].mem.three.params.constant         = p[174];

  fill_int_bytestring_conversions(b, p, 175U);

  set_budget(&out->machine.constr, p, 185U, 186U);
  set_budget(&out->machine.case_step, p, 187U, 188U);

  fill_bls(b, p, 189U);
  fill_keccak_blake224(b, p, 227U);
  fill_bitwise(b, p, 233U);
  fill_exp_mod(b, p, 279U);
  fill_batch6(b, p, 284U);
}

/**
 * \brief Sets the V3 const-above-into-quadratic cpu of an integer-division entry.
 *
 * Under V3 semantics divide, quotient, remainder and mod share this cpu shape,
 * each reading a contiguous run of eight indices in the order constant, c00, c01,
 * c02, c10, c11, c20, minimum.
 *
 * \param[in,out] e The entry whose cpu is set.
 * \param[in] p The parameter array.
 * \param[in] base The index of the off-diagonal constant; the next seven indices
 *            follow in the documented order.
 */
static void
set_v3_div_cpu(cardano_uplc_builtin_cost_t* e, const int64_t* p, size_t base)
{
  e->cpu.two.params.const_above_into_quadratic.constant           = p[base + 0U];
  e->cpu.two.params.const_above_into_quadratic.quadratic.coeff_00 = p[base + 1U];
  e->cpu.two.params.const_above_into_quadratic.quadratic.coeff_01 = p[base + 2U];
  e->cpu.two.params.const_above_into_quadratic.quadratic.coeff_02 = p[base + 3U];
  e->cpu.two.params.const_above_into_quadratic.quadratic.coeff_10 = p[base + 4U];
  e->cpu.two.params.const_above_into_quadratic.quadratic.coeff_11 = p[base + 5U];
  e->cpu.two.params.const_above_into_quadratic.quadratic.coeff_20 = p[base + 6U];
  e->cpu.two.params.const_above_into_quadratic.quadratic.minimum  = p[base + 7U];
}

/**
 * \brief Maps the V3 flat parameter list onto a default V3 cost model.
 *
 * Reads the first \ref CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3 parameters, which
 * cover every builtin through the exp_mod_integer coefficients plus the array
 * and value families appended at protocol version 11 (van Rossem).
 *
 * \param[out] out The cost model to populate.
 * \param[in] p The V3 parameter array.
 */
static void
from_params_v3(cardano_uplc_cost_model_t* out, const int64_t* p)
{
  cardano_uplc_builtin_costs_t* b = &out->builtins;

  out->machine = cardano_uplc_machine_costs_default(CARDANO_UPLC_COST_MODEL_VERSION_V3);
  *b           = cardano_uplc_builtin_costs_v3();

  fill_machine_common(&out->machine, p);
  set_budget(&out->machine.constr, p, 193U, 194U);
  set_budget(&out->machine.case_step, p, 195U, 196U);

  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_ADD_INTEGER], p, 0U, 1U, 2U, 3U);
  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_APPEND_BYTE_STRING], p, 4U, 5U, 6U, 7U);
  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_APPEND_STRING], p, 8U, 9U, 10U, 11U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_B_DATA], p, 12U, 13U);
  set_one_lin_cpu_const_mem(&b->entries[CARDANO_UPLC_BUILTIN_BLAKE2B_256], p, 14U, 15U, 16U);

  b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_DATA].cpu.six.params.constant   = p[33];
  b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_DATA].mem.six.params.constant   = p[34];
  b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_LIST].cpu.three.params.constant = p[35];
  b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_LIST].mem.three.params.constant = p[36];
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_UNIT], p, 37U, 38U);
  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_CONS_BYTE_STRING], p, 39U, 40U, 41U, 42U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_CONSTR_DATA], p, 43U, 44U);
  set_one_lin(&b->entries[CARDANO_UPLC_BUILTIN_DECODE_UTF8], p, 45U, 46U, 47U, 48U);

  set_v3_div_cpu(&b->entries[CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER], p, 49U);
  b->entries[CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER].mem.two.params.subtracted_sizes.intercept = p[57];
  b->entries[CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER].mem.two.params.subtracted_sizes.minimum   = p[58];
  b->entries[CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER].mem.two.params.subtracted_sizes.slope     = p[59];

  set_one_lin(&b->entries[CARDANO_UPLC_BUILTIN_ENCODE_UTF8], p, 60U, 61U, 62U, 63U);

  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING].cpu.two.params.linear_on_diagonal.constant  = p[64];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING].cpu.two.params.linear_on_diagonal.intercept = p[65];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING].cpu.two.params.linear_on_diagonal.slope     = p[66];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING].mem.two.params.constant                     = p[67];

  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_DATA].cpu.two.params.linear.intercept = p[68];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_DATA].cpu.two.params.linear.slope     = p[69];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_DATA].mem.two.params.constant         = p[70];

  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_INTEGER].cpu.two.params.linear.intercept = p[71];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_INTEGER].cpu.two.params.linear.slope     = p[72];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_INTEGER].mem.two.params.constant         = p[73];

  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_STRING].cpu.two.params.linear_on_diagonal.constant  = p[74];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_STRING].cpu.two.params.linear_on_diagonal.intercept = p[75];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_STRING].cpu.two.params.linear_on_diagonal.slope     = p[76];
  b->entries[CARDANO_UPLC_BUILTIN_EQUALS_STRING].mem.two.params.constant                     = p[77];

  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_FST_PAIR], p, 78U, 79U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_HEAD_LIST], p, 80U, 81U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_I_DATA], p, 82U, 83U);
  b->entries[CARDANO_UPLC_BUILTIN_IF_THEN_ELSE].cpu.three.params.constant = p[84];
  b->entries[CARDANO_UPLC_BUILTIN_IF_THEN_ELSE].mem.three.params.constant = p[85];
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_INDEX_BYTE_STRING], p, 86U, 87U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_LENGTH_OF_BYTE_STRING], p, 88U, 89U);

  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_BYTE_STRING].cpu.two.params.linear.intercept = p[90];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_BYTE_STRING].cpu.two.params.linear.slope     = p[91];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_BYTE_STRING].mem.two.params.constant         = p[92];

  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_BYTE_STRING].cpu.two.params.linear.intercept = p[93];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_BYTE_STRING].cpu.two.params.linear.slope     = p[94];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_BYTE_STRING].mem.two.params.constant         = p[95];

  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_INTEGER].cpu.two.params.linear.intercept = p[96];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_INTEGER].cpu.two.params.linear.slope     = p[97];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_INTEGER].mem.two.params.constant         = p[98];

  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_INTEGER].cpu.two.params.linear.intercept = p[99];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_INTEGER].cpu.two.params.linear.slope     = p[100];
  b->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_INTEGER].mem.two.params.constant         = p[101];

  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_LIST_DATA], p, 102U, 103U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_MAP_DATA], p, 104U, 105U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_MK_CONS], p, 106U, 107U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_MK_NIL_DATA], p, 108U, 109U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_MK_NIL_PAIR_DATA], p, 110U, 111U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_MK_PAIR_DATA], p, 112U, 113U);

  set_v3_div_cpu(&b->entries[CARDANO_UPLC_BUILTIN_MOD_INTEGER], p, 114U);
  b->entries[CARDANO_UPLC_BUILTIN_MOD_INTEGER].mem.two.params.linear.intercept = p[122];
  b->entries[CARDANO_UPLC_BUILTIN_MOD_INTEGER].mem.two.params.linear.slope     = p[123];

  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_MULTIPLY_INTEGER], p, 124U, 125U, 126U, 127U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_NULL_LIST], p, 128U, 129U);

  set_v3_div_cpu(&b->entries[CARDANO_UPLC_BUILTIN_QUOTIENT_INTEGER], p, 130U);
  b->entries[CARDANO_UPLC_BUILTIN_QUOTIENT_INTEGER].mem.two.params.subtracted_sizes.intercept = p[138];
  b->entries[CARDANO_UPLC_BUILTIN_QUOTIENT_INTEGER].mem.two.params.subtracted_sizes.minimum   = p[139];
  b->entries[CARDANO_UPLC_BUILTIN_QUOTIENT_INTEGER].mem.two.params.subtracted_sizes.slope     = p[140];

  set_v3_div_cpu(&b->entries[CARDANO_UPLC_BUILTIN_REMAINDER_INTEGER], p, 141U);
  b->entries[CARDANO_UPLC_BUILTIN_REMAINDER_INTEGER].mem.two.params.linear.intercept = p[149];
  b->entries[CARDANO_UPLC_BUILTIN_REMAINDER_INTEGER].mem.two.params.linear.slope     = p[150];

  set_one_lin(&b->entries[CARDANO_UPLC_BUILTIN_SERIALISE_DATA], p, 151U, 152U, 153U, 154U);
  set_one_lin_cpu_const_mem(&b->entries[CARDANO_UPLC_BUILTIN_SHA2_256], p, 155U, 156U, 157U);
  set_one_lin_cpu_const_mem(&b->entries[CARDANO_UPLC_BUILTIN_SHA3_256], p, 158U, 159U, 160U);

  b->entries[CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING].cpu.three.params.linear.intercept = p[161];
  b->entries[CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING].cpu.three.params.linear.slope     = p[162];
  b->entries[CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING].mem.three.params.linear.intercept = p[163];
  b->entries[CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING].mem.three.params.linear.slope     = p[164];

  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_SND_PAIR], p, 165U, 166U);
  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_SUBTRACT_INTEGER], p, 167U, 168U, 169U, 170U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_TAIL_LIST], p, 171U, 172U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_TRACE], p, 173U, 174U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_UN_B_DATA], p, 175U, 176U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_UN_CONSTR_DATA], p, 177U, 178U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_UN_I_DATA], p, 179U, 180U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_UN_LIST_DATA], p, 181U, 182U);
  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_UN_MAP_DATA], p, 183U, 184U);

  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_VERIFY_ECDSA_SECP256K1_SIGNATURE], p, 185U, 186U);

  b->entries[CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE].cpu.three.params.linear.intercept = p[187];
  b->entries[CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE].cpu.three.params.linear.slope     = p[188];
  b->entries[CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE].mem.three.params.constant         = p[189];

  b->entries[CARDANO_UPLC_BUILTIN_VERIFY_SCHNORR_SECP256K1_SIGNATURE].cpu.three.params.linear.intercept = p[190];
  b->entries[CARDANO_UPLC_BUILTIN_VERIFY_SCHNORR_SECP256K1_SIGNATURE].cpu.three.params.linear.slope     = p[191];
  b->entries[CARDANO_UPLC_BUILTIN_VERIFY_SCHNORR_SECP256K1_SIGNATURE].mem.three.params.constant         = p[192];

  fill_bls(b, p, 197U);
  fill_keccak_blake224(b, p, 235U);
  fill_int_bytestring_conversions(b, p, 241U);
  fill_bitwise(b, p, 251U);
  fill_exp_mod(b, p, 297U);
  fill_batch6(b, p, 302U);
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_cost_model_from_params(
  cardano_uplc_cost_model_version_t version,
  const int64_t*                    params,
  size_t                            count,
  cardano_uplc_cost_model_t*        out)
{
  size_t  expected = 0U;
  int64_t padded[CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3];

  if ((params == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  switch (version)
  {
    /* V1 and V2 both span 332 parameters since the van Rossem unification. */
    case CARDANO_UPLC_COST_MODEL_VERSION_V1:
    case CARDANO_UPLC_COST_MODEL_VERSION_V2:
    {
      expected = CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V1;
      break;
    }
    case CARDANO_UPLC_COST_MODEL_VERSION_V3:
    {
      expected = CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3;
      break;
    }
    default:
    {
      return CARDANO_ERROR_INVALID_ARGUMENT;
    }
  }

  const int64_t* effective = params;

  if (count < expected)
  {
    size_t i = 0U;

    for (i = 0U; i < count; ++i)
    {
      padded[i] = params[i];
    }

    for (i = count; i < expected; ++i)
    {
      padded[i] = INT64_MAX;
    }

    effective = padded;
  }

  switch (version)
  {
    case CARDANO_UPLC_COST_MODEL_VERSION_V1:
    {
      from_params_v1(out, effective);
      break;
    }
    case CARDANO_UPLC_COST_MODEL_VERSION_V2:
    {
      from_params_v2(out, effective);
      break;
    }
    case CARDANO_UPLC_COST_MODEL_VERSION_V3:
    default:
    {
      from_params_v3(out, effective);
      break;
    }
  }

  return CARDANO_SUCCESS;
}
