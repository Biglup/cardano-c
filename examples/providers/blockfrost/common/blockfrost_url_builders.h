/**
 * \file blockfrost_url_builders.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BLOCKFROST_URL_BUILDERS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BLOCKFROST_URL_BUILDERS_H

/* INCLUDES ******************************************************************/

#include <cardano/common/network_magic.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/providers/provider_impl.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Retrieves the base URL for the Blockfrost API corresponding to the specified Cardano network.
 *
 * This function returns the appropriate base URL for the Blockfrost API based on the Cardano network specified
 * by the `network` parameter. It selects the URL for either the mainnet or testnet based on the network magic value.
 *
 * \param[in] network A `cardano_network_magic_t` enum specifying the network type. Typically, mainnet and testnet
 *                    networks have different magic values.
 *
 * \return A pointer to a constant string containing the base URL for the specified network.
 *         The returned string is either the URL for the mainnet or testnet.
 *
 * \note The returned URL is a constant string and should not be modified or freed.
 */
const char*
cardano_blockfrost_get_network_base_url(cardano_network_magic_t network);

/**
 * \brief Constructs the full URL for a given endpoint on the Blockfrost API based on the specified Cardano network.
 *
 * This function combines the base URL for the specified network (mainnet or testnet) with the provided API endpoint.
 * The full URL is dynamically allocated and should be freed by the caller.
 *
 * \param[in] network  A `cardano_network_magic_t` enum specifying the Cardano network (mainnet or testnet).
 * \param[in] endpoint A pointer to a null-terminated string that specifies the API endpoint (e.g., "/blocks/latest").
 *
 * \return A dynamically allocated string containing the full URL for the specified endpoint on the network.
 *         The caller is responsible for freeing the returned string.
 *
 * \note If memory allocation fails, this function returns `NULL`.
 */
char*
cardano_blockfrost_get_endpoint_url(cardano_network_magic_t network, const char* endpoint);

/**
 * \brief Constructs a URL string for retrieving UTXOs for a given address.
 *
 * The function constructs the URL string for a Cardano API request that retrieves the UTXOs
 * associated with a Bech32 address. The URL includes pagination parameters (`count` and `page`).
 *
 * \param[in]  provider_impl Pointer to the provider implementation instance.
 * \param[in]  bech32        Bech32 address for which to retrieve UTXOs.
 * \param[in]  page          The current page of results.
 * \param[in]  max_results   The maximum number of results to return.
 *
 * \return A dynamically allocated string containing the full URL.
 *         The caller is responsible for freeing the returned string.
 */
char*
cardano_blockfrost_build_utxo_url(
  cardano_provider_impl_t* provider_impl,
  const char*              bech32,
  size_t                   page,
  size_t                   max_results);

/**
 * \brief Constructs the URL for retrieving staking rewards from the Blockfrost API for a given Bech32 address.
 *
 * This function constructs a URL used to query the Blockfrost API for the staking rewards associated with the specified Bech32 address.
 *
 * \param[in] provider_impl A pointer to an initialized \ref cardano_provider_impl_t object that provides the necessary context (e.g., API base URL).
 *                          This parameter must not be NULL.
 * \param[in] bech32 A pointer to a null-terminated string containing the Bech32 address for which to retrieve rewards.
 *                   This parameter must not be NULL.
 *
 * \return A dynamically allocated string containing the full URL for querying rewards. The caller is responsible for freeing the returned string
 *         using \c free when it is no longer needed. Returns NULL if the URL could not be constructed (e.g., due to invalid input).
 */
char*
cardano_blockfrost_build_rewards_url(
  cardano_provider_impl_t* provider_impl,
  const char*              bech32);

/**
 * \brief Constructs the URL for retrieving UTXOs associated with a given Bech32 address and asset ID from the Blockfrost API.
 *
 * This function constructs a URL used to query the Blockfrost API for the UTXOs that are associated with the specified Bech32 address and asset ID.
 * The URL will also include pagination parameters, allowing the user to specify the page and the maximum number of results per page.
 *
 * \param[in] provider_impl A pointer to an initialized \ref cardano_provider_impl_t object that provides the necessary context (e.g., API base URL).
 *                          This parameter must not be NULL.
 * \param[in] bech32 A pointer to a null-terminated string containing the Bech32 address for which to retrieve UTXOs. This parameter must not be NULL.
 * \param[in] asset_id A pointer to a null-terminated string containing the asset ID to filter the UTXOs by. This parameter must not be NULL.
 * \param[in] page The page number to retrieve. Used for pagination.
 * \param[in] max_results The maximum number of results to retrieve per page.
 *
 * \return A dynamically allocated string containing the full URL for querying UTXOs with the specified asset. The caller is responsible for freeing the
 *         returned string using \c free when it is no longer needed. Returns NULL if the URL could not be constructed (e.g., due to invalid input).
 */
