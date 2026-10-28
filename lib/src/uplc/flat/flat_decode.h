/**
 * \file flat_decode.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_FLAT_FLAT_DECODE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_FLAT_FLAT_DECODE_H

/* INCLUDES ******************************************************************/

#include "../ast/uplc_lang_version.h"
#include "../ast/uplc_program.h"
#include "../ast/uplc_term.h"
#include <cardano/error.h>
#include <cardano/typedefs.h>

#include "../arena/uplc_arena.h"
#include "flat_reader.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Decodes a single flat-encoded constant into an arena-allocated value.
 *
 * Reads the const-type list (a run of 4-bit type tags, each preceded by a one
 * continuation bit, with the application tag building \c list<T>, \c array<T> and
 * \c pair<a,b>) and then the value for that type. The \c data constant is read as
 * a flat bytestring whose contents are decoded as CBOR through
 * \ref cardano_plutus_data_from_cbor. An array is read like a list. A value is
 * read as a list of (currency symbol, list of (token name, quantity)) entries and
 * must be canonical: strictly ascending keys of at most 32 bytes, no empty token
 * list and non-zero quantities in the signed 128-bit range.
 *
 * Every node the decoder produces is allocated from \p arena, and every interior
 * refcounted object (bigint, buffer, plutus data) is registered with \p arena
 * through the \c term.c constructors, so a single \ref cardano_uplc_arena_free
 * releases everything regardless of the exit path. On failure no constant is
 * handed back and nothing the call created outlives \p arena.
 *
 * The BLS12-381 G1, G2 and Miller-loop-result types have no flat value form, as in
 * the reference decoder, so a value of any of them is rejected with
 * \ref CARDANO_ERROR_DECODING.
 *
 * \param[in] arena The arena every node and interior object is allocated from.
 * \param[in,out] reader The flat reader, advanced past the encoded constant.
 * \param[out] constant On success, the decoded constant; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, \ref CARDANO_ERROR_DECODING for a malformed
 *         type list, an unknown tag, a truncated value, invalid UTF-8 in a string,
 *         malformed CBOR for a \c data value, a non-canonical value constant, a
 *         BLS12-381 value, or type/value nesting past the depth bound,
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena or an
 *         interior object cannot be allocated, or \ref CARDANO_ERROR_ILLEGAL_STATE
 *         if the arena is past its byte ceiling.
 */
cardano_error_t
cardano_uplc_flat_decode_constant(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  cardano_uplc_constant_t**   constant);

/**
 * \brief Decodes a single flat-encoded term into an arena-allocated AST.
 *
 * Reads the 4-bit term tag and recurses per the term table:
 * tag 0 is a \c Var carrying a de Bruijn index word, 1 \c Delay, 2 \c Lambda (the
 * binder is implicit in the de Bruijn encoding), 3 \c Apply, 4 a \c Constant read
 * through \ref cardano_uplc_flat_decode_constant, 5 \c Force, 6 \c Error, 7 a
 * \c Builtin naming a default function by a 7-bit tag, 8 a \c Constr (a tag word
 * then a cons-bit list of field terms) and 9 a \c Case (a scrutinee term then a
 * cons-bit list of branch terms). A field or branch list, like the constant list
 * encoding, places a one bit before each element and a zero bit to stop.
 *
 * No program version is known at this level, so \c Constr and \c Case are always
 * accepted; \ref cardano_uplc_flat_decode_program applies the version gate.
 *
 * Recursion through \c Delay, \c Lambda, \c Apply, \c Force, \c Constr and \c Case
 * is bounded by an internal depth ceiling, so a deeply nested adversarial input
 * fails with \ref CARDANO_ERROR_DECODING rather than overflowing the C stack.
 *
 * Every node is allocated from \p arena, and an interior constant is built through
 * the constant decoder, which registers its refcounted objects with \p arena. On
 * failure no term is handed back and nothing the call created outlives \p arena.
 *
 * \param[in] arena The arena every node and interior object is allocated from.
 * \param[in,out] reader The flat reader, advanced past the encoded term.
 * \param[out] term On success, the decoded term; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, \ref CARDANO_ERROR_DECODING for an unknown term or
 *         builtin tag, a malformed constant, a truncated stream or nesting past the
 *         depth bound, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena
 *         cannot serve a node.
 */
cardano_error_t
cardano_uplc_flat_decode_term(
  cardano_uplc_arena_t*       arena,
  cardano_uplc_flat_reader_t* reader,
  const cardano_uplc_term_t** term);

/**
 * \brief Decodes a full flat-encoded program into an arena-allocated value.
 *
 * Reads three version words (major, minor, patch), then one term through
 * \ref cardano_uplc_flat_decode_term, then a final filler run: a sequence of zero
 * bits up to and including a terminating one bit that byte-aligns the stream. A
 * truncated stream, a missing terminating filler bit, bytes left over after the
 * filler or a version word that does not fit in 32 bits is rejected with
 * \ref CARDANO_ERROR_DECODING. A \c Constr or \c Case term in a program whose
 * version is below 1.1.0 is rejected the same way, matching the reference decoder.
 *
 * No ledger script checks are applied; \ref cardano_uplc_flat_decode_script_program
 * decodes a program the way the ledger deserialises a script.
 *
 * The returned program and its whole term tree are owned by \p arena; a single
 * \ref cardano_uplc_arena_free releases everything regardless of the exit path.
 *
 * \param[in] arena The arena every node and interior object is allocated from.
 * \param[in,out] reader The flat reader, advanced past the encoded program.
 * \param[out] program On success, the decoded program; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, \ref CARDANO_ERROR_DECODING for a malformed or
 *         truncated stream, a version word too large for 32 bits, a \c Constr or
 *         \c Case term before version 1.1.0, a missing terminating filler bit or
 *         bytes after it, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the
 *         arena cannot serve a node.
 */
cardano_error_t
cardano_uplc_flat_decode_program(
  cardano_uplc_arena_t*          arena,
  cardano_uplc_flat_reader_t*    reader,
  const cardano_uplc_program_t** program);

/**
 * \brief Decodes a flat-encoded ledger script for a language at a protocol version.
 *
 * Decodes like \ref cardano_uplc_flat_decode_program and also applies the checks
 * the ledger's script decoder makes
 * (plutus-ledger-api/src/PlutusLedgerApi/Common/SerialisedScript.hs,
 * \c scriptCBORDecoder): every builtin must be available in \p language at
 * \p protocol_major (\ref cardano_uplc_builtin_available), every constant type
 * header must be at most 32 flat type tags, and every \c Constr term must have at
 * most 1024 fields. A script that fails any of them cannot be deserialised.
 *
 * Ownership matches \ref cardano_uplc_flat_decode_program, so the program and
 * its term tree are owned by \p arena.
 *
 * \param[in] arena The arena every node and interior object is allocated from.
 * \param[in,out] reader The flat reader, advanced past the encoded program.
 * \param[in] language The Plutus language of the script.
 * \param[in] protocol_major The major protocol version the script is used under.
 * \param[out] program On success, the decoded program; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena, \p reader or \p program is NULL, \ref CARDANO_ERROR_DECODING
 *         for any failure \ref cardano_uplc_flat_decode_program reports or a
 *         failed ledger script check, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve a
 *         node.
 */
cardano_error_t
cardano_uplc_flat_decode_script_program(
  cardano_uplc_arena_t*          arena,
  cardano_uplc_flat_reader_t*    reader,
  cardano_uplc_lang_version_t    language,
  uint64_t                       protocol_major,
  const cardano_uplc_program_t** program);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_FLAT_FLAT_DECODE_H */
