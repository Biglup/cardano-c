/**
 * \file blockfrost_script_parser.c
 *
 * \author angel.castillo
 * \date   Oct 1, 2024
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

#include "blockfrost/common/blockfrost_common.h"
#include "blockfrost/common/blockfrost_url_builders.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Constructs a URL string for a given script hash and optional path suffix.
 *
 * This function constructs a URL string for the Blockfrost , including
 * optional path suffixes such as `/json` or `/cbor` for different representations of the script.
 *
 * \param[in]  provider_impl  Pointer to the provider implementation instance.
 * \param[in]  script_hash    Pointer to the script hash string to append to the URL.
 * \param[in]  script_hash_len The length of the script hash string.
 * \param[in]  path_suffix    Optional path suffix to append to the URL (e.g., "/json", "/cbor").
 *                            Can be NULL or an empty string if no suffix is required.
 * \return A dynamically allocated string containing the constructed URL. The caller is responsible for freeing the returned string.
 */
static char*
construct_script_url_with_suffix(
  cardano_provider_impl_t* provider_impl,
  const char*              script_hash,
  const size_t             script_hash_len,
  const char*              path_suffix)
{
  blockfrost_context_t* context = (blockfrost_context_t*)provider_impl->context;

  char* base_path = cardano_blockfrost_get_endpoint_url(context->network, "scripts/");

  if (!base_path)
  {
    return NULL;
  }

  const char* suffix     = path_suffix ? path_suffix : "";
  size_t      suffix_len = cardano_utils_safe_strlen(suffix, 256U);

  size_t base_path_len = cardano_utils_safe_strlen(base_path, 256U);
  size_t url_len       = base_path_len + script_hash_len + suffix_len + 1;

  char* url = malloc(url_len);

  if (!url)
  {
    free(base_path);
    return NULL;
  }

  snprintf(url, url_len, "%s%s%s", base_path, script_hash, suffix);

  free(base_path);

  return url;
}

/**
 * \brief Parses the script language from a JSON string and returns a \ref cardano_script_language_t value.
 *
 * This static function parses the script language (e.g., Plutus V1, Plutus V2) from the provided JSON string and returns it as a \ref cardano_script_language_t value.
 *
 * \param[in] provider A pointer to an initialized \ref cardano_provider_impl_t object that provides the necessary context for parsing.
 *                     This parameter must not be NULL.
 * \param[in] json A pointer to a character array representing the JSON string containing the script language data.
 *                 This string must not be NULL.
 * \param[in] size The size of the \p json string in bytes.
 * \param[out] language On successful parsing, this will point to a \ref cardano_script_language_t value representing
 *                      the parsed script language (e.g., PLUTUS_V1, PLUTUS_V2). The caller can use this to determine
 *                      the script version.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the script language
 *         was successfully parsed, or an appropriate error code if an error occurred (e.g., if the JSON format is invalid).
 */
static cardano_error_t
parse_script_language(
  cardano_provider_impl_t*   provider,
  const char*                json,
  const size_t               size,
  cardano_script_language_t* language)
{
  cardano_json_object_t* parsed_json = cardano_json_object_parse(json, size);

  if (parsed_json == NULL)
  {
    cardano_utils_set_error_message(provider, "Failed to parse JSON response");

    return CARDANO_ERROR_INVALID_JSON;
  }

  cardano_json_object_t* script_obj = NULL;

  if (!cardano_json_object_get_ex(parsed_json, "type", 4, &script_obj))
  {
    cardano_utils_set_error_message(provider, "Failed to parse script type from JSON response");

    cardano_json_object_unref(&parsed_json);

    return CARDANO_ERROR_INVALID_JSON;
  }

  size_t      script_len = 0U;
  const char* type       = cardano_json_object_get_string(script_obj, &script_len);

  if (strcmp(type, "timelock") == 0)
  {
    *language = CARDANO_SCRIPT_LANGUAGE_NATIVE;
  }
  else if (strcmp(type, "plutusV1") == 0)
  {
    *language = CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1;
  }
  else if (strcmp(type, "plutusV2") == 0)
  {
    *language = CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2;
  }
  else if (strcmp(type, "plutusV3") == 0)
  {
    *language = CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3;
  }
  else
  {
    cardano_utils_set_error_message(provider, "Invalid script type");

    cardano_json_object_unref(&parsed_json);

    return CARDANO_ERROR_INVALID_JSON;
  }

  cardano_json_object_unref(&parsed_json);

  return CARDANO_SUCCESS;
}

