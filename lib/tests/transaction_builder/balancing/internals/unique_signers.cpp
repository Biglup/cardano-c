/**
 * \file unique_signers.cpp
 *
 * \author angel.castillo
 * \date   Nov 07, 2024
 *
 * \section LICENSE
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

#include "../../../allocators_helpers.h"

extern "C" {
#include "../../../../src/transaction_builder/balancing/internals/unique_signers.h"
}

#include <allocators.h>
#include <cardano/common/utxo.h>
#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* BALANCED_TX_CBOR            = "84a300d9010282825820027b68d4c11e97d7e065cc2702912cb1a21b6d0e56c6a74dd605889a5561138500825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a633010182a2005839005cf6c91279a859a072601779fb33bb07c34e1d641d45df51ff63b967f15db05f56035465bf8900a09bdaa16c3d8b8244fea686524408dd8001821a00e4e1c0a1581c0b0d621b5c26d0a1fd0893a4b04c19d860296a69ede1fbcfc5179882a1474e46542d30303101a200583900dc435fc2638f6684bd1f9f6f917d80c92ae642a4a33a412e516479e64245236ab8056760efceebbff57e8cab220182be3e36439e520a6454011a0d294e28021a00029eb9a0f5f6";
static const char* WITHDRAWAL_CBOR             = "a2581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d005581df1c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f05";
static const char* KEY_HASH_CREDENTIAL_CBOR    = "8200581c00000000000000000000000000000000000000000000000000000000";
static const char* SCRIPT_HASH_CREDENTIAL_CBOR = "8201581c00000000000000000000000000000000000000000000000000000000";

static const char* CBOR_AUTHORIZE_COMMITTEE_HOT            = "830e8200581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000001";
static const char* CBOR_GENESIS_DELEGATION                 = "8405581c00010001000100010001000100010001000100010001000100010001581c0002000200020002000200020002000200020002000200020002000258200003000300030003000300030003000300030003000300030003000300030003";
static const char* CBOR_MIR                                = "820682001a000f4240";
static const char* CBOR_POOL_REGISTRATION                  = "8a03581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5";
static const char* CBOR_POOL_RETIREMENT                    = "8304581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef921903e8";
static const char* CBOR_REGISTER_DREP                      = "84108200581c0000000000000000000000000000000000000000000000000000000000f6";
static const char* CBOR_REGISTRATION                       = "83078200581c0000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_RESIGN_COMMITTEE_COLD              = "830f8200581c00000000000000000000000000000000000000000000000000000000f6";
static const char* CBOR_STAKE_DELEGATION                   = "83028200581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef92";
static const char* CBOR_STAKE_DEREGISTRATION               = "82018200581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f";
static const char* CBOR_STAKE_REGISTRATION                 = "82008200581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f";
static const char* CBOR_STAKE_REGISTRATION_DELEGATION      = "840b8200581c00000000000000000000000000000000000000000000000000000000581c0000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_STAKE_VOTE_DELEGATION              = "840a8200581c00000000000000000000000000000000000000000000000000000000581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000";
static const char* CBOR_STAKE_VOTE_REGISTRATION_DELEGATION = "850d8200581c00000000000000000000000000000000000000000000000000000000581c000000000000000000000000000000000000000000000000000000008200581c0000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_UNREGISTER_DREP                    = "83118200581c0000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_UNREGISTRATION                     = "83088200581c0000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_UPDATE_DREP                        = "83128200581c00000000000000000000000000000000000000000000000000000000827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_VOTE_DELEGATION                    = "83098200581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000";
static const char* CBOR_VOTE_REGISTRATION_DELEGATION       = "840c8200581c000000000000000000000000000000000000000000000000000000008200581c0000000000000000000000000000000000000000000000000000000000";

static const char* VOTING_PROCEDURES_CBOR       = "a28202581c10000000000000000000000000000000000000000000000000000000a38258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258202000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008203581c20000000000000000000000000000000000000000000000000000000a28258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* EMPTY_VOTING_PROCEDURES_CBOR = "a0";

static const std::string basePaymentScriptStakeKey           = "addr1z8phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgs9yc0hh";
static const std::string basePaymentKeyStakeScript           = "addr1yx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzerkr0vd4msrxnuwnccdxlhdjar77j6lg0wypcc9uar5d2shs2z78ve";
static const std::string basePaymentScriptStakeScript        = "addr1x8phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gt7r0vd4msrxnuwnccdxlhdjar77j6lg0wypcc9uar5d2shskhj42g";
static const std::string testnetBasePaymentKeyStakeKey       = "addr_test1qz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgs68faae";
static const std::string testnetBasePaymentScriptStakeKey    = "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg";
static const std::string testnetBasePaymentKeyStakeScript    = "addr_test1yz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzerkr0vd4msrxnuwnccdxlhdjar77j6lg0wypcc9uar5d2shsf5r8qx";
static const std::string testnetBasePaymentScriptStakeScript = "addr_test1xrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gt7r0vd4msrxnuwnccdxlhdjar77j6lg0wypcc9uar5d2shs4p04xh";
static const std::string testnetPointerKey                   = "addr_test1gz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer5pnz75xxcrdw5vky";
static const std::string testnetPointerScript                = "addr_test12rphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtupnz75xxcryqrvmw";
static const std::string testnetEnterpriseKey                = "addr_test1vz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzerspjrlsz";
static const std::string testnetEnterpriseScript             = "addr_test1wrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcl6szpr";
static const std::string testnetRewardKey                    = "stake_test1uqehkck0lajq8gr28t9uxnuvgcqrc6070x3k9r8048z8y5gssrtvn";
static const std::string testnetRewardScript                 = "stake_test17rphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcljw6kf";

static const std::string byronMainnetYoroi    = "Ae2tdPwUPEZFRbyhz3cpfC2CumGzNkFBN2L42rcUc2yjQpEkxDbkPodpMAi";
static const std::string byronTestnetDaedalus = "37btjrVyb4KEB2STADSsj3MYSAdj52X5FrFWpw2r7Wmj2GDzXjFRsHWuZqrw7zSkwopv8Ci3VWeg6bisU9dgJxW5hb2MZYeduNKbQJrqz3zVBsu9nT";

static const std::string BYRON_YOROI_ROOT_HEX          = "ba970ad36654d8dd8f74274b733452ddeab9a62a397746be3c42ccdd";
static const std::string BYRON_DAEDALUS_ROOT_HEX       = "9c708538a763ff27169987a489e35057ef3cd3778c05e96f7ba9450e";
static const std::string BYRON_YOROI_ATTRIBUTES_HEX    = "a0";
static const std::string BYRON_DAEDALUS_ATTRIBUTES_HEX = "a201581e581c9c1722f7e446689256e1a30260f3510d558d99d0c391f2ba89cb697702451a4170cb17";
static const char*       COLLATERAL_TX_ID_HEX          = "0f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5";

/* STATIC FUNCTIONS **********************************************************/

