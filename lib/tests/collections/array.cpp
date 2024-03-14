/**
 * \file array.cpp
 *
 * \author luisd.bianchi
 * \date   Mar 04, 2024
 *
 * \section LICENSE
 *
 * Copyright 2023 Biglup Labs
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

#include "../src/collections/array.h"

#include <cardano/allocators.h>

#include "../allocators_helpers.h"

#include <gmock/gmock.h>

/* STRUCTS *******************************************************************/

typedef struct ref_counted_string_t
{
    cardano_object_t base;
    char*            string;
} ref_counted_string_t;

typedef struct
{
    const char* search_string;
} ref_counted_string_find_context_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * Reference counted string deallocator.
 *
 * \param object The object to be deallocated.
 */
static void
cardano_ref_counted_string_deallocate(void* object)
{
  assert(object != NULL);

  ref_counted_string_t* ref_str = (ref_counted_string_t*)object;

  if (ref_str->string != NULL)
  {
    _cardano_free(ref_str->string);
    ref_str->string = NULL;
  }

  _cardano_free(ref_str);
}

/**
 * \brief Allocates a new ref-counted string object.
 * \param string The string to be stored in the object.
 * \return A pointer to the newly allocated object.
 */
static ref_counted_string_t*
ref_counted_string_new(const char* string)
{
  ref_counted_string_t* ref_counted_string = (ref_counted_string_t*)_cardano_malloc(sizeof(ref_counted_string_t));

  ref_counted_string->base.ref_count     = 1;
  ref_counted_string->base.last_error[0] = '\0';
  ref_counted_string->base.deallocator   = cardano_ref_counted_string_deallocate;
  ref_counted_string->string             = (char*)_cardano_malloc(strlen(string) + 1);

  strcpy(ref_counted_string->string, string);

  return ref_counted_string;
}

/**
 * \brief Finds a ref-counted string in an array.
 * \param a The object to be compared.
 * \param context The context to be used in the comparison.
 *
 * \return True if the object is the one being searched for, false otherwise.
 */
static bool
find_predicate(const cardano_object_t* a, const void* context)
{
  const ref_counted_string_find_context_t* findContext = (const ref_counted_string_find_context_t*)context;
  const ref_counted_string_t*              str1        = (const ref_counted_string_t*)a;
  return strcmp(str1->string, findContext->search_string) == 0;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_array_new, returnsNullWhenMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_array_t* array = cardano_array_new(1);

  // Assert
  EXPECT_EQ(array, nullptr);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_array_new, returnsNullIfEventualMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_array_t* array = cardano_array_new(1);

  // Assert
  EXPECT_EQ(array, nullptr);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_array_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_array_t* array = nullptr;

  // Act
  array = cardano_array_new(1);
  cardano_array_ref(array);

  // Assert
  EXPECT_THAT(array, testing::Not((cardano_array_t*)nullptr));
  EXPECT_EQ(cardano_array_refcount(array), 2);

  // Cleanup
  cardano_array_unref(&array);
  cardano_array_unref(&array);
}

TEST(cardano_array_ref, doesntCrashIfArrayIsNull)
{
  // Arrange
  cardano_array_t* array = nullptr;

  // Act
  cardano_array_ref(nullptr);

  // Assert
  EXPECT_EQ(array, nullptr);
}

TEST(cardano_array_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_array_t* array = nullptr;

  // Act
  cardano_array_unref(&array);
}

TEST(cardano_array_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_array_unref((cardano_array_t**)nullptr);
}

TEST(cardano_array_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_array_t* array = cardano_array_new(1);

  // Act
  cardano_array_ref(array);
  size_t ref_count = cardano_array_refcount(array);

  cardano_array_unref(&array);
  size_t updated_ref_count = cardano_array_refcount(array);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_array_unref(&array);
}

TEST(cardano_array_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_array_t* array = cardano_array_new(1);

  // Act
  cardano_array_ref(array);
  size_t ref_count = cardano_array_refcount(array);

  cardano_array_unref(&array);
  size_t updated_ref_count = cardano_array_refcount(array);

  cardano_array_unref(&array);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(array, (cardano_array_t*)nullptr);
}

TEST(cardano_array_move, returnsNullIfArrayIsNull)
{
  // Arrange
  cardano_array_t* array = nullptr;

  // Act
  cardano_array_t* moved = cardano_array_move(array);

  // Assert
  EXPECT_EQ(moved, nullptr);
}

TEST(cardano_array_move, decreasesTheReferenceCountWithoutDeletingTheObject)
{
  // Arrange
  cardano_array_t* array = cardano_array_new(1);

  // Act
  EXPECT_THAT(cardano_array_move(array), testing::Not((cardano_array_t*)nullptr));
  size_t ref_count = cardano_array_refcount(array);

  // Assert
  EXPECT_EQ(ref_count, 0);
  EXPECT_THAT(array, testing::Not((cardano_array_t*)nullptr));

  // Cleanup
  cardano_array_unref(&array);
}

TEST(cardano_array_get_size, returnsZeroIfArrayIsNull)
{
  // Arrange
  cardano_array_t* array = nullptr;

  // Act
  size_t size = cardano_array_get_size(array);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_array_get_capacity, returnsZeroIfArrayIsNull)
{
  // Arrange
  cardano_array_t* array = nullptr;

  // Act
  size_t size = cardano_array_get_capacity(array);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_array_refcount, returnsZeroIfArrayIsNull)
{
  // Arrange
  cardano_array_t* array = nullptr;

  // Act
  size_t size = cardano_array_refcount(array);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_array_add, doesntSegFaultIfArrayIsNull)
{
  // Arrange
  cardano_array_t*  array  = nullptr;
  cardano_object_t* object = nullptr;

  // Act
  size_t new_size = cardano_array_add(array, object);

  // Assert
  EXPECT_EQ(new_size, 0);
}

