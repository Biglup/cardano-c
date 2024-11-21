/**
 * \file spend_from_native_script_example.c
 *
 * \author angel.castillo
 * \date   Nov 13, 2024
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

#include "providers/provider_factory.h"

#include "utils/console.h"
#include "utils/utils.h"
#include <cardano/cardano.h>

#include <stdlib.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const char* ALWAYS_SUCCEEDS_NATIVE_SCRIPT = "{\n"
                                                   "  \"type\": \"all\",\n"
                                                   "  \"scripts\":\n"
                                                   "  [\n"
                                                   "    {\n"
                                                   "      \"type\": \"after\",\n"
                                                   "      \"slot\": \"1001655683199\"\n" // Invalid after year 33658
                                                   "    }\n"
                                                   "  ]\n"
                                                   "}";
static const char*    SERIALIZED_BIP32_KEY_HANDLER = "0a0a0a0a01010000005c97db5e09b3a4919ec75ed1126056241a1e5278731c2e0b01bea0a5f42c22db4131e0a4bbe75633677eb0e60e2ecd3520178f85c7e0d4be77a449087fe9674ee52f946b07c1b56d228c496ec0d36dd44212ba8af0f6eed1a82194dd69f479c603";
static const uint64_t CONFIRM_TX_TIMEOUT_MS        = 240000U;
static const int64_t  LOVELACE_TO_SEND             = 2000000U;
static const uint64_t PAYMENT_CRED_INDEX           = 0U;
static const uint64_t STAKE_CRED_INDEX             = 0U;
static const uint64_t SECONDS_IN_TWO_HOURS         = 60UL * 60UL * 2UL;

static const cardano_account_derivation_path_t ACCOUNT_DERIVATION_PATH = {
  .purpose   = 1852U | 0x80000000,
  .coin_type = 1815U | 0x80000000,
  .account   = 0U
};

static const cardano_derivation_path_t SIGNER_DERIVATION_PATH = {
  .purpose   = 1852U | 0x80000000,
  .coin_type = 1815U | 0x80000000,
  .account   = 0U,
  .role      = 0U,
  .index     = 0U
};

/* DECLARATIONS **************************************************************/

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
  console_warn("Enter passphrase: ");
  char          password[128] = { 0 };
  const int32_t password_len  = console_read_password(password, sizeof(password));

  if (buffer_len < password_len)
  {
    return -1;
  }

  cardano_utils_safe_memcpy(buffer, buffer_len, &password[0], password_len);

  // Clear local password from memory
  cardano_memzero(password, sizeof(password));

  return password_len;
}

/**
 * \brief Funds a script address with the specified amount.
 *
 * This function creates and submits a transaction that transfers the specified amount from the
 * `funding_address` to the `script_address`. It leverages the provided `provider` to interact
 * with the Cardano network, and uses the `key_handler` to sign the transaction.
 *
 * \param[in] provider The Cardano provider object for network access.
 * \param[in] key_handler The secure key handler object for signing the transaction.
 * \param[in] pparams The protocol parameters object required for transaction building and fee calculations.
 * \param[in] funding_address The address from which funds are withdrawn to fund the script address.
 * \param[in] script_address The destination script address to receive the specified amount.
 * \param[in] amount The amount of lovelace to fund the script address.
 *
 * \return A pointer to the \ref cardano_utxo_t object created at the script address if successful.
 *         Returns NULL if the funding transaction fails.
 */
