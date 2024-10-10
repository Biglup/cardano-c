/**
 * \file software_secure_key_handler.cpp
 *
 * \author angel.castillo
 * \date   Oct 10, 2024
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

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <cardano/key_handlers/secure_key_handler_impl.h>
#include <cardano/key_handlers/software_secure_key_handler.h>

#include <cardano/key_handlers/cip_1852_constants.h>
#include <cardano/key_handlers/derivation_path.h>
#include <cardano/object.h>
#include <gmock/gmock.h>

/* CONSTANTS  ****************************************************************/

static const char* PASSWORD                   = "password";
static const char* ENTROPY_BYTES              = "387183ffe785d467ab662c01acbcf79400e2430dde6c9aee74cf0602de0d82e8";
static const char* EXTENDED_ACCOUNT_0_PUB_KEY = "1b39889a420374e41917cf420d88a84d9b40d7eeef533ac37f323076c5f7106a15ef170481a5c4333be2b4cf498525512ac4a3427e1a0e9c9f42cfcb42ba6deb";
static const char* TX_CBOR                    = "84a40081825820f6dd880fb30480aa43117c73bfd09442ba30de5644c3ec1a91d9232fbe715aab000182a20058390071213dc119131f48f54d62e339053388d9d84faedecba9d8722ad2cad9debf34071615fc6452dfc743a4963f6bec68e488001c7384942c13011b0000000253c8e4f6a300581d702ed2631dbb277c84334453c5c437b86325d371f0835a28b910a91a6e011a001e848002820058209d7fee57d1dbb9b000b2a133256af0f2c83ffe638df523b2d1c13d405356d8ae021a0002fb050b582088e4779d217d10398a705530f9fb2af53ffac20aef6e75e85c26e93a00877556a10481d8799fd8799f40ffd8799fa1d8799fd8799fd87980d8799fd8799f581c71213dc119131f48f54d62e339053388d9d84faedecba9d8722ad2caffd8799fd8799fd8799f581cd9debf34071615fc6452dfc743a4963f6bec68e488001c7384942c13ffffffffffd8799f4040ffff1a001e8480a0a000ffd87c9f9fd8799fd8799fd8799fd87980d8799fd8799f581caa47de0ab3b7f0b1d8d196406b6af1b0d88cd46168c49ca0557b4f70ffd8799fd8799fd8799f581cd4b8fc88aec1d1c2f43ca5587898d88da20ef73964b8cf6f8f08ddfbffffffffffd8799fd87980d8799fd8799f581caa47de0ab3b7f0b1d8d196406b6af1b0d88cd46168c49ca0557b4f70ffd8799fd8799fd8799f581cd4b8fc88aec1d1c2f43ca5587898d88da20ef73964b8cf6f8f08ddfbffffffffffd8799f4040ffd87a9f1a00989680ffffd87c9f9fd8799fd87a9fd8799f4752656c65617365d8799fd87980d8799fd8799f581caa47de0ab3b7f0b1d8d196406b6af1b0d88cd46168c49ca0557b4f70ffd8799fd8799fd8799f581cd4b8fc88aec1d1c2f43ca5587898d88da20ef73964b8cf6f8f08ddfbffffffffffff9fd8799f0101ffffffd87c9f9fd8799fd87b9fd9050280ffd87980ffff1b000001884e1fb1c0d87980ffffff1b000001884e1fb1c0d87980ffffff1b000001884e1fb1c0d87980fffff5f6";
static const char* VK_WITNESS_KEY_0           = "07473467683e6a30a13d471a68641f311a14e2b37a38ea592e5d6efc2b446bce";
static const char* VK_WITNESS_SIGNATURE_0     = "5f9f725da55e2a89e725f2c147512c0508956aae6a99cb2f3150c73c812c7373f57311dcee14cb02ad1ab7b1940aecc5bbf0769a9b77aafb996393b08d48830b";
static const char* VK_WITNESS_KEY_2           = "48f090d48246134d6307267451fcefbe4cd9df1530b9ac9a267e3e8cf28b6c61";
static const char* VK_WITNESS_SIGNATURE_2     = "9219b195082d71a1b6b9109862a6a053dc8b5342d3a31cc9067330c8f83824a92803a5fe39087fb8c73c746c6e278e98be24b1ddc0c1408c7d5a02776a7e3f07";
static const char* VK_WITNESS_KEY_3           = "a1765a8230536886e0fd7c6053d5e1d2ea9b22aaf72ffd7f35fe0aaf05c64466";
static const char* VK_WITNESS_SIGNATURE_3     = "ec8810c47be72d720643ca4bda73cc99f3fc6d61398b089aa6264c70347e593af0fa95739bfc6ec693b83d8e97ec837159248b63781a57edfec5ec2090853e06";
static const char* VK_WITNESS_KEY_4           = "9158f62358e9184caa207f017f2f74ec274de18a18c1de0ea83fed4f232ced71";
static const char* VK_WITNESS_SIGNATURE_4     = "008378f19cf610423daf39c1645a1281c77630eaf049d34ecfcb9a3c358905f11290a25dc871a493482e6c4f11c4fa7d67102031260d89ca8981c89a064a3107";
static const char* VK_WITNESS_SIGNATURES[]    = {
  VK_WITNESS_SIGNATURE_0,
  VK_WITNESS_SIGNATURE_2,
  VK_WITNESS_SIGNATURE_3,
  VK_WITNESS_SIGNATURE_4
};

