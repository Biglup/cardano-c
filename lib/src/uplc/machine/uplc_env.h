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

#include <cardano/error.h>
#include <cardano/typedefs.h>
#include "../arena/uplc_arena.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief One cons cell of the de Bruijn environment.
 *
 * The environment is a singly linked list of values that grows at the head when
 * a binder is entered, so lookup is by 1-based de Bruijn index: index 1 is the
 * head (the innermost binder), index 2 is its \c next, and so on. Cells are
 * arena-allocated and immutable; extension allocates a new head and shares the
 * existing tail.
 */
struct cardano_uplc_env_t
{
  /** \brief The value bound at this de Bruijn level. */
  const cardano_uplc_value_t* value;
  /** \brief The enclosing environment, or NULL at the empty environment. */
  const cardano_uplc_env_t* next;
};

/**
 * \brief Extends an environment with a value at a fresh de Bruijn level.
 *
 * Allocates a new cons cell from \p arena whose \c value is \p value and whose
 * \c next is \p env, and returns it through \p out. The new head becomes de
 * Bruijn index 1; the previous head shifts to index 2. \p env may be NULL to
 * extend the empty environment, in which case the result is a one-element
 * environment. The existing tail is shared, not copied.
 *
 * \param[in] arena The arena to allocate the cell from. Must not be NULL.
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
