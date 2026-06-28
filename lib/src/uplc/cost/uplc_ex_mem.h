/**
 * \file uplc_ex_mem.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_EX_MEM_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_EX_MEM_H

/* INCLUDES ******************************************************************/

#include <cardano/common/bigint.h>
#include <cardano/typedefs.h>
#include "../ast/uplc_constant.h"

/* FORWARD DECLARATIONS ******************************************************/

#ifndef CARDANO_UPLC_VALUE_T_FORWARD_DECLARED
#define CARDANO_UPLC_VALUE_T_FORWARD_DECLARED
typedef struct cardano_uplc_value_t cardano_uplc_value_t;
#endif /* CARDANO_UPLC_VALUE_T_FORWARD_DECLARED */

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The ex-mem size of an integer, in 64-bit words.
 *
 * Zero costs one word; a non-zero integer costs floor(log2(|n|)) / 64 + 1 words,
 * which is its 64-bit word count. The bit length is taken from
 * \ref cardano_bigint_bit_length, which returns the base-2 digit count of the
 * absolute value (sign-independent), so floor(log2(|n|)) is that count minus one.
 *
 * \param[in] value The integer to measure. A NULL pointer is measured as zero,
 *            yielding 1.
 *
 * \return The integer's ex-mem size.
 */
int64_t
cardano_uplc_integer_ex_mem(const cardano_bigint_t* value);

/**
 * \brief The ex-mem size of a byte string, in 8-byte chunks.
 *
 * An empty byte string costs 1; otherwise the cost is (len - 1) / 8 + 1, the
 * number of 8-byte chunks needed to hold it.
 *
 * \param[in] length The byte-string length in bytes.
 *
 * \return The byte string's ex-mem size.
 */
int64_t
cardano_uplc_byte_string_ex_mem(size_t length);

/**
 * \brief The ex-mem size of a UTF-8 text string.
 *
 * Under the character-count semantics (variants A, B, C) the cost is the number
 * of Unicode code points; under the utf-8-byte semantics (variants D, E) it is
 * 0 for the empty string and (bytes - 1) / 4 + 1 otherwise. The single boolean
 * selects between the two.
 *
 * \param[in] bytes The UTF-8 encoded bytes, or NULL when \p size is 0.
 * \param[in] size The number of encoded bytes.
 * \param[in] costs_by_utf8_bytes \c true to cost by utf-8 byte length (D/E),
 *            \c false to cost by code-point count (A/B/C).
 *
 * \return The string's ex-mem size.
 */
int64_t
cardano_uplc_string_ex_mem(const byte_t* bytes, size_t size, bool costs_by_utf8_bytes);

/**
 * \brief The ex-mem size of a Plutus data value.
 *
 * Walks the data tree charging 4 per node and adding the integer or byte-string
 * size of each leaf. The walk is depth-bounded so adversarial nesting cannot
 * overflow the C stack; a tree deeper than the bound stops descending past it,
 * which legitimate Plutus data never reaches.
 *
 * \param[in] data The data value to measure. A NULL pointer is measured as 0.
 *
 * \return The data value's ex-mem size.
 */
int64_t
cardano_uplc_data_ex_mem(const struct cardano_uplc_data_t* data);

/**
 * \brief The ex-mem size of a constant value.
 *
 * Sums the per-arm sizes recursively: integer, byte string, string (selected by
 * \p costs_strings_by_utf8_bytes), 1 for bool and unit, the data size for data,
 * the sum of the items for a list, the sum of the components for a pair, and the
 * fixed BLS sizes for the reserved BLS arms.
 *
 * \param[in] constant The constant to measure. A NULL pointer is measured as 0.
 * \param[in] costs_strings_by_utf8_bytes Selects the string measure, as in
 *            \ref cardano_uplc_string_ex_mem.
 *
 * \return The constant's ex-mem size.
 */
int64_t
cardano_uplc_constant_ex_mem(
  const cardano_uplc_constant_t* constant,
  bool                           costs_strings_by_utf8_bytes);

/**
 * \brief The ex-mem size of a CEK runtime value.
 *
 * A constant value measures its constant via \ref cardano_uplc_constant_ex_mem;
 * a delay, lambda, builtin or constr value measures 1.
 *
 * \param[in] value The value to measure. A NULL pointer is measured as 0.
 * \param[in] costs_strings_by_utf8_bytes Selects the string measure, as in
 *            \ref cardano_uplc_string_ex_mem.
 *
 * \return The value's ex-mem size.
 */
int64_t
cardano_uplc_value_ex_mem(
  const cardano_uplc_value_t* value,
  bool                        costs_strings_by_utf8_bytes);

/**
 * \brief Returns the token count of a single value policy entry.
 *
 * A policy entry is a pair whose second component is the token list; the count is
 * the length of that list, or 0 for a malformed entry.
 *
 * \param[in] entry The policy entry constant, or NULL.
 *
 * \return The number of tokens carried by the entry.
 */
size_t
cardano_uplc_value_entry_token_count(const cardano_uplc_constant_t* entry);

/**
 * \brief Returns the total number of (policy, token) pairs in a value constant.
 *
 * The total is the sum of token-list lengths over all policy entries.
 *
 * \param[in] constant The value constant, or NULL.
 *
 * \return The total token count.
 */
int64_t
cardano_uplc_value_token_count(const cardano_uplc_constant_t* constant);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_COST_UPLC_EX_MEM_H */