TEST(cardano_array_add, returnsNullIfItemIsNull)
{
  // Arrange
  cardano_array_t*  array  = cardano_array_new(100);
  cardano_object_t* object = nullptr;

  // Act
  size_t new_size = cardano_array_add(array, object);

  // Assert
  EXPECT_EQ(new_size, 0);

  // Cleanup
  cardano_array_unref(&array);
}

TEST(cardano_array_add, addTheItemToTheArray)
{
  // Arrange
  cardano_array_t*      array   = cardano_array_new(100);
  ref_counted_string_t* ref_str = ref_counted_string_new("Hello, World!");

  EXPECT_EQ(cardano_array_get_size(array), 0);
  EXPECT_EQ(cardano_object_refcount(&ref_str->base), 1);

  // Act
  size_t new_size = cardano_array_add(array, &ref_str->base);

  // Assert
  EXPECT_EQ(new_size, 1);
  EXPECT_EQ(cardano_array_get_size(array), 1);
  EXPECT_EQ(cardano_object_refcount(&ref_str->base), 2);

  // Cleanup
  cardano_array_unref(&array);
  cardano_object_unref((cardano_object_t**)&ref_str);
}

TEST(cardano_array_add, canAddMoreThanOneItem)
{
  // Arrange
  cardano_array_t*      array    = cardano_array_new(1);
  ref_counted_string_t* ref_str1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_str2 = ref_counted_string_new("Hello, World! - 2");
  ref_counted_string_t* ref_str3 = ref_counted_string_new("Hello, World! - 3");

  EXPECT_EQ(cardano_array_get_size(array), 0);
  EXPECT_EQ(cardano_array_get_capacity(array), 1);
  EXPECT_EQ(cardano_object_refcount(&ref_str1->base), 1);
  EXPECT_EQ(cardano_object_refcount(&ref_str2->base), 1);
  EXPECT_EQ(cardano_object_refcount(&ref_str3->base), 1);

  // Act & Assert
  size_t new_size = cardano_array_add(array, &ref_str1->base);
  EXPECT_EQ(new_size, 1);
  EXPECT_EQ(cardano_array_get_capacity(array), 2);

  new_size = cardano_array_add(array, &ref_str2->base);
  EXPECT_EQ(new_size, 2);
  EXPECT_EQ(cardano_array_get_capacity(array), 3);

  new_size = cardano_array_add(array, &ref_str3->base);
  EXPECT_EQ(new_size, 3);
  EXPECT_EQ(cardano_array_get_capacity(array), 5);

  EXPECT_EQ(cardano_array_get_size(array), 3);
  EXPECT_EQ(cardano_object_refcount(&ref_str1->base), 2);
  EXPECT_EQ(cardano_object_refcount(&ref_str2->base), 2);
  EXPECT_EQ(cardano_object_refcount(&ref_str3->base), 2);

  cardano_array_unref(&array);
  EXPECT_EQ(cardano_object_refcount(&ref_str1->base), 1);
  EXPECT_EQ(cardano_object_refcount(&ref_str2->base), 1);
  EXPECT_EQ(cardano_object_refcount(&ref_str3->base), 1);

  cardano_object_unref((cardano_object_t**)&ref_str1);
  cardano_object_unref((cardano_object_t**)&ref_str2);
  cardano_object_unref((cardano_object_t**)&ref_str3);
}

TEST(cardano_array_get, canGetItems)
{
  // Arrange
  cardano_array_t*      array    = cardano_array_new(3);
  ref_counted_string_t* ref_str1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_str2 = ref_counted_string_new("Hello, World! - 2");
  ref_counted_string_t* ref_str3 = ref_counted_string_new("Hello, World! - 3");

  EXPECT_EQ(cardano_array_get_size(array), 0);
  EXPECT_EQ(cardano_array_get_capacity(array), 3);
  EXPECT_EQ(cardano_object_refcount(&ref_str1->base), 1);
  EXPECT_EQ(cardano_object_refcount(&ref_str2->base), 1);
  EXPECT_EQ(cardano_object_refcount(&ref_str3->base), 1);

  size_t new_size = cardano_array_add(array, &ref_str1->base);
  EXPECT_EQ(new_size, 1);
  EXPECT_EQ(cardano_array_get_capacity(array), 3);

  new_size = cardano_array_add(array, &ref_str2->base);
  EXPECT_EQ(new_size, 2);
  EXPECT_EQ(cardano_array_get_capacity(array), 3);

  new_size = cardano_array_add(array, &ref_str3->base);
  EXPECT_EQ(new_size, 3);
  EXPECT_EQ(cardano_array_get_capacity(array), 5);

  EXPECT_EQ(cardano_array_get_size(array), 3);
  EXPECT_EQ(cardano_object_refcount(&ref_str1->base), 2);
  EXPECT_EQ(cardano_object_refcount(&ref_str2->base), 2);
  EXPECT_EQ(cardano_object_refcount(&ref_str3->base), 2);

  // Act
  cardano_object_t* item1 = cardano_array_get(array, 0);
  cardano_object_t* item2 = cardano_array_get(array, 1);
  cardano_object_t* item3 = cardano_array_get(array, 2);

  EXPECT_EQ(cardano_object_refcount(item1), 3);
  EXPECT_EQ(cardano_object_refcount(item2), 3);
  EXPECT_EQ(cardano_object_refcount(item3), 3);

  EXPECT_STREQ(((ref_counted_string_t*)item1)->string, "Hello, World! - 1");
  EXPECT_STREQ(((ref_counted_string_t*)item2)->string, "Hello, World! - 2");
  EXPECT_STREQ(((ref_counted_string_t*)item3)->string, "Hello, World! - 3");

  cardano_object_unref((cardano_object_t**)&item1);
  cardano_object_unref((cardano_object_t**)&item2);
  cardano_object_unref((cardano_object_t**)&item3);

  EXPECT_EQ(cardano_object_refcount(&ref_str1->base), 2);
  EXPECT_EQ(cardano_object_refcount(&ref_str2->base), 2);
  EXPECT_EQ(cardano_object_refcount(&ref_str3->base), 2);

  cardano_array_unref(&array);
  EXPECT_EQ(cardano_object_refcount(&ref_str1->base), 1);
  EXPECT_EQ(cardano_object_refcount(&ref_str2->base), 1);
  EXPECT_EQ(cardano_object_refcount(&ref_str3->base), 1);

  cardano_object_unref((cardano_object_t**)&ref_str1);
  cardano_object_unref((cardano_object_t**)&ref_str2);
  cardano_object_unref((cardano_object_t**)&ref_str3);
}

