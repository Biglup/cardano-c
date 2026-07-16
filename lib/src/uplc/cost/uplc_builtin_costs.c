/**
 * \file uplc_builtin_costs.c
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

#include "uplc_builtin_costs.h"
#include "../ast/uplc_int.h"
#include "../builtins/uplc_builtin.h"
#include "../data/uplc_data.h"
#include "../machine/uplc_value.h"
#include "uplc_builtin_cost.h"
#include "uplc_ex_mem.h"

#include <cardano/common/bigint.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief The prohibitive sentinel cost for a builtin not in a version.
 */
static const int64_t CARDANO_UPLC_UNAVAILABLE_BUILTIN_COST = 30000000000;

/**
 * \brief The byte-string chunk used to convert a literal width to an ex-mem size.
 *
 * The width literal of \c integerToByteString and \c replicateByte is converted
 * to a size of \c 0 when zero else \c ((n - 1) / 8) + 1.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const int64_t CARDANO_UPLC_COST_SIZE_CHUNK = 8;

/**
 * \brief The maximum output length the width-to-size conversion accepts before clamping.
 *
 * This is the maximum output length of \c integerToByteString.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const int64_t CARDANO_UPLC_COST_SIZE_MAX = 8192;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Builds a one-argument constant-cost costing function.
 *
 * \param[in] c The flat cost.
 *
 * \return The costing function.
 */
static cardano_uplc_one_arg_cost_t
one_const(int64_t c)
{
  cardano_uplc_one_arg_cost_t fn;
  (void)memset(&fn, 0, sizeof(fn));
  fn.kind            = CARDANO_UPLC_ONE_ARG_CONSTANT;
  fn.params.constant = c;
  return fn;
}

/**
 * \brief Builds a one-argument linear costing function.
 *
 * \param[in] intercept The constant term.
 * \param[in] slope The coefficient of the size.
 *
 * \return The costing function.
 */
static cardano_uplc_one_arg_cost_t
one_linear(int64_t intercept, int64_t slope)
{
  cardano_uplc_one_arg_cost_t fn;
  (void)memset(&fn, 0, sizeof(fn));
  fn.kind                    = CARDANO_UPLC_ONE_ARG_LINEAR;
  fn.params.linear.intercept = intercept;
  fn.params.linear.slope     = slope;
  return fn;
}

/**
 * \brief Builds a two-argument constant-cost costing function.
 *
 * \param[in] c The flat cost.
 *
 * \return The costing function.
 */
static cardano_uplc_two_arg_cost_t
two_const(int64_t c)
{
  cardano_uplc_two_arg_cost_t fn;
  (void)memset(&fn, 0, sizeof(fn));
  fn.kind            = CARDANO_UPLC_TWO_ARG_CONSTANT;
  fn.params.constant = c;
  return fn;
}

/**
 * \brief Builds a two-argument linear costing function of the given kind.
 *
 * Used for the LINEAR_IN_X, LINEAR_IN_Y, ADDED_SIZES, MULTIPLIED_SIZES, MIN_SIZE
 * and MAX_SIZE arms, all of which carry a plain linear pair.
 *
 * \param[in] kind The two-argument arm.
 * \param[in] intercept The constant term.
 * \param[in] slope The slope.
 *
 * \return The costing function.
 */
static cardano_uplc_two_arg_cost_t
two_linear(cardano_uplc_two_arg_kind_t kind, int64_t intercept, int64_t slope)
{
  cardano_uplc_two_arg_cost_t fn;
  (void)memset(&fn, 0, sizeof(fn));
  fn.kind                    = kind;
  fn.params.linear.intercept = intercept;
  fn.params.linear.slope     = slope;
  return fn;
}

/**
 * \brief Builds a two-argument subtracted-sizes costing function.
 *
 * \param[in] intercept The constant term.
 * \param[in] slope The slope of the floored difference.
 * \param[in] minimum The lower bound on the difference.
 *
 * \return The costing function.
 */
static cardano_uplc_two_arg_cost_t
two_subtracted(int64_t intercept, int64_t slope, int64_t minimum)
{
  cardano_uplc_two_arg_cost_t fn;
  (void)memset(&fn, 0, sizeof(fn));
  fn.kind                              = CARDANO_UPLC_TWO_ARG_SUBTRACTED_SIZES;
  fn.params.subtracted_sizes.intercept = intercept;
  fn.params.subtracted_sizes.slope     = slope;
  fn.params.subtracted_sizes.minimum   = minimum;
  return fn;
}

/**
 * \brief Builds a two-argument linear-on-diagonal costing function.
 *
 * \param[in] constant The flat cost off the diagonal.
 * \param[in] intercept The constant term on the diagonal.
 * \param[in] slope The slope on the diagonal.
 *
 * \return The costing function.
 */
static cardano_uplc_two_arg_cost_t
two_on_diagonal(int64_t constant, int64_t intercept, int64_t slope)
{
  cardano_uplc_two_arg_cost_t fn;
  (void)memset(&fn, 0, sizeof(fn));
  fn.kind                                = CARDANO_UPLC_TWO_ARG_LINEAR_ON_DIAGONAL;
  fn.params.linear_on_diagonal.constant  = constant;
  fn.params.linear_on_diagonal.intercept = intercept;
  fn.params.linear_on_diagonal.slope     = slope;
  return fn;
}

/**
 * \brief Builds a two-argument const-above-diagonal over multiplied sizes.
 *
 * \param[in] constant The flat cost when x is below the diagonal.
 * \param[in] intercept The intercept of the off-diagonal multiplied-sizes model.
 * \param[in] slope The slope of the off-diagonal multiplied-sizes model.
 *
 * \return The costing function.
 */
/**
 * \brief Builds a one-argument quadratic costing function.
 *
 * \param[in] c0 The constant term.
 * \param[in] c1 The linear coefficient.
 * \param[in] c2 The quadratic coefficient.
 *
 * \return The costing function.
 */
static cardano_uplc_one_arg_cost_t
one_quadratic(int64_t c0, int64_t c1, int64_t c2)
{
  cardano_uplc_one_arg_cost_t fn;
  (void)memset(&fn, 0, sizeof(fn));
  fn.kind                     = CARDANO_UPLC_ONE_ARG_QUADRATIC;
  fn.params.quadratic.coeff_0 = c0;
  fn.params.quadratic.coeff_1 = c1;
  fn.params.quadratic.coeff_2 = c2;
  return fn;
}

/**
 * \brief Builds a two-argument bilinear-with-interaction costing function.
 *
 * \param[in] c00 The constant term.
 * \param[in] c10 The coefficient of x.
 * \param[in] c01 The coefficient of y.
 * \param[in] c11 The coefficient of x*y.
 *
 * \return The costing function.
 */
static cardano_uplc_two_arg_cost_t
two_with_interaction(int64_t c00, int64_t c10, int64_t c01, int64_t c11)
{
  cardano_uplc_two_arg_cost_t fn;
  (void)memset(&fn, 0, sizeof(fn));
  fn.kind                        = CARDANO_UPLC_TWO_ARG_WITH_INTERACTION;
  fn.params.with_interaction.c00 = c00;
  fn.params.with_interaction.c10 = c10;
  fn.params.with_interaction.c01 = c01;
  fn.params.with_interaction.c11 = c11;
  return fn;
}

