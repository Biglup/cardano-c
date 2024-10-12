/**
 * \file secure_key_handler_example.c
 *
 * \author angel.castillo
 * \date   Oct 13, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
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

#include "utils/console.h"
#include "utils/utils.h"
#include <cardano/cardano.h>

#include <stdlib.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const char* SERIALIZED_BIP32_KEY_HANDLER = "0a0a0a0a01010000005c97db5e09b3a4919ec75ed1126056241a1e5278731c2e0b01bea0a5f42c22db4131e0a4bbe75633677eb0e60e2ecd3520178f85c7e0d4be77a449087fe9674ee52f946b07c1b56d228c496ec0d36dd44212ba8af0f6eed1a82194dd69f479c603";
static const char* TX_CBOR                      = "84a40081825820f6dd880fb30480aa43117c73bfd09442ba30de5644c3ec1a91d9232fbe715aab000182a20058390071213dc119131f48f54d62e339053388d9d84faedecba9d8722ad2cad9debf34071615fc6452dfc743a4963f6bec68e488001c7384942c13011b0000000253c8e4f6a300581d702ed2631dbb277c84334453c5c437b86325d371f0835a28b910a91a6e011a001e848002820058209d7fee57d1dbb9b000b2a133256af0f2c83ffe638df523b2d1c13d405356d8ae021a0002fb050b582088e4779d217d10398a705530f9fb2af53ffac20aef6e75e85c26e93a00877556a10481d8799fd8799f40ffd8799fa1d8799fd8799fd87980d8799fd8799f581c71213dc119131f48f54d62e339053388d9d84faedecba9d8722ad2caffd8799fd8799fd8799f581cd9debf34071615fc6452dfc743a4963f6bec68e488001c7384942c13ffffffffffd8799f4040ffff1a001e8480a0a000ffd87c9f9fd8799fd8799fd8799fd87980d8799fd8799f581caa47de0ab3b7f0b1d8d196406b6af1b0d88cd46168c49ca0557b4f70ffd8799fd8799fd8799f581cd4b8fc88aec1d1c2f43ca5587898d88da20ef73964b8cf6f8f08ddfbffffffffffd8799fd87980d8799fd8799f581caa47de0ab3b7f0b1d8d196406b6af1b0d88cd46168c49ca0557b4f70ffd8799fd8799fd8799f581cd4b8fc88aec1d1c2f43ca5587898d88da20ef73964b8cf6f8f08ddfbffffffffffd8799f4040ffd87a9f1a00989680ffffd87c9f9fd8799fd87a9fd8799f4752656c65617365d8799fd87980d8799fd8799f581caa47de0ab3b7f0b1d8d196406b6af1b0d88cd46168c49ca0557b4f70ffd8799fd8799fd8799f581cd4b8fc88aec1d1c2f43ca5587898d88da20ef73964b8cf6f8f08ddfbffffffffffff9fd8799f0101ffffffd87c9f9fd8799fd87b9fd9050280ffd87980ffff1b000001884e1fb1c0d87980ffffff1b000001884e1fb1c0d87980ffffff1b000001884e1fb1c0d87980fffff5f6";

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
  console_write("Enter passphrase: ");
  char          password[128] = { 0 };
  const int32_t password_len  = console_read_password(password, sizeof(password));

  if (buffer_len < password_len)
  {
    return -1;
  }

  (void)memcpy(buffer, &password[0], password_len);

  // Clear local password from memory
  cardano_memzero(password, sizeof(password));

  return password_len;
}

/* MAIN **********************************************************************/

/**
 * \brief Entry point of the program.
 *
 * \return Returns `0` on successful execution, or a non-zero value if there is an error.
 */
