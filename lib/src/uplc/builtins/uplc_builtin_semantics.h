/**
 * \file uplc_builtin_semantics.h
 *
 * \author angel.castillo
 * \date   Jun 18, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_BUILTINS_UPLC_BUILTIN_SEMANTICS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_BUILTINS_UPLC_BUILTIN_SEMANTICS_H

/* INCLUDES ******************************************************************/

#include "../ast/uplc_lang_version.h"
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The ledger builtin semantics variants, selected by language and protocol.
 *
 * Some builtins behave or cost differently across protocol upgrades; the variant
 * captures that selection so the cost model and the builtin runtime read one
 * value rather than re-deriving the protocol rules. The observable differences
 * are that variants D, E, F and G cost text strings by their utf-8 byte length
 * rather than code-point count (see \ref cardano_uplc_semantics_costs_strings_by_utf8_bytes),
 * that variants C, E and G apply a range check to \c consByteString (see
 * \ref cardano_uplc_semantics_cons_byte_string_range_checks), that variants D and
 * E bound integer arguments (see \ref cardano_uplc_semantics_bounds_integer_arguments)
 * and that variants D, E, F and G bound byte-string arguments (see
 * \ref cardano_uplc_semantics_bounds_byte_string_arguments).
 */
typedef enum
{
  /** \brief Pre-Chang V1/V2 semantics. */
  CARDANO_UPLC_SEMANTICS_A = 0,
  /** \brief Chang-era V1/V2 semantics. */
  CARDANO_UPLC_SEMANTICS_B = 1,
  /** \brief Pre-Van-Rossem V3 semantics (consByteString range checks). */
  CARDANO_UPLC_SEMANTICS_C = 2,
  /** \brief Van-Rossem-era V1/V2 semantics (utf-8-byte string costing). */
  CARDANO_UPLC_SEMANTICS_D = 3,
  /** \brief V3 from Van-Rossem and V4 before Dijkstra (utf-8 costing, range checks). */
  CARDANO_UPLC_SEMANTICS_E = 4,
  /** \brief Dijkstra-era V1/V2 semantics (utf-8-byte string costing). */
  CARDANO_UPLC_SEMANTICS_F = 5,
  /** \brief Dijkstra-era V3/V4 semantics (utf-8 costing and range checks). */
  CARDANO_UPLC_SEMANTICS_G = 6
} cardano_uplc_builtin_semantics_t;

/**
 * \brief Selects the builtin semantics variant from language and protocol.
 *
 * For V1 and V2 a protocol major version at or above the Dijkstra version
 * selects F, at or above the Van-Rossem version selects D, at or above the Chang
 * version selects B, and anything earlier selects A. For V3 a protocol at or
 * above Dijkstra selects G, at or above Van-Rossem selects E, otherwise C. For V4
 * a protocol at or above Dijkstra selects G and anything earlier selects E. An
 * unrecognized version selects C.
 *
 * \param[in] lang_version The language version selecting the rule.
 * \param[in] protocol_major The protocol major version.
 *
 * \return The selected semantics variant.
 */
cardano_uplc_builtin_semantics_t
cardano_uplc_builtin_semantics_for_language_and_protocol(
  cardano_uplc_lang_version_t lang_version,
  uint64_t                    protocol_major);

/**
 * \brief Whether a semantics variant costs text strings by their utf-8 byte length.
 *
 * True for D, E, F and G, false for A, B and C. This is the boolean the ex-mem
 * string measure consumes (\ref cardano_uplc_string_ex_mem).
 *
 * \param[in] semantics The semantics variant to query.
 *
 * \return \c true for D, E, F and G, \c false otherwise (including an unknown variant).
 */
bool
cardano_uplc_semantics_costs_strings_by_utf8_bytes(cardano_uplc_builtin_semantics_t semantics);

/**
 * \brief Whether a semantics variant range-checks the consByteString builtin.
 *
 * True for C, E and G, false for A, B, D and F. The \c consByteString builtin
 * consults this to decide whether to reject a byte argument outside 0..255.
 *
 * \param[in] semantics The semantics variant to query.
 *
 * \return \c true for C, E and G, \c false otherwise (including an unknown variant).
 */
bool
cardano_uplc_semantics_cons_byte_string_range_checks(cardano_uplc_builtin_semantics_t semantics);

/**
 * \brief Whether a semantics variant bounds the integer arguments of builtins.
 *
 * True for D and E, false for A, B, C, F and G. Under these variants the integer
 * arithmetic and ordering builtins, and the leading integer of \c consByteString,
 * reject an argument outside -(2^262143) .. 2^262143 - 1.
 *
 * \param[in] semantics The semantics variant to query.
 *
 * \return \c true for D and E, \c false otherwise (including an unknown variant).
 */
bool
cardano_uplc_semantics_bounds_integer_arguments(cardano_uplc_builtin_semantics_t semantics);

/**
 * \brief Whether a semantics variant bounds the byte-string arguments of builtins.
 *
 * True for D, E, F and G, false for A, B and C. Under these variants the builtins
 * that consume a byte string's contents reject one longer than 65536 bytes, and
 * the same variants select the bounded forms of \c writeBits (input of at most
 * 4096 bytes), \c shiftByteString and \c rotateByteString (a shift that fits a
 * signed 64-bit integer), \c constrData (a tag that fits an unsigned 64-bit
 * integer) and the BLS12-381 \c scalarMul builtins (a scalar in
 * -(2^4095) .. 2^4095 - 1).
 *
 * \param[in] semantics The semantics variant to query.
 *
 * \return \c true for D, E, F and G, \c false otherwise (including an unknown variant).
 */
bool
cardano_uplc_semantics_bounds_byte_string_arguments(cardano_uplc_builtin_semantics_t semantics);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_BUILTINS_UPLC_BUILTIN_SEMANTICS_H */
