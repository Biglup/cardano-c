/**
 * \file value_splitting.c
 *
 * \author angel.castillo
 * \date   Jul 03, 2026
 *
 * Copyright 2026 Biglup Labs
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

#include <cardano/assets/asset_id.h>
#include <cardano/assets/asset_id_map.h>
#include <cardano/cbor/cbor_writer.h>

#include "./value_splitting.h"

#include "../../../allocators.h"

#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Appends a value to a part list, taking ownership of the reference.
 *
 * \param[in,out] parts The list to append to. Reallocated as needed.
 * \param[in]     value The value to append.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code (in which case
 *         ownership of the value is NOT taken).
 */
static cardano_error_t
part_list_append(cardano_value_part_list_t* parts, cardano_value_t* value)
{
  if (parts->size == parts->capacity)
  {
    const size_t      new_capacity = (parts->capacity == 0U) ? 4U : (parts->capacity * 2U);
    cardano_value_t** new_items    = (cardano_value_t**)_cardano_malloc(new_capacity * sizeof(cardano_value_t*));

    if (new_items == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    for (size_t i = 0U; i < parts->size; ++i)
    {
      new_items[i] = parts->items[i];
    }

    _cardano_free(parts->items);

    parts->items    = new_items;
    parts->capacity = new_capacity;
  }

  parts->items[parts->size] = value;
  ++parts->size;

  return CARDANO_SUCCESS;
}

/**
 * \brief Removes and returns the last value of a part list.
 *
 * \param[in,out] parts The list to pop from. Must not be empty.
 *
 * \return The removed value. Ownership passes to the caller.
 */
static cardano_value_t*
part_list_pop(cardano_value_part_list_t* parts)
{
  --parts->size;

  return parts->items[parts->size];
}

/**
 * \brief Builds a value holding the given slice of another value's assets, with a zero coin.
 *
 * \param[in]  assets The flattened asset map of the source value.
 * \param[in]  start  The index of the first asset to include.
 * \param[in]  end    The index one past the last asset to include.
 * \param[out] value  The resulting asset-only value.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
build_asset_slice(
  cardano_asset_id_map_t* assets,
  const size_t            start,
  const size_t            end,
  cardano_value_t**       value)
{
  *value = cardano_value_new_zero();

  if (*value == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  for (size_t i = start; (i < end) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_asset_id_t* asset_id = NULL;

    result = cardano_asset_id_map_get_key_at(assets, i, &asset_id);

    if (result == CARDANO_SUCCESS)
    {
      if (!cardano_asset_id_is_lovelace(asset_id))
      {
        int64_t quantity = 0;

        result = cardano_asset_id_map_get(assets, asset_id, &quantity);

        if ((result == CARDANO_SUCCESS) && (quantity != 0))
        {
          result = cardano_value_add_asset_with_id(*value, asset_id, quantity);
        }
      }

      cardano_asset_id_unref(&asset_id);
    }
  }

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(value);
  }

  return result;
}

/**
 * \brief Splits an asset-only value in half, appending both halves to the pending list.
 *
 * \param[in]     value   The asset-only value to split. Released by this function.
 * \param[in,out] pending The list receiving the two halves.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
static cardano_error_t
split_in_half(cardano_value_t* value, cardano_value_part_list_t* pending)
{
  cardano_asset_id_map_t* assets = cardano_value_as_assets_map(value);

  if (assets == NULL)
  {
    cardano_value_unref(&value);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  const size_t asset_count = cardano_asset_id_map_get_length(assets);
  const size_t half        = asset_count / 2U;

  cardano_value_t* first_half  = NULL;
  cardano_value_t* second_half = NULL;

  cardano_error_t result = build_asset_slice(assets, 0U, half, &first_half);

  if (result == CARDANO_SUCCESS)
  {
    result = build_asset_slice(assets, half, asset_count, &second_half);
  }

  cardano_asset_id_map_unref(&assets);
  cardano_value_unref(&value);

  if (result == CARDANO_SUCCESS)
  {
    result = part_list_append(pending, first_half);

    if (result != CARDANO_SUCCESS)
    {
      cardano_value_unref(&first_half);
    }
  }

  if (result == CARDANO_SUCCESS)
  {
    result = part_list_append(pending, second_half);

    if (result != CARDANO_SUCCESS)
    {
      cardano_value_unref(&second_half);
      second_half = NULL;
    }
  }

  if ((result != CARDANO_SUCCESS) && (second_half != NULL) && (first_half != NULL))
  {
    cardano_value_unref(&second_half);
  }

  return result;
}

/* DEFINITIONS ****************************************************************/

void
_cardano_coin_selection_value_parts_free(cardano_value_part_list_t* parts)
{
  for (size_t i = 0U; i < parts->size; ++i)
  {
    cardano_value_unref(&parts->items[i]);
  }

  _cardano_free(parts->items);

  parts->items    = NULL;
  parts->size     = 0U;
  parts->capacity = 0U;
}

cardano_error_t
_cardano_coin_selection_value_is_oversized(
  cardano_value_t* value,
  const uint64_t   max_value_size,
  bool*            is_oversized)
{
  // The largest ada quantity that can appear in a transaction output: the total lovelace
  // supply (45 billion ada). Used to assess sizes conservatively before the final ada
  // quantity of a change output is known.
  static const int64_t max_output_ada_quantity = 45000000000000000;

  if ((value == NULL) || (is_oversized == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  *is_oversized = false;

  if (max_value_size == 0U)
  {
    return CARDANO_SUCCESS;
  }

  const int64_t original_coin = cardano_value_get_coin(value);

  cardano_error_t result = cardano_value_set_coin(value, max_output_ada_quantity);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    const cardano_error_t rollback_result = cardano_value_set_coin(value, original_coin);
    CARDANO_UNUSED(rollback_result);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result = cardano_value_to_cbor(value, writer);

  if (result == CARDANO_SUCCESS)
  {
    *is_oversized = cardano_cbor_writer_get_encode_size(writer) > max_value_size;
  }

  cardano_cbor_writer_unref(&writer);

  const cardano_error_t restore_result = cardano_value_set_coin(value, original_coin);

  if (result == CARDANO_SUCCESS)
  {
    result = restore_result;
  }

  return result;
}

cardano_error_t
_cardano_coin_selection_split_value_assets(
  cardano_value_t*           value,
  const uint64_t             max_value_size,
  cardano_value_part_list_t* parts)
{
  if ((value == NULL) || (parts == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_asset_id_map_t* assets = cardano_value_as_assets_map(value);

  if (assets == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  const size_t asset_count = cardano_asset_id_map_get_length(assets);

  cardano_value_t* asset_only = NULL;

  cardano_error_t result = build_asset_slice(assets, 0U, asset_count, &asset_only);

  cardano_asset_id_map_unref(&assets);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  // Work through a pending stack instead of recursing: pop a candidate, and either accept it
  // as a final part or split it in half and push both halves back for re-assessment.
  cardano_value_part_list_t pending = { NULL, 0U, 0U };

  result = part_list_append(&pending, asset_only);

  if (result != CARDANO_SUCCESS)
  {
    cardano_value_unref(&asset_only);
  }

  while ((result == CARDANO_SUCCESS) && (pending.size > 0U))
  {
    cardano_value_t* candidate = part_list_pop(&pending);

    bool is_oversized = false;

    result = _cardano_coin_selection_value_is_oversized(candidate, max_value_size, &is_oversized);

    if (result == CARDANO_SUCCESS)
    {
      size_t candidate_assets = 0U;

      if (is_oversized)
      {
        cardano_asset_id_map_t* candidate_map = cardano_value_as_assets_map(candidate);

        if (candidate_map == NULL)
        {
          result = CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
        }
        else
        {
          candidate_assets = cardano_asset_id_map_get_length(candidate_map);

          cardano_asset_id_map_unref(&candidate_map);
        }
      }

      if (result == CARDANO_SUCCESS)
      {
        if (is_oversized && (candidate_assets >= 2U))
        {
          result    = split_in_half(candidate, &pending);
          candidate = NULL;
        }
        else
        {
          result = part_list_append(parts, candidate);

          if (result == CARDANO_SUCCESS)
          {
            candidate = NULL;
          }
        }
      }
    }

    if (candidate != NULL)
    {
      cardano_value_unref(&candidate);
    }
  }

  _cardano_coin_selection_value_parts_free(&pending);

  if (result != CARDANO_SUCCESS)
  {
    _cardano_coin_selection_value_parts_free(parts);
  }

  return result;
}
