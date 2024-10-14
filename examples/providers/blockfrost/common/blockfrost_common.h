/**
 * \file blockfrost_common.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BLOCKFROST_COMMON_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BLOCKFROST_COMMON_H

/* INCLUDES ******************************************************************/

#include <cardano/common/network_magic.h>
#include <cardano/providers/provider.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* STRUCTURES ***************************************************************/

/**
 * \brief A context structure for managing Blockfrost provider details.
 *
 * This structure holds the necessary information for interacting with the Blockfrost API,
 * including the network magic for the Cardano network and the project ID used for authentication.
 *
 * \var network
 * The Cardano network magic number used to identify the specific network (mainnet, testnet, etc.).
 *
 * \var project_id
 * The project ID associated with a Blockfrost project. This is used for authenticating API requests.
 *
 * \var project_id_size
 * The size (in bytes) of the project ID stored in `project_id`. It represents the actual length of the
 * project ID string, not the full size of the array.
 */
typedef struct blockfrost_context_t
{
    cardano_object_t        base;
    cardano_network_magic_t network;
    char                    project_id[64];
    size_t                  project_id_size;
} blockfrost_context_t;

/* ENUMERATIONS *************************************************************/

/**
 * \brief Enumerates the content types supported by the Blockfrost API.
 */
typedef enum
{
  /**
   * JSON content type.
   */
  CARDANO_BLOCKFROST_CONTENT_TYPE_JSON = 0,

  /**
   * CBOR content type.
   */
  CARDANO_BLOCKFROST_CONTENT_TYPE_CBOR = 1,
} cardano_blockfrost_content_type_t;

/* DECLARATIONS **************************************************************/

/**
 * \brief Constructs the HTTP headers required for Blockfrost API requests.
 *
 * This function creates a list of headers to be used in HTTP requests to the Blockfrost API.
 * Specifically, it includes the `project_id` for authentication in the `project_id` header.
 *
 * \param[in] project_id A pointer to the project ID string used for authentication with the Blockfrost API.
 *                       It should be a valid, non-null C-string.
 * \param[in] project_id_size The length of the project ID string. This helps ensure the correct number
 *                            of bytes is included in the request header.
 * \param[in] content_type The content type to be used in the request. This determines the format of the
 *                         payload (e.g., JSON or CBOR).
 *
 * \return A pointer to a `curl_slist` structure containing the headers. This list must be freed
 *         after usage with `curl_slist_free_all()`. Returns `NULL` if memory allocation fails.
 */
struct curl_slist*
cardano_blockfrost_get_headers(const char* project_id, size_t project_id_size, cardano_blockfrost_content_type_t content_type);

/**
 * \brief Parses the error response from a Cardano provider and extracts the error message.
 *
 * This function takes the error response stored in the `buffer` and extracts relevant error
 * information. It then stores the extracted error message in the provider’s internal error state.
 *
 * \param[in] provider A pointer to the Cardano provider implementation where the error message will be stored.
 *                     This must be a valid pointer to a `cardano_provider_impl_t` structure.
 * \param[in] buffer A pointer to a `cardano_buffer_t` structure containing the error response data
 *                   received from the provider. It should be a valid buffer containing error details.
 *
 * \note This function does not return any value, but it modifies the internal state of the
 *       `provider` by setting the error message for further handling.
 */
void
cardano_blockfrost_parse_error(cardano_provider_impl_t* provider, cardano_buffer_t* buffer);

/**
 * \brief Callback function to handle the response data from a libcurl request.
 *
 * This function is called by libcurl as the data is received from the server. It appends the received data to a
 * buffer provided by the user. The buffer must be managed by the caller, and the total size of the data
 * is calculated as `size * count`.
 *
 * \param[in] contents      A pointer to the raw data received from the server.
 * \param[in] size          The size of each data chunk received.
 * \param[in] count         The number of data chunks received.
 * \param[in] user_provided A pointer to a buffer (e.g., a `cardano_buffer_t` structure) provided by the user
 *                          where the response data will be appended.
 *
 * \return The total size of the data that was processed, calculated as `size * count`. This value tells libcurl
 *         how much data was processed, and if it differs from the input size, libcurl will stop the request.
 */
size_t
cardano_blockfrost_handle_response(void* contents, size_t size, size_t count, void* user_provided);

/**
 * \brief Performs an HTTP GET request using the Blockfrost provider.
 *
 * This function sends an HTTP GET request to the specified URL using the Blockfrost API provider.
 * It retrieves the response code and the response body, which are returned to the caller via
 * the provided output parameters.
 *
 * \param[in]  provider_impl  Pointer to the \ref cardano_provider_impl_t instance, representing
 *                            the Blockfrost provider implementation. Must not be `NULL`.
 * \param[in]  url            Pointer to a null-terminated string containing the URL to which the
 *                            GET request should be sent. Must not be `NULL`.
 * \param[in]  url_size       The size of the URL string in bytes (excluding the null terminator).
 * \param[in]  content_type   The content type to be used in the request. This determines the format of the
 *                            payload (e.g., JSON or CBOR).
 * \param[out] response_buffer Pointer to a \ref cardano_buffer_t* where the function will store the
 *                            response body. The buffer will be allocated within the function, and the caller
 *                            is responsible for freeing it using the appropriate deallocation function.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *          - Other error codes for provider-specific failures.
 */
cardano_error_t
cardano_blockfrost_http_get(
  cardano_provider_impl_t* provider_impl,
  const char*              url,
  size_t                   url_size,
  uint64_t*                response_code,
  cardano_buffer_t**       response_buffer);

/**
 * \brief Sends an HTTP POST request to the Blockfrost API.
 *
 * This function posts data to the specified URL using the Blockfrost API. The request body is included in the HTTP POST request,
 * and the response code and response content are returned.
 *
 * \param[in] provider_impl A pointer to an initialized \ref cardano_provider_impl_t object that contains the necessary context for making the HTTP request.
 *                          This parameter must not be NULL.
 * \param[in] url A pointer to a null-terminated string representing the target URL for the HTTP POST request.
 *                This parameter must not be NULL.
 * \param[in] url_size The size of the URL string, including the null terminator.
 * \param[in] body A pointer to the data to be posted as the body of the HTTP request.
 *                 This parameter must not be NULL if body_size is greater than 0.
 * \param[in] body_size The size of the body data in bytes. If set to 0, the request will have no body content.
 * \param[in] content_type A \ref cardano_blockfrost_content_type_t value specifying the content type of the request body (e.g., JSON or CBOR).
 * \param[out] response_code A pointer to a \c uint64_t that will be set to the HTTP response code from the server (e.g., 200 for success).
 *                           This parameter must not be NULL.
 * \param[out] response_buffer A pointer to a pointer of \ref cardano_buffer_t that will be populated with the response body content from the server.
 *                             The caller is responsible for managing the lifecycle of the returned \ref cardano_buffer_t object and must release it
 *                             by calling \ref cardano_buffer_unref when it is no longer needed. If no response body is provided by the server, this will be set to NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the POST request was successful,
 *         or an appropriate error code if the request failed.
 */
cardano_error_t
cardano_blockfrost_http_post(
  cardano_provider_impl_t*          provider_impl,
  const char*                       url,
  size_t                            url_size,
  const byte_t*                     body,
  size_t                            body_size,
  cardano_blockfrost_content_type_t content_type,
  uint64_t*                         response_code,
  cardano_buffer_t**                response_buffer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BLOCKFROST_COMMON_H
