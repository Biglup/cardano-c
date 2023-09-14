/**
 * \file buffer.cpp
 *
 * \author angel.castillo
 * \date   Sep 09, 2023
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

extern "C" {
#include "../src/buffer/buffer.h"
}

#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_buffer_new, createsANewBufferWithTheGivenCapacity)
{
  // Arrange
  cardano_buffer_t* buffer = nullptr;

  // Act
  buffer = cardano_buffer_new(1000);

  // Assert
  EXPECT_THAT(buffer, testing::Not((cardano_buffer_t*)nullptr));
  EXPECT_EQ(cardano_buffer_get_size(buffer), 0);
  EXPECT_EQ(cardano_buffer_get_capacity(buffer), 1000);
}

TEST(cardano_buffer_free, canRelaseTheBuffer)
{
  // Arrange
  cardano_buffer_t* buffer = nullptr;

  // Assert
  buffer = cardano_buffer_new(1000);
  EXPECT_THAT(buffer, testing::Not((cardano_buffer_t*)nullptr));

  cardano_buffer_free(&buffer);
  EXPECT_THAT(buffer, (cardano_buffer_t*)nullptr);
}
