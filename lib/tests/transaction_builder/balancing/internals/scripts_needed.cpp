/**
 * \file scripts_needed.cpp
 *
 * \author angel.castillo
 * \date   Sep 23, 2026
 *
 * \section LICENSE
 *
 * Copyright 2026 Biglup Labs
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
#include "../../../../src/transaction_builder/balancing/internals/scripts_needed.h"
}

#include <allocators.h>
#include <cardano/address/enterprise_address.h>
#include <cardano/common/guard_set.h>
#include <cardano/common/utxo.h>
#include <cardano/proposal_procedures/proposal_procedure.h>
#include <cardano/proposal_procedures/proposal_procedure_set.h>
#include <cardano/scripts/script.h>
#include <gmock/gmock.h>

#include <string>

/* CONSTANTS *****************************************************************/

static const char* BALANCED_TX_CBOR = "84a300d9010282825820027b68d4c11e97d7e065cc2702912cb1a21b6d0e56c6a74dd605889a5561138500825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a633010182a2005839005cf6c91279a859a072601779fb33bb07c34e1d641d45df51ff63b967f15db05f56035465bf8900a09bdaa16c3d8b8244fea686524408dd8001821a00e4e1c0a1581c0b0d621b5c26d0a1fd0893a4b04c19d860296a69ede1fbcfc5179882a1474e46542d30303101a200583900dc435fc2638f6684bd1f9f6f917d80c92ae642a4a33a412e516479e64245236ab8056760efceebbff57e8cab220182be3e36439e520a6454011a0d294e28021a00029eb9a0f5f6";

static const char* PLUTUS_V1_SCRIPT_CBOR = "82014e4d01000033222220051200120011";
static const char* PLUTUS_V2_SCRIPT_CBOR = "82024e4d02000033222220051200120011";
static const char* PLUTUS_V3_SCRIPT_CBOR = "82034e4d03000033222220051200120011";
static const char* PLUTUS_V4_SCRIPT_CBOR = "82044e4d04000033222220051200120011";
static const char* NATIVE_SCRIPT_CBOR    = "82008200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d398538";

static const char* WITHDRAWAL_CBOR        = "a2581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d005581df1c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f05";
static const char* WITHDRAWAL_SCRIPT_HASH = "c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f";
static const char* KEY_WITHDRAWAL_CBOR    = "a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d005";
static const char* GUARDS_CBOR            = "d90102828200581c000000000000000000000000000000000000000000000000000000018201581c00000000000000000000000000000000000000000000000000000002";
static const char* GUARD_SCRIPT_HASH      = "00000000000000000000000000000000000000000000000000000002";
static const char* ZERO_HASH              = "00000000000000000000000000000000000000000000000000000000";

static const char* CBOR_AUTHORIZE_COMMITTEE_HOT            = "830e8200581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000001";
static const char* CBOR_GENESIS_DELEGATION                 = "8405581c00010001000100010001000100010001000100010001000100010001581c0002000200020002000200020002000200020002000200020002000258200003000300030003000300030003000300030003000300030003000300030003";
static const char* CBOR_MIR                                = "820682001a000f4240";
static const char* CBOR_POOL_RETIREMENT                    = "8304581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef921903e8";
static const char* CBOR_REGISTER_DREP                      = "84108200581c0000000000000000000000000000000000000000000000000000000000f6";
static const char* CBOR_REGISTRATION                       = "83078200581c0000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_RESIGN_COMMITTEE_COLD              = "830f8200581c00000000000000000000000000000000000000000000000000000000f6";
static const char* CBOR_STAKE_DELEGATION                   = "83028200581c00000000000000000000000000000000000000000000000000000000581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef92";
static const char* CBOR_STAKE_DEREGISTRATION               = "82018200581c00000000000000000000000000000000000000000000000000000000";
static const char* CBOR_STAKE_REGISTRATION                 = "82008200581c00000000000000000000000000000000000000000000000000000000";
static const char* CBOR_STAKE_REGISTRATION_DELEGATION      = "840b8200581c00000000000000000000000000000000000000000000000000000000581c0000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_STAKE_VOTE_DELEGATION              = "840a8200581c00000000000000000000000000000000000000000000000000000000581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000";
static const char* CBOR_STAKE_VOTE_REGISTRATION_DELEGATION = "850d8200581c00000000000000000000000000000000000000000000000000000000581c000000000000000000000000000000000000000000000000000000008200581c0000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_UNREGISTER_DREP                    = "83118200581c0000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_UNREGISTRATION                     = "83088200581c0000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_UPDATE_DREP                        = "83128200581c00000000000000000000000000000000000000000000000000000000827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_VOTE_DELEGATION                    = "83098200581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000";
static const char* CBOR_VOTE_REGISTRATION_DELEGATION       = "840c8200581c000000000000000000000000000000000000000000000000000000008200581c0000000000000000000000000000000000000000000000000000000000";

static const char* VOTING_PROCEDURES_CBOR = "a28202581c10000000000000000000000000000000000000000000000000000000a38258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258202000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008203581c20000000000000000000000000000000000000000000000000000000a28258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000000000000000000008258203000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* VOTER_SCRIPT_HASH      = "20000000000000000000000000000000000000000000000000000000";
static const char* KEY_VOTER_CBOR         = "a18202581c10000000000000000000000000000000000000000000000000000000a18258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* CC_SCRIPT_VOTER_CBOR   = "a18201581c30000000000000000000000000000000000000000000000000000000a18258201000000000000000000000000000000000000000000000000000000000000000038200827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* CC_SCRIPT_VOTER_HASH   = "30000000000000000000000000000000000000000000000000000000";

static const char* PARAMETER_CHANGE_PROPOSAL_CBOR     = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8400825820000000000000000000000000000000000000000000000000000000000000000003b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* TREASURY_WITHDRAWALS_PROPOSAL_CBOR = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8302a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f01581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* UNGUARDED_TREASURY_PROPOSAL_CBOR   = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8302a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f01f6827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* NO_CONFIDENCE_PROPOSAL_CBOR        = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8203825820000000000000000000000000000000000000000000000000000000000000000003827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* INFO_PROPOSAL_CBOR                 = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8106827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* GUARDRAILS_SCRIPT_HASH             = "8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d";

static const std::string basePaymentScriptStakeKey    = "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg";
static const std::string basePaymentKeyStakeScript    = "addr_test1yz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzerkr0vd4msrxnuwnccdxlhdjar77j6lg0wypcc9uar5d2shsf5r8qx";
static const std::string basePaymentScriptStakeScript = "addr_test1xrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gt7r0vd4msrxnuwnccdxlhdjar77j6lg0wypcc9uar5d2shs4p04xh";
static const std::string basePaymentKeyStakeKey       = "addr_test1qz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgs68faae";
static const std::string pointerKey                   = "addr_test1gz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer5pnz75xxcrdw5vky";
static const std::string pointerScript                = "addr_test12rphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtupnz75xxcryqrvmw";
static const std::string enterpriseKey                = "addr_test1vz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzerspjrlsz";
static const std::string enterpriseScript             = "addr_test1wrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcl6szpr";
static const std::string rewardKey                    = "stake_test1uqehkck0lajq8gr28t9uxnuvgcqrc6070x3k9r8048z8y5gssrtvn";
static const std::string rewardScript                 = "stake_test17rphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcljw6kf";
static const std::string byron                        = "Ae2tdPwUPEZFRbyhz3cpfC2CumGzNkFBN2L42rcUc2yjQpEkxDbkPodpMAi";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a CBOR reader over a hexadecimal string.
 * \param hex the hexadecimal string.
 * \return A new instance of the reader.
 */
