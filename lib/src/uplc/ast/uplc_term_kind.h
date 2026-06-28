/**
 * \file uplc_term_kind.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_TERM_KIND_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_TERM_KIND_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The kind of an Untyped Plutus Core term.
 *
 * The integer value of each enumerator is the 4-bit flat-encoding tag the
 * decoder reads for that term form, so the enum doubles as the authoritative
 * term-tag list the decoder and the CEK machine switch on. The tags are fixed by
 * the protocol. \c Constr and \c Case exist from Plutus V3 on.
 */
typedef enum
{
  /** \brief A de Bruijn-indexed variable. */
  CARDANO_UPLC_TERM_VAR = 0,
  /** \brief A delayed computation. */
  CARDANO_UPLC_TERM_DELAY = 1,
  /** \brief A lambda abstraction with an implicit binder. */
  CARDANO_UPLC_TERM_LAMBDA = 2,
  /** \brief A function application. */
  CARDANO_UPLC_TERM_APPLY = 3,
  /** \brief A constant of the constant universe. */
  CARDANO_UPLC_TERM_CONSTANT = 4,
  /** \brief Forcing of a delayed computation. */
  CARDANO_UPLC_TERM_FORCE = 5,
  /** \brief The error term. */
  CARDANO_UPLC_TERM_ERROR = 6,
  /** \brief A reference to a default (builtin) function. */
  CARDANO_UPLC_TERM_BUILTIN = 7,
  /** \brief A constructor with a tag and a field list (V3+). */
  CARDANO_UPLC_TERM_CONSTR = 8,
  /** \brief A case expression over a scrutinee and branch list (V3+). */
  CARDANO_UPLC_TERM_CASE = 9
} cardano_uplc_term_kind_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_TERM_KIND_H */