TEST(cardano_array_get, returnsNullWhenGivenNullArray)
{
  // Arrange
  cardano_array_t* array = nullptr;

  // Act
  cardano_object_t* item = cardano_array_get(array, 0);

  // Assert
  EXPECT_EQ(item, nullptr);
}

TEST(cardano_array_get, returnsNullWhenGivenOutOfBoundsIndex)
{
  // Arrange
  cardano_array_t*      array    = cardano_array_new(3);
  ref_counted_string_t* ref_str1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_str2 = ref_counted_string_new("Hello, World! - 2");
  ref_counted_string_t* ref_str3 = ref_counted_string_new("Hello, World! - 3");

  EXPECT_EQ(cardano_array_get_size(array), 0);
  EXPECT_EQ(cardano_array_get_capacity(array), 3);
  EXPECT_EQ(cardano_object_refcount(&ref_str1->base), 1);
  EXPECT_EQ(cardano_object_refcount(&ref_str2->base), 1);
  EXPECT_EQ(cardano_object_refcount(&ref_str3->base), 1);

  size_t new_size = cardano_array_add(array, &ref_str1->base);
  EXPECT_EQ(new_size, 1);
  EXPECT_EQ(cardano_array_get_capacity(array), 3);

  new_size = cardano_array_add(array, &ref_str2->base);
  EXPECT_EQ(new_size, 2);
  EXPECT_EQ(cardano_array_get_capacity(array), 3);

  new_size = cardano_array_add(array, &ref_str3->base);
  EXPECT_EQ(new_size, 3);
  EXPECT_EQ(cardano_array_get_capacity(array), 5);

  EXPECT_EQ(cardano_array_get_size(array), 3);
  EXPECT_EQ(cardano_object_refcount(&ref_str1->base), 2);
  EXPECT_EQ(cardano_object_refcount(&ref_str2->base), 2);
  EXPECT_EQ(cardano_object_refcount(&ref_str3->base), 2);

  // Act
  cardano_object_t* item = cardano_array_get(array, 100);

  // Assert
  EXPECT_EQ(item, nullptr);

  cardano_array_unref(&array);
  cardano_object_unref((cardano_object_t**)&ref_str1);
  cardano_object_unref((cardano_object_t**)&ref_str2);
  cardano_object_unref((cardano_object_t**)&ref_str3);
}

TEST(cardano_array_concat, returnsNullWhenFirstArgumentIsNull)
{
  // Arrange
  cardano_array_t* array1 = nullptr;
  cardano_array_t* array2 = cardano_array_new(1);

  // Act
  cardano_array_t* result = cardano_array_concat(array1, array2);

  // Assert
  EXPECT_EQ(result, nullptr);

  // Cleanup
  cardano_array_unref(&array2);
}

TEST(cardano_array_concat, returnsNullWhenSecondArgumentIsNull)
{
  // Arrange
  cardano_array_t* array1 = cardano_array_new(1);
  cardano_array_t* array2 = nullptr;

  // Act
  cardano_array_t* result = cardano_array_concat(array1, array2);

  // Assert
  EXPECT_EQ(result, nullptr);

  // Cleanup
  cardano_array_unref(&array1);
}

TEST(cardano_array_concat, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();

  cardano_array_t*      array1   = cardano_array_new(1);
  cardano_array_t*      array2   = cardano_array_new(1);
  ref_counted_string_t* ref_str1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_str2 = ref_counted_string_new("Hello, World! - 2");

  size_t new_size = cardano_array_add(array1, &ref_str1->base);
  EXPECT_EQ(new_size, 1);

  new_size = cardano_array_add(array2, &ref_str2->base);
  EXPECT_EQ(new_size, 1);

  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_array_t* result = cardano_array_concat(array1, array2);

  // Assert
  EXPECT_EQ(result, nullptr);

  // Cleanup
  cardano_array_unref(&array1);
  cardano_array_unref(&array2);
  cardano_object_unref((cardano_object_t**)&ref_str1);
  cardano_object_unref((cardano_object_t**)&ref_str2);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_array_concat, returnsNullIfEventualMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();

  cardano_array_t*      array1   = cardano_array_new(1);
  cardano_array_t*      array2   = cardano_array_new(1);
  ref_counted_string_t* ref_str1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_str2 = ref_counted_string_new("Hello, World! - 2");

  size_t new_size = cardano_array_add(array1, &ref_str1->base);
  EXPECT_EQ(new_size, 1);

  new_size = cardano_array_add(array2, &ref_str2->base);
  EXPECT_EQ(new_size, 1);

  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_array_t* result = cardano_array_concat(array1, array2);

  // Assert
  EXPECT_EQ(result, nullptr);

  // Cleanup
  cardano_array_unref(&array1);
  cardano_array_unref(&array2);
  cardano_object_unref((cardano_object_t**)&ref_str1);
  cardano_object_unref((cardano_object_t**)&ref_str2);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_array_concat, canConcatenateTwoArrays)
{
  // Arrange
  cardano_array_t*      array1   = cardano_array_new(1);
  cardano_array_t*      array2   = cardano_array_new(1);
  ref_counted_string_t* ref_str1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_str2 = ref_counted_string_new("Hello, World! - 2");

  size_t new_size = cardano_array_add(array1, &ref_str1->base);
  EXPECT_EQ(new_size, 1);

  new_size = cardano_array_add(array2, &ref_str2->base);
  EXPECT_EQ(new_size, 1);

  // Act
  cardano_array_t* result = cardano_array_concat(array1, array2);

  // Assert
  EXPECT_EQ(cardano_array_get_size(result), 2);
  EXPECT_EQ(cardano_array_get_capacity(result), 2);

  cardano_object_t* item1 = cardano_array_get(result, 0);
  cardano_object_t* item2 = cardano_array_get(result, 1);

  EXPECT_STREQ(((ref_counted_string_t*)item1)->string, "Hello, World! - 1");
  EXPECT_STREQ(((ref_counted_string_t*)item2)->string, "Hello, World! - 2");

  cardano_object_unref((cardano_object_t**)&item1);
  cardano_object_unref((cardano_object_t**)&item2);

  cardano_array_unref(&array1);
  cardano_array_unref(&array2);
  cardano_array_unref(&result);
  cardano_object_unref((cardano_object_t**)&ref_str1);
  cardano_object_unref((cardano_object_t**)&ref_str2);
}