static cardano_cbor_reader_t*
new_reader(const char* hex)
{
  return cardano_cbor_reader_from_hex(hex, strlen(hex));
}

/**
 * Creates an address from its string representation.
 * \param address the address string.
 * \return A new instance of the address.
 */
static cardano_address_t*
new_address(const std::string& address)
{
  cardano_address_t* result = nullptr;

  EXPECT_EQ(cardano_address_from_string(address.c_str(), address.length(), &result), CARDANO_SUCCESS);

  return result;
}

/**
 * Creates a script from its CBOR representation.
 * \param cbor the CBOR of the script.
 * \return A new instance of the script.
 */
static cardano_script_t*
new_script(const char* cbor)
{
  cardano_cbor_reader_t* reader = new_reader(cbor);
  cardano_script_t*      script = nullptr;

  EXPECT_EQ(cardano_script_from_cbor(reader, &script), CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return script;
}

/**
 * Creates a hash from its hexadecimal representation.
 * \param hex the hexadecimal hash.
 * \return A new instance of the hash.
 */
static cardano_blake2b_hash_t*
new_hash(const char* hex)
{
  cardano_blake2b_hash_t* hash = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(hex, strlen(hex), &hash), CARDANO_SUCCESS);

  return hash;
}

/**
 * Creates the enterprise address locked by a script.
 * \param script the script that locks the address.
 * \return A new instance of the address.
 */
static cardano_address_t*
new_script_address(cardano_script_t* script)
{
  cardano_blake2b_hash_t*       hash       = cardano_script_get_hash(script);
  cardano_credential_t*         credential = nullptr;
  cardano_enterprise_address_t* enterprise = nullptr;

  EXPECT_EQ(cardano_credential_new(hash, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, &credential), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_enterprise_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, credential, &enterprise), CARDANO_SUCCESS);

  cardano_address_t* address = cardano_enterprise_address_to_address(enterprise);

  cardano_enterprise_address_unref(&enterprise);
  cardano_credential_unref(&credential);
  cardano_blake2b_hash_unref(&hash);

  return address;
}

/**
 * Creates a UTXO paid to an address, optionally carrying a reference script.
 * \param ordinal a number that makes the transaction id of the UTXO unique.
 * \param address the address that owns the UTXO.
 * \param script_ref the reference script of the UTXO, or NULL.
 * \return A new instance of the UTXO.
 */
static cardano_utxo_t*
new_utxo(const uint64_t ordinal, cardano_address_t* address, cardano_script_t* script_ref)
{
  char hex[65] = { 0 };

  EXPECT_EQ(snprintf(hex, sizeof(hex), "%064llx", (unsigned long long)ordinal), 64);

  cardano_blake2b_hash_t*       id     = new_hash(hex);
  cardano_transaction_input_t*  input  = nullptr;
  cardano_transaction_output_t* output = nullptr;
  cardano_utxo_t*               utxo   = nullptr;

  EXPECT_EQ(cardano_transaction_input_new(id, 0, &input), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_new(address, 5000000U, &output), CARDANO_SUCCESS);

  if (script_ref != nullptr)
  {
    EXPECT_EQ(cardano_transaction_output_set_script_ref(output, script_ref), CARDANO_SUCCESS);
  }

  EXPECT_EQ(cardano_utxo_new(input, output, &utxo), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&id);
  cardano_transaction_input_unref(&input);
  cardano_transaction_output_unref(&output);

  return utxo;
}

/**
 * Creates a list holding up to two UTXOs.
 * \param first the first UTXO, or NULL.
 * \param second the second UTXO, or NULL.
 * \return A new instance of the list.
 */
static cardano_utxo_list_t*
new_utxo_list(cardano_utxo_t* first, cardano_utxo_t* second)
{
  cardano_utxo_list_t* list = nullptr;

  EXPECT_EQ(cardano_utxo_list_new(&list), CARDANO_SUCCESS);

  if (first != nullptr)
  {
    EXPECT_EQ(cardano_utxo_list_add(list, first), CARDANO_SUCCESS);
  }

  if (second != nullptr)
  {
    EXPECT_EQ(cardano_utxo_list_add(list, second), CARDANO_SUCCESS);
  }

  return list;
}

/**
 * Creates a transaction whose body needs no script: it spends no input and has no mint, withdrawal, certificate,
 * vote or guard.
 * \return A new instance of the transaction.
 */
static cardano_transaction_t*
new_empty_transaction()
{
  cardano_cbor_reader_t*           reader = new_reader(BALANCED_TX_CBOR);
  cardano_transaction_t*           tx     = nullptr;
  cardano_transaction_input_set_t* inputs = nullptr;

  EXPECT_EQ(cardano_transaction_from_cbor(reader, &tx), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_new(&inputs), CARDANO_SUCCESS);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);

  EXPECT_EQ(cardano_transaction_body_set_inputs(body, inputs), CARDANO_SUCCESS);

  cardano_transaction_body_unref(&body);
  cardano_transaction_input_set_unref(&inputs);
  cardano_cbor_reader_unref(&reader);

  return tx;
}

/**
 * Creates an input set holding the input of a UTXO.
 * \param utxo the UTXO.
 * \return A new instance of the input set.
 */
