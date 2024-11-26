/**
 * \file value.c
 *
 * \author angel.castillo
 * \date   Sep 12, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License") {}
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

#include <cardano/assets/asset_id_map.h>
#include <cardano/assets/asset_name.h>
#include <cardano/assets/asset_name_list.h>
#include <cardano/assets/asset_name_map.h>
#include <cardano/assets/multi_asset.h>
#include <cardano/assets/policy_id_list.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/object.h>
#include <cardano/transaction_body/value.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t VALUE_ARRAY_REQUIRED_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief A Value object encapsulates the quantity of assets of different types,
 * including ADA (Cardano's native cryptocurrency) expressed in lovelace,
 * where 1 ADA = 1,000,000 lovelace, and other native tokens. Each key in the
 * tokens object is a unique identifier for an asset, and the corresponding
 * value is the quantity of that asset.
 */
typedef struct cardano_value_t
{
    cardano_object_t       base;
    int64_t                coin;
    cardano_multi_asset_t* multi_asset;
} cardano_value_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a value object.
 *
 * This function is responsible for properly deallocating a value object (`cardano_value_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the value object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_value_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the value
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_value_deallocate(void* object)
{
  assert(object != NULL);

  cardano_value_t* value = (cardano_value_t*)object;

  cardano_multi_asset_unref(&value->multi_asset);

  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_value_new(
  const int64_t          coin,
  cardano_multi_asset_t* assets,
  cardano_value_t**      value)
{
  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_value_t* new_value = (cardano_value_t*)_cardano_malloc(sizeof(cardano_value_t));

  if (new_value == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  new_value->base.ref_count     = 1;
  new_value->base.last_error[0] = '\0';
  new_value->base.deallocator   = cardano_value_deallocate;
  new_value->coin               = coin;
  new_value->multi_asset        = NULL;

  if (assets != NULL)
  {
    cardano_multi_asset_ref(assets);
    new_value->multi_asset = assets;
  }
  else
  {
    cardano_error_t multi_asset_new_result = cardano_multi_asset_new(&new_value->multi_asset);

    if (multi_asset_new_result != CARDANO_SUCCESS)
    {
      _cardano_free(new_value);
      return multi_asset_new_result;
    }
  }

  *value = new_value;

  return CARDANO_SUCCESS;
}

cardano_value_t*
cardano_value_new_zero(void)
{
  cardano_value_t* value = NULL;

  cardano_error_t result = cardano_value_new(0, NULL, &value);
  CARDANO_UNUSED(result);

  return value;
}

cardano_value_t*
cardano_value_new_from_coin(const int64_t lovelace)
{
  cardano_value_t* value = NULL;

  cardano_error_t result = cardano_value_new(lovelace, NULL, &value);
  CARDANO_UNUSED(result);

  return value;
}

cardano_error_t
cardano_value_from_asset_map(
  cardano_asset_id_map_t* asset_map,
  cardano_value_t**       value)
{
  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (asset_map == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  uint64_t               coin        = 0U;
  cardano_multi_asset_t* multi_asset = NULL;

  cardano_error_t result = cardano_multi_asset_new(&multi_asset);

  if (result != CARDANO_SUCCESS)
  {
    *value = NULL;
    return result;
  }

  const size_t asset_count = cardano_asset_id_map_get_length(asset_map);

  for (size_t i = 0U; i < asset_count; ++i)
  {
    cardano_asset_id_t* asset_id = NULL;
    int64_t             amount   = 0;

    result = cardano_asset_id_map_get_key_value_at(asset_map, i, &asset_id, &amount);

    if (result != CARDANO_SUCCESS)
    {
      cardano_multi_asset_unref(&multi_asset);

      *value = NULL;
      return result;
    }

    if (cardano_asset_id_is_lovelace(asset_id))
    {
      coin = amount;

      cardano_asset_id_unref(&asset_id);
    }
    else
    {
      cardano_asset_name_t*   asset_name = cardano_asset_id_get_asset_name(asset_id);
      cardano_blake2b_hash_t* policy_id  = cardano_asset_id_get_policy_id(asset_id);

      cardano_asset_id_unref(&asset_id);

      if ((asset_name == NULL) || (policy_id == NULL))
      {
        cardano_multi_asset_unref(&multi_asset);
        cardano_asset_name_unref(&asset_name);
        cardano_blake2b_hash_unref(&policy_id);

        *value = NULL;

        return CARDANO_ERROR_POINTER_IS_NULL;
      }

      result = cardano_multi_asset_set(multi_asset, policy_id, asset_name, amount);

      cardano_asset_name_unref(&asset_name);
      cardano_blake2b_hash_unref(&policy_id);

      if (result != CARDANO_SUCCESS)
      {
        cardano_multi_asset_unref(&multi_asset);

        *value = NULL;
        return result;
      }
    }
  }

  result = cardano_value_new((int64_t)coin, multi_asset, value);

  cardano_multi_asset_unref(&multi_asset);

  return result;
}

cardano_error_t
cardano_value_from_cbor(cardano_cbor_reader_t* reader, cardano_value_t** value)
{
  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *value = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_multi_asset_t*      multi_asset = NULL;
  uint64_t                    coin        = 0U;
  cardano_cbor_reader_state_t state;

  const cardano_error_t peek_result = cardano_cbor_reader_peek_state(reader, &state);

  if (peek_result != CARDANO_SUCCESS)
  {
    *value = NULL;
    return peek_result;
  }

  if (state == CARDANO_CBOR_READER_STATE_UNSIGNED_INTEGER)
  {
    const cardano_error_t read_coin_result = cardano_cbor_reader_read_uint(reader, &coin);

    if (read_coin_result != CARDANO_SUCCESS)
    {
      *value = NULL;
      return read_coin_result;
    }

    return cardano_value_new((int64_t)coin, NULL, value);
  }
  else
  {
    static const char* validator_name = "value";

    const cardano_error_t read_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)VALUE_ARRAY_REQUIRED_SIZE);

    if (read_array_result != CARDANO_SUCCESS)
    {
      *value = NULL;
      return read_array_result;
    }

    const cardano_error_t read_coin_result = cardano_cbor_reader_read_uint(reader, &coin);

    if (read_coin_result != CARDANO_SUCCESS)
    {
      *value = NULL;
      return read_coin_result;
    }

    const cardano_error_t read_multi_asset_result = cardano_multi_asset_from_cbor(reader, &multi_asset);

    if (read_multi_asset_result != CARDANO_SUCCESS)
    {
      *value = NULL;
      return read_multi_asset_result;
    }

    const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

    if (expect_end_array_result != CARDANO_SUCCESS)
    {
      cardano_multi_asset_unref(&multi_asset);
      *value = NULL;
      return expect_end_array_result;
    }

    cardano_error_t new_val_result = cardano_value_new((int64_t)coin, multi_asset, value);

    cardano_multi_asset_unref(&multi_asset);

    return new_val_result;
  }
}

cardano_error_t
cardano_value_to_cbor(
  const cardano_value_t* value,
  cardano_cbor_writer_t* writer)
{
  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((value->multi_asset == NULL) || (cardano_multi_asset_get_policy_count(value->multi_asset) == 0U))
  {
    return cardano_cbor_writer_write_uint(writer, value->coin);
  }

  cardano_error_t write_start_array_result = cardano_cbor_writer_write_start_array(writer, VALUE_ARRAY_REQUIRED_SIZE);

  if (write_start_array_result != CARDANO_SUCCESS)
  {
    return write_start_array_result;
  }

  cardano_error_t write_coin_result = cardano_cbor_writer_write_uint(writer, value->coin);

  if (write_coin_result != CARDANO_SUCCESS)
  {
    return write_coin_result;
  }

  return cardano_multi_asset_to_cbor(value->multi_asset, writer);
}

cardano_multi_asset_t*
cardano_value_get_multi_asset(cardano_value_t* value)
{
  if (value == NULL)
  {
    return NULL;
  }

  cardano_multi_asset_ref(value->multi_asset);

  return value->multi_asset;
}

cardano_error_t
cardano_value_set_multi_asset(cardano_value_t* value, cardano_multi_asset_t* assets)
{
  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_multi_asset_ref(assets);
  cardano_multi_asset_unref(&value->multi_asset);
  value->multi_asset = assets;

  return CARDANO_SUCCESS;
}

int64_t
cardano_value_get_coin(const cardano_value_t* value)
{
  if (value == NULL)
  {
    return 0U;
  }

  return value->coin;
}

cardano_error_t
cardano_value_set_coin(cardano_value_t* value, const int64_t coin)
{
  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  value->coin = coin;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_value_add_coin(cardano_value_t* value, const int64_t coin)
{
  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  value->coin += coin;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_value_subtract_coin(cardano_value_t* value, const int64_t coin)
{
  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  value->coin -= coin;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_value_add_multi_asset(cardano_value_t* value, cardano_multi_asset_t* multi_asset)
{
  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (multi_asset == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_multi_asset_t* add_result = NULL;
  cardano_error_t        result     = cardano_multi_asset_add(value->multi_asset, multi_asset, &add_result);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_multi_asset_unref(&value->multi_asset);
  value->multi_asset = add_result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_value_subtract_multi_asset(cardano_value_t* value, cardano_multi_asset_t* multi_asset)
{
  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (multi_asset == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_multi_asset_t* subtraction_result = NULL;
  cardano_error_t        result             = cardano_multi_asset_subtract(value->multi_asset, multi_asset, &subtraction_result);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_multi_asset_unref(&value->multi_asset);
  value->multi_asset = subtraction_result;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_value_add_asset(
  cardano_value_t*        value,
  cardano_blake2b_hash_t* policy_id,
  cardano_asset_name_t*   asset_name,
  int64_t                 quantity)
{
  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (policy_id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (asset_name == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  assert(value->multi_asset != NULL);

  int64_t         current_quantity = 0;
  cardano_error_t result           = cardano_multi_asset_get(value->multi_asset, policy_id, asset_name, &current_quantity);

  if ((result != CARDANO_SUCCESS) && (result != CARDANO_ERROR_ELEMENT_NOT_FOUND))
  {
    return result;
  }

  return cardano_multi_asset_set(value->multi_asset, policy_id, asset_name, current_quantity + quantity);
}

cardano_error_t
cardano_value_add_asset_ex(
  cardano_value_t* value,
  const char*      policy_id_hex,
  const size_t     policy_id_hex_len,
  const char*      asset_name_hex,
  const size_t     asset_name_hex_len,
  const int64_t    quantity)
{
  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (policy_id_hex == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (asset_name_hex == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  assert(value->multi_asset != NULL);

  cardano_blake2b_hash_t* policy_id  = NULL;
  cardano_asset_name_t*   asset_name = NULL;

  cardano_error_t policy_id_result = cardano_blake2b_hash_from_hex(policy_id_hex, policy_id_hex_len, &policy_id);

  if (policy_id_result != CARDANO_SUCCESS)
  {
    return policy_id_result;
  }

  cardano_error_t asset_name_result = cardano_asset_name_from_hex(asset_name_hex, asset_name_hex_len, &asset_name);

  if (asset_name_result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&policy_id);
    return asset_name_result;
  }

  cardano_error_t add_asset_result = cardano_value_add_asset(value, policy_id, asset_name, quantity);

  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&asset_name);

  return add_asset_result;
}

cardano_error_t
cardano_value_add_asset_with_id(
  cardano_value_t*    value,
  cardano_asset_id_t* asset_id,
  int64_t             quantity)
{
  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (asset_id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  assert(value->multi_asset != NULL);

  cardano_asset_name_t*   asset_name = cardano_asset_id_get_asset_name(asset_id);
  cardano_blake2b_hash_t* policy_id  = cardano_asset_id_get_policy_id(asset_id);

  cardano_error_t result = cardano_value_add_asset(value, policy_id, asset_name, quantity);

  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);

  return result;
}

cardano_error_t
cardano_value_add_asset_with_id_ex(
  cardano_value_t* value,
  const char*      asset_id_hex,
  size_t           asset_id_hex_len,
  int64_t          quantity)
{
  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (asset_id_hex == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  assert(value->multi_asset != NULL);

  cardano_asset_id_t* asset_id = NULL;

  cardano_error_t asset_id_result = cardano_asset_id_from_hex(asset_id_hex, asset_id_hex_len, &asset_id);

  if (asset_id_result != CARDANO_SUCCESS)
  {
    return asset_id_result;
  }

  cardano_error_t add_asset_result = cardano_value_add_asset_with_id(value, asset_id, quantity);

  cardano_asset_id_unref(&asset_id);

  return add_asset_result;
}

cardano_error_t
cardano_value_add(cardano_value_t* lhs, cardano_value_t* rhs, cardano_value_t** result)
{
  if (lhs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (rhs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (result == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((lhs->multi_asset == NULL) || (cardano_multi_asset_get_policy_count(lhs->multi_asset) == 0U))
  {
    return cardano_value_new(lhs->coin + rhs->coin, rhs->multi_asset, result);
  }

  if ((rhs->multi_asset == NULL) || (cardano_multi_asset_get_policy_count(rhs->multi_asset) == 0U))
  {
    return cardano_value_new(lhs->coin + rhs->coin, lhs->multi_asset, result);
  }

  int64_t                coin        = lhs->coin + rhs->coin;
  cardano_multi_asset_t* multi_asset = NULL;

  cardano_error_t multi_asset_result = cardano_multi_asset_add(lhs->multi_asset, rhs->multi_asset, &multi_asset);

  if (multi_asset_result != CARDANO_SUCCESS)
  {
    return multi_asset_result;
  }

  cardano_error_t new_value_result = cardano_value_new(coin, multi_asset, result);
  cardano_multi_asset_unref(&multi_asset);

  return new_value_result;
}

cardano_error_t
cardano_value_subtract(cardano_value_t* lhs, cardano_value_t* rhs, cardano_value_t** result)
{
  if (lhs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (rhs == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (result == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((lhs->multi_asset == NULL) || (cardano_multi_asset_get_policy_count(lhs->multi_asset) == 0U))
  {
    cardano_error_t new_val_result = cardano_value_new(lhs->coin - rhs->coin, NULL, result);

    if (new_val_result != CARDANO_SUCCESS)
    {
      (*result) = NULL;
      return new_val_result;
    }

    new_val_result = cardano_value_subtract_multi_asset(*result, rhs->multi_asset);

    if (new_val_result != CARDANO_SUCCESS)
    {
      cardano_value_unref(result);
      return new_val_result;
    }

    return new_val_result;
  }

  if ((rhs->multi_asset == NULL) || (cardano_multi_asset_get_policy_count(rhs->multi_asset) == 0U))
  {
    return cardano_value_new(lhs->coin - rhs->coin, lhs->multi_asset, result);
  }

  int64_t                coin        = lhs->coin - rhs->coin;
  cardano_multi_asset_t* multi_asset = NULL;

  cardano_error_t multi_asset_result = cardano_multi_asset_subtract(lhs->multi_asset, rhs->multi_asset, &multi_asset);

  if (multi_asset_result != CARDANO_SUCCESS)
  {
    return multi_asset_result;
  }

  cardano_error_t new_value_result = cardano_value_new(coin, multi_asset, result);
  cardano_multi_asset_unref(&multi_asset);

  return new_value_result;
}

cardano_error_t
cardano_value_get_intersection(cardano_value_t* lhs, cardano_value_t* rhs, cardano_asset_id_list_t** result)
{
  if ((lhs == NULL) || (rhs == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_asset_id_list_t* intersection = NULL;

  cardano_error_t get_intersection_result = cardano_asset_id_list_new(&intersection);

  if (get_intersection_result != CARDANO_SUCCESS)
  {
    return get_intersection_result;
  }

  if ((lhs->coin > 0) && (rhs->coin > 0))
  {
    cardano_asset_id_t* lovelace_asset_id = NULL;

    cardano_error_t add_lovelace_result = cardano_asset_id_new_lovelace(&lovelace_asset_id);

    if (add_lovelace_result != CARDANO_SUCCESS)
    {
      cardano_asset_id_list_unref(&intersection);
      return add_lovelace_result;
    }

    add_lovelace_result = cardano_asset_id_list_add(intersection, lovelace_asset_id);

    cardano_asset_id_unref(&lovelace_asset_id);

    if (add_lovelace_result != CARDANO_SUCCESS)
    {
      cardano_asset_id_list_unref(&intersection);
      return add_lovelace_result;
    }
  }

  cardano_policy_id_list_t* lhs_policies    = NULL;
  cardano_error_t           get_keys_result = cardano_multi_asset_get_keys(lhs->multi_asset, &lhs_policies);

  if (get_keys_result != CARDANO_SUCCESS)
  {
    return get_keys_result;
  }

  for (size_t i = 0U; i < cardano_policy_id_list_get_length(lhs_policies); ++i)
  {
    cardano_blake2b_hash_t* policy_id = NULL;

    cardano_error_t get_policy_result = cardano_policy_id_list_get(lhs_policies, i, &policy_id);

    if (get_policy_result != CARDANO_SUCCESS)
    {
      cardano_policy_id_list_unref(&lhs_policies);

      return get_policy_result;
    }

    cardano_asset_name_map_t* lhs_asset_map = NULL;
    cardano_asset_name_map_t* rhs_asset_map = NULL;

    cardano_error_t get_asset_result = cardano_multi_asset_get_assets(lhs->multi_asset, policy_id, &lhs_asset_map);

    if (get_asset_result != CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_unref(&policy_id);
      cardano_policy_id_list_unref(&lhs_policies);

      return get_asset_result;
    }

    get_asset_result = cardano_multi_asset_get_assets(rhs->multi_asset, policy_id, &rhs_asset_map);

    if ((get_asset_result != CARDANO_SUCCESS) || (rhs_asset_map == NULL))
    {
      cardano_blake2b_hash_unref(&policy_id);
      cardano_asset_name_map_unref(&lhs_asset_map);

      continue;
    }

    cardano_asset_name_list_t* lhs_asset_names       = NULL;
    cardano_error_t            get_asset_list_result = cardano_asset_name_map_get_keys(lhs_asset_map, &lhs_asset_names);

    if (get_asset_list_result != CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_unref(&policy_id);
      cardano_asset_name_map_unref(&lhs_asset_map);
      cardano_asset_name_map_unref(&rhs_asset_map);
      cardano_policy_id_list_unref(&lhs_policies);

      return get_asset_list_result;
    }

    for (size_t j = 0U; j < cardano_asset_name_list_get_length(lhs_asset_names); ++j)
    {
      cardano_asset_name_t* asset_name = NULL;

      cardano_error_t get_asset_name_result = cardano_asset_name_list_get(lhs_asset_names, j, &asset_name);

      if (get_asset_name_result != CARDANO_SUCCESS)
      {
        cardano_blake2b_hash_unref(&policy_id);
        cardano_asset_name_list_unref(&lhs_asset_names);
        cardano_asset_name_map_unref(&lhs_asset_map);
        cardano_asset_name_map_unref(&rhs_asset_map);
        cardano_policy_id_list_unref(&lhs_policies);

        return get_asset_name_result;
      }

      int64_t lhs_value = 0;
      int64_t rhs_value = 0;

      cardano_error_t get_asset_value_result = cardano_multi_asset_get(lhs->multi_asset, policy_id, asset_name, &lhs_value);

      if (get_asset_value_result != CARDANO_SUCCESS)
      {
        cardano_asset_name_unref(&asset_name);
        cardano_blake2b_hash_unref(&policy_id);
        cardano_asset_name_list_unref(&lhs_asset_names);
        cardano_asset_name_map_unref(&lhs_asset_map);
        cardano_asset_name_map_unref(&rhs_asset_map);
        cardano_policy_id_list_unref(&lhs_policies);

        return get_asset_value_result;
      }

      if (cardano_multi_asset_get(rhs->multi_asset, policy_id, asset_name, &rhs_value) == CARDANO_SUCCESS)
      {
        cardano_asset_id_t* asset_id = NULL;

        cardano_error_t asset_id_result = cardano_asset_id_new(policy_id, asset_name, &asset_id);

        if (asset_id_result != CARDANO_SUCCESS)
        {
          cardano_asset_name_unref(&asset_name);
          cardano_blake2b_hash_unref(&policy_id);
          cardano_asset_name_list_unref(&lhs_asset_names);
          cardano_asset_name_map_unref(&lhs_asset_map);
          cardano_asset_name_map_unref(&rhs_asset_map);
          cardano_policy_id_list_unref(&lhs_policies);

          return asset_id_result;
        }

        asset_id_result = cardano_asset_id_list_add(intersection, asset_id);

        cardano_asset_id_unref(&asset_id);

        if (asset_id_result != CARDANO_SUCCESS)
        {
          cardano_asset_name_unref(&asset_name);
          cardano_blake2b_hash_unref(&policy_id);
          cardano_asset_name_list_unref(&lhs_asset_names);
          cardano_asset_name_map_unref(&lhs_asset_map);
          cardano_asset_name_map_unref(&rhs_asset_map);
          cardano_policy_id_list_unref(&lhs_policies);

          return asset_id_result;
        }
      }

      cardano_asset_name_unref(&asset_name);
    }

    cardano_asset_name_list_unref(&lhs_asset_names);
    cardano_asset_name_map_unref(&lhs_asset_map);
    cardano_asset_name_map_unref(&rhs_asset_map);
    cardano_blake2b_hash_unref(&policy_id);
  }

  cardano_policy_id_list_unref(&lhs_policies);

  *result = intersection;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_value_get_intersection_count(cardano_value_t* lhs, cardano_value_t* rhs, uint64_t* result)
{
  if ((lhs == NULL) || (rhs == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (result == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_asset_id_list_t* intersection = NULL;

  cardano_error_t get_intersection_result = cardano_value_get_intersection(lhs, rhs, &intersection);

  if (get_intersection_result != CARDANO_SUCCESS)
  {
    return get_intersection_result;
  }

  *result = cardano_asset_id_list_get_length(intersection);

  cardano_asset_id_list_unref(&intersection);

  return CARDANO_SUCCESS;
}

cardano_asset_id_map_t*
cardano_value_as_assets_map(cardano_value_t* value)
{
  if (value == NULL)
  {
    return NULL;
  }

  cardano_asset_id_map_t* asset_map = NULL;
  cardano_error_t         result    = cardano_asset_id_map_new(&asset_map);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  if (value->coin != 0)
  {
    cardano_asset_id_t* lovelace_asset_id = NULL;
    result                                = cardano_asset_id_new_lovelace(&lovelace_asset_id);

    if (result != CARDANO_SUCCESS)
    {
      cardano_asset_id_map_unref(&asset_map);
      return NULL;
    }

    result = cardano_asset_id_map_insert(asset_map, lovelace_asset_id, (int64_t)value->coin);
    cardano_asset_id_unref(&lovelace_asset_id);

    if (result != CARDANO_SUCCESS)
    {
      cardano_asset_id_map_unref(&asset_map);
      return NULL;
    }
  }

  if (value->multi_asset != NULL)
  {
    cardano_policy_id_list_t* policies = NULL;
    result                             = cardano_multi_asset_get_keys(value->multi_asset, &policies);

    if (result != CARDANO_SUCCESS)
    {
      cardano_asset_id_map_unref(&asset_map);
      return NULL;
    }

    for (size_t i = 0U; i < cardano_policy_id_list_get_length(policies); ++i)
    {
      cardano_blake2b_hash_t* policy_id = NULL;
      result                            = cardano_policy_id_list_get(policies, i, &policy_id);

      if (result != CARDANO_SUCCESS)
      {
        cardano_policy_id_list_unref(&policies);
        cardano_asset_id_map_unref(&asset_map);

        return NULL;
      }

      cardano_asset_name_map_t* asset_names = NULL;
      result                                = cardano_multi_asset_get_assets(value->multi_asset, policy_id, &asset_names);

      if (result != CARDANO_SUCCESS)
      {
        cardano_blake2b_hash_unref(&policy_id);
        cardano_policy_id_list_unref(&policies);
        cardano_asset_id_map_unref(&asset_map);

        return NULL;
      }

      cardano_asset_name_list_t* asset_name_list = NULL;
      result                                     = cardano_asset_name_map_get_keys(asset_names, &asset_name_list);

      if (result != CARDANO_SUCCESS)
      {
        cardano_blake2b_hash_unref(&policy_id);
        cardano_asset_name_map_unref(&asset_names);
        cardano_policy_id_list_unref(&policies);
        cardano_asset_id_map_unref(&asset_map);

        return NULL;
      }

      for (size_t j = 0; j < cardano_asset_name_list_get_length(asset_name_list); ++j)
      {
        cardano_asset_name_t* asset_name = NULL;
        result                           = cardano_asset_name_list_get(asset_name_list, j, &asset_name);

        if (result != CARDANO_SUCCESS)
        {
          cardano_asset_name_list_unref(&asset_name_list);
          cardano_asset_name_map_unref(&asset_names);
          cardano_blake2b_hash_unref(&policy_id);
          cardano_policy_id_list_unref(&policies);
          cardano_asset_id_map_unref(&asset_map);

          return NULL;
        }

        int64_t amount = 0;
        result         = cardano_multi_asset_get(value->multi_asset, policy_id, asset_name, &amount);

        if (result != CARDANO_SUCCESS)
        {
          cardano_asset_name_unref(&asset_name);
          cardano_asset_name_list_unref(&asset_name_list);
          cardano_asset_name_map_unref(&asset_names);
          cardano_blake2b_hash_unref(&policy_id);
          cardano_policy_id_list_unref(&policies);
          cardano_asset_id_map_unref(&asset_map);

          return NULL;
        }

        cardano_asset_id_t* asset_id = NULL;
        result                       = cardano_asset_id_new(policy_id, asset_name, &asset_id);

        if (result != CARDANO_SUCCESS)
        {
          cardano_asset_name_unref(&asset_name);
          cardano_asset_name_list_unref(&asset_name_list);
          cardano_asset_name_map_unref(&asset_names);
          cardano_blake2b_hash_unref(&policy_id);
          cardano_policy_id_list_unref(&policies);
          cardano_asset_id_map_unref(&asset_map);

          return NULL;
        }

        result = cardano_asset_id_map_insert(asset_map, asset_id, amount);
        cardano_asset_id_unref(&asset_id);

        if (result != CARDANO_SUCCESS)
        {
          cardano_asset_name_unref(&asset_name);
          cardano_asset_name_list_unref(&asset_name_list);
          cardano_asset_name_map_unref(&asset_names);
          cardano_blake2b_hash_unref(&policy_id);
          cardano_policy_id_list_unref(&policies);
          cardano_asset_id_map_unref(&asset_map);

          return NULL;
        }

        cardano_asset_name_unref(&asset_name);
      }

      cardano_asset_name_list_unref(&asset_name_list);
      cardano_asset_name_map_unref(&asset_names);
      cardano_blake2b_hash_unref(&policy_id);
    }

    cardano_policy_id_list_unref(&policies);
  }

  return asset_map;
}

uint64_t
cardano_value_get_asset_count(const cardano_value_t* value)
{
  if (value == NULL)
  {
    return 0U;
  }

  uint64_t asset_count = 0U;

  if (value->coin > 0)
  {
    ++asset_count;
  }

  if (value->multi_asset != NULL)
  {
    cardano_policy_id_list_t* policies = NULL;
    cardano_error_t           result   = cardano_multi_asset_get_keys(value->multi_asset, &policies);

    if (result != CARDANO_SUCCESS)
    {
      return 0U;
    }

    for (size_t i = 0U; i < cardano_policy_id_list_get_length(policies); ++i)
    {
      cardano_blake2b_hash_t* policy_id = NULL;
      result                            = cardano_policy_id_list_get(policies, i, &policy_id);

      if (result != CARDANO_SUCCESS)
      {
        cardano_policy_id_list_unref(&policies);

        return 0U;
      }

      cardano_asset_name_map_t* asset_names = NULL;
      result                                = cardano_multi_asset_get_assets(value->multi_asset, policy_id, &asset_names);

      if (result != CARDANO_SUCCESS)
      {
        cardano_blake2b_hash_unref(&policy_id);
        cardano_policy_id_list_unref(&policies);

        return 0U;
      }

      asset_count += cardano_asset_name_map_get_length(asset_names);

      cardano_asset_name_map_unref(&asset_names);
      cardano_blake2b_hash_unref(&policy_id);
    }

    cardano_policy_id_list_unref(&policies);
  }

  return asset_count;
}

bool
cardano_value_is_zero(const cardano_value_t* value)
{
  if (value == NULL)
  {
    return true;
  }

  return (value->coin == 0) && (cardano_multi_asset_get_policy_count(value->multi_asset) == 0U);
}

bool
cardano_value_equals(const cardano_value_t* lhs, const cardano_value_t* rhs)
{
  if (lhs == NULL)
  {
    return rhs == NULL;
  }

  if (rhs == NULL)
  {
    return false;
  }

  if (lhs->coin != rhs->coin)
  {
    return false;
  }

  if ((lhs->multi_asset == NULL) || cardano_multi_asset_get_policy_count(lhs->multi_asset) == 0U)
  {
    return (rhs->multi_asset == NULL) || cardano_multi_asset_get_policy_count(rhs->multi_asset) == 0U;
  }

  return cardano_multi_asset_equals(lhs->multi_asset, rhs->multi_asset);
}

void
cardano_value_unref(cardano_value_t** value)
{
  if ((value == NULL) || (*value == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*value)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *value = NULL;
    return;
  }
}

void
cardano_value_ref(cardano_value_t* value)
{
  if (value == NULL)
  {
    return;
  }

  cardano_object_ref(&value->base);
}

size_t
cardano_value_refcount(const cardano_value_t* value)
{
  if (value == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&value->base);
}

void
cardano_value_set_last_error(cardano_value_t* value, const char* message)
{
  cardano_object_set_last_error(&value->base, message);
}

const char*
cardano_value_get_last_error(const cardano_value_t* value)
{
  return cardano_object_get_last_error(&value->base);
}