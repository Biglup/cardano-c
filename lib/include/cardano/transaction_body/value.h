/**
 * \file value.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_VALUE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_VALUE_H

/* INCLUDES ******************************************************************/

#include <cardano/assets/asset_id_map.h>
#include <cardano/assets/multi_asset.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A Value object encapsulates the quantity of assets of different types,
 * including ADA (Cardano's native cryptocurrency) expressed in lovelace,
 * where 1 ADA = 1,000,000 lovelace, and other native tokens. Each key in the
 * tokens object is a unique identifier for an asset, and the corresponding
 * value is the quantity of that asset.
 */
typedef struct cardano_value_t cardano_value_t;

/**
 * \brief Creates and initializes a new instance of the Cardano Value object.
 *
 * This function allocates and initializes a new instance of the \ref cardano_value_t object,
 * which encapsulates the quantity of ADA (in lovelaces) and other native tokens as part of a transaction.
 * Each asset in the multi-asset structure is indexed by a unique policy ID and asset name.
 *
 * \param[in] coin The quantity of ADA expressed in lovelaces, where 1 ADA = 1,000,000 lovelace.
 * \param[in] assets A pointer to a \ref cardano_multi_asset_t object representing the other native tokens associated with the value.
 *                   This parameter can be NULL if there are no additional native tokens to associate.
 * \param[out] value On successful initialization, this will point to a newly created \ref cardano_value_t object.
 *                   This object represents a "strong reference", meaning that it is fully initialized and ready for use.
 *                   The caller is responsible for managing the lifecycle of this object.
 *                   Specifically, once the Value is no longer needed, the caller must release it
 *                   by calling \ref cardano_value_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the Value was successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * int64_t coin = 1000000; // Equivalent to 1 ADA
 * cardano_multi_asset_t* assets = cardano_multi_asset_new(...); // Assume assets are already initialized
 * cardano_value_t* value = NULL;
 *
 * // Attempt to create a new Cardano Value object
 * cardano_error_t result = cardano_value_new(coin, assets, &value);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the value
 *
 *   // Once done, ensure to clean up and release the value
 *   cardano_value_unref(&value);
 * }
 * else
 * {
 *   printf("Failed to create the value object: %s\n", cardano_error_to_string(result));
 * }
 *
 * cardano_multi_asset_unref(&assets); // Cleanup the multi-asset object
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_value_new(
  int64_t                coin,
  cardano_multi_asset_t* assets,
  cardano_value_t**      value);

/**
 * \brief Creates a new `cardano_value_t` object initialized to zero.
 *
 * The `cardano_value_new_zero` function allocates and returns a new `cardano_value_t` object
 * that represents a value initialized to zero for both ADA and any associated assets.
 *
 * \return A pointer to the newly created `cardano_value_t` object with zeroed values, or `NULL`
 * if memory allocation fails.
 *
 * \note The caller is responsible for managing the memory of the returned `cardano_value_t` object
 * and must call `cardano_value_unref` to properly release it when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_value_t* value = cardano_value_new_zero();
 *
 * if (value != NULL)
 * {
 *   // Successfully created a new zeroed value
 *   // Use the `value` as needed
 *
 *   // Release the value object when done
 *   cardano_value_unref(&value);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_value_t*
cardano_value_new_zero(void);

/**
 * \brief Creates a new value instance from a specified amount of Lovelace.
 *
 * This function initializes a new `cardano_value_t` instance with a specific amount of Lovelace, allowing the caller
 * to create a value that represents only ADA (without any multi-asset components).
 *
 * \param[in] lovelace The amount of Lovelace (smallest unit of ADA) to set for the new value instance.
 *
 * \returns A pointer to a newly created \ref cardano_value_t instance containing the specified amount of Lovelace,
 *          or `NULL` if memory allocation fails.
 *
 * Usage Example:
 * \code{.c}
 * int64_t lovelace_amount = 1000000; // 1 ADA in Lovelace
 * cardano_value_t* value = cardano_value_new_from_coin(lovelace_amount);
 *
 * if (value != NULL)
 * {
 *     // Successfully created a value instance with 1 ADA
 * }
 *
 * cardano_value_unref(&value);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_value_t*
cardano_value_new_from_coin(int64_t lovelace);

/**
 * \brief Creates a \ref cardano_value_t from an asset map.
 *
 * This function creates a \ref cardano_value_t object from a given asset map (\ref cardano_asset_id_map_t).
 * The resulting value represents the sum of the assets contained in the map, where each entry in the map is
 * an asset ID and its corresponding quantity.
 *
 * \param[in] asset_map A pointer to an initialized \ref cardano_asset_id_map_t object containing the assets.
 *                      This parameter must not be NULL.
 * \param[out] value On successful creation, this will point to the newly created \ref cardano_value_t object.
 *                   The caller is responsible for managing the lifecycle of the object and must call
 *                   \ref cardano_value_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if
 *         the value was successfully created, or an appropriate error code if an error occurred.
 *
 * \note The caller is responsible for managing the lifecycle of both the \ref cardano_value_t object and the \ref cardano_asset_id_map_t.
 *
 * Usage Example:
 * \code{.c}
 * cardano_asset_id_map_t* asset_map = ...; // Assume asset_map is initialized
 * cardano_value_t* value = NULL;
 *
 * cardano_error_t result = cardano_value_from_asset_map(asset_map, &value);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Value successfully created from asset map.\n");
 * }
 * else
 * {
 *   printf("Failed to create value from asset map.\n");
 * }
 *
 * // Clean up when done
 * cardano_asset_id_map_unref(&asset_map);
 * cardano_value_unref(&value);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_value_from_asset_map(
  cardano_asset_id_map_t* asset_map,
  cardano_value_t**       value);

/**
 * \brief Creates a \ref cardano_value_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_value_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a value.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] value A pointer to a pointer of \ref cardano_value_t that will be set to the address
 *                        of the newly created value object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the value were successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_value_t object by calling
 *       \ref cardano_value_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_value_t* value = NULL;
 *
 * cardano_error_t result = cardano_value_from_cbor(reader, &value);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the value
 *
 *   // Once done, ensure to clean up and release the value
 *   cardano_value_unref(&value);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode value: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_value_from_cbor(cardano_cbor_reader_t* reader, cardano_value_t** value);

/**
 * \brief Serializes value into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_value_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] value A constant pointer to the \ref cardano_value_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p value or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_value_t* value = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_value_to_cbor(value, writer);
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
 * cardano_value_unref(&value);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_value_to_cbor(
  const cardano_value_t* value,
  cardano_cbor_writer_t* writer);

/**
 * \brief Retrieves the multi-asset object associated with a value.
 *
 * This function fetches the multi-asset structure from a given \ref cardano_value_t object. The multi-asset structure contains
 * various native tokens included in the value.
 *
 * \param[in] value A pointer to an initialized \ref cardano_value_t object.
 *
 * \return A pointer to the \ref cardano_multi_asset_t object representing the collection of native tokens. If the value pointer is NULL or does not contain any multi-assets, this function returns NULL.
 *         Note that the returned multi-asset object is a new reference and must be released using \ref cardano_multi_asset_unref
 *         when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_value_t* value = ...; // Assume value is already initialized
 * cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(value);
 *
 * if (multi_asset != NULL)
 * {
 *   // Process the multi-asset
 *   // Once done, ensure to clean up and release the multi_asset
 *   cardano_multi_asset_unref(&multi_asset);
 * }
 * else
 * {
 *   printf("No multi-asset associated with this value or uninitialized value.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_multi_asset_t* cardano_value_get_multi_asset(cardano_value_t* value);

/**
 * \brief Sets the multi-asset component for a Cardano value object.
 *
 * This function assigns a new multi-asset collection to a \ref cardano_value_t object.
 * The multi-asset structure encapsulates various native tokens to be associated with the value.
 *
 * \param[in,out] value A pointer to an initialized \ref cardano_value_t object to which the multi-asset collection will be set.
 * \param[in] assets A pointer to an initialized \ref cardano_multi_asset_t object representing the collection of native tokens.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the multi-asset
 *         was successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * \note This function increases the reference count of the multi-asset object; therefore, the caller retains ownership of their respective references.
 *       It is the caller's responsibility to release their reference to the multi-asset when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_value_t* value = ...; // Assume value is already initialized
 * cardano_multi_asset_t* assets = ...; // Assume assets is initialized
 *
 * cardano_error_t result = cardano_value_set_multi_asset(value, assets);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The multi-asset is now set for the value
 * }
 * else
 * {
 *   printf("Failed to set the multi-asset.\n");
 * }
 *
 * // Clean up resources
 * cardano_multi_asset_unref(&assets);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_value_set_multi_asset(cardano_value_t* value, cardano_multi_asset_t* assets);

/**
 * \brief Retrieves the coin amount (in lovelaces) from a Cardano value object.
 *
 * This function fetches the ADA amount, expressed in lovelaces, from a given \ref cardano_value_t object.
 * One ADA equals 1,000,000 lovelaces. This function is used to retrieve the amount of ADA, excluding any
 * native tokens that might be part of the value.
 *
 * \param[in] value A constant pointer to an initialized \ref cardano_value_t object.
 *
 * \return The amount of ADA in lovelaces contained in the value object. If the input is NULL, the function
 *         will return 0, which should be handled appropriately by the caller to distinguish from a genuine
 *         coin value of 0 lovelaces.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_value_t* value = ...; // Assume value is already initialized
 *
 * int64_t coin = cardano_value_get_coin(value);
 * printf("Coin amount: %llu lovelaces\n", coin);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT int64_t cardano_value_get_coin(const cardano_value_t* value);

/**
 * \brief Sets the coin amount (in lovelaces) for a Cardano value object.
 *
 * This function assigns a coin amount, expressed in lovelaces, to a \ref cardano_value_t object. One ADA equals 1,000,000 lovelaces.
 * This amount specifies the quantity of ADA, excluding any native tokens, that will be set in the value object.
 *
 * \param[in,out] value A pointer to an initialized \ref cardano_value_t object to which the coin amount will be set.
 * \param[in] coin The amount of ADA in lovelaces to set.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the coin amount was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the value pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_value_t* value = ...; // Assume value is already initialized
 * int64_t coin = 1000000; // Set 1 ADA
 *
 * cardano_error_t result = cardano_value_set_coin(value, coin);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Coin amount set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set the coin amount.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_value_set_coin(cardano_value_t* value, int64_t coin);

/**
 * \brief Adds a specified coin amount to a Cardano value object.
 *
 * This function increments the coin amount (expressed in lovelaces) in a \ref cardano_value_t object by a specified value.
 * The addition is performed in place, modifying the existing value object.
 *
 * \param[in,out] value A pointer to an initialized \ref cardano_value_t object whose coin amount will be increased.
 *                      The pointer must not be NULL.
 * \param[in] coin The amount of coin (in lovelaces) to add to the value object.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the coin amount was
 *         successfully added, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the value pointer is NULL, or \ref CARDANO_ERROR_INTEGER_OVERFLOW if the addition results in an overflow.
 *
 * Usage Example:
 * \code{.c}
 * cardano_value_t* value = ...; // Assume value is already initialized
 * int64_t coin_to_add = 500000; // Add 0.5 ADA (since 1 ADA = 1,000,000 lovelaces)
 *
 * cardano_error_t result = cardano_value_add_coin(value, coin_to_add);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Coin amount added successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to add the coin amount: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 *
 * \note Be cautious of potential overflow when adding large coin amounts. The function will return \ref CARDANO_ERROR_INTEGER_OVERFLOW
 *       if the new coin amount exceeds the maximum value representable by a 64-bit unsigned integer.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_value_add_coin(cardano_value_t* value, int64_t coin);

/**
 * \brief Subtracts a specified coin amount from a Cardano value object.
 *
 * This function decrements the coin amount (expressed in lovelaces) in a \ref cardano_value_t object by a specified value.
 * The subtraction is performed in place, modifying the existing value object.
 *
 * \param[in,out] value A pointer to an initialized \ref cardano_value_t object whose coin amount will be decreased.
 *                      The pointer must not be NULL.
 * \param[in] coin The amount of coin (in lovelaces) to subtract from the value object.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the coin amount was
 *         successfully subtracted, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the value pointer is NULL, or \ref CARDANO_ERROR_INTEGER_UNDERFLOW if the subtraction results in a negative coin amount.
 *
 * Usage Example:
 * \code{.c}
 * cardano_value_t* value = ...; // Assume value is already initialized
 * int64_t coin_to_subtract = 500000; // Subtract 0.5 ADA (since 1 ADA = 1,000,000 lovelaces)
 *
 * cardano_error_t result = cardano_value_subtract_coin(value, coin_to_subtract);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Coin amount subtracted successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to subtract the coin amount: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 *
 * \note Be cautious of potential underflow when subtracting coin amounts. The function will return \ref CARDANO_ERROR_INTEGER_UNDERFLOW
 *       if the subtraction results in a negative coin amount, as coin amounts cannot be negative.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_value_subtract_coin(cardano_value_t* value, int64_t coin);

/**
 * \brief Adds a specified multi-asset collection to a Cardano value object.
 *
 * This function increments the multi-asset component in a \ref cardano_value_t object by adding the quantities
 * of assets from a given \ref cardano_multi_asset_t object. The addition is performed in place, modifying the
 * existing value object. If an asset exists in both the value and the multi-asset being added, their quantities
 * are summed. Assets present only in the multi-asset being added are included in the value.
 *
 * \param[in,out] value A pointer to an initialized \ref cardano_value_t object whose multi-asset component will be increased.
 *                      The pointer must not be NULL.
 * \param[in] multi_asset A pointer to an initialized \ref cardano_multi_asset_t object containing the assets to add to the value.
 *                        The pointer must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the multi-asset
 *         was successfully added, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if any of the input pointers are NULL.
 *
 * \note The function handles the reference counting for the multi-asset components appropriately. The caller retains
 *       ownership of the provided \p multi_asset and is responsible for releasing it when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_value_t* value = ...; // Assume value is already initialized
 * cardano_multi_asset_t* multi_asset_to_add = ...; // Assume multi_asset_to_add is initialized
 *
 * cardano_error_t result = cardano_value_add_multi_asset(value, multi_asset_to_add);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Multi-asset added successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to add multi-asset: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Cleanup resources if needed
 * cardano_multi_asset_unref(&multi_asset_to_add);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_value_add_multi_asset(cardano_value_t* value, cardano_multi_asset_t* multi_asset);

/**
 * \brief Subtracts a specified multi-asset collection from a Cardano value object.
 *
 * This function decrements the multi-asset component in a \ref cardano_value_t object by subtracting the quantities
 * of assets from a given \ref cardano_multi_asset_t object. The subtraction is performed in place, modifying the
 * existing value object.
 *
 * \param[in,out] value A pointer to an initialized \ref cardano_value_t object whose multi-asset component will be decreased.
 *                      The pointer must not be NULL.
 * \param[in] multi_asset A pointer to an initialized \ref cardano_multi_asset_t object containing the assets to subtract from the value.
 *                        The pointer must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the multi-asset
 *         was successfully subtracted, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if any of the input pointers are NULL.
 *
 * \note The function handles the reference counting for the multi-asset components appropriately. The caller retains
 *       ownership of the provided \p multi_asset and is responsible for releasing it when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_value_t* value = ...; // Assume value is already initialized with some assets
 * cardano_multi_asset_t* multi_asset_to_subtract = ...; // Assume multi_asset_to_subtract is initialized
 *
 * cardano_error_t result = cardano_value_subtract_multi_asset(value, multi_asset_to_subtract);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Multi-asset subtracted successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to subtract multi-asset: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Cleanup resources if needed
 * cardano_multi_asset_unref(&multi_asset_to_subtract);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_value_subtract_multi_asset(cardano_value_t* value, cardano_multi_asset_t* multi_asset);

/**
 * \brief Adds an asset to the specified value instance.
 *
 * This function adds a specific asset (identified by a policy ID and asset name) and quantity to an existing
 * `cardano_value_t` instance. This enables multi-asset representation within the value.
 *
 * \param[in,out] value       A pointer to the \ref cardano_value_t instance to which the asset will be added.
 *                            Must be a valid instance.
 * \param[in]     policy_id   The policy ID (as a \ref cardano_blake2b_hash_t) identifying the asset's issuance policy.
 * \param[in]     asset_name  The name of the asset within the policy (as a \ref cardano_asset_name_t).
 * \param[in]     quantity    The amount of the asset to add. Positive values increase, negative values decrease
 *                            the asset quantity within the value instance.
 *
 * \returns \ref CARDANO_SUCCESS if the asset was successfully added to the value, or an error code if the operation fails.
 *
 * Usage Example:
 * \code{.c}
 * cardano_value_t* value = cardano_value_new_zero();
 * cardano_blake2b_hash_t* policy_id = ...;  // Initialized policy ID for the asset
 * cardano_asset_name_t* asset_name = ...;   // Initialized asset name
 * int64_t quantity = 500;                   // Quantity of the asset to add
 *
 * cardano_error_t result = cardano_value_add_asset(value, policy_id, asset_name, quantity);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *     // Asset added successfully
 * }
 *
 * // Clean up resources
 * cardano_value_unref(&value);
 * cardano_blake2b_hash_unref(&policy_id);
 * cardano_asset_name_unref(&asset_name);
 * \endcode
 *
 * \note The caller is responsible for ensuring that `policy_id` and `asset_name` are valid. Any memory allocated for
 *       these inputs should be freed appropriately when they are no longer needed.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_value_add_asset(
  cardano_value_t*        value,
  cardano_blake2b_hash_t* policy_id,
  cardano_asset_name_t*   asset_name,
  int64_t                 quantity);

/**
 * \brief Adds an asset to the specified value instance using hex-encoded identifiers.
 *
 * This function allows you to add a specific asset (identified by a hex-encoded policy ID and asset name)
 * and quantity to an existing `cardano_value_t` instance. This enables multi-asset representation within the value.
 *
 * \param[in,out] value              A pointer to the \ref cardano_value_t instance to which the asset will be added.
 *                                   Must be a valid instance.
 * \param[in]     policy_id_hex      A hex-encoded string representing the policy ID that uniquely identifies the asset's issuance policy.
 * \param[in]     policy_id_hex_len  The length of the hex-encoded policy ID string.
 * \param[in]     asset_name_hex     A hex-encoded string representing the asset's name within the policy.
 * \param[in]     asset_name_hex_len The length of the hex-encoded asset name string.
 * \param[in]     quantity           The amount of the asset to add. Positive values increase, negative values decrease
 *                                   the asset quantity within the value instance.
 *
 * \returns \ref CARDANO_SUCCESS if the asset was successfully added to the value, or an error code indicating failure.
 *
 * Usage Example:
 * \code{.c}
 * cardano_value_t* value = cardano_value_new_zero();
 * const char* policy_id_hex = "a0b1c2...";  // Hex string of the policy ID
 * size_t policy_id_hex_len = strlen(policy_id_hex);
 * const char* asset_name_hex = "abcd1234...";  // Hex string of the asset name
 * size_t asset_name_hex_len = strlen(asset_name_hex);
 * int64_t quantity = 500;  // Quantity of the asset to add
 *
 * cardano_error_t result = cardano_value_add_asset_ex(value, policy_id_hex, policy_id_hex_len, asset_name_hex, asset_name_hex_len, quantity);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *     // Asset added successfully
 * }
 *
 * // Clean up resources
 * cardano_value_unref(&value);
 * \endcode
 *
 * \note This function interprets `policy_id_hex` and `asset_name_hex` as hex-encoded strings, which are converted internally.
 *       It is the caller's responsibility to ensure these strings are correctly formatted and freed if they are dynamically allocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_value_add_asset_ex(
  cardano_value_t* value,
  const char*      policy_id_hex,
  size_t           policy_id_hex_len,
  const char*      asset_name_hex,
  size_t           asset_name_hex_len,
  int64_t          quantity);

/**
 * \brief Adds an asset to the specified value instance using an asset ID.
 *
 * This function allows you to add a specific asset, identified by its `cardano_asset_id_t`, and a quantity to
 * an existing `cardano_value_t` instance. This enables the representation of multi-assets within a single value.
 *
 * \param[in,out] value     A pointer to the \ref cardano_value_t instance to which the asset will be added.
 *                          Must be a valid instance.
 * \param[in]     asset_id  A pointer to a \ref cardano_asset_id_t representing the unique identifier of the asset,
 *                          including its policy ID and asset name.
 * \param[in]     quantity  The quantity of the asset to add. Positive values increase, and negative values decrease
 *                          the asset quantity within the value instance.
 *
 * \returns \ref CARDANO_SUCCESS if the asset was successfully added to the value, or an error code indicating failure.
 *
 * Usage Example:
 * \code{.c}
 * cardano_value_t* value = cardano_value_new_zero();
 * cardano_asset_id_t* asset_id = ...;  // Initialized asset ID
 * int64_t quantity = 500;  // Quantity of the asset to add
 *
 * cardano_error_t result = cardano_value_add_asset_with_id(value, asset_id, quantity);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Asset added successfully
 * }
 *
 * // Clean up resources
 * cardano_asset_id_unref(&asset_id);
 * cardano_value_unref(&value);
 * \endcode
 *
 * \note The caller must ensure the `cardano_asset_id_t` provided is valid. Proper memory management of the `cardano_value_t`
 *       instance is required, and it should be freed using `cardano_value_unref` when no longer needed.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_value_add_asset_with_id(
  cardano_value_t*    value,
  cardano_asset_id_t* asset_id,
  int64_t             quantity);

/**
 * \brief Adds an asset to the specified value instance using a hexadecimal asset ID.
 *
 * This function allows you to add a specific asset, identified by its asset ID in hexadecimal form,
 * and a quantity to an existing `cardano_value_t` instance. It enables representing multi-assets within a single value.
 *
 * \param[in,out] value          A pointer to the \ref cardano_value_t instance to which the asset will be added.
 *                               Must be a valid instance.
 * \param[in]     asset_id_hex   A pointer to a hexadecimal string representing the asset ID, which includes
 *                               the policy ID and asset name in hex format.
 * \param[in]     asset_id_hex_len The length of the hexadecimal string for the asset ID.
 * \param[in]     quantity       The quantity of the asset to add. Positive values increase and negative values decrease
 *                               the asset quantity within the value instance.
 *
 * \returns \ref CARDANO_SUCCESS if the asset was successfully added to the value, or an error code indicating failure.
 *
 * Usage Example:
 * \code{.c}
 * cardano_value_t* value = cardano_value_new_zero();
 * const char* asset_id_hex = "abcdef1234567890";  // Example asset ID in hexadecimal
 * size_t asset_id_hex_len = strlen(asset_id_hex);
 * int64_t quantity = 500;  // Quantity of the asset to add
 *
 * cardano_error_t result = cardano_value_add_asset_with_id_ex(value, asset_id_hex, asset_id_hex_len, quantity);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *     // Asset added successfully
 * }
 *
 * // Clean up resources
 * cardano_value_unref(&value);
 * \endcode
 *
 * \note The caller must ensure the `asset_id_hex` is a valid hexadecimal representation of the asset ID. Proper memory management
 *       of the `cardano_value_t` instance is required, and it should be freed using `cardano_value_unref` when no longer needed.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_value_add_asset_with_id_ex(
  cardano_value_t* value,
  const char*      asset_id_hex,
  size_t           asset_id_hex_len,
  int64_t          quantity);

/**
 * \brief Combines two Cardano value objects by adding their coin amounts and multi-asset components.
 *
 * This function adds the contents of two \ref cardano_value_t objects, summing their coin amounts (in lovelaces)
 * and combining their multi-asset collections. The addition is performed element-wise for both the coin amounts
 * and the assets within their multi-asset components. If an asset exists in both values, their quantities are summed. Assets
 * present only in one of the values are included in the result as is.
 *
 * \param[in] lhs A constant pointer to the first \ref cardano_value_t object.
 *                This value represents the left-hand side operand in the addition.
 *                The pointer must not be NULL.
 * \param[in] rhs A constant pointer to the second \ref cardano_value_t object.
 *                This value represents the right-hand side operand in the addition.
 *                The pointer must not be NULL.
 * \param[out] result On successful execution, this will point to a newly created \ref cardano_value_t object
 *             that represents the combined result of both input values.
 *             The result is dynamically allocated and must be freed by the caller using \ref cardano_value_unref
 *             when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         Returns \ref CARDANO_SUCCESS if the values were successfully added,
 *         or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *         or \ref CARDANO_ERROR_INTEGER_OVERFLOW if the addition results in an overflow for the coin amount.
 *
 * \note This function handles internal reference counting for the multi-asset components appropriately.
 *       The caller retains ownership of the provided \p lhs and \p rhs and is responsible for releasing them
 *       when they are no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_value_t* lhs = ...; // Assume lhs is already initialized
 * const cardano_value_t* rhs = ...; // Assume rhs is also initialized
 * cardano_value_t* result = NULL;
 *
 * cardano_error_t add_result = cardano_value_add(lhs, rhs, &result);
 *
 * if (add_result == CARDANO_SUCCESS)
 * {
 *   // The values have been successfully added
 *   // Use the result as needed
 *
 *   // When done, release the result
 *   cardano_value_unref(&result);
 * }
 * else
 * {
 *   printf("Failed to add values: %s\n", cardano_error_to_string(add_result));
 * }
 *
 * // Cleanup resources
 * cardano_value_unref(&lhs);
 * cardano_value_unref(&rhs);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_value_add(cardano_value_t* lhs, cardano_value_t* rhs, cardano_value_t** result);

/**
 * \brief Subtracts one Cardano value from another, resulting in a new value.
 *
 * This function subtracts the contents of the second \ref cardano_value_t object (rhs) from the first \ref cardano_value_t object (lhs),
 * producing a new \ref cardano_value_t object as the result. The subtraction is performed element-wise for both the coin amounts and the assets within their multi-asset components.
 *
 * \param[in] lhs A constant pointer to the \ref cardano_value_t object to subtract from.
 *                This value represents the left-hand side operand in the subtraction.
 *                The pointer must not be NULL.
 * \param[in] rhs A constant pointer to the \ref cardano_value_t object to subtract.
 *                This value represents the right-hand side operand in the subtraction.
 *                The pointer must not be NULL.
 * \param[out] result On successful execution, this will point to a newly created \ref cardano_value_t object
 *             that represents the result of the subtraction.
 *             The result is dynamically allocated and must be freed by the caller using \ref cardano_value_unref
 *             when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation.
 *         Returns \ref CARDANO_SUCCESS if the values were successfully subtracted,
 *         or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL,
 *         or \ref CARDANO_ERROR_INTEGER_UNDERFLOW if the subtraction results in a negative coin amount.
 *
 * \note This function handles internal reference counting for the multi-asset components appropriately.
 *       The caller retains ownership of the provided \p lhs and \p rhs and is responsible for releasing them
 *       when they are no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_value_t* lhs = ...; // Assume lhs is already initialized
 * const cardano_value_t* rhs = ...; // Assume rhs is also initialized
 * cardano_value_t* result = NULL;
 *
 * cardano_error_t subtract_result = cardano_value_subtract(lhs, rhs, &result);
 * if (subtract_result == CARDANO_SUCCESS)
 * {
 *   // The values have been successfully subtracted
 *   // Use the result as needed
 *
 *   // When done, release the result
 *   cardano_value_unref(&result);
 * }
 * else
 * {
 *   printf("Failed to subtract values: %s\n", cardano_error_to_string(subtract_result));
 * }
 *
 * // Cleanup resources
 * cardano_value_unref(&lhs);
 * cardano_value_unref(&rhs);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_value_subtract(cardano_value_t* lhs, cardano_value_t* rhs, cardano_value_t** result);

/**
 * \brief Retrieves the list of intersecting assets between two Cardano value objects.
 *
 * This function calculates the assets that are present in both the left-hand side (lhs) and
 * right-hand side (rhs) \ref cardano_value_t objects. The intersection refers to assets that exist in both value sets.
 *
 * \param[in] lhs A pointer to the first \ref cardano_value_t object, representing the left-hand side value.
 *                This value must not be NULL and should be initialized.
 * \param[in] rhs A pointer to the second \ref cardano_value_t object, representing the right-hand side value.
 *                This value must not be NULL and should be initialized.
 * \param[out] result On successful execution, this will point to a newly created \ref cardano_asset_id_list_t object
 *             containing the intersecting assets between the two value objects. If there are no intersecting assets,
 *             the list will be empty. If any input is NULL, the function will return an error and the result will remain uninitialized.
 *             The caller is responsible for managing the lifecycle of the resulting list, specifically releasing it
 *             by calling \ref cardano_asset_id_list_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the operation was successful,
 *         or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_value_t* lhs = ...; // Assume lhs is initialized with some assets
 * cardano_value_t* rhs = ...; // Assume rhs is initialized with some assets
 * cardano_asset_id_list_t* intersecting_assets = NULL;
 *
 * cardano_error_t result = cardano_value_get_intersection(lhs, rhs, &intersecting_assets);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Process the list of intersecting assets
 *   for (size_t i = 0; i < cardano_asset_id_list_get_length(intersecting_assets); ++i)
 *   {
 *     cardano_asset_id_t* asset_id = NULL;
 *     cardano_asset_id_list_get(intersecting_assets, i, &asset_id);
 *     // Use the asset_id
 *     cardano_asset_id_unref(&asset_id);
 *   }
 *
 *   // Ensure to release the list of intersecting assets when done
 *   cardano_asset_id_list_unref(&intersecting_assets);
 * }
 * else
 * {
 *   printf("Failed to calculate intersection: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_value_get_intersection(cardano_value_t* lhs, cardano_value_t* rhs, cardano_asset_id_list_t** result);

/**
 * \brief Retrieves the count of intersecting assets between two Cardano value objects.
 *
 * This function calculates the number of assets that are present in both the left-hand side (lhs) and
 * right-hand side (rhs) \ref cardano_value_t objects. The intersection refers to assets that exist in both value sets.
 *
 * \param[in] lhs A pointer to the first \ref cardano_value_t object, representing the left-hand side value.
 *                This value must not be NULL and should be initialized.
 * \param[in] rhs A pointer to the second \ref cardano_value_t object, representing the right-hand side value.
 *                This value must not be NULL and should be initialized.
 * \param[out] result On successful execution, this will point to a uint64_t that contains the number of intersecting assets
 *             between the two value objects. The result is set to 0 if there are no intersecting assets or if any input is NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the operation was successful,
 *         or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_value_t* lhs = ...; // Assume lhs is initialized with some assets
 * cardano_value_t* rhs = ...; // Assume rhs is initialized with some assets
 * uint64_t intersection_count = 0;
 *
 * cardano_error_t result = cardano_value_get_intersection_count(lhs, rhs, &intersection_count);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Number of intersecting assets: %llu\n", intersection_count);
 * }
 * else
 * {
 *   printf("Failed to calculate intersection: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_value_get_intersection_count(cardano_value_t* lhs, cardano_value_t* rhs, uint64_t* result);

/**
 * \brief Converts a Cardano value into a flat asset ID map.
 *
 * This function converts a \ref cardano_value_t object, which contains multi-asset values, into a
 * \ref cardano_asset_id_map_t. In this conversion, the policy ID and asset name from the multi-asset structure
 * are combined into a single key in the resulting asset ID map.
 *
 * If the \ref cardano_value_t object contains a coin value (Lovelace) greater than 0, it will also be included
 * in the asset map with the key representing the native Cardano currency (Lovelace).
 *
 * \param[in] value A pointer to an initialized \ref cardano_value_t object containing the multi-assets to be flattened into the map.
 *                  This value must not be NULL and should be initialized.
 *
 * \return A pointer to a newly created \ref cardano_asset_id_map_t object representing the flat map of asset IDs
 *         to their corresponding quantities. The map is dynamically allocated, and the caller is responsible for managing
 *         its lifecycle, specifically releasing it by calling \ref cardano_asset_id_map_unref when it is no longer needed.
 *         If the input value is NULL or an error occurs, this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_value_t* value = ...; // Assume value is already initialized with assets
 * cardano_asset_id_map_t* asset_map = cardano_value_as_assets_map(value);
 *
 * if (asset_map != NULL)
 * {
 *   // Process the flat asset ID map
 *   // Release the map once done
 *   cardano_asset_id_map_unref(&asset_map);
 * }
 * else
 * {
 *   printf("Failed to convert the value to an asset ID map.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_asset_id_map_t* cardano_value_as_assets_map(cardano_value_t* value);

/**
 * \brief Retrieves the count of unique assets in a Cardano value.
 *
 * This function calculates the number of unique assets contained within a \ref cardano_value_t object, including
 * the native ADA (Lovelace) if the coin value is greater than 0.
 *
 * \param[in] value A pointer to an initialized \ref cardano_value_t object from which the asset count is retrieved.
 *                  This value must not be NULL and should be properly initialized.
 *
 * \return The total count of unique assets within the value as an unsigned 64-bit integer. If the value contains ADA (Lovelace) with a coin value
 *         greater than 0, it will be counted as one asset. If the input pointer is NULL or an error occurs, the function returns 0.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_value_t* value = ...; // Assume value is already initialized with assets
 * uint64_t asset_count = cardano_value_get_asset_count(value);
 *
 * if (asset_count > 0)
 * {
 *   printf("The value contains %llu unique assets.\n", asset_count);
 * }
 * else
 * {
 *   printf("No assets found or an error occurred.\n");
 * }
 * \endcode
 *
 * \note If the \ref cardano_value_t object has a coin value (Lovelace) greater than 0, it will be counted as an asset
 *       in the total asset count.
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_value_get_asset_count(const cardano_value_t* value);

/**
 * \brief Checks whether the given Cardano value is zero.
 *
 * This function evaluates whether the provided \ref cardano_value_t object represents zero value.
 * It checks if both the coin amount (in lovelace) and any associated multi-assets are zero.
 *
 * \param[in] value A constant pointer to an initialized \ref cardano_value_t object to be checked.
 *
 * \return A boolean value indicating whether the \ref cardano_value_t object represents zero:
 *         - \c true if the value is zero (both coin and assets are zero),
 *         - \c false if either the coin amount or any of the assets are non-zero.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_value_t* value = ...; // Assume value is already initialized
 * bool is_zero = cardano_value_is_zero(value);
 *
 * if (is_zero)
 * {
 *   printf("The value is zero.\n");
 * }
 * else
 * {
 *   printf("The value is not zero.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_value_is_zero(const cardano_value_t* value);

/**
 * \brief Compares two value objects for equality.
 *
 * This function compares two value objects to determine if they are equal.
 *
 * \param[in] lhs Pointer to the first value object.
 * \param[in] rhs Pointer to the second value object.
 *
 * \return \c true if the value objects are equal, \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_value_t* value1 = NULL;
 * cardano_value_t* value2 = NULL;
 *
 * // Assume value1 and value2 are initialized properly
 *
 * bool are_equal = cardano_value_equals(value1, value2);
 *
 * if (are_equal)
 * {
 *   printf("The value objects are equal.\n");
 * }
 * else
 * {
 *   printf("The value objects are not equal.\n");
 * }
 *
 * // Clean up
 * cardano_value_unref(&value1);
 * cardano_value_unref(&value2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_value_equals(const cardano_value_t* lhs, const cardano_value_t* rhs);

/**
 * \brief Decrements the reference count of a cardano_value_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_value_t object
 * by decreasing its reference count. When the reference count reaches zero, the value is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] value A pointer to the pointer of the value object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_value_t* value = cardano_value_new(major, minor);
 *
 * // Perform operations with the value...
 *
 * cardano_value_unref(&value);
 * // At this point, value is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_value_unref, the pointer to the \ref cardano_value_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_value_unref(cardano_value_t** value);

/**
 * \brief Increases the reference count of the cardano_value_t object.
 *
 * This function is used to manually increment the reference count of an cardano_value_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_value_unref.
 *
 * \param value A pointer to the cardano_value_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming value is a previously created value object
 *
 * cardano_value_ref(value);
 *
 * // Now value can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_value_ref there is a corresponding
 * call to \ref cardano_value_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_value_ref(cardano_value_t* value);

/**
 * \brief Retrieves the current reference count of the cardano_value_t object.
 *
 * This function returns the number of active references to an cardano_value_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_value_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param value A pointer to the cardano_value_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_value_t object. If the object
 * is properly managed (i.e., every \ref cardano_value_ref call is matched with a
 * \ref cardano_value_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming value is a previously created value object
 *
 * size_t ref_count = cardano_value_refcount(value);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_value_refcount(const cardano_value_t* value);

/**
 * \brief Sets the last error message for a given cardano_value_t object.
 *
 * Records an error message in the value's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] value A pointer to the \ref cardano_value_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the value's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_value_set_last_error(
  cardano_value_t* value,
  const char*      message);

/**
 * \brief Retrieves the last error message recorded for a specific value.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_value_set_last_error for the given
 * value. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] value A pointer to the \ref cardano_value_t instance whose last error
 *                   message is to be retrieved. If the value is NULL, the function
 *                   returns a generic error message indicating the null value.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified value. If the value is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_value_set_last_error for the same value, or until
 *       the value is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_value_get_last_error(const cardano_value_t* value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_VALUE_H