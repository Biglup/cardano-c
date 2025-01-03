/**
 * \file reward_account_script_example.c
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

static const char*    ALWAYS_SUCCEEDS_SCRIPT_V2    = "59079201000033232323232323232323232323232332232323232323232222232325335333006300800530070043333573466E1CD55CEA80124000466442466002006004646464646464646464646464646666AE68CDC39AAB9D500C480008CCCCCCCCCCCC88888888888848CCCCCCCCCCCC00403403002C02802402001C01801401000C008CD4060064D5D0A80619A80C00C9ABA1500B33501801A35742A014666AA038EB9406CD5D0A804999AA80E3AE501B35742A01066A0300466AE85401CCCD54070091D69ABA150063232323333573466E1CD55CEA801240004664424660020060046464646666AE68CDC39AAB9D5002480008CC8848CC00400C008CD40B9D69ABA15002302F357426AE8940088C98C80C8CD5CE01981901809AAB9E5001137540026AE854008C8C8C8CCCD5CD19B8735573AA004900011991091980080180119A8173AD35742A004605E6AE84D5D1280111931901919AB9C033032030135573CA00226EA8004D5D09ABA2500223263202E33573805E05C05826AAE7940044DD50009ABA1500533501875C6AE854010CCD540700808004D5D0A801999AA80E3AE200135742A00460446AE84D5D1280111931901519AB9C02B02A028135744A00226AE8940044D5D1280089ABA25001135744A00226AE8940044D5D1280089ABA25001135744A00226AE8940044D55CF280089BAA00135742A00460246AE84D5D1280111931900E19AB9C01D01C01A101B13263201B3357389201035054350001B135573CA00226EA80054049404448C88C008DD6000990009AA80A911999AAB9F0012500A233500930043574200460066AE880080548C8C8CCCD5CD19B8735573AA004900011991091980080180118061ABA150023005357426AE8940088C98C8054CD5CE00B00A80989AAB9E5001137540024646464646666AE68CDC39AAB9D5004480008CCCC888848CCCC00401401000C008C8C8C8CCCD5CD19B8735573AA0049000119910919800801801180A9ABA1500233500F014357426AE8940088C98C8068CD5CE00D80D00C09AAB9E5001137540026AE854010CCD54021D728039ABA150033232323333573466E1D4005200423212223002004357426AAE79400C8CCCD5CD19B875002480088C84888C004010DD71ABA135573CA00846666AE68CDC3A801A400042444006464C6403866AE700740700680640604D55CEA80089BAA00135742A00466A016EB8D5D09ABA2500223263201633573802E02C02826AE8940044D5D1280089AAB9E500113754002266AA002EB9D6889119118011BAB00132001355012223233335573E0044A010466A00E66442466002006004600C6AAE754008C014D55CF280118021ABA200301313574200222440042442446600200800624464646666AE68CDC3A800A40004642446004006600A6AE84D55CF280191999AB9A3370EA0049001109100091931900899AB9C01201100F00E135573AA00226EA80048C8C8CCCD5CD19B875001480188C848888C010014C01CD5D09AAB9E500323333573466E1D400920042321222230020053009357426AAE7940108CCCD5CD19B875003480088C848888C004014C01CD5D09AAB9E500523333573466E1D40112000232122223003005375C6AE84D55CF280311931900899AB9C01201100F00E00D00C135573AA00226EA80048C8C8CCCD5CD19B8735573AA004900011991091980080180118029ABA15002375A6AE84D5D1280111931900699AB9C00E00D00B135573CA00226EA80048C8CCCD5CD19B8735573AA002900011BAE357426AAE7940088C98C802CCD5CE00600580489BAA001232323232323333573466E1D4005200C21222222200323333573466E1D4009200A21222222200423333573466E1D400D2008233221222222233001009008375C6AE854014DD69ABA135744A00A46666AE68CDC3A8022400C4664424444444660040120106EB8D5D0A8039BAE357426AE89401C8CCCD5CD19B875005480108CC8848888888CC018024020C030D5D0A8049BAE357426AE8940248CCCD5CD19B875006480088C848888888C01C020C034D5D09AAB9E500B23333573466E1D401D2000232122222223005008300E357426AAE7940308C98C8050CD5CE00A80A00900880800780700680609AAB9D5004135573CA00626AAE7940084D55CF280089BAA0012323232323333573466E1D400520022333222122333001005004003375A6AE854010DD69ABA15003375A6AE84D5D1280191999AB9A3370EA0049000119091180100198041ABA135573CA00C464C6401A66AE7003803402C0284D55CEA80189ABA25001135573CA00226EA80048C8C8CCCD5CD19B875001480088C8488C00400CDD71ABA135573CA00646666AE68CDC3A8012400046424460040066EB8D5D09AAB9E500423263200A33573801601401000E26AAE7540044DD500089119191999AB9A3370EA00290021091100091999AB9A3370EA00490011190911180180218031ABA135573CA00846666AE68CDC3A801A400042444004464C6401666AE7003002C02402001C4D55CEA80089BAA0012323333573466E1D40052002212200223333573466E1D40092000212200123263200733573801000E00A00826AAE74DD5000891999AB9A3370E6AAE74DD5000A40004008464C6400866AE700140100092612001490103505431001123230010012233003300200200122212200201";
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
 * \brief Registers a reward address and delegates it to a specified stake pool.
 *
 * This function performs two operations:
 * 1. Registers a reward address on the blockchain.
 * 2. Delegates the registered reward address to a specified stake pool using the provided stake pool ID.
 *
 * \param[in] provider A pointer to a \ref cardano_provider_t object used for accessing blockchain state.
 * \param[in] key_handler A pointer to a \ref cardano_secure_key_handler_t object for signing and key management.
 * \param[in] pparams A pointer to a \ref cardano_protocol_parameters_t object containing the protocol parameters required for transaction creation.
 * \param[in] script A pointer to a \ref cardano_script_t The script used in the reward address credential.
 * \param[in] funding_address A pointer to a \ref cardano_address_t object representing the funding source for transaction fees.
 * \param[in] reward_address A pointer to a \ref cardano_reward_address_t object representing the reward address to be registered and delegated.
 * \param[in] pool_id A pointer to a \ref cardano_blake2b_hash_t object representing the stake pool ID to which the delegation will be made.
 */
