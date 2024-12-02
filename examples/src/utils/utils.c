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
static const char*    BURN_ADDRESS       = "addr_test1wza7ec20249sqg87yu2aqkqp735qa02q6yd93u28gzul93gvc4wuw"; // See https://www.cardano-tools.io/burn-address

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

void
sign_transaction_with_keys(
  cardano_secure_key_handler_t*    key_handler,
  const cardano_derivation_path_t* signer_derivation_path,
  const size_t                     signer_derivation_path_count,
  cardano_transaction_t*           transaction)
{
  cardano_vkey_witness_set_t* vkey = NULL;

  console_info("Requesting signature...");

  cardano_error_t result = cardano_secure_key_handler_bip32_sign_transaction(key_handler, transaction, signer_derivation_path, signer_derivation_path_count, &vkey);

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

cardano_drep_t*
create_drep_from_derivation_path(
  cardano_secure_key_handler_t*           key_handler,
  const cardano_account_derivation_path_t account_path)
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

  const uint32_t drep_derivation_path[] = {
    CARDANO_CIP_1852_ROLE_DREP,
    0
  };

  cardano_bip32_public_key_t*   drep_public_key = NULL;
  cardano_ed25519_public_key_t* drep_key        = NULL;

  result = cardano_bip32_public_key_derive(root_public_key, drep_derivation_path, 2U, &drep_public_key);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to derive DRep public key");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  result = cardano_bip32_public_key_to_ed25519_key(drep_public_key, &drep_key);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to convert DRep public key");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  cardano_credential_t* drep_cred = create_credential(drep_key);
  cardano_drep_t*       drep      = NULL;

  result = cardano_drep_new(CARDANO_DREP_TYPE_KEY_HASH, drep_cred, &drep);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create DRep address");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  cardano_bip32_public_key_unref(&root_public_key);
  cardano_bip32_public_key_unref(&drep_public_key);
  cardano_ed25519_public_key_unref(&drep_key);
  cardano_credential_unref(&drep_cred);

  char drep_string[256] = { 0 };

  result = cardano_drep_to_string(drep, drep_string, 256);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to convert DRep to string");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  console_set_foreground_color(CONSOLE_COLOR_GREEN);
  console_write("Computed address: %s\n\n", drep_string);
  console_reset_color();

  return drep;
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