static const char* VK_WITNESS_KEYS[] = {
  VK_WITNESS_KEY_0,
  VK_WITNESS_KEY_2,
  VK_WITNESS_KEY_3,
  VK_WITNESS_KEY_4
};

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Retrieves the password for the secure key handler.
 *
 * \param buffer The buffer where to write the password.
 * \param buffer_len The size of the buffer.
 *
 * \return 0 if the password was successfully retrieved, -1 otherwise.
 */
static int32_t
get_passphrase(byte_t* buffer, const size_t buffer_len)
{
  if (buffer_len < strlen(PASSWORD))
  {
    return -1;
  }

  (void)memcpy(buffer, PASSWORD, strlen(PASSWORD));

  return strlen(PASSWORD);
}

/**
 * \brief Converts hex string to byte array.
 *
 * \param hex The hex string.
 * \param buffer The buffer where to write the byte array.
 * \param buffer_length The size of the buffer.
 */
static void
from_hex_to_buffer(const char* hex, byte_t* buffer, const size_t buffer_length)
{
  for (size_t i = 0; i < buffer_length; ++i)
  {
    sscanf(&hex[2 * i], "%2hhx", &buffer[i]);
  }
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_software_secure_key_handler_new, canCreateABip32SecureKeyHandler)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t entropy_bytes[strlen(ENTROPY_BYTES) / 2];
  from_hex_to_buffer(ENTROPY_BYTES, entropy_bytes, sizeof entropy_bytes);

  cardano_error_t error = cardano_software_secure_key_handler_new(
    entropy_bytes,
    sizeof entropy_bytes,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_bip32_public_key_t* extended_account_0_pub_key = nullptr;

  error = cardano_secure_key_handler_bip32_get_extended_account_public_key(key_handler, { CARDANO_CIP_1852_PURPOSE_STANDARD, CARDANO_CIP_1852_COIN_TYPE, 0U }, &extended_account_0_pub_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_bip32_public_key_get_hex_size(extended_account_0_pub_key);
  char         hex[hex_size];

  error = cardano_bip32_public_key_to_hex(extended_account_0_pub_key, hex, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(hex, EXTENDED_ACCOUNT_0_PUB_KEY);

  // Cleanup
  cardano_secure_key_handler_unref(&key_handler);
  cardano_bip32_public_key_unref(&extended_account_0_pub_key);
  free(key_handler);
}

TEST(cardano_software_secure_key_handler_new, canSignTransactionWithBip32SecureKeyHandler)
{
  // Arrange
  cardano_transaction_t* transaction = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(TX_CBOR, strlen(TX_CBOR));

  cardano_error_t error = cardano_transaction_from_cbor(reader, &transaction);

  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t entropy_bytes[strlen(ENTROPY_BYTES) / 2];
  from_hex_to_buffer(ENTROPY_BYTES, entropy_bytes, sizeof entropy_bytes);

  error = cardano_software_secure_key_handler_new(
    entropy_bytes,
    sizeof entropy_bytes,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;
  cardano_derivation_path_t   path[]           = {
    { CARDANO_CIP_1852_PURPOSE_STANDARD, CARDANO_CIP_1852_COIN_TYPE, 0U, 0U, 0U },
    { CARDANO_CIP_1852_PURPOSE_STANDARD, CARDANO_CIP_1852_COIN_TYPE, 0U, 2U, 0U },
    { CARDANO_CIP_1852_PURPOSE_STANDARD, CARDANO_CIP_1852_COIN_TYPE, 0U, 3U, 0U },
    { CARDANO_CIP_1852_PURPOSE_STANDARD, CARDANO_CIP_1852_COIN_TYPE, 0U, 4U, 0U }
  };

  error = cardano_secure_key_handler_bip32_sign_transaction(key_handler, transaction, &path[0], 4, &vkey_witness_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_vkey_witness_set_get_length(vkey_witness_set), 4);

  for (size_t i = 0; i < 4; ++i)
  {
    cardano_vkey_witness_t* witness = NULL;

    error = cardano_vkey_witness_set_get(vkey_witness_set, i, &witness);

    EXPECT_EQ(error, CARDANO_SUCCESS);

    cardano_ed25519_signature_t*  signature = cardano_vkey_witness_get_signature(witness);
    cardano_ed25519_public_key_t* key       = cardano_vkey_witness_get_vkey(witness);

    EXPECT_NE(signature, nullptr);
    EXPECT_NE(key, nullptr);

    const size_t sig_hex_size = cardano_ed25519_signature_get_hex_size(signature);
    char         sig_hex[sig_hex_size];

    error = cardano_ed25519_signature_to_hex(signature, sig_hex, sig_hex_size);

    EXPECT_EQ(error, CARDANO_SUCCESS);

    const size_t key_hex_size = cardano_ed25519_public_key_get_hex_size(key);
    char         key_hex[key_hex_size];

    error = cardano_ed25519_public_key_to_hex(key, key_hex, key_hex_size);

    EXPECT_EQ(error, CARDANO_SUCCESS);

    EXPECT_STREQ(sig_hex, VK_WITNESS_SIGNATURES[i]);
    EXPECT_STREQ(key_hex, VK_WITNESS_KEYS[i]);

    cardano_vkey_witness_unref(&witness);

    cardano_ed25519_signature_unref(&signature);
    cardano_ed25519_public_key_unref(&key);
  }

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
  cardano_transaction_unref(&transaction);
  cardano_secure_key_handler_unref(&key_handler);
}