/**
 * \file drep_script_example.c
 *
 * \author angel.castillo
 * \date   Nov 21, 2024
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

static const char*    ALWAYS_SUCCEEDS_SCRIPT_V3    = "590dff010000323232332232323232332232323232323232232498c8c8c94cd4ccd5cd19b874800000804c0484c8c8c8c8c8ccc88848ccc00401000c008c8c8c94cd4ccd5cd19b874800000806c0684c8c8c8c8c8c8c8c8c8c8c8c8c8c8c8c8cccccccccccc8ccc8cc8cc888888888888888848cccccccccccccccc00404404003c03803403002c02802402001c01801401000c008c004d5d080a18009aba1013302123232325335333573466e1d2000002031030133221233001003002301d35742002600a6ae84d5d1000898192481035054310035573c0046aae74004dd5000998108009aba101123232325335333573466e1d200000203002f13232333322221233330010050040030023232325335333573466e1d2000002035034133221233001003002302a35742002660564646464a66a666ae68cdc3a40000040720702642446004006605c6ae8400454cd4ccd5cd19b87480080080e40e04c8ccc888488ccc00401401000cdd69aba1002375a6ae84004dd69aba1357440026ae880044c0e92401035054310035573c0046aae74004dd50009aba1357440022606c9201035054310035573c0046aae74004dd51aba1003300735742004646464a66a666ae68cdc3a400000406a068224440062a66a666ae68cdc3a400400406a068264244460020086eb8d5d08008a99a999ab9a3370e900200101a81a099091118010021aba1001130364901035054310035573c0046aae74004dd51aba10013302875c6ae84d5d10009aba200135744002260629201035054310035573c0046aae74004dd50009bad3574201e60026ae84038c008c009d69980f80a9aba100c33302202075a6ae8402cc8c8c94cd4ccd5cd19b87480000080b80b44cc8848cc00400c008c8c8c94cd4ccd5cd19b87480000080c40c04cc8848cc00400c008cc0b9d69aba1001302d357426ae880044c0c9241035054310035573c0046aae74004dd51aba10013232325335333573466e1d20000020310301332212330010030023302e75a6ae84004c0b4d5d09aba200113032491035054310035573c0046aae74004dd51aba1357440022605e921035054310035573c0046aae74004dd51aba100a3301f75c6ae84024ccc0888c8c8c94cd4ccd5cd19b87480000080bc0b84c84888888c01401cdd71aba100115335333573466e1d200200202f02e13212222223002007301b357420022a66a666ae68cdc3a400800405e05c2642444444600600e60486ae8400454cd4ccd5cd19b87480180080bc0b84cc884888888cc01802001cdd69aba10013019357426ae8800454cd4ccd5cd19b87480200080bc0b84c84888888c00401cc068d5d08008a99a999ab9a3370e9005001017817099910911111198020040039bad3574200260306ae84d5d1000898182481035054310035573c0046aae74004dd50008131aba1008330020263574200e6eb8d5d080319981100b198110149191919299a999ab9a3370e9000001017817089110010a99a999ab9a3370e9001001017817089110008a99a999ab9a3370e900200101781708911001898182481035054310035573c0046aae74004dd50009aba10053301f0143574200860026ae8400cc004d5d09aba2003302075a6040eb8d5d10009aba2001357440026ae88004d5d10009aba2001357440026ae88004d5d10009aba2001357440026ae88004d5d10009aba20011301c491035054310035573c0046aae74004dd51aba10063574200a646464a66a666ae68cdc3a40000040360342642444444600a00e6eb8d5d08008a99a999ab9a3370e900100100d80d0999109111111980100400398039aba100133011016357426ae8800454cd4ccd5cd19b874801000806c0684c84888888c00c01cc040d5d08008a99a999ab9a3370e900300100d80d099910911111198030040039bad35742002600a6ae84d5d10008a99a999ab9a3370e900400100d80d0990911111180080398031aba100115335333573466e1d200a00201b01a13322122222233004008007375a6ae84004c010d5d09aba20011301c4901035054310035573c0046aae74004dd51aba13574400a4646464a66a666ae68cdc3a4000004036034264666444246660020080060046eb4d5d080118089aba10013232325335333573466e1d200000201f01e1323332221222222233300300a0090083301601e357420046ae84004cc059d71aba1357440026ae8800454cd4ccd5cd19b874800800807c0784cc8848888888cc01c024020cc054074d5d0800991919299a999ab9a3370e90000010110108999109198008018011bad357420026eb4d5d09aba200113023491035054310035573c0046aae74004dd51aba1357440022a66a666ae68cdc3a400800403e03c26644244444446600401201066602c028eb4d5d08009980abae357426ae8800454cd4ccd5cd19b874801800807c0784c848888888c010020cc054074d5d08008a99a999ab9a3370e900400100f80f09919199991110911111119998008058050048041980b80f9aba1003330150163574200466603002ceb4d5d08009a991919299a999ab9a3370e900000101201189980e1bad357420026eb4d5d09aba2001130254901035054310035573c0046aae74004dd51aba135744002446602a0040026ae88004d5d10008a99a999ab9a3370e900500100f80f0999109111111198028048041980a80e9aba10013232325335333573466e1d200000202202113301875c6ae840044c08d241035054310035573c0046aae74004dd51aba1357440022a66a666ae68cdc3a401800403e03c22444444400c26040921035054310035573c0046aae74004dd51aba1357440026ae880044c071241035054310035573c0046aae74004dd50009191919299a999ab9a3370e900000100d00c899910911111111111980280680618079aba10013301075a6ae84d5d10008a99a999ab9a3370e900100100d00c899910911111111111980100680618079aba10013301075a6ae84d5d10008a9919a999ab9a3370e900200180d80d099910911111111111980500680618081aba10023001357426ae8800854cd4ccd5cd19b874801800c06c0684c8ccc888488888888888ccc018038034030c044d5d080198011aba1001375a6ae84d5d10009aba200215335333573466e1d200800301b01a133221222222222223300700d00c3010357420046eb4d5d09aba200215335333573466e1d200a00301b01a132122222222222300100c3010357420042a66a666ae68cdc3a4018006036034266442444444444446600601a01860206ae84008dd69aba1357440042a66a666ae68cdc3a401c006036034266442444444444446601201a0186eb8d5d08011bae357426ae8800854cd4ccd5cd19b874804000c06c0684cc88488888888888cc020034030dd71aba1002375a6ae84d5d10010a99a999ab9a3370e900900180d80d099910911111111111980580680618081aba10023010357426ae8800854cd4ccd5cd19b874805000c06c0684c8488888888888c010030c040d5d08010980e2481035054310023232325335333573466e1d200000201e01d13212223003004375c6ae8400454c8cd4ccd5cd19b874800800c07c0784c84888c004010c004d5d08010a99a999ab9a3370e900200180f80f099910911198010028021bae3574200460026ae84d5d1001098102481035054310023232325335333573466e1d2000002022021132122230030043017357420022a66a666ae68cdc3a4004004044042224440042a66a666ae68cdc3a40080040440422244400226046921035054310035573c0046aae74004dd50009aab9e00235573a0026ea8004d55cf0011aab9d00137540024646464a66a666ae68cdc3a400000403203026424446006008601c6ae8400454cd4ccd5cd19b87480080080640604c84888c008010c038d5d08008a99a999ab9a3370e900200100c80c099091118008021bae3574200226034921035054310035573c0046aae74004dd50009191919299a999ab9a3370e900000100c00b8999109198008018011bae357420026eb4d5d09aba200113019491035054310035573c0046aae74004dd50009aba200113014491035054310035573c0046aae74004dd50009808911299a999ab9a3370e900000080880809809249035054330015335333573466e20005200001101013300333702900000119b81480000044c8cc8848cc00400c008cdc200180099b840020013300400200130102225335333573466e1d200000101000f10021330030013370c004002464460046eb0004c04088cccd55cf8009005119a80498021aba10023003357440040224646464a66a666ae68cdc3a400000401e01c26424460040066eb8d5d08008a99a999ab9a3370e900100100780709909118008019bae3574200226020921035054310035573c0046aae74004dd500091191919299a999ab9a3370e900100100780708910008a99a999ab9a3370e9000001007807099091180100198029aba1001130104901035054310035573c0046aae74004dd50009119118011bab001300e2233335573e002401046466a0106600e600c6aae74004c014d55cf00098021aba20033574200401e4424660020060042440042442446600200800640024646464a66a666ae68cdc3a400000401000e200e2a66a666ae68cdc3a400400401000e201026012921035054310035573c0046aae74004dd500091191919299a999ab9a3370e9000001004003889110010a99a999ab9a3370e90010010040038990911180180218029aba100115335333573466e1d200400200800711222001130094901035054310035573c0046aae74004dd50009191919299a999ab9a3370e90000010030028999109198008018011bae357420026eb4d5d09aba200113007491035054310035573c0046aae74004dd5000891001091000919319ab9c0010021200123230010012300223300200200101";
static const char*    SERIALIZED_BIP32_KEY_HANDLER = "0a0a0a0a01010000005c97db5e09b3a4919ec75ed1126056241a1e5278731c2e0b01bea0a5f42c22db4131e0a4bbe75633677eb0e60e2ecd3520178f85c7e0d4be77a449087fe9674ee52f946b07c1b56d228c496ec0d36dd44212ba8af0f6eed1a82194dd69f479c603";
static const uint64_t CONFIRM_TX_TIMEOUT_MS        = 240000U;
static const uint64_t PAYMENT_CRED_INDEX           = 0U;
static const uint64_t STAKE_CRED_INDEX             = 0U;
static const uint64_t SECONDS_IN_TWO_HOURS         = 60UL * 60UL * 2UL;
static const char*    ANCHOR_HASH                  = "26ce09df4e6f64fe5cf248968ab78f4b8a0092580c234d78f68c079c0fce34f0";
static const char*    ANCHOR_URL                   = "https://storage.googleapis.com/biglup/Angel_Castillo.jsonld";

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
 * \brief Registers a Delegated Representative (DRep) in the Cardano governance system.
 *
 * This function creates a transaction to add the DRep to the governance system and submits it to the blockchain.
 *
 * \param[in] provider A pointer to a \ref cardano_provider_t object used for accessing blockchain state.
 * \param[in] key_handler A pointer to a \ref cardano_secure_key_handler_t object for signing and key management.
 * \param[in] pparams A pointer to a \ref cardano_protocol_parameters_t object containing the protocol parameters required for transaction creation.
 * \param[in] script A pointer to a \ref cardano_script_t object representing the script used as DRep credential.
 * \param[in] funding_address A pointer to a \ref cardano_address_t object representing the funding source for transaction fees.
 * \param[in] drep_id A string representing the unique identifier of the Delegated Representative (DRep) to be registered.
 */
