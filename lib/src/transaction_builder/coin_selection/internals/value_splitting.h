/**
 * \file value_splitting.h
 *
 * \author angel.castillo
 * \date   Jul 03, 2026
 *
 * Copyright 2026 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_VALUE_SPLITTING_H
#define BIGLUP_LABS_INCLUDE_CARDANO_VALUE_SPLITTING_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/transaction_body/value.h>
#include <cardano/typedefs.h>

/* STRUCTURES ****************************************************************/

/**
 * \brief A growable list of values, used to return the parts of a split value.
 */
typedef struct cardano_value_part_list_t
{
    cardano_value_t** items;
    size_t            size;
    size_t            capacity;
} cardano_value_part_list_t;

/* DECLARATIONS **************************************************************/

/**
 * \brief Releases every value held by the part list and frees its storage.
 *
 * \param[in,out] parts The list to free. Reset to an empty list.
 */
void
_cardano_coin_selection_value_parts_free(cardano_value_part_list_t* parts);

/**
 * \brief Indicates whether a value would exceed the maximum serialized size of a transaction
 * output value.
 *
 * The size is assessed conservatively: the value is measured with the largest possible ada
 * quantity (the total lovelace supply), since the final ada quantity of a change output is
 * not known until coins are assigned. This mirrors the reference implementation, which prefers
 * over-splitting to producing an output that is marginally over the limit.
 *
 * \param[in]  value          The value to assess.
 * \param[in]  max_value_size The protocol maximum serialized size of an output value, in bytes.
 *                            A value of zero means "no limit" and always assesses as within limits.
 * \param[out] is_oversized   Set to true if the serialized size exceeds the maximum.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
cardano_error_t
_cardano_coin_selection_value_is_oversized(
  cardano_value_t* value,
  uint64_t         max_value_size,
  bool*            is_oversized);

/**
 * \brief Splits the assets of a value into the minimum number of groups needed for every group
 * to fit within the maximum serialized size of a transaction output value.
 *
 * This is a port of `splitBundlesWithExcessiveAssetCounts` from the cardano-coin-selection
 * library: an oversized value has its assets equipartitioned into halves until every resulting
 * part is within the limit. The coin quantity of the input value is not distributed; every
 * returned part has a coin quantity of zero, and callers are expected to assign ada quantities
 * afterwards (typically the min-ADA requirement plus a share of the change).
 *
 * The concatenation of the parts' assets is always exactly equal to the assets of the input value.
 *
 * \param[in]     value          The value whose assets to split.
 * \param[in]     max_value_size The protocol maximum serialized size of an output value, in bytes.
 *                               A value of zero means "no limit"; a single part is returned.
 * \param[in,out] parts          The list receiving the parts. Must be zero-initialized by the
 *                               caller and released with \ref _cardano_coin_selection_value_parts_free.
 *                               Always holds at least one part on success.
 *
 * \return \ref CARDANO_SUCCESS on success, or an appropriate error code.
 */
cardano_error_t
_cardano_coin_selection_split_value_assets(
  cardano_value_t*           value,
  uint64_t                   max_value_size,
  cardano_value_part_list_t* parts);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_VALUE_SPLITTING_H