static cardano_transaction_input_set_t*
new_input_set(cardano_utxo_t* utxo)
{
  cardano_transaction_input_set_t* inputs = nullptr;
  cardano_transaction_input_t*     input  = cardano_utxo_get_input(utxo);

  EXPECT_EQ(cardano_transaction_input_set_new(&inputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_add(inputs, input), CARDANO_SUCCESS);

  cardano_transaction_input_unref(&input);

  return inputs;
}

/**
 * Makes a transaction spend the input of a UTXO.
 * \param tx the transaction.
 * \param utxo the UTXO to spend.
 */
static void
set_spent_input(cardano_transaction_t* tx, cardano_utxo_t* utxo)
{
  cardano_transaction_body_t*      body   = cardano_transaction_get_body(tx);
  cardano_transaction_input_set_t* inputs = new_input_set(utxo);

  EXPECT_EQ(cardano_transaction_body_set_inputs(body, inputs), CARDANO_SUCCESS);

  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_body_unref(&body);
}

/**
 * Makes a transaction reference the input of a UTXO.
 * \param tx the transaction.
 * \param utxo the UTXO to reference.
 */
static void
set_reference_input(cardano_transaction_t* tx, cardano_utxo_t* utxo)
{
  cardano_transaction_body_t*      body   = cardano_transaction_get_body(tx);
  cardano_transaction_input_set_t* inputs = new_input_set(utxo);

  EXPECT_EQ(cardano_transaction_body_set_reference_inputs(body, inputs), CARDANO_SUCCESS);

  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_body_unref(&body);
}

/**
 * Makes a transaction mint one token under a policy.
 * \param tx the transaction.
 * \param policy_id_hex the hexadecimal policy id.
 */
static void
set_mint(cardano_transaction_t* tx, const char* policy_id_hex)
{
  const std::string           cbor   = std::string("a1581c") + policy_id_hex + "a14001";
  cardano_cbor_reader_t*      reader = new_reader(cbor.c_str());
  cardano_multi_asset_t*      mint   = nullptr;
  cardano_transaction_body_t* body   = cardano_transaction_get_body(tx);

  EXPECT_EQ(cardano_multi_asset_from_cbor(reader, &mint), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_mint(body, mint), CARDANO_SUCCESS);

  cardano_multi_asset_unref(&mint);
  cardano_transaction_body_unref(&body);
  cardano_cbor_reader_unref(&reader);
}

/**
 * Sets the withdrawals of a transaction.
 * \param tx the transaction.
 * \param cbor the CBOR of the withdrawals.
 */
static void
set_withdrawals(cardano_transaction_t* tx, const char* cbor)
{
  cardano_cbor_reader_t*      reader      = new_reader(cbor);
  cardano_withdrawal_map_t*   withdrawals = nullptr;
  cardano_transaction_body_t* body        = cardano_transaction_get_body(tx);

  EXPECT_EQ(cardano_withdrawal_map_from_cbor(reader, &withdrawals), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_withdrawals(body, withdrawals), CARDANO_SUCCESS);

  cardano_withdrawal_map_unref(&withdrawals);
  cardano_transaction_body_unref(&body);
  cardano_cbor_reader_unref(&reader);
}

/**
 * Sets the guards of a transaction.
 * \param tx the transaction.
 * \param cbor the CBOR of the guard set.
 */
static void
set_guards(cardano_transaction_t* tx, const char* cbor)
{
  cardano_cbor_reader_t*      reader = new_reader(cbor);
  cardano_guard_set_t*        guards = nullptr;
  cardano_transaction_body_t* body   = cardano_transaction_get_body(tx);

  EXPECT_EQ(cardano_guard_set_from_cbor(reader, &guards), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_guards(body, guards), CARDANO_SUCCESS);

  cardano_guard_set_unref(&guards);
  cardano_transaction_body_unref(&body);
  cardano_cbor_reader_unref(&reader);
}

/**
 * Makes a script hash the only guard of a transaction.
 * \param tx the transaction.
 * \param script the script whose hash guards the transaction.
 */
static void
set_script_guard(cardano_transaction_t* tx, cardano_script_t* script)
{
  cardano_blake2b_hash_t*     hash       = cardano_script_get_hash(script);
  cardano_credential_t*       credential = nullptr;
  cardano_guard_set_t*        guards     = nullptr;
  cardano_transaction_body_t* body       = cardano_transaction_get_body(tx);

  EXPECT_EQ(cardano_credential_new(hash, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, &credential), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_guard_set_new(&guards), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_guard_set_add(guards, credential), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_guards(body, guards), CARDANO_SUCCESS);

  cardano_guard_set_unref(&guards);
  cardano_credential_unref(&credential);
  cardano_transaction_body_unref(&body);
  cardano_blake2b_hash_unref(&hash);
}

/**
 * Sets the voting procedures of a transaction.
 * \param tx the transaction.
 * \param cbor the CBOR of the voting procedures.
 */
static void
set_voting_procedures(cardano_transaction_t* tx, const char* cbor)
{
  cardano_cbor_reader_t*       reader     = new_reader(cbor);
  cardano_voting_procedures_t* procedures = nullptr;
  cardano_transaction_body_t*  body       = cardano_transaction_get_body(tx);

  EXPECT_EQ(cardano_voting_procedures_from_cbor(reader, &procedures), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_voting_procedures(body, procedures), CARDANO_SUCCESS);

  cardano_voting_procedures_unref(&procedures);
  cardano_transaction_body_unref(&body);
  cardano_cbor_reader_unref(&reader);
}

/**
 * Makes a certificate the only certificate of a transaction.
 * \param tx the transaction.
 * \param cbor the CBOR of the certificate.
 */
static void
set_certificate(cardano_transaction_t* tx, const std::string& cbor)
{
  cardano_cbor_reader_t*      reader       = new_reader(cbor.c_str());
  cardano_certificate_t*      certificate  = nullptr;
  cardano_certificate_set_t*  certificates = nullptr;
  cardano_transaction_body_t* body         = cardano_transaction_get_body(tx);

  EXPECT_EQ(cardano_certificate_from_cbor(reader, &certificate), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_certificate_set_new(&certificates), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_certificate_set_add(certificates, certificate), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_certificates(body, certificates), CARDANO_SUCCESS);

  cardano_certificate_set_unref(&certificates);
  cardano_certificate_unref(&certificate);
  cardano_transaction_body_unref(&body);
  cardano_cbor_reader_unref(&reader);
}

/**
 * Turns one of the key hash credentials of a CBOR hexadecimal string into a script hash credential.
 * \param cbor the CBOR holding key hash credentials.
 * \param occurrence the zero based position of the key hash credential to turn into a script hash credential.
 * \return The CBOR with the credential changed.
 */
static std::string
with_script_credential(const char* cbor, const size_t occurrence)
{
  std::string result   = cbor;
  size_t      position = result.find("8200581c");

  for (size_t i = 0; i < occurrence; ++i)
  {
    position = result.find("8200581c", position + 1U);
  }

  EXPECT_NE(position, std::string::npos);

  result.replace(position, 8U, "8201581c");

  return result;
}

/**
 * Collects the needed script hashes of a transaction.
 * \param tx the transaction.
 * \param resolved_inputs the resolved inputs, or NULL.
 * \return A new instance of the set of needed script hashes.
 */
static cardano_blake2b_hash_set_t*
get_needed(cardano_transaction_t* tx, cardano_utxo_list_t* resolved_inputs)
{
  cardano_transaction_body_t* body   = cardano_transaction_get_body(tx);
  cardano_blake2b_hash_set_t* needed = nullptr;

  EXPECT_EQ(_cardano_get_script_hashes_needed(body, resolved_inputs, &needed), CARDANO_SUCCESS);

  cardano_transaction_body_unref(&body);

  return needed;
}

/**
 * Checks whether a set of hashes holds exactly one given hash.
 * \param set the set of hashes.
 * \param hex the hexadecimal hash.
 * \return true if the set holds only the hash.
 */
static bool
holds_only(cardano_blake2b_hash_set_t* set, const char* hex)
{
  if (cardano_blake2b_hash_set_get_length(set) != 1U)
  {
    return false;
  }

  cardano_blake2b_hash_t* hash     = nullptr;
  cardano_blake2b_hash_t* expected = new_hash(hex);

  EXPECT_EQ(cardano_blake2b_hash_set_get(set, 0, &hash), CARDANO_SUCCESS);

  const bool is_equal = cardano_blake2b_hash_equals(hash, expected);

  cardano_blake2b_hash_unref(&hash);
  cardano_blake2b_hash_unref(&expected);

  return is_equal;
}

/**
 * Checks whether a set of hashes holds exactly the hash of a script.
 * \param set the set of hashes.
 * \param script the script.
 * \return true if the set holds only the hash of the script.
 */
static bool
holds_only_script(cardano_blake2b_hash_set_t* set, cardano_script_t* script)
{
  cardano_blake2b_hash_t* hash    = cardano_script_get_hash(script);
  char                    hex[57] = { 0 };

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, sizeof(hex)), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&hash);

  return holds_only(set, hex);
}

/**
 * Counts the needed script hashes of a transaction that carries a single certificate.
 * \param cbor the CBOR of the certificate.
 * \return The number of needed script hashes.
 */
