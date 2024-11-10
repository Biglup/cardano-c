/**
 * \file utils.c
 *
 * \author luisd.bianchi
 * \date   Sep 30, 2024
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

#include "utils.h"
#include "cardano/cardano.h"
#include "cardano/key_handlers/software_secure_key_handler.h"
#include "provider_factory.h"

#include <cardano/export.h>

#include <assert.h>
#include <string.h>
#include <time.h>

#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>
#include <unistd.h>
#endif

/* CONSTANTS *****************************************************************/

static const uint64_t API_KEY_MAX_LENGTH = 39U;

/* DEFINITIONS ***************************************************************/

void
cardano_utils_safe_memcpy(void* dest, const size_t dest_size, const void* src, const size_t src_size)
{
  if ((dest == NULL) || (src == NULL) || (dest_size == 0U) || (src_size == 0U))
  {
    return;
  }

  size_t copy_size = (src_size < dest_size) ? src_size : dest_size;

  CARDANO_UNUSED(memcpy(dest, src, copy_size)); // nosemgrep
}

size_t
cardano_utils_safe_strlen(const char* str, const size_t max_length)
{
  if (str == NULL)
  {
    return 0U;
  }

  size_t index = 0U;

  while ((index < max_length) && (str[index] != '\0'))
  {
    ++index;
  }

  return index;
}

void
cardano_utils_set_error_message(cardano_provider_impl_t* provider_impl, const char* message)
{
  assert(provider_impl != NULL);

  size_t message_size = cardano_utils_safe_strlen(message, 1023);

  CARDANO_UNUSED(cardano_utils_safe_memcpy(provider_impl->error_message, 1024, message, message_size));
  provider_impl->error_message[message_size] = '\0';
}

uint64_t
cardano_utils_get_time()
{
  return (uint64_t)time(NULL);
}

uint64_t
cardano_utils_get_elapsed_time_since(const uint64_t start)
{
  const uint64_t current_time = cardano_utils_get_time();

  return ((current_time >= start) ? (current_time - start) : 0U);
}

void
cardano_utils_sleep(const uint64_t milliseconds)
{
#ifdef _WIN32
  Sleep((DWORD)milliseconds); // Windows Sleep function takes milliseconds
#else
  usleep(milliseconds * 1000U); // POSIX usleep takes microseconds
#endif
}

void
print_hash(const char* message, cardano_blake2b_hash_t* hash)
{
  char hash_hex[256] = { 0 };

  cardano_error_t result = cardano_blake2b_hash_to_hex(hash, hash_hex, 256);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to convert hash to hex");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    return;
  }

  console_set_foreground_color(CONSOLE_COLOR_GREEN);
  console_write("\n%s: %s\n\n", message, hash_hex);
  console_reset_color();
}

cardano_provider_t*
create_provider(const uint32_t network_magic, const char* api_key)
{
  cardano_provider_t* provider = NULL;

  cardano_error_t result = create_blockfrost_provider(network_magic, api_key, cardano_utils_safe_strlen(api_key, API_KEY_MAX_LENGTH), &provider);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create provider");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  return provider;
}

void
submit_transaction(cardano_provider_t* provider, const uint64_t timeout_ms, cardano_transaction_t* transaction)
{
  console_info("Submitting transaction...");
  cardano_blake2b_hash_t* tx_id = NULL;

  cardano_error_t result = cardano_provider_submit_transaction(provider, transaction, &tx_id);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to submit transaction");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_provider_get_last_error(provider));

    exit(result);
  }

  print_hash("Transaction submitted", tx_id);

  bool confirmed = false;

  console_info("Waiting for transaction confirmation...");
  result = cardano_provider_confirm_transaction(provider, tx_id, timeout_ms, &confirmed);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to confirm transaction");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_provider_get_last_error(provider));

    exit(result);
  }

  if (confirmed)
  {
    print_hash("Transaction confirmed", tx_id);
  }
  else
  {
    console_error("Transaction not confirmed");
  }

  cardano_blake2b_hash_unref(&tx_id);
}

void
sign_transaction(
  cardano_secure_key_handler_t*   key_handler,
  const cardano_derivation_path_t signer_derivation_path,
  cardano_transaction_t*          transaction)
{
  cardano_vkey_witness_set_t* vkey = NULL;

  console_info("Requesting signature...");

  cardano_error_t result = cardano_secure_key_handler_bip32_sign_transaction(key_handler, transaction, &signer_derivation_path, 1, &vkey);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to sign transaction");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_secure_key_handler_get_last_error(key_handler));

    exit(result);
  }

  result = cardano_transaction_apply_vkey_witnesses(transaction, vkey);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to apply vkey witnesses to transaction");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_transaction_get_last_error(transaction));

    exit(result);
  }

  cardano_vkey_witness_set_unref(&vkey);
}

cardano_protocol_parameters_t*
get_protocol_parameters(cardano_provider_t* provider)
{
  cardano_protocol_parameters_t* pparams = NULL;

  cardano_error_t error = cardano_provider_get_parameters(provider, &pparams);

  if (error != CARDANO_SUCCESS)
  {
    console_error("A error ocurred while getting protocol parameters from provider");
    console_error("Error [%d]: %s", error, cardano_error_to_string(error));
    console_error("%s", cardano_provider_get_last_error(provider));

    exit(error);
  }

  return pparams;
}

