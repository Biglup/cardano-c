/**
 * \file vote_for_proposal_pubkey_example.c
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

static const cardano_derivation_path_t SIGNER_DREP_DERIVATION_PATH = {
  .purpose   = 1852U | 0x80000000,
  .coin_type = 1815U | 0x80000000,
  .account   = 0U,
  .role      = 3U,
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
 * \param[in] funding_address A pointer to a \ref cardano_address_t object representing the funding source for transaction fees.
 * \param[in] drep_id A string representing the unique identifier of the Delegated Representative (DRep) to be registered.
 */
static void
register_as_drep(
  cardano_provider_t*            provider,
  cardano_secure_key_handler_t*  key_handler,
  cardano_protocol_parameters_t* pparams,
  cardano_address_t*             funding_address,
  const char*                    drep_id)
{
  console_info("Registering DRep: %s", drep_id);
  console_info("- Metadata URL: %s", ANCHOR_URL);
  console_info("- Metadata Hash: %s", ANCHOR_HASH);

  cardano_utxo_list_t*  utxo_list  = get_unspent_utxos(provider, funding_address);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(pparams, provider);

  const uint64_t invalid_after = cardano_utils_get_time() + SECONDS_IN_TWO_HOURS;

  cardano_tx_builder_set_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_change_address(tx_builder, funding_address);
  cardano_tx_builder_set_invalid_after_ex(tx_builder, invalid_after);
  cardano_tx_builder_register_drep_ex(tx_builder, drep_id, strlen(drep_id), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), NULL);

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
    SIGNER_DREP_DERIVATION_PATH
  };

  sign_transaction_with_keys(key_handler, &KEYS[0], sizeof(KEYS) / sizeof(cardano_derivation_path_t), transaction);
  submit_transaction(provider, CONFIRM_TX_TIMEOUT_MS, transaction);

  cardano_utxo_list_unref(&utxo_list);
  cardano_tx_builder_unref(&tx_builder);
  cardano_transaction_unref(&transaction);

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
 * \param[in] funding_address A pointer to a \ref cardano_address_t object representing the funding source for transaction fees.
 * \param[in] drep_id A string representing the unique identifier of the Delegated Representative (DRep) to be unregistered.
 */
static void
unregister_as_drep(
  cardano_provider_t*            provider,
  cardano_secure_key_handler_t*  key_handler,
  cardano_protocol_parameters_t* pparams,
  cardano_address_t*             funding_address,
  const char*                    drep_id)
{
  console_info("Unregistering DRep: %s", drep_id);

  cardano_utxo_list_t*  utxo_list  = get_unspent_utxos(provider, funding_address);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(pparams, provider);

  const uint64_t invalid_after = cardano_utils_get_time() + SECONDS_IN_TWO_HOURS;

  cardano_tx_builder_set_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_change_address(tx_builder, funding_address);
  cardano_tx_builder_set_invalid_after_ex(tx_builder, invalid_after);
  cardano_tx_builder_deregister_drep_ex(tx_builder, drep_id, strlen(drep_id), NULL);

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
    SIGNER_DREP_DERIVATION_PATH
  };

  sign_transaction_with_keys(key_handler, &KEYS[0], sizeof(KEYS) / sizeof(cardano_derivation_path_t), transaction);
  submit_transaction(provider, CONFIRM_TX_TIMEOUT_MS, transaction);

  cardano_utxo_list_unref(&utxo_list);
  cardano_tx_builder_unref(&tx_builder);
  cardano_transaction_unref(&transaction);

  console_info("DRep unregistered successfully.");
}

/**
 * \brief Votes for a proposal as a Delegated Representative (DRep).
 *
 * This function creates a transaction to vote for a proposal as a DRep and submits it to the blockchain.
 *
 * \param[in] provider A pointer to a \ref cardano_provider_t object used for accessing blockchain state.
 * \param[in] key_handler A pointer to a \ref cardano_secure_key_handler_t object for signing and key management.
 * \param[in] pparams A pointer to a \ref cardano_protocol_parameters_t object containing the protocol parameters required for transaction creation.
 * \param[in] funding_address A pointer to a \ref cardano_address_t object representing the funding source for transaction fees.
 * \param[in] drep_id A string representing the unique identifier of the Delegated Representative (DRep) voting.
 * \param[in] proposal_id_hex A string representing the hexadecimal identifier of the proposal to vote for.
 * \param[in] proposal_index The index of the proposal to vote for.
 * \param[in] vote The vote to cast for the proposal.
 */