static size_t
count_certificate_scripts(const std::string& cbor)
{
  cardano_transaction_t* tx = new_empty_transaction();

  set_certificate(tx, cbor);

  cardano_blake2b_hash_set_t* needed = get_needed(tx, nullptr);
  const size_t                count  = cardano_blake2b_hash_set_get_length(needed);

  cardano_blake2b_hash_set_unref(&needed);
  cardano_transaction_unref(&tx);

  return count;
}

/**
 * Makes a proposal procedure the only proposal of a transaction.
 * \param tx the transaction.
 * \param cbor the CBOR of the proposal procedure.
 */
static void
set_proposal(cardano_transaction_t* tx, const char* cbor)
{
  cardano_cbor_reader_t*            reader    = new_reader(cbor);
  cardano_proposal_procedure_t*     proposal  = nullptr;
  cardano_proposal_procedure_set_t* proposals = nullptr;
  cardano_transaction_body_t*       body      = cardano_transaction_get_body(tx);

  EXPECT_EQ(cardano_proposal_procedure_from_cbor(reader, &proposal), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_proposal_procedure_set_new(&proposals), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_proposal_procedure_set_add(proposals, proposal), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_proposal_procedure(body, proposals), CARDANO_SUCCESS);

  cardano_proposal_procedure_set_unref(&proposals);
  cardano_proposal_procedure_unref(&proposal);
  cardano_transaction_body_unref(&body);
  cardano_cbor_reader_unref(&reader);
}

/**
 * Counts the needed script hashes of a transaction that carries a single proposal procedure.
 * \param cbor the CBOR of the proposal procedure.
 * \return The number of needed script hashes.
 */
static size_t
count_proposal_scripts(const char* cbor)
{
  cardano_transaction_t* tx = new_empty_transaction();

  set_proposal(tx, cbor);

  cardano_blake2b_hash_set_t* needed = get_needed(tx, nullptr);
  const size_t                count  = cardano_blake2b_hash_set_get_length(needed);

  cardano_blake2b_hash_set_unref(&needed);
  cardano_transaction_unref(&tx);

  return count;
}

/**
 * Adds a script to the witness set of a transaction.
 * \param tx the transaction.
 * \param script the script, a native, PlutusV1, PlutusV2 or PlutusV3 script.
 */
static void
add_witness_script(cardano_transaction_t* tx, cardano_script_t* script)
{
  cardano_witness_set_t*    witnesses = cardano_transaction_get_witness_set(tx);
  cardano_script_language_t language  = CARDANO_SCRIPT_LANGUAGE_NATIVE;

  EXPECT_EQ(cardano_script_get_language(script, &language), CARDANO_SUCCESS);

  if (language == CARDANO_SCRIPT_LANGUAGE_NATIVE)
  {
    cardano_native_script_t*     native = nullptr;
    cardano_native_script_set_t* set    = nullptr;

    EXPECT_EQ(cardano_script_to_native(script, &native), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_native_script_set_new(&set), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_native_script_set_add(set, native), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_witness_set_set_native_scripts(witnesses, set), CARDANO_SUCCESS);

    cardano_native_script_set_unref(&set);
    cardano_native_script_unref(&native);
  }
  else if (language == CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1)
  {
    cardano_plutus_v1_script_t*     plutus = nullptr;
    cardano_plutus_v1_script_set_t* set    = nullptr;

    EXPECT_EQ(cardano_script_to_plutus_v1(script, &plutus), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_v1_script_set_new(&set), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_v1_script_set_add(set, plutus), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_witness_set_set_plutus_v1_scripts(witnesses, set), CARDANO_SUCCESS);

    cardano_plutus_v1_script_set_unref(&set);
    cardano_plutus_v1_script_unref(&plutus);
  }
  else if (language == CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2)
  {
    cardano_plutus_v2_script_t*     plutus = nullptr;
    cardano_plutus_v2_script_set_t* set    = nullptr;

    EXPECT_EQ(cardano_script_to_plutus_v2(script, &plutus), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_v2_script_set_new(&set), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_v2_script_set_add(set, plutus), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_witness_set_set_plutus_v2_scripts(witnesses, set), CARDANO_SUCCESS);

    cardano_plutus_v2_script_set_unref(&set);
    cardano_plutus_v2_script_unref(&plutus);
  }
  else
  {
    cardano_plutus_v3_script_t*     plutus = nullptr;
    cardano_plutus_v3_script_set_t* set    = nullptr;

    EXPECT_EQ(cardano_script_to_plutus_v3(script, &plutus), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_v3_script_set_new(&set), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_v3_script_set_add(set, plutus), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_witness_set_set_plutus_v3_scripts(witnesses, set), CARDANO_SUCCESS);

    cardano_plutus_v3_script_set_unref(&set);
    cardano_plutus_v3_script_unref(&plutus);
  }

  cardano_witness_set_unref(&witnesses);
}

/**
 * The languages reported by \ref _cardano_get_plutus_languages_used.
 */
typedef struct languages_t
{
    bool v1;
    bool v2;
    bool v3;
    bool v4;
    bool missing;
} languages_t;

/**
 * Finds the languages of the needed scripts of a transaction among its provided scripts.
 * \param tx the transaction.
 * \param reference_inputs the resolved reference inputs, or NULL.
 * \param pre_selected_utxo the pre selected UTXOs, or NULL.
 * \param selection the UTXOs chosen by coin selection, or NULL.
 * \return The languages found.
 */
static languages_t
get_languages(
  cardano_transaction_t* tx,
  cardano_utxo_list_t*   reference_inputs,
  cardano_utxo_list_t*   pre_selected_utxo,
  cardano_utxo_list_t*   selection)
{
  languages_t                 languages = { true, true, true, true, true };
  cardano_blake2b_hash_set_t* needed    = get_needed(tx, nullptr);

  EXPECT_EQ(
    _cardano_get_plutus_languages_used(
      tx,
      needed,
      reference_inputs,
      pre_selected_utxo,
      selection,
      &languages.v1,
      &languages.v2,
      &languages.v3,
      &languages.v4,
      &languages.missing),
    CARDANO_SUCCESS);

  cardano_blake2b_hash_set_unref(&needed);

  return languages;
}

/* UNIT TESTS ****************************************************************/

TEST(_cardano_get_payment_script_hash, returnsTheScriptHashOfTheScriptPaymentCredentials)
{
  const std::string script_addresses[] = { basePaymentScriptStakeKey, basePaymentScriptStakeScript, pointerScript, enterpriseScript };

  for (const std::string& value: script_addresses)
  {
    // Arrange
    cardano_address_t* address = new_address(value);

    // Act
    cardano_blake2b_hash_t* hash = _cardano_get_payment_script_hash(address);

    // Assert
    cardano_blake2b_hash_t* expected = new_hash(WITHDRAWAL_SCRIPT_HASH);

    EXPECT_TRUE(cardano_blake2b_hash_equals(hash, expected));

    // Cleanup
    cardano_blake2b_hash_unref(&expected);
    cardano_blake2b_hash_unref(&hash);
    cardano_address_unref(&address);
  }
}

