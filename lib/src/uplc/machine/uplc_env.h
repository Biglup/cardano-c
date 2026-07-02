/**
 * \file uplc_env.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_ENV_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_ENV_H

/* INCLUDES ******************************************************************/

#include "uplc_value.h"

#include "../arena/uplc_arena.h"
#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Number of value slots per environment chunk.
 *
 * Eight slots keep a chunk at 80 bytes while dividing the pointer chase of a
 * deep de Bruijn lookup by eight relative to a plain cons list. Measured on the
 * plutus_use_cases corpus, eight outperforms both four and sixteen slots.
 */
#define CARDANO_UPLC_ENV_CHUNK_SLOTS 8U

/**
 * \brief One chunk of the de Bruijn environment.
 *
 * The environment is a persistent chunked list: each chunk packs up to
 * \ref CARDANO_UPLC_ENV_CHUNK_SLOTS bound values, newest last, in front of a
 * chain of full enclosing chunks. Lookup is by 1-based de Bruijn index: index 1
 * is the newest slot of the head chunk (the innermost binder) and deeper
 * indices step through the slots and then hop chunks, so a lookup at depth
 * \c d chases about \c d / \ref CARDANO_UPLC_ENV_CHUNK_SLOTS pointers rather
 * than \c d. Chunks are arena-allocated and immutable; extension copies the
 * head chunk's slots into a fresh chunk (or starts a new chunk when the head
 * is full) and shares the enclosing chain, so every tail chunk is always full.
 */
struct cardano_uplc_env_t
{
    /** \brief Number of occupied slots, 1 to \ref CARDANO_UPLC_ENV_CHUNK_SLOTS. */
    size_t count;
    /** \brief The enclosing chain of full chunks, or NULL at the outermost chunk. */
    const cardano_uplc_env_t* next;
    /** \brief The bound values, oldest first; \c slots[count - 1] is de Bruijn index 1. */
    const cardano_uplc_value_t* slots[CARDANO_UPLC_ENV_CHUNK_SLOTS];
};

/**
 * \brief Extends an environment with a value at a fresh de Bruijn level.
 *
 * Allocates a new head chunk from \p arena that binds \p value at de Bruijn
 * index 1: when \p env has room in its head chunk the existing slots are copied
 * in front of the enclosing chain, and when it is full (or \p env is NULL) a
 * fresh one-slot chunk is started with \p env as the enclosing chain. The
 * enclosing chunks are shared, never copied, so the previous environment value
 * remains valid and unchanged.
 *
 * \param[in] arena The arena to allocate the chunk from. Must not be NULL.
 * \param[in] env The environment to extend, or NULL for the empty environment.
 * \param[in] value The value to bind at the new head. Must not be NULL and must
 *            live in \p arena.
 * \param[out] out On success, set to the new environment head; left untouched on
 *             failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena, \p value, or \p out is NULL, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve
 *         the cell.
 */
cardano_error_t
cardano_uplc_env_extend(
  struct cardano_uplc_arena_t* arena,
  const cardano_uplc_env_t*    env,
  const cardano_uplc_value_t*  value,
  const cardano_uplc_env_t**   out);

/**
 * \brief Looks a value up in an environment by 1-based de Bruijn index.
 *
 * Walks \p env starting at the head: index 1 returns the head value, index 2 the
 * next, and so on. Index 0 is not a valid de Bruijn index and is reported as not
 * found, as is any index that walks off the end of the list (including a lookup
 * in the empty environment).
 *
 * \param[in] env The environment to search, or NULL for the empty environment.
 * \param[in] index The 1-based de Bruijn index to resolve.
 * \param[out] out On success, set to the bound value; left untouched when the
 *             index is not found or \p out is NULL.
 *
 * \return \ref CARDANO_SUCCESS when the index resolves to a value,
 *         \ref CARDANO_ERROR_POINTER_IS_NULL if \p out is NULL, or
 *         \ref CARDANO_ERROR_ELEMENT_NOT_FOUND when \p index is 0 or out of
 *         range for \p env.
 */
cardano_error_t
cardano_uplc_env_lookup(
  const cardano_uplc_env_t*    env,
  uint64_t                     index,
  const cardano_uplc_value_t** out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_ENV_H */
