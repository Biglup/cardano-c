/**
 * \file object.cpp
 *
 * \author luisd.bianchi
 * \date   Mar 03, 2024
 *
 * \section LICENSE
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

#include "../src/allocators.h"
#include <cardano/object.h>

#include <gmock/gmock.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Allocates and initializes a new Cardano object.
 *
 * This function creates a new instance of a Cardano object, setting up its initial state,
 * including the reference count and the deallocator function. The reference count is
 * initialized to 1, indicating that the caller has the first reference to the newly
 * created object. The deallocator function provided will be called to free the object's
 * resources when its reference count reaches zero.
 *
 * \param deallocator The function to be called to deallocate the object's resources
 *                    when it is no longer needed. This function should handle
 *                    deallocating both the object's specific resources (if any) and
 *                    the object itself.
 *
 * \return A pointer to the newly created Cardano object. If the object could not be
 *         created, returns NULL.
 *
 * \note The caller is responsible for managing the reference count of the object,
 *       including calling `cardano_object_unref` when the object is no longer needed.
 *
 * \warning It is crucial that the deallocator function correctly frees all resources
 *          associated with the object to avoid memory leaks.
 */
static cardano_object_t*
cardano_object_new(cardano_object_deallocator_t deallocator)
{
  if (deallocator == NULL)
  {
    return NULL;
  }

  cardano_object_t* object = (cardano_object_t*)malloc(sizeof(cardano_object_t));

  if (object != NULL)
  {
    object->ref_count     = 1U;
    object->deallocator   = deallocator;
    object->last_error[0] = '\0';
  }

  return object;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_object_new, createsANewObjectWithTheAllocator)
{
  // Arrange
  cardano_object_t* object = nullptr;

  // Act
  object = cardano_object_new(_cardano_free);

  // Assert
  EXPECT_THAT(object, testing::Not((cardano_object_t*)nullptr));
  EXPECT_EQ(cardano_object_refcount(object), 1);

  // Cleanup
  cardano_object_unref(&object);
}

TEST(cardano_object_new_from, returnsNullIfGivenNullAllocator)
{
  // Arrange
  cardano_object_t* object = nullptr;

  // Act
  object = cardano_object_new(nullptr);

  // Assert
  EXPECT_EQ(object, nullptr);
}

TEST(cardano_object_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_object_t* object = nullptr;

  // Act
  object = cardano_object_new(_cardano_free);
  cardano_object_ref(object);

  // Assert
  EXPECT_THAT(object, testing::Not((cardano_object_t*)nullptr));
  EXPECT_EQ(cardano_object_refcount(object), 2);

  // Cleanup
  cardano_object_unref(&object);
  cardano_object_unref(&object);
}

TEST(cardano_object_ref, doesntCrashIfObjectIsNull)
{
  // Arrange
  cardano_object_t* object = nullptr;

  // Act
  cardano_object_ref(nullptr);

  // Assert
  EXPECT_EQ(object, nullptr);
}

TEST(cardano_object_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_object_t* object = nullptr;

  // Act
  cardano_object_unref(&object);
}

TEST(cardano_object_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_object_unref((cardano_object_t**)nullptr);
}

TEST(cardano_object_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_object_t* object = cardano_object_new(_cardano_free);

  // Act
  cardano_object_ref(object);
  size_t ref_count = cardano_object_refcount(object);

  cardano_object_unref(&object);
  size_t updated_ref_count = cardano_object_refcount(object);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_object_unref(&object);
}

TEST(cardano_object_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_object_t* object = cardano_object_new(_cardano_free);

  // Act
  cardano_object_ref(object);
  size_t ref_count = cardano_object_refcount(object);

  cardano_object_unref(&object);
  size_t updated_ref_count = cardano_object_refcount(object);

  cardano_object_unref(&object);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(object, (cardano_object_t*)nullptr);
}

TEST(cardano_object_refcount, returnsZeroIfObjectIsNull)
{
  // Arrange
  cardano_object_t* object = nullptr;

  // Act
  size_t size = cardano_object_refcount(object);

  // Assert
  EXPECT_EQ(size, 0);
}

TEST(cardano_object_get_last_error, returnsNullTerminatedMessage)
{
  // Arrange
  cardano_object_t* object  = cardano_object_new(_cardano_free);
  const char*       message = "This is a test message";

  // Act
  cardano_object_set_last_error(object, message);
  const char* last_error = cardano_object_get_last_error(object);

  // Assert
  EXPECT_STREQ(last_error, message);

  // Cleanup
  cardano_object_unref(&object);
}

TEST(cardano_object_get_last_error, returnsObjectIsNullWhenCalledForNullObject)
{
  // Arrange
  cardano_object_t* object = nullptr;

  // Act
  const char* last_error = cardano_object_get_last_error(object);

  // Assert
  EXPECT_STREQ(last_error, "Object is NULL.");
}

TEST(cardano_object_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_object_t* object  = nullptr;
  const char*       message = "This is a test message";

  // Act
  cardano_object_set_last_error(object, message);

  // Assert
  EXPECT_STREQ(cardano_object_get_last_error(object), "Object is NULL.");
}

TEST(cardano_object_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_object_t* object  = cardano_object_new(_cardano_free);
  const char*       message = nullptr;

  // Act
  cardano_object_set_last_error(object, message);

  // Assert
  EXPECT_STREQ(cardano_object_get_last_error(object), "");

  // Cleanup
  cardano_object_unref(&object);
}
