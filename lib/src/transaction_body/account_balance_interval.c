/**
 * \file account_balance_interval.c
 *
 * \author angel.castillo
 * \date   Jul 16, 2026
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

/* INCLUDES ******************************************************************/

#include <cardano/object.h>
#include <cardano/transaction_body/account_balance_interval.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t ACCOUNT_BALANCE_INTERVAL_EMBEDDED_GROUP_SIZE = 2;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents a half open interval of lovelace amounts.
 */
typedef struct cardano_account_balance_interval_t
{
    cardano_object_t base;
    bool             has_inclusive_lower_bound;
    uint64_t         inclusive_lower_bound;
    bool             has_exclusive_upper_bound;
    uint64_t         exclusive_upper_bound;
} cardano_account_balance_interval_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates an account balance interval object.
 *
 * This function is responsible for properly deallocating an account balance interval object (`cardano_account_balance_interval_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the account_balance_interval object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_account_balance_interval_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the account_balance_interval
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_account_balance_interval_deallocate(void* object)
{
  assert(object != NULL);
  _cardano_free(object);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_account_balance_interval_new(
  const uint64_t*                      inclusive_lower_bound,
  const uint64_t*                      exclusive_upper_bound,
  cardano_account_balance_interval_t** account_balance_interval)
{
  if (account_balance_interval == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((inclusive_lower_bound == NULL) && (exclusive_upper_bound == NULL))
  {
    *account_balance_interval = NULL;
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  *account_balance_interval = _cardano_malloc(sizeof(cardano_account_balance_interval_t));

  if (*account_balance_interval == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  (*account_balance_interval)->base.deallocator   = cardano_account_balance_interval_deallocate;
  (*account_balance_interval)->base.ref_count     = 1;
  (*account_balance_interval)->base.last_error[0] = '\0';

  (*account_balance_interval)->has_inclusive_lower_bound = (inclusive_lower_bound != NULL);
  (*account_balance_interval)->inclusive_lower_bound     = (inclusive_lower_bound != NULL) ? *inclusive_lower_bound : 0U;
  (*account_balance_interval)->has_exclusive_upper_bound = (exclusive_upper_bound != NULL);
  (*account_balance_interval)->exclusive_upper_bound     = (exclusive_upper_bound != NULL) ? *exclusive_upper_bound : 0U;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_account_balance_interval_from_cbor(cardano_cbor_reader_t* reader, cardano_account_balance_interval_t** account_balance_interval)
{
  if (account_balance_interval == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (reader == NULL)
  {
    *account_balance_interval = NULL;
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "account_balance_interval";

  const cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)ACCOUNT_BALANCE_INTERVAL_EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    *account_balance_interval = NULL;
    return expect_array_result;
  }

  bool     has_inclusive_lower_bound = false;
  uint64_t inclusive_lower_bound     = 0U;
  bool     has_exclusive_upper_bound = false;
  uint64_t exclusive_upper_bound     = 0U;

  cardano_cbor_reader_state_t state = CARDANO_CBOR_READER_STATE_UNDEFINED;

  cardano_error_t result = cardano_cbor_reader_peek_state(reader, &state);

  if (result != CARDANO_SUCCESS)
  {
    *account_balance_interval = NULL;
    return result;
  }

  if (state == CARDANO_CBOR_READER_STATE_NULL)
  {
    result = cardano_cbor_reader_read_null(reader);
  }
  else
  {
    result                    = cardano_cbor_reader_read_uint(reader, &inclusive_lower_bound);
    has_inclusive_lower_bound = true;
  }

  if (result != CARDANO_SUCCESS)
  {
    *account_balance_interval = NULL;
    return result;
  }

  result = cardano_cbor_reader_peek_state(reader, &state);

  if (result != CARDANO_SUCCESS)
  {
    *account_balance_interval = NULL;
    return result;
  }

  if (state == CARDANO_CBOR_READER_STATE_NULL)
  {
    result = cardano_cbor_reader_read_null(reader);
  }
  else
  {
    result                    = cardano_cbor_reader_read_uint(reader, &exclusive_upper_bound);
    has_exclusive_upper_bound = true;
  }

  if (result != CARDANO_SUCCESS)
  {
    *account_balance_interval = NULL;
    return result;
  }

  const cardano_error_t expect_end_array_result = cardano_cbor_validate_end_array(validator_name, reader);

  if (expect_end_array_result != CARDANO_SUCCESS)
  {
    *account_balance_interval = NULL;
    return expect_end_array_result;
  }

  if (!has_inclusive_lower_bound && !has_exclusive_upper_bound)
  {
    cardano_cbor_reader_set_last_error(reader, "Both interval bounds cannot be nil.");
    *account_balance_interval = NULL;

    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  return cardano_account_balance_interval_new(
    has_inclusive_lower_bound ? &inclusive_lower_bound : NULL,
    has_exclusive_upper_bound ? &exclusive_upper_bound : NULL,
    account_balance_interval);
}

cardano_error_t
cardano_account_balance_interval_to_cbor(const cardano_account_balance_interval_t* account_balance_interval, cardano_cbor_writer_t* writer)
{
  if (account_balance_interval == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t result = cardano_cbor_writer_write_start_array(writer, ACCOUNT_BALANCE_INTERVAL_EMBEDDED_GROUP_SIZE);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (account_balance_interval->has_inclusive_lower_bound)
  {
    result = cardano_cbor_writer_write_uint(writer, account_balance_interval->inclusive_lower_bound);
  }
  else
  {
    result = cardano_cbor_writer_write_null(writer);
  }

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (account_balance_interval->has_exclusive_upper_bound)
  {
    result = cardano_cbor_writer_write_uint(writer, account_balance_interval->exclusive_upper_bound);
  }
  else
  {
    result = cardano_cbor_writer_write_null(writer);
  }

  return result;
}

const uint64_t*
cardano_account_balance_interval_get_inclusive_lower_bound(const cardano_account_balance_interval_t* account_balance_interval)
{
  if (account_balance_interval == NULL)
  {
    return NULL;
  }

  if (!account_balance_interval->has_inclusive_lower_bound)
  {
    return NULL;
  }

  return &account_balance_interval->inclusive_lower_bound;
}

const uint64_t*
cardano_account_balance_interval_get_exclusive_upper_bound(const cardano_account_balance_interval_t* account_balance_interval)
{
  if (account_balance_interval == NULL)
  {
    return NULL;
  }

  if (!account_balance_interval->has_exclusive_upper_bound)
  {
    return NULL;
  }

  return &account_balance_interval->exclusive_upper_bound;
}

void
cardano_account_balance_interval_unref(cardano_account_balance_interval_t** account_balance_interval)
{
  if ((account_balance_interval == NULL) || (*account_balance_interval == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*account_balance_interval)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *account_balance_interval = NULL;
    return;
  }
}

void
cardano_account_balance_interval_ref(cardano_account_balance_interval_t* account_balance_interval)
{
  if (account_balance_interval == NULL)
  {
    return;
  }

  cardano_object_ref(&account_balance_interval->base);
}

size_t
cardano_account_balance_interval_refcount(const cardano_account_balance_interval_t* account_balance_interval)
{
  if (account_balance_interval == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&account_balance_interval->base);
}

void
cardano_account_balance_interval_set_last_error(cardano_account_balance_interval_t* account_balance_interval, const char* message)
{
  cardano_object_set_last_error(&account_balance_interval->base, message);
}

const char*
cardano_account_balance_interval_get_last_error(const cardano_account_balance_interval_t* account_balance_interval)
{
  return cardano_object_get_last_error(&account_balance_interval->base);
}
