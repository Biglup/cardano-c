/**
 * \file pretty.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_SYNTAX_PRETTY_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_SYNTAX_PRETTY_H

/* INCLUDES ******************************************************************/

#include <cardano/buffer.h>
#include <cardano/error.h>
#include "../ast/uplc_program.h"
#include "../ast/uplc_term.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Renders an Untyped Plutus Core term to the textual surface syntax.
 *
 * The output is the human-readable surface syntax the Plutus conformance
 * \c .uplc.expected files use, produced for diagnostics and round-trip checks
 * rather than for the consensus-critical comparison, which is done structurally
 * on the de Bruijn AST. Application is rendered as \c [ f x ], constants as
 * \c (con <type> <value>), and the other term forms with their \c (lam ...),
 * \c (delay ...), \c (force ...), \c (error), \c (builtin name), \c (constr ...)
 * and \c (case ...) heads. The result is one compact line with single-space
 * separators and is NUL-terminated.
 *
 * Because the AST carries de Bruijn indices and no binder names, a lambda binder
 * is printed as \c v-<level>, where \c level counts binders from the outermost
 * (zero-based), and a variable prints the same name as the binder it resolves to.
 * A variable whose index escapes the binders in scope (a free variable) is
 * printed as \c free-<index> so rendering never fails on a malformed term.
 *
 * \param[in] term The term to render. Must not be NULL.
 * \param[out] out On success, a newly allocated buffer holding the rendered,
 *             NUL-terminated text; the caller owns it and must release it with
 *             \ref cardano_buffer_unref. Left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p term or \p out is NULL, \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED
 *         if the output buffer cannot be grown, \ref CARDANO_ERROR_ILLEGAL_STATE
 *         if the term nests deeper than the rendering depth ceiling, or
 *         \ref CARDANO_ERROR_NOT_IMPLEMENTED for a BLS constant value, which has
 *         no textual representation here.
 */
cardano_error_t
cardano_uplc_pretty_print_term(
  const cardano_uplc_term_t* term,
  cardano_buffer_t**         out);

/**
 * \brief Renders an Untyped Plutus Core program to the textual surface syntax.
 *
 * Wraps the term rendering of \ref cardano_uplc_pretty_print_term in
 * \c (program major.minor.patch <term>), the form the conformance
 * \c .uplc.expected files use.
 *
 * \param[in] program The program to render. Must not be NULL and must carry a
 *            non-NULL term.
 * \param[out] out On success, a newly allocated buffer holding the rendered,
 *             NUL-terminated text; the caller owns it and must release it with
 *             \ref cardano_buffer_unref. Left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p program, \p out, or the program term is NULL,
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the output buffer cannot
 *         be grown, \ref CARDANO_ERROR_ILLEGAL_STATE if the term nests deeper than
 *         the rendering depth ceiling, or \ref CARDANO_ERROR_NOT_IMPLEMENTED for a
 *         BLS constant value.
 */
cardano_error_t
cardano_uplc_pretty_print_program(
  const cardano_uplc_program_t* program,
  cardano_buffer_t**            out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_SYNTAX_PRETTY_H */
