/**
 * \file protocol_parameters.h
 *
 * \author angel.castillo
 * \date   Sep 26, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_PROTOCOL_PARAMETERS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_PROTOCOL_PARAMETERS_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/ex_units.h>
#include <cardano/common/protocol_version.h>
#include <cardano/common/unit_interval.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/protocol_params/costmdls.h>
#include <cardano/protocol_params/drep_voting_thresholds.h>
#include <cardano/protocol_params/ex_unit_prices.h>
#include <cardano/protocol_params/pool_voting_thresholds.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Protocol parameters govern various aspects of the Cardano network.
 */
typedef struct cardano_protocol_parameters_t cardano_protocol_parameters_t;

/**
 * \brief Creates and initializes a new instance of the protocol parameters.
 *
 * This function allocates and initializes a new instance of the protocol parameters,
 * representing a set of updates to the Cardano protocol parameters.
 *
 * \param[out] protocol_parameters On successful initialization, this will point to a newly created
 *            protocol parameters object. This object represents a "strong reference",
 *            meaning that it is fully initialized and ready for use.
 *            The caller is responsible for managing the lifecycle of this object.
 *            Specifically, once the protocol parameters is no longer needed, the caller must release it
 *            by calling \ref cardano_protocol_parameters_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the protocol parameters was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_parameters = NULL;
 *
 * // Attempt to create a new protocol parameters object
 * cardano_error_t result = cardano_protocol_parameters_new(&protocol_parameters);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the protocol_parameters
 *
 *   // Once done, ensure to clean up and release the protocol_parameters
 *   cardano_protocol_parameters_unref(&protocol_parameters);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_protocol_parameters_new(cardano_protocol_parameters_t** protocol_parameters);

/**
 * \brief Retrieves the linear minimum fee coefficient (a) from the protocol parameters.
 *
 * This function returns the value of the linear fee coefficient (a) from the given
 * \ref cardano_protocol_parameters_t object. The minimum fee for a transaction is calculated
 * using the formula:
 *
 * \code
 *   min_fee = a * size_of_transaction + b
 * \endcode
 *
 * where \c a is the linear fee coefficient and \c b is the constant fee coefficient.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The value of the linear fee coefficient (a). If the \p protocol_parameters pointer is NULL,
 *         the function will return 0.
 *
 * \note The linear fee coefficient is part of the Cardano network protocol parameters and is used to
 *       calculate the minimum required transaction fees.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t min_fee_a = cardano_protocol_parameters_get_min_fee_a(protocol_params);
 *
 * printf("Linear fee coefficient (a): %llu\n", min_fee_a);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_min_fee_a(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the constant minimum fee coefficient (b) from the protocol parameters.
 *
 * This function returns the value of the constant fee coefficient (b) from the given
 * \ref cardano_protocol_parameters_t object. The minimum fee for a transaction is calculated
 * using the formula:
 *
 * \code
 *   min_fee = a * size_of_transaction + b
 * \endcode
 *
 * where \c a is the linear fee coefficient and \c b is the constant fee coefficient.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The value of the constant fee coefficient (b). If the \p protocol_parameters pointer is NULL,
 *         the function will return 0.
 *
 * \note The constant fee coefficient is part of the Cardano network protocol parameters and is used to
 *       calculate the minimum required transaction fees.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t min_fee_b = cardano_protocol_parameters_get_min_fee_b(protocol_params);
 *
 * printf("Constant fee coefficient (b): %llu\n", min_fee_b);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_min_fee_b(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the maximum block body size from the protocol parameters.
 *
 * This function returns the maximum size, in bytes, allowed for the body of a block in the
 * Cardano blockchain, as specified in the given \ref cardano_protocol_parameters_t object.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The maximum block body size in bytes. If the \p protocol_parameters pointer is NULL,
 *         the function will return 0.
 *
 * \note The block body size is a critical protocol parameter that limits the total size of all
 *       transactions and other data included in a single block on the Cardano blockchain.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t max_block_body_size = cardano_protocol_parameters_get_max_block_body_size(protocol_params);
 *
 * printf("Maximum block body size: %llu bytes\n", max_block_body_size);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_max_block_body_size(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the maximum transaction size from the protocol parameters.
 *
 * This function returns the maximum size, in bytes, allowed for a transaction in the
 * Cardano blockchain, as specified in the given \ref cardano_protocol_parameters_t object.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The maximum transaction size in bytes. If the \p protocol_parameters pointer is NULL,
 *         the function will return 0.
 *
 * \note The maximum transaction size is a protocol parameter that limits the total size
 *       of a transaction, ensuring that it fits within a block.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t max_tx_size = cardano_protocol_parameters_get_max_tx_size(protocol_params);
 *
 * printf("Maximum transaction size: %llu bytes\n", max_tx_size);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_max_tx_size(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the maximum block header size from the protocol parameters.
 *
 * This function returns the maximum size, in bytes, allowed for the header of a block in the
 * Cardano blockchain, as specified in the given \ref cardano_protocol_parameters_t object.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The maximum block header size in bytes. If the \p protocol_parameters pointer is NULL,
 *         the function will return 0.
 *
 * \note The block header contains essential metadata for the block, including the hash of the previous block,
 *       and this limit ensures that the header does not exceed the specified size.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t max_block_header_size = cardano_protocol_parameters_get_max_block_header_size(protocol_params);
 *
 * printf("Maximum block header size: %llu bytes\n", max_block_header_size);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_max_block_header_size(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the key deposit value from the protocol parameters.
 *
 * This function returns the key deposit required for staking key registration, as specified
 * in the given \ref cardano_protocol_parameters_t object. The key deposit is an amount of ADA
 * required to register a staking key on the Cardano blockchain.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The key deposit amount in lovelaces (1 ADA = 1,000,000 lovelaces). If the \p protocol_parameters
 *         pointer is NULL, the function will return 0.
 *
 * \note The key deposit is a refundable deposit required to participate in the staking process.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t key_deposit = cardano_protocol_parameters_get_key_deposit(protocol_params);
 *
 * printf("Key deposit: %llu lovelaces\n", key_deposit);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_key_deposit(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the pool deposit value from the protocol parameters.
 *
 * This function returns the pool deposit required for registering a stake pool, as specified
 * in the given \ref cardano_protocol_parameters_t object. The pool deposit is an amount of ADA
 * required to register a stake pool on the Cardano blockchain.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The pool deposit amount in lovelaces (1 ADA = 1,000,000 lovelaces). If the \p protocol_parameters
 *         pointer is NULL, the function will return 0.
 *
 * \note The pool deposit is a refundable deposit required for stake pool registration.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t pool_deposit = cardano_protocol_parameters_get_pool_deposit(protocol_params);
 *
 * printf("Pool deposit: %llu lovelaces\n", pool_deposit);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_pool_deposit(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the max pool retirement epoch bounds.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The maximum epoch duration in slots. If the \p protocol_parameters pointer is NULL,
 *         the function will return 0.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t max_epoch = cardano_protocol_parameters_get_max_epoch(protocol_params);
 *
 * printf("Maximum epoch duration: %llu slots\n", max_epoch);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_max_epoch(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the desired number of stake pools (n_opt) from the protocol parameters.
 *
 * This function returns the value of \c n_opt, which represents the optimal number of stake pools
 * for the Cardano network, as specified in the given \ref cardano_protocol_parameters_t object.
 * This parameter is used to calculate pool rewards and incentivize decentralization.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The optimal number of stake pools (\c n_opt). If the \p protocol_parameters pointer is NULL,
 *         the function will return 0.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t n_opt = cardano_protocol_parameters_get_n_opt(protocol_params);
 *
 * printf("Optimal number of stake pools (n_opt): %llu\n", n_opt);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_n_opt(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the pool pledge influence factor from the protocol parameters.
 *
 * This function returns the pool pledge influence factor (represented as a unit interval) from the
 * given \ref cardano_protocol_parameters_t object. The pool pledge influence factor determines how
 * much a stake pool's pledge (the amount of ADA the pool operator commits) influences the pool's rewards.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return A pointer to a \ref cardano_unit_interval_t object representing the pool pledge influence factor.
 *         If the \p protocol_parameters pointer is NULL, the function returns NULL.
 *
 * \note The caller is responsible for managing the lifecycle of the returned \ref cardano_unit_interval_t object.
 *       Specifically, the caller must release the object by calling \ref cardano_unit_interval_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_unit_interval_t* pledge_influence = cardano_protocol_parameters_get_pool_pledge_influence(protocol_params);
 *
 * if (pledge_influence != NULL)
 * {
 *   // Use the pool pledge influence factor
 *
 *   // Clean up when done
 *   cardano_unit_interval_unref(&pledge_influence);
 * }
 * else
 * {
 *   printf("Failed to retrieve the pool pledge influence factor.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_unit_interval_t* cardano_protocol_parameters_get_pool_pledge_influence(
  cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the monetary expansion rate from the protocol parameters.
 *
 * This function returns the expansion rate (represented as a unit interval) from the given
 * \ref cardano_protocol_parameters_t object. The expansion rate determines how much of the
 * remaining reserves are added to the total supply of ADA in each epoch.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return A pointer to a \ref cardano_unit_interval_t object representing the expansion rate.
 *         If the \p protocol_parameters pointer is NULL, the function returns NULL.
 *
 * \note The caller is responsible for managing the lifecycle of the returned \ref cardano_unit_interval_t object.
 *       Specifically, the caller must release the object by calling \ref cardano_unit_interval_unref when it is no longer needed.
 *
 * \note The expansion rate affects the rate at which ADA is gradually released from the reserves into circulation.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_unit_interval_t* expansion_rate = cardano_protocol_parameters_get_expansion_rate(protocol_params);
 *
 * if (expansion_rate != NULL)
 * {
 *   // Use the expansion rate as needed
 *
 *   // Clean up when done
 *   cardano_unit_interval_unref(&expansion_rate);
 * }
 * else
 * {
 *   printf("Failed to retrieve the expansion rate.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_unit_interval_t* cardano_protocol_parameters_get_expansion_rate(
  cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the treasury growth rate from the protocol parameters.
 *
 * This function returns the treasury growth rate (represented as a unit interval) from the given
 * \ref cardano_protocol_parameters_t object. The treasury growth rate determines the portion of the
 * total monetary expansion that is allocated to the treasury, which is used to fund community-driven
 * projects and developments.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return A pointer to a \ref cardano_unit_interval_t object representing the treasury growth rate.
 *         If the \p protocol_parameters pointer is NULL, the function returns NULL.
 *
 * \note The caller is responsible for managing the lifecycle of the returned \ref cardano_unit_interval_t object.
 *       Specifically, the caller must release the object by calling \ref cardano_unit_interval_unref when it is no longer needed.
 *
 * \note The treasury growth rate affects how much of the monetary expansion goes into the treasury versus
 *       rewards for staking participants.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_unit_interval_t* treasury_growth_rate = cardano_protocol_parameters_get_treasury_growth_rate(protocol_params);
 *
 * if (treasury_growth_rate != NULL)
 * {
 *   // Use the treasury growth rate as needed
 *
 *   // Clean up when done
 *   cardano_unit_interval_unref(&treasury_growth_rate);
 * }
 * else
 * {
 *   printf("Failed to retrieve the treasury growth rate.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_unit_interval_t* cardano_protocol_parameters_get_treasury_growth_rate(
  cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the decentralization parameter (d) from the protocol parameters.
 *
 * This function returns the decentralization parameter (represented as a unit interval) from the given
 * \ref cardano_protocol_parameters_t object. The decentralization parameter controls the proportion
 * of blocks produced by federated nodes (IOHK, Emurgo, CF) versus community stake pools. A value of 1
 * means full centralization (all blocks produced by federated nodes), and a value of 0 means full decentralization.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return A pointer to a \ref cardano_unit_interval_t object representing the decentralization parameter.
 *         If the \p protocol_parameters pointer is NULL, the function returns NULL.
 *
 * \note The caller is responsible for managing the lifecycle of the returned \ref cardano_unit_interval_t object.
 *       Specifically, the caller must release the object by calling \ref cardano_unit_interval_unref when it is no longer needed.
 *
 * \note The decentralization parameter affects the rate at which block production shifts from federated nodes
 *       to community-operated stake pools.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_unit_interval_t* d_param = cardano_protocol_parameters_get_d(protocol_params);
 *
 * if (d_param != NULL)
 * {
 *   // Use the decentralization parameter as needed
 *
 *   // Clean up when done
 *   cardano_unit_interval_unref(&d_param);
 * }
 * else
 * {
 *   printf("Failed to retrieve the decentralization parameter.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_unit_interval_t* cardano_protocol_parameters_get_d(
  cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the extra entropy value from the protocol parameters.
 *
 * This function returns the extra entropy value from the given \ref cardano_protocol_parameters_t object.
 * Extra entropy is an optional parameter that can be used to introduce additional randomness into
 * the protocol's random number generation process.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return A pointer to a \ref cardano_buffer_t object containing the extra entropy data.
 *         If no extra entropy is set, or if the \p protocol_parameters pointer is NULL, the function returns NULL.
 *
 * \note The caller is responsible for managing the lifecycle of the returned \ref cardano_buffer_t object.
 *       Specifically, the caller must release the object by calling \ref cardano_buffer_unref when it is no longer needed.
 *
 * \note Extra entropy is an optional protocol parameter that may be used for adding randomness to certain processes,
 *       such as leader election, but it is not always set.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_buffer_t* extra_entropy = cardano_protocol_parameters_get_extra_entropy(protocol_params);
 *
 * if (extra_entropy != NULL)
 * {
 *   // Use the extra entropy data
 *
 *   // Clean up when done
 *   cardano_buffer_unref(&extra_entropy);
 * }
 * else
 * {
 *   printf("No extra entropy is set in the protocol parameters.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_buffer_t* cardano_protocol_parameters_get_extra_entropy(
  cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the protocol version from the protocol parameters.
 *
 * This function returns the protocol version from the given \ref cardano_protocol_parameters_t object.
 * The protocol version is used to track updates to the Cardano protocol, which may include changes
 * to consensus rules or transaction formats.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return A pointer to a \ref cardano_protocol_version_t object representing the protocol version.
 *         If the \p protocol_parameters pointer is NULL, the function returns NULL.
 *
 * \note The caller is responsible for managing the lifecycle of the returned \ref cardano_protocol_version_t object.
 *       Specifically, the caller must release the object by calling \ref cardano_protocol_version_unref when it is no longer needed.
 *
 * \note The protocol version reflects the version of the Cardano protocol in use, which can change
 *       over time as improvements or updates are introduced through hard forks or other governance mechanisms.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_protocol_version_t* protocol_version = cardano_protocol_parameters_get_protocol_version(protocol_params);
 *
 * if (protocol_version != NULL)
 * {
 *   // Use the protocol version as needed
 *
 *   // Clean up when done
 *   cardano_protocol_version_unref(&protocol_version);
 * }
 * else
 * {
 *   printf("Failed to retrieve the protocol version.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_protocol_version_t* cardano_protocol_parameters_get_protocol_version(
  cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the minimum pool cost from the protocol parameters.
 *
 * This function returns the minimum pool cost from the given \ref cardano_protocol_parameters_t object.
 * The minimum pool cost represents the minimum fixed fee that a stake pool operator must charge
 * for delegators to participate in the pool.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The minimum pool cost in lovelaces (1 ADA = 1,000,000 lovelaces). If the \p protocol_parameters
 *         pointer is NULL, the function returns 0.
 *
 * \note The minimum pool cost ensures that pool operators charge a base fee for providing services
 *       and helps sustain the operation of smaller pools in the Cardano ecosystem.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t min_pool_cost = cardano_protocol_parameters_get_min_pool_cost(protocol_params);
 *
 * printf("Minimum pool cost: %llu lovelaces\n", min_pool_cost);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_min_pool_cost(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the ADA cost per UTXO byte from the protocol parameters.
 *
 * This function returns the cost, in lovelaces, required per byte of UTXO stored on the blockchain,
 * as specified in the given \ref cardano_protocol_parameters_t object. This cost affects how much ADA
 * is required to store UTXOs, incentivizing efficient use of the blockchain.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The cost in lovelaces per UTXO byte. If the \p protocol_parameters pointer is NULL,
 *         the function will return 0.
 *
 * \note The cost per UTXO byte ensures that there is a fee associated with storing UTXOs on the
 *       blockchain, helping to maintain the economic sustainability of the system.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t ada_per_utxo_byte = cardano_protocol_parameters_get_ada_per_utxo_byte(protocol_params);
 *
 * printf("ADA per UTXO byte: %llu lovelaces\n", ada_per_utxo_byte);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_ada_per_utxo_byte(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the cost models from the protocol parameters.
 *
 * This function returns the cost models used to calculate the execution costs for Plutus smart contracts,
 * as specified in the given \ref cardano_protocol_parameters_t object. Each cost model defines the resource
 * costs (such as CPU and memory) for executing various Plutus operations.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return A pointer to a \ref cardano_costmdls_t object representing the cost models.
 *         If the \p protocol_parameters pointer is NULL, the function will return NULL.
 *
 * \note The caller is responsible for managing the lifecycle of the returned \ref cardano_costmdls_t object.
 *       Specifically, the caller must release the object by calling \ref cardano_costmdls_unref when it is no longer needed.
 *
 * \note The cost models are essential for calculating the fees for Plutus script execution based on the
 *       resource usage specified by the smart contract.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_costmdls_t* cost_models = cardano_protocol_parameters_get_cost_models(protocol_params);
 *
 * if (cost_models != NULL)
 * {
 *   // Use the cost models as needed
 *
 *   // Clean up when done
 *   cardano_costmdls_unref(&cost_models);
 * }
 * else
 * {
 *   printf("Failed to retrieve the cost models.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_costmdls_t* cardano_protocol_parameters_get_cost_models(
  cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the execution unit prices from the protocol parameters.
 *
 * This function returns the execution unit prices for Plutus script execution from the given
 * \ref cardano_protocol_parameters_t object. Execution unit prices define the cost of using
 * resources such as CPU and memory during the execution of Plutus smart contracts.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return A pointer to a \ref cardano_ex_unit_prices_t object representing the execution unit prices.
 *         If the \p protocol_parameters pointer is NULL, the function will return NULL.
 *
 * \note The caller is responsible for managing the lifecycle of the returned \ref cardano_ex_unit_prices_t object.
 *       Specifically, the caller must release the object by calling \ref cardano_ex_unit_prices_unref when it is no longer needed.
 *
 * \note The execution unit prices are essential for calculating transaction fees based on the resource
 *       usage (CPU and memory) of Plutus smart contracts.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_ex_unit_prices_t* execution_costs = cardano_protocol_parameters_get_execution_costs(protocol_params);
 *
 * if (execution_costs != NULL)
 * {
 *   // Use the execution unit prices as needed
 *
 *   // Clean up when done
 *   cardano_ex_unit_prices_unref(&execution_costs);
 * }
 * else
 * {
 *   printf("Failed to retrieve the execution unit prices.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_ex_unit_prices_t* cardano_protocol_parameters_get_execution_costs(
  cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the maximum execution units for a transaction from the protocol parameters.
 *
 * This function returns the maximum execution units (CPU and memory) allowed for a single transaction
 * on the Cardano blockchain, as specified in the given \ref cardano_protocol_parameters_t object.
 * These execution units are used to limit the computational resources consumed by Plutus scripts
 * within a transaction.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return A pointer to a \ref cardano_ex_units_t object representing the maximum execution units for a transaction.
 *         If the \p protocol_parameters pointer is NULL, the function will return NULL.
 *
 * \note The caller is responsible for managing the lifecycle of the returned \ref cardano_ex_units_t object.
 *       Specifically, the caller must release the object by calling \ref cardano_ex_units_unref when it is no longer needed.
 *
 * \note These limits ensure that a transaction does not consume excessive computational resources,
 *       helping maintain the overall efficiency of the network.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_ex_units_t* max_tx_ex_units = cardano_protocol_parameters_get_max_tx_ex_units(protocol_params);
 *
 * if (max_tx_ex_units != NULL)
 * {
 *   // Use the maximum execution units as needed
 *
 *   // Clean up when done
 *   cardano_ex_units_unref(&max_tx_ex_units);
 * }
 * else
 * {
 *   printf("Failed to retrieve the maximum transaction execution units.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_ex_units_t* cardano_protocol_parameters_get_max_tx_ex_units(
  cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the maximum execution units for a block from the protocol parameters.
 *
 * This function returns the maximum execution units (CPU and memory) allowed for a single block
 * on the Cardano blockchain, as specified in the given \ref cardano_protocol_parameters_t object.
 * These execution units are used to limit the computational resources consumed by Plutus scripts
 * within all transactions in a block.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return A pointer to a \ref cardano_ex_units_t object representing the maximum execution units for a block.
 *         If the \p protocol_parameters pointer is NULL, the function will return NULL.
 *
 * \note The caller is responsible for managing the lifecycle of the returned \ref cardano_ex_units_t object.
 *       Specifically, the caller must release the object by calling \ref cardano_ex_units_unref when it is no longer needed.
 *
 * \note These limits ensure that no single block can consume excessive computational resources,
 *       maintaining network performance and security.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_ex_units_t* max_block_ex_units = cardano_protocol_parameters_get_max_block_ex_units(protocol_params);
 *
 * if (max_block_ex_units != NULL)
 * {
 *   // Use the maximum block execution units as needed
 *
 *   // Clean up when done
 *   cardano_ex_units_unref(&max_block_ex_units);
 * }
 * else
 * {
 *   printf("Failed to retrieve the maximum block execution units.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_ex_units_t* cardano_protocol_parameters_get_max_block_ex_units(
  cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the maximum value size from the protocol parameters.
 *
 * This function returns the maximum size (in bytes) allowed for a transaction output's value
 * in the Cardano blockchain, as specified in the given \ref cardano_protocol_parameters_t object.
 * The value size includes the amount of ADA and any additional tokens in the transaction output.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The maximum value size in bytes. If the \p protocol_parameters pointer is NULL, the function returns 0.
 *
 * \note The maximum value size helps ensure that individual transaction outputs do not exceed
 *       the size limits, maintaining efficiency and preventing potential abuse of network resources.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t max_value_size = cardano_protocol_parameters_get_max_value_size(protocol_params);
 *
 * printf("Maximum value size: %llu bytes\n", max_value_size);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_max_value_size(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the collateral percentage required for Plutus scripts from the protocol parameters.
 *
 * This function returns the percentage of the transaction fee that must be provided as collateral
 * for Plutus script execution, as specified in the given \ref cardano_protocol_parameters_t object.
 * Collateral is required to cover the costs if a Plutus script fails to execute.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The collateral percentage as an integer value. If the \p protocol_parameters pointer is NULL, the function returns 0.
 *
 * \note The collateral percentage determines how much ADA must be set aside to cover potential
 *       costs associated with failed Plutus script execution. This amount is based on the total transaction fee.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t collateral_percentage = cardano_protocol_parameters_get_collateral_percentage(protocol_params);
 *
 * printf("Collateral percentage: %llu%%\n", collateral_percentage);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_collateral_percentage(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the maximum number of collateral inputs allowed for a transaction from the protocol parameters.
 *
 * This function returns the maximum number of collateral inputs that can be included in a transaction
 * on the Cardano blockchain, as specified in the given \ref cardano_protocol_parameters_t object.
 * Collateral inputs are required to cover transaction costs in case of failed Plutus script execution.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The maximum number of collateral inputs allowed for a transaction.
 *         If the \p protocol_parameters pointer is NULL, the function returns 0.
 *
 * \note The maximum number of collateral inputs ensures that transactions cannot include an excessive
 *       number of collateral inputs, helping maintain network performance and preventing potential abuse.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t max_collateral_inputs = cardano_protocol_parameters_get_max_collateral_inputs(protocol_params);
 *
 * printf("Maximum collateral inputs: %llu\n", max_collateral_inputs);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_max_collateral_inputs(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the pool voting thresholds from the protocol parameters.
 *
 * This function returns the voting thresholds for stake pools, as specified in the given
 * \ref cardano_protocol_parameters_t object.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return A pointer to a \ref cardano_pool_voting_thresholds_t object representing the pool voting thresholds.
 *         If the \p protocol_parameters pointer is NULL, the function returns NULL.
 *
 * \note The caller is responsible for managing the lifecycle of the returned \ref cardano_pool_voting_thresholds_t object.
 *       Specifically, the caller must release the object by calling \ref cardano_pool_voting_thresholds_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_pool_voting_thresholds_t* voting_thresholds = cardano_protocol_parameters_get_pool_voting_thresholds(protocol_params);
 *
 * if (voting_thresholds != NULL)
 * {
 *   // Use the voting thresholds as needed
 *
 *   // Clean up when done
 *   cardano_pool_voting_thresholds_unref(&voting_thresholds);
 * }
 * else
 * {
 *   printf("Failed to retrieve the pool voting thresholds.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_pool_voting_thresholds_t* cardano_protocol_parameters_get_pool_voting_thresholds(
  cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the DRep voting thresholds from the protocol parameters.
 *
 * This function returns the voting thresholds for decentralized representatives (DReps) as specified
 * in the given \ref cardano_protocol_parameters_t object.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return A pointer to a \ref cardano_drep_voting_thresholds_t object representing the DRep voting thresholds.
 *         If the \p protocol_parameters pointer is NULL, the function returns NULL.
 *
 * \note The caller is responsible for managing the lifecycle of the returned \ref cardano_drep_voting_thresholds_t object.
 *       Specifically, the caller must release the object by calling \ref cardano_drep_voting_thresholds_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = cardano_protocol_parameters_get_drep_voting_thresholds(protocol_params);
 *
 * if (drep_voting_thresholds != NULL)
 * {
 *   // Use the DRep voting thresholds as needed
 *
 *   // Clean up when done
 *   cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * }
 * else
 * {
 *   printf("Failed to retrieve the DRep voting thresholds.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_drep_voting_thresholds_t* cardano_protocol_parameters_get_drep_voting_thresholds(
  cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the minimum committee size from the protocol parameters.
 *
 * This function returns the minimum size of the governance committee as specified
 * in the given \ref cardano_protocol_parameters_t object. The committee size is relevant
 * for governance-related decisions on the Cardano blockchain.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The minimum committee size as a 64-bit unsigned integer.
 *         If the \p protocol_parameters pointer is NULL, the function returns 0.
 *
 * \note The minimum committee size defines the number of members required to form a valid
 *       governance committee for making decisions on protocol changes and other governance activities.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t min_committee_size = cardano_protocol_parameters_get_min_committee_size(protocol_params);
 *
 * printf("Minimum committee size: %llu\n", min_committee_size);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_min_committee_size(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the committee term limit from the protocol parameters.
 *
 * This function returns the maximum number of terms a committee member can serve, as specified
 * in the given \ref cardano_protocol_parameters_t object. The term limit helps define governance
 * rules for the rotation and tenure of committee members on the Cardano blockchain.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The committee term limit as a 64-bit unsigned integer.
 *         If the \p protocol_parameters pointer is NULL, the function returns 0.
 *
 * \note The committee term limit defines how long a member can serve on the governance committee
 *       before requiring rotation or re-election, contributing to governance stability.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t committee_term_limit = cardano_protocol_parameters_get_committee_term_limit(protocol_params);
 *
 * printf("Committee term limit: %llu terms\n", committee_term_limit);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_committee_term_limit(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the governance action validity period from the protocol parameters.
 *
 * This function returns the validity period for governance actions, as specified
 * in the given \ref cardano_protocol_parameters_t object. The validity period determines
 * the length of time a governance action remains valid before expiring.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The governance action validity period as a 64-bit unsigned integer, representing the number of slots.
 *         If the \p protocol_parameters pointer is NULL, the function returns 0.
 *
 * \note The governance action validity period ensures that governance proposals and actions
 *       have a defined lifespan, ensuring timely decision-making and avoiding stale actions.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t validity_period = cardano_protocol_parameters_get_governance_action_validity_period(protocol_params);
 *
 * printf("Governance action validity period: %llu slots\n", validity_period);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_governance_action_validity_period(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the deposit required for submitting a governance action from the protocol parameters.
 *
 * This function returns the deposit amount required to submit a governance action, as specified
 * in the given \ref cardano_protocol_parameters_t object. The deposit is required to discourage
 * spam proposals and to ensure that proposers are committed to the action.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The deposit required for governance actions, in lovelaces (1 ADA = 1,000,000 lovelaces).
 *         If the \p protocol_parameters pointer is NULL, the function returns 0.
 *
 * \note The governance action deposit ensures that there is a cost associated with submitting proposals,
 *       which helps maintain the quality and seriousness of governance actions.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t governance_deposit = cardano_protocol_parameters_get_governance_action_deposit(protocol_params);
 *
 * printf("Governance action deposit: %llu lovelaces\n", governance_deposit);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_governance_action_deposit(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the deposit required for registering as a DRep (Decentralized Representative) from the protocol parameters.
 *
 * This function returns the deposit amount required to register as a DRep, as specified
 * in the given \ref cardano_protocol_parameters_t object. The deposit is required to ensure
 * that DRep candidates are committed to participating in governance.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The DRep registration deposit in lovelaces (1 ADA = 1,000,000 lovelaces).
 *         If the \p protocol_parameters pointer is NULL, the function returns 0.
 *
 * \note The DRep deposit ensures that registering as a Decentralized Representative
 *       carries a cost, preventing spam registrations and ensuring that candidates are committed.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t drep_deposit = cardano_protocol_parameters_get_drep_deposit(protocol_params);
 *
 * printf("DRep registration deposit: %llu lovelaces\n", drep_deposit);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_drep_deposit(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the DRep (Decentralized Representative) inactivity period from the protocol parameters.
 *
 * This function returns the inactivity period for a DRep, as specified in the given
 * \ref cardano_protocol_parameters_t object. The inactivity period defines how long a DRep can remain inactive
 * (i.e., without participating in governance actions) before they are considered inactive.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return The DRep inactivity period as a 64-bit unsigned integer representing the number of slots.
 *         If the \p protocol_parameters pointer is NULL, the function returns 0.
 *
 * \note The DRep inactivity period ensures that DReps remain active in governance decisions.
 *       If a DRep does not participate in governance for this period, they may be removed or flagged as inactive.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t inactivity_period = cardano_protocol_parameters_get_drep_inactivity_period(protocol_params);
 *
 * printf("DRep inactivity period: %llu slots\n", inactivity_period);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_protocol_parameters_get_drep_inactivity_period(
  const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the cost per byte for reference scripts from the protocol parameters.
 *
 * This function returns the cost per byte associated with storing a reference script, as specified
 * in the given \ref cardano_protocol_parameters_t object. Reference scripts allow transactions
 * to use Plutus scripts without including the full script in each transaction, optimizing storage and efficiency.
 *
 * \param[in] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                This parameter must not be NULL.
 *
 * \return A pointer to a \ref cardano_unit_interval_t object representing the cost per byte for reference scripts.
 *         If the \p protocol_parameters pointer is NULL, the function returns NULL.
 *
 * \note The caller is responsible for managing the lifecycle of the returned \ref cardano_unit_interval_t object.
 *       Specifically, the caller must release the object by calling \ref cardano_unit_interval_unref when it is no longer needed.
 *
 * \note The cost per byte is important for calculating the storage fees associated with storing reference scripts
 *       on-chain, ensuring fair pricing for transactions that utilize reference scripts.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_unit_interval_t* script_cost = cardano_protocol_parameters_get_ref_script_cost_per_byte(protocol_params);
 *
 * if (script_cost != NULL)
 * {
 *   // Use the reference script cost per byte as needed
 *
 *   // Clean up when done
 *   cardano_unit_interval_unref(&script_cost);
 * }
 * else
 * {
 *   printf("Failed to retrieve the reference script cost per byte.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_unit_interval_t* cardano_protocol_parameters_get_ref_script_cost_per_byte(
  cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Sets the minimum fee coefficient "A" in the protocol parameters.
 *
 * This function sets the minimum fee coefficient "A" in the given \ref cardano_protocol_parameters_t object.
 * The "A" parameter is part of the fee calculation formula that determines the minimum fee for transactions.
 * The formula is: fee = A * size + B, where "A" is the multiplier for the transaction size and "B" is the fixed constant.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] min_fee_a The new value for the minimum fee coefficient "A".
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the minimum fee coefficient "A" was successfully set, or an appropriate error code if an error occurred.
 *
 * \note The minimum fee coefficient "A" is an important parameter in determining the cost of transactions,
 *       ensuring that larger transactions pay a proportional fee based on their size.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t new_min_fee_a = 44; // New value for the fee coefficient "A"
 *
 * cardano_error_t result = cardano_protocol_parameters_set_min_fee_a(protocol_params, new_min_fee_a);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Minimum fee coefficient 'A' set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set minimum fee coefficient 'A'.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_min_fee_a(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       min_fee_a);

/**
 * \brief Sets the minimum fee constant "B" in the protocol parameters.
 *
 * This function sets the minimum fee constant "B" in the given \ref cardano_protocol_parameters_t object.
 * The "B" parameter is part of the fee calculation formula that determines the minimum fee for transactions.
 * The formula is: fee = A * size + B, where "A" is the multiplier for the transaction size and "B" is the fixed constant.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] min_fee_b The new value for the minimum fee constant "B".
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the minimum fee constant "B" was successfully set, or an appropriate error code if an error occurred.
 *
 * \note The minimum fee constant "B" ensures that every transaction pays a base fee, independent of its size.
 *       This base fee prevents spam and ensures a minimum cost for processing any transaction.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t new_min_fee_b = 155381; // New value for the fee constant "B"
 *
 * cardano_error_t result = cardano_protocol_parameters_set_min_fee_b(protocol_params, new_min_fee_b);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Minimum fee constant 'B' set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set minimum fee constant 'B'.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_min_fee_b(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       min_fee_b);

/**
 * \brief Sets the maximum block body size in the protocol parameters.
 *
 * This function sets the maximum allowable size (in bytes) for a block's body in the given
 * \ref cardano_protocol_parameters_t object. The block body includes the actual transaction data
 * and other relevant information within a block.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] max_block_body_size The new maximum block body size, in bytes.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the maximum block body size was successfully set, or an appropriate error code if an error occurred.
 *
 * \note The maximum block body size defines the upper limit on how large a block's body can be,
 *       ensuring that blocks remain within a manageable size for network transmission and validation.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t new_max_block_body_size = 65536; // New value for the maximum block body size in bytes
 *
 * cardano_error_t result = cardano_protocol_parameters_set_max_block_body_size(protocol_params, new_max_block_body_size);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Maximum block body size set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set maximum block body size.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_max_block_body_size(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       max_block_body_size);

/**
 * \brief Sets the maximum transaction size in the protocol parameters.
 *
 * This function sets the maximum allowable size (in bytes) for a transaction in the given
 * \ref cardano_protocol_parameters_t object. The transaction size includes all the data
 * required to validate and execute the transaction on the Cardano blockchain.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] max_tx_size The new maximum transaction size, in bytes.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the maximum transaction size was successfully set, or an appropriate error code if an error occurred.
 *
 * \note The maximum transaction size ensures that individual transactions do not exceed a specified
 *       size limit, which helps maintain network efficiency and prevents oversized transactions from
 *       overloading the system.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t new_max_tx_size = 16384; // New value for the maximum transaction size in bytes
 *
 * cardano_error_t result = cardano_protocol_parameters_set_max_tx_size(protocol_params, new_max_tx_size);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Maximum transaction size set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set maximum transaction size.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_max_tx_size(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       max_tx_size);

/**
 * \brief Sets the maximum block header size in the protocol parameters.
 *
 * This function sets the maximum allowable size (in bytes) for a block's header in the given
 * \ref cardano_protocol_parameters_t object. The block header contains essential information
 * about the block, such as metadata, the previous block's hash, and other necessary validation data.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] max_block_header_size The new maximum block header size, in bytes.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the maximum block header size was successfully set, or an appropriate error code if an error occurred.
 *
 * \note The maximum block header size defines the upper limit on how large the header of a block can be,
 *       ensuring that headers remain manageable for storage, validation, and transmission across the network.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t new_max_block_header_size = 1100; // New value for the maximum block header size in bytes
 *
 * cardano_error_t result = cardano_protocol_parameters_set_max_block_header_size(protocol_params, new_max_block_header_size);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Maximum block header size set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set maximum block header size.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_max_block_header_size(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       max_block_header_size);

/**
 * \brief Sets the key deposit value in the protocol parameters.
 *
 * This function sets the deposit amount required for registering a new stake key in the given
 * \ref cardano_protocol_parameters_t object. The key deposit helps prevent spam registrations
 * of stake keys by requiring users to deposit a certain amount of ADA when registering a new key.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] key_deposit The new key deposit value, in lovelaces (1 ADA = 1,000,000 lovelaces).
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the key deposit value was successfully set, or an appropriate error code if an error occurred.
 *
 * \note The key deposit helps secure the network by requiring a financial commitment for registering
 *       a stake key, which is returned when the key is deregistered.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t new_key_deposit = 2000000; // New value for the key deposit (2 ADA in lovelaces)
 *
 * cardano_error_t result = cardano_protocol_parameters_set_key_deposit(protocol_params, new_key_deposit);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Key deposit set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set key deposit.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_key_deposit(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       key_deposit);

/**
 * \brief Sets the pool deposit value in the protocol parameters.
 *
 * This function sets the deposit amount required for registering a new stake pool in the given
 * \ref cardano_protocol_parameters_t object. The pool deposit helps ensure that pool operators
 * are committed to maintaining their stake pool by requiring a financial commitment.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] pool_deposit The new pool deposit value, in lovelaces (1 ADA = 1,000,000 lovelaces).
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the pool deposit value was successfully set, or an appropriate error code if an error occurred.
 *
 * \note The pool deposit is returned to the pool operator when the pool is deregistered. It is designed
 *       to prevent spam or low-effort stake pools from being created.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t new_pool_deposit = 500000000; // New value for the pool deposit (500 ADA in lovelaces)
 *
 * cardano_error_t result = cardano_protocol_parameters_set_pool_deposit(protocol_params, new_pool_deposit);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Pool deposit set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set pool deposit.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_pool_deposit(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       pool_deposit);

/**
 * \brief Sets the maximum epoch duration in the protocol parameters.
 *
 * This function sets the maximum number of epochs for which certain operations (such as stake key registrations or pool certificates)
 * remain valid, as specified in the given \ref cardano_protocol_parameters_t object. The maximum epoch defines the lifespan
 * of these operations within the blockchain.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] max_epoch The new maximum epoch value.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the maximum epoch was successfully set, or an appropriate error code if an error occurred.
 *
 * \note Setting a maximum epoch ensures that certain blockchain operations and commitments have a limited duration
 *       and need to be re-registered or updated after the specified period.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t new_max_epoch = 21600; // New value for the maximum epoch
 *
 * cardano_error_t result = cardano_protocol_parameters_set_max_epoch(protocol_params, new_max_epoch);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Maximum epoch set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set maximum epoch.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_max_epoch(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       max_epoch);

/**
 * \brief Sets the optimal number of stake pools (n_opt) in the protocol parameters.
 *
 * This function sets the optimal number of stake pools (n_opt) in the given \ref cardano_protocol_parameters_t object.
 * The n_opt parameter is used to calculate rewards and incentivize the desired level of decentralization within the network.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] n_opt The new value for the optimal number of stake pools.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the optimal number of stake pools was successfully set, or an appropriate error code if an error occurred.
 *
 * \note The n_opt value plays a key role in determining the distribution of staking rewards across pools
 *       and is important for achieving the desired level of decentralization in the network.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t new_n_opt = 500; // New value for the optimal number of stake pools
 *
 * cardano_error_t result = cardano_protocol_parameters_set_n_opt(protocol_params, new_n_opt);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Optimal number of stake pools (n_opt) set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set optimal number of stake pools.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_n_opt(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       n_opt);

/**
 * \brief Sets the pool pledge influence (a0) in the protocol parameters.
 *
 * This function sets the pool pledge influence (a0) in the given \ref cardano_protocol_parameters_t object.
 * The pool pledge influence determines how much a stake pool's pledge (the amount of ADA the pool operator commits)
 * affects the rewards distribution. A higher pledge results in a higher ranking for the stake pool, which influences
 * the rewards the pool receives.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] pool_pledge_influence A pointer to an initialized \ref cardano_unit_interval_t object representing the
 *                                  new pool pledge influence. This value must be between 0 and 1 (inclusive).
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the pool pledge influence was successfully set, or an appropriate error code if an error occurred.
 *
 * \note The caller is responsible for managing the lifecycle of the \ref cardano_unit_interval_t object. The object must be
 *       released by calling \ref cardano_unit_interval_unref when it is no longer needed.
 *
 * \note The pool pledge influence is crucial in encouraging stake pool operators to commit more ADA to their pool,
 *       helping to secure the network.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_unit_interval_t* new_pledge_influence = ...; // Assume new_pledge_influence is initialized
 *
 * cardano_error_t result = cardano_protocol_parameters_set_pool_pledge_influence(protocol_params, new_pledge_influence);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Pool pledge influence set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set pool pledge influence.\n");
 * }
 *
 * // Clean up when done
 * cardano_unit_interval_unref(&new_pledge_influence);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_pool_pledge_influence(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_unit_interval_t*       pool_pledge_influence);

/**
 * \brief Sets the expansion rate in the protocol parameters.
 *
 * This function sets the expansion rate in the given \ref cardano_protocol_parameters_t object.
 * The expansion rate controls how much of the rewards are taken from reserves and added to the total rewards
 * distributed to stakers. It is expressed as a unit interval between 0 and 1, where 1 means the entire reward comes from reserves.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] expansion_rate A pointer to an initialized \ref cardano_unit_interval_t object representing
 *                           the new expansion rate. This value must be between 0 and 1 (inclusive).
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the expansion rate was successfully set, or an appropriate error code if an error occurred.
 *
 * \note The caller is responsible for managing the lifecycle of the \ref cardano_unit_interval_t object. The object must be
 *       released by calling \ref cardano_unit_interval_unref when it is no longer needed.
 *
 * \note The expansion rate defines the percentage of new ADA minted in each epoch and controls how quickly the total
 *       supply of ADA increases over time.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_unit_interval_t* new_expansion_rate = ...; // Assume new_expansion_rate is initialized
 *
 * cardano_error_t result = cardano_protocol_parameters_set_expansion_rate(protocol_params, new_expansion_rate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Expansion rate set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set expansion rate.\n");
 * }
 *
 * // Clean up when done
 * cardano_unit_interval_unref(&new_expansion_rate);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_expansion_rate(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_unit_interval_t*       expansion_rate);

/**
 * \brief Sets the treasury growth rate in the protocol parameters.
 *
 * This function sets the treasury growth rate in the given \ref cardano_protocol_parameters_t object.
 * The treasury growth rate determines the proportion of rewards that are allocated to the treasury,
 * which funds governance and development activities. It is expressed as a unit interval between 0 and 1,
 * where 1 means all rewards go to the treasury.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] treasury_growth_rate A pointer to an initialized \ref cardano_unit_interval_t object representing
 *                                 the new treasury growth rate. This value must be between 0 and 1 (inclusive).
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the treasury growth rate was successfully set, or an appropriate error code if an error occurred.
 *
 * \note The caller is responsible for managing the lifecycle of the \ref cardano_unit_interval_t object. The object must be
 *       released by calling \ref cardano_unit_interval_unref when it is no longer needed.
 *
 * \note The treasury growth rate defines how much of the staking rewards are redirected to the treasury to fund
 *       community-driven projects and governance.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_unit_interval_t* new_treasury_growth_rate = ...; // Assume new_treasury_growth_rate is initialized
 *
 * cardano_error_t result = cardano_protocol_parameters_set_treasury_growth_rate(protocol_params, new_treasury_growth_rate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Treasury growth rate set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set treasury growth rate.\n");
 * }
 *
 * // Clean up when done
 * cardano_unit_interval_unref(&new_treasury_growth_rate);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_treasury_growth_rate(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_unit_interval_t*       treasury_growth_rate);

/**
 * \brief Sets the decentralization parameter (d) in the protocol parameters.
 *
 * This function sets the decentralization parameter (d) in the given \ref cardano_protocol_parameters_t object.
 * The decentralization parameter controls the extent to which the network is decentralized. As the value of `d`
 * approaches 0, the network becomes more decentralized, with more blocks being produced by the community rather than by centralized entities.
 * The value is expressed as a unit interval between 0 and 1, where 1 indicates full centralization and 0 represents full decentralization.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] d A pointer to an initialized \ref cardano_unit_interval_t object representing the new decentralization parameter (d).
 *              This value must be between 0 and 1 (inclusive).
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the decentralization parameter was successfully set, or an appropriate error code if an error occurred.
 *
 * \note The caller is responsible for managing the lifecycle of the \ref cardano_unit_interval_t object. The object must be
 *       released by calling \ref cardano_unit_interval_unref when it is no longer needed.
 *
 * \note The decentralization parameter (d) defines how decentralized the network is, influencing the control over block production.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_unit_interval_t* new_d = ...; // Assume new_d is initialized
 *
 * cardano_error_t result = cardano_protocol_parameters_set_d(protocol_params, new_d);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Decentralization parameter set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set decentralization parameter.\n");
 * }
 *
 * // Clean up when done
 * cardano_unit_interval_unref(&new_d);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_d(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_unit_interval_t*       d);

/**
 * \brief Sets the extra entropy value in the protocol parameters.
 *
 * This function sets the extra entropy in the given \ref cardano_protocol_parameters_t object.
 * Extra entropy allows for additional randomness to be injected into the protocol, contributing to the randomness used
 * in leader election and other protocol processes.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] extra_entropy A pointer to an initialized \ref cardano_buffer_t object representing the new extra entropy.
 *                          This can be used to provide additional randomness for protocol operations. Passing NULL will unset the extra entropy.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the extra entropy was successfully set, or an appropriate error code if an error occurred.
 *
 * \note The caller is responsible for managing the lifecycle of the \ref cardano_buffer_t object. The object must be
 *       released by calling \ref cardano_buffer_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_buffer_t* new_extra_entropy = ...; // Assume new_extra_entropy is initialized
 *
 * cardano_error_t result = cardano_protocol_parameters_set_extra_entropy(protocol_params, new_extra_entropy);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Extra entropy set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set extra entropy.\n");
 * }
 *
 * // Clean up when done
 * cardano_buffer_unref(&new_extra_entropy);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_extra_entropy(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_buffer_t*              extra_entropy);

/**
 * \brief Sets the protocol version in the protocol parameters.
 *
 * This function sets the protocol version in the given \ref cardano_protocol_parameters_t object.
 * The protocol version indicates the current version of the Cardano protocol being used, and it can be updated as part of
 * governance decisions and protocol upgrades.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] protocol_version A pointer to an initialized \ref cardano_protocol_version_t object representing
 *                             the new protocol version.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the protocol version was successfully set, or an appropriate error code if an error occurred.
 *
 * \note The caller is responsible for managing the lifecycle of the \ref cardano_protocol_version_t object. The object must be
 *       released by calling \ref cardano_protocol_version_unref when it is no longer needed.
 *
 * \note The protocol version is critical to ensuring network consensus and smooth upgrades, allowing nodes to recognize and adapt
 *       to changes in protocol rules.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_protocol_version_t* new_protocol_version = ...; // Assume new_protocol_version is initialized
 *
 * cardano_error_t result = cardano_protocol_parameters_set_protocol_version(protocol_params, new_protocol_version);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Protocol version set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set protocol version.\n");
 * }
 *
 * // Clean up when done
 * cardano_protocol_version_unref(&new_protocol_version);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_protocol_version(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_protocol_version_t*    protocol_version);

/**
 * \brief Sets the minimum pool cost in the protocol parameters.
 *
 * This function sets the minimum pool cost in the given \ref cardano_protocol_parameters_t object.
 * The minimum pool cost defines the least amount of ADA that a stake pool can charge as fees in order to cover its operational costs.
 * This ensures that pool operators are incentivized to maintain their pools and remain competitive in the network.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] min_pool_cost The new minimum pool cost value, in lovelaces (1 ADA = 1,000,000 lovelaces).
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the minimum pool cost was successfully set, or an appropriate error code if an error occurred.
 *
 * \note Setting an appropriate minimum pool cost helps prevent spam or low-effort stake pools from undercharging
 *       and destabilizing the network.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t new_min_pool_cost = 340000000; // New value for the minimum pool cost (340 ADA in lovelaces)
 *
 * cardano_error_t result = cardano_protocol_parameters_set_min_pool_cost(protocol_params, new_min_pool_cost);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Minimum pool cost set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set minimum pool cost.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_min_pool_cost(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       min_pool_cost);

/**
 * \brief Sets the ADA per UTXO byte in the protocol parameters.
 *
 * This function sets the cost, in ADA, per byte of UTXO storage in the given \ref cardano_protocol_parameters_t object.
 * The ADA per UTXO byte parameter defines how much ADA is required to store each byte of a UTXO (Unspent Transaction Output).
 * This helps ensure that the blockchain remains efficient by incentivizing smaller UTXO sizes and discouraging excessive on-chain storage.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] ada_per_utxo_byte The new ADA per UTXO byte value, in lovelaces (1 ADA = 1,000,000 lovelaces).
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the ADA per UTXO byte value was successfully set, or an appropriate error code if an error occurred.
 *
 * \note This value helps maintain efficient storage usage on the blockchain by charging for UTXO space.
 *       A higher value may encourage smaller, more efficient transactions.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t new_ada_per_utxo_byte = 4310; // New value for ADA per UTXO byte (in lovelaces)
 *
 * cardano_error_t result = cardano_protocol_parameters_set_ada_per_utxo_byte(protocol_params, new_ada_per_utxo_byte);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("ADA per UTXO byte set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set ADA per UTXO byte.\n");
 * }
 * \endcode
 */

CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_ada_per_utxo_byte(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       ada_per_utxo_byte);

/**
 * \brief Sets the cost models in the protocol parameters.
 *
 * This function sets the cost models in the given \ref cardano_protocol_parameters_t object.
 * Cost models define the computational costs associated with running Plutus scripts on the blockchain.
 * Each cost model specifies how much computational resources (in terms of CPU and memory) different operations within
 * a script will consume, and this is used to determine the fees that will be charged for executing those scripts.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] cost_models A pointer to an initialized \ref cardano_costmdls_t object representing the new cost models.
 *                        These cost models will replace any previously set models in the protocol parameters.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the cost models were successfully set, or an appropriate error code if an error occurred.
 *
 * \note The caller is responsible for managing the lifecycle of the \ref cardano_costmdls_t object. The object must be
 *       released by calling \ref cardano_costmdls_unref when it is no longer needed.
 *
 * \note Setting accurate cost models ensures that transaction fees fairly reflect the computational resources consumed by Plutus scripts.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_costmdls_t* new_cost_models = ...; // Assume new_cost_models is initialized
 *
 * cardano_error_t result = cardano_protocol_parameters_set_cost_models(protocol_params, new_cost_models);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Cost models set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set cost models.\n");
 * }
 *
 * // Clean up when done
 * cardano_costmdls_unref(&new_cost_models);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_cost_models(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_costmdls_t*            cost_models);

/**
 * \brief Sets the execution costs in the protocol parameters.
 *
 * This function sets the execution costs (or ex-unit prices) in the given \ref cardano_protocol_parameters_t object.
 * Execution costs define the fees for running Plutus smart contracts, represented as prices for each execution unit (ex-unit) of CPU and memory.
 * These prices determine how much a transaction that includes Plutus scripts will need to pay for using computational resources on the blockchain.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] execution_costs A pointer to an initialized \ref cardano_ex_unit_prices_t object representing the new execution costs.
 *                            These costs will replace any previously set execution costs in the protocol parameters.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the execution costs were successfully set, or an appropriate error code if an error occurred.
 *
 * \note The caller is responsible for managing the lifecycle of the \ref cardano_ex_unit_prices_t object. The object must be
 *       released by calling \ref cardano_ex_unit_prices_unref when it is no longer needed.
 *
 * \note Setting appropriate execution costs helps ensure fair pricing for computational resources when running Plutus scripts.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_ex_unit_prices_t* new_execution_costs = ...; // Assume new_execution_costs is initialized
 *
 * cardano_error_t result = cardano_protocol_parameters_set_execution_costs(protocol_params, new_execution_costs);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Execution costs set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set execution costs.\n");
 * }
 *
 * // Clean up when done
 * cardano_ex_unit_prices_unref(&new_execution_costs);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_execution_costs(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_ex_unit_prices_t*      execution_costs);

/**
 * \brief Sets the maximum execution units per transaction in the protocol parameters.
 *
 * This function sets the maximum execution units that can be consumed by a single transaction in the given
 * \ref cardano_protocol_parameters_t object. Execution units are the computational resources (e.g., CPU and memory)
 * required to run Plutus scripts. By setting a limit on the maximum execution units per transaction, the protocol ensures
 * that no transaction can monopolize blockchain resources.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] max_tx_ex_units A pointer to an initialized \ref cardano_ex_units_t object representing the maximum execution units for a transaction.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the maximum transaction execution units were successfully set, or an appropriate error code if an error occurred.
 *
 * \note The caller is responsible for managing the lifecycle of the \ref cardano_ex_units_t object. The object must be
 *       released by calling \ref cardano_ex_units_unref when it is no longer needed.
 *
 * \note This setting helps control resource usage and prevent excessive computational demand by any single transaction.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_ex_units_t* new_max_tx_ex_units = ...; // Assume new_max_tx_ex_units is initialized
 *
 * cardano_error_t result = cardano_protocol_parameters_set_max_tx_ex_units(protocol_params, new_max_tx_ex_units);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Max transaction execution units set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set max transaction execution units.\n");
 * }
 *
 * // Clean up when done
 * cardano_ex_units_unref(&new_max_tx_ex_units);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_max_tx_ex_units(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_ex_units_t*            max_tx_ex_units);

/**
 * \brief Sets the maximum execution units per block in the protocol parameters.
 *
 * This function sets the maximum execution units that can be consumed by all transactions in a single block,
 * in the given \ref cardano_protocol_parameters_t object. Execution units represent computational resources
 * (e.g., CPU and memory) required to run Plutus scripts. Setting a limit on the maximum execution units per block
 * ensures that the blockchain remains performant and that no single block consumes excessive resources.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] max_block_ex_units A pointer to an initialized \ref cardano_ex_units_t object representing the maximum
 *                               execution units allowed per block.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the maximum block execution units were successfully set, or an appropriate error code if an error occurred.
 *
 * \note The caller is responsible for managing the lifecycle of the \ref cardano_ex_units_t object. The object must be
 *       released by calling \ref cardano_ex_units_unref when it is no longer needed.
 *
 * \note Limiting execution units per block helps prevent any single block from consuming too much computational capacity,
 *       ensuring that the blockchain remains balanced in its resource use.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_ex_units_t* new_max_block_ex_units = ...; // Assume new_max_block_ex_units is initialized
 *
 * cardano_error_t result = cardano_protocol_parameters_set_max_block_ex_units(protocol_params, new_max_block_ex_units);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Max block execution units set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set max block execution units.\n");
 * }
 *
 * // Clean up when done
 * cardano_ex_units_unref(&new_max_block_ex_units);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_max_block_ex_units(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_ex_units_t*            max_block_ex_units);

/**
 * \brief Sets the maximum size of a value in the protocol parameters.
 *
 * This function sets the maximum size, in bytes, of a value (such as assets or tokens) that can be included in a transaction output
 * in the given \ref cardano_protocol_parameters_t object. Limiting the size of values helps prevent excessive data from being stored
 * in a single transaction, promoting efficiency and scalability on the blockchain.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] max_value_size The new maximum size of a value, in bytes. This is an `int64_t` type.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the maximum value size was successfully set, or an appropriate error code if an error occurred.
 *
 * \note Setting a reasonable maximum value size ensures that no single transaction carries excessive data, helping the network
 *       process transactions efficiently.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * int64_t new_max_value_size = 4000; // Set the maximum value size to 4000 bytes
 *
 * cardano_error_t result = cardano_protocol_parameters_set_max_value_size(protocol_params, new_max_value_size);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Max value size set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set max value size.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_max_value_size(
  cardano_protocol_parameters_t* protocol_parameters,
  int64_t                        max_value_size);

/**
 * \brief Sets the collateral percentage required for Plutus script transactions in the protocol parameters.
 *
 * This function sets the percentage of the transaction fee that must be provided as collateral in case the execution of
 * a Plutus script fails. The collateral is used to cover the fees for the failed transaction, and setting a required percentage
 * helps ensure the network remains secure and compensated for failed script executions.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] collateral_percentage The new collateral percentage, as a uint64_t. This percentage is relative to the transaction fees.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the collateral percentage was successfully set, or an appropriate error code if an error occurred.
 *
 * \note Setting an appropriate collateral percentage ensures that script failures are adequately compensated without affecting
 *       the network's integrity or transaction throughput.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t new_collateral_percentage = 150; // Set the collateral percentage to 150%
 *
 * cardano_error_t result = cardano_protocol_parameters_set_collateral_percentage(protocol_params, new_collateral_percentage);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Collateral percentage set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set collateral percentage.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_collateral_percentage(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       collateral_percentage);

/**
 * \brief Sets the maximum number of collateral inputs allowed in a Plutus script transaction.
 *
 * This function sets the maximum number of collateral inputs that can be provided in a single transaction involving Plutus scripts.
 * Collateral inputs are required when executing Plutus scripts to cover potential costs in case the script fails.
 * By limiting the number of collateral inputs, the protocol ensures that transactions remain efficient and do not consume excessive resources.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] max_collateral_inputs The new maximum number of collateral inputs allowed in a transaction.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the maximum collateral inputs were successfully set, or an appropriate error code if an error occurred.
 *
 * \note Setting a maximum number of collateral inputs ensures that transactions using Plutus scripts remain within acceptable resource limits.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t new_max_collateral_inputs = 3; // Set the maximum collateral inputs to 3
 *
 * cardano_error_t result = cardano_protocol_parameters_set_max_collateral_inputs(protocol_params, new_max_collateral_inputs);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Max collateral inputs set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set max collateral inputs.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_max_collateral_inputs(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       max_collateral_inputs);

/**
 * \brief Sets the pool voting thresholds in the protocol parameters.
 *
 * This function sets the pool voting thresholds in the given \ref cardano_protocol_parameters_t object.
 * Pool voting thresholds define the minimum stake pool support required for certain governance actions, ensuring that
 * changes to the protocol or other governance decisions have sufficient backing from stake pools before being accepted.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] pool_voting_thresholds A pointer to an initialized \ref cardano_pool_voting_thresholds_t object representing the new pool voting thresholds.
 *                                   This value will replace any existing pool voting thresholds in the protocol parameters.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the pool voting thresholds were successfully set, or an appropriate error code if an error occurred.
 *
 * \note The caller is responsible for managing the lifecycle of the \ref cardano_pool_voting_thresholds_t object. The object must be
 *       released by calling \ref cardano_pool_voting_thresholds_unref when it is no longer needed.
 *
 * \note Setting proper voting thresholds ensures that governance actions are backed by sufficient stake, maintaining the security and fairness of the system.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_pool_voting_thresholds_t* new_pool_voting_thresholds = ...; // Assume new_pool_voting_thresholds is initialized
 *
 * cardano_error_t result = cardano_protocol_parameters_set_pool_voting_thresholds(protocol_params, new_pool_voting_thresholds);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Pool voting thresholds set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set pool voting thresholds.\n");
 * }
 *
 * // Clean up when done
 * cardano_pool_voting_thresholds_unref(&new_pool_voting_thresholds);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_pool_voting_thresholds(
  cardano_protocol_parameters_t*    protocol_parameters,
  cardano_pool_voting_thresholds_t* pool_voting_thresholds);

/**
 * \brief Sets the DRep voting thresholds in the protocol parameters.
 *
 * This function sets the decentralized representative (DRep) voting thresholds in the given \ref cardano_protocol_parameters_t object.
 * DRep voting thresholds define the minimum level of support required from decentralized representatives for specific governance actions
 * to be accepted. These thresholds ensure that governance decisions are made with sufficient backing from the DReps.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] drep_voting_thresholds A pointer to an initialized \ref cardano_drep_voting_thresholds_t object representing the new DRep voting thresholds.
 *                                   This value will replace any existing DRep voting thresholds in the protocol parameters.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the DRep voting thresholds were successfully set, or an appropriate error code if an error occurred.
 *
 * \note The caller is responsible for managing the lifecycle of the \ref cardano_drep_voting_thresholds_t object. The object must be
 *       released by calling \ref cardano_drep_voting_thresholds_unref when it is no longer needed.
 *
 * \note Setting appropriate DRep voting thresholds ensures that governance decisions are backed by sufficient support from decentralized representatives,
 *       promoting the fairness and security of the governance process.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_drep_voting_thresholds_t* new_drep_voting_thresholds = ...; // Assume new_drep_voting_thresholds is initialized
 *
 * cardano_error_t result = cardano_protocol_parameters_set_drep_voting_thresholds(protocol_params, new_drep_voting_thresholds);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("DRep voting thresholds set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set DRep voting thresholds.\n");
 * }
 *
 * // Clean up when done
 * cardano_drep_voting_thresholds_unref(&new_drep_voting_thresholds);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_drep_voting_thresholds(
  cardano_protocol_parameters_t*    protocol_parameters,
  cardano_drep_voting_thresholds_t* drep_voting_thresholds);

/**
 * \brief Sets the minimum committee size in the protocol parameters.
 *
 * This function sets the minimum number of members required in a committee for governance actions in the
 * \ref cardano_protocol_parameters_t object. Committees are responsible for making important governance decisions,
 * and defining a minimum size ensures that decisions are made with enough participants, promoting fairness and representation.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] min_committee_size The minimum number of members required in a governance committee.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the minimum committee size was successfully set, or an appropriate error code if an error occurred.
 *
 * \note Setting a minimum committee size ensures that governance decisions are made with sufficient representation,
 *       preventing decisions from being made by too small a group.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t new_min_committee_size = 5; // Set the minimum committee size to 5 members
 *
 * cardano_error_t result = cardano_protocol_parameters_set_min_committee_size(protocol_params, new_min_committee_size);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Minimum committee size set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set minimum committee size.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_min_committee_size(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       min_committee_size);

/**
 * \brief Sets the committee term limit in the protocol parameters.
 *
 * This function sets the maximum term limit for committee members in the given \ref cardano_protocol_parameters_t object.
 * The term limit defines how long a committee member can serve before they are required to step down or be re-elected.
 * Setting a term limit helps ensure regular turnover and prevents stagnation in governance roles.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] committee_term_limit The maximum term limit, in epochs, for a committee member.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the committee term limit was successfully set, or an appropriate error code if an error occurred.
 *
 * \note Setting a term limit ensures that committee members serve for a limited period, encouraging fresh perspectives in governance.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t new_term_limit = 10; // Set the term limit to 10 epochs
 *
 * cardano_error_t result = cardano_protocol_parameters_set_committee_term_limit(protocol_params, new_term_limit);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Committee term limit set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set committee term limit.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_committee_term_limit(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       committee_term_limit);

/**
 * \brief Sets the governance action validity period in the protocol parameters.
 *
 * This function sets the validity period for governance actions in the given \ref cardano_protocol_parameters_t object.
 * The governance action validity period specifies the number of epochs during which a governance action is considered valid.
 * After this period, the action will expire if not executed. This ensures that governance actions are timely and relevant.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] governance_action_validity_period The validity period, in epochs, for a governance action.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the governance action validity period was successfully set, or an appropriate error code if an error occurred.
 *
 * \note Setting a reasonable validity period ensures that governance actions are handled in a timely manner, preventing outdated decisions.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t new_validity_period = 5; // Set the governance action validity period to 5 epochs
 *
 * cardano_error_t result = cardano_protocol_parameters_set_governance_action_validity_period(protocol_params, new_validity_period);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Governance action validity period set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set governance action validity period.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_governance_action_validity_period(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       governance_action_validity_period);

/**
 * \brief Sets the governance action deposit amount in the protocol parameters.
 *
 * This function sets the deposit required for submitting a governance action in the given \ref cardano_protocol_parameters_t object.
 * The deposit serves as collateral to discourage spam submissions and ensure only serious proposals are submitted for governance consideration.
 * If the governance action is successful, the deposit may be refunded; otherwise, it can be forfeited.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] governance_action_deposit The deposit amount, in lovelaces, required to submit a governance action.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the governance action deposit was successfully set, or an appropriate error code if an error occurred.
 *
 * \note Setting an appropriate deposit amount discourages frivolous or spam submissions while promoting meaningful governance participation.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t new_deposit_amount = 5000000; // Set the governance action deposit to 5 ADA (5,000,000 lovelaces)
 *
 * cardano_error_t result = cardano_protocol_parameters_set_governance_action_deposit(protocol_params, new_deposit_amount);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Governance action deposit set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set governance action deposit.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_governance_action_deposit(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       governance_action_deposit);

/**
 * \brief Sets the DRep deposit amount in the protocol parameters.
 *
 * This function sets the deposit required for registering a decentralized representative (DRep) in the given \ref cardano_protocol_parameters_t object.
 * The DRep deposit is a collateral mechanism to ensure that only serious participants register as decentralized representatives,
 * contributing to the governance process. If the registration is successful, the deposit may be refunded; otherwise, it may be forfeited.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] drep_deposit The deposit amount, in lovelaces, required to register as a decentralized representative (DRep).
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the DRep deposit was successfully set, or an appropriate error code if an error occurred.
 *
 * \note Setting a reasonable DRep deposit ensures that only serious candidates participate in governance, helping maintain the integrity of the system.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t new_drep_deposit = 2000000; // Set the DRep deposit to 2 ADA (2,000,000 lovelaces)
 *
 * cardano_error_t result = cardano_protocol_parameters_set_drep_deposit(protocol_params, new_drep_deposit);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("DRep deposit set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set DRep deposit.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_drep_deposit(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       drep_deposit);

/**
 * \brief Sets the DRep inactivity period in the protocol parameters.
 *
 * This function sets the inactivity period for decentralized representatives (DReps) in the given \ref cardano_protocol_parameters_t object.
 * The inactivity period defines the number of epochs a DRep can remain inactive (i.e., not participate in governance votes)
 * before their registration is considered invalid or they are required to take action to remain active.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] drep_inactivity_period The maximum number of epochs a DRep can remain inactive before their registration is affected.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the DRep inactivity period was successfully set, or an appropriate error code if an error occurred.
 *
 * \note Setting a reasonable inactivity period ensures that DReps actively participate in governance processes, maintaining the health of the system.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * uint64_t new_inactivity_period = 100; // Set the DRep inactivity period to 100 epochs
 *
 * cardano_error_t result = cardano_protocol_parameters_set_drep_inactivity_period(protocol_params, new_inactivity_period);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("DRep inactivity period set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set DRep inactivity period.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_drep_inactivity_period(
  cardano_protocol_parameters_t* protocol_parameters,
  uint64_t                       drep_inactivity_period);

/**
 * \brief Sets the reference script cost per byte in the protocol parameters.
 *
 * This function sets the cost per byte for storing reference scripts in the given \ref cardano_protocol_parameters_t object.
 * Reference scripts are stored on-chain, and this parameter determines the cost associated with each byte of a reference script,
 * ensuring that resources are properly accounted for in transaction fees.
 *
 * \param[in,out] protocol_parameters A pointer to an initialized \ref cardano_protocol_parameters_t object.
 *                                    This parameter must not be NULL.
 * \param[in] ref_script_cost_per_byte A pointer to an initialized \ref cardano_unit_interval_t object representing the cost per byte of reference scripts.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the reference script cost per byte was successfully set, or an appropriate error code if an error occurred.
 *
 * \note The caller is responsible for managing the lifecycle of the \ref cardano_unit_interval_t object. The object must be
 *       released by calling \ref cardano_unit_interval_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_params = ...; // Assume protocol_params is initialized
 * cardano_unit_interval_t* ref_script_cost = ...; // Assume ref_script_cost is initialized
 *
 * cardano_error_t result = cardano_protocol_parameters_set_ref_script_cost_per_byte(protocol_params, ref_script_cost);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Reference script cost per byte set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set reference script cost per byte.\n");
 * }
 *
 * // Clean up the ref_script_cost when no longer needed
 * cardano_unit_interval_unref(&ref_script_cost);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_parameters_set_ref_script_cost_per_byte(
  cardano_protocol_parameters_t* protocol_parameters,
  cardano_unit_interval_t*       ref_script_cost_per_byte);

/**
 * \brief Decrements the reference count of a cardano_protocol_parameters_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_protocol_parameters_t object
 * by decreasing its reference count. When the reference count reaches zero, the protocol_parameters is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] protocol_parameters A pointer to the pointer of the protocol_parameters object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_parameters_t* protocol_parameters = cardano_protocol_parameters_new(major, minor);
 *
 * // Perform operations with the protocol_parameters...
 *
 * cardano_protocol_parameters_unref(&protocol_parameters);
 * // At this point, protocol_parameters is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_protocol_parameters_unref, the pointer to the \ref cardano_protocol_parameters_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_protocol_parameters_unref(cardano_protocol_parameters_t** protocol_parameters);

/**
 * \brief Increases the reference count of the cardano_protocol_parameters_t object.
 *
 * This function is used to manually increment the reference count of an cardano_protocol_parameters_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_protocol_parameters_unref.
 *
 * \param protocol_parameters A pointer to the cardano_protocol_parameters_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming protocol_parameters is a previously created protocol_parameters object
 *
 * cardano_protocol_parameters_ref(protocol_parameters);
 *
 * // Now protocol_parameters can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_protocol_parameters_ref there is a corresponding
 * call to \ref cardano_protocol_parameters_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_protocol_parameters_ref(cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Retrieves the current reference count of the cardano_protocol_parameters_t object.
 *
 * This function returns the number of active references to an cardano_protocol_parameters_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_protocol_parameters_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param protocol_parameters A pointer to the cardano_protocol_parameters_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_protocol_parameters_t object. If the object
 * is properly managed (i.e., every \ref cardano_protocol_parameters_ref call is matched with a
 * \ref cardano_protocol_parameters_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming protocol_parameters is a previously created protocol_parameters object
 *
 * size_t ref_count = cardano_protocol_parameters_refcount(protocol_parameters);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_protocol_parameters_refcount(const cardano_protocol_parameters_t* protocol_parameters);

/**
 * \brief Sets the last error message for a given cardano_protocol_parameters_t object.
 *
 * Records an error message in the protocol_parameters's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] protocol_parameters A pointer to the \ref cardano_protocol_parameters_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the protocol_parameters's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_protocol_parameters_set_last_error(
  cardano_protocol_parameters_t* protocol_parameters,
  const char*                    message);

/**
 * \brief Retrieves the last error message recorded for a specific protocol_parameters.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_protocol_parameters_set_last_error for the given
 * protocol_parameters. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] protocol_parameters A pointer to the \ref cardano_protocol_parameters_t instance whose last error
 *                   message is to be retrieved. If the protocol_parameters is NULL, the function
 *                   returns a generic error message indicating the null protocol_parameters.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified protocol_parameters. If the protocol_parameters is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_protocol_parameters_set_last_error for the same protocol_parameters, or until
 *       the protocol_parameters is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_protocol_parameters_get_last_error(
  const cardano_protocol_parameters_t* protocol_parameters);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_PROTOCOL_PARAMETERS_H