cardano_utxo_t*
cardano_resolve_input(
  cardano_provider_t* provider,
  const char*         tx_id,
  const size_t        tx_id_size,
  const uint32_t      index)
{
  if ((tx_id == NULL) || (tx_id_size == 0U))
  {
    console_error("Failed to resolve input %s:%d", tx_id, index);
    exit(EXIT_FAILURE);
  }

  cardano_utxo_t*                  utxo   = NULL;
  cardano_utxo_list_t*             utxos  = NULL;
  cardano_transaction_input_t*     input  = NULL;
  cardano_blake2b_hash_t*          hash   = NULL;
  cardano_transaction_input_set_t* inputs = NULL;

  cardano_error_t result = cardano_blake2b_hash_from_hex(tx_id, tx_id_size, &hash);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create hash from hex");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  result = cardano_transaction_input_new(hash, index, &input);
  cardano_blake2b_hash_unref(&hash);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create transaction input");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  result = cardano_transaction_input_set_new(&inputs);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_input_unref(&input);

    console_error("Failed to create input set");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  result = cardano_transaction_input_set_add(inputs, input);
  cardano_transaction_input_unref(&input);

  if (result != CARDANO_SUCCESS)
  {
    cardano_transaction_input_set_unref(&inputs);

    console_error("Failed to add input to set");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  result = cardano_provider_resolve_unspent_outputs(provider, inputs, &utxos);
  cardano_transaction_input_set_unref(&inputs);

  if (result != CARDANO_SUCCESS)
  {
    cardano_utxo_list_unref(&utxos);

    console_error("Failed to resolve unspent outputs");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  const size_t utxo_count = cardano_utxo_list_get_length(utxos);

  if (utxo_count == 0U)
  {
    cardano_utxo_list_unref(&utxos);

    console_error("No unspent outputs found for input %s:%d", tx_id, index);
    exit(EXIT_FAILURE);
  }

  result = cardano_utxo_list_get(utxos, 0U, &utxo);
  cardano_utxo_list_unref(&utxos);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to get unspent output");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  return utxo;
}

cardano_script_t*
create_plutus_v2_script_from_hex(const char* script_hex)
{
  cardano_script_t*           script           = NULL;
  cardano_plutus_v2_script_t* plutus_v2_script = NULL;

  cardano_error_t result = cardano_plutus_v2_script_new_bytes_from_hex(script_hex, strlen(script_hex), &plutus_v2_script);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create script from hex");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  result = cardano_script_new_plutus_v2(plutus_v2_script, &script);
  cardano_plutus_v2_script_unref(&plutus_v2_script);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create script");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  return script;
}

cardano_script_t*
create_plutus_v3_script_from_hex(const char* script_hex)
{
  cardano_script_t*           script           = NULL;
  cardano_plutus_v3_script_t* plutus_v3_script = NULL;

  cardano_error_t result = cardano_plutus_v3_script_new_bytes_from_hex(script_hex, strlen(script_hex), &plutus_v3_script);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create script from hex");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  result = cardano_script_new_plutus_v3(plutus_v3_script, &script);
  cardano_plutus_v3_script_unref(&plutus_v3_script);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create script");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  return script;
}

cardano_script_t*
create_native_script_from_json(const char* json)
{
  cardano_script_t*        script        = NULL;
  cardano_native_script_t* native_script = NULL;

  cardano_error_t result = cardano_native_script_from_json(json, strlen(json), &native_script);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create script from JSON");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  result = cardano_script_new_native(native_script, &script);
  cardano_native_script_unref(&native_script);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create script");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  return script;
}

cardano_asset_name_t*
create_asset_name_from_string(const char* name)
{
  cardano_asset_name_t* asset_name = NULL;
  cardano_error_t       result     = cardano_asset_name_from_string(name, strlen(name), &asset_name);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create asset name");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  return asset_name;
}

cardano_address_t*
get_script_address(cardano_script_t* script)
{
  cardano_blake2b_hash_t* hash = cardano_script_get_hash(script);
  cardano_credential_t*   cred = NULL;

  cardano_error_t result = cardano_credential_new(hash, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, &cred);
  cardano_blake2b_hash_unref(&hash);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create credential");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  cardano_enterprise_address_t* enterprise_address = NULL;
  result                                           = cardano_enterprise_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, cred, &enterprise_address);
  cardano_credential_unref(&cred);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create enterprise address");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  cardano_address_t* address = cardano_enterprise_address_to_address(enterprise_address);
  cardano_enterprise_address_unref(&enterprise_address);

  return address;
}

cardano_reward_address_t*
get_script_stake_address(cardano_script_t* script)
{
  cardano_blake2b_hash_t* hash = cardano_script_get_hash(script);
  cardano_credential_t*   cred = NULL;

  cardano_error_t result = cardano_credential_new(hash, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, &cred);
  cardano_blake2b_hash_unref(&hash);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create credential");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  cardano_reward_address_t* reward_address = NULL;
  result                                   = cardano_reward_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, cred, &reward_address);
  cardano_credential_unref(&cred);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create enterprise address");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  return reward_address;
}

cardano_drep_t*
get_script_drep(cardano_script_t* script)
{
  cardano_blake2b_hash_t* hash = cardano_script_get_hash(script);
  cardano_credential_t*   cred = NULL;

  cardano_error_t result = cardano_credential_new(hash, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, &cred);
  cardano_blake2b_hash_unref(&hash);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create script credential");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  cardano_drep_t* drep = NULL;
  result               = cardano_drep_new(CARDANO_DREP_TYPE_SCRIPT_HASH, cred, &drep);
  cardano_credential_unref(&cred);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create DRep");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  return drep;
}

