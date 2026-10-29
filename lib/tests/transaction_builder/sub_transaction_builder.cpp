/**
 * \file sub_transaction_builder.cpp
 *
 * \author angel.castillo
 * \date   Sep 18, 2026
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

#include <cardano/error.h>

#include <cardano/transaction_builder/sub_transaction_builder.h>

#include "../../src/transaction_builder/internals/builder_state.h"
#include <allocators.h>
#include <cardano/address/enterprise_address.h>
#include <cardano/certs/unregistration_cert.h>
#include <cardano/common/datum.h>
#include <cardano/proposal_procedures/hard_fork_initiation_action.h>
#include <cardano/transaction_body/account_balance_intervals_map.h>
#include <cardano/transaction_body/direct_deposit_map.h>
#include <cardano/transaction_body/required_guards_map.h>
#include <cardano/transaction_body/sub_transaction_body.h>
#include <cardano/transaction_body/sub_transaction_set.h>
#include <cardano/transaction_builder/balancing/transaction_balancing.h>
#include <cardano/transaction_builder/evaluation/native_tx_evaluator.h>
#include <cardano/transaction_builder/script_data_hash.h>
#include <gmock/gmock.h>
#include <tests/allocators_helpers.h>

/* SUB TX BUILDER INTERNALS **************************************************/

typedef struct cardano_sub_tx_builder_t
{
    cardano_object_t        base;
    cardano_error_t         last_error;
    cardano_builder_state_t state;
} cardano_sub_tx_builder_t;

/* CONSTANTS *****************************************************************/

static const char* COSTMDLS_ALL_CBOR           = "a30098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a0298b31a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a01020304";
static const char* UTXO_WITH_ASSETS_CBOR       = "82825820027b68d4c11e97d7e065cc2702912cb1a21b6d0e56c6a74dd605889a5561138500a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a00118f32a1581c0b0d621b5c26d0a1fd0893a4b04c19d860296a69ede1fbcfc5179882a1474e46542d30303101";
static const char* UTXO_CBOR                   = "82825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a63301a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8011a0dff3f6f";
static const char* UTXO_WITH_REF_SCRIPT_NATIVE = "82825820bb277abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e003a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d818582282008200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d398538";
static const char* OUTPUT_CBOR                 = "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011";
static const char* PLUTUS_DATA_CBOR            = "d8799f0102030405ff";
static const char* OTHER_PLUTUS_DATA_CBOR      = "d87980";
static const char* ADDRESS                     = "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg";
static const char* REWARD_ADDRESS              = "stake_test1uppy2gm2hqzkwc80em4mlat73j4jyqvzhclrvsu72g9xg4q2yweet";
static const char* REWARD_ADDRESS2             = "stake_test1uqehkck0lajq8gr28t9uxnuvgcqrc6070x3k9r8048z8y5gssrtvn";
static const char* POOL_ID                     = "pool1pzdqdxrv0k74p4q33y98f2u7vzaz95et7mjeedjcfy0jcgk754f";
static const char* POOL_ID_HEX                 = "089a06986c7dbd50d411890a74ab9e60ba22d32bf6e59cb658491f2c";
static const char* DREP_ID                     = "drep15cfxz9exyn5rx0807zvxfrvslrjqfchrd4d47kv9e0f46uedqtc";
static const char* ANCHOR_URL                  = "https://storage.googleapis.com/biglup/Angel_Castillo.jsonld";
static const char* ANCHOR_HASH                 = "26ce09df4e6f64fe5cf248968ab78f4b8a0092580c234d78f68c079c0fce34f0";
static const char* GOVERNANCE_ACTION_ID        = "gov_action1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqpzklpgpf";
static const char* VOTER_CBOR                  = "8200581c00000000000000000000000000000000000000000000000000000000";
static const char* VOTING_PROCEDURE_CBOR       = "8201f6";
static const char* CERTIFICATE_CBOR            = "83028200581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef92";
static const char* CREDENTIAL_SET_CBOR         = "d90102848200581c000000000000000000000000000000000000000000000000000000008200581c100000000000000000000000000000000000000000000000000000008200581c200000000000000000000000000000000000000000000000000000008200581c30000000000000000000000000000000000000000000000000000000";
static const char* COMMITTEE_MEMBERS_MAP_CBOR  = "a48200581c00000000000000000000000000000000000000000000000000000000008200581c10000000000000000000000000000000000000000000000000000000018200581c20000000000000000000000000000000000000000000000000000000028200581c3000000000000000000000000000000000000000000000000000000003";
static const char* CONSTITUTION_CBOR           = "82827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000f6";
static const char* FIRST_ERROR                 = "Direct deposit amount must be greater than zero.";
static const char* HASH_HEX                    = "00000000000000000000000000000000000000000000000000000000";
static const char* SCRIPT_HASH_HEX             = "966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";
static const char* ASSET_ID_HEX                = "0000000000000000000000000000000000000000000000000000000054455854";
static const char* ASSET_NAME_HEX              = "54455854";
static const char* NATIVE_SCRIPT_CBOR          = "82008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0";
static const char* PLUTUS_V1_CBOR              = "82014e4d01000033222220051200120011";
static const char* PLUTUS_V2_CBOR              = "82024e4d01000033222220051200120011";
static const char* PLUTUS_V3_CBOR              = "82034e4d01000033222220051200120011";
static const char* PLUTUS_V4_CBOR              = "82044e4d01000033222220051200120011";
static const char* PLUTUS_SCRIPT_ERROR         = "PlutusV1, PlutusV2 and PlutusV3 scripts can not run inside a sub transaction, only native and PlutusV4 scripts are supported.";
static const char* METADATA_JSON               = "{ \"name\": \"test\" }";

static const uint64_t KEY_DEPOSIT               = 2000000;
static const uint64_t DREP_DEPOSIT              = 500000000;
static const uint64_t GOVERNANCE_ACTION_DEPOSIT = 100000000000;

static const char* EMPTY_BODY_CBOR                = "a200d90102800180";
static const char* NETWORK_ID_BODY_CBOR           = "a300d901028001800f01";
static const char* DONATION_BODY_CBOR             = "a300d90102800180161a000f4240";
static const char* INVALID_AFTER_BODY_CBOR        = "a300d90102800180031903e8";
static const char* INVALID_BEFORE_BODY_CBOR       = "a300d90102800180081903e8";
static const char* INPUTS_BODY_CBOR               = "a200d9010282825820027b68d4c11e97d7e065cc2702912cb1a21b6d0e56c6a74dd605889a5561138500825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a633010180";
static const char* REFERENCE_INPUT_BODY_CBOR      = "a300d9010280018012d9010281825820bb277abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e003";
static const char* OUTPUT_BODY_CBOR               = "a200d90102800181a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011";
static const char* SEND_LOVELACE_BODY_CBOR        = "a200d90102800181a200583910c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251011a001e8480";
static const char* SEND_VALUE_BODY_CBOR           = "a200d90102800181a200583910c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c4725101821a00118f32a1581c0b0d621b5c26d0a1fd0893a4b04c19d860296a69ede1fbcfc5179882a1474e46542d30303101";
static const char* METADATA_BODY_CBOR             = "a300d90102800180075820e628f35cc5d8b14c57a01ec12c7169729e294b5dd83ef01273b20b5c3cafc244";
static const char* METADATA_SUB_TX_CBOR           = "83a300d90102800180075820e628f35cc5d8b14c57a01ec12c7169729e294b5dd83ef01273b20b5c3cafc244a0d90103a100a11902a2a1646e616d656474657374";
static const char* MINT_BODY_CBOR                 = "a300d9010280018009a1581c00000000000000000000000000000000000000000000000000000000a1445445585404";
static const char* NATIVE_SCRIPT_SUB_TX_CBOR      = "83a200d90102800180a101d90102818202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0f6";
static const char* KEY_HASH_GUARD_BODY_CBOR       = "a300d901028001800ed9010281581c00000000000000000000000000000000000000000000000000000000";
static const char* SCRIPT_HASH_GUARD_BODY_CBOR    = "a300d901028001800ed90102828200581c000000000000000000000000000000000000000000000000000000008201581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";
static const char* REQUIRED_GUARD_BODY_CBOR       = "a300d901028001801818a18201581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37f6";
static const char* REQUIRED_GUARD_DATUM_BODY_CBOR = "a300d901028001801818a18201581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37d8799f0102030405ff";
static const char* DIRECT_DEPOSIT_BODY_CBOR       = "a300d901028001801819a1581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a64541a002dc6c0";
static const char* BALANCE_INTERVAL_BODY_CBOR     = "a300d90102800180181aa1581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a6454821a000f4240f6";
static const char* FULL_SUB_TX_CBOR               = "83ad00d9010281825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a633010181a200583910c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251011a001e8480031907d0075820e628f35cc5d8b14c57a01ec12c7169729e294b5dd83ef01273b20b5c3cafc244081903e809a1581c00000000000000000000000000000000000000000000000000000000a14454455854040ed9010281581c000000000000000000000000000000000000000000000000000000000f0012d9010281825820bb277abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e003161a0007a1201818a18201581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37d8799f0102030405ff1819a1581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a64541a002dc6c0181aa1581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a6454821a000f4240f6a101d90102818202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0d90103a100a11902a2a1646e616d656474657374";

static const char* WITHDRAWAL_BODY_CBOR                = "a300d9010280018005a1581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a64541a0026a331";
static const char* REGISTER_REWARD_ADDRESS_BODY_CBOR   = "a300d9010280018004d901028183078200581c4245236ab8056760efceebbff57e8cab220182be3e36439e520a64541a001e8480";
static const char* DEREGISTER_REWARD_ADDRESS_BODY_CBOR = "a300d9010280018004d901028183088200581c4245236ab8056760efceebbff57e8cab220182be3e36439e520a64541a001e8480";
static const char* DELEGATE_STAKE_BODY_CBOR            = "a300d9010280018004d901028183028200581c4245236ab8056760efceebbff57e8cab220182be3e36439e520a6454581c089a06986c7dbd50d411890a74ab9e60ba22d32bf6e59cb658491f2c";
static const char* DELEGATE_VOTING_POWER_BODY_CBOR     = "a300d9010280018004d901028183098200581c4245236ab8056760efceebbff57e8cab220182be3e36439e520a64548200581ca61261172624e8333ceff098648d90f8e404e2e36d5b5f5985cbd35d";
static const char* REGISTER_DREP_BODY_CBOR             = "a300d9010280018004d901028184108200581ca61261172624e8333ceff098648d90f8e404e2e36d5b5f5985cbd35d1a1dcd650082783b68747470733a2f2f73746f726167652e676f6f676c65617069732e636f6d2f6269676c75702f416e67656c5f43617374696c6c6f2e6a736f6e6c64582026ce09df4e6f64fe5cf248968ab78f4b8a0092580c234d78f68c079c0fce34f0";
static const char* UPDATE_DREP_BODY_CBOR               = "a300d9010280018004d901028183128200581ca61261172624e8333ceff098648d90f8e404e2e36d5b5f5985cbd35d82783b68747470733a2f2f73746f726167652e676f6f676c65617069732e636f6d2f6269676c75702f416e67656c5f43617374696c6c6f2e6a736f6e6c64582026ce09df4e6f64fe5cf248968ab78f4b8a0092580c234d78f68c079c0fce34f0";
static const char* DEREGISTER_DREP_BODY_CBOR           = "a300d9010280018004d901028183118200581ca61261172624e8333ceff098648d90f8e404e2e36d5b5f5985cbd35d1a1dcd6500";
static const char* VOTE_BODY_CBOR                      = "a300d9010280018013a18200581c00000000000000000000000000000000000000000000000000000000a18258200000000000000000000000000000000000000000000000000000000000000000118201f6";
static const char* CERTIFICATE_BODY_CBOR               = "a300d9010280018004d901028183028200581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef92";
static const char* HARDFORK_BODY_CBOR                  = "a300d9010280018014d9010281841b000000174876e800581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a64548301825820000000000000000000000000000000000000000000000000000000000000000011820c0082783b68747470733a2f2f73746f726167652e676f6f676c65617069732e636f6d2f6269676c75702f416e67656c5f43617374696c6c6f2e6a736f6e6c64582026ce09df4e6f64fe5cf248968ab78f4b8a0092580c234d78f68c079c0fce34f0";
static const char* NO_CONFIDENCE_BODY_CBOR             = "a300d9010280018014d9010281841b000000174876e800581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a6454820382582000000000000000000000000000000000000000000000000000000000000000001182783b68747470733a2f2f73746f726167652e676f6f676c65617069732e636f6d2f6269676c75702f416e67656c5f43617374696c6c6f2e6a736f6e6c64582026ce09df4e6f64fe5cf248968ab78f4b8a0092580c234d78f68c079c0fce34f0";
static const char* UPDATE_COMMITTEE_BODY_CBOR          = "a300d9010280018014d9010281841b000000174876e800581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a64548504825820000000000000000000000000000000000000000000000000000000000000000011d90102848200581c000000000000000000000000000000000000000000000000000000008200581c100000000000000000000000000000000000000000000000000000008200581c200000000000000000000000000000000000000000000000000000008200581c30000000000000000000000000000000000000000000000000000000a48200581c00000000000000000000000000000000000000000000000000000000008200581c10000000000000000000000000000000000000000000000000000000018200581c20000000000000000000000000000000000000000000000000000000028200581c3000000000000000000000000000000000000000000000000000000003d81e82010282783b68747470733a2f2f73746f726167652e676f6f676c65617069732e636f6d2f6269676c75702f416e67656c5f43617374696c6c6f2e6a736f6e6c64582026ce09df4e6f64fe5cf248968ab78f4b8a0092580c234d78f68c079c0fce34f0";
static const char* NEW_CONSTITUTION_BODY_CBOR          = "a300d9010280018014d9010281841b000000174876e800581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a6454830582582000000000000000000000000000000000000000000000000000000000000000001182827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000f682783b68747470733a2f2f73746f726167652e676f6f676c65617069732e636f6d2f6269676c75702f416e67656c5f43617374696c6c6f2e6a736f6e6c64582026ce09df4e6f64fe5cf248968ab78f4b8a0092580c234d78f68c079c0fce34f0";
static const char* INFO_BODY_CBOR                      = "a300d9010280018014d9010281841b000000174876e800581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a6454810682783b68747470733a2f2f73746f726167652e676f6f676c65617069732e636f6d2f6269676c75702f416e67656c5f43617374696c6c6f2e6a736f6e6c64582026ce09df4e6f64fe5cf248968ab78f4b8a0092580c234d78f68c079c0fce34f0";
static const char* GOVERNANCE_SUB_TX_CBOR              = "83a600d9010280018004d901028283078200581c4245236ab8056760efceebbff57e8cab220182be3e36439e520a64541a001e848083028200581c4245236ab8056760efceebbff57e8cab220182be3e36439e520a6454581c089a06986c7dbd50d411890a74ab9e60ba22d32bf6e59cb658491f2c05a1581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a64541a0026a33113a18200581c00000000000000000000000000000000000000000000000000000000a18258200000000000000000000000000000000000000000000000000000000000000000118201f614d9010281841b000000174876e800581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a6454810682783b68747470733a2f2f73746f726167652e676f6f676c65617069732e636f6d2f6269676c75702f416e67656c5f43617374696c6c6f2e6a736f6e6c64582026ce09df4e6f64fe5cf248968ab78f4b8a0092580c234d78f68c079c0fce34f0a0f6";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the protocol parameters.
 * \return A new instance of the protocol parameters.
 */
static cardano_protocol_parameters_t*
init_protocol_parameters()
{
  cardano_protocol_parameters_t* params = NULL;

  cardano_error_t result = cardano_protocol_parameters_new(&params);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(COSTMDLS_ALL_CBOR, strlen(COSTMDLS_ALL_CBOR));

  cardano_costmdls_t* costmdls = NULL;
  result                       = cardano_costmdls_from_cbor(reader, &costmdls);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_cost_models(params, costmdls);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);
  cardano_costmdls_unref(&costmdls);

  return params;
}

/**
 * Creates a new default instance of the protocol parameters with the deposits set.
 * \return A new instance of the protocol parameters.
 */
static cardano_protocol_parameters_t*
init_protocol_parameters_with_deposits()
{
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  EXPECT_EQ(cardano_protocol_parameters_set_key_deposit(params, KEY_DEPOSIT), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_drep_deposit(params, DREP_DEPOSIT), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_governance_action_deposit(params, GOVERNANCE_ACTION_DEPOSIT), CARDANO_SUCCESS);

  return params;
}

/**
 * Creates a hash from its hexadecimal representation.
 * \param hex The hash in hexadecimal format.
 * \return A new instance of the hash.
 */
static cardano_blake2b_hash_t*
create_hash(const char* hex)
{
  cardano_blake2b_hash_t* hash = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(hex, strlen(hex), &hash), CARDANO_SUCCESS);

  return hash;
}

/**
 * Creates a DRep from its bech32 representation.
 * \param drep_id The DRep ID in CIP-105 or CIP-129 format.
 * \return A new instance of the DRep.
 */
static cardano_drep_t*
create_drep(const char* drep_id)
{
  cardano_drep_t* drep = NULL;

  EXPECT_EQ(cardano_drep_from_string(drep_id, strlen(drep_id), &drep), CARDANO_SUCCESS);

  return drep;
}

/**
 * Creates the default anchor.
 * \return A new instance of the anchor.
 */
static cardano_anchor_t*
create_anchor()
{
  cardano_anchor_t* anchor = NULL;

  EXPECT_EQ(cardano_anchor_from_hash_hex(ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), &anchor), CARDANO_SUCCESS);

  return anchor;
}

/**
 * Creates the default governance action id.
 * \return A new instance of the governance action id.
 */
static cardano_governance_action_id_t*
create_governance_action_id()
{
  cardano_governance_action_id_t* action_id = NULL;

  EXPECT_EQ(cardano_governance_action_id_from_bech32(GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), &action_id), CARDANO_SUCCESS);

  return action_id;
}

/**
 * Creates a voter from its CBOR representation.
 * \param cbor The CBOR hex string of the voter.
 * \return A new instance of the voter.
 */
static cardano_voter_t*
create_voter(const char* cbor)
{
  cardano_voter_t* voter = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_voter_from_cbor(reader, &voter);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return voter;
}

/**
 * Creates a voting procedure from its CBOR representation.
 * \param cbor The CBOR hex string of the voting procedure.
 * \return A new instance of the voting procedure.
 */
static cardano_voting_procedure_t*
create_voting_procedure(const char* cbor)
{
  cardano_voting_procedure_t* voting_procedure = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_voting_procedure_from_cbor(reader, &voting_procedure);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return voting_procedure;
}

/**
 * Creates a certificate from its CBOR representation.
 * \param cbor The CBOR hex string of the certificate.
 * \return A new instance of the certificate.
 */
static cardano_certificate_t*
create_certificate(const char* cbor)
{
  cardano_certificate_t* certificate = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_certificate_from_cbor(reader, &certificate);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return certificate;
}

/**
 * Creates a protocol version.
 * \param major The major protocol version.
 * \param minor The minor protocol version.
 * \return A new instance of the protocol version.
 */
static cardano_protocol_version_t*
create_protocol_version(const uint64_t major, const uint64_t minor)
{
  cardano_protocol_version_t* version = NULL;

  EXPECT_EQ(cardano_protocol_version_new(major, minor, &version), CARDANO_SUCCESS);

  return version;
}

/**
 * Creates a credential set from its CBOR representation.
 * \param cbor The CBOR hex string of the credential set.
 * \return A new instance of the credential set.
 */
static cardano_credential_set_t*
create_credential_set(const char* cbor)
{
  cardano_credential_set_t* credentials = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_credential_set_from_cbor(reader, &credentials);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return credentials;
}

/**
 * Creates a committee members map from its CBOR representation.
 * \param cbor The CBOR hex string of the committee members map.
 * \return A new instance of the committee members map.
 */
static cardano_committee_members_map_t*
create_committee_members_map(const char* cbor)
{
  cardano_committee_members_map_t* members = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_committee_members_map_from_cbor(reader, &members);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return members;
}

/**
 * Creates a constitution from its CBOR representation.
 * \param cbor The CBOR hex string of the constitution.
 * \return A new instance of the constitution.
 */
static cardano_constitution_t*
create_constitution(const char* cbor)
{
  cardano_constitution_t* constitution = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_constitution_from_cbor(reader, &constitution);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return constitution;
}

/**
 * Creates a unit interval from a floating point value.
 * \param value The value of the unit interval.
 * \return A new instance of the unit interval.
 */
static cardano_unit_interval_t*
create_unit_interval(const double value)
{
  cardano_unit_interval_t* unit_interval = NULL;

  EXPECT_EQ(cardano_unit_interval_from_double(value, &unit_interval), CARDANO_SUCCESS);

  return unit_interval;
}

/**
 * Creates a script from its CBOR representation.
 * \param script The CBOR hex string of the script.
 * \return A new instance of the script.
 */
static cardano_script_t*
create_script(const char* script)
{
  cardano_script_t*      result = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(script, strlen(script));
  cardano_error_t        error  = cardano_script_from_cbor(reader, &result);

  EXPECT_THAT(error, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return result;
}

/**
 * Creates a plutus data from its CBOR representation.
 * \param cbor The CBOR hex string of the plutus data.
 * \return A new instance of the plutus data.
 */
static cardano_plutus_data_t*
create_plutus_data(const char* cbor)
{
  cardano_plutus_data_t* data = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_plutus_data_from_cbor(reader, &data);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return data;
}

/**
 * Creates a UTXO from its CBOR representation.
 * \param cbor The CBOR hex string of the UTXO.
 * \return A new instance of the UTXO.
 */
static cardano_utxo_t*
create_utxo(const char* cbor)
{
  cardano_utxo_t* utxo = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_utxo_from_cbor(reader, &utxo);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return utxo;
}

/**
 * Creates a transaction output from its CBOR representation.
 * \param cbor The CBOR hex string of the transaction output.
 * \return A new instance of the transaction output.
 */
static cardano_transaction_output_t*
create_output(const char* cbor)
{
  cardano_transaction_output_t* output = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &output);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return output;
}

/**
 * Creates a credential from a hash in hexadecimal format.
 * \param hash_hex The hash of the credential in hexadecimal format.
 * \param type The type of the credential.
 * \return A new instance of the credential.
 */
static cardano_credential_t*
create_credential(const char* hash_hex, const cardano_credential_type_t type)
{
  cardano_credential_t* credential = NULL;
  cardano_error_t       result     = cardano_credential_from_hash_hex(hash_hex, strlen(hash_hex), type, &credential);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  return credential;
}

/**
 * Creates a reward address from its bech32 representation.
 * \param bech32 The bech32 string of the reward address.
 * \return A new instance of the reward address.
 */
static cardano_reward_address_t*
create_reward_address(const char* bech32)
{
  cardano_reward_address_t* reward_address = NULL;

  EXPECT_EQ(cardano_reward_address_from_bech32(bech32, strlen(bech32), &reward_address), CARDANO_SUCCESS);

  return reward_address;
}

/**
 * Creates an address from its string representation.
 * \param address The string representation of the address.
 * \return A new instance of the address.
 */
static cardano_address_t*
create_address(const char* address)
{
  cardano_address_t* result = NULL;

  EXPECT_EQ(cardano_address_from_string(address, strlen(address), &result), CARDANO_SUCCESS);

  return result;
}

/**
 * Creates an account balance interval with only an inclusive lower bound.
 * \param lower_bound The inclusive lower bound in lovelace.
 * \return A new instance of the account balance interval.
 */
static cardano_account_balance_interval_t*
create_interval(const uint64_t lower_bound)
{
  cardano_account_balance_interval_t* interval = NULL;

  EXPECT_EQ(cardano_account_balance_interval_new(&lower_bound, NULL, &interval), CARDANO_SUCCESS);

  return interval;
}

/**
 * Builds the sub transaction of a builder that is expected to succeed.
 * \param builder The sub transaction builder.
 * \return The sub transaction. The caller must release it.
 */
static cardano_sub_transaction_t*
build_sub_transaction(cardano_sub_tx_builder_t* builder)
{
  cardano_sub_transaction_t* sub_tx = NULL;

  EXPECT_EQ(cardano_sub_tx_builder_build(builder, &sub_tx), CARDANO_SUCCESS);

  return sub_tx;
}

/**
 * Gets a borrowed reference to the body of a sub transaction.
 * \param sub_tx The sub transaction.
 * \return The body of the sub transaction.
 */
static cardano_sub_transaction_body_t*
get_body(cardano_sub_transaction_t* sub_tx)
{
  cardano_sub_transaction_body_t* body = cardano_sub_transaction_get_body(sub_tx);
  cardano_sub_transaction_body_unref(&body);

  return body;
}

/**
 * Encodes the body of a sub transaction to a CBOR hex string.
 * \param sub_tx The sub transaction.
 * \return The CBOR hex string. The caller must free the returned string.
 */
static char*
encode_body(cardano_sub_transaction_t* sub_tx)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_sub_transaction_body_to_cbor(get_body(sub_tx), writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        body_hex = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, body_hex, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return body_hex;
}

/**
 * Encodes a sub transaction to a CBOR hex string.
 * \param sub_tx The sub transaction to encode.
 * \return The CBOR hex string. The caller must free the returned string.
 */
static char*
encode_sub_transaction(cardano_sub_transaction_t* sub_tx)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_sub_transaction_to_cbor(sub_tx, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        tx_hex   = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, tx_hex, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return tx_hex;
}

/**
 * Builds the sub transaction of a builder and encodes its body to a CBOR hex string.
 * \param builder The sub transaction builder.
 * \return The CBOR hex string. The caller must free the returned string.
 */
static char*
build_and_encode_body(cardano_sub_tx_builder_t* builder)
{
  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  char* body_hex = encode_body(sub_tx);

  cardano_sub_transaction_unref(&sub_tx);

  return body_hex;
}

/**
 * Gets a borrowed reference to the body of the transaction a builder is working on.
 * \param builder The sub transaction builder.
 * \return The body of the transaction under construction.
 */
static cardano_transaction_body_t*
get_state_body(cardano_sub_tx_builder_t* builder)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  return body;
}

/**
 * Gets the type of a certificate of a certificate set.
 * \param certificates The certificate set.
 * \param index The index of the certificate.
 * \return The type of the certificate.
 */
static cardano_cert_type_t
get_certificate_type(cardano_certificate_set_t* certificates, const size_t index)
{
  cardano_certificate_t* certificate = NULL;
  cardano_cert_type_t    type        = CARDANO_CERT_TYPE_STAKE_REGISTRATION;

  EXPECT_EQ(cardano_certificate_set_get(certificates, index, &certificate), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cert_get_type(certificate, &type), CARDANO_SUCCESS);

  cardano_certificate_unref(&certificate);

  return type;
}

/**
 * Gets the governance action type of a proposal of a proposal procedure set.
 * \param proposals The proposal procedure set.
 * \param index The index of the proposal.
 * \return The governance action type of the proposal.
 */
static cardano_governance_action_type_t
get_proposal_type(cardano_proposal_procedure_set_t* proposals, const size_t index)
{
  cardano_proposal_procedure_t*    proposal = NULL;
  cardano_governance_action_type_t type     = CARDANO_GOVERNANCE_ACTION_TYPE_PARAMETER_CHANGE;

  EXPECT_EQ(cardano_proposal_procedure_set_get(proposals, index, &proposal), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_proposal_procedure_get_action_type(proposal, &type), CARDANO_SUCCESS);

  cardano_proposal_procedure_unref(&proposal);

  return type;
}

/**
 * Gets the deposit of a proposal of a proposal procedure set.
 * \param proposals The proposal procedure set.
 * \param index The index of the proposal.
 * \return The deposit of the proposal in lovelace.
 */
static uint64_t
get_proposal_deposit(cardano_proposal_procedure_set_t* proposals, const size_t index)
{
  cardano_proposal_procedure_t* proposal = NULL;

  EXPECT_EQ(cardano_proposal_procedure_set_get(proposals, index, &proposal), CARDANO_SUCCESS);

  const uint64_t deposit = cardano_proposal_procedure_get_deposit(proposal);

  cardano_proposal_procedure_unref(&proposal);

  return deposit;
}

/**
 * Gets the protocol version proposed by a hard fork initiation proposal of a proposal procedure set.
 * \param proposals The proposal procedure set.
 * \param index The index of the proposal.
 * \return The proposed protocol version. The caller must release it.
 */
static cardano_protocol_version_t*
get_hardfork_proposal_version(cardano_proposal_procedure_set_t* proposals, const size_t index)
{
  cardano_proposal_procedure_t*          proposal = NULL;
  cardano_hard_fork_initiation_action_t* action   = NULL;

  EXPECT_EQ(cardano_proposal_procedure_set_get(proposals, index, &proposal), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_proposal_procedure_to_hard_fork_initiation_action(proposal, &action), CARDANO_SUCCESS);

  cardano_protocol_version_t* version = cardano_hard_fork_initiation_action_get_protocol_version(action);

  cardano_hard_fork_initiation_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal);

  return version;
}

/**
 * Puts a builder in error state by adding a direct deposit of zero lovelace.
 * \param builder The sub transaction builder.
 */
static void
record_first_error(cardano_sub_tx_builder_t* builder)
{
  cardano_sub_tx_builder_add_direct_deposit_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 0);

  EXPECT_EQ(builder->last_error, CARDANO_ERROR_INVALID_ARGUMENT);
}

/**
 * Gets the first output of a sub transaction.
 * \param sub_tx The sub transaction.
 * \return A new reference to the first output.
 */
static cardano_transaction_output_t*
get_first_output(cardano_sub_transaction_t* sub_tx)
{
  cardano_transaction_output_list_t* outputs = cardano_sub_transaction_body_get_outputs(get_body(sub_tx));
  cardano_transaction_output_t*      output  = NULL;

  EXPECT_EQ(cardano_transaction_output_list_get(outputs, 0, &output), CARDANO_SUCCESS);

  cardano_transaction_output_list_unref(&outputs);

  return output;
}

/**
 * Creates an inline datum from plutus data.
 * \param data The plutus data carried by the datum.
 * \return A new instance of the datum.
 */
static cardano_datum_t*
create_inline_datum(cardano_plutus_data_t* data)
{
  cardano_datum_t* datum = NULL;

  EXPECT_EQ(cardano_datum_new_inline_data(data, &datum), CARDANO_SUCCESS);

  return datum;
}

/**
 * Creates a datum that carries the hash of a datum.
 * \param hash_hex The datum hash, in hexadecimal format.
 * \return A new instance of the datum.
 */
static cardano_datum_t*
create_hash_datum(const char* hash_hex)
{
  cardano_datum_t* datum = NULL;

  EXPECT_EQ(cardano_datum_new_data_hash_hex(hash_hex, strlen(hash_hex), &datum), CARDANO_SUCCESS);

  return datum;
}

/**
 * Creates a value of lovelace and one native asset.
 * \return A new instance of the value.
 */
static cardano_value_t*
create_value_with_assets()
{
  cardano_utxo_t*               utxo     = create_utxo(UTXO_WITH_ASSETS_CBOR);
  cardano_transaction_output_t* resolved = cardano_utxo_get_output(utxo);
  cardano_value_t*              value    = cardano_transaction_output_get_value(resolved);

  cardano_transaction_output_unref(&resolved);
  cardano_utxo_unref(&utxo);

  return value;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_sub_tx_builder_new, canCreateASubTxBuilder)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act
  cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Assert
  EXPECT_THAT(builder, testing::Not((cardano_sub_tx_builder_t*)nullptr));
  EXPECT_EQ(builder->last_error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_tx_builder_refcount(builder), 1);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_new, returnsNullWhenGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act & Assert
  EXPECT_THAT(cardano_sub_tx_builder_new(nullptr, nullptr), testing::IsNull());
  EXPECT_THAT(cardano_sub_tx_builder_new(nullptr, &CARDANO_MAINNET_SLOT_CONFIG), testing::IsNull());
  EXPECT_THAT(cardano_sub_tx_builder_new(params, nullptr), testing::IsNull());

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_new, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 500) && !succeeded; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder != nullptr)
    {
      succeeded = true;

      EXPECT_EQ(builder->last_error, CARDANO_SUCCESS);
      EXPECT_EQ(cardano_sub_tx_builder_refcount(builder), 1U);
      EXPECT_THAT(builder->state.sub_transaction_inputs, testing::Not((cardano_utxo_list_t*)nullptr));
      EXPECT_THAT(builder->state.sub_transaction_reference_inputs, testing::Not((cardano_utxo_list_t*)nullptr));

      cardano_sub_tx_builder_unref(&builder);
    }

    EXPECT_EQ(cardano_protocol_parameters_refcount(params), 1U);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_ref(builder);

  // Assert
  EXPECT_THAT(builder, testing::Not((cardano_sub_tx_builder_t*)nullptr));
  EXPECT_EQ(cardano_sub_tx_builder_refcount(builder), 2);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_sub_tx_builder_ref(nullptr);
}

TEST(cardano_sub_tx_builder_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_sub_tx_builder_t* builder = nullptr;

  // Act
  cardano_sub_tx_builder_unref(&builder);
}

TEST(cardano_sub_tx_builder_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_sub_tx_builder_unref((cardano_sub_tx_builder_t**)nullptr);
}

TEST(cardano_sub_tx_builder_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_ref(builder);
  size_t ref_count = cardano_sub_tx_builder_refcount(builder);

  cardano_sub_tx_builder_unref(&builder);
  size_t updated_ref_count = cardano_sub_tx_builder_refcount(builder);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_ref(builder);
  size_t ref_count = cardano_sub_tx_builder_refcount(builder);

  cardano_sub_tx_builder_unref(&builder);
  size_t updated_ref_count = cardano_sub_tx_builder_refcount(builder);

  cardano_sub_tx_builder_unref(&builder);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(builder, (cardano_sub_tx_builder_t*)nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_sub_tx_builder_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_sub_tx_builder_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_sub_tx_builder_t* builder = nullptr;
  const char*               message = "This is a test message";

  // Act
  cardano_sub_tx_builder_set_last_error(builder, message);

  // Assert
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Object is NULL.");
}

TEST(cardano_sub_tx_builder_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  const char* message = nullptr;

  // Act
  cardano_sub_tx_builder_set_last_error(builder, message);

  // Assert
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "");

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_get_last_error, returnsTheMessageThatWasSet)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_set_last_error(builder, "This is a test message");

  // Assert
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "This is a test message");

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_set_network_id, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_set_network_id(nullptr, CARDANO_NETWORK_ID_MAIN_NET);
}

TEST(cardano_sub_tx_builder_set_network_id, canSetNetworkId)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_set_network_id(builder, CARDANO_NETWORK_ID_MAIN_NET);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  // Assert
  EXPECT_NE(cardano_sub_transaction_body_get_network_id(get_body(sub_tx)), nullptr);
  EXPECT_EQ(*cardano_sub_transaction_body_get_network_id(get_body(sub_tx)), CARDANO_NETWORK_ID_MAIN_NET);
  EXPECT_STREQ(body_hex, NETWORK_ID_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_set_network_id, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_sub_tx_builder_set_network_id(builder, CARDANO_NETWORK_ID_MAIN_NET);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_set_donation, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_set_donation(nullptr, 1000000);
}

TEST(cardano_sub_tx_builder_set_donation, canSetDonation)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_set_donation(builder, 1000000);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  // Assert
  EXPECT_NE(cardano_sub_transaction_body_get_donation(get_body(sub_tx)), nullptr);
  EXPECT_EQ(*cardano_sub_transaction_body_get_donation(get_body(sub_tx)), 1000000);
  EXPECT_STREQ(body_hex, DONATION_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_set_donation, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_sub_tx_builder_set_donation(builder, 1000000);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_set_invalid_after, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_set_invalid_after(nullptr, 1000);
}

TEST(cardano_sub_tx_builder_set_invalid_after, canSetInvalidAfter)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_set_invalid_after(builder, 1000);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  // Assert
  EXPECT_NE(cardano_sub_transaction_body_get_invalid_after(get_body(sub_tx)), nullptr);
  EXPECT_EQ(*cardano_sub_transaction_body_get_invalid_after(get_body(sub_tx)), 1000);
  EXPECT_EQ(cardano_sub_transaction_body_get_invalid_before(get_body(sub_tx)), nullptr);
  EXPECT_STREQ(body_hex, INVALID_AFTER_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_set_invalid_after, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_sub_tx_builder_set_invalid_after(builder, 1000);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_set_invalid_after_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_set_invalid_after_ex(nullptr, 1730901968);
}

TEST(cardano_sub_tx_builder_set_invalid_after_ex, canSetInvalidAfter)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_set_invalid_after_ex(builder, 1730901968);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  EXPECT_NE(cardano_sub_transaction_body_get_invalid_after(get_body(sub_tx)), nullptr);
  EXPECT_EQ(*cardano_sub_transaction_body_get_invalid_after(get_body(sub_tx)), 139335677);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_set_invalid_after_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_sub_tx_builder_set_invalid_after_ex(builder, 1730901968);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_set_invalid_before, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_set_invalid_before(nullptr, 1000);
}

TEST(cardano_sub_tx_builder_set_invalid_before, canSetInvalidBefore)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_set_invalid_before(builder, 1000);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  // Assert
  EXPECT_NE(cardano_sub_transaction_body_get_invalid_before(get_body(sub_tx)), nullptr);
  EXPECT_EQ(*cardano_sub_transaction_body_get_invalid_before(get_body(sub_tx)), 1000);
  EXPECT_EQ(cardano_sub_transaction_body_get_invalid_after(get_body(sub_tx)), nullptr);
  EXPECT_STREQ(body_hex, INVALID_BEFORE_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_set_invalid_before, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_sub_tx_builder_set_invalid_before(builder, 1000);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_set_invalid_before_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_set_invalid_before_ex(nullptr, 1730901968);
}

