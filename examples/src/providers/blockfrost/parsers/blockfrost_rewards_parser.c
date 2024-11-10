/**
 * \file blockfrost_rewards_parser.c
 *
 * \author angel.castillo
 * \date   Oct 01, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
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

#include "blockfrost_parsers.h"
#include "utils.h"

#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

cardano_error_t
cardano_blockfrost_parse_rewards(
  cardano_provider_impl_t* provider,
  const char*              json,
  const size_t             size,
  uint64_t*                rewards)
{
  struct json_tokener* tok         = json_tokener_new();
  struct json_object*  parsed_json = json_tokener_parse_ex(tok, json, (int32_t)size);

  if (parsed_json == NULL)
  {
    cardano_utils_set_error_message(provider, "Failed to parse JSON response");
    json_tokener_free(tok);

    return CARDANO_ERROR_INVALID_JSON;
  }

  struct json_object* rewards_obj = NULL;

  if (json_object_object_get_ex(parsed_json, "withdrawable_amount", &rewards_obj))
  {
    *rewards = json_object_get_int64(rewards_obj);
  }

  json_object_put(parsed_json);
  json_tokener_free(tok);

  return CARDANO_SUCCESS;
}