static void
vote_as_drep(
  cardano_provider_t*            provider,
  cardano_secure_key_handler_t*  key_handler,
  cardano_protocol_parameters_t* pparams,
  cardano_address_t*             funding_address,
  const char*                    drep_id,
  const char*                    proposal_id_hex,
  const uint64_t                 proposal_index,
  const cardano_vote_t           vote)
{
  cardano_voter_t*                drep_voter = create_drep_voter(drep_id);
  cardano_governance_action_id_t* gov_id     = create_governance_id(proposal_id_hex, proposal_index);

  console_info("Voting for proposal %s#%d as DRep %s", proposal_id_hex, proposal_index, proposal_id_hex);

  cardano_voting_procedure_t* voting_procedure = NULL;

  // You can pass a metadata in an anchor here for this example we will use NULL
  cardano_error_t result = cardano_voting_procedure_new(vote, NULL, &voting_procedure);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to create voting procedure");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));

    exit(result);
  }

  cardano_utxo_list_t*  utxo_list  = get_unspent_utxos(provider, funding_address);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(pparams, provider);

  const uint64_t invalid_after = cardano_utils_get_time() + SECONDS_IN_TWO_HOURS;

  cardano_tx_builder_set_utxos(tx_builder, utxo_list);
  cardano_tx_builder_set_change_address(tx_builder, funding_address);
  cardano_tx_builder_set_invalid_after_ex(tx_builder, invalid_after);
  cardano_tx_builder_vote(tx_builder, drep_voter, gov_id, voting_procedure, NULL); // Pass redeemer if voter is a script

  cardano_transaction_t* transaction = NULL;
  result                             = cardano_tx_builder_build(tx_builder, &transaction);

  if (result != CARDANO_SUCCESS)
  {
    console_error("Failed to build transaction");
    console_error("Error [%d]: %s", result, cardano_error_to_string(result));
    console_error("%s", cardano_tx_builder_get_last_error(tx_builder));

    exit(result);
  }

  const cardano_derivation_path_t KEYS[] = {
    SIGNER_DERIVATION_PATH,
    SIGNER_DREP_DERIVATION_PATH
  };

  sign_transaction_with_keys(key_handler, &KEYS[0], sizeof(KEYS) / sizeof(cardano_derivation_path_t), transaction);
  submit_transaction(provider, CONFIRM_TX_TIMEOUT_MS, transaction);

  cardano_utxo_list_unref(&utxo_list);
  cardano_tx_builder_unref(&tx_builder);
  cardano_transaction_unref(&transaction);
  cardano_voter_unref(&drep_voter);
  cardano_governance_action_id_unref(&gov_id);
  cardano_voting_procedure_unref(&voting_procedure);

  console_info("Vote submitted successfully.");
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
  console_info("Vote for proposal as a DRep (Pubkey Hash)");
  console_info("libcardano-c:  V-%s\n", cardano_get_lib_version());

  console_info("This example votes for a proposal as a Pubkey Hash DRep.");

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
  cardano_drep_t*               drep            = create_drep_from_derivation_path(key_handler, ACCOUNT_DERIVATION_PATH);

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
  // register_as_drep(provider, key_handler, protocol_params, payment_address, drep_id);
  // Example register as DRep transaction created by this sample:
  // https://preprod.cardanoscan.io/transaction/6a50b38075037d0b18067e24a2040b44a2bf0f31814d28a5383406b2df8cc310

  // Vote for proposal, we are going to vote for proposal https://preprod.cardanoscan.io/govAction/gov_action1xukk3ra2wls5v7vttqdnytq09xq6jq3hv3ek4hjaztswfeuk47xqqg4644z
  // which is still active as of epoch 182, and will expire on epoch 188. This proposal was created using the example 'propose_withdrawal_example.c'.
  vote_as_drep(provider, key_handler, protocol_params, payment_address, drep_id, "372d688faa77e146798b581b322c0f2981a9023764736ade5d12e0e4e796af8c", 0, CARDANO_VOTE_YES);
  // Example unregister as DRep transaction created by this sample:
  // https://preprod.cardanoscan.io/transaction/382b28b2af90b19a26c2140b7342d09d607389ab912846750b18b3d4faf1f460

  // Unregisters as a DRep
  unregister_as_drep(provider, key_handler, protocol_params, payment_address, drep_id);
  // Example unregister as DRep transaction created by this sample:
  // https://preprod.cardanoscan.io/transaction/358dc0f7252a1e6a49178aa526f121bbf4b596d96aa5b69921105c1cb5b64859

  // Cleanup
  cardano_provider_unref(&provider);
  cardano_address_unref(&payment_address);
  cardano_protocol_parameters_unref(&protocol_params);
  cardano_secure_key_handler_unref(&key_handler);
  cardano_utxo_list_unref(&utxo_list);
  cardano_drep_unref(&drep);

  return EXIT_SUCCESS;
}
