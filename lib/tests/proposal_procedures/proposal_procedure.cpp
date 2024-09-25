/**
 * \file proposal_procedure.cpp
 *
 * \author angel.castillo
 * \date   Aug 31, 2024
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

#include <cardano/error.h>

#include <cardano/cbor/cbor_reader.h>
#include <cardano/proposal_procedures/proposal_procedure.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <cardano/proposal_procedures/hard_fork_initiation_action.h>
#include <cardano/proposal_procedures/info_action.h>
#include <cardano/proposal_procedures/new_constitution_action.h>
#include <cardano/proposal_procedures/no_confidence_action.h>
#include <cardano/proposal_procedures/parameter_change_action.h>
#include <cardano/proposal_procedures/treasury_withdrawals_action.h>
#include <cardano/proposal_procedures/update_committee_action.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* PARAMETER_CHANGE_PROPOSAL_CBOR     = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8400825820000000000000000000000000000000000000000000000000000000000000000003b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* HARD_FORK_INITIATION_PROPOSAL_CBOR = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8301825820000000000000000000000000000000000000000000000000000000000000000003820103827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* TREASURY_WITHDRAWALS_PROPOSAL_CBOR = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8302a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f01581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* NO_CONFIDENCE_PROPOSAL_CBOR        = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8203825820000000000000000000000000000000000000000000000000000000000000000003827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* UPDATE_COMMITTEE_PROPOSAL_CBOR     = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8504825820000000000000000000000000000000000000000000000000000000000000000003d90102828200581c000000000000000000000000000000000000000000000000000000008200581c20000000000000000000000000000000000000000000000000000000a28200581c30000000000000000000000000000000000000000000000000000000018200581c4000000000000000000000000000000000000000000000000000000002d81e820105827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* NEW_CONSTITUTION_PROPOSAL_CBOR     = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f830582582000000000000000000000000000000000000000000000000000000000000000000382827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000f6827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* INFO_PROPOSAL_CBOR                 = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8106827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";

static const char* INVALID_PARAMETER_CHANGE_PROPOSAL_CBOR     = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8400ef5820000000000000000000000000000000000000000000000000000000000000000003b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* INVALID_HARD_FORK_INITIATION_PROPOSAL_CBOR = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8301ef5820000000000000000000000000000000000000000000000000000000000000000003820103827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* INVALID_TREASURY_WITHDRAWALS_PROPOSAL_CBOR = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8302ef581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f01581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* INVALID_NO_CONFIDENCE_PROPOSAL_CBOR        = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8203ef5820000000000000000000000000000000000000000000000000000000000000000003827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* INVALID_UPDATE_COMMITTEE_PROPOSAL_CBOR     = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8504ef5820000000000000000000000000000000000000000000000000000000000000000003d90102828200581c000000000000000000000000000000000000000000000000000000008200581c20000000000000000000000000000000000000000000000000000000a28200581c30000000000000000000000000000000000000000000000000000000018200581c4000000000000000000000000000000000000000000000000000000002d81e820105827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* INVALID_NEW_CONSTITUTION_PROPOSAL_CBOR     = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8305ef582000000000000000000000000000000000000000000000000000000000000000000382827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000f6827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* INVALID_INFO_PROPOSAL_CBOR                 = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8106ef7668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";

static const char* INVALID_DEPOSIT_CBOR        = "84ef000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8106827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* INVALID_REWARD_ADDRESS_CBOR = "841a000f4240ef1de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8106827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* INVALID_ANCHOR_CBOR         = "841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8106ef7668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";

static const char* PARAMETER_CHANGE_CBOR     = "8400825820000000000000000000000000000000000000000000000000000000000000000003b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d";
static const char* HARD_FORK_INITIATION_CBOR = "8301825820000000000000000000000000000000000000000000000000000000000000000003820103";
static const char* TREASURY_WITHDRAWALS_CBOR = "8302a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f01581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d";
static const char* NO_CONFIDENCE_CBOR        = "8203825820000000000000000000000000000000000000000000000000000000000000000003";
static const char* UPDATE_COMMITTEE_CBOR     = "8504825820000000000000000000000000000000000000000000000000000000000000000003d90102828200581c000000000000000000000000000000000000000000000000000000008200581c20000000000000000000000000000000000000000000000000000000a28200581c30000000000000000000000000000000000000000000000000000000018200581c4000000000000000000000000000000000000000000000000000000002d81e820105";
static const char* NEW_CONSTITUTION_CBOR     = "830582582000000000000000000000000000000000000000000000000000000000000000000382827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000f6";
static const char* INFO_CBOR                 = "8106";

static const char*    ANCHOR_CBOR    = "827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char*    REWARD_ACCOUNT = "stake1u89sasnfyjtmgk8ydqfv3fdl52f36x3djedfnzfc9rkgzrcss5vgr";
static const uint64_t DEPOSIT        = 1000000;

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the parameter_change_action.
 * @return A new instance of the parameter_change_action.
 */
static cardano_parameter_change_action_t*
new_parameter_change_action()
{
  cardano_parameter_change_action_t* action = NULL;
  cardano_cbor_reader_t*             reader = cardano_cbor_reader_from_hex(PARAMETER_CHANGE_CBOR, strlen(PARAMETER_CHANGE_CBOR));
  cardano_error_t                    result = cardano_parameter_change_action_from_cbor(reader, &action);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return action;
};

/**
 * Creates a new default instance of the hard_fork_initiation_action.
 * @return A new instance of the hard_fork_initiation_action.
 */
static cardano_hard_fork_initiation_action_t*
new_hard_fork_initiation_action()
{
  cardano_hard_fork_initiation_action_t* action = NULL;
  cardano_cbor_reader_t*                 reader = cardano_cbor_reader_from_hex(HARD_FORK_INITIATION_CBOR, strlen(HARD_FORK_INITIATION_CBOR));
  cardano_error_t                        result = cardano_hard_fork_initiation_action_from_cbor(reader, &action);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return action;
};

/**
 * Creates a new default instance of the treasury_withdrawals_action.
 * @return A new instance of the treasury_withdrawals_action.
 */
