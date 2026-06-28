/**
 * \file evaluator.c
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

/* INCLUDES ******************************************************************/

#include "../ast/uplc_program.h"
#include "../ast/uplc_term.h"
#include <cardano/uplc/uplc_apply_params.h>

#include "../arena/uplc_arena.h"

#include <stddef.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Folds the elements of a plutus list onto a program, left to right.
 *
 * Applies \c list[0] first so it becomes the innermost argument, matching the
 * application order of \c apply_params_to_script. Each element is borrowed from
 * \p params with its own reference, handed to the apply step, then released; the
 * apply step takes the arena's own reference, so the list keeps its contents.
 *
 * \param[in] arena The arena every new node is allocated from.
 * \param[in] params The parameters to apply, or NULL to apply none.
 * \param[in] program The program to wrap.
 * \param[out] out On success, the parameterized program.
 *
 * \return \ref CARDANO_SUCCESS on success, or a propagated error.
 */
static cardano_error_t
apply_param_list(
  cardano_uplc_arena_t*          arena,
  const cardano_plutus_list_t*   params,
  const cardano_uplc_program_t*  program,
  const cardano_uplc_program_t** out)
{
  const cardano_uplc_program_t* current = program;
  size_t                        count   = 0U;
  size_t                        i       = 0U;

  if (params != NULL)
  {
    count = cardano_plutus_list_get_length(params);
  }

  for (i = 0U; i < count; ++i)
  {
    cardano_plutus_data_t*  param   = NULL;
    cardano_uplc_program_t* applied = NULL;
    cardano_error_t         get_res = cardano_plutus_list_get(params, i, &param);

    if (get_res != CARDANO_SUCCESS)
    {
      return get_res;
    }

    cardano_error_t apply_res = cardano_uplc_program_apply_data(arena, current, param, &applied);

    cardano_plutus_data_unref(&param);

    if (apply_res != CARDANO_SUCCESS)
    {
      return apply_res;
    }

    current = applied;
  }

  *out = current;

  return CARDANO_SUCCESS;
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_apply_params_to_script(
  const cardano_plutus_list_t* params,
  const byte_t*                script_bytes,
  size_t                       script_size,
  cardano_buffer_t**           out_script)
{
  static const size_t kArenaBlockSize = 4096U;

  cardano_uplc_arena_t*         arena   = NULL;
  const cardano_uplc_program_t* program = NULL;
  const cardano_uplc_program_t* applied = NULL;
  cardano_buffer_t*             encoded = NULL;
  cardano_error_t               result  = CARDANO_SUCCESS;

  if ((out_script == NULL) || ((script_bytes == NULL) && (script_size != 0U)))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = cardano_uplc_arena_new(kArenaBlockSize, &arena);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_uplc_program_from_script_bytes(arena, script_bytes, script_size, &program);

  if (result != CARDANO_SUCCESS)
  {
    cardano_uplc_arena_free(&arena);

    return result;
  }

  result = apply_param_list(arena, params, program, &applied);

  if (result != CARDANO_SUCCESS)
  {
    cardano_uplc_arena_free(&arena);

    return result;
  }

  result = cardano_uplc_program_to_cbor(applied, &encoded);

  cardano_uplc_arena_free(&arena);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *out_script = encoded;

  return CARDANO_SUCCESS;
}
