/**
 * \file cbor_initial_byte.cpp
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

#include "../../src/cbor/cbor_initial_byte.h"
#include <gmock/gmock.h>

/* UNIT TESTS ****************************************************************/

TEST(cbor_initial_byte, cardano_cbor_initial_byte_pack)
{
  // Arrange
  const byte_t initial_byte = cardano_cbor_initial_byte_pack(CBOR_MAJOR_TYPE_UNSIGNED_INTEGER, CBOR_ADDITIONAL_INFO_8BIT_DATA);

  // Assert
  EXPECT_EQ(initial_byte, 0x18);
}

TEST(cbor_initial_byte, cardano_cbor_initial_byte_get_major_type)
{
  // Arrange
  const cbor_major_type_t major_type = cardano_cbor_initial_byte_get_major_type(0x18);

  // Assert
  EXPECT_EQ(major_type, CBOR_MAJOR_TYPE_UNSIGNED_INTEGER);
}

TEST(cbor_initial_byte, cardano_cbor_initial_byte_get_additional_info)
{
  // Arrange
  const cbor_additional_info_t additional_info = cardano_cbor_initial_byte_get_additional_info(0x18);

  // Assert
  EXPECT_EQ(additional_info, CBOR_ADDITIONAL_INFO_8BIT_DATA);
}
