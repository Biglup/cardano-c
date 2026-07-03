/**
 * \file deferred_redeemer_list.cpp
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

/* INCLUDES ******************************************************************/

#include <cardano/error.h>

#include <cardano/transaction_builder/balancing/deferred_redeemer_list.h>

#include "../../allocators_helpers.h"
#include "../src/allocators.h"

#include <cardano/common/utxo.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/transaction/transaction.h>
#include <cardano/witness_set/redeemer.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* REDEEMER_CBOR = "840000d8799f0102030405ff821821182c";
static const char* TX_CBOR       = "84a300d9010282825820027b68d4c11e97d7e065cc2702912cb1a21b6d0e56c6a74dd605889a5561138500825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a633010182a2005839005cf6c91279a859a072601779fb33bb07c34e1d641d45df51ff63b967f15db05f56035465bf8900a09bdaa16c3d8b8244fea686524408dd8001821a00e4e1c0a1581c0b0d621b5c26d0a1fd0893a4b04c19d860296a69ede1fbcfc5179882a1474e46542d30303101a200583900dc435fc2638f6684bd1f9f6f917d80c92ae642a4a33a412e516479e64245236ab8056760efceebbff57e8cab220182be3e36439e520a6454011a0d294e28021a00029eb9a0f5f6";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the redeemer.
 * @return A new instance of the redeemer.
 */
static cardano_redeemer_t*
new_default_redeemer()
{
  cardano_redeemer_t*    redeemer = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(REDEEMER_CBOR, strlen(REDEEMER_CBOR));
  cardano_error_t        result   = cardano_redeemer_from_cbor(reader, &redeemer);

  cardano_redeemer_clear_cbor_cache(redeemer);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return redeemer;
};

/**
 * Creates a new default instance of the transaction.
 * @return A new instance of the transaction.
 */
static cardano_transaction_t*
new_default_transaction()
{
  cardano_transaction_t* transaction = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(TX_CBOR, strlen(TX_CBOR));
  cardano_error_t        result      = cardano_transaction_from_cbor(reader, &transaction);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return transaction;
};

/**
 * Context shared with the probe callback so tests can observe how the list drives it.
 */
typedef struct resolve_probe_t
{
    cardano_transaction_t* expected_tx;
    cardano_utxo_list_t*   expected_inputs;
    int64_t                payload_value;
    int32_t                call_count;
} resolve_probe_t;

/**
 * Deferred redeemer callback that records its invocation, asserts it receives the registered
 * arguments and produces an integer payload.
 */
static cardano_error_t
probe_callback(void* user_context, cardano_transaction_t* draft_tx, cardano_utxo_list_t* resolved_inputs, cardano_plutus_data_t** redeemer)
{
  resolve_probe_t* probe = (resolve_probe_t*)user_context;

  EXPECT_EQ(draft_tx, probe->expected_tx);
  EXPECT_EQ(resolved_inputs, probe->expected_inputs);

  ++probe->call_count;

  return cardano_plutus_data_new_integer_from_int(probe->payload_value, redeemer);
}

/**
 * Deferred redeemer callback that always fails.
 */
static cardano_error_t
failing_callback(void* user_context, cardano_transaction_t* draft_tx, cardano_utxo_list_t* resolved_inputs, cardano_plutus_data_t** redeemer)
{
  CARDANO_UNUSED(draft_tx);
  CARDANO_UNUSED(resolved_inputs);
  CARDANO_UNUSED(redeemer);

  if (user_context != NULL)
  {
    ++((resolve_probe_t*)user_context)->call_count;
  }

  return CARDANO_ERROR_INVALID_ARGUMENT;
}

/**
 * Deferred redeemer callback that reports success without producing a payload.
 */
static cardano_error_t
null_payload_callback(void* user_context, cardano_transaction_t* draft_tx, cardano_utxo_list_t* resolved_inputs, cardano_plutus_data_t** redeemer)
{
  CARDANO_UNUSED(user_context);
  CARDANO_UNUSED(draft_tx);
  CARDANO_UNUSED(resolved_inputs);
  CARDANO_UNUSED(redeemer);

  return CARDANO_SUCCESS;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_deferred_redeemer_list_new, canCreateList)
{
  // Arrange
  cardano_deferred_redeemer_list_t* list = NULL;

  // Act
  cardano_error_t result = cardano_deferred_redeemer_list_new(&list);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(list, nullptr);
  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(list), 0U);
  EXPECT_EQ(cardano_deferred_redeemer_list_refcount(list), 1U);

  // Cleanup
  cardano_deferred_redeemer_list_unref(&list);
}