/**
 * \brief Builds a two-argument const-above-diagonal over a two-variable linear model.
 *
 * Off the diagonal (x < y) the flat \p constant is charged; otherwise the model
 * \c intercept + \c slope_x * x + \c slope_y * y applies. This is the
 * const-above-diagonal over linear-in-x-and-y shape used by \c valueContains.
 *
 * \param[in] constant The flat off-diagonal cost.
 * \param[in] intercept The constant term of the linear model.
 * \param[in] slope_x The coefficient of x.
 * \param[in] slope_y The coefficient of y.
 *
 * \return The costing function.
 */
static cardano_uplc_two_arg_cost_t
two_const_above_linear(int64_t constant, int64_t intercept, int64_t slope_x, int64_t slope_y)
{
  cardano_uplc_two_arg_cost_t fn;
  (void)memset(&fn, 0, sizeof(fn));
  fn.kind                                                    = CARDANO_UPLC_TWO_ARG_CONST_ABOVE_DIAGONAL;
  fn.params.const_diagonal.constant                          = constant;
  fn.params.const_diagonal.kind                              = CARDANO_UPLC_DIAG_MODEL_LINEAR_IN_X_AND_Y;
  fn.params.const_diagonal.model.linear_in_x_and_y.intercept = intercept;
  fn.params.const_diagonal.model.linear_in_x_and_y.slope1    = slope_x;
  fn.params.const_diagonal.model.linear_in_x_and_y.slope2    = slope_y;
  return fn;
}

static cardano_uplc_two_arg_cost_t
two_const_above_mult(int64_t constant, int64_t intercept, int64_t slope)
{
  cardano_uplc_two_arg_cost_t fn;
  (void)memset(&fn, 0, sizeof(fn));
  fn.kind                                                   = CARDANO_UPLC_TWO_ARG_CONST_ABOVE_DIAGONAL;
  fn.params.const_diagonal.constant                         = constant;
  fn.params.const_diagonal.kind                             = CARDANO_UPLC_DIAG_MODEL_MULTIPLIED_SIZES;
  fn.params.const_diagonal.model.multiplied_sizes.intercept = intercept;
  fn.params.const_diagonal.model.multiplied_sizes.slope     = slope;
  return fn;
}

/**
 * \brief Builds a two-argument const-above-diagonal-into-quadratic function.
 *
 * \param[in] constant The flat cost when x is below the diagonal.
 * \param[in] minimum The lower bound on the quadratic.
 * \param[in] c00 The constant quadratic coefficient.
 * \param[in] c10 The coefficient of x.
 * \param[in] c01 The coefficient of y.
 * \param[in] c20 The coefficient of x*x.
 * \param[in] c11 The coefficient of x*y.
 * \param[in] c02 The coefficient of y*y.
 *
 * \return The costing function.
 */
static cardano_uplc_two_arg_cost_t
two_const_above_quad(
  int64_t constant,
  int64_t minimum,
  int64_t c00,
  int64_t c10,
  int64_t c01,
  int64_t c20,
  int64_t c11,
  int64_t c02)
{
  cardano_uplc_two_arg_cost_t fn;
  (void)memset(&fn, 0, sizeof(fn));
  fn.kind                                                 = CARDANO_UPLC_TWO_ARG_CONST_ABOVE_DIAGONAL_INTO_QUADRATIC;
  fn.params.const_above_into_quadratic.constant           = constant;
  fn.params.const_above_into_quadratic.quadratic.minimum  = minimum;
  fn.params.const_above_into_quadratic.quadratic.coeff_00 = c00;
  fn.params.const_above_into_quadratic.quadratic.coeff_10 = c10;
  fn.params.const_above_into_quadratic.quadratic.coeff_01 = c01;
  fn.params.const_above_into_quadratic.quadratic.coeff_20 = c20;
  fn.params.const_above_into_quadratic.quadratic.coeff_11 = c11;
  fn.params.const_above_into_quadratic.quadratic.coeff_02 = c02;
  return fn;
}

/**
 * \brief Builds a two-argument quadratic-in-y costing function.
 *
 * \param[in] c0 The constant term.
 * \param[in] c1 The linear coefficient.
 * \param[in] c2 The quadratic coefficient.
 *
 * \return The costing function.
 */
static cardano_uplc_two_arg_cost_t
two_quadratic_in_y(int64_t c0, int64_t c1, int64_t c2)
{
  cardano_uplc_two_arg_cost_t fn;
  (void)memset(&fn, 0, sizeof(fn));
  fn.kind                          = CARDANO_UPLC_TWO_ARG_QUADRATIC_IN_Y;
  fn.params.quadratic_in_y.coeff_0 = c0;
  fn.params.quadratic_in_y.coeff_1 = c1;
  fn.params.quadratic_in_y.coeff_2 = c2;
  return fn;
}

/**
 * \brief Builds a three-argument constant-cost costing function.
 *
 * \param[in] c The flat cost.
 *
 * \return The costing function.
 */
static cardano_uplc_three_arg_cost_t
three_const(int64_t c)
{
  cardano_uplc_three_arg_cost_t fn;
  (void)memset(&fn, 0, sizeof(fn));
  fn.kind            = CARDANO_UPLC_THREE_ARG_CONSTANT;
  fn.params.constant = c;
  return fn;
}

/**
 * \brief Builds a three-argument linear costing function of the given kind.
 *
 * \param[in] kind The three-argument arm carrying a plain linear pair.
 * \param[in] intercept The constant term.
 * \param[in] slope The slope.
 *
 * \return The costing function.
 */
static cardano_uplc_three_arg_cost_t
three_linear(cardano_uplc_three_arg_kind_t kind, int64_t intercept, int64_t slope)
{
  cardano_uplc_three_arg_cost_t fn;
  (void)memset(&fn, 0, sizeof(fn));
  fn.kind                    = kind;
  fn.params.linear.intercept = intercept;
  fn.params.linear.slope     = slope;
  return fn;
}

/**
 * \brief Builds a three-argument quadratic-in-z costing function.
 *
 * \param[in] c0 The constant term.
 * \param[in] c1 The linear coefficient.
 * \param[in] c2 The quadratic coefficient.
 *
 * \return The costing function.
 */
static cardano_uplc_three_arg_cost_t
three_quadratic_in_z(int64_t c0, int64_t c1, int64_t c2)
{
  cardano_uplc_three_arg_cost_t fn;
  (void)memset(&fn, 0, sizeof(fn));
  fn.kind                          = CARDANO_UPLC_THREE_ARG_QUADRATIC_IN_Z;
  fn.params.quadratic_in_z.coeff_0 = c0;
  fn.params.quadratic_in_z.coeff_1 = c1;
  fn.params.quadratic_in_z.coeff_2 = c2;
  return fn;
}

/**
 * \brief Builds a three-argument linear-in-y-and-z costing function.
 *
 * \param[in] intercept The constant term.
 * \param[in] slope1 The coefficient of y.
 * \param[in] slope2 The coefficient of z.
 *
 * \return The costing function.
 */
