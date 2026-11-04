/**
 * \file emip3.cpp
 *
 * \author angel.castillo
 * \date   Oct 08, 2023
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

#include <cardano/error.h>

#include <cardano/crypto/emip3.h>
#include <cardano/transaction_builder/coin_selection/random_improve_coin_selector.h>

#include <gmock/gmock.h>

#include <array>
#include <atomic>
#include <limits>
#include <thread>

/* TEST VECTORS **************************************************************/

typedef struct
{
    const char*   hex_data;
    size_t        hex_data_Length;
    const byte_t* password;
    size_t        password_length;
    const char*   encrypted;
} emip3_vectors_t;

static const emip3_vectors_t emip3_test_vectors[] = {
  { "00010203040506070809", strlen("00010203040506070809"), (const byte_t*)"password", strlen("password"), "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000009ce1d7784a05efd109ad89c29fea0775bf085ac03988089b3a93" },
  { "00010203040506070809", strlen("00010203040506070809"), NULL, 0, "0430bb0e1941fd9ec98909e766447883b4af77242a81c7ef2ba8d339f0deeae383227e257c0d6f28ad372a1bc9b87a30e3544258b21a2b576746f5fb83746c7a8e1fa37e2ca3" },
  { "0001020304050607080900010203040506070809000102030405060708090001020304050607080900010203040506070809", strlen("0001020304050607080900010203040506070809000102030405060708090001020304050607080900010203040506070809"), (const byte_t*)"password", strlen("password"), "8daaa90b5e998ac815d0ad9675c5bf328fcf48d12a49aabf01f99d1fc8e4512da687709825ae705bfdbdc7d8b0c662add2bccadbadb9a519d03f9205484f8ba0d66f3d66cd2864c26e8d563fd01a23a066c42b7a94db41e71d70171722012119bc90c51c9ca3a2f1d5041474a544" },
  { "00", strlen("00"), (const byte_t*)"password2222", strlen("password2222"), "ae02db6264aeb86d3dfb8fa33af204ac8189b116d38b7e701c37922034b359c1beaa734fc7fa80d4ab9271e3082aa69bd7e0b355315c986eb740369264" },
  { "a5010102583900d73b4d5548f4d00a1947e9284ccdcdc565dd4b85b36e88533c54ed9bfa2e192363674c755f5efe81c620f18bddf8cf63f181d1366fffef34032720062158203fe822fca223192577130a288b766fcac5b2b8972d89fc229bbc00af60aeaf67", strlen("a5010102583900d73b4d5548f4d00a1947e9284ccdcdc565dd4b85b36e88533c54ed9bfa2e192363674c755f5efe81c620f18bddf8cf63f181d1366fffef34032720062158203fe822fca223192577130a288b766fcac5b2b8972d89fc229bbc00af60aeaf67"), (const byte_t*)"password", strlen("password"), "a8de4eedfe023ee4e00986099c293d6e61ddbb3fbe3c449085820fc42316c52af99236a7387280198214149d6342506bf0e36c3c9244f9af6e3e6ba62821dd984c13e49b7513d96abe529fa1375511c9baab72cc13ed20e4b19cbe09b5e13245da1a9552ff2e35c90e815973c0a77dc401cbef86850cb16cb50b2bda4c7f00c687fcc7409c8f0f08f8af2e66115da8c992daebd42ae3faa563bcc53bb9d1a9b4a96b" },
};

/* STATIC FUNCTIONS **********************************************************/

static void
from_hex_to_buffer(const char* hex, byte_t* buffer, const size_t bufferLength)
{
  for (size_t i = 0; i < bufferLength; ++i)
  {
#ifdef _MSC_VER
    sscanf_s(&hex[2 * i], "%2hhx", &buffer[i]);
#else
    sscanf(&hex[2 * i], "%2hhx", &buffer[i]);
#endif
  }
}