TEST(cardano_sub_tx_builder_set_invalid_before_ex, canSetInvalidBefore)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_set_invalid_before_ex(builder, 1730901968);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  EXPECT_NE(cardano_sub_transaction_body_get_invalid_before(get_body(sub_tx)), nullptr);
  EXPECT_EQ(*cardano_sub_transaction_body_get_invalid_before(get_body(sub_tx)), 139335677);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_set_invalid_before_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_sub_tx_builder_set_invalid_before_ex(builder, 1730901968);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_input, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_add_input(nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_add_input, returnsErrorIfUtxoIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_add_input(builder, nullptr, nullptr, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "UTXO is required");

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_input, canAddInputs)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_utxo_t*                utxo1   = create_utxo(UTXO_CBOR);
  cardano_utxo_t*                utxo2   = create_utxo(UTXO_WITH_ASSETS_CBOR);

  // Act
  cardano_sub_tx_builder_add_input(builder, utxo1, nullptr, nullptr);
  cardano_sub_tx_builder_add_input(builder, utxo2, nullptr, nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_transaction_input_set_t* inputs = cardano_sub_transaction_body_get_inputs(get_body(sub_tx));
  cardano_transaction_input_set_unref(&inputs);

  cardano_transaction_input_t* first_input = NULL;
  EXPECT_EQ(cardano_transaction_input_set_get(inputs, 0, &first_input), CARDANO_SUCCESS);
  cardano_transaction_input_unref(&first_input);

  cardano_transaction_input_t* expected_first_input = cardano_utxo_get_input(utxo2);
  cardano_transaction_input_unref(&expected_first_input);

  // Assert
  EXPECT_EQ(cardano_utxo_list_get_length(builder->state.pre_selected_inputs), 2);
  EXPECT_EQ(cardano_transaction_input_set_get_length(inputs), 2);
  EXPECT_TRUE(cardano_transaction_input_equals(first_input, expected_first_input));
  EXPECT_STREQ(body_hex, INPUTS_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo1);
  cardano_utxo_unref(&utxo2);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_add_input, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_utxo_t*                utxo   = create_utxo(UTXO_CBOR);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_add_input(builder, utxo, nullptr, nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_sub_tx_builder_add_input, returnsErrorWhenBuildingIfTheSameUtxoIsAddedTwice)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_utxo_t*                utxo    = create_utxo(UTXO_CBOR);

  cardano_sub_tx_builder_add_input(builder, utxo, nullptr, nullptr);

  EXPECT_EQ(builder->last_error, CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_input(builder, utxo, nullptr, nullptr);

  cardano_sub_transaction_t* sub_tx = NULL;
  const cardano_error_t      result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Input is already added to the transaction");
  EXPECT_EQ(sub_tx, nullptr);
  EXPECT_EQ(cardano_utxo_list_get_length(builder->state.pre_selected_inputs), 1U);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_sub_tx_builder_add_input, returnsErrorWhenBuildingIfAnEqualUtxoIsAddedTwice)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_utxo_t*                utxo    = create_utxo(UTXO_CBOR);
  cardano_utxo_t*                copy    = create_utxo(UTXO_CBOR);

  cardano_sub_tx_builder_add_input(builder, utxo, nullptr, nullptr);

  EXPECT_EQ(builder->last_error, CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_input(builder, copy, nullptr, nullptr);

  cardano_sub_transaction_t* sub_tx = NULL;
  const cardano_error_t      result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(utxo, copy);
  EXPECT_EQ(result, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Input is already added to the transaction");
  EXPECT_EQ(sub_tx, nullptr);
  EXPECT_EQ(cardano_utxo_list_get_length(builder->state.pre_selected_inputs), 1U);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&copy);
}

TEST(cardano_sub_tx_builder_add_input, spendsAnInputAddedOnceExactlyOnce)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_utxo_t*                utxo    = create_utxo(UTXO_CBOR);

  // Act
  cardano_sub_tx_builder_add_input(builder, utxo, nullptr, nullptr);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  cardano_transaction_input_set_t* inputs = cardano_sub_transaction_body_get_inputs(get_body(sub_tx));
  cardano_transaction_input_set_unref(&inputs);

  cardano_transaction_input_t* input = NULL;
  EXPECT_EQ(cardano_transaction_input_set_get(inputs, 0, &input), CARDANO_SUCCESS);
  cardano_transaction_input_unref(&input);

  cardano_transaction_input_t* expected_input = cardano_utxo_get_input(utxo);
  cardano_transaction_input_unref(&expected_input);

  // Assert
  EXPECT_EQ(cardano_transaction_input_set_get_length(inputs), 1U);
  EXPECT_TRUE(cardano_transaction_input_equals(input, expected_input));

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_sub_tx_builder_add_reference_input, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_add_reference_input(nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_add_reference_input, returnsErrorIfUtxoIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_add_reference_input(builder, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_reference_input, canAddReferenceInput)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_utxo_t*                utxo    = create_utxo(UTXO_WITH_REF_SCRIPT_NATIVE);

  // Act
  cardano_sub_tx_builder_add_reference_input(builder, utxo);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_transaction_input_set_t* reference_inputs = cardano_sub_transaction_body_get_reference_inputs(get_body(sub_tx));
  cardano_transaction_input_set_unref(&reference_inputs);

  cardano_transaction_input_set_t* inputs = cardano_sub_transaction_body_get_inputs(get_body(sub_tx));
  cardano_transaction_input_set_unref(&inputs);

  // Assert
  EXPECT_EQ(cardano_transaction_input_set_get_length(reference_inputs), 1);
  EXPECT_EQ(cardano_transaction_input_set_get_length(inputs), 0);
  EXPECT_STREQ(body_hex, REFERENCE_INPUT_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_add_reference_input, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_utxo_t*                utxo   = create_utxo(UTXO_WITH_REF_SCRIPT_NATIVE);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_add_reference_input(builder, utxo);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_sub_tx_builder_add_reference_input, returnsErrorWhenBuildingIfTheSameUtxoIsAddedTwice)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_utxo_t*                utxo    = create_utxo(UTXO_WITH_REF_SCRIPT_NATIVE);

  cardano_sub_tx_builder_add_reference_input(builder, utxo);

  EXPECT_EQ(builder->last_error, CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_reference_input(builder, utxo);

  cardano_sub_transaction_t* sub_tx = NULL;
  const cardano_error_t      result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Reference input is already added to the transaction.");
  EXPECT_EQ(sub_tx, nullptr);
  EXPECT_EQ(cardano_utxo_list_get_length(builder->state.reference_inputs), 1U);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_sub_tx_builder_add_reference_input, returnsErrorWhenBuildingIfAnEqualUtxoIsAddedTwice)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_utxo_t*                utxo    = create_utxo(UTXO_WITH_REF_SCRIPT_NATIVE);
  cardano_utxo_t*                copy    = create_utxo(UTXO_WITH_REF_SCRIPT_NATIVE);

  cardano_sub_tx_builder_add_reference_input(builder, utxo);

  EXPECT_EQ(builder->last_error, CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_reference_input(builder, copy);

  cardano_sub_transaction_t* sub_tx = NULL;
  const cardano_error_t      result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(utxo, copy);
  EXPECT_EQ(result, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Reference input is already added to the transaction.");
  EXPECT_EQ(sub_tx, nullptr);
  EXPECT_EQ(cardano_utxo_list_get_length(builder->state.reference_inputs), 1U);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&copy);
}

TEST(cardano_sub_tx_builder_add_output, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_add_output(nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_add_output, returnsErrorIfOutputIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_add_output(builder, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_output, canAddOutput)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_transaction_output_t*  output  = create_output(OUTPUT_CBOR);

  // Act
  cardano_sub_tx_builder_add_output(builder, output);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_transaction_output_list_t* outputs = cardano_sub_transaction_body_get_outputs(get_body(sub_tx));
  cardano_transaction_output_list_unref(&outputs);

  // Assert
  EXPECT_EQ(cardano_transaction_output_list_get_length(outputs), 1);
  EXPECT_STREQ(body_hex, OUTPUT_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_transaction_output_unref(&output);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_send_lovelace, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_send_lovelace(nullptr, nullptr, 0);
}

TEST(cardano_sub_tx_builder_send_lovelace, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_send_lovelace(builder, nullptr, 1000);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_send_lovelace, canSendLovelace)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_address_t*             address = create_address(ADDRESS);

  // Act
  cardano_sub_tx_builder_send_lovelace(builder, address, 2000000);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_transaction_output_list_t* outputs = cardano_sub_transaction_body_get_outputs(get_body(sub_tx));
  cardano_transaction_output_list_unref(&outputs);

  cardano_transaction_output_t* output = NULL;
  EXPECT_EQ(cardano_transaction_output_list_get(outputs, 0, &output), CARDANO_SUCCESS);
  cardano_transaction_output_unref(&output);

  cardano_value_t* value = cardano_transaction_output_get_value(output);
  cardano_value_unref(&value);

  // Assert
  EXPECT_EQ(cardano_transaction_output_list_get_length(outputs), 1);
  EXPECT_EQ(cardano_value_get_coin(value), 2000000);
  EXPECT_STREQ(body_hex, SEND_LOVELACE_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_address_unref(&address);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_send_lovelace, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_address_t*             address = create_address(ADDRESS);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_send_lovelace(builder, address, 2000000);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_address_unref(&address);
}

TEST(cardano_sub_tx_builder_send_lovelace_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_send_lovelace_ex(nullptr, nullptr, 0, 0);
}

TEST(cardano_sub_tx_builder_send_lovelace_ex, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_send_lovelace_ex(builder, nullptr, 0, 1000);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_send_lovelace_ex, reportsInvalidAddressWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_send_lovelace_ex(builder, "invalid_address", strlen("invalid_address"), 1000);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_EQ(result, builder->last_error);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_send_lovelace_ex, canSendLovelace)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_send_lovelace_ex(builder, ADDRESS, strlen(ADDRESS), 2000000);

  char* body_hex = build_and_encode_body(builder);

  // Assert
  EXPECT_STREQ(body_hex, SEND_LOVELACE_BODY_CBOR);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_send_value, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_send_value(nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_send_value, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_value_t*               value   = cardano_value_new_from_coin(2000000);

  // Act
  cardano_sub_tx_builder_send_value(builder, nullptr, value);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_value_unref(&value);
}

TEST(cardano_sub_tx_builder_send_value, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_address_t*             address = create_address(ADDRESS);

  // Act
  cardano_sub_tx_builder_send_value(builder, address, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_address_unref(&address);
}

TEST(cardano_sub_tx_builder_send_value, canSendValue)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_address_t*             address = create_address(ADDRESS);
  cardano_utxo_t*                utxo    = create_utxo(UTXO_WITH_ASSETS_CBOR);

  cardano_transaction_output_t* resolved = cardano_utxo_get_output(utxo);
  cardano_transaction_output_unref(&resolved);

  cardano_value_t* value = cardano_transaction_output_get_value(resolved);
  cardano_value_unref(&value);

  // Act
  cardano_sub_tx_builder_send_value(builder, address, value);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_transaction_output_list_t* outputs = cardano_sub_transaction_body_get_outputs(get_body(sub_tx));
  cardano_transaction_output_list_unref(&outputs);

  cardano_transaction_output_t* output = NULL;
  EXPECT_EQ(cardano_transaction_output_list_get(outputs, 0, &output), CARDANO_SUCCESS);
  cardano_transaction_output_unref(&output);

  cardano_value_t* sent_value = cardano_transaction_output_get_value(output);
  cardano_value_unref(&sent_value);

  // Assert
  EXPECT_TRUE(cardano_value_equals(sent_value, value));
  EXPECT_STREQ(body_hex, SEND_VALUE_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_address_unref(&address);
  cardano_utxo_unref(&utxo);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_send_value_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_send_value_ex(nullptr, nullptr, 0, nullptr);
}

TEST(cardano_sub_tx_builder_send_value_ex, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_value_t*               value   = cardano_value_new_from_coin(2000000);

  // Act
  cardano_sub_tx_builder_send_value_ex(builder, nullptr, 0, value);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_value_unref(&value);
}

TEST(cardano_sub_tx_builder_send_value_ex, canSendValue)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_utxo_t*                utxo    = create_utxo(UTXO_WITH_ASSETS_CBOR);

  cardano_transaction_output_t* resolved = cardano_utxo_get_output(utxo);
  cardano_transaction_output_unref(&resolved);

  cardano_value_t* value = cardano_transaction_output_get_value(resolved);
  cardano_value_unref(&value);

  // Act
  cardano_sub_tx_builder_send_value_ex(builder, ADDRESS, strlen(ADDRESS), value);

  char* body_hex = build_and_encode_body(builder);

  // Assert
  EXPECT_STREQ(body_hex, SEND_VALUE_BODY_CBOR);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_lock_lovelace, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_lock_lovelace(nullptr, nullptr, 0, nullptr);
}

TEST(cardano_sub_tx_builder_lock_lovelace, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_lock_lovelace(builder, nullptr, 2000000, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_lock_lovelace, canLockWithInlineDatum)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_address_t*             address = create_address(ADDRESS);
  cardano_plutus_data_t*         data    = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_datum_t*               datum   = create_inline_datum(data);

  // Act
  cardano_sub_tx_builder_lock_lovelace(builder, address, 2000000, datum);

  cardano_sub_transaction_t*    sub_tx         = build_sub_transaction(builder);
  cardano_transaction_output_t* output         = get_first_output(sub_tx);
  cardano_address_t*            locked_address = cardano_transaction_output_get_address(output);
  cardano_value_t*              locked_value   = cardano_transaction_output_get_value(output);
  cardano_datum_t*              locked_datum   = cardano_transaction_output_get_datum(output);
  cardano_witness_set_t*        witness_set    = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_plutus_data_set_t*    witness_datums = cardano_witness_set_get_plutus_data(witness_set);

  // Assert
  ASSERT_NE(sub_tx, nullptr);
  EXPECT_STREQ(cardano_address_get_string(locked_address), ADDRESS);
  EXPECT_EQ(cardano_value_get_coin(locked_value), 2000000U);
  ASSERT_NE(locked_datum, nullptr);
  cardano_datum_type_t locked_type = CARDANO_DATUM_TYPE_DATA_HASH;

  EXPECT_EQ(cardano_datum_get_type(locked_datum, &locked_type), CARDANO_SUCCESS);
  EXPECT_EQ(locked_type, CARDANO_DATUM_TYPE_INLINE_DATA);

  cardano_plutus_data_t* locked_data = cardano_datum_get_inline_data(locked_datum);

  EXPECT_TRUE(cardano_plutus_data_equals(locked_data, data));
  cardano_plutus_data_unref(&locked_data);
  EXPECT_EQ(witness_datums, nullptr);

  // Cleanup
  cardano_plutus_data_set_unref(&witness_datums);
  cardano_witness_set_unref(&witness_set);
  cardano_datum_unref(&locked_datum);
  cardano_value_unref(&locked_value);
  cardano_address_unref(&locked_address);
  cardano_transaction_output_unref(&output);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_datum_unref(&datum);
  cardano_plutus_data_unref(&data);
  cardano_address_unref(&address);
}

TEST(cardano_sub_tx_builder_lock_lovelace, canLockWithDatumHash)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_address_t*             address = create_address(ADDRESS);
  cardano_datum_t*               datum   = create_hash_datum("0000000000000000000000000000000000000000000000000000000000000000");

  // Act
  cardano_sub_tx_builder_lock_lovelace(builder, address, 2000000, datum);

  cardano_sub_transaction_t*    sub_tx         = build_sub_transaction(builder);
  cardano_transaction_output_t* output         = get_first_output(sub_tx);
  cardano_address_t*            locked_address = cardano_transaction_output_get_address(output);
  cardano_value_t*              locked_value   = cardano_transaction_output_get_value(output);
  cardano_datum_t*              locked_datum   = cardano_transaction_output_get_datum(output);
  cardano_witness_set_t*        witness_set    = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_plutus_data_set_t*    witness_datums = cardano_witness_set_get_plutus_data(witness_set);

  // Assert
  ASSERT_NE(sub_tx, nullptr);
  EXPECT_STREQ(cardano_address_get_string(locked_address), ADDRESS);
  EXPECT_EQ(cardano_value_get_coin(locked_value), 2000000U);
  ASSERT_NE(locked_datum, nullptr);
  cardano_datum_type_t locked_type = CARDANO_DATUM_TYPE_DATA_HASH;

  EXPECT_EQ(cardano_datum_get_type(locked_datum, &locked_type), CARDANO_SUCCESS);
  EXPECT_EQ(locked_type, CARDANO_DATUM_TYPE_DATA_HASH);
  EXPECT_TRUE(cardano_datum_equals(locked_datum, datum));
  EXPECT_EQ(witness_datums, nullptr);

  // Cleanup
  cardano_plutus_data_set_unref(&witness_datums);
  cardano_witness_set_unref(&witness_set);
  cardano_datum_unref(&locked_datum);
  cardano_value_unref(&locked_value);
  cardano_address_unref(&locked_address);
  cardano_transaction_output_unref(&output);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_datum_unref(&datum);
  cardano_address_unref(&address);
}

TEST(cardano_sub_tx_builder_lock_lovelace, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_address_t*             address = create_address(ADDRESS);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_lock_lovelace(builder, address, 2000000, nullptr);

  const cardano_error_t              result  = cardano_sub_tx_builder_build(builder, &sub_tx);
  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(get_state_body(builder));

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_output_list_get_length(outputs), 0U);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_transaction_output_list_unref(&outputs);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_address_unref(&address);
}

TEST(cardano_sub_tx_builder_lock_lovelace, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_address_t*             address = create_address(ADDRESS);
  cardano_plutus_data_t*         data    = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_datum_t*               datum   = create_inline_datum(data);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_lock_lovelace(builder, address, 2000000, datum);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_datum_unref(&datum);
  cardano_plutus_data_unref(&data);
  cardano_address_unref(&address);
}

TEST(cardano_sub_tx_builder_lock_lovelace_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_lock_lovelace_ex(nullptr, nullptr, 0, 0, nullptr);
}

TEST(cardano_sub_tx_builder_lock_lovelace_ex, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_lock_lovelace_ex(builder, nullptr, 0, 2000000, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_lock_lovelace_ex, canLockWithInlineDatum)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_plutus_data_t*         data    = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_datum_t*               datum   = create_inline_datum(data);

  // Act
  cardano_sub_tx_builder_lock_lovelace_ex(builder, ADDRESS, strlen(ADDRESS), 2000000, datum);

  cardano_sub_transaction_t*    sub_tx         = build_sub_transaction(builder);
  cardano_transaction_output_t* output         = get_first_output(sub_tx);
  cardano_address_t*            locked_address = cardano_transaction_output_get_address(output);
  cardano_value_t*              locked_value   = cardano_transaction_output_get_value(output);
  cardano_datum_t*              locked_datum   = cardano_transaction_output_get_datum(output);
  cardano_witness_set_t*        witness_set    = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_plutus_data_set_t*    witness_datums = cardano_witness_set_get_plutus_data(witness_set);

  // Assert
  ASSERT_NE(sub_tx, nullptr);
  EXPECT_STREQ(cardano_address_get_string(locked_address), ADDRESS);
  EXPECT_EQ(cardano_value_get_coin(locked_value), 2000000U);
  ASSERT_NE(locked_datum, nullptr);
  cardano_datum_type_t locked_type = CARDANO_DATUM_TYPE_DATA_HASH;

  EXPECT_EQ(cardano_datum_get_type(locked_datum, &locked_type), CARDANO_SUCCESS);
  EXPECT_EQ(locked_type, CARDANO_DATUM_TYPE_INLINE_DATA);

  cardano_plutus_data_t* locked_data = cardano_datum_get_inline_data(locked_datum);

  EXPECT_TRUE(cardano_plutus_data_equals(locked_data, data));
  cardano_plutus_data_unref(&locked_data);
  EXPECT_EQ(witness_datums, nullptr);

  // Cleanup
  cardano_plutus_data_set_unref(&witness_datums);
  cardano_witness_set_unref(&witness_set);
  cardano_datum_unref(&locked_datum);
  cardano_value_unref(&locked_value);
  cardano_address_unref(&locked_address);
  cardano_transaction_output_unref(&output);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_datum_unref(&datum);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_sub_tx_builder_lock_lovelace_ex, canLockWithDatumHash)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_datum_t*               datum   = create_hash_datum("0000000000000000000000000000000000000000000000000000000000000000");

  // Act
  cardano_sub_tx_builder_lock_lovelace_ex(builder, ADDRESS, strlen(ADDRESS), 2000000, datum);

  cardano_sub_transaction_t*    sub_tx         = build_sub_transaction(builder);
  cardano_transaction_output_t* output         = get_first_output(sub_tx);
  cardano_address_t*            locked_address = cardano_transaction_output_get_address(output);
  cardano_value_t*              locked_value   = cardano_transaction_output_get_value(output);
  cardano_datum_t*              locked_datum   = cardano_transaction_output_get_datum(output);
  cardano_witness_set_t*        witness_set    = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_plutus_data_set_t*    witness_datums = cardano_witness_set_get_plutus_data(witness_set);

  // Assert
  ASSERT_NE(sub_tx, nullptr);
  EXPECT_STREQ(cardano_address_get_string(locked_address), ADDRESS);
  EXPECT_EQ(cardano_value_get_coin(locked_value), 2000000U);
  ASSERT_NE(locked_datum, nullptr);
  cardano_datum_type_t locked_type = CARDANO_DATUM_TYPE_DATA_HASH;

  EXPECT_EQ(cardano_datum_get_type(locked_datum, &locked_type), CARDANO_SUCCESS);
  EXPECT_EQ(locked_type, CARDANO_DATUM_TYPE_DATA_HASH);
  EXPECT_TRUE(cardano_datum_equals(locked_datum, datum));
  EXPECT_EQ(witness_datums, nullptr);

  // Cleanup
  cardano_plutus_data_set_unref(&witness_datums);
  cardano_witness_set_unref(&witness_set);
  cardano_datum_unref(&locked_datum);
  cardano_value_unref(&locked_value);
  cardano_address_unref(&locked_address);
  cardano_transaction_output_unref(&output);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_datum_unref(&datum);
}

TEST(cardano_sub_tx_builder_lock_lovelace_ex, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_lock_lovelace_ex(builder, ADDRESS, strlen(ADDRESS), 2000000, nullptr);

  const cardano_error_t              result  = cardano_sub_tx_builder_build(builder, &sub_tx);
  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(get_state_body(builder));

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_output_list_get_length(outputs), 0U);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_transaction_output_list_unref(&outputs);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_lock_lovelace_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_plutus_data_t*         data   = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_datum_t*               datum  = create_inline_datum(data);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_lock_lovelace_ex(builder, ADDRESS, strlen(ADDRESS), 2000000, datum);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_THAT(builder->last_error, testing::AnyOf(CARDANO_ERROR_MEMORY_ALLOCATION_FAILED, CARDANO_ERROR_DECODING));
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_datum_unref(&datum);
  cardano_plutus_data_unref(&data);
}

TEST(cardano_sub_tx_builder_lock_value, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_lock_value(nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_lock_value, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_value_t*               amount  = create_value_with_assets();

  // Act
  cardano_sub_tx_builder_lock_value(builder, nullptr, amount, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_value_unref(&amount);
}

TEST(cardano_sub_tx_builder_lock_value, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_address_t*             address = create_address(ADDRESS);

  // Act
  cardano_sub_tx_builder_lock_value(builder, address, nullptr, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_address_unref(&address);
}

TEST(cardano_sub_tx_builder_lock_value, canLockWithInlineDatum)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_address_t*             address = create_address(ADDRESS);
  cardano_value_t*               amount  = create_value_with_assets();
  cardano_plutus_data_t*         data    = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_datum_t*               datum   = create_inline_datum(data);

  // Act
  cardano_sub_tx_builder_lock_value(builder, address, amount, datum);

  cardano_sub_transaction_t*    sub_tx         = build_sub_transaction(builder);
  cardano_transaction_output_t* output         = get_first_output(sub_tx);
  cardano_address_t*            locked_address = cardano_transaction_output_get_address(output);
  cardano_value_t*              locked_value   = cardano_transaction_output_get_value(output);
  cardano_datum_t*              locked_datum   = cardano_transaction_output_get_datum(output);
  cardano_witness_set_t*        witness_set    = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_plutus_data_set_t*    witness_datums = cardano_witness_set_get_plutus_data(witness_set);

  // Assert
  ASSERT_NE(sub_tx, nullptr);
  EXPECT_STREQ(cardano_address_get_string(locked_address), ADDRESS);
  EXPECT_TRUE(cardano_value_equals(locked_value, amount));
  ASSERT_NE(locked_datum, nullptr);
  cardano_datum_type_t locked_type = CARDANO_DATUM_TYPE_DATA_HASH;

  EXPECT_EQ(cardano_datum_get_type(locked_datum, &locked_type), CARDANO_SUCCESS);
  EXPECT_EQ(locked_type, CARDANO_DATUM_TYPE_INLINE_DATA);

  cardano_plutus_data_t* locked_data = cardano_datum_get_inline_data(locked_datum);

  EXPECT_TRUE(cardano_plutus_data_equals(locked_data, data));
  cardano_plutus_data_unref(&locked_data);
  EXPECT_EQ(witness_datums, nullptr);

  // Cleanup
  cardano_plutus_data_set_unref(&witness_datums);
  cardano_witness_set_unref(&witness_set);
  cardano_datum_unref(&locked_datum);
  cardano_value_unref(&locked_value);
  cardano_address_unref(&locked_address);
  cardano_transaction_output_unref(&output);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_datum_unref(&datum);
  cardano_plutus_data_unref(&data);
  cardano_address_unref(&address);
  cardano_value_unref(&amount);
}

TEST(cardano_sub_tx_builder_lock_value, canLockWithDatumHash)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_address_t*             address = create_address(ADDRESS);
  cardano_value_t*               amount  = create_value_with_assets();
  cardano_datum_t*               datum   = create_hash_datum("0000000000000000000000000000000000000000000000000000000000000000");

  // Act
  cardano_sub_tx_builder_lock_value(builder, address, amount, datum);

  cardano_sub_transaction_t*    sub_tx         = build_sub_transaction(builder);
  cardano_transaction_output_t* output         = get_first_output(sub_tx);
  cardano_address_t*            locked_address = cardano_transaction_output_get_address(output);
  cardano_value_t*              locked_value   = cardano_transaction_output_get_value(output);
  cardano_datum_t*              locked_datum   = cardano_transaction_output_get_datum(output);
  cardano_witness_set_t*        witness_set    = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_plutus_data_set_t*    witness_datums = cardano_witness_set_get_plutus_data(witness_set);

  // Assert
  ASSERT_NE(sub_tx, nullptr);
  EXPECT_STREQ(cardano_address_get_string(locked_address), ADDRESS);
  EXPECT_TRUE(cardano_value_equals(locked_value, amount));
  ASSERT_NE(locked_datum, nullptr);
  cardano_datum_type_t locked_type = CARDANO_DATUM_TYPE_DATA_HASH;

  EXPECT_EQ(cardano_datum_get_type(locked_datum, &locked_type), CARDANO_SUCCESS);
  EXPECT_EQ(locked_type, CARDANO_DATUM_TYPE_DATA_HASH);
  EXPECT_TRUE(cardano_datum_equals(locked_datum, datum));
  EXPECT_EQ(witness_datums, nullptr);

  // Cleanup
  cardano_plutus_data_set_unref(&witness_datums);
  cardano_witness_set_unref(&witness_set);
  cardano_datum_unref(&locked_datum);
  cardano_value_unref(&locked_value);
  cardano_address_unref(&locked_address);
  cardano_transaction_output_unref(&output);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_datum_unref(&datum);
  cardano_address_unref(&address);
  cardano_value_unref(&amount);
}

TEST(cardano_sub_tx_builder_lock_value, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_address_t*             address = create_address(ADDRESS);
  cardano_value_t*               amount  = create_value_with_assets();
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_lock_value(builder, address, amount, nullptr);

  const cardano_error_t              result  = cardano_sub_tx_builder_build(builder, &sub_tx);
  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(get_state_body(builder));

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_output_list_get_length(outputs), 0U);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_transaction_output_list_unref(&outputs);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_address_unref(&address);
  cardano_value_unref(&amount);
}

TEST(cardano_sub_tx_builder_lock_value, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_address_t*             address = create_address(ADDRESS);
  cardano_value_t*               amount  = create_value_with_assets();
  cardano_plutus_data_t*         data    = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_datum_t*               datum   = create_inline_datum(data);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_lock_value(builder, address, amount, datum);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_datum_unref(&datum);
  cardano_plutus_data_unref(&data);
  cardano_address_unref(&address);
  cardano_value_unref(&amount);
}

TEST(cardano_sub_tx_builder_lock_value_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_lock_value_ex(nullptr, nullptr, 0, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_lock_value_ex, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_value_t*               amount  = create_value_with_assets();

  // Act
  cardano_sub_tx_builder_lock_value_ex(builder, nullptr, 0, amount, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_value_unref(&amount);
}

TEST(cardano_sub_tx_builder_lock_value_ex, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_lock_value_ex(builder, ADDRESS, strlen(ADDRESS), nullptr, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_lock_value_ex, canLockWithInlineDatum)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_value_t*               amount  = create_value_with_assets();
  cardano_plutus_data_t*         data    = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_datum_t*               datum   = create_inline_datum(data);

  // Act
  cardano_sub_tx_builder_lock_value_ex(builder, ADDRESS, strlen(ADDRESS), amount, datum);

  cardano_sub_transaction_t*    sub_tx         = build_sub_transaction(builder);
  cardano_transaction_output_t* output         = get_first_output(sub_tx);
  cardano_address_t*            locked_address = cardano_transaction_output_get_address(output);
  cardano_value_t*              locked_value   = cardano_transaction_output_get_value(output);
  cardano_datum_t*              locked_datum   = cardano_transaction_output_get_datum(output);
  cardano_witness_set_t*        witness_set    = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_plutus_data_set_t*    witness_datums = cardano_witness_set_get_plutus_data(witness_set);

  // Assert
  ASSERT_NE(sub_tx, nullptr);
  EXPECT_STREQ(cardano_address_get_string(locked_address), ADDRESS);
  EXPECT_TRUE(cardano_value_equals(locked_value, amount));
  ASSERT_NE(locked_datum, nullptr);
  cardano_datum_type_t locked_type = CARDANO_DATUM_TYPE_DATA_HASH;

  EXPECT_EQ(cardano_datum_get_type(locked_datum, &locked_type), CARDANO_SUCCESS);
  EXPECT_EQ(locked_type, CARDANO_DATUM_TYPE_INLINE_DATA);

  cardano_plutus_data_t* locked_data = cardano_datum_get_inline_data(locked_datum);

  EXPECT_TRUE(cardano_plutus_data_equals(locked_data, data));
  cardano_plutus_data_unref(&locked_data);
  EXPECT_EQ(witness_datums, nullptr);

  // Cleanup
  cardano_plutus_data_set_unref(&witness_datums);
  cardano_witness_set_unref(&witness_set);
  cardano_datum_unref(&locked_datum);
  cardano_value_unref(&locked_value);
  cardano_address_unref(&locked_address);
  cardano_transaction_output_unref(&output);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_datum_unref(&datum);
  cardano_plutus_data_unref(&data);
  cardano_value_unref(&amount);
}

TEST(cardano_sub_tx_builder_lock_value_ex, canLockWithDatumHash)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_value_t*               amount  = create_value_with_assets();
  cardano_datum_t*               datum   = create_hash_datum("0000000000000000000000000000000000000000000000000000000000000000");

  // Act
  cardano_sub_tx_builder_lock_value_ex(builder, ADDRESS, strlen(ADDRESS), amount, datum);

  cardano_sub_transaction_t*    sub_tx         = build_sub_transaction(builder);
  cardano_transaction_output_t* output         = get_first_output(sub_tx);
  cardano_address_t*            locked_address = cardano_transaction_output_get_address(output);
  cardano_value_t*              locked_value   = cardano_transaction_output_get_value(output);
  cardano_datum_t*              locked_datum   = cardano_transaction_output_get_datum(output);
  cardano_witness_set_t*        witness_set    = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_plutus_data_set_t*    witness_datums = cardano_witness_set_get_plutus_data(witness_set);

  // Assert
  ASSERT_NE(sub_tx, nullptr);
  EXPECT_STREQ(cardano_address_get_string(locked_address), ADDRESS);
  EXPECT_TRUE(cardano_value_equals(locked_value, amount));
  ASSERT_NE(locked_datum, nullptr);
  cardano_datum_type_t locked_type = CARDANO_DATUM_TYPE_DATA_HASH;

  EXPECT_EQ(cardano_datum_get_type(locked_datum, &locked_type), CARDANO_SUCCESS);
  EXPECT_EQ(locked_type, CARDANO_DATUM_TYPE_DATA_HASH);
  EXPECT_TRUE(cardano_datum_equals(locked_datum, datum));
  EXPECT_EQ(witness_datums, nullptr);

  // Cleanup
  cardano_plutus_data_set_unref(&witness_datums);
  cardano_witness_set_unref(&witness_set);
  cardano_datum_unref(&locked_datum);
  cardano_value_unref(&locked_value);
  cardano_address_unref(&locked_address);
  cardano_transaction_output_unref(&output);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_datum_unref(&datum);
  cardano_value_unref(&amount);
}

TEST(cardano_sub_tx_builder_lock_value_ex, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_value_t*               amount  = create_value_with_assets();
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_lock_value_ex(builder, ADDRESS, strlen(ADDRESS), amount, nullptr);

  const cardano_error_t              result  = cardano_sub_tx_builder_build(builder, &sub_tx);
  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(get_state_body(builder));

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_output_list_get_length(outputs), 0U);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_transaction_output_list_unref(&outputs);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_value_unref(&amount);
}

TEST(cardano_sub_tx_builder_lock_value_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_value_t*               amount = create_value_with_assets();
  cardano_plutus_data_t*         data   = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_datum_t*               datum  = create_inline_datum(data);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_lock_value_ex(builder, ADDRESS, strlen(ADDRESS), amount, datum);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_THAT(builder->last_error, testing::AnyOf(CARDANO_ERROR_MEMORY_ALLOCATION_FAILED, CARDANO_ERROR_DECODING));
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_datum_unref(&datum);
  cardano_plutus_data_unref(&data);
  cardano_value_unref(&amount);
}

TEST(cardano_sub_tx_builder_set_metadata, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_set_metadata(nullptr, 0, nullptr);
}

