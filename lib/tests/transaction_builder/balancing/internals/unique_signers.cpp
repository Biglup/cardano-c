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
  cardano_set_allocators(_cardano_malloc, _cardano_realloc, _cardano_free);

  EXPECT_EQ(cardano_address_from_string(addr.c_str(), addr.length(), &address), CARDANO_SUCCESS);

  reset_allocators_run_count();

  cardano_set_allocators(fail_right_away_malloc, _cardano_realloc, _cardano_free);
  hash = _cardano_get_payment_pub_key_hash(address);

  EXPECT_EQ(hash, nullptr);

  reset_allocators_run_count();

  cardano_set_allocators(fail_after_one_malloc, _cardano_realloc, _cardano_free);
  hash = _cardano_get_payment_pub_key_hash(address);

  EXPECT_EQ(hash, nullptr);

  reset_allocators_run_count();

  cardano_set_allocators(fail_after_two_malloc, _cardano_realloc, _cardano_free);
  hash = _cardano_get_payment_pub_key_hash(address);

  EXPECT_EQ(hash, nullptr);

  reset_allocators_run_count();

  cardano_set_allocators(fail_after_three_malloc, _cardano_realloc, _cardano_free);
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
  cardano_set_allocators(fail_right_away_malloc, _cardano_realloc, _cardano_free);

  cardano_error_t result = _process_certificate_with_credential(unique_signers, certificate, type);

  EXPECT_EQ(cardano_blake2b_hash_set_get_length(unique_signers), 0U);

  cardano_certificate_unref(&certificate);
  cardano_blake2b_hash_set_unref(&unique_signers);
  cardano_set_allocators(malloc, _cardano_realloc, _cardano_free);

  return result;
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
  cardano_set_allocators(_cardano_malloc, _cardano_realloc, _cardano_free);

  EXPECT_EQ(_cardano_add_withdrawals(unique_signers, withdrawals), CARDANO_SUCCESS);

  reset_allocators_run_count();

  cardano_set_allocators(fail_right_away_malloc, _cardano_realloc, _cardano_free);
  EXPECT_EQ(_cardano_add_withdrawals(unique_signers, withdrawals), CARDANO_ERROR_POINTER_IS_NULL);

  reset_allocators_run_count();

  cardano_set_allocators(fail_after_one_malloc, _cardano_realloc, _cardano_free);
  EXPECT_EQ(_cardano_add_withdrawals(unique_signers, withdrawals), CARDANO_ERROR_POINTER_IS_NULL);

  reset_allocators_run_count();

  cardano_set_allocators(fail_after_two_malloc, _cardano_realloc, _cardano_free);
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