namespace
{
void
emip3_round_trip_thread(const size_t index, std::atomic<size_t>* ready_count, std::atomic<bool>* start, bool* success)
{
  const byte_t             input          = static_cast<byte_t>(index);
  cardano_buffer_t*        encrypted_data = NULL;
  cardano_buffer_t*        decrypted_data = NULL;
  cardano_coin_selector_t* selector       = NULL;

  ready_count->fetch_add(1U);
  while (!start->load())
  {
    std::this_thread::yield();
  }

  // Let both native RNG entry points compete to initialize libsodium.
  if (((index % 2U) == 0U) && (cardano_random_improve_coin_selector_new(&selector) != CARDANO_SUCCESS))
  {
    return;
  }

  if (cardano_crypto_emip3_encrypt(&input, 1, NULL, 0, &encrypted_data) == CARDANO_SUCCESS)
  {
    if (cardano_crypto_emip3_decrypt(cardano_buffer_get_data(encrypted_data), cardano_buffer_get_size(encrypted_data), NULL, 0, &decrypted_data) == CARDANO_SUCCESS)
    {
      *success = (cardano_buffer_get_size(decrypted_data) == 1U) && (cardano_buffer_get_data(decrypted_data)[0] == input);
    }
  }

  if ((index % 2U) != 0U)
  {
    *success = (cardano_random_improve_coin_selector_new(&selector) == CARDANO_SUCCESS) && *success;
  }

  cardano_coin_selector_unref(&selector);
  cardano_buffer_unref(&decrypted_data);
  cardano_buffer_unref(&encrypted_data);
}
} // namespace

/* UNIT TESTS ****************************************************************/

TEST(cardano_crypto_emip3_encrypt, correctlyComputesCipherForTestVectors)
{
  for (size_t i = 0; i < sizeof(emip3_test_vectors) / sizeof(emip3_test_vectors[0]); ++i)
  {
    const emip3_vectors_t* vector = &emip3_test_vectors[i];

    byte_t plain_data[2048] = { 0 };
    from_hex_to_buffer(vector->hex_data, plain_data, vector->hex_data_Length / 2);

    byte_t data[2048] = { 0 };
    from_hex_to_buffer(vector->encrypted, data, strlen(vector->encrypted) / 2);

    cardano_buffer_t* encrypted_data = NULL;
    ASSERT_EQ(cardano_crypto_emip3_encrypt(plain_data, vector->hex_data_Length / 2, vector->password, vector->password_length, &encrypted_data), CARDANO_SUCCESS);

    cardano_buffer_t* decrypted_data = NULL;
    cardano_error_t   result         = cardano_crypto_emip3_decrypt(cardano_buffer_get_data(encrypted_data), cardano_buffer_get_size(encrypted_data), vector->password, vector->password_length, &decrypted_data);

    ASSERT_EQ(CARDANO_SUCCESS, result);

    const size_t encrypted_data_length    = cardano_buffer_get_hex_size(encrypted_data);
    char         encrypted_data_hex[2048] = { 0 };

    result = cardano_buffer_to_hex(encrypted_data, encrypted_data_hex, encrypted_data_length);

    ASSERT_EQ(CARDANO_SUCCESS, result);

    const size_t decrypted_data_length    = cardano_buffer_get_hex_size(decrypted_data);
    char         decrypted_data_hex[2048] = { 0 };

    result = cardano_buffer_to_hex(decrypted_data, decrypted_data_hex, decrypted_data_length);

    cardano_buffer_unref(&decrypted_data);
    cardano_buffer_unref(&encrypted_data);

    ASSERT_EQ(CARDANO_SUCCESS, result);

    EXPECT_STREQ(decrypted_data_hex, vector->hex_data);
  }
}

TEST(cardano_crypto_emip3_decrypt, correctlyDecryptsCipherForTestVectors)
{
  for (size_t i = 0; i < sizeof(emip3_test_vectors) / sizeof(emip3_test_vectors[0]); ++i)
  {
    const emip3_vectors_t* vector = &emip3_test_vectors[i];

    byte_t data[2048] = { 0 };
    from_hex_to_buffer(vector->encrypted, data, strlen(vector->encrypted) / 2);

    cardano_buffer_t* decrypted_data = NULL;
    cardano_error_t   result         = cardano_crypto_emip3_decrypt(data, strlen(vector->encrypted) / 2, vector->password, vector->password_length, &decrypted_data);

    ASSERT_EQ(CARDANO_SUCCESS, result);

    char decrypted_data_hex[2048] = { 0 };

    const size_t decrypted_data_length = cardano_buffer_get_hex_size(decrypted_data);
    result                             = cardano_buffer_to_hex(decrypted_data, decrypted_data_hex, decrypted_data_length);

    cardano_buffer_unref(&decrypted_data);

    ASSERT_EQ(CARDANO_SUCCESS, result);

    EXPECT_STREQ(decrypted_data_hex, vector->hex_data);
  }
}

