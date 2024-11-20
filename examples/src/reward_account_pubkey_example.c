/**
 * \file reward_account_pubkey_example.c
 *
 * \author angel.castillo
 * \date   Nov 18, 2024
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

static const cardano_derivation_path_t SIGNER_STAKE_DERIVATION_PATH = {
  .purpose   = 1852U | 0x80000000,
  .coin_type = 1815U | 0x80000000,
  .account   = 0U,
  .role      = 2U,
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
 * \brief Registers a reward address and delegates it to a stake pool.
 *
 * This function performs the following actions:
 * 1. Registers a reward address on the blockchain if it is not already registered.
 * 2. Delegates the registered reward address to a specified stake pool.
 *
 * \param[in] provider A pointer to a \ref cardano_provider_t object used for accessing blockchain state.
 * \param[in] key_handler A pointer to a \ref cardano_secure_key_handler_t object for signing and key management.
 * \param[in] pparams A pointer to a \ref cardano_protocol_parameters_t object containing the protocol parameters required for transaction creation.
 * \param[in] funding_address A pointer to a \ref cardano_address_t object representing the funding source for transaction fees.
 * \param[in] reward_address A string representing the Bech32-encoded reward address to be registered and delegated.
 * \param[in] pool_id A string representing the Bech32-encoded pool ID to which the reward address will delegate its stake.
 */
static void
register_and_delegate(
  cardano_provider_t*            provider,
  cardano_secure_key_handler_t*  key_handler,
  cardano_protocol_parameters_t* pparams,
  cardano_address_t*             funding_address,
  const char*                    reward_address,
  const char*                    pool_id)
{
  cardano_utxo_list_t*  utxo_list  = get_unspent_utxos(provider, funding_address);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(pparams, provider);

  const uint64_t invalid_after = cardano_utils_get_time() + SECONDS_IN_TWO_HOURS;

  cardano_tx_builder_set_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_change_address(tx_builder, funding_address);
  cardano_tx_builder_set_invalid_after_ex(tx_builder, invalid_after);
  cardano_tx_builder_register_reward_address_ex(tx_builder, reward_address, strlen(reward_address), NULL);
  cardano_tx_builder_delegate_stake_ex(tx_builder, reward_address, strlen(reward_address), pool_id, strlen(pool_id), NULL);

  cardano_transaction_t* transaction = NULL;
  cardano_error_t        result      = cardano_tx_builder_build(tx_builder, &transaction);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to build transaction");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_tx_builder_get_last_error(tx_builder));

    exit(result);
  }

  const cardano_derivation_path_t KEYS[] = {
    SIGNER_DERIVATION_PATH,
    SIGNER_STAKE_DERIVATION_PATH
  };

  sign_transaction_with_keys(key_handler, &KEYS[0], sizeof(KEYS) / sizeof(cardano_derivation_path_t), transaction);
  submit_transaction(provider, CONFIRM_TX_TIMEOUT_MS, transaction);

  cardano_utxo_list_unref(&utxo_list);
  cardano_tx_builder_unref(&tx_builder);
  cardano_transaction_unref(&transaction);

  console_info("Stake key registered and delegated successfully.");
}

/**
 * \brief Unregisters a reward address and withdraws any rewards associated with it.
 *
 * This function performs the following actions:
 * 1. Withdraws any rewards available in the specified reward address.
 * 2. Unregisters the reward address from the blockchain.
 *
 * \param[in] provider A pointer to a \ref cardano_provider_t object used for accessing blockchain state.
 * \param[in] key_handler A pointer to a \ref cardano_secure_key_handler_t object for signing and key management.
 * \param[in] pparams A pointer to a \ref cardano_protocol_parameters_t object containing the protocol parameters required for transaction creation.
 * \param[in] funding_address A pointer to a \ref cardano_address_t object representing the funding source for transaction fees.
 * \param[in] reward_address A string representing the Bech32-encoded reward address to be unregistered and from which rewards will be withdrawn.
 */
