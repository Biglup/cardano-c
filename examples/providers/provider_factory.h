/**
 * \file provider_factory.h
 *
 * \author angel.castillo
 * \date   Sep 28, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_PROVIDER_FACTORY_H
#define BIGLUP_LABS_INCLUDE_CARDANO_PROVIDER_FACTORY_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>

#include <cardano/common/network_magic.h>
#include <cardano/providers/provider.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Creates a Blockfrost provider for interacting with the Cardano blockchain.
 *
 * This function creates a Blockfrost provider that can be used to communicate with the Cardano blockchain
 * via the Blockfrost API. The provider is initialized with the network magic and a project ID associated
 * with the Blockfrost project.
 *
 * \param[in]  network            The Cardano network magic number (e.g., mainnet or testnet) that the provider will interact with.
 * \param[in]  project_id         A pointer to the Blockfrost project ID string. This is used to authenticate with the Blockfrost API.
 * \param[in]  project_id_size    The size of the `project_id` string in bytes.
 * \param[out] provider           A pointer to a pointer where the newly created Blockfrost provider will be stored.
 *                                The caller is responsible for deallocating the provider using the appropriate function.
 *
 * \return A \ref cardano_error_t indicating the success or failure of the operation.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t create_blockfrost_provider(
  cardano_network_magic_t network,
  const char*             project_id,
  size_t                  project_id_size,
  cardano_provider_t**    provider);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_PROVIDER_FACTORY_H