TEST(cardano_crypto_emip3_encrypt, returnsErrorIfDataIsNull)
{
  cardano_buffer_t* encrypted_data = NULL;
  cardano_error_t   result         = cardano_crypto_emip3_encrypt(NULL, 0, (const byte_t*)"password", strlen("password"), &encrypted_data);
  ASSERT_EQ(CARDANO_ERROR_POINTER_IS_NULL, result);
}

TEST(cardano_crypto_emip3_encrypt, returnsErrorIfOutputIsNull)
{
  byte_t          data[1] = { 0 };
  cardano_error_t result  = cardano_crypto_emip3_encrypt(data, sizeof data, (const byte_t*)"password", strlen("password"), NULL);
  ASSERT_EQ(CARDANO_ERROR_POINTER_IS_NULL, result);
}

TEST(cardano_crypto_emip3_decrypt, returnsErrorIfDataIsNull)
{
  cardano_buffer_t* decrypted_data = NULL;
  cardano_error_t   result         = cardano_crypto_emip3_decrypt(NULL, 60, (const byte_t*)"password", strlen("password"), &decrypted_data);
  ASSERT_EQ(CARDANO_ERROR_POINTER_IS_NULL, result);
}

TEST(cardano_crypto_emip3_decrypt, returnsErrorIfOutputIsNull)
{
  byte_t          data[1] = { 0 };
  cardano_error_t result  = cardano_crypto_emip3_decrypt(data, 60, (const byte_t*)"password", strlen("password"), NULL);
  ASSERT_EQ(CARDANO_ERROR_POINTER_IS_NULL, result);
}

TEST(cardano_crypto_emip3_encrypt, roundTripsEmptyPlaintext)
{
  // Arrange
  const byte_t      input          = 0;
  cardano_buffer_t* encrypted_data = NULL;
  cardano_buffer_t* decrypted_data = NULL;

  // Act
  ASSERT_EQ(CARDANO_SUCCESS, cardano_crypto_emip3_encrypt(&input, 0, NULL, 0, &encrypted_data));
  ASSERT_EQ(CARDANO_SUCCESS, cardano_crypto_emip3_decrypt(cardano_buffer_get_data(encrypted_data), cardano_buffer_get_size(encrypted_data), NULL, 0, &decrypted_data));

  // Assert
  EXPECT_EQ(60U, cardano_buffer_get_size(encrypted_data));
  EXPECT_EQ(0U, cardano_buffer_get_size(decrypted_data));
  EXPECT_NE(nullptr, cardano_buffer_get_data(decrypted_data));
  cardano_buffer_unref(&decrypted_data);
  cardano_buffer_unref(&encrypted_data);
}

TEST(cardano_crypto_emip3_encrypt, rejectsInvalidPassphraseAndClearsOutput)
{
  // Arrange
  const byte_t      input  = 0;
  cardano_buffer_t* output = reinterpret_cast<cardano_buffer_t*>(1);

  // Act
  const cardano_error_t result = cardano_crypto_emip3_encrypt(&input, 1, NULL, 1, &output);

  // Assert
  EXPECT_EQ(CARDANO_ERROR_POINTER_IS_NULL, result);
  EXPECT_EQ(nullptr, output);
}

TEST(cardano_crypto_emip3_decrypt, rejectsInvalidPassphraseAndClearsOutput)
{
  // Arrange
  const byte_t      input[60] = { 0 };
  cardano_buffer_t* output    = reinterpret_cast<cardano_buffer_t*>(1);

  // Act
  const cardano_error_t result = cardano_crypto_emip3_decrypt(input, sizeof(input), NULL, 1, &output);

  // Assert
  EXPECT_EQ(CARDANO_ERROR_POINTER_IS_NULL, result);
  EXPECT_EQ(nullptr, output);
}