static void
register_as_drep(
  cardano_provider_t*            provider,
  cardano_secure_key_handler_t*  key_handler,
  cardano_protocol_parameters_t* pparams,
  cardano_script_t*              script,
  cardano_address_t*             funding_address,
  const char*                    drep_id)
{
  console_info("Registering DRep: %s", drep_id);
  console_info("- Metadata URL: %s", ANCHOR_URL);
  console_info("- Metadata Hash: %s", ANCHOR_HASH);

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
  cardano_tx_builder_register_drep_ex(tx_builder, drep_id, strlen(drep_id), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), redeemer);

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
    SIGNER_DERIVATION_PATH
  };

  sign_transaction_with_keys(key_handler, &KEYS[0], sizeof(KEYS) / sizeof(cardano_derivation_path_t), transaction);
  submit_transaction(provider, CONFIRM_TX_TIMEOUT_MS, transaction);

  cardano_utxo_list_unref(&utxo_list);
  cardano_tx_builder_unref(&tx_builder);
  cardano_transaction_unref(&transaction);
  cardano_plutus_data_unref(&redeemer);

  console_info("DRep registered successfully.");
}

/**
 * \brief Unregisters a Delegated Representative (DRep) from the Cardano governance system.
 *
 * This function performs the deregistration of a DRep identified by its unique identifier (`drep_id`) from the governance system.
 *
 * \param[in] provider A pointer to a \ref cardano_provider_t object used for accessing blockchain state.
 * \param[in] key_handler A pointer to a \ref cardano_secure_key_handler_t object for signing and key management.
 * \param[in] pparams A pointer to a \ref cardano_protocol_parameters_t object containing the protocol parameters required for transaction creation.
 * \param[in] script A pointer to a \ref cardano_script_t object representing the script used as DRep credential.
 * \param[in] funding_address A pointer to a \ref cardano_address_t object representing the funding source for transaction fees.
 * \param[in] drep_id A string representing the unique identifier of the Delegated Representative (DRep) to be unregistered.
 */