TEST(cardano_array_concat, canConcatenateTwoArraysOfDifferentSizes)
{
  // Arrange
  cardano_array_t*      array1   = cardano_array_new(1);
  cardano_array_t*      array2   = cardano_array_new(1);
  ref_counted_string_t* ref_str1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_str2 = ref_counted_string_new("Hello, World! - 2");
  ref_counted_string_t* ref_str3 = ref_counted_string_new("Hello, World! - 3");

  size_t new_size = cardano_array_add(array1, &ref_str1->base);
  EXPECT_EQ(new_size, 1);

  new_size = cardano_array_add(array2, &ref_str2->base);
  EXPECT_EQ(new_size, 1);

  new_size = cardano_array_add(array2, &ref_str3->base);
  EXPECT_EQ(new_size, 2);

  // Act
  cardano_array_t* result = cardano_array_concat(array1, array2);

  // Assert
  EXPECT_EQ(cardano_array_get_size(result), 3);
  EXPECT_EQ(cardano_array_get_capacity(result), 3);

  cardano_object_t* item1 = cardano_array_get(result, 0);
  cardano_object_t* item2 = cardano_array_get(result, 1);
  cardano_object_t* item3 = cardano_array_get(result, 2);

  EXPECT_STREQ(((ref_counted_string_t*)item1)->string, "Hello, World! - 1");
  EXPECT_STREQ(((ref_counted_string_t*)item2)->string, "Hello, World! - 2");
  EXPECT_STREQ(((ref_counted_string_t*)item3)->string, "Hello, World! - 3");

  cardano_object_unref((cardano_object_t**)&item1);
  cardano_object_unref((cardano_object_t**)&item2);
  cardano_object_unref((cardano_object_t**)&item3);

  cardano_array_unref(&array1);
  cardano_array_unref(&array2);
  cardano_array_unref(&result);
  cardano_object_unref((cardano_object_t**)&ref_str1);
  cardano_object_unref((cardano_object_t**)&ref_str2);
  cardano_object_unref((cardano_object_t**)&ref_str3);
}

TEST(cardano_array_slice, returnsNullIfArrayIsNull)
{
  // Arrange
  cardano_array_t* array = nullptr;

  // Act
  cardano_array_t* result = cardano_array_slice(array, 0, 1);

  // Assert
  EXPECT_EQ(result, nullptr);
}

TEST(cardano_array_slice, returnsNullIfStartIndexIsGreaterThanEndIndex)
{
  // Arrange
  cardano_array_t*      array   = cardano_array_new(1);
  ref_counted_string_t* ref_str = ref_counted_string_new("Hello, World!");

  size_t new_size = cardano_array_add(array, &ref_str->base);
  EXPECT_EQ(new_size, 1);

  // Act
  cardano_array_t* result = cardano_array_slice(array, 1, 0);

  // Assert
  EXPECT_EQ(result, nullptr);

  cardano_array_unref(&array);
  cardano_object_unref((cardano_object_t**)&ref_str);
}

TEST(cardano_array_slice, returnsNullIfStartIndexIsGreaterThanArraySize)
{
  // Arrange
  cardano_array_t*      array   = cardano_array_new(1);
  ref_counted_string_t* ref_str = ref_counted_string_new("Hello, World!");

  size_t new_size = cardano_array_add(array, &ref_str->base);
  EXPECT_EQ(new_size, 1);

  // Act
  cardano_array_t* result = cardano_array_slice(array, 2, 3);

  // Assert
  EXPECT_EQ(result, nullptr);

  cardano_array_unref(&array);
  cardano_object_unref((cardano_object_t**)&ref_str);
}

TEST(cardano_array_slice, returnsNullIfEndIndexIsGreaterThanArraySize)
{
  // Arrange
  cardano_array_t*      array   = cardano_array_new(1);
  ref_counted_string_t* ref_str = ref_counted_string_new("Hello, World!");

  size_t new_size = cardano_array_add(array, &ref_str->base);
  EXPECT_EQ(new_size, 1);

  // Act
  cardano_array_t* result = cardano_array_slice(array, 0, 2);

  // Assert
  EXPECT_EQ(result, nullptr);

  cardano_array_unref(&array);
  cardano_object_unref((cardano_object_t**)&ref_str);
}

TEST(cardano_array_slice, returnsNullIfStartIndexIsEqualToEndIndex)
{
  // Arrange
  cardano_array_t*      array   = cardano_array_new(1);
  ref_counted_string_t* ref_str = ref_counted_string_new("Hello, World!");

  size_t new_size = cardano_array_add(array, &ref_str->base);
  EXPECT_EQ(new_size, 1);

  // Act
  cardano_array_t* result = cardano_array_slice(array, 0, 0);

  // Assert
  EXPECT_EQ(result, nullptr);

  cardano_array_unref(&array);
  cardano_object_unref((cardano_object_t**)&ref_str);
}