/**
 * \brief Parses a Plutus script from a JSON string and returns a \ref cardano_script_t object.
 *
 * This function parses a Plutus script from a provided JSON string, which can represent a Plutus V1 or V2 script depending on the specified language.
 *
 * \param[in] provider A pointer to an initialized \ref cardano_provider_impl_t object that provides the necessary context for parsing.
 *                     This parameter must not be NULL.
 * \param[in] json A pointer to a character array representing the JSON string containing the Plutus script data. This string must not be NULL.
 * \param[in] size The size of the \p json string in bytes.
 * \param[in] language A \ref cardano_script_language_t value indicating the version of the Plutus language (e.g., PLUTUS_V1, PLUTUS_V2).
 * \param[out] script On successful parsing, this will point to a newly created \ref cardano_script_t object representing
 *                    the parsed Plutus script. The caller is responsible for managing the lifecycle of this object and must
 *                    call \ref cardano_script_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the Plutus script
 *         was successfully parsed, or an appropriate error code if an error occurred (e.g., if the JSON format is invalid or unsupported).
 */
static cardano_error_t
parse_plutus_script(
  cardano_provider_impl_t*        provider,
  const char*                     json,
  const size_t                    size,
  const cardano_script_language_t language,
  cardano_script_t**              script)
{
  cardano_json_object_t* parsed_json = cardano_json_object_parse(json, size);

  if (parsed_json == NULL)
  {
    cardano_utils_set_error_message(provider, "Failed to parse JSON response");

    return CARDANO_ERROR_INVALID_JSON;
  }

  cardano_json_object_t* script_obj = NULL;

  if (!cardano_json_object_get_ex(parsed_json, "cbor", 4, &script_obj))
  {
    cardano_utils_set_error_message(provider, "Failed to parse script from JSON response");

    cardano_json_object_unref(&parsed_json);

    return CARDANO_ERROR_INVALID_JSON;
  }

  size_t      script_len  = 0U;
  const char* script_data = cardano_json_object_get_string(script_obj, &script_len);

  cardano_error_t result = CARDANO_SUCCESS;

  switch (language)
  {
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1:
    {
      cardano_plutus_v1_script_t* plutus_v1_script = NULL;

      result = cardano_plutus_v1_script_new_bytes_from_hex(script_data, script_len - 1U, &plutus_v1_script);

      if (result != CARDANO_SUCCESS)
      {
        cardano_utils_set_error_message(provider, "Failed to parse Plutus V1 script from JSON response");

        cardano_json_object_unref(&parsed_json);

        return result;
      }

      result = cardano_script_new_plutus_v1(plutus_v1_script, script);

      cardano_plutus_v1_script_unref(&plutus_v1_script);
      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2:
    {
      cardano_plutus_v2_script_t* plutus_v2_script = NULL;

      result = cardano_plutus_v2_script_new_bytes_from_hex(script_data, script_len - 1U, &plutus_v2_script);

      if (result != CARDANO_SUCCESS)
      {
        cardano_utils_set_error_message(provider, "Failed to parse Plutus V2 script from JSON response");

        cardano_json_object_unref(&parsed_json);

        return result;
      }

      result = cardano_script_new_plutus_v2(plutus_v2_script, script);

      cardano_plutus_v2_script_unref(&plutus_v2_script);
      break;
    }
    case CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3:
    {
      cardano_plutus_v3_script_t* plutus_v3_script = NULL;

      result = cardano_plutus_v3_script_new_bytes_from_hex(script_data, script_len - 1U, &plutus_v3_script);

      if (result != CARDANO_SUCCESS)
      {
        cardano_utils_set_error_message(provider, "Failed to parse Plutus V3 script from JSON response");

        cardano_json_object_unref(&parsed_json);

        return result;
      }

      result = cardano_script_new_plutus_v3(plutus_v3_script, script);

      cardano_plutus_v3_script_unref(&plutus_v3_script);

      break;
    }
    default:
    {
      cardano_utils_set_error_message(provider, "Invalid Plutus language version");

      cardano_json_object_unref(&parsed_json);

      return CARDANO_ERROR_INVALID_JSON;
    }
  }

  cardano_json_object_unref(&parsed_json);

  return result;
}

/**
 * \brief Parses a native script from a JSON string and returns a \ref cardano_script_t object.
 *
 * This static function parses a native Cardano script from a JSON string and converts it into a \ref cardano_script_t object.
 *
 * \param[in] provider A pointer to an initialized \ref cardano_provider_impl_t object that provides the necessary context for parsing.
 *                     This parameter must not be NULL.
 * \param[in] json A pointer to a character array representing the JSON string containing the native script data.
 *                 This string must not be NULL.
 * \param[in] size The size of the \p json string in bytes.
 * \param[out] script On successful parsing, this will point to a newly created \ref cardano_script_t object representing
 *                    the parsed native script. The caller is responsible for managing the lifecycle of this object
 *                    and must call \ref cardano_script_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the native script
 *         was successfully parsed, or an appropriate error code if an error occurred (e.g., if the JSON format is invalid).
 */
static cardano_error_t
parse_native_script(
  cardano_provider_impl_t* provider,
  const char*              json,
  const size_t             size,
  cardano_script_t**       script)
{
  cardano_native_script_t* native_script = NULL;

  cardano_error_t result = cardano_native_script_from_json(json, size, &native_script);

  if (result != CARDANO_SUCCESS)
  {
    cardano_utils_set_error_message(provider, "Failed to parse native script from JSON response");
    return result;
  }

  result = cardano_script_new_native(native_script, script);

  cardano_native_script_unref(&native_script);

  return result;
}

/* IMPLEMENTATION ************************************************************/

cardano_error_t
cardano_blockfrost_get_script(
  cardano_provider_impl_t* provider_impl,
  const char*              script_hash,
  const size_t             script_hash_len,
  cardano_script_t**       script)
{
  char* get_script_type_url = construct_script_url_with_suffix(provider_impl, script_hash, script_hash_len, NULL);

  cardano_buffer_t* response_buffer = NULL;
  uint64_t          response_code   = 0U;

  cardano_error_t result = cardano_blockfrost_http_get(provider_impl, get_script_type_url, cardano_utils_safe_strlen(get_script_type_url, 256U), &response_code, &response_buffer);
  free(get_script_type_url);

  if ((response_code != 200U) || (result != CARDANO_SUCCESS))
  {
    cardano_blockfrost_parse_error(provider_impl, response_buffer);

    cardano_buffer_unref(&response_buffer);

    return CARDANO_ERROR_INVALID_HTTP_REQUEST;
  }

  cardano_script_language_t language = CARDANO_SCRIPT_LANGUAGE_NATIVE;
  result                             = parse_script_language(provider_impl, (char*)cardano_buffer_get_data(response_buffer), cardano_buffer_get_size(response_buffer), &language);

  cardano_buffer_unref(&response_buffer);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (language == CARDANO_SCRIPT_LANGUAGE_NATIVE)
  {
    char* get_script_json_url = construct_script_url_with_suffix(provider_impl, script_hash, script_hash_len, "/json");
    result                    = cardano_blockfrost_http_get(provider_impl, get_script_json_url, cardano_utils_safe_strlen(get_script_json_url, 256U), &response_code, &response_buffer);
    free(get_script_json_url);

    if ((response_code != 200U) || (result != CARDANO_SUCCESS))
    {
      cardano_blockfrost_parse_error(provider_impl, response_buffer);

      cardano_buffer_unref(&response_buffer);

      return CARDANO_ERROR_INVALID_HTTP_REQUEST;
    }

    result = parse_native_script(provider_impl, (char*)cardano_buffer_get_data(response_buffer), cardano_buffer_get_size(response_buffer), script);

    cardano_buffer_unref(&response_buffer);
  }
  else
  {
    char* get_script_cbor_url = construct_script_url_with_suffix(provider_impl, script_hash, script_hash_len, "/cbor");
    result                    = cardano_blockfrost_http_get(provider_impl, get_script_cbor_url, cardano_utils_safe_strlen(get_script_cbor_url, 256U), &response_code, &response_buffer);
    free(get_script_cbor_url);

    if ((response_code != 200U) || (result != CARDANO_SUCCESS))
    {
      cardano_blockfrost_parse_error(provider_impl, response_buffer);

      cardano_buffer_unref(&response_buffer);

      return CARDANO_ERROR_INVALID_HTTP_REQUEST;
    }

    result = parse_plutus_script(provider_impl, (char*)cardano_buffer_get_data(response_buffer), cardano_buffer_get_size(response_buffer), language, script);

    cardano_buffer_unref(&response_buffer);
  }

  if (result != CARDANO_SUCCESS)
  {
    cardano_script_unref(script);
  }

  return result;
}
