/**
 * \file script_data_hash.c
 *
 * \author angel.castillo
 * \date   Nov 12, 2024
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

/* INCLUDES ******************************************************************/

#include <cardano/crypto/blake2b_hash_size.h>
#include <cardano/transaction_builder/script_data_hash.h>
#include <cardano/witness_set/plutus_data_set.h>
#include <cardano/witness_set/redeemer_list.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Checks if the provided cost models are empty.
 *
 * \param costmdls A pointer to \ref cardano_costmdls_t representing the cost models for script execution.
 * \return `true` if the cost models are empty; otherwise, `false`.
 */
static bool
is_costmdls_empty(const cardano_costmdls_t* costmdls)
{
  const bool has_plutus_v1 = cardano_costmdls_has(costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V1);
  const bool has_plutus_v2 = cardano_costmdls_has(costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V2);
  const bool has_plutus_v3 = cardano_costmdls_has(costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V3);

  return (costmdls == NULL) || (!has_plutus_v1 && !has_plutus_v2 && !has_plutus_v3);
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_compute_script_data_hash(
  cardano_costmdls_t*        costmdls,
  cardano_redeemer_list_t*   redeemers,
  cardano_plutus_data_set_t* datums,
  cardano_blake2b_hash_t**   data_hash)
{
  if (data_hash == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (costmdls == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = CARDANO_SUCCESS;

  if (is_costmdls_empty(costmdls))
  {
    *data_hash = NULL;
    return result;
  }

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  if (writer == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  //  These are safe to call, returns 0 if the pointer is NULL
  const size_t plutus_data_size = cardano_plutus_data_set_get_length(datums);
  const size_t redeemers_size   = cardano_redeemer_list_get_length(redeemers);

  if ((plutus_data_size > 0U) && (redeemers_size == 0U))
  {
    result = cardano_cbor_writer_write_start_map(writer, 0);

    if (result != CARDANO_SUCCESS)
    {
      cardano_cbor_writer_unref(&writer);
      return result;
    }

    result = cardano_plutus_data_set_to_cbor(datums, writer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_cbor_writer_unref(&writer);
      return result;
    }

    result = cardano_cbor_writer_write_start_map(writer, 0);

    if (result != CARDANO_SUCCESS)
    {
      cardano_cbor_writer_unref(&writer);
      return result;
    }
  }
  else
  {
    if (redeemers_size == 0U)
    {
      *data_hash = NULL;
      cardano_cbor_writer_unref(&writer);

      return CARDANO_SUCCESS;
    }

    result = cardano_redeemer_list_to_cbor(redeemers, writer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_cbor_writer_unref(&writer);
      return result;
    }

    if (plutus_data_size > 0U)
    {
      result = cardano_plutus_data_set_to_cbor(datums, writer);

      if (result != CARDANO_SUCCESS)
      {
        cardano_cbor_writer_unref(&writer);
        return result;
      }
    }

    cardano_buffer_t* buffer = NULL;
    result                   = cardano_costmdls_get_language_views_encoding(costmdls, &buffer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_cbor_writer_unref(&writer);
      return result;
    }

    result = cardano_cbor_writer_write_encoded(writer, cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer));

    cardano_buffer_unref(&buffer);

    if (result != CARDANO_SUCCESS)
    {
      cardano_cbor_writer_unref(&writer);
      return result;
    }
  }

  cardano_buffer_t* preimage_buffer = NULL;
  result                            = cardano_cbor_writer_encode_in_buffer(writer, &preimage_buffer);

  if (result != CARDANO_SUCCESS)
  {
    cardano_cbor_writer_unref(&writer);
    return result;
  }

  cardano_blake2b_hash_t* hash = NULL;
  result                       = cardano_blake2b_compute_hash(
    cardano_buffer_get_data(preimage_buffer),
    cardano_buffer_get_size(preimage_buffer),
    CARDANO_BLAKE2B_HASH_SIZE_256,
    &hash);

  cardano_buffer_unref(&preimage_buffer);
  cardano_cbor_writer_unref(&writer);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *data_hash = hash;

  return CARDANO_SUCCESS;
}