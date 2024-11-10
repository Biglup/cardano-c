/**
 * \file blockfrost_common.c
 *
 * \author angel.castillo
 * \date   Sep 30, 2024
 *
 * Copyright 2024 Biglup Labs
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

#include "blockfrost_common.h"
#include "../utils/console.h"
#include "utils.h"

#include <cardano/export.h>

#include <curl/curl.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* DEFINITIONS ***************************************************************/

struct curl_slist*
cardano_blockfrost_get_headers(
  const char*                             project_id,
  const size_t                            project_id_size,
  const cardano_blockfrost_content_type_t content_type)
{
  struct curl_slist* headers = NULL;

  char* project_id_header = malloc(project_id_size + 13U);
  CARDANO_UNUSED(memset(project_id_header, 0, project_id_size + 13U));

  CARDANO_UNUSED(snprintf(project_id_header, project_id_size + 13U, "project_id: %s", project_id));

  if (content_type == CARDANO_BLOCKFROST_CONTENT_TYPE_JSON)
  {
    headers = curl_slist_append(headers, "Content-Type: application/json");
  }
  else if (content_type == CARDANO_BLOCKFROST_CONTENT_TYPE_CBOR)
  {
    headers = curl_slist_append(headers, "Content-Type: application/cbor");
  }

  headers = curl_slist_append(headers, project_id_header);

  free(project_id_header);

  return headers;
}

void
cardano_blockfrost_parse_error(cardano_provider_impl_t* provider, cardano_buffer_t* buffer)
{
  if (buffer == NULL)
  {
    return;
  }

  struct json_tokener* tok         = json_tokener_new();
  struct json_object*  parsed_json = NULL;
  struct json_object*  status_code = NULL;
  struct json_object*  error       = NULL;
  struct json_object*  message     = NULL;

  parsed_json = json_tokener_parse_ex(tok, (char*)cardano_buffer_get_data(buffer), (int32_t)cardano_buffer_get_size(buffer));

  if (parsed_json == NULL)
  {
    cardano_utils_set_error_message(provider, "Failed to parse JSON response");
    json_tokener_free(tok);
    return;
  }

  CARDANO_UNUSED(json_object_object_get_ex(parsed_json, "status_code", &status_code));
  CARDANO_UNUSED(json_object_object_get_ex(parsed_json, "error", &error));
  CARDANO_UNUSED(json_object_object_get_ex(parsed_json, "message", &message));

  CARDANO_UNUSED(snprintf(provider->error_message, 1024, "%lu - %s - %s", json_object_get_uint64(status_code), json_object_get_string(error), json_object_get_string(message)));

  json_object_put(parsed_json);
  json_tokener_free(tok);
}

size_t
cardano_blockfrost_handle_response(void* contents, const size_t size, const size_t count, void* user_provided)
{
  const size_t      total_size = size * count;
  cardano_buffer_t* buffer     = (cardano_buffer_t*)user_provided;

  cardano_error_t result = cardano_buffer_write(buffer, contents, total_size);

  console_debug("Received response of %lu bytes", total_size);
  console_debug("%.*s", (int)total_size, (char*)contents);

  CARDANO_UNUSED(result);

  return total_size;
}

cardano_error_t
cardano_blockfrost_http_get(cardano_provider_impl_t* provider_impl, const char* url, const size_t url_size, uint64_t* response_code, cardano_buffer_t** response_buffer)
{
  CARDANO_UNUSED(url_size);

  blockfrost_context_t* context = (blockfrost_context_t*)provider_impl->context;
  *response_buffer              = cardano_buffer_new(1024);
  *response_code                = 0;

  curl_global_init(CURL_GLOBAL_DEFAULT);
  CURL* curl = curl_easy_init();

  if (!curl)
  {
    cardano_buffer_unref(response_buffer);
    cardano_utils_set_error_message(provider_impl, "Failed to initialize libcurl");

    return CARDANO_ERROR_GENERIC;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);

  struct curl_slist* headers = cardano_blockfrost_get_headers(context->project_id, context->project_id_size, CARDANO_BLOCKFROST_CONTENT_TYPE_JSON);

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cardano_blockfrost_handle_response);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)(*response_buffer));

  console_debug("Sending GET request to endpoint: %s", url);

  CURLcode res = curl_easy_perform(curl);

  if (res != CURLE_OK)
  {
    cardano_utils_set_error_message(provider_impl, curl_easy_strerror(res));

    cardano_buffer_unref(response_buffer);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return CARDANO_ERROR_GENERIC;
  }

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, response_code);

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_blockfrost_http_post(
  cardano_provider_impl_t*          provider_impl,
  const char*                       url,
  const size_t                      url_size,
  const byte_t*                     body,
  const size_t                      body_size,
  cardano_blockfrost_content_type_t content_type,
  uint64_t*                         response_code,
  cardano_buffer_t**                response_buffer)
{
  CARDANO_UNUSED(url_size);

  blockfrost_context_t* context = (blockfrost_context_t*)provider_impl->context;
  *response_buffer              = cardano_buffer_new(1024);
  *response_code                = 0;

  curl_global_init(CURL_GLOBAL_DEFAULT);
  CURL* curl = curl_easy_init();

  if (!curl)
  {
    cardano_buffer_unref(response_buffer);
    cardano_utils_set_error_message(provider_impl, "Failed to initialize libcurl");

    return CARDANO_ERROR_GENERIC;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);

  struct curl_slist* headers = cardano_blockfrost_get_headers(context->project_id, context->project_id_size, content_type);

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cardano_blockfrost_handle_response);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)(*response_buffer));
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body_size);

  console_debug("Sending POST request to endpoint: %s", url);
  console_debug("Sending POST request payload: %.*s", body_size, (char*)body);

  CURLcode res = curl_easy_perform(curl);

  if (res != CURLE_OK)
  {
    cardano_utils_set_error_message(provider_impl, curl_easy_strerror(res));

    cardano_buffer_unref(response_buffer);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return CARDANO_ERROR_GENERIC;
  }

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, response_code);

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  return CARDANO_SUCCESS;
}