TEST(cardano_array_slice, returnsNullIfStartIndexIsEqualToArraySize)
{
  // Arrange
  cardano_array_t*      array   = cardano_array_new(1);
  ref_counted_string_t* ref_str = ref_counted_string_new("Hello, World!");

  size_t new_size = cardano_array_add(array, &ref_str->base);
  EXPECT_EQ(new_size, 1);

  // Act
  cardano_array_t* result = cardano_array_slice(array, 1, 1);

  // Assert
  EXPECT_EQ(result, nullptr);

  cardano_array_unref(&array);
  cardano_object_unref((cardano_object_t**)&ref_str);
}

TEST(cardano_array_slice, returnsNullIfArrayIsEmpty)
{
  // Arrange
  cardano_array_t* array = cardano_array_new(1);

  // Act
  cardano_array_t* result = cardano_array_slice(array, 0, 1);

  // Assert
  EXPECT_EQ(result, nullptr);

  cardano_array_unref(&array);
}

TEST(cardano_array_slice, canSliceAnArrayOfSeveralItems)
{
  // Arrange
  cardano_array_t*      array    = cardano_array_new(1);
  ref_counted_string_t* ref_str1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_str2 = ref_counted_string_new("Hello, World! - 2");
  ref_counted_string_t* ref_str3 = ref_counted_string_new("Hello, World! - 3");

  size_t new_size = cardano_array_add(array, &ref_str1->base);
  EXPECT_EQ(new_size, 1);

  new_size = cardano_array_add(array, &ref_str2->base);
  EXPECT_EQ(new_size, 2);

  new_size = cardano_array_add(array, &ref_str3->base);
  EXPECT_EQ(new_size, 3);

  // Act
  cardano_array_t* result = cardano_array_slice(array, 1, 2);

  // Assert
  EXPECT_EQ(cardano_array_get_size(result), 1);
  EXPECT_EQ(cardano_array_get_capacity(result), 1);

  cardano_object_t* item = cardano_array_get(result, 0);
  EXPECT_STREQ(((ref_counted_string_t*)item)->string, "Hello, World! - 2");

  cardano_object_unref((cardano_object_t**)&item);

  cardano_array_unref(&array);
  cardano_array_unref(&result);
  cardano_object_unref((cardano_object_t**)&ref_str1);
  cardano_object_unref((cardano_object_t**)&ref_str2);
  cardano_object_unref((cardano_object_t**)&ref_str3);
}

TEST(cardano_array_slice, canSliceAnArrayOfOneItem)
{
  // Arrange
  cardano_array_t*      array    = cardano_array_new(1);
  ref_counted_string_t* ref_str1 = ref_counted_string_new("Hello, World! - 1");

  size_t new_size = cardano_array_add(array, &ref_str1->base);
  EXPECT_EQ(new_size, 1);

  // Act
  cardano_array_t* result = cardano_array_slice(array, 0, 1);

  // Assert
  EXPECT_EQ(cardano_array_get_size(result), 1);
  EXPECT_EQ(cardano_array_get_capacity(result), 1);

  cardano_object_t* item = cardano_array_get(result, 0);
  EXPECT_STREQ(((ref_counted_string_t*)item)->string, "Hello, World! - 1");

  cardano_object_unref((cardano_object_t**)&item);

  cardano_array_unref(&array);
  cardano_array_unref(&result);
  cardano_object_unref((cardano_object_t**)&ref_str1);
}

TEST(cardano_array_slice, canSliceAnArrayOfManyItemsFromAnArrayOfManyItems)
{
  // Arrange
  cardano_array_t*      array    = cardano_array_new(1);
  ref_counted_string_t* ref_str1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_str2 = ref_counted_string_new("Hello, World! - 2");
  ref_counted_string_t* ref_str3 = ref_counted_string_new("Hello, World! - 3");
  ref_counted_string_t* ref_str4 = ref_counted_string_new("Hello, World! - 4");
  ref_counted_string_t* ref_str5 = ref_counted_string_new("Hello, World! - 5");
  ref_counted_string_t* ref_str6 = ref_counted_string_new("Hello, World! - 6");

  size_t new_size = cardano_array_add(array, &ref_str1->base);
  EXPECT_EQ(new_size, 1);

  new_size = cardano_array_add(array, &ref_str2->base);
  EXPECT_EQ(new_size, 2);

  new_size = cardano_array_add(array, &ref_str3->base);
  EXPECT_EQ(new_size, 3);

  new_size = cardano_array_add(array, &ref_str4->base);
  EXPECT_EQ(new_size, 4);

  new_size = cardano_array_add(array, &ref_str5->base);
  EXPECT_EQ(new_size, 5);

  new_size = cardano_array_add(array, &ref_str6->base);
  EXPECT_EQ(new_size, 6);

  // Act
  cardano_array_t* result = cardano_array_slice(array, 2, 5);

  // Assert
  EXPECT_EQ(cardano_array_get_size(result), 3);
  EXPECT_EQ(cardano_array_get_capacity(result), 3);

  cardano_object_t* item1 = cardano_array_get(result, 0);
  cardano_object_t* item2 = cardano_array_get(result, 1);
  cardano_object_t* item3 = cardano_array_get(result, 2);

  EXPECT_STREQ(((ref_counted_string_t*)item1)->string, "Hello, World! - 3");
  EXPECT_STREQ(((ref_counted_string_t*)item2)->string, "Hello, World! - 4");
  EXPECT_STREQ(((ref_counted_string_t*)item3)->string, "Hello, World! - 5");

  cardano_object_unref((cardano_object_t**)&item1);
  cardano_object_unref((cardano_object_t**)&item2);
  cardano_object_unref((cardano_object_t**)&item3);

  cardano_array_unref(&array);
  cardano_array_unref(&result);
  cardano_object_unref((cardano_object_t**)&ref_str1);
  cardano_object_unref((cardano_object_t**)&ref_str2);
  cardano_object_unref((cardano_object_t**)&ref_str3);
  cardano_object_unref((cardano_object_t**)&ref_str4);
  cardano_object_unref((cardano_object_t**)&ref_str5);
  cardano_object_unref((cardano_object_t**)&ref_str6);
}

