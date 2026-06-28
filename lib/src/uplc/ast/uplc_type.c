/**
 * \file uplc_type.c
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

#include "uplc_type.h"

#include "../arena/uplc_arena.h"

#include <stddef.h>

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_type_new(
  cardano_uplc_arena_t*      arena,
  cardano_uplc_type_kind_t   kind,
  const cardano_uplc_type_t* fst,
  const cardano_uplc_type_t* snd,
  cardano_uplc_type_t**      type)
{
  cardano_uplc_type_t* result = NULL;

  if ((arena == NULL) || (type == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  switch (kind)
  {
    case CARDANO_UPLC_TYPE_LIST:
    case CARDANO_UPLC_TYPE_ARRAY:
    {
      if (fst == NULL)
      {
        return CARDANO_ERROR_POINTER_IS_NULL;
      }

      if (snd != NULL)
      {
        return CARDANO_ERROR_INVALID_ARGUMENT;
      }

      break;
    }
    case CARDANO_UPLC_TYPE_PAIR:
    {
      if ((fst == NULL) || (snd == NULL))
      {
        return CARDANO_ERROR_POINTER_IS_NULL;
      }

      break;
    }
    case CARDANO_UPLC_TYPE_INTEGER:
    case CARDANO_UPLC_TYPE_BYTE_STRING:
    case CARDANO_UPLC_TYPE_STRING:
    case CARDANO_UPLC_TYPE_UNIT:
    case CARDANO_UPLC_TYPE_BOOL:
    case CARDANO_UPLC_TYPE_DATA:
    case CARDANO_UPLC_TYPE_BLS_G1:
    case CARDANO_UPLC_TYPE_BLS_G2:
    case CARDANO_UPLC_TYPE_BLS_ML_RESULT:
    case CARDANO_UPLC_TYPE_VALUE:
    {
      if ((fst != NULL) || (snd != NULL))
      {
        return CARDANO_ERROR_INVALID_ARGUMENT;
      }

      break;
    }
    default:
    {
      return CARDANO_ERROR_INVALID_ARGUMENT;
    }
  }

  result = (cardano_uplc_type_t*)cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_type_t), 0U);

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->kind = kind;
  result->fst  = fst;
  result->snd  = snd;

  *type = result;

  return CARDANO_SUCCESS;
}