static cardano_uplc_three_arg_cost_t
three_linear_yz(int64_t intercept, int64_t slope1, int64_t slope2)
{
  cardano_uplc_three_arg_cost_t fn;
  (void)memset(&fn, 0, sizeof(fn));
  fn.kind                               = CARDANO_UPLC_THREE_ARG_LINEAR_IN_Y_AND_Z;
  fn.params.linear_in_y_and_z.intercept = intercept;
  fn.params.linear_in_y_and_z.slope1    = slope1;
  fn.params.linear_in_y_and_z.slope2    = slope2;
  return fn;
}

/**
 * \brief Builds a three-argument modular-exponentiation costing function.
 *
 * \param[in] coeff_00 The constant term.
 * \param[in] coeff_11 The coefficient of the exponent-by-modulus product.
 * \param[in] coeff_12 The coefficient of the exponent-by-modulus-squared product.
 *
 * \return The costing function.
 */
static cardano_uplc_three_arg_cost_t
three_exp_mod(int64_t coeff_00, int64_t coeff_11, int64_t coeff_12)
{
  cardano_uplc_three_arg_cost_t fn;
  (void)memset(&fn, 0, sizeof(fn));
  fn.kind                    = CARDANO_UPLC_THREE_ARG_EXP_MOD;
  fn.params.exp_mod.coeff_00 = coeff_00;
  fn.params.exp_mod.coeff_11 = coeff_11;
  fn.params.exp_mod.coeff_12 = coeff_12;
  return fn;
}

/**
 * \brief Builds a six-argument constant-cost costing function.
 *
 * \param[in] c The flat cost.
 *
 * \return The costing function.
 */
static cardano_uplc_six_arg_cost_t
six_const(int64_t c)
{
  cardano_uplc_six_arg_cost_t fn;
  (void)memset(&fn, 0, sizeof(fn));
  fn.kind            = CARDANO_UPLC_SIX_ARG_CONSTANT;
  fn.params.constant = c;
  return fn;
}

/**
 * \brief Builds a four-argument constant-cost costing function.
 *
 * \param[in] c The flat cost.
 *
 * \return The costing function.
 */
static cardano_uplc_four_arg_cost_t
four_const(int64_t c)
{
  cardano_uplc_four_arg_cost_t fn;
  (void)memset(&fn, 0, sizeof(fn));
  fn.kind            = CARDANO_UPLC_FOUR_ARG_CONSTANT;
  fn.params.constant = c;
  return fn;
}

/**
 * \brief Builds a one-argument builtin cost entry from cpu and mem functions.
 *
 * \param[in] cpu The cpu costing function.
 * \param[in] mem The mem costing function.
 *
 * \return The builtin cost entry.
 */
static cardano_uplc_builtin_cost_t
entry_one(cardano_uplc_one_arg_cost_t cpu, cardano_uplc_one_arg_cost_t mem)
{
  cardano_uplc_builtin_cost_t e;
  (void)memset(&e, 0, sizeof(e));
  e.arity   = CARDANO_UPLC_BUILTIN_COST_ARITY_ONE;
  e.cpu.one = cpu;
  e.mem.one = mem;
  return e;
}

/**
 * \brief Builds a two-argument builtin cost entry from cpu and mem functions.
 *
 * \param[in] cpu The cpu costing function.
 * \param[in] mem The mem costing function.
 *
 * \return The builtin cost entry.
 */
static cardano_uplc_builtin_cost_t
entry_two(cardano_uplc_two_arg_cost_t cpu, cardano_uplc_two_arg_cost_t mem)
{
  cardano_uplc_builtin_cost_t e;
  (void)memset(&e, 0, sizeof(e));
  e.arity   = CARDANO_UPLC_BUILTIN_COST_ARITY_TWO;
  e.cpu.two = cpu;
  e.mem.two = mem;
  return e;
}

/**
 * \brief Builds a three-argument builtin cost entry from cpu and mem functions.
 *
 * \param[in] cpu The cpu costing function.
 * \param[in] mem The mem costing function.
 *
 * \return The builtin cost entry.
 */
static cardano_uplc_builtin_cost_t
entry_three(cardano_uplc_three_arg_cost_t cpu, cardano_uplc_three_arg_cost_t mem)
{
  cardano_uplc_builtin_cost_t e;
  (void)memset(&e, 0, sizeof(e));
  e.arity     = CARDANO_UPLC_BUILTIN_COST_ARITY_THREE;
  e.cpu.three = cpu;
  e.mem.three = mem;
  return e;
}

/**
 * \brief Builds a four-argument builtin cost entry from cpu and mem functions.
 *
 * \param[in] cpu The cpu costing function.
 * \param[in] mem The mem costing function.
 *
 * \return The builtin cost entry.
 */
static cardano_uplc_builtin_cost_t
entry_four(cardano_uplc_four_arg_cost_t cpu, cardano_uplc_four_arg_cost_t mem)
{
  cardano_uplc_builtin_cost_t e;
  (void)memset(&e, 0, sizeof(e));
  e.arity    = CARDANO_UPLC_BUILTIN_COST_ARITY_FOUR;
  e.cpu.four = cpu;
  e.mem.four = mem;
  return e;
}

/**
 * \brief Builds a six-argument builtin cost entry from cpu and mem functions.
 *
 * \param[in] cpu The cpu costing function.
 * \param[in] mem The mem costing function.
 *
 * \return The builtin cost entry.
 */
static cardano_uplc_builtin_cost_t
entry_six(cardano_uplc_six_arg_cost_t cpu, cardano_uplc_six_arg_cost_t mem)
{
  cardano_uplc_builtin_cost_t e;
  (void)memset(&e, 0, sizeof(e));
  e.arity   = CARDANO_UPLC_BUILTIN_COST_ARITY_SIX;
  e.cpu.six = cpu;
  e.mem.six = mem;
  return e;
}

/**
 * \brief Builds a one-argument sentinel cost entry for an unavailable builtin.
 *
 * \return The sentinel one-argument entry.
 */
static cardano_uplc_builtin_cost_t
unavail_one(void)
{
  int64_t s = CARDANO_UPLC_UNAVAILABLE_BUILTIN_COST;
  return entry_one(one_const(s), one_const(s));
}

/**
 * \brief Builds a two-argument sentinel cost entry for an unavailable builtin.
 *
 * \return The sentinel two-argument entry.
 */
static cardano_uplc_builtin_cost_t
unavail_two(void)
{
  int64_t s = CARDANO_UPLC_UNAVAILABLE_BUILTIN_COST;
  return entry_two(two_const(s), two_const(s));
}

/**
 * \brief Builds a three-argument sentinel cost entry for an unavailable builtin.
 *
 * \return The sentinel three-argument entry.
 */
static cardano_uplc_builtin_cost_t
unavail_three(void)
{
  int64_t s = CARDANO_UPLC_UNAVAILABLE_BUILTIN_COST;
  return entry_three(three_const(s), three_const(s));
}

/**
 * \brief Fills the V1-and-V2 shared core builtins into a costs table.
 *
 * Every builtin available in V1 has the same shape and coefficients in V2; this
 * sets those shared entries and leaves the version-specific ones (serialise_data,
 * the secp256k1 verifiers) to the caller.
 *
 * \param[out] c The costs table to populate.
 */
