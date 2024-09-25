/**
 * \file protocol_param_update.h
 *
 * \author angel.castillo
 * \date   Jun 09, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_PROTOCOL_PARAM_UPDATE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_PROTOCOL_PARAM_UPDATE_H

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
 * \brief The ProtocolParamUpdate structure in Cardano is used to propose changes to
 * the protocol parameters of the blockchain. Protocol parameters govern various
 * aspects of the Cardano network.
 */
typedef struct cardano_protocol_param_update_t cardano_protocol_param_update_t;

/**
 * \brief Creates and initializes a new instance of the protocol parameter update.
 *
 * This function allocates and initializes a new instance of the protocol parameter update,
 * representing a set of updates to the Cardano protocol parameters.
 *
 * \param[out] protocol_param_update On successful initialization, this will point to a newly created
 *            protocol parameter update object. This object represents a "strong reference",
 *            meaning that it is fully initialized and ready for use.
 *            The caller is responsible for managing the lifecycle of this object.
 *            Specifically, once the protocol parameter update is no longer needed, the caller must release it
 *            by calling \ref cardano_protocol_param_update_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the protocol parameter update was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 *
 * // Attempt to create a new protocol parameter update object
 * cardano_error_t result = cardano_protocol_param_update_new(&protocol_param_update);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the protocol_param_update
 *
 *   // Once done, ensure to clean up and release the protocol_param_update
 *   cardano_protocol_param_update_unref(&protocol_param_update);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_protocol_param_update_new(cardano_protocol_param_update_t** protocol_param_update);

/**
 * \brief Creates a \ref cardano_protocol_param_update_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_protocol_param_update_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a protocol_param_update.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] protocol_param_update A pointer to a pointer of \ref cardano_protocol_param_update_t that will be set to the address
 *                        of the newly created protocol_param_update object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_protocol_param_update_t object by calling
 *       \ref cardano_protocol_param_update_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 *
 * cardano_error_t result = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the protocol_param_update
 *
 *   // Once done, ensure to clean up and release the protocol_param_update
 *   cardano_protocol_param_update_unref(&protocol_param_update);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode protocol_param_update: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_protocol_param_update_from_cbor(cardano_cbor_reader_t* reader, cardano_protocol_param_update_t** protocol_param_update);

/**
 * \brief Serializes protocol version into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_protocol_param_update_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] protocol_param_update A constant pointer to the \ref cardano_protocol_param_update_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p protocol_param_update or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_protocol_param_update_to_cbor(protocol_param_update, writer);
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
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_to_cbor(
  const cardano_protocol_param_update_t* protocol_param_update,
  cardano_cbor_writer_t*                 writer);

/**
 * \brief Retrieves the minimum fee A parameter from the protocol parameter update.
 *
 * This function returns the minimum fee A parameter from the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] min_fee_a Pointer to where the minimum fee A value will be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the minimum fee A was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if the
 *         parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t min_fee_a;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_min_fee_a(protocol_param_update, &min_fee_a);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Min Fee A: %lu\n", min_fee_a);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Min Fee A is not proposed for change.\n");
 * }
 * else
 * {
 *   printf("Error retrieving Min Fee A: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_min_fee_a(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              min_fee_a);

/**
 * \brief Retrieves the minimum fee B parameter from the protocol parameter update.
 *
 * This function returns the minimum fee B parameter from the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] min_fee_b Pointer to where the minimum fee B value will be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the minimum fee B was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if the
 *         parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t min_fee_b;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_min_fee_b(protocol_param_update, &min_fee_b);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Min Fee B: %lu\n", min_fee_b);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Min Fee B is not proposed for change.\n");
 * }
 * else
 * {
 *   printf("Error retrieving Min Fee B: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_min_fee_b(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              min_fee_b);

/**
 * \brief Retrieves the maximum block body size parameter from the protocol parameter update.
 *
 * This function returns the maximum block body size parameter from the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] max_block_body_size Pointer to where the maximum block body size value will be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the maximum block body size was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t max_block_body_size;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_max_block_body_size(protocol_param_update, &max_block_body_size);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Max Block Body Size: %lu\n", max_block_body_size);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Max Block Body Size is not proposed for change.\n");
 * }
 * else
 * {
 *   printf("Error retrieving Max Block Body Size: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_max_block_body_size(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              max_block_body_size);

/**
 * \brief Retrieves the maximum transaction size parameter from the protocol parameter update.
 *
 * This function returns the maximum transaction size parameter from the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] max_tx_size Pointer to where the maximum transaction size value will be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the maximum transaction size was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t max_tx_size;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_max_tx_size(protocol_param_update, &max_tx_size);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Max Transaction Size: %lu\n", max_tx_size);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Max Transaction Size is not proposed for change.\n");
 * }
 * else
 * {
 *   printf("Error retrieving Max Transaction Size: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_max_tx_size(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              max_tx_size);

/**
 * \brief Retrieves the maximum block header size parameter from the protocol parameter update.
 *
 * This function returns the maximum block header size parameter from the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] max_block_header_size Pointer to where the maximum block header size value will be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the maximum block header size was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t max_block_header_size;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_max_block_header_size(protocol_param_update, &max_block_header_size);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Max Block Header Size: %lu\n", max_block_header_size);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Max Block Header Size is not proposed for change.\n");
 * }
 * else
 * {
 *   printf("Error retrieving Max Block Header Size: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_max_block_header_size(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              max_block_header_size);

/**
 * \brief Retrieves the key deposit parameter from the protocol parameter update.
 *
 * This function returns the key deposit parameter from the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] key_deposit           Pointer to where the key deposit value will be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the key deposit was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t key_deposit;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_key_deposit(protocol_param_update, &key_deposit);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Key Deposit: %lu\n", key_deposit);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Key Deposit is not proposed for change.\n");
 * }
 * else
 * {
 *   printf("Error retrieving Key Deposit: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_key_deposit(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              key_deposit);

/**
 * \brief Retrieves the pool deposit parameter from the protocol parameter update.
 *
 * This function returns the pool deposit parameter from the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] pool_deposit          Pointer to where the pool deposit value will be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the pool deposit was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t pool_deposit;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_pool_deposit(protocol_param_update, &pool_deposit);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Pool Deposit: %lu\n", pool_deposit);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Pool Deposit is not proposed for change.\n");
 * }
 * else
 * {
 *   printf("Error retrieving Pool Deposit: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_pool_deposit(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              pool_deposit);

/**
 * \brief Retrieves the max epoch parameter from the protocol parameter update.
 *
 * This function returns the max epoch parameter from the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] max_epoch             Pointer to where the max epoch value will be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the max epoch was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t max_epoch;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_max_epoch(protocol_param_update, &max_epoch);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Max Epoch: %lu\n", max_epoch);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Max Epoch is not proposed for change.\n");
 * }
 * else
 * {
 *   printf("Error retrieving Max Epoch: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_max_epoch(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              max_epoch);

/**
 * \brief Retrieves the desired number of stake pools (nOpt) from the protocol parameter update.
 *
 * This function returns the desired number of stake pools (nOpt) from the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] n_opt                 Pointer to where the desired number of stake pools will be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the desired number of stake pools was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t n_opt;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_n_opt(protocol_param_update, &n_opt);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Desired number of stake pools (nOpt): %lu\n", n_opt);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Desired number of stake pools (nOpt) is not proposed for change.\n");
 * }
 * else
 * {
 *   printf("Error retrieving desired number of stake pools (nOpt): %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_n_opt(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              n_opt);

/**
 * \brief Retrieves the pool pledge influence (a0) setting from the protocol parameter update.
 *
 * This function returns the pool pledge influence (a0) from the protocol parameter update.
 *
 * \param[in]  protocol_param_update   Pointer to the protocol parameter update object.
 * \param[out] pool_pledge_influence   Pointer to where the pool pledge influence value will be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the pool pledge influence was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_unit_interval_t* pool_pledge_influence = NULL;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_pool_pledge_influence(protocol_param_update, &pool_pledge_influence);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Pool pledge influence: %f\n", unit_interval_to_float(pool_pledge_influence));
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Pool pledge influence is not proposed for change.\n");
 * }
 * else
 * {
 *   printf("Error retrieving pool pledge influence: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_pool_pledge_influence(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t**        pool_pledge_influence);

/**
 * \brief Retrieves the expansion rate setting from the protocol parameter update.
 *
 * This function returns the expansion rate from the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] expansion_rate Pointer to where the expansion rate value will be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the expansion rate was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_unit_interval_t* expansion_rate = NULL;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_expansion_rate(protocol_param_update, &expansion_rate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Expansion rate: %f\n", unit_interval_to_float(expansion_rate));
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Expansion rate is not proposed for change.\n");
 * }
 * else
 * {
 *   printf("Error retrieving expansion rate: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_expansion_rate(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t**        expansion_rate);

/**
 * \brief Retrieves the treasury growth rate setting from the protocol parameter update.
 *
 * This function returns the treasury growth rate from the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] treasury_growth_rate Pointer to where the treasury growth rate value will be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the treasury growth rate was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_unit_interval_t* treasury_growth_rate = NULL;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_treasury_growth_rate(protocol_param_update, &treasury_growth_rate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Treasury growth rate: %f\n", unit_interval_to_float(treasury_growth_rate));
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Treasury growth rate is not proposed for change.\n");
 * }
 * else
 * {
 *   printf("Error retrieving treasury growth rate: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_treasury_growth_rate(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t**        treasury_growth_rate);

/**
 * \brief Retrieves the decentralization parameter (d) from the protocol parameter update.
 *
 * This function retrieves the value of the decentralization parameter (d) from the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] d                     On successful retrieval, this will point to the decentralization parameter (d).
 *                                   If the parameter is not set, the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the decentralization parameter was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_unit_interval_t* d = NULL;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_d(protocol_param_update, &d);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the decentralization parameter
 *   printf("Decentralization parameter: %f\n", unit_interval_to_float(d));
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Decentralization parameter is not set.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get decentralization parameter: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * cardano_unit_interval_unref(&d);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_d(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t**        d);

/**
 * \brief Retrieves the extra entropy setting from the protocol parameter update.
 *
 * This function retrieves the extra entropy from the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] extra_entropy Pointer to where the extra entropy value will be stored.
 *                           If the parameter is not set, the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the extra entropy was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_buffer_t* extra_entropy = NULL;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_extra_entropy(protocol_param_update, &extra_entropy);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the extra entropy
 *   printf("Extra entropy retrieved.\n");
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Extra entropy is not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get extra entropy: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * cardano_buffer_unref(&extra_entropy);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_extra_entropy(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_buffer_t**               extra_entropy);

/**
 * \brief Retrieves the protocol version from the protocol parameter update.
 *
 * This function retrieves the protocol version from the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] protocol_version Pointer to where the protocol version will be stored.
 *                              If the parameter is not set, the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the protocol version was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_protocol_version_t* protocol_version = NULL;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_protocol_version(protocol_param_update, &protocol_version);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the protocol version
 *   printf("Protocol version retrieved.\n");
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Protocol version is not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get protocol version: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * cardano_protocol_version_unref(&protocol_version);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_protocol_version(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_protocol_version_t**     protocol_version);

/**
 * \brief Retrieves the minimum pool cost from the protocol parameter update.
 *
 * This function retrieves the minimum pool cost from the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] min_pool_cost Pointer to where the minimum pool cost will be stored.
 *                           If the parameter is not set, the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the minimum pool cost was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t min_pool_cost = 0;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_min_pool_cost(protocol_param_update, &min_pool_cost);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Minimum pool cost retrieved: %lu.\n", min_pool_cost);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Minimum pool cost is not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get minimum pool cost: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_min_pool_cost(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              min_pool_cost);

/**
 * \brief Retrieves the ADA cost per UTXO byte from the protocol parameter update.
 *
 * This function retrieves the ADA cost per UTXO byte from the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] ada_per_utxo_byte Pointer to where the ADA per UTXO byte will be stored.
 *                               If the parameter is not set, the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the ADA per UTXO byte was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t ada_per_utxo_byte = 0;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_ada_per_utxo_byte(protocol_param_update, &ada_per_utxo_byte);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("ADA per UTXO byte retrieved: %lu.\n", ada_per_utxo_byte);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("ADA per UTXO byte is not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get ADA per UTXO byte: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_ada_per_utxo_byte(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              ada_per_utxo_byte);

/**
 * \brief Retrieves the cost models from the protocol parameter update.
 *
 * This function retrieves the cost models from the protocol parameter update. The cost models are used
 * to specify the computational cost of various operations within Plutus scripts.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] cost_models Pointer to where the cost models object will be stored.
 *                         If the parameter is not set, the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the cost models were successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_costmdls_t* cost_models = NULL;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_cost_models(protocol_param_update, &cost_models);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Cost models retrieved.\n");
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Cost models are not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get cost models: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * cardano_costmdls_unref(&cost_models);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_cost_models(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_costmdls_t**             cost_models);

/**
 * \brief Retrieves the execution costs from the protocol parameter update.
 *
 * This function returns the execution costs from the protocol parameter update. Execution costs
 * define the price for computational resources used by smart contracts on the blockchain.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] execution_costs Pointer to where the execution costs object will be stored.
 *                             If the parameter is not set, the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the execution costs were successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_ex_unit_prices_t* execution_costs = NULL;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_execution_costs(protocol_param_update, &execution_costs);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Execution costs retrieved.\n");
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Execution costs are not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get execution costs: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * cardano_ex_unit_prices_unref(&execution_costs);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_execution_costs(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_ex_unit_prices_t**       execution_costs);

/**
 * \brief Retrieves the maximum transaction execution units from the protocol parameter update.
 *
 * This function returns the maximum execution units that a transaction can consume. Execution units
 * are measurements of the computational and memory resources used by transactions involving smart contracts.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] max_tx_ex_units Pointer to where the maximum transaction execution units object will be stored.
 *                             If the parameter is not set, the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the maximum transaction execution units were successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_ex_units_t* max_tx_ex_units = NULL;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_max_tx_ex_units(protocol_param_update, &max_tx_ex_units);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Maximum transaction execution units retrieved.\n");
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Maximum transaction execution units are not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get maximum transaction execution units: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * cardano_ex_units_unref(&max_tx_ex_units);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_max_tx_ex_units(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_ex_units_t**             max_tx_ex_units);

/**
 * \brief Retrieves the maximum block execution units from the protocol parameter update.
 *
 * This function returns the maximum execution units that a block can consume. Execution units
 * measure the computational and memory resources used by transactions involving smart contracts within a block.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] max_block_ex_units Pointer to where the maximum block execution units object will be stored.
 *                                If the parameter is not set, the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the maximum block execution units were successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_ex_units_t* max_block_ex_units = NULL;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_max_block_ex_units(protocol_param_update, &max_block_ex_units);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Maximum block execution units retrieved.\n");
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Maximum block execution units are not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get maximum block execution units: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * cardano_ex_units_unref(&max_block_ex_units);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_max_block_ex_units(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_ex_units_t**             max_block_ex_units);

/**
 * \brief Retrieves the maximum value size from the protocol parameter update.
 *
 * This function returns the maximum serialized length (in bytes) of a multi-asset value (token bundle)
 * in a transaction output.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] max_value_size Pointer to where the maximum value size will be stored. If the parameter is not set,
 *                            the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the maximum value size was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t max_value_size = 0;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_max_value_size(protocol_param_update, &max_value_size);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Maximum value size retrieved: %lu bytes.\n", max_value_size);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Maximum value size is not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get maximum value size: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_max_value_size(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              max_value_size);

/**
 * \brief Retrieves the collateral percentage from the protocol parameter update.
 *
 * This function returns the percentage of the total transaction fee that the collateral must (at minimum) cover.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] collateral_percentage Pointer to where the collateral percentage will be stored. If the parameter is not set,
 *                                   the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the collateral percentage was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t collateral_percentage = 0;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_collateral_percentage(protocol_param_update, &collateral_percentage);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Collateral percentage retrieved: %lu.\n", collateral_percentage);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Collateral percentage is not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get collateral percentage: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_collateral_percentage(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              collateral_percentage);

/**
 * \brief Retrieves the maximum number of collateral inputs from the protocol parameter update.
 *
 * This function returns the limit on the total number of collateral inputs allowed in a transaction.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] max_collateral_inputs Pointer to where the maximum number of collateral inputs will be stored.
 *                                   If the parameter is not set, the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the maximum number of collateral inputs was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t max_collateral_inputs = 0;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_max_collateral_inputs(protocol_param_update, &max_collateral_inputs);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Max collateral inputs retrieved: %lu.\n", max_collateral_inputs);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Max collateral inputs is not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get max collateral inputs: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_max_collateral_inputs(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              max_collateral_inputs);

/**
 * \brief Retrieves the pool voting thresholds from the protocol parameter update.
 *
 * This function returns the thresholds for pool voting included in the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] pool_voting_thresholds Pointer to where the pool voting thresholds will be stored.
 *                                    If the parameter is not set, the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the pool voting thresholds were successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_pool_voting_thresholds_t* pool_voting_thresholds = NULL;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_pool_voting_thresholds(protocol_param_update, &pool_voting_thresholds);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Pool voting thresholds retrieved successfully.\n");
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Pool voting thresholds are not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get pool voting thresholds: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_pool_voting_thresholds(
  cardano_protocol_param_update_t*   protocol_param_update,
  cardano_pool_voting_thresholds_t** pool_voting_thresholds);

/**
 * \brief Retrieves the DRep voting thresholds from the protocol parameter update.
 *
 * This function returns the thresholds for DRep voting included in the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] drep_voting_thresholds Pointer to where the DRep voting thresholds will be stored.
 *                                    If the parameter is not set, the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the DRep voting thresholds were successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = NULL;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_drep_voting_thresholds(protocol_param_update, &drep_voting_thresholds);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("DRep voting thresholds retrieved successfully.\n");
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("DRep voting thresholds are not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get DRep voting thresholds: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_drep_voting_thresholds(
  cardano_protocol_param_update_t*   protocol_param_update,
  cardano_drep_voting_thresholds_t** drep_voting_thresholds);

/**
 * \brief Retrieves the minimum committee size from the protocol parameter update.
 *
 * This function returns the minimum committee size included in the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] min_committee_size Pointer to where the minimum committee size will be stored.
 *                                If the parameter is not set, the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the minimum committee size was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t min_committee_size = 0;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_min_committee_size(protocol_param_update, &min_committee_size);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Minimum committee size retrieved successfully: %lu.\n", min_committee_size);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Minimum committee size is not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get minimum committee size: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_min_committee_size(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              min_committee_size);

/**
 * \brief Retrieves the committee term limit from the protocol parameter update.
 *
 * This function returns the committee term limit included in the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] committee_term_limit Pointer to where the committee term limit will be stored.
 *                                  If the parameter is not set, the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the committee term limit was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t committee_term_limit = 0;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_committee_term_limit(protocol_param_update, &committee_term_limit);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Committee term limit retrieved successfully: %lu.\n", committee_term_limit);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Committee term limit is not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get committee term limit: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_committee_term_limit(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              committee_term_limit);

/**
 * \brief Retrieves the governance action validity period from the protocol parameter update.
 *
 * This function returns the governance action validity period included in the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] governance_action_validity_period Pointer to where the governance action validity period will be stored.
 *                                               If the parameter is not set, the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the governance action validity period was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t governance_action_validity_period = 0;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_governance_action_validity_period(protocol_param_update, &governance_action_validity_period);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Governance action validity period retrieved successfully: %lu.\n", governance_action_validity_period);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Governance action validity period is not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get governance action validity period: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_governance_action_validity_period(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              governance_action_validity_period);

/**
 * \brief Retrieves the governance action deposit from the protocol parameter update.
 *
 * This function returns the governance action deposit included in the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] governance_action_deposit Pointer to where the governance action deposit will be stored.
 *                                       If the parameter is not set, the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the governance action deposit was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t governance_action_deposit = 0;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_governance_action_deposit(protocol_param_update, &governance_action_deposit);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Governance action deposit retrieved successfully: %lu.\n", governance_action_deposit);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Governance action deposit is not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get governance action deposit: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_governance_action_deposit(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              governance_action_deposit);

/**
 * \brief Retrieves the DRep deposit from the protocol parameter update.
 *
 * This function returns the DRep deposit included in the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] drep_deposit Pointer to where the DRep deposit will be stored. If the parameter is not set,
 *                          the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the DRep deposit was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t drep_deposit = 0;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_drep_deposit(protocol_param_update, &drep_deposit);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("DRep deposit retrieved successfully: %lu.\n", drep_deposit);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("DRep deposit is not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get DRep deposit: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_drep_deposit(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              drep_deposit);

/**
 * \brief Retrieves the DRep inactivity period from the protocol parameter update.
 *
 * This function returns the DRep inactivity period included in the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] drep_inactivity_period Pointer to where the DRep inactivity period will be stored. If the parameter is not set,
 *                                    the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the DRep inactivity period was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t drep_inactivity_period = 0;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_drep_inactivity_period(protocol_param_update, &drep_inactivity_period);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("DRep inactivity period retrieved successfully: %lu.\n", drep_inactivity_period);
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("DRep inactivity period is not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get DRep inactivity period: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_drep_inactivity_period(
  const cardano_protocol_param_update_t* protocol_param_update,
  uint64_t*                              drep_inactivity_period);

/**
 * \brief Retrieves the reference script cost per byte from the protocol parameter update.
 *
 * This function returns the reference script cost per byte included in the protocol parameter update.
 *
 * \param[in]  protocol_param_update Pointer to the protocol parameter update object.
 * \param[out] ref_script_cost_per_byte Pointer to where the reference script cost per byte will be stored.
 *                                      If the parameter is not set, the function will return \ref CARDANO_ERROR_ELEMENT_NOT_FOUND.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the reference script cost per byte was successfully retrieved, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the parameter is not set in the update. Other appropriate error codes may indicate different
 *         failure reasons.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_unit_interval_t* ref_script_cost_per_byte = NULL;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_get_ref_script_cost_per_byte(protocol_param_update, &ref_script_cost_per_byte);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Reference script cost per byte retrieved successfully: %f.\n", cardano_unit_interval_to_double(ref_script_cost_per_byte));
 * }
 * else if (result == CARDANO_ERROR_ELEMENT_NOT_FOUND)
 * {
 *   printf("Reference script cost per byte is not proposed for change.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get reference script cost per byte: %d\n", result);
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * cardano_unit_interval_unref(&ref_script_cost_per_byte);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_get_ref_script_cost_per_byte(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t**        ref_script_cost_per_byte);

/**
 * \brief Sets the minimum fee A in the protocol parameter update.
 *
 * This function sets the minimum fee A value in the protocol parameter update.
 * If NULL is passed for min_fee_a, it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     min_fee_a Pointer to the value of minimum fee A to be set.
 *               If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the minimum fee A was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t proposed_min_fee_a = 500;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_min_fee_a(protocol_param_update, &proposed_min_fee_a);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Minimum fee A set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set minimum fee A.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_min_fee_a(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  min_fee_a);

/**
 * \brief Sets the minimum fee B in the protocol parameter update.
 *
 * This function sets the minimum fee B value in the protocol parameter update.
 * If NULL is passed for min_fee_b, it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     min_fee_b Pointer to the value of minimum fee B to be set.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the minimum fee B was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t proposed_min_fee_b = 2;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_min_fee_b(protocol_param_update, &proposed_min_fee_b);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Minimum fee B set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set minimum fee B.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_min_fee_b(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  min_fee_b);

/**
 * \brief Sets the maximum block body size in the protocol parameter update.
 *
 * This function sets the maximum block body size in the protocol parameter update.
 * If NULL is passed for max_block_body_size, it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     max_block_body_size Pointer to the value of maximum block body size to be set.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the maximum block body size was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t proposed_max_block_body_size = 1024000;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_max_block_body_size(protocol_param_update, &proposed_max_block_body_size);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Maximum block body size set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set maximum block body size.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_max_block_body_size(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  max_block_body_size);

/**
 * \brief Sets the maximum transaction size in the protocol parameter update.
 *
 * This function sets the maximum transaction size in the protocol parameter update.
 * If NULL is passed for max_tx_size, it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     max_tx_size Pointer to the value of maximum transaction size to be set.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the maximum transaction size was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t proposed_max_tx_size = 16384;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_max_tx_size(protocol_param_update, &proposed_max_tx_size);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Maximum transaction size set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set maximum transaction size.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_max_tx_size(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  max_tx_size);

/**
 * \brief Sets the maximum block header size in the protocol parameter update.
 *
 * This function sets the maximum block header size in the protocol parameter update.
 * If NULL is passed for max_block_header_size, it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     max_block_header_size Pointer to the value of maximum block header size to be set.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the maximum block header size was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t proposed_max_block_header_size = 1024;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_max_block_header_size(protocol_param_update, &proposed_max_block_header_size);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Maximum block header size set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set maximum block header size.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_max_block_header_size(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  max_block_header_size);

/**
 * \brief Sets the key deposit in the protocol parameter update.
 *
 * This function sets the key deposit in the protocol parameter update.
 * If NULL is passed for key_deposit, it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     key_deposit Pointer to the value of key deposit to be set.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the key deposit was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t proposed_key_deposit = 2000000; // Example value in lovelace
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_key_deposit(protocol_param_update, &proposed_key_deposit);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Key deposit set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set key deposit.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_key_deposit(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  key_deposit);

/**
 * \brief Sets the pool deposit in the protocol parameter update.
 *
 * This function sets the pool deposit in the protocol parameter update.
 * If NULL is passed for pool_deposit, it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     pool_deposit Pointer to the value of pool deposit to be set.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the pool deposit was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t proposed_pool_deposit = 5000000; // Example value in lovelace
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_pool_deposit(protocol_param_update, &proposed_pool_deposit);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Pool deposit set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set pool deposit.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_pool_deposit(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  pool_deposit);

/**
 * \brief Sets the maximum epoch (number of epochs) for which a pool can be ranked in
 * the non-myopic member rewards.

 * This function sets the maximum epoch in the protocol parameter update.
 * If NULL is passed for max_epoch, it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     max_epoch Pointer to the value of maximum epoch to be set.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the maximum epoch was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t proposed_max_epoch = 250; // Example value
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_max_epoch(protocol_param_update, &proposed_max_epoch);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Maximum epoch set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set maximum epoch.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_max_epoch(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  max_epoch);

/**
 * \brief Sets the desired number of stake pools. It's used in the rewards calculation
 * to encourage a certain number of active stake pools.
 *
 * This function sets the desired number of stake pools (n_opt) in the protocol parameter update.
 * If NULL is passed for n_opt, it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     n_opt Pointer to the value of the desired number of stake pools to be set.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the desired number of stake pools was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t proposed_n_opt = 150; // Example value
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_n_opt(protocol_param_update, &proposed_n_opt);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Desired number of stake pools set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set desired number of stake pools.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_n_opt(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  n_opt);

/**
 * \brief Sets the pool pledge influence in the protocol parameter update.
 *
 * This function sets the pool pledge influence (a0 parameter) in the protocol parameter update.
 * This parameter determines how much influence a pool's pledge has on the pool's rewards. If NULL is passed
 * for pool_pledge_influence, it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     pool_pledge_influence Pointer to the pool pledge influence value to be set.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the pool pledge influence was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_unit_interval_t* proposed_pool_pledge_influence = NULL; // Example pointer to a unit interval
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_pool_pledge_influence(protocol_param_update, proposed_pool_pledge_influence);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Pool pledge influence set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set pool pledge influence.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_pool_pledge_influence(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t*         pool_pledge_influence);

/**
 * \brief Sets the expansion rate in the protocol parameter update.
 *
 * This function sets the expansion rate in the protocol parameter update.
 * The expansion rate determines the percentage of the remaining reserve that is
 * used to fund rewards and treasury each epoch. If NULL is passed for expansion_rate,
 * it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     expansion_rate Pointer to the expansion rate value to be set.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the expansion rate was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_unit_interval_t* proposed_expansion_rate = NULL; // Example pointer to a unit interval
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_expansion_rate(protocol_param_update, proposed_expansion_rate);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Expansion rate set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set expansion rate.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_expansion_rate(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t*         expansion_rate);

/**
 * \brief Sets the treasury growth rate in the protocol parameter update.
 *
 * This function sets the treasury growth rate in the protocol parameter update.
 * The treasury growth rate determines the percentage of the expansion taken from reserves
 * that is allocated to the treasury each epoch. If NULL is passed for treasury_growth_rate,
 * it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     treasury_growth_rate Pointer to the treasury growth rate value to be set.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the treasury growth rate was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_unit_interval_t* proposed_treasury_growth_rate = NULL; // Example pointer to a unit interval
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_treasury_growth_rate(protocol_param_update, proposed_treasury_growth_rate);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Treasury growth rate set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set treasury growth rate.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_treasury_growth_rate(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t*         treasury_growth_rate);

/**
 * \brief Sets the decentralization parameter 'd' in the protocol parameter update.
 *
 * This function sets the decentralization parameter 'd' in the protocol parameter update.
 * The parameter 'd' ranges from 0 to 1, where 0 represents a fully decentralized network with all blocks produced by community stake pools,
 * and 1 indicates a fully centralized scenario where all blocks are produced by a federated system. If NULL is passed for 'd',
 * it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     d Pointer to the decentralization parameter value to be set.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the decentralization parameter was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_unit_interval_t* proposed_d = NULL; // Example pointer to a unit interval
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_d(protocol_param_update, proposed_d);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Decentralization parameter set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set decentralization parameter.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_d(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t*         d);

/**
 * \brief Sets the extra entropy in the protocol parameter update.
 *
 * This function sets the extra entropy used to seed the pseudo-random number generator for leader election
 * in the protocol parameter update. If NULL is passed for extra_entropy, it indicates that the update should
 * not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     extra_entropy Pointer to the buffer containing the extra entropy.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the extra entropy was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_buffer_t* proposed_extra_entropy = NULL; // Example pointer to an entropy buffer
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_extra_entropy(protocol_param_update, proposed_extra_entropy);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Extra entropy set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set extra entropy.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_extra_entropy(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_buffer_t*                extra_entropy);

/**
 * \brief Sets the protocol version in the protocol parameter update.
 *
 * This function sets the protocol version, which consists of major and minor version numbers, in the protocol parameter update.
 * If NULL is passed for protocol_version, it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     protocol_version Pointer to the protocol version structure.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the protocol version was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_protocol_version_t* proposed_protocol_version = NULL; // Example pointer to a protocol version structure
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_protocol_version(protocol_param_update, proposed_protocol_version);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Protocol version set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set protocol version.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_protocol_version(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_protocol_version_t*      protocol_version);

/**
 * \brief Sets the minimum pool cost in the protocol parameter update.
 *
 * This function sets the minimum pool cost, which determines the minimum operational cost that a stake pool can declare.
 * The cost is specified in lovelaces. If NULL is passed for min_pool_cost, it indicates that the update should not propose
 * a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     min_pool_cost Pointer to the minimum pool cost in lovelaces.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the minimum pool cost was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t proposed_min_pool_cost = 340000000;  // Example minimum pool cost in lovelaces
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_min_pool_cost(protocol_param_update, &proposed_min_pool_cost);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Minimum pool cost set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set minimum pool cost.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_min_pool_cost(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  min_pool_cost);

/**
 * \brief Sets the ADA cost per UTXO byte in the protocol parameter update.
 *
 * This function sets the ADA cost per UTXO byte, which determines the amount of ADA charged per byte of UTXO
 * in the transaction. The cost is specified in lovelaces. If NULL is passed for ada_per_utxo_byte, it indicates
 * that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     ada_per_utxo_byte Pointer to the cost in lovelaces per UTXO byte.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the ADA cost per UTXO byte was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t proposed_ada_per_utxo_byte = 1;  // Example cost in lovelaces per byte
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_ada_per_utxo_byte(protocol_param_update, &proposed_ada_per_utxo_byte);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("ADA per UTXO byte cost set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set ADA per UTXO byte cost.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_ada_per_utxo_byte(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  ada_per_utxo_byte);

/**
 * \brief Sets the cost models for Plutus scripts in the protocol parameter update.
 *
 * This function sets the cost models associated with executing Plutus scripts. The cost models
 * define the computational cost parameters for various script operations. If NULL is passed for cost_models,
 * it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     cost_models Pointer to the cost models for Plutus scripts.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the cost models were successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_costmdls_t* proposed_cost_models = NULL;
 *
 * // Assume proposed_cost_models is initialized properly
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_cost_models(protocol_param_update, proposed_cost_models);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Cost models set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set cost models.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_cost_models(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_costmdls_t*              cost_models);

/**
 * \brief Sets the execution costs for Plutus scripts in the protocol parameter update.
 *
 * This function sets the execution costs associated with Plutus scripts, defining the
 * resource pricing for script execution. If NULL is passed for execution_costs,
 * it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     execution_costs Pointer to the execution costs for Plutus scripts.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the execution costs were successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_ex_unit_prices_t* proposed_execution_costs = NULL;
 *
 * // Assume proposed_execution_costs is initialized properly
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_execution_costs(protocol_param_update, proposed_execution_costs);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Execution costs set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set execution costs.\n");
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * cardano_ex_unit_prices_unref(&proposed_execution_costs);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_execution_costs(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_ex_unit_prices_t*        execution_costs);

/**
 * \brief Sets the maximum transaction execution units in the protocol parameter update.
 *
 * This function sets the maximum execution units that a transaction can consume.
 * If NULL is passed for max_tx_ex_units, it indicates that the update should not
 * propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     max_tx_ex_units Pointer to the maximum execution units for a transaction.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the maximum transaction execution units were successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_ex_units_t* proposed_max_tx_ex_units = NULL;
 *
 * // Assume proposed_max_tx_ex_units is initialized properly
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_max_tx_ex_units(protocol_param_update, proposed_max_tx_ex_units);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Maximum transaction execution units set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set maximum transaction execution units.\n");
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * cardano_ex_units_unref(&proposed_max_tx_ex_units);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_max_tx_ex_units(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_ex_units_t*              max_tx_ex_units);

/**
 * \brief Sets the maximum block execution units in the protocol parameter update.
 *
 * This function sets the maximum execution units that a block can consume.
 * If NULL is passed for max_block_ex_units, it indicates that the update should not
 * propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     max_block_ex_units Pointer to the maximum execution units for a block.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the maximum block execution units were successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_ex_units_t* proposed_max_block_ex_units = NULL;
 *
 * // Assume proposed_max_block_ex_units is initialized properly
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_max_block_ex_units(protocol_param_update, proposed_max_block_ex_units);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Maximum block execution units set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set maximum block execution units.\n");
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * cardano_ex_units_unref(&proposed_max_block_ex_units);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_max_block_ex_units(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_ex_units_t*              max_block_ex_units);

/**
 * \brief Sets the maximum value size in the protocol parameter update.
 *
 * This function sets the maximum serialized length (in bytes) of a multi-asset value (token bundle)
 * in a transaction output. If NULL is passed for max_value_size, it indicates that the update should not
 * propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     max_value_size Pointer to the maximum value size.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the maximum value size was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t proposed_max_value_size = 5000;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_max_value_size(protocol_param_update, &proposed_max_value_size);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Maximum value size set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set maximum value size.\n");
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_max_value_size(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  max_value_size);

/**
 * \brief Sets the collateral percentage in the protocol parameter update.
 *
 * This function sets the percentage of the total transaction fee that its collateral must (at minimum) cover.
 * If NULL is passed for collateral_percentage, it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     collateral_percentage Pointer to the collateral percentage.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the collateral percentage was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t proposed_collateral_percentage = 150;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_collateral_percentage(protocol_param_update, &proposed_collateral_percentage);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Collateral percentage set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set collateral percentage.\n");
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_collateral_percentage(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  collateral_percentage);

/**
 * \brief Sets the maximum number of collateral inputs in the protocol parameter update.
 *
 * This function sets the limit for the total number of collateral inputs allowed in a transaction.
 * If NULL is passed for max_collateral_inputs, it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     max_collateral_inputs Pointer to the maximum number of collateral inputs.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the maximum number of collateral inputs was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t proposed_max_collateral_inputs = 10;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_max_collateral_inputs(protocol_param_update, &proposed_max_collateral_inputs);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Max collateral inputs set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set max collateral inputs.\n");
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_max_collateral_inputs(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  max_collateral_inputs);

/**
 * \brief Sets the pool voting thresholds in the protocol parameter update.
 *
 * This function sets the SPO vote thresholds which must be met as a percentage of the stake held by all stake pools
 * to enact the different governance actions that must be ratified by them.
 * If NULL is passed for pool_voting_thresholds, it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     pool_voting_thresholds Pointer to the pool voting thresholds.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the pool voting thresholds were successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_pool_voting_thresholds_t* pool_voting_thresholds = NULL;
 *
 * // Assume protocol_param_update and pool_voting_thresholds are initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_pool_voting_thresholds(protocol_param_update, pool_voting_thresholds);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Pool voting thresholds set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set pool voting thresholds.\n");
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * cardano_pool_voting_thresholds_unref(&pool_voting_thresholds);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_pool_voting_thresholds(
  cardano_protocol_param_update_t*  protocol_param_update,
  cardano_pool_voting_thresholds_t* pool_voting_thresholds);

/**
 * \brief Sets the DRep voting thresholds in the protocol parameter update.
 *
 * This function sets the DRep vote thresholds that must be met as a percentage of active voting stake
 * to enact the different governance actions that must be ratified by them.
 * If NULL is passed for drep_voting_thresholds, it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     drep_voting_thresholds Pointer to the DRep voting thresholds.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the DRep voting thresholds were successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_drep_voting_thresholds_t* drep_voting_thresholds = NULL;
 *
 * // Assume protocol_param_update and drep_voting_thresholds are initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_drep_voting_thresholds(protocol_param_update, drep_voting_thresholds);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("DRep voting thresholds set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set DRep voting thresholds.\n");
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * cardano_drep_voting_thresholds_unref(&drep_voting_thresholds);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_drep_voting_thresholds(
  cardano_protocol_param_update_t*  protocol_param_update,
  cardano_drep_voting_thresholds_t* drep_voting_thresholds);

/**
 * \brief Sets the minimum committee size in the protocol parameter update.
 *
 * This function sets the minimum size of the constitutional committee.
 * If NULL is passed for min_committee_size, it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     min_committee_size Pointer to the minimum committee size.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the minimum committee size was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t min_committee_size = 5;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_min_committee_size(protocol_param_update, &min_committee_size);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Minimum committee size set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set minimum committee size.\n");
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_min_committee_size(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  min_committee_size);

/**
 * \brief Sets the committee term limit in the protocol parameter update.
 *
 * This function sets the term limit for the constitutional committee members.
 * If NULL is passed for committee_term_limit, it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     committee_term_limit Pointer to the committee term limit.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the committee term limit was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t committee_term_limit = 10;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_committee_term_limit(protocol_param_update, &committee_term_limit);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Committee term limit set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set committee term limit.\n");
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_committee_term_limit(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  committee_term_limit);

/**
 * \brief Sets the governance action validity period in the protocol parameter update.
 *
 * This function sets the validity period for governance actions. If NULL is passed for governance_action_validity_period,
 * it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     governance_action_validity_period Pointer to the governance action validity period.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the governance action validity period was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t governance_action_validity_period = 20;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_governance_action_validity_period(protocol_param_update, &governance_action_validity_period);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Governance action validity period set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set governance action validity period.\n");
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_governance_action_validity_period(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  governance_action_validity_period);

/**
 * \brief Sets the governance action deposit in the protocol parameter update.
 *
 * This function sets the deposit amount required for a governance action. If NULL is passed for governance_action_deposit,
 * it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     governance_action_deposit Pointer to the governance action deposit amount.
 *              If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the governance action deposit was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t governance_action_deposit = 500000;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_governance_action_deposit(protocol_param_update, &governance_action_deposit);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Governance action deposit set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set governance action deposit.\n");
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_governance_action_deposit(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  governance_action_deposit);

/**
 * \brief Sets the DRep deposit in the protocol parameter update.
 *
 * This function sets the deposit amount required for a DRep. If NULL is passed for drep_deposit,
 * it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     drep_deposit Pointer to the DRep deposit amount. If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the DRep deposit was successfully set, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t drep_deposit = 500000;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_drep_deposit(protocol_param_update, &drep_deposit);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("DRep deposit set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set DRep deposit.\n");
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_drep_deposit(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  drep_deposit);

/**
 * \brief Sets the DRep inactivity period in the protocol parameter update.
 *
 * This function sets the inactivity period for a DRep. If NULL is passed for drep_inactivity_period,
 * it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     drep_inactivity_period Pointer to the DRep inactivity period. If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the DRep inactivity period was successfully set, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * uint64_t drep_inactivity_period = 10;
 *
 * // Assume protocol_param_update is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_drep_inactivity_period(protocol_param_update, &drep_inactivity_period);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("DRep inactivity period set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set DRep inactivity period.\n");
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_drep_inactivity_period(
  cardano_protocol_param_update_t* protocol_param_update,
  const uint64_t*                  drep_inactivity_period);

/**
 * \brief Sets the reference scripts cost per byte in the protocol parameter update.
 *
 * This function sets the reference scripts cost per byte. If NULL is passed for ref_script_cost_per_byte,
 * it indicates that the update should not propose a change for this parameter.
 *
 * \param[in,out] protocol_param_update Pointer to the protocol parameter update object.
 * \param[in]     ref_script_cost_per_byte Pointer to the reference scripts cost per bytes. If NULL, it indicates no change is being proposed for this field.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the reference scripts cost per bytes was successfully set, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = NULL;
 * cardano_unit_interval_t* ref_script_cost_per_byte = NULL;
 *
 * // Assume ref_script_cost_per_byte is initialized properly
 *
 * cardano_error_t result = cardano_protocol_param_update_set_ref_script_cost_per_byte(protocol_param_update, ref_script_cost_per_byte);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Reference scripts cost per bytes set successfully.\n");
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to set reference scripts cost per bytes.\n");
 * }
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_protocol_param_update_set_ref_script_cost_per_byte(
  cardano_protocol_param_update_t* protocol_param_update,
  cardano_unit_interval_t*         ref_script_cost_per_byte);

/**
 * \brief Decrements the reference count of a cardano_protocol_param_update_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_protocol_param_update_t object
 * by decreasing its reference count. When the reference count reaches zero, the protocol_param_update is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] protocol_param_update A pointer to the pointer of the protocol_param_update object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_protocol_param_update_t* protocol_param_update = cardano_protocol_param_update_new(major, minor);
 *
 * // Perform operations with the protocol_param_update...
 *
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * // At this point, protocol_param_update is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_protocol_param_update_unref, the pointer to the \ref cardano_protocol_param_update_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_protocol_param_update_unref(cardano_protocol_param_update_t** protocol_param_update);

/**
 * \brief Increases the reference count of the cardano_protocol_param_update_t object.
 *
 * This function is used to manually increment the reference count of an cardano_protocol_param_update_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_protocol_param_update_unref.
 *
 * \param protocol_param_update A pointer to the cardano_protocol_param_update_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming protocol_param_update is a previously created protocol_param_update object
 *
 * cardano_protocol_param_update_ref(protocol_param_update);
 *
 * // Now protocol_param_update can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_protocol_param_update_ref there is a corresponding
 * call to \ref cardano_protocol_param_update_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_protocol_param_update_ref(cardano_protocol_param_update_t* protocol_param_update);

/**
 * \brief Retrieves the current reference count of the cardano_protocol_param_update_t object.
 *
 * This function returns the number of active references to an cardano_protocol_param_update_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_protocol_param_update_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param protocol_param_update A pointer to the cardano_protocol_param_update_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_protocol_param_update_t object. If the object
 * is properly managed (i.e., every \ref cardano_protocol_param_update_ref call is matched with a
 * \ref cardano_protocol_param_update_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming protocol_param_update is a previously created protocol_param_update object
 *
 * size_t ref_count = cardano_protocol_param_update_refcount(protocol_param_update);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_protocol_param_update_refcount(const cardano_protocol_param_update_t* protocol_param_update);

/**
 * \brief Sets the last error message for a given cardano_protocol_param_update_t object.
 *
 * Records an error message in the protocol_param_update's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] protocol_param_update A pointer to the \ref cardano_protocol_param_update_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the protocol_param_update's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_protocol_param_update_set_last_error(
  cardano_protocol_param_update_t* protocol_param_update,
  const char*                      message);

/**
 * \brief Retrieves the last error message recorded for a specific protocol_param_update.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_protocol_param_update_set_last_error for the given
 * protocol_param_update. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] protocol_param_update A pointer to the \ref cardano_protocol_param_update_t instance whose last error
 *                   message is to be retrieved. If the protocol_param_update is NULL, the function
 *                   returns a generic error message indicating the null protocol_param_update.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified protocol_param_update. If the protocol_param_update is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_protocol_param_update_set_last_error for the same protocol_param_update, or until
 *       the protocol_param_update is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_protocol_param_update_get_last_error(
  const cardano_protocol_param_update_t* protocol_param_update);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_PROTOCOL_PARAM_UPDATE_H