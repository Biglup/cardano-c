/**
 * \file bip39_example.c
 *
 * \author angel.castillo
 * \date   Nov 23, 2024
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

static const char* MNEMONIC_WORDS[] = { "antenna", "whale", "clutch", "cushion", "narrow", "chronic", "matrix", "alarm", "raise", "much", "stove", "beach", "mimic", "daughter", "review", "build", "dinner", "twelve", "orbit", "soap", "decorate", "bachelor", "athlete", "close" };
static const char* PASSWORD         = "password";

static const cardano_account_derivation_path_t ACCOUNT_DERIVATION_PATH = {
  .purpose   = 1852U | 0x80000000,
  .coin_type = 1815U | 0x80000000,
  .account   = 0U
};

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Retrieves the password for the secure key handler.
 *
 * \param buffer The buffer where to write the password.
 * \param buffer_len The size of the buffer.
 *
 * \return The length of the password (or -1 on error).
 */
static int32_t
get_passphrase(byte_t* buffer, const size_t buffer_len)
{
  const int32_t password_len = (int32_t)strlen(PASSWORD);

  if (buffer_len < password_len)
  {
    return -1;
  }

  cardano_utils_safe_memcpy(buffer, buffer_len, &PASSWORD[0], password_len);

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
  console_info("Cardano BIP-039 Example");
  console_debug("libcardano-c:  V-%s", cardano_get_lib_version());

  console_info("\nThis example demonstrates how to create a software secure key handler from a mnemonic phrase.\n");

  byte_t entropy_bytes[64] = { 0 };
  size_t entropy_size      = 0;

  console_info("Converting mnemonic words to entropy...\n");
  cardano_error_t result = cardano_bip39_mnemonic_words_to_entropy(MNEMONIC_WORDS, 24, entropy_bytes, 64, &entropy_size);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to convert mnemonic words to entropy: %s", cardano_error_to_string(result));

    return EXIT_FAILURE;
  }

  cardano_secure_key_handler_t* key_handler = NULL;

  result = cardano_software_secure_key_handler_new(
    entropy_bytes,
    entropy_size,
    (const byte_t*)&PASSWORD[0],
    strlen(PASSWORD),
    &get_passphrase,
    &key_handler);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create secure key handler: %s\n", cardano_error_to_string(result));

    return EXIT_FAILURE;
  }

  console_info("Deriving address at: m / 44' / 1815' / 0' / 0 / 0 ...\n");

  cardano_bip32_public_key_t*       extended_public_key = NULL;
  cardano_account_derivation_path_t path                = { CARDANO_CIP_1852_PURPOSE_STANDARD, CARDANO_CIP_1852_COIN_TYPE, 0U };
  result                                                = cardano_secure_key_handler_bip32_get_extended_account_public_key(key_handler, path, &extended_public_key);

  if (result != CARDANO_SUCCESS)
  {
    cardano_secure_key_handler_unref(&key_handler);

    console_error("Error: %s", cardano_error_to_string(result));
    return EXIT_FAILURE;
  }

  cardano_address_t* payment_address = create_address_from_derivation_paths(key_handler, ACCOUNT_DERIVATION_PATH, 0, 0);

  cardano_bip32_public_key_unref(&extended_public_key);
  cardano_address_unref(&payment_address);
  cardano_secure_key_handler_unref(&key_handler);

  return EXIT_SUCCESS;
}