/**
 * \file voting_procedures.c
 *
 * \author angel.castillo
 * \date   Aug 08, 2024
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

#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/voting_procedures/governance_action_id_list.h>
#include <cardano/voting_procedures/voter.h>
#include <cardano/voting_procedures/voter_list.h>
#include <cardano/voting_procedures/voting_procedures.h>

#include "../allocators.h"
#include "../collections/array.h"
#include "voting_procedure_map.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a Cardano voting procedure key value pair.
 */
typedef struct cardano_voting_procedure_kvp_t
{
    cardano_object_t                base;
    cardano_voter_t*                key;
    cardano_voting_procedure_map_t* value;
} cardano_voting_procedure_kvp_t;

/**
 * \brief Updates the voting procedure metadata.
 */
typedef struct cardano_voting_procedures_t
{
    cardano_object_t base;
    cardano_array_t* array;
} cardano_voting_procedures_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a voting_procedures object.
 *
 * This function is responsible for properly deallocating a voting_procedures object (`cardano_voting_procedures_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the voting_procedures object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_voting_procedures_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the voting_procedures
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_voting_procedures_deallocate(void* object)
{
  assert(object != NULL);

  cardano_voting_procedures_t* map = (cardano_voting_procedures_t*)object;

  if (map->array != NULL)
  {
    cardano_array_unref(&map->array);
  }

  _cardano_free(map);
}

/**
 * \brief Deallocates a voting_procedure map key value pair object.
 *
 * This function is responsible for properly deallocating a voting_procedure map key value pair object (`cardano_voting_procedure_map_kvp_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the voting_procedure_map object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_voting_procedure_map_kvp_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the voting_procedure_map
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_voting_procedure_kvp_deallocate(void* object)
{
  assert(object != NULL);

  cardano_voting_procedure_kvp_t* map = (cardano_voting_procedure_kvp_t*)object;

  if (map->key != NULL)
  {
    cardano_voter_unref(&map->key);
  }

  if (map->value != NULL)
  {
    cardano_voting_procedure_map_unref(&map->value);
  }

  _cardano_free(map);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_voting_procedures_new(cardano_voting_procedures_t** voting_procedures)
{
  if (voting_procedures == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_voting_procedures_t* map = _cardano_malloc(sizeof(cardano_voting_procedures_t));

  if (map == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  map->base.ref_count     = 1;
  map->base.last_error[0] = '\0';
  map->base.deallocator   = cardano_voting_procedures_deallocate;

  map->array = cardano_array_new(128);

  if (map->array == NULL)
  {
    _cardano_free(map);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *voting_procedures = map;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_voting_procedures_from_cbor(cardano_cbor_reader_t* reader, cardano_voting_procedures_t** voting_procedures)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (voting_procedures == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  int64_t         map_size          = 0;
  cardano_error_t expect_map_result = cardano_cbor_reader_read_start_map(reader, &map_size);

  CARDANO_UNUSED(map_size);

  if (expect_map_result != CARDANO_SUCCESS)
  {
    return expect_map_result;
  }

  cardano_voting_procedures_t* data       = NULL;
  cardano_error_t              new_result = cardano_voting_procedures_new(&data);

  if (new_result != CARDANO_SUCCESS)
  {
    return new_result;
  }

  cardano_cbor_reader_state_t state;

  cardano_error_t read_state = cardano_cbor_reader_peek_state(reader, &state);

  if (read_state != CARDANO_SUCCESS)
  {
    cardano_voting_procedures_unref(&data);
    return read_state;
  }

  while (state != CARDANO_CBOR_READER_STATE_END_MAP)
  {
    cardano_voter_t* voter = NULL;

    cardano_error_t read_voter_result = cardano_voter_from_cbor(reader, &voter);

    if (read_voter_result != CARDANO_SUCCESS)
    {
      cardano_voter_unref(&voter);
      cardano_voting_procedures_unref(&data);

      return read_voter_result;
    }

    int64_t         votes_map_size               = 0;
    cardano_error_t expect_votes_map_size_result = cardano_cbor_reader_read_start_map(reader, &votes_map_size);

    CARDANO_UNUSED(votes_map_size);

    if (expect_votes_map_size_result != CARDANO_SUCCESS)
    {
      cardano_voter_unref(&voter);
      cardano_voting_procedures_unref(&data);

      return expect_votes_map_size_result;
    }

    read_state = cardano_cbor_reader_peek_state(reader, &state);

    if (read_state != CARDANO_SUCCESS)
    {
      cardano_voter_unref(&voter);
      cardano_voting_procedures_unref(&data);
      return read_state;
    }

    while (state != CARDANO_CBOR_READER_STATE_END_MAP)
    {
      cardano_governance_action_id_t* action_id = NULL;
      cardano_voting_procedure_t*     vote      = NULL;

      cardano_error_t read_action_id_result = cardano_governance_action_id_from_cbor(reader, &action_id);

      if (read_action_id_result != CARDANO_SUCCESS)
      {
        cardano_governance_action_id_unref(&action_id);
        cardano_voting_procedure_unref(&vote);
        cardano_voter_unref(&voter);
        cardano_voting_procedures_unref(&data);

        return read_action_id_result;
      }

      cardano_error_t read_vote_result = cardano_voting_procedure_from_cbor(reader, &vote);

      if (read_vote_result != CARDANO_SUCCESS)
      {
        cardano_governance_action_id_unref(&action_id);
        cardano_voting_procedure_unref(&vote);
        cardano_voter_unref(&voter);
        cardano_voting_procedures_unref(&data);

        return read_vote_result;
      }

      cardano_error_t insert_result = cardano_voting_procedures_insert(data, voter, action_id, vote);

      if (insert_result != CARDANO_SUCCESS)
      {
        cardano_governance_action_id_unref(&action_id);
        cardano_voting_procedure_unref(&vote);
        cardano_voter_unref(&voter);
        cardano_voting_procedures_unref(&data);

        return insert_result;
      }

      cardano_governance_action_id_unref(&action_id);
      cardano_voting_procedure_unref(&vote);

      read_state = cardano_cbor_reader_peek_state(reader, &state);

      if (read_state != CARDANO_SUCCESS)
      {
        cardano_voter_unref(&voter);
        cardano_voting_procedures_unref(&data);
        return read_state;
      }
    }

    cardano_error_t end_inner_map_result = cardano_cbor_reader_read_end_map(reader);

    if (end_inner_map_result != CARDANO_SUCCESS)
    {
      cardano_voter_unref(&voter);
      cardano_voting_procedures_unref(&data);

      return end_inner_map_result;
    }

    read_state = cardano_cbor_reader_peek_state(reader, &state);

    if (read_state != CARDANO_SUCCESS)
    {
      cardano_voter_unref(&voter);
      cardano_voting_procedures_unref(&data);

      return read_state;
    }

    cardano_voter_unref(&voter);
  }

  *voting_procedures = data;

  return cardano_cbor_reader_read_end_map(reader);
}

cardano_error_t
cardano_voting_procedures_to_cbor(
  const cardano_voting_procedures_t* voting_procedures,
  cardano_cbor_writer_t*             writer)
{
  if (voting_procedures == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  size_t size = cardano_array_get_size(voting_procedures->array);

  cardano_error_t write_map_result = cardano_cbor_writer_write_start_map(writer, (int64_t)size);

  if (write_map_result != CARDANO_SUCCESS)
  {
    return write_map_result;
  }

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_voting_procedure_kvp_t* kvp = (cardano_voting_procedure_kvp_t*)((void*)cardano_array_get(voting_procedures->array, i));

    if (kvp == NULL)
    {
      return CARDANO_ERROR_ELEMENT_NOT_FOUND;
    }

    cardano_error_t write_voter_result = cardano_voter_to_cbor(kvp->key, writer);

    if (write_voter_result != CARDANO_SUCCESS)
    {
      cardano_object_unref((cardano_object_t**)((void*)&kvp));

      return write_voter_result;
    }

    size_t votes_size = cardano_voting_procedure_map_get_length(kvp->value);

    cardano_error_t write_votes_map_result = cardano_cbor_writer_write_start_map(writer, (int64_t)votes_size);

    if (write_votes_map_result != CARDANO_SUCCESS)
    {
      cardano_object_unref((cardano_object_t**)((void*)&kvp));

      return write_votes_map_result;
    }

    cardano_governance_action_id_list_t* action_ids = NULL;

    cardano_error_t get_keys_result = cardano_voting_procedure_map_get_keys(kvp->value, &action_ids);

    if (get_keys_result != CARDANO_SUCCESS)
    {
      cardano_object_unref((cardano_object_t**)((void*)&kvp));

      return get_keys_result;
    }

    size_t action_ids_size = cardano_governance_action_id_list_get_length(action_ids);

    for (size_t j = 0U; j < action_ids_size; ++j)
    {
      cardano_governance_action_id_t* action_id = NULL;

      cardano_error_t get_action_id_result = cardano_governance_action_id_list_get(action_ids, j, &action_id);

      if (get_action_id_result != CARDANO_SUCCESS)
      {
        cardano_governance_action_id_list_unref(&action_ids);
        cardano_object_unref((cardano_object_t**)((void*)&kvp));

        return get_action_id_result;
      }

      cardano_voting_procedure_t* vote = NULL;

      cardano_error_t get_vote_result = cardano_voting_procedure_map_get(kvp->value, action_id, &vote);

      if (get_vote_result != CARDANO_SUCCESS)
      {
        cardano_governance_action_id_unref(&action_id);
        cardano_governance_action_id_list_unref(&action_ids);
        cardano_object_unref((cardano_object_t**)((void*)&kvp));

        return get_vote_result;
      }

      cardano_error_t write_action_id_result = cardano_governance_action_id_to_cbor(action_id, writer);

      if (write_action_id_result != CARDANO_SUCCESS)
      {
        cardano_governance_action_id_unref(&action_id);
        cardano_governance_action_id_list_unref(&action_ids);
        cardano_object_unref((cardano_object_t**)((void*)&kvp));

        return write_action_id_result;
      }

      cardano_error_t write_vote_result = cardano_voting_procedure_to_cbor(vote, writer);

      if (write_vote_result != CARDANO_SUCCESS)
      {
        cardano_governance_action_id_unref(&action_id);
        cardano_governance_action_id_list_unref(&action_ids);
        cardano_object_unref((cardano_object_t**)((void*)&kvp));

        return write_vote_result;
      }

      cardano_governance_action_id_unref(&action_id);
      cardano_voting_procedure_unref(&vote);
    }

    cardano_governance_action_id_list_unref(&action_ids);
    cardano_object_unref((cardano_object_t**)((void*)&kvp));
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_voting_procedures_insert(
  cardano_voting_procedures_t*    voting_procedures,
  cardano_voter_t*                voter,
  cardano_governance_action_id_t* governance_action_id,
  cardano_voting_procedure_t*     value)
{
  if (voting_procedures == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (voter == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (governance_action_id == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (value == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  size_t size = cardano_array_get_size(voting_procedures->array);

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_voting_procedure_kvp_t* kvp = (cardano_voting_procedure_kvp_t*)((void*)cardano_array_get(voting_procedures->array, i));

    if (kvp == NULL)
    {
      return CARDANO_ERROR_ELEMENT_NOT_FOUND;
    }

    if (cardano_voter_equals(kvp->key, voter))
    {
      cardano_error_t insert_result = cardano_voting_procedure_map_insert(kvp->value, governance_action_id, value);

      if (insert_result != CARDANO_SUCCESS)
      {
        cardano_object_unref((cardano_object_t**)((void*)&kvp));

        return insert_result;
      }

      cardano_object_unref((cardano_object_t**)((void*)&kvp));

      return CARDANO_SUCCESS;
    }

    cardano_object_unref((cardano_object_t**)((void*)&kvp));
  }

  cardano_voting_procedure_map_t* map               = NULL;
  cardano_error_t                 create_map_result = cardano_voting_procedure_map_new(&map);

  if (create_map_result != CARDANO_SUCCESS)
  {
    return create_map_result;
  }

  cardano_error_t insert_result = cardano_voting_procedure_map_insert(map, governance_action_id, value);

  if (insert_result != CARDANO_SUCCESS)
  {
    cardano_voting_procedure_map_unref(&map);
    return insert_result;
  }

  cardano_voting_procedure_kvp_t* kvp = _cardano_malloc(sizeof(cardano_voting_procedure_kvp_t));

  if (kvp == NULL)
  {
    cardano_voting_procedure_map_unref(&map);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  kvp->base.ref_count     = 0;
  kvp->base.last_error[0] = '\0';
  kvp->base.deallocator   = cardano_voting_procedure_kvp_deallocate;
  kvp->key                = voter;
  kvp->value              = map;

  cardano_voter_ref(voter);

  const size_t old_size = cardano_array_get_size(voting_procedures->array);
  const size_t new_size = cardano_array_push(voting_procedures->array, (cardano_object_t*)((void*)kvp));

  if (new_size != (old_size + 1U))
  {
    cardano_voting_procedure_kvp_deallocate(kvp);
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  return CARDANO_SUCCESS;
}

cardano_voting_procedure_t*
cardano_voting_procedures_get(
  cardano_voting_procedures_t*    voting_procedures,
  cardano_voter_t*                voter,
  cardano_governance_action_id_t* governance_action_id)
{
  if (voting_procedures == NULL)
  {
    return NULL;
  }

  if (voter == NULL)
  {
    return NULL;
  }

  if (governance_action_id == NULL)
  {
    return NULL;
  }

  size_t size = cardano_array_get_size(voting_procedures->array);

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_voting_procedure_kvp_t* kvp = (cardano_voting_procedure_kvp_t*)((void*)cardano_array_get(voting_procedures->array, i));

    if (kvp == NULL)
    {
      return NULL;
    }

    if (cardano_voter_equals(kvp->key, voter))
    {
      cardano_voting_procedure_t* procedure = NULL;

      cardano_error_t get_element_result = cardano_voting_procedure_map_get(kvp->value, governance_action_id, &procedure);

      cardano_object_unref((cardano_object_t**)((void*)&kvp));

      if (get_element_result != CARDANO_SUCCESS)
      {
        return NULL;
      }

      return procedure;
    }

    cardano_object_unref((cardano_object_t**)((void*)&kvp));
  }

  return NULL;
}

cardano_error_t
cardano_voting_procedures_get_governance_ids_by_voter(
  cardano_voting_procedures_t*          voting_procedures,
  cardano_voter_t*                      voter,
  cardano_governance_action_id_list_t** governance_action_ids)
{
  if (voting_procedures == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (voter == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (governance_action_ids == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  size_t size = cardano_array_get_size(voting_procedures->array);

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_voting_procedure_kvp_t* kvp = (cardano_voting_procedure_kvp_t*)((void*)cardano_array_get(voting_procedures->array, i));

    if (kvp == NULL)
    {
      return CARDANO_ERROR_ELEMENT_NOT_FOUND;
    }

    if (cardano_voter_equals(kvp->key, voter))
    {
      cardano_error_t get_result = cardano_voting_procedure_map_get_keys(kvp->value, governance_action_ids);

      if (get_result != CARDANO_SUCCESS)
      {
        cardano_object_unref((cardano_object_t**)((void*)&kvp));
        return get_result;
      }
    }

    cardano_object_unref((cardano_object_t**)((void*)&kvp));
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_voting_procedures_get_voters(cardano_voting_procedures_t* voting_procedures, cardano_voter_list_t** voters)
{
  if (voting_procedures == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (voters == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_voter_list_t* list = NULL;

  cardano_error_t create_list_result = cardano_voter_list_new(&list);

  if (create_list_result != CARDANO_SUCCESS)
  {
    return create_list_result;
  }

  size_t size = cardano_array_get_size(voting_procedures->array);

  for (size_t i = 0U; i < size; ++i)
  {
    cardano_voting_procedure_kvp_t* kvp = (cardano_voting_procedure_kvp_t*)((void*)cardano_array_get(voting_procedures->array, i));

    if (kvp == NULL)
    {
      cardano_voter_list_unref(&list);
      return CARDANO_ERROR_ELEMENT_NOT_FOUND;
    }

    cardano_object_unref((cardano_object_t**)((void*)&kvp));

    cardano_error_t add_result = cardano_voter_list_add(list, kvp->key);

    if (add_result != CARDANO_SUCCESS)
    {
      cardano_voter_list_unref(&list);
      return add_result;
    }
  }

  *voters = list;

  return CARDANO_SUCCESS;
}

void
cardano_voting_procedures_unref(cardano_voting_procedures_t** voting_procedures)
{
  if ((voting_procedures == NULL) || (*voting_procedures == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*voting_procedures)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *voting_procedures = NULL;
    return;
  }
}

void
cardano_voting_procedures_ref(cardano_voting_procedures_t* voting_procedures)
{
  if (voting_procedures == NULL)
  {
    return;
  }

  cardano_object_ref(&voting_procedures->base);
}

size_t
cardano_voting_procedures_refcount(const cardano_voting_procedures_t* voting_procedures)
{
  if (voting_procedures == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&voting_procedures->base);
}

void
cardano_voting_procedures_set_last_error(cardano_voting_procedures_t* voting_procedures, const char* message)
{
  cardano_object_set_last_error(&voting_procedures->base, message);
}

const char*
cardano_voting_procedures_get_last_error(const cardano_voting_procedures_t* voting_procedures)
{
  return cardano_object_get_last_error(&voting_procedures->base);
}
