/**
 * \file uplc_int.h
 *
 * \author angel.castillo
 * \date   Jun 19, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_INT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_INT_H

/* INCLUDES ******************************************************************/

#include "uplc_term.h"
#include <cardano/common/bigint.h>
#include <cardano/error.h>

#include "../arena/uplc_arena.h"

#include <stdbool.h>
#include <stdint.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* FORWARD DECLARATIONS ******************************************************/

/**
 * \brief A VM-internal, arena-allocated Plutus-data node.
 */
struct cardano_uplc_data_t;

/**
 * \brief Allocates a zero-initialized constant node from the arena.
 *
 * Shared interior helper behind the constant constructors. The returned node is
 * raw arena memory, not zeroed beyond what the arena guarantees, and is released
 * with the arena. The \c cardano_uplc_int_ prefix marks it module-internal.
 *
 * \param[in] arena The arena to allocate from.
 *
 * \return The new constant node, or NULL if the arena cannot serve it.
 */
cardano_uplc_constant_t*
cardano_uplc_int_alloc_constant(cardano_uplc_arena_t* arena);

/**
 * \brief Releases the arena's reference to a bigint registered for unref.
 *
 * The callback the constant constructors pass to
 * \ref cardano_uplc_arena_register_unref so the arena drops its bigint reference
 * on free. The \c cardano_uplc_int_ prefix marks it module-internal.
 *
 * \param[in] object The \ref cardano_bigint_t the arena owns a reference to.
 */
void
cardano_uplc_int_unref_bigint(void* object);

/**
 * \brief Builds a data constant that wraps an arena-allocated data node directly.
 *
 * The node and the constant share \p arena and are released together; nothing is
 * refcounted. This is the lean path the decoder, the text parser and the data
 * builtins use to wrap an arena \c cardano_uplc_data_t without a round-trip through
 * the library \ref cardano_plutus_data_t.
 *
 * \param[in] arena The arena to allocate the constant from. Must not be NULL.
 * \param[in] data The arena data node. Must not be NULL and must live in \p arena.
 * \param[out] constant On success, set to the new constant; untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if any
 *         argument is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the
 *         arena cannot serve the node.
 */
cardano_error_t
cardano_uplc_int_constant_new_data_node(
  cardano_uplc_arena_t*             arena,
  const struct cardano_uplc_data_t* data,
  cardano_uplc_constant_t**         constant);

/**
 * \brief Builds a byte-string constant by copying a raw byte span into the arena.
 *
 * The lean hot path behind every byte-string-producing builtin: it bump-allocates
 * \p size bytes from \p arena, copies \p data into them, and stores the inline
 * \c (data, size) descriptor directly in the constant. Nothing is refcounted and an
 * empty span yields a NULL pointer with size 0.
 *
 * \param[in] arena The arena the constant and the byte copy are allocated from.
 * \param[in] data The bytes to copy, or NULL when \p size is 0.
 * \param[in] size The number of bytes.
 * \param[out] constant On success, the new constant; untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p constant is NULL, \ref CARDANO_ERROR_INVALID_ARGUMENT if
 *         \p data is NULL while \p size is non-zero, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve the
 *         node or the byte copy.
 */
cardano_error_t
cardano_uplc_int_constant_new_byte_string_copy(
  cardano_uplc_arena_t*     arena,
  const byte_t*             data,
  size_t                    size,
  cardano_uplc_constant_t** constant);

/**
 * \brief Builds a UTF-8 string constant by copying a raw byte span into the arena.
 *
 * The string counterpart of \ref cardano_uplc_int_constant_new_byte_string_copy. The
 * caller is responsible for having validated the UTF-8; this does not re-validate.
 *
 * \param[in] arena The arena the constant and the byte copy are allocated from.
 * \param[in] data The UTF-8 bytes to copy, or NULL when \p size is 0.
 * \param[in] size The number of bytes.
 * \param[out] constant On success, the new constant; untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p constant is NULL, \ref CARDANO_ERROR_INVALID_ARGUMENT if
 *         \p data is NULL while \p size is non-zero, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve the
 *         node or the byte copy.
 */
cardano_error_t
cardano_uplc_int_constant_new_string_copy(
  cardano_uplc_arena_t*     arena,
  const byte_t*             data,
  size_t                    size,
  cardano_uplc_constant_t** constant);

/**
 * \brief Builds a byte-string constant that adopts an existing arena byte span.
 *
 * The zero-copy hot path: \p data must already live in \p arena (it is typically the
 * scratch a bitwise or encoding builtin just filled), so the constant stores the
 * pointer and length directly with no copy. An empty span stores a NULL pointer.
 *
 * \param[in] arena The arena the constant node is allocated from and that \p data
 *            already belongs to.
 * \param[in] data The arena bytes to adopt, or NULL when \p size is 0.
 * \param[in] size The number of bytes.
 * \param[out] constant On success, the new constant; untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p constant is NULL, \ref CARDANO_ERROR_INVALID_ARGUMENT if
 *         \p data is NULL while \p size is non-zero, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve the
 *         node.
 */
cardano_error_t
cardano_uplc_int_constant_new_byte_string_take(
  cardano_uplc_arena_t*     arena,
  const byte_t*             data,
  size_t                    size,
  cardano_uplc_constant_t** constant);

/**
 * \brief Tests whether a bigint fits exactly in a signed 64-bit integer.
 *
 * Handles the \c INT64_MIN edge (magnitude 2^63, bit length 64) explicitly: it is
 * the one value with a 64-bit magnitude that still fits, so the cheap bit-length
 * test alone would wrongly reject it.
 *
 * \param[in] value The bigint to test. Must not be NULL.
 * \param[out] out On a fit, set to the \c int64_t value; untouched otherwise.
 *
 * \return \c true when \p value fits an \c int64_t, \c false otherwise.
 */
bool
cardano_uplc_int_bigint_fits_int64(const cardano_bigint_t* value, int64_t* out);

/**
 * \brief Tests whether an integer constant is stored inline.
 *
 * \param[in] constant An integer constant. Must not be NULL.
 *
 * \return \c true when the value is stored inline as an \c int64_t, \c false when it
 *         is stored as a \ref cardano_bigint_t.
 */
bool
cardano_uplc_constant_int_is_small(const cardano_uplc_constant_t* constant);

/**
 * \brief Returns the inline value of a small integer constant.
 *
 * Only valid when \ref cardano_uplc_constant_int_is_small returns \c true.
 *
 * \param[in] constant A small integer constant. Must not be NULL.
 *
 * \return The inline \c int64_t value.
 */
int64_t
cardano_uplc_constant_int_small(const cardano_uplc_constant_t* constant);

/**
 * \brief Returns the \ref cardano_bigint_t for any integer constant, building one
 *        lazily for an inline value and caching it in the node.
 *
 * For a big constant this returns the stored bigint directly. For an inline
 * constant it builds a bigint equal to the inline value, registers it with \p arena
 * so the arena releases it, caches it in the node, and returns it; later calls
 * return the cached pointer with no further allocation. The returned bigint is owned
 * by the arena; the caller must not unref it.
 *
 * \param[in] arena The arena the lazily built bigint is registered with.
 * \param[in] constant An integer constant. Must not be NULL.
 * \param[out] out On success, the bigint view; untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if a bigint cannot be built or
 *         registered.
 */
cardano_error_t
cardano_uplc_constant_int_materialize(
  cardano_uplc_arena_t*          arena,
  const cardano_uplc_constant_t* constant,
  const cardano_bigint_t**       out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_INT_H */
