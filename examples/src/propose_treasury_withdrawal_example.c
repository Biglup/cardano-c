/**
 * \file propose_treasury_withdrawal_example.c
 *
 * \author angel.castillo
 * \date   Dec 1, 2024
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

// This metadata was taken from the 'Intersect Hard Fork Working Group - Rename the
// Chang 2 Hard Fork to the Plomin Hard Fork' as an example for metadata.
static const char*    METADATA_URL                 = "https://raw.githubusercontent.com/IntersectMBO/governance-actions/refs/heads/main/mainnet/2024-11-19-infohf/metadata.jsonld";
static const char*    METADATA_HASH                = "93106d082a93e94df5aff74f678438bae3a647dac63465fbfcde6a3058f41a1e";
static const char*    STAKE_ADDRESS                = "stake_test1uppy2gm2hqzkwc80em4mlat73j4jyqvzhclrvsu72g9xg4q2yweet";
static const char*    CONSTITUTION_SCRIPT_HASH     = "fa24fb305126805cf2164c161d852a0e7330cf988f1fe558cf7d4a64";
static const char*    SERIALIZED_BIP32_KEY_HANDLER = "0a0a0a0a01010000005c97db5e09b3a4919ec75ed1126056241a1e5278731c2e0b01bea0a5f42c22db4131e0a4bbe75633677eb0e60e2ecd3520178f85c7e0d4be77a449087fe9674ee52f946b07c1b56d228c496ec0d36dd44212ba8af0f6eed1a82194dd69f479c603";
static const uint64_t CONFIRM_TX_TIMEOUT_MS        = 240000U;
static const int64_t  WITHDRAWAL_AMOUNT            = 1000000000000U;
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
 * \brief Builds a withdrawal map with a single entry. Requesting a withdrawal of `WITHDRAWAL_AMOUNT` to the `STAKE_ADDRESS`.
 *
 * \return Returns a pointer to the withdrawal map.
 */
static cardano_withdrawal_map_t*
build_withdrawal_map(void)
{
  cardano_withdrawal_map_t* withdrawal_map = NULL;
  cardano_error_t           result         = cardano_withdrawal_map_new(&withdrawal_map);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create withdrawal map");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_withdrawal_map_get_last_error(withdrawal_map));

    exit(result);
  }

  result = cardano_withdrawal_map_insert_ex(withdrawal_map, STAKE_ADDRESS, strlen(STAKE_ADDRESS), WITHDRAWAL_AMOUNT);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to insert withdrawal entry");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_withdrawal_map_get_last_error(withdrawal_map));

    exit(result);
  }

  return withdrawal_map;
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
  console_info("Propose withdrawal Example");
  console_info("libcardano-c:  V-%s\n", cardano_get_lib_version());

  console_info("This example will issue a withdrawal proposal to withdraw from treasury %d to %s.", WITHDRAWAL_AMOUNT, STAKE_ADDRESS);

  console_set_foreground_color(CONSOLE_COLOR_GREEN);
  console_write("\nUse passphrase: 'password'\n\n");
  console_reset_color();

  const char* api_key = getenv("BLOCKFROST_API_KEY");

  if (api_key == NULL)
  {
    console_error("BLOCKFROST_API_KEY environment variable is not set.\n");

    return EXIT_FAILURE;
  }

  // 0.- Initialize dependencies
  cardano_secure_key_handler_t*  key_handler     = create_secure_key_handler(SERIALIZED_BIP32_KEY_HANDLER, cardano_utils_safe_strlen(SERIALIZED_BIP32_KEY_HANDLER, 256), get_passphrase);
  cardano_provider_t*            provider        = create_provider(CARDANO_NETWORK_MAGIC_PREPROD, api_key);
  cardano_address_t*             payment_address = create_address_from_derivation_paths(key_handler, ACCOUNT_DERIVATION_PATH, PAYMENT_CRED_INDEX, STAKE_CRED_INDEX);
  cardano_utxo_list_t*           utxo_list       = get_unspent_utxos(provider, payment_address);
  cardano_protocol_parameters_t* protocol_params = get_protocol_parameters(provider);
  cardano_withdrawal_map_t*      withdrawal_map  = build_withdrawal_map();

  // https://book.world.dev.cardano.org/env-preprod.html
  // 2 hours from now in UNIX time (seconds)
  const uint64_t invalid_after = cardano_utils_get_time() + SECONDS_IN_TWO_HOURS;

  // 1.- Build transaction
  console_info("Building transaction...");

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(protocol_params, provider);

  cardano_tx_builder_set_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_change_address(tx_builder, payment_address);
  cardano_tx_builder_set_invalid_after_ex(tx_builder, invalid_after);

  // Withdrawal proposals requires the execution of the constitution guardrail script, so we need to
  // set the both UTXOs and collateral change address.
  cardano_tx_builder_set_collateral_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_collateral_change_address(tx_builder, payment_address);

  // We also need to provider the constitution script, either by including it directly
  // in the witness set or by including a reference input with contains it. Currently (as of epoch 163), the
  // script is deployed at UTXO: 9aabbac24d1e39cb3e677981c84998a4210bae8d56b0f60908eedb9f59efffc8#0
  cardano_utxo_t* reference_utxo = cardano_resolve_input(provider, "9aabbac24d1e39cb3e677981c84998a4210bae8d56b0f60908eedb9f59efffc8", 64, 0);

  cardano_tx_builder_add_reference_input(tx_builder, reference_utxo);

  // Si extension function allows you to pass most of the parameters as C strings for convenience in cases
  // such as this, but you also have another overload `cardano_tx_builder_propose_treasury_withdrawals` which
  // takes the objects rather than their string representations.
  cardano_tx_builder_propose_treasury_withdrawals_ex(
    tx_builder,
    // The stake key must be registered or the transaction will fail.
    STAKE_ADDRESS,
    strlen(STAKE_ADDRESS),
    // We also need to add the anchor with the metadata (URL and hash).
    METADATA_URL,
    strlen(METADATA_URL),
    METADATA_HASH,
    strlen(METADATA_HASH),
    // The constitution script hash as of epoch 163.
    CONSTITUTION_SCRIPT_HASH,
    strlen(CONSTITUTION_SCRIPT_HASH),
    // Map with all withdrawals requested, this example requests to a single address.
    withdrawal_map);

  cardano_transaction_t* transaction = NULL;
  cardano_error_t        result      = cardano_tx_builder_build(tx_builder, &transaction);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to build transaction");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_tx_builder_get_last_error(tx_builder));

    return result;
  }

  // 2.- Sign transaction
  sign_transaction(key_handler, SIGNER_DERIVATION_PATH, transaction);

  // 3.- Submit transaction & confirm
  submit_transaction(provider, CONFIRM_TX_TIMEOUT_MS, transaction);

  // Example withdrawal proposal transaction created by this sample:
  // https://preprod.cardanoscan.io/transaction/372d688faa77e146798b581b322c0f2981a9023764736ade5d12e0e4e796af8c
  // Which created the following proposal:
  // https://preprod.cardanoscan.io/govAction/gov_action1xukk3ra2wls5v7vttqdnytq09xq6jq3hv3ek4hjaztswfeuk47xqqg4644z

  // Cleanup
  cardano_provider_unref(&provider);
  cardano_address_unref(&payment_address);
  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_tx_builder_unref(&tx_builder);
  cardano_transaction_unref(&transaction);
  cardano_secure_key_handler_unref(&key_handler);
  cardano_withdrawal_map_unref(&withdrawal_map);
  cardano_utxo_unref(&reference_utxo);

  return EXIT_SUCCESS;
}
