/**
 * \file uplc_program.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_PROGRAM_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_PROGRAM_H

/* INCLUDES ******************************************************************/

#include <cardano/buffer.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/typedefs.h>
#include "../arena/uplc_arena.h"
#include "uplc_term.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A decoded program: a version triple and its top-level term.
 */
typedef struct cardano_uplc_program_t
{
  uint32_t                   version_major;
  uint32_t                   version_minor;
  uint32_t                   version_patch;
  const cardano_uplc_term_t* term;
} cardano_uplc_program_t;

/**
 * \brief Decodes on-chain Plutus script bytes into an arena-allocated program.
 *
 * This entry expects the ledger witness-set form: the flat-encoded program
 * wrapped in exactly one CBOR bytestring (CBOR major type 2). It peels that one
 * bytestring and runs the flat program decoder on the resulting flat bytes,
 * peeling exactly one CBOR layer as the consensus encoding prescribes.
 *
 * Only the single-CBOR-wrapped form is accepted. Raw (unwrapped) flat bytes are
 * not accepted: the leading byte is read as a CBOR major-type-2 header, so a raw
 * program is rejected with \ref CARDANO_ERROR_DECODING. Double-CBOR-wrapped bytes
 * are likewise not accepted: after one peel the buffer is still a CBOR bytestring
 * rather than valid flat, so the flat decoder rejects it. A caller that holds
 * such a script knows its wrapping and removes the extra layer before calling.
 *
 * The returned program and its whole term tree are owned by \p arena; a single
 * \ref cardano_uplc_arena_free releases everything regardless of the exit path.
 * Every intermediate buffer the peeling produces is released before this returns.
 *
 * \param[in] arena The arena every node and interior object is allocated from.
 * \param[in] script_bytes The raw on-chain script bytes. May be NULL only when
 *            \p size is 0.
 * \param[in] size The number of bytes in \p script_bytes.
 * \param[out] program On success, the decoded program; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p program is NULL, or if \p script_bytes is NULL while
 *         \p size is non-zero, \ref CARDANO_ERROR_DECODING for malformed CBOR, an
 *         outer item that is not a bytestring, trailing bytes after the outer
 *         bytestring, or a flat program that fails to decode,
 *         \ref CARDANO_ERROR_NOT_IMPLEMENTED for a BLS constant value, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena or an interior
 *         object cannot be allocated.
 */
cardano_error_t
cardano_uplc_program_from_script_bytes(
  struct cardano_uplc_arena_t*   arena,
  const byte_t*                  script_bytes,
  size_t                         size,
  const cardano_uplc_program_t** program);

/**
 * \brief Flat-encodes a program into a newly allocated byte buffer.
 *
 * The exact inverse of \ref cardano_uplc_flat_decode_program: the three version
 * words are written, then the term, then a byte-aligning filler. The output is the
 * canonical flat byte stream, so a program obtained by decoding flat bytes encodes
 * back to those same bytes, and the result is a fixed point under a second
 * decode-encode pass.
 *
 * The \c array, \c value and \c bls12_381_mlresult constant kinds have no flat
 * serialization (the decoder rejects them), so a program carrying one is rejected
 * here with \ref CARDANO_ERROR_INVALID_ARGUMENT rather than producing bytes that
 * could not be decoded back.
 *
 * \param[in] program The program to encode. Must not be NULL and must carry a
 *            non-NULL term.
 * \param[out] out_flat On success, the flat bytes; the caller releases them with
 *             \ref cardano_buffer_unref. Left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p program, its term, or \p out_flat is NULL,
 *         \ref CARDANO_ERROR_INVALID_ARGUMENT for an unknown term kind or a
 *         constant kind with no flat serialization, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if a buffer cannot be grown.
 */
cardano_error_t
cardano_uplc_flat_encode_program(
  const cardano_uplc_program_t* program,
  cardano_buffer_t**            out_flat);

