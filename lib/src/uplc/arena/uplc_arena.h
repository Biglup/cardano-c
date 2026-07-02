/**
 * \file uplc_arena.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_ARENA_UPLC_ARENA_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_ARENA_UPLC_ARENA_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Region (arena) allocator for the UPLC interpreter interior.
 *
 * Serves aligned chunks from \ref _cardano_malloc by bumping an offset inside a
 * singly linked list of blocks and releases every block in one call. Terms,
 * constants, values, environment cells and continuation frames are allocated
 * here and never freed individually; the single free point is
 * \ref cardano_uplc_arena_free. A request larger than the configured block size
 * gets its own dedicated block.
 *
 * The caller creates an arena, hands it to the public decode/evaluate entry
 * points, and frees it once the results have been consumed; the arena is not
 * refcounted.
 */
typedef struct cardano_uplc_arena_t cardano_uplc_arena_t;

/**
 * \brief Function invoked for each registered object when the arena is freed.
 *
 * The arena owns a small number of interior values that are refcounted library
 * objects (bigints, plutus data) rather than raw arena memory. Such an object is
 * registered with \ref cardano_uplc_arena_register_unref and released through a
 * callback of this type when the arena is freed.
 *
 * \param[in] object The object passed to \ref cardano_uplc_arena_register_unref.
 */
typedef void (*cardano_uplc_arena_unref_t)(void* object);

/**
 * \brief Creates a region allocator that serves blocks from \ref _cardano_malloc
 *        and releases them all at once.
 *
 * \param[in] block_size Size in bytes of each underlying block payload. A value
 *            of 0 selects an internal default. Allocations larger than this get
 *            their own oversized block.
 * \param[out] arena On success, set to the new arena; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the
 *         backing allocator fails.
 */
cardano_error_t
cardano_uplc_arena_new(size_t block_size, cardano_uplc_arena_t** arena);

/**
 * \brief Allocates \p size bytes aligned to \p align from the arena.
 *
 * Rounds the current block offset up to \p align and bumps it by \p size,
 * starting a new block when the current one cannot satisfy the request and
 * giving an oversized request its own block. The returned memory is not zeroed.
 *
 * \param[in] arena The arena to allocate from.
 * \param[in] size Number of bytes to allocate. A size of 0 yields a valid,
 *            non-dereferenceable pointer into the arena.
 * \param[in] align Required alignment in bytes. Must be a power of two; a value
 *            of 0 selects the natural maximum alignment.
 *
 * \return A pointer to the allocated memory, or \c NULL when \p arena is NULL,
 *         \p align is not a power of two, the request cannot be represented, the
 *         total-bytes ceiling would be exceeded, or the backing allocator fails.
 */
void*
cardano_uplc_arena_alloc(cardano_uplc_arena_t* arena, size_t size, size_t align);

/**
 * \brief Registers a refcounted object to be released when the arena is freed.
 *
 * Records \p object and \p unref on an intrusive list owned by the arena. When
 * \ref cardano_uplc_arena_free runs, every registered \p unref is called with its
 * \p object, in unspecified order, exactly once. This is the single bridge between
 * the arena and the library's refcounted interior values; the caller must have a
 * reference for the arena to drop.
 *
 * \param[in] arena The arena that will own the unref.
 * \param[in] object The object to release on free. Must not be NULL.
 * \param[in] unref The callback that releases \p object. Must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the
 *         list node cannot be allocated, or \ref CARDANO_ERROR_ILLEGAL_STATE if the
 *         node would push the arena past its byte ceiling.
 */
cardano_error_t
cardano_uplc_arena_register_unref(cardano_uplc_arena_t* arena, void* object, cardano_uplc_arena_unref_t unref);

/**
 * \brief Returns the total number of payload bytes the arena has served.
 *
 * Counts the aligned, padded bytes handed out by \ref cardano_uplc_arena_alloc; it
 * does not count block headers or list bookkeeping. Intended for tests and budget
 * cross-checks.
 *
 * \param[in] arena The arena to query.
 *
 * \return The number of bytes served, or 0 if \p arena is NULL.
 */
size_t
cardano_uplc_arena_bytes_used(const cardano_uplc_arena_t* arena);

/**
 * \brief Releases every block owned by the arena, runs all registered unref
 *        callbacks, and frees the arena itself.
 *
 * \param[in,out] arena Address of the arena pointer. The pointee is freed and set
 *                to NULL. Does nothing if \p arena or \p *arena is NULL.
 */
void
cardano_uplc_arena_free(cardano_uplc_arena_t** arena);

/**
 * \brief Runs all registered unref callbacks and rewinds the arena for reuse.
 *
 * Calls every registered unref callback, clears the unref list, and moves every
 * block onto a spare list while keeping the allocated block memory, so the next
 * generation of allocations reuses the same blocks without touching the backing
 * allocator. The arena remains valid; it is not freed.
 *
 * \param[in,out] arena The arena to rewind. Does nothing if \p arena is NULL.
 */
void
cardano_uplc_arena_reset(cardano_uplc_arena_t* arena);

/* INTERNAL DECLARATIONS *****************************************************/

/* The following entry points are module-internal (the cardano_uplc_int_ prefix
 * marks them so). They are not part of the public API and are declared here so
 * that the arena implementation and a small number of internal callers/tests can
 * reach them. */

/**
 * \brief Creates a region allocator with an explicit byte ceiling.
 *
 * Internal entry point behind \ref cardano_uplc_arena_new. The public function
 * delegates here with the default ceiling; exposing the explicit ceiling lets
 * callers and tests drive the ceiling-enforcement branches with a small, cheap
 * bound. Not part of the public API; the \c cardano_uplc_int_ prefix marks it
 * module-internal.
 *
 * \param[in] block_size Size in bytes of each underlying block payload. A value
 *            of 0 selects an internal default. Allocations larger than this get
 *            their own oversized block.
 * \param[in] byte_ceiling Upper bound on the total payload bytes the arena will
 *            serve before allocation fails and \ref cardano_uplc_arena_register_unref
 *            refuses.
 * \param[out] arena On success, set to the new arena; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the
 *         backing allocator fails.
 */
cardano_error_t
cardano_uplc_int_arena_new_with_ceiling(size_t block_size, size_t byte_ceiling, cardano_uplc_arena_t** arena);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_ARENA_UPLC_ARENA_H */