static cardano_treasury_withdrawals_action_t*
new_treasury_withdrawals_action()
{
  cardano_treasury_withdrawals_action_t* action = NULL;
  cardano_cbor_reader_t*                 reader = cardano_cbor_reader_from_hex(TREASURY_WITHDRAWALS_CBOR, strlen(TREASURY_WITHDRAWALS_CBOR));
  cardano_error_t                        result = cardano_treasury_withdrawals_action_from_cbor(reader, &action);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return action;
};

/**
 * Creates a new default instance of the no_confidence_action.
 * @return A new instance of the no_confidence_action.
 */
static cardano_no_confidence_action_t*
new_no_confidence_action()
{
  cardano_no_confidence_action_t* action = NULL;
  cardano_cbor_reader_t*          reader = cardano_cbor_reader_from_hex(NO_CONFIDENCE_CBOR, strlen(NO_CONFIDENCE_CBOR));
  cardano_error_t                 result = cardano_no_confidence_action_from_cbor(reader, &action);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return action;
};

/**
 * Creates a new default instance of the update_committee_action.
 * @return A new instance of the update_committee_action.
 */
static cardano_update_committee_action_t*
new_update_committee_action()
{
  cardano_update_committee_action_t* action = NULL;
  cardano_cbor_reader_t*             reader = cardano_cbor_reader_from_hex(UPDATE_COMMITTEE_CBOR, strlen(UPDATE_COMMITTEE_CBOR));
  cardano_error_t                    result = cardano_update_committee_action_from_cbor(reader, &action);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return action;
};

/**
 * Creates a new default instance of the new_constitution_action.
 * @return A new instance of the new_constitution_action.
 */
static cardano_new_constitution_action_t*
new_new_constitution_action()
{
  cardano_new_constitution_action_t* action = NULL;
  cardano_cbor_reader_t*             reader = cardano_cbor_reader_from_hex(NEW_CONSTITUTION_CBOR, strlen(NEW_CONSTITUTION_CBOR));
  cardano_error_t                    result = cardano_new_constitution_action_from_cbor(reader, &action);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return action;
};

/**
 * Creates a new default instance of the info_action.
 * @return A new instance of the info_action.
 */
static cardano_info_action_t*
new_info_action()
{
  cardano_info_action_t* action = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(INFO_CBOR, strlen(INFO_CBOR));
  cardano_error_t        result = cardano_info_action_from_cbor(reader, &action);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return action;
};

/**
 * Creates a new default instance of the anchor.
 * @return A new instance of the anchor.
 */
static cardano_anchor_t*
new_anchor()
{
  cardano_anchor_t*      anchor = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  cardano_error_t        result = cardano_anchor_from_cbor(reader, &anchor);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return anchor;
}

/**
 * Creates a new default instance of the reward_address.
 * @return A new instance of the reward_address.
 */
static cardano_reward_address_t*
new_reward_address()
{
  cardano_reward_address_t* reward_address = NULL;
  cardano_error_t           result         = cardano_reward_address_from_bech32(REWARD_ACCOUNT, strlen(REWARD_ACCOUNT), &reward_address);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  return reward_address;
}

/**
 * Creates a new default instance of the reward.
 * @return A new instance of the reward.
 */