static void
unregister_and_withdraw_rewards(
  cardano_provider_t*            provider,
  cardano_secure_key_handler_t*  key_handler,
  cardano_protocol_parameters_t* pparams,
  cardano_address_t*             funding_address,
  const char*                    reward_address)
{
  cardano_utxo_list_t*  utxo_list  = get_unspent_utxos(provider, funding_address);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(pparams, provider);

  const uint64_t invalid_after = cardano_utils_get_time() + SECONDS_IN_TWO_HOURS;

  cardano_tx_builder_set_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_change_address(tx_builder, funding_address);
  cardano_tx_builder_set_invalid_after_ex(tx_builder, invalid_after);
  // Withdraw 0 since we just register this account, so there are no rewards to withdraw
  cardano_tx_builder_withdraw_rewards_ex(tx_builder, reward_address, strlen(reward_address), 0, NULL);
  cardano_tx_builder_deregister_reward_address_ex(tx_builder, reward_address, strlen(reward_address), NULL);

  cardano_transaction_t* transaction = NULL;
  cardano_error_t        result      = cardano_tx_builder_build(tx_builder, &transaction);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to build transaction");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_tx_builder_get_last_error(tx_builder));

    exit(result);
  }

  const cardano_derivation_path_t KEYS[] = {
    SIGNER_DERIVATION_PATH,
    SIGNER_STAKE_DERIVATION_PATH
  };

  sign_transaction_with_keys(key_handler, &KEYS[0], sizeof(KEYS) / sizeof(cardano_derivation_path_t), transaction);

  submit_transaction(provider, CONFIRM_TX_TIMEOUT_MS, transaction);

  cardano_utxo_list_unref(&utxo_list);
  cardano_tx_builder_unref(&tx_builder);
  cardano_transaction_unref(&transaction);

  console_info("Stake key registered and delegated successfully.");
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
  console_info("Delegate and Withdraw Example (Pubkey Hash)");
  console_info("libcardano-c:  V-%s\n", cardano_get_lib_version());

  console_info("This example registers and delegates a stake key to a pool, and finally withdraws and deregisters it.");

  console_set_foreground_color(CONSOLE_COLOR_GREEN);
  console_write("\nUse passphrase: 'password'\n\n");
  console_reset_color();

  const char* api_key = getenv("BLOCKFROST_API_KEY");

  if (api_key == NULL)
  {
    console_error("BLOCKFROST_API_KEY environment variable is not set.\n");

    return EXIT_FAILURE;
  }

  cardano_secure_key_handler_t* key_handler     = create_secure_key_handler(SERIALIZED_BIP32_KEY_HANDLER, cardano_utils_safe_strlen(SERIALIZED_BIP32_KEY_HANDLER, 256), get_passphrase);
  cardano_provider_t*           provider        = create_provider(CARDANO_NETWORK_MAGIC_PREPROD, api_key);
  cardano_address_t*            payment_address = create_address_from_derivation_paths(key_handler, ACCOUNT_DERIVATION_PATH, PAYMENT_CRED_INDEX, STAKE_CRED_INDEX);

  const char* stake_address = "stake_test1uppy2gm2hqzkwc80em4mlat73j4jyqvzhclrvsu72g9xg4q2yweet";
  const char* pool_id       = "pool1pzdqdxrv0k74p4q33y98f2u7vzaz95et7mjeedjcfy0jcgk754f"; // SMAUG Pool

  cardano_protocol_parameters_t* protocol_params = get_protocol_parameters(provider);
  cardano_utxo_list_t*           utxo_list       = get_unspent_utxos(provider, payment_address);

  // Registers and delegates the stake key to the pool
  register_and_delegate(provider, key_handler, protocol_params, payment_address, stake_address, pool_id);
  // Example register & delegate transaction created by this sample:
  // https://preprod.cardanoscan.io/transaction/00193063d1fd409fa22359459b24f5ea1519ae20eaf33dcea5ea9f480a6b28cc

  // Unregisters and withdraws the rewards from the pool
  unregister_and_withdraw_rewards(provider, key_handler, protocol_params, payment_address, stake_address);
  // Example unregister & withdrawal transaction created by this sample:
  // https://preprod.cardanoscan.io/transaction/58c65c3093d2104fdf9fb1fce276892872289cffa4b7722ae7bfd77fbd312613

  // Cleanup
  cardano_provider_unref(&provider);
  cardano_address_unref(&payment_address);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_secure_key_handler_unref(&key_handler);
  cardano_utxo_list_unref(&utxo_list);

  return EXIT_SUCCESS;
}