static cardano_utxo_t*
fund_script_address(
  cardano_provider_t*            provider,
  cardano_secure_key_handler_t*  key_handler,
  cardano_protocol_parameters_t* pparams,
  cardano_address_t*             funding_address,
  cardano_address_t*             script_address,
  const uint32_t                 amount)
{
  console_info("Funding script address: %s", cardano_address_get_string(script_address));

  cardano_utxo_list_t*  utxo_list  = get_unspent_utxos(provider, funding_address);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(pparams, provider);

  const uint64_t invalid_after = cardano_utils_get_time() + SECONDS_IN_TWO_HOURS;

  cardano_tx_builder_set_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_change_address(tx_builder, funding_address);
  cardano_tx_builder_set_invalid_after_ex(tx_builder, invalid_after);
  cardano_tx_builder_lock_lovelace(tx_builder, script_address, amount, NULL);

  cardano_transaction_t* transaction = NULL;
  cardano_error_t        result      = cardano_tx_builder_build(tx_builder, &transaction);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to build transaction");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_tx_builder_get_last_error(tx_builder));

    exit(result);
  }

  sign_transaction(key_handler, SIGNER_DERIVATION_PATH, transaction);
  submit_transaction(provider, CONFIRM_TX_TIMEOUT_MS, transaction);

  cardano_utxo_t* utxo = get_utxo_at_index(transaction, 0);

  cardano_utxo_list_unref(&utxo_list);
  cardano_tx_builder_unref(&tx_builder);
  cardano_transaction_unref(&transaction);

  console_info("Script address funded successfully.");

  return utxo;
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
  console_info("Spend from Native Script Example");
  console_info("libcardano-c:  V-%s\n", cardano_get_lib_version());

  console_info("This example will spend balance from a native script.");

  console_set_foreground_color(CONSOLE_COLOR_GREEN);
  console_write("\nUse passphrase: 'password'\n\n");
  console_reset_color();

  const char* api_key = getenv("BLOCKFROST_API_KEY");

  if (api_key == NULL)
  {
    console_error("BLOCKFROST_API_KEY environment variable is not set.\n");

    return EXIT_FAILURE;
  }

  cardano_script_t*              always_succeeds_script = create_native_script_from_json(ALWAYS_SUCCEEDS_NATIVE_SCRIPT);
  cardano_address_t*             script_address         = get_script_address(always_succeeds_script);
  cardano_secure_key_handler_t*  key_handler            = create_secure_key_handler(SERIALIZED_BIP32_KEY_HANDLER, cardano_utils_safe_strlen(SERIALIZED_BIP32_KEY_HANDLER, 256), get_passphrase);
  cardano_provider_t*            provider               = create_provider(CARDANO_NETWORK_MAGIC_PREPROD, api_key);
  cardano_address_t*             payment_address        = create_address_from_derivation_paths(key_handler, ACCOUNT_DERIVATION_PATH, PAYMENT_CRED_INDEX, STAKE_CRED_INDEX);
  cardano_protocol_parameters_t* protocol_params        = get_protocol_parameters(provider);

  cardano_utxo_t* script_utxo = fund_script_address(provider, key_handler, protocol_params, payment_address, script_address, LOVELACE_TO_SEND);

  cardano_utxo_list_t* script_utxo_list = get_unspent_utxos(provider, script_address);
  cardano_utxo_list_t* utxo_list        = get_unspent_utxos(provider, payment_address);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(protocol_params, provider);

  const uint64_t invalid_after = cardano_utils_get_time() + SECONDS_IN_TWO_HOURS;

  cardano_tx_builder_set_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_change_address(tx_builder, payment_address);
  cardano_tx_builder_set_invalid_after_ex(tx_builder, invalid_after);
  cardano_tx_builder_add_script(tx_builder, always_succeeds_script);
  cardano_tx_builder_send_lovelace(tx_builder, payment_address, 1000000);
  cardano_tx_builder_add_input(tx_builder, script_utxo, NULL, NULL);

  cardano_transaction_t* transaction = NULL;

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &transaction);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to build transaction");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_tx_builder_get_last_error(tx_builder));

    exit(result);
  }

  sign_transaction(key_handler, SIGNER_DERIVATION_PATH, transaction);
  submit_transaction(provider, CONFIRM_TX_TIMEOUT_MS, transaction);

  // Example transaction created by this sample:
  // https://preprod.cardanoscan.io/transaction/efaee69371127b78d4c7dea06f8ef5de695d87c224a5951eaaf06a1f65d57e7f

  // Cleanup
  cardano_script_unref(&always_succeeds_script);
  cardano_address_unref(&script_address);
  cardano_provider_unref(&provider);
  cardano_address_unref(&payment_address);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_secure_key_handler_unref(&key_handler);
  cardano_utxo_list_unref(&utxo_list);
  cardano_utxo_list_unref(&script_utxo_list);
  cardano_tx_builder_unref(&tx_builder);
  cardano_transaction_unref(&transaction);
  cardano_utxo_unref(&script_utxo);

  return EXIT_SUCCESS;
}
