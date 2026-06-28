/**
 * \file uplc_selected_cost_model.c
 *
 * \author angel.castillo
 * \date   Jun 27, 2026
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

#include "uplc_selected_cost_model.h"
#include "uplc_cost_model.h"
#include "../ast/uplc_lang_version.h"
#include "../builtins/uplc_builtin_semantics.h"

#include <stddef.h>
#include <stdint.h>

/* DEFINITIONS ***************************************************************/

cardano_uplc_cost_model_version_t
cardano_uplc_cost_model_version_for_language(cardano_uplc_lang_version_t lang_version)
{
  cardano_uplc_cost_model_version_t result = CARDANO_UPLC_COST_MODEL_VERSION_V3;

  switch (lang_version)
  {
    case CARDANO_UPLC_LANG_VERSION_V1:
    {
      result = CARDANO_UPLC_COST_MODEL_VERSION_V1;
      break;
    }
    case CARDANO_UPLC_LANG_VERSION_V2:
    {
      result = CARDANO_UPLC_COST_MODEL_VERSION_V2;
      break;
    }
    case CARDANO_UPLC_LANG_VERSION_V3:
    case CARDANO_UPLC_LANG_VERSION_V4:
    {
      result = CARDANO_UPLC_COST_MODEL_VERSION_V3;
      break;
    }
    default:
    {
      break;
    }
  }

  return result;
}

cardano_error_t
cardano_uplc_select_cost_model(
  cardano_uplc_lang_version_t         lang_version,
  uint64_t                            protocol_major,
  const int64_t*                      params,
  size_t                              count,
  cardano_uplc_selected_cost_model_t* out)
{
  cardano_uplc_cost_model_version_t cost_version;
  cardano_uplc_cost_model_t         model;
  cardano_error_t                   result;

  if ((params == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cost_version = cardano_uplc_cost_model_version_for_language(lang_version);

  result = cardano_uplc_cost_model_from_params(cost_version, params, count, &model);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  out->model     = model;
  out->semantics = cardano_uplc_builtin_semantics_for_language_and_protocol(lang_version, protocol_major);

  return CARDANO_SUCCESS;
}
