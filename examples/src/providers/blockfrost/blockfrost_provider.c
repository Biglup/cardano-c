/**
 * \file blockfrost_provider.c
 *
 * \author angel.castillo
 * \date   Sep 28, 2024
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

#include "../provider_factory.h"

#include "../../utils/utils.h"
#include "blockfrost/common/blockfrost_common.h"
#include "blockfrost/common/blockfrost_url_builders.h"
#include "blockfrost/parsers/blockfrost_parsers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Creates a new blockfrost context object.
 *
 * \return A pointer to the newly created blockfrost context object, or `NULL` if the operation failed.
 */
static blockfrost_context_t*
cardano_blockfrost_context_new(void)
{
  blockfrost_context_t* data = malloc(sizeof(blockfrost_context_t));

  if (data == NULL)
  {
    return NULL;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = free;

  data->network = CARDANO_NETWORK_MAGIC_PREPROD;

  CARDANO_UNUSED(memset(data->project_id, 0, sizeof(data->project_id)));

  return data;
}

/**
 * \brief Retrieves protocol parameters from the provider and stores them in a `cardano_protocol_parameters_t` structure.
 *
 * This function queries the provider (such as Blockfrost) for the current protocol parameters and parses
 * the response into the `cardano_protocol_parameters_t` structure. The function is responsible for handling
 * the request, processing the response, and properly allocating memory for the parameters structure.
 *
 * \param[in] provider_impl  A pointer to the `cardano_provider_impl_t` structure, which contains provider-specific data and logic.
 * \param[out] parameters    A double pointer to a `cardano_protocol_parameters_t` structure. The function will allocate memory
 *                           for the parameters and populate them with the retrieved protocol parameters. The caller is responsible
 *                           for deallocating the memory using the appropriate function once done.
 *
 * \return `CARDANO_SUCCESS` if the parameters were successfully retrieved and parsed; otherwise, an appropriate `cardano_error_t`
 *         error code is returned, indicating the failure reason (e.g., network errors, parsing errors).
 */
static cardano_error_t
get_parameters(cardano_provider_impl_t* provider_impl, cardano_protocol_parameters_t** parameters)
{
  blockfrost_context_t* context         = (blockfrost_context_t*)provider_impl->context;
  char*                 url             = cardano_blockfrost_get_endpoint_url(context->network, "epochs/latest/parameters");
  cardano_buffer_t*     response_buffer = NULL;
  uint64_t              response_code   = 0;

  cardano_error_t result = cardano_blockfrost_http_get(provider_impl, url, cardano_utils_safe_strlen(url, 256U), &response_code, &response_buffer);
  free(url);

  if ((response_code != 200U) || (result != CARDANO_SUCCESS))
  {
    cardano_blockfrost_parse_error(provider_impl, response_buffer);

    cardano_buffer_unref(&response_buffer);

    return CARDANO_ERROR_INVALID_HTTP_REQUEST;
  }

  result = cardano_blockfrost_parse_protocol_parameters(provider_impl, (char*)cardano_buffer_get_data(response_buffer), cardano_buffer_get_size(response_buffer), parameters);

  cardano_buffer_unref(&response_buffer);

  if (result != CARDANO_SUCCESS)
  {
    cardano_protocol_parameters_unref(parameters);
  }

  return result;
}

/**
 * \brief Retrieves the unspent transaction outputs (UTXOs) for a given address.
 *
 * This function fetches the unspent transaction outputs (UTXOs) associated with the specified Cardano address using the provided provider implementation.
 * The UTXOs are returned as a \ref cardano_utxo_list_t object.
 *
 * \param[in] provider_impl A pointer to an initialized \ref cardano_provider_impl_t object that provides the necessary context for querying the blockchain.
 *                          This parameter must not be NULL.
 * \param[in] address A pointer to an initialized \ref cardano_address_t object representing the Cardano address for which UTXOs are being retrieved.
 *                    This parameter must not be NULL.
 * \param[out] utxo_list On successful execution, this will point to a newly created \ref cardano_utxo_list_t object containing the list of UTXOs for the specified address.
 *                       The caller is responsible for managing the lifecycle of this object and must call \ref cardano_utxo_list_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the UTXOs were successfully retrieved,
 *         or an appropriate error code if an error occurred (e.g., if the query failed or the address is invalid).
 */
static cardano_error_t
get_unspent_outputs(
  cardano_provider_impl_t* provider_impl,
  cardano_address_t*       address,
  cardano_utxo_list_t**    utxo_list)
{
  size_t page         = 1;
  size_t max_results  = 100;
  size_t last_results = 0;

  cardano_error_t result = CARDANO_SUCCESS;

  do
  {
    char*             url             = cardano_blockfrost_build_utxo_url(provider_impl, cardano_address_get_string(address), page, max_results);
    cardano_buffer_t* response_buffer = NULL;
    uint64_t          response_code   = 0;

    result = cardano_blockfrost_http_get(provider_impl, url, cardano_utils_safe_strlen(url, 256U), &response_code, &response_buffer);
    free(url);

    if (response_code == 404U)
    {
      cardano_buffer_unref(&response_buffer);
      return cardano_utxo_list_new(utxo_list);
    }

    if ((response_code != 200U) || (result != CARDANO_SUCCESS))
    {
      cardano_blockfrost_parse_error(provider_impl, response_buffer);

      cardano_buffer_unref(&response_buffer);

      return CARDANO_ERROR_INVALID_HTTP_REQUEST;
    }

    cardano_utxo_list_t* current_list = NULL;
    result                            = cardano_blockfrost_parse_unspent_outputs(provider_impl, (char*)cardano_buffer_get_data(response_buffer), cardano_buffer_get_size(response_buffer), &current_list);

    cardano_buffer_unref(&response_buffer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_utxo_list_unref(utxo_list);
    }

    last_results = cardano_utxo_list_get_length(current_list);

    if (*utxo_list == NULL)
    {
      *utxo_list = current_list;
    }
    else
    {
      cardano_utxo_list_t* tmp_list = *utxo_list;

      *utxo_list = cardano_utxo_list_concat(tmp_list, current_list);

      cardano_utxo_list_unref(&tmp_list);
      cardano_utxo_list_unref(&current_list);
    }

    ++page;
  }
  while (last_results == max_results);

  return result;
}

/**
 * \brief Retrieves the staking rewards balance for a given reward address.
 *
 * This function queries the staking rewards associated with a given reward address using the provided \ref cardano_provider_impl_t.
 * The rewards balance is returned as a 64-bit unsigned integer.
 *
 * \param[in] provider_impl A pointer to an initialized \ref cardano_provider_impl_t object that provides the necessary context for querying the rewards.
 *                          This parameter must not be NULL.
 * \param[in] address A pointer to an initialized \ref cardano_reward_address_t object representing the reward address for which to retrieve the rewards.
 *                    This parameter must not be NULL.
 * \param[out] rewards On successful execution, this will hold the total rewards balance as a 64-bit unsigned integer. The caller must ensure this pointer is valid.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the rewards were successfully retrieved,
 *         or an appropriate error code if an error occurred (e.g., if the query failed or if the address is invalid).
 */
static cardano_error_t
get_rewards_balance(cardano_provider_impl_t* provider_impl, cardano_reward_address_t* address, uint64_t* rewards)
{
  char*             url             = cardano_blockfrost_build_rewards_url(provider_impl, cardano_reward_address_get_string(address));
  cardano_buffer_t* response_buffer = NULL;
  uint64_t          response_code   = 0;

  cardano_error_t result = cardano_blockfrost_http_get(provider_impl, url, cardano_utils_safe_strlen(url, 256U), &response_code, &response_buffer);
  free(url);

  if (response_code == 404U)
  {
    *rewards = 0;

    cardano_buffer_unref(&response_buffer);

    return CARDANO_SUCCESS;
  }

  if ((response_code != 200U) || (result != CARDANO_SUCCESS))
  {
    cardano_blockfrost_parse_error(provider_impl, response_buffer);

    cardano_buffer_unref(&response_buffer);

    return CARDANO_ERROR_INVALID_HTTP_REQUEST;
  }

  result = cardano_blockfrost_parse_rewards(provider_impl, (char*)cardano_buffer_get_data(response_buffer), cardano_buffer_get_size(response_buffer), rewards);

  cardano_buffer_unref(&response_buffer);

  return result;
}

/**
 * \brief Retrieves the unspent transaction outputs (UTXOs) associated with a given address and asset ID.
 *
 * This function queries the UTXOs associated with a specified address and filters them by the provided asset ID. The results are stored in a UTXO list.
 *
 * \param[in] provider_impl A pointer to an initialized \ref cardano_provider_impl_t object that provides the necessary context for querying the UTXOs.
 *                          This parameter must not be NULL.
 * \param[in] address A pointer to an initialized \ref cardano_address_t object representing the address to query for UTXOs. This parameter must not be NULL.
 * \param[in] asset_id A pointer to an initialized \ref cardano_asset_id_t object representing the asset ID used to filter the UTXOs. This parameter must not be NULL.
 * \param[out] utxo_list A pointer to a pointer of \ref cardano_utxo_list_t that will be populated with the filtered UTXOs.
 *                       The caller must manage the lifecycle of this list and free it when no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the UTXOs were successfully retrieved,
 *         or an appropriate error code if an error occurred (e.g., if the query failed or the address/asset ID is invalid).
 */
static cardano_error_t
get_unspent_outputs_with_asset(
  cardano_provider_impl_t* provider_impl,
  cardano_address_t*       address,
  cardano_asset_id_t*      asset_id,
  cardano_utxo_list_t**    utxo_list)
{
  size_t page         = 1;
  size_t max_results  = 100;
  size_t last_results = 0;

  cardano_error_t result = CARDANO_SUCCESS;

  do
  {
    char*             url             = cardano_blockfrost_build_utxo_with_asset_url(provider_impl, cardano_address_get_string(address), cardano_asset_id_get_hex(asset_id), page, max_results);
    cardano_buffer_t* response_buffer = NULL;
    uint64_t          response_code   = 0;

    result = cardano_blockfrost_http_get(provider_impl, url, cardano_utils_safe_strlen(url, 256U), &response_code, &response_buffer);
    free(url);

    if (response_code == 404U)
    {
      cardano_buffer_unref(&response_buffer);
      return cardano_utxo_list_new(utxo_list);
    }

    if ((response_code != 200U) || (result != CARDANO_SUCCESS))
    {
      cardano_blockfrost_parse_error(provider_impl, response_buffer);

      cardano_buffer_unref(&response_buffer);

      return CARDANO_ERROR_INVALID_HTTP_REQUEST;
    }

    cardano_utxo_list_t* current_list = NULL;
    result                            = cardano_blockfrost_parse_unspent_outputs(provider_impl, (char*)cardano_buffer_get_data(response_buffer), cardano_buffer_get_size(response_buffer), &current_list);

    cardano_buffer_unref(&response_buffer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_utxo_list_unref(utxo_list);
    }

    last_results = cardano_utxo_list_get_length(current_list);

    if (*utxo_list == NULL)
    {
      *utxo_list = current_list;
    }
    else
    {
      cardano_utxo_list_t* tmp_list = *utxo_list;

      *utxo_list = cardano_utxo_list_concat(tmp_list, current_list);

      cardano_utxo_list_unref(&tmp_list);
      cardano_utxo_list_unref(&current_list);
    }

    ++page;
  }
  while (last_results == max_results);

  return result;
}

/**
 * \brief Retrieves the unspent transaction output (UTXO) associated with a specific NFT (Non-Fungible Token).
 *
 * This function queries the provider implementation to retrieve the UTXO corresponding to the provided NFT asset ID.
 *
 * \param[in] provider_impl A pointer to an initialized \ref cardano_provider_impl_t object that provides the necessary context for querying the UTXO.
 *                          This parameter must not be NULL.
 * \param[in] asset_id A pointer to an initialized \ref cardano_asset_id_t object representing the asset ID of the NFT. This parameter must not be NULL.
 * \param[out] utxo A pointer to a pointer of \ref cardano_utxo_t that will be populated with the UTXO containing the specified NFT.
 *                  The caller is responsible for managing the lifecycle of the returned UTXO and must free it by calling \ref cardano_utxo_unref when no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the UTXO was successfully retrieved,
 *         or an appropriate error code if an error occurred (e.g., failure to find the UTXO or invalid asset ID).
 */
static cardano_error_t
get_unspent_output_by_nft(
  cardano_provider_impl_t* provider_impl,
  cardano_asset_id_t*      asset_id,
  cardano_utxo_t**         utxo)
{
  char*             url             = cardano_blockfrost_build_addresses_with_asset_url(provider_impl, cardano_asset_id_get_hex(asset_id));
  cardano_buffer_t* response_buffer = NULL;
  uint64_t          response_code   = 0;

  cardano_error_t result = cardano_blockfrost_http_get(provider_impl, url, cardano_utils_safe_strlen(url, 256U), &response_code, &response_buffer);
  free(url);

  if ((response_code != 200U) || (result != CARDANO_SUCCESS))
  {
    cardano_blockfrost_parse_error(provider_impl, response_buffer);
    cardano_buffer_unref(&response_buffer);
    return CARDANO_ERROR_INVALID_HTTP_REQUEST;
  }

  cardano_json_object_t* parsed_json = cardano_json_object_parse((const char*)cardano_buffer_get_data(response_buffer), (int32_t)cardano_buffer_get_size(response_buffer));

  if (parsed_json == NULL)
  {
    cardano_buffer_unref(&response_buffer);

    return CARDANO_ERROR_INVALID_JSON;
  }

  size_t array_len = cardano_json_object_array_get_length(parsed_json);

  if (array_len == 0U)
  {
    cardano_buffer_unref(&response_buffer);
    cardano_json_object_unref(&parsed_json);

    cardano_utils_set_error_message(provider_impl, "No asset found for the specified asset ID");

    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  if (array_len != 1U)
  {
    cardano_buffer_unref(&response_buffer);
    cardano_json_object_unref(&parsed_json);

    cardano_utils_set_error_message(provider_impl, "Asset is not an NFT. Multiple assets found for the specified asset ID");

    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  cardano_json_object_t* address_with_count_obj = cardano_json_object_array_get(parsed_json, 0);

  if (address_with_count_obj == NULL)
  {
    cardano_buffer_unref(&response_buffer);
    cardano_json_object_unref(&parsed_json);

    return CARDANO_ERROR_INVALID_JSON;
  }

  cardano_json_object_t* address_obj = NULL;
  cardano_address_t*     address     = NULL;

  if (cardano_json_object_get_ex(address_with_count_obj, "address", 7, &address_obj))
  {
    size_t      address_len = 0U;
    const char* address_str = cardano_json_object_get_string(address_obj, &address_len);

    result = cardano_address_from_string(address_str, address_len - 1U, &address);

    if (result != CARDANO_SUCCESS)
    {
      cardano_buffer_unref(&response_buffer);
      cardano_json_object_unref(&parsed_json);

      return result;
    }
  }

  if (address == NULL)
  {
    cardano_buffer_unref(&response_buffer);
    cardano_json_object_unref(&parsed_json);

    return CARDANO_ERROR_INVALID_JSON;
  }

  cardano_utxo_list_t* utxo_list = NULL;
  result                         = get_unspent_outputs_with_asset(provider_impl, address, asset_id, &utxo_list);

  cardano_json_object_unref(&parsed_json);

  if (result != CARDANO_SUCCESS)
  {
    cardano_address_unref(&address);
    cardano_buffer_unref(&response_buffer);
    return result;
  }

  if (cardano_utxo_list_get_length(utxo_list) == 0)
  {
    cardano_address_unref(&address);
    cardano_utxo_list_unref(&utxo_list);
    cardano_buffer_unref(&response_buffer);

    cardano_utils_set_error_message(provider_impl, "No unspent outputs found for the specified asset ID");

    return CARDANO_ERROR_ELEMENT_NOT_FOUND;
  }

  if (cardano_utxo_list_get_length(utxo_list) > 1)
  {
    cardano_address_unref(&address);
    cardano_utxo_list_unref(&utxo_list);
    cardano_buffer_unref(&response_buffer);

    cardano_utils_set_error_message(provider_impl, "Asset is not an NFT. Multiple unspent outputs found for the specified asset ID");

    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  result = cardano_utxo_list_get(utxo_list, 0, utxo);

  cardano_address_unref(&address);
  cardano_utxo_list_unref(&utxo_list);
  cardano_buffer_unref(&response_buffer);

  return result;
}

/**
 * \brief Resolves the unspent transaction outputs (UTXOs) for a given set of transaction inputs.
 *
 * This function retrieves the UTXOs corresponding to the provided set of transaction inputs by querying the underlying provider implementation.
 * The results are stored in a UTXO list.
 *
 * \param[in] provider_impl A pointer to an initialized \ref cardano_provider_impl_t object that provides the necessary context for resolving UTXOs.
 *                          This parameter must not be NULL.
 * \param[in] tx_ins A pointer to an initialized \ref cardano_transaction_input_set_t object representing the set of transaction inputs to resolve.
 *                   This parameter must not be NULL.
 * \param[out] utxo_list A pointer to a pointer of \ref cardano_utxo_list_t that will be populated with the resolved UTXOs.
 *                       The caller is responsible for managing the lifecycle of this list and freeing it when no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the UTXOs were successfully resolved,
 *         or an appropriate error code if an error occurred (e.g., failure to resolve the UTXOs or invalid input data).
 */
static cardano_error_t
resolve_unspent_outputs(
  cardano_provider_impl_t*         provider_impl,
  cardano_transaction_input_set_t* tx_ins,
  cardano_utxo_list_t**            utxo_list)
{
  cardano_error_t result = cardano_utxo_list_new(utxo_list);

  if (result != CARDANO_SUCCESS)
  {
    cardano_utils_set_error_message(provider_impl, "Failed to create UTXO list");
    return result;
  }

  for (size_t i = 0; i < cardano_transaction_input_set_get_length(tx_ins); ++i)
  {
    cardano_transaction_input_t* tx_in = NULL;
    result                             = cardano_transaction_input_set_get(tx_ins, i, &tx_in);
    cardano_transaction_input_unref(&tx_in);

    if (result != CARDANO_SUCCESS)
    {
      cardano_utils_set_error_message(provider_impl, "Failed to get transaction input");
      cardano_utxo_list_unref(utxo_list);
      return result;
    }

    const uint64_t          index   = cardano_transaction_input_get_index(tx_in);
    cardano_blake2b_hash_t* tx_hash = cardano_transaction_input_get_id(tx_in);
    cardano_blake2b_hash_unref(&tx_hash);

    const size_t tx_id_hex_size = cardano_blake2b_hash_get_hex_size(tx_hash);
    char*        tx_id_hex      = malloc(tx_id_hex_size);

    if (tx_id_hex == NULL)
    {
      cardano_utils_set_error_message(provider_impl, "Failed to allocate memory for transaction ID");
      cardano_utxo_list_unref(utxo_list);

      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    result = cardano_blake2b_hash_to_hex(tx_hash, tx_id_hex, tx_id_hex_size);

    if (result != CARDANO_SUCCESS)
    {
      cardano_utils_set_error_message(provider_impl, "Failed to convert transaction ID to hex");
      cardano_utxo_list_unref(utxo_list);
      free(tx_id_hex);

      return result;
    }

    char* url = cardano_blockfrost_build_transaction_utxos_url(provider_impl, tx_id_hex);

    cardano_buffer_t* response_buffer = NULL;

    uint64_t response_code = 0;

    result = cardano_blockfrost_http_get(provider_impl, url, cardano_utils_safe_strlen(url, 256U), &response_code, &response_buffer);

    free(url);

    if ((response_code != 200U) || (result != CARDANO_SUCCESS))
    {
      cardano_blockfrost_parse_error(provider_impl, response_buffer);
      cardano_buffer_unref(&response_buffer);
      cardano_utxo_list_unref(utxo_list);
      free(tx_id_hex);

      return CARDANO_ERROR_INVALID_HTTP_REQUEST;
    }

    cardano_json_object_t* parsed_json = cardano_json_object_parse((const char*)cardano_buffer_get_data(response_buffer), (int32_t)cardano_buffer_get_size(response_buffer));

    if (parsed_json == NULL)
    {
      cardano_buffer_unref(&response_buffer);
      cardano_utxo_list_unref(utxo_list);
      free(tx_id_hex);

      return CARDANO_ERROR_INVALID_JSON;
    }

    cardano_json_object_t* tx_outputs = NULL;

    if (!cardano_json_object_get_ex(parsed_json, "outputs", 7, &tx_outputs))
    {
      cardano_buffer_unref(&response_buffer);
      cardano_json_object_unref(&parsed_json);
      cardano_utxo_list_unref(utxo_list);
      free(tx_id_hex);

      return CARDANO_ERROR_INVALID_JSON;
    }

    size_t      json_len    = 0;
    const char* json_string = cardano_json_object_to_json_string(tx_outputs, CARDANO_JSON_FORMAT_COMPACT, &json_len);

    cardano_utxo_list_t* tmp_utxo_list = NULL;
    result                             = cardano_blockfrost_parse_tx_unspent_outputs(provider_impl, json_string, json_len - 1U, tx_id_hex, tx_id_hex_size - 1, &tmp_utxo_list);

    if (result != CARDANO_SUCCESS)
    {
      cardano_buffer_unref(&response_buffer);
      cardano_json_object_unref(&parsed_json);
      cardano_utxo_list_unref(utxo_list);
      cardano_utxo_list_unref(&tmp_utxo_list);
      free(tx_id_hex);

      return result;
    }

    for (size_t j = 0; j < cardano_utxo_list_get_length(tmp_utxo_list); ++j)
    {
      cardano_utxo_t* utxo = NULL;

      result = cardano_utxo_list_get(tmp_utxo_list, j, &utxo);
      cardano_utxo_unref(&utxo);

      if (result != CARDANO_SUCCESS)
      {
        cardano_buffer_unref(&response_buffer);
        cardano_json_object_unref(&parsed_json);
        cardano_utxo_list_unref(utxo_list);
        cardano_utxo_list_unref(&tmp_utxo_list);
        free(tx_id_hex);

        return result;
      }

      cardano_transaction_input_t* out_tx_in = cardano_utxo_get_input(utxo);
      cardano_transaction_input_unref(&out_tx_in);

      if (out_tx_in == NULL)
      {
        cardano_buffer_unref(&response_buffer);
        cardano_json_object_unref(&parsed_json);
        cardano_utxo_list_unref(utxo_list);
        cardano_utxo_list_unref(&tmp_utxo_list);
        free(tx_id_hex);

        return CARDANO_ERROR_INVALID_JSON;
      }

      if (index == cardano_transaction_input_get_index(out_tx_in))
      {
        result = cardano_utxo_list_add(*utxo_list, utxo);

        if (result != CARDANO_SUCCESS)
        {
          cardano_buffer_unref(&response_buffer);
          cardano_json_object_unref(&parsed_json);
          cardano_utxo_list_unref(utxo_list);
          cardano_utxo_list_unref(&tmp_utxo_list);
          free(tx_id_hex);

          return result;
        }

        break;
      }
    }

    cardano_buffer_unref(&response_buffer);
    cardano_utxo_list_unref(&tmp_utxo_list);
    free(tx_id_hex);
    cardano_json_object_unref(&parsed_json);
  }

  return result;
}

/**
 * \brief Resolves a datum from the blockchain using its datum hash.
 *
 * This function retrieves the Plutus datum associated with a given datum hash from the blockchain. It queries the blockchain provider
 * to resolve the datum and returns the result in the provided \ref cardano_plutus_data_t pointer.
 *
 * \param[in] provider_impl A pointer to an initialized \ref cardano_provider_impl_t object that provides the necessary context and functions for blockchain interaction.
 *                          This parameter must not be NULL.
 * \param[in] datum_hash A pointer to a \ref cardano_blake2b_hash_t object representing the datum hash to be resolved.
 *                       This parameter must not be NULL.
 * \param[out] datum On success, this will point to the resolved \ref cardano_plutus_data_t object representing the datum.
 *                   The caller is responsible for managing the lifecycle of this object, and must call \ref cardano_plutus_data_unref to release it when no longer needed.
 *                   This parameter must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the datum was successfully resolved,
 *         or an appropriate error code if the datum could not be found or an error occurred during the operation.
 */
static cardano_error_t
resolve_datum(
  cardano_provider_impl_t* provider_impl,
  cardano_blake2b_hash_t*  datum_hash,
  cardano_plutus_data_t**  datum)
{
  size_t hash_size = cardano_blake2b_hash_get_hex_size(datum_hash);
  char*  hash      = malloc(hash_size);

  if (hash == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_blake2b_hash_to_hex(datum_hash, hash, hash_size);

  if (result != CARDANO_SUCCESS)
  {
    free(hash);
    return result;
  }

  char*             url             = cardano_blockfrost_build_datum_url(provider_impl, hash);
  cardano_buffer_t* response_buffer = NULL;
  uint64_t          response_code   = 0U;

  free(hash);

  result = cardano_blockfrost_http_get(provider_impl, url, cardano_utils_safe_strlen(url, 256U), &response_code, &response_buffer);
  free(url);

  if ((response_code != 200U) || (result != CARDANO_SUCCESS))
  {
    cardano_blockfrost_parse_error(provider_impl, response_buffer);

    cardano_buffer_unref(&response_buffer);

    return CARDANO_ERROR_INVALID_HTTP_REQUEST;
  }

  result = cardano_blockfrost_parse_datum(provider_impl, (char*)cardano_buffer_get_data(response_buffer), cardano_buffer_get_size(response_buffer), datum);

  cardano_buffer_unref(&response_buffer);

  return result;
}

/**
 * \brief Awaits confirmation of a transaction on the blockchain within a specified timeout.
 *
 * This function waits for the specified transaction to be confirmed on the blockchain. It periodically checks the transaction's status
 * until either the transaction is confirmed or the specified timeout is reached.
 *
 * \param[in] provider_impl A pointer to an initialized \ref cardano_provider_impl_t object that contains the necessary context for querying the blockchain.
 *                          This parameter must not be NULL.
 * \param[in] tx_id A pointer to a \ref cardano_blake2b_hash_t object representing the transaction ID to be checked for confirmation.
 *                  This parameter must not be NULL.
 * \param[in] timeout_ms The maximum amount of time, in milliseconds, to wait for the transaction to be confirmed.
 *                       If the timeout is reached without confirmation, the function will return.
 * \param[out] confirmed A pointer to a boolean that will be set to \c true if the transaction is confirmed before the timeout, or \c false if the timeout is reached without confirmation.
 *                       This parameter must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the transaction was successfully confirmed,
 *         \ref CARDANO_ERROR_TIMEOUT if the timeout was reached without confirmation, or an appropriate error code if another failure occurred.
 */
static cardano_error_t
await_transaction_confirmation(
  cardano_provider_impl_t* provider_impl,
  cardano_blake2b_hash_t*  tx_id,
  const uint64_t           timeout_ms,
  bool*                    confirmed)
{
  uint64_t        remaining_time_ms = timeout_ms;
  uint64_t        start_time_sec    = cardano_utils_get_time();
  cardano_error_t result            = CARDANO_SUCCESS;

  size_t hash_size = cardano_blake2b_hash_get_hex_size(tx_id);
  char*  hash      = malloc(hash_size);

  result = cardano_blake2b_hash_to_hex(tx_id, hash, hash_size);

  if (result != CARDANO_SUCCESS)
  {
    *confirmed = false;
    free(hash);

    return result;
  }

  char* url = cardano_blockfrost_build_tx_metadata_cbor_url(provider_impl, hash);

  if ((hash == NULL) || (url == NULL))
  {
    free(hash);
    free(url);

    *confirmed = false;

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  do
  {
    uint64_t elapsed_time_sec = cardano_utils_get_elapsed_time_since(start_time_sec);
    uint64_t elapsed_time_ms  = elapsed_time_sec * 1000U;

    if (elapsed_time_ms >= remaining_time_ms)
    {
      remaining_time_ms = 0U;
    }
    else
    {
      remaining_time_ms = remaining_time_ms - elapsed_time_ms;
    }

    cardano_buffer_t* response_buffer = NULL;
    uint64_t          response_code   = 0U;

    result = cardano_blockfrost_http_get(provider_impl, url, cardano_utils_safe_strlen(url, 256U), &response_code, &response_buffer);

    if (result != CARDANO_SUCCESS)
    {
      *confirmed = false;
      cardano_blockfrost_parse_error(provider_impl, response_buffer);

      cardano_buffer_unref(&response_buffer);
      free(hash);
      free(url);

      return CARDANO_ERROR_INVALID_HTTP_REQUEST;
    }

    cardano_buffer_unref(&response_buffer);

    *confirmed = (response_code == 200U);

    if (!(*confirmed) && (remaining_time_ms > 0U))
    {
      uint64_t sleep_time_ms = (remaining_time_ms > 20000U) ? 20000U : remaining_time_ms;
      cardano_utils_sleep(sleep_time_ms);
    }
  }
  while (!(*confirmed) && (remaining_time_ms > 0U));

  free(hash);
  free(url);

  return result;
}

/**
 * \brief Posts a transaction to the Cardano blockchain using the provider implementation.
 *
 * This function submits a signed transaction to the Cardano blockchain using the specified \ref cardano_provider_impl_t provider.
 * Upon successful submission, it returns the transaction ID (hash).
 *
 * \param[in] provider_impl A pointer to an initialized \ref cardano_provider_impl_t object that provides the necessary context for
 *                          interacting with the Cardano blockchain. This parameter must not be NULL.
 * \param[in] tx A pointer to an initialized \ref cardano_transaction_t object representing the signed transaction to be posted to the blockchain.
 *               This parameter must not be NULL.
 * \param[out] tx_id A pointer to a pointer of \ref cardano_blake2b_hash_t that will be populated with the transaction ID (hash) after the transaction is posted.
 *                   The caller is responsible for managing the lifecycle of the returned \ref cardano_blake2b_hash_t object and must release it
 *                   by calling \ref cardano_blake2b_hash_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the transaction was successfully posted,
 *         or an appropriate error code if an error occurred during submission.
 */
static cardano_error_t
post_transaction_to_chain(
  cardano_provider_impl_t* provider_impl,
  cardano_transaction_t*   tx,
  cardano_blake2b_hash_t** tx_id)
{
  blockfrost_context_t* context         = (blockfrost_context_t*)provider_impl->context;
  char*                 base_path       = cardano_blockfrost_get_endpoint_url(context->network, "tx/submit");
  cardano_buffer_t*     response_buffer = NULL;
  uint64_t              response_code   = 0U;

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    free(base_path);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_transaction_to_cbor(tx, writer);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);

    free(base_path);
    return result;
  }

  const size_t cbor_size = cardano_cbor_writer_get_encode_size(writer);
  byte_t*      cbor_data = malloc(cbor_size);

  if (cbor_data == NULL)
  {
    free(base_path);
    cardano_cbor_writer_unref(&writer);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result = cardano_cbor_writer_encode(writer, cbor_data, cbor_size);

  if (result != CARDANO_SUCCESS)
  {
    free(cbor_data);
    free(base_path);
    cardano_cbor_writer_unref(&writer);

    return result;
  }

  const size_t tx_encoded_size = cardano_cbor_writer_get_hex_size(writer);
  char*        tx_encoded      = malloc(tx_encoded_size);

  if (tx_encoded == NULL)
  {
    free(cbor_data);
    free(base_path);
    cardano_cbor_writer_unref(&writer);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result = cardano_cbor_writer_encode_hex(writer, tx_encoded, tx_encoded_size);
  cardano_cbor_writer_unref(&writer);

  if (result != CARDANO_SUCCESS)
  {
    free(tx_encoded);
    free(cbor_data);
    free(base_path);

    return result;
  }

  console_debug("Sending transaction: %s", tx_encoded);
  free(tx_encoded);

  result = cardano_blockfrost_http_post(
    provider_impl,
    base_path,
    cardano_utils_safe_strlen(base_path, 256U),
    cbor_data,
    cbor_size,
    CARDANO_BLOCKFROST_CONTENT_TYPE_CBOR,
    &response_code,
    &response_buffer);

  free(base_path);
  free(cbor_data);

  if ((response_code != 200U) || (result != CARDANO_SUCCESS))
  {
    cardano_blockfrost_parse_error(provider_impl, response_buffer);

    cardano_buffer_unref(&response_buffer);

    return CARDANO_ERROR_INVALID_HTTP_REQUEST;
  }

  cardano_json_object_t* parsed_json = cardano_json_object_parse((char*)cardano_buffer_get_data(response_buffer), (int32_t)cardano_buffer_get_size(response_buffer));

  if (parsed_json == NULL)
  {
    cardano_buffer_unref(&response_buffer);

    cardano_utils_set_error_message(provider_impl, "Failed to parse JSON response");

    return CARDANO_ERROR_INVALID_JSON;
  }

  if (cardano_json_object_get_type(parsed_json) != CARDANO_JSON_OBJECT_TYPE_STRING)
  {
    cardano_buffer_unref(&response_buffer);

    cardano_utils_set_error_message(provider_impl, "Invalid JSON response");

    return CARDANO_ERROR_INVALID_JSON;
  }

  size_t      hex_len    = 0U;
  const char* hex_string = cardano_json_object_get_string(parsed_json, &hex_len);

  result = cardano_blake2b_hash_from_hex(hex_string, hex_len - 1, tx_id);

  cardano_buffer_unref(&response_buffer);

  cardano_json_object_unref(&parsed_json);

  return result;
}

/**
 * \brief Evaluates a transaction using the specified provider and updates redeemer execution units.
 *
 * This function evaluates a transaction in the context of a set of additional UTXOs. It communicates with the provider
 * to evaluate the transaction and updates the redeemer list with execution units based on the result.
 *
 * \param[in] provider_impl A pointer to an initialized \ref cardano_provider_impl_t object representing the provider. This must not be NULL.
 * \param[in] tx A pointer to an initialized \ref cardano_transaction_t object representing the transaction to be evaluated. This must not be NULL.
 * \param[in] additional_utxos A pointer to an initialized \ref cardano_utxo_list_t object representing the additional UTXOs. This parameter can be NULL
 *                             if no additional UTXOs are needed for the evaluation.
 * \param[out] redeemers On success, this will point to a newly created \ref cardano_redeemer_list_t object containing the redeemers
 *                       with updated execution units based on the evaluation result. The caller is responsible for managing the lifecycle
 *                       of this object, including calling \ref cardano_redeemer_list_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the transaction was successfully evaluated,
 *         or an appropriate error code such as \ref CARDANO_POINTER_IS_NULL if any required input pointer is NULL, or a specific error from the provider.
 *
 * \note The caller is responsible for releasing the memory of the \p redeemers object when it is no longer needed.
 */
static cardano_error_t
evaluate_transaction(
  cardano_provider_impl_t*  provider_impl,
  cardano_transaction_t*    tx,
  cardano_utxo_list_t*      additional_utxos,
  cardano_redeemer_list_t** redeemers)
{
  blockfrost_context_t* context         = (blockfrost_context_t*)provider_impl->context;
  char*                 base_path       = cardano_blockfrost_get_endpoint_url(context->network, "/utils/txs/evaluate/utxos");
  cardano_buffer_t*     response_buffer = NULL;
  uint64_t              response_code   = 0;

  char*           json_payload = NULL;
  size_t          json_size    = 0;
  cardano_error_t result       = cardano_evaluate_params_to_json(tx, additional_utxos, &json_payload, &json_size);

  if (result != CARDANO_SUCCESS)
  {
    free(base_path);

    return result;
  }

  result = cardano_blockfrost_http_post(
    provider_impl,
    base_path,
    cardano_utils_safe_strlen(base_path, 256U),
    (byte_t*)json_payload,
    json_size,
    CARDANO_BLOCKFROST_CONTENT_TYPE_JSON,
    &response_code,
    &response_buffer);

  free(base_path);
  free(json_payload);

  if ((response_code != 200U) || (result != CARDANO_SUCCESS))
  {
    cardano_blockfrost_parse_error(provider_impl, response_buffer);

    cardano_buffer_unref(&response_buffer);

    return CARDANO_ERROR_INVALID_HTTP_REQUEST;
  }

  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(tx);
  cardano_witness_set_unref(&witness_set);

  if (witness_set == NULL)
  {
    cardano_buffer_unref(&response_buffer);

    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  cardano_redeemer_list_t* original_redeemers = cardano_witness_set_get_redeemers(witness_set);
  cardano_redeemer_list_unref(&original_redeemers);

  if (original_redeemers == NULL)
  {
    cardano_buffer_unref(&response_buffer);

    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  result = cardano_blockfrost_parse_tx_eval_response(provider_impl, (char*)cardano_buffer_get_data(response_buffer), cardano_buffer_get_size(response_buffer), original_redeemers, redeemers);

  cardano_buffer_unref(&response_buffer);

  return result;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
create_blockfrost_provider(
  const cardano_network_magic_t network,
  const char*                   project_id,
  const size_t                  project_id_size,
  cardano_provider_t**          provider)
{
  cardano_provider_impl_t impl = { 0 };

  CARDANO_UNUSED(snprintf(impl.name, 256, "blockfrost-%s", cardano_network_magic_to_string(network)));

  if ((provider == NULL) || (project_id == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((project_id_size > 64U) || (project_id_size == 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  blockfrost_context_t* context = cardano_blockfrost_context_new();

  if (context == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_utils_safe_memcpy(context->project_id, 64U, project_id, project_id_size);

  context->project_id_size = project_id_size;
  context->network         = network;

  impl.get_parameters                 = get_parameters;
  impl.get_unspent_outputs            = get_unspent_outputs;
  impl.get_rewards_balance            = get_rewards_balance;
  impl.get_unspent_outputs_with_asset = get_unspent_outputs_with_asset;
  impl.get_unspent_output_by_nft      = get_unspent_output_by_nft;
  impl.resolve_unspent_outputs        = resolve_unspent_outputs;
  impl.resolve_datum                  = resolve_datum;
  impl.await_transaction_confirmation = await_transaction_confirmation;
  impl.post_transaction_to_chain      = post_transaction_to_chain;
  impl.evaluate_transaction           = evaluate_transaction;
  impl.network_magic                  = network;

  impl.context = (cardano_object_t*)context;

  return cardano_provider_new(impl, provider);
}
