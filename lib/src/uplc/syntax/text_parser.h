/**
 * \file text_parser.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_SYNTAX_TEXT_PARSER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_SYNTAX_TEXT_PARSER_H

/* INCLUDES ******************************************************************/

#include "../ast/uplc_program.h"
#include "../ast/uplc_term.h"
#include <cardano/error.h>
#include <cardano/typedefs.h>

#include "../arena/uplc_arena.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Parses the textual UPLC surface syntax into an arena-allocated program.
 *
 * Reads the surface form the Plutus conformance corpus uses, for example
 * \c "(program 1.0.0 (lam x x))", and produces a \ref cardano_uplc_program_t whose
 * whole term tree is owned by \p arena. Named variables are resolved to de Bruijn
 * indices during parsing using a scope stack, so the produced AST matches, term for
 * term, the AST the flat decoder yields for the same program (alpha-equivalence).
 * Only \c lam introduces a term-level binder; \c delay and \c force do not bind.
 *
 * Application is written \c [ f x y ... ] and is left-associative n-ary, so
 * \c [ f x y ] builds \c ((f x) y). Constants follow the \c (con \<type\> \<value\>)
 * form for integer, bytestring (\c \#hex), string, bool (\c True / \c False),
 * unit (\c ()), \c (list T), \c (pair A B) and \c data (with the textual
 * \c Constr / \c Map / \c List / \c I / \c B data syntax). The \c constr and
 * \c case forms are parsed only when the program version is 1.1.0 or later; under
 * an earlier version they are a parse error.
 *
 * A free (unbound) variable is not a parse error: it is given a de Bruijn index
 * that is out of range for the evaluation environment, so the parser yields an
 * open term that the machine fails on, matching the reference.
 *
 * BLS constant value literals are not parsed: a \c (con bls12_381_G1_element ...)
 * or G2 constant is rejected with \ref CARDANO_ERROR_NOT_IMPLEMENTED rather than
 * crashing. No corpus value-literal needs them at parse time.
 *
 * The whole tree, the version triple and every interior object live in \p arena;
 * a single \ref cardano_uplc_arena_free releases everything on every exit path,
 * including failure. Recursion over nested terms and constants is bounded; input
 * that nests past the bound is rejected with the parse-error code. The parser
 * never reads past \p text + \p len.
 *
 * \param[in] arena The arena every node and interior object is allocated from.
 *            Must not be NULL.
 * \param[in] text The UTF-8 source text. May be NULL only when \p len is 0.
 * \param[in] len The number of bytes in \p text.
 * \param[out] program On success, the parsed program owned by \p arena; left
 *             untouched on failure.
 * \param[out] error_offset On any return, set to the byte offset into \p text at
 *             which parsing failed, or to \p len on success. May be NULL when the
 *             caller does not want the offset.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p program is NULL, or if \p text is NULL while \p len is
 *         non-zero, \ref CARDANO_ERROR_DECODING for any malformed input (bad token,
 *         unbalanced parens or brackets, unknown builtin name, a \c constr or
 *         \c case form under a version below 1.1.0, bad constant, bad hex, or
 *         nesting past the bound),
 *         \ref CARDANO_ERROR_NOT_IMPLEMENTED for a BLS constant value literal, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena or an interior
 *         object cannot be allocated.
 */
cardano_error_t
cardano_uplc_parse_program(
  cardano_uplc_arena_t*          arena,
  const char*                    text,
  size_t                         len,
  const cardano_uplc_program_t** program,
  size_t*                        error_offset);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_SYNTAX_TEXT_PARSER_H */