TEST(cardano_array_slice, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();

  cardano_array_t*      array    = cardano_array_new(1);
  ref_counted_string_t* ref_str1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_str2 = ref_counted_string_new("Hello, World! - 2");
  ref_counted_string_t* ref_str3 = ref_counted_string_new("Hello, World! - 3");
  ref_counted_string_t* ref_str4 = ref_counted_string_new("Hello, World! - 4");
  ref_counted_string_t* ref_str5 = ref_counted_string_new("Hello, World! - 5");
  ref_counted_string_t* ref_str6 = ref_counted_string_new("Hello, World! - 6");

  size_t new_size = cardano_array_add(array, &ref_str1->base);
  EXPECT_EQ(new_size, 1);

  new_size = cardano_array_add(array, &ref_str2->base);
  EXPECT_EQ(new_size, 2);

  new_size = cardano_array_add(array, &ref_str3->base);
  EXPECT_EQ(new_size, 3);

  new_size = cardano_array_add(array, &ref_str4->base);
  EXPECT_EQ(new_size, 4);

  new_size = cardano_array_add(array, &ref_str5->base);
  EXPECT_EQ(new_size, 5);

  new_size = cardano_array_add(array, &ref_str6->base);
  EXPECT_EQ(new_size, 6);

  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_array_t* result = cardano_array_slice(array, 2, 5);

  // Assert
  EXPECT_EQ(result, nullptr);

  // Cleanup
  cardano_array_unref(&array);
  cardano_object_unref((cardano_object_t**)&ref_str1);
  cardano_object_unref((cardano_object_t**)&ref_str2);
  cardano_object_unref((cardano_object_t**)&ref_str3);
  cardano_object_unref((cardano_object_t**)&ref_str4);
  cardano_object_unref((cardano_object_t**)&ref_str5);
  cardano_object_unref((cardano_object_t**)&ref_str6);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_array_slice, returnsNullIfEventualMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();

  cardano_array_t*      array    = cardano_array_new(1);
  ref_counted_string_t* ref_str1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_str2 = ref_counted_string_new("Hello, World! - 2");
  ref_counted_string_t* ref_str3 = ref_counted_string_new("Hello, World! - 3");
  ref_counted_string_t* ref_str4 = ref_counted_string_new("Hello, World! - 4");
  ref_counted_string_t* ref_str5 = ref_counted_string_new("Hello, World! - 5");
  ref_counted_string_t* ref_str6 = ref_counted_string_new("Hello, World! - 6");

  size_t new_size = cardano_array_add(array, &ref_str1->base);
  EXPECT_EQ(new_size, 1);

  new_size = cardano_array_add(array, &ref_str2->base);
  EXPECT_EQ(new_size, 2);

  new_size = cardano_array_add(array, &ref_str3->base);
  EXPECT_EQ(new_size, 3);

  new_size = cardano_array_add(array, &ref_str4->base);
  EXPECT_EQ(new_size, 4);

  new_size = cardano_array_add(array, &ref_str5->base);
  EXPECT_EQ(new_size, 5);

  new_size = cardano_array_add(array, &ref_str6->base);
  EXPECT_EQ(new_size, 6);

  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_array_t* result = cardano_array_slice(array, 2, 5);

  // Assert
  EXPECT_EQ(result, nullptr);

  // Cleanup
  cardano_array_unref(&array);
  cardano_object_unref((cardano_object_t**)&ref_str1);
  cardano_object_unref((cardano_object_t**)&ref_str2);
  cardano_object_unref((cardano_object_t**)&ref_str3);
  cardano_object_unref((cardano_object_t**)&ref_str4);
  cardano_object_unref((cardano_object_t**)&ref_str5);
  cardano_object_unref((cardano_object_t**)&ref_str6);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_array_get_last_error, returnsNullTerminatedMessage)
{
  // Arrange
  cardano_array_t* array   = cardano_array_new(1);
  const char*      message = "This is a test message";

  // Act
  cardano_array_set_last_error(array, message);
  const char* last_error = cardano_array_get_last_error(array);

  // Assert
  EXPECT_STREQ(last_error, message);

  // Cleanup
  cardano_array_unref(&array);
}

TEST(cardano_array_get_last_error, returnsObjectIsNullWhenCalledForNullObject)
{
  // Arrange
  cardano_array_t* array = nullptr;

  // Act
  const char* last_error = cardano_array_get_last_error(array);

  // Assert
  EXPECT_STREQ(last_error, "Object is NULL.");
}

TEST(cardano_array_clear, doesNothingWhenArrayIsNull)
{
  // Arrange
  cardano_array_t* array = nullptr;

  // Act
  cardano_array_clear(array);

  // Assert
  EXPECT_EQ(cardano_array_get_size(array), 0);
}

TEST(cardano_array_clear, clearsTheArray)
{
  // Arrange
  cardano_array_t*      array    = cardano_array_new(1);
  ref_counted_string_t* ref_str1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_str2 = ref_counted_string_new("Hello, World! - 2");
  ref_counted_string_t* ref_str3 = ref_counted_string_new("Hello, World! - 3");

  size_t new_size = cardano_array_add(array, &ref_str1->base);
  EXPECT_EQ(new_size, 1);

  new_size = cardano_array_add(array, &ref_str2->base);
  EXPECT_EQ(new_size, 2);

  new_size = cardano_array_add(array, &ref_str3->base);
  EXPECT_EQ(new_size, 3);

  // Act
  cardano_array_clear(array);

  // Assert
  EXPECT_EQ(cardano_array_get_size(array), 0);

  cardano_array_unref(&array);
  cardano_object_unref((cardano_object_t**)&ref_str1);
  cardano_object_unref((cardano_object_t**)&ref_str2);
  cardano_object_unref((cardano_object_t**)&ref_str3);
}

