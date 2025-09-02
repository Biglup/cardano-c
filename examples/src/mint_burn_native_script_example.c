/**
 * \file mint_burn_native_script_example.c
 *
 * \author angel.castillo
 * \date   Nov 16, 2024
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

static const char* EXAMPLE_CIP_25_METADATA = "{\n"
                                             "   \"b863bc7369f46136ac1048adb2fa7dae3af944c3bbb2be2f216a8d4f\": {\n"
                                             "      \"BerryOnyx\": {\n"
                                             "         \"color\": \"#0F0F0F\",\n"
                                             "         \"image\": \"ipfs://ipfs/QmS7w3Q5oVL9NE1gJnsMVPp6fcxia1e38cRT5pE5mmxawL\",\n"
                                             "         \"name\": \"Berry Onyx\"\n"
                                             "      },\n"
                                             "      \"BerryRaspberry\": {\n"
                                             "         \"color\": \"#E30B5D\",\n"
                                             "         \"image\": \"ipfs://ipfs/QmXjegt568JqSUpAz9phxbXq5noWE3AeymZMUP43Ej2DRZ\",\n"
                                             "         \"name\": \"Berry Raspberry\"\n"
                                             "      },\n"
                                             "   }\n"
                                             "}";

static const char* ALWAYS_SUCCEEDS_NATIVE_SCRIPT = "{\n"
                                                   "  \"type\": \"all\",\n"
                                                   "  \"scripts\":\n"
                                                   "  [\n"
                                                   "    {\n"
                                                   "      \"type\": \"before\",\n"
                                                   "      \"slot\": \"1001655683199\"\n" // Invalid after year 33658
                                                   "    }\n"
                                                   "  ]\n"
                                                   "}";

static const char*    SERIALIZED_BIP32_KEY_HANDLER = "0a0a0a0a01010000005c97db5e09b3a4919ec75ed1126056241a1e5278731c2e0b01bea0a5f42c22db4131e0a4bbe75633677eb0e60e2ecd3520178f85c7e0d4be77a449087fe9674ee52f946b07c1b56d228c496ec0d36dd44212ba8af0f6eed1a82194dd69f479c603";
static const uint64_t CONFIRM_TX_TIMEOUT_MS        = 240000U;
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
 * \brief Creates a \ref cardano_value_t object with the assets we are about to mint.
 *
 * \param[in] policy_id A pointer to a \ref cardano_blake2b_hash_t object representing the policy under which the assets are minted.
 * \param[in] onyx_name A pointer to a \ref cardano_asset_name_t object representing the name of the first asset (e.g., "BerryOnyx").
 * \param[in] raspberry_name A pointer to a \ref cardano_asset_name_t object representing the name of the second asset (e.g., "BerryRaspberry").
 *
 * \return A pointer to a newly created \ref cardano_value_t object representing the minting value for the specified assets.
 *         Returns \c NULL if the operation fails due to memory allocation or invalid parameters.
 *
 * \note The caller is responsible for managing the lifecycle of the returned \ref cardano_value_t object.
 *       Use \ref cardano_value_unref to free the object when it is no longer needed.
 */
static cardano_value_t*
create_mint_value(
  cardano_blake2b_hash_t* policy_id,
  cardano_asset_name_t*   onyx_name,
  cardano_asset_name_t*   raspberry_name)
{
  cardano_value_t* value = cardano_value_new_from_coin(2000000);

  cardano_error_t result = cardano_value_add_asset(value, policy_id, onyx_name, 1);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to add asset to value");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  result = cardano_value_add_asset(value, policy_id, raspberry_name, 1);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to add asset to value");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  return value;
}

/**
 * \brief Mints tokens using a specified funding address and an always-succeeds script.
 *
 * \param[in] provider A pointer to a \ref cardano_provider_t object used for accessing blockchain state.
 * \param[in] key_handler A pointer to a \ref cardano_secure_key_handler_t object for signing and key management.
 * \param[in] pparams A pointer to a \ref cardano_protocol_parameters_t object containing the protocol parameters required for transaction creation.
 * \param[in] funding_address A pointer to a \ref cardano_address_t object representing the address providing the funding for the minting operation.
 * \param[in] always_succeeds_script A pointer to a \ref cardano_script_t object representing the always-succeeds script governing the minted tokens.
 */