TEST(cardano_crypto_emip3_decrypt, rejectsTruncatedEnvelopeAndClearsOutput)
{
  // Arrange
  const byte_t      input[59] = { 0 };
  cardano_buffer_t* output    = reinterpret_cast<cardano_buffer_t*>(1);

  // Act
  const cardano_error_t result = cardano_crypto_emip3_decrypt(input, sizeof(input), NULL, 0, &output);

  // Assert
  EXPECT_EQ(CARDANO_ERROR_INVALID_ARGUMENT, result);
  EXPECT_EQ(nullptr, output);
}

TEST(cardano_crypto_emip3_encrypt, rejectsOversizedPayloadBeforeReadingInput)
{
  // Arrange
  const byte_t   input         = 0;
  const uint64_t counter_limit = 64ULL * ((1ULL << 32U) - 1ULL);
  const size_t   lengths[]     = { std::numeric_limits<size_t>::max(), static_cast<size_t>(counter_limit + 1ULL) };

  for (const size_t length: lengths)
  {
    if (length <= counter_limit && length != std::numeric_limits<size_t>::max())
    {
      continue;
    }

    cardano_buffer_t* output = reinterpret_cast<cardano_buffer_t*>(1);

    // Act
    const cardano_error_t result = cardano_crypto_emip3_encrypt(&input, length, NULL, 0, &output);

    // Assert
    EXPECT_EQ(CARDANO_ERROR_INVALID_ARGUMENT, result);
    EXPECT_EQ(nullptr, output);
  }
}

TEST(cardano_crypto_emip3_decrypt, rejectsOversizedPayloadBeforeReadingInput)
{
  // Arrange
  const byte_t   input         = 0;
  const uint64_t counter_limit = 64ULL * ((1ULL << 32U) - 1ULL);
  if (std::numeric_limits<size_t>::max() <= counter_limit)
  {
    return;
  }
  cardano_buffer_t* output = reinterpret_cast<cardano_buffer_t*>(1);

  // Act
  const cardano_error_t result = cardano_crypto_emip3_decrypt(&input, static_cast<size_t>(counter_limit + 61ULL), NULL, 0, &output);

  // Assert
  EXPECT_EQ(CARDANO_ERROR_INVALID_ARGUMENT, result);
  EXPECT_EQ(nullptr, output);
}

// Run this case with its GoogleTest filter in a fresh process to exercise
// initialization before any other crypto test has initialized libsodium.
TEST(cardano_crypto_emip3_encrypt, supportsConcurrentFirstUse)
{
  // Arrange
  constexpr size_t                      thread_count = 8;
  std::atomic<size_t>                   ready_count  = 0;
  std::atomic<bool>                     start        = false;
  std::array<bool, thread_count>        success      = {};
  std::array<std::thread, thread_count> threads;

  // Act
  for (size_t i = 0; i < thread_count; ++i)
  {
    threads[i] = std::thread(emip3_round_trip_thread, i, &ready_count, &start, &success[i]);
  }
  while (ready_count.load() != thread_count)
  {
    std::this_thread::yield();
  }
  start.store(true);
  for (std::thread& thread: threads)
  {
    thread.join();
  }

  // Assert
  for (const bool result: success)
  {
    EXPECT_TRUE(result);
  }
}

TEST(cardano_crypto_emip3_decrypt, rejectsUnauthenticatedEmptyPlaintextAndClearsOutput)
{
  // Arrange
  const byte_t      input          = 0;
  cardano_buffer_t* encrypted_data = NULL;
  ASSERT_EQ(CARDANO_SUCCESS, cardano_crypto_emip3_encrypt(&input, 0, NULL, 0, &encrypted_data));
  cardano_buffer_get_data(encrypted_data)[44] ^= 1U;
  cardano_buffer_t* decrypted_data            = reinterpret_cast<cardano_buffer_t*>(1);

  // Act
  const cardano_error_t result = cardano_crypto_emip3_decrypt(cardano_buffer_get_data(encrypted_data), cardano_buffer_get_size(encrypted_data), NULL, 0, &decrypted_data);

  // Assert
  EXPECT_EQ(CARDANO_ERROR_GENERIC, result);
  EXPECT_EQ(nullptr, decrypted_data);
  cardano_buffer_unref(&encrypted_data);
}