TEST(_cardano_get_payment_script_hash, returnsNullIfThePaymentCredentialIsNotAScript)
{
  const std::string other_addresses[] = { basePaymentKeyStakeKey, basePaymentKeyStakeScript, pointerKey, enterpriseKey, rewardKey, rewardScript, byron };

  for (const std::string& value: other_addresses)
  {
    // Arrange
    cardano_address_t* address = new_address(value);

    // Act
    cardano_blake2b_hash_t* hash = _cardano_get_payment_script_hash(address);

    // Assert
    EXPECT_EQ(hash, nullptr);

    // Cleanup
    cardano_address_unref(&address);
  }
}

TEST(_cardano_get_payment_script_hash, returnsNullIfGivenNull)
{
  // Act
  cardano_blake2b_hash_t* hash = _cardano_get_payment_script_hash(nullptr);

  // Assert
  EXPECT_EQ(hash, nullptr);
}

TEST(_cardano_get_payment_script_hash, returnsNullIfMemoryAllocationFails)
{
  const std::string script_addresses[] = { basePaymentScriptStakeKey, pointerScript, enterpriseScript };

  for (const std::string& value: script_addresses)
  {
    for (int i = 0; i < 4; ++i)
    {
      // Arrange
      cardano_address_t* address = new_address(value);

      reset_allocators_run_count();
      set_malloc_limit(i);
      cardano_set_allocators(fail_malloc_at_limit, realloc, free);

      // Act
      cardano_blake2b_hash_t* hash = _cardano_get_payment_script_hash(address);

      // Assert
      cardano_set_allocators(malloc, realloc, free);
      reset_limited_malloc();

      if (hash != nullptr)
      {
        cardano_blake2b_hash_t* expected = new_hash(WITHDRAWAL_SCRIPT_HASH);

        EXPECT_TRUE(cardano_blake2b_hash_equals(hash, expected));

        cardano_blake2b_hash_unref(&expected);
      }

      // Cleanup
      cardano_blake2b_hash_unref(&hash);
      cardano_address_unref(&address);
    }
  }
}

TEST(_cardano_get_script_hashes_needed, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_transaction_t*      tx     = new_empty_transaction();
  cardano_transaction_body_t* body   = cardano_transaction_get_body(tx);
  cardano_blake2b_hash_set_t* needed = nullptr;

  // Act & Assert
  EXPECT_EQ(_cardano_get_script_hashes_needed(nullptr, nullptr, &needed), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(_cardano_get_script_hashes_needed(body, nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_body_unref(&body);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_script_hashes_needed, returnsAnEmptySetIfTheBodyNeedsNoScript)
{
  // Arrange
  cardano_transaction_t* tx = new_empty_transaction();

  // Act
  cardano_blake2b_hash_set_t* needed = get_needed(tx, nullptr);

  // Assert
  EXPECT_EQ(cardano_blake2b_hash_set_get_length(needed), 0U);

  // Cleanup
  cardano_blake2b_hash_set_unref(&needed);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_script_hashes_needed, collectsTheScriptOfASpentScriptInput)
{
  // Arrange
  cardano_transaction_t* tx       = new_empty_transaction();
  cardano_script_t*      script   = new_script(PLUTUS_V2_SCRIPT_CBOR);
  cardano_address_t*     address  = new_script_address(script);
  cardano_utxo_t*        utxo     = new_utxo(1U, address, nullptr);
  cardano_utxo_list_t*   resolved = new_utxo_list(utxo, nullptr);

  set_spent_input(tx, utxo);

  // Act
  cardano_blake2b_hash_set_t* needed = get_needed(tx, resolved);

  // Assert
  EXPECT_TRUE(holds_only_script(needed, script));

  // Cleanup
  cardano_blake2b_hash_set_unref(&needed);
  cardano_utxo_list_unref(&resolved);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&address);
  cardano_script_unref(&script);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_script_hashes_needed, doesNotCollectASpentKeyInput)
{
  // Arrange
  cardano_transaction_t* tx       = new_empty_transaction();
  cardano_address_t*     address  = new_address(basePaymentKeyStakeScript);
  cardano_utxo_t*        utxo     = new_utxo(1U, address, nullptr);
  cardano_utxo_list_t*   resolved = new_utxo_list(utxo, nullptr);

  set_spent_input(tx, utxo);

  // Act
  cardano_blake2b_hash_set_t* needed = get_needed(tx, resolved);

  // Assert
  EXPECT_EQ(cardano_blake2b_hash_set_get_length(needed), 0U);

  // Cleanup
  cardano_blake2b_hash_set_unref(&needed);
  cardano_utxo_list_unref(&resolved);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&address);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_script_hashes_needed, ignoresTheSpentInputsIfNoResolvedInputsAreGiven)
{
  // Arrange
  cardano_transaction_t* tx      = new_empty_transaction();
  cardano_script_t*      script  = new_script(PLUTUS_V2_SCRIPT_CBOR);
  cardano_address_t*     address = new_script_address(script);
  cardano_utxo_t*        utxo    = new_utxo(1U, address, nullptr);

  set_spent_input(tx, utxo);

  // Act
  cardano_blake2b_hash_set_t* needed = get_needed(tx, nullptr);

  // Assert
  EXPECT_EQ(cardano_blake2b_hash_set_get_length(needed), 0U);

  // Cleanup
  cardano_blake2b_hash_set_unref(&needed);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&address);
  cardano_script_unref(&script);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_script_hashes_needed, returnsErrorIfASpentInputIsNotResolved)
{
  // Arrange
  cardano_transaction_t*      tx       = new_empty_transaction();
  cardano_address_t*          address  = new_address(enterpriseScript);
  cardano_utxo_t*             utxo     = new_utxo(1U, address, nullptr);
  cardano_utxo_t*             other    = new_utxo(2U, address, nullptr);
  cardano_utxo_list_t*        resolved = new_utxo_list(other, nullptr);
  cardano_transaction_body_t* body     = cardano_transaction_get_body(tx);
  cardano_blake2b_hash_set_t* needed   = nullptr;

  set_spent_input(tx, utxo);

  // Act
  cardano_error_t result = _cardano_get_script_hashes_needed(body, resolved, &needed);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_EQ(needed, nullptr);

  // Cleanup
  cardano_transaction_body_unref(&body);
  cardano_utxo_list_unref(&resolved);
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&other);
  cardano_address_unref(&address);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_script_hashes_needed, neverCollectsTheScriptOfAReferenceInput)
{
  // Arrange
  cardano_transaction_t* tx       = new_empty_transaction();
  cardano_script_t*      script   = new_script(PLUTUS_V2_SCRIPT_CBOR);
  cardano_address_t*     address  = new_script_address(script);
  cardano_utxo_t*        utxo     = new_utxo(1U, address, script);
  cardano_utxo_list_t*   resolved = new_utxo_list(utxo, nullptr);

  set_reference_input(tx, utxo);

  // Act
  cardano_blake2b_hash_set_t* needed = get_needed(tx, resolved);

  // Assert
  EXPECT_EQ(cardano_blake2b_hash_set_get_length(needed), 0U);

  // Cleanup
  cardano_blake2b_hash_set_unref(&needed);
  cardano_utxo_list_unref(&resolved);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&address);
  cardano_script_unref(&script);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_script_hashes_needed, collectsTheMintPolicy)
{
  // Arrange
  cardano_transaction_t* tx = new_empty_transaction();

  set_mint(tx, WITHDRAWAL_SCRIPT_HASH);

  // Act
  cardano_blake2b_hash_set_t* needed = get_needed(tx, nullptr);

  // Assert
  EXPECT_TRUE(holds_only(needed, WITHDRAWAL_SCRIPT_HASH));

  // Cleanup
  cardano_blake2b_hash_set_unref(&needed);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_script_hashes_needed, collectsTheCredentialOfAScriptWithdrawalOnly)
{
  // Arrange
  cardano_transaction_t* tx = new_empty_transaction();

  set_withdrawals(tx, WITHDRAWAL_CBOR);

  // Act
  cardano_blake2b_hash_set_t* needed = get_needed(tx, nullptr);

  // Assert
  EXPECT_TRUE(holds_only(needed, WITHDRAWAL_SCRIPT_HASH));

  // Cleanup
  cardano_blake2b_hash_set_unref(&needed);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_script_hashes_needed, doesNotCollectAKeyWithdrawal)
{
  // Arrange
  cardano_transaction_t* tx = new_empty_transaction();

  set_withdrawals(tx, KEY_WITHDRAWAL_CBOR);

  // Act
  cardano_blake2b_hash_set_t* needed = get_needed(tx, nullptr);

  // Assert
  EXPECT_EQ(cardano_blake2b_hash_set_get_length(needed), 0U);

  // Cleanup
  cardano_blake2b_hash_set_unref(&needed);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_script_hashes_needed, collectsTheScriptCredentialOfEachCertificateThatNeedsAWitness)
{
  const char* certificates[] = {
    CBOR_STAKE_DEREGISTRATION,
    CBOR_STAKE_DELEGATION,
    CBOR_REGISTRATION,
    CBOR_UNREGISTRATION,
    CBOR_VOTE_DELEGATION,
    CBOR_STAKE_VOTE_DELEGATION,
    CBOR_STAKE_REGISTRATION_DELEGATION,
    CBOR_VOTE_REGISTRATION_DELEGATION,
    CBOR_STAKE_VOTE_REGISTRATION_DELEGATION,
    CBOR_RESIGN_COMMITTEE_COLD,
    CBOR_REGISTER_DREP,
    CBOR_UNREGISTER_DREP,
    CBOR_UPDATE_DREP,
    CBOR_AUTHORIZE_COMMITTEE_HOT
  };

  for (const char* cbor: certificates)
  {
    // Arrange
    cardano_transaction_t* tx = new_empty_transaction();

    set_certificate(tx, with_script_credential(cbor, 0U));

    // Act
    cardano_blake2b_hash_set_t* needed = get_needed(tx, nullptr);

    // Assert
    EXPECT_TRUE(holds_only(needed, ZERO_HASH)) << cbor;

    // Cleanup
    cardano_blake2b_hash_set_unref(&needed);
    cardano_transaction_unref(&tx);
  }
}

