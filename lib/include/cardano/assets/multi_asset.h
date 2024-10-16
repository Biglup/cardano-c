/**
 * \file multi_asset.h
 *
 * \author angel.castillo
 * \date   Sep 06, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_MULTI_ASSET_H
#define BIGLUP_LABS_INCLUDE_CARDANO_MULTI_ASSET_H

/* INCLUDES ******************************************************************/

#include <cardano/assets/asset_id.h>
#include <cardano/assets/asset_name.h>
#include <cardano/assets/asset_name_map.h>
#include <cardano/assets/policy_id_list.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a collection of assets in the Cardano blockchain.
 *
 * This struct is used to represent a multi-asset, which can include various native tokens.
 * Each asset is identified by a unique combination of a policy ID and asset name, and the
 * struct manages quantities of these assets.
 */
typedef struct cardano_multi_asset_t cardano_multi_asset_t;

/**
 * \brief Creates and initializes a new instance of a Multi-Asset.
 *
 * This function allocates and initializes a new instance of a Multi-Asset,
 * which can hold various native tokens within the Cardano network.
 *
 * \param[out] multi_asset On successful initialization, this will point to a newly created
 *             cardano_multi_asset_t object. This object represents a "strong reference",
 *             meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the Multi-Asset is no longer needed, the caller must release it
 *             by calling \ref cardano_multi_asset_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the Multi-Asset was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_multi_asset_t* multi_asset = NULL;
 *
 * // Attempt to create a new Multi-Asset object
 * cardano_error_t result = cardano_multi_asset_new(&multi_asset);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the multi_asset
 *
 *   // Once done, ensure to clean up and release the multi_asset
 *   cardano_multi_asset_unref(&multi_asset);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_multi_asset_new(cardano_multi_asset_t** multi_asset);

/**
 * \brief Creates a \ref cardano_multi_asset_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_multi_asset_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a multi_asset.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] multi_asset A pointer to a pointer of \ref cardano_multi_asset_t that will be set to the address
 *                        of the newly created multi_asset object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the multi asset were successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_multi_asset_t object by calling
 *       \ref cardano_multi_asset_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_multi_asset_t* multi_asset = NULL;
 *
 * cardano_error_t result = cardano_multi_asset_from_cbor(reader, &multi_asset);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the multi_asset
 *
 *   // Once done, ensure to clean up and release the multi_asset
 *   cardano_multi_asset_unref(&multi_asset);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode multi_asset: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_multi_asset_from_cbor(cardano_cbor_reader_t* reader, cardano_multi_asset_t** multi_asset);

/**
 * \brief Serializes multi asset into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_multi_asset_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] multi_asset A constant pointer to the \ref cardano_multi_asset_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p multi_asset or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_multi_asset_t* multi_asset = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_multi_asset_to_cbor(multi_asset, writer);
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
 * cardano_multi_asset_unref(&multi_asset);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_multi_asset_to_cbor(
  const cardano_multi_asset_t* multi_asset,
  cardano_cbor_writer_t*       writer);

/**
 * \brief Retrieves the number of distinct policy IDs within a multi-asset container.
 *
 * This function counts the number of distinct policy IDs in a given \ref cardano_multi_asset_t object.
 * Each policy ID can have one or more asset names associated with it, but this function only counts the
 * unique policies.
 *
 * \param[in] multi_asset A constant pointer to an initialized \ref cardano_multi_asset_t object.
 *                        The pointer must not be NULL.
 *
 * \return The number of unique policy IDs contained within the multi_asset object. If the input is NULL,
 *         the function returns 0 as a safe guard. This helps in managing conditions where the
 *         multi-asset container might be empty or incorrectly initialized.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_multi_asset_t* multi_asset = ...; // Assume multi_asset is already initialized
 * size_t policy_count = cardano_multi_asset_get_policy_count(multi_asset);
 *
 * printf("The multi-asset container has %zu distinct policy IDs.\n", policy_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_multi_asset_get_policy_count(
  const cardano_multi_asset_t* multi_asset);

/**
 * \brief Inserts assets under a specific policy ID into a multi-asset container.
 *
 * This function adds a collection of assets, identified by their names, along with their respective quantities,
 * under a given policy ID into a \ref cardano_multi_asset_t object. If the policy ID already exists in the multi-asset,
 * the assets are merged with the existing ones under that policy.
 *
 * \param[in,out] multi_asset A pointer to an initialized \ref cardano_multi_asset_t object.
 *                            This object will be modified to include the new assets under the specified policy ID.
 *                            The pointer must not be NULL.
 * \param[in] policy_id A pointer to an initialized \ref cardano_blake2b_hash_t object representing the policy ID
 *                      under which the assets are grouped. The pointer must not be NULL.
 * \param[in] assets A pointer to an initialized \ref cardano_asset_name_map_t object containing the asset names
 *                   and their corresponding quantities to be inserted. The pointer must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the assets
 *         were successfully inserted, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * \remark If the key already exists in the multi asset, the value associated with the policy id will be updated.
 *
 * Usage Example:
 * \code{.c}
 * cardano_multi_asset_t* multi_asset = cardano_multi_asset_new(...); // Assume multi_asset is already initialized
 * cardano_blake2b_hash_t* policy_id = cardano_blake2b_hash_new(...); // Assume policy_id is initialized
 * cardano_asset_name_map_t* assets = cardano_asset_name_map_new(...); // Assume assets are prepared
 *
 * // Populate assets map
 * cardano_asset_name_t* asset_name = ...; // Assume asset_name is initialized
 * uint64_t quantity = 1000;
 * cardano_asset_name_map_insert(assets, asset_name, quantity);
 *
 * cardano_error_t result = cardano_multi_asset_insert_assets(multi_asset, policy_id, assets);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Assets successfully inserted under the policy ID.\n");
 * }
 * else
 * {
 *   printf("Failed to insert assets: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Cleanup resources
 * cardano_multi_asset_unref(&multi_asset);
 * cardano_blake2b_hash_unref(&policy_id);
 * cardano_asset_name_map_unref(&assets);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_multi_asset_insert_assets(
  cardano_multi_asset_t*    multi_asset,
  cardano_blake2b_hash_t*   policy_id,
  cardano_asset_name_map_t* assets);

/**
 * \brief Retrieves assets grouped under a specific policy ID from a multi-asset container.
 *
 * This function fetches the collection of assets associated with a given policy ID from a \ref cardano_multi_asset_t object.
 * It returns a map of asset names to their quantities.
 *
 * \param[in] multi_asset A constant pointer to an initialized \ref cardano_multi_asset_t object.
 *                        This object is queried to extract the assets associated with the specified policy ID.
 *                        The pointer must not be NULL.
 * \param[in] policy_id A pointer to an initialized \ref cardano_blake2b_hash_t object representing the policy ID
 *                      under which the assets are grouped. The pointer must not be NULL.
 * \param[out] assets On successful retrieval, this parameter will point to an initialized \ref cardano_asset_name_map_t object
 *                    containing the asset names and their corresponding quantities under the given policy ID.
 *                    The caller is responsible for managing the lifecycle of this object,
 *                    specifically, once the assets map is no longer needed, the caller must release it
 *                    by calling \ref cardano_asset_name_map_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the assets
 *         were successfully retrieved, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL, or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND
 *         if the specified policy ID does not exist in the multi-asset container.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_multi_asset_t* multi_asset = ...; // Assume multi_asset is already initialized
 * cardano_blake2b_hash_t* policy_id = cardano_blake2b_hash_new(...); // Assume policy_id is initialized
 * cardano_asset_name_map_t* assets = NULL;
 *
 * cardano_error_t result = cardano_multi_asset_get_assets(multi_asset, policy_id, &assets);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Process the retrieved assets
 *   // Iterate over assets map, access names and quantities
 * }
 * else
 * {
 *   printf("Failed to retrieve assets: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Cleanup resources
 * if (assets)
 * {
 *   cardano_asset_name_map_unref(&assets);
 * }
 *
 * cardano_blake2b_hash_unref(&policy_id);
 * cardano_multi_asset_unref(&multi_asset);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_multi_asset_get_assets(
  const cardano_multi_asset_t* multi_asset,
  cardano_blake2b_hash_t*      policy_id,
  cardano_asset_name_map_t**   assets);

/**
 * \brief Retrieves the quantity of a specific asset identified by a policy ID and asset name from a multi-asset container.
 *
 * This function fetches the quantity of an asset based on its policy ID and asset name from a \ref cardano_multi_asset_t object.
 * It provides a direct method to access the quantity of a specified asset within the broader multi-asset structure.
 *
 * \param[in] multi_asset A constant pointer to an initialized \ref cardano_multi_asset_t object from which the asset's quantity is to be retrieved.
 *                        The pointer must not be NULL.
 * \param[in] policy_id A pointer to an initialized \ref cardano_blake2b_hash_t object representing the policy ID under which the asset is grouped.
 *                      The pointer must not be NULL.
 * \param[in] asset A pointer to an initialized \ref cardano_asset_name_t object representing the name of the asset.
 *                  The pointer must not be NULL.
 * \param[out] value On successful retrieval, this will contain the quantity of the asset. The value is represented as a signed 64-bit integer,
 *                   which accounts for possible negative quantities due to burning or other operations affecting asset balance.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the asset quantity was successfully retrieved,
 *         or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL,
 *         or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if either the specified policy ID or asset name does not exist within the multi-asset container.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_multi_asset_t* multi_asset = ...; // Assume multi_asset is already initialized
 * cardano_blake2b_hash_t* policy_id = cardano_blake2b_hash_new(...); // Assume policy_id is initialized
 * cardano_asset_name_t* asset = cardano_asset_name_new(...); // Assume asset is initialized
 * int64_t value = 0;
 *
 * cardano_error_t result = cardano_multi_asset_get(multi_asset, policy_id, asset, &value);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Asset quantity: %ld\n", value);
 *   // Process the retrieved asset quantity
 * }
 * else
 * {
 *   printf("Failed to retrieve asset quantity: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Cleanup resources
 * cardano_blake2b_hash_unref(&policy_id);
 * cardano_asset_name_unref(&asset);
 * cardano_multi_asset_unref(&multi_asset);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_multi_asset_get(
  const cardano_multi_asset_t* multi_asset,
  cardano_blake2b_hash_t*      policy_id,
  cardano_asset_name_t*        asset,
  int64_t*                     value);

/**
 * \brief Retrieves the amount associated with a specific asset ID from a multi-asset object.
 *
 * The `cardano_multi_asset_get_with_id` function allows you to retrieve the amount of a specific asset within
 * a \ref cardano_multi_asset_t object by providing the asset’s unique identifier.
 *
 * \param[in] multi_asset A pointer to the \ref cardano_multi_asset_t object from which to retrieve the asset amount.
 * \param[in] id A pointer to the \ref cardano_asset_id_t representing the unique identifier of the asset.
 * \param[out] value A pointer to an int64_t where the amount of the specified asset will be stored if it exists.
 *                   If the asset is not present, the function will store zero in this location.
 *
 * \return \ref CARDANO_SUCCESS if the amount was successfully retrieved or zeroed out if the asset is absent,
 *         or an appropriate error code indicating failure (e.g., if the asset ID is invalid).
 *
 * \note This function does not modify the `multi_asset` object but requires a valid, non-null `id` and `value` pointer.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_multi_asset_t* multi_asset = ...;  // Multi-asset object containing various assets
 * cardano_asset_id_t* asset_id = ...;              // The asset ID of the asset to retrieve
 * int64_t amount = 0;
 *
 * cardano_error_t result = cardano_multi_asset_get_with_id(multi_asset, asset_id, &amount);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // 'amount' now holds the quantity of the specified asset, or zero if not present
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_multi_asset_get_with_id(
  const cardano_multi_asset_t* multi_asset,
  cardano_asset_id_t*          id,
  int64_t*                     value);

/**
 * \brief Sets the quantity of a specific asset identified by a policy ID and asset name in a multi-asset container.
 *
 * This function sets or updates the quantity of an asset within a \ref cardano_multi_asset_t object based on its policy ID and asset name.
 * It allows the manipulation of asset quantities, such as issuing new assets or adjusting existing quantities.
 *
 * \param[in] multi_asset A constant pointer to an initialized \ref cardano_multi_asset_t object to which the asset's quantity will be set or updated.
 *                        The pointer must not be NULL.
 * \param[in] policy_id A pointer to an initialized \ref cardano_blake2b_hash_t object representing the policy ID under which the asset is grouped.
 *                      The pointer must not be NULL.
 * \param[in] asset A pointer to an initialized \ref cardano_asset_name_t object representing the name of the asset.
 *                  The pointer must not be NULL.
 * \param[in] value The new quantity for the asset. This is a signed 64-bit integer, allowing for the subtraction or addition of asset quantities.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the asset quantity was successfully set,
 *         or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL,
 *         or \ref CARDANO_ERROR_ELEMENT_NOT_FOUND if either the specified policy ID or asset name does not exist within the multi-asset container.
 *
 * Usage Example:
 * \code{.c}
 * cardano_multi_asset_t* multi_asset = ...; // Assume multi_asset is already initialized
 * cardano_blake2b_hash_t* policy_id = cardano_blake2b_hash_new(...); // Assume policy_id is initialized
 * cardano_asset_name_t* asset = cardano_asset_name_new(...); // Assume asset is initialized
 * int64_t new_quantity = 1000; // Define a new quantity for the asset
 *
 * cardano_error_t result = cardano_multi_asset_set(multi_asset, policy_id, asset, new_quantity);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Asset quantity set successfully.\n");
 *   // Continue processing with the updated asset quantity
 * }
 * else
 * {
 *   printf("Failed to set asset quantity: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Cleanup resources
 * cardano_blake2b_hash_unref(&policy_id);
 * cardano_asset_name_unref(&asset);
 * cardano_multi_asset_unref(&multi_asset);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_multi_asset_set(
  cardano_multi_asset_t*  multi_asset,
  cardano_blake2b_hash_t* policy_id,
  cardano_asset_name_t*   asset,
  int64_t                 value);

/**
 * \brief Retrieves a list of all policy IDs from a multi-asset container.
 *
 * This function extracts all policy IDs (keys) from a given \ref cardano_multi_asset_t object and stores them in a list.
 * Each policy ID corresponds to a distinct set of assets grouped under that policy within the multi-asset structure.
 *
 * \param[in] multi_asset A constant pointer to an initialized \ref cardano_multi_asset_t object from which the policy IDs are to be retrieved.
 *                        The pointer must not be NULL.
 * \param[out] keys On successful execution, this will point to a newly created \ref cardano_policy_id_list_t object containing all the policy IDs.
 *                  The list is dynamically allocated and must be freed by the caller using \ref cardano_policy_id_list_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the policy IDs were successfully retrieved,
 *         or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the multi_asset pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_multi_asset_t* multi_asset = ...; // Assume multi_asset is already initialized
 * cardano_policy_id_list_t* policy_ids = NULL;
 *
 * cardano_error_t result = cardano_multi_asset_get_keys(multi_asset, &policy_ids);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Policy IDs retrieved successfully.\n");
 *   // Process the list of policy IDs
 *   cardano_policy_id_list_unref(&policy_ids);
 * }
 * else
 * {
 *   printf("Failed to retrieve policy IDs: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_multi_asset_get_keys(
  const cardano_multi_asset_t* multi_asset,
  cardano_policy_id_list_t**   keys);

/**
 * \brief Combines two multi-asset containers by adding the quantities of assets under each policy.
 *
 * This function adds the assets from two \ref cardano_multi_asset_t objects. It combines the quantities
 * of assets under each policy ID. If a policy ID exists in both objects, the assets under that policy are summed.
 * If a policy ID exists in only one of the objects, the assets under that policy are copied to the result.
 *
 * \param[in] lhs A constant pointer to the first \ref cardano_multi_asset_t object.
 * \param[in] rhs A constant pointer to the second \ref cardano_multi_asset_t object.
 * \param[out] result On successful execution, this will point to a newly created \ref cardano_multi_asset_t object
 *             that represents the combined result of both input multi-asset containers.
 *             The result is dynamically allocated and must be freed by the caller using \ref cardano_multi_asset_unref
 *             when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the assets were
 *         successfully combined, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_multi_asset_t* lhs = ...; // Assume lhs is already initialized with some assets
 * const cardano_multi_asset_t* rhs = ...; // Assume rhs is also initialized with some assets
 * cardano_multi_asset_t* result = NULL;
 *
 * cardano_error_t add_result = cardano_multi_asset_add(lhs, rhs, &result);
 * if (add_result == CARDANO_SUCCESS)
 * {
 *   // The assets have been successfully combined
 *   // Use the result
 *   cardano_multi_asset_unref(&result);
 * }
 * else
 * {
 *   printf("Failed to combine assets: %s\n", cardano_error_to_string(add_result));
 * }
 *
 * cardano_multi_asset_unref(&lhs);
 * cardano_multi_asset_unref(&rhs);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_multi_asset_add(
  const cardano_multi_asset_t* lhs,
  const cardano_multi_asset_t* rhs,
  cardano_multi_asset_t**      result);

/**
 * \brief Subtracts the quantities of assets under each policy from one multi-asset container to another.
 *
 * This function subtracts the assets from the second \ref cardano_multi_asset_t object (rhs) from the first
 * \ref cardano_multi_asset_t object (lhs). It subtracts the quantities of assets under each policy ID present
 * in both objects. If a policy ID exists only in the rhs, the value is copied as negative, effectively subtracting
 * it from the lhs. If a policy ID exists only in the lhs, the value is copied as is to the result.
 *
 * \param[in] lhs A constant pointer to the first \ref cardano_multi_asset_t object from which assets will be subtracted.
 * \param[in] rhs A constant pointer to the second \ref cardano_multi_asset_t object that provides the quantities to subtract.
 * \param[out] result On successful execution, this will point to a newly created \ref cardano_multi_asset_t object
 *             that represents the result of the subtraction.
 *             The result is dynamically allocated and must be freed by the caller using \ref cardano_multi_asset_unref
 *             when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the assets were
 *         successfully subtracted, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_multi_asset_t* lhs = ...; // Assume lhs is already initialized with some assets
 * const cardano_multi_asset_t* rhs = ...; // Assume rhs is also initialized with some assets to subtract
 * cardano_multi_asset_t* result = NULL;
 *
 * cardano_error_t subtract_result = cardano_multi_asset_subtract(lhs, rhs, &result);
 * if (subtract_result == CARDANO_SUCCESS)
 * {
 *   // The assets have been successfully subtracted
 *   // Use the result
 *   cardano_multi_asset_unref(&result);
 * }
 * else
 * {
 *   printf("Failed to subtract assets: %s\n", cardano_error_to_string(subtract_result));
 * }
 *
 * cardano_multi_asset_unref(&lhs);
 * cardano_multi_asset_unref(&rhs);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_multi_asset_subtract(
  const cardano_multi_asset_t* lhs,
  const cardano_multi_asset_t* rhs,
  cardano_multi_asset_t**      result);

/**
 * \brief Retrieves a multi-asset containing only assets with positive quantities from the given multi-asset.
 *
 * This function filters the provided \ref cardano_multi_asset_t object and creates a new multi-asset that includes
 * only those assets whose quantities are positive (greater than zero). Assets with zero or negative quantities are
 * excluded from the result.
 *
 * \param[in] multi_asset A pointer to an initialized \ref cardano_multi_asset_t object from which to extract assets with positive quantities.
 *                The pointer must not be NULL.
 * \param[out] result On successful execution, this will point to a newly created \ref cardano_multi_asset_t object
 *             containing only the assets with positive quantities from the input multi-asset.
 *             The result is dynamically allocated and must be freed by the caller using \ref cardano_multi_asset_unref
 *             when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the operation was
 *         successful, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_multi_asset_t* multi_asset = ...; // Assume lhs is already initialized with some assets
 * cardano_multi_asset_t* positive_assets = NULL;
 *
 * cardano_error_t result = cardano_multi_asset_get_positive(multi_asset, &positive_assets);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // positive_assets now contains only assets with positive quantities
 *   // Use positive_assets as needed
 *
 *   // When done, release the positive_assets
 *   cardano_multi_asset_unref(&positive_assets);
 * }
 * else
 * {
 *   printf("Failed to retrieve positive assets: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_multi_asset_get_positive(
  cardano_multi_asset_t*  multi_asset,
  cardano_multi_asset_t** result);

/**
 * \brief Retrieves a multi-asset containing only assets with negative quantities from the given multi-asset.
 *
 * This function filters the provided \ref cardano_multi_asset_t object and creates a new multi-asset that includes
 * only those assets whose quantities are negative (less than zero). Assets with zero or positive quantities are
 * excluded from the result.
 *
 * \param[in] multi_asset A pointer to an initialized \ref cardano_multi_asset_t object from which to extract assets with negative quantities.
 *                        The pointer must not be NULL.
 * \param[out] result On successful execution, this will point to a newly created \ref cardano_multi_asset_t object
 *             containing only the assets with negative quantities from the input multi-asset.
 *             The result is dynamically allocated and must be freed by the caller using \ref cardano_multi_asset_unref
 *             when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the operation was
 *         successful, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_multi_asset_t* multi_asset = ...; // Assume multi_asset is already initialized with some assets
 * cardano_multi_asset_t* negative_assets = NULL;
 *
 * cardano_error_t result = cardano_multi_asset_get_negative(multi_asset, &negative_assets);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // negative_assets now contains only assets with negative quantities
 *   // Use negative_assets as needed
 *
 *   // When done, release the negative_assets
 *   cardano_multi_asset_unref(&negative_assets);
 * }
 * else
 * {
 *   printf("Failed to retrieve negative assets: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_multi_asset_get_negative(
  cardano_multi_asset_t*  multi_asset,
  cardano_multi_asset_t** result);

/**
 * \brief Compares two multi asset objects for equality.
 *
 * This function compares two multi_asset objects to determine if they are equal.
 *
 * \param[in] lhs Pointer to the first multi asset object.
 * \param[in] rhs Pointer to the second multi asset object.
 *
 * \return \c true if the multi asset objects are equal, \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_multi_asset_t* multi_asset1 = NULL;
 * cardano_multi_asset_t* multi_asset2 = NULL;
 *
 * // Assume multi_asset1 and multi_asset2 are initialized properly
 *
 * bool are_equal = cardano_multi_asset_equals(multi_asset1, multi_asset2);
 *
 * if (are_equal)
 * {
 *   printf("The multi_asset objects are equal.\n");
 * }
 * else
 * {
 *   printf("The multi_asset objects are not equal.\n");
 * }
 *
 * // Clean up
 * cardano_multi_asset_unref(&multi_asset1);
 * cardano_multi_asset_unref(&multi_asset2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool
cardano_multi_asset_equals(const cardano_multi_asset_t* lhs, const cardano_multi_asset_t* rhs);

/**
 * \brief Decrements the reference count of a cardano_multi_asset_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_multi_asset_t object
 * by decreasing its reference count. When the reference count reaches zero, the multi_asset is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] multi_asset A pointer to the pointer of the multi_asset object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_multi_asset_t* multi_asset = cardano_multi_asset_new(major, minor);
 *
 * // Perform operations with the multi_asset...
 *
 * cardano_multi_asset_unref(&multi_asset);
 * // At this point, multi_asset is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_multi_asset_unref, the pointer to the \ref cardano_multi_asset_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_multi_asset_unref(cardano_multi_asset_t** multi_asset);

/**
 * \brief Increases the reference count of the cardano_multi_asset_t object.
 *
 * This function is used to manually increment the reference count of an cardano_multi_asset_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_multi_asset_unref.
 *
 * \param multi_asset A pointer to the cardano_multi_asset_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming multi_asset is a previously created multi_asset object
 *
 * cardano_multi_asset_ref(multi_asset);
 *
 * // Now multi_asset can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_multi_asset_ref there is a corresponding
 * call to \ref cardano_multi_asset_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_multi_asset_ref(cardano_multi_asset_t* multi_asset);

/**
 * \brief Retrieves the current reference count of the cardano_multi_asset_t object.
 *
 * This function returns the number of active references to an cardano_multi_asset_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_multi_asset_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param multi_asset A pointer to the cardano_multi_asset_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_multi_asset_t object. If the object
 * is properly managed (i.e., every \ref cardano_multi_asset_ref call is matched with a
 * \ref cardano_multi_asset_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming multi_asset is a previously created multi_asset object
 *
 * size_t ref_count = cardano_multi_asset_refcount(multi_asset);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_multi_asset_refcount(const cardano_multi_asset_t* multi_asset);

/**
 * \brief Sets the last error message for a given cardano_multi_asset_t object.
 *
 * Records an error message in the multi_asset's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] multi_asset A pointer to the \ref cardano_multi_asset_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the multi_asset's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_multi_asset_set_last_error(
  cardano_multi_asset_t* multi_asset,
  const char*            message);

/**
 * \brief Retrieves the last error message recorded for a specific multi_asset.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_multi_asset_set_last_error for the given
 * multi_asset. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] multi_asset A pointer to the \ref cardano_multi_asset_t instance whose last error
 *                   message is to be retrieved. If the multi_asset is NULL, the function
 *                   returns a generic error message indicating the null multi_asset.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified multi_asset. If the multi_asset is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_multi_asset_set_last_error for the same multi_asset, or until
 *       the multi_asset is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_multi_asset_get_last_error(
  const cardano_multi_asset_t* multi_asset);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_MULTI_ASSET_H