cardano_datum_t*
create_void_datum()
{
  cardano_datum_t*       datum       = NULL;
  cardano_plutus_data_t* plutus_data = create_void_plutus_data();

  cardano_error_t result = cardano_datum_new_inline_data(plutus_data, &datum);
  cardano_plutus_data_unref(&plutus_data);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create datum");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  return datum;
}

cardano_address_t*
get_brun_address()
{
  cardano_address_t* brun_address = NULL;
  cardano_error_t    result       = cardano_address_from_string(BURN_ADDRESS, strlen(BURN_ADDRESS), &brun_address);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create burn address");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  return brun_address;
}

cardano_transaction_output_t*
create_output_with_ref_script(cardano_address_t* address, const uint32_t amount, cardano_script_t* script)
{
  cardano_transaction_output_t* output = NULL;
  cardano_error_t               result = cardano_transaction_output_new(address, amount, &output);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create transaction output");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  result = cardano_transaction_output_set_script_ref(output, script);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to set script to transaction output");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  return output;
}

cardano_plutus_data_t*
create_void_plutus_data()
{
  static const char* VOID_DATA = "d87980";

  cardano_plutus_data_t* plutus_data = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(VOID_DATA, strlen(VOID_DATA));

  cardano_error_t result = cardano_plutus_data_from_cbor(reader, &plutus_data);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create plutus data");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  cardano_cbor_reader_unref(&reader);

  return plutus_data;
}

cardano_utxo_t*
create_utxo(cardano_blake2b_hash_t* tx_id, const uint32_t index, cardano_transaction_output_t* output)
{
  cardano_transaction_input_t* input = NULL;

  cardano_error_t result = cardano_transaction_input_new(tx_id, index, &input);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create transaction input");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  cardano_utxo_t* utxo = NULL;

  result = cardano_utxo_new(input, output, &utxo);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create utxo");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  cardano_transaction_input_unref(&input);

  return utxo;
}

cardano_utxo_t*
get_utxo_at_index(cardano_transaction_t* transaction, const uint32_t index)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  cardano_transaction_output_t* output = NULL;
  cardano_error_t               result = cardano_transaction_output_list_get(outputs, index, &output);
  cardano_transaction_output_unref(&output);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to get output at index %d", index);
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  cardano_blake2b_hash_t* tx_id = cardano_transaction_get_id(transaction);
  cardano_utxo_t*         utxo  = create_utxo(tx_id, index, output);

  cardano_blake2b_hash_unref(&tx_id);

  return utxo;
}

cardano_voter_t*
create_drep_voter(const char* drep_id)
{
  cardano_drep_t* drep = NULL;

  cardano_error_t result = cardano_drep_from_string(drep_id, strlen(drep_id), &drep);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to convert DRep ID to DRep");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  cardano_voter_t*      voter     = NULL;
  cardano_credential_t* drep_cred = NULL;

  result = cardano_drep_get_credential(drep, &drep_cred);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to get DRep credential");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  cardano_drep_type_t drep_type = CARDANO_DREP_TYPE_KEY_HASH;

  result = cardano_drep_get_type(drep, &drep_type);
  cardano_drep_unref(&drep);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to get DRep type");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  result = cardano_voter_new(
    CARDANO_DREP_TYPE_KEY_HASH ? CARDANO_VOTER_TYPE_DREP_KEY_HASH : CARDANO_VOTER_TYPE_DREP_SCRIPT_HASH, drep_cred, &voter);

  cardano_credential_unref(&drep_cred);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create DRep voter");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  return voter;
}

cardano_governance_action_id_t*
create_governance_id(const char* gov_id_hex, uint64_t index)
{
  cardano_governance_action_id_t* gov_id = NULL;

  cardano_error_t result = cardano_governance_action_id_from_hash_hex(gov_id_hex, strlen(gov_id_hex), index, &gov_id);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create governance action ID");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  return gov_id;
}
