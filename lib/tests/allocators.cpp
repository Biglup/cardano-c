/**
 * \file allocators.cpp
 *
 * \author luisd.bianchi
 * \date   Mar 11, 2024
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

#include <cardano/allocators.h>
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_set_allocators, cbor_set_allocators)
{
  // Arrange
  _cardano_malloc_t  custom_malloc  = (_cardano_malloc_t)NULL;
  _cardano_realloc_t custom_realloc = (_cardano_realloc_t)NULL;
  _cardano_free_t    custom_free    = (_cardano_free_t)NULL;

  // Act
  cardano_set_allocators(custom_malloc, custom_realloc, custom_free);

  // Assert
  ASSERT_EQ((ptrdiff_t)_cardano_malloc, (ptrdiff_t)NULL);
  ASSERT_EQ((ptrdiff_t)_cardano_realloc, (ptrdiff_t)NULL);
  ASSERT_EQ((ptrdiff_t)_cardano_free, (ptrdiff_t)NULL);
}