static void
unregister_as_drep(
  cardano_provider_t*            provider,
  cardano_secure_key_handler_t*  key_handler,
  cardano_protocol_parameters_t* pparams,
  cardano_script_t*              script,
  cardano_address_t*             funding_address,
  const char*                    drep_id)
{
  console_info("Unregistering DRep: %s", drep_id);

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
  cardano_tx_builder_deregister_drep_ex(tx_builder, drep_id, strlen(drep_id), redeemer);

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
    SIGNER_DERIVATION_PATH
  };

  sign_transaction_with_keys(key_handler, &KEYS[0], sizeof(KEYS) / sizeof(cardano_derivation_path_t), transaction);
  submit_transaction(provider, CONFIRM_TX_TIMEOUT_MS, transaction);

  cardano_utxo_list_unref(&utxo_list);
  cardano_tx_builder_unref(&tx_builder);
  cardano_transaction_unref(&transaction);
  cardano_plutus_data_unref(&redeemer);

  console_info("DRep unregistered successfully.");
}

/**
 * \brief Registers a reward address and delegates it to a DRep.
 *
 * This function performs the following actions:
 * 1. Registers a reward address on the blockchain if it is not already registered.
 * 2. Delegates the registered reward address to a specified DRep.
 *
 * \param[in] provider A pointer to a \ref cardano_provider_t object used for accessing blockchain state.
 * \param[in] key_handler A pointer to a \ref cardano_secure_key_handler_t object for signing and key management.
 * \param[in] pparams A pointer to a \ref cardano_protocol_parameters_t object containing the protocol parameters required for transaction creation.
 * \param[in] funding_address A pointer to a \ref cardano_address_t object representing the funding source for transaction fees.
 * \param[in] reward_address A string representing the Bech32-encoded reward address to be registered and delegated.
 * \param[in] drep_id A string representing the Bech32-encoded DRep ID to which the reward address will delegate its voting power.
 */
