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

static const char* ED25519_NOR_PUBLIC_KEY_HEX  = "bbdafd1393fffa82352b9792e7e8ff66fa05877a79a2486965e28049380c2cac";
static const char* ED25519_NOR_PRIVATE_KEY_HEX = "f04462421183d227bbc0fa60799ef338169c05eed7aa6aac19bc4db20557df51";
static const char* ED25519_PUBLIC_KEY_HEX      = "07473467683e6a30a13d471a68641f311a14e2b37a38ea592e5d6efc2b446bce";
static const char* ED25519_PRIVATE_KEY_HEX     = "f04462421183d227bbc0fa60799ef338169c05eed7aa6aac19bc4db20557df51e154255decce80ae4ab8a61af6abde05e7fbc049861cc040a7afe4fb0a875899";
static const char* PASSWORD                    = "password";
static const char* ENTROPY_BYTES               = "387183ffe785d467ab662c01acbcf79400e2430dde6c9aee74cf0602de0d82e8";
static const char* EXTENDED_ACCOUNT_0_PUB_KEY  = "1b39889a420374e41917cf420d88a84d9b40d7eeef533ac37f323076c5f7106a15ef170481a5c4333be2b4cf498525512ac4a3427e1a0e9c9f42cfcb42ba6deb";
static const char* TX_CBOR                     = "84a40081825820f6dd880fb30480aa43117c73bfd09442ba30de5644c3ec1a91d9232fbe715aab000182a20058390071213dc119131f48f54d62e339053388d9d84faedecba9d8722ad2cad9debf34071615fc6452dfc743a4963f6bec68e488001c7384942c13011b0000000253c8e4f6a300581d702ed2631dbb277c84334453c5c437b86325d371f0835a28b910a91a6e011a001e848002820058209d7fee57d1dbb9b000b2a133256af0f2c83ffe638df523b2d1c13d405356d8ae021a0002fb050b582088e4779d217d10398a705530f9fb2af53ffac20aef6e75e85c26e93a00877556a10481d8799fd8799f40ffd8799fa1d8799fd8799fd87980d8799fd8799f581c71213dc119131f48f54d62e339053388d9d84faedecba9d8722ad2caffd8799fd8799fd8799f581cd9debf34071615fc6452dfc743a4963f6bec68e488001c7384942c13ffffffffffd8799f4040ffff1a001e8480a0a000ffd87c9f9fd8799fd8799fd8799fd87980d8799fd8799f581caa47de0ab3b7f0b1d8d196406b6af1b0d88cd46168c49ca0557b4f70ffd8799fd8799fd8799f581cd4b8fc88aec1d1c2f43ca5587898d88da20ef73964b8cf6f8f08ddfbffffffffffd8799fd87980d8799fd8799f581caa47de0ab3b7f0b1d8d196406b6af1b0d88cd46168c49ca0557b4f70ffd8799fd8799fd8799f581cd4b8fc88aec1d1c2f43ca5587898d88da20ef73964b8cf6f8f08ddfbffffffffffd8799f4040ffd87a9f1a00989680ffffd87c9f9fd8799fd87a9fd8799f4752656c65617365d8799fd87980d8799fd8799f581caa47de0ab3b7f0b1d8d196406b6af1b0d88cd46168c49ca0557b4f70ffd8799fd8799fd8799f581cd4b8fc88aec1d1c2f43ca5587898d88da20ef73964b8cf6f8f08ddfbffffffffffff9fd8799f0101ffffffd87c9f9fd8799fd87b9fd9050280ffd87980ffff1b000001884e1fb1c0d87980ffffff1b000001884e1fb1c0d87980ffffff1b000001884e1fb1c0d87980fffff5f6";
static const char* VK_WITNESS_KEY_0            = "07473467683e6a30a13d471a68641f311a14e2b37a38ea592e5d6efc2b446bce";
static const char* VK_WITNESS_SIGNATURE_0      = "5f9f725da55e2a89e725f2c147512c0508956aae6a99cb2f3150c73c812c7373f57311dcee14cb02ad1ab7b1940aecc5bbf0769a9b77aafb996393b08d48830b";
static const char* VK_WITNESS_KEY_2            = "48f090d48246134d6307267451fcefbe4cd9df1530b9ac9a267e3e8cf28b6c61";
static const char* VK_WITNESS_SIGNATURE_2      = "9219b195082d71a1b6b9109862a6a053dc8b5342d3a31cc9067330c8f83824a92803a5fe39087fb8c73c746c6e278e98be24b1ddc0c1408c7d5a02776a7e3f07";
static const char* VK_WITNESS_KEY_3            = "a1765a8230536886e0fd7c6053d5e1d2ea9b22aaf72ffd7f35fe0aaf05c64466";
static const char* VK_WITNESS_SIGNATURE_3      = "ec8810c47be72d720643ca4bda73cc99f3fc6d61398b089aa6264c70347e593af0fa95739bfc6ec693b83d8e97ec837159248b63781a57edfec5ec2090853e06";
static const char* VK_WITNESS_KEY_4            = "9158f62358e9184caa207f017f2f74ec274de18a18c1de0ea83fed4f232ced71";
static const char* VK_WITNESS_SIGNATURE_4      = "008378f19cf610423daf39c1645a1281c77630eaf049d34ecfcb9a3c358905f11290a25dc871a493482e6c4f11c4fa7d67102031260d89ca8981c89a064a3107";
static const char* VK_WITNESS_SIGNATURES[]     = {
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

static const char* SERIALIZED_BIP32_KEY_HANDLER = "0a0a0a0a01010000005c97db5e09b3a4919ec75ed1126056241a1e5278731c2e0b01bea0a5f42c22db4131e0a4bbe75633677eb0e60e2ecd3520178f85c7e0d4be77a449087fe9674ee52f946b07c1b56d228c496ec0d36dd44212ba8af0f6eed1a82194dd69f479c603";

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

  return (int32_t)strlen(PASSWORD);
}