static void
verify_credential(const std::string& addr, const bool is_null)
{
  cardano_address_t*      address = nullptr;
  cardano_blake2b_hash_t* hash    = nullptr;

  EXPECT_EQ(cardano_address_from_string(addr.c_str(), addr.length(), &address), CARDANO_SUCCESS);

  hash = _cardano_get_payment_pub_key_hash(address);

  if (is_null)
  {
    EXPECT_EQ(hash, nullptr);
  }
  else
  {
    EXPECT_NE(hash, nullptr);
  }

  cardano_blake2b_hash_unref(&hash);
  cardano_address_unref(&address);
}

static void
verify_memory_allocation_fail(const std::string& addr)
{
  cardano_address_t*      address = nullptr;
  cardano_blake2b_hash_t* hash    = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(cardano_address_from_string(addr.c_str(), addr.length(), &address), CARDANO_SUCCESS);

  reset_allocators_run_count();

  cardano_set_allocators(fail_right_away_malloc, realloc, free);
  hash = _cardano_get_payment_pub_key_hash(address);

  EXPECT_EQ(hash, nullptr);

  reset_allocators_run_count();

  cardano_set_allocators(fail_after_one_malloc, realloc, free);
  hash = _cardano_get_payment_pub_key_hash(address);

  EXPECT_EQ(hash, nullptr);

  reset_allocators_run_count();

  cardano_set_allocators(fail_after_two_malloc, realloc, free);
  hash = _cardano_get_payment_pub_key_hash(address);

  EXPECT_EQ(hash, nullptr);

  reset_allocators_run_count();

  cardano_set_allocators(fail_after_three_malloc, realloc, free);
  hash = _cardano_get_payment_pub_key_hash(address);

  EXPECT_EQ(hash, nullptr);

  reset_allocators_run_count();
  cardano_set_allocators(malloc, realloc, free);
  cardano_blake2b_hash_unref(&hash);
  cardano_address_unref(&address);
}

static cardano_transaction_t*
new_default_transaction(const char* cbor)
{
  cardano_transaction_t* transaction = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_transaction_from_cbor(reader, &transaction);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return transaction;
};

static cardano_certificate_t*
new_default_certificate(const char* cbor)
{
  cardano_certificate_t* certificate = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_certificate_from_cbor(reader, &certificate);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return certificate;
};

static cardano_error_t
validate_cert(const char* cbor, cardano_cert_type_t type, const size_t expected_creds)
{
  cardano_certificate_t* certificate = new_default_certificate(cbor);

  cardano_blake2b_hash_set_t* unique_signers = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_set_new(&unique_signers), CARDANO_SUCCESS);

  cardano_error_t result = _process_certificate_with_credential(unique_signers, certificate, type);

  EXPECT_EQ(cardano_blake2b_hash_set_get_length(unique_signers), expected_creds);

  cardano_certificate_unref(&certificate);
  cardano_blake2b_hash_set_unref(&unique_signers);

  return result;
}

static cardano_error_t
validate_add_cert(const char* cbor, const size_t expected_creds)
{
  cardano_certificate_t*      certificate    = new_default_certificate(cbor);
  cardano_certificate_set_t*  certificates   = nullptr;
  cardano_blake2b_hash_set_t* unique_signers = nullptr;

  EXPECT_EQ(cardano_certificate_set_new(&certificates), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_set_new(&unique_signers), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_certificate_set_add(certificates, certificate), CARDANO_SUCCESS);

  cardano_error_t result = _cardano_add_certificates_pub_key_hashes(unique_signers, certificates);

  EXPECT_EQ(cardano_blake2b_hash_set_get_length(unique_signers), expected_creds);

  cardano_certificate_unref(&certificate);
  cardano_certificate_set_unref(&certificates);
  cardano_blake2b_hash_set_unref(&unique_signers);

  return result;
}

static cardano_error_t
validate_cert_memory_alloc_error(const char* cbor, cardano_cert_type_t type)
{
  cardano_certificate_t* certificate = new_default_certificate(cbor);

  cardano_blake2b_hash_set_t* unique_signers = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_set_new(&unique_signers), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = _process_certificate_with_credential(unique_signers, certificate, type);

  EXPECT_EQ(cardano_blake2b_hash_set_get_length(unique_signers), 0U);

  cardano_certificate_unref(&certificate);
  cardano_blake2b_hash_set_unref(&unique_signers);
  cardano_set_allocators(malloc, realloc, free);

  return result;
}

/**
 * The number of VK witnesses and of bootstrap witnesses a transaction needs.
 */
struct signer_counts_t
{
    size_t vkey_signers;
    size_t bootstrap_signers;
};

/**
 * Creates an address from its string representation.
 * \param address_string the bech32 or base58 representation of the address.
 * \return A new instance of the address.
 */
static cardano_address_t*
new_address(const std::string& address_string)
{
  cardano_address_t* address = nullptr;

  EXPECT_EQ(cardano_address_from_string(address_string.c_str(), address_string.length(), &address), CARDANO_SUCCESS);

  return address;
}

/**
 * Gets the hexadecimal representation of a hash.
 * \param hash the hash.
 * \return The hexadecimal string, empty if the hash is NULL.
 */
static std::string
get_hash_hex(cardano_blake2b_hash_t* hash)
{
  if (hash == nullptr)
  {
    return "";
  }

  std::string hex(cardano_blake2b_hash_get_hex_size(hash), '\0');

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, &hex[0], hex.size()), CARDANO_SUCCESS);
  hex.resize(hex.size() - 1U);

  return hex;
}

/**
 * Gets the hexadecimal representation of the content of a buffer.
 * \param buffer the buffer.
 * \return The hexadecimal string, empty if the buffer is NULL.
 */
static std::string
get_buffer_hex(cardano_buffer_t* buffer)
{
  if (buffer == nullptr)
  {
    return "";
  }

  std::string hex(cardano_buffer_get_hex_size(buffer), '\0');

  EXPECT_EQ(cardano_buffer_to_hex(buffer, &hex[0], hex.size()), CARDANO_SUCCESS);
  hex.resize(hex.size() - 1U);

  return hex;
}

/**
 * Adds to a list of resolved inputs the UTXO an address holds at a transaction input.
 * \param resolved_inputs the list that receives the UTXO.
 * \param input the transaction input the UTXO resolves.
 * \param address_string the address that holds the UTXO.
 */
static void
add_resolved_input(cardano_utxo_list_t* resolved_inputs, cardano_transaction_input_t* input, const std::string& address_string)
{
  cardano_address_t*            address = new_address(address_string);
  cardano_transaction_output_t* output  = nullptr;
  cardano_utxo_t*               utxo    = nullptr;

  EXPECT_EQ(cardano_transaction_output_new(address, 2000000U, &output), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_new(input, output, &utxo), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(resolved_inputs, utxo), CARDANO_SUCCESS);

  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&output);
  cardano_address_unref(&address);
}

/**
 * Resolves the two inputs of a transaction to UTXOs held by the given addresses.
 * \param tx the transaction, which spends two inputs.
 * \param first the address that holds the first input.
 * \param second the address that holds the second input.
 * \return A new list with the resolved inputs.
 */
