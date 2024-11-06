/**
 * \file blockfrost_url_builders.c
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

#include "blockfrost_url_builders.h"
#include "blockfrost_common.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

const char*
cardano_blockfrost_get_network_base_url(const cardano_network_magic_t network)
{
  switch (network)
  {
    case CARDANO_NETWORK_MAGIC_MAINNET:
      return "https://cardano-mainnet.blockfrost.io/api/v0/";
    case CARDANO_NETWORK_MAGIC_PREPROD:
      return "https://cardano-preprod.blockfrost.io/api/v0/";
    case CARDANO_NETWORK_MAGIC_PREVIEW:
      return "https://cardano-preview.blockfrost.io/api/v0/";
    case CARDANO_NETWORK_MAGIC_SANCHONET:
      return "https://cardano-sanchonet.blockfrost.io/api/v0/";
    default:
      return NULL;
  }
}

char*
cardano_blockfrost_get_endpoint_url(const cardano_network_magic_t network, const char* endpoint)
{
  const char* base_url = cardano_blockfrost_get_network_base_url(network);

  if (base_url == NULL)
  {
    return NULL;
  }

  const size_t base_url_size = cardano_utils_safe_strlen(base_url, 256U);
  const size_t endpoint_size = cardano_utils_safe_strlen(endpoint, 256U);

  char* url = malloc(256);
  memset(url, 0, 256);

  if (url == NULL)
  {
    return NULL;
  }

  cardano_utils_safe_memcpy(url, base_url_size + endpoint_size + 1U, base_url, base_url_size);
  cardano_utils_safe_memcpy(url + base_url_size, endpoint_size + 1U, endpoint, endpoint_size);
  url[base_url_size + endpoint_size] = '\0';

  return url;
}

char*
cardano_blockfrost_build_utxo_url(
  cardano_provider_impl_t* provider_impl,
  const char*              bech32,
  const size_t             page,
  const size_t             max_results)
{
  blockfrost_context_t* context = (blockfrost_context_t*)provider_impl->context;

  char* base_path = cardano_blockfrost_get_endpoint_url(context->network, "addresses/");

  const size_t base_path_len  = cardano_utils_safe_strlen(base_path, 256U);
  const size_t bech32_len     = cardano_utils_safe_strlen(bech32, 256U);
  const size_t pagination_len = 50U;
  const size_t url_len        = base_path_len + bech32_len + pagination_len + 20U;

  char* url = malloc(256);
  memset(url, 0, 256);

  if (!url)
  {
    free(base_path);

    return NULL;
  }

  CARDANO_UNUSED(snprintf(url, url_len, "%s%s/utxos?count=%zu&page=%zu", base_path, bech32, max_results, page));

  free(base_path);

  return url;
}

char*
cardano_blockfrost_build_utxo_with_asset_url(
  cardano_provider_impl_t* provider_impl,
  const char*              bech32,
  const char*              asset_id,
  const size_t             page,
  const size_t             max_results)
{
  blockfrost_context_t* context = (blockfrost_context_t*)provider_impl->context;

  char* base_path = cardano_blockfrost_get_endpoint_url(context->network, "addresses/");

  const size_t base_path_len  = cardano_utils_safe_strlen(base_path, 256U);
  const size_t bech32_len     = cardano_utils_safe_strlen(bech32, 256U);
  const size_t asset_id_len   = cardano_utils_safe_strlen(asset_id, 256U);
  const size_t pagination_len = 50U;
  const size_t url_len        = base_path_len + bech32_len + pagination_len + asset_id_len + 20U;

  char* url = malloc(256);
  memset(url, 0, 256);

  if (!url)
  {
    free(base_path);

    return NULL;
  }

  CARDANO_UNUSED(snprintf(url, url_len, "%s%s/utxos/%s?count=%zu&page=%zu", base_path, bech32, asset_id, max_results, page));

  free(base_path);

  return url;
}

char*
cardano_blockfrost_build_addresses_with_asset_url(
  cardano_provider_impl_t* provider_impl,
  const char*              asset_id)
{
  blockfrost_context_t* context = (blockfrost_context_t*)provider_impl->context;

  char* base_path = cardano_blockfrost_get_endpoint_url(context->network, "assets/");

  const size_t base_path_len  = cardano_utils_safe_strlen(base_path, 256U);
  const size_t asset_id_len   = cardano_utils_safe_strlen(asset_id, 256U);
  const size_t pagination_len = 50U;
  const size_t url_len        = base_path_len + pagination_len + asset_id_len + 1U;

  char* url = malloc(256);
  memset(url, 0, 256);

  if (!url)
  {
    free(base_path);

    return NULL;
  }

  CARDANO_UNUSED(snprintf(url, url_len, "%s%s/addresses", base_path, asset_id));

  free(base_path);

  return url;
}

char*
cardano_blockfrost_build_transaction_utxos_url(
  cardano_provider_impl_t* provider_impl,
  const char*              tx_id)
{
  blockfrost_context_t* context = (blockfrost_context_t*)provider_impl->context;

  char* base_path = cardano_blockfrost_get_endpoint_url(context->network, "txs/");

  const size_t base_path_len = cardano_utils_safe_strlen(base_path, 256U);
  const size_t tx_id_len     = cardano_utils_safe_strlen(tx_id, 256U);
  const size_t url_len       = base_path_len + tx_id_len + 10U;

  char* url = malloc(256);
  memset(url, 0, 256);

  if (!url)
  {
    free(base_path);

    return NULL;
  }

  CARDANO_UNUSED(snprintf(url, url_len, "%s%s/utxos", base_path, tx_id));

  free(base_path);

  return url;
}

char*
cardano_blockfrost_build_datum_url(
  cardano_provider_impl_t* provider_impl,
  const char*              datum_hash)
{
  blockfrost_context_t* context = (blockfrost_context_t*)provider_impl->context;

  char* base_path = cardano_blockfrost_get_endpoint_url(context->network, "scripts/datum/");

  const size_t base_path_len  = cardano_utils_safe_strlen(base_path, 256U);
  const size_t datum_hash_len = cardano_utils_safe_strlen(datum_hash, 256U);
  const size_t url_len        = base_path_len + datum_hash_len + 6U;

  char* url = malloc(256);
  memset(url, 0, 256);

  if (!url)
  {
    free(base_path);

    return NULL;
  }

  CARDANO_UNUSED(snprintf(url, url_len, "%s%s/cbor", base_path, datum_hash));

  free(base_path);

  return url;
}

char*
cardano_blockfrost_build_rewards_url(
  cardano_provider_impl_t* provider_impl,
  const char*              bech32)
{
  blockfrost_context_t* context = (blockfrost_context_t*)provider_impl->context;

  char* base_path = cardano_blockfrost_get_endpoint_url(context->network, "accounts/");

  const size_t base_path_len = cardano_utils_safe_strlen(base_path, 256U);
  const size_t bech32_len    = cardano_utils_safe_strlen(bech32, 256U);
  ;
  const size_t url_len = base_path_len + bech32_len + 1U;

  char* url = malloc(256);
  memset(url, 0, 256);

  if (!url)
  {
    free(base_path);

    return NULL;
  }

  CARDANO_UNUSED(snprintf(url, url_len, "%s%s", base_path, bech32));

  free(base_path);

  return url;
}

char*
cardano_blockfrost_build_tx_metadata_cbor_url(
  cardano_provider_impl_t* provider_impl,
  const char*              hash)
{
  blockfrost_context_t* context = (blockfrost_context_t*)provider_impl->context;

  char* base_path = cardano_blockfrost_get_endpoint_url(context->network, "txs/");

  const size_t base_path_len = cardano_utils_safe_strlen(base_path, 256U);
  const size_t hash_len      = cardano_utils_safe_strlen(hash, 256U);
  const size_t url_len       = base_path_len + hash_len + 20U;

  char* url = malloc(256);

  if (!url)
  {
    free(base_path);

    return NULL;
  }

  memset(url, 0, 256);

  CARDANO_UNUSED(snprintf(url, url_len, "%s%s/metadata/cbor", base_path, hash));

  free(base_path);

  return url;
}