/**
 * \file string_safe.cpp
 *
 * \author luisd.bianchi
 * \date   Apr 28, 2024
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

#include "../src/string_safe.h"
#include <gmock/gmock.h>

#include <cardano/typedefs.h>

/* UNIT TESTS ****************************************************************/

TEST(cardano_safe_memcpy, canCopyBytes)
{
  const size_t dest_size = 10U;
  const size_t count     = 6U;

  byte_t       dest[dest_size]     = { 0 };
  const byte_t src[count]          = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
  const byte_t expected[dest_size] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0, 0, 0, 0 };

  cardano_safe_memcpy(dest, dest_size, src, count);

  ASSERT_THAT(dest, testing::ElementsAreArray(expected));
}

TEST(cardano_safe_memcpy, dontOverflowBuffer)
{
  const size_t dest_size = 4U;
  const size_t count     = 6U;

  byte_t       dest[dest_size]     = { 0 };
  const byte_t src[count]          = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
  const byte_t expected[dest_size] = { 0x01, 0x02, 0x03, 0x04 };

  cardano_safe_memcpy(dest, dest_size, src, count);

  ASSERT_THAT(dest, testing::ElementsAreArray(expected));
}

TEST(cardano_safe_memcpy, canHandleNullDestination)
{
  const size_t dest_size = 10U;
  const size_t count     = 6U;

  const byte_t src[count] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };

  cardano_safe_memcpy(nullptr, dest_size, src, count);
}

TEST(cardano_safe_memcpy, canHandleNullSource)
{
  const size_t dest_size = 10U;
  const size_t count     = 6U;

  byte_t dest[dest_size] = { 0 };

  cardano_safe_memcpy(dest, dest_size, nullptr, count);
}

TEST(cardano_safe_memcpy, canHandleZeroDestinationSize)
{
  const size_t dest_size = 0U;
  const size_t count     = 6U;

  byte_t       dest[10]   = { 0 };
  const byte_t src[count] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };

  cardano_safe_memcpy(dest, dest_size, src, count);
}

TEST(cardano_safe_memcpy, canHandleZeroCount)
{
  const size_t dest_size = 10U;
  const size_t count     = 0U;

  byte_t       dest[dest_size] = { 0 };
  const byte_t src[6]          = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };

  cardano_safe_memcpy(dest, dest_size, src, count);
}

TEST(cardano_safe_strlen, canMeasureLength)
{
  const char*  str        = "Hello, World!";
  const size_t max_length = 20U;

  const size_t length = cardano_safe_strlen(str, max_length);

  ASSERT_EQ(length, 13U);
}

TEST(cardano_safe_strlen, canLimitLength)
{
  const char*  str        = "Hello, World!";
  const size_t max_length = 5U;

  const size_t length = cardano_safe_strlen(str, max_length);

  ASSERT_EQ(length, 5U);
}

TEST(cardano_safe_strlen, canHandleEmptyString)
{
  const char*  str        = "";
  const size_t max_length = 20U;

  const size_t length = cardano_safe_strlen(str, max_length);

  ASSERT_EQ(length, 0U);
}

TEST(cardano_safe_strlen, canHandleNullString)
{
  const char*  str        = nullptr;
  const size_t max_length = 20U;

  const size_t length = cardano_safe_strlen(str, max_length);

  ASSERT_EQ(length, 0U);
}

TEST(cardano_safe_strlen, canHandleNullMaxLength)
{
  const char*  str        = "Hello, World!";
  const size_t max_length = 0U;

  const size_t length = cardano_safe_strlen(str, max_length);

  ASSERT_EQ(length, 0U);
}
