/**
 * \file utxo_list.cpp
 *
 * \author angel.castillo
 * \date   May 21, 2024
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

#include <cardano/common/utxo.h>
#include <cardano/common/utxo_list.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>
#include <gmock/gmock.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                  = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c410a";
static const char* CBOR_DIFFERENT_INPUT  = "82825820bb217abaca60fc0ca78c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c410a";
static const char* CBOR_DIFFERENT_OUTPUT = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c420a";
static const char* CBOR_DIFFERENT_VAL1   = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c20a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c420a";
static const char* CBOR_DIFFERENT_VAL2   = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c420a";
static const char* CBOR_DIFFERENT_VAL3   = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c22a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c420a";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the utxo.
 * @return A new instance of the utxo.
 */
static cardano_utxo_t*
new_default_utxo(const char* utxo)
{
  cardano_utxo_t*        utxo_obj = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(utxo, strlen(utxo));

  cardano_error_t result = cardano_utxo_from_cbor(reader, &utxo_obj);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return utxo_obj;
};

/**
 * Creates a new default instance of the governance action list.
 * @return  A new instance of the utxo list.
 */
static cardano_utxo_list_t*
new_default_utxo_list()
{
  cardano_utxo_list_t* list = NULL;

  cardano_error_t error = cardano_utxo_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_t* gai1 = new_default_utxo(CBOR_DIFFERENT_INPUT);
  cardano_utxo_t* gai2 = new_default_utxo(CBOR_DIFFERENT_OUTPUT);

  EXPECT_EQ(cardano_utxo_list_add(list, gai1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(list, gai2), CARDANO_SUCCESS);

  cardano_utxo_unref(&gai1);
  cardano_utxo_unref(&gai2);

  return list;
};

/**
 * Creates a new default instance of the governance action list.
 * @return  A new instance of the utxo list.
 */
static cardano_utxo_list_t*
new_utxo_list_diff_vals()
{
  cardano_utxo_list_t* list = NULL;

  cardano_error_t error = cardano_utxo_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_t* gai1 = new_default_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t* gai2 = new_default_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_t* gai3 = new_default_utxo(CBOR_DIFFERENT_VAL3);

  EXPECT_EQ(cardano_utxo_list_add(list, gai2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(list, gai1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(list, gai3), CARDANO_SUCCESS);

  cardano_utxo_unref(&gai1);
  cardano_utxo_unref(&gai2);
  cardano_utxo_unref(&gai3);

  return list;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_utxo_list_new, createsANewInstanceOfRewardAddressList)
{
  cardano_utxo_list_t* list = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&list), CARDANO_SUCCESS);

  ASSERT_NE(list, nullptr);
  ASSERT_EQ(cardano_utxo_list_get_length(list), 0);

  cardano_utxo_list_unref(&list);
}

TEST(cardano_utxo_list_new, returnsErrorIfListIsNull)
{
  EXPECT_EQ(cardano_utxo_list_new(nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_utxo_list_new, returnErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_utxo_list_t* list = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&list), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  ASSERT_EQ(list, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_utxo_list_new, returnErrorIfMemoryAllocationFails2)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_utxo_list_t* list = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&list), CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  ASSERT_EQ(list, nullptr);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_utxo_list_get_length, returnsZeroIfListIsNull)
{
  // Act
  size_t result = cardano_utxo_list_get_length(nullptr);

  // Assert
  ASSERT_EQ(result, 0);
}

TEST(cardano_utxo_list_get, returnsNullIfListIsNull)
{
  // Act
  EXPECT_EQ(cardano_utxo_list_get(nullptr, 0, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_utxo_list_get, returnsNullIfElementIsNull)
{
  // Act
  EXPECT_EQ(cardano_utxo_list_get((cardano_utxo_list_t*)"", 0, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_utxo_list_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_utxo_list_t* list = NULL;

  cardano_error_t error = cardano_utxo_list_new(&list);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_utxo_t* utxo = NULL;
  error                = cardano_utxo_list_get(list, 0, &utxo);

  // Assert
  ASSERT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_utxo_list_unref(&list);
}

TEST(cardano_utxo_list_get, returnsTheElementAtGivenIndex)
{
  // Arrange
  cardano_utxo_list_t* list = new_default_utxo_list();

  // Act
  cardano_utxo_t* utxo  = NULL;
  cardano_error_t error = cardano_utxo_list_get(list, 0, &utxo);

  // Assert
  ASSERT_EQ(error, CARDANO_SUCCESS);

  ASSERT_NE(utxo, nullptr);

  // Cleanup
  cardano_utxo_list_unref(&list);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_utxo_list_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_utxo_list_t* list = new_default_utxo_list();

  // Act
  cardano_utxo_list_ref(list);

  // Assert
  EXPECT_THAT(list, testing::Not((cardano_utxo_list_t*)nullptr));
  EXPECT_EQ(cardano_utxo_list_refcount(list), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_utxo_list_unref(&list);
  cardano_utxo_list_unref(&list);
}

TEST(cardano_utxo_list_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_utxo_list_ref(nullptr);
}

TEST(cardano_utxo_list_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_utxo_list_t* utxo_list = nullptr;

  // Act
  cardano_utxo_list_unref(&utxo_list);
}

TEST(cardano_utxo_list_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_utxo_list_unref((cardano_utxo_list_t**)nullptr);
}

TEST(cardano_utxo_list_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_utxo_list_t* list = new_default_utxo_list();

  // Act
  cardano_utxo_list_ref(list);
  size_t ref_count = cardano_utxo_list_refcount(list);

  cardano_utxo_list_unref(&list);
  size_t updated_ref_count = cardano_utxo_list_refcount(list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_utxo_list_unref(&list);
}

TEST(cardano_utxo_list_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_utxo_list_t* utxo_list = new_default_utxo_list();

  // Act
  cardano_utxo_list_ref(utxo_list);
  size_t ref_count = cardano_utxo_list_refcount(utxo_list);

  cardano_utxo_list_unref(&utxo_list);
  size_t updated_ref_count = cardano_utxo_list_refcount(utxo_list);

  cardano_utxo_list_unref(&utxo_list);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(utxo_list, (cardano_utxo_list_t*)nullptr);

  // Cleanup
  cardano_utxo_list_unref(&utxo_list);
}

TEST(cardano_utxo_list_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_utxo_list_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_utxo_list_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_utxo_list_t* utxo_list = nullptr;
  const char*          message   = "This is a test message";

  // Act
  cardano_utxo_list_set_last_error(utxo_list, message);

  // Assert
  EXPECT_STREQ(cardano_utxo_list_get_last_error(utxo_list), "Object is NULL.");
}

TEST(cardano_utxo_list_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_utxo_list_t* utxo_list = new_default_utxo_list();

  const char* message = nullptr;

  // Act
  cardano_utxo_list_set_last_error(utxo_list, message);

  // Assert
  EXPECT_STREQ(cardano_utxo_list_get_last_error(utxo_list), "");

  // Cleanup
  cardano_utxo_list_unref(&utxo_list);
}

TEST(cardano_utxo_list_add, returnsErrorIfListIsNull)
{
  // Arrange
  cardano_utxo_t* id = nullptr;

  // Act
  cardano_error_t result = cardano_utxo_list_add(nullptr, id);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_utxo_list_add, returnsErrorIfScriptIsNull)
{
  // Act
  cardano_error_t result = cardano_utxo_list_add((cardano_utxo_list_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_utxo_list_clear, doesNothingIfListIsNull)
{
  // Act
  cardano_utxo_list_clear(nullptr);
}

TEST(cardano_utxo_list_clear, removesAllElementsFromTheList)
{
  // Arrange
  cardano_utxo_list_t* list = new_default_utxo_list();

  // Act
  cardano_utxo_list_clear(list);

  // Assert
  ASSERT_EQ(cardano_utxo_list_get_length(list), 0);

  // Cleanup
  cardano_utxo_list_unref(&list);
}

TEST(cardano_utxo_list_sort, doesNothingIfListIsNull)
{
  // Act
  cardano_utxo_list_sort(
    nullptr, [](cardano_utxo_t* a, cardano_utxo_t* b, void* context)
    { return 0; },
    nullptr);

  // Assert
  // Nothing to assert.
}

TEST(cardano_utxo_list_sort, doesNothingIfComparatorIsNull)
{
  // Arrange
  cardano_utxo_list_t* list = new_default_utxo_list();

  // Act
  cardano_utxo_list_sort(list, nullptr, nullptr);

  // Assert
  // Nothing to assert.

  // Cleanup
  cardano_utxo_list_unref(&list);
}

TEST(cardano_utxo_list_sort, sortsTheListUsingTheComparator)
{
  // Arrange
  cardano_utxo_list_t* list = new_utxo_list_diff_vals();

  // Act
  cardano_utxo_list_sort(
    list, [](cardano_utxo_t* a, cardano_utxo_t* b, void*) -> int32_t
    {
    cardano_transaction_output_t* output_a = cardano_utxo_get_output(a);
    cardano_transaction_output_t* output_b = cardano_utxo_get_output(b);
    cardano_value_t* value_a = cardano_transaction_output_get_value(output_a);
    cardano_value_t* value_b = cardano_transaction_output_get_value(output_b);
    uint64_t coin_a = cardano_value_get_coin(value_a);
    uint64_t coin_b = cardano_value_get_coin(value_b);

    cardano_transaction_output_unref(&output_a);
    cardano_transaction_output_unref(&output_b);
    cardano_value_unref(&value_a);
    cardano_value_unref(&value_b);

    return (int32_t)((int64_t)coin_a - (int64_t)coin_b); },
    nullptr);

  // Assert
  ASSERT_EQ(cardano_utxo_list_get_length(list), 3);

  cardano_utxo_t* utxo = NULL;
  EXPECT_EQ(cardano_utxo_list_get(list, 0, &utxo), CARDANO_SUCCESS);

  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
  cardano_value_t*              value  = cardano_transaction_output_get_value(output);

  ASSERT_EQ(cardano_value_get_coin(value), 4027026464);

  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&output);
  cardano_value_unref(&value);

  EXPECT_EQ(cardano_utxo_list_get(list, 1, &utxo), CARDANO_SUCCESS);

  output = cardano_utxo_get_output(utxo);
  value  = cardano_transaction_output_get_value(output);

  ASSERT_EQ(cardano_value_get_coin(value), 4027026465);

  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&output);
  cardano_value_unref(&value);

  EXPECT_EQ(cardano_utxo_list_get(list, 2, &utxo), CARDANO_SUCCESS);

  output = cardano_utxo_get_output(utxo);
  value  = cardano_transaction_output_get_value(output);

  ASSERT_EQ(cardano_value_get_coin(value), 4027026466);

  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&output);
  cardano_value_unref(&value);

  // Cleanup
  cardano_utxo_list_unref(&list);
}

TEST(cardano_utxo_list_find, returnsErrorIfListIsNull)
{
  // Arrange

  cardano_utxo_t* utxo = cardano_utxo_list_find(
    nullptr, [](cardano_utxo_t* a, const void* context) -> bool
    { return false; },
    nullptr);

  // Assert
  ASSERT_EQ(utxo, nullptr);
}

TEST(cardano_utxo_list_find, returnsNullIfComparatorIsNull)
{
  // Arrange
  cardano_utxo_list_t* list = new_default_utxo_list();

  // Act
  cardano_utxo_t* utxo = cardano_utxo_list_find(list, nullptr, nullptr);

  // Assert
  ASSERT_EQ(utxo, nullptr);

  // Cleanup
  cardano_utxo_list_unref(&list);
}

TEST(cardano_utxo_list_find, returnsNullIfNoElementMatches)
{
  // Arrange
  cardano_utxo_list_t* list = new_default_utxo_list();

  // Act
  cardano_utxo_t* utxo = cardano_utxo_list_find(
    list, [](cardano_utxo_t* a, const void* context) -> bool
    { return false; },
    nullptr);

  // Assert
  ASSERT_EQ(utxo, nullptr);

  // Cleanup
  cardano_utxo_list_unref(&list);
}

TEST(cardano_utxo_list_find, returnsTheFirstElementThatMatches)
{
  // Arrange
  cardano_utxo_list_t* list = new_utxo_list_diff_vals();

  // Act
  cardano_utxo_t* utxo = cardano_utxo_list_find(
    list, [](cardano_utxo_t* a, const void* context) -> bool
    {
    cardano_transaction_output_t* output = cardano_utxo_get_output(a);
    cardano_value_t* value = cardano_transaction_output_get_value(output);
    uint64_t coin = cardano_value_get_coin(value);

    cardano_transaction_output_unref(&output);
    cardano_value_unref(&value);

    return coin == 4027026465; },
    nullptr);

  // Assert
  ASSERT_NE(utxo, nullptr);

  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
  cardano_value_t*              value  = cardano_transaction_output_get_value(output);

  ASSERT_EQ(cardano_value_get_coin(value), 4027026465);

  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&output);
  cardano_value_unref(&value);

  // Cleanup
  cardano_utxo_list_unref(&list);
}

TEST(cardano_utxo_list_filter, returnsNullIfListIsNull)
{
  // Arrange
  cardano_utxo_list_t* result = cardano_utxo_list_filter(
    nullptr, [](cardano_utxo_t* a, const void* context) -> bool
    { return false; },
    nullptr);

  // Assert
  ASSERT_EQ(result, nullptr);
}

TEST(cardano_utxo_list_filter, returnsNullIfPredicateIsNull)
{
  // Arrange
  cardano_utxo_list_t* list = new_default_utxo_list();

  // Act
  cardano_utxo_list_t* result = cardano_utxo_list_filter(list, nullptr, nullptr);

  // Assert
  ASSERT_EQ(result, nullptr);

  // Cleanup
  cardano_utxo_list_unref(&list);
}

TEST(cardano_utxo_list_filter, returnsNullIfNoElementMatches)
{
  // Arrange
  cardano_utxo_list_t* list = new_default_utxo_list();

  // Act
  cardano_utxo_list_t* result = cardano_utxo_list_filter(
    list, [](cardano_utxo_t* a, const void* context) -> bool
    { return false; },
    nullptr);

  // Assert
  ASSERT_EQ(cardano_utxo_list_get_length(result), 0);

  // Cleanup
  cardano_utxo_list_unref(&list);
  cardano_utxo_list_unref(&result);
}

TEST(cardano_utxo_list_filter, returnsTheElementsThatMatch)
{
  // Arrange
  cardano_utxo_list_t* list = new_utxo_list_diff_vals();

  // Act
  cardano_utxo_list_t* result = cardano_utxo_list_filter(
    list, [](cardano_utxo_t* a, const void* context) -> bool
    {
    cardano_transaction_output_t* output = cardano_utxo_get_output(a);
    cardano_value_t* value = cardano_transaction_output_get_value(output);
    uint64_t coin = cardano_value_get_coin(value);

    cardano_transaction_output_unref(&output);
    cardano_value_unref(&value);

    return coin == 4027026465; },
    nullptr);

  // Assert
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(cardano_utxo_list_get_length(result), 1);

  cardano_utxo_t* utxo = NULL;
  EXPECT_EQ(cardano_utxo_list_get(result, 0, &utxo), CARDANO_SUCCESS);

  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
  cardano_value_t*              value  = cardano_transaction_output_get_value(output);

  ASSERT_EQ(cardano_value_get_coin(value), 4027026465);

  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&output);
  cardano_value_unref(&value);

  // Cleanup
  cardano_utxo_list_unref(&list);
  cardano_utxo_list_unref(&result);
}

TEST(cardano_utxo_list_concat, returnsNullIfEitherListIsNull)
{
  // Act
  cardano_utxo_list_t* result = cardano_utxo_list_concat(nullptr, nullptr);

  // Assert
  ASSERT_EQ(result, nullptr);
}

TEST(cardano_utxo_list_concat, returnsTheConcatenatedList)
{
  // Arrange
  cardano_utxo_list_t* list = new_default_utxo_list();

  // Act
  cardano_utxo_list_t* result = cardano_utxo_list_concat(list, list);

  // Assert
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(cardano_utxo_list_get_length(result), 4);

  // Cleanup
  cardano_utxo_list_unref(&list);
  cardano_utxo_list_unref(&result);
}

TEST(cardano_utxo_list_erase, returnsErrorIfListIsNull)
{
  // Act
  cardano_utxo_list_t* result = cardano_utxo_list_erase(nullptr, 0, 0);

  // Assert
  ASSERT_EQ(result, nullptr);
}

TEST(cardano_utxo_list_slice, returnsNullIfListIsNull)
{
  // Act
  cardano_utxo_list_t* result = cardano_utxo_list_slice(nullptr, 0, 0);

  // Assert
  ASSERT_EQ(result, nullptr);
}

TEST(cardano_utxo_list_slice, returnsTheSliceOfTheList)
{
  // Arrange
  cardano_utxo_list_t* list = new_default_utxo_list();

  // Act
  cardano_utxo_list_t* result = cardano_utxo_list_slice(list, 0, 1);

  // Assert
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(cardano_utxo_list_get_length(result), 1);

  // Cleanup
  cardano_utxo_list_unref(&list);
  cardano_utxo_list_unref(&result);
}

TEST(cardano_utxo_list_erase, canEraseElements)
{
  // Arrange
  cardano_utxo_list_t* list = new_default_utxo_list();

  // Act
  cardano_utxo_list_t* result = cardano_utxo_list_erase(list, 0, 1);

  // Assert
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(cardano_utxo_list_get_length(list), 1);
  ASSERT_EQ(cardano_utxo_list_get_length(result), 1);

  // Cleanup
  cardano_utxo_list_unref(&list);
  cardano_utxo_list_unref(&result);
}

TEST(cardano_utxo_list_clone, returnsNullIfListIsNull)
{
  // Act
  cardano_utxo_list_t* result = cardano_utxo_list_clone(nullptr);

  // Assert
  ASSERT_EQ(result, nullptr);
}

TEST(cardano_utxo_list_clone, returnsACloneOfTheList)
{
  // Arrange
  cardano_utxo_list_t* list = new_default_utxo_list();

  // Act
  cardano_utxo_list_t* result = cardano_utxo_list_clone(list);

  // Assert
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(cardano_utxo_list_get_length(result), 2);

  // Cleanup
  cardano_utxo_list_unref(&list);
  cardano_utxo_list_unref(&result);
}