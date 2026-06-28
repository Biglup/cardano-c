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
#include "uplc_builtin_costs.h"
#include "uplc_machine_costs.h"
#include "../builtins/uplc_builtin.h"

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
  e->cpu.two.params.const_diagonal.constant                  = p[cpu_c];
  e->cpu.two.params.const_diagonal.model.multiplied_sizes.intercept = p[cpu_i];
  e->cpu.two.params.const_diagonal.model.multiplied_sizes.slope     = p[cpu_s];
  e->mem.two.params.subtracted_sizes.intercept               = p[mem_i];
  e->mem.two.params.subtracted_sizes.minimum                 = p[mem_min];
  e->mem.two.params.subtracted_sizes.slope                   = p[mem_s];
}

/**
 * \brief Maps the V1 flat parameter list onto a default V1 cost model.
 *
 * The default shapes come from \ref cardano_uplc_builtin_costs_v1; this
 * overwrites their coefficients.
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

  b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_DATA].cpu.six.params.constant = p[33];
  b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_DATA].mem.six.params.constant = p[34];
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
}

/**
 * \brief Maps the V2 flat parameter list onto a default V2 cost model.
 *
 * The default shapes come from \ref cardano_uplc_builtin_costs_v2; this
 * overwrites their coefficients.
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

  b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_DATA].cpu.six.params.constant = p[33];
  b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_DATA].mem.six.params.constant = p[34];
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
 * Reads parameters through the count that activates the bitwise tail
 * (\ref CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3). The builtins past that count
 * (exp_mod, the array and value families) carry no parameter at this count and
 * keep the defaults set by \ref cardano_uplc_builtin_costs_v3.
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

  b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_DATA].cpu.six.params.constant = p[33];
  b->entries[CARDANO_UPLC_BUILTIN_CHOOSE_DATA].mem.six.params.constant = p[34];
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

  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_ADD], p, 197U, 198U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_COMPRESS], p, 199U, 200U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_EQUAL], p, 201U, 202U);

  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_HASH_TO_GROUP].cpu.two.params.linear.intercept = p[203];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_HASH_TO_GROUP].cpu.two.params.linear.slope     = p[204];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_HASH_TO_GROUP].mem.two.params.constant         = p[205];

  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_NEG], p, 206U, 207U);

  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_SCALAR_MUL].cpu.two.params.linear.intercept = p[208];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_SCALAR_MUL].cpu.two.params.linear.slope     = p[209];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_SCALAR_MUL].mem.two.params.constant         = p[210];

  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_UNCOMPRESS], p, 211U, 212U);

  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_ADD], p, 213U, 214U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_COMPRESS], p, 215U, 216U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_EQUAL], p, 217U, 218U);

  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_HASH_TO_GROUP].cpu.two.params.linear.intercept = p[219];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_HASH_TO_GROUP].cpu.two.params.linear.slope     = p[220];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_HASH_TO_GROUP].mem.two.params.constant         = p[221];

  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_NEG], p, 222U, 223U);

  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_SCALAR_MUL].cpu.two.params.linear.intercept = p[224];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_SCALAR_MUL].cpu.two.params.linear.slope     = p[225];
  b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_SCALAR_MUL].mem.two.params.constant         = p[226];

  set_one_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_UNCOMPRESS], p, 227U, 228U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_FINAL_VERIFY], p, 229U, 230U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_MILLER_LOOP], p, 231U, 232U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_BLS12_381_MUL_ML_RESULT], p, 233U, 234U);

  set_one_lin_cpu_const_mem(&b->entries[CARDANO_UPLC_BUILTIN_KECCAK_256], p, 235U, 236U, 237U);
  set_one_lin_cpu_const_mem(&b->entries[CARDANO_UPLC_BUILTIN_BLAKE2B_224], p, 238U, 239U, 240U);

  b->entries[CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING].cpu.three.params.quadratic_in_z.coeff_0 = p[241];
  b->entries[CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING].cpu.three.params.quadratic_in_z.coeff_1 = p[242];
  b->entries[CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING].cpu.three.params.quadratic_in_z.coeff_2 = p[243];
  b->entries[CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING].mem.three.params.linear.intercept       = p[244];
  b->entries[CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING].mem.three.params.linear.slope           = p[245];

  b->entries[CARDANO_UPLC_BUILTIN_BYTE_STRING_TO_INTEGER].cpu.two.params.quadratic_in_y.coeff_0 = p[246];
  b->entries[CARDANO_UPLC_BUILTIN_BYTE_STRING_TO_INTEGER].cpu.two.params.quadratic_in_y.coeff_1 = p[247];
  b->entries[CARDANO_UPLC_BUILTIN_BYTE_STRING_TO_INTEGER].cpu.two.params.quadratic_in_y.coeff_2 = p[248];
  b->entries[CARDANO_UPLC_BUILTIN_BYTE_STRING_TO_INTEGER].mem.two.params.linear.intercept       = p[249];
  b->entries[CARDANO_UPLC_BUILTIN_BYTE_STRING_TO_INTEGER].mem.two.params.linear.slope           = p[250];

  b->entries[CARDANO_UPLC_BUILTIN_AND_BYTE_STRING].cpu.three.params.linear_in_y_and_z.intercept = p[251];
  b->entries[CARDANO_UPLC_BUILTIN_AND_BYTE_STRING].cpu.three.params.linear_in_y_and_z.slope1    = p[252];
  b->entries[CARDANO_UPLC_BUILTIN_AND_BYTE_STRING].cpu.three.params.linear_in_y_and_z.slope2    = p[253];
  b->entries[CARDANO_UPLC_BUILTIN_AND_BYTE_STRING].mem.three.params.linear.intercept            = p[254];
  b->entries[CARDANO_UPLC_BUILTIN_AND_BYTE_STRING].mem.three.params.linear.slope                = p[255];

  b->entries[CARDANO_UPLC_BUILTIN_OR_BYTE_STRING].cpu.three.params.linear_in_y_and_z.intercept = p[256];
  b->entries[CARDANO_UPLC_BUILTIN_OR_BYTE_STRING].cpu.three.params.linear_in_y_and_z.slope1    = p[257];
  b->entries[CARDANO_UPLC_BUILTIN_OR_BYTE_STRING].cpu.three.params.linear_in_y_and_z.slope2    = p[258];
  b->entries[CARDANO_UPLC_BUILTIN_OR_BYTE_STRING].mem.three.params.linear.intercept            = p[259];
  b->entries[CARDANO_UPLC_BUILTIN_OR_BYTE_STRING].mem.three.params.linear.slope                = p[260];

  b->entries[CARDANO_UPLC_BUILTIN_XOR_BYTE_STRING].cpu.three.params.linear_in_y_and_z.intercept = p[261];
  b->entries[CARDANO_UPLC_BUILTIN_XOR_BYTE_STRING].cpu.three.params.linear_in_y_and_z.slope1    = p[262];
  b->entries[CARDANO_UPLC_BUILTIN_XOR_BYTE_STRING].cpu.three.params.linear_in_y_and_z.slope2    = p[263];
  b->entries[CARDANO_UPLC_BUILTIN_XOR_BYTE_STRING].mem.three.params.linear.intercept            = p[264];
  b->entries[CARDANO_UPLC_BUILTIN_XOR_BYTE_STRING].mem.three.params.linear.slope                = p[265];

  set_one_lin(&b->entries[CARDANO_UPLC_BUILTIN_COMPLEMENT_BYTE_STRING], p, 266U, 267U, 268U, 269U);
  set_two_const(&b->entries[CARDANO_UPLC_BUILTIN_READ_BIT], p, 270U, 271U);

  b->entries[CARDANO_UPLC_BUILTIN_WRITE_BITS].cpu.three.params.linear.intercept = p[272];
  b->entries[CARDANO_UPLC_BUILTIN_WRITE_BITS].cpu.three.params.linear.slope     = p[273];
  b->entries[CARDANO_UPLC_BUILTIN_WRITE_BITS].mem.three.params.linear.intercept = p[274];
  b->entries[CARDANO_UPLC_BUILTIN_WRITE_BITS].mem.three.params.linear.slope     = p[275];

  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_REPLICATE_BYTE], p, 276U, 277U, 278U, 279U);
  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_SHIFT_BYTE_STRING], p, 280U, 281U, 282U, 283U);
  set_two_lin(&b->entries[CARDANO_UPLC_BUILTIN_ROTATE_BYTE_STRING], p, 284U, 285U, 286U, 287U);
  set_one_lin_cpu_const_mem(&b->entries[CARDANO_UPLC_BUILTIN_COUNT_SET_BITS], p, 288U, 289U, 290U);
  set_one_lin_cpu_const_mem(&b->entries[CARDANO_UPLC_BUILTIN_FIND_FIRST_SET_BIT], p, 291U, 292U, 293U);
  set_one_lin_cpu_const_mem(&b->entries[CARDANO_UPLC_BUILTIN_RIPEMD_160], p, 294U, 295U, 296U);
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_cost_model_from_params(
  cardano_uplc_cost_model_version_t version,
  const int64_t*                    params,
  size_t                            count,
  cardano_uplc_cost_model_t*        out)
{
  if ((params == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  switch (version)
  {
    case CARDANO_UPLC_COST_MODEL_VERSION_V1:
    {
      if (count != CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V1)
      {
        return CARDANO_ERROR_INVALID_ARGUMENT;
      }

      from_params_v1(out, params);
      break;
    }
    case CARDANO_UPLC_COST_MODEL_VERSION_V2:
    {
      if (count != CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V2)
      {
        return CARDANO_ERROR_INVALID_ARGUMENT;
      }

      from_params_v2(out, params);
      break;
    }
    case CARDANO_UPLC_COST_MODEL_VERSION_V3:
    {
      if (count != CARDANO_UPLC_COST_MODEL_PARAM_COUNT_V3)
      {
        return CARDANO_ERROR_INVALID_ARGUMENT;
      }

      from_params_v3(out, params);
      break;
    }
    default:
    {
      return CARDANO_ERROR_INVALID_ARGUMENT;
    }
  }

  return CARDANO_SUCCESS;
}
