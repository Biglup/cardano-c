/**
 * \file vendor_secp256k1.cpp
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

#ifndef SECP256K1_STATIC
#define SECP256K1_STATIC
#endif

#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_schnorrsig.h>

#include <array>
#include <cstdint>
#include <gtest/gtest.h>

/* STATIC HELPERS ************************************************************/

namespace {

template <size_t N>
std::array<uint8_t, N>
from_hex(const char* hex)
{
  std::array<uint8_t, N> out{};

  for (size_t i = 0U; i < N; ++i)
  {
    auto nibble = [](char c) -> uint8_t {
      if ((c >= '0') && (c <= '9'))
      {
        return static_cast<uint8_t>(c - '0');
      }

      if ((c >= 'a') && (c <= 'f'))
      {
        return static_cast<uint8_t>((c - 'a') + 10);
      }

      return static_cast<uint8_t>((c - 'A') + 10);
    };

    out[i] = static_cast<uint8_t>((nibble(hex[2U * i]) << 4) | nibble(hex[(2U * i) + 1U]));
  }

  return out;
}

/* KAT: ECDSA vector produced by signing with the vendored library, secret key
 * 01..20, message 0xA0..0xBF. The verify side here is the actual subject. */
constexpr const char* ECDSA_PUBKEY_HEX =
  "0284bf7562262bbd6940085748f3be6afa52ae317155181ece31b66351ccffa4b0";
constexpr const char* ECDSA_MSG_HEX =
  "a0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebf";
constexpr const char* ECDSA_SIG_HEX =
  "b8ce6c0e1cf885cecd1a17c28c67eaa8ec94d7c7fe89be6cf08db7db759d14a9"
  "7d07482d174a2aeb9ce750dfd61a9acb3d58d75cf78c0c22e709c2b353bed95f";

/* KAT: BIP340 official test vector 0
 * https://github.com/bitcoin/bips/blob/master/bip-0340/test-vectors.csv */
constexpr const char* SCHNORR0_PUBKEY_HEX =
  "F9308A019258C31049344F85F89D5229B531C845836F99B08601F113BCE036F9";
constexpr const char* SCHNORR0_MSG_HEX =
  "0000000000000000000000000000000000000000000000000000000000000000";
constexpr const char* SCHNORR0_SIG_HEX =
  "E907831F80848D1069A5371B402410364BDF1C5F8307B0084C55F1CE2DCA8215"
  "25F66A4A85EA8B71E482A74F382D2CE5EBEEE8FDB2172F477DF4900D310536C0";

/* KAT: BIP340 official test vector 1 */
constexpr const char* SCHNORR1_PUBKEY_HEX =
  "DFF1D77F2A671C5F36183726DB2341BE58FEAE1DA2DECED843240F7B502BA659";
constexpr const char* SCHNORR1_MSG_HEX =
  "243F6A8885A308D313198A2E03707344A4093822299F31D0082EFA98EC4E6C89";
constexpr const char* SCHNORR1_SIG_HEX =
  "6896BD60EEAE296DB48A229FF71DFE071BDE413E6D43F917DC8DCF8C78DE3341"
  "8906D11AC976ABCCB20B091292BFF4EA897EFCB639EA871CFA95F6DE339E4B0A";

} // namespace

/* UNIT TESTS ****************************************************************/

TEST(vendor_secp256k1, ecdsa_verify_accepts_valid_signature)
{
  // Arrange
  secp256k1_context* ctx     = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
  auto               pub_in  = from_hex<33>(ECDSA_PUBKEY_HEX);
  auto               msg     = from_hex<32>(ECDSA_MSG_HEX);
  auto               sig_in  = from_hex<64>(ECDSA_SIG_HEX);
  secp256k1_pubkey   pubkey;
  secp256k1_ecdsa_signature sig;

  ASSERT_NE(ctx, nullptr);
  ASSERT_EQ(secp256k1_ec_pubkey_parse(ctx, &pubkey, pub_in.data(), pub_in.size()), 1);
  ASSERT_EQ(secp256k1_ecdsa_signature_parse_compact(ctx, &sig, sig_in.data()), 1);

  // Act
  int valid = secp256k1_ecdsa_verify(ctx, &sig, msg.data(), &pubkey);

  // Assert
  EXPECT_EQ(valid, 1);

  secp256k1_context_destroy(ctx);
}