TEST(cardano_array_sort, doesNothingWhenArrayIsNull)
{
  // Arrange
  cardano_array_t* array = nullptr;

  // Act
  cardano_array_sort(array, nullptr);

  // Assert
  EXPECT_EQ(cardano_array_get_size(array), 0);
}

TEST(cardano_array_sort, doesNothingWhenComparatorIsNull)
{
  // Arrange
  cardano_array_t* array = cardano_array_new(1);

  // Act
  cardano_array_sort(array, nullptr);

  // Assert
  EXPECT_EQ(cardano_array_get_size(array), 0);

  // Cleanup
  cardano_array_unref(&array);
}

TEST(cardano_array_sort, sortsTheArray)
{
  // Arrange
  cardano_array_t*      array    = cardano_array_new(1);
  ref_counted_string_t* ref_str1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_str2 = ref_counted_string_new("Hello, World! - 2");
  ref_counted_string_t* ref_str3 = ref_counted_string_new("Hello, World! - 3");

  size_t new_size = cardano_array_add(array, &ref_str3->base);
  EXPECT_EQ(new_size, 1);

  new_size = cardano_array_add(array, &ref_str2->base);
  EXPECT_EQ(new_size, 2);

  new_size = cardano_array_add(array, &ref_str1->base);
  EXPECT_EQ(new_size, 3);

  // Act
  cardano_array_sort(array, [](const cardano_object_t* a, const cardano_object_t* b) -> int
                     {
    ref_counted_string_t* str1 = (ref_counted_string_t*)a;
    ref_counted_string_t* str2 = (ref_counted_string_t*)b;

    return strcmp(str1->string, str2->string); });

  // Assert
  cardano_object_t* item1 = cardano_array_get(array, 0);
  cardano_object_t* item2 = cardano_array_get(array, 1);
  cardano_object_t* item3 = cardano_array_get(array, 2);

  EXPECT_STREQ(((ref_counted_string_t*)item1)->string, "Hello, World! - 1");
  EXPECT_STREQ(((ref_counted_string_t*)item2)->string, "Hello, World! - 2");
  EXPECT_STREQ(((ref_counted_string_t*)item3)->string, "Hello, World! - 3");

  cardano_object_unref((cardano_object_t**)&item1);
  cardano_object_unref((cardano_object_t**)&item2);
  cardano_object_unref((cardano_object_t**)&item3);

  cardano_array_unref(&array);
  cardano_object_unref((cardano_object_t**)&ref_str1);
  cardano_object_unref((cardano_object_t**)&ref_str2);
  cardano_object_unref((cardano_object_t**)&ref_str3);
}

TEST(cardano_array_find, returnsNullWhenArrayIsNull)
{
  // Arrange
  cardano_array_t* array = nullptr;

  // Act
  cardano_object_t* result = cardano_array_find(array, nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, nullptr);
}

TEST(cardano_array_find, returnsNullWhenComparatorIsNull)
{
  // Arrange
  cardano_array_t* array = cardano_array_new(1);

  // Act
  cardano_object_t* result = cardano_array_find(array, nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, nullptr);

  // Cleanup
  cardano_array_unref(&array);
}

TEST(cardano_array_find, returnsNullWhenArrayIsEmpty)
{
  // Arrange
  cardano_array_t* array = cardano_array_new(1);

  // Act
  cardano_object_t* result = cardano_array_find(
    array, [](const cardano_object_t*, const void*) -> bool
    { return false; },
    nullptr);

  // Assert
  EXPECT_EQ(result, nullptr);

  // Cleanup
  cardano_array_unref(&array);
}

TEST(cardano_array_find, returnsNullWhenItemIsNotFound)
{
  // Arrange
  cardano_array_t*      array    = cardano_array_new(1);
  ref_counted_string_t* ref_str1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_str2 = ref_counted_string_new("Hello, World! - 2");
  ref_counted_string_t* ref_str3 = ref_counted_string_new("Hello, World! - 3");

  size_t new_size = cardano_array_add(array, &ref_str1->base);
  EXPECT_EQ(new_size, 1);

  new_size = cardano_array_add(array, &ref_str2->base);
  EXPECT_EQ(new_size, 2);

  new_size = cardano_array_add(array, &ref_str3->base);
  EXPECT_EQ(new_size, 3);

  // Act
  cardano_object_t* result = cardano_array_find(
    array, [](const cardano_object_t* a, const void*) -> bool
    { return false; },
    nullptr);

  // Assert
  EXPECT_EQ(result, nullptr);

  // Cleanup
  cardano_array_unref(&array);
  cardano_object_unref((cardano_object_t**)&ref_str1);
  cardano_object_unref((cardano_object_t**)&ref_str2);
  cardano_object_unref((cardano_object_t**)&ref_str3);
}

TEST(cardano_array_find, returnsTheItemWhenItemIsFound)
{
  // Arrange
  cardano_array_t*      array    = cardano_array_new(1);
  ref_counted_string_t* ref_str1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_str2 = ref_counted_string_new("Hello, World! - 2");
  ref_counted_string_t* ref_str3 = ref_counted_string_new("Hello, World! - 3");

  size_t new_size = cardano_array_add(array, &ref_str1->base);
  EXPECT_EQ(new_size, 1);

  new_size = cardano_array_add(array, &ref_str2->base);
  EXPECT_EQ(new_size, 2);

  new_size = cardano_array_add(array, &ref_str3->base);
  EXPECT_EQ(new_size, 3);

  ref_counted_string_find_context_t context = { .search_string = "Hello, World! - 2" };

  // Act
  cardano_object_t* result = cardano_array_find(array, find_predicate, &context);

  // Assert
  EXPECT_EQ(result, &ref_str2->base);

  // Cleanup
  cardano_array_unref(&array);
  cardano_object_unref((cardano_object_t**)&ref_str1);
  cardano_object_unref((cardano_object_t**)&ref_str2);
  cardano_object_unref((cardano_object_t**)&ref_str3);
  cardano_object_unref((cardano_object_t**)&result);
}