char*
cardano_blockfrost_build_utxo_with_asset_url(
  cardano_provider_impl_t* provider_impl,
  const char*              bech32,
  const char*              asset_id,
  size_t                   page,
  size_t                   max_results);

/**
 * \brief Constructs the URL for retrieving addresses associated with a given asset ID from the Blockfrost API.
 *
 * This function constructs a URL used to query the Blockfrost API for the addresses that hold the specified asset.
 *
 * \param[in] provider_impl A pointer to an initialized \ref cardano_provider_impl_t object that provides the necessary context (e.g., API base URL).
 *                          This parameter must not be NULL.
 * \param[in] asset_id A pointer to a null-terminated string containing the asset ID to query for addresses. This parameter must not be NULL.
 *
 * \return A dynamically allocated string containing the full URL for querying addresses holding the specified asset. The caller is responsible for freeing the
 *         returned string using \c free when it is no longer needed. Returns NULL if the URL could not be constructed (e.g., due to invalid input).
 */
char*
cardano_blockfrost_build_addresses_with_asset_url(
  cardano_provider_impl_t* provider_impl,
  const char*              asset_id);

/**
 * \brief Constructs a URL for retrieving UTXOs from a transaction.
 *
 * This function constructs the URL required to fetch the UTXOs associated with a given transaction ID.
 * The URL is built based on the Blockfrost API and the network details provided in the `provider_impl`.
 *
 * \param[in] provider_impl A pointer to the provider implementation that contains network information.
 * \param[in] tx_id         A string representing the transaction ID.
 *
 * \return A dynamically allocated string containing the constructed URL, or NULL if memory allocation fails.
 *         The caller is responsible for freeing the memory returned by this function.
 */
char*
cardano_blockfrost_build_transaction_utxos_url(
  cardano_provider_impl_t* provider_impl,
  const char*              tx_id);

/**
 * \brief Constructs a URL to query a datum by its hash from the Blockfrost API.
 *
 * This function constructs a URL string that can be used to retrieve a datum from the Blockfrost API by providing its hash.
 *
 * \param[in] provider_impl A pointer to an initialized \ref cardano_provider_impl_t object that contains the necessary context for building the URL.
 *                          This parameter must not be NULL.
 * \param[in] datum_hash A pointer to a null-terminated string representing the datum hash. This parameter must not be NULL.
 *
 * \return A pointer to a dynamically allocated string containing the URL to query the datum. The caller is responsible for freeing
 *         the returned string using \c free when it is no longer needed. If an error occurs (e.g., invalid parameters), the function returns NULL.
 */
char*
cardano_blockfrost_build_datum_url(
  cardano_provider_impl_t* provider_impl,
  const char*              datum_hash);

/**
 * \brief Constructs a URL to query transaction metadata in CBOR format from the Blockfrost API.
 *
 * This function builds a URL string that can be used to retrieve transaction metadata (in CBOR format) from the Blockfrost API
 * by providing the transaction hash.
 *
 * \param[in] provider_impl A pointer to an initialized \ref cardano_provider_impl_t object that contains the necessary context for building the URL.
 *                          This parameter must not be NULL.
 * \param[in] hash A pointer to a null-terminated string representing the transaction hash. This parameter must not be NULL.
 *
 * \return A pointer to a dynamically allocated string containing the URL to query the transaction metadata in CBOR format.
 *         The caller is responsible for freeing the returned string using \c free when it is no longer needed.
 *         If an error occurs (e.g., invalid parameters), the function returns NULL.
 */
char*
cardano_blockfrost_build_tx_metadata_cbor_url(
  cardano_provider_impl_t* provider_impl,
  const char*              hash);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BLOCKFROST_URL_BUILDERS_H