TEST(cardano_sub_tx_builder_set_metadata, returnsErrorIfMetadataIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_set_metadata(builder, 0, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_set_metadata, canSetMetadata)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder  = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_metadatum_t*           metadata = NULL;

  EXPECT_EQ(cardano_metadatum_from_json(METADATA_JSON, strlen(METADATA_JSON), &metadata), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_set_metadata(builder, 674, metadata);

  cardano_sub_transaction_t* sub_tx     = build_sub_transaction(builder);
  char*                      body_hex   = encode_body(sub_tx);
  char*                      sub_tx_hex = encode_sub_transaction(sub_tx);

  cardano_auxiliary_data_t* aux_data = cardano_sub_transaction_get_auxiliary_data(sub_tx);
  cardano_auxiliary_data_unref(&aux_data);

  cardano_blake2b_hash_t* expected_hash = cardano_auxiliary_data_get_hash(aux_data);
  cardano_blake2b_hash_t* aux_data_hash = cardano_sub_transaction_body_get_aux_data_hash(get_body(sub_tx));

  // Assert
  EXPECT_NE(aux_data, nullptr);
  EXPECT_TRUE(cardano_blake2b_hash_equals(aux_data_hash, expected_hash));
  EXPECT_STREQ(body_hex, METADATA_BODY_CBOR);
  EXPECT_STREQ(sub_tx_hex, METADATA_SUB_TX_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_metadatum_unref(&metadata);
  cardano_blake2b_hash_unref(&expected_hash);
  cardano_blake2b_hash_unref(&aux_data_hash);
  free(body_hex);
  free(sub_tx_hex);
}

TEST(cardano_sub_tx_builder_set_metadata, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_metadatum_t*           metadata = NULL;

  EXPECT_EQ(cardano_metadatum_from_json(METADATA_JSON, strlen(METADATA_JSON), &metadata), CARDANO_SUCCESS);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_set_metadata(builder, 674, metadata);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_metadatum_unref(&metadata);
}

TEST(cardano_sub_tx_builder_set_metadata_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_set_metadata_ex(nullptr, 0, nullptr, 0);
}

TEST(cardano_sub_tx_builder_set_metadata_ex, returnsErrorIfMetadataIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_set_metadata_ex(builder, 0, nullptr, 0);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_set_metadata_ex, canSetMetadata)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_set_metadata_ex(builder, 674, METADATA_JSON, strlen(METADATA_JSON));

  cardano_sub_transaction_t* sub_tx     = build_sub_transaction(builder);
  char*                      sub_tx_hex = encode_sub_transaction(sub_tx);

  // Assert
  EXPECT_STREQ(sub_tx_hex, METADATA_SUB_TX_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(sub_tx_hex);
}

TEST(cardano_sub_tx_builder_mint_token, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_mint_token(nullptr, nullptr, nullptr, 0, nullptr);
}

TEST(cardano_sub_tx_builder_mint_token, returnsErrorIfPolicyIdOrNameIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*      policy_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      name_builder   = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_asset_name_t*          asset_name     = NULL;
  cardano_blake2b_hash_t*        policy_id      = NULL;

  EXPECT_EQ(cardano_asset_name_from_string("TEXT", 4, &asset_name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_id), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_mint_token(policy_builder, nullptr, asset_name, 4, nullptr);
  cardano_sub_tx_builder_mint_token(name_builder, policy_id, nullptr, 4, nullptr);

  // Assert
  EXPECT_EQ(policy_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(name_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&policy_builder);
  cardano_sub_tx_builder_unref(&name_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_sub_tx_builder_mint_token, canMintToken)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder    = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_asset_name_t*          asset_name = NULL;
  cardano_blake2b_hash_t*        policy_id  = NULL;

  EXPECT_EQ(cardano_asset_name_from_string("TEXT", 4, &asset_name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_id), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_mint_token(builder, policy_id, asset_name, 4, nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_multi_asset_t* mint = cardano_sub_transaction_body_get_mint(get_body(sub_tx));
  cardano_multi_asset_unref(&mint);

  int64_t quantity = 0;
  EXPECT_EQ(cardano_multi_asset_get(mint, policy_id, asset_name, &quantity), CARDANO_SUCCESS);

  cardano_witness_set_t* witnesses = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_witness_set_unref(&witnesses);

  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witnesses);
  cardano_redeemer_list_unref(&redeemers);

  // Assert
  EXPECT_EQ(quantity, 4);
  EXPECT_EQ(cardano_redeemer_list_get_length(redeemers), 0);
  EXPECT_STREQ(body_hex, MINT_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_mint_token, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_protocol_parameters();
  cardano_asset_name_t*          asset_name = NULL;
  cardano_blake2b_hash_t*        policy_id  = NULL;

  EXPECT_EQ(cardano_asset_name_from_string("TEXT", 4, &asset_name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_id), CARDANO_SUCCESS);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_mint_token(builder, policy_id, asset_name, 4, nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_sub_tx_builder_mint_token_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_mint_token_ex(nullptr, nullptr, 0, nullptr, 0, 0, nullptr);
}

TEST(cardano_sub_tx_builder_mint_token_ex, reportsInvalidPolicyIdWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_mint_token_ex(builder, "abc", 3, ASSET_NAME_HEX, strlen(ASSET_NAME_HEX), 4, nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_mint_token_ex, canMintToken)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_mint_token_ex(builder, HASH_HEX, strlen(HASH_HEX), ASSET_NAME_HEX, strlen(ASSET_NAME_HEX), 4, nullptr);

  char* body_hex = build_and_encode_body(builder);

  // Assert
  EXPECT_STREQ(body_hex, MINT_BODY_CBOR);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_mint_token_with_id, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_mint_token_with_id(nullptr, nullptr, 0, nullptr);
}

TEST(cardano_sub_tx_builder_mint_token_with_id, returnsErrorIfAssetIdIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_mint_token_with_id(builder, nullptr, 4, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_mint_token_with_id, canMintToken)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder  = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_asset_id_t*            asset_id = NULL;

  EXPECT_EQ(cardano_asset_id_from_hex(ASSET_ID_HEX, strlen(ASSET_ID_HEX), &asset_id), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_mint_token_with_id(builder, asset_id, 4, nullptr);

  char* body_hex = build_and_encode_body(builder);

  // Assert
  EXPECT_STREQ(body_hex, MINT_BODY_CBOR);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_asset_id_unref(&asset_id);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_mint_token_with_id_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_mint_token_with_id_ex(nullptr, nullptr, 0, 0, nullptr);
}

TEST(cardano_sub_tx_builder_mint_token_with_id_ex, reportsInvalidAssetIdWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_mint_token_with_id_ex(builder, "abc", 3, 4, nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_mint_token_with_id_ex, canMintToken)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_mint_token_with_id_ex(builder, ASSET_ID_HEX, strlen(ASSET_ID_HEX), 4, nullptr);

  char* body_hex = build_and_encode_body(builder);

  // Assert
  EXPECT_STREQ(body_hex, MINT_BODY_CBOR);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_add_script, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_add_script(nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_add_script, returnsErrorIfScriptIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_add_script(builder, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Script is NULL.");

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_script, canAddNativeScript)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script  = create_script(NATIVE_SCRIPT_CBOR);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);

  cardano_sub_transaction_t* sub_tx     = build_sub_transaction(builder);
  char*                      sub_tx_hex = encode_sub_transaction(sub_tx);

  cardano_witness_set_t* witnesses = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_witness_set_unref(&witnesses);

  cardano_native_script_set_t* native_scripts = cardano_witness_set_get_native_scripts(witnesses);
  cardano_native_script_set_unref(&native_scripts);

  // Assert
  EXPECT_EQ(cardano_native_script_set_get_length(native_scripts), 1);
  EXPECT_STREQ(sub_tx_hex, NATIVE_SCRIPT_SUB_TX_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_script_unref(&script);
  free(sub_tx_hex);
}

TEST(cardano_sub_tx_builder_add_script, rejectsPlutusV1Scripts)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script  = create_script(PLUTUS_V1_CBOR);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_add_script(builder, script);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), PLUTUS_SCRIPT_ERROR);
  EXPECT_EQ(sub_tx, nullptr);
  EXPECT_FALSE(builder->state.has_plutus_v1);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_script_unref(&script);
}

TEST(cardano_sub_tx_builder_add_script, rejectsPlutusV2Scripts)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script  = create_script(PLUTUS_V2_CBOR);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_add_script(builder, script);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), PLUTUS_SCRIPT_ERROR);
  EXPECT_EQ(sub_tx, nullptr);
  EXPECT_FALSE(builder->state.has_plutus_v2);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_script_unref(&script);
}

TEST(cardano_sub_tx_builder_add_script, rejectsPlutusV3Scripts)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script  = create_script(PLUTUS_V3_CBOR);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_add_script(builder, script);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), PLUTUS_SCRIPT_ERROR);
  EXPECT_EQ(sub_tx, nullptr);
  EXPECT_FALSE(builder->state.has_plutus_v3);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_script_unref(&script);
}

TEST(cardano_sub_tx_builder_add_script, acceptsPlutusV4Scripts)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script  = create_script(PLUTUS_V4_CBOR);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_add_script(builder, script);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  cardano_witness_set_t*          witness_set = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_plutus_v4_script_set_t* scripts     = cardano_witness_set_get_plutus_v4_scripts(witness_set);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS) << cardano_sub_tx_builder_get_last_error(builder);
  EXPECT_EQ(cardano_plutus_v4_script_set_get_length(scripts), 1U);
  EXPECT_TRUE(builder->state.has_plutus_v4);

  // Cleanup
  cardano_plutus_v4_script_set_unref(&scripts);
  cardano_witness_set_unref(&witness_set);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_script_unref(&script);
}

TEST(cardano_sub_tx_builder_add_script, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_script_t*              script = create_script(NATIVE_SCRIPT_CBOR);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_add_script(builder, script);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_script_unref(&script);
}