TEST(vendor_secp256k1, ecdsa_verify_rejects_tampered_signature)
{
  // Arrange
  secp256k1_context* ctx    = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
  auto               pub_in = from_hex<33>(ECDSA_PUBKEY_HEX);
  auto               msg    = from_hex<32>(ECDSA_MSG_HEX);
  auto               sig_in = from_hex<64>(ECDSA_SIG_HEX);
  secp256k1_pubkey   pubkey;
  secp256k1_ecdsa_signature sig;

  ASSERT_NE(ctx, nullptr);
  ASSERT_EQ(secp256k1_ec_pubkey_parse(ctx, &pubkey, pub_in.data(), pub_in.size()), 1);

  msg[0] = static_cast<uint8_t>(msg[0] ^ 0x01U);
  ASSERT_EQ(secp256k1_ecdsa_signature_parse_compact(ctx, &sig, sig_in.data()), 1);

  // Act
  int valid = secp256k1_ecdsa_verify(ctx, &sig, msg.data(), &pubkey);

  // Assert
  EXPECT_EQ(valid, 0);

  secp256k1_context_destroy(ctx);
}

TEST(vendor_secp256k1, schnorr_verify_accepts_bip340_vector_0)
{
  // Arrange
  secp256k1_context*    ctx    = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
  auto                  pub_in = from_hex<32>(SCHNORR0_PUBKEY_HEX);
  auto                  msg    = from_hex<32>(SCHNORR0_MSG_HEX);
  auto                  sig    = from_hex<64>(SCHNORR0_SIG_HEX);
  secp256k1_xonly_pubkey pubkey;

  ASSERT_NE(ctx, nullptr);
  ASSERT_EQ(secp256k1_xonly_pubkey_parse(ctx, &pubkey, pub_in.data()), 1);

  // Act
  int valid = secp256k1_schnorrsig_verify(ctx, sig.data(), msg.data(), msg.size(), &pubkey);

  // Assert
  EXPECT_EQ(valid, 1);

  secp256k1_context_destroy(ctx);
}

TEST(vendor_secp256k1, schnorr_verify_accepts_bip340_vector_1)
{
  // Arrange
  secp256k1_context*    ctx    = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
  auto                  pub_in = from_hex<32>(SCHNORR1_PUBKEY_HEX);
  auto                  msg    = from_hex<32>(SCHNORR1_MSG_HEX);
  auto                  sig    = from_hex<64>(SCHNORR1_SIG_HEX);
  secp256k1_xonly_pubkey pubkey;

  ASSERT_NE(ctx, nullptr);
  ASSERT_EQ(secp256k1_xonly_pubkey_parse(ctx, &pubkey, pub_in.data()), 1);

  // Act
  int valid = secp256k1_schnorrsig_verify(ctx, sig.data(), msg.data(), msg.size(), &pubkey);

  // Assert
  EXPECT_EQ(valid, 1);

  secp256k1_context_destroy(ctx);
}

TEST(vendor_secp256k1, schnorr_verify_rejects_tampered_signature)
{
  // Arrange
  secp256k1_context*    ctx    = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
  auto                  pub_in = from_hex<32>(SCHNORR0_PUBKEY_HEX);
  auto                  msg    = from_hex<32>(SCHNORR0_MSG_HEX);
  auto                  sig    = from_hex<64>(SCHNORR0_SIG_HEX);
  secp256k1_xonly_pubkey pubkey;

  ASSERT_NE(ctx, nullptr);
  ASSERT_EQ(secp256k1_xonly_pubkey_parse(ctx, &pubkey, pub_in.data()), 1);

  sig[0] = static_cast<uint8_t>(sig[0] ^ 0x01U);

  // Act
  int valid = secp256k1_schnorrsig_verify(ctx, sig.data(), msg.data(), msg.size(), &pubkey);

  // Assert
  EXPECT_EQ(valid, 0);

  secp256k1_context_destroy(ctx);
}