static void
fill_v1_v2_core(cardano_uplc_builtin_costs_t* c)
{
  c->entries[CARDANO_UPLC_BUILTIN_ADD_INTEGER]              = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_MAX_SIZE, 100788, 420), two_linear(CARDANO_UPLC_TWO_ARG_MAX_SIZE, 1, 1));
  c->entries[CARDANO_UPLC_BUILTIN_SUBTRACT_INTEGER]         = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_MAX_SIZE, 100788, 420), two_linear(CARDANO_UPLC_TWO_ARG_MAX_SIZE, 1, 1));
  c->entries[CARDANO_UPLC_BUILTIN_MULTIPLY_INTEGER]         = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_MULTIPLIED_SIZES, 90434, 519), two_linear(CARDANO_UPLC_TWO_ARG_ADDED_SIZES, 0, 1));
  c->entries[CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER]           = entry_two(two_const_above_mult(85848, 228465, 122), two_subtracted(0, 1, 1));
  c->entries[CARDANO_UPLC_BUILTIN_QUOTIENT_INTEGER]         = entry_two(two_const_above_mult(85848, 228465, 122), two_subtracted(0, 1, 1));
  c->entries[CARDANO_UPLC_BUILTIN_REMAINDER_INTEGER]        = entry_two(two_const_above_mult(85848, 228465, 122), two_subtracted(0, 1, 1));
  c->entries[CARDANO_UPLC_BUILTIN_MOD_INTEGER]              = entry_two(two_const_above_mult(85848, 228465, 122), two_subtracted(0, 1, 1));
  c->entries[CARDANO_UPLC_BUILTIN_EQUALS_INTEGER]           = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_MIN_SIZE, 51775, 558), two_const(1));
  c->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_INTEGER]        = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_MIN_SIZE, 44749, 541), two_const(1));
  c->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_INTEGER] = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_MIN_SIZE, 43285, 552), two_const(1));

  c->entries[CARDANO_UPLC_BUILTIN_APPEND_BYTE_STRING]           = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_ADDED_SIZES, 1000, 173), two_linear(CARDANO_UPLC_TWO_ARG_ADDED_SIZES, 0, 1));
  c->entries[CARDANO_UPLC_BUILTIN_CONS_BYTE_STRING]             = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_LINEAR_IN_Y, 72010, 178), two_linear(CARDANO_UPLC_TWO_ARG_ADDED_SIZES, 0, 1));
  c->entries[CARDANO_UPLC_BUILTIN_SLICE_BYTE_STRING]            = entry_three(three_linear(CARDANO_UPLC_THREE_ARG_LINEAR_IN_Z, 20467, 1), three_linear(CARDANO_UPLC_THREE_ARG_LINEAR_IN_Z, 4, 0));
  c->entries[CARDANO_UPLC_BUILTIN_LENGTH_OF_BYTE_STRING]        = entry_one(one_const(22100), one_const(10));
  c->entries[CARDANO_UPLC_BUILTIN_INDEX_BYTE_STRING]            = entry_two(two_const(13169), two_const(4));
  c->entries[CARDANO_UPLC_BUILTIN_EQUALS_BYTE_STRING]           = entry_two(two_on_diagonal(24548, 29498, 38), two_const(1));
  c->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_BYTE_STRING]        = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_MIN_SIZE, 28999, 74), two_const(1));
  c->entries[CARDANO_UPLC_BUILTIN_LESS_THAN_EQUALS_BYTE_STRING] = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_MIN_SIZE, 28999, 74), two_const(1));

  c->entries[CARDANO_UPLC_BUILTIN_SHA2_256]                 = entry_one(one_linear(270652, 22588), one_const(4));
  c->entries[CARDANO_UPLC_BUILTIN_SHA3_256]                 = entry_one(one_linear(1457325, 64566), one_const(4));
  c->entries[CARDANO_UPLC_BUILTIN_BLAKE2B_256]              = entry_one(one_linear(201305, 8356), one_const(4));
  c->entries[CARDANO_UPLC_BUILTIN_VERIFY_ED25519_SIGNATURE] = entry_three(three_linear(CARDANO_UPLC_THREE_ARG_LINEAR_IN_Y, 53384111, 14333), three_const(10));

  c->entries[CARDANO_UPLC_BUILTIN_APPEND_STRING] = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_ADDED_SIZES, 1000, 59957), two_linear(CARDANO_UPLC_TWO_ARG_ADDED_SIZES, 4, 1));
  c->entries[CARDANO_UPLC_BUILTIN_EQUALS_STRING] = entry_two(two_on_diagonal(39184, 1000, 60594), two_const(1));
  c->entries[CARDANO_UPLC_BUILTIN_ENCODE_UTF8]   = entry_one(one_linear(1000, 42921), one_linear(4, 2));
  c->entries[CARDANO_UPLC_BUILTIN_DECODE_UTF8]   = entry_one(one_linear(91189, 769), one_linear(4, 2));

  c->entries[CARDANO_UPLC_BUILTIN_IF_THEN_ELSE]     = entry_three(three_const(76049), three_const(1));
  c->entries[CARDANO_UPLC_BUILTIN_CHOOSE_UNIT]      = entry_two(two_const(61462), two_const(4));
  c->entries[CARDANO_UPLC_BUILTIN_TRACE]            = entry_two(two_const(59498), two_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_FST_PAIR]         = entry_one(one_const(141895), one_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_SND_PAIR]         = entry_one(one_const(141992), one_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_CHOOSE_LIST]      = entry_three(three_const(132994), three_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_MK_CONS]          = entry_two(two_const(72362), two_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_HEAD_LIST]        = entry_one(one_const(83150), one_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_TAIL_LIST]        = entry_one(one_const(81663), one_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_NULL_LIST]        = entry_one(one_const(74433), one_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_CHOOSE_DATA]      = entry_six(six_const(94375), six_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_CONSTR_DATA]      = entry_two(two_const(22151), two_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_MAP_DATA]         = entry_one(one_const(68246), one_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_LIST_DATA]        = entry_one(one_const(33852), one_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_I_DATA]           = entry_one(one_const(15299), one_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_B_DATA]           = entry_one(one_const(11183), one_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_UN_CONSTR_DATA]   = entry_one(one_const(24588), one_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_UN_MAP_DATA]      = entry_one(one_const(24623), one_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_UN_LIST_DATA]     = entry_one(one_const(25933), one_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_UN_I_DATA]        = entry_one(one_const(20744), one_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_UN_B_DATA]        = entry_one(one_const(20142), one_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_EQUALS_DATA]      = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_MIN_SIZE, 898148, 27279), two_const(1));
  c->entries[CARDANO_UPLC_BUILTIN_MK_PAIR_DATA]     = entry_two(two_const(11546), two_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_MK_NIL_DATA]      = entry_one(one_const(7243), one_const(32));
  c->entries[CARDANO_UPLC_BUILTIN_MK_NIL_PAIR_DATA] = entry_one(one_const(7391), one_const(32));
}

/**
 * \brief Sets every entry of a costs table to the unavailable sentinel.
 *
 * Each builtin is set to the sentinel of its own arity so the dispatch reads the
 * active union member. The caller overwrites the entries available in a version.
 *
 * \param[out] c The costs table to fill with sentinels.
 */
