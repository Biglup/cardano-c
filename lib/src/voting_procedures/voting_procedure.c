/**
 * \file voting_procedure.c
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

#include <cardano/certs/cert_type.h>
#include <cardano/common/anchor.h>
#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/voting_procedures/vote.h>
#include <cardano/voting_procedures/voting_procedure.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief Updates the DRep anchored metadata.
 */
typedef struct cardano_voting_procedure_t
{
    cardano_object_t  base;
    cardano_vote_t    vote;
    cardano_anchor_t* anchor;
} cardano_voting_procedure_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a voting_procedure object.
 *
 * This function is responsible for properly deallocating a voting_procedure object (`cardano_voting_procedure_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the voting_procedure object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_voting_procedure_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the voting_procedure
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_voting_procedure_deallocate(void* object)
{
  assert(object != NULL);

  cardano_voting_procedure_t* data = (cardano_voting_procedure_t*)object;

  cardano_anchor_unref(&data->anchor);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_voting_procedure_new(
  const cardano_vote_t         vote,
  cardano_anchor_t*            anchor,
  cardano_voting_procedure_t** voting_procedure)
{
  if (voting_procedure == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_voting_procedure_t* data = _cardano_malloc(sizeof(cardano_voting_procedure_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_voting_procedure_deallocate;

  if (anchor != NULL)
  {
    cardano_anchor_ref(anchor);
  }

  data->anchor = anchor;
  data->vote   = vote;

  *voting_procedure = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_voting_procedure_from_cbor(cardano_cbor_reader_t* reader, cardano_voting_procedure_t** voting_procedure)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (voting_procedure == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "voting_procedure";

  cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    return expect_array_result;
  }

  uint64_t              type             = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_uint_in_range(
    validator_name,
    "voter_type",
    reader,
    &type,
    CARDANO_VOTE_NO,
    CARDANO_VOTE_ABSTAIN);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_cbor_reader_state_t state;

  cardano_error_t read_state = cardano_cbor_reader_peek_state(reader, &state);

  if (read_state != CARDANO_SUCCESS)
  {
    return read_state;
  }

  if (state == CARDANO_CBOR_READER_STATE_NULL)
  {
    cardano_error_t read_null = cardano_cbor_reader_read_null(reader);
    CARDANO_UNUSED(read_null);
  }
  else
  {
    cardano_error_t read_anchor_result = cardano_anchor_from_cbor(reader, &anchor);

    if (read_anchor_result != CARDANO_SUCCESS)
    {
      return read_anchor_result;
    }
  }

  cardano_error_t new_result = cardano_voting_procedure_new((cardano_vote_t)type, anchor, voting_procedure);

  cardano_anchor_unref(&anchor);

  if (new_result != CARDANO_SUCCESS)
  {
    return new_result;
  }

  return cardano_cbor_validate_end_array(validator_name, reader);
}

cardano_error_t
cardano_voting_procedure_to_cbor(
  const cardano_voting_procedure_t* voting_procedure,
  cardano_cbor_writer_t*            writer)
{
  if (voting_procedure == NULL)
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

  cardano_error_t write_uint_result = cardano_cbor_writer_write_uint(writer, voting_procedure->vote);

  if (write_uint_result != CARDANO_SUCCESS)
  {
    return write_uint_result;
  }

  if (voting_procedure->anchor != NULL)
  {
    cardano_error_t write_anchor_result = cardano_anchor_to_cbor(voting_procedure->anchor, writer);

    if (write_anchor_result != CARDANO_SUCCESS)
    {
      return write_anchor_result;
    }
  }
  else
  {
    cardano_error_t write_null_result = cardano_cbor_writer_write_null(writer);

    if (write_null_result != CARDANO_SUCCESS)
    {
      return write_null_result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_vote_t
cardano_voting_procedure_get_vote(const cardano_voting_procedure_t* voting_procedure)
{
  if (voting_procedure == NULL)
  {
    return CARDANO_VOTE_NO;
  }

  return voting_procedure->vote;
}

cardano_error_t
cardano_voting_procedure_set_vote(
  cardano_voting_procedure_t* voting_procedure,
  const cardano_vote_t        vote)
{
  if (voting_procedure == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  voting_procedure->vote = vote;

  return CARDANO_SUCCESS;
}

cardano_anchor_t*
cardano_voting_procedure_get_anchor(cardano_voting_procedure_t* certificate)
{
  if (certificate == NULL)
  {
    return NULL;
  }

  cardano_anchor_ref(certificate->anchor);

  return certificate->anchor;
}

cardano_error_t
cardano_voting_procedure_set_anchor(
  cardano_voting_procedure_t* certificate,
  cardano_anchor_t*           anchor)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (anchor == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_anchor_ref(anchor);
  cardano_anchor_unref(&certificate->anchor);
  certificate->anchor = anchor;

  return CARDANO_SUCCESS;
}

void
cardano_voting_procedure_unref(cardano_voting_procedure_t** voting_procedure)
{
  if ((voting_procedure == NULL) || (*voting_procedure == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*voting_procedure)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *voting_procedure = NULL;
    return;
  }
}

void
cardano_voting_procedure_ref(cardano_voting_procedure_t* voting_procedure)
{
  if (voting_procedure == NULL)
  {
    return;
  }

  cardano_object_ref(&voting_procedure->base);
}

size_t
cardano_voting_procedure_refcount(const cardano_voting_procedure_t* voting_procedure)
{
  if (voting_procedure == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&voting_procedure->base);
}

void
cardano_voting_procedure_set_last_error(cardano_voting_procedure_t* voting_procedure, const char* message)
{
  cardano_object_set_last_error(&voting_procedure->base, message);
}

const char*
cardano_voting_procedure_get_last_error(const cardano_voting_procedure_t* voting_procedure)
{
  return cardano_object_get_last_error(&voting_procedure->base);
}
