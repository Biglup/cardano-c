/**
 * \file set.cpp
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

#include "../src/collections/set.h"
#include "../allocators_helpers.h"
#include "../src/allocators.h"

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
 * \param[in] object The object to be deallocated.
 */
static void
cardano_ref_counted_string_deallocate(void* object)
{
  assert(object != nullptr);

  ref_counted_string_t* ref_str = (ref_counted_string_t*)object;

  if (ref_str->string != nullptr)
  {
    _cardano_free(ref_str->string);
    ref_str->string = nullptr;
  }

  _cardano_free(ref_str);
}

/**
 * \brief Allocates a new ref-counted string object.
 * \param[in] string The string to be stored in the object.
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
 * \brief Finds a ref-counted string in an set.
 * \param[in] a The object to be compared.
 * \param[in] context The context to be used in the comparison.
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

/**
 * \brief A function pointer type for hashing objects within a set.
 *
 * \param[in] object A pointer to the `cardano_object_t` object to be hashed.
 *
 * \return A 64-bit unsigned integer representing the hash value of the object.
 *
 * \note The implementer must ensure that the function is deterministic—calling the hash
 *       function with the same object should always return the same hash value.
 */
static uint64_t
hash(const cardano_object_t* object)
{
  const ref_counted_string_t* ref_str = (const ref_counted_string_t*)object;
  return (uint64_t)strlen(ref_str->string);
}

/**
 * \brief Function pointer type that compares two objects of the same type and returns a value
 * indicating whether one object is less than, equal to, or greater than the other.
 *
 * \param[in] lhs The left-hand side object to compare.
 * \param[in] rhs The right-hand side object to compare.
 *
 * \return A negative value if `lhs` is less than `rhs`, 0 if `lhs` is equal to `rhs`, or a positive
 * value if `lhs` is greater than `rhs`.
 */
static int
compare(const cardano_object_t* lhs, const cardano_object_t* rhs)
{
  const ref_counted_string_t* ref_str1 = (const ref_counted_string_t*)lhs;
  const ref_counted_string_t* ref_str2 = (const ref_counted_string_t*)rhs;
  return strcmp(ref_str1->string, ref_str2->string);
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_set_from_array, returnsNullIfArrayIsNull)
{
  // Arrange
  cardano_array_t* array = nullptr;

  // Act
  cardano_set_t* set = cardano_set_from_array(array, compare, hash);

  // Assert
  EXPECT_EQ(set, nullptr);
}

TEST(cardano_set_from_array, returnsNullIfCompareIsNull)
{
  // Arrange
  cardano_array_t* array = cardano_array_new(128);

  // Act
  cardano_set_t* set = cardano_set_from_array(array, nullptr, hash);

  // Assert
  EXPECT_EQ(set, nullptr);

  // Cleanup
  cardano_array_unref(&array);
}

TEST(cardano_set_from_array, returnsNullIfHashIsNull)
{
  // Arrange
  cardano_array_t* array = cardano_array_new(128);

  // Act
  cardano_set_t* set = cardano_set_from_array(array, compare, nullptr);

  // Assert
  EXPECT_EQ(set, nullptr);

  // Cleanup
  cardano_array_unref(&array);
}

TEST(cardano_set_from_array, addsElementsOnTheArrayToTheNewSet)
{
  // Arrange
  cardano_array_t* array = cardano_array_new(128);
  cardano_set_t*   set   = nullptr;

  ref_counted_string_t* ref_counted_string1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_counted_string2 = ref_counted_string_new("Hello, World! - 2");

  size_t new_size = cardano_array_add(array, (cardano_object_t*)ref_counted_string1);
  EXPECT_EQ(new_size, 1);
  new_size = cardano_array_add(array, (cardano_object_t*)ref_counted_string2);
  EXPECT_EQ(new_size, 2);

  // Act
  set = cardano_set_from_array(array, compare, hash);

  // Assert
  EXPECT_THAT(set, testing::Not((cardano_set_t*)nullptr));
  EXPECT_EQ(cardano_set_get_size(set), 2);

  // Cleanup
  cardano_set_unref(&set);
  cardano_array_unref(&array);
  cardano_object_unref((cardano_object_t**)&ref_counted_string1);
  cardano_object_unref((cardano_object_t**)&ref_counted_string2);
}