static void
fill_all_unavail(cardano_uplc_builtin_costs_t* c)
{
  size_t  i = 0U;
  int64_t s = CARDANO_UPLC_UNAVAILABLE_BUILTIN_COST;

  (void)memset(c, 0, sizeof(*c));

  for (i = 0U; i < (size_t)CARDANO_UPLC_BUILTIN_COUNT; ++i)
  {
    cardano_uplc_builtin_t func  = (cardano_uplc_builtin_t)i;
    size_t                 arity = 2U;

    (void)cardano_uplc_builtin_arity(func, &arity);

    switch (arity)
    {
      case 1U:
      {
        c->entries[i] = unavail_one();
        break;
      }
      case 3U:
      {
        c->entries[i] = unavail_three();
        break;
      }
      case 4U:
      {
        c->entries[i] = entry_four(four_const(s), four_const(s));
        break;
      }
      case 6U:
      {
        c->entries[i] = entry_six(six_const(s), six_const(s));
        break;
      }
      case 2U:
      default:
      {
        c->entries[i] = unavail_two();
        break;
      }
    }
  }
}

/**
 * \brief Fills the builtins shared by every version's list from protocol
 *        version 11 (van Rossem) onwards, with the enacted default coefficients.
 *
 * Covers serialise_data, the secp256k1 verifiers, the additional hashes, the
 * BLS12-381 family, the integer/byte-string conversions, the bitwise family,
 * exp_mod_integer and the batch-6 builtins. Version 11 unified the builtin set
 * across the three languages, so V1 and V2 carry these entries too.
 *
 * \param[in,out] c The builtin costs to fill.
 */
static void
fill_shared_extension(cardano_uplc_builtin_costs_t* c)
{
  c->entries[CARDANO_UPLC_BUILTIN_SERIALISE_DATA]                     = entry_one(one_linear(955506, 213312), one_linear(0, 2));
  c->entries[CARDANO_UPLC_BUILTIN_VERIFY_ECDSA_SECP256K1_SIGNATURE]   = entry_three(three_const(43053543), three_const(10));
  c->entries[CARDANO_UPLC_BUILTIN_VERIFY_SCHNORR_SECP256K1_SIGNATURE] = entry_three(three_linear(CARDANO_UPLC_THREE_ARG_LINEAR_IN_Y, 43574283, 26308), three_const(10));

  c->entries[CARDANO_UPLC_BUILTIN_BLAKE2B_224] = entry_one(one_linear(207616, 8310), one_const(4));
  c->entries[CARDANO_UPLC_BUILTIN_KECCAK_256]  = entry_one(one_linear(2261318, 64571), one_const(4));

  c->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_ADD]           = entry_two(two_const(962335), two_const(18));
  c->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_NEG]           = entry_one(one_const(267929), one_const(18));
  c->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_SCALAR_MUL]    = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_LINEAR_IN_X, 76433006, 8868), two_const(18));
  c->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_EQUAL]         = entry_two(two_const(442008), two_const(1));
  c->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_COMPRESS]      = entry_one(one_const(2780678), one_const(6));
  c->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_UNCOMPRESS]    = entry_one(one_const(52948122), one_const(18));
  c->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_HASH_TO_GROUP] = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_LINEAR_IN_X, 52538055, 3756), two_const(18));

  c->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_ADD]           = entry_two(two_const(1995836), two_const(36));
  c->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_NEG]           = entry_one(one_const(284546), one_const(36));
  c->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_SCALAR_MUL]    = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_LINEAR_IN_X, 158221314, 26549), two_const(36));
  c->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_EQUAL]         = entry_two(two_const(901022), two_const(1));
  c->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_COMPRESS]      = entry_one(one_const(3227919), one_const(12));
  c->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_UNCOMPRESS]    = entry_one(one_const(74698472), one_const(36));
  c->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_HASH_TO_GROUP] = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_LINEAR_IN_X, 166917843, 4307), two_const(36));

  c->entries[CARDANO_UPLC_BUILTIN_BLS12_381_MILLER_LOOP]   = entry_two(two_const(254006273), two_const(72));
  c->entries[CARDANO_UPLC_BUILTIN_BLS12_381_MUL_ML_RESULT] = entry_two(two_const(2174038), two_const(72));
  c->entries[CARDANO_UPLC_BUILTIN_BLS12_381_FINAL_VERIFY]  = entry_two(two_const(333849714), two_const(1));

  c->entries[CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING] = entry_three(three_quadratic_in_z(1293828, 28716, 63), three_linear(CARDANO_UPLC_THREE_ARG_LITERAL_IN_Y_OR_LINEAR_IN_Z, 0, 1));
  c->entries[CARDANO_UPLC_BUILTIN_BYTE_STRING_TO_INTEGER] = entry_two(two_quadratic_in_y(1006041, 43623, 251), two_linear(CARDANO_UPLC_TWO_ARG_LINEAR_IN_Y, 0, 1));
  c->entries[CARDANO_UPLC_BUILTIN_AND_BYTE_STRING]        = entry_three(three_linear_yz(100181, 726, 719), three_linear(CARDANO_UPLC_THREE_ARG_LINEAR_IN_MAX_YZ, 0, 1));
  c->entries[CARDANO_UPLC_BUILTIN_OR_BYTE_STRING]         = entry_three(three_linear_yz(100181, 726, 719), three_linear(CARDANO_UPLC_THREE_ARG_LINEAR_IN_MAX_YZ, 0, 1));
  c->entries[CARDANO_UPLC_BUILTIN_XOR_BYTE_STRING]        = entry_three(three_linear_yz(100181, 726, 719), three_linear(CARDANO_UPLC_THREE_ARG_LINEAR_IN_MAX_YZ, 0, 1));
  c->entries[CARDANO_UPLC_BUILTIN_COMPLEMENT_BYTE_STRING] = entry_one(one_linear(107878, 680), one_linear(0, 1));
  c->entries[CARDANO_UPLC_BUILTIN_READ_BIT]               = entry_two(two_const(95336), two_const(1));
  c->entries[CARDANO_UPLC_BUILTIN_WRITE_BITS]             = entry_three(three_linear(CARDANO_UPLC_THREE_ARG_LINEAR_IN_Y, 281145, 18848), three_linear(CARDANO_UPLC_THREE_ARG_LINEAR_IN_X, 0, 1));
  c->entries[CARDANO_UPLC_BUILTIN_REPLICATE_BYTE]         = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_LINEAR_IN_X, 180194, 159), two_linear(CARDANO_UPLC_TWO_ARG_LINEAR_IN_X, 1, 1));
  c->entries[CARDANO_UPLC_BUILTIN_SHIFT_BYTE_STRING]      = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_LINEAR_IN_X, 158519, 8942), two_linear(CARDANO_UPLC_TWO_ARG_LINEAR_IN_X, 0, 1));
  c->entries[CARDANO_UPLC_BUILTIN_ROTATE_BYTE_STRING]     = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_LINEAR_IN_X, 159378, 8813), two_linear(CARDANO_UPLC_TWO_ARG_LINEAR_IN_X, 0, 1));
  c->entries[CARDANO_UPLC_BUILTIN_COUNT_SET_BITS]         = entry_one(one_linear(107490, 3298), one_const(1));
  c->entries[CARDANO_UPLC_BUILTIN_FIND_FIRST_SET_BIT]     = entry_one(one_linear(106057, 655), one_const(1));
  c->entries[CARDANO_UPLC_BUILTIN_RIPEMD_160]             = entry_one(one_linear(1964219, 24520), one_const(3));

  c->entries[CARDANO_UPLC_BUILTIN_EXP_MOD_INTEGER] = entry_three(three_exp_mod(607153, 231697, 53144), three_linear(CARDANO_UPLC_THREE_ARG_LINEAR_IN_Z, 0, 1));

  c->entries[CARDANO_UPLC_BUILTIN_DROP_LIST]       = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_LINEAR_IN_X, 116711, 1957), two_const(4));
  c->entries[CARDANO_UPLC_BUILTIN_LENGTH_OF_ARRAY] = entry_one(one_const(231883), one_const(10));
  c->entries[CARDANO_UPLC_BUILTIN_LIST_TO_ARRAY]   = entry_one(one_linear(1000, 24838), one_linear(7, 1));
  c->entries[CARDANO_UPLC_BUILTIN_INDEX_ARRAY]     = entry_two(two_const(232010), two_const(32));

  c->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G1_MULTI_SCALAR_MUL] = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_LINEAR_IN_X, 321837444, 25087669), two_const(18));
  c->entries[CARDANO_UPLC_BUILTIN_BLS12_381_G2_MULTI_SCALAR_MUL] = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_LINEAR_IN_X, 617887431, 67302824), two_const(36));

  c->entries[CARDANO_UPLC_BUILTIN_INSERT_COIN]    = entry_one(one_linear(356924, 18413), one_linear(45, 21));
  c->entries[CARDANO_UPLC_BUILTIN_LOOKUP_COIN]    = entry_three(three_linear(CARDANO_UPLC_THREE_ARG_LINEAR_IN_Z, 219951, 9444), three_const(1));
  c->entries[CARDANO_UPLC_BUILTIN_UNION_VALUE]    = entry_two(two_with_interaction(1000, 172116, 183150, 6), two_linear(CARDANO_UPLC_TWO_ARG_ADDED_SIZES, 24, 21));
  c->entries[CARDANO_UPLC_BUILTIN_VALUE_CONTAINS] = entry_two(two_const_above_linear(213283, 618401, 1998, 28258), two_const(1));
  c->entries[CARDANO_UPLC_BUILTIN_VALUE_DATA]     = entry_one(one_linear(1000, 38159), one_linear(2, 22));
  c->entries[CARDANO_UPLC_BUILTIN_UN_VALUE_DATA]  = entry_one(one_quadratic(1000, 95933, 1), one_linear(1, 11));
  c->entries[CARDANO_UPLC_BUILTIN_SCALE_VALUE]    = entry_two(two_linear(CARDANO_UPLC_TWO_ARG_LINEAR_IN_Y, 1000, 277577), two_linear(CARDANO_UPLC_TWO_ARG_LINEAR_IN_Y, 12, 21));
}