static cardano_proposal_procedure_t*
new_default_proposal_procedure(const char* cbor)
{
  cardano_proposal_procedure_t* proposal_procedure = NULL;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t               result             = cardano_proposal_procedure_from_cbor(reader, &proposal_procedure);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return proposal_procedure;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_proposal_procedure_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(PARAMETER_CHANGE_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_proposal_procedure_ref(proposal_procedure);

  // Assert
  EXPECT_THAT(proposal_procedure, testing::Not((cardano_proposal_procedure_t*)nullptr));
  EXPECT_EQ(cardano_proposal_procedure_refcount(proposal_procedure), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_proposal_procedure_unref(&proposal_procedure);
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_proposal_procedure_ref(nullptr);
}

TEST(cardano_proposal_procedure_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = nullptr;

  // Act
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_proposal_procedure_unref((cardano_proposal_procedure_t**)nullptr);
}

TEST(cardano_proposal_procedure_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(PARAMETER_CHANGE_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_proposal_procedure_ref(proposal_procedure);
  size_t ref_count = cardano_proposal_procedure_refcount(proposal_procedure);

  cardano_proposal_procedure_unref(&proposal_procedure);
  size_t updated_ref_count = cardano_proposal_procedure_refcount(proposal_procedure);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(PARAMETER_CHANGE_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_proposal_procedure_ref(proposal_procedure);
  size_t ref_count = cardano_proposal_procedure_refcount(proposal_procedure);

  cardano_proposal_procedure_unref(&proposal_procedure);
  size_t updated_ref_count = cardano_proposal_procedure_refcount(proposal_procedure);

  cardano_proposal_procedure_unref(&proposal_procedure);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(proposal_procedure, (cardano_proposal_procedure_t*)nullptr);

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_proposal_procedure_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_proposal_procedure_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = nullptr;
  const char*                   message            = "This is a test message";

  // Act
  cardano_proposal_procedure_set_last_error(proposal_procedure, message);

  // Assert
  EXPECT_STREQ(cardano_proposal_procedure_get_last_error(proposal_procedure), "Object is NULL.");
}

TEST(cardano_proposal_procedure_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(PARAMETER_CHANGE_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  const char* message = nullptr;

  // Act
  cardano_proposal_procedure_set_last_error(proposal_procedure, message);

  // Assert
  EXPECT_STREQ(cardano_proposal_procedure_get_last_error(proposal_procedure), "");

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  // Act
  cardano_error_t result = cardano_proposal_procedure_from_cbor(nullptr, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposal_procedure_from_cbor, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(PARAMETER_CHANGE_PROPOSAL_CBOR, strlen(PARAMETER_CHANGE_PROPOSAL_CBOR));

  // Act
  cardano_error_t result = cardano_proposal_procedure_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposal_procedure_from_cbor, returnsErrorIfInvalidArraySize)
{
  // Arrange
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex("80", strlen("80"));
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  // Act
  cardano_error_t result = cardano_proposal_procedure_from_cbor(reader, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposal_procedure_from_cbor, returnsErrorIfInvalidDeposit)
{
  // Arrange
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(INVALID_DEPOSIT_CBOR, strlen(INVALID_DEPOSIT_CBOR));
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  // Act
  cardano_error_t result = cardano_proposal_procedure_from_cbor(reader, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposal_procedure_from_cbor, returnsErrorIfInvalidRewardAddress)
{
  // Arrange
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(INVALID_REWARD_ADDRESS_CBOR, strlen(INVALID_REWARD_ADDRESS_CBOR));
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  // Act
  cardano_error_t result = cardano_proposal_procedure_from_cbor(reader, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposal_procedure_from_cbor, returnsErrorIfInvalidAnchor)
{
  // Arrange
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(INVALID_ANCHOR_CBOR, strlen(INVALID_ANCHOR_CBOR));
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  // Act
  cardano_error_t result = cardano_proposal_procedure_from_cbor(reader, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposal_procedure_from_cbor, returnsErrorIfInvalidHardForkInitiationProposal)
{
  // Arrange
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(INVALID_HARD_FORK_INITIATION_PROPOSAL_CBOR, strlen(INVALID_HARD_FORK_INITIATION_PROPOSAL_CBOR));
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  // Act
  cardano_error_t result = cardano_proposal_procedure_from_cbor(reader, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposal_procedure_from_cbor, returnsErrorIfInvalidTreasuryWithdrawalsProposal)
{
  // Arrange
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(INVALID_TREASURY_WITHDRAWALS_PROPOSAL_CBOR, strlen(INVALID_TREASURY_WITHDRAWALS_PROPOSAL_CBOR));
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  // Act
  cardano_error_t result = cardano_proposal_procedure_from_cbor(reader, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposal_procedure_from_cbor, returnsErrorIfInvalidNoConfidenceProposal)
{
  // Arrange
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(INVALID_NO_CONFIDENCE_PROPOSAL_CBOR, strlen(INVALID_NO_CONFIDENCE_PROPOSAL_CBOR));
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  // Act
  cardano_error_t result = cardano_proposal_procedure_from_cbor(reader, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposal_procedure_from_cbor, returnsErrorIfInvalidUpdateCommitteeProposal)
{
  // Arrange
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(INVALID_UPDATE_COMMITTEE_PROPOSAL_CBOR, strlen(INVALID_UPDATE_COMMITTEE_PROPOSAL_CBOR));
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  // Act
  cardano_error_t result = cardano_proposal_procedure_from_cbor(reader, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposal_procedure_from_cbor, returnsErrorIfInvalidNewConstitutionProposal)
{
  // Arrange
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(INVALID_NEW_CONSTITUTION_PROPOSAL_CBOR, strlen(INVALID_NEW_CONSTITUTION_PROPOSAL_CBOR));
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  // Act
  cardano_error_t result = cardano_proposal_procedure_from_cbor(reader, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposal_procedure_from_cbor, returnsErrorIfInvalidInfoProposal)
{
  // Arrange
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(INVALID_INFO_PROPOSAL_CBOR, strlen(INVALID_INFO_PROPOSAL_CBOR));
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  // Act
  cardano_error_t result = cardano_proposal_procedure_from_cbor(reader, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_proposal_procedure_to_cbor, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_proposal_procedure_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_proposal_procedure_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_proposal_procedure_to_cbor((cardano_proposal_procedure_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

// Proposals specific tests

TEST(cardano_proposal_procedure_new_parameter_change_action, canCreate)
{
  // Arrange
  cardano_parameter_change_action_t* action         = new_parameter_change_action();
  cardano_reward_address_t*          reward_address = new_reward_address();
  cardano_anchor_t*                  anchor         = new_anchor();

  // Act

  cardano_proposal_procedure_t* proposal_procedure = NULL;

  EXPECT_THAT(cardano_proposal_procedure_new_parameter_change_action(DEPOSIT, reward_address, anchor, action, &proposal_procedure), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(action, testing::Not((cardano_parameter_change_action_t*)nullptr));

  // Serialize and compare results

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t result = cardano_proposal_procedure_to_cbor(proposal_procedure, writer);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, PARAMETER_CHANGE_PROPOSAL_CBOR);

  // Cleanup
  cardano_parameter_change_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_proposal_procedure_new_parameter_change_action, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = new_reward_address();
  cardano_anchor_t*         anchor         = new_anchor();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_parameter_change_action(DEPOSIT, reward_address, anchor, nullptr, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_parameter_change_action, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_parameter_change_action_t* action = new_parameter_change_action();
  cardano_anchor_t*                  anchor = new_anchor();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_parameter_change_action(DEPOSIT, nullptr, anchor, action, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_parameter_change_action_unref(&action);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_parameter_change_action, returnsErrorIfAnchorIsNull)
{
  // Arrange
  cardano_parameter_change_action_t* action         = new_parameter_change_action();
  cardano_reward_address_t*          reward_address = new_reward_address();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_parameter_change_action(DEPOSIT, reward_address, nullptr, action, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_parameter_change_action_unref(&action);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_proposal_procedure_new_parameter_change_action, returnsErrorIfProposalProcedureIsNull)
{
  // Arrange
  cardano_parameter_change_action_t* action         = new_parameter_change_action();
  cardano_reward_address_t*          reward_address = new_reward_address();
  cardano_anchor_t*                  anchor         = new_anchor();

  // Act
  cardano_error_t result = cardano_proposal_procedure_new_parameter_change_action(DEPOSIT, reward_address, anchor, action, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_parameter_change_action_unref(&action);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_parameter_change_action, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_parameter_change_action_t* action         = new_parameter_change_action();
  cardano_reward_address_t*          reward_address = new_reward_address();
  cardano_anchor_t*                  anchor         = new_anchor();

  cardano_proposal_procedure_t* proposal_procedure = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_proposal_procedure_new_parameter_change_action(DEPOSIT, reward_address, anchor, action, &proposal_procedure);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_parameter_change_action_unref(&action);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_proposal_procedure_unref(&proposal_procedure);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_proposal_procedure_new_hard_fork_initiation_action, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_hard_fork_initiation_action_t* action         = new_hard_fork_initiation_action();
  cardano_reward_address_t*              reward_address = new_reward_address();
  cardano_anchor_t*                      anchor         = new_anchor();

  cardano_proposal_procedure_t* proposal_procedure = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_proposal_procedure_new_hard_fork_initiation_action(DEPOSIT, reward_address, anchor, action, &proposal_procedure);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_hard_fork_initiation_action_unref(&action);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_proposal_procedure_unref(&proposal_procedure);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_proposal_procedure_new_treasury_withdrawals_action, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_treasury_withdrawals_action_t* action         = new_treasury_withdrawals_action();
  cardano_reward_address_t*              reward_address = new_reward_address();
  cardano_anchor_t*                      anchor         = new_anchor();

  cardano_proposal_procedure_t* proposal_procedure = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_proposal_procedure_new_treasury_withdrawals_action(DEPOSIT, reward_address, anchor, action, &proposal_procedure);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_proposal_procedure_new_no_confidence_action, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_no_confidence_action_t* action         = new_no_confidence_action();
  cardano_reward_address_t*       reward_address = new_reward_address();
  cardano_anchor_t*               anchor         = new_anchor();

  cardano_proposal_procedure_t* proposal_procedure = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_proposal_procedure_new_no_confidence_action(DEPOSIT, reward_address, anchor, action, &proposal_procedure);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_no_confidence_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_proposal_procedure_new_update_committee_action, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_update_committee_action_t* action         = new_update_committee_action();
  cardano_reward_address_t*          reward_address = new_reward_address();
  cardano_anchor_t*                  anchor         = new_anchor();

  cardano_proposal_procedure_t* proposal_procedure = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_proposal_procedure_new_update_committee_action(DEPOSIT, reward_address, anchor, action, &proposal_procedure);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_update_committee_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_proposal_procedure_new_constitution_action, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_new_constitution_action_t* action         = new_new_constitution_action();
  cardano_reward_address_t*          reward_address = new_reward_address();
  cardano_anchor_t*                  anchor         = new_anchor();

  cardano_proposal_procedure_t* proposal_procedure = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_proposal_procedure_new_constitution_action(DEPOSIT, reward_address, anchor, action, &proposal_procedure);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_new_constitution_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_proposal_procedure_new_info_action, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_info_action_t*    action         = new_info_action();
  cardano_reward_address_t* reward_address = new_reward_address();
  cardano_anchor_t*         anchor         = new_anchor();

  cardano_proposal_procedure_t* proposal_procedure = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_proposal_procedure_new_info_action(DEPOSIT, reward_address, anchor, action, &proposal_procedure);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_info_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_proposal_procedure_to_parameter_change_action, canCreate)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(PARAMETER_CHANGE_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_parameter_change_action_t* action = NULL;

  EXPECT_THAT(cardano_proposal_procedure_to_parameter_change_action(proposal_procedure, &action), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(action, testing::Not((cardano_parameter_change_action_t*)nullptr));

  // Cleanup
  cardano_parameter_change_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_parameter_change_action, returnsErrorIfProposalProcedureIsNull)
{
  // Act
  cardano_parameter_change_action_t* action = NULL;

  cardano_error_t result = cardano_proposal_procedure_to_parameter_change_action(nullptr, &action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposal_procedure_to_parameter_change_action, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(PARAMETER_CHANGE_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_error_t result = cardano_proposal_procedure_to_parameter_change_action(proposal_procedure, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_parameter_change_action, returnsErrorIfActionIsNotAParameterChangeAction)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(INFO_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_parameter_change_action_t* action = NULL;

  cardano_error_t result = cardano_proposal_procedure_to_parameter_change_action(proposal_procedure, &action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_PROCEDURE_PROPOSAL_TYPE);

  // Cleanup
  cardano_parameter_change_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_hard_fork_initiation_action, canCreate)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(HARD_FORK_INITIATION_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_hard_fork_initiation_action_t* action = NULL;

  EXPECT_THAT(cardano_proposal_procedure_to_hard_fork_initiation_action(proposal_procedure, &action), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(action, testing::Not((cardano_hard_fork_initiation_action_t*)nullptr));

  // Cleanup
  cardano_hard_fork_initiation_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_hard_fork_initiation_action, returnsErrorIfProposalProcedureIsNull)
{
  // Act
  cardano_hard_fork_initiation_action_t* action = NULL;

  cardano_error_t result = cardano_proposal_procedure_to_hard_fork_initiation_action(nullptr, &action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposal_procedure_to_hard_fork_initiation_action, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(HARD_FORK_INITIATION_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_error_t result = cardano_proposal_procedure_to_hard_fork_initiation_action(proposal_procedure, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_hard_fork_initiation_action, returnsErrorIfActionIsNotAHardForkInitiationAction)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(INFO_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_hard_fork_initiation_action_t* action = NULL;

  cardano_error_t result = cardano_proposal_procedure_to_hard_fork_initiation_action(proposal_procedure, &action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_PROCEDURE_PROPOSAL_TYPE);

  // Cleanup
  cardano_hard_fork_initiation_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_treasury_withdrawals_action, canCreate)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(TREASURY_WITHDRAWALS_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_treasury_withdrawals_action_t* action = NULL;

  EXPECT_THAT(cardano_proposal_procedure_to_treasury_withdrawals_action(proposal_procedure, &action), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(action, testing::Not((cardano_treasury_withdrawals_action_t*)nullptr));

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_treasury_withdrawals_action, returnsErrorIfProposalProcedureIsNull)
{
  // Act
  cardano_treasury_withdrawals_action_t* action = NULL;

  cardano_error_t result = cardano_proposal_procedure_to_treasury_withdrawals_action(nullptr, &action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposal_procedure_to_treasury_withdrawals_action, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(TREASURY_WITHDRAWALS_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_error_t result = cardano_proposal_procedure_to_treasury_withdrawals_action(proposal_procedure, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_treasury_withdrawals_action, returnsErrorIfActionIsNotATreasuryWithdrawalsAction)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(INFO_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_treasury_withdrawals_action_t* action = NULL;

  cardano_error_t result = cardano_proposal_procedure_to_treasury_withdrawals_action(proposal_procedure, &action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_PROCEDURE_PROPOSAL_TYPE);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_no_confidence_action, canCreate)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(NO_CONFIDENCE_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_no_confidence_action_t* action = NULL;

  EXPECT_THAT(cardano_proposal_procedure_to_no_confidence_action(proposal_procedure, &action), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(action, testing::Not((cardano_no_confidence_action_t*)nullptr));

  // Cleanup
  cardano_no_confidence_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_no_confidence_action, returnsErrorIfProposalProcedureIsNull)
{
  // Act
  cardano_no_confidence_action_t* action = NULL;

  cardano_error_t result = cardano_proposal_procedure_to_no_confidence_action(nullptr, &action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposal_procedure_to_no_confidence_action, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(NO_CONFIDENCE_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_error_t result = cardano_proposal_procedure_to_no_confidence_action(proposal_procedure, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_no_confidence_action, returnsErrorIfActionIsNotANoConfidenceAction)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(INFO_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_no_confidence_action_t* action = NULL;

  cardano_error_t result = cardano_proposal_procedure_to_no_confidence_action(proposal_procedure, &action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_PROCEDURE_PROPOSAL_TYPE);

  // Cleanup
  cardano_no_confidence_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_update_committee_action, canCreate)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(UPDATE_COMMITTEE_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_update_committee_action_t* action = NULL;

  EXPECT_THAT(cardano_proposal_procedure_to_update_committee_action(proposal_procedure, &action), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(action, testing::Not((cardano_update_committee_action_t*)nullptr));

  // Cleanup
  cardano_update_committee_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_update_committee_action, returnsErrorIfProposalProcedureIsNull)
{
  // Act
  cardano_update_committee_action_t* action = NULL;

  cardano_error_t result = cardano_proposal_procedure_to_update_committee_action(nullptr, &action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposal_procedure_to_update_committee_action, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(UPDATE_COMMITTEE_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_error_t result = cardano_proposal_procedure_to_update_committee_action(proposal_procedure, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_update_committee_action, returnsErrorIfActionIsNotAnUpdateCommitteeAction)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(INFO_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_update_committee_action_t* action = NULL;

  cardano_error_t result = cardano_proposal_procedure_to_update_committee_action(proposal_procedure, &action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_PROCEDURE_PROPOSAL_TYPE);

  // Cleanup
  cardano_update_committee_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_new_constitution_action, canCreate)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(NEW_CONSTITUTION_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_new_constitution_action_t* action = NULL;

  EXPECT_THAT(cardano_proposal_procedure_to_constitution_action(proposal_procedure, &action), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(action, testing::Not((cardano_new_constitution_action_t*)nullptr));

  // Cleanup
  cardano_new_constitution_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_new_constitution_action, returnsErrorIfProposalProcedureIsNull)
{
  // Act
  cardano_new_constitution_action_t* action = NULL;

  cardano_error_t result = cardano_proposal_procedure_to_constitution_action(nullptr, &action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposal_procedure_to_new_constitution_action, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(NEW_CONSTITUTION_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_error_t result = cardano_proposal_procedure_to_constitution_action(proposal_procedure, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_new_constitution_action, returnsErrorIfActionIsNotAConstitutionAction)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(INFO_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_new_constitution_action_t* action = NULL;

  cardano_error_t result = cardano_proposal_procedure_to_constitution_action(proposal_procedure, &action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_PROCEDURE_PROPOSAL_TYPE);

  // Cleanup
  cardano_new_constitution_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_info_action, canCreate)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(INFO_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_info_action_t* action = NULL;

  EXPECT_THAT(cardano_proposal_procedure_to_info_action(proposal_procedure, &action), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(action, testing::Not((cardano_info_action_t*)nullptr));

  // Cleanup
  cardano_info_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_info_action, returnsErrorIfProposalProcedureIsNull)
{
  // Act
  cardano_info_action_t* action = NULL;

  cardano_error_t result = cardano_proposal_procedure_to_info_action(nullptr, &action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposal_procedure_to_info_action, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(INFO_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_error_t result = cardano_proposal_procedure_to_info_action(proposal_procedure, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_to_info_action, returnsErrorIfActionIsNotAnInfoAction)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(NEW_CONSTITUTION_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_info_action_t* action = NULL;

  cardano_error_t result = cardano_proposal_procedure_to_info_action(proposal_procedure, &action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_PROCEDURE_PROPOSAL_TYPE);

  // Cleanup
  cardano_info_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_new_hard_fork_initiation_action, canCreate)
{
  // Arrange
  cardano_hard_fork_initiation_action_t* action         = new_hard_fork_initiation_action();
  cardano_reward_address_t*              reward_address = new_reward_address();
  cardano_anchor_t*                      anchor         = new_anchor();

  EXPECT_NE(action, nullptr);

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  EXPECT_THAT(cardano_proposal_procedure_new_hard_fork_initiation_action(DEPOSIT, reward_address, anchor, action, &proposal_procedure), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(action, testing::Not((cardano_hard_fork_initiation_action_t*)nullptr));

  // Serialize and compare results

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t result = cardano_proposal_procedure_to_cbor(proposal_procedure, writer);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, HARD_FORK_INITIATION_PROPOSAL_CBOR);

  // Cleanup
  cardano_hard_fork_initiation_action_unref(&action);

  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_proposal_procedure_unref(&proposal_procedure);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_proposal_procedure_new_hard_fork_initiation_action, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = new_reward_address();
  cardano_anchor_t*         anchor         = new_anchor();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_hard_fork_initiation_action(DEPOSIT, reward_address, anchor, nullptr, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_hard_fork_initiation_action, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_hard_fork_initiation_action_t* action = new_hard_fork_initiation_action();
  cardano_anchor_t*                      anchor = new_anchor();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_hard_fork_initiation_action(DEPOSIT, nullptr, anchor, action, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_hard_fork_initiation_action_unref(&action);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_hard_fork_initiation_action, returnsErrorIfAnchorIsNull)
{
  // Arrange
  cardano_hard_fork_initiation_action_t* action         = new_hard_fork_initiation_action();
  cardano_reward_address_t*              reward_address = new_reward_address();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_hard_fork_initiation_action(DEPOSIT, reward_address, nullptr, action, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_hard_fork_initiation_action_unref(&action);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_proposal_procedure_new_hard_fork_initiation_action, returnsErrorIfProposalProcedureIsNull)
{
  // Arrange
  cardano_hard_fork_initiation_action_t* action         = new_hard_fork_initiation_action();
  cardano_reward_address_t*              reward_address = new_reward_address();
  cardano_anchor_t*                      anchor         = new_anchor();

  // Act
  cardano_error_t result = cardano_proposal_procedure_new_hard_fork_initiation_action(DEPOSIT, reward_address, anchor, action, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_hard_fork_initiation_action_unref(&action);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_treasury_withdrawals_action, canCreate)
{
  // Arrange
  cardano_treasury_withdrawals_action_t* action         = new_treasury_withdrawals_action();
  cardano_reward_address_t*              reward_address = new_reward_address();
  cardano_anchor_t*                      anchor         = new_anchor();

  EXPECT_NE(action, nullptr);

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  EXPECT_THAT(cardano_proposal_procedure_new_treasury_withdrawals_action(DEPOSIT, reward_address, anchor, action, &proposal_procedure), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(action, testing::Not((cardano_treasury_withdrawals_action_t*)nullptr));

  // Serialize and compare results

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t result = cardano_proposal_procedure_to_cbor(proposal_procedure, writer);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, TREASURY_WITHDRAWALS_PROPOSAL_CBOR);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&action);

  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_proposal_procedure_unref(&proposal_procedure);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_proposal_procedure_new_treasury_withdrawals_action, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = new_reward_address();
  cardano_anchor_t*         anchor         = new_anchor();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_treasury_withdrawals_action(DEPOSIT, reward_address, anchor, nullptr, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_treasury_withdrawals_action, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_treasury_withdrawals_action_t* action = new_treasury_withdrawals_action();
  cardano_anchor_t*                      anchor = new_anchor();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_treasury_withdrawals_action(DEPOSIT, nullptr, anchor, action, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&action);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_treasury_withdrawals_action, returnsErrorIfAnchorIsNull)
{
  // Arrange
  cardano_treasury_withdrawals_action_t* action         = new_treasury_withdrawals_action();
  cardano_reward_address_t*              reward_address = new_reward_address();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_treasury_withdrawals_action(DEPOSIT, reward_address, nullptr, action, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&action);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_proposal_procedure_new_treasury_withdrawals_action, returnsErrorIfProposalProcedureIsNull)
{
  // Arrange
  cardano_treasury_withdrawals_action_t* action         = new_treasury_withdrawals_action();
  cardano_reward_address_t*              reward_address = new_reward_address();
  cardano_anchor_t*                      anchor         = new_anchor();

  // Act
  cardano_error_t result = cardano_proposal_procedure_new_treasury_withdrawals_action(DEPOSIT, reward_address, anchor, action, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_treasury_withdrawals_action_unref(&action);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_no_confidence_action, canCreate)
{
  // Arrange
  cardano_no_confidence_action_t* action         = new_no_confidence_action();
  cardano_reward_address_t*       reward_address = new_reward_address();
  cardano_anchor_t*               anchor         = new_anchor();

  EXPECT_NE(action, nullptr);

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  EXPECT_THAT(cardano_proposal_procedure_new_no_confidence_action(DEPOSIT, reward_address, anchor, action, &proposal_procedure), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(action, testing::Not((cardano_no_confidence_action_t*)nullptr));

  // Serialize and compare results

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t result = cardano_proposal_procedure_to_cbor(proposal_procedure, writer);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, NO_CONFIDENCE_PROPOSAL_CBOR);

  // Cleanup
  cardano_no_confidence_action_unref(&action);

  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_proposal_procedure_unref(&proposal_procedure);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_proposal_procedure_new_no_confidence_action, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = new_reward_address();
  cardano_anchor_t*         anchor         = new_anchor();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_no_confidence_action(DEPOSIT, reward_address, anchor, nullptr, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_no_confidence_action, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_no_confidence_action_t* action = new_no_confidence_action();
  cardano_anchor_t*               anchor = new_anchor();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_no_confidence_action(DEPOSIT, nullptr, anchor, action, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_no_confidence_action_unref(&action);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_no_confidence_action, returnsErrorIfAnchorIsNull)
{
  // Arrange
  cardano_no_confidence_action_t* action         = new_no_confidence_action();
  cardano_reward_address_t*       reward_address = new_reward_address();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_no_confidence_action(DEPOSIT, reward_address, nullptr, action, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_no_confidence_action_unref(&action);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_proposal_procedure_new_no_confidence_action, returnsErrorIfProposalProcedureIsNull)
{
  // Arrange
  cardano_no_confidence_action_t* action         = new_no_confidence_action();
  cardano_reward_address_t*       reward_address = new_reward_address();
  cardano_anchor_t*               anchor         = new_anchor();

  // Act
  cardano_error_t result = cardano_proposal_procedure_new_no_confidence_action(DEPOSIT, reward_address, anchor, action, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_no_confidence_action_unref(&action);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_update_committee_action, canCreate)
{
  // Arrange
  cardano_update_committee_action_t* action         = new_update_committee_action();
  cardano_reward_address_t*          reward_address = new_reward_address();
  cardano_anchor_t*                  anchor         = new_anchor();

  EXPECT_NE(action, nullptr);

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  EXPECT_THAT(cardano_proposal_procedure_new_update_committee_action(DEPOSIT, reward_address, anchor, action, &proposal_procedure), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(action, testing::Not((cardano_update_committee_action_t*)nullptr));

  // Serialize and compare results

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t result = cardano_proposal_procedure_to_cbor(proposal_procedure, writer);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, UPDATE_COMMITTEE_PROPOSAL_CBOR);

  // Cleanup
  cardano_update_committee_action_unref(&action);

  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_proposal_procedure_unref(&proposal_procedure);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_proposal_procedure_new_update_committee_action, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = new_reward_address();
  cardano_anchor_t*         anchor         = new_anchor();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_update_committee_action(DEPOSIT, reward_address, anchor, nullptr, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_update_committee_action, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_update_committee_action_t* action = new_update_committee_action();
  cardano_anchor_t*                  anchor = new_anchor();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_update_committee_action(DEPOSIT, nullptr, anchor, action, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_update_committee_action_unref(&action);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_update_committee_action, returnsErrorIfAnchorIsNull)
{
  // Arrange
  cardano_update_committee_action_t* action         = new_update_committee_action();
  cardano_reward_address_t*          reward_address = new_reward_address();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_update_committee_action(DEPOSIT, reward_address, nullptr, action, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_update_committee_action_unref(&action);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_proposal_procedure_new_update_committee_action, returnsErrorIfProposalProcedureIsNull)
{
  // Arrange
  cardano_update_committee_action_t* action         = new_update_committee_action();
  cardano_reward_address_t*          reward_address = new_reward_address();
  cardano_anchor_t*                  anchor         = new_anchor();

  // Act
  cardano_error_t result = cardano_proposal_procedure_new_update_committee_action(DEPOSIT, reward_address, anchor, action, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_update_committee_action_unref(&action);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_constitution_action, canCreate)
{
  // Arrange
  cardano_new_constitution_action_t* action         = new_new_constitution_action();
  cardano_reward_address_t*          reward_address = new_reward_address();
  cardano_anchor_t*                  anchor         = new_anchor();

  EXPECT_NE(action, nullptr);

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  EXPECT_THAT(cardano_proposal_procedure_new_constitution_action(DEPOSIT, reward_address, anchor, action, &proposal_procedure), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(action, testing::Not((cardano_new_constitution_action_t*)nullptr));

  // Serialize and compare results

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t result = cardano_proposal_procedure_to_cbor(proposal_procedure, writer);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, NEW_CONSTITUTION_PROPOSAL_CBOR);

  // Cleanup
  cardano_new_constitution_action_unref(&action);

  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_proposal_procedure_unref(&proposal_procedure);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_proposal_procedure_new_constitution_action, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = new_reward_address();
  cardano_anchor_t*         anchor         = new_anchor();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_constitution_action(DEPOSIT, reward_address, anchor, nullptr, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_constitution_action, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_new_constitution_action_t* action = new_new_constitution_action();
  cardano_anchor_t*                  anchor = new_anchor();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_constitution_action(DEPOSIT, nullptr, anchor, action, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_new_constitution_action_unref(&action);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_constitution_action, returnsErrorIfAnchorIsNull)
{
  // Arrange
  cardano_new_constitution_action_t* action         = new_new_constitution_action();
  cardano_reward_address_t*          reward_address = new_reward_address();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_constitution_action(DEPOSIT, reward_address, nullptr, action, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_new_constitution_action_unref(&action);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_proposal_procedure_new_constitution_action, returnsErrorIfProposalProcedureIsNull)
{
  // Arrange
  cardano_new_constitution_action_t* action         = new_new_constitution_action();
  cardano_reward_address_t*          reward_address = new_reward_address();
  cardano_anchor_t*                  anchor         = new_anchor();

  // Act
  cardano_error_t result = cardano_proposal_procedure_new_constitution_action(DEPOSIT, reward_address, anchor, action, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_new_constitution_action_unref(&action);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_info_action, canCreate)
{
  // Arrange
  cardano_info_action_t*    action         = new_info_action();
  cardano_reward_address_t* reward_address = new_reward_address();
  cardano_anchor_t*         anchor         = new_anchor();

  EXPECT_NE(action, nullptr);

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  EXPECT_THAT(cardano_proposal_procedure_new_info_action(DEPOSIT, reward_address, anchor, action, &proposal_procedure), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(action, testing::Not((cardano_info_action_t*)nullptr));

  // Serialize and compare results

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t result = cardano_proposal_procedure_to_cbor(proposal_procedure, writer);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, INFO_PROPOSAL_CBOR);

  // Cleanup
  cardano_info_action_unref(&action);

  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_proposal_procedure_unref(&proposal_procedure);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_proposal_procedure_new_info_action, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = new_reward_address();
  cardano_anchor_t*         anchor         = new_anchor();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_info_action(DEPOSIT, reward_address, anchor, nullptr, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_info_action, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_info_action_t* action = new_info_action();
  cardano_anchor_t*      anchor = new_anchor();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_info_action(DEPOSIT, nullptr, anchor, action, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_info_action_unref(&action);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_new_info_action, returnsErrorIfAnchorIsNull)
{
  // Arrange
  cardano_info_action_t*    action         = new_info_action();
  cardano_reward_address_t* reward_address = new_reward_address();

  // Act
  cardano_proposal_procedure_t* proposal_procedure = NULL;

  cardano_error_t result = cardano_proposal_procedure_new_info_action(DEPOSIT, reward_address, nullptr, action, &proposal_procedure);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_info_action_unref(&action);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_proposal_procedure_new_info_action, returnsErrorIfProposalProcedureIsNull)
{
  // Arrange
  cardano_info_action_t*    action         = new_info_action();
  cardano_reward_address_t* reward_address = new_reward_address();
  cardano_anchor_t*         anchor         = new_anchor();

  // Act
  cardano_error_t result = cardano_proposal_procedure_new_info_action(DEPOSIT, reward_address, anchor, action, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_info_action_unref(&action);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_get_action_type, canGetActionType)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(HARD_FORK_INITIATION_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_governance_action_type_t action_type;

  EXPECT_EQ(cardano_proposal_procedure_get_action_type(proposal_procedure, &action_type), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(action_type, CARDANO_GOVERNANCE_ACTION_TYPE_HARD_FORK_INITIATION);

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_get_action_type, returnsErrorIfProposalProcedureIsNull)
{
  // Act
  cardano_governance_action_type_t action_type;

  cardano_error_t result = cardano_proposal_procedure_get_action_type(nullptr, &action_type);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposal_procedure_get_action_type, returnsErrorIfActionTypeIsNull)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(HARD_FORK_INITIATION_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_error_t result = cardano_proposal_procedure_get_action_type(proposal_procedure, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_set_anchor, canSetAnchor)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(HARD_FORK_INITIATION_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  cardano_anchor_t* anchor = new_anchor();
  EXPECT_NE(anchor, nullptr);

  // Act
  EXPECT_EQ(cardano_proposal_procedure_set_anchor(proposal_procedure, anchor), CARDANO_SUCCESS);

  // Assert
  cardano_anchor_t* result = cardano_proposal_procedure_get_anchor(proposal_procedure);
  EXPECT_EQ(result, anchor);

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
  cardano_anchor_unref(&anchor);
  cardano_anchor_unref(&result);
}

TEST(cardano_proposal_procedure_set_anchor, returnsErrorIfProposalProcedureIsNull)
{
  // Arrange
  cardano_anchor_t* anchor = new_anchor();
  EXPECT_NE(anchor, nullptr);

  // Act
  cardano_error_t result = cardano_proposal_procedure_set_anchor(nullptr, anchor);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_anchor_unref(&anchor);
}

TEST(cardano_proposal_procedure_set_anchor, returnsErrorIfAnchorIsNull)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(HARD_FORK_INITIATION_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_error_t result = cardano_proposal_procedure_set_anchor(proposal_procedure, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_get_anchor, canGetAnchor)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(HARD_FORK_INITIATION_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  cardano_anchor_t* anchor = new_anchor();
  EXPECT_NE(anchor, nullptr);

  EXPECT_EQ(cardano_proposal_procedure_set_anchor(proposal_procedure, anchor), CARDANO_SUCCESS);

  // Act
  cardano_anchor_t* result = cardano_proposal_procedure_get_anchor(proposal_procedure);

  // Assert
  EXPECT_EQ(result, anchor);

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
  cardano_anchor_unref(&anchor);
  cardano_anchor_unref(&result);
}

TEST(cardano_proposal_procedure_get_anchor, returnsNullIfProposalProcedureIsNull)
{
  // Act
  cardano_anchor_t* result = cardano_proposal_procedure_get_anchor(nullptr);

  // Assert
  ASSERT_EQ(result, nullptr);
}

TEST(cardano_proposal_procedure_set_reward_address, canSetRewardAddress)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(HARD_FORK_INITIATION_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  cardano_reward_address_t* reward_address = new_reward_address();
  EXPECT_NE(reward_address, nullptr);

  // Act
  EXPECT_EQ(cardano_proposal_procedure_set_reward_address(proposal_procedure, reward_address), CARDANO_SUCCESS);

  // Assert
  cardano_reward_address_t* result = cardano_proposal_procedure_get_reward_address(proposal_procedure);
  EXPECT_EQ(result, reward_address);

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
  cardano_reward_address_unref(&reward_address);
  cardano_reward_address_unref(&result);
}

TEST(cardano_proposal_procedure_set_reward_address, returnsErrorIfProposalProcedureIsNull)
{
  // Arrange
  cardano_reward_address_t* reward_address = new_reward_address();
  EXPECT_NE(reward_address, nullptr);

  // Act
  cardano_error_t result = cardano_proposal_procedure_set_reward_address(nullptr, reward_address);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_proposal_procedure_set_reward_address, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(HARD_FORK_INITIATION_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  cardano_error_t result = cardano_proposal_procedure_set_reward_address(proposal_procedure, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_get_reward_address, canGetRewardAddress)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(HARD_FORK_INITIATION_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  cardano_reward_address_t* reward_address = new_reward_address();
  EXPECT_NE(reward_address, nullptr);

  EXPECT_EQ(cardano_proposal_procedure_set_reward_address(proposal_procedure, reward_address), CARDANO_SUCCESS);

  // Act
  cardano_reward_address_t* result = cardano_proposal_procedure_get_reward_address(proposal_procedure);

  // Assert
  EXPECT_EQ(result, reward_address);

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
  cardano_reward_address_unref(&reward_address);
  cardano_reward_address_unref(&result);
}

TEST(cardano_proposal_procedure_get_reward_address, returnsNullIfProposalProcedureIsNull)
{
  // Act
  cardano_reward_address_t* result = cardano_proposal_procedure_get_reward_address(nullptr);

  // Assert
  ASSERT_EQ(result, nullptr);
}

TEST(cardano_proposal_procedure_set_deposit, canSetDeposit)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(HARD_FORK_INITIATION_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  // Act
  EXPECT_EQ(cardano_proposal_procedure_set_deposit(proposal_procedure, DEPOSIT), CARDANO_SUCCESS);

  // Assert
  uint64_t result = cardano_proposal_procedure_get_deposit(proposal_procedure);
  EXPECT_EQ(result, DEPOSIT);

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_set_deposit, returnsErrorIfProposalProcedureIsNull)
{
  // Act
  cardano_error_t result = cardano_proposal_procedure_set_deposit(nullptr, DEPOSIT);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_proposal_procedure_get_deposit, canGetDeposit)
{
  // Arrange
  cardano_proposal_procedure_t* proposal_procedure = new_default_proposal_procedure(HARD_FORK_INITIATION_PROPOSAL_CBOR);
  EXPECT_NE(proposal_procedure, nullptr);

  EXPECT_EQ(cardano_proposal_procedure_set_deposit(proposal_procedure, DEPOSIT), CARDANO_SUCCESS);

  // Act
  uint64_t result = cardano_proposal_procedure_get_deposit(proposal_procedure);

  // Assert
  EXPECT_EQ(result, DEPOSIT);

  // Cleanup
  cardano_proposal_procedure_unref(&proposal_procedure);
}

TEST(cardano_proposal_procedure_get_deposit, returnsZeroIfProposalProcedureIsNull)
{
  // Act
  uint64_t result = cardano_proposal_procedure_get_deposit(nullptr);

  // Assert
  ASSERT_EQ(result, 0);
}
