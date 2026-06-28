/**
 * \file vendor_crypto.cpp
 *
 * \author angel.castillo
 * \date   Jun 19, 2026
 *
 * Copyright 2026 Biglup Labs
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

extern "C"
{
#include <ripemd160.h>
#include <sha3.h>
}

#include <cstdint>
#include <gtest/gtest.h>
#include <string>

/* STATIC FUNCTIONS **********************************************************/

namespace
{
std::string
to_hex(const uint8_t* data, const size_t len)
{
  static const char* digits = "0123456789abcdef";
  std::string        out;
  out.reserve(len * 2U);

  for (size_t i = 0U; i < len; ++i)
  {
    out.push_back(digits[(data[i] >> 4) & 0x0FU]);
    out.push_back(digits[data[i] & 0x0FU]);
  }

  return out;
}

std::string
sha3_256_hex(const std::string& message)
{
  uint8_t    digest[32] = { 0 };
  sha3_ctx_t ctx;

  sha3_init(&ctx, 32);
  sha3_update(&ctx, message.data(), message.size());
  sha3_final_pad(digest, &ctx, 0x06);

  return to_hex(digest, sizeof(digest));
}

std::string
keccak_256_hex(const std::string& message)
{
  uint8_t    digest[32] = { 0 };
  sha3_ctx_t ctx;

  sha3_init(&ctx, 32);
  sha3_update(&ctx, message.data(), message.size());
  sha3_final_pad(digest, &ctx, 0x01);

  return to_hex(digest, sizeof(digest));
}

std::string
ripemd160_hex(const std::string& message)
{
  uint8_t digest[RIPEMD160_DIGEST_LENGTH] = { 0 };

  ripemd160(reinterpret_cast<const uint8_t*>(message.data()), message.size(), digest);

  return to_hex(digest, sizeof(digest));
}
} // namespace

/* UNIT TESTS ****************************************************************/

TEST(vendor_crypto, sha3_256_empty_matches_known_answer)
{
  // Act
  const std::string digest = sha3_256_hex("");

  // Assert
  EXPECT_EQ(digest, "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a");
}

TEST(vendor_crypto, keccak_256_empty_matches_known_answer)
{
  // Act
  const std::string digest = keccak_256_hex("");

  // Assert
  EXPECT_EQ(digest, "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
}

TEST(vendor_crypto, ripemd160_empty_matches_known_answer)
{
  // Act
  const std::string digest = ripemd160_hex("");

  // Assert
  EXPECT_EQ(digest, "9c1185a5c5e9fc54612808977ee8f548b2258d31");
}

TEST(vendor_crypto, ripemd160_abc_matches_known_answer)
{
  // Act
  const std::string digest = ripemd160_hex("abc");

  // Assert
  EXPECT_EQ(digest, "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc");
}