TEST(cardano_deferred_redeemer_list_new, returnsErrorIfGivenNull)
{
  // Act
  cardano_error_t result = cardano_deferred_redeemer_list_new(NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_deferred_redeemer_list_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_deferred_redeemer_list_t* list = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_deferred_redeemer_list_new(&list);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(list, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_deferred_redeemer_list_add, canAddEntries)
{
  // Arrange
  cardano_deferred_redeemer_list_t* list     = NULL;
  cardano_redeemer_t*               redeemer = new_default_redeemer();

  EXPECT_EQ(cardano_deferred_redeemer_list_new(&list), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_deferred_redeemer_list_add(list, redeemer, probe_callback, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(list), 1U);

  EXPECT_EQ(cardano_deferred_redeemer_list_add(list, redeemer, probe_callback, NULL), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(list), 2U);

  // Cleanup
  cardano_deferred_redeemer_list_unref(&list);
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_deferred_redeemer_list_add, growsCapacityWhenNeeded)
{
  // Arrange
  cardano_deferred_redeemer_list_t* list     = NULL;
  cardano_redeemer_t*               redeemer = new_default_redeemer();

  EXPECT_EQ(cardano_deferred_redeemer_list_new(&list), CARDANO_SUCCESS);

  // Act
  for (size_t i = 0U; i < 9U; ++i)
  {
    EXPECT_EQ(cardano_deferred_redeemer_list_add(list, redeemer, probe_callback, NULL), CARDANO_SUCCESS);
  }

  // Assert
  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(list), 9U);

  // Cleanup
  cardano_deferred_redeemer_list_unref(&list);
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_deferred_redeemer_list_add, returnsErrorIfGivenNullArguments)
{
  // Arrange
  cardano_deferred_redeemer_list_t* list     = NULL;
  cardano_redeemer_t*               redeemer = new_default_redeemer();

  EXPECT_EQ(cardano_deferred_redeemer_list_new(&list), CARDANO_SUCCESS);

  // Act & Assert
  EXPECT_EQ(cardano_deferred_redeemer_list_add(NULL, redeemer, probe_callback, NULL), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_deferred_redeemer_list_add(list, NULL, probe_callback, NULL), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_deferred_redeemer_list_add(list, redeemer, NULL, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(list), 0U);

  // Cleanup
  cardano_deferred_redeemer_list_unref(&list);
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_deferred_redeemer_list_add, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_deferred_redeemer_list_t* list     = NULL;
  cardano_redeemer_t*               redeemer = new_default_redeemer();

  EXPECT_EQ(cardano_deferred_redeemer_list_new(&list), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_deferred_redeemer_list_add(list, redeemer, probe_callback, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(list), 0U);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_deferred_redeemer_list_unref(&list);
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_deferred_redeemer_list_get_length, returnsZeroIfGivenNull)
{
  // Act & Assert
  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(NULL), 0U);
}

TEST(cardano_deferred_redeemer_list_resolve, succeedsIfListIsNull)
{
  // Arrange
  cardano_transaction_t* transaction = new_default_transaction();
  cardano_utxo_list_t*   inputs      = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&inputs), CARDANO_SUCCESS);

  // Act & Assert
  EXPECT_EQ(cardano_deferred_redeemer_list_resolve(NULL, transaction, inputs), CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_unref(&transaction);
  cardano_utxo_list_unref(&inputs);
}

TEST(cardano_deferred_redeemer_list_resolve, returnsErrorIfGivenNullArguments)
{
  // Arrange
  cardano_deferred_redeemer_list_t* list        = NULL;
  cardano_transaction_t*            transaction = new_default_transaction();
  cardano_utxo_list_t*              inputs      = NULL;

  EXPECT_EQ(cardano_deferred_redeemer_list_new(&list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_new(&inputs), CARDANO_SUCCESS);

  // Act & Assert
  EXPECT_EQ(cardano_deferred_redeemer_list_resolve(list, NULL, inputs), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_deferred_redeemer_list_resolve(list, transaction, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_deferred_redeemer_list_unref(&list);
  cardano_transaction_unref(&transaction);
  cardano_utxo_list_unref(&inputs);
}

TEST(cardano_deferred_redeemer_list_resolve, invokesCallbackAndReplacesRedeemerPayload)
{
  // Arrange
  cardano_deferred_redeemer_list_t* list        = NULL;
  cardano_redeemer_t*               redeemer    = new_default_redeemer();
  cardano_transaction_t*            transaction = new_default_transaction();
  cardano_utxo_list_t*              inputs      = NULL;

  EXPECT_EQ(cardano_deferred_redeemer_list_new(&list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_new(&inputs), CARDANO_SUCCESS);

  resolve_probe_t probe = { transaction, inputs, 42, 0 };

  EXPECT_EQ(cardano_deferred_redeemer_list_add(list, redeemer, probe_callback, &probe), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_deferred_redeemer_list_resolve(list, transaction, inputs);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(probe.call_count, 1);

  cardano_plutus_data_t* expected = NULL;
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(42, &expected), CARDANO_SUCCESS);

  cardano_plutus_data_t* payload = cardano_redeemer_get_data(redeemer);

  EXPECT_TRUE(cardano_plutus_data_equals(payload, expected));

  // The callback runs on every balancing iteration; a second resolve must invoke it again.
  EXPECT_EQ(cardano_deferred_redeemer_list_resolve(list, transaction, inputs), CARDANO_SUCCESS);
  EXPECT_EQ(probe.call_count, 2);

  // Cleanup
  cardano_plutus_data_unref(&expected);
  cardano_plutus_data_unref(&payload);
  cardano_deferred_redeemer_list_unref(&list);
  cardano_redeemer_unref(&redeemer);
  cardano_transaction_unref(&transaction);
  cardano_utxo_list_unref(&inputs);
}

TEST(cardano_deferred_redeemer_list_resolve, resolvesEveryRegisteredEntry)
{
  // Arrange
  cardano_deferred_redeemer_list_t* list        = NULL;
  cardano_redeemer_t*               redeemer_a  = new_default_redeemer();
  cardano_redeemer_t*               redeemer_b  = new_default_redeemer();
  cardano_transaction_t*            transaction = new_default_transaction();
  cardano_utxo_list_t*              inputs      = NULL;

  EXPECT_EQ(cardano_deferred_redeemer_list_new(&list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_new(&inputs), CARDANO_SUCCESS);

  resolve_probe_t probe_a = { transaction, inputs, 7, 0 };
  resolve_probe_t probe_b = { transaction, inputs, 11, 0 };

  EXPECT_EQ(cardano_deferred_redeemer_list_add(list, redeemer_a, probe_callback, &probe_a), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_deferred_redeemer_list_add(list, redeemer_b, probe_callback, &probe_b), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_deferred_redeemer_list_resolve(list, transaction, inputs);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(probe_a.call_count, 1);
  EXPECT_EQ(probe_b.call_count, 1);

  cardano_plutus_data_t* expected_a = NULL;
  cardano_plutus_data_t* expected_b = NULL;

  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(7, &expected_a), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(11, &expected_b), CARDANO_SUCCESS);

  cardano_plutus_data_t* payload_a = cardano_redeemer_get_data(redeemer_a);
  cardano_plutus_data_t* payload_b = cardano_redeemer_get_data(redeemer_b);

  EXPECT_TRUE(cardano_plutus_data_equals(payload_a, expected_a));
  EXPECT_TRUE(cardano_plutus_data_equals(payload_b, expected_b));

  // Cleanup
  cardano_plutus_data_unref(&expected_a);
  cardano_plutus_data_unref(&expected_b);
  cardano_plutus_data_unref(&payload_a);
  cardano_plutus_data_unref(&payload_b);
  cardano_deferred_redeemer_list_unref(&list);
  cardano_redeemer_unref(&redeemer_a);
  cardano_redeemer_unref(&redeemer_b);
  cardano_transaction_unref(&transaction);
  cardano_utxo_list_unref(&inputs);
}

TEST(cardano_deferred_redeemer_list_resolve, propagatesCallbackErrorAndStops)
{
  // Arrange
  cardano_deferred_redeemer_list_t* list        = NULL;
  cardano_redeemer_t*               redeemer    = new_default_redeemer();
  cardano_transaction_t*            transaction = new_default_transaction();
  cardano_utxo_list_t*              inputs      = NULL;

  EXPECT_EQ(cardano_deferred_redeemer_list_new(&list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_new(&inputs), CARDANO_SUCCESS);

  resolve_probe_t failing_probe = { transaction, inputs, 0, 0 };
  resolve_probe_t second_probe  = { transaction, inputs, 42, 0 };

  EXPECT_EQ(cardano_deferred_redeemer_list_add(list, redeemer, failing_callback, &failing_probe), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_deferred_redeemer_list_add(list, redeemer, probe_callback, &second_probe), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_deferred_redeemer_list_resolve(list, transaction, inputs);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(failing_probe.call_count, 1);
  EXPECT_EQ(second_probe.call_count, 0);

  // Cleanup
  cardano_deferred_redeemer_list_unref(&list);
  cardano_redeemer_unref(&redeemer);
  cardano_transaction_unref(&transaction);
  cardano_utxo_list_unref(&inputs);
}

TEST(cardano_deferred_redeemer_list_resolve, returnsErrorIfCallbackYieldsNullPayload)
{
  // Arrange
  cardano_deferred_redeemer_list_t* list        = NULL;
  cardano_redeemer_t*               redeemer    = new_default_redeemer();
  cardano_transaction_t*            transaction = new_default_transaction();
  cardano_utxo_list_t*              inputs      = NULL;

  EXPECT_EQ(cardano_deferred_redeemer_list_new(&list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_new(&inputs), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_deferred_redeemer_list_add(list, redeemer, null_payload_callback, NULL), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_deferred_redeemer_list_resolve(list, transaction, inputs);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_deferred_redeemer_list_unref(&list);
  cardano_redeemer_unref(&redeemer);
  cardano_transaction_unref(&transaction);
  cardano_utxo_list_unref(&inputs);
}

TEST(cardano_deferred_redeemer_list_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_deferred_redeemer_list_t* list = NULL;

  EXPECT_EQ(cardano_deferred_redeemer_list_new(&list), CARDANO_SUCCESS);

  // Act
  cardano_deferred_redeemer_list_ref(list);

  // Assert
  EXPECT_EQ(cardano_deferred_redeemer_list_refcount(list), 2U);

  // Cleanup
  cardano_deferred_redeemer_list_unref(&list);
  cardano_deferred_redeemer_list_unref(&list);
}

TEST(cardano_deferred_redeemer_list_ref, doesntCrashIfGivenNull)
{
  // Act
  cardano_deferred_redeemer_list_ref(NULL);
}

TEST(cardano_deferred_redeemer_list_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_deferred_redeemer_list_t* list = NULL;

  EXPECT_EQ(cardano_deferred_redeemer_list_new(&list), CARDANO_SUCCESS);

  // Act
  cardano_deferred_redeemer_list_ref(list);
  size_t ref_count = cardano_deferred_redeemer_list_refcount(list);

  cardano_deferred_redeemer_list_unref(&list);
  size_t updated_ref_count = cardano_deferred_redeemer_list_refcount(list);

  // Assert
  EXPECT_EQ(ref_count, 2U);
  EXPECT_EQ(updated_ref_count, 1U);

  // Cleanup
  cardano_deferred_redeemer_list_unref(&list);
}

TEST(cardano_deferred_redeemer_list_unref, freesTheObjectAndItsEntriesOnLastReference)
{
  // Arrange
  cardano_deferred_redeemer_list_t* list     = NULL;
  cardano_redeemer_t*               redeemer = new_default_redeemer();

  EXPECT_EQ(cardano_deferred_redeemer_list_new(&list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_deferred_redeemer_list_add(list, redeemer, probe_callback, NULL), CARDANO_SUCCESS);

  // The list must hold its own reference on the redeemer.
  EXPECT_EQ(cardano_redeemer_refcount(redeemer), 2U);

  // Act
  cardano_deferred_redeemer_list_unref(&list);

  // Assert
  EXPECT_EQ(list, nullptr);
  EXPECT_EQ(cardano_redeemer_refcount(redeemer), 1U);

  // Cleanup
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_deferred_redeemer_list_unref, doesntCrashIfGivenNull)
{
  // Act
  cardano_deferred_redeemer_list_unref(NULL);

  cardano_deferred_redeemer_list_t* list = NULL;

  cardano_deferred_redeemer_list_unref(&list);
}

TEST(cardano_deferred_redeemer_list_refcount, returnsZeroIfGivenNull)
{
  // Act & Assert
  EXPECT_EQ(cardano_deferred_redeemer_list_refcount(NULL), 0U);
}
