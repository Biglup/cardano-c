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
 * are that variants D and E cost text strings by their utf-8 byte length rather
 * than code-point count (see \ref cardano_uplc_semantics_costs_strings_by_utf8_bytes)
 * and that variants C and E apply a range check to \c consByteString (see
 * \ref cardano_uplc_semantics_cons_byte_string_range_checks).
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
  /** \brief Van-Rossem-era V3 semantics (utf-8 costing and range checks). */
  CARDANO_UPLC_SEMANTICS_E = 4
} cardano_uplc_builtin_semantics_t;

/**
 * \brief Selects the builtin semantics variant from a language version alone.
 *
 * The protocol-unaware selection maps V1 and V2 to B and V3 to C. Protocol-aware
 * callers should use \ref cardano_uplc_builtin_semantics_for_language_and_protocol
 * instead, which can also select the pre-Chang A and the Van-Rossem D/E variants.
 * An unrecognized version is treated as V3 (variant C).
 *
 * \param[in] lang_version The language version selecting the variant.
 *
 * \return The selected semantics variant.
 */
cardano_uplc_builtin_semantics_t
cardano_uplc_builtin_semantics_for_language(cardano_uplc_lang_version_t lang_version);

/**
 * \brief Selects the builtin semantics variant from language and protocol.
 *
 * For V1 and V2 a protocol major version at or above the Van-Rossem version
 * selects D, at or above the Chang version selects B, and anything earlier
 * selects A. For V3 a protocol at or above Van-Rossem selects E, otherwise C.
 * V4 follows the V3 rule (Van-Rossem and later select E, earlier select C). An
 * unrecognized version is treated as V3.
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
 * True for D and E, false for A, B and C. This is the boolean the ex-mem string
 * measure consumes (\ref cardano_uplc_string_ex_mem).
 *
 * \param[in] semantics The semantics variant to query.
 *
 * \return \c true for D and E, \c false otherwise (including an unknown variant).
 */
bool
cardano_uplc_semantics_costs_strings_by_utf8_bytes(cardano_uplc_builtin_semantics_t semantics);

/**
 * \brief Whether a semantics variant range-checks the consByteString builtin.
 *
 * True for C and E, false for A, B and D. The \c consByteString builtin consults
 * this to decide whether to reject a byte argument outside 0..255.
 *
 * \param[in] semantics The semantics variant to query.
 *
 * \return \c true for C and E, \c false otherwise (including an unknown variant).
 */
bool
cardano_uplc_semantics_cons_byte_string_range_checks(cardano_uplc_builtin_semantics_t semantics);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_BUILTINS_UPLC_BUILTIN_SEMANTICS_H */