TEST(_cardano_get_script_hashes_needed, doesNotCollectTheKeyCredentialOfACertificate)
{
  const char* certificates[] = {
    CBOR_STAKE_DEREGISTRATION,
    CBOR_STAKE_DELEGATION,
    CBOR_REGISTRATION,
    CBOR_RESIGN_COMMITTEE_COLD,
    CBOR_REGISTER_DREP,
    CBOR_AUTHORIZE_COMMITTEE_HOT
  };

  for (const char* cbor: certificates)
  {
    // Act
    const size_t count = count_certificate_scripts(cbor);

    // Assert
    EXPECT_EQ(count, 0U) << cbor;
  }
}

TEST(_cardano_get_script_hashes_needed, doesNotCollectTheHotCredentialOfACommitteeHotKeyAuthorization)
{
  // Act
  const size_t count = count_certificate_scripts(with_script_credential(CBOR_AUTHORIZE_COMMITTEE_HOT, 1U));

  // Assert
  EXPECT_EQ(count, 0U);
}

TEST(_cardano_get_script_hashes_needed, doesNotCollectTheCertificatesThatNeedNoScriptWitness)
{
  // Act & Assert
  EXPECT_EQ(count_certificate_scripts(with_script_credential(CBOR_STAKE_REGISTRATION, 0U)), 0U);
  EXPECT_EQ(count_certificate_scripts(CBOR_POOL_RETIREMENT), 0U);
  EXPECT_EQ(count_certificate_scripts(CBOR_GENESIS_DELEGATION), 0U);
  EXPECT_EQ(count_certificate_scripts(CBOR_MIR), 0U);
}

TEST(_cardano_get_script_hashes_needed, collectsTheDRepScriptVoterOnly)
{
  // Arrange
  cardano_transaction_t* tx = new_empty_transaction();

  set_voting_procedures(tx, VOTING_PROCEDURES_CBOR);

  // Act
  cardano_blake2b_hash_set_t* needed = get_needed(tx, nullptr);

  // Assert
  EXPECT_TRUE(holds_only(needed, VOTER_SCRIPT_HASH));

  // Cleanup
  cardano_blake2b_hash_set_unref(&needed);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_script_hashes_needed, collectsTheCommitteeHotScriptVoter)
{
  // Arrange
  cardano_transaction_t* tx = new_empty_transaction();

  set_voting_procedures(tx, CC_SCRIPT_VOTER_CBOR);

  // Act
  cardano_blake2b_hash_set_t* needed = get_needed(tx, nullptr);

  // Assert
  EXPECT_TRUE(holds_only(needed, CC_SCRIPT_VOTER_HASH));

  // Cleanup
  cardano_blake2b_hash_set_unref(&needed);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_script_hashes_needed, doesNotCollectAKeyVoter)
{
  // Arrange
  cardano_transaction_t* tx = new_empty_transaction();

  set_voting_procedures(tx, KEY_VOTER_CBOR);

  // Act
  cardano_blake2b_hash_set_t* needed = get_needed(tx, nullptr);

  // Assert
  EXPECT_EQ(cardano_blake2b_hash_set_get_length(needed), 0U);

  // Cleanup
  cardano_blake2b_hash_set_unref(&needed);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_script_hashes_needed, collectsTheScriptGuardsOnly)
{
  // Arrange
  cardano_transaction_t* tx = new_empty_transaction();

  set_guards(tx, GUARDS_CBOR);

  // Act
  cardano_blake2b_hash_set_t* needed = get_needed(tx, nullptr);

  // Assert
  EXPECT_TRUE(holds_only(needed, GUARD_SCRIPT_HASH));

  // Cleanup
  cardano_blake2b_hash_set_unref(&needed);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_script_hashes_needed, collectsTheGuardrailsScriptOfAParameterChangeProposal)
{
  // Arrange
  cardano_transaction_t* tx = new_empty_transaction();

  set_proposal(tx, PARAMETER_CHANGE_PROPOSAL_CBOR);

  // Act
  cardano_blake2b_hash_set_t* needed = get_needed(tx, nullptr);

  // Assert
  EXPECT_TRUE(holds_only(needed, GUARDRAILS_SCRIPT_HASH));

  // Cleanup
  cardano_blake2b_hash_set_unref(&needed);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_script_hashes_needed, collectsTheGuardrailsScriptOfATreasuryWithdrawalsProposal)
{
  // Arrange
  cardano_transaction_t* tx = new_empty_transaction();

  set_proposal(tx, TREASURY_WITHDRAWALS_PROPOSAL_CBOR);

  // Act
  cardano_blake2b_hash_set_t* needed = get_needed(tx, nullptr);

  // Assert
  EXPECT_TRUE(holds_only(needed, GUARDRAILS_SCRIPT_HASH));

  // Cleanup
  cardano_blake2b_hash_set_unref(&needed);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_script_hashes_needed, doesNotCollectAProposalWithoutAGuardrailsScript)
{
  // Act
  const size_t count = count_proposal_scripts(UNGUARDED_TREASURY_PROPOSAL_CBOR);

  // Assert
  EXPECT_EQ(count, 0U);
}