static void
register_and_delegate(
  cardano_provider_t*            provider,
  cardano_secure_key_handler_t*  key_handler,
  cardano_protocol_parameters_t* pparams,
  cardano_script_t*              script,
  cardano_address_t*             funding_address,
  cardano_reward_address_t*      reward_address,
  cardano_blake2b_hash_t*        pool_id)
{
  cardano_utxo_list_t*   utxo_list  = get_unspent_utxos(provider, funding_address);
  cardano_tx_builder_t*  tx_builder = cardano_tx_builder_new(pparams, provider);
  cardano_plutus_data_t* redeemer   = create_void_plutus_data();

  const uint64_t invalid_after = cardano_utils_get_time() + SECONDS_IN_TWO_HOURS;

  cardano_tx_builder_set_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_change_address(tx_builder, funding_address);
  cardano_tx_builder_set_collateral_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_collateral_change_address(tx_builder, funding_address);
  cardano_tx_builder_set_invalid_after_ex(tx_builder, invalid_after);
  cardano_tx_builder_add_script(tx_builder, script);
  cardano_tx_builder_register_reward_address(tx_builder, reward_address, redeemer);
  cardano_tx_builder_delegate_stake(tx_builder, reward_address, pool_id, redeemer);

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

  cardano_utxo_list_unref(&utxo_list);
  cardano_tx_builder_unref(&tx_builder);
  cardano_transaction_unref(&transaction);
  cardano_plutus_data_unref(&redeemer);

  console_info("Stake key registered and delegated successfully.");
}

/**
 * \brief Unregisters a reward address and withdraws its rewards.
 *
 * This function performs two operations:
 * 1. Unregisters a specified reward address from the blockchain.
 * 2. Withdraws any accumulated rewards associated with the reward address.
 *
 * \param[in] provider A pointer to a \ref cardano_provider_t object used for accessing blockchain state.
 * \param[in] key_handler A pointer to a \ref cardano_secure_key_handler_t object for signing and key management.
 * \param[in] pparams A pointer to a \ref cardano_protocol_parameters_t object containing the protocol parameters required for transaction creation.
 * \param[in] script A pointer to a \ref cardano_script_t The script used in the reward address credential.
 * \param[in] funding_address A pointer to a \ref cardano_address_t object representing the funding source for transaction fees.
 * \param[in] reward_address A pointer to a \ref cardano_reward_address_t object representing the reward address to be unregistered and from which rewards will be withdrawn.
 */