TEST(cardano_sub_tx_builder_add_signer, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_add_signer(nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_add_signer, returnsErrorIfSignerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_add_signer(builder, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Public key hash is NULL.");

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_signer, canAddSigner)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder     = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_blake2b_hash_t*        signer_hash = create_hash(HASH_HEX);

  // Act
  cardano_sub_tx_builder_add_signer(builder, signer_hash);
  cardano_sub_tx_builder_add_signer(builder, signer_hash);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_guard_set_t* guards = cardano_sub_transaction_body_get_guards(get_body(sub_tx));
  cardano_guard_set_unref(&guards);

  cardano_credential_t* first_guard = nullptr;
  EXPECT_EQ(cardano_guard_set_get(guards, 0, &first_guard), CARDANO_SUCCESS);

  cardano_blake2b_hash_t* first_guard_hash = cardano_credential_get_hash(first_guard);

  cardano_credential_type_t first_guard_type = CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH;
  EXPECT_EQ(cardano_credential_get_type(first_guard, &first_guard_type), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_guard_set_get_length(guards), 1);
  EXPECT_EQ(first_guard_type, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  EXPECT_TRUE(cardano_blake2b_hash_equals(first_guard_hash, signer_hash));
  EXPECT_STREQ(body_hex, KEY_HASH_GUARD_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_blake2b_hash_unref(&signer_hash);
  cardano_blake2b_hash_unref(&first_guard_hash);
  cardano_credential_unref(&first_guard);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_add_signer, buildsTheSameBodyAsAKeyHashGuard)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*      signer_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      guard_builder  = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_blake2b_hash_t*        signer_hash    = create_hash(HASH_HEX);
  cardano_credential_t*          guard          = create_credential(HASH_HEX, CARDANO_CREDENTIAL_TYPE_KEY_HASH);

  // Act
  cardano_sub_tx_builder_add_signer(signer_builder, signer_hash);
  cardano_sub_tx_builder_add_guard(guard_builder, guard);

  cardano_sub_transaction_t* signer_sub_tx = build_sub_transaction(signer_builder);
  cardano_sub_transaction_t* guard_sub_tx  = build_sub_transaction(guard_builder);

  char* signer_body_hex = encode_body(signer_sub_tx);
  char* guard_body_hex  = encode_body(guard_sub_tx);

  // Assert
  EXPECT_STREQ(signer_body_hex, guard_body_hex);
  EXPECT_STREQ(signer_body_hex, KEY_HASH_GUARD_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&signer_sub_tx);
  cardano_sub_transaction_unref(&guard_sub_tx);
  cardano_sub_tx_builder_unref(&signer_builder);
  cardano_sub_tx_builder_unref(&guard_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_blake2b_hash_unref(&signer_hash);
  cardano_credential_unref(&guard);
  free(signer_body_hex);
  free(guard_body_hex);
}

TEST(cardano_sub_tx_builder_add_signer, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder     = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_blake2b_hash_t*        signer_hash = create_hash(HASH_HEX);
  cardano_sub_transaction_t*     sub_tx      = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_add_signer(builder, signer_hash);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_guards(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_blake2b_hash_unref(&signer_hash);
}

TEST(cardano_sub_tx_builder_add_signer, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_blake2b_hash_t*        signer_hash = create_hash(HASH_HEX);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_add_signer(builder, signer_hash);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_blake2b_hash_unref(&signer_hash);
}

TEST(cardano_sub_tx_builder_add_signer_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_add_signer_ex(nullptr, nullptr, 0);
}

TEST(cardano_sub_tx_builder_add_signer_ex, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_add_signer_ex(builder, nullptr, 0);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Public key hash is NULL or empty.");

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_signer_ex, returnsErrorIfHashIsEmpty)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_add_signer_ex(builder, HASH_HEX, 0);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_signer_ex, canAddSigner)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_credential_t*          guard   = create_credential(HASH_HEX, CARDANO_CREDENTIAL_TYPE_KEY_HASH);

  // Act
  cardano_sub_tx_builder_add_signer_ex(builder, HASH_HEX, strlen(HASH_HEX));
  cardano_sub_tx_builder_add_guard(builder, guard);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_guard_set_t* guards = cardano_sub_transaction_body_get_guards(get_body(sub_tx));
  cardano_guard_set_unref(&guards);

  // Assert
  EXPECT_EQ(cardano_guard_set_get_length(guards), 1);
  EXPECT_STREQ(body_hex, KEY_HASH_GUARD_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_credential_unref(&guard);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_add_signer_ex, reportsInvalidHexWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_add_signer_ex(builder, "abc", 3);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse public key hash.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_signer_ex, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_add_signer_ex(builder, HASH_HEX, strlen(HASH_HEX));

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_guards(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_signer_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_add_signer_ex(builder, HASH_HEX, strlen(HASH_HEX));

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_guard, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_add_guard(nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_add_guard, returnsErrorIfGuardIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_add_guard(builder, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_guard, canAddKeyHashGuard)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_credential_t*          guard   = create_credential(HASH_HEX, CARDANO_CREDENTIAL_TYPE_KEY_HASH);

  // Act
  cardano_sub_tx_builder_add_guard(builder, guard);
  cardano_sub_tx_builder_add_guard(builder, guard);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_guard_set_t* guards = cardano_sub_transaction_body_get_guards(get_body(sub_tx));
  cardano_guard_set_unref(&guards);

  cardano_credential_t* first_guard = nullptr;
  EXPECT_EQ(cardano_guard_set_get(guards, 0, &first_guard), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_guard_set_get_length(guards), 1);
  EXPECT_TRUE(cardano_credential_equals(first_guard, guard));
  EXPECT_STREQ(body_hex, KEY_HASH_GUARD_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_credential_unref(&guard);
  cardano_credential_unref(&first_guard);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_add_guard, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_credential_t*          guard  = create_credential(HASH_HEX, CARDANO_CREDENTIAL_TYPE_KEY_HASH);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_add_guard(builder, guard);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_credential_unref(&guard);
}

TEST(cardano_sub_tx_builder_add_guard_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_add_guard_ex(nullptr, nullptr, 0, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
}

TEST(cardano_sub_tx_builder_add_guard_ex, returnsErrorIfHashIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_add_guard_ex(builder, nullptr, 0, CARDANO_CREDENTIAL_TYPE_KEY_HASH);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_guard_ex, canAddScriptHashGuard)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_add_guard_ex(builder, HASH_HEX, strlen(HASH_HEX), CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  cardano_sub_tx_builder_add_guard_ex(builder, SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_guard_set_t* guards = cardano_sub_transaction_body_get_guards(get_body(sub_tx));
  cardano_guard_set_unref(&guards);

  // Assert
  EXPECT_EQ(cardano_guard_set_get_length(guards), 2);
  EXPECT_STREQ(body_hex, SCRIPT_HASH_GUARD_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_add_guard_ex, reportsInvalidHexWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_add_guard_ex(builder, "abc", 3, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse guard hash.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_require_top_level_guard, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_require_top_level_guard(nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_require_top_level_guard, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_require_top_level_guard(builder, nullptr, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Required top level guard is NULL.");

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_require_top_level_guard, canRequireGuardWithoutDatum)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder    = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_credential_t*          credential = create_credential(SCRIPT_HASH_HEX, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);

  // Act
  cardano_sub_tx_builder_require_top_level_guard(builder, credential, nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_required_guards_map_t* required_guards = cardano_sub_transaction_body_get_required_top_level_guards(get_body(sub_tx));
  cardano_required_guards_map_unref(&required_guards);

  cardano_plutus_data_t* datum = nullptr;

  // Assert
  EXPECT_EQ(cardano_required_guards_map_get_length(required_guards), 1);
  EXPECT_EQ(cardano_required_guards_map_get(required_guards, credential, &datum), CARDANO_SUCCESS);
  EXPECT_EQ(datum, nullptr);
  EXPECT_EQ(cardano_sub_transaction_body_get_guards(get_body(sub_tx)), nullptr);
  EXPECT_STREQ(body_hex, REQUIRED_GUARD_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_credential_unref(&credential);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_require_top_level_guard, canRequireGuardWithDatum)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder    = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_credential_t*          credential = create_credential(SCRIPT_HASH_HEX, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);
  cardano_plutus_data_t*         datum      = create_plutus_data(PLUTUS_DATA_CBOR);

  // Act
  cardano_sub_tx_builder_require_top_level_guard(builder, credential, datum);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_required_guards_map_t* required_guards = cardano_sub_transaction_body_get_required_top_level_guards(get_body(sub_tx));
  cardano_required_guards_map_unref(&required_guards);

  cardano_plutus_data_t* stored_datum = nullptr;
  EXPECT_EQ(cardano_required_guards_map_get(required_guards, credential, &stored_datum), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_required_guards_map_get_length(required_guards), 1);
  EXPECT_TRUE(cardano_plutus_data_equals(stored_datum, datum));
  EXPECT_STREQ(body_hex, REQUIRED_GUARD_DATUM_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_credential_unref(&credential);
  cardano_plutus_data_unref(&datum);
  cardano_plutus_data_unref(&stored_datum);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_require_top_level_guard, requiringTheSameCredentialTwiceReplacesItsDatum)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder     = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_credential_t*          credential  = create_credential(SCRIPT_HASH_HEX, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);
  cardano_credential_t*          credential2 = create_credential(HASH_HEX, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  cardano_plutus_data_t*         datum       = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_plutus_data_t*         other_datum = create_plutus_data(OTHER_PLUTUS_DATA_CBOR);

  // Act
  cardano_sub_tx_builder_require_top_level_guard(builder, credential, other_datum);
  cardano_sub_tx_builder_require_top_level_guard(builder, credential2, nullptr);
  cardano_sub_tx_builder_require_top_level_guard(builder, credential, datum);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  cardano_required_guards_map_t* required_guards = cardano_sub_transaction_body_get_required_top_level_guards(get_body(sub_tx));
  cardano_required_guards_map_unref(&required_guards);

  cardano_credential_t*  first_credential = nullptr;
  cardano_plutus_data_t* first_datum      = nullptr;
  EXPECT_EQ(cardano_required_guards_map_get_key_value_at(required_guards, 0, &first_credential, &first_datum), CARDANO_SUCCESS);

  cardano_plutus_data_t* second_datum = nullptr;
  EXPECT_EQ(cardano_required_guards_map_get(required_guards, credential2, &second_datum), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_required_guards_map_get_length(required_guards), 2);
  EXPECT_TRUE(cardano_credential_equals(first_credential, credential));
  EXPECT_TRUE(cardano_plutus_data_equals(first_datum, datum));
  EXPECT_EQ(second_datum, nullptr);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_credential_unref(&credential);
  cardano_credential_unref(&credential2);
  cardano_credential_unref(&first_credential);
  cardano_plutus_data_unref(&datum);
  cardano_plutus_data_unref(&other_datum);
  cardano_plutus_data_unref(&first_datum);
}

TEST(cardano_sub_tx_builder_require_top_level_guard, keepsThePreviousGuardsWhenAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_credential_t*          credential  = create_credential(SCRIPT_HASH_HEX, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);
  cardano_credential_t*          credential2 = create_credential(HASH_HEX, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  cardano_plutus_data_t*         datum       = create_plutus_data(PLUTUS_DATA_CBOR);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    cardano_sub_tx_builder_require_top_level_guard(builder, credential, nullptr);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_require_top_level_guard(builder, credential2, datum);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
    cardano_transaction_body_unref(&body);

    cardano_required_guards_map_t* required_guards = cardano_transaction_body_get_required_top_level_guards(body);
    cardano_required_guards_map_unref(&required_guards);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_EQ(cardano_required_guards_map_get_length(required_guards), 2);
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
      EXPECT_STRNE(cardano_sub_tx_builder_get_last_error(builder), "");
      EXPECT_EQ(cardano_required_guards_map_get_length(required_guards), 1);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_credential_unref(&credential);
  cardano_credential_unref(&credential2);
  cardano_plutus_data_unref(&datum);
}

TEST(cardano_sub_tx_builder_require_top_level_guard_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_require_top_level_guard_ex(nullptr, nullptr, 0, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, nullptr);
}

TEST(cardano_sub_tx_builder_require_top_level_guard_ex, returnsErrorIfHashIsNullOrEmpty)
{
  // Arrange
  cardano_protocol_parameters_t* params        = init_protocol_parameters();
  cardano_sub_tx_builder_t*      null_builder  = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      empty_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_require_top_level_guard_ex(null_builder, nullptr, 0, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, nullptr);
  cardano_sub_tx_builder_require_top_level_guard_ex(empty_builder, SCRIPT_HASH_HEX, 0, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, nullptr);

  // Assert
  EXPECT_EQ(null_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(empty_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(null_builder), "Required top level guard hash is NULL or empty.");

  // Cleanup
  cardano_sub_tx_builder_unref(&null_builder);
  cardano_sub_tx_builder_unref(&empty_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_require_top_level_guard_ex, canRequireGuard)
{
  // Arrange
  cardano_protocol_parameters_t* params        = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder       = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      datum_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_plutus_data_t*         datum         = create_plutus_data(PLUTUS_DATA_CBOR);

  // Act
  cardano_sub_tx_builder_require_top_level_guard_ex(builder, SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, nullptr);
  cardano_sub_tx_builder_require_top_level_guard_ex(datum_builder, SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, datum);

  char* body_hex       = build_and_encode_body(builder);
  char* datum_body_hex = build_and_encode_body(datum_builder);

  // Assert
  EXPECT_STREQ(body_hex, REQUIRED_GUARD_BODY_CBOR);
  EXPECT_STREQ(datum_body_hex, REQUIRED_GUARD_DATUM_BODY_CBOR);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_sub_tx_builder_unref(&datum_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_plutus_data_unref(&datum);
  free(body_hex);
  free(datum_body_hex);
}

TEST(cardano_sub_tx_builder_require_top_level_guard_ex, reportsInvalidHexWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_require_top_level_guard_ex(builder, "abc", 3, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, nullptr);
  cardano_sub_tx_builder_require_top_level_guard_ex(builder, SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse required top level guard hash.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_require_top_level_guard_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_require_top_level_guard_ex(builder, SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_direct_deposit, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_add_direct_deposit(nullptr, nullptr, 0);
}

TEST(cardano_sub_tx_builder_add_direct_deposit, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_add_direct_deposit(builder, nullptr, 3000000);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_direct_deposit, returnsErrorIfAmountIsZero)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);

  // Act
  cardano_sub_tx_builder_add_direct_deposit(builder, reward_address, 0);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_sub_tx_builder_add_direct_deposit, canAddDirectDeposit)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);

  // Act
  cardano_sub_tx_builder_add_direct_deposit(builder, reward_address, 1000000);
  cardano_sub_tx_builder_add_direct_deposit(builder, reward_address, 2000000);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_direct_deposit_map_t* deposits = cardano_sub_transaction_body_get_direct_deposits(get_body(sub_tx));
  cardano_direct_deposit_map_unref(&deposits);

  uint64_t amount = 0;
  EXPECT_EQ(cardano_direct_deposit_map_get(deposits, reward_address, &amount), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_direct_deposit_map_get_length(deposits), 1);
  EXPECT_EQ(amount, 3000000);
  EXPECT_STREQ(body_hex, DIRECT_DEPOSIT_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_add_direct_deposit, reportsAnAmountAboveTheMaximumRepresentableAmountWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_sub_transaction_t*     sub_tx         = nullptr;

  // Act
  cardano_sub_tx_builder_add_direct_deposit(builder, reward_address, (uint64_t)INT64_MAX + 1U);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Direct deposit amount exceeds the maximum representable amount.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_sub_tx_builder_add_direct_deposit, reportsAnAccumulatedAmountAboveTheMaximumRepresentableAmountWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_sub_transaction_t*     sub_tx         = nullptr;

  // Act
  cardano_sub_tx_builder_add_direct_deposit(builder, reward_address, (uint64_t)INT64_MAX);
  cardano_sub_tx_builder_add_direct_deposit(builder, reward_address, 1);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Accumulated direct deposit amount exceeds the maximum representable amount.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_sub_tx_builder_add_direct_deposit, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_add_direct_deposit(builder, reward_address, 3000000);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_sub_tx_builder_add_direct_deposit_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_add_direct_deposit_ex(nullptr, nullptr, 0, 0);
}

TEST(cardano_sub_tx_builder_add_direct_deposit_ex, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_add_direct_deposit_ex(builder, nullptr, 0, 3000000);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_direct_deposit_ex, canAddDirectDeposit)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_add_direct_deposit_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 3000000);

  char* body_hex = build_and_encode_body(builder);

  // Assert
  EXPECT_STREQ(body_hex, DIRECT_DEPOSIT_BODY_CBOR);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_add_direct_deposit_ex, reportsInvalidRewardAddressWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_add_direct_deposit_ex(builder, "invalid", strlen("invalid"), 3000000);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse reward address.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_account_balance_interval, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_add_account_balance_interval(nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_add_account_balance_interval, returnsErrorIfRewardAddressOrIntervalIsNull)
{
  // Arrange
  cardano_protocol_parameters_t*      params           = init_protocol_parameters();
  cardano_sub_tx_builder_t*           address_builder  = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*           interval_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*           reward_address   = create_reward_address(REWARD_ADDRESS);
  cardano_account_balance_interval_t* interval         = create_interval(1000000);

  // Act
  cardano_sub_tx_builder_add_account_balance_interval(address_builder, nullptr, interval);
  cardano_sub_tx_builder_add_account_balance_interval(interval_builder, reward_address, nullptr);

  // Assert
  EXPECT_EQ(address_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(interval_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&address_builder);
  cardano_sub_tx_builder_unref(&interval_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_sub_tx_builder_add_account_balance_interval, canAddAccountBalanceInterval)
{
  // Arrange
  cardano_protocol_parameters_t*      params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*           builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*           reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_account_balance_interval_t* interval       = create_interval(1000000);

  // Act
  cardano_sub_tx_builder_add_account_balance_interval(builder, reward_address, interval);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_account_balance_intervals_map_t* intervals = cardano_sub_transaction_body_get_account_balance_intervals(get_body(sub_tx));
  cardano_account_balance_intervals_map_unref(&intervals);

  cardano_account_balance_interval_t* stored_interval = nullptr;
  EXPECT_EQ(cardano_account_balance_intervals_map_get(intervals, reward_address, &stored_interval), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_account_balance_intervals_map_get_length(intervals), 1);
  EXPECT_EQ(stored_interval, interval);
  EXPECT_STREQ(body_hex, BALANCE_INTERVAL_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_account_balance_interval_unref(&interval);
  cardano_account_balance_interval_unref(&stored_interval);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_add_account_balance_interval, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*      params         = init_protocol_parameters();
  cardano_reward_address_t*           reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_account_balance_interval_t* interval       = create_interval(1000000);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_add_account_balance_interval(builder, reward_address, interval);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_sub_tx_builder_add_account_balance_interval_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_add_account_balance_interval_ex(nullptr, nullptr, 0, nullptr);
}

TEST(cardano_sub_tx_builder_add_account_balance_interval_ex, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t*      params   = init_protocol_parameters();
  cardano_sub_tx_builder_t*           builder  = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_account_balance_interval_t* interval = create_interval(1000000);

  // Act
  cardano_sub_tx_builder_add_account_balance_interval_ex(builder, nullptr, 0, interval);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_sub_tx_builder_add_account_balance_interval_ex, canAddAccountBalanceInterval)
{
  // Arrange
  cardano_protocol_parameters_t*      params   = init_protocol_parameters();
  cardano_sub_tx_builder_t*           builder  = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_account_balance_interval_t* interval = create_interval(1000000);

  // Act
  cardano_sub_tx_builder_add_account_balance_interval_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), interval);

  char* body_hex = build_and_encode_body(builder);

  // Assert
  EXPECT_STREQ(body_hex, BALANCE_INTERVAL_BODY_CBOR);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_account_balance_interval_unref(&interval);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_withdraw_rewards, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_withdraw_rewards(nullptr, nullptr, 0, nullptr);
}

TEST(cardano_sub_tx_builder_withdraw_rewards, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_withdraw_rewards(builder, nullptr, 2532145, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_withdraw_rewards, returnsErrorIfAmountIsNegative)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);

  // Act
  cardano_sub_tx_builder_withdraw_rewards(builder, reward_address, -1, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_sub_tx_builder_withdraw_rewards, canWithdrawRewards)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);

  // Act
  cardano_sub_tx_builder_withdraw_rewards(builder, reward_address, 2532145, nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_withdrawal_map_t* withdrawals = cardano_sub_transaction_body_get_withdrawals(get_body(sub_tx));
  cardano_withdrawal_map_unref(&withdrawals);

  cardano_reward_address_t* account = create_reward_address(REWARD_ADDRESS);
  uint64_t                  amount  = 0;

  EXPECT_EQ(cardano_withdrawal_map_get(withdrawals, account, &amount), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_withdrawal_map_get_length(withdrawals), 1);
  EXPECT_EQ(amount, 2532145);
  EXPECT_STREQ(body_hex, WITHDRAWAL_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_reward_address_unref(&account);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_withdraw_rewards, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_sub_transaction_t*     sub_tx         = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_withdraw_rewards(builder, reward_address, 2532145, nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_withdrawals(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_sub_tx_builder_withdraw_rewards, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_withdraw_rewards(builder, reward_address, 2532145, nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_sub_tx_builder_withdraw_rewards_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_withdraw_rewards_ex(nullptr, nullptr, 0, 0, nullptr);
}

TEST(cardano_sub_tx_builder_withdraw_rewards_ex, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_withdraw_rewards_ex(builder, nullptr, 0, 2532145, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_withdraw_rewards_ex, canWithdrawRewards)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_withdraw_rewards_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 2532145, nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_withdrawal_map_t* withdrawals = cardano_sub_transaction_body_get_withdrawals(get_body(sub_tx));
  cardano_withdrawal_map_unref(&withdrawals);

  cardano_reward_address_t* account = create_reward_address(REWARD_ADDRESS);
  uint64_t                  amount  = 0;

  EXPECT_EQ(cardano_withdrawal_map_get(withdrawals, account, &amount), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_withdrawal_map_get_length(withdrawals), 1);
  EXPECT_EQ(amount, 2532145);
  EXPECT_STREQ(body_hex, WITHDRAWAL_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&account);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_withdraw_rewards_ex, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_withdraw_rewards_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 2532145, nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_withdrawals(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_withdraw_rewards_ex, reportsInvalidRewardAddressWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_withdraw_rewards_ex(builder, "invalid", strlen("invalid"), 2532145, nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse reward address.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_withdraw_rewards_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_withdraw_rewards_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 2532145, nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_register_reward_address, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_register_reward_address(nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_register_reward_address, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_register_reward_address(builder, nullptr, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_register_reward_address, canRegisterRewardAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);

  // Act
  cardano_sub_tx_builder_register_reward_address(builder, reward_address, nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_certificate_set_t* certificates = cardano_sub_transaction_body_get_certificates(get_body(sub_tx));
  cardano_certificate_set_unref(&certificates);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certificates), 1);
  EXPECT_EQ(get_certificate_type(certificates, 0), CARDANO_CERT_TYPE_REGISTRATION);
  EXPECT_STREQ(body_hex, REGISTER_REWARD_ADDRESS_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_register_reward_address, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_sub_transaction_t*     sub_tx         = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_register_reward_address(builder, reward_address, nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_certificates(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_sub_tx_builder_register_reward_address, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_register_reward_address(builder, reward_address, nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_sub_tx_builder_register_reward_address_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_register_reward_address_ex(nullptr, nullptr, 0, nullptr);
}

TEST(cardano_sub_tx_builder_register_reward_address_ex, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_register_reward_address_ex(builder, nullptr, 0, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_register_reward_address_ex, canRegisterRewardAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_register_reward_address_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_certificate_set_t* certificates = cardano_sub_transaction_body_get_certificates(get_body(sub_tx));
  cardano_certificate_set_unref(&certificates);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certificates), 1);
  EXPECT_EQ(get_certificate_type(certificates, 0), CARDANO_CERT_TYPE_REGISTRATION);
  EXPECT_STREQ(body_hex, REGISTER_REWARD_ADDRESS_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_register_reward_address_ex, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_register_reward_address_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_certificates(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_register_reward_address_ex, reportsInvalidRewardAddressWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_register_reward_address_ex(builder, "invalid", strlen("invalid"), nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse reward address.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_register_reward_address_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_register_reward_address_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_deregister_reward_address, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_deregister_reward_address(nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_deregister_reward_address, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_deregister_reward_address(builder, nullptr, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_deregister_reward_address, canDeregisterRewardAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);

  // Act
  cardano_sub_tx_builder_deregister_reward_address(builder, reward_address, nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_certificate_set_t* certificates = cardano_sub_transaction_body_get_certificates(get_body(sub_tx));
  cardano_certificate_set_unref(&certificates);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certificates), 1);
  EXPECT_EQ(get_certificate_type(certificates, 0), CARDANO_CERT_TYPE_UNREGISTRATION);
  EXPECT_STREQ(body_hex, DEREGISTER_REWARD_ADDRESS_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_deregister_reward_address, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_sub_transaction_t*     sub_tx         = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_deregister_reward_address(builder, reward_address, nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_certificates(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_sub_tx_builder_deregister_reward_address, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_deregister_reward_address(builder, reward_address, nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_sub_tx_builder_deregister_reward_address_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_deregister_reward_address_ex(nullptr, nullptr, 0, nullptr);
}

TEST(cardano_sub_tx_builder_deregister_reward_address_ex, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_deregister_reward_address_ex(builder, nullptr, 0, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_deregister_reward_address_ex, canDeregisterRewardAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_deregister_reward_address_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_certificate_set_t* certificates = cardano_sub_transaction_body_get_certificates(get_body(sub_tx));
  cardano_certificate_set_unref(&certificates);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certificates), 1);
  EXPECT_EQ(get_certificate_type(certificates, 0), CARDANO_CERT_TYPE_UNREGISTRATION);
  EXPECT_STREQ(body_hex, DEREGISTER_REWARD_ADDRESS_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_deregister_reward_address_ex, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_deregister_reward_address_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_certificates(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_deregister_reward_address_ex, reportsInvalidRewardAddressWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_deregister_reward_address_ex(builder, "invalid", strlen("invalid"), nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse reward address.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_deregister_reward_address_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_deregister_reward_address_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_delegate_stake, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_delegate_stake(nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_delegate_stake, returnsErrorIfAddressOrPoolIdIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  cardano_sub_tx_builder_t*      address_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      pool_id_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address  = create_reward_address(REWARD_ADDRESS);
  cardano_blake2b_hash_t*        pool_id         = create_hash(POOL_ID_HEX);

  // Act
  cardano_sub_tx_builder_delegate_stake(address_builder, nullptr, pool_id, nullptr);
  cardano_sub_tx_builder_delegate_stake(pool_id_builder, reward_address, nullptr, nullptr);

  // Assert
  EXPECT_EQ(address_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(pool_id_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&address_builder);
  cardano_sub_tx_builder_unref(&pool_id_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_blake2b_hash_unref(&pool_id);
}

TEST(cardano_sub_tx_builder_delegate_stake, canDelegateStake)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_blake2b_hash_t*        pool_id        = create_hash(POOL_ID_HEX);

  // Act
  cardano_sub_tx_builder_delegate_stake(builder, reward_address, pool_id, nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_certificate_set_t* certificates = cardano_sub_transaction_body_get_certificates(get_body(sub_tx));
  cardano_certificate_set_unref(&certificates);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certificates), 1);
  EXPECT_EQ(get_certificate_type(certificates, 0), CARDANO_CERT_TYPE_STAKE_DELEGATION);
  EXPECT_STREQ(body_hex, DELEGATE_STAKE_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_blake2b_hash_unref(&pool_id);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_delegate_stake, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_blake2b_hash_t*        pool_id        = create_hash(POOL_ID_HEX);
  cardano_sub_transaction_t*     sub_tx         = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_delegate_stake(builder, reward_address, pool_id, nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_certificates(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_blake2b_hash_unref(&pool_id);
}

TEST(cardano_sub_tx_builder_delegate_stake, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_blake2b_hash_t*        pool_id        = create_hash(POOL_ID_HEX);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_delegate_stake(builder, reward_address, pool_id, nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_blake2b_hash_unref(&pool_id);
}

TEST(cardano_sub_tx_builder_delegate_stake_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_delegate_stake_ex(nullptr, nullptr, 0, nullptr, 0, nullptr);
}

TEST(cardano_sub_tx_builder_delegate_stake_ex, returnsErrorIfRewardAddressOrPoolIdIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  cardano_sub_tx_builder_t*      address_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      pool_id_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_delegate_stake_ex(address_builder, nullptr, 0, POOL_ID, strlen(POOL_ID), nullptr);
  cardano_sub_tx_builder_delegate_stake_ex(pool_id_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr, 0, nullptr);

  // Assert
  EXPECT_EQ(address_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(pool_id_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&address_builder);
  cardano_sub_tx_builder_unref(&pool_id_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_delegate_stake_ex, canDelegateStake)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_delegate_stake_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), POOL_ID, strlen(POOL_ID), nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_certificate_set_t* certificates = cardano_sub_transaction_body_get_certificates(get_body(sub_tx));
  cardano_certificate_set_unref(&certificates);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certificates), 1);
  EXPECT_EQ(get_certificate_type(certificates, 0), CARDANO_CERT_TYPE_STAKE_DELEGATION);
  EXPECT_STREQ(body_hex, DELEGATE_STAKE_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_delegate_stake_ex, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_delegate_stake_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), POOL_ID, strlen(POOL_ID), nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_certificates(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_delegate_stake_ex, reportsInvalidRewardAddressWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_delegate_stake_ex(builder, "invalid", strlen("invalid"), POOL_ID, strlen(POOL_ID), nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse reward address.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_delegate_stake_ex, reportsInvalidPoolIdWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_delegate_stake_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), "invalid", strlen("invalid"), nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to decode pool ID.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_delegate_stake_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_delegate_stake_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), POOL_ID, strlen(POOL_ID), nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_delegate_voting_power, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_delegate_voting_power(nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_delegate_voting_power, returnsErrorIfAddressOrDrepIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  cardano_sub_tx_builder_t*      address_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      drep_builder    = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address  = create_reward_address(REWARD_ADDRESS);
  cardano_drep_t*                drep            = create_drep(DREP_ID);

  // Act
  cardano_sub_tx_builder_delegate_voting_power(address_builder, nullptr, drep, nullptr);
  cardano_sub_tx_builder_delegate_voting_power(drep_builder, reward_address, nullptr, nullptr);

  // Assert
  EXPECT_EQ(address_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(drep_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&address_builder);
  cardano_sub_tx_builder_unref(&drep_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_drep_unref(&drep);
}

TEST(cardano_sub_tx_builder_delegate_voting_power, canDelegateVotingPower)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_drep_t*                drep           = create_drep(DREP_ID);

  // Act
  cardano_sub_tx_builder_delegate_voting_power(builder, reward_address, drep, nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_certificate_set_t* certificates = cardano_sub_transaction_body_get_certificates(get_body(sub_tx));
  cardano_certificate_set_unref(&certificates);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certificates), 1);
  EXPECT_EQ(get_certificate_type(certificates, 0), CARDANO_CERT_TYPE_VOTE_DELEGATION);
  EXPECT_STREQ(body_hex, DELEGATE_VOTING_POWER_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_drep_unref(&drep);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_delegate_voting_power, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_drep_t*                drep           = create_drep(DREP_ID);
  cardano_sub_transaction_t*     sub_tx         = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_delegate_voting_power(builder, reward_address, drep, nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_certificates(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_drep_unref(&drep);
}

TEST(cardano_sub_tx_builder_delegate_voting_power, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_drep_t*                drep           = create_drep(DREP_ID);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_delegate_voting_power(builder, reward_address, drep, nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_drep_unref(&drep);
}

TEST(cardano_sub_tx_builder_delegate_voting_power_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_delegate_voting_power_ex(nullptr, nullptr, 0, nullptr, 0, nullptr);
}

TEST(cardano_sub_tx_builder_delegate_voting_power_ex, returnsErrorIfRewardAddressOrDrepIdIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  cardano_sub_tx_builder_t*      address_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      drep_builder    = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_delegate_voting_power_ex(address_builder, nullptr, 0, DREP_ID, strlen(DREP_ID), nullptr);
  cardano_sub_tx_builder_delegate_voting_power_ex(drep_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr, 0, nullptr);

  // Assert
  EXPECT_EQ(address_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(drep_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&address_builder);
  cardano_sub_tx_builder_unref(&drep_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_delegate_voting_power_ex, canDelegateVotingPower)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_delegate_voting_power_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), DREP_ID, strlen(DREP_ID), nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_certificate_set_t* certificates = cardano_sub_transaction_body_get_certificates(get_body(sub_tx));
  cardano_certificate_set_unref(&certificates);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certificates), 1);
  EXPECT_EQ(get_certificate_type(certificates, 0), CARDANO_CERT_TYPE_VOTE_DELEGATION);
  EXPECT_STREQ(body_hex, DELEGATE_VOTING_POWER_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_delegate_voting_power_ex, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_delegate_voting_power_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), DREP_ID, strlen(DREP_ID), nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_certificates(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_delegate_voting_power_ex, reportsInvalidRewardAddressWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_delegate_voting_power_ex(builder, "invalid", strlen("invalid"), DREP_ID, strlen(DREP_ID), nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse reward address.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_delegate_voting_power_ex, reportsInvalidDrepIdWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_delegate_voting_power_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), "invalid", strlen("invalid"), nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse DREP ID.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_delegate_voting_power_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_delegate_voting_power_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), DREP_ID, strlen(DREP_ID), nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_register_drep, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_register_drep(nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_register_drep, returnsErrorIfDrepIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_anchor_t*              anchor  = create_anchor();

  // Act
  cardano_sub_tx_builder_register_drep(builder, nullptr, anchor, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_sub_tx_builder_register_drep, canRegisterDrep)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_drep_t*                drep    = create_drep(DREP_ID);
  cardano_anchor_t*              anchor  = create_anchor();

  // Act
  cardano_sub_tx_builder_register_drep(builder, drep, anchor, nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_certificate_set_t* certificates = cardano_sub_transaction_body_get_certificates(get_body(sub_tx));
  cardano_certificate_set_unref(&certificates);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certificates), 1);
  EXPECT_EQ(get_certificate_type(certificates, 0), CARDANO_CERT_TYPE_DREP_REGISTRATION);
  EXPECT_STREQ(body_hex, REGISTER_DREP_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_drep_unref(&drep);
  cardano_anchor_unref(&anchor);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_register_drep, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_drep_t*                drep    = create_drep(DREP_ID);
  cardano_anchor_t*              anchor  = create_anchor();
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_register_drep(builder, drep, anchor, nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_certificates(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_drep_unref(&drep);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_sub_tx_builder_register_drep, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_drep_t*                drep   = create_drep(DREP_ID);
  cardano_anchor_t*              anchor = create_anchor();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_register_drep(builder, drep, anchor, nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_drep_unref(&drep);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_sub_tx_builder_register_drep_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_register_drep_ex(nullptr, nullptr, 0, nullptr, 0, nullptr, 0, nullptr);
}

TEST(cardano_sub_tx_builder_register_drep_ex, returnsErrorIfDrepIdOrAnchorIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params       = init_protocol_parameters();
  cardano_sub_tx_builder_t*      drep_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      url_builder  = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      hash_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_register_drep_ex(drep_builder, nullptr, 0, ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr);
  cardano_sub_tx_builder_register_drep_ex(url_builder, DREP_ID, strlen(DREP_ID), nullptr, 0, ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr);
  cardano_sub_tx_builder_register_drep_ex(hash_builder, DREP_ID, strlen(DREP_ID), ANCHOR_URL, strlen(ANCHOR_URL), nullptr, 0, nullptr);

  // Assert
  EXPECT_EQ(drep_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(url_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(hash_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&drep_builder);
  cardano_sub_tx_builder_unref(&url_builder);
  cardano_sub_tx_builder_unref(&hash_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_register_drep_ex, canRegisterDrep)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_register_drep_ex(builder, DREP_ID, strlen(DREP_ID), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_certificate_set_t* certificates = cardano_sub_transaction_body_get_certificates(get_body(sub_tx));
  cardano_certificate_set_unref(&certificates);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certificates), 1);
  EXPECT_EQ(get_certificate_type(certificates, 0), CARDANO_CERT_TYPE_DREP_REGISTRATION);
  EXPECT_STREQ(body_hex, REGISTER_DREP_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_register_drep_ex, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_register_drep_ex(builder, DREP_ID, strlen(DREP_ID), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_certificates(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_register_drep_ex, reportsInvalidDrepIdWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_register_drep_ex(builder, "invalid", strlen("invalid"), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse DRep ID.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_register_drep_ex, reportsInvalidMetadataHashWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_register_drep_ex(builder, DREP_ID, strlen(DREP_ID), ANCHOR_URL, strlen(ANCHOR_URL), "invalid", strlen("invalid"), nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to create anchor.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_register_drep_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_register_drep_ex(builder, DREP_ID, strlen(DREP_ID), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_update_drep, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_update_drep(nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_update_drep, returnsErrorIfDrepIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_anchor_t*              anchor  = create_anchor();

  // Act
  cardano_sub_tx_builder_update_drep(builder, nullptr, anchor, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_sub_tx_builder_update_drep, canUpdateDrep)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_drep_t*                drep    = create_drep(DREP_ID);
  cardano_anchor_t*              anchor  = create_anchor();

  // Act
  cardano_sub_tx_builder_update_drep(builder, drep, anchor, nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_certificate_set_t* certificates = cardano_sub_transaction_body_get_certificates(get_body(sub_tx));
  cardano_certificate_set_unref(&certificates);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certificates), 1);
  EXPECT_EQ(get_certificate_type(certificates, 0), CARDANO_CERT_TYPE_UPDATE_DREP);
  EXPECT_STREQ(body_hex, UPDATE_DREP_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_drep_unref(&drep);
  cardano_anchor_unref(&anchor);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_update_drep, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_drep_t*                drep    = create_drep(DREP_ID);
  cardano_anchor_t*              anchor  = create_anchor();
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_update_drep(builder, drep, anchor, nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_certificates(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_drep_unref(&drep);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_sub_tx_builder_update_drep, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_drep_t*                drep   = create_drep(DREP_ID);
  cardano_anchor_t*              anchor = create_anchor();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_update_drep(builder, drep, anchor, nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_drep_unref(&drep);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_sub_tx_builder_update_drep_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_update_drep_ex(nullptr, nullptr, 0, nullptr, 0, nullptr, 0, nullptr);
}

TEST(cardano_sub_tx_builder_update_drep_ex, returnsErrorIfDrepIdOrAnchorIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params       = init_protocol_parameters();
  cardano_sub_tx_builder_t*      drep_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      url_builder  = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      hash_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_update_drep_ex(drep_builder, nullptr, 0, ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr);
  cardano_sub_tx_builder_update_drep_ex(url_builder, DREP_ID, strlen(DREP_ID), nullptr, 0, ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr);
  cardano_sub_tx_builder_update_drep_ex(hash_builder, DREP_ID, strlen(DREP_ID), ANCHOR_URL, strlen(ANCHOR_URL), nullptr, 0, nullptr);

  // Assert
  EXPECT_EQ(drep_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(url_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(hash_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&drep_builder);
  cardano_sub_tx_builder_unref(&url_builder);
  cardano_sub_tx_builder_unref(&hash_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_update_drep_ex, canUpdateDrep)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_update_drep_ex(builder, DREP_ID, strlen(DREP_ID), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_certificate_set_t* certificates = cardano_sub_transaction_body_get_certificates(get_body(sub_tx));
  cardano_certificate_set_unref(&certificates);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certificates), 1);
  EXPECT_EQ(get_certificate_type(certificates, 0), CARDANO_CERT_TYPE_UPDATE_DREP);
  EXPECT_STREQ(body_hex, UPDATE_DREP_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_update_drep_ex, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_update_drep_ex(builder, DREP_ID, strlen(DREP_ID), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_certificates(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_update_drep_ex, reportsInvalidDrepIdWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_update_drep_ex(builder, "invalid", strlen("invalid"), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse DRep ID.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_update_drep_ex, reportsInvalidMetadataHashWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_update_drep_ex(builder, DREP_ID, strlen(DREP_ID), ANCHOR_URL, strlen(ANCHOR_URL), "invalid", strlen("invalid"), nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to create anchor.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_update_drep_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_update_drep_ex(builder, DREP_ID, strlen(DREP_ID), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_deregister_drep, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_deregister_drep(nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_deregister_drep, returnsErrorIfDrepIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_deregister_drep(builder, nullptr, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_deregister_drep, canDeregisterDrep)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_drep_t*                drep    = create_drep(DREP_ID);

  // Act
  cardano_sub_tx_builder_deregister_drep(builder, drep, nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_certificate_set_t* certificates = cardano_sub_transaction_body_get_certificates(get_body(sub_tx));
  cardano_certificate_set_unref(&certificates);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certificates), 1);
  EXPECT_EQ(get_certificate_type(certificates, 0), CARDANO_CERT_TYPE_DREP_UNREGISTRATION);
  EXPECT_STREQ(body_hex, DEREGISTER_DREP_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_drep_unref(&drep);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_deregister_drep, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_drep_t*                drep    = create_drep(DREP_ID);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_deregister_drep(builder, drep, nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_certificates(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_drep_unref(&drep);
}

TEST(cardano_sub_tx_builder_deregister_drep, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_drep_t*                drep   = create_drep(DREP_ID);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_deregister_drep(builder, drep, nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_drep_unref(&drep);
}

TEST(cardano_sub_tx_builder_deregister_drep_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_deregister_drep_ex(nullptr, nullptr, 0, nullptr);
}

TEST(cardano_sub_tx_builder_deregister_drep_ex, returnsErrorIfDrepIdIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_deregister_drep_ex(builder, nullptr, 0, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_deregister_drep_ex, canDeregisterDrep)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_deregister_drep_ex(builder, DREP_ID, strlen(DREP_ID), nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_certificate_set_t* certificates = cardano_sub_transaction_body_get_certificates(get_body(sub_tx));
  cardano_certificate_set_unref(&certificates);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certificates), 1);
  EXPECT_EQ(get_certificate_type(certificates, 0), CARDANO_CERT_TYPE_DREP_UNREGISTRATION);
  EXPECT_STREQ(body_hex, DEREGISTER_DREP_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_deregister_drep_ex, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_deregister_drep_ex(builder, DREP_ID, strlen(DREP_ID), nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_certificates(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_deregister_drep_ex, reportsInvalidDrepIdWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_deregister_drep_ex(builder, "invalid", strlen("invalid"), nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse DRep ID.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_deregister_drep_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_deregister_drep_ex(builder, DREP_ID, strlen(DREP_ID), nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_vote, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_vote(nullptr, nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_vote, returnsErrorIfVoterActionIdOrVoteIsNull)
{
  // Arrange
  cardano_protocol_parameters_t*  params            = init_protocol_parameters();
  cardano_sub_tx_builder_t*       voter_builder     = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*       action_id_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*       vote_builder      = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_voter_t*                voter             = create_voter(VOTER_CBOR);
  cardano_governance_action_id_t* action_id         = create_governance_action_id();
  cardano_voting_procedure_t*     vote              = create_voting_procedure(VOTING_PROCEDURE_CBOR);

  // Act
  cardano_sub_tx_builder_vote(voter_builder, nullptr, action_id, vote, nullptr);
  cardano_sub_tx_builder_vote(action_id_builder, voter, nullptr, vote, nullptr);
  cardano_sub_tx_builder_vote(vote_builder, voter, action_id, nullptr, nullptr);

  // Assert
  EXPECT_EQ(voter_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(action_id_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(vote_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&voter_builder);
  cardano_sub_tx_builder_unref(&action_id_builder);
  cardano_sub_tx_builder_unref(&vote_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_voter_unref(&voter);
  cardano_governance_action_id_unref(&action_id);
  cardano_voting_procedure_unref(&vote);
}

TEST(cardano_sub_tx_builder_vote, canVote)
{
  // Arrange
  cardano_protocol_parameters_t*  params    = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*       builder   = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_voter_t*                voter     = create_voter(VOTER_CBOR);
  cardano_governance_action_id_t* action_id = create_governance_action_id();
  cardano_voting_procedure_t*     vote      = create_voting_procedure(VOTING_PROCEDURE_CBOR);

  // Act
  cardano_sub_tx_builder_vote(builder, voter, action_id, vote, nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_voting_procedures_t* procedures = cardano_sub_transaction_body_get_voting_procedures(get_body(sub_tx));
  cardano_voting_procedures_unref(&procedures);

  cardano_voter_list_t* voters = nullptr;

  EXPECT_EQ(cardano_voting_procedures_get_voters(procedures, &voters), CARDANO_SUCCESS);

  cardano_voting_procedure_t* cast_vote = cardano_voting_procedures_get(procedures, voter, action_id);

  // Assert
  EXPECT_EQ(cardano_voter_list_get_length(voters), 1);
  EXPECT_NE(cast_vote, nullptr);
  EXPECT_STREQ(body_hex, VOTE_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_voter_unref(&voter);
  cardano_governance_action_id_unref(&action_id);
  cardano_voting_procedure_unref(&vote);
  cardano_voter_list_unref(&voters);
  cardano_voting_procedure_unref(&cast_vote);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_vote, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t*  params    = init_protocol_parameters();
  cardano_sub_tx_builder_t*       builder   = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_voter_t*                voter     = create_voter(VOTER_CBOR);
  cardano_governance_action_id_t* action_id = create_governance_action_id();
  cardano_voting_procedure_t*     vote      = create_voting_procedure(VOTING_PROCEDURE_CBOR);
  cardano_sub_transaction_t*      sub_tx    = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_vote(builder, voter, action_id, vote, nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_voting_procedures(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_voter_unref(&voter);
  cardano_governance_action_id_unref(&action_id);
  cardano_voting_procedure_unref(&vote);
}

TEST(cardano_sub_tx_builder_vote, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*  params    = init_protocol_parameters();
  cardano_voter_t*                voter     = create_voter(VOTER_CBOR);
  cardano_governance_action_id_t* action_id = create_governance_action_id();
  cardano_voting_procedure_t*     vote      = create_voting_procedure(VOTING_PROCEDURE_CBOR);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_vote(builder, voter, action_id, vote, nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_voter_unref(&voter);
  cardano_governance_action_id_unref(&action_id);
  cardano_voting_procedure_unref(&vote);
}

TEST(cardano_sub_tx_builder_add_certificate, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_add_certificate(nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_add_certificate, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_add_certificate(builder, nullptr, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_certificate, canAddCertificate)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder     = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_certificate_t*         certificate = create_certificate(CERTIFICATE_CBOR);

  // Act
  cardano_sub_tx_builder_add_certificate(builder, certificate, nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_certificate_set_t* certificates = cardano_sub_transaction_body_get_certificates(get_body(sub_tx));
  cardano_certificate_set_unref(&certificates);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certificates), 1);
  EXPECT_EQ(get_certificate_type(certificates, 0), CARDANO_CERT_TYPE_STAKE_DELEGATION);
  EXPECT_STREQ(body_hex, CERTIFICATE_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_certificate_unref(&certificate);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_add_certificate, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder     = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_certificate_t*         certificate = create_certificate(CERTIFICATE_CBOR);
  cardano_sub_transaction_t*     sub_tx      = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_add_certificate(builder, certificate, nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_certificates(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_certificate_unref(&certificate);
}

TEST(cardano_sub_tx_builder_add_certificate, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_certificate_t*         certificate = create_certificate(CERTIFICATE_CBOR);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_add_certificate(builder, certificate, nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_certificate_unref(&certificate);
}

TEST(cardano_sub_tx_builder_propose_hardfork, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_propose_hardfork(nullptr, nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_propose_hardfork, returnsErrorIfARequiredArgumentIsNull)
{
  // Arrange
  cardano_protocol_parameters_t*  params                 = init_protocol_parameters();
  cardano_sub_tx_builder_t*       reward_address_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*       anchor_builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*       version_builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*       reward_address         = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*               anchor                 = create_anchor();
  cardano_protocol_version_t*     version                = create_protocol_version(12, 0);
  cardano_governance_action_id_t* action_id              = create_governance_action_id();

  // Act
  cardano_sub_tx_builder_propose_hardfork(reward_address_builder, nullptr, anchor, version, action_id);
  cardano_sub_tx_builder_propose_hardfork(anchor_builder, reward_address, nullptr, version, action_id);
  cardano_sub_tx_builder_propose_hardfork(version_builder, reward_address, anchor, nullptr, action_id);

  // Assert
  EXPECT_EQ(reward_address_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(anchor_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(version_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&reward_address_builder);
  cardano_sub_tx_builder_unref(&anchor_builder);
  cardano_sub_tx_builder_unref(&version_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_protocol_version_unref(&version);
  cardano_governance_action_id_unref(&action_id);
}

TEST(cardano_sub_tx_builder_propose_hardfork, canProposeHardfork)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*       builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*       reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*               anchor         = create_anchor();
  cardano_protocol_version_t*     version        = create_protocol_version(12, 0);
  cardano_governance_action_id_t* action_id      = create_governance_action_id();

  // Act
  cardano_sub_tx_builder_propose_hardfork(builder, reward_address, anchor, version, action_id);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_proposal_procedure_set_t* proposals = cardano_sub_transaction_body_get_proposal_procedures(get_body(sub_tx));
  cardano_proposal_procedure_set_unref(&proposals);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(proposals), 1);
  EXPECT_EQ(get_proposal_type(proposals, 0), CARDANO_GOVERNANCE_ACTION_TYPE_HARD_FORK_INITIATION);
  EXPECT_EQ(get_proposal_deposit(proposals, 0), GOVERNANCE_ACTION_DEPOSIT);
  EXPECT_STREQ(body_hex, HARDFORK_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_protocol_version_unref(&version);
  cardano_governance_action_id_unref(&action_id);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_propose_hardfork, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*       builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*       reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*               anchor         = create_anchor();
  cardano_protocol_version_t*     version        = create_protocol_version(12, 0);
  cardano_governance_action_id_t* action_id      = create_governance_action_id();
  cardano_sub_transaction_t*      sub_tx         = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_propose_hardfork(builder, reward_address, anchor, version, action_id);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_proposal_procedures(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_protocol_version_unref(&version);
  cardano_governance_action_id_unref(&action_id);
}

TEST(cardano_sub_tx_builder_propose_hardfork, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_protocol_parameters();
  cardano_reward_address_t*       reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*               anchor         = create_anchor();
  cardano_protocol_version_t*     version        = create_protocol_version(12, 0);
  cardano_governance_action_id_t* action_id      = create_governance_action_id();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_propose_hardfork(builder, reward_address, anchor, version, action_id);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_protocol_version_unref(&version);
  cardano_governance_action_id_unref(&action_id);
}

TEST(cardano_sub_tx_builder_propose_hardfork_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_propose_hardfork_ex(nullptr, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0, 0, 0);
}

TEST(cardano_sub_tx_builder_propose_hardfork_ex, returnsErrorIfAStringIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params            = init_protocol_parameters();
  cardano_sub_tx_builder_t*      address_builder   = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      url_builder       = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      hash_builder      = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      action_id_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_propose_hardfork_ex(address_builder, nullptr, 0, ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), 0, 12);
  cardano_sub_tx_builder_propose_hardfork_ex(url_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr, 0, ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), 0, 12);
  cardano_sub_tx_builder_propose_hardfork_ex(hash_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), nullptr, 0, GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), 0, 12);
  cardano_sub_tx_builder_propose_hardfork_ex(action_id_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr, 0, 0, 12);

  // Assert
  EXPECT_EQ(address_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(url_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(hash_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(action_id_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&address_builder);
  cardano_sub_tx_builder_unref(&url_builder);
  cardano_sub_tx_builder_unref(&hash_builder);
  cardano_sub_tx_builder_unref(&action_id_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_propose_hardfork_ex, canProposeHardfork)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_propose_hardfork_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), 0, 12);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_proposal_procedure_set_t* proposals = cardano_sub_transaction_body_get_proposal_procedures(get_body(sub_tx));
  cardano_proposal_procedure_set_unref(&proposals);

  cardano_protocol_version_t* version = get_hardfork_proposal_version(proposals, 0);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(proposals), 1);
  EXPECT_EQ(get_proposal_type(proposals, 0), CARDANO_GOVERNANCE_ACTION_TYPE_HARD_FORK_INITIATION);
  EXPECT_EQ(get_proposal_deposit(proposals, 0), GOVERNANCE_ACTION_DEPOSIT);
  EXPECT_EQ(cardano_protocol_version_get_major(version), 12U);
  EXPECT_EQ(cardano_protocol_version_get_minor(version), 0U);
  EXPECT_STREQ(body_hex, HARDFORK_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_protocol_version_unref(&version);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_propose_hardfork_ex, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_propose_hardfork_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), 0, 12);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_proposal_procedures(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_propose_hardfork_ex, reportsInvalidRewardAddressWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_propose_hardfork_ex(builder, "invalid", strlen("invalid"), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), 0, 12);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse reward address.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_propose_hardfork_ex, reportsInvalidMetadataHashWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_propose_hardfork_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), "invalid", strlen("invalid"), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), 0, 12);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to create anchor.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_propose_hardfork_ex, reportsInvalidGovernanceActionIdWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_propose_hardfork_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), "invalid", strlen("invalid"), 0, 12);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse governance action ID.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_propose_hardfork_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_propose_hardfork_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), 0, 12);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_propose_no_confidence, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_propose_no_confidence(nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_propose_no_confidence, returnsErrorIfARequiredArgumentIsNull)
{
  // Arrange
  cardano_protocol_parameters_t*  params                 = init_protocol_parameters();
  cardano_sub_tx_builder_t*       reward_address_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*       anchor_builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*       reward_address         = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*               anchor                 = create_anchor();
  cardano_governance_action_id_t* action_id              = create_governance_action_id();

  // Act
  cardano_sub_tx_builder_propose_no_confidence(reward_address_builder, nullptr, anchor, action_id);
  cardano_sub_tx_builder_propose_no_confidence(anchor_builder, reward_address, nullptr, action_id);

  // Assert
  EXPECT_EQ(reward_address_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(anchor_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&reward_address_builder);
  cardano_sub_tx_builder_unref(&anchor_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
}

TEST(cardano_sub_tx_builder_propose_no_confidence, canProposeNoConfidence)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*       builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*       reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*               anchor         = create_anchor();
  cardano_governance_action_id_t* action_id      = create_governance_action_id();

  // Act
  cardano_sub_tx_builder_propose_no_confidence(builder, reward_address, anchor, action_id);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_proposal_procedure_set_t* proposals = cardano_sub_transaction_body_get_proposal_procedures(get_body(sub_tx));
  cardano_proposal_procedure_set_unref(&proposals);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(proposals), 1);
  EXPECT_EQ(get_proposal_type(proposals, 0), CARDANO_GOVERNANCE_ACTION_TYPE_NO_CONFIDENCE);
  EXPECT_EQ(get_proposal_deposit(proposals, 0), GOVERNANCE_ACTION_DEPOSIT);
  EXPECT_STREQ(body_hex, NO_CONFIDENCE_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_propose_no_confidence, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*       builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*       reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*               anchor         = create_anchor();
  cardano_governance_action_id_t* action_id      = create_governance_action_id();
  cardano_sub_transaction_t*      sub_tx         = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_propose_no_confidence(builder, reward_address, anchor, action_id);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_proposal_procedures(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
}

TEST(cardano_sub_tx_builder_propose_no_confidence, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_protocol_parameters();
  cardano_reward_address_t*       reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*               anchor         = create_anchor();
  cardano_governance_action_id_t* action_id      = create_governance_action_id();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_propose_no_confidence(builder, reward_address, anchor, action_id);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
}

TEST(cardano_sub_tx_builder_propose_no_confidence_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_propose_no_confidence_ex(nullptr, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0);
}

TEST(cardano_sub_tx_builder_propose_no_confidence_ex, returnsErrorIfAStringIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params            = init_protocol_parameters();
  cardano_sub_tx_builder_t*      address_builder   = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      url_builder       = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      hash_builder      = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      action_id_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_propose_no_confidence_ex(address_builder, nullptr, 0, ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID));
  cardano_sub_tx_builder_propose_no_confidence_ex(url_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr, 0, ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID));
  cardano_sub_tx_builder_propose_no_confidence_ex(hash_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), nullptr, 0, GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID));
  cardano_sub_tx_builder_propose_no_confidence_ex(action_id_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr, 0);

  // Assert
  EXPECT_EQ(address_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(url_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(hash_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(action_id_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&address_builder);
  cardano_sub_tx_builder_unref(&url_builder);
  cardano_sub_tx_builder_unref(&hash_builder);
  cardano_sub_tx_builder_unref(&action_id_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_propose_no_confidence_ex, canProposeNoConfidence)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_propose_no_confidence_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID));

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_proposal_procedure_set_t* proposals = cardano_sub_transaction_body_get_proposal_procedures(get_body(sub_tx));
  cardano_proposal_procedure_set_unref(&proposals);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(proposals), 1);
  EXPECT_EQ(get_proposal_type(proposals, 0), CARDANO_GOVERNANCE_ACTION_TYPE_NO_CONFIDENCE);
  EXPECT_EQ(get_proposal_deposit(proposals, 0), GOVERNANCE_ACTION_DEPOSIT);
  EXPECT_STREQ(body_hex, NO_CONFIDENCE_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_propose_no_confidence_ex, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_propose_no_confidence_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID));

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_proposal_procedures(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_propose_no_confidence_ex, reportsInvalidRewardAddressWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_propose_no_confidence_ex(builder, "invalid", strlen("invalid"), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID));

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse reward address.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_propose_no_confidence_ex, reportsInvalidMetadataHashWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_propose_no_confidence_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), "invalid", strlen("invalid"), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID));

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to create anchor.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_propose_no_confidence_ex, reportsInvalidGovernanceActionIdWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_propose_no_confidence_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), "invalid", strlen("invalid"));

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse governance action ID.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_propose_no_confidence_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_propose_no_confidence_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID));

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_propose_update_committee, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_propose_update_committee(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_propose_update_committee, returnsErrorIfARequiredArgumentIsNull)
{
  // Arrange
  cardano_protocol_parameters_t*   params                        = init_protocol_parameters();
  cardano_sub_tx_builder_t*        reward_address_builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*        anchor_builder                = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*        members_to_be_removed_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*        members_to_be_added_builder   = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*        new_quorum_builder            = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*        reward_address                = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*                anchor                        = create_anchor();
  cardano_governance_action_id_t*  action_id                     = create_governance_action_id();
  cardano_credential_set_t*        members_to_be_removed         = create_credential_set(CREDENTIAL_SET_CBOR);
  cardano_committee_members_map_t* members_to_be_added           = create_committee_members_map(COMMITTEE_MEMBERS_MAP_CBOR);
  cardano_unit_interval_t*         new_quorum                    = create_unit_interval(0.5);

  // Act
  cardano_sub_tx_builder_propose_update_committee(reward_address_builder, nullptr, anchor, action_id, members_to_be_removed, members_to_be_added, new_quorum);
  cardano_sub_tx_builder_propose_update_committee(anchor_builder, reward_address, nullptr, action_id, members_to_be_removed, members_to_be_added, new_quorum);
  cardano_sub_tx_builder_propose_update_committee(members_to_be_removed_builder, reward_address, anchor, action_id, nullptr, members_to_be_added, new_quorum);
  cardano_sub_tx_builder_propose_update_committee(members_to_be_added_builder, reward_address, anchor, action_id, members_to_be_removed, nullptr, new_quorum);
  cardano_sub_tx_builder_propose_update_committee(new_quorum_builder, reward_address, anchor, action_id, members_to_be_removed, members_to_be_added, nullptr);

  // Assert
  EXPECT_EQ(reward_address_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(anchor_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(members_to_be_removed_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(members_to_be_added_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(new_quorum_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&reward_address_builder);
  cardano_sub_tx_builder_unref(&anchor_builder);
  cardano_sub_tx_builder_unref(&members_to_be_removed_builder);
  cardano_sub_tx_builder_unref(&members_to_be_added_builder);
  cardano_sub_tx_builder_unref(&new_quorum_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
  cardano_credential_set_unref(&members_to_be_removed);
  cardano_committee_members_map_unref(&members_to_be_added);
  cardano_unit_interval_unref(&new_quorum);
}

TEST(cardano_sub_tx_builder_propose_update_committee, canProposeUpdateCommittee)
{
  // Arrange
  cardano_protocol_parameters_t*   params                = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*        builder               = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*        reward_address        = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*                anchor                = create_anchor();
  cardano_governance_action_id_t*  action_id             = create_governance_action_id();
  cardano_credential_set_t*        members_to_be_removed = create_credential_set(CREDENTIAL_SET_CBOR);
  cardano_committee_members_map_t* members_to_be_added   = create_committee_members_map(COMMITTEE_MEMBERS_MAP_CBOR);
  cardano_unit_interval_t*         new_quorum            = create_unit_interval(0.5);

  // Act
  cardano_sub_tx_builder_propose_update_committee(builder, reward_address, anchor, action_id, members_to_be_removed, members_to_be_added, new_quorum);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_proposal_procedure_set_t* proposals = cardano_sub_transaction_body_get_proposal_procedures(get_body(sub_tx));
  cardano_proposal_procedure_set_unref(&proposals);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(proposals), 1);
  EXPECT_EQ(get_proposal_type(proposals, 0), CARDANO_GOVERNANCE_ACTION_TYPE_UPDATE_COMMITTEE);
  EXPECT_EQ(get_proposal_deposit(proposals, 0), GOVERNANCE_ACTION_DEPOSIT);
  EXPECT_STREQ(body_hex, UPDATE_COMMITTEE_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
  cardano_credential_set_unref(&members_to_be_removed);
  cardano_committee_members_map_unref(&members_to_be_added);
  cardano_unit_interval_unref(&new_quorum);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_propose_update_committee, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t*   params                = init_protocol_parameters();
  cardano_sub_tx_builder_t*        builder               = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*        reward_address        = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*                anchor                = create_anchor();
  cardano_governance_action_id_t*  action_id             = create_governance_action_id();
  cardano_credential_set_t*        members_to_be_removed = create_credential_set(CREDENTIAL_SET_CBOR);
  cardano_committee_members_map_t* members_to_be_added   = create_committee_members_map(COMMITTEE_MEMBERS_MAP_CBOR);
  cardano_unit_interval_t*         new_quorum            = create_unit_interval(0.5);
  cardano_sub_transaction_t*       sub_tx                = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_propose_update_committee(builder, reward_address, anchor, action_id, members_to_be_removed, members_to_be_added, new_quorum);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_proposal_procedures(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
  cardano_credential_set_unref(&members_to_be_removed);
  cardano_committee_members_map_unref(&members_to_be_added);
  cardano_unit_interval_unref(&new_quorum);
}

TEST(cardano_sub_tx_builder_propose_update_committee, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*   params                = init_protocol_parameters();
  cardano_reward_address_t*        reward_address        = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*                anchor                = create_anchor();
  cardano_governance_action_id_t*  action_id             = create_governance_action_id();
  cardano_credential_set_t*        members_to_be_removed = create_credential_set(CREDENTIAL_SET_CBOR);
  cardano_committee_members_map_t* members_to_be_added   = create_committee_members_map(COMMITTEE_MEMBERS_MAP_CBOR);
  cardano_unit_interval_t*         new_quorum            = create_unit_interval(0.5);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_propose_update_committee(builder, reward_address, anchor, action_id, members_to_be_removed, members_to_be_added, new_quorum);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
  cardano_credential_set_unref(&members_to_be_removed);
  cardano_committee_members_map_unref(&members_to_be_added);
  cardano_unit_interval_unref(&new_quorum);
}

TEST(cardano_sub_tx_builder_propose_update_committee_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_propose_update_committee_ex(nullptr, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, nullptr, 0);
}

TEST(cardano_sub_tx_builder_propose_update_committee_ex, returnsErrorIfAStringIsNull)
{
  // Arrange
  cardano_protocol_parameters_t*   params                = init_protocol_parameters();
  cardano_sub_tx_builder_t*        address_builder       = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*        url_builder           = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*        hash_builder          = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*        action_id_builder     = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_credential_set_t*        members_to_be_removed = create_credential_set(CREDENTIAL_SET_CBOR);
  cardano_committee_members_map_t* members_to_be_added   = create_committee_members_map(COMMITTEE_MEMBERS_MAP_CBOR);

  // Act
  cardano_sub_tx_builder_propose_update_committee_ex(address_builder, nullptr, 0, ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), members_to_be_removed, members_to_be_added, 0.5);
  cardano_sub_tx_builder_propose_update_committee_ex(url_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr, 0, ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), members_to_be_removed, members_to_be_added, 0.5);
  cardano_sub_tx_builder_propose_update_committee_ex(hash_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), nullptr, 0, GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), members_to_be_removed, members_to_be_added, 0.5);
  cardano_sub_tx_builder_propose_update_committee_ex(action_id_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr, 0, members_to_be_removed, members_to_be_added, 0.5);

  // Assert
  EXPECT_EQ(address_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(url_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(hash_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(action_id_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&address_builder);
  cardano_sub_tx_builder_unref(&url_builder);
  cardano_sub_tx_builder_unref(&hash_builder);
  cardano_sub_tx_builder_unref(&action_id_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_credential_set_unref(&members_to_be_removed);
  cardano_committee_members_map_unref(&members_to_be_added);
}

TEST(cardano_sub_tx_builder_propose_update_committee_ex, canProposeUpdateCommittee)
{
  // Arrange
  cardano_protocol_parameters_t*   params                = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*        builder               = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_credential_set_t*        members_to_be_removed = create_credential_set(CREDENTIAL_SET_CBOR);
  cardano_committee_members_map_t* members_to_be_added   = create_committee_members_map(COMMITTEE_MEMBERS_MAP_CBOR);

  // Act
  cardano_sub_tx_builder_propose_update_committee_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), members_to_be_removed, members_to_be_added, 0.5);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_proposal_procedure_set_t* proposals = cardano_sub_transaction_body_get_proposal_procedures(get_body(sub_tx));
  cardano_proposal_procedure_set_unref(&proposals);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(proposals), 1);
  EXPECT_EQ(get_proposal_type(proposals, 0), CARDANO_GOVERNANCE_ACTION_TYPE_UPDATE_COMMITTEE);
  EXPECT_EQ(get_proposal_deposit(proposals, 0), GOVERNANCE_ACTION_DEPOSIT);
  EXPECT_STREQ(body_hex, UPDATE_COMMITTEE_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_credential_set_unref(&members_to_be_removed);
  cardano_committee_members_map_unref(&members_to_be_added);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_propose_update_committee_ex, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t*   params                = init_protocol_parameters();
  cardano_sub_tx_builder_t*        builder               = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_credential_set_t*        members_to_be_removed = create_credential_set(CREDENTIAL_SET_CBOR);
  cardano_committee_members_map_t* members_to_be_added   = create_committee_members_map(COMMITTEE_MEMBERS_MAP_CBOR);
  cardano_sub_transaction_t*       sub_tx                = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_propose_update_committee_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), members_to_be_removed, members_to_be_added, 0.5);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_proposal_procedures(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_credential_set_unref(&members_to_be_removed);
  cardano_committee_members_map_unref(&members_to_be_added);
}

TEST(cardano_sub_tx_builder_propose_update_committee_ex, reportsInvalidRewardAddressWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t*   params                = init_protocol_parameters();
  cardano_sub_tx_builder_t*        builder               = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_credential_set_t*        members_to_be_removed = create_credential_set(CREDENTIAL_SET_CBOR);
  cardano_committee_members_map_t* members_to_be_added   = create_committee_members_map(COMMITTEE_MEMBERS_MAP_CBOR);
  cardano_sub_transaction_t*       sub_tx                = nullptr;

  // Act
  cardano_sub_tx_builder_propose_update_committee_ex(builder, "invalid", strlen("invalid"), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), members_to_be_removed, members_to_be_added, 0.5);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse reward address.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_credential_set_unref(&members_to_be_removed);
  cardano_committee_members_map_unref(&members_to_be_added);
}

TEST(cardano_sub_tx_builder_propose_update_committee_ex, reportsInvalidMetadataHashWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t*   params                = init_protocol_parameters();
  cardano_sub_tx_builder_t*        builder               = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_credential_set_t*        members_to_be_removed = create_credential_set(CREDENTIAL_SET_CBOR);
  cardano_committee_members_map_t* members_to_be_added   = create_committee_members_map(COMMITTEE_MEMBERS_MAP_CBOR);
  cardano_sub_transaction_t*       sub_tx                = nullptr;

  // Act
  cardano_sub_tx_builder_propose_update_committee_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), "invalid", strlen("invalid"), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), members_to_be_removed, members_to_be_added, 0.5);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to create anchor.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_credential_set_unref(&members_to_be_removed);
  cardano_committee_members_map_unref(&members_to_be_added);
}

TEST(cardano_sub_tx_builder_propose_update_committee_ex, reportsInvalidGovernanceActionIdWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t*   params                = init_protocol_parameters();
  cardano_sub_tx_builder_t*        builder               = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_credential_set_t*        members_to_be_removed = create_credential_set(CREDENTIAL_SET_CBOR);
  cardano_committee_members_map_t* members_to_be_added   = create_committee_members_map(COMMITTEE_MEMBERS_MAP_CBOR);
  cardano_sub_transaction_t*       sub_tx                = nullptr;

  // Act
  cardano_sub_tx_builder_propose_update_committee_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), "invalid", strlen("invalid"), members_to_be_removed, members_to_be_added, 0.5);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse governance action ID.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_credential_set_unref(&members_to_be_removed);
  cardano_committee_members_map_unref(&members_to_be_added);
}

TEST(cardano_sub_tx_builder_propose_update_committee_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*   params                = init_protocol_parameters();
  cardano_credential_set_t*        members_to_be_removed = create_credential_set(CREDENTIAL_SET_CBOR);
  cardano_committee_members_map_t* members_to_be_added   = create_committee_members_map(COMMITTEE_MEMBERS_MAP_CBOR);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_propose_update_committee_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), members_to_be_removed, members_to_be_added, 0.5);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_credential_set_unref(&members_to_be_removed);
  cardano_committee_members_map_unref(&members_to_be_added);
}

TEST(cardano_sub_tx_builder_propose_new_constitution, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_propose_new_constitution(nullptr, nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_propose_new_constitution, returnsErrorIfARequiredArgumentIsNull)
{
  // Arrange
  cardano_protocol_parameters_t*  params                 = init_protocol_parameters();
  cardano_sub_tx_builder_t*       reward_address_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*       anchor_builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*       constitution_builder   = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*       reward_address         = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*               anchor                 = create_anchor();
  cardano_governance_action_id_t* action_id              = create_governance_action_id();
  cardano_constitution_t*         constitution           = create_constitution(CONSTITUTION_CBOR);

  // Act
  cardano_sub_tx_builder_propose_new_constitution(reward_address_builder, nullptr, anchor, action_id, constitution);
  cardano_sub_tx_builder_propose_new_constitution(anchor_builder, reward_address, nullptr, action_id, constitution);
  cardano_sub_tx_builder_propose_new_constitution(constitution_builder, reward_address, anchor, action_id, nullptr);

  // Assert
  EXPECT_EQ(reward_address_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(anchor_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(constitution_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&reward_address_builder);
  cardano_sub_tx_builder_unref(&anchor_builder);
  cardano_sub_tx_builder_unref(&constitution_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
  cardano_constitution_unref(&constitution);
}

TEST(cardano_sub_tx_builder_propose_new_constitution, canProposeNewConstitution)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*       builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*       reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*               anchor         = create_anchor();
  cardano_governance_action_id_t* action_id      = create_governance_action_id();
  cardano_constitution_t*         constitution   = create_constitution(CONSTITUTION_CBOR);

  // Act
  cardano_sub_tx_builder_propose_new_constitution(builder, reward_address, anchor, action_id, constitution);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_proposal_procedure_set_t* proposals = cardano_sub_transaction_body_get_proposal_procedures(get_body(sub_tx));
  cardano_proposal_procedure_set_unref(&proposals);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(proposals), 1);
  EXPECT_EQ(get_proposal_type(proposals, 0), CARDANO_GOVERNANCE_ACTION_TYPE_NEW_CONSTITUTION);
  EXPECT_EQ(get_proposal_deposit(proposals, 0), GOVERNANCE_ACTION_DEPOSIT);
  EXPECT_STREQ(body_hex, NEW_CONSTITUTION_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
  cardano_constitution_unref(&constitution);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_propose_new_constitution, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*       builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*       reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*               anchor         = create_anchor();
  cardano_governance_action_id_t* action_id      = create_governance_action_id();
  cardano_constitution_t*         constitution   = create_constitution(CONSTITUTION_CBOR);
  cardano_sub_transaction_t*      sub_tx         = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_propose_new_constitution(builder, reward_address, anchor, action_id, constitution);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_proposal_procedures(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
  cardano_constitution_unref(&constitution);
}

TEST(cardano_sub_tx_builder_propose_new_constitution, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_protocol_parameters();
  cardano_reward_address_t*       reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*               anchor         = create_anchor();
  cardano_governance_action_id_t* action_id      = create_governance_action_id();
  cardano_constitution_t*         constitution   = create_constitution(CONSTITUTION_CBOR);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_propose_new_constitution(builder, reward_address, anchor, action_id, constitution);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_governance_action_id_unref(&action_id);
  cardano_constitution_unref(&constitution);
}

TEST(cardano_sub_tx_builder_propose_new_constitution_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_propose_new_constitution_ex(nullptr, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0, nullptr);
}

TEST(cardano_sub_tx_builder_propose_new_constitution_ex, returnsErrorIfAStringIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params            = init_protocol_parameters();
  cardano_sub_tx_builder_t*      address_builder   = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      url_builder       = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      hash_builder      = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      action_id_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_constitution_t*        constitution      = create_constitution(CONSTITUTION_CBOR);

  // Act
  cardano_sub_tx_builder_propose_new_constitution_ex(address_builder, nullptr, 0, ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), constitution);
  cardano_sub_tx_builder_propose_new_constitution_ex(url_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr, 0, ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), constitution);
  cardano_sub_tx_builder_propose_new_constitution_ex(hash_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), nullptr, 0, GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), constitution);
  cardano_sub_tx_builder_propose_new_constitution_ex(action_id_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr, 0, constitution);

  // Assert
  EXPECT_EQ(address_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(url_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(hash_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(action_id_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&address_builder);
  cardano_sub_tx_builder_unref(&url_builder);
  cardano_sub_tx_builder_unref(&hash_builder);
  cardano_sub_tx_builder_unref(&action_id_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_constitution_unref(&constitution);
}

TEST(cardano_sub_tx_builder_propose_new_constitution_ex, canProposeNewConstitution)
{
  // Arrange
  cardano_protocol_parameters_t* params       = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder      = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_constitution_t*        constitution = create_constitution(CONSTITUTION_CBOR);

  // Act
  cardano_sub_tx_builder_propose_new_constitution_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), constitution);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_proposal_procedure_set_t* proposals = cardano_sub_transaction_body_get_proposal_procedures(get_body(sub_tx));
  cardano_proposal_procedure_set_unref(&proposals);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(proposals), 1);
  EXPECT_EQ(get_proposal_type(proposals, 0), CARDANO_GOVERNANCE_ACTION_TYPE_NEW_CONSTITUTION);
  EXPECT_EQ(get_proposal_deposit(proposals, 0), GOVERNANCE_ACTION_DEPOSIT);
  EXPECT_STREQ(body_hex, NEW_CONSTITUTION_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_constitution_unref(&constitution);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_propose_new_constitution_ex, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params       = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder      = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_constitution_t*        constitution = create_constitution(CONSTITUTION_CBOR);
  cardano_sub_transaction_t*     sub_tx       = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_propose_new_constitution_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), constitution);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_proposal_procedures(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_constitution_unref(&constitution);
}

TEST(cardano_sub_tx_builder_propose_new_constitution_ex, reportsInvalidRewardAddressWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params       = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder      = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_constitution_t*        constitution = create_constitution(CONSTITUTION_CBOR);
  cardano_sub_transaction_t*     sub_tx       = nullptr;

  // Act
  cardano_sub_tx_builder_propose_new_constitution_ex(builder, "invalid", strlen("invalid"), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), constitution);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse reward address.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_constitution_unref(&constitution);
}

TEST(cardano_sub_tx_builder_propose_new_constitution_ex, reportsInvalidMetadataHashWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params       = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder      = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_constitution_t*        constitution = create_constitution(CONSTITUTION_CBOR);
  cardano_sub_transaction_t*     sub_tx       = nullptr;

  // Act
  cardano_sub_tx_builder_propose_new_constitution_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), "invalid", strlen("invalid"), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), constitution);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to create anchor.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_constitution_unref(&constitution);
}

TEST(cardano_sub_tx_builder_propose_new_constitution_ex, reportsInvalidGovernanceActionIdWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params       = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder      = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_constitution_t*        constitution = create_constitution(CONSTITUTION_CBOR);
  cardano_sub_transaction_t*     sub_tx       = nullptr;

  // Act
  cardano_sub_tx_builder_propose_new_constitution_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), "invalid", strlen("invalid"), constitution);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse governance action ID.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_constitution_unref(&constitution);
}

TEST(cardano_sub_tx_builder_propose_new_constitution_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params       = init_protocol_parameters();
  cardano_constitution_t*        constitution = create_constitution(CONSTITUTION_CBOR);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_propose_new_constitution_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), constitution);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_constitution_unref(&constitution);
}

TEST(cardano_sub_tx_builder_propose_info, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_propose_info(nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_propose_info, returnsErrorIfARequiredArgumentIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params                 = init_protocol_parameters();
  cardano_sub_tx_builder_t*      reward_address_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      anchor_builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address         = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*              anchor                 = create_anchor();

  // Act
  cardano_sub_tx_builder_propose_info(reward_address_builder, nullptr, anchor);
  cardano_sub_tx_builder_propose_info(anchor_builder, reward_address, nullptr);

  // Assert
  EXPECT_EQ(reward_address_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(anchor_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&reward_address_builder);
  cardano_sub_tx_builder_unref(&anchor_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_sub_tx_builder_propose_info, canProposeInfo)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*              anchor         = create_anchor();

  // Act
  cardano_sub_tx_builder_propose_info(builder, reward_address, anchor);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_proposal_procedure_set_t* proposals = cardano_sub_transaction_body_get_proposal_procedures(get_body(sub_tx));
  cardano_proposal_procedure_set_unref(&proposals);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(proposals), 1);
  EXPECT_EQ(get_proposal_type(proposals, 0), CARDANO_GOVERNANCE_ACTION_TYPE_INFO);
  EXPECT_EQ(get_proposal_deposit(proposals, 0), GOVERNANCE_ACTION_DEPOSIT);
  EXPECT_STREQ(body_hex, INFO_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_propose_info, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*              anchor         = create_anchor();
  cardano_sub_transaction_t*     sub_tx         = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_propose_info(builder, reward_address, anchor);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_proposal_procedures(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_sub_tx_builder_propose_info, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*              anchor         = create_anchor();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_propose_info(builder, reward_address, anchor);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_sub_tx_builder_propose_info_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_propose_info_ex(nullptr, nullptr, 0, nullptr, 0, nullptr, 0);
}

TEST(cardano_sub_tx_builder_propose_info_ex, returnsErrorIfAStringIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  cardano_sub_tx_builder_t*      address_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      url_builder     = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      hash_builder    = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_propose_info_ex(address_builder, nullptr, 0, ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH));
  cardano_sub_tx_builder_propose_info_ex(url_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr, 0, ANCHOR_HASH, strlen(ANCHOR_HASH));
  cardano_sub_tx_builder_propose_info_ex(hash_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), nullptr, 0);

  // Assert
  EXPECT_EQ(address_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(url_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(hash_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&address_builder);
  cardano_sub_tx_builder_unref(&url_builder);
  cardano_sub_tx_builder_unref(&hash_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_propose_info_ex, canProposeInfo)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_propose_info_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH));

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  cardano_proposal_procedure_set_t* proposals = cardano_sub_transaction_body_get_proposal_procedures(get_body(sub_tx));
  cardano_proposal_procedure_set_unref(&proposals);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(proposals), 1);
  EXPECT_EQ(get_proposal_type(proposals, 0), CARDANO_GOVERNANCE_ACTION_TYPE_INFO);
  EXPECT_EQ(get_proposal_deposit(proposals, 0), GOVERNANCE_ACTION_DEPOSIT);
  EXPECT_STREQ(body_hex, INFO_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_propose_info_ex, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_propose_info_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH));

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_proposal_procedures(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_propose_info_ex, reportsInvalidRewardAddressWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_propose_info_ex(builder, "invalid", strlen("invalid"), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH));

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to parse reward address.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_propose_info_ex, reportsInvalidMetadataHashWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_propose_info_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), "invalid", strlen("invalid"));

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to create anchor.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_propose_info_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_propose_info_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH));

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_build, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_sub_transaction_t* sub_tx = nullptr;

  // Act
  const cardano_error_t result = cardano_sub_tx_builder_build(nullptr, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(sub_tx, nullptr);
}

TEST(cardano_sub_tx_builder_build, returnsErrorIfSubTransactionIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  const cardano_error_t result = cardano_sub_tx_builder_build(builder, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_build, returnsErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_build, reportsTheFirstErrorAndIgnoresLaterCalls)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script  = create_script(PLUTUS_V3_CBOR);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_guard(builder, nullptr);
  cardano_sub_tx_builder_send_lovelace_ex(builder, ADDRESS, strlen(ADDRESS), 2000000);
  cardano_sub_tx_builder_add_direct_deposit_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 3000000);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), PLUTUS_SCRIPT_ERROR);
  EXPECT_EQ(sub_tx, nullptr);
  EXPECT_EQ(cardano_transaction_output_list_get_length(outputs), 0);
  EXPECT_EQ(cardano_transaction_body_get_direct_deposits(body), nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_script_unref(&script);
}

TEST(cardano_sub_tx_builder_build, canBuildWithNothingAdded)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  char*                      body_hex = encode_body(sub_tx);

  // Assert
  EXPECT_NE(sub_tx, nullptr);
  EXPECT_EQ(cardano_sub_transaction_get_auxiliary_data(sub_tx), nullptr);
  EXPECT_STREQ(body_hex, EMPTY_BODY_CBOR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  free(body_hex);
}

TEST(cardano_sub_tx_builder_build, canOnlyBuildOnce)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_transaction_t*     sub_tx  = build_sub_transaction(builder);
  cardano_sub_transaction_t*     second  = nullptr;

  // Act
  cardano_sub_tx_builder_send_lovelace_ex(builder, ADDRESS, strlen(ADDRESS), 2000000);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &second);

  cardano_transaction_output_list_t* outputs = cardano_sub_transaction_body_get_outputs(get_body(sub_tx));
  cardano_transaction_output_list_unref(&outputs);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ILLEGAL_STATE);
  EXPECT_EQ(second, nullptr);
  EXPECT_EQ(cardano_transaction_output_list_get_length(outputs), 0);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_build, builtSubTransactionRoundTripsByteExact)
{
  // Arrange
  cardano_protocol_parameters_t*      params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*           builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_utxo_t*                     utxo           = create_utxo(UTXO_CBOR);
  cardano_utxo_t*                     reference_utxo = create_utxo(UTXO_WITH_REF_SCRIPT_NATIVE);
  cardano_script_t*                   script         = create_script(NATIVE_SCRIPT_CBOR);
  cardano_plutus_data_t*              datum          = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_account_balance_interval_t* interval       = create_interval(1000000);

  cardano_sub_tx_builder_set_network_id(builder, CARDANO_NETWORK_ID_TEST_NET);
  cardano_sub_tx_builder_set_invalid_before(builder, 1000);
  cardano_sub_tx_builder_set_invalid_after(builder, 2000);
  cardano_sub_tx_builder_set_donation(builder, 500000);
  cardano_sub_tx_builder_add_input(builder, utxo, nullptr, nullptr);
  cardano_sub_tx_builder_add_reference_input(builder, reference_utxo);
  cardano_sub_tx_builder_send_lovelace_ex(builder, ADDRESS, strlen(ADDRESS), 2000000);
  cardano_sub_tx_builder_mint_token_with_id_ex(builder, ASSET_ID_HEX, strlen(ASSET_ID_HEX), 4, nullptr);
  cardano_sub_tx_builder_set_metadata_ex(builder, 674, METADATA_JSON, strlen(METADATA_JSON));
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_guard_ex(builder, HASH_HEX, strlen(HASH_HEX), CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  cardano_sub_tx_builder_require_top_level_guard_ex(builder, SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, datum);
  cardano_sub_tx_builder_add_direct_deposit_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 3000000);
  cardano_sub_tx_builder_add_account_balance_interval_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), interval);

  cardano_sub_transaction_t* sub_tx     = build_sub_transaction(builder);
  char*                      sub_tx_hex = encode_sub_transaction(sub_tx);

  // Act
  cardano_cbor_reader_t*     reader  = cardano_cbor_reader_from_hex(sub_tx_hex, strlen(sub_tx_hex));
  cardano_sub_transaction_t* decoded = nullptr;

  EXPECT_EQ(cardano_sub_transaction_from_cbor(reader, &decoded), CARDANO_SUCCESS);

  cardano_sub_transaction_clear_cbor_cache(decoded);

  char* decoded_hex = encode_sub_transaction(decoded);

  cardano_blake2b_hash_t* id         = cardano_sub_transaction_get_id(sub_tx);
  cardano_blake2b_hash_t* decoded_id = cardano_sub_transaction_get_id(decoded);

  // Assert
  EXPECT_STREQ(sub_tx_hex, FULL_SUB_TX_CBOR);
  EXPECT_STREQ(decoded_hex, sub_tx_hex);
  EXPECT_TRUE(cardano_blake2b_hash_equals(id, decoded_id));

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_transaction_unref(&decoded);
  cardano_cbor_reader_unref(&reader);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_script_unref(&script);
  cardano_plutus_data_unref(&datum);
  cardano_account_balance_interval_unref(&interval);
  cardano_blake2b_hash_unref(&id);
  cardano_blake2b_hash_unref(&decoded_id);
  free(sub_tx_hex);
  free(decoded_hex);
}

TEST(cardano_sub_tx_builder_build, imbalanceIsTheInputsMinusTheOutputs)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_utxo_t*                utxo            = create_utxo(UTXO_CBOR);
  cardano_utxo_list_t*           resolved_inputs = nullptr;
  cardano_value_t*               imbalance       = nullptr;

  cardano_value_t* expected_imbalance = cardano_value_new_from_coin(234831727 - 100000000);

  EXPECT_EQ(cardano_utxo_list_new(&resolved_inputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(resolved_inputs, utxo), CARDANO_SUCCESS);

  cardano_sub_tx_builder_add_input(builder, utxo, nullptr, nullptr);
  cardano_sub_tx_builder_send_lovelace_ex(builder, ADDRESS, strlen(ADDRESS), 100000000);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Act
  const cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, params, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_value_equals(imbalance, expected_imbalance));
  EXPECT_EQ(cardano_value_get_coin(imbalance), 134831727);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_value_unref(&imbalance);
  cardano_value_unref(&expected_imbalance);
}

TEST(cardano_sub_tx_builder_build, imbalanceCarriesTheAssetsThatAreNotSent)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_utxo_t*                utxo            = create_utxo(UTXO_WITH_ASSETS_CBOR);
  cardano_utxo_list_t*           resolved_inputs = nullptr;
  cardano_value_t*               imbalance       = nullptr;

  EXPECT_EQ(cardano_utxo_list_new(&resolved_inputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(resolved_inputs, utxo), CARDANO_SUCCESS);

  cardano_transaction_output_t* resolved = cardano_utxo_get_output(utxo);
  cardano_transaction_output_unref(&resolved);

  cardano_value_t* offered_value = cardano_transaction_output_get_value(resolved);
  cardano_value_unref(&offered_value);

  cardano_sub_tx_builder_add_input(builder, utxo, nullptr, nullptr);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Act
  const cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, params, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_value_equals(imbalance, offered_value));

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_value_unref(&imbalance);
}

TEST(cardano_sub_tx_builder_build, certificatesWithdrawalsVotesAndProposalsRoundTripByteExact)
{
  // Arrange
  cardano_protocol_parameters_t*  params    = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*       builder   = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_voter_t*                voter     = create_voter(VOTER_CBOR);
  cardano_governance_action_id_t* action_id = create_governance_action_id();
  cardano_voting_procedure_t*     vote      = create_voting_procedure(VOTING_PROCEDURE_CBOR);

  cardano_sub_tx_builder_withdraw_rewards_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 2532145, nullptr);
  cardano_sub_tx_builder_register_reward_address_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr);
  cardano_sub_tx_builder_delegate_stake_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), POOL_ID, strlen(POOL_ID), nullptr);
  cardano_sub_tx_builder_vote(builder, voter, action_id, vote, nullptr);
  cardano_sub_tx_builder_propose_info_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH));

  cardano_sub_transaction_t* sub_tx     = build_sub_transaction(builder);
  char*                      sub_tx_hex = encode_sub_transaction(sub_tx);

  // Act
  cardano_cbor_reader_t*     reader  = cardano_cbor_reader_from_hex(sub_tx_hex, strlen(sub_tx_hex));
  cardano_sub_transaction_t* decoded = nullptr;

  EXPECT_EQ(cardano_sub_transaction_from_cbor(reader, &decoded), CARDANO_SUCCESS);

  cardano_sub_transaction_clear_cbor_cache(decoded);

  char* decoded_hex = encode_sub_transaction(decoded);

  cardano_blake2b_hash_t* id         = cardano_sub_transaction_get_id(sub_tx);
  cardano_blake2b_hash_t* decoded_id = cardano_sub_transaction_get_id(decoded);

  cardano_witness_set_t* witness_set = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_witness_set_unref(&witness_set);

  // Assert
  EXPECT_STREQ(sub_tx_hex, GOVERNANCE_SUB_TX_CBOR);
  EXPECT_STREQ(decoded_hex, sub_tx_hex);
  EXPECT_TRUE(cardano_blake2b_hash_equals(id, decoded_id));
  EXPECT_EQ(cardano_witness_set_get_redeemers(witness_set), nullptr);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_transaction_unref(&decoded);
  cardano_cbor_reader_unref(&reader);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_voter_unref(&voter);
  cardano_governance_action_id_unref(&action_id);
  cardano_voting_procedure_unref(&vote);
  cardano_blake2b_hash_unref(&id);
  cardano_blake2b_hash_unref(&decoded_id);
  free(sub_tx_hex);
  free(decoded_hex);
}

TEST(cardano_sub_tx_builder_build, imbalanceReflectsWithdrawals)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_utxo_list_t*           resolved_inputs = nullptr;
  cardano_value_t*               imbalance       = nullptr;

  EXPECT_EQ(cardano_utxo_list_new(&resolved_inputs), CARDANO_SUCCESS);

  cardano_sub_tx_builder_withdraw_rewards_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 2532145, nullptr);
  cardano_sub_tx_builder_withdraw_rewards_ex(builder, REWARD_ADDRESS2, strlen(REWARD_ADDRESS2), 1000000, nullptr);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Act
  const cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, params, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(imbalance), 3532145);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_value_unref(&imbalance);
}

TEST(cardano_sub_tx_builder_build, imbalanceReflectsCertificateDeposits)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_utxo_list_t*           resolved_inputs = nullptr;
  cardano_value_t*               imbalance       = nullptr;

  EXPECT_EQ(cardano_utxo_list_new(&resolved_inputs), CARDANO_SUCCESS);

  cardano_sub_tx_builder_register_reward_address_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr);
  cardano_sub_tx_builder_register_drep_ex(builder, DREP_ID, strlen(DREP_ID), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr);
  cardano_sub_tx_builder_delegate_stake_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), POOL_ID, strlen(POOL_ID), nullptr);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Act
  const cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, params, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(imbalance), -502000000);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_value_unref(&imbalance);
}

TEST(cardano_sub_tx_builder_build, imbalanceReflectsCertificateRefunds)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_utxo_list_t*           resolved_inputs = nullptr;
  cardano_value_t*               imbalance       = nullptr;

  EXPECT_EQ(cardano_utxo_list_new(&resolved_inputs), CARDANO_SUCCESS);

  cardano_sub_tx_builder_deregister_reward_address_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr);
  cardano_sub_tx_builder_deregister_drep_ex(builder, DREP_ID, strlen(DREP_ID), nullptr);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Act
  const cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, params, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(imbalance), 502000000);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_value_unref(&imbalance);
}

TEST(cardano_sub_tx_builder_build, imbalanceIsNotChangedByVotes)
{
  // Arrange
  cardano_protocol_parameters_t*  params          = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*       builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_voter_t*                voter           = create_voter(VOTER_CBOR);
  cardano_governance_action_id_t* action_id       = create_governance_action_id();
  cardano_voting_procedure_t*     vote            = create_voting_procedure(VOTING_PROCEDURE_CBOR);
  cardano_utxo_list_t*            resolved_inputs = nullptr;
  cardano_value_t*                imbalance       = nullptr;

  EXPECT_EQ(cardano_utxo_list_new(&resolved_inputs), CARDANO_SUCCESS);

  cardano_sub_tx_builder_vote(builder, voter, action_id, vote, nullptr);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Act
  const cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, params, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_value_is_zero(imbalance));

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_voter_unref(&voter);
  cardano_governance_action_id_unref(&action_id);
  cardano_voting_procedure_unref(&vote);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_value_unref(&imbalance);
}

TEST(cardano_sub_tx_builder_build, imbalanceReflectsProposalDeposits)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters_with_deposits();
  cardano_sub_tx_builder_t*      builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_utxo_list_t*           resolved_inputs = nullptr;
  cardano_value_t*               imbalance       = nullptr;

  EXPECT_EQ(cardano_utxo_list_new(&resolved_inputs), CARDANO_SUCCESS);

  cardano_sub_tx_builder_propose_info_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH));
  cardano_sub_tx_builder_propose_no_confidence_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID));

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Act
  const cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, params, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(imbalance), -200000000000);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_value_unref(&imbalance);
}

TEST(cardano_sub_tx_builder_build, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_utxo_t*                utxo   = create_utxo(UTXO_CBOR);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t*  builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
    cardano_sub_transaction_t* sub_tx  = nullptr;

    cardano_sub_tx_builder_add_input(builder, utxo, nullptr, nullptr);
    cardano_sub_tx_builder_set_invalid_after(builder, 1000);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_NE(sub_tx, nullptr);
    }
    else
    {
      EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
      EXPECT_STRNE(cardano_sub_tx_builder_get_last_error(builder), "");
      EXPECT_EQ(sub_tx, nullptr);
    }

    cardano_sub_transaction_unref(&sub_tx);
    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
}

/* GUARDING REDEEMERS ********************************************************/

static const char* ALWAYS_SUCCEEDS_V4_CBOR = "820446450100002499";
static const char* ALWAYS_FAILS_V4_CBOR    = "820446450100002601";
static const char* GUARD_ERROR_KEY_HASH    = "Only a script hash guard is satisfied by a redeemer.";
static const char* GUARD_ERROR_DUPLICATED  = "The guard already has a redeemer.";

/**
 * Creates protocol parameters for the Dijkstra era, with a Plutus V4 cost model.
 * \return A new instance of the protocol parameters.
 */
static cardano_protocol_parameters_t*
init_dijkstra_protocol_parameters()
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_protocol_version_t*    version  = create_protocol_version(12U, 0U);
  cardano_costmdls_t*            costmdls = cardano_protocol_parameters_get_cost_models(params);
  cardano_cost_model_t*          v3_model = NULL;
  cardano_cost_model_t*          v4_model = NULL;

  EXPECT_EQ(cardano_protocol_parameters_set_protocol_version(params, version), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_costmdls_get(costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V3, &v3_model), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_cost_model_new(CARDANO_PLUTUS_LANGUAGE_VERSION_V4, cardano_cost_model_get_costs(v3_model), cardano_cost_model_get_costs_size(v3_model), &v4_model), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_costmdls_insert(costmdls, v4_model), CARDANO_SUCCESS);

  cardano_cost_model_unref(&v4_model);
  cardano_cost_model_unref(&v3_model);
  cardano_costmdls_unref(&costmdls);
  cardano_protocol_version_unref(&version);

  return params;
}

/**
 * Creates a script hash credential for a script.
 * \param script The script.
 * \return A new instance of the credential.
 */
static cardano_credential_t*
create_script_credential(cardano_script_t* script)
{
  cardano_blake2b_hash_t* hash       = cardano_script_get_hash(script);
  cardano_credential_t*   credential = NULL;

  EXPECT_EQ(cardano_credential_new(hash, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, &credential), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&hash);

  return credential;
}

/**
 * Gets the redeemer at a position of the witness set of a sub transaction.
 * \param sub_tx The sub transaction.
 * \param index The position of the redeemer.
 * \return A new reference to the redeemer.
 */
static cardano_redeemer_t*
get_sub_tx_redeemer(cardano_sub_transaction_t* sub_tx, size_t index)
{
  cardano_witness_set_t* witness_set = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_witness_set_unref(&witness_set);

  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness_set);
  cardano_redeemer_list_unref(&redeemers);

  cardano_redeemer_t* redeemer = NULL;

  EXPECT_EQ(cardano_redeemer_list_get(redeemers, index, &redeemer), CARDANO_SUCCESS);

  return redeemer;
}

TEST(cardano_sub_tx_builder_add_guard_with_redeemer, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_add_guard_with_redeemer(nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_add_guard_with_redeemer, returnsErrorIfGuardOrRedeemerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_sub_tx_builder_t*      no_guard    = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      no_redeemer = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_credential_t*          guard       = create_credential(SCRIPT_HASH_HEX, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);
  cardano_plutus_data_t*         redeemer    = create_plutus_data(PLUTUS_DATA_CBOR);

  // Act
  cardano_sub_tx_builder_add_guard_with_redeemer(no_guard, nullptr, redeemer);
  cardano_sub_tx_builder_add_guard_with_redeemer(no_redeemer, guard, nullptr);

  // Assert
  EXPECT_EQ(no_guard->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(no_guard), "Guard is NULL.");
  EXPECT_EQ(no_redeemer->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(no_redeemer), "Redeemer is NULL.");

  // Cleanup
  cardano_plutus_data_unref(&redeemer);
  cardano_credential_unref(&guard);
  cardano_sub_tx_builder_unref(&no_redeemer);
  cardano_sub_tx_builder_unref(&no_guard);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_guard_with_redeemer, rejectsAKeyHashGuard)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder  = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_credential_t*          guard    = create_credential(HASH_HEX, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);

  // Act
  cardano_sub_tx_builder_add_guard_with_redeemer(builder, guard, redeemer);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), GUARD_ERROR_KEY_HASH);

  // Cleanup
  cardano_plutus_data_unref(&redeemer);
  cardano_credential_unref(&guard);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_guard_with_redeemer, rejectsASecondRedeemerForTheSameGuard)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder  = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_credential_t*          guard    = create_credential(SCRIPT_HASH_HEX, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);

  // Act
  cardano_sub_tx_builder_add_guard_with_redeemer(builder, guard, redeemer);

  const cardano_error_t first = builder->last_error;

  cardano_sub_tx_builder_add_guard_with_redeemer(builder, guard, redeemer);

  // Assert
  EXPECT_EQ(first, CARDANO_SUCCESS);
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), GUARD_ERROR_DUPLICATED);

  // Cleanup
  cardano_plutus_data_unref(&redeemer);
  cardano_credential_unref(&guard);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_guard_with_redeemer, evaluatesTheGuardScriptWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params    = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder   = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script    = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_credential_t*          key_guard = create_credential(HASH_HEX, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  cardano_credential_t*          guard     = create_script_credential(script);
  cardano_plutus_data_t*         redeemer  = create_plutus_data(PLUTUS_DATA_CBOR);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_guard(builder, key_guard);
  cardano_sub_tx_builder_add_guard_with_redeemer(builder, guard, redeemer);

  cardano_sub_transaction_t* sub_tx  = build_sub_transaction(builder);
  cardano_redeemer_t*        guarded = get_sub_tx_redeemer(sub_tx, 0U);
  cardano_ex_units_t*        units   = cardano_redeemer_get_ex_units(guarded);
  cardano_blake2b_hash_t*    hash    = cardano_sub_transaction_body_get_script_data_hash(get_body(sub_tx));

  // Assert
  EXPECT_TRUE(builder->state.has_plutus_v4);
  EXPECT_EQ(cardano_redeemer_get_tag(guarded), CARDANO_REDEEMER_TAG_GUARDING);
  EXPECT_EQ(cardano_redeemer_get_index(guarded), 1U);
  EXPECT_EQ(cardano_ex_units_get_cpu_steps(units), 92100U);
  EXPECT_EQ(cardano_ex_units_get_memory(units), 500U);
  EXPECT_NE(hash, nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_ex_units_unref(&units);
  cardano_redeemer_unref(&guarded);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_credential_unref(&guard);
  cardano_credential_unref(&key_guard);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_guard_with_redeemer, reportsAFailingGuardScriptWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder  = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script   = create_script(ALWAYS_FAILS_V4_CBOR);
  cardano_credential_t*          guard    = create_script_credential(script);
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_sub_transaction_t*     sub_tx   = nullptr;

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_guard_with_redeemer(builder, guard, redeemer);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_SCRIPT_EVALUATION_FAILURE);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "phase-2 validation failed for a redeemer");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_plutus_data_unref(&redeemer);
  cardano_credential_unref(&guard);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_guard_with_redeemer, reportsAMissingPlutusV4CostModelWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_protocol_version_t*    version  = create_protocol_version(12U, 0U);
  cardano_sub_tx_builder_t*      builder  = nullptr;
  cardano_script_t*              script   = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_credential_t*          guard    = create_script_credential(script);
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_sub_transaction_t*     sub_tx   = nullptr;

  EXPECT_EQ(cardano_protocol_parameters_set_protocol_version(params, version), CARDANO_SUCCESS);

  builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_guard_with_redeemer(builder, guard, redeemer);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), "Failed to compute the script data hash of the sub transaction.");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_plutus_data_unref(&redeemer);
  cardano_credential_unref(&guard);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_version_unref(&version);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_guard_with_redeemer, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_credential_t*          guard    = create_credential(SCRIPT_HASH_HEX, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);
  bool                           added    = false;

  for (int i = 0; (i < 100) && !added; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    reset_limited_malloc();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_sub_tx_builder_add_guard_with_redeemer(builder, guard, redeemer);

    cardano_set_allocators(malloc, realloc, free);

    // Assert
    added = (builder->last_error == CARDANO_SUCCESS);

    if (!added)
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(added);

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_plutus_data_unref(&redeemer);
  cardano_credential_unref(&guard);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_guard_with_redeemer, returnsErrorIfMemoryAllocationFailsWhileBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_dijkstra_protocol_parameters();
  cardano_script_t*              script   = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_credential_t*          guard    = create_script_credential(script);
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);
  bool                           built    = false;

  for (int i = 0; (i < 100000) && !built; ++i)
  {
    cardano_sub_tx_builder_t*  builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
    cardano_sub_transaction_t* sub_tx  = nullptr;

    cardano_sub_tx_builder_add_script(builder, script);
    cardano_sub_tx_builder_add_guard_with_redeemer(builder, guard, redeemer);

    reset_allocators_run_count();
    reset_limited_malloc();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

    cardano_set_allocators(malloc, realloc, free);

    // Assert
    built = (result == CARDANO_SUCCESS);

    if (!built)
    {
      EXPECT_EQ(sub_tx, nullptr);
    }

    cardano_sub_transaction_unref(&sub_tx);
    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(built);

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_plutus_data_unref(&redeemer);
  cardano_credential_unref(&guard);
  cardano_script_unref(&script);
  cardano_protocol_parameters_unref(&params);
}

/* SPEND REDEEMERS ***********************************************************/

static const char* ALWAYS_SUCCEEDS_V3_CBOR = "820346450100002499";
static const char* LOW_TX_ID               = "0000000000000000000000000000000000000000000000000000000000000001";
static const char* HIGH_TX_ID              = "ff00000000000000000000000000000000000000000000000000000000000000";
static const char* REFERENCE_TX_ID         = "1111111111111111111111111111111111111111111111111111111111111111";

/**
 * Computes the hash of a plutus data, as an output that is locked by a datum hash carries it.
 * \param data The plutus data.
 * \return A new instance of the hash.
 */
static cardano_blake2b_hash_t*
compute_datum_hash(cardano_plutus_data_t* data)
{
  cardano_cbor_writer_t*  writer = cardano_cbor_writer_new();
  cardano_blake2b_hash_t* hash   = NULL;

  EXPECT_EQ(cardano_plutus_data_to_cbor(data, writer), CARDANO_SUCCESS);

  const size_t size  = cardano_cbor_writer_get_encode_size(writer);
  byte_t*      bytes = (byte_t*)malloc(size);

  EXPECT_EQ(cardano_cbor_writer_encode(writer, bytes, size), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_compute_hash(bytes, size, 32, &hash), CARDANO_SUCCESS);

  free(bytes);
  cardano_cbor_writer_unref(&writer);

  return hash;
}

/**
 * Creates a UTXO locked by a script.
 * \param tx_id The transaction id of the UTXO, in hexadecimal format.
 * \param script The script that locks the UTXO.
 * \param datum The datum whose hash the UTXO carries, or NULL for a UTXO without a datum.
 * \return A new instance of the UTXO.
 */
static cardano_utxo_t*
create_script_utxo(const char* tx_id, cardano_script_t* script, cardano_plutus_data_t* datum)
{
  cardano_credential_t*         credential         = create_script_credential(script);
  cardano_enterprise_address_t* enterprise_address = NULL;
  cardano_blake2b_hash_t*       id                 = create_hash(tx_id);
  cardano_transaction_input_t*  input              = NULL;
  cardano_transaction_output_t* output             = NULL;
  cardano_utxo_t*               utxo               = NULL;

  EXPECT_EQ(cardano_enterprise_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, credential, &enterprise_address), CARDANO_SUCCESS);

  cardano_address_t* address = cardano_enterprise_address_to_address(enterprise_address);

  EXPECT_EQ(cardano_transaction_input_new(id, 0U, &input), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_new(address, 10000000U, &output), CARDANO_SUCCESS);

  if (datum != NULL)
  {
    cardano_blake2b_hash_t* datum_hash   = compute_datum_hash(datum);
    cardano_datum_t*        output_datum = NULL;

    EXPECT_EQ(cardano_datum_new_data_hash(datum_hash, &output_datum), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_transaction_output_set_datum(output, output_datum), CARDANO_SUCCESS);

    cardano_datum_unref(&output_datum);
    cardano_blake2b_hash_unref(&datum_hash);
  }

  EXPECT_EQ(cardano_utxo_new(input, output, &utxo), CARDANO_SUCCESS);

  cardano_transaction_output_unref(&output);
  cardano_transaction_input_unref(&input);
  cardano_blake2b_hash_unref(&id);
  cardano_address_unref(&address);
  cardano_enterprise_address_unref(&enterprise_address);
  cardano_credential_unref(&credential);

  return utxo;
}

/**
 * Creates a UTXO that carries a reference script.
 * \param tx_id The transaction id of the UTXO, in hexadecimal format.
 * \param script The reference script carried by the UTXO.
 * \return A new instance of the UTXO.
 */
static cardano_utxo_t*
create_reference_script_utxo(const char* tx_id, cardano_script_t* script)
{
  cardano_utxo_t*               base   = create_utxo(UTXO_CBOR);
  cardano_transaction_output_t* output = cardano_utxo_get_output(base);
  cardano_blake2b_hash_t*       id     = create_hash(tx_id);
  cardano_transaction_input_t*  input  = NULL;
  cardano_utxo_t*               utxo   = NULL;

  EXPECT_EQ(cardano_transaction_output_set_script_ref(output, script), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_new(id, 0U, &input), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_new(input, output, &utxo), CARDANO_SUCCESS);

  cardano_transaction_input_unref(&input);
  cardano_blake2b_hash_unref(&id);
  cardano_transaction_output_unref(&output);
  cardano_utxo_unref(&base);

  return utxo;
}

/**
 * Finds the redeemer of a sub transaction that carries a given plutus data.
 * \param sub_tx The sub transaction.
 * \param data The plutus data carried by the redeemer.
 * \return A new reference to the redeemer, or NULL when no redeemer carries the plutus data.
 */
static cardano_redeemer_t*
find_sub_tx_redeemer(cardano_sub_transaction_t* sub_tx, cardano_plutus_data_t* data)
{
  cardano_witness_set_t* witness_set = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_witness_set_unref(&witness_set);

  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness_set);
  cardano_redeemer_list_unref(&redeemers);

  for (size_t i = 0U; i < cardano_redeemer_list_get_length(redeemers); ++i)
  {
    cardano_redeemer_t* redeemer = NULL;

    EXPECT_EQ(cardano_redeemer_list_get(redeemers, i, &redeemer), CARDANO_SUCCESS);

    cardano_plutus_data_t* redeemer_data = cardano_redeemer_get_data(redeemer);
    const bool             is_match      = cardano_plutus_data_equals(redeemer_data, data);

    cardano_plutus_data_unref(&redeemer_data);

    if (is_match)
    {
      return redeemer;
    }

    cardano_redeemer_unref(&redeemer);
  }

  return NULL;
}

/**
 * Evaluates the redeemers of a sub transaction with the native evaluator, carried at position 0 of a transaction
 * that holds nothing else.
 * \param params The protocol parameters.
 * \param sub_tx The sub transaction.
 * \param utxos The UTXOs that resolve the inputs and reference inputs of the sub transaction.
 * \return A new instance of the evaluated redeemer list.
 */
static cardano_redeemer_list_t*
evaluate_sub_tx(cardano_protocol_parameters_t* params, cardano_sub_transaction_t* sub_tx, cardano_utxo_list_t* utxos)
{
  cardano_costmdls_t*                cost_models      = cardano_protocol_parameters_get_cost_models(params);
  cardano_protocol_version_t*        version          = cardano_protocol_parameters_get_protocol_version(params);
  cardano_transaction_input_set_t*   inputs           = NULL;
  cardano_transaction_output_list_t* outputs          = NULL;
  cardano_transaction_body_t*        body             = NULL;
  cardano_sub_transaction_set_t*     sub_transactions = NULL;
  cardano_witness_set_t*             witness_set      = NULL;
  cardano_transaction_t*             carrier          = NULL;
  cardano_tx_evaluator_t*            evaluator        = NULL;
  cardano_redeemer_list_t*           evaluated        = NULL;

  EXPECT_EQ(cardano_transaction_input_set_new(&inputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_list_new(&outputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_new(inputs, outputs, 0U, NULL, &body), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_set_new(&sub_transactions), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_set_add(sub_transactions, sub_tx), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_sub_transactions(body, sub_transactions), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_witness_set_new(&witness_set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_new(body, witness_set, NULL, &carrier), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new_native_sub_transaction(&CARDANO_MAINNET_SLOT_CONFIG, cost_models, cardano_protocol_version_get_major(version), 0U, &evaluator), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_evaluate(evaluator, carrier, utxos, &evaluated), CARDANO_SUCCESS);

  cardano_tx_evaluator_unref(&evaluator);
  cardano_transaction_unref(&carrier);
  cardano_witness_set_unref(&witness_set);
  cardano_sub_transaction_set_unref(&sub_transactions);
  cardano_transaction_body_unref(&body);
  cardano_transaction_output_list_unref(&outputs);
  cardano_transaction_input_set_unref(&inputs);
  cardano_protocol_version_unref(&version);
  cardano_costmdls_unref(&cost_models);

  return evaluated;
}

/**
 * Expects the execution units of a redeemer to be the ones of the evaluated redeemer with the same tag and index.
 * \param redeemer The redeemer.
 * \param evaluated The evaluated redeemers.
 */
static void
expect_evaluated_ex_units(cardano_redeemer_t* redeemer, cardano_redeemer_list_t* evaluated)
{
  cardano_ex_units_t* units = cardano_redeemer_get_ex_units(redeemer);
  bool                found = false;

  EXPECT_NE(cardano_ex_units_get_cpu_steps(units), 0U);
  EXPECT_NE(cardano_ex_units_get_memory(units), 0U);

  for (size_t i = 0U; i < cardano_redeemer_list_get_length(evaluated); ++i)
  {
    cardano_redeemer_t* expected = NULL;

    EXPECT_EQ(cardano_redeemer_list_get(evaluated, i, &expected), CARDANO_SUCCESS);

    if ((cardano_redeemer_get_tag(expected) == cardano_redeemer_get_tag(redeemer)) && (cardano_redeemer_get_index(expected) == cardano_redeemer_get_index(redeemer)))
    {
      cardano_ex_units_t* expected_units = cardano_redeemer_get_ex_units(expected);

      EXPECT_EQ(cardano_ex_units_get_cpu_steps(units), cardano_ex_units_get_cpu_steps(expected_units));
      EXPECT_EQ(cardano_ex_units_get_memory(units), cardano_ex_units_get_memory(expected_units));

      cardano_ex_units_unref(&expected_units);

      found = true;
    }

    cardano_redeemer_unref(&expected);
  }

  EXPECT_TRUE(found);

  cardano_ex_units_unref(&units);
}

TEST(cardano_sub_tx_builder_add_input, spendsPlutusV4ScriptInputsAtTheirPositionInTheSortedInputs)
{
  // Arrange
  cardano_protocol_parameters_t* params        = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder       = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script        = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_utxo_t*                high_utxo     = create_script_utxo(HIGH_TX_ID, script, nullptr);
  cardano_utxo_t*                key_utxo      = create_utxo(UTXO_CBOR);
  cardano_utxo_t*                low_utxo      = create_script_utxo(LOW_TX_ID, script, nullptr);
  cardano_plutus_data_t*         high_redeemer = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_plutus_data_t*         low_redeemer  = create_plutus_data(OTHER_PLUTUS_DATA_CBOR);
  cardano_utxo_list_t*           utxos         = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(utxos, high_utxo), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(utxos, key_utxo), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(utxos, low_utxo), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_input(builder, high_utxo, high_redeemer, nullptr);
  cardano_sub_tx_builder_add_input(builder, key_utxo, nullptr, nullptr);
  cardano_sub_tx_builder_add_input(builder, low_utxo, low_redeemer, nullptr);

  cardano_sub_transaction_t* sub_tx    = build_sub_transaction(builder);
  cardano_redeemer_t*        high      = find_sub_tx_redeemer(sub_tx, high_redeemer);
  cardano_redeemer_t*        low       = find_sub_tx_redeemer(sub_tx, low_redeemer);
  cardano_redeemer_list_t*   evaluated = evaluate_sub_tx(params, sub_tx, utxos);
  cardano_blake2b_hash_t*    hash      = cardano_sub_transaction_body_get_script_data_hash(get_body(sub_tx));

  // Assert
  ASSERT_NE(high, nullptr);
  ASSERT_NE(low, nullptr);
  EXPECT_EQ(cardano_redeemer_get_tag(high), CARDANO_REDEEMER_TAG_SPEND);
  EXPECT_EQ(cardano_redeemer_get_tag(low), CARDANO_REDEEMER_TAG_SPEND);
  EXPECT_EQ(cardano_redeemer_get_index(low), 0U);
  EXPECT_EQ(cardano_redeemer_get_index(high), 2U);
  EXPECT_EQ(cardano_redeemer_list_get_length(evaluated), 2U);
  expect_evaluated_ex_units(high, evaluated);
  expect_evaluated_ex_units(low, evaluated);
  EXPECT_NE(hash, nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_redeemer_list_unref(&evaluated);
  cardano_redeemer_unref(&low);
  cardano_redeemer_unref(&high);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_data_unref(&low_redeemer);
  cardano_plutus_data_unref(&high_redeemer);
  cardano_utxo_unref(&low_utxo);
  cardano_utxo_unref(&key_utxo);
  cardano_utxo_unref(&high_utxo);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_input, carriesTheDatumOfAnInputLockedByADatumHash)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder  = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script   = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_plutus_data_t*         datum    = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_plutus_data_t*         redeemer = create_plutus_data(OTHER_PLUTUS_DATA_CBOR);
  cardano_utxo_t*                utxo     = create_script_utxo(LOW_TX_ID, script, datum);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_input(builder, utxo, redeemer, datum);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  ASSERT_NE(sub_tx, nullptr);

  cardano_witness_set_t*     witness_set = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_plutus_data_set_t* datums      = cardano_witness_set_get_plutus_data(witness_set);
  cardano_plutus_data_t*     carried     = NULL;
  cardano_redeemer_t*        spend       = find_sub_tx_redeemer(sub_tx, redeemer);
  cardano_ex_units_t*        units       = cardano_redeemer_get_ex_units(spend);

  // Assert
  EXPECT_EQ(cardano_plutus_data_set_get_length(datums), 1U);
  EXPECT_EQ(cardano_plutus_data_set_get(datums, 0U, &carried), CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_plutus_data_equals(carried, datum));
  EXPECT_EQ(cardano_redeemer_get_index(spend), 0U);
  EXPECT_NE(cardano_ex_units_get_cpu_steps(units), 0U);
  EXPECT_NE(cardano_ex_units_get_memory(units), 0U);

  // Cleanup
  cardano_ex_units_unref(&units);
  cardano_redeemer_unref(&spend);
  cardano_plutus_data_unref(&carried);
  cardano_plutus_data_set_unref(&datums);
  cardano_witness_set_unref(&witness_set);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_utxo_unref(&utxo);
  cardano_plutus_data_unref(&redeemer);
  cardano_plutus_data_unref(&datum);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_input, reportsAnInputLockedByAPlutusV3ScriptWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V3_CBOR);
  cardano_utxo_t*                utxo           = create_script_utxo(LOW_TX_ID, script, nullptr);
  cardano_utxo_t*                reference_utxo = create_reference_script_utxo(REFERENCE_TX_ID, script);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_sub_transaction_t*     sub_tx         = nullptr;

  // Act
  cardano_sub_tx_builder_add_reference_input(builder, reference_utxo);
  cardano_sub_tx_builder_add_input(builder, utxo, redeemer, nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE);
  EXPECT_STRNE(cardano_sub_tx_builder_get_last_error(builder), "");
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_plutus_data_unref(&redeemer);
  cardano_utxo_unref(&reference_utxo);
  cardano_utxo_unref(&utxo);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_input, spendsAnInputWithoutARedeemerWithoutScriptData)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_utxo_t*                utxo    = create_utxo(UTXO_CBOR);

  // Act
  cardano_sub_tx_builder_add_input(builder, utxo, nullptr, nullptr);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  ASSERT_NE(sub_tx, nullptr);

  cardano_witness_set_t*   witness_set = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_redeemer_list_t* redeemers   = cardano_witness_set_get_redeemers(witness_set);
  cardano_blake2b_hash_t*  hash        = cardano_sub_transaction_body_get_script_data_hash(get_body(sub_tx));

  // Assert
  EXPECT_EQ(cardano_redeemer_list_get_length(redeemers), 0U);
  EXPECT_EQ(hash, nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_redeemer_list_unref(&redeemers);
  cardano_witness_set_unref(&witness_set);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_utxo_unref(&utxo);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_input, returnsErrorIfMemoryAllocationFailsWhileBuildingWithSpendRedeemers)
{
  // Arrange
  cardano_protocol_parameters_t* params        = init_dijkstra_protocol_parameters();
  cardano_script_t*              script        = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_utxo_t*                high_utxo     = create_script_utxo(HIGH_TX_ID, script, nullptr);
  cardano_utxo_t*                low_utxo      = create_script_utxo(LOW_TX_ID, script, nullptr);
  cardano_plutus_data_t*         high_redeemer = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_plutus_data_t*         low_redeemer  = create_plutus_data(OTHER_PLUTUS_DATA_CBOR);
  bool                           built         = false;

  for (int i = 0; (i < 100000) && !built; ++i)
  {
    cardano_sub_tx_builder_t*  builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
    cardano_sub_transaction_t* sub_tx  = nullptr;

    cardano_sub_tx_builder_add_script(builder, script);
    cardano_sub_tx_builder_add_input(builder, high_utxo, high_redeemer, nullptr);
    cardano_sub_tx_builder_add_input(builder, low_utxo, low_redeemer, nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

    cardano_set_allocators(malloc, realloc, free);

    // Assert
    built = (result == CARDANO_SUCCESS);

    if (!built)
    {
      EXPECT_EQ(sub_tx, nullptr);
    }

    cardano_sub_transaction_unref(&sub_tx);
    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(built);

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_plutus_data_unref(&low_redeemer);
  cardano_plutus_data_unref(&high_redeemer);
  cardano_utxo_unref(&low_utxo);
  cardano_utxo_unref(&high_utxo);
  cardano_script_unref(&script);
  cardano_protocol_parameters_unref(&params);
}

/* SCRIPT PURPOSE REDEEMERS **************************************************/

static const char* OTHER_ALWAYS_SUCCEEDS_V4_CBOR = "820446450101002499";

/**
 * Creates a reward address whose stake credential is the hash of a script.
 * \param script The script.
 * \return A new instance of the reward address.
 */
static cardano_reward_address_t*
create_script_reward_address(cardano_script_t* script)
{
  cardano_credential_t*     credential     = create_script_credential(script);
  cardano_reward_address_t* reward_address = NULL;

  EXPECT_EQ(cardano_reward_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, credential, &reward_address), CARDANO_SUCCESS);

  cardano_credential_unref(&credential);

  return reward_address;
}

/**
 * Creates a DRep voter whose credential is the hash of a script.
 * \param script The script.
 * \return A new instance of the voter.
 */
static cardano_voter_t*
create_script_voter(cardano_script_t* script)
{
  cardano_credential_t* credential = create_script_credential(script);
  cardano_voter_t*      voter      = NULL;

  EXPECT_EQ(cardano_voter_new(CARDANO_VOTER_TYPE_DREP_SCRIPT_HASH, credential, &voter), CARDANO_SUCCESS);

  cardano_credential_unref(&credential);

  return voter;
}

/**
 * Creates the id of a native asset minted under a script policy.
 * \param script The script that is the minting policy.
 * \return A new instance of the asset id.
 */
static cardano_asset_id_t*
create_script_asset_id(cardano_script_t* script)
{
  cardano_blake2b_hash_t* policy_id  = cardano_script_get_hash(script);
  cardano_asset_name_t*   asset_name = NULL;
  cardano_asset_id_t*     asset_id   = NULL;

  EXPECT_EQ(cardano_asset_name_from_hex(ASSET_NAME_HEX, strlen(ASSET_NAME_HEX), &asset_name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_id_new(policy_id, asset_name, &asset_id), CARDANO_SUCCESS);

  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);

  return asset_id;
}

/**
 * Expects a sub transaction to carry a redeemer with a given payload, tag and index and non zero execution units.
 * \param sub_tx The sub transaction.
 * \param data The plutus data carried by the redeemer.
 * \param tag The expected tag.
 * \param index The expected index.
 */
static void
expect_sub_tx_redeemer(cardano_sub_transaction_t* sub_tx, cardano_plutus_data_t* data, const cardano_redeemer_tag_t tag, const uint64_t index)
{
  cardano_redeemer_t* redeemer = find_sub_tx_redeemer(sub_tx, data);

  ASSERT_NE(redeemer, nullptr);

  cardano_ex_units_t* units = cardano_redeemer_get_ex_units(redeemer);

  EXPECT_EQ(cardano_redeemer_get_tag(redeemer), tag);
  EXPECT_EQ(cardano_redeemer_get_index(redeemer), index);
  EXPECT_NE(cardano_ex_units_get_cpu_steps(units), 0U);
  EXPECT_NE(cardano_ex_units_get_memory(units), 0U);

  cardano_ex_units_unref(&units);
  cardano_redeemer_unref(&redeemer);
}

/**
 * Gets the number of redeemers of a sub transaction.
 * \param sub_tx The sub transaction.
 * \return The number of redeemers.
 */
static size_t
get_sub_tx_redeemer_count(cardano_sub_transaction_t* sub_tx)
{
  cardano_witness_set_t*   witness_set = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_redeemer_list_t* redeemers   = cardano_witness_set_get_redeemers(witness_set);
  const size_t             count       = cardano_redeemer_list_get_length(redeemers);

  cardano_redeemer_list_unref(&redeemers);
  cardano_witness_set_unref(&witness_set);

  return count;
}

TEST(cardano_sub_tx_builder_mint_token, mintsWithAPlutusV4PolicyAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder    = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script     = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_blake2b_hash_t*        policy_id  = cardano_script_get_hash(script);
  cardano_asset_name_t*          asset_name = NULL;
  cardano_plutus_data_t*         redeemer   = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_asset_name_from_hex(ASSET_NAME_HEX, strlen(ASSET_NAME_HEX), &asset_name), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_mint_token(builder, policy_id, asset_name, 10, redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);
  cardano_blake2b_hash_t*    hash   = cardano_sub_transaction_body_get_script_data_hash(get_body(sub_tx));

  // Assert
  EXPECT_EQ(get_sub_tx_redeemer_count(sub_tx), 1U);
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_MINT, 0U);
  EXPECT_NE(hash, nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_mint_token_ex, mintsWithAPlutusV4PolicyAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params    = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder   = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script    = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_blake2b_hash_t*        policy_id = cardano_script_get_hash(script);
  cardano_plutus_data_t*         redeemer  = create_plutus_data(PLUTUS_DATA_CBOR);
  char                           hex[57]   = { 0 };

  EXPECT_EQ(cardano_blake2b_hash_to_hex(policy_id, hex, sizeof(hex)), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_mint_token_ex(builder, hex, strlen(hex), ASSET_NAME_HEX, strlen(ASSET_NAME_HEX), -5, redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_MINT, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_mint_token_with_id, mintsWithAPlutusV4PolicyAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder  = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script   = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_asset_id_t*            asset_id = create_script_asset_id(script);
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_mint_token_with_id(builder, asset_id, 7, redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_MINT, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_asset_id_unref(&asset_id);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_mint_token_with_id_ex, mintsWithAPlutusV4PolicyAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder  = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script   = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_asset_id_t*            asset_id = create_script_asset_id(script);
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);
  const char*                    hex      = cardano_asset_id_get_hex(asset_id);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_mint_token_with_id_ex(builder, hex, strlen(hex), 7, redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_MINT, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_asset_id_unref(&asset_id);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_mint_token, indexesPlutusV4PoliciesByPolicyIdWhateverTheOrderTheyWereAdded)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script          = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_script_t*              other_script    = create_script(OTHER_ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_asset_id_t*            asset_id        = create_script_asset_id(script);
  cardano_asset_id_t*            other_asset_id  = create_script_asset_id(other_script);
  cardano_blake2b_hash_t*        policy_id       = cardano_script_get_hash(script);
  cardano_blake2b_hash_t*        other_policy_id = cardano_script_get_hash(other_script);
  cardano_plutus_data_t*         redeemer        = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_plutus_data_t*         other_redeemer  = create_plutus_data(OTHER_PLUTUS_DATA_CBOR);
  const bool                     script_is_first = cardano_blake2b_hash_compare(policy_id, other_policy_id) < 0;
  cardano_asset_id_t*            low_asset_id    = script_is_first ? asset_id : other_asset_id;
  cardano_asset_id_t*            high_asset_id   = script_is_first ? other_asset_id : asset_id;
  cardano_plutus_data_t*         low_redeemer    = script_is_first ? redeemer : other_redeemer;
  cardano_plutus_data_t*         high_redeemer   = script_is_first ? other_redeemer : redeemer;

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_script(builder, other_script);
  cardano_sub_tx_builder_mint_token_with_id(builder, high_asset_id, 1, high_redeemer);
  cardano_sub_tx_builder_mint_token_with_id(builder, low_asset_id, 1, low_redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  EXPECT_EQ(get_sub_tx_redeemer_count(sub_tx), 2U);
  expect_sub_tx_redeemer(sub_tx, low_redeemer, CARDANO_REDEEMER_TAG_MINT, 0U);
  expect_sub_tx_redeemer(sub_tx, high_redeemer, CARDANO_REDEEMER_TAG_MINT, 1U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&other_redeemer);
  cardano_plutus_data_unref(&redeemer);
  cardano_blake2b_hash_unref(&other_policy_id);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_id_unref(&other_asset_id);
  cardano_asset_id_unref(&asset_id);
  cardano_script_unref(&other_script);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_mint_token, reportsAPlutusV3PolicyWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V3_CBOR);
  cardano_asset_id_t*            asset_id       = create_script_asset_id(script);
  cardano_utxo_t*                reference_utxo = create_reference_script_utxo(REFERENCE_TX_ID, script);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_sub_transaction_t*     sub_tx         = nullptr;

  // Act
  cardano_sub_tx_builder_add_reference_input(builder, reference_utxo);
  cardano_sub_tx_builder_mint_token_with_id(builder, asset_id, 1, redeemer);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_plutus_data_unref(&redeemer);
  cardano_utxo_unref(&reference_utxo);
  cardano_asset_id_unref(&asset_id);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_mint_token, mintsWithoutARedeemerForANativeScriptPolicy)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder  = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script   = create_script(NATIVE_SCRIPT_CBOR);
  cardano_asset_id_t*            asset_id = create_script_asset_id(script);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_mint_token_with_id(builder, asset_id, 1, nullptr);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);
  cardano_blake2b_hash_t*    hash   = cardano_sub_transaction_body_get_script_data_hash(get_body(sub_tx));

  // Assert
  EXPECT_EQ(get_sub_tx_redeemer_count(sub_tx), 0U);
  EXPECT_EQ(hash, nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_asset_id_unref(&asset_id);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_mint_token, returnsErrorIfMemoryAllocationFailsWhileBuildingWithMintRedeemers)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_dijkstra_protocol_parameters();
  cardano_script_t*              script   = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_asset_id_t*            asset_id = create_script_asset_id(script);
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);
  bool                           built    = false;

  for (int i = 0; (i < 100000) && !built; ++i)
  {
    cardano_sub_tx_builder_t*  builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
    cardano_sub_transaction_t* sub_tx  = nullptr;

    cardano_sub_tx_builder_add_script(builder, script);
    cardano_sub_tx_builder_mint_token_with_id(builder, asset_id, 1, redeemer);

    reset_allocators_run_count();
    reset_limited_malloc();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

    cardano_set_allocators(malloc, realloc, free);

    // Assert
    built = (result == CARDANO_SUCCESS);

    if (!built)
    {
      EXPECT_EQ(sub_tx, nullptr);
    }

    cardano_sub_transaction_unref(&sub_tx);
    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(built);

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_plutus_data_unref(&redeemer);
  cardano_asset_id_unref(&asset_id);
  cardano_script_unref(&script);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_withdraw_rewards, withdrawsFromAPlutusV4ScriptAccountAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_reward_address_t*      reward_address = create_script_reward_address(script);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_withdraw_rewards(builder, reward_address, 1000000, redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);
  cardano_blake2b_hash_t*    hash   = cardano_sub_transaction_body_get_script_data_hash(get_body(sub_tx));

  // Assert
  EXPECT_EQ(get_sub_tx_redeemer_count(sub_tx), 1U);
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_REWARD, 0U);
  EXPECT_NE(hash, nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_reward_address_unref(&reward_address);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_withdraw_rewards_ex, withdrawsFromAPlutusV4ScriptAccountAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_reward_address_t*      reward_address = create_script_reward_address(script);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  const char*                    bech32         = cardano_reward_address_get_string(reward_address);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_withdraw_rewards_ex(builder, bech32, strlen(bech32), 1000000, redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_REWARD, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_reward_address_unref(&reward_address);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_withdraw_rewards, indexesScriptWithdrawalsInLedgerOrderBeforeAKeyWithdrawal)
{
  // Arrange
  cardano_protocol_parameters_t* params               = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder              = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script               = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_script_t*              other_script         = create_script(OTHER_ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_reward_address_t*      key_address          = create_reward_address(REWARD_ADDRESS);
  cardano_reward_address_t*      script_address       = create_script_reward_address(script);
  cardano_reward_address_t*      other_script_address = create_script_reward_address(other_script);
  cardano_blake2b_hash_t*        script_hash          = cardano_script_get_hash(script);
  cardano_blake2b_hash_t*        other_script_hash    = cardano_script_get_hash(other_script);
  cardano_plutus_data_t*         redeemer             = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_plutus_data_t*         other_redeemer       = create_plutus_data(OTHER_PLUTUS_DATA_CBOR);
  const bool                     script_is_first      = cardano_blake2b_hash_compare(script_hash, other_script_hash) < 0;
  cardano_plutus_data_t*         low_redeemer         = script_is_first ? redeemer : other_redeemer;
  cardano_plutus_data_t*         high_redeemer        = script_is_first ? other_redeemer : redeemer;

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_script(builder, other_script);
  cardano_sub_tx_builder_withdraw_rewards(builder, key_address, 1000000, nullptr);
  cardano_sub_tx_builder_withdraw_rewards(builder, other_script_address, 1000000, other_redeemer);
  cardano_sub_tx_builder_withdraw_rewards(builder, script_address, 1000000, redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  EXPECT_EQ(get_sub_tx_redeemer_count(sub_tx), 2U);
  expect_sub_tx_redeemer(sub_tx, low_redeemer, CARDANO_REDEEMER_TAG_REWARD, 0U);
  expect_sub_tx_redeemer(sub_tx, high_redeemer, CARDANO_REDEEMER_TAG_REWARD, 1U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&other_redeemer);
  cardano_plutus_data_unref(&redeemer);
  cardano_blake2b_hash_unref(&other_script_hash);
  cardano_blake2b_hash_unref(&script_hash);
  cardano_reward_address_unref(&other_script_address);
  cardano_reward_address_unref(&script_address);
  cardano_reward_address_unref(&key_address);
  cardano_script_unref(&other_script);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_withdraw_rewards, reportsAPlutusV3ScriptAccountWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V3_CBOR);
  cardano_reward_address_t*      reward_address = create_script_reward_address(script);
  cardano_utxo_t*                reference_utxo = create_reference_script_utxo(REFERENCE_TX_ID, script);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_sub_transaction_t*     sub_tx         = nullptr;

  // Act
  cardano_sub_tx_builder_add_reference_input(builder, reference_utxo);
  cardano_sub_tx_builder_withdraw_rewards(builder, reward_address, 1000000, redeemer);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_plutus_data_unref(&redeemer);
  cardano_utxo_unref(&reference_utxo);
  cardano_reward_address_unref(&reward_address);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_withdraw_rewards, returnsErrorIfMemoryAllocationFailsWhileBuildingWithRewardRedeemers)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_reward_address_t*      reward_address = create_script_reward_address(script);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  bool                           built          = false;

  for (int i = 0; (i < 100000) && !built; ++i)
  {
    cardano_sub_tx_builder_t*  builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
    cardano_sub_transaction_t* sub_tx  = nullptr;

    cardano_sub_tx_builder_add_script(builder, script);
    cardano_sub_tx_builder_withdraw_rewards(builder, reward_address, 1000000, redeemer);

    reset_allocators_run_count();
    reset_limited_malloc();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

    cardano_set_allocators(malloc, realloc, free);

    // Assert
    built = (result == CARDANO_SUCCESS);

    if (!built)
    {
      EXPECT_EQ(sub_tx, nullptr);
    }

    cardano_sub_transaction_unref(&sub_tx);
    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(built);

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_plutus_data_unref(&redeemer);
  cardano_reward_address_unref(&reward_address);
  cardano_script_unref(&script);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_register_reward_address, registersAPlutusV4ScriptAccountAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_reward_address_t*      reward_address = create_script_reward_address(script);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_register_reward_address(builder, reward_address, redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  EXPECT_EQ(get_sub_tx_redeemer_count(sub_tx), 1U);
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_CERTIFYING, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_reward_address_unref(&reward_address);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_register_reward_address_ex, registersAPlutusV4ScriptAccountAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_reward_address_t*      reward_address = create_script_reward_address(script);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  const char*                    bech32         = cardano_reward_address_get_string(reward_address);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_register_reward_address_ex(builder, bech32, strlen(bech32), redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_CERTIFYING, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_reward_address_unref(&reward_address);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_deregister_reward_address, deregistersAPlutusV4ScriptAccountAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_reward_address_t*      reward_address = create_script_reward_address(script);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_deregister_reward_address(builder, reward_address, redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_CERTIFYING, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_reward_address_unref(&reward_address);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_deregister_reward_address_ex, deregistersAPlutusV4ScriptAccountAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_reward_address_t*      reward_address = create_script_reward_address(script);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  const char*                    bech32         = cardano_reward_address_get_string(reward_address);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_deregister_reward_address_ex(builder, bech32, strlen(bech32), redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_CERTIFYING, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_reward_address_unref(&reward_address);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_delegate_stake, delegatesAPlutusV4ScriptAccountAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_reward_address_t*      reward_address = create_script_reward_address(script);
  cardano_blake2b_hash_t*        pool_id        = create_hash(POOL_ID_HEX);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_delegate_stake(builder, reward_address, pool_id, redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_CERTIFYING, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_blake2b_hash_unref(&pool_id);
  cardano_reward_address_unref(&reward_address);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_delegate_stake_ex, delegatesAPlutusV4ScriptAccountAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_reward_address_t*      reward_address = create_script_reward_address(script);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  const char*                    bech32         = cardano_reward_address_get_string(reward_address);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_delegate_stake_ex(builder, bech32, strlen(bech32), POOL_ID, strlen(POOL_ID), redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_CERTIFYING, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_reward_address_unref(&reward_address);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_delegate_voting_power, delegatesAPlutusV4ScriptAccountAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_reward_address_t*      reward_address = create_script_reward_address(script);
  cardano_drep_t*                drep           = create_drep(DREP_ID);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_delegate_voting_power(builder, reward_address, drep, redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_CERTIFYING, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_drep_unref(&drep);
  cardano_reward_address_unref(&reward_address);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_delegate_voting_power_ex, delegatesAPlutusV4ScriptAccountAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_reward_address_t*      reward_address = create_script_reward_address(script);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  const char*                    bech32         = cardano_reward_address_get_string(reward_address);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_delegate_voting_power_ex(builder, bech32, strlen(bech32), DREP_ID, strlen(DREP_ID), redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_CERTIFYING, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_reward_address_unref(&reward_address);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_register_drep, registersAPlutusV4ScriptDRepAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder    = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script     = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_credential_t*          credential = create_script_credential(script);
  cardano_drep_t*                drep       = NULL;
  cardano_anchor_t*              anchor     = create_anchor();
  cardano_plutus_data_t*         redeemer   = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_drep_new(CARDANO_DREP_TYPE_SCRIPT_HASH, credential, &drep), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_register_drep(builder, drep, anchor, redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_CERTIFYING, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_anchor_unref(&anchor);
  cardano_drep_unref(&drep);
  cardano_credential_unref(&credential);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_update_drep, updatesAPlutusV4ScriptDRepAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder    = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script     = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_credential_t*          credential = create_script_credential(script);
  cardano_drep_t*                drep       = NULL;
  cardano_anchor_t*              anchor     = create_anchor();
  cardano_plutus_data_t*         redeemer   = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_drep_new(CARDANO_DREP_TYPE_SCRIPT_HASH, credential, &drep), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_update_drep(builder, drep, anchor, redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_CERTIFYING, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_anchor_unref(&anchor);
  cardano_drep_unref(&drep);
  cardano_credential_unref(&credential);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_deregister_drep, deregistersAPlutusV4ScriptDRepAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder    = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script     = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_credential_t*          credential = create_script_credential(script);
  cardano_drep_t*                drep       = NULL;
  cardano_plutus_data_t*         redeemer   = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_drep_new(CARDANO_DREP_TYPE_SCRIPT_HASH, credential, &drep), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_deregister_drep(builder, drep, redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_CERTIFYING, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_drep_unref(&drep);
  cardano_credential_unref(&credential);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_register_drep_ex, registersAPlutusV4ScriptDRepAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params       = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder      = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script       = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_credential_t*          credential   = create_script_credential(script);
  cardano_drep_t*                drep         = NULL;
  cardano_plutus_data_t*         redeemer     = create_plutus_data(PLUTUS_DATA_CBOR);
  char                           drep_id[128] = { 0 };

  EXPECT_EQ(cardano_drep_new(CARDANO_DREP_TYPE_SCRIPT_HASH, credential, &drep), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_drep_to_string(drep, drep_id, sizeof(drep_id)), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_register_drep_ex(builder, drep_id, strlen(drep_id), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_CERTIFYING, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_drep_unref(&drep);
  cardano_credential_unref(&credential);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_update_drep_ex, updatesAPlutusV4ScriptDRepAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params       = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder      = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script       = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_credential_t*          credential   = create_script_credential(script);
  cardano_drep_t*                drep         = NULL;
  cardano_plutus_data_t*         redeemer     = create_plutus_data(PLUTUS_DATA_CBOR);
  char                           drep_id[128] = { 0 };

  EXPECT_EQ(cardano_drep_new(CARDANO_DREP_TYPE_SCRIPT_HASH, credential, &drep), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_drep_to_string(drep, drep_id, sizeof(drep_id)), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_update_drep_ex(builder, drep_id, strlen(drep_id), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_CERTIFYING, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_drep_unref(&drep);
  cardano_credential_unref(&credential);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_deregister_drep_ex, deregistersAPlutusV4ScriptDRepAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params       = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder      = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script       = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_credential_t*          credential   = create_script_credential(script);
  cardano_drep_t*                drep         = NULL;
  cardano_plutus_data_t*         redeemer     = create_plutus_data(PLUTUS_DATA_CBOR);
  char                           drep_id[128] = { 0 };

  EXPECT_EQ(cardano_drep_new(CARDANO_DREP_TYPE_SCRIPT_HASH, credential, &drep), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_drep_to_string(drep, drep_id, sizeof(drep_id)), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_deregister_drep_ex(builder, drep_id, strlen(drep_id), redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_CERTIFYING, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_drep_unref(&drep);
  cardano_credential_unref(&credential);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_certificate, indexesCertificateRedeemersByTheirPositionInTheSubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_script_t*              other_script   = create_script(OTHER_ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_reward_address_t*      key_address    = create_reward_address(REWARD_ADDRESS);
  cardano_reward_address_t*      address        = create_script_reward_address(script);
  cardano_reward_address_t*      other_address  = create_script_reward_address(other_script);
  cardano_certificate_t*         certificate    = create_certificate(CERTIFICATE_CBOR);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_plutus_data_t*         other_redeemer = create_plutus_data(OTHER_PLUTUS_DATA_CBOR);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_script(builder, other_script);
  cardano_sub_tx_builder_register_reward_address(builder, key_address, nullptr);
  cardano_sub_tx_builder_add_certificate(builder, certificate, nullptr);
  cardano_sub_tx_builder_register_reward_address(builder, other_address, other_redeemer);
  cardano_sub_tx_builder_register_reward_address(builder, address, redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  EXPECT_EQ(get_sub_tx_redeemer_count(sub_tx), 2U);
  expect_sub_tx_redeemer(sub_tx, other_redeemer, CARDANO_REDEEMER_TAG_CERTIFYING, 2U);
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_CERTIFYING, 3U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&other_redeemer);
  cardano_plutus_data_unref(&redeemer);
  cardano_certificate_unref(&certificate);
  cardano_reward_address_unref(&other_address);
  cardano_reward_address_unref(&address);
  cardano_reward_address_unref(&key_address);
  cardano_script_unref(&other_script);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_certificate, evaluatesTheRedeemerOfACertificateOfAPlutusV4Script)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_credential_t*          credential     = create_script_credential(script);
  cardano_unregistration_cert_t* unregistration = NULL;
  cardano_certificate_t*         certificate    = NULL;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_unregistration_cert_new(credential, KEY_DEPOSIT, &unregistration), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_certificate_new_unregistration(unregistration, &certificate), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_certificate(builder, certificate, redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_CERTIFYING, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_certificate_unref(&certificate);
  cardano_unregistration_cert_unref(&unregistration);
  cardano_credential_unref(&credential);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_register_reward_address, reportsAPlutusV3ScriptAccountWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V3_CBOR);
  cardano_reward_address_t*      reward_address = create_script_reward_address(script);
  cardano_utxo_t*                reference_utxo = create_reference_script_utxo(REFERENCE_TX_ID, script);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_sub_transaction_t*     sub_tx         = nullptr;

  // Act
  cardano_sub_tx_builder_add_reference_input(builder, reference_utxo);
  cardano_sub_tx_builder_register_reward_address(builder, reward_address, redeemer);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_plutus_data_unref(&redeemer);
  cardano_utxo_unref(&reference_utxo);
  cardano_reward_address_unref(&reward_address);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_register_reward_address, returnsErrorIfMemoryAllocationFailsWhileBuildingWithCertificateRedeemers)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_reward_address_t*      reward_address = create_script_reward_address(script);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  bool                           built          = false;

  for (int i = 0; (i < 100000) && !built; ++i)
  {
    cardano_sub_tx_builder_t*  builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
    cardano_sub_transaction_t* sub_tx  = nullptr;

    cardano_sub_tx_builder_add_script(builder, script);
    cardano_sub_tx_builder_register_reward_address(builder, reward_address, redeemer);

    reset_allocators_run_count();
    reset_limited_malloc();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

    cardano_set_allocators(malloc, realloc, free);

    // Assert
    built = (result == CARDANO_SUCCESS);

    if (!built)
    {
      EXPECT_EQ(sub_tx, nullptr);
    }

    cardano_sub_transaction_unref(&sub_tx);
    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(built);

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_plutus_data_unref(&redeemer);
  cardano_reward_address_unref(&reward_address);
  cardano_script_unref(&script);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_vote, votesAsAPlutusV4ScriptVoterAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t*  params    = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*       builder   = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*               script    = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_voter_t*                voter     = create_script_voter(script);
  cardano_governance_action_id_t* action_id = create_governance_action_id();
  cardano_voting_procedure_t*     vote      = create_voting_procedure(VOTING_PROCEDURE_CBOR);
  cardano_plutus_data_t*          redeemer  = create_plutus_data(PLUTUS_DATA_CBOR);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_vote(builder, voter, action_id, vote, redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);
  cardano_blake2b_hash_t*    hash   = cardano_sub_transaction_body_get_script_data_hash(get_body(sub_tx));

  // Assert
  EXPECT_EQ(get_sub_tx_redeemer_count(sub_tx), 1U);
  expect_sub_tx_redeemer(sub_tx, redeemer, CARDANO_REDEEMER_TAG_VOTING, 0U);
  EXPECT_NE(hash, nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_voting_procedure_unref(&vote);
  cardano_governance_action_id_unref(&action_id);
  cardano_voter_unref(&voter);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_vote, indexesPlutusV4ScriptVotersByVoterOrder)
{
  // Arrange
  cardano_protocol_parameters_t*  params          = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*       builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*               script          = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_script_t*               other_script    = create_script(OTHER_ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_voter_t*                voter           = create_script_voter(script);
  cardano_voter_t*                other_voter     = create_script_voter(other_script);
  cardano_blake2b_hash_t*         script_hash     = cardano_script_get_hash(script);
  cardano_blake2b_hash_t*         other_hash      = cardano_script_get_hash(other_script);
  cardano_governance_action_id_t* action_id       = create_governance_action_id();
  cardano_voting_procedure_t*     vote            = create_voting_procedure(VOTING_PROCEDURE_CBOR);
  cardano_plutus_data_t*          redeemer        = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_plutus_data_t*          other_redeemer  = create_plutus_data(OTHER_PLUTUS_DATA_CBOR);
  const bool                      script_is_first = cardano_blake2b_hash_compare(script_hash, other_hash) < 0;
  cardano_voter_t*                low_voter       = script_is_first ? voter : other_voter;
  cardano_voter_t*                high_voter      = script_is_first ? other_voter : voter;
  cardano_plutus_data_t*          low_redeemer    = script_is_first ? redeemer : other_redeemer;
  cardano_plutus_data_t*          high_redeemer   = script_is_first ? other_redeemer : redeemer;

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_script(builder, other_script);
  cardano_sub_tx_builder_vote(builder, high_voter, action_id, vote, high_redeemer);
  cardano_sub_tx_builder_vote(builder, low_voter, action_id, vote, low_redeemer);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  EXPECT_EQ(get_sub_tx_redeemer_count(sub_tx), 2U);
  expect_sub_tx_redeemer(sub_tx, low_redeemer, CARDANO_REDEEMER_TAG_VOTING, 0U);
  expect_sub_tx_redeemer(sub_tx, high_redeemer, CARDANO_REDEEMER_TAG_VOTING, 1U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&other_redeemer);
  cardano_plutus_data_unref(&redeemer);
  cardano_voting_procedure_unref(&vote);
  cardano_governance_action_id_unref(&action_id);
  cardano_blake2b_hash_unref(&other_hash);
  cardano_blake2b_hash_unref(&script_hash);
  cardano_voter_unref(&other_voter);
  cardano_voter_unref(&voter);
  cardano_script_unref(&other_script);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_vote, reportsAPlutusV3ScriptVoterWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*       builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*               script         = create_script(ALWAYS_SUCCEEDS_V3_CBOR);
  cardano_voter_t*                voter          = create_script_voter(script);
  cardano_governance_action_id_t* action_id      = create_governance_action_id();
  cardano_voting_procedure_t*     vote           = create_voting_procedure(VOTING_PROCEDURE_CBOR);
  cardano_utxo_t*                 reference_utxo = create_reference_script_utxo(REFERENCE_TX_ID, script);
  cardano_plutus_data_t*          redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_sub_transaction_t*      sub_tx         = nullptr;

  // Act
  cardano_sub_tx_builder_add_reference_input(builder, reference_utxo);
  cardano_sub_tx_builder_vote(builder, voter, action_id, vote, redeemer);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_plutus_data_unref(&redeemer);
  cardano_utxo_unref(&reference_utxo);
  cardano_voting_procedure_unref(&vote);
  cardano_governance_action_id_unref(&action_id);
  cardano_voter_unref(&voter);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_vote, returnsErrorIfMemoryAllocationFailsWhileBuildingWithVotingRedeemers)
{
  // Arrange
  cardano_protocol_parameters_t*  params    = init_dijkstra_protocol_parameters();
  cardano_script_t*               script    = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_voter_t*                voter     = create_script_voter(script);
  cardano_governance_action_id_t* action_id = create_governance_action_id();
  cardano_voting_procedure_t*     vote      = create_voting_procedure(VOTING_PROCEDURE_CBOR);
  cardano_plutus_data_t*          redeemer  = create_plutus_data(PLUTUS_DATA_CBOR);
  bool                            built     = false;

  for (int i = 0; (i < 100000) && !built; ++i)
  {
    cardano_sub_tx_builder_t*  builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
    cardano_sub_transaction_t* sub_tx  = nullptr;

    cardano_sub_tx_builder_add_script(builder, script);
    cardano_sub_tx_builder_vote(builder, voter, action_id, vote, redeemer);

    reset_allocators_run_count();
    reset_limited_malloc();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

    cardano_set_allocators(malloc, realloc, free);

    // Assert
    built = (result == CARDANO_SUCCESS);

    if (!built)
    {
      EXPECT_EQ(sub_tx, nullptr);
    }

    cardano_sub_transaction_unref(&sub_tx);
    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(built);

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_plutus_data_unref(&redeemer);
  cardano_voting_procedure_unref(&vote);
  cardano_governance_action_id_unref(&action_id);
  cardano_voter_unref(&voter);
  cardano_script_unref(&script);
  cardano_protocol_parameters_unref(&params);
}

/* GUARDRAILS PROPOSALS ******************************************************/

static const char* WITHDRAWAL_MAP_CBOR = "a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d005";

/**
 * Creates a protocol parameter update that changes the minimum fee coefficient.
 * \return A new instance of the protocol parameter update.
 */
static cardano_protocol_param_update_t*
create_protocol_param_update()
{
  cardano_protocol_param_update_t* update    = NULL;
  const uint64_t                   min_fee_a = 500U;

  EXPECT_EQ(cardano_protocol_param_update_new(&update), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_param_update_set_min_fee_a(update, &min_fee_a), CARDANO_SUCCESS);

  return update;
}

/**
 * Creates the default treasury withdrawals.
 * \return A new instance of the withdrawal map.
 */
static cardano_withdrawal_map_t*
create_treasury_withdrawals()
{
  cardano_withdrawal_map_t* withdrawals = NULL;
  cardano_cbor_reader_t*    reader      = cardano_cbor_reader_from_hex(WITHDRAWAL_MAP_CBOR, strlen(WITHDRAWAL_MAP_CBOR));

  EXPECT_EQ(cardano_withdrawal_map_from_cbor(reader, &withdrawals), CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return withdrawals;
}

/**
 * Expects a sub transaction to carry a single proposing redeemer with non zero execution units.
 * \param sub_tx The sub transaction.
 * \param index The expected index of the redeemer.
 */
static void
expect_sub_tx_proposing_redeemer(cardano_sub_transaction_t* sub_tx, const uint64_t index)
{
  ASSERT_EQ(get_sub_tx_redeemer_count(sub_tx), 1U);

  cardano_redeemer_t* redeemer = get_sub_tx_redeemer(sub_tx, 0U);
  cardano_ex_units_t* units    = cardano_redeemer_get_ex_units(redeemer);

  EXPECT_EQ(cardano_redeemer_get_tag(redeemer), CARDANO_REDEEMER_TAG_PROPOSING);
  EXPECT_EQ(cardano_redeemer_get_index(redeemer), index);
  EXPECT_NE(cardano_ex_units_get_cpu_steps(units), 0U);
  EXPECT_NE(cardano_ex_units_get_memory(units), 0U);

  cardano_ex_units_unref(&units);
  cardano_redeemer_unref(&redeemer);
}

TEST(cardano_sub_tx_builder_propose_parameter_change, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_propose_parameter_change(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_propose_parameter_change, returnsErrorIfARequiredArgumentIsNull)
{
  // Arrange
  cardano_protocol_parameters_t*   params                 = init_protocol_parameters();
  cardano_sub_tx_builder_t*        reward_address_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*        anchor_builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*        update_builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*        policy_builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*        reward_address         = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*                anchor                 = create_anchor();
  cardano_protocol_param_update_t* update                 = create_protocol_param_update();
  cardano_governance_action_id_t*  action_id              = create_governance_action_id();
  cardano_blake2b_hash_t*          policy_hash            = create_hash(SCRIPT_HASH_HEX);

  // Act
  cardano_sub_tx_builder_propose_parameter_change(reward_address_builder, nullptr, anchor, update, action_id, policy_hash);
  cardano_sub_tx_builder_propose_parameter_change(anchor_builder, reward_address, nullptr, update, action_id, policy_hash);
  cardano_sub_tx_builder_propose_parameter_change(update_builder, reward_address, anchor, nullptr, action_id, policy_hash);
  cardano_sub_tx_builder_propose_parameter_change(policy_builder, reward_address, anchor, update, action_id, nullptr);

  // Assert
  EXPECT_EQ(reward_address_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(anchor_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(update_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(policy_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&reward_address_builder);
  cardano_sub_tx_builder_unref(&anchor_builder);
  cardano_sub_tx_builder_unref(&update_builder);
  cardano_sub_tx_builder_unref(&policy_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_protocol_param_update_unref(&update);
  cardano_governance_action_id_unref(&action_id);
  cardano_blake2b_hash_unref(&policy_hash);
}

TEST(cardano_sub_tx_builder_propose_parameter_change, rejectsAMissingPolicyHashWithoutAttachingARedeemer)
{
  // Arrange
  cardano_protocol_parameters_t*   params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*        builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*        reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*                anchor         = create_anchor();
  cardano_protocol_param_update_t* update         = create_protocol_param_update();
  cardano_sub_transaction_t*       sub_tx         = nullptr;

  // Act
  cardano_sub_tx_builder_propose_parameter_change(builder, reward_address, anchor, update, nullptr, nullptr);

  cardano_witness_set_t*   witness_set = cardano_transaction_get_witness_set(builder->state.transaction);
  cardano_redeemer_list_t* redeemers   = cardano_witness_set_get_redeemers(witness_set);
  const cardano_error_t    result      = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(cardano_redeemer_list_get_length(redeemers), 0U);
  EXPECT_EQ(cardano_transaction_body_get_proposal_procedures(get_state_body(builder)), nullptr);
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_redeemer_list_unref(&redeemers);
  cardano_witness_set_unref(&witness_set);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_protocol_param_update_unref(&update);
}

TEST(cardano_sub_tx_builder_propose_parameter_change, proposesWithAPlutusV4GuardrailsScriptAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t*   params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*        builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*                script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_blake2b_hash_t*          policy_hash    = cardano_script_get_hash(script);
  cardano_reward_address_t*        reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*                anchor         = create_anchor();
  cardano_protocol_param_update_t* update         = create_protocol_param_update();

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_propose_parameter_change(builder, reward_address, anchor, update, nullptr, policy_hash);

  cardano_sub_transaction_t*        sub_tx    = build_sub_transaction(builder);
  cardano_proposal_procedure_set_t* proposals = cardano_sub_transaction_body_get_proposal_procedures(get_body(sub_tx));
  cardano_blake2b_hash_t*           hash      = cardano_sub_transaction_body_get_script_data_hash(get_body(sub_tx));

  cardano_proposal_procedure_set_unref(&proposals);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(proposals), 1U);
  EXPECT_EQ(get_proposal_type(proposals, 0U), CARDANO_GOVERNANCE_ACTION_TYPE_PARAMETER_CHANGE);
  expect_sub_tx_proposing_redeemer(sub_tx, 0U);
  EXPECT_NE(hash, nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_script_unref(&script);
  cardano_blake2b_hash_unref(&policy_hash);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_protocol_param_update_unref(&update);
}

TEST(cardano_sub_tx_builder_propose_parameter_change, indexesTheRedeemerAtThePositionOfTheProposal)
{
  // Arrange
  cardano_protocol_parameters_t*   params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*        builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*                script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_blake2b_hash_t*          policy_hash    = cardano_script_get_hash(script);
  cardano_reward_address_t*        reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*                anchor         = create_anchor();
  cardano_protocol_param_update_t* update         = create_protocol_param_update();

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_propose_info(builder, reward_address, anchor);
  cardano_sub_tx_builder_propose_parameter_change(builder, reward_address, anchor, update, nullptr, policy_hash);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_proposing_redeemer(sub_tx, 1U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_script_unref(&script);
  cardano_blake2b_hash_unref(&policy_hash);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_protocol_param_update_unref(&update);
}

TEST(cardano_sub_tx_builder_propose_parameter_change, reportsAPlutusV3GuardrailsScriptWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t*   params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*        builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*                script         = create_script(ALWAYS_SUCCEEDS_V3_CBOR);
  cardano_blake2b_hash_t*          policy_hash    = cardano_script_get_hash(script);
  cardano_utxo_t*                  reference_utxo = create_reference_script_utxo(REFERENCE_TX_ID, script);
  cardano_reward_address_t*        reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*                anchor         = create_anchor();
  cardano_protocol_param_update_t* update         = create_protocol_param_update();
  cardano_sub_transaction_t*       sub_tx         = nullptr;

  // Act
  cardano_sub_tx_builder_add_reference_input(builder, reference_utxo);
  cardano_sub_tx_builder_propose_parameter_change(builder, reward_address, anchor, update, nullptr, policy_hash);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_script_unref(&script);
  cardano_blake2b_hash_unref(&policy_hash);
  cardano_utxo_unref(&reference_utxo);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_protocol_param_update_unref(&update);
}

TEST(cardano_sub_tx_builder_propose_parameter_change, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t*   params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*        builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*        reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*                anchor         = create_anchor();
  cardano_protocol_param_update_t* update         = create_protocol_param_update();
  cardano_blake2b_hash_t*          policy_hash    = create_hash(SCRIPT_HASH_HEX);
  cardano_sub_transaction_t*       sub_tx         = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_propose_parameter_change(builder, reward_address, anchor, update, nullptr, policy_hash);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_proposal_procedures(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_protocol_param_update_unref(&update);
  cardano_blake2b_hash_unref(&policy_hash);
}

TEST(cardano_sub_tx_builder_propose_parameter_change, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*   params         = init_protocol_parameters();
  cardano_reward_address_t*        reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*                anchor         = create_anchor();
  cardano_protocol_param_update_t* update         = create_protocol_param_update();
  cardano_governance_action_id_t*  action_id      = create_governance_action_id();
  cardano_blake2b_hash_t*          policy_hash    = create_hash(SCRIPT_HASH_HEX);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_propose_parameter_change(builder, reward_address, anchor, update, action_id, policy_hash);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_protocol_param_update_unref(&update);
  cardano_governance_action_id_unref(&action_id);
  cardano_blake2b_hash_unref(&policy_hash);
}

TEST(cardano_sub_tx_builder_propose_parameter_change_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_propose_parameter_change_ex(nullptr, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0, nullptr);
}

TEST(cardano_sub_tx_builder_propose_parameter_change_ex, returnsErrorIfARequiredArgumentIsNull)
{
  // Arrange
  cardano_protocol_parameters_t*   params          = init_protocol_parameters();
  cardano_sub_tx_builder_t*        address_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*        url_builder     = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*        hash_builder    = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*        action_builder  = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*        policy_builder  = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*        update_builder  = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_protocol_param_update_t* update          = create_protocol_param_update();

  // Act
  cardano_sub_tx_builder_propose_parameter_change_ex(address_builder, nullptr, 0, ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), update);
  cardano_sub_tx_builder_propose_parameter_change_ex(url_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr, 0, ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), update);
  cardano_sub_tx_builder_propose_parameter_change_ex(hash_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), nullptr, 0, GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), update);
  cardano_sub_tx_builder_propose_parameter_change_ex(action_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr, 0, SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), update);
  cardano_sub_tx_builder_propose_parameter_change_ex(policy_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), nullptr, 0, update);
  cardano_sub_tx_builder_propose_parameter_change_ex(update_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), nullptr);

  // Assert
  EXPECT_EQ(address_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(url_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(hash_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(action_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(action_builder), "Governance action ID is NULL or empty.");
  EXPECT_EQ(policy_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(update_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&address_builder);
  cardano_sub_tx_builder_unref(&url_builder);
  cardano_sub_tx_builder_unref(&hash_builder);
  cardano_sub_tx_builder_unref(&action_builder);
  cardano_sub_tx_builder_unref(&policy_builder);
  cardano_sub_tx_builder_unref(&update_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_protocol_param_update_unref(&update);
}

TEST(cardano_sub_tx_builder_propose_parameter_change_ex, proposesWithAPlutusV4GuardrailsScriptAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t*   params      = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*        builder     = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*                script      = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_blake2b_hash_t*          policy_hash = cardano_script_get_hash(script);
  cardano_protocol_param_update_t* update      = create_protocol_param_update();
  char*                            policy_hex  = (char*)malloc(cardano_blake2b_hash_get_hex_size(policy_hash));

  EXPECT_EQ(cardano_blake2b_hash_to_hex(policy_hash, policy_hex, cardano_blake2b_hash_get_hex_size(policy_hash)), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_propose_parameter_change_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), policy_hex, strlen(policy_hex), update);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_proposing_redeemer(sub_tx, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_script_unref(&script);
  cardano_blake2b_hash_unref(&policy_hash);
  cardano_protocol_param_update_unref(&update);
  free(policy_hex);
}

TEST(cardano_sub_tx_builder_propose_parameter_change_ex, reportsAPlutusV3GuardrailsScriptWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t*   params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*        builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*                script         = create_script(ALWAYS_SUCCEEDS_V3_CBOR);
  cardano_blake2b_hash_t*          policy_hash    = cardano_script_get_hash(script);
  cardano_utxo_t*                  reference_utxo = create_reference_script_utxo(REFERENCE_TX_ID, script);
  cardano_protocol_param_update_t* update         = create_protocol_param_update();
  char*                            policy_hex     = (char*)malloc(cardano_blake2b_hash_get_hex_size(policy_hash));
  cardano_sub_transaction_t*       sub_tx         = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_to_hex(policy_hash, policy_hex, cardano_blake2b_hash_get_hex_size(policy_hash)), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_reference_input(builder, reference_utxo);
  cardano_sub_tx_builder_propose_parameter_change_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), policy_hex, strlen(policy_hex), update);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_script_unref(&script);
  cardano_blake2b_hash_unref(&policy_hash);
  cardano_utxo_unref(&reference_utxo);
  cardano_protocol_param_update_unref(&update);
  free(policy_hex);
}

TEST(cardano_sub_tx_builder_propose_parameter_change_ex, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t*   params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*        builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_protocol_param_update_t* update  = create_protocol_param_update();
  cardano_sub_transaction_t*       sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_propose_parameter_change_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), update);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_protocol_param_update_unref(&update);
}

TEST(cardano_sub_tx_builder_propose_parameter_change_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*   params = init_protocol_parameters();
  cardano_protocol_param_update_t* update = create_protocol_param_update();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 400) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_propose_parameter_change_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), GOVERNANCE_ACTION_ID, strlen(GOVERNANCE_ACTION_ID), SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), update);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_protocol_param_update_unref(&update);
}

TEST(cardano_sub_tx_builder_propose_treasury_withdrawals, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_propose_treasury_withdrawals(nullptr, nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_propose_treasury_withdrawals, returnsErrorIfARequiredArgumentIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params                 = init_protocol_parameters();
  cardano_sub_tx_builder_t*      reward_address_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      anchor_builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      withdrawals_builder    = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      policy_builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address         = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*              anchor                 = create_anchor();
  cardano_withdrawal_map_t*      withdrawals            = create_treasury_withdrawals();
  cardano_blake2b_hash_t*        policy_hash            = create_hash(SCRIPT_HASH_HEX);

  // Act
  cardano_sub_tx_builder_propose_treasury_withdrawals(reward_address_builder, nullptr, anchor, withdrawals, policy_hash);
  cardano_sub_tx_builder_propose_treasury_withdrawals(anchor_builder, reward_address, nullptr, withdrawals, policy_hash);
  cardano_sub_tx_builder_propose_treasury_withdrawals(withdrawals_builder, reward_address, anchor, nullptr, policy_hash);
  cardano_sub_tx_builder_propose_treasury_withdrawals(policy_builder, reward_address, anchor, withdrawals, nullptr);

  // Assert
  EXPECT_EQ(reward_address_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(anchor_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(withdrawals_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(policy_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&reward_address_builder);
  cardano_sub_tx_builder_unref(&anchor_builder);
  cardano_sub_tx_builder_unref(&withdrawals_builder);
  cardano_sub_tx_builder_unref(&policy_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_withdrawal_map_unref(&withdrawals);
  cardano_blake2b_hash_unref(&policy_hash);
}

TEST(cardano_sub_tx_builder_propose_treasury_withdrawals, rejectsAMissingPolicyHashWithoutAttachingARedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*              anchor         = create_anchor();
  cardano_withdrawal_map_t*      withdrawals    = create_treasury_withdrawals();
  cardano_sub_transaction_t*     sub_tx         = nullptr;

  // Act
  cardano_sub_tx_builder_propose_treasury_withdrawals(builder, reward_address, anchor, withdrawals, nullptr);

  cardano_witness_set_t*   witness_set = cardano_transaction_get_witness_set(builder->state.transaction);
  cardano_redeemer_list_t* redeemers   = cardano_witness_set_get_redeemers(witness_set);
  const cardano_error_t    result      = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(cardano_redeemer_list_get_length(redeemers), 0U);
  EXPECT_EQ(cardano_transaction_body_get_proposal_procedures(get_state_body(builder)), nullptr);
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_redeemer_list_unref(&redeemers);
  cardano_witness_set_unref(&witness_set);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_withdrawal_map_unref(&withdrawals);
}

TEST(cardano_sub_tx_builder_propose_treasury_withdrawals, proposesWithAPlutusV4GuardrailsScriptAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_blake2b_hash_t*        policy_hash    = cardano_script_get_hash(script);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*              anchor         = create_anchor();
  cardano_withdrawal_map_t*      withdrawals    = create_treasury_withdrawals();

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_propose_treasury_withdrawals(builder, reward_address, anchor, withdrawals, policy_hash);

  cardano_sub_transaction_t*        sub_tx    = build_sub_transaction(builder);
  cardano_proposal_procedure_set_t* proposals = cardano_sub_transaction_body_get_proposal_procedures(get_body(sub_tx));
  cardano_blake2b_hash_t*           hash      = cardano_sub_transaction_body_get_script_data_hash(get_body(sub_tx));

  cardano_proposal_procedure_set_unref(&proposals);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(proposals), 1U);
  EXPECT_EQ(get_proposal_type(proposals, 0U), CARDANO_GOVERNANCE_ACTION_TYPE_TREASURY_WITHDRAWALS);
  expect_sub_tx_proposing_redeemer(sub_tx, 0U);
  EXPECT_NE(hash, nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&hash);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_script_unref(&script);
  cardano_blake2b_hash_unref(&policy_hash);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_withdrawal_map_unref(&withdrawals);
}

TEST(cardano_sub_tx_builder_propose_treasury_withdrawals, reportsAPlutusV3GuardrailsScriptWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V3_CBOR);
  cardano_blake2b_hash_t*        policy_hash    = cardano_script_get_hash(script);
  cardano_utxo_t*                reference_utxo = create_reference_script_utxo(REFERENCE_TX_ID, script);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*              anchor         = create_anchor();
  cardano_withdrawal_map_t*      withdrawals    = create_treasury_withdrawals();
  cardano_sub_transaction_t*     sub_tx         = nullptr;

  // Act
  cardano_sub_tx_builder_add_reference_input(builder, reference_utxo);
  cardano_sub_tx_builder_propose_treasury_withdrawals(builder, reward_address, anchor, withdrawals, policy_hash);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_script_unref(&script);
  cardano_blake2b_hash_unref(&policy_hash);
  cardano_utxo_unref(&reference_utxo);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_withdrawal_map_unref(&withdrawals);
}

TEST(cardano_sub_tx_builder_propose_treasury_withdrawals, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*              anchor         = create_anchor();
  cardano_withdrawal_map_t*      withdrawals    = create_treasury_withdrawals();
  cardano_blake2b_hash_t*        policy_hash    = create_hash(SCRIPT_HASH_HEX);
  cardano_sub_transaction_t*     sub_tx         = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_propose_treasury_withdrawals(builder, reward_address, anchor, withdrawals, policy_hash);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_transaction_body_get_proposal_procedures(get_state_body(builder)), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_withdrawal_map_unref(&withdrawals);
  cardano_blake2b_hash_unref(&policy_hash);
}

TEST(cardano_sub_tx_builder_propose_treasury_withdrawals, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  cardano_anchor_t*              anchor         = create_anchor();
  cardano_withdrawal_map_t*      withdrawals    = create_treasury_withdrawals();
  cardano_blake2b_hash_t*        policy_hash    = create_hash(SCRIPT_HASH_HEX);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 200) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_propose_treasury_withdrawals(builder, reward_address, anchor, withdrawals, policy_hash);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_withdrawal_map_unref(&withdrawals);
  cardano_blake2b_hash_unref(&policy_hash);
}

TEST(cardano_sub_tx_builder_propose_treasury_withdrawals_ex, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_propose_treasury_withdrawals_ex(nullptr, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0, nullptr);
}

TEST(cardano_sub_tx_builder_propose_treasury_withdrawals_ex, returnsErrorIfARequiredArgumentIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params              = init_protocol_parameters();
  cardano_sub_tx_builder_t*      address_builder     = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      url_builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      hash_builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      policy_builder      = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      withdrawals_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_withdrawal_map_t*      withdrawals         = create_treasury_withdrawals();

  // Act
  cardano_sub_tx_builder_propose_treasury_withdrawals_ex(address_builder, nullptr, 0, ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), withdrawals);
  cardano_sub_tx_builder_propose_treasury_withdrawals_ex(url_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr, 0, ANCHOR_HASH, strlen(ANCHOR_HASH), SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), withdrawals);
  cardano_sub_tx_builder_propose_treasury_withdrawals_ex(hash_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), nullptr, 0, SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), withdrawals);
  cardano_sub_tx_builder_propose_treasury_withdrawals_ex(policy_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr, 0, withdrawals);
  cardano_sub_tx_builder_propose_treasury_withdrawals_ex(withdrawals_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), nullptr);

  // Assert
  EXPECT_EQ(address_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(url_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(hash_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(policy_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(withdrawals_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&address_builder);
  cardano_sub_tx_builder_unref(&url_builder);
  cardano_sub_tx_builder_unref(&hash_builder);
  cardano_sub_tx_builder_unref(&policy_builder);
  cardano_sub_tx_builder_unref(&withdrawals_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_withdrawal_map_unref(&withdrawals);
}

TEST(cardano_sub_tx_builder_propose_treasury_withdrawals_ex, proposesWithAPlutusV4GuardrailsScriptAndEvaluatesItsRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder     = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script      = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_blake2b_hash_t*        policy_hash = cardano_script_get_hash(script);
  cardano_withdrawal_map_t*      withdrawals = create_treasury_withdrawals();
  char*                          policy_hex  = (char*)malloc(cardano_blake2b_hash_get_hex_size(policy_hash));

  EXPECT_EQ(cardano_blake2b_hash_to_hex(policy_hash, policy_hex, cardano_blake2b_hash_get_hex_size(policy_hash)), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_propose_treasury_withdrawals_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), policy_hex, strlen(policy_hex), withdrawals);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);

  // Assert
  expect_sub_tx_proposing_redeemer(sub_tx, 0U);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_script_unref(&script);
  cardano_blake2b_hash_unref(&policy_hash);
  cardano_withdrawal_map_unref(&withdrawals);
  free(policy_hex);
}

TEST(cardano_sub_tx_builder_propose_treasury_withdrawals_ex, reportsAPlutusV3GuardrailsScriptWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V3_CBOR);
  cardano_blake2b_hash_t*        policy_hash    = cardano_script_get_hash(script);
  cardano_utxo_t*                reference_utxo = create_reference_script_utxo(REFERENCE_TX_ID, script);
  cardano_withdrawal_map_t*      withdrawals    = create_treasury_withdrawals();
  char*                          policy_hex     = (char*)malloc(cardano_blake2b_hash_get_hex_size(policy_hash));
  cardano_sub_transaction_t*     sub_tx         = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_to_hex(policy_hash, policy_hex, cardano_blake2b_hash_get_hex_size(policy_hash)), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_reference_input(builder, reference_utxo);
  cardano_sub_tx_builder_propose_treasury_withdrawals_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), policy_hex, strlen(policy_hex), withdrawals);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_script_unref(&script);
  cardano_blake2b_hash_unref(&policy_hash);
  cardano_utxo_unref(&reference_utxo);
  cardano_withdrawal_map_unref(&withdrawals);
  free(policy_hex);
}

TEST(cardano_sub_tx_builder_propose_treasury_withdrawals_ex, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder     = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_withdrawal_map_t*      withdrawals = create_treasury_withdrawals();
  cardano_sub_transaction_t*     sub_tx      = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_propose_treasury_withdrawals_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), withdrawals);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_withdrawal_map_unref(&withdrawals);
}

TEST(cardano_sub_tx_builder_propose_treasury_withdrawals_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_withdrawal_map_t*      withdrawals = create_treasury_withdrawals();

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 400) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_propose_treasury_withdrawals_ex(builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), withdrawals);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_withdrawal_map_unref(&withdrawals);
}

/* DEFERRED REDEEMERS ********************************************************/

static const char* DEFERRED_REDEEMER_ERROR = "Failed to resolve the deferred redeemers of the sub transaction.";

/**
 * Produces an integer redeemer payload that carries an index.
 * \param result The result of the index lookup.
 * \param index The index to carry.
 * \param redeemer On success, the produced payload.
 * \return The result of the lookup when it failed, or the result of creating the payload.
 */
static cardano_error_t
create_index_payload(const cardano_error_t result, const uint64_t index, cardano_plutus_data_t** redeemer)
{
  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  return cardano_plutus_data_new_integer_from_int((int64_t)index, redeemer);
}

/**
 * Deferred redeemer callback that produces the position of its own input in the inputs of the body, and checks that
 * the resolved inputs are given in that same order.
 */
static cardano_error_t
input_index_callback(void* user_context, cardano_transaction_t* draft_tx, cardano_utxo_list_t* resolved_inputs, cardano_plutus_data_t** redeemer)
{
  cardano_utxo_t*              own_utxo  = (cardano_utxo_t*)user_context;
  cardano_transaction_input_t* own_input = cardano_utxo_get_input(own_utxo);
  cardano_blake2b_hash_t*      id        = cardano_transaction_input_get_id(own_input);
  uint64_t                     index     = 0U;
  cardano_utxo_t*              resolved  = NULL;

  cardano_error_t result = cardano_transaction_find_input_index(draft_tx, id, cardano_transaction_input_get_index(own_input), &index);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_utxo_list_get(resolved_inputs, (size_t)index, &resolved);
  }

  if (result == CARDANO_SUCCESS)
  {
    cardano_transaction_input_t* resolved_input = cardano_utxo_get_input(resolved);

    if (!cardano_transaction_input_equals(resolved_input, own_input))
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;
    }

    cardano_transaction_input_unref(&resolved_input);
  }

  cardano_utxo_unref(&resolved);
  cardano_blake2b_hash_unref(&id);
  cardano_transaction_input_unref(&own_input);

  return create_index_payload(result, index, redeemer);
}

/**
 * Deferred redeemer callback that produces the position of an output in the outputs of the body.
 */
static cardano_error_t
output_index_callback(void* user_context, cardano_transaction_t* draft_tx, cardano_utxo_list_t* resolved_inputs, cardano_plutus_data_t** redeemer)
{
  CARDANO_UNUSED(resolved_inputs);

  cardano_transaction_output_t* output  = (cardano_transaction_output_t*)user_context;
  cardano_address_t*            address = cardano_transaction_output_get_address(output);
  cardano_value_t*              value   = cardano_transaction_output_get_value(output);
  uint64_t                      index   = 0U;

  const cardano_error_t result = cardano_transaction_find_output_index(draft_tx, address, (uint64_t)cardano_value_get_coin(value), &index);

  cardano_value_unref(&value);
  cardano_address_unref(&address);

  return create_index_payload(result, index, redeemer);
}

/**
 * Deferred redeemer callback that produces the position of a minting policy in the mint of the body.
 */
static cardano_error_t
mint_index_callback(void* user_context, cardano_transaction_t* draft_tx, cardano_utxo_list_t* resolved_inputs, cardano_plutus_data_t** redeemer)
{
  CARDANO_UNUSED(resolved_inputs);

  uint64_t index = 0U;

  const cardano_error_t result = cardano_transaction_find_mint_policy_index(draft_tx, (cardano_blake2b_hash_t*)user_context, &index);

  return create_index_payload(result, index, redeemer);
}

/**
 * Deferred redeemer callback that produces the position of a withdrawal in the withdrawals of the body.
 */
static cardano_error_t
withdrawal_index_callback(void* user_context, cardano_transaction_t* draft_tx, cardano_utxo_list_t* resolved_inputs, cardano_plutus_data_t** redeemer)
{
  CARDANO_UNUSED(resolved_inputs);

  uint64_t index = 0U;

  const cardano_error_t result = cardano_transaction_find_withdrawal_index(draft_tx, (cardano_reward_address_t*)user_context, &index);

  return create_index_payload(result, index, redeemer);
}

/**
 * Deferred redeemer callback that produces the position of a certificate in the certificates of the body.
 */
static cardano_error_t
certificate_index_callback(void* user_context, cardano_transaction_t* draft_tx, cardano_utxo_list_t* resolved_inputs, cardano_plutus_data_t** redeemer)
{
  CARDANO_UNUSED(resolved_inputs);

  uint64_t index = 0U;

  const cardano_error_t result = cardano_transaction_find_certificate_index(draft_tx, (cardano_certificate_t*)user_context, &index);

  return create_index_payload(result, index, redeemer);
}

/**
 * Deferred redeemer callback that produces the position of a voter in the voting procedures of the body.
 */
static cardano_error_t
vote_index_callback(void* user_context, cardano_transaction_t* draft_tx, cardano_utxo_list_t* resolved_inputs, cardano_plutus_data_t** redeemer)
{
  CARDANO_UNUSED(resolved_inputs);

  uint64_t index = 0U;

  const cardano_error_t result = cardano_transaction_find_vote_index(draft_tx, (cardano_voter_t*)user_context, &index);

  return create_index_payload(result, index, redeemer);
}

/**
 * Deferred redeemer callback that always fails.
 */
static cardano_error_t
failing_sub_tx_callback(void* user_context, cardano_transaction_t* draft_tx, cardano_utxo_list_t* resolved_inputs, cardano_plutus_data_t** redeemer)
{
  CARDANO_UNUSED(user_context);
  CARDANO_UNUSED(draft_tx);
  CARDANO_UNUSED(resolved_inputs);
  CARDANO_UNUSED(redeemer);

  return CARDANO_ERROR_INVALID_ARGUMENT;
}

/**
 * Deferred redeemer callback that reports success without producing a payload.
 */
static cardano_error_t
null_payload_sub_tx_callback(void* user_context, cardano_transaction_t* draft_tx, cardano_utxo_list_t* resolved_inputs, cardano_plutus_data_t** redeemer)
{
  CARDANO_UNUSED(user_context);
  CARDANO_UNUSED(draft_tx);
  CARDANO_UNUSED(resolved_inputs);
  CARDANO_UNUSED(redeemer);

  return CARDANO_SUCCESS;
}

/**
 * Reads the integer payload of a redeemer.
 * \param redeemer The redeemer.
 * \return The integer carried by the redeemer.
 */
static int64_t
read_index_payload(cardano_redeemer_t* redeemer)
{
  cardano_plutus_data_t* data  = cardano_redeemer_get_data(redeemer);
  cardano_bigint_t*      value = NULL;

  EXPECT_EQ(cardano_plutus_data_to_integer(data, &value), CARDANO_SUCCESS);

  const int64_t index = cardano_bigint_to_int(value);

  cardano_bigint_unref(&value);
  cardano_plutus_data_unref(&data);

  return index;
}

/**
 * Expects every redeemer of a sub transaction to have a tag, to carry its own index as payload and to have non zero
 * execution units.
 * \param sub_tx The sub transaction.
 * \param tag The expected tag.
 * \param count The expected number of redeemers.
 */
static void
expect_redeemers_carry_their_index(cardano_sub_transaction_t* sub_tx, const cardano_redeemer_tag_t tag, const size_t count)
{
  ASSERT_EQ(get_sub_tx_redeemer_count(sub_tx), count);

  for (size_t i = 0U; i < count; ++i)
  {
    cardano_redeemer_t* redeemer = get_sub_tx_redeemer(sub_tx, i);
    cardano_ex_units_t* units    = cardano_redeemer_get_ex_units(redeemer);

    EXPECT_EQ(cardano_redeemer_get_tag(redeemer), tag);
    EXPECT_EQ(read_index_payload(redeemer), (int64_t)cardano_redeemer_get_index(redeemer));
    EXPECT_NE(cardano_ex_units_get_cpu_steps(units), 0U);
    EXPECT_NE(cardano_ex_units_get_memory(units), 0U);

    cardano_ex_units_unref(&units);
    cardano_redeemer_unref(&redeemer);
  }
}

/**
 * Computes the script data hash of the redeemers and datums of a sub transaction with the PlutusV4 cost model.
 * \param params The protocol parameters.
 * \param sub_tx The sub transaction.
 * \return A new instance of the hash.
 */
static cardano_blake2b_hash_t*
compute_sub_tx_script_data_hash(cardano_protocol_parameters_t* params, cardano_sub_transaction_t* sub_tx)
{
  cardano_costmdls_t*        params_costmdls = cardano_protocol_parameters_get_cost_models(params);
  cardano_costmdls_t*        costmdls        = NULL;
  cardano_cost_model_t*      v4_model        = NULL;
  cardano_witness_set_t*     witness_set     = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_redeemer_list_t*   redeemers       = cardano_witness_set_get_redeemers(witness_set);
  cardano_plutus_data_set_t* datums          = cardano_witness_set_get_plutus_data(witness_set);
  cardano_blake2b_hash_t*    hash            = NULL;

  EXPECT_EQ(cardano_costmdls_new(&costmdls), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_costmdls_get(params_costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V4, &v4_model), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_costmdls_insert(costmdls, v4_model), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_compute_script_data_hash(costmdls, redeemers, datums, &hash), CARDANO_SUCCESS);

  cardano_plutus_data_set_unref(&datums);
  cardano_redeemer_list_unref(&redeemers);
  cardano_witness_set_unref(&witness_set);
  cardano_cost_model_unref(&v4_model);
  cardano_costmdls_unref(&costmdls);
  cardano_costmdls_unref(&params_costmdls);

  return hash;
}

TEST(cardano_sub_tx_builder_add_input_with_deferred_redeemer, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_add_input_with_deferred_redeemer(nullptr, nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_add_input_with_deferred_redeemer, returnsErrorIfUtxoOrCallbackIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_sub_tx_builder_t*      no_utxo     = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      no_callback = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_utxo_t*                utxo        = create_utxo(UTXO_CBOR);

  // Act
  cardano_sub_tx_builder_add_input_with_deferred_redeemer(no_utxo, nullptr, input_index_callback, nullptr, nullptr);
  cardano_sub_tx_builder_add_input_with_deferred_redeemer(no_callback, utxo, nullptr, nullptr, nullptr);

  // Assert
  EXPECT_EQ(no_utxo->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(no_utxo), "UTXO and callback are required");
  EXPECT_EQ(no_callback->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(no_callback->state.deferred_redeemers), 0U);

  // Cleanup
  cardano_utxo_unref(&utxo);
  cardano_sub_tx_builder_unref(&no_callback);
  cardano_sub_tx_builder_unref(&no_utxo);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_input_with_deferred_redeemer, resolvesEachRedeemerToThePositionOfItsInputInTheSortedInputs)
{
  // Arrange
  cardano_protocol_parameters_t* params    = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder   = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script    = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_utxo_t*                high_utxo = create_script_utxo(HIGH_TX_ID, script, nullptr);
  cardano_utxo_t*                key_utxo  = create_utxo(UTXO_CBOR);
  cardano_utxo_t*                low_utxo  = create_script_utxo(LOW_TX_ID, script, nullptr);
  cardano_plutus_data_t*         low_data  = NULL;
  cardano_plutus_data_t*         high_data = NULL;
  cardano_utxo_list_t*           utxos     = NULL;

  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(0, &low_data), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(2, &high_data), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(utxos, high_utxo), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(utxos, key_utxo), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(utxos, low_utxo), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_input_with_deferred_redeemer(builder, high_utxo, input_index_callback, high_utxo, nullptr);
  cardano_sub_tx_builder_add_input(builder, key_utxo, nullptr, nullptr);
  cardano_sub_tx_builder_add_input_with_deferred_redeemer(builder, low_utxo, input_index_callback, low_utxo, nullptr);

  cardano_sub_transaction_t* sub_tx    = build_sub_transaction(builder);
  cardano_redeemer_t*        low       = find_sub_tx_redeemer(sub_tx, low_data);
  cardano_redeemer_t*        high      = find_sub_tx_redeemer(sub_tx, high_data);
  cardano_redeemer_list_t*   evaluated = evaluate_sub_tx(params, sub_tx, utxos);
  cardano_blake2b_hash_t*    hash      = cardano_sub_transaction_body_get_script_data_hash(get_body(sub_tx));
  cardano_blake2b_hash_t*    expected  = compute_sub_tx_script_data_hash(params, sub_tx);

  // Assert
  ASSERT_NE(low, nullptr);
  ASSERT_NE(high, nullptr);
  expect_redeemers_carry_their_index(sub_tx, CARDANO_REDEEMER_TAG_SPEND, 2U);
  EXPECT_EQ(cardano_redeemer_get_index(low), 0U);
  EXPECT_EQ(cardano_redeemer_get_index(high), 2U);
  EXPECT_EQ(cardano_redeemer_list_get_length(evaluated), 2U);
  expect_evaluated_ex_units(low, evaluated);
  expect_evaluated_ex_units(high, evaluated);
  ASSERT_NE(hash, nullptr);
  EXPECT_TRUE(cardano_blake2b_hash_equals(hash, expected));

  // Cleanup
  cardano_blake2b_hash_unref(&expected);
  cardano_blake2b_hash_unref(&hash);
  cardano_redeemer_list_unref(&evaluated);
  cardano_redeemer_unref(&high);
  cardano_redeemer_unref(&low);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_utxo_list_unref(&utxos);
  cardano_plutus_data_unref(&high_data);
  cardano_plutus_data_unref(&low_data);
  cardano_utxo_unref(&low_utxo);
  cardano_utxo_unref(&key_utxo);
  cardano_utxo_unref(&high_utxo);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_input_with_deferred_redeemer, resolvesTheRedeemerFromTheOutputsOfTheSubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script  = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_utxo_t*                utxo    = create_script_utxo(LOW_TX_ID, script, nullptr);
  cardano_utxo_t*                paid    = create_utxo(UTXO_CBOR);
  cardano_transaction_output_t*  output  = cardano_utxo_get_output(paid);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_send_lovelace_ex(builder, ADDRESS, strlen(ADDRESS), 1000000U);
  cardano_sub_tx_builder_add_output(builder, output);
  cardano_sub_tx_builder_add_input_with_deferred_redeemer(builder, utxo, output_index_callback, output, nullptr);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);
  cardano_redeemer_t*        spend  = get_sub_tx_redeemer(sub_tx, 0U);

  // Assert
  EXPECT_EQ(get_sub_tx_redeemer_count(sub_tx), 1U);
  EXPECT_EQ(cardano_redeemer_get_tag(spend), CARDANO_REDEEMER_TAG_SPEND);
  EXPECT_EQ(read_index_payload(spend), 1);

  // Cleanup
  cardano_redeemer_unref(&spend);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_output_unref(&output);
  cardano_utxo_unref(&paid);
  cardano_utxo_unref(&utxo);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_input_with_deferred_redeemer, reportsTheErrorOfAFailingCallbackWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script  = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_utxo_t*                utxo    = create_script_utxo(LOW_TX_ID, script, nullptr);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_input_with_deferred_redeemer(builder, utxo, failing_sub_tx_callback, nullptr, nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), DEFERRED_REDEEMER_ERROR);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_utxo_unref(&utxo);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_input_with_deferred_redeemer, reportsACallbackThatProducesNoPayloadWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script  = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_utxo_t*                utxo    = create_script_utxo(LOW_TX_ID, script, nullptr);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_input_with_deferred_redeemer(builder, utxo, null_payload_sub_tx_callback, nullptr, nullptr);

  const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), DEFERRED_REDEEMER_ERROR);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_utxo_unref(&utxo);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_input_with_deferred_redeemer, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_utxo_t*                utxo    = create_utxo(UTXO_CBOR);

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_add_input_with_deferred_redeemer(builder, utxo, input_index_callback, utxo, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(cardano_utxo_list_get_length(builder->state.pre_selected_inputs), 0U);
  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(builder->state.deferred_redeemers), 0U);

  // Cleanup
  cardano_utxo_unref(&utxo);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_input_with_deferred_redeemer, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_utxo_t*                utxo   = create_utxo(UTXO_CBOR);
  bool                           added  = false;

  for (int i = 0; (i < 100) && !added; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    reset_limited_malloc();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_sub_tx_builder_add_input_with_deferred_redeemer(builder, utxo, input_index_callback, utxo, nullptr);

    cardano_set_allocators(malloc, realloc, free);

    // Assert
    added = (builder->last_error == CARDANO_SUCCESS);

    if (!added)
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(added);

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_utxo_unref(&utxo);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_input_with_deferred_redeemer, returnsErrorIfMemoryAllocationFailsWhileBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params    = init_dijkstra_protocol_parameters();
  cardano_script_t*              script    = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_utxo_t*                high_utxo = create_script_utxo(HIGH_TX_ID, script, nullptr);
  cardano_utxo_t*                low_utxo  = create_script_utxo(LOW_TX_ID, script, nullptr);
  bool                           built     = false;

  for (int i = 0; (i < 100000) && !built; ++i)
  {
    cardano_sub_tx_builder_t*  builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
    cardano_sub_transaction_t* sub_tx  = nullptr;

    cardano_sub_tx_builder_add_script(builder, script);
    cardano_sub_tx_builder_add_input_with_deferred_redeemer(builder, high_utxo, input_index_callback, high_utxo, nullptr);
    cardano_sub_tx_builder_add_input_with_deferred_redeemer(builder, low_utxo, input_index_callback, low_utxo, nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    const cardano_error_t result = cardano_sub_tx_builder_build(builder, &sub_tx);

    cardano_set_allocators(malloc, realloc, free);

    // Assert
    built = (result == CARDANO_SUCCESS);

    if (!built)
    {
      EXPECT_EQ(sub_tx, nullptr);
    }

    cardano_sub_transaction_unref(&sub_tx);
    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(built);

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_utxo_unref(&low_utxo);
  cardano_utxo_unref(&high_utxo);
  cardano_script_unref(&script);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_mint_token_with_deferred_redeemer, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_mint_token_with_deferred_redeemer(nullptr, nullptr, nullptr, 0, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_mint_token_with_deferred_redeemer, returnsErrorIfPolicyIdOrCallbackIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_sub_tx_builder_t*      no_policy   = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      no_callback = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_blake2b_hash_t*        policy_id   = create_hash(SCRIPT_HASH_HEX);
  cardano_asset_name_t*          asset_name  = NULL;

  EXPECT_EQ(cardano_asset_name_from_hex(ASSET_NAME_HEX, strlen(ASSET_NAME_HEX), &asset_name), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_mint_token_with_deferred_redeemer(no_policy, nullptr, asset_name, 10, mint_index_callback, nullptr);
  cardano_sub_tx_builder_mint_token_with_deferred_redeemer(no_callback, policy_id, asset_name, 10, nullptr, nullptr);

  // Assert
  EXPECT_EQ(no_policy->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(no_policy), "Policy id and callback are required");
  EXPECT_EQ(no_callback->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_sub_tx_builder_unref(&no_callback);
  cardano_sub_tx_builder_unref(&no_policy);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_mint_token_with_deferred_redeemer, resolvesEachRedeemerToThePositionOfItsPolicyInTheMint)
{
  // Arrange
  cardano_protocol_parameters_t* params       = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder      = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script       = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_script_t*              other_script = create_script(OTHER_ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_blake2b_hash_t*        policy_id    = cardano_script_get_hash(script);
  cardano_blake2b_hash_t*        other_policy = cardano_script_get_hash(other_script);
  cardano_asset_name_t*          asset_name   = NULL;

  EXPECT_EQ(cardano_asset_name_from_hex(ASSET_NAME_HEX, strlen(ASSET_NAME_HEX), &asset_name), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_script(builder, other_script);
  cardano_sub_tx_builder_mint_token_with_deferred_redeemer(builder, policy_id, asset_name, 10, mint_index_callback, policy_id);
  cardano_sub_tx_builder_mint_token_with_deferred_redeemer(builder, other_policy, asset_name, 10, mint_index_callback, other_policy);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);
  cardano_blake2b_hash_t*    hash   = cardano_sub_transaction_body_get_script_data_hash(get_body(sub_tx));
  cardano_redeemer_t*        first  = get_sub_tx_redeemer(sub_tx, 0U);
  cardano_redeemer_t*        second = get_sub_tx_redeemer(sub_tx, 1U);

  // Assert
  expect_redeemers_carry_their_index(sub_tx, CARDANO_REDEEMER_TAG_MINT, 2U);
  EXPECT_NE(cardano_redeemer_get_index(first), cardano_redeemer_get_index(second));
  EXPECT_NE(hash, nullptr);

  // Cleanup
  cardano_redeemer_unref(&second);
  cardano_redeemer_unref(&first);
  cardano_blake2b_hash_unref(&hash);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&other_policy);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_script_unref(&other_script);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_mint_token_with_deferred_redeemer, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_protocol_parameters();
  cardano_blake2b_hash_t*        policy_id  = create_hash(SCRIPT_HASH_HEX);
  cardano_asset_name_t*          asset_name = NULL;
  bool                           added      = false;

  EXPECT_EQ(cardano_asset_name_from_hex(ASSET_NAME_HEX, strlen(ASSET_NAME_HEX), &asset_name), CARDANO_SUCCESS);

  for (int i = 0; (i < 100) && !added; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    reset_limited_malloc();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_sub_tx_builder_mint_token_with_deferred_redeemer(builder, policy_id, asset_name, 10, mint_index_callback, policy_id);

    cardano_set_allocators(malloc, realloc, free);

    // Assert
    added = (builder->last_error == CARDANO_SUCCESS);

    if (!added)
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(added);

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_withdraw_rewards_with_deferred_redeemer, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_withdraw_rewards_with_deferred_redeemer(nullptr, nullptr, 0, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_withdraw_rewards_with_deferred_redeemer, returnsErrorIfAddressOrCallbackIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*      no_address     = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      no_callback    = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);

  // Act
  cardano_sub_tx_builder_withdraw_rewards_with_deferred_redeemer(no_address, nullptr, 1000000, withdrawal_index_callback, nullptr);
  cardano_sub_tx_builder_withdraw_rewards_with_deferred_redeemer(no_callback, reward_address, 1000000, nullptr, nullptr);

  // Assert
  EXPECT_EQ(no_address->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(no_address), "Reward address and callback are required");
  EXPECT_EQ(no_callback->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_reward_address_unref(&reward_address);
  cardano_sub_tx_builder_unref(&no_callback);
  cardano_sub_tx_builder_unref(&no_address);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_withdraw_rewards_with_deferred_redeemer, resolvesEachRedeemerToThePositionOfItsWithdrawal)
{
  // Arrange
  cardano_protocol_parameters_t* params        = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder       = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script        = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_script_t*              other_script  = create_script(OTHER_ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_reward_address_t*      address       = create_script_reward_address(script);
  cardano_reward_address_t*      other_address = create_script_reward_address(other_script);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_script(builder, other_script);
  cardano_sub_tx_builder_withdraw_rewards_with_deferred_redeemer(builder, address, 1000000, withdrawal_index_callback, address);
  cardano_sub_tx_builder_withdraw_rewards_with_deferred_redeemer(builder, other_address, 1000000, withdrawal_index_callback, other_address);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);
  cardano_redeemer_t*        first  = get_sub_tx_redeemer(sub_tx, 0U);
  cardano_redeemer_t*        second = get_sub_tx_redeemer(sub_tx, 1U);

  // Assert
  expect_redeemers_carry_their_index(sub_tx, CARDANO_REDEEMER_TAG_REWARD, 2U);
  EXPECT_NE(cardano_redeemer_get_index(first), cardano_redeemer_get_index(second));

  // Cleanup
  cardano_redeemer_unref(&second);
  cardano_redeemer_unref(&first);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_reward_address_unref(&other_address);
  cardano_reward_address_unref(&address);
  cardano_script_unref(&other_script);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_withdraw_rewards_with_deferred_redeemer, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_reward_address_t*      reward_address = create_reward_address(REWARD_ADDRESS);
  bool                           added          = false;

  for (int i = 0; (i < 100) && !added; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    reset_limited_malloc();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_sub_tx_builder_withdraw_rewards_with_deferred_redeemer(builder, reward_address, 1000000, withdrawal_index_callback, reward_address);

    cardano_set_allocators(malloc, realloc, free);

    // Assert
    added = (builder->last_error == CARDANO_SUCCESS);

    if (!added)
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(added);

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_reward_address_unref(&reward_address);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_certificate_with_deferred_redeemer, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_add_certificate_with_deferred_redeemer(nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_add_certificate_with_deferred_redeemer, returnsErrorIfCertificateOrCallbackIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_sub_tx_builder_t*      no_certificate = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      no_callback    = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_certificate_t*         certificate    = create_certificate(CERTIFICATE_CBOR);

  // Act
  cardano_sub_tx_builder_add_certificate_with_deferred_redeemer(no_certificate, nullptr, certificate_index_callback, nullptr);
  cardano_sub_tx_builder_add_certificate_with_deferred_redeemer(no_callback, certificate, nullptr, nullptr);

  // Assert
  EXPECT_EQ(no_certificate->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(no_certificate), "Certificate and callback are required");
  EXPECT_EQ(no_callback->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&certificate);
  cardano_sub_tx_builder_unref(&no_callback);
  cardano_sub_tx_builder_unref(&no_certificate);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_certificate_with_deferred_redeemer, resolvesTheRedeemerToThePositionOfTheCertificate)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_credential_t*          credential     = create_script_credential(script);
  cardano_reward_address_t*      key_address    = create_reward_address(REWARD_ADDRESS);
  cardano_unregistration_cert_t* unregistration = NULL;
  cardano_certificate_t*         certificate    = NULL;

  EXPECT_EQ(cardano_unregistration_cert_new(credential, KEY_DEPOSIT, &unregistration), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_certificate_new_unregistration(unregistration, &certificate), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_register_reward_address(builder, key_address, nullptr);
  cardano_sub_tx_builder_add_certificate_with_deferred_redeemer(builder, certificate, certificate_index_callback, certificate);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  cardano_redeemer_t*        redeemer = get_sub_tx_redeemer(sub_tx, 0U);

  // Assert
  expect_redeemers_carry_their_index(sub_tx, CARDANO_REDEEMER_TAG_CERTIFYING, 1U);
  EXPECT_EQ(cardano_redeemer_get_index(redeemer), 1U);

  // Cleanup
  cardano_redeemer_unref(&redeemer);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_certificate_unref(&certificate);
  cardano_unregistration_cert_unref(&unregistration);
  cardano_reward_address_unref(&key_address);
  cardano_credential_unref(&credential);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_certificate_with_deferred_redeemer, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_certificate_t*         certificate = create_certificate(CERTIFICATE_CBOR);
  bool                           added       = false;

  for (int i = 0; (i < 100) && !added; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    reset_limited_malloc();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_sub_tx_builder_add_certificate_with_deferred_redeemer(builder, certificate, certificate_index_callback, certificate);

    cardano_set_allocators(malloc, realloc, free);

    // Assert
    added = (builder->last_error == CARDANO_SUCCESS);

    if (!added)
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(added);

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_certificate_unref(&certificate);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_vote_with_deferred_redeemer, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_vote_with_deferred_redeemer(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_vote_with_deferred_redeemer, returnsErrorIfARequiredArgumentIsNull)
{
  // Arrange
  cardano_protocol_parameters_t*  params      = init_protocol_parameters();
  cardano_sub_tx_builder_t*       no_voter    = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*       no_action   = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*       no_vote     = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*       no_callback = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*               script      = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_voter_t*                voter       = create_script_voter(script);
  cardano_governance_action_id_t* action_id   = create_governance_action_id();
  cardano_voting_procedure_t*     vote        = create_voting_procedure(VOTING_PROCEDURE_CBOR);

  // Act
  cardano_sub_tx_builder_vote_with_deferred_redeemer(no_voter, nullptr, action_id, vote, vote_index_callback, nullptr);
  cardano_sub_tx_builder_vote_with_deferred_redeemer(no_action, voter, nullptr, vote, vote_index_callback, voter);
  cardano_sub_tx_builder_vote_with_deferred_redeemer(no_vote, voter, action_id, nullptr, vote_index_callback, voter);
  cardano_sub_tx_builder_vote_with_deferred_redeemer(no_callback, voter, action_id, vote, nullptr, nullptr);

  // Assert
  EXPECT_EQ(no_voter->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(no_voter), "Voter, action id, vote and callback are required");
  EXPECT_EQ(no_action->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(no_vote->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(no_callback->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_voting_procedure_unref(&vote);
  cardano_governance_action_id_unref(&action_id);
  cardano_voter_unref(&voter);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&no_callback);
  cardano_sub_tx_builder_unref(&no_vote);
  cardano_sub_tx_builder_unref(&no_action);
  cardano_sub_tx_builder_unref(&no_voter);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_vote_with_deferred_redeemer, resolvesEachRedeemerToThePositionOfItsVoter)
{
  // Arrange
  cardano_protocol_parameters_t*  params       = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*       builder      = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*               script       = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_script_t*               other_script = create_script(OTHER_ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_voter_t*                voter        = create_script_voter(script);
  cardano_voter_t*                other_voter  = create_script_voter(other_script);
  cardano_governance_action_id_t* action_id    = create_governance_action_id();
  cardano_voting_procedure_t*     vote         = create_voting_procedure(VOTING_PROCEDURE_CBOR);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_script(builder, other_script);
  cardano_sub_tx_builder_vote_with_deferred_redeemer(builder, voter, action_id, vote, vote_index_callback, voter);
  cardano_sub_tx_builder_vote_with_deferred_redeemer(builder, other_voter, action_id, vote, vote_index_callback, other_voter);

  cardano_sub_transaction_t* sub_tx = build_sub_transaction(builder);
  cardano_redeemer_t*        first  = get_sub_tx_redeemer(sub_tx, 0U);
  cardano_redeemer_t*        second = get_sub_tx_redeemer(sub_tx, 1U);

  // Assert
  expect_redeemers_carry_their_index(sub_tx, CARDANO_REDEEMER_TAG_VOTING, 2U);
  EXPECT_NE(cardano_redeemer_get_index(first), cardano_redeemer_get_index(second));

  // Cleanup
  cardano_redeemer_unref(&second);
  cardano_redeemer_unref(&first);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_voting_procedure_unref(&vote);
  cardano_governance_action_id_unref(&action_id);
  cardano_voter_unref(&other_voter);
  cardano_voter_unref(&voter);
  cardano_script_unref(&other_script);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_vote_with_deferred_redeemer, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*  params    = init_protocol_parameters();
  cardano_script_t*               script    = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_voter_t*                voter     = create_script_voter(script);
  cardano_governance_action_id_t* action_id = create_governance_action_id();
  cardano_voting_procedure_t*     vote      = create_voting_procedure(VOTING_PROCEDURE_CBOR);
  bool                            added     = false;

  for (int i = 0; (i < 100) && !added; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    reset_limited_malloc();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_sub_tx_builder_vote_with_deferred_redeemer(builder, voter, action_id, vote, vote_index_callback, voter);

    cardano_set_allocators(malloc, realloc, free);

    // Assert
    added = (builder->last_error == CARDANO_SUCCESS);

    if (!added)
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(added);

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_voting_procedure_unref(&vote);
  cardano_governance_action_id_unref(&action_id);
  cardano_voter_unref(&voter);
  cardano_script_unref(&script);
  cardano_protocol_parameters_unref(&params);
}

static const char* DATUM_SCRIPT_DATA_HASH = "bdbe6c43b5d8a5c73cd5bcbef8bd410e321f62be95bbed4b26142a72848f32c4";

TEST(cardano_sub_tx_builder_add_datum, doesntCrashIfGivenNull)
{
  cardano_sub_tx_builder_add_datum(nullptr, nullptr);
}

TEST(cardano_sub_tx_builder_add_datum, returnsErrorIfDatumIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_sub_tx_builder_add_datum(builder, nullptr);

  // Assert
  EXPECT_EQ(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_datum, keepsTheFirstErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_plutus_data_t*         datum   = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_sub_transaction_t*     sub_tx  = nullptr;

  record_first_error(builder);

  // Act
  cardano_sub_tx_builder_add_datum(builder, datum);

  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(builder->state.transaction);
  const cardano_error_t  result      = cardano_sub_tx_builder_build(builder, &sub_tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_sub_tx_builder_get_last_error(builder), FIRST_ERROR);
  EXPECT_EQ(cardano_witness_set_get_plutus_data(witness_set), nullptr);
  EXPECT_EQ(sub_tx, nullptr);

  // Cleanup
  cardano_witness_set_unref(&witness_set);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_plutus_data_unref(&datum);
}

TEST(cardano_sub_tx_builder_add_datum, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_plutus_data_t*         datum  = create_plutus_data(PLUTUS_DATA_CBOR);

  // Act & Assert
  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_sub_tx_builder_t* builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_sub_tx_builder_add_datum(builder, datum);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_EQ(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_sub_tx_builder_unref(&builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_plutus_data_unref(&datum);
}

TEST(cardano_sub_tx_builder_add_datum, putsTheDatumInTheWitnessSet)
{
  // Arrange
  cardano_protocol_parameters_t* params  = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_plutus_data_t*         datum   = create_plutus_data(PLUTUS_DATA_CBOR);

  // Act
  cardano_sub_tx_builder_add_datum(builder, datum);

  cardano_sub_transaction_t* sub_tx      = build_sub_transaction(builder);
  cardano_witness_set_t*     witness_set = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_plutus_data_set_t* datums      = cardano_witness_set_get_plutus_data(witness_set);
  cardano_plutus_data_t*     carried     = NULL;

  // Assert
  ASSERT_NE(datums, nullptr);
  EXPECT_EQ(cardano_plutus_data_set_get_length(datums), 1U);
  EXPECT_EQ(cardano_plutus_data_set_get(datums, 0U, &carried), CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_plutus_data_equals(carried, datum));

  // Cleanup
  cardano_plutus_data_unref(&carried);
  cardano_plutus_data_set_unref(&datums);
  cardano_witness_set_unref(&witness_set);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_plutus_data_unref(&datum);
}

TEST(cardano_sub_tx_builder_add_datum, includesTheDatumInTheScriptDataHash)
{
  // Arrange
  cardano_protocol_parameters_t* params        = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder       = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_sub_tx_builder_t*      plain_builder = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_address_t*             address       = create_address(ADDRESS);
  cardano_plutus_data_t*         datum         = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_blake2b_hash_t*        datum_hash    = compute_datum_hash(datum);
  cardano_blake2b_hash_t*        expected      = create_hash(DATUM_SCRIPT_DATA_HASH);
  cardano_datum_t*               locked        = NULL;

  EXPECT_EQ(cardano_datum_new_data_hash(datum_hash, &locked), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_lock_lovelace(builder, address, 2000000, locked);
  cardano_sub_tx_builder_lock_lovelace(plain_builder, address, 2000000, locked);
  cardano_sub_tx_builder_add_datum(builder, datum);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  cardano_sub_transaction_t* plain_tx = build_sub_transaction(plain_builder);
  cardano_blake2b_hash_t*    hash     = cardano_sub_transaction_body_get_script_data_hash(get_body(sub_tx));
  cardano_blake2b_hash_t*    plain    = cardano_sub_transaction_body_get_script_data_hash(get_body(plain_tx));

  // Assert
  ASSERT_NE(hash, nullptr);
  EXPECT_TRUE(cardano_blake2b_hash_equals(hash, expected));
  EXPECT_EQ(plain, nullptr);

  // Cleanup
  cardano_blake2b_hash_unref(&plain);
  cardano_blake2b_hash_unref(&hash);
  cardano_sub_transaction_unref(&plain_tx);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_datum_unref(&locked);
  cardano_blake2b_hash_unref(&expected);
  cardano_blake2b_hash_unref(&datum_hash);
  cardano_plutus_data_unref(&datum);
  cardano_address_unref(&address);
  cardano_sub_tx_builder_unref(&plain_builder);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_add_datum, carriesTheDatumOfALockedOutputAlongsideAScriptInput)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder    = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script     = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_plutus_data_t*         datum      = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_plutus_data_t*         redeemer   = create_plutus_data(OTHER_PLUTUS_DATA_CBOR);
  cardano_utxo_t*                utxo       = create_script_utxo(LOW_TX_ID, script, datum);
  cardano_address_t*             address    = create_address(ADDRESS);
  cardano_blake2b_hash_t*        datum_hash = compute_datum_hash(datum);
  cardano_datum_t*               locked     = NULL;

  EXPECT_EQ(cardano_datum_new_data_hash(datum_hash, &locked), CARDANO_SUCCESS);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_input(builder, utxo, redeemer, nullptr);
  cardano_sub_tx_builder_lock_lovelace(builder, address, 2000000, locked);
  cardano_sub_tx_builder_add_datum(builder, datum);

  cardano_sub_transaction_t*    sub_tx      = build_sub_transaction(builder);
  cardano_witness_set_t*        witness_set = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_plutus_data_set_t*    datums      = cardano_witness_set_get_plutus_data(witness_set);
  cardano_transaction_output_t* output      = get_first_output(sub_tx);
  cardano_datum_t*              out_datum   = cardano_transaction_output_get_datum(output);
  cardano_blake2b_hash_t*       hash        = cardano_sub_transaction_body_get_script_data_hash(get_body(sub_tx));
  cardano_blake2b_hash_t*       expected    = compute_sub_tx_script_data_hash(params, sub_tx);
  cardano_redeemer_t*           spend       = find_sub_tx_redeemer(sub_tx, redeemer);
  cardano_plutus_data_t*        carried     = NULL;

  // Assert
  ASSERT_NE(sub_tx, nullptr);
  ASSERT_NE(spend, nullptr);
  EXPECT_EQ(cardano_plutus_data_set_get_length(datums), 1U);
  EXPECT_EQ(cardano_plutus_data_set_get(datums, 0U, &carried), CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_plutus_data_equals(carried, datum));
  EXPECT_TRUE(cardano_datum_equals(out_datum, locked));
  ASSERT_NE(hash, nullptr);
  EXPECT_TRUE(cardano_blake2b_hash_equals(hash, expected));

  // Cleanup
  cardano_plutus_data_unref(&carried);
  cardano_redeemer_unref(&spend);
  cardano_blake2b_hash_unref(&expected);
  cardano_blake2b_hash_unref(&hash);
  cardano_datum_unref(&out_datum);
  cardano_transaction_output_unref(&output);
  cardano_plutus_data_set_unref(&datums);
  cardano_witness_set_unref(&witness_set);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_datum_unref(&locked);
  cardano_blake2b_hash_unref(&datum_hash);
  cardano_address_unref(&address);
  cardano_utxo_unref(&utxo);
  cardano_plutus_data_unref(&redeemer);
  cardano_plutus_data_unref(&datum);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

/* SCRIPT DATA HASH LANGUAGE VIEWS *******************************************/

TEST(cardano_sub_tx_builder_build, leavesTheLanguageViewOfAnUnneededReferenceScriptOutOfTheScriptDataHash)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_script_t*              unneeded       = create_script(ALWAYS_SUCCEEDS_V3_CBOR);
  cardano_utxo_t*                utxo           = create_script_utxo(LOW_TX_ID, script, nullptr);
  cardano_utxo_t*                reference_utxo = create_reference_script_utxo(REFERENCE_TX_ID, unneeded);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  // Act
  cardano_sub_tx_builder_add_script(builder, script);
  cardano_sub_tx_builder_add_reference_input(builder, reference_utxo);
  cardano_sub_tx_builder_add_input(builder, utxo, redeemer, nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  cardano_blake2b_hash_t*    hash     = cardano_sub_transaction_body_get_script_data_hash(get_body(sub_tx));
  cardano_blake2b_hash_t*    expected = compute_sub_tx_script_data_hash(params, sub_tx);

  // Assert
  EXPECT_TRUE(builder->state.has_plutus_v3);
  ASSERT_NE(hash, nullptr);
  EXPECT_TRUE(cardano_blake2b_hash_equals(hash, expected));

  // Cleanup
  cardano_blake2b_hash_unref(&expected);
  cardano_blake2b_hash_unref(&hash);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_utxo_unref(&reference_utxo);
  cardano_utxo_unref(&utxo);
  cardano_script_unref(&unneeded);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_sub_tx_builder_build, keepsTheLanguageViewOfANeededScriptProvidedByReference)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_dijkstra_protocol_parameters();
  cardano_sub_tx_builder_t*      builder        = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_script_t*              script         = create_script(ALWAYS_SUCCEEDS_V4_CBOR);
  cardano_utxo_t*                utxo           = create_script_utxo(LOW_TX_ID, script, nullptr);
  cardano_utxo_t*                reference_utxo = create_reference_script_utxo(REFERENCE_TX_ID, script);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  // Act
  cardano_sub_tx_builder_add_reference_input(builder, reference_utxo);
  cardano_sub_tx_builder_add_input(builder, utxo, redeemer, nullptr);

  cardano_sub_transaction_t* sub_tx   = build_sub_transaction(builder);
  cardano_blake2b_hash_t*    hash     = cardano_sub_transaction_body_get_script_data_hash(get_body(sub_tx));
  cardano_blake2b_hash_t*    expected = compute_sub_tx_script_data_hash(params, sub_tx);

  // Assert
  ASSERT_NE(hash, nullptr);
  EXPECT_TRUE(cardano_blake2b_hash_equals(hash, expected));

  // Cleanup
  cardano_blake2b_hash_unref(&expected);
  cardano_blake2b_hash_unref(&hash);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_utxo_unref(&reference_utxo);
  cardano_utxo_unref(&utxo);
  cardano_script_unref(&script);
  cardano_sub_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}