static void
register_and_delegate(
  cardano_provider_t*            provider,
  cardano_secure_key_handler_t*  key_handler,
  cardano_protocol_parameters_t* pparams,
  cardano_address_t*             funding_address,
  const char*                    reward_address,
  const char*                    drep_id)
{
  cardano_utxo_list_t*  utxo_list  = get_unspent_utxos(provider, funding_address);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(pparams, provider);

  const uint64_t invalid_after = cardano_utils_get_time() + SECONDS_IN_TWO_HOURS;

  cardano_tx_builder_set_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_change_address(tx_builder, funding_address);
  cardano_tx_builder_set_invalid_after_ex(tx_builder, invalid_after);
  cardano_tx_builder_register_reward_address_ex(tx_builder, reward_address, strlen(reward_address), NULL);
  cardano_tx_builder_delegate_voting_power_ex(tx_builder, reward_address, strlen(reward_address), drep_id, strlen(drep_id), NULL);

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

  console_info("Stake key unregistered and rewards withdrawn successfully.");
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
  console_info("Registers DRep and Delegate Voting Power (Script Hash)");
  console_info("libcardano-c:  V-%s\n", cardano_get_lib_version());

  console_info("This example registers a PlutusV3 as DReps, delegate our voting power to the DRep credential, and finally unregisters both the DRep and stake key.");

  console_set_foreground_color(CONSOLE_COLOR_GREEN);
  console_write("\nUse passphrase: 'password'\n\n");
  console_reset_color();

  const char* api_key = getenv("BLOCKFROST_API_KEY");

  if (api_key == NULL)
  {
    console_error("BLOCKFROST_API_KEY environment variable is not set.\n");

    return EXIT_FAILURE;
  }

  cardano_script_t*             always_succeeds_script = create_plutus_v3_script_from_hex(ALWAYS_SUCCEEDS_SCRIPT_V3);
  cardano_drep_t*               drep                   = get_script_drep(always_succeeds_script);
  cardano_secure_key_handler_t* key_handler            = create_secure_key_handler(SERIALIZED_BIP32_KEY_HANDLER, cardano_utils_safe_strlen(SERIALIZED_BIP32_KEY_HANDLER, 256), get_passphrase);
  cardano_provider_t*           provider               = create_provider(CARDANO_NETWORK_MAGIC_PREPROD, api_key);
  cardano_address_t*            payment_address        = create_address_from_derivation_paths(key_handler, ACCOUNT_DERIVATION_PATH, PAYMENT_CRED_INDEX, STAKE_CRED_INDEX);

  const char* stake_address = "stake_test1uppy2gm2hqzkwc80em4mlat73j4jyqvzhclrvsu72g9xg4q2yweet";

  cardano_protocol_parameters_t* protocol_params = get_protocol_parameters(provider);
  cardano_utxo_list_t*           utxo_list       = get_unspent_utxos(provider, payment_address);

  char            drep_id[256] = { 0 };
  cardano_error_t result       = cardano_drep_to_string(drep, drep_id, 256);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to convert DRep to string");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  // Registers as a DRep
  register_as_drep(provider, key_handler, protocol_params, always_succeeds_script, payment_address, drep_id);
  // Example register as DRep transaction created by this sample:
  // https://preprod.cardanoscan.io/transaction/abefcdb7c61b55bde548b1d4d0209c2968c1739ec01bddb471bca39f9c744f5e

  // Registers and delegates the voting power to ourselves as DRep
  register_and_delegate(provider, key_handler, protocol_params, payment_address, stake_address, drep_id);
  // Example register & delegate transaction created by this sample:
  // https://preprod.cardanoscan.io/transaction/d958be2795d1e2afdcec2699741d277bbb1f7d470ae60053e370bdc8bedbbb6f

  // Unregisters and withdraws the rewards
  unregister_and_withdraw_rewards(provider, key_handler, protocol_params, payment_address, stake_address);
  // Example unregister & withdrawal transaction created by this sample:
  // https://preprod.cardanoscan.io/transaction/506940b1cbee111d194ad71682fd668b97e595fa9e9b99edf3f8900b629ec280

  // Unregisters as a DRep
  unregister_as_drep(provider, key_handler, protocol_params, always_succeeds_script, payment_address, drep_id);
  // Example unregister as DRep transaction created by this sample:
  // https://preprod.cardanoscan.io/transaction/bad4a190f2a22baf0241e30a95c09fbf7403ca6831cca62cdad0db6b5d7e0e98

  // Cleanup
  cardano_provider_unref(&provider);
  cardano_address_unref(&payment_address);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_secure_key_handler_unref(&key_handler);
  cardano_utxo_list_unref(&utxo_list);
  cardano_drep_unref(&drep);
  cardano_script_unref(&always_succeeds_script);

  return EXIT_SUCCESS;
}