cardano_utxo_list_t*
get_unspent_utxos(cardano_provider_t* provider, cardano_address_t* address)
{
  cardano_utxo_list_t* utxo_list = NULL;

  cardano_error_t error = cardano_provider_get_unspent_outputs(provider, address, &utxo_list);

  if (error != CARDANO_SUCCESS)
  {
    console_error("A error ocurred while getting unspent outputs from provider");
    console_error("Error [%d]: %s", error, cardano_error_to_string(error));
    console_error("%s", cardano_provider_get_last_error(provider));

    exit(error);
  }

  return utxo_list;
}

cardano_credential_t*
create_credential(cardano_ed25519_public_key_t* public_key)
{
  cardano_blake2b_hash_t* hash = NULL;

  cardano_error_t result = cardano_ed25519_public_key_to_hash(public_key, &hash);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to hash public key");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  cardano_credential_t* credential = NULL;

  result = cardano_credential_new(hash, CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create credential");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  cardano_blake2b_hash_unref(&hash);

  return credential;
}

cardano_reward_address_t*
create_reward_address(const char* address_str, const size_t address_str_length)
{
  cardano_reward_address_t* reward_address = NULL;
  cardano_error_t           result         = cardano_reward_address_from_bech32(address_str, address_str_length, &reward_address);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create reward address: %s", cardano_error_to_string(result));
    return NULL;
  }

  return reward_address;
}

cardano_address_t*
create_address_from_derivation_paths(
  cardano_secure_key_handler_t*           key_handler,
  const cardano_account_derivation_path_t account_path,
  const uint32_t                          payment_index,
  const uint32_t                          stake_key_index)
{
  console_info("Requesting account root public key...");

  cardano_bip32_public_key_t* root_public_key = NULL;

  cardano_error_t result = cardano_secure_key_handler_bip32_get_extended_account_public_key(key_handler, account_path, &root_public_key);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to get account root public key");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  const uint32_t payment_key_derivation_path[] = {
    CARDANO_CIP_1852_ROLE_EXTERNAL,
    payment_index
  };

  const uint32_t stake_key_derivation_path[] = {
    CARDANO_CIP_1852_ROLE_STAKING,
    stake_key_index
  };

  cardano_bip32_public_key_t*   payment_public_key = NULL;
  cardano_bip32_public_key_t*   stake_public_key   = NULL;
  cardano_ed25519_public_key_t* payment_key        = NULL;
  cardano_ed25519_public_key_t* stake_key          = NULL;

  result = cardano_bip32_public_key_derive(root_public_key, payment_key_derivation_path, 2U, &payment_public_key);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to derive payment public key");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  result = cardano_bip32_public_key_derive(root_public_key, stake_key_derivation_path, 2U, &stake_public_key);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to derive stake public key");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  result = cardano_bip32_public_key_to_ed25519_key(payment_public_key, &payment_key);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to convert payment public key");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  result = cardano_bip32_public_key_to_ed25519_key(stake_public_key, &stake_key);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to convert stake public key");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  cardano_base_address_t* base_address = NULL;

  cardano_credential_t* payment_cred = create_credential(payment_key);
  cardano_credential_t* stake_cred   = create_credential(stake_key);

  result = cardano_base_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, payment_cred, stake_cred, &base_address);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create payment address");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  cardano_address_t* address = cardano_base_address_to_address(base_address);

  cardano_bip32_public_key_unref(&root_public_key);
  cardano_bip32_public_key_unref(&payment_public_key);
  cardano_bip32_public_key_unref(&stake_public_key);
  cardano_credential_unref(&payment_cred);
  cardano_credential_unref(&stake_cred);
  cardano_base_address_unref(&base_address);
  cardano_ed25519_public_key_unref(&payment_key);
  cardano_ed25519_public_key_unref(&stake_key);

  console_set_foreground_color(CONSOLE_COLOR_GREEN);
  console_write("Computed address: %s\n\n", cardano_address_get_string(address));
  console_reset_color();

  return address;
}

cardano_secure_key_handler_t*
create_secure_key_handler(const char* serialized_data, const size_t length, cardano_get_passphrase_func_t get_passphrase)
{
  cardano_buffer_t*             serialized_key_handler = cardano_buffer_from_hex(serialized_data, length);
  cardano_secure_key_handler_t* key_handler            = NULL;

  cardano_error_t error = cardano_software_secure_key_handler_deserialize(
    cardano_buffer_get_data(serialized_key_handler),
    cardano_buffer_get_size(serialized_key_handler),
    get_passphrase,
    &key_handler);

  cardano_buffer_unref(&serialized_key_handler);

  if (error != CARDANO_SUCCESS)
  {
    console_error("Failed to create secure key handler");
    console_error("Error [%d]: %s", error, cardano_error_to_string(error));

    exit(error);
  }

  return key_handler;
}