TEST(_cardano_get_script_hashes_needed, doesNotCollectTheProposalsOfOtherActionKinds)
{
  // Act & Assert
  EXPECT_EQ(count_proposal_scripts(NO_CONFIDENCE_PROPOSAL_CBOR), 0U);
  EXPECT_EQ(count_proposal_scripts(INFO_PROPOSAL_CBOR), 0U);
}

TEST(_cardano_get_script_hashes_needed, collectsAScriptNeededTwiceOnce)
{
  // Arrange
  cardano_transaction_t* tx = new_empty_transaction();

  set_mint(tx, WITHDRAWAL_SCRIPT_HASH);
  set_withdrawals(tx, WITHDRAWAL_CBOR);

  // Act
  cardano_blake2b_hash_set_t* needed = get_needed(tx, nullptr);

  // Assert
  EXPECT_TRUE(holds_only(needed, WITHDRAWAL_SCRIPT_HASH));

  // Cleanup
  cardano_blake2b_hash_set_unref(&needed);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_script_hashes_needed, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_transaction_t* tx       = new_empty_transaction();
  cardano_script_t*      script   = new_script(PLUTUS_V2_SCRIPT_CBOR);
  cardano_address_t*     address  = new_script_address(script);
  cardano_utxo_t*        utxo     = new_utxo(1U, address, nullptr);
  cardano_utxo_list_t*   resolved = new_utxo_list(utxo, nullptr);

  set_spent_input(tx, utxo);
  set_mint(tx, WITHDRAWAL_SCRIPT_HASH);
  set_withdrawals(tx, WITHDRAWAL_CBOR);
  set_certificate(tx, with_script_credential(CBOR_AUTHORIZE_COMMITTEE_HOT, 0U));
  set_voting_procedures(tx, VOTING_PROCEDURES_CBOR);
  set_guards(tx, GUARDS_CBOR);
  set_proposal(tx, TREASURY_WITHDRAWALS_PROPOSAL_CBOR);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);

  bool has_succeeded = false;

  for (int i = 0; (i < 200) && !has_succeeded; ++i)
  {
    cardano_blake2b_hash_set_t* needed = nullptr;

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_error_t result = _cardano_get_script_hashes_needed(body, resolved, &needed);

    cardano_set_allocators(malloc, realloc, free);
    reset_limited_malloc();

    // Assert
    has_succeeded = result == CARDANO_SUCCESS;

    if (has_succeeded)
    {
      EXPECT_EQ(cardano_blake2b_hash_set_get_length(needed), 6U);
    }
    else
    {
      EXPECT_EQ(needed, nullptr);
    }

    cardano_blake2b_hash_set_unref(&needed);
  }

  EXPECT_TRUE(has_succeeded);

  // Cleanup
  cardano_transaction_body_unref(&body);
  cardano_utxo_list_unref(&resolved);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&address);
  cardano_script_unref(&script);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_plutus_languages_used, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_transaction_t*      tx     = new_empty_transaction();
  cardano_blake2b_hash_set_t* needed = get_needed(tx, nullptr);
  bool                        flag   = false;

  // Act & Assert
  EXPECT_EQ(_cardano_get_plutus_languages_used(nullptr, needed, nullptr, nullptr, nullptr, &flag, &flag, &flag, &flag, &flag), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(_cardano_get_plutus_languages_used(tx, nullptr, nullptr, nullptr, nullptr, &flag, &flag, &flag, &flag, &flag), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(_cardano_get_plutus_languages_used(tx, needed, nullptr, nullptr, nullptr, nullptr, &flag, &flag, &flag, &flag), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(_cardano_get_plutus_languages_used(tx, needed, nullptr, nullptr, nullptr, &flag, nullptr, &flag, &flag, &flag), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(_cardano_get_plutus_languages_used(tx, needed, nullptr, nullptr, nullptr, &flag, &flag, nullptr, &flag, &flag), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(_cardano_get_plutus_languages_used(tx, needed, nullptr, nullptr, nullptr, &flag, &flag, &flag, nullptr, &flag), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(_cardano_get_plutus_languages_used(tx, needed, nullptr, nullptr, nullptr, &flag, &flag, &flag, &flag, nullptr), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_set_unref(&needed);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_plutus_languages_used, reportsNothingIfNoScriptIsNeeded)
{
  // Arrange
  cardano_transaction_t* tx     = new_empty_transaction();
  cardano_script_t*      script = new_script(PLUTUS_V2_SCRIPT_CBOR);
  cardano_address_t*     owner  = new_address(enterpriseKey);
  cardano_utxo_t*        utxo   = new_utxo(1U, owner, script);
  cardano_utxo_list_t*   list   = new_utxo_list(utxo, nullptr);

  add_witness_script(tx, script);

  // Act
  const languages_t languages = get_languages(tx, list, list, list);

  // Assert
  EXPECT_FALSE(languages.v1);
  EXPECT_FALSE(languages.v2);
  EXPECT_FALSE(languages.v3);
  EXPECT_FALSE(languages.v4);
  EXPECT_FALSE(languages.missing);

  // Cleanup
  cardano_utxo_list_unref(&list);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&owner);
  cardano_script_unref(&script);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_plutus_languages_used, findsANeededScriptOfTheWitnessSet)
{
  const char* scripts[] = { PLUTUS_V1_SCRIPT_CBOR, PLUTUS_V2_SCRIPT_CBOR, PLUTUS_V3_SCRIPT_CBOR };

  for (size_t i = 0U; i < 3U; ++i)
  {
    // Arrange
    cardano_transaction_t* tx     = new_empty_transaction();
    cardano_script_t*      script = new_script(scripts[i]);

    add_witness_script(tx, script);
    set_script_guard(tx, script);

    // Act
    const languages_t languages = get_languages(tx, nullptr, nullptr, nullptr);

    // Assert
    EXPECT_EQ(languages.v1, i == 0U);
    EXPECT_EQ(languages.v2, i == 1U);
    EXPECT_EQ(languages.v3, i == 2U);
    EXPECT_FALSE(languages.v4);
    EXPECT_FALSE(languages.missing);

    // Cleanup
    cardano_script_unref(&script);
    cardano_transaction_unref(&tx);
  }
}

TEST(_cardano_get_plutus_languages_used, findsANeededReferenceScriptOnEachKindOfResolvedUtxo)
{
  for (int source = 0; source < 3; ++source)
  {
    // Arrange
    cardano_transaction_t* tx     = new_empty_transaction();
    cardano_script_t*      script = new_script(PLUTUS_V2_SCRIPT_CBOR);
    cardano_address_t*     owner  = new_address(enterpriseKey);
    cardano_utxo_t*        utxo   = new_utxo(1U, owner, script);
    cardano_utxo_list_t*   list   = new_utxo_list(utxo, nullptr);

    set_script_guard(tx, script);

    // Act
    const languages_t languages = get_languages(
      tx,
      (source == 0) ? list : nullptr,
      (source == 1) ? list : nullptr,
      (source == 2) ? list : nullptr);

    // Assert
    EXPECT_FALSE(languages.v1);
    EXPECT_TRUE(languages.v2);
    EXPECT_FALSE(languages.v3);
    EXPECT_FALSE(languages.v4);
    EXPECT_FALSE(languages.missing);

    // Cleanup
    cardano_utxo_list_unref(&list);
    cardano_utxo_unref(&utxo);
    cardano_address_unref(&owner);
    cardano_script_unref(&script);
    cardano_transaction_unref(&tx);
  }
}

TEST(_cardano_get_plutus_languages_used, findsANeededPlutusV4ReferenceScript)
{
  // Arrange
  cardano_transaction_t* tx     = new_empty_transaction();
  cardano_script_t*      script = new_script(PLUTUS_V4_SCRIPT_CBOR);
  cardano_address_t*     owner  = new_address(enterpriseKey);
  cardano_utxo_t*        utxo   = new_utxo(1U, owner, script);
  cardano_utxo_list_t*   list   = new_utxo_list(utxo, nullptr);

  set_script_guard(tx, script);

  // Act
  const languages_t languages = get_languages(tx, list, nullptr, nullptr);

  // Assert
  EXPECT_FALSE(languages.v1);
  EXPECT_FALSE(languages.v2);
  EXPECT_FALSE(languages.v3);
  EXPECT_TRUE(languages.v4);
  EXPECT_FALSE(languages.missing);

  // Cleanup
  cardano_utxo_list_unref(&list);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&owner);
  cardano_script_unref(&script);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_plutus_languages_used, reportsNoLanguageForANeededNativeScript)
{
  // Arrange
  cardano_transaction_t* tx     = new_empty_transaction();
  cardano_script_t*      script = new_script(NATIVE_SCRIPT_CBOR);
  cardano_address_t*     owner  = new_address(enterpriseKey);
  cardano_utxo_t*        utxo   = new_utxo(1U, owner, script);
  cardano_utxo_list_t*   list   = new_utxo_list(utxo, nullptr);

  add_witness_script(tx, script);
  set_script_guard(tx, script);

  // Act
  const languages_t languages = get_languages(tx, list, nullptr, nullptr);

  // Assert
  EXPECT_FALSE(languages.v1);
  EXPECT_FALSE(languages.v2);
  EXPECT_FALSE(languages.v3);
  EXPECT_FALSE(languages.v4);
  EXPECT_FALSE(languages.missing);

  // Cleanup
  cardano_utxo_list_unref(&list);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&owner);
  cardano_script_unref(&script);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_plutus_languages_used, reportsANeededScriptThatIsNotProvided)
{
  // Arrange
  cardano_transaction_t* tx     = new_empty_transaction();
  cardano_script_t*      script = new_script(PLUTUS_V2_SCRIPT_CBOR);
  cardano_script_t*      other  = new_script(PLUTUS_V3_SCRIPT_CBOR);
  cardano_address_t*     owner  = new_address(enterpriseKey);
  cardano_utxo_t*        utxo   = new_utxo(1U, owner, other);
  cardano_utxo_list_t*   list   = new_utxo_list(utxo, nullptr);

  add_witness_script(tx, other);
  set_script_guard(tx, script);

  // Act
  const languages_t languages = get_languages(tx, list, list, list);

  // Assert
  EXPECT_FALSE(languages.v1);
  EXPECT_FALSE(languages.v2);
  EXPECT_FALSE(languages.v3);
  EXPECT_FALSE(languages.v4);
  EXPECT_TRUE(languages.missing);

  // Cleanup
  cardano_utxo_list_unref(&list);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&owner);
  cardano_script_unref(&script);
  cardano_script_unref(&other);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_plutus_languages_used, findsTheScriptOfASpentScriptInputOnItsOwnUtxo)
{
  // Arrange
  cardano_transaction_t*      tx        = new_empty_transaction();
  cardano_script_t*           script    = new_script(PLUTUS_V1_SCRIPT_CBOR);
  cardano_address_t*          address   = new_script_address(script);
  cardano_utxo_t*             utxo      = new_utxo(1U, address, script);
  cardano_utxo_list_t*        selection = new_utxo_list(utxo, nullptr);
  cardano_blake2b_hash_set_t* needed    = nullptr;

  set_spent_input(tx, utxo);

  needed = get_needed(tx, selection);

  bool v1      = false;
  bool v2      = true;
  bool v3      = true;
  bool v4      = true;
  bool missing = true;

  // Act
  cardano_error_t result = _cardano_get_plutus_languages_used(tx, needed, nullptr, nullptr, selection, &v1, &v2, &v3, &v4, &missing);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(v1);
  EXPECT_FALSE(v2);
  EXPECT_FALSE(v3);
  EXPECT_FALSE(v4);
  EXPECT_FALSE(missing);

  // Cleanup
  cardano_blake2b_hash_set_unref(&needed);
  cardano_utxo_list_unref(&selection);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&address);
  cardano_script_unref(&script);
  cardano_transaction_unref(&tx);
}

TEST(_cardano_get_plutus_languages_used, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_transaction_t* tx     = new_empty_transaction();
  cardano_script_t*      native = new_script(NATIVE_SCRIPT_CBOR);
  cardano_script_t*      v1     = new_script(PLUTUS_V1_SCRIPT_CBOR);
  cardano_script_t*      v2     = new_script(PLUTUS_V2_SCRIPT_CBOR);
  cardano_script_t*      v3     = new_script(PLUTUS_V3_SCRIPT_CBOR);
  cardano_script_t*      v4     = new_script(PLUTUS_V4_SCRIPT_CBOR);
  cardano_address_t*     owner  = new_address(enterpriseKey);
  cardano_utxo_t*        utxo   = new_utxo(1U, owner, v4);
  cardano_utxo_list_t*   list   = new_utxo_list(utxo, nullptr);

  add_witness_script(tx, native);
  add_witness_script(tx, v1);
  add_witness_script(tx, v2);
  add_witness_script(tx, v3);
  set_script_guard(tx, v4);

  cardano_blake2b_hash_set_t* needed = get_needed(tx, nullptr);

  bool has_succeeded = false;

  for (int i = 0; (i < 200) && !has_succeeded; ++i)
  {
    bool has_v1      = false;
    bool has_v2      = false;
    bool has_v3      = false;
    bool has_v4      = false;
    bool has_missing = true;

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_error_t result = _cardano_get_plutus_languages_used(tx, needed, list, list, list, &has_v1, &has_v2, &has_v3, &has_v4, &has_missing);

    cardano_set_allocators(malloc, realloc, free);
    reset_limited_malloc();

    // Assert
    has_succeeded = result == CARDANO_SUCCESS;

    if (has_succeeded)
    {
      EXPECT_TRUE(has_v4);
      EXPECT_FALSE(has_missing);
    }
  }

  EXPECT_TRUE(has_succeeded);

  // Cleanup
  cardano_blake2b_hash_set_unref(&needed);
  cardano_utxo_list_unref(&list);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&owner);
  cardano_script_unref(&native);
  cardano_script_unref(&v1);
  cardano_script_unref(&v2);
  cardano_script_unref(&v3);
  cardano_script_unref(&v4);
  cardano_transaction_unref(&tx);
}
