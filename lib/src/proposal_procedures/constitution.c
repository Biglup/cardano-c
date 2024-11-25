/**
 * \file constitution.c
 *
 * \author angel.castillo
 * \date   Aug 15, 2024
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

#include <cardano/common/anchor.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/proposal_procedures/constitution.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief The Cardano Constitution is a text document that defines Cardano's shared values and guiding principles.
 * At this stage, the Constitution is an informational document that unambiguously captures the core values
 * of Cardano and acts to ensure its long-term sustainability. At a later stage, we can imagine the Constitution
 * perhaps evolving into a smart-contract based set of rules that drives the entire governance framework.
 *
 * For now, however, the Constitution will remain an off-chain document whose hash digest value will be recorded
 * on-chain.
 */
typedef struct cardano_constitution_t
{
    cardano_object_t        base;
    cardano_anchor_t*       anchor;
    cardano_blake2b_hash_t* script_hash;
} cardano_constitution_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a constitution object.
 *
 * This function is responsible for properly deallocating a constitution object (`cardano_constitution_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the constitution object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_constitution_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the constitution
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_constitution_deallocate(void* object)
{
  assert(object != NULL);

  cardano_constitution_t* data = (cardano_constitution_t*)object;

  cardano_anchor_unref(&data->anchor);
  cardano_blake2b_hash_unref(&data->script_hash);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_constitution_new(
  cardano_anchor_t*        anchor,
  cardano_blake2b_hash_t*  script_hash,
  cardano_constitution_t** constitution)
{
  if (anchor == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (constitution == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_constitution_t* data = _cardano_malloc(sizeof(cardano_constitution_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_constitution_deallocate;
  data->script_hash        = NULL;

  cardano_anchor_ref(anchor);
  data->anchor = anchor;

  if (script_hash != NULL)
  {
    cardano_blake2b_hash_ref(script_hash);
    data->script_hash = script_hash;
  }

  *constitution = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_constitution_from_cbor(cardano_cbor_reader_t* reader, cardano_constitution_t** constitution)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (constitution == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "constitution";

  cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    return expect_array_result;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_error_t read_quorum_result = cardano_anchor_from_cbor(reader, &anchor);

  if (read_quorum_result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    return read_quorum_result;
  }

  cardano_blake2b_hash_t* script_hash = NULL;

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_NULL;

  cardano_error_t read_map_result = cardano_cbor_reader_peek_state(reader, &state);

  if (read_map_result != CARDANO_SUCCESS)
  {
    cardano_anchor_unref(&anchor);
    return read_map_result;
  }

  if (state == CARDANO_CBOR_READER_STATE_NULL)
  {
    cardano_error_t read_null = cardano_cbor_reader_read_null(reader);

    if (read_null != CARDANO_SUCCESS)
    {
      cardano_anchor_unref(&anchor);
      return read_null;
    }
  }
  else
  {
    cardano_error_t read_hash_result = cardano_blake2b_hash_from_cbor(reader, &script_hash);

    if (read_hash_result != CARDANO_SUCCESS)
    {
      cardano_anchor_unref(&anchor);
      cardano_blake2b_hash_unref(&script_hash);

      return read_hash_result;
    }
  }

  cardano_error_t result = cardano_constitution_new(anchor, script_hash, constitution);

  cardano_anchor_unref(&anchor);
  cardano_blake2b_hash_unref(&script_hash);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return cardano_cbor_validate_end_array(validator_name, reader);
}

cardano_error_t
cardano_constitution_to_cbor(
  const cardano_constitution_t* constitution,
  cardano_cbor_writer_t*        writer)
{
  if (constitution == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_array_result = cardano_cbor_writer_write_start_array(writer, EMBEDDED_GROUP_SIZE);

  if (write_array_result != CARDANO_SUCCESS)
  {
    return write_array_result;
  }

  cardano_error_t write_anchor_result = cardano_anchor_to_cbor(constitution->anchor, writer);

  if (write_anchor_result != CARDANO_SUCCESS)
  {
    return write_anchor_result;
  }

  if (constitution->script_hash == NULL)
  {
    cardano_error_t write_null_result = cardano_cbor_writer_write_null(writer);

    if (write_null_result != CARDANO_SUCCESS)
    {
      return write_null_result;
    }
  }
  else
  {
    cardano_error_t write_hash_result = cardano_blake2b_hash_to_cbor(constitution->script_hash, writer);

    if (write_hash_result != CARDANO_SUCCESS)
    {
      return write_hash_result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_constitution_set_anchor(cardano_constitution_t* constitution, cardano_anchor_t* anchor)
{
  if (constitution == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (anchor == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_anchor_ref(anchor);
  cardano_anchor_unref(&constitution->anchor);
  constitution->anchor = anchor;

  return CARDANO_SUCCESS;
}

cardano_anchor_t*
cardano_constitution_get_anchor(cardano_constitution_t* constitution)
{
  if (constitution == NULL)
  {
    return NULL;
  }

  cardano_anchor_ref(constitution->anchor);

  return constitution->anchor;
}

cardano_error_t
cardano_constitution_set_script_hash(cardano_constitution_t* constitution, cardano_blake2b_hash_t* script_hash)
{
  if (constitution == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (script_hash == NULL)
  {
    cardano_blake2b_hash_unref(&constitution->script_hash);
    constitution->script_hash = script_hash;
  }
  else
  {
    cardano_blake2b_hash_ref(script_hash);
    cardano_blake2b_hash_unref(&constitution->script_hash);
    constitution->script_hash = script_hash;
  }

  return CARDANO_SUCCESS;
}

cardano_blake2b_hash_t*
cardano_constitution_get_script_hash(cardano_constitution_t* constitution)
{
  if (constitution == NULL)
  {
    return NULL;
  }

  cardano_blake2b_hash_ref(constitution->script_hash);

  return constitution->script_hash;
}

void
cardano_constitution_unref(cardano_constitution_t** constitution)
{
  if ((constitution == NULL) || (*constitution == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*constitution)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *constitution = NULL;
    return;
  }
}

void
cardano_constitution_ref(cardano_constitution_t* constitution)
{
  if (constitution == NULL)
  {
    return;
  }

  cardano_object_ref(&constitution->base);
}

size_t
cardano_constitution_refcount(const cardano_constitution_t* constitution)
{
  if (constitution == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&constitution->base);
}

void
cardano_constitution_set_last_error(cardano_constitution_t* constitution, const char* message)
{
  cardano_object_set_last_error(&constitution->base, message);
}

const char*
cardano_constitution_get_last_error(const cardano_constitution_t* constitution)
{
  return cardano_object_get_last_error(&constitution->base);
}