TEST(cardano_set_from_array, doesntAddTheSameElementFromTheArrayTwice)
{
  // Arrange
  cardano_array_t* array = cardano_array_new(128);
  cardano_set_t*   set   = nullptr;

  ref_counted_string_t* ref_counted_string1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_counted_string2 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_counted_string3 = ref_counted_string_new("Hello, World! - 2");

  size_t new_size = cardano_array_add(array, (cardano_object_t*)ref_counted_string1);
  EXPECT_EQ(new_size, 1);
  new_size = cardano_array_add(array, (cardano_object_t*)ref_counted_string2);
  EXPECT_EQ(new_size, 2);
  new_size = cardano_array_add(array, (cardano_object_t*)ref_counted_string3);
  EXPECT_EQ(new_size, 3);

  // Act
  set = cardano_set_from_array(array, compare, hash);

  // Assert
  EXPECT_THAT(set, testing::Not((cardano_set_t*)nullptr));
  EXPECT_EQ(cardano_set_get_size(set), 2);

  // Cleanup
  cardano_set_unref(&set);
  cardano_array_unref(&array);
  cardano_object_unref((cardano_object_t**)&ref_counted_string1);
  cardano_object_unref((cardano_object_t**)&ref_counted_string2);
  cardano_object_unref((cardano_object_t**)&ref_counted_string3);
}

