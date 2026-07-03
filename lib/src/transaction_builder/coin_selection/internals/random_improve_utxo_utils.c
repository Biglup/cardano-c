/**
 * \file random_improve_utxo_utils.c
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

#include <cardano/assets/asset_id_map.h>
#include <cardano/assets/multi_asset.h>

#include "./random_improve_utxo_utils.h"

#include "../../../allocators.h"

#include <string.h>

/* STATIC FUNCTIONS **********************************************************/

/* DEFINITIONS ****************************************************************/

int64_t
_cardano_random_improve_get_asset_quantity(cardano_value_t* value, cardano_asset_id_t* asset_id)
{
  cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(value);
  cardano_multi_asset_unref(&multi_asset);

  if (multi_asset == NULL)
  {
    return 0;
  }

  int64_t quantity = 0;

  if (cardano_multi_asset_get_with_id(multi_asset, asset_id, &quantity) != CARDANO_SUCCESS)
  {
    return 0;
  }

  return quantity;
}

cardano_value_t*
_cardano_random_improve_borrow_utxo_value(cardano_utxo_t* utxo)
{
  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
  cardano_value_t*              value  = cardano_transaction_output_get_value(output);

  cardano_transaction_output_unref(&output);
  cardano_value_unref(&value);

  return value;
}

bool
_cardano_random_improve_asset_id_equals(cardano_asset_id_t* lhs, cardano_asset_id_t* rhs)
{
  const char* lhs_hex = cardano_asset_id_get_hex(lhs);
  const char* rhs_hex = cardano_asset_id_get_hex(rhs);

  if ((lhs_hex == NULL) || (rhs_hex == NULL))
  {
    return false;
  }

  return strcmp(lhs_hex, rhs_hex) == 0;
}

uint64_t
_cardano_random_improve_selected_quantity(cardano_utxo_list_t* selection, cardano_asset_id_t* asset_id)
{
  uint64_t total = 0U;

  const size_t size = cardano_utxo_list_get_length(selection);

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_utxo_t* utxo = NULL;

    if (cardano_utxo_list_get(selection, i, &utxo) != CARDANO_SUCCESS)
    {
      continue;
    }

    cardano_utxo_unref(&utxo);

    cardano_value_t* value = _cardano_random_improve_borrow_utxo_value(utxo);

    int64_t quantity = 0;

    if (asset_id == NULL)
    {
      quantity = cardano_value_get_coin(value);
    }
    else
    {
      quantity = _cardano_random_improve_get_asset_quantity(value, asset_id);
    }

    if (quantity > 0)
    {
      total += (uint64_t)quantity;
    }
  }

  return total;
}
