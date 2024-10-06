/**
 * \file cardano.cpp
 *
 * \author angel.castillo
 * \date   Mar 13, 2024
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

#include <cardano/cardano.h>
#include <gmock/gmock.h>

#include "../src/config.h"

/* UNIT TESTS ****************************************************************/

TEST(cardano_get_lib_version, returns_version)
{
  const char* version = cardano_get_lib_version();
  ASSERT_THAT(version, testing::StrEq(LIB_CARDANO_C_VERSION));
}

TEST(cardano_memzero, zeroes_buffer)
{
  uint8_t buffer[5] = { 0x01, 0x02, 0x03, 0x04, 0x05 };

  cardano_memzero(buffer, sizeof buffer);

  for (size_t i = 0; i < sizeof buffer; ++i)
  {
    ASSERT_EQ(buffer[i], 0);
  }
}

TEST(cardano_memzero, doesntCrashIfGivenNullOrEmpty)
{
  cardano_memzero(NULL, 0);
  cardano_memzero(NULL, 12);
}
