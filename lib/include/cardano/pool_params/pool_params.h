/**
 * \file pool_params.h
 *
 * \author angel.castillo
 * \date   Jun 26, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_POOL_PARAMS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_POOL_PARAMS_H

/* INCLUDES ******************************************************************/

#include <cardano/address/reward_address.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/unit_interval.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/pool_params/pool_metadata.h>
#include <cardano/pool_params/pool_owners.h>
#include <cardano/pool_params/relays.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Stake pool update certificate parameters.
 *
 * When a stake pool operator wants to change the parameters of their pool, they
 * must submit a pool update certificate with these parameters.
 */
typedef struct cardano_pool_params_t cardano_pool_params_t;

/**
 * \brief Creates and initializes a new instance of pool parameters.
 *
 * This function allocates and initializes a new instance of \ref cardano_pool_params_t,
 * which represents the parameters of a staking pool in the Cardano blockchain.
 *
 * \param[in] operator_key_hash A pointer to an initialized \ref cardano_blake2b_hash_t object
 *                              representing the operator's key hash.
 * \param[in] vrf_vk_hash A pointer to an initialized \ref cardano_blake2b_hash_t object containing the VRF (Verifiable Random Function)
 *                        verification key hash.
 * \param[in] pledge The amount of ADA pledged to the pool by the operator.
 * \param[in] cost The operational cost of the pool.
 * \param[in] margin A pointer to an initialized \ref cardano_unit_interval_t object representing the margin.
 * \param[in] reward_account A pointer to an initialized \ref cardano_reward_address_t object representing the reward account.
 * \param[in] owners A pointer to an initialized \ref cardano_pool_owners_t object containing the pool owners.
 * \param[in] relays A pointer to an initialized \ref cardano_relays_t object containing the pool relays.
 * \param[in] metadata A pointer to an initialized \ref cardano_pool_metadata_t object containing the pool metadata.
 * \param[out] pool_params On successful initialization, this will point to a newly created \ref cardano_pool_params_t object.
 *                         The caller is responsible for managing the lifecycle of this object. Specifically, once the pool parameters
 *                         are no longer needed, the caller must release it by calling \ref cardano_pool_params_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the pool parameters were successfully created, or an appropriate error code indicating the failure reason.
 *
 * \note This function takes references from all input objects. The caller is responsible for managing
 *       the lifecycle of these objects and must call the appropriate unref functions to release them
 *       when they are no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_blake2b_hash_t* operator_key_hash = ...; // Assume operator_key_hash is initialized
 * cardano_blake2b_hash_t* vrf_vk_hash = ...; // Assume vrf_vk_hash is initialized
 * uint64_t pledge = 1000000;
 * uint64_t cost = 340000000;
 * cardano_unit_interval_t* margin = ...; // Assume margin is initialized
 * cardano_reward_address_t* reward_account = ...; // Assume reward_account is initialized
 * cardano_pool_owners_t* owners = ...; // Assume owners is initialized
 * cardano_relays_t* relays = ...; // Assume relays is initialized
 * cardano_pool_metadata_t* metadata = ...; // Assume metadata is initialized
 * cardano_pool_params_t* pool_params = NULL;
 *
 * // Attempt to create a new pool parameters object
 * cardano_error_t result = cardano_pool_params_new(operator_key_hash, vrf_vk_hash, pledge, cost, margin, reward_account, owners, relays, metadata, &pool_params);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the pool_params
 *
 *   // Once done, ensure to clean up and release the pool_params
 *   cardano_pool_params_unref(&pool_params);
 * }
 *
 * // Ensure to clean up and release all other objects
 * cardano_blake2b_hash_unref(&operator_key_hash);
 * cardano_blake2b_hash_unref(&vrf_vk_hash);
 * cardano_unit_interval_unref(&margin);
 * cardano_reward_address_unref(&reward_account);
 * cardano_pool_owners_unref(&owners);
 * cardano_relays_unref(&relays);
 * cardano_pool_metadata_unref(&metadata);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_pool_params_new(
  cardano_blake2b_hash_t*   operator_key_hash,
  cardano_blake2b_hash_t*   vrf_vk_hash,
  uint64_t                  pledge,
  uint64_t                  cost,
  cardano_unit_interval_t*  margin,
  cardano_reward_address_t* reward_account,
  cardano_pool_owners_t*    owners,
  cardano_relays_t*         relays,
  cardano_pool_metadata_t*  metadata,
  cardano_pool_params_t**   pool_params);

/**
 * \brief Creates a \ref cardano_pool_params_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_pool_params_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a pool_params.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] pool_params A pointer to a pointer of \ref cardano_pool_params_t that will be set to the address
 *                        of the newly created pool_params object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_pool_params_t object by calling
 *       \ref cardano_pool_params_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_pool_params_t* pool_params = NULL;
 *
 * cardano_error_t result = cardano_pool_params_from_cbor(reader, &pool_params);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the pool_params
 *
 *   // Once done, ensure to clean up and release the pool_params
 *   cardano_pool_params_unref(&pool_params);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode pool_params: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_pool_params_from_cbor(cardano_cbor_reader_t* reader, cardano_pool_params_t** pool_params);

/**
 * \brief Serializes protocol version into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_pool_params_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] pool_params A constant pointer to the \ref cardano_pool_params_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p pool_params or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_pool_params_to_cbor(pool_params, writer);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     // Use the writer's buffer containing the serialized data
 *   }
 *   else
 *   {
 *     const char* error_message = cardano_cbor_writer_get_last_error(writer);
 *     printf("Serialization failed: %s\n", error_message);
 *   }
 *
 *   cardano_cbor_writer_unref(&writer);
 * }
 *
 * cardano_pool_params_unref(&pool_params);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_params_to_cbor(
  const cardano_pool_params_t* pool_params,
  cardano_cbor_writer_t*       writer);

/**
 * \brief Retrieves the operator's key hash from the pool parameters.
 *
 * This function provides access to the operator's key hash stored within the pool parameters.
 *
 * \param[in] pool_params A pointer to an initialized \ref cardano_pool_params_t object from which the key hash is retrieved.
 * \param[out] operator_key_hash On successful retrieval, this will point to an initialized \ref cardano_blake2b_hash_t object
 *                               that represents the operator's key hash. The caller gains a reference to this object and is responsible
 *                               for managing its lifecycle, which includes releasing it by calling \ref cardano_blake2b_hash_unref
 *                               when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the operator's key hash was successfully retrieved, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if the input pool_params or output operator_key_hash is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = ...; // Assume pool_params is already initialized
 * cardano_blake2b_hash_t* operator_key_hash = NULL;
 *
 * cardano_error_t result = cardano_pool_params_get_operator_key_hash(pool_params, &operator_key_hash);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the operator_key_hash
 *
 *   // Once done, ensure to clean up and release the operator_key_hash
 *   cardano_blake2b_hash_unref(&operator_key_hash);
 * }
 * else
 * {
 *   printf("Failed to retrieve the operator's key hash.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_params_get_operator_key_hash(
  cardano_pool_params_t*   pool_params,
  cardano_blake2b_hash_t** operator_key_hash);

/**
 * \brief Sets the operator's key hash for the pool parameters.
 *
 * This function assigns a new operator's key hash to a given \ref cardano_pool_params_t object.
 *
 * \param[in] pool_params A pointer to an initialized \ref cardano_pool_params_t object to which the key hash will be set.
 * \param[in] operator_key_hash A pointer to an initialized \ref cardano_blake2b_hash_t object representing the operator's key hash.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the operator's key hash was successfully set, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * \note This function increments the reference count of the operator_key_hash. It is the caller's responsibility to manage
 *       its own reference and ensure that it calls \ref cardano_blake2b_hash_unref when the hash is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = ...; // Assume pool_params is already initialized
 * cardano_blake2b_hash_t* operator_key_hash = ...; // Assume operator_key_hash is already initialized and referenced
 *
 * cardano_error_t result = cardano_pool_params_set_operator_key_hash(pool_params, operator_key_hash);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The operator_key_hash is now set for the pool_params
 *   // Remember to unref operator_key_hash when it's no longer needed
 * }
 * else
 * {
 *   printf("Failed to set the operator's key hash.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_params_set_operator_key_hash(
  cardano_pool_params_t*  pool_params,
  cardano_blake2b_hash_t* operator_key_hash);

/**
 * \brief Retrieves the VRF verification key hash from the pool parameters.
 *
 * This function gets the VRF verification key hash associated with a \ref cardano_pool_params_t object.
 *
 * \param[in] pool_params A pointer to an initialized \ref cardano_pool_params_t object from which the VRF key hash will be retrieved.
 * \param[out] vrf_vk_hash On successful retrieval, this will point to the \ref cardano_blake2b_hash_t object containing the VRF key hash.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the VRF key hash was successfully retrieved,
 *         or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the input pointer is NULL or the
 *         vrf_vk_hash pointer could not be set.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = ...; // Assume pool_params is already initialized
 * cardano_blake2b_hash_t* vrf_vk_hash = NULL;
 *
 * cardano_error_t result = cardano_pool_params_get_vrf_vk_hash(pool_params, &vrf_vk_hash);
 * if (result == CARDANO_SUCCESS && vrf_vk_hash != NULL)
 * {
 *   // Use the vrf_vk_hash
 *   // Remember to unref the vrf_vk_hash when it's no longer needed
 * }
 * else
 * {
 *   printf("Failed to retrieve the VRF key hash.\n");
 * }
 * \endcode
 *
 * \note This function increments the reference count of the returned vrf_vk_hash to ensure the caller has a valid reference.
 *       It is the caller's responsibility to decrement this count when the hash is no longer needed.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_params_get_vrf_vk_hash(
  cardano_pool_params_t*   pool_params,
  cardano_blake2b_hash_t** vrf_vk_hash);

/**
 * \brief Sets the VRF verification key hash for the pool parameters.
 *
 * This function assigns a new VRF verification key hash to a given \ref cardano_pool_params_t object.
 *
 * \param[in] pool_params A pointer to an initialized \ref cardano_pool_params_t object to which the VRF key hash will be set.
 * \param[in] vrf_vk_hash A pointer to an initialized \ref cardano_blake2b_hash_t object representing the VRF verification key hash.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the VRF key hash was successfully set,
 *         or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = ...; // Assume pool_params is already initialized
 * cardano_blake2b_hash_t* vrf_vk_hash = ...; // Assume vrf_vk_hash is already initialized
 *
 * cardano_error_t result = cardano_pool_params_set_vrf_vk_hash(pool_params, vrf_vk_hash);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The vrf_vk_hash is now set for the pool_params
 * }
 * else
 * {
 *   printf("Failed to set the VRF key hash.\n");
 * }
 * \endcode
 *
 * \note This function takes a reference to the vrf_vk_hash provided. The caller is responsible for managing their reference and must ensure
 *       that it is decremented when no longer needed. The pool parameters object will hold its own reference.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_params_set_vrf_vk_hash(
  cardano_pool_params_t*  pool_params,
  cardano_blake2b_hash_t* vrf_vk_hash);

/**
 * \brief Retrieves the pledge amount from the pool parameters.
 *
 * This function retrieves the pledge amount set for a staking pool, which is the amount of ADA pledged
 * by the pool operator to their own pool.
 *
 * \param[in] pool_params A pointer to an initialized \ref cardano_pool_params_t object from which the pledge amount is to be retrieved.
 * \param[out] pledge Pointer to a uint64_t where the pledge amount will be stored upon successful retrieval.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the pledge amount was successfully retrieved,
 *         or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = ...; // Assume pool_params is already initialized
 * uint64_t pledge;
 *
 * cardano_error_t result = cardano_pool_params_get_pledge(pool_params, &pledge);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Pledge amount: %llu ADA\n", pledge);
 * }
 * else
 * {
 *   printf("Failed to retrieve the pledge amount.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_params_get_pledge(
  cardano_pool_params_t* pool_params,
  uint64_t*              pledge);

/**
 * \brief Sets the pledge amount for the pool parameters.
 *
 * This function assigns a new pledge amount to a staking pool. The pledge is the amount of ADA that the pool operator
 * commits to keeping staked in their pool.
 *
 * \param[in] pool_params A pointer to an initialized \ref cardano_pool_params_t object to which the pledge amount will be set.
 * \param[in] pledge The new pledge amount in lovelaces (1 ADA = 1,000,000 lovelaces).
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the pledge amount
 *         was successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the input pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = ...; // Assume pool_params is already initialized
 * uint64_t new_pledge = 1500000000; // New pledge amount of 1,500 ADA
 *
 * cardano_error_t result = cardano_pool_params_set_pledge(pool_params, new_pledge);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Pledge amount successfully updated.\n");
 * }
 * else
 * {
 *   printf("Failed to update the pledge amount.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_params_set_pledge(
  cardano_pool_params_t* pool_params,
  uint64_t               pledge);

/**
 * \brief Retrieves the operational cost of the pool from the pool parameters.
 *
 * This function fetches the operational cost set for a staking pool. The operational cost is a fixed amount of ADA
 * that the pool deducts from the total rewards before distributing the rest among the stakers. This cost covers the
 * expenses incurred by the pool operators for maintaining the pool.
 *
 * \param[in] pool_params A pointer to an initialized \ref cardano_pool_params_t object from which the cost will be retrieved.
 * \param[out] cost Pointer to a variable where the operational cost will be stored upon successful retrieval.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the cost was
 *         successfully retrieved, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = ...; // Assume pool_params is already initialized
 * uint64_t cost;
 *
 * cardano_error_t result = cardano_pool_params_get_cost(pool_params, &cost);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Operational cost of the pool: %llu lovelaces.\n", cost);
 * }
 * else
 * {
 *   printf("Failed to retrieve the operational cost.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_params_get_cost(
  cardano_pool_params_t* pool_params,
  uint64_t*              cost);

/**
 * \brief Sets the operational cost of the pool in the pool parameters.
 *
 * This function assigns a new operational cost to a staking pool. The operational cost is a fixed amount of ADA
 * that the pool deducts from the total rewards before distributing the rest among the stakers. This fee is meant to cover
 * the pool operator's expenses in running the pool.
 *
 * \param[in] pool_params A pointer to an initialized \ref cardano_pool_params_t object to which the new cost will be set.
 * \param[in] cost The new operational cost in lovelaces to be set for the pool.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the cost was
 *         successfully updated, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = ...; // Assume pool_params is already initialized
 * uint64_t new_cost = 350000000; // 350 ADA expressed in lovelaces
 *
 * cardano_error_t result = cardano_pool_params_set_cost(pool_params, new_cost);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Operational cost updated to: %llu lovelaces.\n", new_cost);
 * }
 * else
 * {
 *   printf("Failed to update the operational cost.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_params_set_cost(
  cardano_pool_params_t* pool_params,
  uint64_t               cost);

/**
 * \brief Retrieves the margin from the pool parameters.
 *
 * This function fetches the margin setting from a staking pool's parameters. The margin represents the percentage
 * of total rewards that the pool operator takes as a fee before distributing the remaining rewards among the delegators.
 * It is expressed as a unit interval, ranging from 0 to 1, where 0 represents 0% and 1 represents 100%.
 *
 * \param[in] pool_params A pointer to an initialized \ref cardano_pool_params_t object from which the margin will be retrieved.
 * \param[out] margin A pointer to a pointer that will be set to point to the margin of the pool. This object is reference counted
 *                    and a new reference is obtained before it is returned to the caller. The caller is responsible for releasing
 *                    this reference when it is no longer needed by calling \ref cardano_unit_interval_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the margin was
 *         successfully retrieved, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = ...; // Assume pool_params is already initialized
 * cardano_unit_interval_t* margin = NULL;
 *
 * cardano_error_t result = cardano_pool_params_get_margin(pool_params, &margin);
 * if (result == CARDANO_SUCCESS && margin != NULL)
 * {
 *   printf("Pool margin: %f\n", cardano_unit_interval_to_double(margin));
 *   // When done using the margin
 *   cardano_unit_interval_unref(&margin);
 * }
 * else
 * {
 *   printf("Failed to retrieve the pool margin.\n");
 * }
 * \endcode
 *
 * \note It is the caller's responsibility to release the margin object by calling \ref cardano_unit_interval_unref
 *       to prevent memory leaks.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_params_get_margin(
  cardano_pool_params_t*    pool_params,
  cardano_unit_interval_t** margin);

/**
 * \brief Sets the margin for the pool parameters.
 *
 * This function assigns a new margin to a given \ref cardano_pool_params_t object.
 * The margin represents the percentage of total rewards that the pool operator takes
 * as a fee before distributing the remaining rewards among the delegators.
 *
 * \param[in] pool_params A pointer to an initialized \ref cardano_pool_params_t object to which the margin will be set.
 * \param[in] margin A pointer to an initialized \ref cardano_unit_interval_t object representing the new margin value.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the margin
 *         was successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = ...; // Assume pool_params is already initialized
 * cardano_unit_interval_t* margin = ...; // Assume margin is already initialized
 *
 * cardano_error_t result = cardano_pool_params_set_margin(pool_params, margin);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The margin has been successfully set for the pool_params
 * }
 * else
 * {
 *   printf("Failed to set the pool margin.\n");
 * }
 *
 * // Continue to manage the margin reference as needed, including releasing it when no longer required
 * cardano_unit_interval_unref(&margin);
 * \endcode
 *
 * \note This function takes a new reference to the margin object for use by the pool_params object.
 *       The caller must continue to manage their own reference to the margin object.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_params_set_margin(
  cardano_pool_params_t*   pool_params,
  cardano_unit_interval_t* margin);

/**
 * \brief Retrieves the reward account associated with the pool parameters.
 *
 * This function provides access to the reward account associated with a given \ref cardano_pool_params_t object.
 * The reward account is where the pool's rewards are deposited before distribution.
 *
 * \param[in] pool_params A pointer to an initialized \ref cardano_pool_params_t object from which to retrieve the reward account.
 * \param[out] reward_account On successful return, this will point to an initialized \ref cardano_reward_address_t object.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the reward account
 *         was successfully retrieved, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if any input pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = ...; // Assume pool_params is already initialized
 * cardano_reward_address_t* reward_account = NULL;
 *
 * cardano_error_t result = cardano_pool_params_get_reward_account(pool_params, &reward_account);
 * if (result == CARDANO_SUCCESS && reward_account != NULL)
 * {
 *   // Use the reward_account
 *
 *   // Once done, ensure to clean up and release the reward_account
 *   cardano_reward_address_unref(&reward_account);
 * }
 * else
 * {
 *   printf("Failed to retrieve the reward account.\n");
 * }
 * \endcode
 *
 * \note The caller is responsible for managing the lifecycle of the returned reward_account object.
 *       Ensure to call \ref cardano_reward_address_unref to release the reference when it is no longer needed.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_params_get_reward_account(
  cardano_pool_params_t*     pool_params,
  cardano_reward_address_t** reward_account);

/**
 * \brief Sets the reward account for the pool parameters.
 *
 * This function assigns a new reward account to a given \ref cardano_pool_params_t object.
 *
 * \param[in] pool_params A pointer to an initialized \ref cardano_pool_params_t object to which the reward account will be set.
 * \param[in] reward_account A pointer to an initialized \ref cardano_reward_address_t object representing the reward account.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the reward account was successfully set, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = ...; // Assume pool_params is already initialized
 * cardano_reward_address_t* reward_account = ...; // Assume reward_account is already initialized
 *
 * cardano_error_t result = cardano_pool_params_set_reward_account(pool_params, reward_account);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The reward_account is now set for the pool_params
 * }
 * else
 * {
 *   printf("Failed to set the reward account.\n");
 * }
 *
 * // The caller must continue to manage the lifecycle of reward_account
 * cardano_reward_address_unref(&reward_account);
 * \endcode
 *
 * \note After setting, the pool_params object holds a reference to the reward_account. It is important for the caller
 *       to manage their reference to the reward_account object appropriately, including calling \ref cardano_reward_address_unref
 *       when it is no longer needed.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_params_set_reward_account(
  cardano_pool_params_t*    pool_params,
  cardano_reward_address_t* reward_account);

/**
 * \brief Retrieves the owners of a staking pool.
 *
 * This function accesses the list of owners from a \ref cardano_pool_params_t object.
 * The function increments the reference count for the owners object before returning it,
 * ensuring that the returned reference is valid for the caller to use.
 *
 * \param[in] pool_params A pointer to an initialized \ref cardano_pool_params_t object from which the owners will be retrieved.
 * \param[out] owners On successful retrieval, this will point to a \ref cardano_pool_owners_t object that lists the owners of the pool.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the owners were successfully retrieved, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if the input pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = ...; // Assume pool_params is already initialized
 * cardano_pool_owners_t* owners = NULL;
 *
 * cardano_error_t result = cardano_pool_params_get_owners(pool_params, &owners);
 * if (result == CARDANO_SUCCESS && owners != NULL)
 * {
 *   // Use the owners
 *   // Once done, ensure to clean up and release the owners
 *   cardano_pool_owners_unref(&owners);
 * }
 * else
 * {
 *   printf("Failed to retrieve pool owners.\n");
 * }
 * \endcode
 *
 * \note The caller is responsible for releasing the retrieved owners object by calling \ref cardano_pool_owners_unref.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_params_get_owners(
  cardano_pool_params_t*  pool_params,
  cardano_pool_owners_t** owners);

/**
 * \brief Sets the owners of a staking pool.
 *
 * This function assigns a new list of owners to a \ref cardano_pool_params_t object.
 *
 * \param[in] pool_params A pointer to an initialized \ref cardano_pool_params_t object to which the owners will be set.
 * \param[in] owners A pointer to an initialized \ref cardano_pool_owners_t object representing the new owners of the pool.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the owners were successfully set, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = ...; // Assume pool_params is already initialized
 * cardano_pool_owners_t* new_owners = ...;  // Assume new_owners is already initialized
 *
 * cardano_error_t result = cardano_pool_params_set_owners(pool_params, new_owners);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The new owners have been set for the pool_params
 * }
 * else
 * {
 *   printf("Failed to set the pool owners.\n");
 * }
 *
 * // Once done, ensure to clean up and release the owners
 * cardano_pool_owners_unref(&new_owners);
 * \endcode
 *
 * \note The caller is responsible for releasing the owners object by calling \ref cardano_pool_owners_unref.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_params_set_owners(
  cardano_pool_params_t* pool_params,
  cardano_pool_owners_t* owners);

/**
 * \brief Retrieves the relays associated with a staking pool.
 *
 * This function gets the current list of relays set for a \ref cardano_pool_params_t object. It provides a pointer to the
 * \ref cardano_relays_t object containing the relays.
 *
 * \param[in] pool_params A constant pointer to an initialized \ref cardano_pool_params_t object from which the relays will be retrieved.
 * \param[out] relays On successful execution, this will point to an initialized \ref cardano_relays_t object representing the pool's relays.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the relays were successfully retrieved, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if the input pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = ...; // Assume pool_params is already initialized
 * cardano_relays_t* relays = NULL;
 *
 * cardano_error_t result = cardano_pool_params_get_relays(pool_params, &relays);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the relays
 *
 *   // Once done, ensure to clean up and release the relays
 *   cardano_relays_unref(&relays);
 * }
 * else
 * {
 *   printf("Failed to retrieve the pool relays.\n");
 * }
 * \endcode
 *
 * \note The function increments the reference count of the returned relays object to ensure its validity. The caller must manage
 *       the lifecycle of the returned object by calling \ref cardano_relays_unref when it is no longer needed.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_params_get_relays(
  cardano_pool_params_t* pool_params,
  cardano_relays_t**     relays);

/**
 * \brief Sets the relays for a staking pool.
 *
 * This function assigns a new set of relays to a \ref cardano_pool_params_t object.
 *
 * \param[in] pool_params A pointer to an initialized \ref cardano_pool_params_t object to which the relays will be set.
 * \param[in] relays A pointer to an initialized \ref cardano_relays_t object representing the new set of relays for the pool.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the relays were successfully set, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * \note The function increments the reference count of the relays object. The caller must manage
 *       the lifecycle of its object by calling \ref cardano_relays_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = ...; // Assume pool_params is already initialized
 * cardano_relays_t* relays = ...; // Assume relays is already initialized
 *
 * cardano_error_t result = cardano_pool_params_set_relays(pool_params, relays);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The relays are now set for the pool_params
 * }
 * else
 * {
 *   printf("Failed to set the pool relays.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_params_set_relays(
  cardano_pool_params_t* pool_params,
  cardano_relays_t*      relays);

/**
 * \brief Retrieves the metadata for a staking pool.
 *
 * This function obtains the metadata associated with a \ref cardano_pool_params_t object.
 *
 * \param[in] pool_params A pointer to an initialized \ref cardano_pool_params_t object from which to retrieve the metadata.
 * \param[out] metadata On successful return, this will point to an initialized \ref cardano_pool_metadata_t object representing the pool's metadata.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the metadata was successfully retrieved, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * \note The caller is responsible for managing the lifecycle of the returned metadata object and must call
 *       \ref cardano_pool_metadata_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = ...; // Assume pool_params is already initialized
 * cardano_pool_metadata_t* metadata = NULL;
 *
 * cardano_error_t result = cardano_pool_params_get_metadata(pool_params, &metadata);
 * if (result == CARDANO_SUCCESS && metadata != NULL)
 * {
 *   // Use the metadata
 *   // Once done, ensure to clean up and release the metadata
 *   cardano_pool_metadata_unref(&metadata);
 * }
 * else
 * {
 *   printf("Failed to retrieve the pool metadata.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_params_get_metadata(
  cardano_pool_params_t*    pool_params,
  cardano_pool_metadata_t** metadata);

/**
 * \brief Sets the metadata for a staking pool.
 *
 * This function assigns a metadata object to a \ref cardano_pool_params_t object. If the pool parameters
 * already have a metadata object assigned, it is replaced by the new one.
 *
 * \param[in] pool_params A pointer to an initialized \ref cardano_pool_params_t object to which the metadata will be set.
 * \param[in] metadata A pointer to an initialized \ref cardano_pool_metadata_t object representing the pool's metadata.
 *                      This function increments the reference count of the metadata object, ensuring it remains valid for the duration
 *                      of its association with the pool parameters.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the metadata was successfully set, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * \note It is the responsibility of the caller to manage the lifecycle of the metadata object passed to this function.
 *       The pool_params object will hold its own reference to the metadata object, so the caller must still unref their
 *       own reference when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = ...; // Assume pool_params is already initialized
 * cardano_pool_metadata_t* metadata = ...; // Assume metadata is initialized
 *
 * cardano_error_t result = cardano_pool_params_set_metadata(pool_params, metadata);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The metadata is now set for the pool_params
 * }
 * else
 * {
 *   printf("Failed to set the pool metadata.\n");
 * }
 *
 * // Remember to unref your own reference to the metadata if it is no longer needed
 * cardano_pool_metadata_unref(&metadata);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_pool_params_set_metadata(
  cardano_pool_params_t*   pool_params,
  cardano_pool_metadata_t* metadata);

/**
 * \brief Decrements the reference count of a cardano_pool_params_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_pool_params_t object
 * by decreasing its reference count. When the reference count reaches zero, the pool_params is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] pool_params A pointer to the pointer of the pool_params object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_pool_params_t* pool_params = cardano_pool_params_new(major, minor);
 *
 * // Perform operations with the pool_params...
 *
 * cardano_pool_params_unref(&pool_params);
 * // At this point, pool_params is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_pool_params_unref, the pointer to the \ref cardano_pool_params_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_pool_params_unref(cardano_pool_params_t** pool_params);

/**
 * \brief Increases the reference count of the cardano_pool_params_t object.
 *
 * This function is used to manually increment the reference count of an cardano_pool_params_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_pool_params_unref.
 *
 * \param pool_params A pointer to the cardano_pool_params_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming pool_params is a previously created pool_params object
 *
 * cardano_pool_params_ref(pool_params);
 *
 * // Now pool_params can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_pool_params_ref there is a corresponding
 * call to \ref cardano_pool_params_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_pool_params_ref(cardano_pool_params_t* pool_params);

/**
 * \brief Retrieves the current reference count of the cardano_pool_params_t object.
 *
 * This function returns the number of active references to an cardano_pool_params_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_pool_params_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param pool_params A pointer to the cardano_pool_params_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_pool_params_t object. If the object
 * is properly managed (i.e., every \ref cardano_pool_params_ref call is matched with a
 * \ref cardano_pool_params_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming pool_params is a previously created pool_params object
 *
 * size_t ref_count = cardano_pool_params_refcount(pool_params);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_pool_params_refcount(const cardano_pool_params_t* pool_params);

/**
 * \brief Sets the last error message for a given cardano_pool_params_t object.
 *
 * Records an error message in the pool_params's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] pool_params A pointer to the \ref cardano_pool_params_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the pool_params's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_pool_params_set_last_error(
  cardano_pool_params_t* pool_params,
  const char*            message);

/**
 * \brief Retrieves the last error message recorded for a specific pool_params.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_pool_params_set_last_error for the given
 * pool_params. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] pool_params A pointer to the \ref cardano_pool_params_t instance whose last error
 *                   message is to be retrieved. If the pool_params is NULL, the function
 *                   returns a generic error message indicating the null pool_params.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified pool_params. If the pool_params is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_pool_params_set_last_error for the same pool_params, or until
 *       the pool_params is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_pool_params_get_last_error(
  const cardano_pool_params_t* pool_params);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_POOL_PARAMS_H