/**
 * \brief Retrieves an invalid password for the secure key handler.
 * \return -1.
 */
static int32_t
get_invalid_passphrase(byte_t*, const size_t)
{
  return -1;
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
#ifdef _MSC_VER
    sscanf_s(&hex[2 * i], "%2hhx", &buffer[i]);
#else
    sscanf(&hex[2 * i], "%2hhx", &buffer[i]);
#endif
  }
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_software_secure_key_handler_new, canCreateABip32SecureKeyHandler)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t entropy_bytes[1024];
  from_hex_to_buffer(ENTROPY_BYTES, entropy_bytes, strlen(ENTROPY_BYTES) / 2);

  cardano_error_t error = cardano_software_secure_key_handler_new(
    entropy_bytes,
    strlen(ENTROPY_BYTES) / 2,
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
  char         hex[1024];

  error = cardano_bip32_public_key_to_hex(extended_account_0_pub_key, hex, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(hex, EXTENDED_ACCOUNT_0_PUB_KEY);

  // Cleanup
  cardano_secure_key_handler_unref(&key_handler);
  cardano_bip32_public_key_unref(&extended_account_0_pub_key);
  free(key_handler);
}

TEST(cardano_software_secure_key_handler_bip32_sign_transaction, canSignTransactionWithBip32SecureKeyHandler)
{
  // Arrange
  cardano_transaction_t* transaction = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(TX_CBOR, strlen(TX_CBOR));

  cardano_error_t error = cardano_transaction_from_cbor(reader, &transaction);

  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t entropy_bytes[1024];
  from_hex_to_buffer(ENTROPY_BYTES, entropy_bytes, strlen(ENTROPY_BYTES) / 2);

  error = cardano_software_secure_key_handler_new(
    entropy_bytes,
    strlen(ENTROPY_BYTES) / 2,
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
    char         sig_hex[1024];

    error = cardano_ed25519_signature_to_hex(signature, sig_hex, sig_hex_size);

    EXPECT_EQ(error, CARDANO_SUCCESS);

    const size_t key_hex_size = cardano_ed25519_public_key_get_hex_size(key);
    char         key_hex[1024];

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

TEST(cardano_software_secure_key_handler_bip32_sign_transaction, failsWithInvalidPassword)
{
  // Arrange
  cardano_transaction_t* transaction = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(TX_CBOR, strlen(TX_CBOR));

  cardano_error_t error = cardano_transaction_from_cbor(reader, &transaction);

  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t entropy_bytes[1024];
  from_hex_to_buffer(ENTROPY_BYTES, entropy_bytes, strlen(ENTROPY_BYTES) / 2);

  error = cardano_software_secure_key_handler_new(
    entropy_bytes,
    strlen(ENTROPY_BYTES) / 2,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_invalid_passphrase,
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

  EXPECT_EQ(error, CARDANO_ERROR_INVALID_PASSPHRASE);

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
  cardano_transaction_unref(&transaction);
  cardano_secure_key_handler_unref(&key_handler);
}

TEST(cardano_software_secure_key_handler_serialize, canSerializeBip32SecureKeyHandler)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t entropy_bytes[1024];
  from_hex_to_buffer(ENTROPY_BYTES, entropy_bytes, strlen(ENTROPY_BYTES) / 2);

  cardano_error_t error = cardano_software_secure_key_handler_new(
    entropy_bytes,
    strlen(ENTROPY_BYTES) / 2,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_buffer_t* buffer = NULL;

  error = cardano_secure_key_handler_serialize(key_handler, &buffer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // deserialize it and compare key
  cardano_secure_key_handler_t* deserialized_key_handler = nullptr;

  error = cardano_software_secure_key_handler_deserialize(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer), &get_passphrase, &deserialized_key_handler);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_bip32_public_key_t* extended_account_0_pub_key = nullptr;

  error = cardano_secure_key_handler_bip32_get_extended_account_public_key(deserialized_key_handler, { CARDANO_CIP_1852_PURPOSE_STANDARD, CARDANO_CIP_1852_COIN_TYPE, 0U }, &extended_account_0_pub_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_bip32_public_key_get_hex_size(extended_account_0_pub_key);
  char         hex[1024];

  error = cardano_bip32_public_key_to_hex(extended_account_0_pub_key, hex, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(hex, EXTENDED_ACCOUNT_0_PUB_KEY);

  // Cleanup
  cardano_buffer_unref(&buffer);
  cardano_secure_key_handler_unref(&key_handler);
  cardano_secure_key_handler_unref(&deserialized_key_handler);
  cardano_bip32_public_key_unref(&extended_account_0_pub_key);
}

TEST(cardano_secure_key_handler_bip32_get_extended_account_public_key, returnsErrorIfInvalidPassword)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t entropy_bytes[1024];
  from_hex_to_buffer(ENTROPY_BYTES, entropy_bytes, strlen(ENTROPY_BYTES) / 2);

  cardano_error_t error = cardano_software_secure_key_handler_new(
    entropy_bytes,
    strlen(ENTROPY_BYTES) / 2,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_invalid_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_bip32_public_key_t* extended_account_0_pub_key = nullptr;

  error = cardano_secure_key_handler_bip32_get_extended_account_public_key(key_handler, { CARDANO_CIP_1852_PURPOSE_STANDARD, CARDANO_CIP_1852_COIN_TYPE, 0U }, &extended_account_0_pub_key);

  EXPECT_EQ(error, CARDANO_ERROR_INVALID_PASSPHRASE);

  // Cleanup
  cardano_secure_key_handler_unref(&key_handler);
  cardano_bip32_public_key_unref(&extended_account_0_pub_key);
}

TEST(cardano_software_secure_key_handler_new, canCreateAEd25519ExtendedSecureKeyHandler)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t key_bytes[1024];
  from_hex_to_buffer(ED25519_PRIVATE_KEY_HEX, key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2);

  cardano_ed25519_private_key_t* private_key = nullptr;

  cardano_error_t error = cardano_ed25519_private_key_from_extended_bytes(key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2, &private_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_software_secure_key_handler_ed25519_new(
    private_key,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_public_key_t* public_key = nullptr;

  error = cardano_secure_key_handler_ed25519_get_public_key(key_handler, &public_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_ed25519_public_key_get_hex_size(public_key);
  char         hex[1024];

  error = cardano_ed25519_public_key_to_hex(public_key, hex, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(hex, ED25519_PUBLIC_KEY_HEX);

  // Cleanup
  cardano_secure_key_handler_unref(&key_handler);
  cardano_ed25519_public_key_unref(&public_key);
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_software_secure_key_handler_ed25519_sign_transaction, canSignTransactionWithEd25519ExtendedSecureKeyHandler)
{
  // Arrange
  cardano_transaction_t* transaction = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(TX_CBOR, strlen(TX_CBOR));

  cardano_error_t error = cardano_transaction_from_cbor(reader, &transaction);

  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t key_bytes[1024];
  from_hex_to_buffer(ED25519_PRIVATE_KEY_HEX, key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2);

  cardano_ed25519_private_key_t* private_key = nullptr;

  error = cardano_ed25519_private_key_from_extended_bytes(key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2, &private_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_software_secure_key_handler_ed25519_new(
    private_key,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;

  error = cardano_secure_key_handler_ed25519_sign_transaction(key_handler, transaction, &vkey_witness_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_vkey_witness_set_get_length(vkey_witness_set), 1);

  cardano_vkey_witness_t* witness = NULL;

  error = cardano_vkey_witness_set_get(vkey_witness_set, 0, &witness);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_signature_t*  signature = cardano_vkey_witness_get_signature(witness);
  cardano_ed25519_public_key_t* key       = cardano_vkey_witness_get_vkey(witness);

  EXPECT_NE(signature, nullptr);
  EXPECT_NE(key, nullptr);

  const size_t sig_hex_size = cardano_ed25519_signature_get_hex_size(signature);
  char         sig_hex[1024];

  error = cardano_ed25519_signature_to_hex(signature, sig_hex, sig_hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t key_hex_size = cardano_ed25519_public_key_get_hex_size(key);
  char         key_hex[1024];

  error = cardano_ed25519_public_key_to_hex(key, key_hex, key_hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(sig_hex, VK_WITNESS_SIGNATURE_0);

  EXPECT_STREQ(key_hex, VK_WITNESS_KEY_0);

  // Cleanup
  cardano_vkey_witness_unref(&witness);
  cardano_ed25519_signature_unref(&signature);
  cardano_ed25519_public_key_unref(&key);
  cardano_vkey_witness_set_unref(&vkey_witness_set);
  cardano_transaction_unref(&transaction);
  cardano_secure_key_handler_unref(&key_handler);
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_software_secure_key_handler_ed25519_sign_transaction, failsWhenPasswordIsInvalid)
{
  // Arrange
  cardano_transaction_t* transaction = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(TX_CBOR, strlen(TX_CBOR));

  cardano_error_t error = cardano_transaction_from_cbor(reader, &transaction);

  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t key_bytes[1024];
  from_hex_to_buffer(ED25519_PRIVATE_KEY_HEX, key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2);

  cardano_ed25519_private_key_t* private_key = nullptr;

  error = cardano_ed25519_private_key_from_extended_bytes(key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2, &private_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_software_secure_key_handler_ed25519_new(
    private_key,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_invalid_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;

  error = cardano_secure_key_handler_ed25519_sign_transaction(key_handler, transaction, &vkey_witness_set);

  EXPECT_EQ(error, CARDANO_ERROR_INVALID_PASSPHRASE);

  // Cleanup
  cardano_vkey_witness_set_unref(&vkey_witness_set);
  cardano_transaction_unref(&transaction);
  cardano_secure_key_handler_unref(&key_handler);
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_software_secure_key_handler_new, canCreateAEd25519NormalSecureKeyHandler)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t key_bytes[1024];
  from_hex_to_buffer(ED25519_NOR_PRIVATE_KEY_HEX, key_bytes, strlen(ED25519_NOR_PRIVATE_KEY_HEX) / 2);

  cardano_ed25519_private_key_t* private_key = nullptr;

  cardano_error_t error = cardano_ed25519_private_key_from_normal_bytes(key_bytes, strlen(ED25519_NOR_PRIVATE_KEY_HEX) / 2, &private_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_software_secure_key_handler_ed25519_new(
    private_key,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_public_key_t* public_key = nullptr;

  error = cardano_secure_key_handler_ed25519_get_public_key(key_handler, &public_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_ed25519_public_key_get_hex_size(public_key);
  char         hex[1024];

  error = cardano_ed25519_public_key_to_hex(public_key, hex, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(hex, ED25519_NOR_PUBLIC_KEY_HEX);

  // Cleanup
  cardano_secure_key_handler_unref(&key_handler);
  cardano_ed25519_public_key_unref(&public_key);
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_secure_software_key_handler_ed25519_get_public_key, returnsErrorIfInvalidPassword)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t key_bytes[1024];
  from_hex_to_buffer(ED25519_NOR_PRIVATE_KEY_HEX, key_bytes, strlen(ED25519_NOR_PRIVATE_KEY_HEX) / 2);

  cardano_ed25519_private_key_t* private_key = nullptr;

  cardano_error_t error = cardano_ed25519_private_key_from_normal_bytes(key_bytes, strlen(ED25519_NOR_PRIVATE_KEY_HEX) / 2, &private_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_software_secure_key_handler_ed25519_new(
    private_key,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_invalid_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_public_key_t* public_key = nullptr;

  error = cardano_secure_key_handler_ed25519_get_public_key(key_handler, &public_key);

  EXPECT_EQ(error, CARDANO_ERROR_INVALID_PASSPHRASE);

  // Cleanup
  cardano_secure_key_handler_unref(&key_handler);
  cardano_ed25519_public_key_unref(&public_key);
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_software_secure_key_handler_ed25519_sign_transaction, canSignTransactionWithEd25519NormalSecureKeyHandler)
{
  // Arrange
  cardano_transaction_t* transaction = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(TX_CBOR, strlen(TX_CBOR));

  cardano_error_t error = cardano_transaction_from_cbor(reader, &transaction);

  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t key_bytes[1024];
  from_hex_to_buffer(ED25519_NOR_PRIVATE_KEY_HEX, key_bytes, strlen(ED25519_NOR_PRIVATE_KEY_HEX) / 2);

  cardano_ed25519_private_key_t* private_key = nullptr;

  error = cardano_ed25519_private_key_from_normal_bytes(key_bytes, strlen(ED25519_NOR_PRIVATE_KEY_HEX) / 2, &private_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_software_secure_key_handler_ed25519_new(
    private_key,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;

  error = cardano_secure_key_handler_ed25519_sign_transaction(key_handler, transaction, &vkey_witness_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_vkey_witness_set_get_length(vkey_witness_set), 1);

  cardano_vkey_witness_t* witness = NULL;

  error = cardano_vkey_witness_set_get(vkey_witness_set, 0, &witness);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_signature_t*  signature = cardano_vkey_witness_get_signature(witness);
  cardano_ed25519_public_key_t* key       = cardano_vkey_witness_get_vkey(witness);

  EXPECT_NE(signature, nullptr);
  EXPECT_NE(key, nullptr);

  const size_t sig_hex_size = cardano_ed25519_signature_get_hex_size(signature);
  char         sig_hex[1024];

  error = cardano_ed25519_signature_to_hex(signature, sig_hex, sig_hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t key_hex_size = cardano_ed25519_public_key_get_hex_size(key);
  char         key_hex[1024];

  error = cardano_ed25519_public_key_to_hex(key, key_hex, key_hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(sig_hex, "86576c12e53d8721801580fbfe6c72b814c43069f3aa200fc0c28acd78b80ed0b4b3a8cac7060c005058fee07163286f47c9beaaebcaa950fe289aa46e8a5e09");

  EXPECT_STREQ(key_hex, ED25519_NOR_PUBLIC_KEY_HEX);

  // Cleanup
  cardano_vkey_witness_unref(&witness);
  cardano_ed25519_signature_unref(&signature);
  cardano_ed25519_public_key_unref(&key);
  cardano_vkey_witness_set_unref(&vkey_witness_set);
  cardano_transaction_unref(&transaction);
  cardano_secure_key_handler_unref(&key_handler);
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_software_secure_key_handler_serialize, canSerializeEd25519SecureKeyHandler)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t key_bytes[1024];
  from_hex_to_buffer(ED25519_PRIVATE_KEY_HEX, key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2);

  cardano_ed25519_private_key_t* private_key = nullptr;

  cardano_error_t error = cardano_ed25519_private_key_from_extended_bytes(key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2, &private_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_software_secure_key_handler_ed25519_new(
    private_key,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_buffer_t* buffer = NULL;

  error = cardano_secure_key_handler_serialize(key_handler, &buffer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // deserialize it and compare key
  cardano_secure_key_handler_t* deserialized_key_handler = nullptr;

  error = cardano_software_secure_key_handler_deserialize(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer), &get_passphrase, &deserialized_key_handler);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_ed25519_public_key_t* public_key = nullptr;

  error = cardano_secure_key_handler_ed25519_get_public_key(deserialized_key_handler, &public_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_ed25519_public_key_get_hex_size(public_key);
  char         hex[1024];

  error = cardano_ed25519_public_key_to_hex(public_key, hex, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(hex, ED25519_PUBLIC_KEY_HEX);

  // Cleanup
  cardano_buffer_unref(&buffer);
  cardano_secure_key_handler_unref(&key_handler);
  cardano_secure_key_handler_unref(&deserialized_key_handler);
  cardano_ed25519_public_key_unref(&public_key);
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_software_secure_key_handler_new, returnsErrorIfEntropyBytesIsNull)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  cardano_error_t error = cardano_software_secure_key_handler_new(
    nullptr,
    0,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_software_secure_key_handler_new, returnsErrorIfEntropyBytesIsNotNullButSizeIsZero)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t entropy_bytes[1] = { 0 };

  cardano_error_t error = cardano_software_secure_key_handler_new(
    entropy_bytes,
    0,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_software_secure_key_handler_new, returnsErrorIfPasswordIsNull)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t entropy_bytes[1024];
  from_hex_to_buffer(ENTROPY_BYTES, entropy_bytes, strlen(ENTROPY_BYTES) / 2);

  cardano_error_t error = cardano_software_secure_key_handler_new(
    entropy_bytes,
    strlen(ENTROPY_BYTES) / 2,
    nullptr,
    0,
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_software_secure_key_handler_new, returnsErrorIfPasswordIsNoNtullButSizeIsZero)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t entropy_bytes[1024];
  from_hex_to_buffer(ENTROPY_BYTES, entropy_bytes, strlen(ENTROPY_BYTES) / 2);

  cardano_error_t error = cardano_software_secure_key_handler_new(
    entropy_bytes,
    strlen(ENTROPY_BYTES) / 2,
    (const byte_t*)&PASSWORD[0],
    0,
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
}

TEST(cardano_software_secure_key_handler_new, returnsErrorIfGetPassphraseIsNull)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t entropy_bytes[1024];
  from_hex_to_buffer(ENTROPY_BYTES, entropy_bytes, strlen(ENTROPY_BYTES) / 2);

  cardano_error_t error = cardano_software_secure_key_handler_new(
    entropy_bytes,
    strlen(ENTROPY_BYTES) / 2,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    nullptr,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_software_secure_key_handler_new, returnsErrorIfKeyHandlerIsNull)
{
  // Arrange
  byte_t entropy_bytes[1024];
  from_hex_to_buffer(ENTROPY_BYTES, entropy_bytes, strlen(ENTROPY_BYTES) / 2);

  cardano_error_t error = cardano_software_secure_key_handler_new(
    entropy_bytes,
    strlen(ENTROPY_BYTES) / 2,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_software_secure_key_handler_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t entropy_bytes[1024];
  from_hex_to_buffer(ENTROPY_BYTES, entropy_bytes, strlen(ENTROPY_BYTES) / 2);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t error = cardano_software_secure_key_handler_new(
    entropy_bytes,
    strlen(ENTROPY_BYTES) / 2,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  error = cardano_software_secure_key_handler_new(
    entropy_bytes,
    strlen(ENTROPY_BYTES) / 2,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_secure_key_handler_unref(&key_handler);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_software_secure_key_handler_ed25519_new, returnsErrorIfPrivateKeyIsNull)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  cardano_error_t error = cardano_software_secure_key_handler_ed25519_new(
    nullptr,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_software_secure_key_handler_ed25519_new, returnsErrorIfPasswordIsNull)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t key_bytes[1024];
  from_hex_to_buffer(ED25519_PRIVATE_KEY_HEX, key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2);

  cardano_ed25519_private_key_t* private_key = nullptr;

  cardano_error_t error = cardano_ed25519_private_key_from_extended_bytes(key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2, &private_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_software_secure_key_handler_ed25519_new(
    private_key,
    nullptr,
    0,
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_software_secure_key_handler_ed25519_new, returnsErrorIfPasswordIsNotNullButSizeIsZero)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t key_bytes[1024];
  from_hex_to_buffer(ED25519_PRIVATE_KEY_HEX, key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2);

  cardano_ed25519_private_key_t* private_key = nullptr;

  cardano_error_t error = cardano_ed25519_private_key_from_extended_bytes(key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2, &private_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_software_secure_key_handler_ed25519_new(
    private_key,
    (const byte_t*)&PASSWORD[0],
    0,
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_software_secure_key_handler_ed25519_new, returnsErrorIfGetPassphraseIsNull)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t key_bytes[1024];
  from_hex_to_buffer(ED25519_PRIVATE_KEY_HEX, key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2);

  cardano_ed25519_private_key_t* private_key = nullptr;

  cardano_error_t error = cardano_ed25519_private_key_from_extended_bytes(key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2, &private_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_software_secure_key_handler_ed25519_new(
    private_key,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    nullptr,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_software_secure_key_handler_ed25519_new, returnsErrorIfKeyHandlerIsNull)
{
  // Arrange
  byte_t key_bytes[1024];
  from_hex_to_buffer(ED25519_PRIVATE_KEY_HEX, key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2);

  cardano_ed25519_private_key_t* private_key = nullptr;

  cardano_error_t error = cardano_ed25519_private_key_from_extended_bytes(key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2, &private_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_software_secure_key_handler_ed25519_new(
    private_key,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_software_secure_key_handler_ed25519_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t key_bytes[1024];
  from_hex_to_buffer(ED25519_PRIVATE_KEY_HEX, key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2);

  cardano_ed25519_private_key_t* private_key = nullptr;

  cardano_error_t error = cardano_ed25519_private_key_from_extended_bytes(key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2, &private_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  error = cardano_software_secure_key_handler_ed25519_new(
    private_key,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  error = cardano_software_secure_key_handler_ed25519_new(
    private_key,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_secure_key_handler_unref(&key_handler);
  cardano_ed25519_private_key_unref(&private_key);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_software_secure_key_handler_serialize, returnsErrorIfKeyHandlerIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = NULL;

  cardano_error_t error = cardano_secure_key_handler_serialize(nullptr, &buffer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_software_secure_key_handler_serialize, returnsErrorIfBufferIsNull)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t entropy_bytes[1024];
  from_hex_to_buffer(ENTROPY_BYTES, entropy_bytes, strlen(ENTROPY_BYTES) / 2);

  cardano_error_t error = cardano_software_secure_key_handler_new(
    entropy_bytes,
    strlen(ENTROPY_BYTES) / 2,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_secure_key_handler_serialize(key_handler, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_secure_key_handler_unref(&key_handler);
}

TEST(cardano_software_secure_key_handler_serialize, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t entropy_bytes[1024];
  from_hex_to_buffer(ENTROPY_BYTES, entropy_bytes, strlen(ENTROPY_BYTES) / 2);

  cardano_error_t error = cardano_software_secure_key_handler_new(
    entropy_bytes,
    strlen(ENTROPY_BYTES) / 2,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_buffer_t* buffer = NULL;

  error = cardano_secure_key_handler_serialize(key_handler, &buffer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  error = cardano_secure_key_handler_serialize(key_handler, &buffer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_secure_key_handler_unref(&key_handler);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_software_secure_key_handler_deserialize, returnsErrorIfBufferIsNull)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  cardano_error_t error = cardano_software_secure_key_handler_deserialize(nullptr, 0, &get_passphrase, &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_software_secure_key_handler_deserialize, returnsErrorIfGetPassphraseIsNull)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  cardano_buffer_t* buffer = cardano_buffer_from_hex(SERIALIZED_BIP32_KEY_HANDLER, strlen(SERIALIZED_BIP32_KEY_HANDLER));

  cardano_error_t error = cardano_software_secure_key_handler_deserialize(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer), nullptr, &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_software_secure_key_handler_deserialize, returnsErrorIfKeyHandlerIsNull)
{
  // Arrange
  cardano_buffer_t* buffer = cardano_buffer_from_hex(SERIALIZED_BIP32_KEY_HANDLER, strlen(SERIALIZED_BIP32_KEY_HANDLER));

  cardano_error_t error = cardano_software_secure_key_handler_deserialize(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer), &get_passphrase, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_software_secure_key_handler_deserialize, doesntCrashIfInvalidSerializedData)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  cardano_buffer_t* buffer = cardano_buffer_from_hex(SERIALIZED_BIP32_KEY_HANDLER, strlen(SERIALIZED_BIP32_KEY_HANDLER));

  for (size_t i = 0; i < cardano_buffer_get_size(buffer) - 1; ++i)
  {
    cardano_error_t error = cardano_software_secure_key_handler_deserialize(cardano_buffer_get_data(buffer), i, &get_passphrase, &key_handler);

    EXPECT_NE(error, CARDANO_SUCCESS);
  }

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_software_secure_key_handler_deserialize, returnErrorIfInvalidMagic)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  const char* INVALID_SER_DATA = "1a0a0a0a01010000005c97db5e09b3a4919ec75ed1126056241a1e5278731c2e0b01bea0a5f42c22db4131e0a4bbe75633677eb0e60e2ecd3520178f85c7e0d4be77a449087fe9674ee52f946b07c1b56d228c496ec0d36dd44212ba8af0f6eed1a82194dd69f479c603";

  cardano_buffer_t* buffer = cardano_buffer_from_hex(INVALID_SER_DATA, strlen(INVALID_SER_DATA));

  // Act
  cardano_error_t error = cardano_software_secure_key_handler_deserialize(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer), &get_passphrase, &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_MAGIC);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_software_secure_key_handler_deserialize, returnErrorIfInvalidVersion)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  const char* INVALID_SER_DATA = "0a0a0a0a02010000005c97db5e09b3a4919ec75ed1126056241a1e5278731c2e0b01bea0a5f42c22db4131e0a4bbe75633677eb0e60e2ecd3520178f85c7e0d4be77a449087fe9674ee52f946b07c1b56d228c496ec0d36dd44212ba8af0f6eed1a82194dd69f479c603";

  cardano_buffer_t* buffer = cardano_buffer_from_hex(INVALID_SER_DATA, strlen(INVALID_SER_DATA));

  // Act
  cardano_error_t error = cardano_software_secure_key_handler_deserialize(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer), &get_passphrase, &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_software_secure_key_handler_deserialize, returnErrorIfInvalidEncryptedDataSize)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  const char* INVALID_SER_DATA = "0a0a0a0a01010000000097db5e09b3a4919ec75ed1126056241a1e5278731c2e0b01bea0a5f42c22db4131e0a4bbe75633677eb0e60e2ecd3520178f85c7e0d4be77a449087fe9674ee52f946b07c1b56d228c496ec0d36dd44212ba8af0f6eed1a82194dd69f479c603";

  cardano_buffer_t* buffer = cardano_buffer_from_hex(INVALID_SER_DATA, strlen(INVALID_SER_DATA));

  // Act
  cardano_error_t error = cardano_software_secure_key_handler_deserialize(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer), &get_passphrase, &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_software_secure_key_handler_deserialize, returnErrorIfInvalidChecksum)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  const char* INVALID_SER_DATA = "0a0a0a0a01010000005c97db5e09b3a4919ec75ed1126056241a1e5278731c2e0b01bea0a5f42c22db4131e0a4bbe75633677eb0e60e2ecd3520178f85c7e0d4be77a449087fe9674ee52f946b07c1b56d228c496ec0d36dd44212ba8af0f6eed1a82194dd69f479c600";

  cardano_buffer_t* buffer = cardano_buffer_from_hex(INVALID_SER_DATA, strlen(INVALID_SER_DATA));

  // Act
  cardano_error_t error = cardano_software_secure_key_handler_deserialize(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer), &get_passphrase, &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_CHECKSUM_MISMATCH);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_software_secure_key_handler_deserialize, returnErrorIfInvalidKeyHandlerType)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  const char* INVALID_SER_DATA = "0a0a0a0a01030000005c97db5e09b3a4919ec75ed1126056241a1e5278731c2e0b01bea0a5f42c22db4131e0a4bbe75633677eb0e60e2ecd3520178f85c7e0d4be77a449087fe9674ee52f946b07c1b56d228c496ec0d36dd44212ba8af0f6eed1a82194dd691f128c21";

  cardano_buffer_t* buffer = cardano_buffer_from_hex(INVALID_SER_DATA, strlen(INVALID_SER_DATA));

  // Act
  cardano_error_t error = cardano_software_secure_key_handler_deserialize(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer), &get_passphrase, &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_buffer_unref(&buffer);
}

TEST(cardano_software_secure_key_handler_deserialize, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_secure_key_handler_t* key_handler = nullptr;

  cardano_buffer_t* buffer = cardano_buffer_from_hex(SERIALIZED_BIP32_KEY_HANDLER, strlen(SERIALIZED_BIP32_KEY_HANDLER));

  // Act
  for (int i = 0; i < 6; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    const cardano_error_t error = cardano_software_secure_key_handler_deserialize(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer), &get_passphrase, &key_handler);

    // Assert
    EXPECT_TRUE((error == CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ) || (error == CARDANO_ERROR_MEMORY_ALLOCATION_FAILED));
  }

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_buffer_unref(&buffer);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_software_secure_key_handler_ed25519_sign_transaction, returnsErrorOnMemoryAllocationFail)
{
  // Arrange
  cardano_transaction_t* transaction = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(TX_CBOR, strlen(TX_CBOR));

  cardano_error_t error = cardano_transaction_from_cbor(reader, &transaction);

  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t key_bytes[1024];
  from_hex_to_buffer(ED25519_PRIVATE_KEY_HEX, key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2);

  cardano_ed25519_private_key_t* private_key = nullptr;

  error = cardano_ed25519_private_key_from_extended_bytes(key_bytes, strlen(ED25519_PRIVATE_KEY_HEX) / 2, &private_key);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_software_secure_key_handler_ed25519_new(
    private_key,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    &key_handler);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_vkey_witness_set_t* vkey_witness_set = nullptr;

  for (int i = 0; i < 24; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    error = cardano_secure_key_handler_ed25519_sign_transaction(key_handler, transaction, &vkey_witness_set);

    EXPECT_NE(error, CARDANO_SUCCESS);
  }

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_vkey_witness_set_unref(&vkey_witness_set);
  cardano_transaction_unref(&transaction);
  cardano_secure_key_handler_unref(&key_handler);
  cardano_ed25519_private_key_unref(&private_key);
}

TEST(cardano_software_secure_key_handler_bip32_sign_transaction, returnsErrorOnMemoryAllocationFail)
{
  // Arrange
  cardano_transaction_t* transaction = nullptr;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(TX_CBOR, strlen(TX_CBOR));

  cardano_error_t error = cardano_transaction_from_cbor(reader, &transaction);

  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_secure_key_handler_t* key_handler = nullptr;

  byte_t entropy_bytes[1024];
  from_hex_to_buffer(ENTROPY_BYTES, entropy_bytes, strlen(ENTROPY_BYTES) / 2);

  error = cardano_software_secure_key_handler_new(
    entropy_bytes,
    strlen(ENTROPY_BYTES) / 2,
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

  for (int i = 0; i < 135; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    error = cardano_secure_key_handler_bip32_sign_transaction(key_handler, transaction, &path[0], 4, &vkey_witness_set);

    EXPECT_NE(error, CARDANO_SUCCESS);
  }

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_vkey_witness_set_unref(&vkey_witness_set);
  cardano_transaction_unref(&transaction);
  cardano_secure_key_handler_unref(&key_handler);
}
