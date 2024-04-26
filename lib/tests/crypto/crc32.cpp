/**
 * \file crc32.cpp
 *
 * \author angel.castillo
 * \date   Apr 22, 2024
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

#include <cardano/crypto/crc32.h>
#include <gmock/gmock.h>

/* TEST VECTORS **************************************************************/

typedef struct
{
    const char* content;
    size_t      content_length;
    uint32_t    checksum;
} crc32_vectors_t;

static const crc32_vectors_t crc32_vectors[] = {
  { "", 0, 0 },
  { "The quick brown fox jumps over the lazy dog", 43, 0x414FA339 },
  { "various CRC algorithms input data", 33, 0x9BD366AE },
  { "Test vector from febooti.com", 28, 0x0C877F61 }
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_crypto_crc32, correctlyComputesChecksums)
{
  for (size_t i = 0; i < sizeof(crc32_vectors_t) / sizeof(crc32_vectors[0]); ++i)
  {
    EXPECT_EQ(
      cardano_checksum_crc32((byte_t*)crc32_vectors[i].content, crc32_vectors[i].content_length),
      crc32_vectors[i].checksum);
  }
}