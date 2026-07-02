/**
 * \file uplc_env.c
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

/* INCLUDES ******************************************************************/

#include "uplc_env.h"

#include "../arena/uplc_arena.h"

#include <stddef.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Allocates an environment chunk from the arena.
 *
 * \param[in] arena The arena to allocate from.
 *
 * \return The new chunk, or NULL if the arena cannot serve the node.
 */
static cardano_uplc_env_t*
alloc_env(cardano_uplc_arena_t* arena)
{
  return (cardano_uplc_env_t*)cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_env_t), 0U);
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_env_extend(
  cardano_uplc_arena_t*       arena,
  const cardano_uplc_env_t*   env,
  const cardano_uplc_value_t* value,
  const cardano_uplc_env_t**  out)
{
  cardano_uplc_env_t* result = NULL;

  if ((arena == NULL) || (value == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = alloc_env(arena);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  if ((env == NULL) || (env->count >= CARDANO_UPLC_ENV_CHUNK_SLOTS))
  {
    result->count    = (size_t)1;
    result->next     = env;
    result->slots[0] = value;
  }
  else
  {
    size_t i = (size_t)0;

    for (i = (size_t)0; i < env->count; ++i)
    {
      result->slots[i] = env->slots[i];
    }

    result->slots[env->count] = value;
    result->count             = env->count + (size_t)1;
    result->next              = env->next;
  }

  *out = result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_env_lookup(
  const cardano_uplc_env_t*    env,
  uint64_t                     index,
  const cardano_uplc_value_t** out)
{
  const cardano_uplc_env_t* current   = env;
  uint64_t                  remaining = index;

  if (out == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (index == 0U)
  {
    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  while (current != NULL)
  {
    if (remaining <= (uint64_t)current->count)
    {
      *out = current->slots[current->count - (size_t)remaining];

      return CARDANO_SUCCESS;
    }

    remaining -= (uint64_t)current->count;
    current   = current->next;
  }

  return CARDANO_ERROR_ELEMENT_NOT_FOUND;
}