static cardano_utxo_list_t*
new_resolved_inputs(cardano_transaction_t* tx, const std::string& first, const std::string& second)
{
  cardano_utxo_list_t*             resolved_inputs = nullptr;
  cardano_transaction_body_t*      body            = cardano_transaction_get_body(tx);
  cardano_transaction_input_set_t* inputs          = cardano_transaction_body_get_inputs(body);
  cardano_transaction_input_t*     first_input     = nullptr;
  cardano_transaction_input_t*     second_input    = nullptr;

  EXPECT_EQ(cardano_utxo_list_new(&resolved_inputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_get(inputs, 0U, &first_input), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_get(inputs, 1U, &second_input), CARDANO_SUCCESS);

  add_resolved_input(resolved_inputs, first_input, first);
  add_resolved_input(resolved_inputs, second_input, second);

  cardano_transaction_input_unref(&first_input);
  cardano_transaction_input_unref(&second_input);
  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_body_unref(&body);

  return resolved_inputs;
}

/**
 * Counts the VK signers and the bootstrap signers of a transaction.
 * \param tx the transaction.
 * \param resolved_inputs the UTXOs that resolve the inputs of the transaction.
 * \return The number of signers of each kind.
 */
static signer_counts_t
get_signer_counts(cardano_transaction_t* tx, cardano_utxo_list_t* resolved_inputs)
{
  cardano_blake2b_hash_set_t* unique_signers    = nullptr;
  cardano_utxo_list_t*        bootstrap_signers = nullptr;

  EXPECT_EQ(_cardano_get_unique_signers(tx, resolved_inputs, &unique_signers), CARDANO_SUCCESS);
  EXPECT_EQ(_cardano_get_bootstrap_signers(tx, resolved_inputs, &bootstrap_signers), CARDANO_SUCCESS);

  signer_counts_t counts = { cardano_blake2b_hash_set_get_length(unique_signers), cardano_utxo_list_get_length(bootstrap_signers) };

  cardano_blake2b_hash_set_unref(&unique_signers);
  cardano_utxo_list_unref(&bootstrap_signers);

  return counts;
}

/**
 * Gets the hexadecimal representation of the root of the Byron address that holds a UTXO of a list.
 * \param utxos the list of UTXOs.
 * \param index the position of the UTXO in the list.
 * \return The hexadecimal root, empty if the UTXO is not held by a Byron address.
 */
static std::string
get_utxo_root_hex(cardano_utxo_list_t* utxos, const size_t index)
{
  cardano_utxo_t* utxo = nullptr;

  EXPECT_EQ(cardano_utxo_list_get(utxos, index, &utxo), CARDANO_SUCCESS);

  cardano_transaction_output_t* output  = cardano_utxo_get_output(utxo);
  cardano_address_t*            address = cardano_transaction_output_get_address(output);
  cardano_blake2b_hash_t*       root    = nullptr;

  EXPECT_EQ(_cardano_get_bootstrap_key_hash(address, &root), CARDANO_SUCCESS);

  std::string hex = get_hash_hex(root);

  cardano_blake2b_hash_unref(&root);
  cardano_address_unref(&address);
  cardano_transaction_output_unref(&output);
  cardano_utxo_unref(&utxo);

  return hex;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_blake2b_hash_set_has, returnFalseIfGivenNull)
{
  EXPECT_FALSE(_cardano_blake2b_hash_set_has(nullptr, nullptr));
}

TEST(_cardano_add_required_signers, returnFalseIfGivenNull)
{
  EXPECT_EQ(_cardano_add_required_signers(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(_cardano_get_payment_pub_key_hash, returnFalseIfGivenNull)
{
  EXPECT_EQ(_cardano_get_payment_pub_key_hash(nullptr), nullptr);
}

TEST(_cardano_get_payment_pub_key_hash, returnsTheCredential)
{
  verify_credential(basePaymentScriptStakeKey, true);
  verify_credential(basePaymentKeyStakeScript, false);
  verify_credential(basePaymentScriptStakeScript, true);
  verify_credential(testnetBasePaymentKeyStakeKey, false);
  verify_credential(testnetBasePaymentScriptStakeKey, true);
  verify_credential(testnetBasePaymentKeyStakeScript, false);
  verify_credential(testnetBasePaymentScriptStakeScript, true);
  verify_credential(testnetPointerKey, false);
  verify_credential(testnetPointerScript, true);
  verify_credential(testnetEnterpriseKey, false);
  verify_credential(testnetEnterpriseScript, true);
  verify_credential(testnetRewardKey, true);
  verify_credential(testnetRewardScript, true);
}

TEST(_cardano_get_payment_pub_key_hash, returnsNullIfMemoryAllocationFails)
{
  verify_memory_allocation_fail(basePaymentScriptStakeKey);
  verify_memory_allocation_fail(basePaymentKeyStakeScript);
  verify_memory_allocation_fail(basePaymentScriptStakeScript);
  verify_memory_allocation_fail(testnetBasePaymentKeyStakeKey);
  verify_memory_allocation_fail(testnetBasePaymentScriptStakeKey);
  verify_memory_allocation_fail(testnetBasePaymentKeyStakeScript);
  verify_memory_allocation_fail(testnetBasePaymentScriptStakeScript);
  verify_memory_allocation_fail(testnetPointerKey);
  verify_memory_allocation_fail(testnetPointerScript);
  verify_memory_allocation_fail(testnetEnterpriseKey);
  verify_memory_allocation_fail(testnetEnterpriseScript);
  verify_memory_allocation_fail(testnetRewardKey);
  verify_memory_allocation_fail(testnetRewardScript);
}

TEST(_cardano_add_input_signers, returnsErrorIfGivenNull)
{
  EXPECT_EQ(_cardano_add_input_signers(nullptr, nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(_cardano_add_input_signers, returnsSuccessIfVivenAnEmptyArray)
{
  cardano_blake2b_hash_set_t*      unique_signers  = nullptr;
  cardano_transaction_input_set_t* set             = nullptr;
  cardano_utxo_list_t*             resolved_inputs = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_set_new(&unique_signers), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_new(&set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_new(&resolved_inputs), CARDANO_SUCCESS);

  EXPECT_EQ(_cardano_add_input_signers(unique_signers, set, resolved_inputs), CARDANO_SUCCESS);

  cardano_blake2b_hash_set_unref(&unique_signers);
  cardano_transaction_input_set_unref(&set);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(_cardano_add_input_signers, returnsNotFoundIfItCantFindInput)
{
  cardano_blake2b_hash_set_t* unique_signers  = nullptr;
  cardano_utxo_list_t*        resolved_inputs = nullptr;

  cardano_transaction_t*           tx     = new_default_transaction(BALANCED_TX_CBOR);
  cardano_transaction_body_t*      body   = cardano_transaction_get_body(tx);
  cardano_transaction_input_set_t* inputs = cardano_transaction_body_get_inputs(body);

  cardano_transaction_unref(&tx);
  cardano_transaction_body_unref(&body);

  EXPECT_EQ(cardano_blake2b_hash_set_new(&unique_signers), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_new(&resolved_inputs), CARDANO_SUCCESS);

  EXPECT_EQ(_cardano_add_input_signers(unique_signers, inputs, resolved_inputs), CARDANO_ERROR_ELEMENT_NOT_FOUND);

  cardano_blake2b_hash_set_unref(&unique_signers);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_transaction_input_set_unref(&inputs);
}

TEST(_cardano_add_withdrawals, returnsErrorIfGivenNullUniqueSigners)
{
  EXPECT_EQ(_cardano_add_withdrawals(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(_cardano_add_withdrawals, returnsSuccessIfGivenNull)
{
  EXPECT_EQ(_cardano_add_withdrawals((cardano_blake2b_hash_set_t*)"", nullptr), CARDANO_SUCCESS);
}

TEST(_cardano_add_withdrawals, returnsSuccessIfGivenEmpty)
{
  cardano_blake2b_hash_set_t* unique_signers = nullptr;
  cardano_withdrawal_map_t*   withdrawals    = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_set_new(&unique_signers), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_withdrawal_map_new(&withdrawals), CARDANO_SUCCESS);

  EXPECT_EQ(_cardano_add_withdrawals(unique_signers, withdrawals), CARDANO_SUCCESS);

  cardano_blake2b_hash_set_unref(&unique_signers);
  cardano_withdrawal_map_unref(&withdrawals);
}

TEST(_cardano_add_withdrawals, returnsErrorIfMemoryAllocationFails)
{
  cardano_blake2b_hash_set_t* unique_signers = nullptr;
  cardano_withdrawal_map_t*   withdrawals    = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(WITHDRAWAL_CBOR, strlen(WITHDRAWAL_CBOR));
  EXPECT_EQ(cardano_withdrawal_map_from_cbor(reader, &withdrawals), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_blake2b_hash_set_new(&unique_signers), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(malloc, realloc, free);

  EXPECT_EQ(_cardano_add_withdrawals(unique_signers, withdrawals), CARDANO_SUCCESS);

  reset_allocators_run_count();

  cardano_set_allocators(fail_right_away_malloc, realloc, free);
  EXPECT_EQ(_cardano_add_withdrawals(unique_signers, withdrawals), CARDANO_ERROR_POINTER_IS_NULL);

  reset_allocators_run_count();

  cardano_set_allocators(fail_after_one_malloc, realloc, free);
  EXPECT_EQ(_cardano_add_withdrawals(unique_signers, withdrawals), CARDANO_ERROR_POINTER_IS_NULL);

  reset_allocators_run_count();

  cardano_set_allocators(fail_after_two_malloc, realloc, free);
  EXPECT_EQ(_cardano_add_withdrawals(unique_signers, withdrawals), CARDANO_ERROR_POINTER_IS_NULL);

  reset_allocators_run_count();
  cardano_set_allocators(malloc, realloc, free);

  cardano_blake2b_hash_set_unref(&unique_signers);
  cardano_withdrawal_map_unref(&withdrawals);
}

TEST(_process_credential, returnsErrorIfGivenNull)
{
  EXPECT_EQ(_process_credential(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(_process_credential, onlyAddPubKeyHashes)
{
  cardano_credential_t* pub_key_hash_cred = nullptr;
  cardano_credential_t* script_hash_cred  = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(KEY_HASH_CREDENTIAL_CBOR, strlen(KEY_HASH_CREDENTIAL_CBOR));
  EXPECT_EQ(cardano_credential_from_cbor(reader, &pub_key_hash_cred), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(SCRIPT_HASH_CREDENTIAL_CBOR, strlen(SCRIPT_HASH_CREDENTIAL_CBOR));
  EXPECT_EQ(cardano_credential_from_cbor(reader, &script_hash_cred), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  cardano_blake2b_hash_set_t* unique_signers = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_set_new(&unique_signers), CARDANO_SUCCESS);

  EXPECT_EQ(_process_credential(unique_signers, pub_key_hash_cred), CARDANO_SUCCESS);
  EXPECT_EQ(_process_credential(unique_signers, script_hash_cred), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_blake2b_hash_set_get_length(unique_signers), 1);

  cardano_blake2b_hash_t* hash = NULL;
  EXPECT_EQ(cardano_blake2b_hash_set_get(unique_signers, 0, &hash), CARDANO_SUCCESS);

  cardano_blake2b_hash_t* expected = cardano_credential_get_hash(pub_key_hash_cred);

  EXPECT_EQ(cardano_blake2b_hash_compare(hash, expected), 0);

  cardano_blake2b_hash_set_unref(&unique_signers);
  cardano_credential_unref(&pub_key_hash_cred);
  cardano_credential_unref(&script_hash_cred);
  cardano_blake2b_hash_unref(&hash);
  cardano_blake2b_hash_unref(&expected);
}

TEST(_process_credential, returnsErrorOnMemoryAllocationFail)
{
  cardano_credential_t* pub_key_hash_cred = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(KEY_HASH_CREDENTIAL_CBOR, strlen(KEY_HASH_CREDENTIAL_CBOR));
  EXPECT_EQ(cardano_credential_from_cbor(reader, &pub_key_hash_cred), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  cardano_blake2b_hash_set_t* unique_signers = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_set_new(&unique_signers), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  EXPECT_EQ(_process_credential(unique_signers, pub_key_hash_cred), CARDANO_ERROR_POINTER_IS_NULL);

  reset_allocators_run_count();

  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  EXPECT_EQ(_process_credential(unique_signers, pub_key_hash_cred), CARDANO_ERROR_POINTER_IS_NULL);

  reset_allocators_run_count();

  cardano_set_allocators(malloc, realloc, free);
  cardano_blake2b_hash_set_unref(&unique_signers);
  cardano_credential_unref(&pub_key_hash_cred);
}

TEST(_process_pool_registration, returnsErrorIfGivenNull)
{
  EXPECT_EQ(_process_pool_registration(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(_process_pool_registration, addCredential)
{
  cardano_certificate_t* certificate = new_default_certificate(CBOR_POOL_REGISTRATION);

  cardano_blake2b_hash_set_t* unique_signers = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_set_new(&unique_signers), CARDANO_SUCCESS);

  EXPECT_EQ(_process_pool_registration(unique_signers, certificate), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_blake2b_hash_set_get_length(unique_signers), 1);

  cardano_certificate_unref(&certificate);
  cardano_blake2b_hash_set_unref(&unique_signers);
}

TEST(_process_pool_retirement, returnsErrorIfGivenNull)
{
  EXPECT_EQ(_process_pool_retirement(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(_process_pool_retirement, addCredential)
{
  cardano_certificate_t* certificate = new_default_certificate(CBOR_POOL_RETIREMENT);

  cardano_blake2b_hash_set_t* unique_signers = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_set_new(&unique_signers), CARDANO_SUCCESS);

  EXPECT_EQ(_process_pool_retirement(unique_signers, certificate), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_blake2b_hash_set_get_length(unique_signers), 1);

  cardano_certificate_unref(&certificate);
  cardano_blake2b_hash_set_unref(&unique_signers);
}

TEST(_process_auth_committee_hot, returnsErrorIfGivenNull)
{
  EXPECT_EQ(_process_auth_committee_hot(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(_process_auth_committee_hot, addCredential)
{
  cardano_certificate_t* certificate = new_default_certificate(CBOR_AUTHORIZE_COMMITTEE_HOT);

  cardano_blake2b_hash_set_t* unique_signers = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_set_new(&unique_signers), CARDANO_SUCCESS);

  EXPECT_EQ(_process_auth_committee_hot(unique_signers, certificate), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_blake2b_hash_set_get_length(unique_signers), 1);

  cardano_certificate_unref(&certificate);
  cardano_blake2b_hash_set_unref(&unique_signers);
}

TEST(_cardano_voting_procedures_pub_key_hashes, returnsErrorIfGivenNull)
{
  EXPECT_EQ(_cardano_voting_procedures_pub_key_hashes(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(_cardano_get_certificate_credential, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_certificate_t* certificate = new_default_certificate(CBOR_REGISTER_DREP);
  cardano_credential_t*  credential  = nullptr;

  // Act & Assert
  EXPECT_EQ(_cardano_get_certificate_credential(certificate, CARDANO_CERT_TYPE_DREP_REGISTRATION, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(_cardano_get_certificate_credential(nullptr, CARDANO_CERT_TYPE_DREP_REGISTRATION, &credential), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(credential, nullptr);

  // Cleanup
  cardano_certificate_unref(&certificate);
}

TEST(_cardano_get_certificate_credential, returnsTheCredentialOfACertificate)
{
  // Arrange
  cardano_certificate_t* certificate = new_default_certificate(CBOR_STAKE_DELEGATION);
  cardano_credential_t*  credential  = nullptr;

  // Act
  cardano_error_t result = _cardano_get_certificate_credential(certificate, CARDANO_CERT_TYPE_STAKE_DELEGATION, &credential);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(credential, nullptr);

  // Cleanup
  cardano_credential_unref(&credential);
  cardano_certificate_unref(&certificate);
}

TEST(_cardano_get_certificate_credential, returnsNoCredentialForACertificateWithoutOne)
{
  // Arrange
  cardano_certificate_t* certificate = new_default_certificate(CBOR_AUTHORIZE_COMMITTEE_HOT);
  cardano_credential_t*  credential  = nullptr;

  // Act
  cardano_error_t result = _cardano_get_certificate_credential(certificate, CARDANO_CERT_TYPE_AUTH_COMMITTEE_HOT, &credential);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(credential, nullptr);

  // Cleanup
  cardano_certificate_unref(&certificate);
}

TEST(_process_certificate_with_credential, returnsErrorIfGivenNull)
{
  EXPECT_EQ(_process_certificate_with_credential(nullptr, nullptr, CARDANO_CERT_TYPE_DREP_UNREGISTRATION), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(_process_certificate_with_credential, processCertificates)
{
  EXPECT_EQ(validate_cert(CBOR_GENESIS_DELEGATION, CARDANO_CERT_TYPE_GENESIS_KEY_DELEGATION, 0), CARDANO_SUCCESS);
  EXPECT_EQ(validate_cert(CBOR_MIR, CARDANO_CERT_TYPE_MOVE_INSTANTANEOUS_REWARDS, 0), CARDANO_SUCCESS);
  EXPECT_EQ(validate_cert(CBOR_REGISTER_DREP, CARDANO_CERT_TYPE_DREP_REGISTRATION, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_cert(CBOR_UNREGISTER_DREP, CARDANO_CERT_TYPE_DREP_UNREGISTRATION, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_cert(CBOR_UPDATE_DREP, CARDANO_CERT_TYPE_UPDATE_DREP, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_cert(CBOR_POOL_REGISTRATION, CARDANO_CERT_TYPE_POOL_REGISTRATION, 0), CARDANO_SUCCESS);
  EXPECT_EQ(validate_cert(CBOR_POOL_RETIREMENT, CARDANO_CERT_TYPE_POOL_RETIREMENT, 0), CARDANO_SUCCESS);
  EXPECT_EQ(validate_cert(CBOR_REGISTRATION, CARDANO_CERT_TYPE_REGISTRATION, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_cert(CBOR_RESIGN_COMMITTEE_COLD, CARDANO_CERT_TYPE_RESIGN_COMMITTEE_COLD, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_cert(CBOR_STAKE_DELEGATION, CARDANO_CERT_TYPE_STAKE_DELEGATION, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_cert(CBOR_STAKE_DEREGISTRATION, CARDANO_CERT_TYPE_STAKE_DEREGISTRATION, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_cert(CBOR_STAKE_REGISTRATION, CARDANO_CERT_TYPE_STAKE_REGISTRATION, 0), CARDANO_SUCCESS);
  EXPECT_EQ(validate_cert(CBOR_STAKE_REGISTRATION_DELEGATION, CARDANO_CERT_TYPE_STAKE_REGISTRATION_DELEGATION, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_cert(CBOR_STAKE_VOTE_DELEGATION, CARDANO_CERT_TYPE_STAKE_VOTE_DELEGATION, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_cert(CBOR_STAKE_VOTE_REGISTRATION_DELEGATION, CARDANO_CERT_TYPE_STAKE_VOTE_REGISTRATION_DELEGATION, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_cert(CBOR_VOTE_DELEGATION, CARDANO_CERT_TYPE_VOTE_DELEGATION, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_cert(CBOR_VOTE_REGISTRATION_DELEGATION, CARDANO_CERT_TYPE_VOTE_REGISTRATION_DELEGATION, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_cert(CBOR_AUTHORIZE_COMMITTEE_HOT, CARDANO_CERT_TYPE_AUTH_COMMITTEE_HOT, 0), CARDANO_SUCCESS);
  EXPECT_EQ(validate_cert(CBOR_UNREGISTRATION, CARDANO_CERT_TYPE_UNREGISTRATION, 1), CARDANO_SUCCESS);
}

TEST(_process_certificate_with_credential, errorOnMemoryAllocationFailure)
{
  EXPECT_EQ(validate_cert_memory_alloc_error(CBOR_REGISTER_DREP, CARDANO_CERT_TYPE_DREP_REGISTRATION), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(validate_cert_memory_alloc_error(CBOR_UNREGISTER_DREP, CARDANO_CERT_TYPE_DREP_UNREGISTRATION), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(validate_cert_memory_alloc_error(CBOR_UPDATE_DREP, CARDANO_CERT_TYPE_UPDATE_DREP), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(validate_cert_memory_alloc_error(CBOR_REGISTRATION, CARDANO_CERT_TYPE_REGISTRATION), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(validate_cert_memory_alloc_error(CBOR_RESIGN_COMMITTEE_COLD, CARDANO_CERT_TYPE_RESIGN_COMMITTEE_COLD), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(validate_cert_memory_alloc_error(CBOR_STAKE_DELEGATION, CARDANO_CERT_TYPE_STAKE_DELEGATION), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(validate_cert_memory_alloc_error(CBOR_STAKE_DEREGISTRATION, CARDANO_CERT_TYPE_STAKE_DEREGISTRATION), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(validate_cert_memory_alloc_error(CBOR_STAKE_REGISTRATION_DELEGATION, CARDANO_CERT_TYPE_STAKE_REGISTRATION_DELEGATION), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(validate_cert_memory_alloc_error(CBOR_STAKE_VOTE_DELEGATION, CARDANO_CERT_TYPE_STAKE_VOTE_DELEGATION), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(validate_cert_memory_alloc_error(CBOR_STAKE_VOTE_REGISTRATION_DELEGATION, CARDANO_CERT_TYPE_STAKE_VOTE_REGISTRATION_DELEGATION), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(validate_cert_memory_alloc_error(CBOR_VOTE_DELEGATION, CARDANO_CERT_TYPE_VOTE_DELEGATION), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(validate_cert_memory_alloc_error(CBOR_VOTE_REGISTRATION_DELEGATION, CARDANO_CERT_TYPE_VOTE_REGISTRATION_DELEGATION), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(validate_cert_memory_alloc_error(CBOR_UNREGISTRATION, CARDANO_CERT_TYPE_UNREGISTRATION), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(_cardano_voting_procedures_pub_key_hashes, returnsErrorIfMemoryAllocationFails)
{
  cardano_cbor_reader_t*       reader     = cardano_cbor_reader_from_hex(VOTING_PROCEDURES_CBOR, strlen(VOTING_PROCEDURES_CBOR));
  cardano_voting_procedures_t* procedures = nullptr;

  EXPECT_EQ(cardano_voting_procedures_from_cbor(reader, &procedures), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  EXPECT_EQ(_cardano_voting_procedures_pub_key_hashes(nullptr, procedures), CARDANO_ERROR_POINTER_IS_NULL);

  reset_allocators_run_count();

  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  EXPECT_EQ(_cardano_voting_procedures_pub_key_hashes(nullptr, procedures), CARDANO_ERROR_POINTER_IS_NULL);

  reset_allocators_run_count();

  cardano_set_allocators(fail_after_two_malloc, realloc, free);

  EXPECT_EQ(_cardano_voting_procedures_pub_key_hashes(nullptr, procedures), CARDANO_ERROR_POINTER_IS_NULL);

  reset_allocators_run_count();

  cardano_set_allocators(malloc, realloc, free);
  cardano_voting_procedures_unref(&procedures);
  cardano_cbor_reader_unref(&reader);
}

TEST(_cardano_voting_procedures_pub_key_hashes, returnsErrorIfGivenNullProcedures)
{
  EXPECT_EQ(_cardano_voting_procedures_pub_key_hashes(nullptr, (cardano_voting_procedures_t*)""), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(_cardano_voting_procedures_pub_key_hashes((cardano_blake2b_hash_set_t*)"", nullptr), CARDANO_SUCCESS);
  EXPECT_EQ(_cardano_voting_procedures_pub_key_hashes(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(_cardano_voting_procedures_pub_key_hashes, canAddHashes)
{
  cardano_cbor_reader_t*       reader     = cardano_cbor_reader_from_hex(VOTING_PROCEDURES_CBOR, strlen(VOTING_PROCEDURES_CBOR));
  cardano_voting_procedures_t* procedures = nullptr;

  EXPECT_EQ(cardano_voting_procedures_from_cbor(reader, &procedures), CARDANO_SUCCESS);

  cardano_blake2b_hash_set_t* unique_signers = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_set_new(&unique_signers), CARDANO_SUCCESS);

  EXPECT_EQ(_cardano_voting_procedures_pub_key_hashes(unique_signers, procedures), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_blake2b_hash_set_get_length(unique_signers), 1);

  cardano_voting_procedures_unref(&procedures);
  cardano_blake2b_hash_set_unref(&unique_signers);
  cardano_cbor_reader_unref(&reader);
}

TEST(_cardano_voting_procedures_pub_key_hashes, canWorkWithEmptyProcedures)
{
  cardano_cbor_reader_t*       reader     = cardano_cbor_reader_from_hex(EMPTY_VOTING_PROCEDURES_CBOR, strlen(EMPTY_VOTING_PROCEDURES_CBOR));
  cardano_voting_procedures_t* procedures = nullptr;

  EXPECT_EQ(cardano_voting_procedures_from_cbor(reader, &procedures), CARDANO_SUCCESS);

  cardano_blake2b_hash_set_t* unique_signers = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_set_new(&unique_signers), CARDANO_SUCCESS);

  EXPECT_EQ(_cardano_voting_procedures_pub_key_hashes(unique_signers, procedures), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_blake2b_hash_set_get_length(unique_signers), 0);

  cardano_voting_procedures_unref(&procedures);
  cardano_blake2b_hash_set_unref(&unique_signers);
  cardano_cbor_reader_unref(&reader);
}

TEST(_cardano_add_certificates_pub_key_hashes, canAddCertificateCerts)
{
  EXPECT_EQ(validate_add_cert(CBOR_GENESIS_DELEGATION, 0), CARDANO_SUCCESS);
  EXPECT_EQ(validate_add_cert(CBOR_MIR, 0), CARDANO_SUCCESS);
  EXPECT_EQ(validate_add_cert(CBOR_REGISTER_DREP, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_add_cert(CBOR_UNREGISTER_DREP, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_add_cert(CBOR_UPDATE_DREP, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_add_cert(CBOR_POOL_REGISTRATION, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_add_cert(CBOR_POOL_RETIREMENT, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_add_cert(CBOR_REGISTRATION, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_add_cert(CBOR_RESIGN_COMMITTEE_COLD, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_add_cert(CBOR_STAKE_DELEGATION, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_add_cert(CBOR_STAKE_DEREGISTRATION, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_add_cert(CBOR_STAKE_REGISTRATION, 0), CARDANO_SUCCESS);
  EXPECT_EQ(validate_add_cert(CBOR_STAKE_REGISTRATION_DELEGATION, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_add_cert(CBOR_STAKE_VOTE_DELEGATION, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_add_cert(CBOR_STAKE_VOTE_REGISTRATION_DELEGATION, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_add_cert(CBOR_VOTE_DELEGATION, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_add_cert(CBOR_VOTE_REGISTRATION_DELEGATION, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_add_cert(CBOR_AUTHORIZE_COMMITTEE_HOT, 1), CARDANO_SUCCESS);
  EXPECT_EQ(validate_add_cert(CBOR_UNREGISTRATION, 1), CARDANO_SUCCESS);
}

TEST(_cardano_add_certificates_pub_key_hashes, returnsErrorIfGivenNull)
{
  EXPECT_EQ(_cardano_add_certificates_pub_key_hashes(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(_cardano_add_certificates_pub_key_hashes, returnsSuccessIfGivenNull)
{
  EXPECT_EQ(_cardano_add_certificates_pub_key_hashes((cardano_blake2b_hash_set_t*)"", nullptr), CARDANO_SUCCESS);
}

TEST(_cardano_add_certificates_pub_key_hashes, returnsSuccessIfGivenEmpty)
{
  cardano_certificate_set_t*  certificates   = nullptr;
  cardano_blake2b_hash_set_t* unique_signers = nullptr;

  EXPECT_EQ(cardano_certificate_set_new(&certificates), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_set_new(&unique_signers), CARDANO_SUCCESS);

  EXPECT_EQ(_cardano_add_certificates_pub_key_hashes(unique_signers, certificates), CARDANO_SUCCESS);

  cardano_certificate_set_unref(&certificates);
  cardano_blake2b_hash_set_unref(&unique_signers);
}

TEST(_cardano_get_unique_signers, returnsErrorIfGivenNull)
{
  EXPECT_EQ(_cardano_get_unique_signers(nullptr, nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(_cardano_get_payment_pub_key_hash, returnsNullForAByronAddress)
{
  verify_credential(byronMainnetYoroi, true);
  verify_credential(byronTestnetDaedalus, true);
}

TEST(_cardano_get_bootstrap_key_hash, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_address_t*      address = new_address(byronMainnetYoroi);
  cardano_blake2b_hash_t* root    = nullptr;

  // Act
  cardano_error_t null_address_result = _cardano_get_bootstrap_key_hash(nullptr, &root);
  cardano_error_t null_root_result    = _cardano_get_bootstrap_key_hash(address, nullptr);

  // Assert
  EXPECT_EQ(null_address_result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(null_root_result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_address_unref(&address);
}

TEST(_cardano_get_bootstrap_key_hash, returnsNoRootIfTheAddressIsNotAByronAddress)
{
  // Arrange
  cardano_address_t*      address = new_address(testnetEnterpriseKey);
  cardano_blake2b_hash_t* root    = nullptr;

  // Act
  cardano_error_t result = _cardano_get_bootstrap_key_hash(address, &root);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(root, nullptr);

  // Cleanup
  cardano_address_unref(&address);
}

TEST(_cardano_get_bootstrap_key_hash, returnsTheRootOfTheByronAddress)
{
  // Arrange
  cardano_address_t*      yoroi_address    = new_address(byronMainnetYoroi);
  cardano_address_t*      daedalus_address = new_address(byronTestnetDaedalus);
  cardano_blake2b_hash_t* yoroi_root       = nullptr;
  cardano_blake2b_hash_t* daedalus_root    = nullptr;

  // Act
  cardano_error_t yoroi_result    = _cardano_get_bootstrap_key_hash(yoroi_address, &yoroi_root);
  cardano_error_t daedalus_result = _cardano_get_bootstrap_key_hash(daedalus_address, &daedalus_root);

  // Assert
  EXPECT_EQ(yoroi_result, CARDANO_SUCCESS);
  EXPECT_EQ(daedalus_result, CARDANO_SUCCESS);
  EXPECT_EQ(get_hash_hex(yoroi_root), BYRON_YOROI_ROOT_HEX);
  EXPECT_EQ(get_hash_hex(daedalus_root), BYRON_DAEDALUS_ROOT_HEX);

  // Cleanup
  cardano_blake2b_hash_unref(&yoroi_root);
  cardano_blake2b_hash_unref(&daedalus_root);
  cardano_address_unref(&yoroi_address);
  cardano_address_unref(&daedalus_address);
}

TEST(_cardano_get_bootstrap_key_hash, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_address_t*      address = new_address(byronTestnetDaedalus);
  cardano_blake2b_hash_t* root    = nullptr;

  // Act
  for (int i = 0; i < 3; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_error_t result = _cardano_get_bootstrap_key_hash(address, &root);

    // Assert
    EXPECT_NE(result, CARDANO_SUCCESS);
    EXPECT_EQ(root, nullptr);
  }

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_set_allocators(malloc, realloc, free);
  cardano_address_unref(&address);
}

TEST(_cardano_get_bootstrap_witness_attributes, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_address_t* address    = new_address(byronMainnetYoroi);
  cardano_buffer_t*  attributes = nullptr;

  // Act
  cardano_error_t null_address_result    = _cardano_get_bootstrap_witness_attributes(nullptr, &attributes);
  cardano_error_t null_attributes_result = _cardano_get_bootstrap_witness_attributes(address, nullptr);

  // Assert
  EXPECT_EQ(null_address_result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(null_attributes_result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_address_unref(&address);
}

TEST(_cardano_get_bootstrap_witness_attributes, returnsErrorIfTheAddressIsNotAByronAddress)
{
  // Arrange
  cardano_address_t* address    = new_address(testnetEnterpriseKey);
  cardano_buffer_t*  attributes = nullptr;

  // Act
  cardano_error_t result = _cardano_get_bootstrap_witness_attributes(address, &attributes);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ADDRESS_TYPE);
  EXPECT_EQ(attributes, nullptr);

  // Cleanup
  cardano_address_unref(&address);
}

TEST(_cardano_get_bootstrap_witness_attributes, returnsTheEncodedAttributesOfTheAddress)
{
  // Arrange
  cardano_address_t* yoroi_address       = new_address(byronMainnetYoroi);
  cardano_address_t* daedalus_address    = new_address(byronTestnetDaedalus);
  cardano_buffer_t*  yoroi_attributes    = nullptr;
  cardano_buffer_t*  daedalus_attributes = nullptr;

  // Act
  cardano_error_t yoroi_result    = _cardano_get_bootstrap_witness_attributes(yoroi_address, &yoroi_attributes);
  cardano_error_t daedalus_result = _cardano_get_bootstrap_witness_attributes(daedalus_address, &daedalus_attributes);

  // Assert
  EXPECT_EQ(yoroi_result, CARDANO_SUCCESS);
  EXPECT_EQ(daedalus_result, CARDANO_SUCCESS);
  EXPECT_EQ(get_buffer_hex(yoroi_attributes), BYRON_YOROI_ATTRIBUTES_HEX);
  EXPECT_EQ(get_buffer_hex(daedalus_attributes), BYRON_DAEDALUS_ATTRIBUTES_HEX);

  // Cleanup
  cardano_buffer_unref(&yoroi_attributes);
  cardano_buffer_unref(&daedalus_attributes);
  cardano_address_unref(&yoroi_address);
  cardano_address_unref(&daedalus_address);
}

TEST(_cardano_get_bootstrap_witness_attributes, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_address_t* address    = new_address(byronTestnetDaedalus);
  cardano_buffer_t*  attributes = nullptr;

  // Act
  for (int i = 0; i < 4; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_error_t result = _cardano_get_bootstrap_witness_attributes(address, &attributes);

    // Assert
    EXPECT_NE(result, CARDANO_SUCCESS);
    EXPECT_EQ(attributes, nullptr);
  }

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_set_allocators(malloc, realloc, free);
  cardano_address_unref(&address);
}

TEST(_cardano_add_input_bootstrap_signers, returnsErrorIfGivenNull)
{
  EXPECT_EQ(_cardano_add_input_bootstrap_signers(nullptr, nullptr, nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(_cardano_add_input_bootstrap_signers, returnsNotFoundIfItCantFindInput)
{
  // Arrange
  cardano_blake2b_hash_set_t*      roots             = nullptr;
  cardano_utxo_list_t*             bootstrap_signers = nullptr;
  cardano_utxo_list_t*             resolved_inputs   = nullptr;
  cardano_transaction_t*           tx                = new_default_transaction(BALANCED_TX_CBOR);
  cardano_transaction_body_t*      body              = cardano_transaction_get_body(tx);
  cardano_transaction_input_set_t* inputs            = cardano_transaction_body_get_inputs(body);

  EXPECT_EQ(cardano_blake2b_hash_set_new(&roots), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_new(&bootstrap_signers), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_new(&resolved_inputs), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = _cardano_add_input_bootstrap_signers(roots, bootstrap_signers, inputs, resolved_inputs);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ELEMENT_NOT_FOUND);

  // Cleanup
  cardano_blake2b_hash_set_unref(&roots);
  cardano_utxo_list_unref(&bootstrap_signers);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_body_unref(&body);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_bootstrap_signers, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_transaction_t* tx                = new_default_transaction(BALANCED_TX_CBOR);
  cardano_utxo_list_t*   bootstrap_signers = nullptr;

  // Act
  cardano_error_t null_tx_result      = _cardano_get_bootstrap_signers(nullptr, nullptr, &bootstrap_signers);
  cardano_error_t null_signers_result = _cardano_get_bootstrap_signers(tx, nullptr, nullptr);

  // Assert
  EXPECT_EQ(null_tx_result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(null_signers_result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_bootstrap_signers, returnsAnEmptyListIfTheInputsAreNotResolved)
{
  // Arrange
  cardano_transaction_t* tx                = new_default_transaction(BALANCED_TX_CBOR);
  cardano_utxo_list_t*   bootstrap_signers = nullptr;

  // Act
  cardano_error_t result = _cardano_get_bootstrap_signers(tx, nullptr, &bootstrap_signers);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_get_length(bootstrap_signers), 0U);

  // Cleanup
  cardano_utxo_list_unref(&bootstrap_signers);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_bootstrap_signers, returnsNotFoundIfItCantFindInput)
{
  // Arrange
  cardano_transaction_t* tx                = new_default_transaction(BALANCED_TX_CBOR);
  cardano_utxo_list_t*   resolved_inputs   = nullptr;
  cardano_utxo_list_t*   bootstrap_signers = nullptr;

  EXPECT_EQ(cardano_utxo_list_new(&resolved_inputs), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = _cardano_get_bootstrap_signers(tx, resolved_inputs, &bootstrap_signers);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_EQ(bootstrap_signers, nullptr);

  // Cleanup
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_bootstrap_signers, countsTheOwnerOfOneByronInput)
{
  // Arrange
  cardano_transaction_t* tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_utxo_list_t*   resolved_inputs = new_resolved_inputs(tx, byronMainnetYoroi, testnetEnterpriseScript);

  // Act
  signer_counts_t counts = get_signer_counts(tx, resolved_inputs);

  // Assert
  EXPECT_EQ(counts.vkey_signers, 0U);
  EXPECT_EQ(counts.bootstrap_signers, 1U);

  // Cleanup
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_bootstrap_signers, countsOneOwnerForTwoInputsOfTheSameByronAddress)
{
  // Arrange
  cardano_transaction_t* tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_utxo_list_t*   resolved_inputs = new_resolved_inputs(tx, byronTestnetDaedalus, byronTestnetDaedalus);

  // Act
  signer_counts_t counts = get_signer_counts(tx, resolved_inputs);

  // Assert
  EXPECT_EQ(counts.vkey_signers, 0U);
  EXPECT_EQ(counts.bootstrap_signers, 1U);

  // Cleanup
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_bootstrap_signers, countsEachDistinctByronOwner)
{
  // Arrange
  cardano_transaction_t* tx                = new_default_transaction(BALANCED_TX_CBOR);
  cardano_utxo_list_t*   resolved_inputs   = new_resolved_inputs(tx, byronMainnetYoroi, byronTestnetDaedalus);
  cardano_utxo_list_t*   bootstrap_signers = nullptr;

  // Act
  signer_counts_t counts = get_signer_counts(tx, resolved_inputs);

  EXPECT_EQ(_cardano_get_bootstrap_signers(tx, resolved_inputs, &bootstrap_signers), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(counts.vkey_signers, 0U);
  EXPECT_EQ(counts.bootstrap_signers, 2U);
  EXPECT_EQ(get_utxo_root_hex(bootstrap_signers, 0U), BYRON_YOROI_ROOT_HEX);
  EXPECT_EQ(get_utxo_root_hex(bootstrap_signers, 1U), BYRON_DAEDALUS_ROOT_HEX);

  // Cleanup
  cardano_utxo_list_unref(&bootstrap_signers);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_bootstrap_signers, countsByronOwnersApartFromVkeySigners)
{
  // Arrange
  cardano_transaction_t* tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_utxo_list_t*   resolved_inputs = new_resolved_inputs(tx, byronMainnetYoroi, testnetEnterpriseKey);

  // Act
  signer_counts_t counts = get_signer_counts(tx, resolved_inputs);

  // Assert
  EXPECT_EQ(counts.vkey_signers, 1U);
  EXPECT_EQ(counts.bootstrap_signers, 1U);

  // Cleanup
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_bootstrap_signers, countsNoOwnerIfNoInputIsHeldByAByronAddress)
{
  // Arrange
  cardano_transaction_t* tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_utxo_list_t*   resolved_inputs = new_resolved_inputs(tx, testnetEnterpriseKey, testnetBasePaymentKeyStakeKey);

  // Act
  signer_counts_t counts = get_signer_counts(tx, resolved_inputs);

  // Assert
  EXPECT_EQ(counts.vkey_signers, 1U);
  EXPECT_EQ(counts.bootstrap_signers, 0U);

  // Cleanup
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_bootstrap_signers, countsTheOwnerOfAByronCollateralInput)
{
  // Arrange
  cardano_transaction_t*           tx               = new_default_transaction(BALANCED_TX_CBOR);
  cardano_utxo_list_t*             resolved_inputs  = new_resolved_inputs(tx, byronMainnetYoroi, byronMainnetYoroi);
  cardano_transaction_body_t*      body             = cardano_transaction_get_body(tx);
  cardano_transaction_input_set_t* collateral       = nullptr;
  cardano_transaction_input_t*     collateral_input = nullptr;
  cardano_blake2b_hash_t*          collateral_id    = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(COLLATERAL_TX_ID_HEX, strlen(COLLATERAL_TX_ID_HEX), &collateral_id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_new(collateral_id, 0U, &collateral_input), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_new(&collateral), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_add(collateral, collateral_input), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_collateral(body, collateral), CARDANO_SUCCESS);

  add_resolved_input(resolved_inputs, collateral_input, byronTestnetDaedalus);

  // Act
  signer_counts_t counts = get_signer_counts(tx, resolved_inputs);

  // Assert
  EXPECT_EQ(counts.vkey_signers, 0U);
  EXPECT_EQ(counts.bootstrap_signers, 2U);

  // Cleanup
  cardano_blake2b_hash_unref(&collateral_id);
  cardano_transaction_input_unref(&collateral_input);
  cardano_transaction_input_set_unref(&collateral);
  cardano_transaction_body_unref(&body);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_bootstrap_signers, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_transaction_t* tx                = new_default_transaction(BALANCED_TX_CBOR);
  cardano_utxo_list_t*   resolved_inputs   = new_resolved_inputs(tx, byronMainnetYoroi, byronTestnetDaedalus);
  cardano_utxo_list_t*   bootstrap_signers = nullptr;

  // Act
  for (int i = 0; i < 6; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_error_t result = _cardano_get_bootstrap_signers(tx, resolved_inputs, &bootstrap_signers);

    // Assert
    EXPECT_NE(result, CARDANO_SUCCESS);
    EXPECT_EQ(bootstrap_signers, nullptr);
  }

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_set_allocators(malloc, realloc, free);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_transaction_unref(&tx);
}