cardano_uplc_builtin_costs_t
cardano_uplc_builtin_costs_v1(void)
{
  cardano_uplc_builtin_costs_t c;

  fill_all_unavail(&c);
  fill_v1_v2_core(&c);
  fill_shared_extension(&c);

  return c;
}

cardano_uplc_builtin_costs_t
cardano_uplc_builtin_costs_v2(void)
{
  cardano_uplc_builtin_costs_t c;

  fill_all_unavail(&c);
  fill_v1_v2_core(&c);
  fill_shared_extension(&c);

  return c;
}

cardano_uplc_builtin_costs_t
cardano_uplc_builtin_costs_v3(void)
{
  cardano_uplc_builtin_costs_t c;

  fill_all_unavail(&c);
  fill_v1_v2_core(&c);
  fill_shared_extension(&c);

  c.entries[CARDANO_UPLC_BUILTIN_DIVIDE_INTEGER]    = entry_two(two_const_above_quad(85848, 85848, 123203, 1716, 7305, 57, 549, -900), two_subtracted(0, 1, 1));
  c.entries[CARDANO_UPLC_BUILTIN_QUOTIENT_INTEGER]  = entry_two(two_const_above_quad(85848, 85848, 123203, 1716, 7305, 57, 549, -900), two_subtracted(0, 1, 1));
  c.entries[CARDANO_UPLC_BUILTIN_REMAINDER_INTEGER] = entry_two(two_const_above_quad(85848, 85848, 123203, 1716, 7305, 57, 549, -900), two_linear(CARDANO_UPLC_TWO_ARG_LINEAR_IN_Y, 0, 1));
  c.entries[CARDANO_UPLC_BUILTIN_MOD_INTEGER]       = entry_two(two_const_above_quad(85848, 85848, 123203, 1716, 7305, 57, 549, -900), two_linear(CARDANO_UPLC_TWO_ARG_LINEAR_IN_Y, 0, 1));

  return c;
}

/**
 * \brief Returns floor(log2(n)) for a positive count, or 0 for 0.
 *
 * \param[in] n The count.
 *
 * \return The floor of the base-2 logarithm, or 0 when \p n is 0.
 */
static int64_t
floor_log2(size_t n)
{
  int64_t log = 0;

  while (n > 1U)
  {
    // cppcheck-suppress misra-c2012-17.8; Reason: parameter reused as a local iteration cursor
    n   >>= 1U;
    log += 1;
  }

  return log;
}

/**
 * \brief Returns the cost-model max-depth size of a value constant.
 *
 * The size is (log2(outer) + 1 if outer > 0 else 0) + (log2(maxInner) + 1 if
 * maxInner > 0 else 0). Used by \c insertCoin and \c lookupCoin.
 *
 * \param[in] constant The value constant, or NULL.
 *
 * \return The max-depth size.
 */
static int64_t
value_max_depth(const cardano_uplc_constant_t* constant)
{
  size_t  outer     = 0U;
  size_t  max_inner = 0U;
  size_t  i         = 0U;
  int64_t log_outer = 0;
  int64_t log_inner = 0;

  if ((constant == NULL) || (constant->kind != CARDANO_UPLC_TYPE_VALUE))
  {
    return 0;
  }

  outer = constant->as.list.count;

  for (i = 0U; i < outer; ++i)
  {
    size_t inner = cardano_uplc_value_entry_token_count(constant->as.list.items[i]);

    if (inner > max_inner)
    {
      max_inner = inner;
    }
  }

  log_outer = (outer > 0U) ? (floor_log2(outer) + 1) : 0;
  log_inner = (max_inner > 0U) ? (floor_log2(max_inner) + 1) : 0;

  return log_outer + log_inner;
}

/**
 * \brief Returns the constant integer a value holds, or a fallback otherwise.
 *
 * Reads the integer literal of a constant integer value, used by the builtins
 * whose cost depends on a literal argument rather than its ex-mem.
 *
 * \param[in] value The value to inspect, or NULL.
 * \param[in] fallback The value returned when \p value is not a constant integer.
 *
 * \return The integer literal, or \p fallback.
 */
static int64_t
value_integer(const cardano_uplc_value_t* value, int64_t fallback)
{
  if ((value == NULL) || (value->kind != CARDANO_UPLC_VALUE_CONSTANT))
  {
    return fallback;
  }

  if ((value->as.constant == NULL) || (value->as.constant->kind != CARDANO_UPLC_TYPE_INTEGER))
  {
    return fallback;
  }

  if (cardano_uplc_constant_int_is_small(value->as.constant))
  {
    return cardano_uplc_constant_int_small(value->as.constant);
  }

  if (value->as.constant->as.integer.big == NULL)
  {
    return fallback;
  }

  return cardano_bigint_to_int(value->as.constant->as.integer.big);
}