int
main(void)
{
  console_info("Cardano secure key handler Example");
  console_debug("libcardano-c:  V-%s", cardano_get_lib_version());
  console_info("Use passphrase: 'password'\n");

  cardano_buffer_t*             serialized_key_handler = cardano_buffer_from_hex(SERIALIZED_BIP32_KEY_HANDLER, strlen(SERIALIZED_BIP32_KEY_HANDLER));
  cardano_secure_key_handler_t* key_handler            = NULL;

  cardano_error_t error = cardano_software_secure_key_handler_deserialize(
    cardano_buffer_get_data(serialized_key_handler),
    cardano_buffer_get_size(serialized_key_handler),
    get_passphrase,
    &key_handler);

  cardano_buffer_unref(&serialized_key_handler);

  if (error != CARDANO_SUCCESS)
  {
    console_error("Error: %s", cardano_error_to_string(error));
    return EXIT_FAILURE;
  }

  // Get extended account public key
  console_info("Requesting extended account public key...");
  cardano_bip32_public_key_t*       extended_public_key = NULL;
  cardano_account_derivation_path_t path                = { CARDANO_CIP_1852_PURPOSE_STANDARD, CARDANO_CIP_1852_COIN_TYPE, 0U };
  error                                                 = cardano_secure_key_handler_bip32_get_extended_account_public_key(key_handler, path, &extended_public_key);

  if (error != CARDANO_SUCCESS)
  {
    cardano_secure_key_handler_unref(&key_handler);

    console_error("Error: %s", cardano_error_to_string(error));
    return EXIT_FAILURE;
  }

  const size_t key_hex_size = cardano_bip32_public_key_get_hex_size(extended_public_key);
  char         key_hex[key_hex_size];

  error = cardano_bip32_public_key_to_hex(extended_public_key, key_hex, key_hex_size);

  if (error != CARDANO_SUCCESS)
  {
    cardano_secure_key_handler_unref(&key_handler);
    cardano_bip32_public_key_unref(&extended_public_key);

    console_error("Error: %s", cardano_error_to_string(error));
    return EXIT_FAILURE;
  }

  console_info("Extended account public key: %s\n", key_hex);

  cardano_bip32_public_key_unref(&extended_public_key);

  // Sign transaction
  console_info("Requesting signature for transaction...");
  cardano_transaction_t* transaction = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(TX_CBOR, strlen(TX_CBOR));

  error = cardano_transaction_from_cbor(reader, &transaction);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_secure_key_handler_unref(&key_handler);
    console_error("Error: %s", cardano_error_to_string(error));

    return EXIT_FAILURE;
  }

  cardano_vkey_witness_set_t* vkey_witness_set = NULL;
  cardano_derivation_path_t   key_path         = { CARDANO_CIP_1852_PURPOSE_STANDARD, CARDANO_CIP_1852_COIN_TYPE, 0U, 0U, 0U };

  error = cardano_secure_key_handler_bip32_sign_transaction(key_handler, transaction, &key_path, 1, &vkey_witness_set);

  if (error != CARDANO_SUCCESS)
  {
    cardano_secure_key_handler_unref(&key_handler);
    cardano_transaction_unref(&transaction);

    console_error("Error: %s", cardano_error_to_string(error));

    return EXIT_FAILURE;
  }

  cardano_vkey_witness_t* vkey_witness = NULL;

  error = cardano_vkey_witness_set_get(vkey_witness_set, 0, &vkey_witness);

  if (error != CARDANO_SUCCESS)
  {
    cardano_secure_key_handler_unref(&key_handler);
    cardano_transaction_unref(&transaction);
    cardano_vkey_witness_set_unref(&vkey_witness_set);

    console_error("Error: %s", cardano_error_to_string(error));
    return EXIT_FAILURE;
  }

  cardano_ed25519_signature_t*  signature = cardano_vkey_witness_get_signature(vkey_witness);
  cardano_ed25519_public_key_t* key       = cardano_vkey_witness_get_vkey(vkey_witness);

  const size_t sig_hex_size = cardano_ed25519_signature_get_hex_size(signature);
  char         sig_hex[sig_hex_size];

  error = cardano_ed25519_signature_to_hex(signature, sig_hex, sig_hex_size);

  if (error != CARDANO_SUCCESS)
  {
    cardano_secure_key_handler_unref(&key_handler);
    cardano_transaction_unref(&transaction);
    cardano_vkey_witness_set_unref(&vkey_witness_set);
    cardano_vkey_witness_unref(&vkey_witness);
    cardano_ed25519_signature_unref(&signature);
    cardano_ed25519_public_key_unref(&key);

    console_error("Error: %s", cardano_error_to_string(error));
    return EXIT_FAILURE;
  }

  cardano_ed25519_signature_unref(&signature);

  const size_t pub_key_hex_size = cardano_ed25519_public_key_get_hex_size(key);
  char         pub_key_hex[key_hex_size];

  error = cardano_ed25519_public_key_to_hex(key, pub_key_hex, pub_key_hex_size);

  cardano_ed25519_public_key_unref(&key);

  if (error != CARDANO_SUCCESS)
  {
    cardano_secure_key_handler_unref(&key_handler);
    cardano_transaction_unref(&transaction);
    cardano_vkey_witness_set_unref(&vkey_witness_set);
    cardano_vkey_witness_unref(&vkey_witness);

    console_error("Error: %s", cardano_error_to_string(error));
    return EXIT_FAILURE;
  }

  console_info("Signature: %s", sig_hex);
  console_info("Public key: %s", pub_key_hex);

  cardano_secure_key_handler_unref(&key_handler);
  cardano_transaction_unref(&transaction);
  cardano_vkey_witness_set_unref(&vkey_witness_set);
  cardano_vkey_witness_unref(&vkey_witness);

  return EXIT_SUCCESS;
}