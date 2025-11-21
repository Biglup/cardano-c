/**
 * \file cip8.cpp
 *
 * \author angel.castillo
 * \date   Nov 21, 2025
 *
 * \section LICENSE
 *
 * Copyright 2025 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
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

#include <cardano/address/base_address.h>
#include <cardano/message_signing/cip8.h>
#include <gmock/gmock.h>

/* TEST VECTORS **************************************************************/

static const char* private_key_hex      = "d06d3744d9089b21b1fbb736a45d359ed5d5b4028800e70aa1a2968183cb68528ef06f1c2b289a85e09738d528869dd1f69f436ada4b471b12e950a2b9e780b6";
static const char* address_to_sign_with = "addr_test1qqja52pwpq7v7amg34r6x9dpp5le04n6cmqf2zpnurt2lm48wgx7j5cur9w0zxv7ky333eef3akg092hhcmp3teeth3qktnslv";
static const char* pubkey_hash          = "25da282e083ccf77688d47a315a10d3f97d67ac6c0950833e0d6afee";

/* HELPERS *******************************************************************/

static cardano_ed25519_private_key_t*
make_signing_key()
{
  cardano_ed25519_private_key_t* signing_key = nullptr;
  const cardano_error_t          error       = cardano_ed25519_private_key_from_extended_hex(
    private_key_hex,
    strlen(private_key_hex),
    &signing_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_NE(signing_key, nullptr);

  return signing_key;
}

static cardano_address_t*
make_address()
{
  cardano_address_t*    address = nullptr;
  const cardano_error_t error   = cardano_address_from_string(
    address_to_sign_with,
    strlen(address_to_sign_with),
    &address);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_NE(address, nullptr);

  return address;
}

static cardano_blake2b_hash_t*
make_pubkey_hash()
{
  cardano_blake2b_hash_t* key_hash = nullptr;
  const cardano_error_t   error    = cardano_blake2b_hash_from_hex(
    pubkey_hash,
    strlen(pubkey_hash),
    &key_hash);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_NE(key_hash, nullptr);

  return key_hash;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_cip8_sign, canCorrectlySignMessage)
{
  cardano_ed25519_private_key_t* signing_key = make_signing_key();
  cardano_address_t*             address     = make_address();

  const byte_t message[] = { 0xab, 0xc1, 0x23 };

  cardano_buffer_t* cose_sign1_out = nullptr;
  cardano_buffer_t* cose_key_out   = nullptr;

  cardano_error_t error = cardano_cip8_sign(
    message,
    sizeof(message),
    address,
    signing_key,
    &cose_sign1_out,
    &cose_key_out);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(cose_sign1_out, nullptr);
  ASSERT_NE(cose_key_out, nullptr);

  const size_t sign1_size = cardano_buffer_get_hex_size(cose_sign1_out);
  const size_t key_size   = cardano_buffer_get_hex_size(cose_key_out);

  EXPECT_GT(sign1_size, 0U);
  EXPECT_GT(key_size, 0U);

  char* cose_sign1_hex = static_cast<char*>(malloc(sign1_size));
  char* cose_key_hex   = static_cast<char*>(malloc(key_size));
  ASSERT_NE(cose_sign1_hex, nullptr);
  ASSERT_NE(cose_key_hex, nullptr);

  error = cardano_buffer_to_hex(cose_sign1_out, cose_sign1_hex, sign1_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_buffer_to_hex(cose_key_out, cose_key_hex, key_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(
    cose_sign1_hex,
    "845882a301270458390025da282e083ccf77688d47a315a10d3f97d67ac6c0950833e0d6afeea7720de9531c195cf1199eb12318e7298f6c879557be3618af395de2676164647265737358390025da282e083ccf77688d47a315a10d3f97d67ac6c0950833e0d6afeea7720de9531c195cf1199eb12318e7298f6c879557be3618af395de2a166686173686564f443abc1235840cf4f8356899ef40f4c21869b50a3d5dc95414a8d3c1aae088b7518a65069cdf841331877c16f11f6a88bbfe402e8fbb338a5646ff2d931d5e955c6717cf1c404");

  EXPECT_STREQ(
    cose_key_hex,
    "a501010258390025da282e083ccf77688d47a315a10d3f97d67ac6c0950833e0d6afeea7720de9531c195cf1199eb12318e7298f6c879557be3618af395de20327200621582088cb67866b59520bffcfe9c421ef5d9e0db88815637796f597d3305126c8c78c");

  cardano_buffer_unref(&cose_sign1_out);
  cardano_buffer_unref(&cose_key_out);
  cardano_address_unref(&address);
  cardano_ed25519_private_key_unref(&signing_key);
  free(cose_sign1_hex);
  free(cose_key_hex);
}

TEST(cardano_cip8_sign_ex, canCorrectlySignMessage)
{
  cardano_ed25519_private_key_t* signing_key = make_signing_key();
  cardano_blake2b_hash_t*        key_hash    = make_pubkey_hash();

  const byte_t message[] = { 0xab, 0xc1, 0x23 };

  cardano_buffer_t* cose_sign1_out = nullptr;
  cardano_buffer_t* cose_key_out   = nullptr;

  cardano_error_t error = cardano_cip8_sign_ex(
    message,
    sizeof(message),
    key_hash,
    signing_key,
    &cose_sign1_out,
    &cose_key_out);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  ASSERT_NE(cose_sign1_out, nullptr);
  ASSERT_NE(cose_key_out, nullptr);

  const size_t sign1_size = cardano_buffer_get_hex_size(cose_sign1_out);
  const size_t key_size   = cardano_buffer_get_hex_size(cose_key_out);

  EXPECT_GT(sign1_size, 0U);
  EXPECT_GT(key_size, 0U);

  char* cose_sign1_hex = static_cast<char*>(malloc(sign1_size));
  char* cose_key_hex   = static_cast<char*>(malloc(key_size));
  ASSERT_NE(cose_sign1_hex, nullptr);
  ASSERT_NE(cose_key_hex, nullptr);

  error = cardano_buffer_to_hex(cose_sign1_out, cose_sign1_hex, sign1_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_buffer_to_hex(cose_key_out, cose_key_hex, key_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(
    cose_sign1_hex,
    "845848a3012704581c25da282e083ccf77688d47a315a10d3f97d67ac6c0950833e0d6afee676b657948617368581c25da282e083ccf77688d47a315a10d3f97d67ac6c0950833e0d6afeea166686173686564f443abc123584021b71bfddb8794a41f6420c70968085b8bc4ca61d55980da8378eac2ceb9531efff499fd7f111a1f8d64674246aeddbfc4aed82c79f9f6cfa7f66b4a791b5c04");

  EXPECT_STREQ(
    cose_key_hex,
    "a5010102581c25da282e083ccf77688d47a315a10d3f97d67ac6c0950833e0d6afee0327200621582088cb67866b59520bffcfe9c421ef5d9e0db88815637796f597d3305126c8c78c");

  cardano_buffer_unref(&cose_sign1_out);
  cardano_buffer_unref(&cose_key_out);
  cardano_blake2b_hash_unref(&key_hash);
  cardano_ed25519_private_key_unref(&signing_key);
  free(cose_sign1_hex);
  free(cose_key_hex);
}

TEST(cardano_cip8_sign, returnsErrorOnNullArguments)
{
  const byte_t    message[] = { 0x01, 0x02 };
  cardano_error_t error     = cardano_cip8_sign(
    nullptr,
    sizeof(message),
    nullptr,
    nullptr,
    nullptr,
    nullptr);

  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_cip8_sign_ex, returnsErrorOnNullArguments)
{
  const byte_t    message[] = { 0x01, 0x02 };
  cardano_error_t error     = cardano_cip8_sign_ex(
    nullptr,
    sizeof(message),
    nullptr,
    nullptr,
    nullptr,
    nullptr);

  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_cip8_sign, rejectsZeroLengthMessage)
{
  cardano_ed25519_private_key_t* signing_key = make_signing_key();
  cardano_address_t*             address     = make_address();

  const byte_t dummy_message = 0x00;

  cardano_buffer_t* cose_sign1_out = nullptr;
  cardano_buffer_t* cose_key_out   = nullptr;

  const cardano_error_t error = cardano_cip8_sign(
    &dummy_message,
    0U,
    address,
    signing_key,
    &cose_sign1_out,
    &cose_key_out);

  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(cose_sign1_out, nullptr);
  EXPECT_EQ(cose_key_out, nullptr);

  cardano_address_unref(&address);
  cardano_ed25519_private_key_unref(&signing_key);
}

TEST(cardano_cip8_sign_ex, rejectsZeroLengthMessage)
{
  cardano_ed25519_private_key_t* signing_key = make_signing_key();
  cardano_blake2b_hash_t*        key_hash    = make_pubkey_hash();

  const byte_t dummy_message = 0x00;

  cardano_buffer_t* cose_sign1_out = nullptr;
  cardano_buffer_t* cose_key_out   = nullptr;

  const cardano_error_t error = cardano_cip8_sign_ex(
    &dummy_message,
    0U,
    key_hash,
    signing_key,
    &cose_sign1_out,
    &cose_key_out);

  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(cose_sign1_out, nullptr);
  EXPECT_EQ(cose_key_out, nullptr);

  cardano_blake2b_hash_unref(&key_hash);
  cardano_ed25519_private_key_unref(&signing_key);
}

TEST(cardano_cip8_sign, producesNonEmptyOutputsForArbitraryMessage)
{
  cardano_ed25519_private_key_t* signing_key = make_signing_key();
  cardano_address_t*             address     = make_address();

  const char*   message_str  = "hello CIP8";
  const byte_t* message      = reinterpret_cast<const byte_t*>(message_str);
  const size_t  message_size = strlen(message_str);

  cardano_buffer_t* cose_sign1_out = nullptr;
  cardano_buffer_t* cose_key_out   = nullptr;

  const cardano_error_t error = cardano_cip8_sign(
    message,
    message_size,
    address,
    signing_key,
    &cose_sign1_out,
    &cose_key_out);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_NE(cose_sign1_out, nullptr);
  EXPECT_NE(cose_key_out, nullptr);

  EXPECT_GT(cardano_buffer_get_size(cose_sign1_out), 0U);
  EXPECT_GT(cardano_buffer_get_size(cose_key_out), 0U);

  cardano_buffer_unref(&cose_sign1_out);
  cardano_buffer_unref(&cose_key_out);
  cardano_address_unref(&address);
  cardano_ed25519_private_key_unref(&signing_key);
}

TEST(cardano_cip8_sign, differentMessagesProduceDifferentSign1)
{
  cardano_ed25519_private_key_t* signing_key = make_signing_key();
  cardano_address_t*             address     = make_address();

  const byte_t msg1[] = { 0x01, 0x02, 0x03 };
  const byte_t msg2[] = { 0x01, 0x02, 0x04 };

  cardano_buffer_t* cose_sign1_msg1 = nullptr;
  cardano_buffer_t* cose_key_msg1   = nullptr;

  cardano_error_t error = cardano_cip8_sign(
    msg1,
    sizeof(msg1),
    address,
    signing_key,
    &cose_sign1_msg1,
    &cose_key_msg1);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_buffer_t* cose_sign1_msg2 = nullptr;
  cardano_buffer_t* cose_key_msg2   = nullptr;

  error = cardano_cip8_sign(
    msg2,
    sizeof(msg2),
    address,
    signing_key,
    &cose_sign1_msg2,
    &cose_key_msg2);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size1 = cardano_buffer_get_hex_size(cose_sign1_msg1);
  const size_t hex_size2 = cardano_buffer_get_hex_size(cose_sign1_msg2);

  ASSERT_GT(hex_size1, 0U);
  ASSERT_GT(hex_size2, 0U);

  char* hex1 = static_cast<char*>(malloc(hex_size1));
  char* hex2 = static_cast<char*>(malloc(hex_size2));

  ASSERT_NE(hex1, nullptr);
  ASSERT_NE(hex2, nullptr);

  error = cardano_buffer_to_hex(cose_sign1_msg1, hex1, hex_size1);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_buffer_to_hex(cose_sign1_msg2, hex2, hex_size2);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STRNE(hex1, hex2);

  free(hex1);
  free(hex2);
  cardano_buffer_unref(&cose_sign1_msg1);
  cardano_buffer_unref(&cose_key_msg1);
  cardano_buffer_unref(&cose_sign1_msg2);
  cardano_buffer_unref(&cose_key_msg2);
  cardano_address_unref(&address);
  cardano_ed25519_private_key_unref(&signing_key);
}