/**
 * \brief Returns the element count a constant list value holds, or 0 otherwise.
 *
 * Used by \c writeBits, which feeds the length of its update list as the second
 * cost size.
 *
 * \param[in] value The value to inspect, or NULL.
 *
 * \return The list length, or 0 when \p value is not a constant list.
 */
static int64_t
value_list_len(const cardano_uplc_value_t* value)
{
  if ((value == NULL) || (value->kind != CARDANO_UPLC_VALUE_CONSTANT))
  {
    return 0;
  }

  if ((value->as.constant == NULL) || ((value->as.constant->kind != CARDANO_UPLC_TYPE_LIST) && (value->as.constant->kind != CARDANO_UPLC_TYPE_ARRAY)))
  {
    return (int64_t)0;
  }

  return (int64_t)value->as.constant->as.list.count;
}

/**
 * \brief Returns the token count of a value argument, or 0 otherwise.
 *
 * \param[in] value The value to inspect, or NULL.
 *
 * \return The total token count, or 0 when \p value is not a value constant.
 */
static int64_t
value_arg_token_count(const cardano_uplc_value_t* value)
{
  if ((value == NULL) || (value->kind != CARDANO_UPLC_VALUE_CONSTANT))
  {
    return 0;
  }

  return cardano_uplc_value_token_count(value->as.constant);
}

/**
 * \brief Returns the max-depth size of a value argument, or 0 otherwise.
 *
 * \param[in] value The value to inspect, or NULL.
 *
 * \return The max-depth size, or 0 when \p value is not a value constant.
 */
static int64_t
value_arg_max_depth(const cardano_uplc_value_t* value)
{
  if ((value == NULL) || (value->kind != CARDANO_UPLC_VALUE_CONSTANT))
  {
    return 0;
  }

  return value_max_depth(value->as.constant);
}

/**
 * \brief Returns the data-node count of a data argument, or 0 otherwise.
 *
 * \param[in] value The value to inspect, or NULL.
 *
 * \return The node count, or 0 when \p value is not a data constant.
 */
static int64_t
value_arg_data_node_count(const cardano_uplc_value_t* value)
{
  if ((value == NULL) || (value->kind != CARDANO_UPLC_VALUE_CONSTANT))
  {
    return 0;
  }

  if ((value->as.constant == NULL) || (value->as.constant->kind != CARDANO_UPLC_TYPE_DATA))
  {
    return 0;
  }

  return cardano_uplc_data_node_count(value->as.constant->as.data);
}

/**
 * \brief Returns the absolute value of a constant integer literal, saturating.
 *
 * Used by \c dropList, whose cpu cost is linear in the count it is asked to drop,
 * taken as the absolute value of the literal (a negative drop is costed by its
 * magnitude). A literal too large to fit a signed 64-bit value saturates to
 * \c INT64_MAX.
 *
 * \param[in] value The value to inspect, or NULL.
 *
 * \return The saturating absolute literal, or 0 when \p value is not a constant
 *         integer.
 */
static int64_t
value_abs_integer_saturating(const cardano_uplc_value_t* value)
{
  const cardano_bigint_t* integer = NULL;

  if ((value == NULL) || (value->kind != CARDANO_UPLC_VALUE_CONSTANT))
  {
    return 0;
  }

  if ((value->as.constant == NULL) || (value->as.constant->kind != CARDANO_UPLC_TYPE_INTEGER))
  {
    return 0;
  }

  if (cardano_uplc_constant_int_is_small(value->as.constant))
  {
    const int64_t raw = cardano_uplc_constant_int_small(value->as.constant);

    if (raw == (int64_t)INT64_MIN)
    {
      return INT64_MAX;
    }

    return (raw < 0) ? -raw : raw;
  }

  integer = value->as.constant->as.integer.big;

  if ((integer == NULL) || (cardano_bigint_bit_length(integer) > 63U))
  {
    return INT64_MAX;
  }

  {
    const int64_t raw = cardano_bigint_to_int(integer);

    if (raw == (int64_t)INT64_MIN)
    {
      return INT64_MAX;
    }

    return (raw < 0) ? -raw : raw;
  }
}

/**
 * \brief Converts a literal width to a byte-string ex-mem size, saturating.
 *
 * Negative widths and widths above the maximum are clamped, then the size is 0
 * when the width is 0 else \c ((width - 1) / 8) + 1.
 *
 * \param[in] width The literal width.
 *
 * \return The byte-string ex-mem size of an output of that width.
 */
static int64_t
cost_as_size(int64_t width)
{
  if (width <= 0)
  {
    return 0;
  }

  if (width > CARDANO_UPLC_COST_SIZE_MAX)
  {
    // cppcheck-suppress misra-c2012-17.8; Reason: parameter reused as a local iteration cursor
    width = CARDANO_UPLC_COST_SIZE_MAX;
  }

  return ((width - 1) / CARDANO_UPLC_COST_SIZE_CHUNK) + 1;
}

/**
 * \brief Returns the absolute value of a signed 64-bit integer, saturating.
 *
 * \param[in] v The value.
 *
 * \return The absolute value, clamped to INT64_MAX for INT64_MIN.
 */
static int64_t
abs_i64(int64_t v)
{
  if (v == (int64_t)INT64_MIN)
  {
    return INT64_MAX;
  }

  return (v < 0) ? -v : v;
}

/**
 * \brief Evaluates a builtin cost entry over up to six argument sizes.
 *
 * Selects the active union member by the entry arity and evaluates both the cpu
 * and mem cost functions over the supplied sizes in a single arity dispatch.
 *
 * \param[in] e The builtin cost entry.
 * \param[in] s The six argument sizes; unused sizes are ignored by the arm.
 *
 * \return The saturating cpu and mem cost.
 */
static inline cardano_uplc_budget_t
eval_entry(const cardano_uplc_builtin_cost_t* e, const int64_t s[6])
{
  cardano_uplc_budget_t result = { 0, 0 };

  switch (e->arity)
  {
    case CARDANO_UPLC_BUILTIN_COST_ARITY_ONE:
    {
      result.cpu = cardano_uplc_one_arg_eval(&e->cpu.one, s[0]);
      result.mem = cardano_uplc_one_arg_eval(&e->mem.one, s[0]);
      break;
    }
    case CARDANO_UPLC_BUILTIN_COST_ARITY_TWO:
    {
      result.cpu = cardano_uplc_two_arg_eval(&e->cpu.two, s[0], s[1]);
      result.mem = cardano_uplc_two_arg_eval(&e->mem.two, s[0], s[1]);
      break;
    }
    case CARDANO_UPLC_BUILTIN_COST_ARITY_THREE:
    {
      result.cpu = cardano_uplc_three_arg_eval(&e->cpu.three, s[0], s[1], s[2]);
      result.mem = cardano_uplc_three_arg_eval(&e->mem.three, s[0], s[1], s[2]);
      break;
    }
    case CARDANO_UPLC_BUILTIN_COST_ARITY_FOUR:
    {
      result.cpu = cardano_uplc_four_arg_eval(&e->cpu.four, s[0], s[1], s[2], s[3]);
      result.mem = cardano_uplc_four_arg_eval(&e->mem.four, s[0], s[1], s[2], s[3]);
      break;
    }
    case CARDANO_UPLC_BUILTIN_COST_ARITY_SIX:
    {
      result.cpu = cardano_uplc_six_arg_eval(&e->cpu.six, s[0], s[1], s[2], s[3], s[4], s[5]);
      result.mem = cardano_uplc_six_arg_eval(&e->mem.six, s[0], s[1], s[2], s[3], s[4], s[5]);
      break;
    }
    default:
    {
      break;
    }
  }

  return result;
}