static void
mint_tokens(
  cardano_provider_t*            provider,
  cardano_secure_key_handler_t*  key_handler,
  cardano_protocol_parameters_t* pparams,
  cardano_address_t*             funding_address,
  cardano_script_t*              always_succeeds_script)
{
  cardano_blake2b_hash_t* policy_id          = cardano_script_get_hash(always_succeeds_script);
  const size_t            policy_id_hex_size = cardano_blake2b_hash_get_hex_size(policy_id);
  char                    policy_id_hex[65]  = { 0 };

  cardano_error_t result = cardano_blake2b_hash_to_hex(policy_id, policy_id_hex, policy_id_hex_size);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to convert policy ID to hex");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  cardano_asset_name_t* berry_onyx_name      = create_asset_name_from_string("BerryOnyx");
  cardano_asset_name_t* berry_raspberry_name = create_asset_name_from_string("BerryRaspberry");

  console_info("Minting tokens:");
  console_info("- %s.BerryOnyx+1", policy_id_hex);
  console_info("- %s.BerryRaspberry+1", policy_id_hex);

  cardano_utxo_list_t*  utxo_list  = get_unspent_utxos(provider, funding_address);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(pparams, provider);

  const uint64_t invalid_after = cardano_utils_get_time() + SECONDS_IN_TWO_HOURS;

  cardano_tx_builder_set_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_change_address(tx_builder, funding_address);
  cardano_tx_builder_set_invalid_after_ex(tx_builder, invalid_after);
  cardano_tx_builder_set_metadata_ex(tx_builder, 721, EXAMPLE_CIP_25_METADATA, strlen(EXAMPLE_CIP_25_METADATA));
  cardano_tx_builder_mint_token(tx_builder, policy_id, berry_onyx_name, 1, NULL);
  cardano_tx_builder_mint_token(tx_builder, policy_id, berry_raspberry_name, 1, NULL);
  cardano_tx_builder_add_script(tx_builder, always_succeeds_script);

  cardano_value_t* value = create_mint_value(policy_id, berry_onyx_name, berry_raspberry_name);
  cardano_tx_builder_send_value(tx_builder, funding_address, value);

  cardano_transaction_t* transaction = NULL;
  result                             = cardano_tx_builder_build(tx_builder, &transaction);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to build transaction");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_tx_builder_get_last_error(tx_builder));

    exit(result);
  }

  sign_transaction(key_handler, SIGNER_DERIVATION_PATH, transaction);
  submit_transaction(provider, CONFIRM_TX_TIMEOUT_MS, transaction);

  cardano_utxo_list_unref(&utxo_list);
  cardano_tx_builder_unref(&tx_builder);
  cardano_transaction_unref(&transaction);
  cardano_asset_name_unref(&berry_onyx_name);
  cardano_asset_name_unref(&berry_raspberry_name);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_value_unref(&value);

  console_info("Tokens minted successfully.");
}

/**
 * \brief Burns tokens using a specified funding address and an always-succeeds script.
 *
 * \param[in] provider A pointer to a \ref cardano_provider_t object used for accessing blockchain state.
 * \param[in] key_handler A pointer to a \ref cardano_secure_key_handler_t object for signing and key management.
 * \param[in] pparams A pointer to a \ref cardano_protocol_parameters_t object containing the protocol parameters required for transaction creation.
 * \param[in] funding_address A pointer to a \ref cardano_address_t object representing the address providing the funding for the burn operation.
 * \param[in] always_succeeds_script A pointer to a \ref cardano_script_t object representing the always-succeeds script governing the burning process.
 */
static void
burn_token(
  cardano_provider_t*            provider,
  cardano_secure_key_handler_t*  key_handler,
  cardano_protocol_parameters_t* pparams,
  cardano_address_t*             funding_address,
  cardano_script_t*              always_succeeds_script)
{
  cardano_blake2b_hash_t* policy_id          = cardano_script_get_hash(always_succeeds_script);
  const size_t            policy_id_hex_size = cardano_blake2b_hash_get_hex_size(policy_id);
  char                    policy_id_hex[65]  = { 0 };

  cardano_error_t result = cardano_blake2b_hash_to_hex(policy_id, policy_id_hex, policy_id_hex_size);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to convert policy ID to hex");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  cardano_asset_name_t* berry_onyx_name = create_asset_name_from_string("BerryOnyx");

  console_info("Burning token:");
  console_info("- %s.BerryOnyx-1", policy_id_hex);

  cardano_utxo_list_t*  utxo_list  = get_unspent_utxos(provider, funding_address);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(pparams, provider);

  const uint64_t invalid_after = cardano_utils_get_time() + SECONDS_IN_TWO_HOURS;

  cardano_tx_builder_set_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_change_address(tx_builder, funding_address);
  cardano_tx_builder_set_invalid_after_ex(tx_builder, invalid_after);
  cardano_tx_builder_mint_token(tx_builder, policy_id, berry_onyx_name, -1, NULL);
  cardano_tx_builder_add_script(tx_builder, always_succeeds_script);

  cardano_transaction_t* transaction = NULL;
  result                             = cardano_tx_builder_build(tx_builder, &transaction);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to build transaction");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_tx_builder_get_last_error(tx_builder));

    exit(result);
  }

  sign_transaction(key_handler, SIGNER_DERIVATION_PATH, transaction);
  submit_transaction(provider, CONFIRM_TX_TIMEOUT_MS, transaction);

  cardano_utxo_list_unref(&utxo_list);
  cardano_tx_builder_unref(&tx_builder);
  cardano_transaction_unref(&transaction);
  cardano_asset_name_unref(&berry_onyx_name);
  cardano_blake2b_hash_unref(&policy_id);

  console_info("Token burned successfully.");
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
  console_info("Mint & Burn with Native Scripts Example");
  console_info("libcardano-c:  V-%s\n", cardano_get_lib_version());

  console_info("This example mints two CIP-025 tokens and burn one afterwards.");

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
  cardano_utxo_list_t*           utxo_list              = get_unspent_utxos(provider, payment_address);

  // Mint two tokens: BerryOnyx and BerryRaspberry
  mint_tokens(provider, key_handler, protocol_params, payment_address, always_succeeds_script);
  // Example mint transaction created by this sample:
  // https://preprod.cardanoscan.io/transaction/ec4898f56d7e331ac92e443fa8cfb692986345f5bc8e6d276dd8716ee6023885

  // Burn one BerryOnyx token
  burn_token(provider, key_handler, protocol_params, payment_address, always_succeeds_script);
  // Example burn transaction created by this sample:
  // https://preprod.cardanoscan.io/transaction/ecf7cc500b53e98264b864121877576925fdf5565437319e6dd0acd1b91778ea

  // Cleanup
  cardano_script_unref(&always_succeeds_script);
  cardano_address_unref(&script_address);
  cardano_provider_unref(&provider);
  cardano_address_unref(&payment_address);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_secure_key_handler_unref(&key_handler);
  cardano_utxo_list_unref(&utxo_list);

  return EXIT_SUCCESS;
}