/**
 * \brief Flat-encodes a program and wraps it in a single CBOR bytestring.
 *
 * Runs \ref cardano_uplc_flat_encode_program and wraps the resulting flat bytes in
 * exactly one CBOR bytestring (CBOR major type 2) through
 * \ref cardano_cbor_writer_write_bytestring, producing the ledger witness-set form
 * that \ref cardano_uplc_program_from_script_bytes consumes.
 *
 * \param[in] program The program to encode. Must not be NULL and must carry a
 *            non-NULL term.
 * \param[out] out_cbor On success, the CBOR-wrapped flat bytes; the caller releases
 *             them with \ref cardano_buffer_unref. Left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p program, its term, or \p out_cbor is NULL,
 *         \ref CARDANO_ERROR_INVALID_ARGUMENT for a constant kind with no flat
 *         serialization, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if a buffer
 *         or the CBOR writer cannot be allocated.
 */
cardano_error_t
cardano_uplc_program_to_cbor(
  const cardano_uplc_program_t* program,
  cardano_buffer_t**            out_cbor);

/**
 * \brief Applies a single Plutus-data parameter to a program.
 *
 * Produces a new program with the same version as \p program and a term that
 * wraps the original in one application of a data constant:
 * \c Apply(program->term, Constant(Data(param))). This is how a parameterized
 * validator binds a compile-time parameter before evaluation.
 *
 * Ownership: the new program, its application term, the constant term and the
 * data constant are all allocated from \p arena, and the original
 * \p program->term is shared by reference (it is not copied). The data constant
 * registers \p param with \p arena, which takes its own reference; the caller
 * keeps the reference it passed in and must still release it. A single
 * \ref cardano_uplc_arena_free releases the new program and its new nodes. Both
 * \p program and the result share \p arena, so they stay valid for the same
 * lifetime.
 *
 * \param[in] arena The arena every new node is allocated from. Must not be NULL.
 * \param[in] program The program to wrap. Must not be NULL, must carry a non-NULL
 *            term, and must live in \p arena.
 * \param[in] param The data parameter to apply. Must not be NULL.
 * \param[out] out On success, set to the new program; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena, \p program, \p program->term, \p param, or \p out is NULL,
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve a
 *         node, or \ref CARDANO_ERROR_ILLEGAL_STATE if the arena is past its byte
 *         ceiling.
 */
cardano_error_t
cardano_uplc_program_apply_data(
  struct cardano_uplc_arena_t*  arena,
  const cardano_uplc_program_t* program,
  cardano_plutus_data_t*        param,
  cardano_uplc_program_t**      out);

/**
 * \brief Applies a list of Plutus-data parameters to a program, left to right.
 *
 * Folds \ref cardano_uplc_program_apply_data over \p params in order, so the
 * resulting term is \c [[...[term p0] p1] ... pn]: \c params[0] is applied first
 * and becomes the innermost argument, so a left-to-right list of datum, redeemer
 * and script context is applied in that order. With \p count 0 the result is a
 * new program equal to \p program (same version, same term, no wrapping).
 *
 * Ownership: identical to \ref cardano_uplc_program_apply_data applied \p count
 * times. Every intermediate and the final program live in \p arena; each
 * parameter is registered with \p arena, which takes its own reference, and the
 * caller keeps the references it passed in.
 *
 * \param[in] arena The arena every new node is allocated from. Must not be NULL.
 * \param[in] program The program to wrap. Must not be NULL, must carry a non-NULL
 *            term, and must live in \p arena.
 * \param[in] params The data parameters, or NULL when \p count is 0. No element
 *            may be NULL.
 * \param[in] count The number of parameters.
 * \param[out] out On success, set to the new program; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena, \p program, \p program->term, \p out, or any element of
 *         \p params is NULL, \ref CARDANO_ERROR_INVALID_ARGUMENT if \p params is
 *         NULL while \p count is non-zero,
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve a
 *         node, or \ref CARDANO_ERROR_ILLEGAL_STATE if the arena is past its byte
 *         ceiling.
 */
cardano_error_t
cardano_uplc_program_apply_data_params(
  struct cardano_uplc_arena_t*  arena,
  const cardano_uplc_program_t* program,
  cardano_plutus_data_t* const* params,
  size_t                        count,
  cardano_uplc_program_t**      out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_PROGRAM_H */