/**
 * \brief Returns the ex-mem size of an argument value, fast-pathing the common kinds.
 *
 * Inlines the integer and byte-string measures that dominate the builtin call path
 * and uses the value-level ex-mem cache so its result is identical to
 * \ref cardano_uplc_value_ex_mem. Falls back to the general routine for every other
 * value shape.
 *
 * \param[in] value The argument value, or NULL.
 * \param[in] costs_strings_by_utf8_bytes The string-measure flag for the fallback.
 *
 * \return The ex-mem size of \p value.
 */
static inline int64_t
arg_ex_mem(const cardano_uplc_value_t* value, bool costs_strings_by_utf8_bytes)
{
  const cardano_uplc_constant_t* k = NULL;

  if (value == NULL)
  {
    return 0;
  }

  if (value->ex_mem != CARDANO_UPLC_VALUE_EX_MEM_UNCOMPUTED)
  {
    return value->ex_mem;
  }

  if (value->kind == CARDANO_UPLC_VALUE_CONSTANT)
  {
    k = value->as.constant;

    if ((k != NULL) && (k->kind == CARDANO_UPLC_TYPE_INTEGER) && k->as.integer.is_small)
    {
      // cppcheck-suppress misra-c2012-11.8; Reason: interfacing a non-const-correct API
      ((cardano_uplc_value_t*)((void*)value))->ex_mem = 1;

      return 1;
    }

    if ((k != NULL) && (k->kind == CARDANO_UPLC_TYPE_BYTE_STRING))
    {
      int64_t total = cardano_uplc_byte_string_ex_mem(k->as.bytes.size);

      // cppcheck-suppress misra-c2012-11.8; Reason: interfacing a non-const-correct API
      ((cardano_uplc_value_t*)((void*)value))->ex_mem = total;

      return total;
    }
  }

  return cardano_uplc_value_ex_mem(value, costs_strings_by_utf8_bytes);
}

/* DEFINITIONS ***************************************************************/

cardano_uplc_budget_t
cardano_uplc_builtin_cost(
  const cardano_uplc_builtin_costs_t* costs,
  cardano_uplc_builtin_t              func,
  const cardano_uplc_value_t* const*  args,
  size_t                              arg_count,
  bool                                costs_strings_by_utf8_bytes)
{
  cardano_uplc_budget_t              result = { 0, 0 };
  const cardano_uplc_builtin_cost_t* e      = NULL;
  int64_t                            s[6]   = { 0, 0, 0, 0, 0, 0 };
  size_t                             i      = 0U;

  if ((costs == NULL) || ((size_t)func >= (size_t)CARDANO_UPLC_BUILTIN_COUNT))
  {
    return result;
  }

  e = &costs->entries[(size_t)func];

  for (i = 0U; (i < arg_count) && (i < 6U); ++i)
  {
    const cardano_uplc_value_t* arg = (args != NULL) ? args[i] : NULL;
    s[i]                            = arg_ex_mem(arg, costs_strings_by_utf8_bytes);
  }

  if (func == CARDANO_UPLC_BUILTIN_INTEGER_TO_BYTE_STRING)
  {
    if (arg_count > 1U)
    {
      s[1] = cost_as_size(value_integer((args != NULL) ? args[1] : NULL, 0));
    }
  }
  else if (func == CARDANO_UPLC_BUILTIN_REPLICATE_BYTE)
  {
    if (arg_count > 0U)
    {
      s[0] = cost_as_size(value_integer((args != NULL) ? args[0] : NULL, 0));
    }
  }
  else if (func == CARDANO_UPLC_BUILTIN_WRITE_BITS)
  {
    if (arg_count > 1U)
    {
      s[1] = value_list_len((args != NULL) ? args[1] : NULL);
    }
  }
  else if ((func == CARDANO_UPLC_BUILTIN_SHIFT_BYTE_STRING) || (func == CARDANO_UPLC_BUILTIN_ROTATE_BYTE_STRING))
  {
    if (arg_count > 1U)
    {
      s[1] = abs_i64(value_integer((args != NULL) ? args[1] : NULL, 0));
    }
  }
  else if (func == CARDANO_UPLC_BUILTIN_DROP_LIST)
  {
    if (arg_count > 0U)
    {
      s[0] = value_abs_integer_saturating((args != NULL) ? args[0] : NULL);
    }

    if (arg_count > 1U)
    {
      s[1] = value_list_len((args != NULL) ? args[1] : NULL);
    }
  }
  else if (func == CARDANO_UPLC_BUILTIN_LIST_TO_ARRAY)
  {
    if (arg_count > 0U)
    {
      s[0] = value_list_len((args != NULL) ? args[0] : NULL);
    }
  }
  else if ((func == CARDANO_UPLC_BUILTIN_BLS12_381_G1_MULTI_SCALAR_MUL) || (func == CARDANO_UPLC_BUILTIN_BLS12_381_G2_MULTI_SCALAR_MUL))
  {
    if (arg_count > 0U)
    {
      s[0] = value_list_len((args != NULL) ? args[0] : NULL);
    }

    if (arg_count > 1U)
    {
      s[1] = value_list_len((args != NULL) ? args[1] : NULL);
    }
  }
  else if (func == CARDANO_UPLC_BUILTIN_INSERT_COIN)
  {
    if (arg_count > 3U)
    {
      s[0] = value_arg_max_depth((args != NULL) ? args[3] : NULL);
    }
  }
  else if (func == CARDANO_UPLC_BUILTIN_LOOKUP_COIN)
  {
    if (arg_count > 2U)
    {
      s[2] = value_arg_max_depth((args != NULL) ? args[2] : NULL);
    }
  }
  else if ((func == CARDANO_UPLC_BUILTIN_UNION_VALUE) || (func == CARDANO_UPLC_BUILTIN_VALUE_CONTAINS))
  {
    if (arg_count > 0U)
    {
      s[0] = value_arg_token_count((args != NULL) ? args[0] : NULL);
    }

    if (arg_count > 1U)
    {
      s[1] = value_arg_token_count((args != NULL) ? args[1] : NULL);
    }
  }
  else if (func == CARDANO_UPLC_BUILTIN_VALUE_DATA)
  {
    if (arg_count > 0U)
    {
      s[0] = value_arg_token_count((args != NULL) ? args[0] : NULL);
    }
  }
  else if (func == CARDANO_UPLC_BUILTIN_UN_VALUE_DATA)
  {
    if (arg_count > 0U)
    {
      s[0] = value_arg_data_node_count((args != NULL) ? args[0] : NULL);
    }
  }
  else if (func == CARDANO_UPLC_BUILTIN_SCALE_VALUE)
  {
    if (arg_count > 1U)
    {
      s[1] = value_arg_token_count((args != NULL) ? args[1] : NULL);
    }
  }
  else
  {
    /* No literal-derived size; all sizes are ex-mem. */
  }

  result = eval_entry(e, s);

  return result;
}