TEST(cardano_set_from_array, returnsNullIfThereIsMemoryAllocFailure)
{
  // Arrange
  reset_allocators_run_count();
  cardano_array_t* array = cardano_array_new(128);
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_set_t* set = cardano_set_from_array(array, compare, hash);

  // Assert
  EXPECT_EQ(set, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_array_unref(&array);
}

TEST(cardano_set_from_array, returnsNullIfThereIsEventualMemoryAllocFailure)
{
  // Arrange
  reset_allocators_run_count();
  cardano_array_t*      array              = cardano_array_new(128);
  ref_counted_string_t* ref_counted_string = ref_counted_string_new("Hello, World! - 1");

  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  size_t new_size = cardano_array_add(array, (cardano_object_t*)ref_counted_string);
  EXPECT_EQ(new_size, 1);

  // Act
  cardano_set_t* set = cardano_set_from_array(array, compare, hash);

  // Assert
  EXPECT_EQ(set, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_array_unref(&array);
  cardano_object_unref((cardano_object_t**)&ref_counted_string);
}

TEST(cardano_set_new, returnsNullIfCompareIfThereIsMemoryAllocFailure)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_set_t* set = cardano_set_new(compare, hash);

  // Assert
  EXPECT_EQ(set, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_set_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_set_t* set = nullptr;

  // Act
  set = cardano_set_new(compare, hash);
  cardano_set_ref(set);

  // Assert
  EXPECT_THAT(set, testing::Not((cardano_set_t*)nullptr));
  EXPECT_EQ(cardano_set_refcount(set), 2);

  // Cleanup
  cardano_set_unref(&set);
  cardano_set_unref(&set);
}

TEST(cardano_set_ref, doesntCrashIfSetIsNull)
{
  // Arrange
  cardano_set_t* set = nullptr;

  // Act
  cardano_set_ref(nullptr);

  // Assert
  EXPECT_EQ(set, nullptr);
}

TEST(cardano_set_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_set_t* set = nullptr;

  // Act
  cardano_set_unref(&set);
}

TEST(cardano_set_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_set_unref((cardano_set_t**)nullptr);
}

TEST(cardano_set_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_set_t* set = cardano_set_new(compare, hash);

  // Act
  cardano_set_ref(set);
  size_t ref_count = cardano_set_refcount(set);

  cardano_set_unref(&set);
  size_t updated_ref_count = cardano_set_refcount(set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_set_unref(&set);
}

TEST(cardano_set_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_set_t* set = cardano_set_new(compare, hash);

  // Act
  cardano_set_ref(set);
  size_t ref_count = cardano_set_refcount(set);

  cardano_set_unref(&set);
  size_t updated_ref_count = cardano_set_refcount(set);

  cardano_set_unref(&set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(set, (cardano_set_t*)nullptr);
}

TEST(cardano_set_get_size, returnsZeroIfSetIsNull)
{
  // Arrange
  cardano_set_t* set = nullptr;

  // Act
  size_t size = cardano_set_get_size(set);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_set_refcount, returnsZeroIfSetIsNull)
{
  // Arrange
  cardano_set_t* set = nullptr;

  // Act
  size_t size = cardano_set_refcount(set);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_set_add, doesntSegFaultIfSetIsNull)
{
  // Arrange
  cardano_set_t*    set    = nullptr;
  cardano_object_t* object = nullptr;

  // Act
  size_t new_size = cardano_set_add(set, object);

  // Assert
  EXPECT_EQ(new_size, 0);
}

TEST(cardano_set_add, doesntSegFaultIfObjectIsNull)
{
  // Arrange
  cardano_set_t*    set    = cardano_set_new(compare, hash);
  cardano_object_t* object = nullptr;

  // Act
  size_t new_size = cardano_set_add(set, object);

  // Assert
  EXPECT_EQ(new_size, 0);

  // Cleanup
  cardano_set_unref(&set);
}

TEST(cardano_set_add, addsAnObjectToTheSet)
{
  // Arrange
  cardano_set_t* set = cardano_set_new(compare, hash);

  ref_counted_string_t* ref_counted_string = ref_counted_string_new("Hello, World!");
  cardano_object_t*     object             = (cardano_object_t*)ref_counted_string;

  // Act
  size_t new_size = cardano_set_add(set, object);

  // Assert
  EXPECT_EQ(new_size, 1);

  // Cleanup
  cardano_set_unref(&set);
  cardano_object_unref(&object);
}

TEST(cardano_set_add, doesntAddTheSameObjectTwice)
{
  // Arrange
  cardano_set_t* set = cardano_set_new(compare, hash);

  ref_counted_string_t* ref_counted_string = ref_counted_string_new("Hello, World!");
  cardano_object_t*     object             = (cardano_object_t*)ref_counted_string;

  // Act
  size_t new_size  = cardano_set_add(set, object);
  size_t same_size = cardano_set_add(set, object);

  // Assert
  EXPECT_EQ(new_size, 1);
  EXPECT_EQ(same_size, 1);

  // Cleanup
  cardano_set_unref(&set);
  cardano_object_unref(&object);
}

TEST(cardano_set_add, returnsZeroIfAllocFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_t* set = cardano_set_new(compare, hash);

  ref_counted_string_t* ref_counted_string = ref_counted_string_new("Hello, World!");
  cardano_object_t*     object             = (cardano_object_t*)ref_counted_string;

  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  size_t new_size = cardano_set_add(set, object);

  // Assert
  EXPECT_EQ(new_size, 0);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_set_unref(&set);
  cardano_object_unref(&object);
}

TEST(cardano_set_add, addsMultipleObjectsToTheSet)
{
  // Arrange
  cardano_set_t* set = cardano_set_new(compare, hash);

  ref_counted_string_t* ref_counted_string1 = ref_counted_string_new("Hello, World!");
  ref_counted_string_t* ref_counted_string2 = ref_counted_string_new("Hello, World!");
  cardano_object_t*     object1             = (cardano_object_t*)ref_counted_string1;
  cardano_object_t*     object2             = (cardano_object_t*)ref_counted_string2;

  // Act
  size_t new_size  = cardano_set_add(set, object1);
  size_t same_size = cardano_set_add(set, object2);

  // Assert
  EXPECT_EQ(new_size, 1);
  EXPECT_EQ(same_size, 1);

  // Cleanup
  cardano_set_unref(&set);
  cardano_object_unref(&object1);
  cardano_object_unref(&object2);
}

TEST(cardano_set_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_set_t* set     = nullptr;
  const char*    message = "This is a test message";

  // Act
  cardano_set_set_last_error(set, message);

  // Assert
  EXPECT_STREQ(cardano_set_get_last_error(set), "Object is NULL.");
}

TEST(cardano_set_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_set_t* set     = cardano_set_new(compare, hash);
  const char*    message = nullptr;

  // Act
  cardano_set_set_last_error(set, message);

  // Assert
  EXPECT_STREQ(cardano_set_get_last_error(set), "");

  // Cleanup
  cardano_set_unref(&set);
}

TEST(cardano_set_has, returnsFalseIfSetIsNull)
{
  // Arrange
  cardano_set_t*        set                = nullptr;
  ref_counted_string_t* ref_counted_string = ref_counted_string_new("Hello, World!");
  cardano_object_t*     object             = (cardano_object_t*)ref_counted_string;

  // Act
  bool has = cardano_set_has(set, object);

  // Assert
  EXPECT_FALSE(has);

  // Cleanup
  cardano_set_unref(&set);
  cardano_object_unref(&object);
}

TEST(cardano_set_has, returnsFalseIfObjectIsNull)
{
  // Arrange
  cardano_set_t*    set    = cardano_set_new(compare, hash);
  cardano_object_t* object = nullptr;

  // Act
  bool has = cardano_set_has(set, object);

  // Assert
  EXPECT_FALSE(has);

  // Cleanup
  cardano_set_unref(&set);
}

TEST(cardano_set_has, returnsFalseIfObjectIsNotInSet)
{
  // Arrange
  cardano_set_t*        set                = cardano_set_new(compare, hash);
  ref_counted_string_t* ref_counted_string = ref_counted_string_new("Hello, World!");
  cardano_object_t*     object             = (cardano_object_t*)ref_counted_string;

  // Act
  bool has = cardano_set_has(set, object);

  // Assert
  EXPECT_FALSE(has);

  // Cleanup
  cardano_set_unref(&set);
  cardano_object_unref(&object);
}

TEST(cardano_set_has, returnsTrueIfObjectIsInSet)
{
  // Arrange
  cardano_set_t*        set                = cardano_set_new(compare, hash);
  ref_counted_string_t* ref_counted_string = ref_counted_string_new("Hello, World!");
  cardano_object_t*     object             = (cardano_object_t*)ref_counted_string;

  size_t new_size = cardano_set_add(set, object);

  // Act
  bool has = cardano_set_has(set, object);

  // Assert
  EXPECT_TRUE(has);
  EXPECT_EQ(new_size, 1);

  // Cleanup
  cardano_set_unref(&set);
  cardano_object_unref(&object);
}

TEST(cardano_set_delete, doesNothingIfSetIsNull)
{
  // Arrange
  cardano_set_t*        set                = nullptr;
  ref_counted_string_t* ref_counted_string = ref_counted_string_new("Hello, World!");
  cardano_object_t*     object             = (cardano_object_t*)ref_counted_string;

  // Act
  size_t new_size = cardano_set_delete(set, object);

  // Assert
  EXPECT_EQ(new_size, 0);

  // Cleanup
  cardano_object_unref(&object);
}

TEST(cardano_set_delete, doesNothingIfObjectIsNull)
{
  // Arrange
  cardano_set_t*    set    = cardano_set_new(compare, hash);
  cardano_object_t* object = nullptr;

  // Act
  size_t new_size = cardano_set_delete(set, object);

  // Assert
  EXPECT_EQ(new_size, 0);

  // Cleanup
  cardano_set_unref(&set);
}

TEST(cardano_set_delete, doesNothingIfObjectIsNotInSet)
{
  // Arrange
  cardano_set_t*        set                = cardano_set_new(compare, hash);
  ref_counted_string_t* ref_counted_string = ref_counted_string_new("Hello, World!");
  cardano_object_t*     object             = (cardano_object_t*)ref_counted_string;

  // Act
  size_t new_size = cardano_set_delete(set, object);

  // Assert
  EXPECT_EQ(new_size, 0);

  // Cleanup
  cardano_set_unref(&set);
  cardano_object_unref(&object);
}

TEST(cardano_set_delete, removesAnObjectFromTheSet)
{
  // Arrange
  cardano_set_t*        set                = cardano_set_new(compare, hash);
  ref_counted_string_t* ref_counted_string = ref_counted_string_new("Hello, World!");
  cardano_object_t*     object             = (cardano_object_t*)ref_counted_string;

  size_t new_size = cardano_set_add(set, object);

  // Act
  bool deleted = cardano_set_delete(set, object);

  // Assert
  EXPECT_EQ(new_size, 1);
  EXPECT_EQ(deleted, true);

  // Cleanup
  cardano_set_unref(&set);
  cardano_object_unref(&object);
}

TEST(cardano_set_delete, removesOnOfManyFromTheSet)
{
  // Arrange
  cardano_set_t*        set                 = cardano_set_new(compare, hash);
  ref_counted_string_t* ref_counted_string1 = ref_counted_string_new("Hello, World! - 1");
  ref_counted_string_t* ref_counted_string2 = ref_counted_string_new("Hello, World! - 2");
  cardano_object_t*     object1             = (cardano_object_t*)ref_counted_string1;
  cardano_object_t*     object2             = (cardano_object_t*)ref_counted_string2;

  size_t new_size = cardano_set_add(set, object1);
  EXPECT_EQ(new_size, 1);

  new_size = cardano_set_add(set, object2);
  EXPECT_EQ(new_size, 2);

  // Act
  bool deleted = cardano_set_delete(set, object2);

  // Assert
  EXPECT_EQ(deleted, true);

  // Cleanup
  cardano_set_unref(&set);
  cardano_object_unref(&object1);
  cardano_object_unref(&object2);
}

TEST(cardano_get_entries, returnsNullIfSetIsNull)
{
  // Arrange
  cardano_set_t* set = nullptr;

  // Act
  cardano_array_t* array = cardano_get_entries(set);

  // Assert
  EXPECT_EQ(array, nullptr);
}

TEST(cardano_get_entries, returnsAnArrayContainingAllSetEntries)
{
  // Arrange
  cardano_set_t*        set                 = cardano_set_new(compare, hash);
  ref_counted_string_t* ref_counted_string1 = ref_counted_string_new("Hello, World!");
  ref_counted_string_t* ref_counted_string2 = ref_counted_string_new("Hello, World!");
  cardano_object_t*     object1             = (cardano_object_t*)ref_counted_string1;
  cardano_object_t*     object2             = (cardano_object_t*)ref_counted_string2;

  size_t new_size  = cardano_set_add(set, object1);
  size_t same_size = cardano_set_add(set, object2);

  // Act
  cardano_array_t* array = cardano_get_entries(set);

  // Assert
  EXPECT_EQ(new_size, 1);
  EXPECT_EQ(same_size, 1);
  EXPECT_THAT(array, testing::Not((cardano_array_t*)nullptr));

  // Cleanup
  cardano_set_unref(&set);
  cardano_array_unref(&array);
  cardano_object_unref(&object1);
  cardano_object_unref(&object2);
}

TEST(cardano_get_entries, returnsAnEmptyArrayIfSetIsEmpty)
{
  // Arrange
  cardano_set_t* set = cardano_set_new(compare, hash);

  // Act
  cardano_array_t* array = cardano_get_entries(set);

  // Assert
  EXPECT_EQ(cardano_array_get_size(array), 0);

  // Cleanup
  cardano_set_unref(&set);
  cardano_array_unref(&array);
}

TEST(cardano_get_entries, returnsNullIfAllocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_t* set = cardano_set_new(compare, hash);

  ref_counted_string_t* ref_counted_string = ref_counted_string_new("Hello, World!");
  cardano_object_t*     object             = (cardano_object_t*)ref_counted_string;

  size_t new_size = cardano_set_add(set, object);
  EXPECT_EQ(new_size, 1);

  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_array_t* array = cardano_get_entries(set);

  // Assert
  EXPECT_EQ(array, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_set_unref(&set);
  cardano_object_unref(&object);
}

TEST(cardano_get_entries, returnsNullIfReallocationFails)
{
  // Arrange
  reset_allocators_run_count();
  cardano_set_t* set = cardano_set_new(compare, hash);

  ref_counted_string_t* ref_counted_string = ref_counted_string_new("Hello, World!");
  cardano_object_t*     object             = (cardano_object_t*)ref_counted_string;

  size_t new_size = cardano_set_add(set, object);
  EXPECT_EQ(new_size, 1);

  cardano_set_allocators(malloc, fail_right_away_realloc, free);

  // Act
  cardano_array_t* array = cardano_get_entries(set);

  // Assert
  EXPECT_EQ(array, nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_set_unref(&set);
  cardano_object_unref(&object);
}

TEST(cardano_set_clear, doesNothingIfSetIsNull)
{
  // Arrange
  cardano_set_t* set = nullptr;

  // Act
  cardano_set_clear(set);

  // Assert
  EXPECT_EQ(cardano_set_get_size(set), 0);
}

TEST(cardano_set_clear, removesAllElementsFromTheSet)
{
  // Arrange
  cardano_set_t*        set                 = cardano_set_new(compare, hash);
  ref_counted_string_t* ref_counted_string1 = ref_counted_string_new("Hello, World!");
  ref_counted_string_t* ref_counted_string2 = ref_counted_string_new("Hello, World!");
  cardano_object_t*     object1             = (cardano_object_t*)ref_counted_string1;
  cardano_object_t*     object2             = (cardano_object_t*)ref_counted_string2;

  size_t new_size  = cardano_set_add(set, object1);
  size_t same_size = cardano_set_add(set, object2);

  // Act
  cardano_set_clear(set);

  // Assert
  EXPECT_EQ(new_size, 1);
  EXPECT_EQ(same_size, 1);
  EXPECT_EQ(cardano_set_get_size(set), 0);

  // Cleanup
  cardano_set_unref(&set);
  cardano_object_unref(&object1);
  cardano_object_unref(&object2);
}

TEST(cardano_set_find, returnsNullIfSetIsNull)
{
  // Arrange
  cardano_set_t* set = nullptr;

  // Act
  cardano_object_t* found = cardano_set_find(set, find_predicate, nullptr);

  // Assert
  EXPECT_EQ(found, nullptr);
}

TEST(cardano_set_find, returnsNullIfPredicateIsNull)
{
  // Arrange
  cardano_set_t* set = cardano_set_new(compare, hash);

  // Act
  cardano_object_t* found = cardano_set_find(set, nullptr, nullptr);

  // Assert
  EXPECT_EQ(found, nullptr);

  // Cleanup
  cardano_set_unref(&set);
}

TEST(cardano_set_find, returnsNullIfObjectIsNotInSet)
{
  // Arrange
  cardano_set_t*        set                = cardano_set_new(compare, hash);
  ref_counted_string_t* ref_counted_string = ref_counted_string_new("Hello, World!");

  // Act
  cardano_object_t* found = cardano_set_find(set, find_predicate, nullptr);

  // Assert
  EXPECT_EQ(found, nullptr);

  // Cleanup
  cardano_set_unref(&set);
  cardano_object_unref((cardano_object_t**)&ref_counted_string);
}

TEST(cardano_set_find, returnsTheObjectIfItIsInSet)
{
  // Arrange
  cardano_set_t*        set                = cardano_set_new(compare, hash);
  ref_counted_string_t* ref_counted_string = ref_counted_string_new("Hello, World!");
  cardano_object_t*     object             = (cardano_object_t*)ref_counted_string;

  size_t                            new_size = cardano_set_add(set, object);
  ref_counted_string_find_context_t context  = { .search_string = "Hello, World!" };

  // Act
  cardano_object_t* found = cardano_set_find(set, find_predicate, &context);

  // Assert
  EXPECT_EQ(new_size, 1);
  EXPECT_THAT(found, testing::Not((cardano_object_t*)nullptr));

  // Cleanup
  cardano_set_unref(&set);
  cardano_object_unref(&object);
  cardano_object_unref(&found);
}
