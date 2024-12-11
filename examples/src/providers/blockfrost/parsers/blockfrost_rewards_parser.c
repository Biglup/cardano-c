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

#include <cardano/json/json_object.h>

#include "blockfrost_parsers.h"
#include "utils.h"

#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

cardano_error_t
cardano_blockfrost_parse_rewards(
  cardano_provider_impl_t* provider,
  const char*              json,
  const size_t             size,
  uint64_t*                rewards)
{
  cardano_json_object_t* parsed_json = cardano_json_object_parse(json, size);

  if (parsed_json == NULL)
  {
    cardano_utils_set_error_message(provider, "Failed to parse JSON response");
    cardano_json_object_unref(&parsed_json);

    return CARDANO_ERROR_INVALID_JSON;
  }

  cardano_json_object_t* rewards_obj = NULL;

  if (cardano_json_object_get_ex(parsed_json, "withdrawable_amount", strlen("withdrawable_amount"), &rewards_obj))
  {
    cardano_error_t result = cardano_json_object_get_uint(rewards_obj, rewards);

    if (result != CARDANO_SUCCESS)
    {
      cardano_utils_set_error_message(provider, "Failed to parse rewards from JSON response");
      cardano_json_object_unref(&parsed_json);

      return CARDANO_ERROR_INVALID_JSON;
    }
  }

  cardano_json_object_unref(&parsed_json);

  return CARDANO_SUCCESS;
}
