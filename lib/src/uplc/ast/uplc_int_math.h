/**
 * \file uplc_int_math.h
 *
 * \author angel.castillo
 * \date   Oct 31, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_INT_MATH_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_INT_MATH_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Adds two signed 64-bit integers and detects overflow.
 *
 * \param[in] a The first operand.
 * \param[in] b The second operand.
 * \param[out] result Receives the result when no overflow occurs. Must not be NULL.
 *
 * \return true if the mathematical result is outside the int64 range; false
 *         otherwise. The value of \p result is unspecified on overflow.
 */
bool
cardano_uplc_i64_add_overflow(int64_t a, int64_t b, int64_t* result);

/**
 * \brief Subtracts two signed 64-bit integers and detects overflow.
 *
 * \param[in] a The first operand.
 * \param[in] b The second operand.
 * \param[out] result Receives the result when no overflow occurs. Must not be NULL.
 *
 * \return true if the mathematical result is outside the int64 range; false
 *         otherwise. The value of \p result is unspecified on overflow.
 */
bool
cardano_uplc_i64_sub_overflow(int64_t a, int64_t b, int64_t* result);

/**
 * \brief Multiplies two signed 64-bit integers and detects overflow.
 *
 * \param[in] a The first operand.
 * \param[in] b The second operand.
 * \param[out] result Receives the result when no overflow occurs. Must not be NULL.
 *
 * \return true if the mathematical result is outside the int64 range; false
 *         otherwise. The value of \p result is unspecified on overflow.
 */
bool
cardano_uplc_i64_mul_overflow(int64_t a, int64_t b, int64_t* result);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_INT_MATH_H */