TEST(cardano_array_filter, returnsNullWhenArrayIsNull)
{
  // Arrange
  cardano_array_t* array = nullptr;

  // Act
  cardano_array_t* result = cardano_array_filter(array, nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, nullptr);
}

TEST(cardano_array_filter, returnsNullWhenPredicateIsNull)
{
  // Arrange
  cardano_array_t* array = cardano_array_new(1);

  // Act
  cardano_array_t* result = cardano_array_filter(array, nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, nullptr);

  // Cleanup
  cardano_array_unref(&array);
}

TEST(cardano_array_filter, returnsNullWhenArrayIsEmpty)
{
  // Arrange
  cardano_array_t* array = cardano_array_new(1);

  // Act
  cardano_array_t* result = cardano_array_filter(
    array, [](const cardano_object_t* a, const void*) -> bool
    { return true; },
    nullptr);

  // Assert
  EXPECT_EQ(cardano_array_get_size(result), 0);

  // Cleanup
  cardano_array_unref(&array);
  cardano_array_unref(&result);
}

TEST(cardano_array_filter, returnsNullWhenNoItemsMatchPredicate)
{
  // Arrange
  cardano_array_t*      array    = cardano_array_new(1);
  ref_counted_string_t* ref_str1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_str2 = ref_counted_string_new("Hello, World! - 2");
  ref_counted_string_t* ref_str3 = ref_counted_string_new("Hello, World! - 3");

  size_t new_size = cardano_array_add(array, &ref_str1->base);
  EXPECT_EQ(new_size, 1);

  new_size = cardano_array_add(array, &ref_str2->base);
  EXPECT_EQ(new_size, 2);

  new_size = cardano_array_add(array, &ref_str3->base);
  EXPECT_EQ(new_size, 3);

  // Act
  cardano_array_t* result = cardano_array_filter(
    array, [](const cardano_object_t* a, const void*) -> bool
    { return false; },
    nullptr);

  // Assert
  EXPECT_EQ(cardano_array_get_size(result), 0);

  // Cleanup
  cardano_array_unref(&array);
  cardano_array_unref(&result);
  cardano_object_unref((cardano_object_t**)&ref_str1);
  cardano_object_unref((cardano_object_t**)&ref_str2);
  cardano_object_unref((cardano_object_t**)&ref_str3);
}

TEST(cardano_array_filter, returnsTheItemsThatMatchPredicate)
{
  // Arrange
  cardano_array_t*      array    = cardano_array_new(1);
  ref_counted_string_t* ref_str1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_str2 = ref_counted_string_new("Hello, World! - 2");
  ref_counted_string_t* ref_str3 = ref_counted_string_new("Hello, World! - 3");

  size_t new_size = cardano_array_add(array, &ref_str1->base);
  EXPECT_EQ(new_size, 1);

  new_size = cardano_array_add(array, &ref_str2->base);
  EXPECT_EQ(new_size, 2);

  new_size = cardano_array_add(array, &ref_str3->base);
  EXPECT_EQ(new_size, 3);

  ref_counted_string_find_context_t context = { .search_string = "Hello, World! - 1" };

  // Act
  cardano_array_t* result = cardano_array_filter(array, find_predicate, &context);

  // Assert
  EXPECT_EQ(cardano_array_get_size(result), 1);
  EXPECT_EQ(cardano_array_get_capacity(result), 3);

  cardano_object_t* item = cardano_array_get(result, 0);

  EXPECT_STREQ(((ref_counted_string_t*)item)->string, "Hello, World! - 1");

  cardano_object_unref((cardano_object_t**)&item);

  // Cleanup
  cardano_array_unref(&array);
  cardano_array_unref(&result);
  cardano_object_unref((cardano_object_t**)&ref_str1);
  cardano_object_unref((cardano_object_t**)&ref_str2);
  cardano_object_unref((cardano_object_t**)&ref_str3);
}

TEST(cardano_array_filter, returnsNullWhenMemoryAllocationFails)
{
  // Arrange
  reset_allocators_run_count();

  cardano_array_t*      array    = cardano_array_new(1);
  ref_counted_string_t* ref_str1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_str2 = ref_counted_string_new("Hello, World! - 2");
  ref_counted_string_t* ref_str3 = ref_counted_string_new("Hello, World! - 3");

  size_t new_size = cardano_array_add(array, &ref_str1->base);
  EXPECT_EQ(new_size, 1);

  new_size = cardano_array_add(array, &ref_str2->base);
  EXPECT_EQ(new_size, 2);

  new_size = cardano_array_add(array, &ref_str3->base);
  EXPECT_EQ(new_size, 3);

  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_array_t* result = cardano_array_filter(
    array, [](const cardano_object_t* a, const void*) -> bool
    { return true; },
    nullptr);

  // Assert
  EXPECT_EQ(result, nullptr);

  // Cleanup
  cardano_array_unref(&array);
  cardano_object_unref((cardano_object_t**)&ref_str1);
  cardano_object_unref((cardano_object_t**)&ref_str2);
  cardano_object_unref((cardano_object_t**)&ref_str3);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_array_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_array_t* array   = nullptr;
  const char*      message = "This is a test message";

  // Act
  cardano_array_set_last_error(array, message);

  // Assert
  EXPECT_STREQ(cardano_array_get_last_error(array), "Object is NULL.");
}

TEST(cardano_array_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_array_t* array   = cardano_array_new(1);
  const char*      message = nullptr;

  // Act
  cardano_array_set_last_error(array, message);

  // Assert
  EXPECT_STREQ(cardano_array_get_last_error(array), "");

  // Cleanup
  cardano_array_unref(&array);
}