static void
unregister_and_withdraw_rewards(
  cardano_provider_t*            provider,
  cardano_secure_key_handler_t*  key_handler,
  cardano_protocol_parameters_t* pparams,
  cardano_script_t*              script,
  cardano_address_t*             funding_address,
  cardano_reward_address_t*      reward_address)
{
  cardano_utxo_list_t*   utxo_list  = get_unspent_utxos(provider, funding_address);
  cardano_tx_builder_t*  tx_builder = cardano_tx_builder_new(pparams, provider);
  cardano_plutus_data_t* redeemer   = create_void_plutus_data();

  const uint64_t invalid_after = cardano_utils_get_time() + SECONDS_IN_TWO_HOURS;

  cardano_tx_builder_set_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_change_address(tx_builder, funding_address);
  cardano_tx_builder_set_collateral_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_collateral_change_address(tx_builder, funding_address);
  cardano_tx_builder_set_invalid_after_ex(tx_builder, invalid_after);
  cardano_tx_builder_add_script(tx_builder, script);
  // Withdraw 0 since we just register this account, so there are no rewards to withdraw
  cardano_tx_builder_withdraw_rewards(tx_builder, reward_address, 0, redeemer);
  cardano_tx_builder_deregister_reward_address(tx_builder, reward_address, redeemer);

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

  cardano_utxo_list_unref(&utxo_list);
  cardano_tx_builder_unref(&tx_builder);
  cardano_transaction_unref(&transaction);
  cardano_plutus_data_unref(&redeemer);

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
  console_info("Delegate and Withdraw Example (Script Hash)");
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

  const char*             POOL_ID_HEX = "089a06986c7dbd50d411890a74ab9e60ba22d32bf6e59cb658491f2c"; // SMAUG
  cardano_blake2b_hash_t* policy_id   = NULL;

  cardano_error_t result = cardano_blake2b_hash_from_hex(POOL_ID_HEX, strlen(POOL_ID_HEX), &policy_id);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to convert pool ID to hash");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    return EXIT_FAILURE;
  }

  cardano_script_t*              always_succeeds_script = create_plutus_v2_script_from_hex(ALWAYS_SUCCEEDS_SCRIPT_V2);
  cardano_address_t*             script_address         = get_script_address(always_succeeds_script);
  cardano_reward_address_t*      reward_address         = get_script_stake_address(always_succeeds_script);
  cardano_secure_key_handler_t*  key_handler            = create_secure_key_handler(SERIALIZED_BIP32_KEY_HANDLER, cardano_utils_safe_strlen(SERIALIZED_BIP32_KEY_HANDLER, 256), get_passphrase);
  cardano_provider_t*            provider               = create_provider(CARDANO_NETWORK_MAGIC_PREPROD, api_key);
  cardano_address_t*             payment_address        = create_address_from_derivation_paths(key_handler, ACCOUNT_DERIVATION_PATH, PAYMENT_CRED_INDEX, STAKE_CRED_INDEX);
  cardano_protocol_parameters_t* protocol_params        = get_protocol_parameters(provider);
  cardano_utxo_list_t*           utxo_list              = get_unspent_utxos(provider, payment_address);

  // Registers and delegates the stake key to the pool
  register_and_delegate(provider, key_handler, protocol_params, always_succeeds_script, payment_address, reward_address, policy_id);
  // Example register & delegate transaction created by this sample:
  // https://preprod.cardanoscan.io/transaction/55f7d5eb3f7ac9a9fd6b9324e8fa4bab6df2d6e6c7bbcd3175d1086e47132a39

  // Unregisters and withdraws the rewards from the pool
  unregister_and_withdraw_rewards(provider, key_handler, protocol_params, always_succeeds_script, payment_address, reward_address);
  // Example unregister & withdrawal transaction created by this sample:
  // https://preprod.cardanoscan.io/transaction/706fa99900c79047677025de565723cc7f359b200f75c30056786b9eb615d7dd

  // Cleanup
  cardano_script_unref(&always_succeeds_script);
  cardano_address_unref(&script_address);
  cardano_reward_address_unref(&reward_address);
  cardano_provider_unref(&provider);
  cardano_address_unref(&payment_address);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_secure_key_handler_unref(&key_handler);
  cardano_utxo_list_unref(&utxo_list);
  cardano_blake2b_hash_unref(&policy_id);

  return EXIT_SUCCESS;
}
