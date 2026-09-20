/**
 * \file transaction_builder.cpp
 *
 * \author angel.castillo
 * \date   Nov 06, 2024
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

#include <cardano/error.h>

#include <cardano/plutus_data/constr_plutus_data.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/proposal_procedures/hard_fork_initiation_action.h>
#include <cardano/transaction_builder/sub_transaction_builder.h>
#include <cardano/transaction_builder/transaction_builder.h>

#include "../../src/transaction_builder/internals/blake2b_hash_to_redeemer_map.h"
#include "../../src/transaction_builder/internals/builder_state.h"
#include <allocators.h>
#include <cardano/address/enterprise_address.h>
#include <cardano/crypto/ed25519_private_key.h>
#include <cardano/crypto/ed25519_signature.h>
#include <cardano/scripts/native_scripts/script_pubkey.h>
#include <cardano/transaction_body/sub_transaction_set.h>
#include <cardano/transaction_body/transaction_output.h>
#include <cardano/transaction_builder/balancing/deferred_redeemer_list.h>
#include <cardano/transaction_builder/balancing/input_to_redeemer_map.h>
#include <cardano/transaction_builder/balancing/transaction_balancing.h>
#include <cardano/transaction_builder/evaluation/provider_tx_evaluator.h>
#include <cardano/transaction_builder/fee.h>
#include <gmock/gmock.h>
#include <string_safe.h>
#include <tests/allocators_helpers.h>

/* TX BUILDER INTERNALS ******************************************************/

typedef struct cardano_tx_builder_t
{
    cardano_object_t        base;
    cardano_error_t         last_error;
    cardano_builder_state_t state;
} cardano_tx_builder_t;

/* CONSTANTS *****************************************************************/

static const char* ANCHOR_CBOR                 = "827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* PROTOCOL_PARAM_UPDATE       = "b8210018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d018201913881821d81e82185902";
static const char* UTXO_WITH_SCRIPT_ADDRESS    = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a300583911537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d8185182014e4d01000033222220051200120011";
static const char* UTXO_WITH_REF_SCRIPT_PV1    = "82825820bb247abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d8185182014e4d01000033222220051200120011";
static const char* UTXO_WITH_REF_SCRIPT_PV2    = "82825820bb257abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e002a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d8185182024e4d02000033222220051200120011";
static const char* UTXO_WITH_REF_SCRIPT_PV3    = "82825820bb267abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e003a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d8185182034e4d03000033222220051200120011";
static const char* UTXO_WITH_REF_SCRIPT_NATIVE = "82825820bb277abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e003a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d818582282008200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d398538";
static const char* CBOR_DIFFERENT_VAL1         = "82825820027b68d4c11e97d7e065cc2702912cb1a21b6d0e56c6a74dd605889a5561138500a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a00118f32a1581c0b0d621b5c26d0a1fd0893a4b04c19d860296a69ede1fbcfc5179882a1474e46542d30303101";
static const char* CBOR_DIFFERENT_VAL2         = "82825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a63301a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8011a0dff3f6f";
static const char* CBOR_DIFFERENT_VAL3         = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a026679b8a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c420a";
static const char* OUTPUT_CBOR                 = "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011";
static const char* PLUTUS_DATA_CBOR            = "d8799f0102030405ff";
static const char* COSTMDLS_ALL_CBOR           = "a30098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a0298b31a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a01020304";
static const char* SCRIPT_ADDRESS              = "addr1x8phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gt7r0vd4msrxnuwnccdxlhdjar77j6lg0wypcc9uar5d2shskhj42g";
static const char* HASH_HEX                    = "00000000000000000000000000000000000000000000000000000000";
static const char* HASH_HEX1                   = "10000000000000000000000000000000000000000000000000000000";
static const char* SCRIPT_HASH_HEX             = "966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";
static const char* KEY_HASH_GUARDS_BODY_CBOR   = "a400d9010280018002000ed9010281581c00000000000000000000000000000000000000000000000000000000";
static const char* CREDENTIAL_GUARDS_BODY_CBOR = "a400d9010280018002000ed90102828200581c000000000000000000000000000000000000000000000000000000008201581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";
static const char* CHANGE_ADDRESS              = "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg";
static const char* EMPTY_BODY_CBOR             = "a300d901028001800200";
static const char* DIRECT_DEPOSIT_MAP_CBOR     = "a1581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a64541a002dc6c0";
static const char* DIRECT_DEPOSIT_BODY_CBOR    = "a400d9010280018002001819a1581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a64541a002dc6c0";
static const char* BALANCE_INTERVAL_MAP_CBOR   = "a1581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a6454821a000f4240f6";
static const char* BALANCE_INTERVAL_BODY_CBOR  = "a400d901028001800200181aa1581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a6454821a000f4240f6";
static const char* STARTING_INTERVAL_MAP_CBOR  = "a1581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a64541a004c4b40";
static const char* STARTING_INTERVAL_BODY_CBOR = "a400d901028001800200181ba1581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a64541a004c4b40";
static const char* SUB_TX_CBOR                 = "83a20081825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a633010180a0f6";
static const char* OTHER_SUB_TX_CBOR           = "83a200d9010281825820027b68d4c11e97d7e065cc2702912cb1a21b6d0e56c6a74dd605889a55611385000180a0f6";
static const char* OVERLAPPING_SUB_TX_CBOR     = "83a200d9010282825820027b68d4c11e97d7e065cc2702912cb1a21b6d0e56c6a74dd605889a5561138500825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a633010180a0f6";
static const char* SUB_TX_BODY_CBOR            = "a400d90102800180020017d901028183a20081825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a633010180a0f6";
static const char* MISSING_GUARD_ERROR         = "A sub transaction requires a top level guard that the transaction does not carry. You must add it with `cardano_tx_builder_add_guard` before calling `build`.";
static const char* NFT_ASSET_ID_HEX            = "0b0d621b5c26d0a1fd0893a4b04c19d860296a69ede1fbcfc51798824e46542d303031";
static const char* ASSET_ID_HEX                = "0000000000000000000000000000000000000000000000000000000054455854";
static const char* PLUTUS_V1_CBOR              = "82014e4d01000033222220051200120011";
static const char* PLUTUS_V2_CBOR              = "82025908955908920100003233223232323232332232323232323232323232332232323232322223232533532323232325335001101d13357389211e77726f6e67207573616765206f66207265666572656e636520696e7075740001c3232533500221533500221333573466e1c00800408007c407854cd4004840784078d40900114cd4c8d400488888888888802d40044c08526221533500115333533550222350012222002350022200115024213355023320015021001232153353235001222222222222300e00250052133550253200150233355025200100115026320013550272253350011502722135002225335333573466e3c00801c0940904d40b00044c01800c884c09526135001220023333573466e1cd55cea80224000466442466002006004646464646464646464646464646666ae68cdc39aab9d500c480008cccccccccccc88888888888848cccccccccccc00403403002c02802402001c01801401000c008cd405c060d5d0a80619a80b80c1aba1500b33501701935742a014666aa036eb94068d5d0a804999aa80dbae501a35742a01066a02e0446ae85401cccd5406c08dd69aba150063232323333573466e1cd55cea801240004664424660020060046464646666ae68cdc39aab9d5002480008cc8848cc00400c008cd40b5d69aba15002302e357426ae8940088c98c80c0cd5ce01901a01709aab9e5001137540026ae854008c8c8c8cccd5cd19b8735573aa004900011991091980080180119a816bad35742a004605c6ae84d5d1280111931901819ab9c03203402e135573ca00226ea8004d5d09aba2500223263202c33573805c06005426aae7940044dd50009aba1500533501775c6ae854010ccd5406c07c8004d5d0a801999aa80dbae200135742a00460426ae84d5d1280111931901419ab9c02a02c026135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d55cf280089baa00135742a00860226ae84d5d1280211931900d19ab9c01c01e018375a00a6666ae68cdc39aab9d375400a9000100e11931900c19ab9c01a01c016101b132632017335738921035054350001b135573ca00226ea800448c88c008dd6000990009aa80d911999aab9f0012500a233500930043574200460066ae880080608c8c8cccd5cd19b8735573aa004900011991091980080180118061aba150023005357426ae8940088c98c8050cd5ce00b00c00909aab9e5001137540024646464646666ae68cdc39aab9d5004480008cccc888848cccc00401401000c008c8c8c8cccd5cd19b8735573aa0049000119910919800801801180a9aba1500233500f014357426ae8940088c98c8064cd5ce00d80e80b89aab9e5001137540026ae854010ccd54021d728039aba150033232323333573466e1d4005200423212223002004357426aae79400c8cccd5cd19b875002480088c84888c004010dd71aba135573ca00846666ae68cdc3a801a400042444006464c6403666ae7007407c06406005c4d55cea80089baa00135742a00466a016eb8d5d09aba2500223263201533573802e03202626ae8940044d5d1280089aab9e500113754002266aa002eb9d6889119118011bab00132001355018223233335573e0044a010466a00e66442466002006004600c6aae754008c014d55cf280118021aba200301613574200222440042442446600200800624464646666ae68cdc3a800a400046a02e600a6ae84d55cf280191999ab9a3370ea00490011280b91931900819ab9c01201400e00d135573aa00226ea80048c8c8cccd5cd19b875001480188c848888c010014c01cd5d09aab9e500323333573466e1d400920042321222230020053009357426aae7940108cccd5cd19b875003480088c848888c004014c01cd5d09aab9e500523333573466e1d40112000232122223003005375c6ae84d55cf280311931900819ab9c01201400e00d00c00b135573aa00226ea80048c8c8cccd5cd19b8735573aa004900011991091980080180118029aba15002375a6ae84d5d1280111931900619ab9c00e01000a135573ca00226ea80048c8cccd5cd19b8735573aa002900011bae357426aae7940088c98c8028cd5ce00600700409baa001232323232323333573466e1d4005200c21222222200323333573466e1d4009200a21222222200423333573466e1d400d2008233221222222233001009008375c6ae854014dd69aba135744a00a46666ae68cdc3a8022400c4664424444444660040120106eb8d5d0a8039bae357426ae89401c8cccd5cd19b875005480108cc8848888888cc018024020c030d5d0a8049bae357426ae8940248cccd5cd19b875006480088c848888888c01c020c034d5d09aab9e500b23333573466e1d401d2000232122222223005008300e357426aae7940308c98c804ccd5ce00a80b80880800780700680600589aab9d5004135573ca00626aae7940084d55cf280089baa0012323232323333573466e1d400520022333222122333001005004003375a6ae854010dd69aba15003375a6ae84d5d1280191999ab9a3370ea0049000119091180100198041aba135573ca00c464c6401866ae700380400280244d55cea80189aba25001135573ca00226ea80048c8c8cccd5cd19b875001480088c8488c00400cdd71aba135573ca00646666ae68cdc3a8012400046424460040066eb8d5d09aab9e500423263200933573801601a00e00c26aae7540044dd500089119191999ab9a3370ea00290021091100091999ab9a3370ea00490011190911180180218031aba135573ca00846666ae68cdc3a801a400042444004464c6401466ae7003003802001c0184d55cea80089baa0012323333573466e1d40052002200623333573466e1d40092000200623263200633573801001400800626aae74dd5000a4c244004244002921035054310012333333357480024a00c4a00c4a00c46a00e6eb400894018008480044488c0080049400848488c00800c4488004448c8c00400488cc00cc0080080041";
static const char* PLUTUS_V3_CBOR              = "82035908955908920100003233223232323232332232323232323232323232332232323232322223232533532323232325335001101d13357389211e77726f6e67207573616765206f66207265666572656e636520696e7075740001c3232533500221533500221333573466e1c00800408007c407854cd4004840784078d40900114cd4c8d400488888888888802d40044c08526221533500115333533550222350012222002350022200115024213355023320015021001232153353235001222222222222300e00250052133550253200150233355025200100115026320013550272253350011502722135002225335333573466e3c00801c0940904d40b00044c01800c884c09526135001220023333573466e1cd55cea80224000466442466002006004646464646464646464646464646666ae68cdc39aab9d500c480008cccccccccccc88888888888848cccccccccccc00403403002c02802402001c01801401000c008cd405c060d5d0a80619a80b80c1aba1500b33501701935742a014666aa036eb94068d5d0a804999aa80dbae501a35742a01066a02e0446ae85401cccd5406c08dd69aba150063232323333573466e1cd55cea801240004664424660020060046464646666ae68cdc39aab9d5002480008cc8848cc00400c008cd40b5d69aba15002302e357426ae8940088c98c80c0cd5ce01901a01709aab9e5001137540026ae854008c8c8c8cccd5cd19b8735573aa004900011991091980080180119a816bad35742a004605c6ae84d5d1280111931901819ab9c03203402e135573ca00226ea8004d5d09aba2500223263202c33573805c06005426aae7940044dd50009aba1500533501775c6ae854010ccd5406c07c8004d5d0a801999aa80dbae200135742a00460426ae84d5d1280111931901419ab9c02a02c026135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d55cf280089baa00135742a00860226ae84d5d1280211931900d19ab9c01c01e018375a00a6666ae68cdc39aab9d375400a9000100e11931900c19ab9c01a01c016101b132632017335738921035054350001b135573ca00226ea800448c88c008dd6000990009aa80d911999aab9f0012500a233500930043574200460066ae880080608c8c8cccd5cd19b8735573aa004900011991091980080180118061aba150023005357426ae8940088c98c8050cd5ce00b00c00909aab9e5001137540024646464646666ae68cdc39aab9d5004480008cccc888848cccc00401401000c008c8c8c8cccd5cd19b8735573aa0049000119910919800801801180a9aba1500233500f014357426ae8940088c98c8064cd5ce00d80e80b89aab9e5001137540026ae854010ccd54021d728039aba150033232323333573466e1d4005200423212223002004357426aae79400c8cccd5cd19b875002480088c84888c004010dd71aba135573ca00846666ae68cdc3a801a400042444006464c6403666ae7007407c06406005c4d55cea80089baa00135742a00466a016eb8d5d09aba2500223263201533573802e03202626ae8940044d5d1280089aab9e500113754002266aa002eb9d6889119118011bab00132001355018223233335573e0044a010466a00e66442466002006004600c6aae754008c014d55cf280118021aba200301613574200222440042442446600200800624464646666ae68cdc3a800a400046a02e600a6ae84d55cf280191999ab9a3370ea00490011280b91931900819ab9c01201400e00d135573aa00226ea80048c8c8cccd5cd19b875001480188c848888c010014c01cd5d09aab9e500323333573466e1d400920042321222230020053009357426aae7940108cccd5cd19b875003480088c848888c004014c01cd5d09aab9e500523333573466e1d40112000232122223003005375c6ae84d55cf280311931900819ab9c01201400e00d00c00b135573aa00226ea80048c8c8cccd5cd19b8735573aa004900011991091980080180118029aba15002375a6ae84d5d1280111931900619ab9c00e01000a135573ca00226ea80048c8cccd5cd19b8735573aa002900011bae357426aae7940088c98c8028cd5ce00600700409baa001232323232323333573466e1d4005200c21222222200323333573466e1d4009200a21222222200423333573466e1d400d2008233221222222233001009008375c6ae854014dd69aba135744a00a46666ae68cdc3a8022400c4664424444444660040120106eb8d5d0a8039bae357426ae89401c8cccd5cd19b875005480108cc8848888888cc018024020c030d5d0a8049bae357426ae8940248cccd5cd19b875006480088c848888888c01c020c034d5d09aab9e500b23333573466e1d401d2000232122222223005008300e357426aae7940308c98c804ccd5ce00a80b80880800780700680600589aab9d5004135573ca00626aae7940084d55cf280089baa0012323232323333573466e1d400520022333222122333001005004003375a6ae854010dd69aba15003375a6ae84d5d1280191999ab9a3370ea0049000119091180100198041aba135573ca00c464c6401866ae700380400280244d55cea80189aba25001135573ca00226ea80048c8c8cccd5cd19b875001480088c8488c00400cdd71aba135573ca00646666ae68cdc3a8012400046424460040066eb8d5d09aab9e500423263200933573801601a00e00c26aae7540044dd500089119191999ab9a3370ea00290021091100091999ab9a3370ea00490011190911180180218031aba135573ca00846666ae68cdc3a801a400042444004464c6401466ae7003003802001c0184d55cea80089baa0012323333573466e1d40052002200623333573466e1d40092000200623263200633573801001400800626aae74dd5000a4c244004244002921035054310012333333357480024a00c4a00c4a00c46a00e6eb400894018008480044488c0080049400848488c00800c4488004448c8c00400488cc00cc0080080041";
static const char* NATIVE_SCRIPT_CBOR          = "82008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0";
static const char* REWARD_ADDRESS              = "stake_test1uppy2gm2hqzkwc80em4mlat73j4jyqvzhclrvsu72g9xg4q2yweet";
static const char* REWARD_ADDRESS2             = "stake_test1uqehkck0lajq8gr28t9uxnuvgcqrc6070x3k9r8048z8y5gssrtvn";
static const char* SCRIPT_REWARD_ADDRESS       = "stake_test17rphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcljw6kf";
static const char* SCRIPT_REWARD_ADDRESS2      = "stake_test17rphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4ggq9fnfqd";
static const char* DREP_KEY_HASH_CBOR          = "8200581c00000000000000000000000000000000000000000000000000000000";
static const char* DREP_ID                     = "drep15cfxz9exyn5rx0807zvxfrvslrjqfchrd4d47kv9e0f46uedqtc";
static const char* ANCHOR_HASH                 = "26ce09df4e6f64fe5cf248968ab78f4b8a0092580c234d78f68c079c0fce34f0";
static const char* ANCHOR_URL                  = "https://storage.googleapis.com/biglup/Angel_Castillo.jsonld";
static const char* GOVERNANCE_ACTION_ID_CBOR   = "825820000000000000000000000000000000000000000000000000000000000000000003";
static const char* CBOR_YES_WITH_ANCHOR        = "8201827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* GOVERNANCE_ACTION_HEX       = "0000000000000000000000000000000000000000000000000000000000000000";
static const char* WITHDRAWAL_MAP_CBOR         = "a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d005";
static const char* CREDENTIAL_SET_CBOR         = "d90102848200581c000000000000000000000000000000000000000000000000000000008200581c100000000000000000000000000000000000000000000000000000008200581c200000000000000000000000000000000000000000000000000000008200581c30000000000000000000000000000000000000000000000000000000";
static const char* COMITTEE_MEMBERS_MAP_CBOR   = "a48200581c00000000000000000000000000000000000000000000000000000000008200581c10000000000000000000000000000000000000000000000000000000018200581c20000000000000000000000000000000000000000000000000000000028200581c3000000000000000000000000000000000000000000000000000000003";
static const char* CONSTITUTION_CBOR           = "82827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000f6";
static const char* CIP129_BECH32_1             = "gov_action1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqpzklpgpf";
static const char* CBOR_YES_WITHOUT_ANCHOR     = "8201f6";
static const char* HARDFORK_PROPOSALS_CBOR     = "d90102818400581de04245236ab8056760efceebbff57e8cab220182be3e36439e520a64548301825820000000000000000000000000000000000000000000000000000000000000000011820c0082783b68747470733a2f2f73746f726167652e676f6f676c65617069732e636f6d2f6269676c75702f416e67656c5f43617374696c6c6f2e6a736f6e6c64582026ce09df4e6f64fe5cf248968ab78f4b8a0092580c234d78f68c079c0fce34f0";
static const char* SIGNER_KEY_HEX              = "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60";

/**
 * The fee excess tolerated once a transaction is signed, in bytes of transaction. The estimate the fee is computed from
 * and the signed transaction can differ by the header of the witness set collections.
 */
static const int64_t MAX_FEE_EXCESS_IN_BYTES = 3;

/**
 * The fee of the native reference script that requires one signature, with the reference script price per byte set by
 * \ref init_protocol_parameters (15 lovelace): a native script of 32 bytes plus the two bytes of the array that holds
 * the language tag and the script, all of it inside the first pricing tier.
 */
static const uint64_t NATIVE_REFERENCE_SCRIPT_FEE = 34U * 15U;

/* STATIC FUNCTIONS **********************************************************/

static cardano_voting_procedure_t*
new_default_voting_procedure()
{
  cardano_voting_procedure_t* voting_procedure = NULL;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex(CBOR_YES_WITHOUT_ANCHOR, strlen(CBOR_YES_WITHOUT_ANCHOR));
  cardano_error_t             result           = cardano_voting_procedure_from_cbor(reader, &voting_procedure);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return voting_procedure;
};

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

static cardano_utxo_list_t*
new_utxo_list()
{
  cardano_utxo_list_t* list = NULL;

  cardano_error_t error = cardano_utxo_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_t* gai1 = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t* gai2 = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_t* gai3 = create_utxo(CBOR_DIFFERENT_VAL3);

  EXPECT_EQ(cardano_utxo_list_add(list, gai2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(list, gai1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(list, gai3), CARDANO_SUCCESS);

  cardano_utxo_unref(&gai1);
  cardano_utxo_unref(&gai2);
  cardano_utxo_unref(&gai3);

  return list;
};

static cardano_protocol_parameters_t*
init_protocol_parameters()
{
  cardano_protocol_parameters_t* params = NULL;

  cardano_error_t result = cardano_protocol_parameters_new(&params);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_ex_unit_prices_t* ex_unit_prices  = NULL;
  cardano_unit_interval_t*  memory_prices   = NULL;
  cardano_unit_interval_t*  steps_prices    = NULL;
  cardano_unit_interval_t*  script_ref_cost = NULL;

  result = cardano_unit_interval_from_double(0.0577, &memory_prices);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_unit_interval_from_double(0.0000721, &steps_prices);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_unit_interval_from_double(15.0, &script_ref_cost);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_ex_unit_prices_new(memory_prices, steps_prices, &ex_unit_prices);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_min_fee_a(params, 44);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_min_fee_b(params, 155381);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_execution_costs(params, ex_unit_prices);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_ref_script_cost_per_byte(params, script_ref_cost);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_ada_per_utxo_byte(params, 4310U);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_key_deposit(params, 2000000U);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_pool_deposit(params, 2000000U);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_drep_deposit(params, 500000000U);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(COSTMDLS_ALL_CBOR, strlen(COSTMDLS_ALL_CBOR));

  cardano_costmdls_t* costmdls = NULL;
  result                       = cardano_costmdls_from_cbor(reader, &costmdls);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_protocol_parameters_set_cost_models(params, costmdls);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);
  cardano_costmdls_unref(&costmdls);
  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_ex_unit_prices_unref(&ex_unit_prices);

  return params;
}

/**
 * \brief Allocates and initializes a new Cardano provider context.
 */
typedef struct api_context_t
{
    cardano_object_t base;
    char             key[256];
} ref_counted_string_t;

/**
 * \brief Allocates and initializes a new Cardano provider context.
 */
static cardano_provider_impl_t
cardano_provider_impl_new()
{
  cardano_provider_impl_t impl    = { 0 };
  api_context_t*          context = reinterpret_cast<api_context_t*>(malloc(sizeof(api_context_t)));

  if (context != NULL)
  {
    context->base.ref_count     = 1U;
    context->base.deallocator   = _cardano_free;
    context->base.last_error[0] = '\0';

    CARDANO_UNUSED(memset(context->key, 0, sizeof(context->key)));
    CARDANO_UNUSED(cardano_safe_memcpy((void*)&context->key[0], sizeof(context->key), "This is a test key", sizeof(context->key)));

    impl.context = (cardano_object_t*)context;
  }

  impl.post_transaction_to_chain = [](cardano_provider_impl_t*, cardano_transaction_t*, cardano_blake2b_hash_t** tx_id) -> cardano_error_t
  {
    return cardano_blake2b_compute_hash((const byte_t*)"a", 1, 32, tx_id);
  };

  impl.await_transaction_confirmation = [](cardano_provider_impl_t*, cardano_blake2b_hash_t*, uint64_t, bool*) -> cardano_error_t
  {
    return CARDANO_SUCCESS;
  };

  impl.get_parameters = [](cardano_provider_impl_t*, cardano_protocol_parameters_t** param) -> cardano_error_t
  {
    return cardano_protocol_parameters_new(param);
  };

  impl.get_unspent_outputs = [](cardano_provider_impl_t*, cardano_address_t*, cardano_utxo_list_t** utxo_list) -> cardano_error_t
  {
    return cardano_utxo_list_new(utxo_list);
  };

  impl.get_unspent_outputs_with_asset = [](cardano_provider_impl_t*, cardano_address_t*, cardano_asset_id_t*, cardano_utxo_list_t** utxo_list) -> cardano_error_t
  {
    return cardano_utxo_list_new(utxo_list);
  };

  impl.get_unspent_output_by_nft = [](cardano_provider_impl_t*, cardano_asset_id_t*, cardano_utxo_t** utxo) -> cardano_error_t
  {
    static const char*     CBOR   = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c410a";
    cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

    if (reader == NULL)
    {
      return CARDANO_ERROR_GENERIC;
    }

    cardano_error_t result = cardano_utxo_from_cbor(reader, utxo);

    cardano_cbor_reader_unref(&reader);

    return result;
  };

  impl.resolve_unspent_outputs = [](cardano_provider_impl_t*, cardano_transaction_input_set_t*, cardano_utxo_list_t** utxo_list) -> cardano_error_t
  {
    return cardano_utxo_list_new(utxo_list);
  };

  impl.resolve_datum = [](cardano_provider_impl_t*, cardano_blake2b_hash_t*, cardano_plutus_data_t** datum) -> cardano_error_t
  {
    return cardano_plutus_data_new_integer_from_int(0, datum);
  };

  impl.evaluate_transaction = [](cardano_provider_impl_t*, cardano_transaction_t*, cardano_utxo_list_t*, cardano_redeemer_list_t**) -> cardano_error_t
  {
    return CARDANO_SUCCESS;
  };

  impl.get_rewards_balance = [](cardano_provider_impl_t*, cardano_reward_address_t*, uint64_t* balance) -> cardano_error_t
  {
    *balance = 0U;

    return CARDANO_SUCCESS;
  };

  impl.network_magic = CARDANO_NETWORK_MAGIC_MAINNET;

  return impl;
}

/**
 * \brief Allocates and initializes a new Cardano coin_selector context.
 */
static cardano_coin_selector_impl_t
cardano_empty_coin_selector_impl_new()
{
  cardano_coin_selector_impl_t impl = { 0 };

  CARDANO_UNUSED(memset(impl.name, 0, sizeof(impl.name)));
  CARDANO_UNUSED(cardano_safe_memcpy((void*)&impl.name[0], sizeof(impl.name), "Empty Coin Selector", sizeof(impl.name)));

  impl.select = NULL;

  return impl;
}

/**
 * \brief Allocates and initializes a new Cardano tx_evaluator context.
 */
static cardano_tx_evaluator_impl_t
cardano_empty_tx_evaluator_impl_new()
{
  cardano_tx_evaluator_impl_t impl = { 0 };

  CARDANO_UNUSED(memset(impl.name, 0, sizeof(impl.name)));
  CARDANO_UNUSED(cardano_safe_memcpy((void*)&impl.name[0], sizeof(impl.name), "Empty Tx Evaluator", sizeof(impl.name)));

  impl.evaluate = NULL;

  return impl;
}

/**
 * \brief creates a transaction output from a CBOR hex string.
 *
 * \param cbor the CBOR hex string.
 */
static cardano_transaction_output_t*
cardano_tx_out_new(const char* cbor)
{
  cardano_transaction_output_t* output = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &output);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return output;
}

/**
 * Creates a new default instance of the voter.
 * \return A new instance of the voter.
 */
static cardano_voter_t*
new_default_voter()
{
  static const char* CBOR = "8200581c00000000000000000000000000000000000000000000000000000000";

  cardano_voter_t*       voter  = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t        result = cardano_voter_from_cbor(reader, &voter);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return voter;
};

/**
 * Creates a new default instance of the voter.
 * \return A new instance of the voter.
 */
static cardano_voter_t*
new_default_voter2()
{
  static const char* CBOR = "8201581c01000000000000000000000000000000000000000000000000000000";

  cardano_voter_t*       voter  = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t        result = cardano_voter_from_cbor(reader, &voter);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return voter;
};

/**
 * Creates a credential from the given hash hex and type.
 * \param hash_hex the hash hex string.
 * \param type the credential type.
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
 * Encodes the body of the transaction under construction to a CBOR hex string.
 * \param tx_builder the transaction builder.
 * \return The CBOR hex string. The caller must free the returned string.
 */
static char*
encode_body(cardano_tx_builder_t* tx_builder)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_transaction_body_to_cbor(body, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        body_hex = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, body_hex, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return body_hex;
}

/**
 * Creates a sub transaction from its CBOR representation.
 * \param cbor the CBOR hex string of the sub transaction.
 * \return A new instance of the sub transaction.
 */
static cardano_sub_transaction_t*
create_sub_transaction(const char* cbor)
{
  cardano_sub_transaction_t* sub_transaction = NULL;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_sub_transaction_from_cbor(reader, &sub_transaction);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return sub_transaction;
}

/**
 * Gets a borrowed reference to the sub transactions of the transaction under construction.
 * \param tx_builder the transaction builder.
 * \return The sub transaction set of the transaction body, or NULL when it has none.
 */
static cardano_sub_transaction_set_t*
get_sub_transactions(cardano_tx_builder_t* tx_builder)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_sub_transaction_set_t* sub_transactions = cardano_transaction_body_get_sub_transactions(body);
  cardano_sub_transaction_set_unref(&sub_transactions);

  return sub_transactions;
}

/**
 * Gets a borrowed reference to the proposal procedures of the transaction under construction.
 * \param tx_builder the transaction builder.
 * \return The proposal procedures of the transaction body.
 */
static cardano_proposal_procedure_set_t*
get_proposal_procedures(cardano_tx_builder_t* tx_builder)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* proposals = cardano_transaction_body_get_proposal_procedures(body);
  cardano_proposal_procedure_set_unref(&proposals);

  return proposals;
}

/**
 * Encodes the proposal procedures of the transaction under construction to a CBOR hex string.
 * \param tx_builder the transaction builder.
 * \return The CBOR hex string. The caller must free the returned string.
 */
static char*
encode_proposal_procedures(cardano_tx_builder_t* tx_builder)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_proposal_procedure_set_to_cbor(get_proposal_procedures(tx_builder), writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size      = cardano_cbor_writer_get_hex_size(writer);
  char*        proposals_hex = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, proposals_hex, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return proposals_hex;
}

/**
 * Gets the protocol version proposed by a hard fork initiation proposal of the transaction under construction.
 * \param tx_builder the transaction builder.
 * \param index the index of the proposal.
 * \return The proposed protocol version. The caller must release it.
 */
static cardano_protocol_version_t*
get_hardfork_proposal_version(cardano_tx_builder_t* tx_builder, const size_t index)
{
  cardano_proposal_procedure_t*          proposal = NULL;
  cardano_hard_fork_initiation_action_t* action   = NULL;

  EXPECT_EQ(cardano_proposal_procedure_set_get(get_proposal_procedures(tx_builder), index, &proposal), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_proposal_procedure_to_hard_fork_initiation_action(proposal, &action), CARDANO_SUCCESS);

  cardano_protocol_version_t* version = cardano_hard_fork_initiation_action_get_protocol_version(action);

  cardano_hard_fork_initiation_action_unref(&action);
  cardano_proposal_procedure_unref(&proposal);

  return version;
}

/**
 * Encodes a transaction to a CBOR hex string.
 * \param tx the transaction to encode.
 * \return The CBOR hex string. The caller must free the returned string.
 */
static char*
encode_transaction(cardano_transaction_t* tx)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_transaction_to_cbor(tx, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        tx_hex   = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, tx_hex, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return tx_hex;
}

/**
 * Sums the lovelace held by the UTXOs that the transaction spends.
 * \param tx the transaction whose inputs are resolved.
 * \param utxos the UTXOs the inputs were selected from.
 * \return The total lovelace consumed by the inputs of the transaction.
 */
static uint64_t
sum_input_lovelace(cardano_transaction_t* tx, cardano_utxo_list_t* utxos)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  cardano_transaction_input_set_t* inputs = cardano_transaction_body_get_inputs(body);
  cardano_transaction_input_set_unref(&inputs);

  uint64_t total = 0U;

  for (size_t i = 0U; i < cardano_transaction_input_set_get_length(inputs); ++i)
  {
    cardano_transaction_input_t* input = NULL;

    EXPECT_EQ(cardano_transaction_input_set_get(inputs, i, &input), CARDANO_SUCCESS);
    cardano_transaction_input_unref(&input);

    for (size_t j = 0U; j < cardano_utxo_list_get_length(utxos); ++j)
    {
      cardano_utxo_t* utxo = NULL;

      EXPECT_EQ(cardano_utxo_list_get(utxos, j, &utxo), CARDANO_SUCCESS);
      cardano_utxo_unref(&utxo);

      cardano_transaction_input_t* utxo_input = cardano_utxo_get_input(utxo);
      cardano_transaction_input_unref(&utxo_input);

      if (cardano_transaction_input_equals(input, utxo_input))
      {
        cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
        cardano_transaction_output_unref(&output);

        cardano_value_t* value = cardano_transaction_output_get_value(output);
        cardano_value_unref(&value);

        total += (uint64_t)cardano_value_get_coin(value);
      }
    }
  }

  return total;
}

/**
 * Sums the lovelace held by the outputs of a transaction.
 * \param tx the transaction whose outputs are added up.
 * \return The total lovelace locked by the outputs of the transaction.
 */
static uint64_t
sum_output_lovelace(cardano_transaction_t* tx)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  uint64_t total = 0U;

  for (size_t i = 0U; i < cardano_transaction_output_list_get_length(outputs); ++i)
  {
    cardano_transaction_output_t* output = NULL;

    EXPECT_EQ(cardano_transaction_output_list_get(outputs, i, &output), CARDANO_SUCCESS);
    cardano_transaction_output_unref(&output);

    cardano_value_t* value = cardano_transaction_output_get_value(output);
    cardano_value_unref(&value);

    total += (uint64_t)cardano_value_get_coin(value);
  }

  return total;
}

/**
 * Creates a transaction builder that is ready to build, with a change address and spendable UTXOs.
 * \param params the protocol parameters.
 * \param utxos the UTXOs available for coin selection.
 * \return A new instance of the transaction builder.
 */
static cardano_tx_builder_t*
new_funded_tx_builder(cardano_protocol_parameters_t* params, cardano_utxo_list_t* utxos)
{
  cardano_address_t* change_address = nullptr;

  EXPECT_EQ(cardano_address_from_string(CHANGE_ADDRESS, strlen(CHANGE_ADDRESS), &change_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  cardano_address_unref(&change_address);

  return tx_builder;
}

/**
 * Builds the sub transaction of a party that spends one UTXO and pays a value back to the address of that UTXO.
 * \param params the protocol parameters.
 * \param utxo the UTXO the party spends.
 * \param lovelace the lovelace the party pays back to itself.
 * \param nft_quantity the quantity of the NFT the party pays back to itself.
 * \return A new instance of the sub transaction.
 */
static cardano_sub_transaction_t*
build_party_sub_transaction(
  cardano_protocol_parameters_t* params,
  cardano_utxo_t*                utxo,
  const int64_t                  lovelace,
  const int64_t                  nft_quantity)
{
  cardano_sub_tx_builder_t*     builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_transaction_output_t* output          = cardano_utxo_get_output(utxo);
  cardano_address_t*            address         = cardano_transaction_output_get_address(output);
  cardano_value_t*              value           = cardano_value_new_from_coin(lovelace);
  cardano_sub_transaction_t*    sub_transaction = nullptr;

  if (nft_quantity > 0)
  {
    EXPECT_EQ(cardano_value_add_asset_with_id_ex(value, NFT_ASSET_ID_HEX, strlen(NFT_ASSET_ID_HEX), nft_quantity), CARDANO_SUCCESS);
  }

  cardano_sub_tx_builder_add_input(builder, utxo);
  cardano_sub_tx_builder_send_value(builder, address, value);

  EXPECT_EQ(cardano_sub_tx_builder_build(builder, &sub_transaction), CARDANO_SUCCESS);

  cardano_value_unref(&value);
  cardano_address_unref(&address);
  cardano_transaction_output_unref(&output);
  cardano_sub_tx_builder_unref(&builder);

  return sub_transaction;
}

/**
 * Builds the sub transaction of a party that spends one UTXO, pays lovelace back to the address of that UTXO and
 * requires a guard from the top level transaction.
 * \param params the protocol parameters.
 * \param utxo the UTXO the party spends.
 * \param lovelace the lovelace the party pays back to itself.
 * \param required_guard the credential the top level transaction must carry as a guard.
 * \return A new instance of the sub transaction.
 */
static cardano_sub_transaction_t*
build_guarded_party_sub_transaction(
  cardano_protocol_parameters_t* params,
  cardano_utxo_t*                utxo,
  const int64_t                  lovelace,
  cardano_credential_t*          required_guard)
{
  cardano_sub_tx_builder_t*     builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_transaction_output_t* output          = cardano_utxo_get_output(utxo);
  cardano_address_t*            address         = cardano_transaction_output_get_address(output);
  cardano_value_t*              value           = cardano_value_new_from_coin(lovelace);
  cardano_sub_transaction_t*    sub_transaction = nullptr;

  cardano_sub_tx_builder_add_input(builder, utxo);
  cardano_sub_tx_builder_send_value(builder, address, value);
  cardano_sub_tx_builder_require_top_level_guard(builder, required_guard, nullptr);

  EXPECT_EQ(cardano_sub_tx_builder_build(builder, &sub_transaction), CARDANO_SUCCESS);

  cardano_value_unref(&value);
  cardano_address_unref(&address);
  cardano_transaction_output_unref(&output);
  cardano_sub_tx_builder_unref(&builder);

  return sub_transaction;
}

/**
 * Builds the sub transaction of a party whose witness set carries a redeemer, as received from a tool that supports
 * scripts inside sub transactions. It spends one UTXO, references another one and pays lovelace back to the address
 * of the spent UTXO.
 * \param params the protocol parameters.
 * \param utxo the UTXO the party spends.
 * \param reference_utxo the UTXO the party references.
 * \param lovelace the lovelace the party pays back to itself.
 * \param memory the memory units of the redeemer.
 * \param cpu_steps the CPU steps of the redeemer.
 * \return A new instance of the sub transaction.
 */
static cardano_sub_transaction_t*
build_script_party_sub_transaction(
  cardano_protocol_parameters_t* params,
  cardano_utxo_t*                utxo,
  cardano_utxo_t*                reference_utxo,
  const int64_t                  lovelace,
  const uint64_t                 memory,
  const uint64_t                 cpu_steps)
{
  cardano_sub_tx_builder_t*     builder         = cardano_sub_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_transaction_output_t* output          = cardano_utxo_get_output(utxo);
  cardano_address_t*            address         = cardano_transaction_output_get_address(output);
  cardano_value_t*              value           = cardano_value_new_from_coin(lovelace);
  cardano_sub_transaction_t*    sub_transaction = nullptr;
  cardano_redeemer_list_t*      redeemers       = nullptr;
  cardano_redeemer_t*           redeemer        = nullptr;
  cardano_plutus_data_t*        data            = nullptr;
  cardano_ex_units_t*           ex_units        = nullptr;

  cardano_sub_tx_builder_add_input(builder, utxo);
  cardano_sub_tx_builder_add_reference_input(builder, reference_utxo);
  cardano_sub_tx_builder_send_value(builder, address, value);

  EXPECT_EQ(cardano_sub_tx_builder_build(builder, &sub_transaction), CARDANO_SUCCESS);

  cardano_witness_set_t* witness_set = cardano_sub_transaction_get_witness_set(sub_transaction);

  EXPECT_EQ(cardano_redeemer_list_new(&redeemers), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(42, &data), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ex_units_new(memory, cpu_steps, &ex_units), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_new(CARDANO_REDEEMER_TAG_SPEND, 0, data, ex_units, &redeemer), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_list_add(redeemers, redeemer), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_witness_set_set_redeemers(witness_set, redeemers), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&data);
  cardano_ex_units_unref(&ex_units);
  cardano_redeemer_unref(&redeemer);
  cardano_redeemer_list_unref(&redeemers);
  cardano_witness_set_unref(&witness_set);
  cardano_value_unref(&value);
  cardano_address_unref(&address);
  cardano_transaction_output_unref(&output);
  cardano_sub_tx_builder_unref(&builder);

  return sub_transaction;
}

/**
 * Creates a UTXO list with a single UTXO.
 * \param utxo the UTXO to add to the list.
 * \return A new instance of the UTXO list.
 */
static cardano_utxo_list_t*
new_single_utxo_list(cardano_utxo_t* utxo)
{
  cardano_utxo_list_t* list = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(list, utxo), CARDANO_SUCCESS);

  return list;
}

/**
 * Checks whether the transaction spends a UTXO at the top level.
 * \param tx the transaction whose inputs are searched.
 * \param utxo the UTXO to look up.
 * \return true if one of the inputs of the transaction body points to the UTXO.
 */
static bool
transaction_spends(cardano_transaction_t* tx, cardano_utxo_t* utxo)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  cardano_transaction_input_set_t* inputs = cardano_transaction_body_get_inputs(body);
  cardano_transaction_input_set_unref(&inputs);

  cardano_transaction_input_t* utxo_input = cardano_utxo_get_input(utxo);
  cardano_transaction_input_unref(&utxo_input);

  bool is_spent = false;

  for (size_t i = 0U; i < cardano_transaction_input_set_get_length(inputs); ++i)
  {
    cardano_transaction_input_t* input = NULL;

    EXPECT_EQ(cardano_transaction_input_set_get(inputs, i, &input), CARDANO_SUCCESS);
    cardano_transaction_input_unref(&input);

    is_spent = is_spent || cardano_transaction_input_equals(input, utxo_input);
  }

  return is_spent;
}

/**
 * Creates an account balance interval that only bounds the balance from below.
 * \param inclusive_lower_bound the minimum balance in lovelace.
 * \return A new instance of the account balance interval.
 */
static cardano_account_balance_interval_t*
new_lower_bounded_interval(const uint64_t inclusive_lower_bound)
{
  cardano_account_balance_interval_t* interval = NULL;

  EXPECT_EQ(cardano_account_balance_interval_new(&inclusive_lower_bound, NULL, &interval), CARDANO_SUCCESS);

  return interval;
}

/**
 * Creates an account balance interval that requires an exact balance.
 * \param balance the exact balance in lovelace.
 * \return A new instance of the account balance interval.
 */
static cardano_account_balance_interval_t*
new_exact_interval(const uint64_t balance)
{
  cardano_account_balance_interval_t* interval = NULL;

  EXPECT_EQ(cardano_account_balance_interval_new_exact(balance, &interval), CARDANO_SUCCESS);

  return interval;
}

/**
 * \brief The owner of a UTXO: the key it signs with and the enterprise address that key controls.
 */
struct signer_t
{
    cardano_ed25519_private_key_t* private_key;
    cardano_ed25519_public_key_t*  public_key;
    cardano_blake2b_hash_t*        key_hash;
    cardano_address_t*             address;
};

/**
 * Creates a signer from its private key. The signer controls the enterprise address of its key hash.
 * \param private_key_hex the Ed25519 private key of the signer.
 * \return The new signer. The caller must release it with \ref free_signer.
 */
static signer_t
new_signer(const char* private_key_hex)
{
  signer_t signer = { NULL, NULL, NULL, NULL };

  cardano_credential_t*         credential         = NULL;
  cardano_enterprise_address_t* enterprise_address = NULL;

  EXPECT_EQ(cardano_ed25519_private_key_from_normal_hex(private_key_hex, strlen(private_key_hex), &signer.private_key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ed25519_private_key_get_public_key(signer.private_key, &signer.public_key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ed25519_public_key_to_hash(signer.public_key, &signer.key_hash), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_credential_new(signer.key_hash, CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_enterprise_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, credential, &enterprise_address), CARDANO_SUCCESS);

  signer.address = cardano_enterprise_address_to_address(enterprise_address);

  cardano_credential_unref(&credential);
  cardano_enterprise_address_unref(&enterprise_address);

  return signer;
}

/**
 * Releases everything a signer holds.
 * \param signer the signer to release.
 */
static void
free_signer(signer_t& signer)
{
  cardano_ed25519_private_key_unref(&signer.private_key);
  cardano_ed25519_public_key_unref(&signer.public_key);
  cardano_blake2b_hash_unref(&signer.key_hash);
  cardano_address_unref(&signer.address);
}

/**
 * Creates the native script that requires the signature of a signer, as a script reference carries it.
 * \param signer the signer the native script requires.
 * \return A new instance of the script.
 */
static cardano_script_t*
new_native_reference_script(const signer_t& signer)
{
  cardano_script_pubkey_t* script_pubkey = NULL;
  cardano_native_script_t* native_script = NULL;
  cardano_script_t*        script        = NULL;

  EXPECT_EQ(cardano_script_pubkey_new(signer.key_hash, &script_pubkey), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_native_script_new_pubkey(script_pubkey, &native_script), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_script_new_native(native_script, &script), CARDANO_SUCCESS);

  cardano_script_pubkey_unref(&script_pubkey);
  cardano_native_script_unref(&native_script);

  return script;
}

/**
 * Creates a UTXO that holds only lovelace and optionally carries a reference script.
 * \param ordinal a number that makes the transaction id of the UTXO unique.
 * \param address the address that owns the UTXO.
 * \param coin the lovelace held by the UTXO.
 * \param script the reference script carried by the UTXO, or NULL if it carries none.
 * \return A new instance of the UTXO.
 */
static cardano_utxo_t*
new_coin_utxo(const uint64_t ordinal, cardano_address_t* address, const uint64_t coin, cardano_script_t* script)
{
  char hex[65] = { 0 };

  EXPECT_EQ(snprintf(hex, sizeof(hex), "%064llx", (unsigned long long)ordinal), 64);

  cardano_blake2b_hash_t*       id     = NULL;
  cardano_transaction_input_t*  input  = NULL;
  cardano_transaction_output_t* output = NULL;
  cardano_utxo_t*               utxo   = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(hex, 64, &id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_new(id, 0, &input), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_new(address, coin, &output), CARDANO_SUCCESS);

  if (script != NULL)
  {
    EXPECT_EQ(cardano_transaction_output_set_script_ref(output, script), CARDANO_SUCCESS);
  }

  EXPECT_EQ(cardano_utxo_new(input, output, &utxo), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&id);
  cardano_transaction_input_unref(&input);
  cardano_transaction_output_unref(&output);

  return utxo;
}

/**
 * Signs a transaction with the key of a signer and applies its VK witness to it.
 * \param tx the transaction to sign.
 * \param signer the signer of the transaction.
 */
static void
sign_transaction(cardano_transaction_t* tx, const signer_t& signer)
{
  cardano_blake2b_hash_t*      tx_id     = cardano_transaction_get_id(tx);
  cardano_ed25519_signature_t* signature = NULL;
  cardano_vkey_witness_t*      witness   = NULL;
  cardano_vkey_witness_set_t*  witnesses = NULL;

  EXPECT_EQ(cardano_ed25519_private_key_sign(signer.private_key, cardano_blake2b_hash_get_data(tx_id), cardano_blake2b_hash_get_bytes_size(tx_id), &signature), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_vkey_witness_new(signer.public_key, signature, &witness), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_vkey_witness_set_new(&witnesses), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_vkey_witness_set_add(witnesses, witness), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_apply_vkey_witnesses(tx, witnesses), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&tx_id);
  cardano_ed25519_signature_unref(&signature);
  cardano_vkey_witness_unref(&witness);
  cardano_vkey_witness_set_unref(&witnesses);
}

/**
 * Builds a transaction of a signer that mints one token under the native script that requires its signature. The
 * script is not part of the transaction, a reference input supplies it, so the transaction has no redeemers.
 * \param params the protocol parameters.
 * \param signer the signer that funds the transaction and receives the change.
 * \param script the native script of the minting policy.
 * \param reference_utxo the UTXO added as reference input.
 * \param funding_utxo the UTXO that funds the transaction.
 * \return A new instance of the transaction, or NULL if it could not be built.
 */
static cardano_transaction_t*
build_mint_with_reference_input(
  cardano_protocol_parameters_t* params,
  const signer_t&                signer,
  cardano_script_t*              script,
  cardano_utxo_t*                reference_utxo,
  cardano_utxo_t*                funding_utxo)
{
  cardano_utxo_list_t*    utxos      = new_single_utxo_list(funding_utxo);
  cardano_tx_builder_t*   tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_blake2b_hash_t* policy_id  = cardano_script_get_hash(script);
  cardano_asset_name_t*   asset_name = NULL;
  cardano_transaction_t*  tx         = NULL;

  EXPECT_EQ(cardano_asset_name_from_string("TEXT", 4, &asset_name), CARDANO_SUCCESS);

  cardano_tx_builder_set_change_address(tx_builder, signer.address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);
  cardano_tx_builder_add_reference_input(tx_builder, reference_utxo);
  cardano_tx_builder_mint_token(tx_builder, policy_id, asset_name, 1, NULL);

  EXPECT_EQ(cardano_tx_builder_build(tx_builder, &tx), CARDANO_SUCCESS) << cardano_tx_builder_get_last_error(tx_builder);

  cardano_utxo_list_unref(&utxos);
  cardano_tx_builder_unref(&tx_builder);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&asset_name);

  return tx;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_tx_builder_new, canCreateATxBuilder)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Clean up
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_new, returnsErrorOnMemoryAllocationFailure)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act
  for (int i = 0; i < 25; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);
    cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    EXPECT_EQ(builder, nullptr);
  }

  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_set_allocators(malloc, realloc, free);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_coin_selector, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_coin_selector(nullptr, nullptr);
  cardano_tx_builder_set_coin_selector(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_coin_selector, canSetCoinSelector)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_coin_selector_t* selector = NULL;

  EXPECT_EQ(cardano_coin_selector_new(cardano_empty_coin_selector_impl_new(), &selector), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_coin_selector(builder, selector);

  // Assert
  EXPECT_EQ(builder->state.coin_selector, selector);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_coin_selector_unref(&selector);
}

TEST(cardano_tx_builder_set_network_id, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_network_id(nullptr, CARDANO_NETWORK_ID_MAIN_NET);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_network_id, canSetNetworkId)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_network_id(builder, CARDANO_NETWORK_ID_MAIN_NET);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  const cardano_network_id_t* network_id = cardano_transaction_body_get_network_id(body);

  // Assert
  EXPECT_EQ(*network_id, CARDANO_NETWORK_ID_MAIN_NET);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_network_id, returnsErroIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_transaction_unref(&builder->state.transaction);
  builder->state.transaction = NULL;

  // Act
  cardano_tx_builder_set_network_id(builder, CARDANO_NETWORK_ID_MAIN_NET);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_network_id, returnsErroIfMemoryAllocaitonFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_network_id(builder, CARDANO_NETWORK_ID_MAIN_NET);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_tx_evaluator, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_tx_evaluator(nullptr, nullptr);
  cardano_tx_builder_set_tx_evaluator(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_tx_evaluator, canSetTxEvaluator)
{
  // Arrange
  cardano_protocol_parameters_t* params    = init_protocol_parameters();
  cardano_provider_t*            provider  = NULL;
  cardano_tx_evaluator_t*        evaluator = NULL;

  EXPECT_EQ(cardano_tx_evaluator_new(cardano_empty_tx_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_tx_evaluator(builder, evaluator);

  // Assert
  EXPECT_EQ(builder->state.tx_evaluator, evaluator);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_tx_evaluator_unref(&evaluator);
}

TEST(cardano_tx_builder_set_change_address, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_change_address(nullptr, nullptr);
  cardano_tx_builder_set_change_address(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_change_address, canSetChangeAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_address_t* address = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_change_address(builder, address);

  // Assert
  EXPECT_EQ(builder->state.change_address, address);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_set_change_address_ex, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_change_address_ex(nullptr, "", 0);
  cardano_tx_builder_set_change_address_ex(builder, nullptr, 0);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_change_address_ex, canSetChangeAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_address_t* address = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_change_address_ex(builder, "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"));

  // Assert
  EXPECT_STREQ(cardano_address_get_string(builder->state.change_address), cardano_address_get_string(address));

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_set_change_address_ex, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_change_address_ex(builder, "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"));

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_INVALID_ADDRESS_FORMAT);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_collateral_change_address, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_collateral_change_address(nullptr, nullptr);
  cardano_tx_builder_set_collateral_change_address(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_collateral_change_address, canSetCollateralChangeAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_address_t* address = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_collateral_change_address(builder, address);

  // Assert
  EXPECT_EQ(builder->state.collateral_address, address);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_set_collateral_change_address_ex, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_collateral_change_address_ex(nullptr, "", 0);
  cardano_tx_builder_set_collateral_change_address_ex(builder, nullptr, 0);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_collateral_change_address_ex, canSetCollateralChangeAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_address_t* address = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_collateral_change_address_ex(builder, "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"));

  // Assert
  EXPECT_STREQ(cardano_address_get_string(builder->state.collateral_address), cardano_address_get_string(address));

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_set_collateral_change_address_ex, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_collateral_change_address_ex(builder, "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"));

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_INVALID_ADDRESS_FORMAT);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_minimum_fee, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_minimum_fee(nullptr, 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_minimum_fee, canSetMinimumFee)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_minimum_fee(builder, 1000);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_EQ(cardano_transaction_body_get_fee(body), 1000);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_minimum_fee, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_transaction_unref(&builder->state.transaction);
  builder->state.transaction = NULL;

  // Act
  cardano_tx_builder_set_minimum_fee(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_donation, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_donation(nullptr, 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_donation, canSetDonationFee)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_donation(builder, 1000);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_EQ(*cardano_transaction_body_get_donation(body), 1000);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_donation, canUnsetDonation)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_donation(builder, 0);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_EQ(cardano_transaction_body_get_donation(body), (uint64_t*)NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_new, returnsErrorWhenGiveNull)
{
  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(nullptr, nullptr);

  // Assert
  EXPECT_THAT(builder, testing::IsNull());
}

TEST(cardano_tx_builder_set_utxos, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_utxo_list_t* utxos = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_utxos(nullptr, nullptr);
  cardano_tx_builder_set_utxos(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_utxo_list_unref(&utxos);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_utxos, canSetUtxos)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_utxo_list_t* utxos = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_utxos(builder, utxos);

  // Assert
  EXPECT_EQ(builder->state.available_utxos, utxos);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_new, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Assert
  EXPECT_THAT(builder, nullptr);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_collateral_utxos, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_utxo_list_t* utxos = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_collateral_utxos(nullptr, nullptr);
  cardano_tx_builder_set_collateral_utxos(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_utxo_list_unref(&utxos);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_collateral_utxos, canSetCollateralUtxos)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_utxo_list_t* utxos = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_collateral_utxos(builder, utxos);

  // Assert
  EXPECT_EQ(builder->state.collateral_utxos, utxos);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_set_invalid_after, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_invalid_after(nullptr, 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_invalid_after, canSetInvalidAfter)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_invalid_after(builder, 1000);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_EQ(*cardano_transaction_body_get_invalid_after(body), 1000);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_invalid_after, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_transaction_unref(&builder->state.transaction);
  builder->state.transaction = NULL;

  // Act
  cardano_tx_builder_set_invalid_after(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_invalid_after, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_invalid_after(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_invalid_after_ex, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_invalid_after_ex(nullptr, 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_invalid_after_ex, canSetInvalidAfter)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_invalid_after_ex(builder, 1730901968);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_EQ(*cardano_transaction_body_get_invalid_after(body), 139335677);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_invalid_after_ex, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_transaction_unref(&builder->state.transaction);
  builder->state.transaction = NULL;

  // Act
  cardano_tx_builder_set_invalid_after_ex(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_invalid_after_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_invalid_after_ex(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_invalid_before, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_invalid_before(nullptr, 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_invalid_before, canSetInvalidBefore)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_invalid_before(builder, 1000);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_EQ(*cardano_transaction_body_get_invalid_before(body), 1000);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_invalid_before, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_transaction_unref(&builder->state.transaction);
  builder->state.transaction = NULL;

  // Act
  cardano_tx_builder_set_invalid_before(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_invalid_before, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_invalid_before(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_invalid_before_ex, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act
  cardano_tx_builder_set_invalid_before_ex(nullptr, 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_invalid_before_ex, canSetInvalidBefore)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_invalid_before_ex(builder, 1730901968);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_EQ(*cardano_transaction_body_get_invalid_before(body), 139335677);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_invalid_before_ex, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_transaction_unref(&builder->state.transaction);
  builder->state.transaction = NULL;

  cardano_tx_builder_set_invalid_before_ex(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_invalid_before_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_invalid_before_ex(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_add_reference_input, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_utxo_t* utxo = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_reference_input(nullptr, utxo);
  cardano_tx_builder_add_reference_input(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_utxo_unref(&utxo);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_add_reference_input, canAddReferenceInput)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_utxo_t* utxo1 = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_utxo_t* utxo2 = create_utxo(UTXO_WITH_REF_SCRIPT_PV2);
  cardano_utxo_t* utxo3 = create_utxo(UTXO_WITH_REF_SCRIPT_PV3);
  cardano_utxo_t* utxo4 = create_utxo(UTXO_WITH_REF_SCRIPT_NATIVE);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_reference_input(builder, utxo1);
  cardano_tx_builder_add_reference_input(builder, utxo2);
  cardano_tx_builder_add_reference_input(builder, utxo3);
  cardano_tx_builder_add_reference_input(builder, utxo4);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_input_set_t* inputs = cardano_transaction_body_get_reference_inputs(body);
  cardano_transaction_input_set_unref(&inputs);

  // Assert
  EXPECT_EQ(cardano_transaction_input_set_get_length(inputs), 4);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_utxo_unref(&utxo1);
  cardano_utxo_unref(&utxo2);
  cardano_utxo_unref(&utxo3);
  cardano_utxo_unref(&utxo4);
}

TEST(cardano_tx_builder_add_reference_input, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_utxo_t* utxo = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_transaction_unref(&builder->state.transaction);
  builder->state.transaction = NULL;

  // Act
  cardano_tx_builder_add_reference_input(builder, utxo);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_utxo_unref(&utxo);
}

TEST(cardano_tx_builder_add_reference_input, returnsErrorIfReferenceInputsIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_utxo_t* utxo = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_utxo_list_unref(&builder->state.reference_inputs);
  builder->state.reference_inputs = NULL;

  cardano_tx_builder_add_reference_input(builder, utxo);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_utxo_unref(&utxo);
}

TEST(cardano_tx_builder_add_reference_input, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_utxo_t* utxo = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_add_reference_input(builder, utxo);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_utxo_unref(&utxo);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_send_lovelace, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_address_t* address = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_send_lovelace(nullptr, address, 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_send_lovelace, canSendLovelace)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_address_t* address = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_send_lovelace(builder, address, 1000);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  cardano_transaction_output_t* output = NULL;
  EXPECT_EQ(cardano_transaction_output_list_get(outputs, 0, &output), CARDANO_SUCCESS);
  cardano_transaction_output_unref(&output);

  cardano_value_t* value = cardano_transaction_output_get_value(output);
  cardano_value_unref(&value);

  // Assert
  EXPECT_EQ(cardano_value_get_coin(value), 1000);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_send_lovelace, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_address_t* address = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_transaction_unref(&builder->state.transaction);
  builder->state.transaction = NULL;

  // Act
  cardano_tx_builder_send_lovelace(builder, address, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_send_lovelace, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_send_lovelace(builder, nullptr, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_send_lovelace, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_address_t* address = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_send_lovelace(builder, address, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&address);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_send_lovelace_ex, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  const char* address = "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg";

  // Act
  cardano_tx_builder_send_lovelace_ex(nullptr, address, strlen(address), 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_send_lovelace_ex, canSendLovelace)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  const char* address = "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg";

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_send_lovelace_ex(builder, address, strlen(address), 1000);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  cardano_transaction_output_t* output = NULL;
  EXPECT_EQ(cardano_transaction_output_list_get(outputs, 0, &output), CARDANO_SUCCESS);
  cardano_transaction_output_unref(&output);

  cardano_value_t* value = cardano_transaction_output_get_value(output);
  cardano_value_unref(&value);

  // Assert
  EXPECT_EQ(cardano_value_get_coin(value), 1000);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_send_lovelace_ex, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  const char* address = nullptr;

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_send_lovelace_ex(builder, address, 0, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_send_lovelace_ex, returnsErrorIfInvalidAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  const char* address = "invalid_address";

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_send_lovelace_ex(builder, address, strlen(address), 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_send_value, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_address_t* address = NULL;
  cardano_value_t*   value   = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_send_value(nullptr, address, value);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&address);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value, canSendValue)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_address_t* address = NULL;
  cardano_value_t*   value   = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_value_set_coin(value, 1000), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_send_value(builder, address, value);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  cardano_transaction_output_t* output = NULL;
  EXPECT_EQ(cardano_transaction_output_list_get(outputs, 0, &output), CARDANO_SUCCESS);
  cardano_transaction_output_unref(&output);

  cardano_value_t* output_value = cardano_transaction_output_get_value(output);
  cardano_value_unref(&output_value);

  // Assert
  EXPECT_EQ(cardano_value_get_coin(output_value), 1000);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&address);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_address_t* address = NULL;
  cardano_value_t*   value   = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_value_set_coin(value, 1000), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_transaction_unref(&builder->state.transaction);
  builder->state.transaction = NULL;

  cardano_tx_builder_send_value(builder, address, value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
  cardano_address_unref(&address);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_value_t* value = cardano_value_new_zero();

  cardano_address_t* address = nullptr;

  EXPECT_EQ(cardano_value_set_coin(value, 1000), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_send_value(builder, address, value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_address_t* address = NULL;

  cardano_value_t* value = nullptr;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_send_value(builder, address, value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_send_value, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_address_t* address = NULL;
  cardano_value_t*   value   = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_send_value(builder, address, value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&address);
  cardano_value_unref(&value);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_send_value_ex, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  const char*      address = "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg";
  cardano_value_t* value   = cardano_value_new_zero();

  // Act
  cardano_tx_builder_send_value_ex(nullptr, address, strlen(address), value);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value_ex, canSendValue)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  const char*      address = "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg";
  cardano_value_t* value   = cardano_value_new_zero();

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_send_value_ex(builder, address, strlen(address), value);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  cardano_transaction_output_t* output = NULL;
  EXPECT_EQ(cardano_transaction_output_list_get(outputs, 0, &output), CARDANO_SUCCESS);
  cardano_transaction_output_unref(&output);

  cardano_value_t* output_value = cardano_transaction_output_get_value(output);
  cardano_value_unref(&output_value);

  // Assert
  EXPECT_EQ(cardano_value_get_coin(output_value), 0);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);

  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value_ex, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  const char*      address = nullptr;
  cardano_value_t* value   = cardano_value_new_zero();

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_send_value_ex(builder, address, 0, value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value_ex, returnsErrorIfInvalidAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  const char*      address = "invalid_address";
  cardano_value_t* value   = cardano_value_new_zero();

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_send_value_ex(builder, address, strlen(address), value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value_ex, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  const char*      address = "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg";
  cardano_value_t* value   = nullptr;

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_send_value_ex(builder, address, strlen(address), value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_ref(tx_builder);

  // Assert
  EXPECT_THAT(tx_builder, testing::Not((cardano_tx_builder_t*)nullptr));
  EXPECT_EQ(cardano_tx_builder_refcount(tx_builder), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_tx_builder_unref(&tx_builder);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_pad_signer_count, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_tx_builder_pad_signer_count(nullptr, 0);
}

TEST(cardano_tx_builder_pad_signer_count, canSetTheSignerCount)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_pad_signer_count(builder, 10);

  // Assert
  EXPECT_EQ(builder->state.additional_signature_count, 10);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_tx_builder_ref(nullptr);
}

TEST(cardano_tx_builder_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_tx_builder_t* tx_builder = nullptr;

  // Act
  cardano_tx_builder_unref(&tx_builder);
}

TEST(cardano_tx_builder_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_tx_builder_unref((cardano_tx_builder_t**)nullptr);
}

TEST(cardano_tx_builder_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_ref(tx_builder);
  size_t ref_count = cardano_tx_builder_refcount(tx_builder);

  cardano_tx_builder_unref(&tx_builder);
  size_t updated_ref_count = cardano_tx_builder_refcount(tx_builder);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_ref(tx_builder);
  size_t ref_count = cardano_tx_builder_refcount(tx_builder);

  cardano_tx_builder_unref(&tx_builder);
  size_t updated_ref_count = cardano_tx_builder_refcount(tx_builder);

  cardano_tx_builder_unref(&tx_builder);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(tx_builder, (cardano_tx_builder_t*)nullptr);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_tx_builder_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_tx_builder_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_tx_builder_t* tx_builder = nullptr;
  const char*           message    = "This is a test message";

  // Act
  cardano_tx_builder_set_last_error(tx_builder, message);

  // Assert
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "Object is NULL.");
}

TEST(cardano_tx_builder_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  const char* message = nullptr;

  // Act
  cardano_tx_builder_set_last_error(tx_builder, message);

  // Assert
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "");

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_build, returnsErrorIfGivenNull)
{
  // Act
  cardano_tx_builder_t*  tx_builder = nullptr;
  cardano_transaction_t* tx         = nullptr;

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Assert
  EXPECT_EQ(tx, nullptr);
}

TEST(cardano_tx_builder_build, returnsErrorIfTransactionIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_error_t result = cardano_tx_builder_build(tx_builder, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_build, returnsErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_transaction_t* tx = nullptr;

  tx_builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_build, returnsErrorIfChangeAddressNotSet)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_build, returnsErrorIfUtxosNotSet)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, change_address);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&change_address);
}

TEST(cardano_tx_builder_build, canBuildTheTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_build, paysForANativeScriptSuppliedByReferenceIfTheTransactionHasNoRedeemers)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  signer_t                       signer          = new_signer(SIGNER_KEY_HEX);
  cardano_script_t*              script          = new_native_reference_script(signer);
  cardano_utxo_t*                script_utxo     = new_coin_utxo(1U, signer.address, 2000000U, script);
  cardano_utxo_t*                plain_utxo      = new_coin_utxo(1U, signer.address, 2000000U, NULL);
  cardano_utxo_t*                funding_utxo    = new_coin_utxo(2U, signer.address, 20000000U, NULL);
  cardano_utxo_list_t*           resolved_utxos  = new_single_utxo_list(script_utxo);
  uint64_t                       signed_min_fee  = 0U;
  uint64_t                       signed_size_fee = 0U;

  EXPECT_EQ(cardano_utxo_list_add(resolved_utxos, funding_utxo), CARDANO_SUCCESS);

  // Act
  cardano_transaction_t* tx       = build_mint_with_reference_input(params, signer, script, script_utxo, funding_utxo);
  cardano_transaction_t* plain_tx = build_mint_with_reference_input(params, signer, script, plain_utxo, funding_utxo);

  ASSERT_NE(tx, nullptr);
  ASSERT_NE(plain_tx, nullptr);

  sign_transaction(tx, signer);

  // Assert
  cardano_transaction_body_t* body       = cardano_transaction_get_body(tx);
  cardano_transaction_body_t* plain_body = cardano_transaction_get_body(plain_tx);
  cardano_witness_set_t*      witnesses  = cardano_transaction_get_witness_set(tx);
  cardano_redeemer_list_t*    redeemers  = cardano_witness_set_get_redeemers(witnesses);

  EXPECT_EQ(cardano_compute_transaction_fee(tx, resolved_utxos, params, &signed_min_fee), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_compute_min_fee_without_scripts(tx, 155381, 44, &signed_size_fee), CARDANO_SUCCESS);

  const uint64_t fee        = cardano_transaction_body_get_fee(body);
  const int64_t  fee_excess = (int64_t)fee - (int64_t)signed_min_fee;

  EXPECT_EQ(cardano_redeemer_list_get_length(redeemers), 0U);
  EXPECT_EQ(signed_min_fee, signed_size_fee + NATIVE_REFERENCE_SCRIPT_FEE);
  EXPECT_GE(fee, signed_size_fee + NATIVE_REFERENCE_SCRIPT_FEE);
  EXPECT_GE(fee_excess, 0);
  EXPECT_LT(fee_excess, MAX_FEE_EXCESS_IN_BYTES * 44);
  EXPECT_EQ(fee, cardano_transaction_body_get_fee(plain_body) + NATIVE_REFERENCE_SCRIPT_FEE);

  // Cleanup
  cardano_transaction_body_unref(&body);
  cardano_transaction_body_unref(&plain_body);
  cardano_witness_set_unref(&witnesses);
  cardano_redeemer_list_unref(&redeemers);
  cardano_transaction_unref(&tx);
  cardano_transaction_unref(&plain_tx);
  cardano_protocol_parameters_unref(&params);
  cardano_script_unref(&script);
  cardano_utxo_unref(&script_utxo);
  cardano_utxo_unref(&plain_utxo);
  cardano_utxo_unref(&funding_utxo);
  cardano_utxo_list_unref(&resolved_utxos);
  free_signer(signer);
}

TEST(cardano_tx_builder_build, returnsErrorIfBalancingFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_transaction_unref(&tx_builder->state.transaction);
  tx_builder->state.transaction = NULL;

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_lock_lovelace, doesntCrashIfGivenNull)
{
  cardano_tx_builder_lock_lovelace(nullptr, nullptr, 0, nullptr);
}

TEST(cardano_tx_builder_lock_lovelace, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_lovelace(tx_builder, nullptr, 1000, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);
  EXPECT_THAT(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_lock_lovelace, canLockLovelace)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_lovelace(tx_builder, change_address, 1000, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_ILLEGAL_STATE);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_lock_lovelace, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_lovelace(tx_builder, change_address, 1000, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_lock_lovelace_ex, doesntCrashIfGivenNull)
{
  cardano_tx_builder_lock_lovelace_ex(nullptr, nullptr, 0, 0, nullptr);
}

TEST(cardano_tx_builder_lock_lovelace_ex, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_lovelace_ex(tx_builder, nullptr, 0, 1000, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);
  EXPECT_THAT(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_lock_lovelace_ex, canLockLovelace)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_lovelace_ex(tx_builder, "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), 1000, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_ILLEGAL_STATE);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_lock_lovelace_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_lovelace_ex(
    tx_builder,
    "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg",
    strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"),
    1000,
    nullptr);

  const cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ADDRESS_FORMAT);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_lock_value, doesntCrashIfGivenNull)
{
  cardano_tx_builder_lock_value(nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_lock_value, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();
  cardano_value_t*               value          = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_value(tx_builder, nullptr, value, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);
  EXPECT_THAT(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_lock_value, canLockValue)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();
  cardano_value_t*               value          = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_value(tx_builder, change_address, value, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_ILLEGAL_STATE);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_lock_value, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();
  cardano_value_t*               value          = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_value(tx_builder, change_address, value, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_value_unref(&value);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_lock_value_ex, doesntCrashIfGivenNull)
{
  cardano_tx_builder_lock_value_ex(nullptr, nullptr, 0, nullptr, nullptr);
}

TEST(cardano_tx_builder_lock_value_ex, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();
  cardano_value_t*               value          = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_value_ex(tx_builder, nullptr, 0, value, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);
  EXPECT_THAT(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_lock_value_ex, canLockValue)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();
  cardano_value_t*               value          = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_lock_value_ex(tx_builder, "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), value, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_ILLEGAL_STATE);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_add_output, doesntCrashIfGivenNull)
{
  cardano_tx_builder_add_output(nullptr, nullptr);
}

TEST(cardano_tx_builder_add_output, returnsErrorIfOutputIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_add_output(tx_builder, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);
  EXPECT_THAT(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_add_output, canAddOutput)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();
  cardano_transaction_output_t*  output         = cardano_tx_out_new(OUTPUT_CBOR);

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_add_output(tx_builder, output);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  EXPECT_EQ(result, CARDANO_ERROR_BALANCE_INSUFFICIENT);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_BALANCE_INSUFFICIENT);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  cardano_transaction_output_unref(&output);
}

TEST(cardano_tx_builder_add_input, doesntCrashIfGivenNull)
{
  cardano_tx_builder_add_input(nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_add_input, returnsErrorIfInputIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_add_input(tx_builder, nullptr, nullptr, nullptr);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);
  EXPECT_THAT(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_transaction_unref(&tx);
}

TEST(cardano_tx_builder_add_input, canAddInput)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_tx_evaluator_t*        tx_evaluator   = NULL;
  cardano_utxo_t*                utxo           = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_utxo_t*                utxo1          = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_utxo_t*                utxo2          = create_utxo(UTXO_WITH_REF_SCRIPT_PV2);
  cardano_utxo_t*                utxo3          = create_utxo(UTXO_WITH_REF_SCRIPT_PV3);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_plutus_data_t*         datum          = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_from_provider(provider, &tx_evaluator), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_tx_evaluator(tx_builder, tx_evaluator);
  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);
  cardano_tx_builder_set_collateral_change_address(tx_builder, change_address);
  cardano_tx_builder_set_collateral_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_add_reference_input(tx_builder, utxo1);
  cardano_tx_builder_add_reference_input(tx_builder, utxo2);
  cardano_tx_builder_add_reference_input(tx_builder, utxo3);
  cardano_tx_builder_add_input(tx_builder, utxo, redeemer, datum);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);
  EXPECT_THAT(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_ILLEGAL_STATE);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_transaction_unref(&tx);
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&utxo1);
  cardano_utxo_unref(&utxo2);
  cardano_utxo_unref(&utxo3);
  cardano_address_unref(&change_address);
  cardano_plutus_data_unref(&redeemer);
  cardano_plutus_data_unref(&datum);
  cardano_utxo_list_unref(&utxos);
  cardano_tx_evaluator_unref(&tx_evaluator);
}

TEST(cardano_tx_builder_add_input, retursErrorIfMissingCollateralUtxos)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_utxo_t*                utxo           = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_utxo_t*                utxo1          = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_utxo_t*                utxo2          = create_utxo(UTXO_WITH_REF_SCRIPT_PV2);
  cardano_utxo_t*                utxo3          = create_utxo(UTXO_WITH_REF_SCRIPT_PV3);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_plutus_data_t*         datum          = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);
  cardano_tx_builder_set_collateral_change_address(tx_builder, change_address);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_add_reference_input(tx_builder, utxo1);
  cardano_tx_builder_add_reference_input(tx_builder, utxo2);
  cardano_tx_builder_add_reference_input(tx_builder, utxo3);
  cardano_tx_builder_add_input(tx_builder, utxo, redeemer, datum);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);
  EXPECT_THAT(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_transaction_unref(&tx);
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&utxo1);
  cardano_utxo_unref(&utxo2);
  cardano_utxo_unref(&utxo3);
  cardano_address_unref(&change_address);
  cardano_plutus_data_unref(&redeemer);
  cardano_plutus_data_unref(&datum);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_add_input, returnsErrorIfScriptInputIsAddedWithoutRedeemer)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_utxo_t*                utxo           = create_utxo(UTXO_WITH_SCRIPT_ADDRESS);
  cardano_plutus_data_t*         datum          = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string(SCRIPT_ADDRESS, strlen(SCRIPT_ADDRESS), &change_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_add_input(tx_builder, utxo, nullptr, datum);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);
  EXPECT_THAT(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_transaction_unref(&tx);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&change_address);
  cardano_plutus_data_unref(&datum);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_add_input, returnsErrorOnMemoryAllocationFail)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_utxo_t*                utxo           = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_plutus_data_t*         datum          = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  for (int i = 0; i < 9; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    cardano_tx_builder_set_change_address(tx_builder, change_address);
    cardano_tx_builder_set_utxos(tx_builder, utxos);

    reset_allocators_run_count();
    set_malloc_limit(i);

    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_add_input(tx_builder, utxo, redeemer, datum);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_utxo_unref(&utxo);
  cardano_address_unref(&change_address);
  cardano_plutus_data_unref(&redeemer);
  cardano_plutus_data_unref(&datum);
  cardano_utxo_list_unref(&utxos);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_build, doesntCrashOnMemoryAllocationFail)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_utxo_t*                utxo           = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_utxo_t*                utxo1          = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_utxo_t*                utxo2          = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_utxo_t*                utxo3          = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_plutus_data_t*         datum          = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);

  for (int i = 0; i < 1024; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    cardano_tx_builder_set_change_address(tx_builder, change_address);
    cardano_tx_builder_set_utxos(tx_builder, utxos);
    cardano_tx_builder_add_input(tx_builder, utxo, redeemer, datum);
    cardano_tx_builder_add_reference_input(tx_builder, utxo1);
    cardano_tx_builder_add_reference_input(tx_builder, utxo2);
    cardano_tx_builder_add_reference_input(tx_builder, utxo3);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_transaction_t* tx     = nullptr;
    cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

    cardano_tx_builder_unref(&tx_builder);

    cardano_transaction_unref(&tx);
    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&utxo1);
  cardano_utxo_unref(&utxo2);
  cardano_utxo_unref(&utxo3);
  cardano_address_unref(&change_address);
  cardano_plutus_data_unref(&redeemer);
  cardano_plutus_data_unref(&datum);
  cardano_utxo_list_unref(&utxos);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_metadata, doesntCrashIfGivenNull)
{
  cardano_tx_builder_set_metadata(nullptr, 0, nullptr);
}

TEST(cardano_tx_builder_set_metadata, returnsErrorIfMetadataIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_metadata(tx_builder, 0, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_metadata, canSetMetadata)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_metadatum_t* metadata = NULL;

  EXPECT_EQ(cardano_metadatum_new_string("TEST", 4, &metadata), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_metadata(tx_builder, 0, metadata);

  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(tx_builder->state.transaction);
  cardano_witness_set_unref(&witnesses);

  cardano_auxiliary_data_t* aux_data = cardano_transaction_get_auxiliary_data(tx_builder->state.transaction);
  cardano_auxiliary_data_unref(&aux_data);

  cardano_transaction_metadata_t* tx_metadata = cardano_auxiliary_data_get_transaction_metadata(aux_data);
  cardano_transaction_metadata_unref(&tx_metadata);

  cardano_metadatum_t* metadata_out = NULL;
  EXPECT_EQ(cardano_transaction_metadata_get(tx_metadata, 0, &metadata_out), CARDANO_SUCCESS);

  cardano_metadatum_unref(&metadata_out);

  // Assert
  EXPECT_EQ(metadata_out, metadata);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_metadatum_unref(&metadata);
}

TEST(cardano_tx_builder_set_metadata, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_metadatum_t* metadata = NULL;

  EXPECT_EQ(cardano_metadatum_new_string("TEST", 4, &metadata), CARDANO_SUCCESS);

  for (int i = 0; i < 5; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_set_metadata(tx_builder, 0, metadata);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_metadatum_unref(&metadata);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_metadata_ex, doesntCrashIfGivenNull)
{
  cardano_tx_builder_set_metadata_ex(nullptr, 0, nullptr, 0);
}

TEST(cardano_tx_builder_set_metadata_ex, returnsErrorIfMetadataIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_metadata_ex(tx_builder, 0, nullptr, 0);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_metadata_ex, canSetMetadata)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_set_metadata_ex(tx_builder, 0, "{ \"name\": \"test\" }", strlen("{ \"name\": \"test\" }"));

  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(tx_builder->state.transaction);
  cardano_witness_set_unref(&witnesses);

  cardano_auxiliary_data_t* aux_data = cardano_transaction_get_auxiliary_data(tx_builder->state.transaction);
  cardano_auxiliary_data_unref(&aux_data);

  cardano_transaction_metadata_t* tx_metadata = cardano_auxiliary_data_get_transaction_metadata(aux_data);
  cardano_transaction_metadata_unref(&tx_metadata);

  cardano_metadatum_t* metadata_out = NULL;
  EXPECT_EQ(cardano_transaction_metadata_get(tx_metadata, 0, &metadata_out), CARDANO_SUCCESS);

  cardano_metadatum_unref(&metadata_out);

  cardano_metadatum_kind_t kind;

  // Assert
  EXPECT_EQ(cardano_metadatum_get_kind(metadata_out, &kind), CARDANO_SUCCESS);
  EXPECT_EQ(kind, CARDANO_METADATUM_KIND_MAP);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_metadata_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  for (int i = 0; i < 16; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_set_metadata_ex(tx_builder, 0, "{ \"name\": \"test\" }", strlen("{ \"name\": \"test\" }"));

    // Assert
    EXPECT_NE(tx_builder->last_error, CARDANO_SUCCESS);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_mint_token, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_mint_token(nullptr, nullptr, nullptr, 0, nullptr);
  cardano_tx_builder_mint_token(tx_builder, nullptr, nullptr, 0, nullptr);
  cardano_tx_builder_mint_token(tx_builder, (cardano_blake2b_hash_t*)"", nullptr, 0, nullptr);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_mint_token, canSentMintToken)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_protocol_parameters();
  cardano_provider_t*            provider   = NULL;
  cardano_asset_name_t*          asset_name = NULL;
  cardano_blake2b_hash_t*        policy_id  = NULL;
  cardano_blake2b_hash_t*        policy_id1 = NULL;
  cardano_plutus_data_t*         redeemer   = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_asset_name_from_string("TEXT", 4, &asset_name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX1, strlen(HASH_HEX1), &policy_id1), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_transaction_t* tx = nullptr;
  cardano_tx_builder_mint_token(tx_builder, policy_id, asset_name, 4, redeemer);
  cardano_tx_builder_mint_token(tx_builder, policy_id, asset_name, 4, redeemer);
  cardano_tx_builder_mint_token(tx_builder, policy_id1, asset_name, 5, NULL);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_multi_asset_t* mint = cardano_transaction_body_get_mint(body);
  cardano_multi_asset_unref(&mint);

  int64_t quantity = 0;
  EXPECT_EQ(cardano_multi_asset_get(mint, policy_id, asset_name, &quantity), CARDANO_SUCCESS);

  int64_t quantity2 = 0;
  EXPECT_EQ(cardano_multi_asset_get(mint, policy_id1, asset_name, &quantity2), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(quantity, 4);
  EXPECT_EQ(quantity2, 5);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_transaction_unref(&tx);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_blake2b_hash_unref(&policy_id1);
  cardano_plutus_data_unref(&redeemer);
}

TEST(cardano_tx_builder_mint_token, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_protocol_parameters();
  cardano_provider_t*            provider   = NULL;
  cardano_asset_name_t*          asset_name = NULL;
  cardano_blake2b_hash_t*        policy_id  = NULL;
  cardano_plutus_data_t*         redeemer   = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_asset_name_from_string("TEXT", 4, &asset_name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_id), CARDANO_SUCCESS);

  for (int i = 0; i < 14; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_mint_token(tx_builder, policy_id, asset_name, 4, redeemer);
    cardano_tx_builder_mint_token(tx_builder, policy_id, asset_name, 4, NULL);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_plutus_data_unref(&redeemer);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_mint_token_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_mint_token_ex(nullptr, nullptr, 0, nullptr, 0, 0, nullptr);
  cardano_tx_builder_mint_token_ex(tx_builder, nullptr, 0, nullptr, 0, 0, nullptr);
  tx_builder->last_error = CARDANO_SUCCESS;
  cardano_tx_builder_mint_token_ex(tx_builder, "1", 1, nullptr, 0, 0, nullptr);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_mint_token_ex, canSentMintToken)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_protocol_parameters();
  cardano_provider_t*            provider   = NULL;
  cardano_plutus_data_t*         redeemer   = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_asset_name_t*          asset_name = NULL;
  cardano_blake2b_hash_t*        policy_id  = NULL;

  EXPECT_EQ(cardano_asset_name_from_string("TEXT", 4, &asset_name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_id), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_transaction_t* tx = nullptr;
  cardano_tx_builder_mint_token_ex(tx_builder, HASH_HEX, strlen(HASH_HEX), "54455854", strlen("54455854"), 4, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_multi_asset_t* mint = cardano_transaction_body_get_mint(body);
  cardano_multi_asset_unref(&mint);

  int64_t quantity = 0;
  EXPECT_EQ(cardano_multi_asset_get(mint, policy_id, asset_name, &quantity), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(quantity, 4);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_transaction_unref(&tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_tx_builder_mint_token_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_protocol_parameters();
  cardano_provider_t*            provider   = NULL;
  cardano_plutus_data_t*         redeemer   = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_asset_name_t*          asset_name = NULL;
  cardano_blake2b_hash_t*        policy_id  = NULL;

  EXPECT_EQ(cardano_asset_name_from_string("TEXT", 4, &asset_name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_id), CARDANO_SUCCESS);

  for (int i = 0; i < 18; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_mint_token_ex(tx_builder, HASH_HEX, strlen(HASH_HEX), "54455854", strlen("54455854"), 4, redeemer);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_plutus_data_unref(&redeemer);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_mint_token_with_id, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_mint_token_with_id(nullptr, nullptr, 0, nullptr);
  cardano_tx_builder_mint_token_with_id(tx_builder, nullptr, 0, nullptr);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_mint_token_with_id, canSentMintToken)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_protocol_parameters();
  cardano_provider_t*            provider   = NULL;
  cardano_plutus_data_t*         redeemer   = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_asset_id_t*            asset_id   = NULL;
  cardano_asset_name_t*          asset_name = NULL;
  cardano_blake2b_hash_t*        policy_id  = NULL;

  EXPECT_EQ(cardano_asset_name_from_string("TEXT", 4, &asset_name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_id), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_asset_id_from_hex(ASSET_ID_HEX, strlen(ASSET_ID_HEX), &asset_id), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_transaction_t* tx = nullptr;
  cardano_tx_builder_mint_token_with_id(tx_builder, asset_id, 4, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_multi_asset_t* mint = cardano_transaction_body_get_mint(body);
  cardano_multi_asset_unref(&mint);

  int64_t quantity = 0;
  EXPECT_EQ(cardano_multi_asset_get(mint, policy_id, asset_name, &quantity), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(quantity, 4);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_transaction_unref(&tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_id_unref(&asset_id);
}

TEST(cardano_tx_builder_mint_token_with_id_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_mint_token_with_id_ex(nullptr, nullptr, 0, 0, nullptr);
  cardano_tx_builder_mint_token_with_id_ex(tx_builder, nullptr, 0, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_mint_token_with_id_ex, canSentMintToken)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_protocol_parameters();
  cardano_provider_t*            provider   = NULL;
  cardano_plutus_data_t*         redeemer   = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_asset_id_t*            asset_id   = NULL;
  cardano_asset_name_t*          asset_name = NULL;
  cardano_blake2b_hash_t*        policy_id  = NULL;

  EXPECT_EQ(cardano_asset_name_from_string("TEXT", 4, &asset_name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_id), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_asset_id_from_hex(ASSET_ID_HEX, strlen(ASSET_ID_HEX), &asset_id), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_transaction_t* tx = nullptr;
  cardano_tx_builder_mint_token_with_id_ex(tx_builder, ASSET_ID_HEX, strlen(ASSET_ID_HEX), 4, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_multi_asset_t* mint = cardano_transaction_body_get_mint(body);
  cardano_multi_asset_unref(&mint);

  int64_t quantity = 0;
  EXPECT_EQ(cardano_multi_asset_get(mint, policy_id, asset_name, &quantity), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(quantity, 4);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_transaction_unref(&tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_id_unref(&asset_id);
}

TEST(cardano_tx_builder_mint_token_with_id_ex, returnsErrorOnMemoryAllocationFail)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_protocol_parameters();
  cardano_provider_t*            provider   = NULL;
  cardano_plutus_data_t*         redeemer   = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_asset_id_t*            asset_id   = NULL;
  cardano_asset_name_t*          asset_name = NULL;
  cardano_blake2b_hash_t*        policy_id  = NULL;

  EXPECT_EQ(cardano_asset_name_from_string("TEXT", 4, &asset_name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_id), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_asset_id_from_hex(ASSET_ID_HEX, strlen(ASSET_ID_HEX), &asset_id), CARDANO_SUCCESS);

  for (int i = 0; i < 19; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_mint_token_with_id_ex(tx_builder, ASSET_ID_HEX, strlen(ASSET_ID_HEX), 4, redeemer);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_plutus_data_unref(&redeemer);
  cardano_asset_id_unref(&asset_id);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_add_signer, doesntCrashIfGivenNull)
{
  cardano_tx_builder_add_signer(nullptr, nullptr);
}

TEST(cardano_tx_builder_add_signer, returnsErrorIfSignerIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_signer(tx_builder, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_add_signer, canAddSigner)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_provider_t*            provider    = NULL;
  cardano_blake2b_hash_t*        signing_key = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &signing_key), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_signer(tx_builder, signing_key);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_blake2b_hash_set_t* signers = cardano_transaction_body_get_required_signers(body);

  cardano_blake2b_hash_t* signer = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_set_get(signers, 0, &signer), CARDANO_SUCCESS);

  // Assert
  EXPECT_TRUE(cardano_blake2b_hash_equals(signer, signing_key));

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_blake2b_hash_set_unref(&signers);
  cardano_blake2b_hash_unref(&signing_key);
  cardano_blake2b_hash_unref(&signer);
}

TEST(cardano_tx_builder_add_signer, addingTheSameSignerTwiceKeepsASingleGuard)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_blake2b_hash_t*        signing_key = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &signing_key), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_signer(tx_builder, signing_key);
  cardano_tx_builder_add_signer(tx_builder, signing_key);
  cardano_tx_builder_add_signer_ex(tx_builder, HASH_HEX, strlen(HASH_HEX));

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_guard_set_t* guards = cardano_transaction_body_get_guards(body);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_NE(guards, nullptr);
  EXPECT_EQ(cardano_guard_set_get_length(guards), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_guard_set_unref(&guards);
  cardano_blake2b_hash_unref(&signing_key);
}

TEST(cardano_tx_builder_add_signer, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_provider_t*            provider    = NULL;
  cardano_blake2b_hash_t*        signing_key = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &signing_key), CARDANO_SUCCESS);

  for (int i = 0; i < 3; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_add_signer(tx_builder, signing_key);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_blake2b_hash_unref(&signing_key);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_add_signer_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_add_signer_ex(nullptr, nullptr, 0);
  cardano_tx_builder_add_signer_ex(tx_builder, nullptr, 0);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_add_signer_ex, canAddSigner)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_provider_t*            provider    = NULL;
  cardano_blake2b_hash_t*        signing_key = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &signing_key), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_signer_ex(tx_builder, HASH_HEX, strlen(HASH_HEX));

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_blake2b_hash_set_t* signers = cardano_transaction_body_get_required_signers(body);

  cardano_blake2b_hash_t* signer = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_set_get(signers, 0, &signer), CARDANO_SUCCESS);

  // Assert
  EXPECT_TRUE(cardano_blake2b_hash_equals(signer, signing_key));

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_blake2b_hash_set_unref(&signers);
  cardano_blake2b_hash_unref(&signing_key);
  cardano_blake2b_hash_unref(&signer);
}

TEST(cardano_tx_builder_add_signer_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_provider_t*            provider    = NULL;
  cardano_blake2b_hash_t*        signing_key = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &signing_key), CARDANO_SUCCESS);

  for (int i = 0; i < 6; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_add_signer_ex(tx_builder, HASH_HEX, strlen(HASH_HEX));

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_blake2b_hash_unref(&signing_key);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_add_guard, doesntCrashIfGivenNull)
{
  cardano_tx_builder_add_guard(nullptr, nullptr);
}

TEST(cardano_tx_builder_add_guard, returnsErrorIfGuardIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_guard(tx_builder, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_add_guard, keyHashGuardMatchesAddSigner)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_blake2b_hash_t*        signing_key = NULL;
  cardano_credential_t*          guard       = create_credential(HASH_HEX, CARDANO_CREDENTIAL_TYPE_KEY_HASH);

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &signing_key), CARDANO_SUCCESS);

  cardano_tx_builder_t* guard_builder  = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_t* signer_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_guard(guard_builder, guard);
  cardano_tx_builder_add_signer(signer_builder, signing_key);

  char* guard_body_hex  = encode_body(guard_builder);
  char* signer_body_hex = encode_body(signer_builder);

  cardano_transaction_body_t* body = cardano_transaction_get_body(guard_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_blake2b_hash_set_t* signers = cardano_transaction_body_get_required_signers(body);
  cardano_blake2b_hash_t*     signer  = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_set_get(signers, 0, &signer), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(guard_builder->last_error, CARDANO_SUCCESS);
  EXPECT_STREQ(guard_body_hex, signer_body_hex);
  EXPECT_STREQ(guard_body_hex, KEY_HASH_GUARDS_BODY_CBOR);
  EXPECT_TRUE(cardano_blake2b_hash_equals(signer, signing_key));

  // Cleanup
  cardano_tx_builder_unref(&guard_builder);
  cardano_tx_builder_unref(&signer_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_blake2b_hash_set_unref(&signers);
  cardano_blake2b_hash_unref(&signing_key);
  cardano_blake2b_hash_unref(&signer);
  cardano_credential_unref(&guard);
  free(guard_body_hex);
  free(signer_body_hex);
}

TEST(cardano_tx_builder_add_guard, canAddScriptHashGuardAfterSigner)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_credential_t*          guard  = create_credential(SCRIPT_HASH_HEX, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_signer_ex(tx_builder, HASH_HEX, strlen(HASH_HEX));
  cardano_tx_builder_add_guard(tx_builder, guard);

  char* body_hex = encode_body(tx_builder);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_guard_set_t*  guards       = cardano_transaction_body_get_guards(body);
  cardano_credential_t* second_guard = nullptr;

  EXPECT_EQ(cardano_guard_set_get(guards, 1, &second_guard), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_guard_set_get_length(guards), 2);
  EXPECT_TRUE(cardano_credential_equals(second_guard, guard));
  EXPECT_STREQ(body_hex, CREDENTIAL_GUARDS_BODY_CBOR);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_guard_set_unref(&guards);
  cardano_credential_unref(&second_guard);
  cardano_credential_unref(&guard);
  free(body_hex);
}

TEST(cardano_tx_builder_add_guard, addingTheSameGuardTwiceKeepsASingleGuard)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_credential_t*          guard  = create_credential(SCRIPT_HASH_HEX, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_guard(tx_builder, guard);
  cardano_tx_builder_add_guard(tx_builder, guard);
  cardano_tx_builder_add_guard_ex(tx_builder, SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_guard_set_t* guards = cardano_transaction_body_get_guards(body);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_NE(guards, nullptr);
  EXPECT_EQ(cardano_guard_set_get_length(guards), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_guard_set_unref(&guards);
  cardano_credential_unref(&guard);
}

TEST(cardano_tx_builder_add_guard, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_credential_t*          guard  = create_credential(SCRIPT_HASH_HEX, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);

  for (int i = 0; i < 2; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_add_guard(tx_builder, guard);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_credential_unref(&guard);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_add_guard_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_add_guard_ex(nullptr, nullptr, 0, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);
  cardano_tx_builder_add_guard_ex(tx_builder, nullptr, 0, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);

  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_add_guard_ex, keyHashGuardMatchesAddSignerEx)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* guard_builder  = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_t* signer_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_guard_ex(guard_builder, HASH_HEX, strlen(HASH_HEX), CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  cardano_tx_builder_add_signer_ex(signer_builder, HASH_HEX, strlen(HASH_HEX));

  char* guard_body_hex  = encode_body(guard_builder);
  char* signer_body_hex = encode_body(signer_builder);

  // Assert
  EXPECT_THAT(guard_builder->last_error, CARDANO_SUCCESS);
  EXPECT_STREQ(guard_body_hex, signer_body_hex);
  EXPECT_STREQ(guard_body_hex, KEY_HASH_GUARDS_BODY_CBOR);

  // Cleanup
  cardano_tx_builder_unref(&guard_builder);
  cardano_tx_builder_unref(&signer_builder);
  cardano_protocol_parameters_unref(&params);

  free(guard_body_hex);
  free(signer_body_hex);
}

TEST(cardano_tx_builder_add_guard_ex, canAddScriptHashGuard)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_guard_ex(tx_builder, HASH_HEX, strlen(HASH_HEX), CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  cardano_tx_builder_add_guard_ex(tx_builder, SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);

  char* body_hex = encode_body(tx_builder);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_guard_set_t*      guards = cardano_transaction_body_get_guards(body);
  cardano_credential_t*     guard  = nullptr;
  cardano_credential_type_t type   = CARDANO_CREDENTIAL_TYPE_KEY_HASH;

  EXPECT_EQ(cardano_guard_set_get(guards, 1, &guard), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_credential_get_type(guard, &type), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_guard_set_get_length(guards), 2);
  EXPECT_EQ(type, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);
  EXPECT_STREQ(cardano_credential_get_hash_hex(guard), SCRIPT_HASH_HEX);
  EXPECT_STREQ(body_hex, CREDENTIAL_GUARDS_BODY_CBOR);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_guard_set_unref(&guards);
  cardano_credential_unref(&guard);
  free(body_hex);
}

TEST(cardano_tx_builder_add_guard_ex, reportsInvalidHexWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_guard_ex(tx_builder, "abc", 3, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);
  cardano_tx_builder_add_guard_ex(tx_builder, SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_guard_set_t* guards = cardano_transaction_body_get_guards(body);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_INVALID_BLAKE2B_HASH_SIZE);
  EXPECT_EQ(tx, nullptr);
  EXPECT_EQ(guards, nullptr);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_add_guard_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  for (int i = 0; i < 5; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_add_guard_ex(tx_builder, SCRIPT_HASH_HEX, strlen(SCRIPT_HASH_HEX), CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_add_datum, doesntCrashIfGivenNull)
{
  cardano_tx_builder_add_datum(nullptr, nullptr);
}

TEST(cardano_tx_builder_add_datum, returnsErrorIfDatumIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_datum(tx_builder, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_add_datum, canAddDatum)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_plutus_data_t* datum = create_plutus_data(PLUTUS_DATA_CBOR);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_datum(tx_builder, datum);

  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(tx_builder->state.transaction);
  cardano_witness_set_unref(&witnesses);

  cardano_plutus_data_set_t* data = cardano_witness_set_get_plutus_data(witnesses);
  cardano_plutus_data_set_unref(&data);

  cardano_plutus_data_t* datum_out = nullptr;

  EXPECT_EQ(cardano_plutus_data_set_get(data, 0, &datum_out), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(datum_out, datum);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_plutus_data_unref(&datum);
  cardano_plutus_data_unref(&datum_out);
}

TEST(cardano_tx_builder_add_datum, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_plutus_data_t* datum = create_plutus_data(PLUTUS_DATA_CBOR);

  for (int i = 0; i < 3; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_add_datum(tx_builder, datum);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_plutus_data_unref(&datum);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_add_script, doesntCrashIfGivenNull)
{
  cardano_tx_builder_add_script(nullptr, nullptr);
}

TEST(cardano_tx_builder_add_script, returnsErrorIfScriptIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_script(tx_builder, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_add_script, canAddScript)
{
  // Arrange
  cardano_protocol_parameters_t* params       = init_protocol_parameters();
  cardano_provider_t*            provider     = NULL;
  cardano_script_t*              scriptV1     = create_script(PLUTUS_V1_CBOR);
  cardano_script_t*              scriptV2     = create_script(PLUTUS_V2_CBOR);
  cardano_script_t*              scriptV3     = create_script(PLUTUS_V3_CBOR);
  cardano_script_t*              scriptNative = create_script(NATIVE_SCRIPT_CBOR);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_script(tx_builder, scriptV1);
  cardano_tx_builder_add_script(tx_builder, scriptV2);
  cardano_tx_builder_add_script(tx_builder, scriptV3);
  cardano_tx_builder_add_script(tx_builder, scriptNative);

  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(tx_builder->state.transaction);
  cardano_witness_set_unref(&witnesses);

  cardano_plutus_v1_script_set_t* scripts = cardano_witness_set_get_plutus_v1_scripts(witnesses);
  cardano_plutus_v1_script_set_unref(&scripts);
  EXPECT_EQ(cardano_plutus_v1_script_set_get_length(scripts), 1);

  cardano_plutus_v2_script_set_t* scriptsV2 = cardano_witness_set_get_plutus_v2_scripts(witnesses);
  cardano_plutus_v2_script_set_unref(&scriptsV2);
  EXPECT_EQ(cardano_plutus_v2_script_set_get_length(scriptsV2), 1);

  cardano_plutus_v3_script_set_t* scriptsV3 = cardano_witness_set_get_plutus_v3_scripts(witnesses);
  cardano_plutus_v3_script_set_unref(&scriptsV3);
  EXPECT_EQ(cardano_plutus_v3_script_set_get_length(scriptsV3), 1);

  cardano_native_script_set_t* scriptsNative = cardano_witness_set_get_native_scripts(witnesses);
  cardano_native_script_set_unref(&scriptsNative);
  EXPECT_EQ(cardano_native_script_set_get_length(scriptsNative), 1);

  // Assert
  EXPECT_TRUE(tx_builder->state.has_plutus_v1);
  EXPECT_TRUE(tx_builder->state.has_plutus_v2);
  EXPECT_TRUE(tx_builder->state.has_plutus_v3);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_script_unref(&scriptV1);
  cardano_script_unref(&scriptV2);
  cardano_script_unref(&scriptV3);
  cardano_script_unref(&scriptNative);
}

TEST(cardano_tx_builder_add_script, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_script_t* script = create_script(PLUTUS_V1_CBOR);

  for (int i = 0; i < 3; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_add_script(tx_builder, script);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_script_unref(&script);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_withdraw_rewards, doesntCrashIfGivenNull)
{
  cardano_tx_builder_withdraw_rewards(nullptr, nullptr, 0, nullptr);
}

TEST(cardano_tx_builder_withdraw_rewards, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_withdraw_rewards(tx_builder, nullptr, 0, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_withdraw_rewards, returnsErrorIfRewardAmountLessThanZero)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_reward_address_t*      reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_withdraw_rewards(tx_builder, reward_address, -1, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_tx_builder_withdraw_rewards, canWithdrawRewards)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  cardano_provider_t*            provider        = NULL;
  cardano_reward_address_t*      reward_address  = nullptr;
  cardano_reward_address_t*      script_reward   = nullptr;
  cardano_reward_address_t*      reward_address2 = nullptr;
  cardano_reward_address_t*      script_reward2  = nullptr;
  cardano_plutus_data_t*         redeemer        = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_reward_address_from_bech32(SCRIPT_REWARD_ADDRESS, strlen(SCRIPT_REWARD_ADDRESS), &script_reward), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS2, strlen(REWARD_ADDRESS2), &reward_address2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_reward_address_from_bech32(SCRIPT_REWARD_ADDRESS2, strlen(SCRIPT_REWARD_ADDRESS2), &script_reward2), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_withdraw_rewards(tx_builder, reward_address, 1000, redeemer);
  cardano_tx_builder_withdraw_rewards(tx_builder, reward_address2, 1001, NULL);
  cardano_tx_builder_withdraw_rewards(tx_builder, script_reward, 1002, redeemer);
  cardano_tx_builder_withdraw_rewards(tx_builder, script_reward2, 1003, NULL);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_withdrawal_map_t* withdrawals = cardano_transaction_body_get_withdrawals(body);
  cardano_withdrawal_map_unref(&withdrawals);

  uint64_t withdrawal1 = 0;
  uint64_t withdrawal2 = 0;
  uint64_t withdrawal3 = 0;
  uint64_t withdrawal4 = 0;

  EXPECT_EQ(cardano_withdrawal_map_get(withdrawals, reward_address, &withdrawal1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_withdrawal_map_get(withdrawals, reward_address2, &withdrawal2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_withdrawal_map_get(withdrawals, script_reward, &withdrawal3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_withdrawal_map_get(withdrawals, script_reward2, &withdrawal4), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(withdrawal1, 1000);
  EXPECT_EQ(withdrawal2, 1001);
  EXPECT_EQ(withdrawal3, 1002);
  EXPECT_EQ(withdrawal4, 1003);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_reward_address_unref(&script_reward);
  cardano_reward_address_unref(&reward_address2);
  cardano_reward_address_unref(&script_reward2);
  cardano_plutus_data_unref(&redeemer);
}

TEST(cardano_tx_builder_withdraw_rewards, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  cardano_provider_t*            provider        = NULL;
  cardano_reward_address_t*      reward_address  = nullptr;
  cardano_reward_address_t*      script_reward   = nullptr;
  cardano_reward_address_t*      reward_address2 = nullptr;
  cardano_reward_address_t*      script_reward2  = nullptr;
  cardano_plutus_data_t*         redeemer        = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_reward_address_from_bech32(SCRIPT_REWARD_ADDRESS, strlen(SCRIPT_REWARD_ADDRESS), &script_reward), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS2, strlen(REWARD_ADDRESS2), &reward_address2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_reward_address_from_bech32(SCRIPT_REWARD_ADDRESS2, strlen(SCRIPT_REWARD_ADDRESS2), &script_reward2), CARDANO_SUCCESS);

  for (int i = 0; i < 45; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_withdraw_rewards(tx_builder, reward_address, 1000, redeemer);
    cardano_tx_builder_withdraw_rewards(tx_builder, reward_address2, 1001, NULL);
    cardano_tx_builder_withdraw_rewards(tx_builder, script_reward, 1002, redeemer);
    cardano_tx_builder_withdraw_rewards(tx_builder, script_reward2, 1003, NULL);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_reward_address_unref(&script_reward);
  cardano_reward_address_unref(&reward_address2);
  cardano_reward_address_unref(&script_reward2);
  cardano_plutus_data_unref(&redeemer);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_withdraw_rewards_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_withdraw_rewards_ex(nullptr, nullptr, 0, 0, nullptr);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_withdraw_rewards_ex(tx_builder, nullptr, 0, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_withdraw_rewards_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_withdraw_rewards_ex(tx_builder, REWARD_ADDRESS, 1, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_withdraw_rewards_ex, canWithdrawRewards)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_withdraw_rewards_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 1000, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_withdrawal_map_t* withdrawals = cardano_transaction_body_get_withdrawals(body);
  cardano_withdrawal_map_unref(&withdrawals);

  uint64_t withdrawal = 0;

  EXPECT_EQ(cardano_withdrawal_map_get(withdrawals, reward_address, &withdrawal), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(withdrawal, 1000);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_plutus_data_unref(&redeemer);
}

TEST(cardano_tx_builder_add_direct_deposit, doesntCrashIfGivenNull)
{
  cardano_tx_builder_add_direct_deposit(nullptr, nullptr, 0);
}

TEST(cardano_tx_builder_add_direct_deposit, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_direct_deposit(tx_builder, nullptr, 3000000);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_add_direct_deposit, returnsErrorIfAmountIsZero)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_reward_address_t*      reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_direct_deposit(tx_builder, reward_address, 0);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "Direct deposit amount must be greater than zero.");

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_tx_builder_add_direct_deposit, canAddDirectDeposit)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_reward_address_t*      reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_direct_deposit(tx_builder, reward_address, 3000000);

  char* body_hex = encode_body(tx_builder);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_direct_deposit_map_t* deposits = cardano_transaction_body_get_direct_deposits(body);
  cardano_direct_deposit_map_unref(&deposits);

  uint64_t amount = 0;

  EXPECT_EQ(cardano_direct_deposit_map_get(deposits, reward_address, &amount), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_direct_deposit_map_get_length(deposits), 1);
  EXPECT_EQ(amount, 3000000);
  EXPECT_STREQ(body_hex, DIRECT_DEPOSIT_BODY_CBOR);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  free(body_hex);
}

TEST(cardano_tx_builder_add_direct_deposit, accumulatesDepositsToTheSameRewardAccount)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_reward_address_t*      reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_direct_deposit(tx_builder, reward_address, 1000000);
  cardano_tx_builder_add_direct_deposit(tx_builder, reward_address, 2000000);

  char* body_hex = encode_body(tx_builder);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_STREQ(body_hex, DIRECT_DEPOSIT_BODY_CBOR);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  free(body_hex);
}

TEST(cardano_tx_builder_add_direct_deposit, reportsOverflowWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_utxo_list_t*           utxos          = new_utxo_list();
  cardano_reward_address_t*      reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, utxos);

  // Act
  cardano_tx_builder_add_direct_deposit(tx_builder, reward_address, UINT64_MAX);
  cardano_tx_builder_add_direct_deposit(tx_builder, reward_address, 1);
  cardano_tx_builder_add_direct_deposit(tx_builder, reward_address, 1);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "Direct deposit amount overflows.");
  EXPECT_EQ(tx, nullptr);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_add_direct_deposit, bodyWithoutDirectDepositsKeepsItsEncoding)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  char* body_hex = encode_body(tx_builder);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_STREQ(body_hex, EMPTY_BODY_CBOR);
  EXPECT_EQ(cardano_transaction_body_get_direct_deposits(body), nullptr);
  EXPECT_EQ(cardano_transaction_body_get_account_balance_intervals(body), nullptr);
  EXPECT_EQ(cardano_transaction_body_get_starting_account_balance_intervals(body), nullptr);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  free(body_hex);
}

TEST(cardano_tx_builder_add_direct_deposit, buildsABalancedTransactionThatFundsTheDeposit)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_utxo_list_t*           utxos          = new_utxo_list();
  cardano_reward_address_t*      reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, utxos);

  // Act
  cardano_tx_builder_send_lovelace_ex(tx_builder, CHANGE_ADDRESS, strlen(CHANGE_ADDRESS), 2000000);
  cardano_tx_builder_add_direct_deposit(tx_builder, reward_address, 3000000);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(tx, nullptr);

  bool is_balanced = false;

  EXPECT_EQ(cardano_is_transaction_balanced(tx, utxos, params, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  const uint64_t fee = cardano_transaction_body_get_fee(body);

  EXPECT_GT(fee, 0U);
  EXPECT_EQ(sum_input_lovelace(tx, utxos), sum_output_lovelace(tx) + fee + 3000000U);

  char* tx_hex = encode_transaction(tx);

  EXPECT_NE(strstr(tx_hex, (std::string("1819") + DIRECT_DEPOSIT_MAP_CBOR).c_str()), nullptr);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  free(tx_hex);
}

TEST(cardano_tx_builder_add_direct_deposit, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_reward_address_t*      reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_add_direct_deposit(tx_builder, reward_address, 3000000);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    // Assert
    if (tx_builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_tx_builder_unref(&tx_builder);
  }

  EXPECT_TRUE(succeeded);

  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_tx_builder_add_direct_deposit_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_add_direct_deposit_ex(nullptr, nullptr, 0, 0);
  cardano_tx_builder_add_direct_deposit_ex(tx_builder, nullptr, 0, 3000000);

  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_add_direct_deposit_ex, canAddDirectDeposit)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_direct_deposit_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 3000000);

  char* body_hex = encode_body(tx_builder);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_STREQ(body_hex, DIRECT_DEPOSIT_BODY_CBOR);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  free(body_hex);
}

TEST(cardano_tx_builder_add_direct_deposit_ex, reportsInvalidRewardAddressWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_utxo_list_t*           utxos  = new_utxo_list();

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, utxos);

  // Act
  cardano_tx_builder_add_direct_deposit_ex(tx_builder, "invalid", strlen("invalid"), 3000000);
  cardano_tx_builder_add_direct_deposit_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 3000000);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_EQ(result, tx_builder->last_error);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "Failed to parse reward address.");
  EXPECT_EQ(tx, nullptr);
  EXPECT_EQ(cardano_transaction_body_get_direct_deposits(body), nullptr);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_add_direct_deposit_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_add_direct_deposit_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 3000000);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    // Assert
    if (tx_builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
      cardano_transaction_body_unref(&body);

      EXPECT_EQ(cardano_transaction_body_get_direct_deposits(body), nullptr);
    }

    cardano_tx_builder_unref(&tx_builder);
  }

  EXPECT_TRUE(succeeded);

  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_add_account_balance_interval, doesntCrashIfGivenNull)
{
  cardano_tx_builder_add_account_balance_interval(nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_add_account_balance_interval, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t*      params   = init_protocol_parameters();
  cardano_account_balance_interval_t* interval = new_lower_bounded_interval(1000000);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_account_balance_interval(tx_builder, nullptr, interval);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_tx_builder_add_account_balance_interval, returnsErrorIfIntervalIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_reward_address_t*      reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_account_balance_interval(tx_builder, reward_address, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_tx_builder_add_account_balance_interval, canAddInterval)
{
  // Arrange
  cardano_protocol_parameters_t*      params         = init_protocol_parameters();
  cardano_account_balance_interval_t* interval       = new_lower_bounded_interval(1000000);
  cardano_reward_address_t*           reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_account_balance_interval(tx_builder, reward_address, interval);

  char* body_hex = encode_body(tx_builder);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_account_balance_intervals_map_t* intervals = cardano_transaction_body_get_account_balance_intervals(body);
  cardano_account_balance_intervals_map_unref(&intervals);

  cardano_account_balance_interval_t* stored = nullptr;

  EXPECT_EQ(cardano_account_balance_intervals_map_get(intervals, reward_address, &stored), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_account_balance_intervals_map_get_length(intervals), 1);
  EXPECT_EQ(stored, interval);
  EXPECT_EQ(cardano_transaction_body_get_starting_account_balance_intervals(body), nullptr);
  EXPECT_STREQ(body_hex, BALANCE_INTERVAL_BODY_CBOR);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_account_balance_interval_unref(&stored);
  cardano_account_balance_interval_unref(&interval);
  cardano_reward_address_unref(&reward_address);
  free(body_hex);
}

TEST(cardano_tx_builder_add_account_balance_interval, replacesTheIntervalOfTheSameRewardAccount)
{
  // Arrange
  cardano_protocol_parameters_t*      params         = init_protocol_parameters();
  cardano_account_balance_interval_t* previous       = new_exact_interval(1);
  cardano_account_balance_interval_t* interval       = new_lower_bounded_interval(1000000);
  cardano_reward_address_t*           reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_account_balance_interval(tx_builder, reward_address, previous);
  cardano_tx_builder_add_account_balance_interval(tx_builder, reward_address, interval);

  char* body_hex = encode_body(tx_builder);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_STREQ(body_hex, BALANCE_INTERVAL_BODY_CBOR);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_account_balance_interval_unref(&previous);
  cardano_account_balance_interval_unref(&interval);
  cardano_reward_address_unref(&reward_address);
  free(body_hex);
}

TEST(cardano_tx_builder_add_account_balance_interval, buildsATransactionThatCarriesTheInterval)
{
  // Arrange
  cardano_protocol_parameters_t*      params         = init_protocol_parameters();
  cardano_utxo_list_t*                utxos          = new_utxo_list();
  cardano_account_balance_interval_t* interval       = new_lower_bounded_interval(1000000);
  cardano_reward_address_t*           reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, utxos);

  // Act
  cardano_tx_builder_send_lovelace_ex(tx_builder, CHANGE_ADDRESS, strlen(CHANGE_ADDRESS), 2000000);
  cardano_tx_builder_add_account_balance_interval(tx_builder, reward_address, interval);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(tx, nullptr);

  bool is_balanced = false;

  EXPECT_EQ(cardano_is_transaction_balanced(tx, utxos, params, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  char* tx_hex = encode_transaction(tx);

  EXPECT_NE(strstr(tx_hex, (std::string("181a") + BALANCE_INTERVAL_MAP_CBOR).c_str()), nullptr);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_account_balance_interval_unref(&interval);
  cardano_reward_address_unref(&reward_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  free(tx_hex);
}

TEST(cardano_tx_builder_add_account_balance_interval, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*      params         = init_protocol_parameters();
  cardano_account_balance_interval_t* interval       = new_lower_bounded_interval(1000000);
  cardano_reward_address_t*           reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_add_account_balance_interval(tx_builder, reward_address, interval);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    // Assert
    if (tx_builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_tx_builder_unref(&tx_builder);
  }

  EXPECT_TRUE(succeeded);

  cardano_protocol_parameters_unref(&params);

  cardano_account_balance_interval_unref(&interval);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_tx_builder_add_account_balance_interval_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t*      params   = init_protocol_parameters();
  cardano_account_balance_interval_t* interval = new_lower_bounded_interval(1000000);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_add_account_balance_interval_ex(nullptr, nullptr, 0, nullptr);
  cardano_tx_builder_add_account_balance_interval_ex(tx_builder, nullptr, 0, interval);

  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_tx_builder_add_account_balance_interval_ex, returnsErrorIfIntervalIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_account_balance_interval_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_add_account_balance_interval_ex, canAddInterval)
{
  // Arrange
  cardano_protocol_parameters_t*      params   = init_protocol_parameters();
  cardano_account_balance_interval_t* interval = new_lower_bounded_interval(1000000);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_account_balance_interval_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), interval);

  char* body_hex = encode_body(tx_builder);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_STREQ(body_hex, BALANCE_INTERVAL_BODY_CBOR);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_account_balance_interval_unref(&interval);
  free(body_hex);
}

TEST(cardano_tx_builder_add_account_balance_interval_ex, reportsInvalidRewardAddressWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t*      params   = init_protocol_parameters();
  cardano_utxo_list_t*                utxos    = new_utxo_list();
  cardano_account_balance_interval_t* interval = new_lower_bounded_interval(1000000);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, utxos);

  // Act
  cardano_tx_builder_add_account_balance_interval_ex(tx_builder, "invalid", strlen("invalid"), interval);
  cardano_tx_builder_add_account_balance_interval_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), interval);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_EQ(result, tx_builder->last_error);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "Failed to parse reward address.");
  EXPECT_EQ(tx, nullptr);
  EXPECT_EQ(cardano_transaction_body_get_account_balance_intervals(body), nullptr);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_account_balance_interval_unref(&interval);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_add_account_balance_interval_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*      params   = init_protocol_parameters();
  cardano_account_balance_interval_t* interval = new_lower_bounded_interval(1000000);

  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_add_account_balance_interval_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), interval);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    // Assert
    if (tx_builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
      cardano_transaction_body_unref(&body);

      EXPECT_EQ(cardano_transaction_body_get_account_balance_intervals(body), nullptr);
    }

    cardano_tx_builder_unref(&tx_builder);
  }

  EXPECT_TRUE(succeeded);

  cardano_protocol_parameters_unref(&params);

  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_tx_builder_add_starting_account_balance_interval, doesntCrashIfGivenNull)
{
  cardano_tx_builder_add_starting_account_balance_interval(nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_add_starting_account_balance_interval, returnsErrorIfRewardAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t*      params   = init_protocol_parameters();
  cardano_account_balance_interval_t* interval = new_exact_interval(5000000);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_starting_account_balance_interval(tx_builder, nullptr, interval);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_tx_builder_add_starting_account_balance_interval, returnsErrorIfIntervalIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_reward_address_t*      reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_starting_account_balance_interval(tx_builder, reward_address, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_tx_builder_add_starting_account_balance_interval, canAddInterval)
{
  // Arrange
  cardano_protocol_parameters_t*      params         = init_protocol_parameters();
  cardano_account_balance_interval_t* interval       = new_exact_interval(5000000);
  cardano_reward_address_t*           reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_starting_account_balance_interval(tx_builder, reward_address, interval);

  char* body_hex = encode_body(tx_builder);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_account_balance_intervals_map_t* intervals = cardano_transaction_body_get_starting_account_balance_intervals(body);
  cardano_account_balance_intervals_map_unref(&intervals);

  cardano_account_balance_interval_t* stored = nullptr;

  EXPECT_EQ(cardano_account_balance_intervals_map_get(intervals, reward_address, &stored), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_account_balance_intervals_map_get_length(intervals), 1);
  EXPECT_EQ(stored, interval);
  EXPECT_EQ(cardano_transaction_body_get_account_balance_intervals(body), nullptr);
  EXPECT_STREQ(body_hex, STARTING_INTERVAL_BODY_CBOR);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_account_balance_interval_unref(&stored);
  cardano_account_balance_interval_unref(&interval);
  cardano_reward_address_unref(&reward_address);
  free(body_hex);
}

TEST(cardano_tx_builder_add_starting_account_balance_interval, replacesTheIntervalOfTheSameRewardAccount)
{
  // Arrange
  cardano_protocol_parameters_t*      params         = init_protocol_parameters();
  cardano_account_balance_interval_t* previous       = new_exact_interval(1);
  cardano_account_balance_interval_t* interval       = new_exact_interval(5000000);
  cardano_reward_address_t*           reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_starting_account_balance_interval(tx_builder, reward_address, previous);
  cardano_tx_builder_add_starting_account_balance_interval(tx_builder, reward_address, interval);

  char* body_hex = encode_body(tx_builder);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_STREQ(body_hex, STARTING_INTERVAL_BODY_CBOR);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_account_balance_interval_unref(&previous);
  cardano_account_balance_interval_unref(&interval);
  cardano_reward_address_unref(&reward_address);
  free(body_hex);
}

TEST(cardano_tx_builder_add_starting_account_balance_interval, buildsATransactionThatCarriesTheInterval)
{
  // Arrange
  cardano_protocol_parameters_t*      params         = init_protocol_parameters();
  cardano_utxo_list_t*                utxos          = new_utxo_list();
  cardano_account_balance_interval_t* interval       = new_exact_interval(5000000);
  cardano_reward_address_t*           reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, utxos);

  // Act
  cardano_tx_builder_send_lovelace_ex(tx_builder, CHANGE_ADDRESS, strlen(CHANGE_ADDRESS), 2000000);
  cardano_tx_builder_add_starting_account_balance_interval(tx_builder, reward_address, interval);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(tx, nullptr);

  bool is_balanced = false;

  EXPECT_EQ(cardano_is_transaction_balanced(tx, utxos, params, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  char* tx_hex = encode_transaction(tx);

  EXPECT_NE(strstr(tx_hex, (std::string("181b") + STARTING_INTERVAL_MAP_CBOR).c_str()), nullptr);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_account_balance_interval_unref(&interval);
  cardano_reward_address_unref(&reward_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
  free(tx_hex);
}

TEST(cardano_tx_builder_add_starting_account_balance_interval, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*      params         = init_protocol_parameters();
  cardano_account_balance_interval_t* interval       = new_exact_interval(5000000);
  cardano_reward_address_t*           reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_add_starting_account_balance_interval(tx_builder, reward_address, interval);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    // Assert
    if (tx_builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
    }

    cardano_tx_builder_unref(&tx_builder);
  }

  EXPECT_TRUE(succeeded);

  cardano_protocol_parameters_unref(&params);

  cardano_account_balance_interval_unref(&interval);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_tx_builder_add_starting_account_balance_interval_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t*      params   = init_protocol_parameters();
  cardano_account_balance_interval_t* interval = new_exact_interval(5000000);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_add_starting_account_balance_interval_ex(nullptr, nullptr, 0, nullptr);
  cardano_tx_builder_add_starting_account_balance_interval_ex(tx_builder, nullptr, 0, interval);

  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_tx_builder_add_starting_account_balance_interval_ex, returnsErrorIfIntervalIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_starting_account_balance_interval_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_add_starting_account_balance_interval_ex, canAddInterval)
{
  // Arrange
  cardano_protocol_parameters_t*      params   = init_protocol_parameters();
  cardano_account_balance_interval_t* interval = new_exact_interval(5000000);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_starting_account_balance_interval_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), interval);

  char* body_hex = encode_body(tx_builder);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_STREQ(body_hex, STARTING_INTERVAL_BODY_CBOR);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_account_balance_interval_unref(&interval);
  free(body_hex);
}

TEST(cardano_tx_builder_add_starting_account_balance_interval_ex, reportsInvalidRewardAddressWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t*      params   = init_protocol_parameters();
  cardano_utxo_list_t*                utxos    = new_utxo_list();
  cardano_account_balance_interval_t* interval = new_exact_interval(5000000);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, utxos);

  // Act
  cardano_tx_builder_add_starting_account_balance_interval_ex(tx_builder, "invalid", strlen("invalid"), interval);
  cardano_tx_builder_add_starting_account_balance_interval_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), interval);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);
  EXPECT_EQ(result, tx_builder->last_error);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "Failed to parse reward address.");
  EXPECT_EQ(tx, nullptr);
  EXPECT_EQ(cardano_transaction_body_get_starting_account_balance_intervals(body), nullptr);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_account_balance_interval_unref(&interval);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_add_starting_account_balance_interval_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*      params   = init_protocol_parameters();
  cardano_account_balance_interval_t* interval = new_exact_interval(5000000);

  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_add_starting_account_balance_interval_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), interval);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    // Assert
    if (tx_builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;
    }
    else
    {
      cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
      cardano_transaction_body_unref(&body);

      EXPECT_EQ(cardano_transaction_body_get_starting_account_balance_intervals(body), nullptr);
    }

    cardano_tx_builder_unref(&tx_builder);
  }

  EXPECT_TRUE(succeeded);

  cardano_protocol_parameters_unref(&params);

  cardano_account_balance_interval_unref(&interval);
}

TEST(cardano_tx_builder_add_sub_transaction, doesntCrashIfGivenNull)
{
  cardano_tx_builder_add_sub_transaction(nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_add_sub_transaction, returnsErrorIfSubTransactionIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_utxos = new_utxo_list();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_sub_transaction(tx_builder, nullptr, resolved_utxos);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "Sub transaction is NULL.");

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_utxo_list_unref(&resolved_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, returnsErrorIfResolvedUtxosIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  cardano_sub_transaction_t*     sub_transaction = create_sub_transaction(SUB_TX_CBOR);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_sub_transaction(tx_builder, sub_transaction, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "Resolved UTXOs list is NULL.");

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
}

TEST(cardano_tx_builder_add_sub_transaction, canAddSubTransactions)
{
  // Arrange
  cardano_protocol_parameters_t* params           = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_utxos   = new_utxo_list();
  cardano_sub_transaction_t*     sub_transaction1 = create_sub_transaction(SUB_TX_CBOR);
  cardano_sub_transaction_t*     sub_transaction2 = create_sub_transaction(OTHER_SUB_TX_CBOR);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_sub_transaction(tx_builder, sub_transaction1, resolved_utxos);
  cardano_tx_builder_add_sub_transaction(tx_builder, sub_transaction2, resolved_utxos);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_SUCCESS);

  cardano_sub_transaction_set_t* sub_transactions = get_sub_transactions(tx_builder);

  EXPECT_EQ(cardano_sub_transaction_set_get_length(sub_transactions), 2U);
  EXPECT_EQ(cardano_utxo_list_get_length(tx_builder->state.sub_transaction_inputs), 2U);
  EXPECT_EQ(cardano_utxo_list_get_length(tx_builder->state.sub_transaction_reference_inputs), 0U);
  EXPECT_EQ(cardano_utxo_list_get_length(tx_builder->state.pre_selected_inputs), 0U);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction1);
  cardano_sub_transaction_unref(&sub_transaction2);
  cardano_utxo_list_unref(&resolved_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, preservesTheBytesAndTheIdOfTheSubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_utxos  = new_utxo_list();
  cardano_sub_transaction_t*     sub_transaction = create_sub_transaction(SUB_TX_CBOR);
  cardano_blake2b_hash_t*        id              = cardano_sub_transaction_get_id(sub_transaction);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_sub_transaction(tx_builder, sub_transaction, resolved_utxos);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_SUCCESS);

  cardano_sub_transaction_t* embedded = nullptr;

  EXPECT_EQ(cardano_sub_transaction_set_find_by_id(get_sub_transactions(tx_builder), id, &embedded), CARDANO_SUCCESS);
  EXPECT_EQ(embedded, sub_transaction);

  cardano_blake2b_hash_t* embedded_id = cardano_sub_transaction_get_id(embedded);

  EXPECT_TRUE(cardano_blake2b_hash_equals(embedded_id, id));

  char* body_hex = encode_body(tx_builder);

  EXPECT_STREQ(body_hex, SUB_TX_BODY_CBOR);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_sub_transaction_unref(&embedded);
  cardano_blake2b_hash_unref(&id);
  cardano_blake2b_hash_unref(&embedded_id);
  cardano_utxo_list_unref(&resolved_utxos);

  free(body_hex);
}

TEST(cardano_tx_builder_add_sub_transaction, bodyWithoutSubTransactionsKeepsItsEncoding)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  char* body_hex = encode_body(tx_builder);

  // Assert
  EXPECT_STREQ(body_hex, EMPTY_BODY_CBOR);
  EXPECT_EQ(get_sub_transactions(tx_builder), nullptr);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  free(body_hex);
}

TEST(cardano_tx_builder_add_sub_transaction, reportsADuplicatedSubTransactionWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_utxos  = new_utxo_list();
  cardano_sub_transaction_t*     sub_transaction = create_sub_transaction(SUB_TX_CBOR);
  cardano_sub_transaction_t*     same_id         = create_sub_transaction(SUB_TX_CBOR);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_sub_transaction(tx_builder, sub_transaction, resolved_utxos);
  cardano_tx_builder_add_sub_transaction(tx_builder, same_id, resolved_utxos);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "Sub transaction is already part of the transaction.");
  EXPECT_EQ(tx, nullptr);
  EXPECT_EQ(cardano_sub_transaction_set_get_length(get_sub_transactions(tx_builder)), 1U);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_sub_transaction_unref(&same_id);
  cardano_utxo_list_unref(&resolved_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, reportsAnInputSpentByAnotherSubTransactionWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params           = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_utxos   = new_utxo_list();
  cardano_sub_transaction_t*     sub_transaction1 = create_sub_transaction(SUB_TX_CBOR);
  cardano_sub_transaction_t*     sub_transaction2 = create_sub_transaction(OVERLAPPING_SUB_TX_CBOR);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_sub_transaction(tx_builder, sub_transaction1, resolved_utxos);
  cardano_tx_builder_add_sub_transaction(tx_builder, sub_transaction2, resolved_utxos);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "Sub transaction spends an input that is already spent by another sub transaction.");
  EXPECT_EQ(tx, nullptr);
  EXPECT_EQ(cardano_sub_transaction_set_get_length(get_sub_transactions(tx_builder)), 1U);
  EXPECT_EQ(cardano_utxo_list_get_length(tx_builder->state.sub_transaction_inputs), 1U);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction1);
  cardano_sub_transaction_unref(&sub_transaction2);
  cardano_utxo_list_unref(&resolved_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, reportsAnInputSpentByTheTransactionWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_utxos  = new_utxo_list();
  cardano_sub_transaction_t*     sub_transaction = create_sub_transaction(SUB_TX_CBOR);
  cardano_utxo_t*                utxo            = create_utxo(CBOR_DIFFERENT_VAL2);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_input(tx_builder, utxo, nullptr, nullptr);
  cardano_tx_builder_add_sub_transaction(tx_builder, sub_transaction, resolved_utxos);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "Sub transaction spends an input that is already spent by the transaction.");
  EXPECT_EQ(tx, nullptr);
  EXPECT_EQ(get_sub_transactions(tx_builder), nullptr);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_utxo_list_unref(&resolved_utxos);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_tx_builder_add_sub_transaction, makesAddInputReportAnInputSpentByTheSubTransactionWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_utxos  = new_utxo_list();
  cardano_sub_transaction_t*     sub_transaction = create_sub_transaction(SUB_TX_CBOR);
  cardano_utxo_t*                utxo            = create_utxo(CBOR_DIFFERENT_VAL2);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_sub_transaction(tx_builder, sub_transaction, resolved_utxos);
  cardano_tx_builder_add_input(tx_builder, utxo, nullptr, nullptr);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "Input is already spent by a sub transaction");
  EXPECT_EQ(tx, nullptr);
  EXPECT_EQ(cardano_utxo_list_get_length(tx_builder->state.pre_selected_inputs), 0U);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_utxo_list_unref(&resolved_utxos);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_tx_builder_add_sub_transaction, reportsAnUnresolvedInputWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_utxos  = nullptr;
  cardano_sub_transaction_t*     sub_transaction = create_sub_transaction(SUB_TX_CBOR);

  EXPECT_EQ(cardano_utxo_list_new(&resolved_utxos), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_sub_transaction(tx_builder, sub_transaction, resolved_utxos);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "Sub transaction spends an input that is not in the resolved UTXOs.");
  EXPECT_EQ(tx, nullptr);
  EXPECT_EQ(get_sub_transactions(tx_builder), nullptr);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_utxo_list_unref(&resolved_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, doesNothingIfTheBuilderAlreadyHasAnError)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_utxos  = new_utxo_list();
  cardano_sub_transaction_t*     sub_transaction = create_sub_transaction(SUB_TX_CBOR);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_add_sub_transaction(tx_builder, nullptr, resolved_utxos);

  // Act
  cardano_tx_builder_add_sub_transaction(tx_builder, sub_transaction, resolved_utxos);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "Sub transaction is NULL.");
  EXPECT_EQ(get_sub_transactions(tx_builder), nullptr);
  EXPECT_EQ(cardano_sub_transaction_refcount(sub_transaction), 1U);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_utxo_list_unref(&resolved_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, releasesTheSubTransactionWithTheBuilder)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_utxos  = new_utxo_list();
  cardano_sub_transaction_t*     sub_transaction = create_sub_transaction(SUB_TX_CBOR);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_sub_transaction(tx_builder, sub_transaction, resolved_utxos);

  // Assert
  EXPECT_EQ(cardano_sub_transaction_refcount(sub_transaction), 2U);

  cardano_tx_builder_unref(&tx_builder);

  EXPECT_EQ(cardano_sub_transaction_refcount(sub_transaction), 1U);
  EXPECT_EQ(cardano_utxo_list_refcount(resolved_utxos), 1U);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_utxo_list_unref(&resolved_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_utxos  = new_utxo_list();
  cardano_sub_transaction_t*     sub_transaction = create_sub_transaction(SUB_TX_CBOR);

  bool succeeded = false;

  for (int i = 0; (i < 100) && !succeeded; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_add_sub_transaction(tx_builder, sub_transaction, resolved_utxos);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    // Assert
    if (tx_builder->last_error == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_EQ(cardano_sub_transaction_set_get_length(get_sub_transactions(tx_builder)), 1U);
    }
    else
    {
      EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
      EXPECT_EQ(get_sub_transactions(tx_builder), nullptr);
      EXPECT_EQ(cardano_utxo_list_get_length(tx_builder->state.sub_transaction_inputs), 0U);
    }

    cardano_tx_builder_unref(&tx_builder);
  }

  EXPECT_TRUE(succeeded);

  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_utxo_list_unref(&resolved_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, buildsABalancedBatchIfThePartiesCancelOut)
{
  // Arrange
  cardano_protocol_parameters_t* params        = init_protocol_parameters();
  cardano_utxo_list_t*           all_utxos     = new_utxo_list();
  cardano_utxo_t*                seller_utxo   = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                batcher_utxo  = create_utxo(CBOR_DIFFERENT_VAL3);
  cardano_utxo_t*                buyer_utxo    = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_list_t*           seller_utxos  = new_single_utxo_list(seller_utxo);
  cardano_utxo_list_t*           batcher_utxos = new_single_utxo_list(batcher_utxo);
  cardano_utxo_list_t*           buyer_utxos   = new_single_utxo_list(buyer_utxo);
  cardano_sub_transaction_t*     seller_sub_tx = build_party_sub_transaction(params, seller_utxo, 11150770, 0);
  cardano_sub_transaction_t*     buyer_sub_tx  = build_party_sub_transaction(params, buyer_utxo, 224831727, 1);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, batcher_utxos);

  // Act
  cardano_tx_builder_add_sub_transaction(tx_builder, seller_sub_tx, seller_utxos);
  cardano_tx_builder_add_sub_transaction(tx_builder, buyer_sub_tx, buyer_utxos);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(tx, nullptr);

  bool is_balanced = false;

  EXPECT_EQ(cardano_is_transaction_balanced(tx, all_utxos, params, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  cardano_sub_transaction_set_t* sub_transactions = cardano_transaction_body_get_sub_transactions(body);
  cardano_sub_transaction_set_unref(&sub_transactions);

  EXPECT_EQ(cardano_sub_transaction_set_get_length(sub_transactions), 2U);
  EXPECT_TRUE(transaction_spends(tx, batcher_utxo));
  EXPECT_FALSE(transaction_spends(tx, seller_utxo));
  EXPECT_FALSE(transaction_spends(tx, buyer_utxo));
  EXPECT_EQ(sum_input_lovelace(tx, batcher_utxos), sum_output_lovelace(tx) + cardano_transaction_body_get_fee(body));

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&seller_sub_tx);
  cardano_sub_transaction_unref(&buyer_sub_tx);
  cardano_utxo_unref(&seller_utxo);
  cardano_utxo_unref(&batcher_utxo);
  cardano_utxo_unref(&buyer_utxo);
  cardano_utxo_list_unref(&all_utxos);
  cardano_utxo_list_unref(&seller_utxos);
  cardano_utxo_list_unref(&batcher_utxos);
  cardano_utxo_list_unref(&buyer_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, buildsABalancedBatchThatFundsTheDeficitOfAParty)
{
  // Arrange
  cardano_protocol_parameters_t* params        = init_protocol_parameters();
  cardano_utxo_list_t*           all_utxos     = new_utxo_list();
  cardano_utxo_t*                seller_utxo   = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                batcher_utxo  = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_list_t*           seller_utxos  = new_single_utxo_list(seller_utxo);
  cardano_utxo_list_t*           batcher_utxos = new_single_utxo_list(batcher_utxo);
  cardano_sub_transaction_t*     seller_sub_tx = build_party_sub_transaction(params, seller_utxo, 11150770, 0);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, batcher_utxos);

  // Act
  cardano_tx_builder_add_sub_transaction(tx_builder, seller_sub_tx, seller_utxos);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(tx, nullptr);

  bool is_balanced = false;

  EXPECT_EQ(cardano_is_transaction_balanced(tx, all_utxos, params, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  cardano_value_t* top_level_imbalance = nullptr;

  EXPECT_EQ(cardano_compute_transaction_imbalance(tx, batcher_utxos, params, &top_level_imbalance), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(top_level_imbalance), 10000000);

  cardano_asset_id_map_t* assets       = cardano_value_as_assets_map(top_level_imbalance);
  cardano_asset_id_t*     nft_asset_id = nullptr;
  int64_t                 nft_quantity = 0;

  EXPECT_EQ(cardano_asset_id_from_hex(NFT_ASSET_ID_HEX, strlen(NFT_ASSET_ID_HEX), &nft_asset_id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_id_map_get(assets, nft_asset_id, &nft_quantity), CARDANO_SUCCESS);
  EXPECT_EQ(nft_quantity, -1);
  EXPECT_FALSE(transaction_spends(tx, seller_utxo));

  // Cleanup
  cardano_asset_id_unref(&nft_asset_id);
  cardano_asset_id_map_unref(&assets);
  cardano_value_unref(&top_level_imbalance);
  cardano_transaction_unref(&tx);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&seller_sub_tx);
  cardano_utxo_unref(&seller_utxo);
  cardano_utxo_unref(&batcher_utxo);
  cardano_utxo_list_unref(&all_utxos);
  cardano_utxo_list_unref(&seller_utxos);
  cardano_utxo_list_unref(&batcher_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, buildsABalancedBatchThatReturnsTheSurplusOfAPartyAsChange)
{
  // Arrange
  cardano_protocol_parameters_t* params        = init_protocol_parameters();
  cardano_utxo_list_t*           all_utxos     = new_utxo_list();
  cardano_utxo_t*                batcher_utxo  = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                party_utxo    = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_list_t*           batcher_utxos = new_single_utxo_list(batcher_utxo);
  cardano_utxo_list_t*           party_utxos   = new_single_utxo_list(party_utxo);
  cardano_sub_transaction_t*     party_sub_tx  = build_party_sub_transaction(params, party_utxo, 224831727, 0);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, batcher_utxos);

  // Act
  cardano_tx_builder_add_sub_transaction(tx_builder, party_sub_tx, party_utxos);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(tx, nullptr);

  bool is_balanced = false;

  EXPECT_EQ(cardano_is_transaction_balanced(tx, all_utxos, params, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  EXPECT_EQ(sum_input_lovelace(tx, batcher_utxos) + 10000000U, sum_output_lovelace(tx) + cardano_transaction_body_get_fee(body));
  EXPECT_FALSE(transaction_spends(tx, party_utxo));

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&party_sub_tx);
  cardano_utxo_unref(&batcher_utxo);
  cardano_utxo_unref(&party_utxo);
  cardano_utxo_list_unref(&all_utxos);
  cardano_utxo_list_unref(&batcher_utxos);
  cardano_utxo_list_unref(&party_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, neverSelectsAnAvailableUtxoSpentByASubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params       = init_protocol_parameters();
  cardano_utxo_list_t*           all_utxos    = new_utxo_list();
  cardano_utxo_t*                party_utxo   = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_sub_transaction_t*     party_sub_tx = build_party_sub_transaction(params, party_utxo, 234831727, 0);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, all_utxos);

  // Act
  cardano_tx_builder_add_sub_transaction(tx_builder, party_sub_tx, all_utxos);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(tx, nullptr);

  bool is_balanced = false;

  EXPECT_EQ(cardano_is_transaction_balanced(tx, all_utxos, params, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);
  EXPECT_FALSE(transaction_spends(tx, party_utxo));

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&party_sub_tx);
  cardano_utxo_unref(&party_utxo);
  cardano_utxo_list_unref(&all_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, buildsTheSameBatchIfTheBatcherIsAlsoAParty)
{
  // Arrange
  cardano_protocol_parameters_t* params       = init_protocol_parameters();
  cardano_utxo_list_t*           wallet_utxos = new_utxo_list();
  cardano_utxo_t*                party_utxo   = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_t*                first_utxo   = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                second_utxo  = create_utxo(CBOR_DIFFERENT_VAL3);
  cardano_utxo_list_t*           party_utxos  = new_single_utxo_list(party_utxo);
  cardano_utxo_list_t*           other_utxos  = new_single_utxo_list(first_utxo);
  cardano_sub_transaction_t*     party_sub_tx = build_party_sub_transaction(params, party_utxo, 224831727, 0);

  EXPECT_EQ(cardano_utxo_list_add(other_utxos, second_utxo), CARDANO_SUCCESS);

  cardano_tx_builder_t* party_builder = new_funded_tx_builder(params, wallet_utxos);
  cardano_tx_builder_t* other_builder = new_funded_tx_builder(params, other_utxos);

  // Act
  cardano_tx_builder_add_sub_transaction(party_builder, party_sub_tx, party_utxos);
  cardano_tx_builder_add_sub_transaction(other_builder, party_sub_tx, party_utxos);

  cardano_transaction_t* party_tx     = nullptr;
  cardano_transaction_t* other_tx     = nullptr;
  cardano_error_t        party_result = cardano_tx_builder_build(party_builder, &party_tx);
  cardano_error_t        other_result = cardano_tx_builder_build(other_builder, &other_tx);

  // Assert
  EXPECT_EQ(party_result, CARDANO_SUCCESS);
  EXPECT_EQ(other_result, CARDANO_SUCCESS);
  ASSERT_NE(party_tx, nullptr);
  ASSERT_NE(other_tx, nullptr);

  bool  is_balanced  = false;
  char* party_tx_hex = encode_transaction(party_tx);
  char* other_tx_hex = encode_transaction(other_tx);

  EXPECT_EQ(cardano_is_transaction_balanced(party_tx, wallet_utxos, params, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);
  EXPECT_FALSE(transaction_spends(party_tx, party_utxo));
  EXPECT_STREQ(party_tx_hex, other_tx_hex);
  EXPECT_EQ(cardano_utxo_list_get_length(wallet_utxos), 3U);
  EXPECT_EQ(party_builder->state.available_utxos, wallet_utxos);

  // Cleanup
  free(party_tx_hex);
  free(other_tx_hex);
  cardano_transaction_unref(&party_tx);
  cardano_transaction_unref(&other_tx);
  cardano_tx_builder_unref(&party_builder);
  cardano_tx_builder_unref(&other_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&party_sub_tx);
  cardano_utxo_unref(&party_utxo);
  cardano_utxo_unref(&first_utxo);
  cardano_utxo_unref(&second_utxo);
  cardano_utxo_list_unref(&wallet_utxos);
  cardano_utxo_list_unref(&party_utxos);
  cardano_utxo_list_unref(&other_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, reportsUnbalancedSubTransactionsInLegacyModeWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_utxo_t*                seller_utxo    = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                batcher_utxo   = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_t*                reference_utxo = create_utxo(UTXO_WITH_REF_SCRIPT_PV2);
  cardano_utxo_list_t*           seller_utxos   = new_single_utxo_list(seller_utxo);
  cardano_utxo_list_t*           batcher_utxos  = new_single_utxo_list(batcher_utxo);
  cardano_sub_transaction_t*     seller_sub_tx  = build_party_sub_transaction(params, seller_utxo, 11150770, 0);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, batcher_utxos);

  // Act
  cardano_tx_builder_add_reference_input(tx_builder, reference_utxo);
  cardano_tx_builder_add_sub_transaction(tx_builder, seller_sub_tx, seller_utxos);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNBALANCED_SUB_TRANSACTIONS);
  EXPECT_EQ(tx, nullptr);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "The top level transaction uses PlutusV1, PlutusV2 or PlutusV3 scripts, so the sub transactions must balance between themselves. Add a balancing sub transaction, top level change can not absorb their imbalance.");

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&seller_sub_tx);
  cardano_utxo_unref(&seller_utxo);
  cardano_utxo_unref(&batcher_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_utxo_list_unref(&seller_utxos);
  cardano_utxo_list_unref(&batcher_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, buildsABalancedBatchInLegacyModeIfThePartiesCancelOut)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_utxo_list_t*           all_utxos      = new_utxo_list();
  cardano_utxo_t*                seller_utxo    = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                batcher_utxo   = create_utxo(CBOR_DIFFERENT_VAL3);
  cardano_utxo_t*                buyer_utxo     = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_t*                reference_utxo = create_utxo(UTXO_WITH_REF_SCRIPT_PV2);
  cardano_utxo_list_t*           seller_utxos   = new_single_utxo_list(seller_utxo);
  cardano_utxo_list_t*           batcher_utxos  = new_single_utxo_list(batcher_utxo);
  cardano_utxo_list_t*           buyer_utxos    = new_single_utxo_list(buyer_utxo);
  cardano_sub_transaction_t*     seller_sub_tx  = build_party_sub_transaction(params, seller_utxo, 11150770, 0);
  cardano_sub_transaction_t*     buyer_sub_tx   = build_party_sub_transaction(params, buyer_utxo, 224831727, 1);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, batcher_utxos);

  // Act
  cardano_tx_builder_add_reference_input(tx_builder, reference_utxo);
  cardano_tx_builder_add_sub_transaction(tx_builder, seller_sub_tx, seller_utxos);
  cardano_tx_builder_add_sub_transaction(tx_builder, buyer_sub_tx, buyer_utxos);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(tx, nullptr);

  bool is_balanced = false;

  EXPECT_EQ(cardano_is_transaction_balanced(tx, all_utxos, params, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  cardano_value_t* top_level_imbalance = nullptr;

  EXPECT_EQ(cardano_compute_transaction_imbalance(tx, batcher_utxos, params, &top_level_imbalance), CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_value_is_zero(top_level_imbalance));

  // Cleanup
  cardano_value_unref(&top_level_imbalance);
  cardano_transaction_unref(&tx);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&seller_sub_tx);
  cardano_sub_transaction_unref(&buyer_sub_tx);
  cardano_utxo_unref(&seller_utxo);
  cardano_utxo_unref(&batcher_utxo);
  cardano_utxo_unref(&buyer_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_utxo_list_unref(&all_utxos);
  cardano_utxo_list_unref(&seller_utxos);
  cardano_utxo_list_unref(&batcher_utxos);
  cardano_utxo_list_unref(&buyer_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, reportsAMissingCollateralChangeAddressIfOnlyASubTransactionHasRedeemers)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_utxo_t*                seller_utxo    = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                batcher_utxo   = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_t*                reference_utxo = create_utxo(UTXO_WITH_REF_SCRIPT_PV2);
  cardano_utxo_list_t*           seller_utxos   = new_single_utxo_list(seller_utxo);
  cardano_utxo_list_t*           batcher_utxos  = new_single_utxo_list(batcher_utxo);
  cardano_sub_transaction_t*     seller_sub_tx  = build_script_party_sub_transaction(params, seller_utxo, reference_utxo, 11150770, 1000000, 200000000);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, batcher_utxos);

  // Act
  cardano_tx_builder_add_sub_transaction(tx_builder, seller_sub_tx, seller_utxos);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(tx, nullptr);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "This transaction interacts with plutus validators. You must set a collateral change address before calling `build`.");

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&seller_sub_tx);
  cardano_utxo_unref(&seller_utxo);
  cardano_utxo_unref(&batcher_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_utxo_list_unref(&seller_utxos);
  cardano_utxo_list_unref(&batcher_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, reportsMissingCollateralUtxosIfOnlyASubTransactionHasRedeemers)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_utxo_t*                seller_utxo    = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                batcher_utxo   = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_t*                reference_utxo = create_utxo(UTXO_WITH_REF_SCRIPT_PV2);
  cardano_utxo_list_t*           seller_utxos   = new_single_utxo_list(seller_utxo);
  cardano_utxo_list_t*           batcher_utxos  = new_single_utxo_list(batcher_utxo);
  cardano_sub_transaction_t*     seller_sub_tx  = build_script_party_sub_transaction(params, seller_utxo, reference_utxo, 11150770, 1000000, 200000000);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, batcher_utxos);

  // Act
  cardano_tx_builder_set_collateral_change_address_ex(tx_builder, CHANGE_ADDRESS, strlen(CHANGE_ADDRESS));
  cardano_tx_builder_add_sub_transaction(tx_builder, seller_sub_tx, seller_utxos);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(tx, nullptr);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "This transaction interacts with plutus validators. You must set the collateral UTXOs before calling `build`.");

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&seller_sub_tx);
  cardano_utxo_unref(&seller_utxo);
  cardano_utxo_unref(&batcher_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_utxo_list_unref(&seller_utxos);
  cardano_utxo_list_unref(&batcher_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, buildsABatchThatPaysForTheScriptsOfASubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params          = init_protocol_parameters();
  cardano_unit_interval_t*       script_ref_cost = cardano_protocol_parameters_get_ref_script_cost_per_byte(params);
  cardano_utxo_list_t*           all_utxos       = new_utxo_list();
  cardano_utxo_t*                seller_utxo     = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                batcher_utxo    = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_t*                reference_utxo  = create_utxo(UTXO_WITH_REF_SCRIPT_PV2);
  cardano_utxo_list_t*           seller_utxos    = new_single_utxo_list(seller_utxo);
  cardano_utxo_list_t*           resolved_utxos  = new_single_utxo_list(seller_utxo);
  cardano_utxo_list_t*           reference_utxos = new_single_utxo_list(reference_utxo);
  cardano_utxo_list_t*           batcher_utxos   = new_single_utxo_list(batcher_utxo);
  cardano_sub_transaction_t*     seller_sub_tx   = build_script_party_sub_transaction(params, seller_utxo, reference_utxo, 11150770, 1000000, 200000000);
  uint64_t                       ref_script_fee  = 0U;

  EXPECT_EQ(cardano_protocol_parameters_set_collateral_percentage(params, 150), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(resolved_utxos, reference_utxo), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_compute_script_ref_fee(reference_utxos, script_ref_cost, &ref_script_fee), CARDANO_SUCCESS);

  cardano_tx_builder_t* unpriced_builder = new_funded_tx_builder(params, batcher_utxos);
  cardano_tx_builder_t* tx_builder       = new_funded_tx_builder(params, batcher_utxos);

  cardano_tx_builder_set_collateral_change_address_ex(unpriced_builder, CHANGE_ADDRESS, strlen(CHANGE_ADDRESS));
  cardano_tx_builder_set_collateral_utxos(unpriced_builder, batcher_utxos);
  cardano_tx_builder_set_collateral_change_address_ex(tx_builder, CHANGE_ADDRESS, strlen(CHANGE_ADDRESS));
  cardano_tx_builder_set_collateral_utxos(tx_builder, batcher_utxos);

  // Act
  cardano_tx_builder_add_sub_transaction(unpriced_builder, seller_sub_tx, seller_utxos);
  cardano_tx_builder_add_sub_transaction(tx_builder, seller_sub_tx, resolved_utxos);

  cardano_transaction_t* unpriced_tx = nullptr;
  cardano_transaction_t* tx          = nullptr;

  cardano_error_t unpriced_result = cardano_tx_builder_build(unpriced_builder, &unpriced_tx);
  cardano_error_t result          = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(unpriced_result, CARDANO_SUCCESS);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(unpriced_tx, nullptr);
  ASSERT_NE(tx, nullptr);

  bool is_balanced = false;

  EXPECT_EQ(cardano_is_transaction_balanced(tx, all_utxos, params, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  cardano_transaction_body_t* unpriced_body = cardano_transaction_get_body(unpriced_tx);
  cardano_transaction_body_unref(&unpriced_body);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  cardano_transaction_input_set_t* collateral = cardano_transaction_body_get_collateral(body);
  cardano_transaction_input_set_unref(&collateral);

  const uint64_t  fee              = cardano_transaction_body_get_fee(body);
  const uint64_t* total_collateral = cardano_transaction_body_get_total_collateral(body);
  uint64_t        size_fee         = 0U;

  EXPECT_EQ(cardano_compute_min_fee_without_scripts(tx, 155381, 44, &size_fee), CARDANO_SUCCESS);

  EXPECT_GT(ref_script_fee, 0U);
  EXPECT_GE(fee, size_fee + 72120U + ref_script_fee);
  EXPECT_EQ(fee, cardano_transaction_body_get_fee(unpriced_body) + ref_script_fee);
  EXPECT_EQ(cardano_transaction_input_set_get_length(collateral), 1U);
  ASSERT_NE(total_collateral, nullptr);
  EXPECT_EQ(*total_collateral, (uint64_t)ceil(((double)fee * 150.0) / 100.0));
  EXPECT_FALSE(transaction_spends(tx, seller_utxo));

  // Cleanup
  cardano_transaction_unref(&unpriced_tx);
  cardano_transaction_unref(&tx);
  cardano_tx_builder_unref(&unpriced_builder);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_sub_transaction_unref(&seller_sub_tx);
  cardano_utxo_unref(&seller_utxo);
  cardano_utxo_unref(&batcher_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_utxo_list_unref(&all_utxos);
  cardano_utxo_list_unref(&seller_utxos);
  cardano_utxo_list_unref(&resolved_utxos);
  cardano_utxo_list_unref(&reference_utxos);
  cardano_utxo_list_unref(&batcher_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, buildsABatchWithoutCollateralIfNoSubTransactionHasRedeemers)
{
  // Arrange
  cardano_protocol_parameters_t* params        = init_protocol_parameters();
  cardano_utxo_t*                seller_utxo   = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                batcher_utxo  = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_list_t*           seller_utxos  = new_single_utxo_list(seller_utxo);
  cardano_utxo_list_t*           batcher_utxos = new_single_utxo_list(batcher_utxo);
  cardano_sub_transaction_t*     seller_sub_tx = build_party_sub_transaction(params, seller_utxo, 11150770, 0);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, batcher_utxos);

  cardano_tx_builder_set_collateral_change_address_ex(tx_builder, CHANGE_ADDRESS, strlen(CHANGE_ADDRESS));
  cardano_tx_builder_set_collateral_utxos(tx_builder, batcher_utxos);

  // Act
  cardano_tx_builder_add_sub_transaction(tx_builder, seller_sub_tx, seller_utxos);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(tx, nullptr);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  cardano_transaction_input_set_t* collateral = cardano_transaction_body_get_collateral(body);
  cardano_transaction_input_set_unref(&collateral);

  EXPECT_EQ(cardano_transaction_input_set_get_length(collateral), 0U);
  EXPECT_EQ(cardano_transaction_body_get_total_collateral(body), nullptr);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&seller_sub_tx);
  cardano_utxo_unref(&seller_utxo);
  cardano_utxo_unref(&batcher_utxo);
  cardano_utxo_list_unref(&seller_utxos);
  cardano_utxo_list_unref(&batcher_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, reportsAMissingRequiredTopLevelGuardWhenBuilding)
{
  // Arrange
  cardano_protocol_parameters_t* params        = init_protocol_parameters();
  cardano_utxo_list_t*           all_utxos     = new_utxo_list();
  cardano_utxo_t*                seller_utxo   = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                batcher_utxo  = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_list_t*           seller_utxos  = new_single_utxo_list(seller_utxo);
  cardano_utxo_list_t*           batcher_utxos = new_single_utxo_list(batcher_utxo);
  cardano_credential_t*          guard         = create_credential(HASH_HEX, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  cardano_sub_transaction_t*     seller_sub_tx = build_guarded_party_sub_transaction(params, seller_utxo, 11150770, guard);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, batcher_utxos);

  // Act
  cardano_tx_builder_add_sub_transaction(tx_builder, seller_sub_tx, seller_utxos);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_EQ(tx, nullptr);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), MISSING_GUARD_ERROR);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&seller_sub_tx);
  cardano_credential_unref(&guard);
  cardano_utxo_unref(&seller_utxo);
  cardano_utxo_unref(&batcher_utxo);
  cardano_utxo_list_unref(&all_utxos);
  cardano_utxo_list_unref(&seller_utxos);
  cardano_utxo_list_unref(&batcher_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, buildsABatchIfTheRequiredTopLevelGuardIsAddedBeforeTheSubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params        = init_protocol_parameters();
  cardano_utxo_list_t*           all_utxos     = new_utxo_list();
  cardano_utxo_t*                seller_utxo   = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                batcher_utxo  = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_list_t*           seller_utxos  = new_single_utxo_list(seller_utxo);
  cardano_utxo_list_t*           batcher_utxos = new_single_utxo_list(batcher_utxo);
  cardano_credential_t*          guard         = create_credential(HASH_HEX, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  cardano_sub_transaction_t*     seller_sub_tx = build_guarded_party_sub_transaction(params, seller_utxo, 11150770, guard);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, batcher_utxos);

  // Act
  cardano_tx_builder_add_guard(tx_builder, guard);
  cardano_tx_builder_add_sub_transaction(tx_builder, seller_sub_tx, seller_utxos);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(tx, nullptr);

  bool is_balanced = false;

  EXPECT_EQ(cardano_is_transaction_balanced(tx, all_utxos, params, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);
  EXPECT_EQ(cardano_sub_transaction_set_get_length(get_sub_transactions(tx_builder)), 1U);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&seller_sub_tx);
  cardano_credential_unref(&guard);
  cardano_utxo_unref(&seller_utxo);
  cardano_utxo_unref(&batcher_utxo);
  cardano_utxo_list_unref(&all_utxos);
  cardano_utxo_list_unref(&seller_utxos);
  cardano_utxo_list_unref(&batcher_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, buildsABatchIfTheRequiredTopLevelGuardIsAddedAfterTheSubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params        = init_protocol_parameters();
  cardano_utxo_list_t*           all_utxos     = new_utxo_list();
  cardano_utxo_t*                seller_utxo   = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                batcher_utxo  = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_list_t*           seller_utxos  = new_single_utxo_list(seller_utxo);
  cardano_utxo_list_t*           batcher_utxos = new_single_utxo_list(batcher_utxo);
  cardano_credential_t*          guard         = create_credential(SCRIPT_HASH_HEX, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);
  cardano_sub_transaction_t*     seller_sub_tx = build_guarded_party_sub_transaction(params, seller_utxo, 11150770, guard);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, batcher_utxos);

  // Act
  cardano_tx_builder_add_sub_transaction(tx_builder, seller_sub_tx, seller_utxos);
  cardano_tx_builder_add_guard(tx_builder, guard);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(tx, nullptr);

  bool is_balanced = false;

  EXPECT_EQ(cardano_is_transaction_balanced(tx, all_utxos, params, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);
  EXPECT_EQ(cardano_sub_transaction_set_get_length(get_sub_transactions(tx_builder)), 1U);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&seller_sub_tx);
  cardano_credential_unref(&guard);
  cardano_utxo_unref(&seller_utxo);
  cardano_utxo_unref(&batcher_utxo);
  cardano_utxo_list_unref(&all_utxos);
  cardano_utxo_list_unref(&seller_utxos);
  cardano_utxo_list_unref(&batcher_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, reportsAMissingRequiredTopLevelGuardIfOnlyTheGuardOfAnotherSubTransactionIsPresent)
{
  // Arrange
  cardano_protocol_parameters_t* params        = init_protocol_parameters();
  cardano_utxo_t*                seller_utxo   = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                batcher_utxo  = create_utxo(CBOR_DIFFERENT_VAL3);
  cardano_utxo_t*                buyer_utxo    = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_list_t*           seller_utxos  = new_single_utxo_list(seller_utxo);
  cardano_utxo_list_t*           batcher_utxos = new_single_utxo_list(batcher_utxo);
  cardano_utxo_list_t*           buyer_utxos   = new_single_utxo_list(buyer_utxo);
  cardano_credential_t*          seller_guard  = create_credential(HASH_HEX, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  cardano_credential_t*          buyer_guard   = create_credential(HASH_HEX1, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  cardano_sub_transaction_t*     seller_sub_tx = build_guarded_party_sub_transaction(params, seller_utxo, 11150770, seller_guard);
  cardano_sub_transaction_t*     buyer_sub_tx  = build_guarded_party_sub_transaction(params, buyer_utxo, 224831727, buyer_guard);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, batcher_utxos);

  // Act
  cardano_tx_builder_add_guard(tx_builder, seller_guard);
  cardano_tx_builder_add_sub_transaction(tx_builder, seller_sub_tx, seller_utxos);
  cardano_tx_builder_add_sub_transaction(tx_builder, buyer_sub_tx, buyer_utxos);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_EQ(tx, nullptr);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), MISSING_GUARD_ERROR);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&seller_sub_tx);
  cardano_sub_transaction_unref(&buyer_sub_tx);
  cardano_credential_unref(&seller_guard);
  cardano_credential_unref(&buyer_guard);
  cardano_utxo_unref(&seller_utxo);
  cardano_utxo_unref(&batcher_utxo);
  cardano_utxo_unref(&buyer_utxo);
  cardano_utxo_list_unref(&seller_utxos);
  cardano_utxo_list_unref(&batcher_utxos);
  cardano_utxo_list_unref(&buyer_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, buildsABatchIfEverySubTransactionFindsItsRequiredTopLevelGuard)
{
  // Arrange
  cardano_protocol_parameters_t* params        = init_protocol_parameters();
  cardano_utxo_list_t*           all_utxos     = new_utxo_list();
  cardano_utxo_t*                seller_utxo   = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                batcher_utxo  = create_utxo(CBOR_DIFFERENT_VAL3);
  cardano_utxo_t*                buyer_utxo    = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_list_t*           seller_utxos  = new_single_utxo_list(seller_utxo);
  cardano_utxo_list_t*           batcher_utxos = new_single_utxo_list(batcher_utxo);
  cardano_utxo_list_t*           buyer_utxos   = new_single_utxo_list(buyer_utxo);
  cardano_credential_t*          seller_guard  = create_credential(HASH_HEX, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  cardano_credential_t*          buyer_guard   = create_credential(SCRIPT_HASH_HEX, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);
  cardano_sub_transaction_t*     seller_sub_tx = build_guarded_party_sub_transaction(params, seller_utxo, 11150770, seller_guard);
  cardano_sub_transaction_t*     buyer_sub_tx  = build_guarded_party_sub_transaction(params, buyer_utxo, 224831727, buyer_guard);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, batcher_utxos);

  // Act
  cardano_tx_builder_add_guard(tx_builder, seller_guard);
  cardano_tx_builder_add_sub_transaction(tx_builder, seller_sub_tx, seller_utxos);
  cardano_tx_builder_add_sub_transaction(tx_builder, buyer_sub_tx, buyer_utxos);
  cardano_tx_builder_add_guard(tx_builder, buyer_guard);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(tx, nullptr);

  bool is_balanced = false;

  EXPECT_EQ(cardano_is_transaction_balanced(tx, all_utxos, params, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);
  EXPECT_EQ(cardano_sub_transaction_set_get_length(get_sub_transactions(tx_builder)), 2U);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&seller_sub_tx);
  cardano_sub_transaction_unref(&buyer_sub_tx);
  cardano_credential_unref(&seller_guard);
  cardano_credential_unref(&buyer_guard);
  cardano_utxo_unref(&seller_utxo);
  cardano_utxo_unref(&batcher_utxo);
  cardano_utxo_unref(&buyer_utxo);
  cardano_utxo_list_unref(&all_utxos);
  cardano_utxo_list_unref(&seller_utxos);
  cardano_utxo_list_unref(&batcher_utxos);
  cardano_utxo_list_unref(&buyer_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, reportsAMissingRequiredTopLevelGuardIfTheGuardHasTheSameHashButAnotherType)
{
  // Arrange
  cardano_protocol_parameters_t* params        = init_protocol_parameters();
  cardano_utxo_list_t*           all_utxos     = new_utxo_list();
  cardano_utxo_t*                seller_utxo   = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                batcher_utxo  = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_list_t*           seller_utxos  = new_single_utxo_list(seller_utxo);
  cardano_utxo_list_t*           batcher_utxos = new_single_utxo_list(batcher_utxo);
  cardano_credential_t*          guard         = create_credential(HASH_HEX, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);
  cardano_credential_t*          other_type    = create_credential(HASH_HEX, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  cardano_sub_transaction_t*     seller_sub_tx = build_guarded_party_sub_transaction(params, seller_utxo, 11150770, guard);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, batcher_utxos);

  // Act
  cardano_tx_builder_add_guard(tx_builder, other_type);
  cardano_tx_builder_add_sub_transaction(tx_builder, seller_sub_tx, seller_utxos);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_EQ(tx, nullptr);
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), MISSING_GUARD_ERROR);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&seller_sub_tx);
  cardano_credential_unref(&guard);
  cardano_credential_unref(&other_type);
  cardano_utxo_unref(&seller_utxo);
  cardano_utxo_unref(&batcher_utxo);
  cardano_utxo_list_unref(&all_utxos);
  cardano_utxo_list_unref(&seller_utxos);
  cardano_utxo_list_unref(&batcher_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, buildsABatchWithGuardsIfTheSubTransactionRequiresNone)
{
  // Arrange
  cardano_protocol_parameters_t* params        = init_protocol_parameters();
  cardano_utxo_list_t*           all_utxos     = new_utxo_list();
  cardano_utxo_t*                seller_utxo   = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                batcher_utxo  = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_list_t*           seller_utxos  = new_single_utxo_list(seller_utxo);
  cardano_utxo_list_t*           batcher_utxos = new_single_utxo_list(batcher_utxo);
  cardano_credential_t*          guard         = create_credential(HASH_HEX, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  cardano_sub_transaction_t*     seller_sub_tx = build_party_sub_transaction(params, seller_utxo, 11150770, 0);

  cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, batcher_utxos);

  // Act
  cardano_tx_builder_add_guard(tx_builder, guard);
  cardano_tx_builder_add_sub_transaction(tx_builder, seller_sub_tx, seller_utxos);

  cardano_transaction_t* tx     = nullptr;
  cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(tx, nullptr);

  bool is_balanced = false;

  EXPECT_EQ(cardano_is_transaction_balanced(tx, all_utxos, params, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);
  EXPECT_EQ(cardano_sub_transaction_set_get_length(get_sub_transactions(tx_builder)), 1U);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&seller_sub_tx);
  cardano_credential_unref(&guard);
  cardano_utxo_unref(&seller_utxo);
  cardano_utxo_unref(&batcher_utxo);
  cardano_utxo_list_unref(&all_utxos);
  cardano_utxo_list_unref(&seller_utxos);
  cardano_utxo_list_unref(&batcher_utxos);
}

TEST(cardano_tx_builder_add_sub_transaction, doesntCrashOnMemoryAllocationFailIfASubTransactionRequiresATopLevelGuard)
{
  // Arrange
  cardano_protocol_parameters_t* params        = init_protocol_parameters();
  cardano_utxo_t*                seller_utxo   = create_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                batcher_utxo  = create_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_list_t*           seller_utxos  = new_single_utxo_list(seller_utxo);
  cardano_utxo_list_t*           batcher_utxos = new_single_utxo_list(batcher_utxo);
  cardano_credential_t*          guard         = create_credential(HASH_HEX, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  cardano_sub_transaction_t*     seller_sub_tx = build_guarded_party_sub_transaction(params, seller_utxo, 11150770, guard);

  bool succeeded = false;

  for (int i = 0; (i < 1024) && !succeeded; ++i)
  {
    cardano_tx_builder_t* tx_builder = new_funded_tx_builder(params, batcher_utxos);

    cardano_tx_builder_add_guard(tx_builder, guard);
    cardano_tx_builder_add_sub_transaction(tx_builder, seller_sub_tx, seller_utxos);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_transaction_t* tx     = nullptr;
    cardano_error_t        result = cardano_tx_builder_build(tx_builder, &tx);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    // Assert
    if (result == CARDANO_SUCCESS)
    {
      succeeded = true;

      EXPECT_NE(tx, nullptr);
    }
    else
    {
      EXPECT_NE(result, CARDANO_ERROR_ELEMENT_NOT_FOUND);
      EXPECT_EQ(tx, nullptr);
    }

    cardano_transaction_unref(&tx);
    cardano_tx_builder_unref(&tx_builder);
  }

  EXPECT_TRUE(succeeded);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_sub_transaction_unref(&seller_sub_tx);
  cardano_credential_unref(&guard);
  cardano_utxo_unref(&seller_utxo);
  cardano_utxo_unref(&batcher_utxo);
  cardano_utxo_list_unref(&seller_utxos);
  cardano_utxo_list_unref(&batcher_utxos);
}

TEST(cardano_tx_builder_register_reward_address, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_register_reward_address(nullptr, nullptr, nullptr);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_register_reward_address(tx_builder, nullptr, nullptr);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_register_reward_address, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_register_reward_address(tx_builder, nullptr, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_register_reward_address, canRegisterRewardAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_register_reward_address(tx_builder, reward_address, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_plutus_data_unref(&redeemer);
}

TEST(cardano_tx_builder_register_reward_address, returnsErrorOnMemoryAllocationFailure)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  for (int i = 0; i < 10; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_register_reward_address(tx_builder, reward_address, redeemer);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_plutus_data_unref(&redeemer);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_register_reward_address_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_register_reward_address_ex(nullptr, nullptr, 0, nullptr);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_register_reward_address_ex(tx_builder, nullptr, 0, nullptr);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_register_reward_address_ex, canRegisterRewardAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_register_reward_address_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_plutus_data_unref(&redeemer);
}

TEST(cardano_tx_builder_deregister_reward_address, doesntCrashIfGivenNull)
{
  cardano_tx_builder_deregister_reward_address(nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_deregister_reward_address, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_deregister_reward_address(tx_builder, nullptr, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_deregister_reward_address, canDeregisterRewardAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_deregister_reward_address(tx_builder, reward_address, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_plutus_data_unref(&redeemer);
}

TEST(cardano_tx_builder_deregister_reward_address, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  for (int i = 0; i < 10; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_deregister_reward_address(tx_builder, reward_address, redeemer);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_plutus_data_unref(&redeemer);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_deregister_reward_address_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params = init_protocol_parameters();

  cardano_tx_builder_deregister_reward_address_ex(nullptr, nullptr, 0, nullptr);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_deregister_reward_address_ex(tx_builder, nullptr, 0, nullptr);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_deregister_reward_address_ex, canDeregisterRewardAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_deregister_reward_address_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_plutus_data_unref(&redeemer);
}

TEST(cardano_tx_builder_delegate_stake, doesntCrashIfGivenNull)
{
  cardano_tx_builder_delegate_stake(nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_delegate_stake, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = nullptr;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_blake2b_hash_t*        pool_id        = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &pool_id), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_delegate_stake(tx_builder, nullptr, pool_id, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_delegate_stake(tx_builder, reward_address, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_blake2b_hash_unref(&pool_id);
}

TEST(cardano_tx_builder_delegate_stake, canDelegateStake)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = nullptr;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_blake2b_hash_t*        pool_id        = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &pool_id), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_delegate_stake(tx_builder, reward_address, pool_id, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_blake2b_hash_unref(&pool_id);
  cardano_plutus_data_unref(&redeemer);
}

TEST(cardano_tx_builder_delegate_stake, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = nullptr;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_blake2b_hash_t*        pool_id        = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &pool_id), CARDANO_SUCCESS);

  for (int i = 0; i < 10; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_delegate_stake(tx_builder, reward_address, pool_id, redeemer);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_blake2b_hash_unref(&pool_id);
  cardano_plutus_data_unref(&redeemer);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_delegate_stake_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;

  cardano_tx_builder_delegate_stake_ex(nullptr, nullptr, 0, nullptr, 0, nullptr);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_delegate_stake_ex(tx_builder, nullptr, 0, nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_delegate_stake_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_delegate_stake_ex, canDelegateStake)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = nullptr;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_blake2b_hash_t*        pool_id        = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &pool_id), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_delegate_stake_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), "pool1pzdqdxrv0k74p4q33y98f2u7vzaz95et7mjeedjcfy0jcgk754f", strlen("pool1pzdqdxrv0k74p4q33y98f2u7vzaz95et7mjeedjcfy0jcgk754f"), redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_blake2b_hash_unref(&pool_id);
  cardano_plutus_data_unref(&redeemer);
}

TEST(cardano_tx_builder_delegate_stake_ex, returnsErrorIfInvalidPoolId)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = nullptr;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_delegate_stake_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), "pool1pzdqdxrv0k74p4q33y98f2u7vzaz95et7mjeedjcfy0jcgk754", strlen("pool1pzdqdxrv0k74p4q33y98f2u7vzaz95et7mjeedjcfy0jcgk754"), redeemer);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_DECODING);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_delegate_stake_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), "test1pzdqdxrv0k74p4q33y98f2u7vzaz95et7mjeedjcfy0jclsudc9", strlen("test1pzdqdxrv0k74p4q33y98f2u7vzaz95et7mjeedjcfy0jclsudc9"), redeemer);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_INVALID_ARGUMENT);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_plutus_data_unref(&redeemer);
}

TEST(cardano_tx_builder_delegate_stake_ex, returnsErrorOnMemoryAllocationFail)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = nullptr;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  for (int i = 0; i < 44; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_delegate_stake_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), "pool1pzdqdxrv0k74p4q33y98f2u7vzaz95et7mjeedjcfy0jcgk754f", strlen("pool1pzdqdxrv0k74p4q33y98f2u7vzaz95et7mjeedjcfy0jcgk754f"), redeemer);

    // Assert
    EXPECT_NE(tx_builder->last_error, CARDANO_SUCCESS);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_plutus_data_unref(&redeemer);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_delegate_voting_power, doesntCrashIfGivenNull)
{
  cardano_tx_builder_delegate_voting_power(nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_delegate_voting_power, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = nullptr;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_drep_t*                drep           = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_KEY_HASH_CBOR, strlen(DREP_KEY_HASH_CBOR));

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_drep_from_cbor(reader, &drep), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_delegate_voting_power(tx_builder, nullptr, drep, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_delegate_voting_power(tx_builder, reward_address, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_drep_unref(&drep);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_tx_builder_delegate_voting_power, canDelegateVotingPower)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = nullptr;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_drep_t*                drep           = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_KEY_HASH_CBOR, strlen(DREP_KEY_HASH_CBOR));

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_drep_from_cbor(reader, &drep), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_delegate_voting_power(tx_builder, reward_address, drep, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_drep_unref(&drep);
  cardano_plutus_data_unref(&redeemer);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_tx_builder_delegate_voting_power, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = nullptr;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_drep_t*                drep           = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_KEY_HASH_CBOR, strlen(DREP_KEY_HASH_CBOR));

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_drep_from_cbor(reader, &drep), CARDANO_SUCCESS);

  for (int i = 0; i < 10; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_delegate_voting_power(tx_builder, reward_address, drep, redeemer);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_drep_unref(&drep);
  cardano_plutus_data_unref(&redeemer);
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_add_certificate, doesntCrashIfGivenNull)
{
  cardano_tx_builder_add_certificate(nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_add_certificate, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;
  cardano_certificate_t*         cert     = nullptr;

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_add_certificate(tx_builder, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_delegate_voting_power_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;

  cardano_tx_builder_delegate_voting_power_ex(nullptr, nullptr, 0, nullptr, 0, nullptr);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_delegate_voting_power_ex(tx_builder, nullptr, 0, nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_delegate_voting_power_ex(tx_builder, "1", 1, nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_delegate_voting_power_ex(tx_builder, "1", 1, "1", 1, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_delegate_voting_power_ex, canDelegateVotingPower)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = nullptr;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_drep_t*                drep           = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_KEY_HASH_CBOR, strlen(DREP_KEY_HASH_CBOR));

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_drep_from_cbor(reader, &drep), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_delegate_voting_power_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), DREP_ID, strlen(DREP_ID), redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_drep_unref(&drep);
  cardano_plutus_data_unref(&redeemer);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_tx_builder_register_drep, doesntCrashIfGivenNull)
{
  cardano_tx_builder_register_drep(nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_register_drep, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;
  cardano_drep_t*                drep     = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_KEY_HASH_CBOR, strlen(DREP_KEY_HASH_CBOR));

  EXPECT_EQ(cardano_drep_from_cbor(reader, &drep), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_register_drep(tx_builder, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_drep_unref(&drep);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_tx_builder_register_drep, canRegisterDrep)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;
  cardano_drep_t*                drep     = nullptr;
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_anchor_t*              anchor   = nullptr;

  EXPECT_EQ(cardano_anchor_from_hash_hex(ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_KEY_HASH_CBOR, strlen(DREP_KEY_HASH_CBOR));

  EXPECT_EQ(cardano_drep_from_cbor(reader, &drep), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_register_drep(tx_builder, drep, anchor, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_drep_unref(&drep);
  cardano_plutus_data_unref(&redeemer);
  cardano_cbor_reader_unref(&reader);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_tx_builder_register_drep, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;
  cardano_drep_t*                drep     = nullptr;
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_anchor_t*              anchor   = nullptr;

  EXPECT_EQ(cardano_anchor_from_hash_hex(ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_KEY_HASH_CBOR, strlen(DREP_KEY_HASH_CBOR));

  EXPECT_EQ(cardano_drep_from_cbor(reader, &drep), CARDANO_SUCCESS);

  for (int i = 0; i < 10; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_register_drep(tx_builder, drep, anchor, redeemer);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_drep_unref(&drep);
  cardano_plutus_data_unref(&redeemer);
  cardano_cbor_reader_unref(&reader);
  cardano_anchor_unref(&anchor);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_register_drep_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;

  cardano_tx_builder_register_drep_ex(nullptr, nullptr, 0, nullptr, 0, nullptr, 0, nullptr);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_register_drep_ex(tx_builder, nullptr, 0, nullptr, 0, nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_register_drep_ex(tx_builder, DREP_ID, strlen(DREP_ID), nullptr, 0, nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_register_drep_ex(tx_builder, DREP_ID, strlen(DREP_ID), ANCHOR_URL, strlen(DREP_ID), nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_register_drep_ex(tx_builder, "1", 1, ANCHOR_URL, strlen(DREP_ID), nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_register_drep_ex(tx_builder, DREP_ID, strlen(DREP_ID), "1", 1, "1", 1, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_register_drep_ex, canRegisterDrep)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_anchor_t*              anchor   = nullptr;

  EXPECT_EQ(cardano_anchor_from_hash_hex(ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), &anchor), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_register_drep_ex(tx_builder, DREP_ID, strlen(DREP_ID), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_plutus_data_unref(&redeemer);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_tx_builder_update_drep, doesntCrashIfGivenNull)
{
  cardano_tx_builder_update_drep(nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_update_drep, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;
  cardano_drep_t*                drep     = nullptr;
  cardano_anchor_t*              anchor   = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_KEY_HASH_CBOR, strlen(DREP_KEY_HASH_CBOR));

  EXPECT_EQ(cardano_drep_from_cbor(reader, &drep), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_anchor_from_hash_hex(ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), &anchor), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_update_drep(tx_builder, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_drep_unref(&drep);
  cardano_anchor_unref(&anchor);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_tx_builder_update_drep, canUpdateDrep)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;
  cardano_drep_t*                drep     = nullptr;
  cardano_anchor_t*              anchor   = nullptr;
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_KEY_HASH_CBOR, strlen(DREP_KEY_HASH_CBOR));

  EXPECT_EQ(cardano_drep_from_cbor(reader, &drep), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_anchor_from_hash_hex(ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), &anchor), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_update_drep(tx_builder, drep, anchor, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_drep_unref(&drep);
  cardano_plutus_data_unref(&redeemer);
  cardano_anchor_unref(&anchor);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_tx_builder_update_drep, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;
  cardano_drep_t*                drep     = nullptr;
  cardano_anchor_t*              anchor   = nullptr;
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_KEY_HASH_CBOR, strlen(DREP_KEY_HASH_CBOR));

  EXPECT_EQ(cardano_drep_from_cbor(reader, &drep), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_anchor_from_hash_hex(ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), &anchor), CARDANO_SUCCESS);

  for (int i = 0; i < 10; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_update_drep(tx_builder, drep, anchor, redeemer);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_drep_unref(&drep);
  cardano_plutus_data_unref(&redeemer);
  cardano_anchor_unref(&anchor);
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_update_drep_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;

  cardano_tx_builder_update_drep_ex(nullptr, nullptr, 0, nullptr, 0, nullptr, 0, nullptr);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_update_drep_ex(tx_builder, nullptr, 0, nullptr, 0, nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_update_drep_ex(tx_builder, DREP_ID, strlen(DREP_ID), nullptr, 0, nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_update_drep_ex(tx_builder, DREP_ID, strlen(DREP_ID), ANCHOR_URL, strlen(ANCHOR_URL), nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_update_drep_ex(tx_builder, "1", 1, ANCHOR_URL, strlen(ANCHOR_URL), "1", 1, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_update_drep_ex(tx_builder, "1", 1, ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr);
  cardano_tx_builder_unref(&tx_builder);

  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_update_drep_ex, canUpdateDrep)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_anchor_t*              anchor   = nullptr;

  EXPECT_EQ(cardano_anchor_from_hash_hex(ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), &anchor), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_update_drep_ex(tx_builder, DREP_ID, strlen(DREP_ID), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_plutus_data_unref(&redeemer);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_tx_builder_deregister_drep, doesntCrashIfGivenNull)
{
  cardano_tx_builder_deregister_drep(nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_deregister_drep, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;
  cardano_drep_t*                drep     = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_KEY_HASH_CBOR, strlen(DREP_KEY_HASH_CBOR));

  EXPECT_EQ(cardano_drep_from_cbor(reader, &drep), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_deregister_drep(tx_builder, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_drep_unref(&drep);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_tx_builder_deregister_drep, canDeregisterDrep)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;
  cardano_drep_t*                drep     = nullptr;
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_KEY_HASH_CBOR, strlen(DREP_KEY_HASH_CBOR));

  EXPECT_EQ(cardano_drep_from_cbor(reader, &drep), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_deregister_drep(tx_builder, drep, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_drep_unref(&drep);
  cardano_plutus_data_unref(&redeemer);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_tx_builder_deregister_drep, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;
  cardano_drep_t*                drep     = nullptr;
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_KEY_HASH_CBOR, strlen(DREP_KEY_HASH_CBOR));

  EXPECT_EQ(cardano_drep_from_cbor(reader, &drep), CARDANO_SUCCESS);

  for (int i = 0; i < 10; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_deregister_drep(tx_builder, drep, redeemer);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);

  cardano_drep_unref(&drep);
  cardano_plutus_data_unref(&redeemer);
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_deregister_drep_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;

  cardano_tx_builder_deregister_drep_ex(nullptr, nullptr, 0, nullptr);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_deregister_drep_ex(tx_builder, nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_deregister_drep_ex(tx_builder, DREP_ID, strlen(DREP_ID), nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_deregister_drep_ex(tx_builder, "1", 1, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_deregister_drep_ex, canDeregisterDrep)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_deregister_drep_ex(tx_builder, DREP_ID, strlen(DREP_ID), redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_plutus_data_unref(&redeemer);
}

TEST(cardano_tx_builder_vote, doesntCrashIfGivenNull)
{
  cardano_tx_builder_vote(nullptr, nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_vote, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t*  params    = init_protocol_parameters();
  cardano_provider_t*             provider  = nullptr;
  cardano_voter_t*                voter     = new_default_voter();
  cardano_voter_t*                voter2    = new_default_voter2();
  cardano_voting_procedure_t*     proc      = new_default_voting_procedure();
  cardano_plutus_data_t*          redeemer  = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_governance_action_id_t* action_id = nullptr;
  cardano_voting_procedure_t*     procedure = nullptr;

  cardano_cbor_reader_t* gov_action_reader       = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  cardano_cbor_reader_t* voting_procedure_reader = cardano_cbor_reader_from_hex(CBOR_YES_WITH_ANCHOR, strlen(CBOR_YES_WITH_ANCHOR));

  EXPECT_EQ(cardano_governance_action_id_from_cbor(gov_action_reader, &action_id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_voting_procedure_from_cbor(voting_procedure_reader, &procedure), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_vote(tx_builder, nullptr, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_vote(tx_builder, voter, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_vote(tx_builder, voter, action_id, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_vote(tx_builder, voter, action_id, proc, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_SUCCESS);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_vote(tx_builder, voter, action_id, proc, redeemer);
  EXPECT_THAT(tx_builder->last_error, CARDANO_SUCCESS);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_vote(tx_builder, voter2, action_id, proc, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_SUCCESS);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_vote(tx_builder, voter2, action_id, proc, redeemer);
  EXPECT_THAT(tx_builder->last_error, CARDANO_SUCCESS);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_plutus_data_unref(&redeemer);
  cardano_governance_action_id_unref(&action_id);
  cardano_voting_procedure_unref(&procedure);
  cardano_cbor_reader_unref(&gov_action_reader);
  cardano_cbor_reader_unref(&voting_procedure_reader);
  cardano_voter_unref(&voter);
  cardano_voter_unref(&voter2);
  cardano_voting_procedure_unref(&proc);
}

TEST(cardano_tx_builder_vote, canVote)
{
  // Arrange
  cardano_protocol_parameters_t*  params    = init_protocol_parameters();
  cardano_provider_t*             provider  = nullptr;
  cardano_voter_t*                voter     = new_default_voter();
  cardano_plutus_data_t*          redeemer  = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_governance_action_id_t* action_id = nullptr;
  cardano_voting_procedure_t*     procedure = nullptr;

  cardano_cbor_reader_t* gov_action_reader       = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  cardano_cbor_reader_t* voting_procedure_reader = cardano_cbor_reader_from_hex(CBOR_YES_WITH_ANCHOR, strlen(CBOR_YES_WITH_ANCHOR));

  EXPECT_EQ(cardano_governance_action_id_from_cbor(gov_action_reader, &action_id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_voting_procedure_from_cbor(voting_procedure_reader, &procedure), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_vote(tx_builder, voter, action_id, procedure, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_voting_procedures_t* procedures = cardano_transaction_body_get_voting_procedures(body);
  cardano_voting_procedures_unref(&procedures);

  cardano_voter_list_t* voters = NULL;
  EXPECT_EQ(cardano_voting_procedures_get_voters(procedures, &voters), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(cardano_voter_list_get_length(voters), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_plutus_data_unref(&redeemer);
  cardano_governance_action_id_unref(&action_id);
  cardano_voting_procedure_unref(&procedure);
  cardano_cbor_reader_unref(&gov_action_reader);
  cardano_cbor_reader_unref(&voting_procedure_reader);
  cardano_voter_unref(&voter);
  cardano_voter_list_unref(&voters);
}

TEST(cardano_tx_builder_vote, keyHashVotersOccupyARedeemerIndexSlot)
{
  // Arrange
  cardano_protocol_parameters_t*  params    = init_protocol_parameters();
  cardano_plutus_data_t*          redeemer  = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_governance_action_id_t* action_id = nullptr;
  cardano_voting_procedure_t*     procedure = nullptr;

  cardano_cbor_reader_t* gov_action_reader       = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  cardano_cbor_reader_t* voting_procedure_reader = cardano_cbor_reader_from_hex(CBOR_YES_WITH_ANCHOR, strlen(CBOR_YES_WITH_ANCHOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(gov_action_reader, &action_id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_voting_procedure_from_cbor(voting_procedure_reader, &procedure), CARDANO_SUCCESS);

  static const char* CC_KEYHASH_VOTER  = "8200581caaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
  static const char* DREP_SCRIPT_VOTER = "8203581cbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";

  cardano_voter_t*       cc_keyhash_voter  = nullptr;
  cardano_voter_t*       drep_script_voter = nullptr;
  cardano_cbor_reader_t* r1                = cardano_cbor_reader_from_hex(CC_KEYHASH_VOTER, strlen(CC_KEYHASH_VOTER));
  cardano_cbor_reader_t* r2                = cardano_cbor_reader_from_hex(DREP_SCRIPT_VOTER, strlen(DREP_SCRIPT_VOTER));
  ASSERT_EQ(cardano_voter_from_cbor(r1, &cc_keyhash_voter), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_voter_from_cbor(r2, &drep_script_voter), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_vote(tx_builder, cc_keyhash_voter, action_id, procedure, nullptr);
  cardano_tx_builder_vote(tx_builder, drep_script_voter, action_id, procedure, redeemer);

  // Assert
  EXPECT_EQ(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_to_redeemer_map_get_length(tx_builder->state.votes_to_redeemer_map), 2U);

  size_t non_null_slots = 0;
  for (size_t i = 0; i < cardano_blake2b_hash_to_redeemer_map_get_length(tx_builder->state.votes_to_redeemer_map); ++i)
  {
    cardano_redeemer_t* value = nullptr;
    EXPECT_EQ(cardano_blake2b_hash_to_redeemer_map_get_value_at(tx_builder->state.votes_to_redeemer_map, i, &value), CARDANO_SUCCESS);

    if (value != nullptr)
    {
      ++non_null_slots;
      EXPECT_EQ(cardano_redeemer_get_index(value), i);
    }

    cardano_redeemer_unref(&value);
  }
  EXPECT_EQ(non_null_slots, 1U);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_plutus_data_unref(&redeemer);
  cardano_governance_action_id_unref(&action_id);
  cardano_voting_procedure_unref(&procedure);
  cardano_cbor_reader_unref(&gov_action_reader);
  cardano_cbor_reader_unref(&voting_procedure_reader);
  cardano_cbor_reader_unref(&r1);
  cardano_cbor_reader_unref(&r2);
  cardano_voter_unref(&cc_keyhash_voter);
  cardano_voter_unref(&drep_script_voter);
}

TEST(cardano_tx_builder_vote, oneVoterVotingOnSeveralActionsUsesASingleRedeemerSlot)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);

  static const char* GOV_ACTION_A      = "825820000000000000000000000000000000000000000000000000000000000000000003";
  static const char* GOV_ACTION_B      = "825820000000000000000000000000000000000000000000000000000000000000000004";
  static const char* DREP_SCRIPT_VOTER = "8203581cbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";

  cardano_governance_action_id_t* action_a  = nullptr;
  cardano_governance_action_id_t* action_b  = nullptr;
  cardano_voting_procedure_t*     procedure = nullptr;
  cardano_voter_t*                voter     = nullptr;

  cardano_cbor_reader_t* ra = cardano_cbor_reader_from_hex(GOV_ACTION_A, strlen(GOV_ACTION_A));
  cardano_cbor_reader_t* rb = cardano_cbor_reader_from_hex(GOV_ACTION_B, strlen(GOV_ACTION_B));
  cardano_cbor_reader_t* rp = cardano_cbor_reader_from_hex(CBOR_YES_WITH_ANCHOR, strlen(CBOR_YES_WITH_ANCHOR));
  cardano_cbor_reader_t* rv = cardano_cbor_reader_from_hex(DREP_SCRIPT_VOTER, strlen(DREP_SCRIPT_VOTER));

  ASSERT_EQ(cardano_governance_action_id_from_cbor(ra, &action_a), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_governance_action_id_from_cbor(rb, &action_b), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_voting_procedure_from_cbor(rp, &procedure), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_voter_from_cbor(rv, &voter), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_vote(tx_builder, voter, action_a, procedure, redeemer);
  cardano_tx_builder_vote(tx_builder, voter, action_b, procedure, redeemer);

  // Assert
  EXPECT_EQ(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_to_redeemer_map_get_length(tx_builder->state.votes_to_redeemer_map), 1U);

  cardano_redeemer_t* value = nullptr;
  EXPECT_EQ(cardano_blake2b_hash_to_redeemer_map_get_value_at(tx_builder->state.votes_to_redeemer_map, 0, &value), CARDANO_SUCCESS);
  ASSERT_NE(value, (cardano_redeemer_t*)nullptr);
  EXPECT_EQ(cardano_redeemer_get_index(value), 0U);
  cardano_redeemer_unref(&value);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_plutus_data_unref(&redeemer);
  cardano_governance_action_id_unref(&action_a);
  cardano_governance_action_id_unref(&action_b);
  cardano_voting_procedure_unref(&procedure);
  cardano_voter_unref(&voter);
  cardano_cbor_reader_unref(&ra);
  cardano_cbor_reader_unref(&rb);
  cardano_cbor_reader_unref(&rp);
  cardano_cbor_reader_unref(&rv);
}

TEST(cardano_tx_builder_vote, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*  params    = init_protocol_parameters();
  cardano_provider_t*             provider  = nullptr;
  cardano_voter_t*                voter     = new_default_voter();
  cardano_voter_t*                voter2    = new_default_voter2();
  cardano_plutus_data_t*          redeemer  = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_governance_action_id_t* action_id = nullptr;
  cardano_voting_procedure_t*     procedure = nullptr;

  cardano_cbor_reader_t* gov_action_reader       = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  cardano_cbor_reader_t* voting_procedure_reader = cardano_cbor_reader_from_hex(CBOR_YES_WITH_ANCHOR, strlen(CBOR_YES_WITH_ANCHOR));

  EXPECT_EQ(cardano_governance_action_id_from_cbor(gov_action_reader, &action_id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_voting_procedure_from_cbor(voting_procedure_reader, &procedure), CARDANO_SUCCESS);

  for (int i = 0; i < 52; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_vote(tx_builder, voter, action_id, procedure, redeemer);
    cardano_tx_builder_vote(tx_builder, voter, action_id, procedure, NULL);
    cardano_tx_builder_vote(tx_builder, voter2, action_id, procedure, redeemer);
    cardano_tx_builder_vote(tx_builder, voter2, action_id, procedure, NULL);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_plutus_data_unref(&redeemer);
  cardano_governance_action_id_unref(&action_id);
  cardano_voting_procedure_unref(&procedure);
  cardano_cbor_reader_unref(&gov_action_reader);
  cardano_cbor_reader_unref(&voting_procedure_reader);
  cardano_voter_unref(&voter);
  cardano_voter_unref(&voter2);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_propose_parameter_change, doesntCrashIfGivenNull)
{
  cardano_tx_builder_propose_parameter_change(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_propose_parameter_change, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t*   params         = init_protocol_parameters();
  cardano_provider_t*              provider       = nullptr;
  cardano_reward_address_t*        reward_address = nullptr;
  cardano_governance_action_id_t*  action_id      = nullptr;
  cardano_anchor_t*                anchor         = nullptr;
  cardano_protocol_param_update_t* pparam_update  = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(PROTOCOL_PARAM_UPDATE, strlen(PROTOCOL_PARAM_UPDATE));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &pparam_update), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_parameter_change(tx_builder, nullptr, nullptr, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_parameter_change(tx_builder, reward_address, nullptr, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_parameter_change(tx_builder, reward_address, anchor, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_parameter_change(tx_builder, reward_address, anchor, pparam_update, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_parameter_change(tx_builder, reward_address, anchor, pparam_update, action_id, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_protocol_param_update_unref(&pparam_update);
}

TEST(cardano_tx_builder_propose_parameter_change, canProposeParameterChange)
{
  // Arrange
  cardano_protocol_parameters_t*   params         = init_protocol_parameters();
  cardano_provider_t*              provider       = nullptr;
  cardano_reward_address_t*        reward_address = nullptr;
  cardano_governance_action_id_t*  action_id      = nullptr;
  cardano_anchor_t*                anchor         = nullptr;
  cardano_protocol_param_update_t* pparam_update  = nullptr;
  cardano_blake2b_hash_t*          hash           = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &hash), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(PROTOCOL_PARAM_UPDATE, strlen(PROTOCOL_PARAM_UPDATE));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &pparam_update), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_propose_parameter_change(tx_builder, reward_address, anchor, pparam_update, action_id, hash);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* actions = cardano_transaction_body_get_proposal_procedures(body);
  cardano_proposal_procedure_set_unref(&actions);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(actions), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_protocol_param_update_unref(&pparam_update);
  cardano_blake2b_hash_unref(&hash);
}

TEST(cardano_tx_builder_propose_parameter_change, returnsErrorIfMemoryAllocaitonFails)
{
  // Arrange
  cardano_protocol_parameters_t*   params         = init_protocol_parameters();
  cardano_provider_t*              provider       = nullptr;
  cardano_reward_address_t*        reward_address = nullptr;
  cardano_governance_action_id_t*  action_id      = nullptr;
  cardano_anchor_t*                anchor         = nullptr;
  cardano_protocol_param_update_t* pparam_update  = nullptr;
  cardano_blake2b_hash_t*          hash           = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &hash), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(PROTOCOL_PARAM_UPDATE, strlen(PROTOCOL_PARAM_UPDATE));
  EXPECT_EQ(cardano_protocol_param_update_from_cbor(reader, &pparam_update), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  for (int i = 0; i < 61; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_propose_parameter_change(tx_builder, reward_address, anchor, pparam_update, action_id, hash);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_protocol_param_update_unref(&pparam_update);
  cardano_blake2b_hash_unref(&hash);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_propose_parameter_change_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t*   params        = init_protocol_parameters();
  cardano_provider_t*              provider      = nullptr;
  cardano_protocol_param_update_t* pparam_update = nullptr;

  // clang-format off
  cardano_tx_builder_propose_parameter_change_ex(
    nullptr,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    pparam_update);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_parameter_change_ex(
    tx_builder,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    pparam_update);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_parameter_change_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    pparam_update);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_parameter_change_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    pparam_update);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_parameter_change_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    ANCHOR_HASH, strlen(ANCHOR_HASH),
    nullptr, 0,
    nullptr, 0,
    pparam_update);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_parameter_change_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    ANCHOR_HASH, strlen(ANCHOR_HASH),
    CIP129_BECH32_1, strlen(CIP129_BECH32_1),
    nullptr, 0,
    pparam_update);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_parameter_change_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    ANCHOR_HASH, strlen(ANCHOR_HASH),
    CIP129_BECH32_1, strlen(CIP129_BECH32_1), // Gov action ID
    HASH_HEX, strlen(HASH_HEX), // Script hash
    nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_parameter_change_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    ANCHOR_HASH, strlen(ANCHOR_HASH),
    CIP129_BECH32_1, strlen(CIP129_BECH32_1), // Gov action ID
    "xxx", 3, // Script hash
    nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_parameter_change_ex(
    tx_builder,
    "xxxxx", 5,
    ANCHOR_URL, strlen(ANCHOR_URL),
    ANCHOR_HASH, strlen(ANCHOR_HASH),
    CIP129_BECH32_1, strlen(CIP129_BECH32_1), // Gov action ID
    HASH_HEX, strlen(HASH_HEX), // Script hash
    nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_parameter_change_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    "yyyyy", 5,
    CIP129_BECH32_1, strlen(CIP129_BECH32_1), // Gov action ID
    HASH_HEX, strlen(HASH_HEX), // Script hash
    nullptr);
  cardano_tx_builder_unref(&tx_builder);

  // clang-format on

  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_propose_hardfork, doesntCrashIfGivenNull)
{
  cardano_tx_builder_propose_hardfork(nullptr, nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_propose_hardfork, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t*  params           = init_protocol_parameters();
  cardano_provider_t*             provider         = nullptr;
  cardano_reward_address_t*       reward_address   = nullptr;
  cardano_governance_action_id_t* action_id        = nullptr;
  cardano_anchor_t*               anchor           = nullptr;
  cardano_protocol_version_t*     protocol_version = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_protocol_version_new(1, 2, &protocol_version), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_hardfork(tx_builder, nullptr, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_hardfork(tx_builder, reward_address, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_hardfork(tx_builder, reward_address, anchor, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_protocol_version_unref(&protocol_version);
}

TEST(cardano_tx_builder_propose_hardfork, canProposeHardfork)
{
  // Arrange
  cardano_protocol_parameters_t*  params           = init_protocol_parameters();
  cardano_provider_t*             provider         = nullptr;
  cardano_reward_address_t*       reward_address   = nullptr;
  cardano_governance_action_id_t* action_id        = nullptr;
  cardano_anchor_t*               anchor           = nullptr;
  cardano_protocol_version_t*     protocol_version = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_protocol_version_new(1, 2, &protocol_version), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_propose_hardfork(tx_builder, reward_address, anchor, protocol_version, action_id);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* actions = cardano_transaction_body_get_proposal_procedures(body);
  cardano_proposal_procedure_set_unref(&actions);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(actions), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_protocol_version_unref(&protocol_version);
}

TEST(cardano_tx_builder_propose_hardfork, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*  params           = init_protocol_parameters();
  cardano_provider_t*             provider         = nullptr;
  cardano_reward_address_t*       reward_address   = nullptr;
  cardano_governance_action_id_t* action_id        = nullptr;
  cardano_anchor_t*               anchor           = nullptr;
  cardano_protocol_version_t*     protocol_version = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_protocol_version_new(1, 2, &protocol_version), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  for (int i = 0; i < 2; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_propose_hardfork(tx_builder, reward_address, anchor, protocol_version, action_id);

    // Assert
    EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_protocol_version_unref(&protocol_version);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_propose_hardfork_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;

  // clang-format off
  cardano_tx_builder_propose_hardfork_ex(
    nullptr,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    0, 0);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_hardfork_ex(
    tx_builder,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    0, 0);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_hardfork_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    0, 0);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_hardfork_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    nullptr, 0,
    nullptr, 0,
    0, 0);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_hardfork_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    ANCHOR_HASH, strlen(ANCHOR_HASH),
    nullptr, 0,
    0, 0);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_hardfork_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    ANCHOR_HASH, strlen(ANCHOR_HASH),
    CIP129_BECH32_1, strlen(CIP129_BECH32_1),
    0, 0);
  cardano_tx_builder_unref(&tx_builder);

  // clang-format on

  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_propose_hardfork_ex, canProposeHardfork)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_protocol_parameters();
  cardano_tx_builder_t*          tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_propose_hardfork_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), CIP129_BECH32_1, strlen(CIP129_BECH32_1), 0, 12);

  char*                       proposals_hex = encode_proposal_procedures(tx_builder);
  cardano_protocol_version_t* version       = get_hardfork_proposal_version(tx_builder, 0);

  // Assert
  EXPECT_EQ(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(get_proposal_procedures(tx_builder)), 1);
  EXPECT_EQ(cardano_protocol_version_get_major(version), 12U);
  EXPECT_EQ(cardano_protocol_version_get_minor(version), 0U);
  EXPECT_STREQ(proposals_hex, HARDFORK_PROPOSALS_CBOR);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_protocol_version_unref(&version);
  free(proposals_hex);
}

TEST(cardano_tx_builder_propose_hardfork_ex, producesTheSameProposalAsTheObjectVariant)
{
  // Arrange
  cardano_protocol_parameters_t*  params           = init_protocol_parameters();
  cardano_reward_address_t*       reward_address   = nullptr;
  cardano_governance_action_id_t* action_id        = nullptr;
  cardano_anchor_t*               anchor           = nullptr;
  cardano_protocol_version_t*     protocol_version = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_anchor_from_hash_hex(ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), &anchor), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_governance_action_id_from_bech32(CIP129_BECH32_1, strlen(CIP129_BECH32_1), &action_id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_version_new(12, 0, &protocol_version), CARDANO_SUCCESS);

  cardano_tx_builder_t* object_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_t* string_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_propose_hardfork(object_builder, reward_address, anchor, protocol_version, action_id);
  cardano_tx_builder_propose_hardfork_ex(string_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), CIP129_BECH32_1, strlen(CIP129_BECH32_1), 0, 12);

  char* object_proposals_hex = encode_proposal_procedures(object_builder);
  char* string_proposals_hex = encode_proposal_procedures(string_builder);

  // Assert
  EXPECT_EQ(object_builder->last_error, CARDANO_SUCCESS);
  EXPECT_EQ(string_builder->last_error, CARDANO_SUCCESS);
  EXPECT_STREQ(string_proposals_hex, object_proposals_hex);
  EXPECT_STREQ(object_proposals_hex, HARDFORK_PROPOSALS_CBOR);

  // Cleanup
  cardano_tx_builder_unref(&object_builder);
  cardano_tx_builder_unref(&string_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_protocol_version_unref(&protocol_version);
  free(object_proposals_hex);
  free(string_proposals_hex);
}

TEST(cardano_tx_builder_propose_hardfork_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;

  for (int i = 0; i < 39; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_propose_hardfork_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), CIP129_BECH32_1, strlen(CIP129_BECH32_1), 0, 0);

    // Assert
    EXPECT_NE(tx_builder->last_error, CARDANO_SUCCESS);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_propose_treasury_withdrawals, doesntCrashIfGivenNull)
{
  cardano_tx_builder_propose_treasury_withdrawals(nullptr, nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_propose_treasury_withdrawals, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t*  params              = init_protocol_parameters();
  cardano_provider_t*             provider            = nullptr;
  cardano_reward_address_t*       reward_address      = nullptr;
  cardano_governance_action_id_t* action_id           = nullptr;
  cardano_anchor_t*               anchor              = nullptr;
  cardano_withdrawal_map_t*       treasury_withdrawal = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(WITHDRAWAL_MAP_CBOR, strlen(WITHDRAWAL_MAP_CBOR));
  EXPECT_EQ(cardano_withdrawal_map_from_cbor(reader, &treasury_withdrawal), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_treasury_withdrawals(tx_builder, nullptr, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_treasury_withdrawals(tx_builder, reward_address, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_treasury_withdrawals(tx_builder, reward_address, anchor, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_treasury_withdrawals(tx_builder, reward_address, anchor, treasury_withdrawal, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_withdrawal_map_unref(&treasury_withdrawal);
}

TEST(cardano_tx_builder_propose_treasury_withdrawals, canProposeTreasuryWithdrawals)
{
  // Arrange
  cardano_protocol_parameters_t* params              = init_protocol_parameters();
  cardano_provider_t*            provider            = nullptr;
  cardano_reward_address_t*      reward_address      = nullptr;
  cardano_anchor_t*              anchor              = nullptr;
  cardano_withdrawal_map_t*      treasury_withdrawal = nullptr;
  cardano_blake2b_hash_t*        policy_id           = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_id), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(WITHDRAWAL_MAP_CBOR, strlen(WITHDRAWAL_MAP_CBOR));
  EXPECT_EQ(cardano_withdrawal_map_from_cbor(reader, &treasury_withdrawal), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_propose_treasury_withdrawals(tx_builder, reward_address, anchor, treasury_withdrawal, policy_id);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* actions = cardano_transaction_body_get_proposal_procedures(body);
  cardano_proposal_procedure_set_unref(&actions);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(actions), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_withdrawal_map_unref(&treasury_withdrawal);
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_tx_builder_propose_treasury_withdrawals, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params              = init_protocol_parameters();
  cardano_provider_t*            provider            = nullptr;
  cardano_reward_address_t*      reward_address      = nullptr;
  cardano_anchor_t*              anchor              = nullptr;
  cardano_withdrawal_map_t*      treasury_withdrawal = nullptr;
  cardano_blake2b_hash_t*        policy_id           = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_id), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(WITHDRAWAL_MAP_CBOR, strlen(WITHDRAWAL_MAP_CBOR));
  EXPECT_EQ(cardano_withdrawal_map_from_cbor(reader, &treasury_withdrawal), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  for (int i = 0; i < 9; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_propose_treasury_withdrawals(tx_builder, reward_address, anchor, treasury_withdrawal, policy_id);

    // Assert
    EXPECT_NE(tx_builder->last_error, CARDANO_SUCCESS);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_withdrawal_map_unref(&treasury_withdrawal);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_propose_treasury_withdrawals_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params              = init_protocol_parameters();
  cardano_provider_t*            provider            = nullptr;
  cardano_withdrawal_map_t*      treasury_withdrawal = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(WITHDRAWAL_MAP_CBOR, strlen(WITHDRAWAL_MAP_CBOR));
  EXPECT_EQ(cardano_withdrawal_map_from_cbor(reader, &treasury_withdrawal), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  // clang-format off
  cardano_tx_builder_propose_treasury_withdrawals_ex(
    nullptr,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    treasury_withdrawal);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_treasury_withdrawals_ex(
    tx_builder,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    treasury_withdrawal);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_treasury_withdrawals_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    treasury_withdrawal);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_treasury_withdrawals_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    nullptr, 0,
    nullptr, 0,
    treasury_withdrawal);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_treasury_withdrawals_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    ANCHOR_HASH, strlen(ANCHOR_HASH),
    nullptr, 0,
    treasury_withdrawal);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_treasury_withdrawals_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    ANCHOR_HASH, strlen(ANCHOR_HASH),
    GOVERNANCE_ACTION_HEX, strlen(GOVERNANCE_ACTION_HEX),
    treasury_withdrawal);
  cardano_tx_builder_unref(&tx_builder);

  // clang-format on

  cardano_protocol_parameters_unref(&params);

  cardano_withdrawal_map_unref(&treasury_withdrawal);
}

TEST(cardano_tx_builder_propose_treasury_withdrawals_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params              = init_protocol_parameters();
  cardano_provider_t*            provider            = nullptr;
  cardano_withdrawal_map_t*      treasury_withdrawal = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(WITHDRAWAL_MAP_CBOR, strlen(WITHDRAWAL_MAP_CBOR));
  EXPECT_EQ(cardano_withdrawal_map_from_cbor(reader, &treasury_withdrawal), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  for (int i = 0; i < 93; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_propose_treasury_withdrawals_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), HASH_HEX, strlen(HASH_HEX), treasury_withdrawal);

    // Assert
    EXPECT_NE(tx_builder->last_error, CARDANO_SUCCESS);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_withdrawal_map_unref(&treasury_withdrawal);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_propose_no_confidence, doesntCrashIfGivenNull)
{
  cardano_tx_builder_propose_no_confidence(nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_propose_no_confidence, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_protocol_parameters();
  cardano_provider_t*             provider       = nullptr;
  cardano_reward_address_t*       reward_address = nullptr;
  cardano_governance_action_id_t* action_id      = nullptr;
  cardano_anchor_t*               anchor         = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_no_confidence(tx_builder, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_no_confidence(tx_builder, reward_address, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_tx_builder_propose_no_confidence, canProposeNoConfidence)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_protocol_parameters();
  cardano_provider_t*             provider       = nullptr;
  cardano_reward_address_t*       reward_address = nullptr;
  cardano_governance_action_id_t* action_id      = nullptr;
  cardano_anchor_t*               anchor         = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_propose_no_confidence(tx_builder, reward_address, anchor, action_id);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* actions = cardano_transaction_body_get_proposal_procedures(body);
  cardano_proposal_procedure_set_unref(&actions);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(actions), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_tx_builder_propose_no_confidence, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_protocol_parameters();
  cardano_provider_t*             provider       = nullptr;
  cardano_reward_address_t*       reward_address = nullptr;
  cardano_governance_action_id_t* action_id      = nullptr;
  cardano_anchor_t*               anchor         = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  for (int i = 0; i < 5; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_propose_no_confidence(tx_builder, reward_address, anchor, action_id);

    // Assert
    EXPECT_NE(tx_builder->last_error, CARDANO_SUCCESS);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_propose_no_confidence_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;

  // clang-format off
  cardano_tx_builder_propose_no_confidence_ex(
    nullptr,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_no_confidence_ex(
    tx_builder,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_no_confidence_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    nullptr, 0,
    nullptr, 0,
    nullptr, 0);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_no_confidence_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    nullptr, 0,
    nullptr, 0);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_no_confidence_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    ANCHOR_HASH, strlen(ANCHOR_HASH),
    nullptr, 0);
  cardano_tx_builder_unref(&tx_builder);

  // clang-format on

  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_propose_no_confidence_ex, canAddNoConfidencePorposal)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_protocol_parameters();
  cardano_provider_t*             provider       = nullptr;
  cardano_reward_address_t*       reward_address = nullptr;
  cardano_governance_action_id_t* action_id      = nullptr;
  cardano_anchor_t*               anchor         = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_propose_no_confidence_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), CIP129_BECH32_1, strlen(CIP129_BECH32_1));

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* actions = cardano_transaction_body_get_proposal_procedures(body);
  cardano_proposal_procedure_set_unref(&actions);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(actions), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_tx_builder_propose_no_confidence_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_protocol_parameters();
  cardano_provider_t*             provider       = nullptr;
  cardano_reward_address_t*       reward_address = nullptr;
  cardano_governance_action_id_t* action_id      = nullptr;
  cardano_anchor_t*               anchor         = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  for (int i = 0; i < 38; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_propose_no_confidence_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), CIP129_BECH32_1, strlen(CIP129_BECH32_1));

    // Assert
    EXPECT_NE(tx_builder->last_error, CARDANO_SUCCESS);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_propose_update_committee, doesntCrashIfGivenNull)
{
  cardano_tx_builder_propose_update_committee(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_propose_update_committee, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t*   params                = init_protocol_parameters();
  cardano_provider_t*              provider              = nullptr;
  cardano_reward_address_t*        reward_address        = nullptr;
  cardano_governance_action_id_t*  action_id             = nullptr;
  cardano_anchor_t*                anchor                = nullptr;
  cardano_credential_set_t*        members_to_be_removed = nullptr;
  cardano_committee_members_map_t* members_to_be_added   = nullptr;
  cardano_unit_interval_t*         new_quorum            = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(CREDENTIAL_SET_CBOR, strlen(CREDENTIAL_SET_CBOR));
  EXPECT_EQ(cardano_credential_set_from_cbor(reader, &members_to_be_removed), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(COMITTEE_MEMBERS_MAP_CBOR, strlen(COMITTEE_MEMBERS_MAP_CBOR));
  EXPECT_EQ(cardano_committee_members_map_from_cbor(reader, &members_to_be_added), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_update_committee(tx_builder, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_update_committee(tx_builder, reward_address, nullptr, nullptr, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_update_committee(tx_builder, reward_address, anchor, nullptr, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_update_committee(tx_builder, reward_address, anchor, action_id, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_update_committee(tx_builder, reward_address, anchor, action_id, members_to_be_removed, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_update_committee(tx_builder, reward_address, anchor, action_id, members_to_be_removed, members_to_be_added, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_credential_set_unref(&members_to_be_removed);
  cardano_committee_members_map_unref(&members_to_be_added);
}

TEST(cardano_tx_builder_propose_update_committee, canProposeUpdateCommittee)
{
  // Arrange
  cardano_protocol_parameters_t*   params                = init_protocol_parameters();
  cardano_provider_t*              provider              = nullptr;
  cardano_reward_address_t*        reward_address        = nullptr;
  cardano_governance_action_id_t*  action_id             = nullptr;
  cardano_anchor_t*                anchor                = nullptr;
  cardano_credential_set_t*        members_to_be_removed = nullptr;
  cardano_committee_members_map_t* members_to_be_added   = nullptr;
  cardano_unit_interval_t*         new_quorum            = nullptr;

  EXPECT_EQ(cardano_unit_interval_new(0, 0, &new_quorum), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(CREDENTIAL_SET_CBOR, strlen(CREDENTIAL_SET_CBOR));
  EXPECT_EQ(cardano_credential_set_from_cbor(reader, &members_to_be_removed), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(COMITTEE_MEMBERS_MAP_CBOR, strlen(COMITTEE_MEMBERS_MAP_CBOR));
  EXPECT_EQ(cardano_committee_members_map_from_cbor(reader, &members_to_be_added), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_propose_update_committee(tx_builder, reward_address, anchor, action_id, members_to_be_removed, members_to_be_added, new_quorum);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* actions = cardano_transaction_body_get_proposal_procedures(body);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(actions), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_credential_set_unref(&members_to_be_removed);
  cardano_committee_members_map_unref(&members_to_be_added);
  cardano_unit_interval_unref(&new_quorum);
  cardano_proposal_procedure_set_unref(&actions);
}

TEST(cardano_tx_builder_propose_update_committee, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*   params                = init_protocol_parameters();
  cardano_provider_t*              provider              = nullptr;
  cardano_reward_address_t*        reward_address        = nullptr;
  cardano_governance_action_id_t*  action_id             = nullptr;
  cardano_anchor_t*                anchor                = nullptr;
  cardano_credential_set_t*        members_to_be_removed = nullptr;
  cardano_committee_members_map_t* members_to_be_added   = nullptr;
  cardano_unit_interval_t*         new_quorum            = nullptr;

  EXPECT_EQ(cardano_unit_interval_new(0, 0, &new_quorum), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(CREDENTIAL_SET_CBOR, strlen(CREDENTIAL_SET_CBOR));
  EXPECT_EQ(cardano_credential_set_from_cbor(reader, &members_to_be_removed), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(COMITTEE_MEMBERS_MAP_CBOR, strlen(COMITTEE_MEMBERS_MAP_CBOR));
  EXPECT_EQ(cardano_committee_members_map_from_cbor(reader, &members_to_be_added), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  for (int i = 0; i < 5; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_propose_update_committee(tx_builder, reward_address, anchor, action_id, members_to_be_removed, members_to_be_added, new_quorum);

    // Assert
    EXPECT_NE(tx_builder->last_error, CARDANO_SUCCESS);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_credential_set_unref(&members_to_be_removed);
  cardano_committee_members_map_unref(&members_to_be_added);
  cardano_unit_interval_unref(&new_quorum);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_propose_update_committee_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t*   params                = init_protocol_parameters();
  cardano_provider_t*              provider              = nullptr;
  cardano_credential_set_t*        members_to_be_removed = nullptr;
  cardano_committee_members_map_t* members_to_be_added   = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CREDENTIAL_SET_CBOR, strlen(CREDENTIAL_SET_CBOR));
  EXPECT_EQ(cardano_credential_set_from_cbor(reader, &members_to_be_removed), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(COMITTEE_MEMBERS_MAP_CBOR, strlen(COMITTEE_MEMBERS_MAP_CBOR));
  EXPECT_EQ(cardano_committee_members_map_from_cbor(reader, &members_to_be_added), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  // clang-format off
  cardano_tx_builder_propose_update_committee_ex(
    nullptr,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    nullptr,
    nullptr,
    0.0);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_update_committee_ex(
    tx_builder,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    nullptr,
    nullptr,
    0.0);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_update_committee_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    nullptr,
    nullptr,
    0.0);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_update_committee_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    nullptr, 0,
    nullptr, 0,
    nullptr,
    nullptr,
    0.0);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_update_committee_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    ANCHOR_HASH, strlen(ANCHOR_HASH),
    nullptr, 0,
    nullptr,
    nullptr,
    0.0);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_update_committee_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    ANCHOR_HASH, strlen(ANCHOR_HASH),
    CIP129_BECH32_1, strlen(CIP129_BECH32_1),
    nullptr,
    nullptr,
    0.0);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_update_committee_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    ANCHOR_HASH, strlen(ANCHOR_HASH),
    CIP129_BECH32_1, strlen(CIP129_BECH32_1),
    members_to_be_removed,
    nullptr,
    0.0);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_update_committee_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    ANCHOR_HASH, strlen(ANCHOR_HASH),
    CIP129_BECH32_1, strlen(CIP129_BECH32_1),
    members_to_be_removed,
    members_to_be_added,
    0.0);
  cardano_tx_builder_unref(&tx_builder);

  // clang-format on

  cardano_protocol_parameters_unref(&params);

  cardano_credential_set_unref(&members_to_be_removed);
  cardano_committee_members_map_unref(&members_to_be_added);
}

TEST(cardano_tx_builder_propose_update_committee_ex, canProposeUpdateCommittee)
{
  // Arrange
  cardano_protocol_parameters_t*   params                = init_protocol_parameters();
  cardano_provider_t*              provider              = nullptr;
  cardano_reward_address_t*        reward_address        = nullptr;
  cardano_governance_action_id_t*  action_id             = nullptr;
  cardano_anchor_t*                anchor                = nullptr;
  cardano_credential_set_t*        members_to_be_removed = nullptr;
  cardano_committee_members_map_t* members_to_be_added   = nullptr;
  cardano_unit_interval_t*         new_quorum            = nullptr;

  EXPECT_EQ(cardano_unit_interval_new(0, 0, &new_quorum), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(CREDENTIAL_SET_CBOR, strlen(CREDENTIAL_SET_CBOR));
  EXPECT_EQ(cardano_credential_set_from_cbor(reader, &members_to_be_removed), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(COMITTEE_MEMBERS_MAP_CBOR, strlen(COMITTEE_MEMBERS_MAP_CBOR));
  EXPECT_EQ(cardano_committee_members_map_from_cbor(reader, &members_to_be_added), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_propose_update_committee_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), CIP129_BECH32_1, strlen(CIP129_BECH32_1), members_to_be_removed, members_to_be_added, 0.0);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* actions = cardano_transaction_body_get_proposal_procedures(body);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(actions), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_credential_set_unref(&members_to_be_removed);
  cardano_committee_members_map_unref(&members_to_be_added);
  cardano_unit_interval_unref(&new_quorum);
  cardano_proposal_procedure_set_unref(&actions);
}

TEST(cardano_tx_builder_propose_update_committee_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*   params                = init_protocol_parameters();
  cardano_provider_t*              provider              = nullptr;
  cardano_reward_address_t*        reward_address        = nullptr;
  cardano_governance_action_id_t*  action_id             = nullptr;
  cardano_anchor_t*                anchor                = nullptr;
  cardano_credential_set_t*        members_to_be_removed = nullptr;
  cardano_committee_members_map_t* members_to_be_added   = nullptr;
  cardano_unit_interval_t*         new_quorum            = nullptr;

  EXPECT_EQ(cardano_unit_interval_new(0, 0, &new_quorum), CARDANO_SUCCESS);

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(CREDENTIAL_SET_CBOR, strlen(CREDENTIAL_SET_CBOR));
  EXPECT_EQ(cardano_credential_set_from_cbor(reader, &members_to_be_removed), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(COMITTEE_MEMBERS_MAP_CBOR, strlen(COMITTEE_MEMBERS_MAP_CBOR));
  EXPECT_EQ(cardano_committee_members_map_from_cbor(reader, &members_to_be_added), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  for (int i = 0; i < 38; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_propose_update_committee_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), CIP129_BECH32_1, strlen(CIP129_BECH32_1), members_to_be_removed, members_to_be_added, 0.0);

    // Assert
    EXPECT_NE(tx_builder->last_error, CARDANO_SUCCESS);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_credential_set_unref(&members_to_be_removed);
  cardano_committee_members_map_unref(&members_to_be_added);
  cardano_unit_interval_unref(&new_quorum);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_propose_new_constitution, doesntCrashIfGivenNull)
{
  cardano_tx_builder_propose_new_constitution(nullptr, nullptr, nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_propose_new_constitution, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_protocol_parameters();
  cardano_provider_t*             provider       = nullptr;
  cardano_reward_address_t*       reward_address = nullptr;
  cardano_governance_action_id_t* action_id      = nullptr;
  cardano_anchor_t*               anchor         = nullptr;
  cardano_constitution_t*         constitution   = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(CONSTITUTION_CBOR, strlen(CONSTITUTION_CBOR));
  EXPECT_EQ(cardano_constitution_from_cbor(reader, &constitution), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_new_constitution(tx_builder, nullptr, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_new_constitution(tx_builder, reward_address, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_new_constitution(tx_builder, reward_address, anchor, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_new_constitution(tx_builder, reward_address, anchor, action_id, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_constitution_unref(&constitution);
}

TEST(cardano_tx_builder_propose_new_constitution, canProposeNewConstitution)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_protocol_parameters();
  cardano_provider_t*             provider       = nullptr;
  cardano_reward_address_t*       reward_address = nullptr;
  cardano_governance_action_id_t* action_id      = nullptr;
  cardano_anchor_t*               anchor         = nullptr;
  cardano_constitution_t*         constitution   = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(CONSTITUTION_CBOR, strlen(CONSTITUTION_CBOR));
  EXPECT_EQ(cardano_constitution_from_cbor(reader, &constitution), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_propose_new_constitution(tx_builder, reward_address, anchor, action_id, constitution);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* actions = cardano_transaction_body_get_proposal_procedures(body);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(actions), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_constitution_unref(&constitution);
  cardano_proposal_procedure_set_unref(&actions);
}

TEST(cardano_tx_builder_propose_new_constitution, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_protocol_parameters();
  cardano_provider_t*             provider       = nullptr;
  cardano_reward_address_t*       reward_address = nullptr;
  cardano_governance_action_id_t* action_id      = nullptr;
  cardano_anchor_t*               anchor         = nullptr;
  cardano_constitution_t*         constitution   = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(CONSTITUTION_CBOR, strlen(CONSTITUTION_CBOR));
  EXPECT_EQ(cardano_constitution_from_cbor(reader, &constitution), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  for (int i = 0; i < 3; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_propose_new_constitution(tx_builder, reward_address, anchor, action_id, constitution);

    // Assert
    EXPECT_NE(tx_builder->last_error, CARDANO_SUCCESS);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);

  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_constitution_unref(&constitution);
}

TEST(cardano_tx_builder_propose_new_constitution_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params       = init_protocol_parameters();
  cardano_provider_t*            provider     = nullptr;
  cardano_constitution_t*        constitution = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CONSTITUTION_CBOR, strlen(CONSTITUTION_CBOR));
  EXPECT_EQ(cardano_constitution_from_cbor(reader, &constitution), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  // clang-format off
  cardano_tx_builder_propose_new_constitution_ex(
    nullptr,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    constitution);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_new_constitution_ex(
    tx_builder,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    constitution);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_new_constitution_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    nullptr, 0,
    nullptr, 0,
    nullptr, 0,
    constitution);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_new_constitution_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    nullptr, 0,
    nullptr, 0,
    constitution);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_new_constitution_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    ANCHOR_HASH, strlen(ANCHOR_HASH),
    nullptr, 0,
    constitution);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_new_constitution_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    ANCHOR_HASH, strlen(ANCHOR_HASH),
    CIP129_BECH32_1, strlen(CIP129_BECH32_1),
    constitution);
  cardano_tx_builder_unref(&tx_builder);

  // clang-format on

  cardano_protocol_parameters_unref(&params);

  cardano_constitution_unref(&constitution);
}

TEST(cardano_tx_builder_propose_new_constitution_ex, canProposeNewConstitution)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_protocol_parameters();
  cardano_provider_t*             provider       = nullptr;
  cardano_reward_address_t*       reward_address = nullptr;
  cardano_governance_action_id_t* action_id      = nullptr;
  cardano_anchor_t*               anchor         = nullptr;
  cardano_constitution_t*         constitution   = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(CONSTITUTION_CBOR, strlen(CONSTITUTION_CBOR));
  EXPECT_EQ(cardano_constitution_from_cbor(reader, &constitution), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_propose_new_constitution_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), CIP129_BECH32_1, strlen(CIP129_BECH32_1), constitution);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* actions = cardano_transaction_body_get_proposal_procedures(body);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(actions), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_constitution_unref(&constitution);
  cardano_proposal_procedure_set_unref(&actions);
}

TEST(cardano_tx_builder_propose_new_constitution_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*  params         = init_protocol_parameters();
  cardano_provider_t*             provider       = nullptr;
  cardano_reward_address_t*       reward_address = nullptr;
  cardano_governance_action_id_t* action_id      = nullptr;
  cardano_anchor_t*               anchor         = nullptr;
  cardano_constitution_t*         constitution   = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(reader, &action_id), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  reader = cardano_cbor_reader_from_hex(CONSTITUTION_CBOR, strlen(CONSTITUTION_CBOR));
  EXPECT_EQ(cardano_constitution_from_cbor(reader, &constitution), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  for (int i = 0; i < 38; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_propose_new_constitution_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), CIP129_BECH32_1, strlen(CIP129_BECH32_1), constitution);

    // Assert
    EXPECT_NE(tx_builder->last_error, CARDANO_SUCCESS);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_governance_action_id_unref(&action_id);
  cardano_anchor_unref(&anchor);
  cardano_constitution_unref(&constitution);
}

TEST(cardano_tx_builder_propose_info, doesntCrashIfGivenNull)
{
  cardano_tx_builder_propose_info(nullptr, nullptr, nullptr);
}

TEST(cardano_tx_builder_propose_info, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = nullptr;
  cardano_reward_address_t*      reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_info(tx_builder, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_info(tx_builder, reward_address, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_tx_builder_propose_info, canProposeInfo)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = nullptr;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_anchor_t*              anchor         = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_propose_info(tx_builder, reward_address, anchor);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* actions = cardano_transaction_body_get_proposal_procedures(body);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(actions), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
  cardano_proposal_procedure_set_unref(&actions);
}

TEST(cardano_tx_builder_propose_info, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = nullptr;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_anchor_t*              anchor         = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  for (int i = 0; i < 5; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_propose_info(tx_builder, reward_address, anchor);

    // Assert
    EXPECT_NE(tx_builder->last_error, CARDANO_SUCCESS);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_reward_address_unref(&reward_address);
  cardano_anchor_unref(&anchor);
}

TEST(cardano_tx_builder_propose_info_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;
  cardano_anchor_t*              anchor   = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  // clang-format off
  cardano_tx_builder_propose_info_ex(
    nullptr,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_info_ex(
    tx_builder,
    nullptr, 0,
    nullptr, 0,
    nullptr, 0);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_info_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    nullptr, 0,
    nullptr, 0);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);
  cardano_tx_builder_propose_info_ex(
    tx_builder,
    REWARD_ADDRESS, strlen(REWARD_ADDRESS),
    ANCHOR_URL, strlen(ANCHOR_URL),
    nullptr, 0);
  cardano_tx_builder_unref(&tx_builder);

  // clang-format on

  cardano_protocol_parameters_unref(&params);

  cardano_anchor_unref(&anchor);
}

TEST(cardano_tx_builder_propose_info_ex, canProposeInfo)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;
  cardano_anchor_t*              anchor   = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_propose_info_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH));

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_proposal_procedure_set_t* actions = cardano_transaction_body_get_proposal_procedures(body);

  // Assert
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(actions), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);

  cardano_anchor_unref(&anchor);
  cardano_proposal_procedure_set_unref(&actions);
}

TEST(cardano_tx_builder_propose_info_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;
  cardano_anchor_t*              anchor   = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));
  EXPECT_EQ(cardano_anchor_from_cbor(reader, &anchor), CARDANO_SUCCESS);
  cardano_cbor_reader_unref(&reader);

  for (int i = 0; i < 10; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_propose_info_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH));

    // Assert
    EXPECT_NE(tx_builder->last_error, CARDANO_SUCCESS);

    cardano_tx_builder_unref(&tx_builder);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }

  // Cleanup
  cardano_protocol_parameters_unref(&params);

  cardano_anchor_unref(&anchor);
}

/**
 * Deferred redeemer callback used by the tests: returns Constr 0 [own_input_index] derived from
 * the balanced draft via the canonical index lookup helpers.
 */
static cardano_error_t
build_self_index_redeemer(void* user_context, cardano_transaction_t* draft_tx, cardano_utxo_list_t* resolved_inputs, cardano_plutus_data_t** redeemer)
{
  CARDANO_UNUSED(resolved_inputs);

  cardano_utxo_t* own_utxo = (cardano_utxo_t*)user_context;

  cardano_transaction_input_t* input = cardano_utxo_get_input(own_utxo);
  cardano_transaction_input_unref(&input);

  cardano_blake2b_hash_t* id = cardano_transaction_input_get_id(input);
  cardano_blake2b_hash_unref(&id);

  uint64_t own_index = 0U;

  cardano_error_t result = cardano_transaction_find_input_index(draft_tx, id, cardano_transaction_input_get_index(input), &own_index);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_plutus_data_t* index_data = NULL;

  result = cardano_plutus_data_new_integer_from_int((int64_t)own_index, &index_data);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_plutus_list_t* fields = NULL;
  result                        = cardano_plutus_list_new(&fields);

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_list_add(fields, index_data);
  }

  cardano_constr_plutus_data_t* constr = NULL;

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_constr_plutus_data_new(0U, fields, &constr);
  }

  if (result == CARDANO_SUCCESS)
  {
    result = cardano_plutus_data_new_constr(constr, redeemer);
  }

  cardano_constr_plutus_data_unref(&constr);
  cardano_plutus_list_unref(&fields);
  cardano_plutus_data_unref(&index_data);

  return result;
}

TEST(cardano_tx_builder_add_input_with_deferred_redeemer, resolvesTheRedeemerFromTheFinalTransaction)
{
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_tx_evaluator_t*        tx_evaluator   = NULL;
  cardano_utxo_t*                utxo           = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_plutus_data_t*         datum          = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_from_provider(provider, &tx_evaluator), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_tx_evaluator(tx_builder, tx_evaluator);
  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);
  cardano_tx_builder_set_collateral_change_address(tx_builder, change_address);
  cardano_tx_builder_set_collateral_utxos(tx_builder, utxos);

  cardano_transaction_t* tx = nullptr;

  cardano_tx_builder_add_input_with_deferred_redeemer(tx_builder, utxo, build_self_index_redeemer, utxo, datum);

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);
  EXPECT_THAT(result, CARDANO_SUCCESS);

  // The resolved redeemer must be Constr 0 [own_input_index] with the index matching the
  // canonical position of the script input in the final transaction.
  cardano_transaction_input_t* input = cardano_utxo_get_input(utxo);
  cardano_transaction_input_unref(&input);

  cardano_blake2b_hash_t* id = cardano_transaction_input_get_id(input);
  cardano_blake2b_hash_unref(&id);

  uint64_t expected_index = 0U;
  EXPECT_EQ(cardano_transaction_find_input_index(tx, id, cardano_transaction_input_get_index(input), &expected_index), CARDANO_SUCCESS);

  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(tx);
  cardano_witness_set_unref(&witness_set);

  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness_set);
  cardano_redeemer_list_unref(&redeemers);

  ASSERT_EQ(cardano_redeemer_list_get_length(redeemers), 1U);

  cardano_redeemer_t* redeemer = NULL;
  ASSERT_EQ(cardano_redeemer_list_get(redeemers, 0U, &redeemer), CARDANO_SUCCESS);
  cardano_redeemer_unref(&redeemer);

  EXPECT_EQ(cardano_redeemer_get_index(redeemer), expected_index);

  cardano_plutus_data_t* payload = cardano_redeemer_get_data(redeemer);
  cardano_plutus_data_unref(&payload);

  cardano_constr_plutus_data_t* constr = NULL;
  ASSERT_EQ(cardano_plutus_data_to_constr(payload, &constr), CARDANO_SUCCESS);
  cardano_constr_plutus_data_unref(&constr);

  cardano_plutus_list_t* fields = NULL;
  ASSERT_EQ(cardano_constr_plutus_data_get_data(constr, &fields), CARDANO_SUCCESS);
  cardano_plutus_list_unref(&fields);

  ASSERT_EQ(cardano_plutus_list_get_length(fields), 1U);

  cardano_plutus_data_t* field = NULL;
  ASSERT_EQ(cardano_plutus_list_get(fields, 0U, &field), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&field);

  cardano_bigint_t* value = NULL;
  ASSERT_EQ(cardano_plutus_data_to_integer(field, &value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_bigint_to_int(value), (int64_t)expected_index);
  cardano_bigint_unref(&value);

  // The redeemer index lookup helper agrees with the witness set ordering.
  uint64_t redeemer_rank = 0U;
  EXPECT_EQ(cardano_transaction_find_redeemer_index(tx, CARDANO_REDEEMER_TAG_SPEND, expected_index, &redeemer_rank), CARDANO_SUCCESS);
  EXPECT_EQ(redeemer_rank, 0U);

  // The output lookup helper can find the change output.
  uint64_t change_index = 0U;
  EXPECT_EQ(cardano_transaction_find_output_index(tx, change_address, 1U, &change_index), CARDANO_SUCCESS);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_transaction_unref(&tx);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&change_address);
  cardano_plutus_data_unref(&datum);
  cardano_utxo_list_unref(&utxos);
  cardano_tx_evaluator_unref(&tx_evaluator);
}

TEST(cardano_tx_builder_add_input_with_deferred_redeemer, latchesErrorOnNullArguments)
{
  cardano_protocol_parameters_t* params     = init_protocol_parameters();
  cardano_tx_builder_t*          tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_add_input_with_deferred_redeemer(nullptr, nullptr, nullptr, nullptr, nullptr);
  cardano_tx_builder_add_input_with_deferred_redeemer(tx_builder, nullptr, nullptr, nullptr, nullptr);

  EXPECT_EQ(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
}

/**
 * Deferred redeemer callback that always fails.
 */
static cardano_error_t
failing_deferred_callback(void* user_context, cardano_transaction_t* draft_tx, cardano_utxo_list_t* resolved_inputs, cardano_plutus_data_t** redeemer)
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
null_payload_deferred_callback(void* user_context, cardano_transaction_t* draft_tx, cardano_utxo_list_t* resolved_inputs, cardano_plutus_data_t** redeemer)
{
  CARDANO_UNUSED(user_context);
  CARDANO_UNUSED(draft_tx);
  CARDANO_UNUSED(resolved_inputs);
  CARDANO_UNUSED(redeemer);

  return CARDANO_SUCCESS;
}

/**
 * Deferred redeemer callback that produces the integer 7.
 */
static cardano_error_t
fixed_payload_deferred_callback(void* user_context, cardano_transaction_t* draft_tx, cardano_utxo_list_t* resolved_inputs, cardano_plutus_data_t** redeemer)
{
  CARDANO_UNUSED(user_context);
  CARDANO_UNUSED(draft_tx);
  CARDANO_UNUSED(resolved_inputs);

  return cardano_plutus_data_new_integer_from_int(7, redeemer);
}

/**
 * Builds the placeholder payload used by deferred redeemers before resolution (constr 0 []).
 * @return The placeholder plutus data.
 */
static cardano_plutus_data_t*
new_placeholder_plutus_data()
{
  cardano_plutus_list_t* fields = NULL;
  EXPECT_EQ(cardano_plutus_list_new(&fields), CARDANO_SUCCESS);

  cardano_constr_plutus_data_t* constr = NULL;
  EXPECT_EQ(cardano_constr_plutus_data_new(0U, fields, &constr), CARDANO_SUCCESS);

  cardano_plutus_data_t* data = NULL;
  EXPECT_EQ(cardano_plutus_data_new_constr(constr, &data), CARDANO_SUCCESS);

  cardano_constr_plutus_data_unref(&constr);
  cardano_plutus_list_unref(&fields);

  return data;
}

/**
 * Reads the integer payload of a `Constr 0 [index]` redeemer produced by the self-index callback.
 * @return The index carried by the payload.
 */
static int64_t
read_self_index_payload(cardano_plutus_data_t* payload)
{
  cardano_constr_plutus_data_t* constr = NULL;
  EXPECT_EQ(cardano_plutus_data_to_constr(payload, &constr), CARDANO_SUCCESS);
  cardano_constr_plutus_data_unref(&constr);

  cardano_plutus_list_t* fields = NULL;
  EXPECT_EQ(cardano_constr_plutus_data_get_data(constr, &fields), CARDANO_SUCCESS);
  cardano_plutus_list_unref(&fields);

  EXPECT_EQ(cardano_plutus_list_get_length(fields), 1U);

  cardano_plutus_data_t* field = NULL;
  EXPECT_EQ(cardano_plutus_list_get(fields, 0U, &field), CARDANO_SUCCESS);
  cardano_plutus_data_unref(&field);

  cardano_bigint_t* value = NULL;
  EXPECT_EQ(cardano_plutus_data_to_integer(field, &value), CARDANO_SUCCESS);

  const int64_t index = cardano_bigint_to_int(value);
  cardano_bigint_unref(&value);

  return index;
}

TEST(cardano_tx_builder_add_input_with_deferred_redeemer, propagatesCallbackErrorFromBuild)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_tx_evaluator_t*        tx_evaluator   = NULL;
  cardano_utxo_t*                utxo           = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_plutus_data_t*         datum          = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_from_provider(provider, &tx_evaluator), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_tx_evaluator(tx_builder, tx_evaluator);
  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);
  cardano_tx_builder_set_collateral_change_address(tx_builder, change_address);
  cardano_tx_builder_set_collateral_utxos(tx_builder, utxos);

  cardano_tx_builder_add_input_with_deferred_redeemer(tx_builder, utxo, failing_deferred_callback, NULL, datum);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(tx, nullptr);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&change_address);
  cardano_plutus_data_unref(&datum);
  cardano_utxo_list_unref(&utxos);
  cardano_tx_evaluator_unref(&tx_evaluator);
}

TEST(cardano_tx_builder_add_input_with_deferred_redeemer, failsBuildIfCallbackYieldsNoPayload)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_tx_evaluator_t*        tx_evaluator   = NULL;
  cardano_utxo_t*                utxo           = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_plutus_data_t*         datum          = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_from_provider(provider, &tx_evaluator), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_tx_evaluator(tx_builder, tx_evaluator);
  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);
  cardano_tx_builder_set_collateral_change_address(tx_builder, change_address);
  cardano_tx_builder_set_collateral_utxos(tx_builder, utxos);

  cardano_tx_builder_add_input_with_deferred_redeemer(tx_builder, utxo, null_payload_deferred_callback, NULL, datum);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(tx, nullptr);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_unref(&utxo);
  cardano_address_unref(&change_address);
  cardano_plutus_data_unref(&datum);
  cardano_utxo_list_unref(&utxos);
  cardano_tx_evaluator_unref(&tx_evaluator);
}

TEST(cardano_tx_builder_add_input_with_deferred_redeemer, resolvesEachInputToItsOwnCanonicalIndex)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_tx_evaluator_t*        tx_evaluator   = NULL;
  cardano_utxo_t*                utxo_a         = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_utxo_t*                utxo_b         = create_utxo(UTXO_WITH_REF_SCRIPT_PV2);
  cardano_plutus_data_t*         datum          = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_from_provider(provider, &tx_evaluator), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_set_tx_evaluator(tx_builder, tx_evaluator);
  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);
  cardano_tx_builder_set_collateral_change_address(tx_builder, change_address);
  cardano_tx_builder_set_collateral_utxos(tx_builder, utxos);

  // Both inputs register the same self-index callback; each one must resolve to its own
  // canonical index in the final transaction.
  cardano_tx_builder_add_input_with_deferred_redeemer(tx_builder, utxo_a, build_self_index_redeemer, utxo_a, datum);
  cardano_tx_builder_add_input_with_deferred_redeemer(tx_builder, utxo_b, build_self_index_redeemer, utxo_b, datum);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);
  EXPECT_THAT(result, CARDANO_SUCCESS);

  // Assert
  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(tx);
  cardano_witness_set_unref(&witness_set);

  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness_set);
  cardano_redeemer_list_unref(&redeemers);

  ASSERT_EQ(cardano_redeemer_list_get_length(redeemers), 2U);

  cardano_utxo_t* utxos_under_test[] = { utxo_a, utxo_b };
  uint64_t        canonical_index[]  = { 0U, 0U };

  for (size_t i = 0U; i < 2U; ++i)
  {
    cardano_transaction_input_t* input = cardano_utxo_get_input(utxos_under_test[i]);
    cardano_transaction_input_unref(&input);

    cardano_blake2b_hash_t* id = cardano_transaction_input_get_id(input);
    cardano_blake2b_hash_unref(&id);

    EXPECT_EQ(cardano_transaction_find_input_index(tx, id, cardano_transaction_input_get_index(input), &canonical_index[i]), CARDANO_SUCCESS);

    // The redeemer for this input sits at the rank given by the redeemer index lookup helper.
    uint64_t rank = 0U;
    EXPECT_EQ(cardano_transaction_find_redeemer_index(tx, CARDANO_REDEEMER_TAG_SPEND, canonical_index[i], &rank), CARDANO_SUCCESS);

    cardano_redeemer_t* redeemer = NULL;
    ASSERT_EQ(cardano_redeemer_list_get(redeemers, rank, &redeemer), CARDANO_SUCCESS);
    cardano_redeemer_unref(&redeemer);

    EXPECT_EQ(cardano_redeemer_get_tag(redeemer), CARDANO_REDEEMER_TAG_SPEND);
    EXPECT_EQ(cardano_redeemer_get_index(redeemer), canonical_index[i]);

    cardano_plutus_data_t* payload = cardano_redeemer_get_data(redeemer);
    cardano_plutus_data_unref(&payload);

    EXPECT_EQ(read_self_index_payload(payload), (int64_t)canonical_index[i]);
  }

  EXPECT_NE(canonical_index[0], canonical_index[1]);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_transaction_unref(&tx);
  cardano_utxo_unref(&utxo_a);
  cardano_utxo_unref(&utxo_b);
  cardano_address_unref(&change_address);
  cardano_plutus_data_unref(&datum);
  cardano_utxo_list_unref(&utxos);
  cardano_tx_evaluator_unref(&tx_evaluator);
}

TEST(cardano_tx_builder_add_input_with_deferred_redeemer, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params = init_protocol_parameters();
  cardano_utxo_t*                utxo   = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_plutus_data_t*         datum  = create_plutus_data(PLUTUS_DATA_CBOR);

  // Sweep the allocation failure point across the whole call until it succeeds, so that every
  // intermediate allocation (placeholder, declaration and deferred registration) fails at
  // least once.
  for (int i = 0; i < 512; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_add_input_with_deferred_redeemer(tx_builder, utxo, fixed_payload_deferred_callback, NULL, datum);

    const cardano_error_t error = tx_builder->last_error;

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    cardano_tx_builder_unref(&tx_builder);

    if (error == CARDANO_SUCCESS)
    {
      break;
    }

    // Assert
    EXPECT_THAT(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);
  cardano_utxo_unref(&utxo);
  cardano_plutus_data_unref(&datum);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_mint_token_with_deferred_redeemer, registersDeferredRedeemerForThePolicy)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_protocol_parameters();
  cardano_asset_name_t*          asset_name = NULL;
  cardano_blake2b_hash_t*        policy_id  = NULL;

  EXPECT_EQ(cardano_asset_name_from_string("TEXT", 4, &asset_name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_id), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_mint_token_with_deferred_redeemer(tx_builder, policy_id, asset_name, 4, fixed_payload_deferred_callback, NULL);

  // Assert
  EXPECT_EQ(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(tx_builder->state.deferred_redeemers), 1U);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_multi_asset_t* mint = cardano_transaction_body_get_mint(body);
  cardano_multi_asset_unref(&mint);

  int64_t quantity = 0;
  EXPECT_EQ(cardano_multi_asset_get(mint, policy_id, asset_name, &quantity), CARDANO_SUCCESS);
  EXPECT_EQ(quantity, 4);

  // The mint redeemer starts out as the placeholder `constr 0 []`...
  cardano_redeemer_t* redeemer = NULL;
  ASSERT_EQ(cardano_blake2b_hash_to_redeemer_map_get(tx_builder->state.mints_to_redeemer_map, policy_id, &redeemer), CARDANO_SUCCESS);
  cardano_redeemer_unref(&redeemer);

  cardano_plutus_data_t* placeholder = new_placeholder_plutus_data();
  cardano_plutus_data_t* payload     = cardano_redeemer_get_data(redeemer);

  EXPECT_TRUE(cardano_plutus_data_equals(payload, placeholder));
  cardano_plutus_data_unref(&payload);

  // ...and a resolution pass (one balancing iteration) replaces it with the callback payload.
  cardano_utxo_list_t* resolved_inputs = NULL;
  EXPECT_EQ(cardano_utxo_list_new(&resolved_inputs), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_deferred_redeemer_list_resolve(tx_builder->state.deferred_redeemers, tx_builder->state.transaction, resolved_inputs), CARDANO_SUCCESS);

  cardano_plutus_data_t* expected = NULL;
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(7, &expected), CARDANO_SUCCESS);

  payload = cardano_redeemer_get_data(redeemer);
  EXPECT_TRUE(cardano_plutus_data_equals(payload, expected));

  // Cleanup
  cardano_plutus_data_unref(&payload);
  cardano_plutus_data_unref(&expected);
  cardano_plutus_data_unref(&placeholder);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_tx_builder_mint_token_with_deferred_redeemer, latchesErrorOnNullArguments)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_protocol_parameters();
  cardano_asset_name_t*          asset_name = NULL;
  cardano_blake2b_hash_t*        policy_id  = NULL;

  EXPECT_EQ(cardano_asset_name_from_string("TEXT", 4, &asset_name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_id), CARDANO_SUCCESS);

  // Act & Assert
  cardano_tx_builder_mint_token_with_deferred_redeemer(nullptr, policy_id, asset_name, 4, fixed_payload_deferred_callback, NULL);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_mint_token_with_deferred_redeemer(tx_builder, nullptr, asset_name, 4, fixed_payload_deferred_callback, NULL);
  EXPECT_EQ(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_tx_builder_unref(&tx_builder);
  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_mint_token_with_deferred_redeemer(tx_builder, policy_id, asset_name, 4, nullptr, NULL);
  EXPECT_EQ(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_tx_builder_unref(&tx_builder);
  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // A builder already in an error state must not register anything.
  tx_builder->last_error = CARDANO_ERROR_GENERIC;
  cardano_tx_builder_mint_token_with_deferred_redeemer(tx_builder, policy_id, asset_name, 4, fixed_payload_deferred_callback, NULL);
  EXPECT_EQ(tx_builder->last_error, CARDANO_ERROR_GENERIC);
  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(tx_builder->state.deferred_redeemers), 0U);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
}

TEST(cardano_tx_builder_mint_token_with_deferred_redeemer, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_protocol_parameters();
  cardano_asset_name_t*          asset_name = NULL;
  cardano_blake2b_hash_t*        policy_id  = NULL;

  EXPECT_EQ(cardano_asset_name_from_string("TEXT", 4, &asset_name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_id), CARDANO_SUCCESS);

  // Sweep the allocation failure point across the whole call until it succeeds, so that every
  // intermediate allocation (placeholder, declaration and deferred registration) fails at
  // least once.
  for (int i = 0; i < 512; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_mint_token_with_deferred_redeemer(tx_builder, policy_id, asset_name, 4, fixed_payload_deferred_callback, NULL);

    const cardano_error_t error = tx_builder->last_error;

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    cardano_tx_builder_unref(&tx_builder);

    if (error == CARDANO_SUCCESS)
    {
      break;
    }

    // Assert
    EXPECT_THAT(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_withdraw_rewards_with_deferred_redeemer, registersDeferredRedeemerForTheWithdrawal)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_reward_address_t*      reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(SCRIPT_REWARD_ADDRESS, strlen(SCRIPT_REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_withdraw_rewards_with_deferred_redeemer(tx_builder, reward_address, 0, fixed_payload_deferred_callback, NULL);

  // Assert
  EXPECT_EQ(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(tx_builder->state.deferred_redeemers), 1U);
  EXPECT_EQ(cardano_blake2b_hash_to_redeemer_map_get_length(tx_builder->state.withdrawals_to_redeemer_map), 1U);

  // The withdrawal redeemer starts out as the placeholder `constr 0 []`...
  cardano_redeemer_t* redeemer = NULL;
  ASSERT_EQ(cardano_blake2b_hash_to_redeemer_map_get_value_at(tx_builder->state.withdrawals_to_redeemer_map, 0U, &redeemer), CARDANO_SUCCESS);
  cardano_redeemer_unref(&redeemer);

  cardano_plutus_data_t* placeholder = new_placeholder_plutus_data();
  cardano_plutus_data_t* payload     = cardano_redeemer_get_data(redeemer);

  EXPECT_TRUE(cardano_plutus_data_equals(payload, placeholder));
  cardano_plutus_data_unref(&payload);

  // ...and a resolution pass (one balancing iteration) replaces it with the callback payload.
  cardano_utxo_list_t* resolved_inputs = NULL;
  EXPECT_EQ(cardano_utxo_list_new(&resolved_inputs), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_deferred_redeemer_list_resolve(tx_builder->state.deferred_redeemers, tx_builder->state.transaction, resolved_inputs), CARDANO_SUCCESS);

  cardano_plutus_data_t* expected = NULL;
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(7, &expected), CARDANO_SUCCESS);

  payload = cardano_redeemer_get_data(redeemer);
  EXPECT_TRUE(cardano_plutus_data_equals(payload, expected));

  // Cleanup
  cardano_plutus_data_unref(&payload);
  cardano_plutus_data_unref(&expected);
  cardano_plutus_data_unref(&placeholder);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_tx_builder_withdraw_rewards_with_deferred_redeemer, latchesErrorOnNullArguments)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_reward_address_t*      reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(SCRIPT_REWARD_ADDRESS, strlen(SCRIPT_REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  // Act & Assert
  cardano_tx_builder_withdraw_rewards_with_deferred_redeemer(nullptr, reward_address, 0, fixed_payload_deferred_callback, NULL);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_withdraw_rewards_with_deferred_redeemer(tx_builder, nullptr, 0, fixed_payload_deferred_callback, NULL);
  EXPECT_EQ(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_tx_builder_unref(&tx_builder);
  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_withdraw_rewards_with_deferred_redeemer(tx_builder, reward_address, 0, nullptr, NULL);
  EXPECT_EQ(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_tx_builder_unref(&tx_builder);
  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // A failure in the underlying withdrawal declaration must abort the registration.
  cardano_tx_builder_withdraw_rewards_with_deferred_redeemer(tx_builder, reward_address, -1, fixed_payload_deferred_callback, NULL);
  EXPECT_EQ(tx_builder->last_error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(tx_builder->state.deferred_redeemers), 0U);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_tx_builder_withdraw_rewards_with_deferred_redeemer, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_reward_address_t*      reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(SCRIPT_REWARD_ADDRESS, strlen(SCRIPT_REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);

  // Sweep the allocation failure point across the whole call until it succeeds, so that every
  // intermediate allocation (placeholder, declaration and deferred registration) fails at
  // least once.
  for (int i = 0; i < 512; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_withdraw_rewards_with_deferred_redeemer(tx_builder, reward_address, 0, fixed_payload_deferred_callback, NULL);

    const cardano_error_t error = tx_builder->last_error;

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    cardano_tx_builder_unref(&tx_builder);

    if (error == CARDANO_SUCCESS)
    {
      break;
    }

    // Assert
    EXPECT_THAT(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);
  cardano_reward_address_unref(&reward_address);
  cardano_set_allocators(malloc, realloc, free);
}

/**
 * Creates a stake registration certificate from a fixed CBOR fixture.
 * \return A new certificate instance.
 */
static cardano_certificate_t*
new_default_certificate()
{
  static const char* CERT_CBOR = "82008200581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f";

  cardano_certificate_t* certificate = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(CERT_CBOR, strlen(CERT_CBOR));

  EXPECT_EQ(cardano_certificate_from_cbor(reader, &certificate), CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return certificate;
}

TEST(cardano_tx_builder_add_certificate_with_deferred_redeemer, registersDeferredRedeemerForTheCertificate)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_certificate_t*         certificate = new_default_certificate();

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_add_certificate_with_deferred_redeemer(tx_builder, certificate, fixed_payload_deferred_callback, NULL);

  // Assert
  EXPECT_EQ(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(tx_builder->state.deferred_redeemers), 1U);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->state.transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1U);

  // The certificate redeemer starts out as the placeholder `constr 0 []`...
  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(tx_builder->state.transaction);
  cardano_witness_set_unref(&witnesses);

  cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witnesses);
  cardano_redeemer_list_unref(&redeemers);

  ASSERT_EQ(cardano_redeemer_list_get_length(redeemers), 1U);

  cardano_redeemer_t* redeemer = NULL;
  ASSERT_EQ(cardano_redeemer_list_get(redeemers, 0U, &redeemer), CARDANO_SUCCESS);
  cardano_redeemer_unref(&redeemer);

  EXPECT_EQ(cardano_redeemer_get_tag(redeemer), CARDANO_REDEEMER_TAG_CERTIFYING);
  EXPECT_EQ(cardano_redeemer_get_index(redeemer), 0U);

  cardano_plutus_data_t* placeholder = new_placeholder_plutus_data();
  cardano_plutus_data_t* payload     = cardano_redeemer_get_data(redeemer);

  EXPECT_TRUE(cardano_plutus_data_equals(payload, placeholder));
  cardano_plutus_data_unref(&payload);

  // ...and a resolution pass (one balancing iteration) replaces it with the callback payload.
  cardano_utxo_list_t* resolved_inputs = NULL;
  EXPECT_EQ(cardano_utxo_list_new(&resolved_inputs), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_deferred_redeemer_list_resolve(tx_builder->state.deferred_redeemers, tx_builder->state.transaction, resolved_inputs), CARDANO_SUCCESS);

  cardano_plutus_data_t* expected = NULL;
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(7, &expected), CARDANO_SUCCESS);

  payload = cardano_redeemer_get_data(redeemer);
  EXPECT_TRUE(cardano_plutus_data_equals(payload, expected));

  // Cleanup
  cardano_plutus_data_unref(&payload);
  cardano_plutus_data_unref(&expected);
  cardano_plutus_data_unref(&placeholder);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_certificate_unref(&certificate);
}

TEST(cardano_tx_builder_add_certificate_with_deferred_redeemer, latchesErrorOnNullArguments)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_certificate_t*         certificate = new_default_certificate();

  // Act & Assert
  cardano_tx_builder_add_certificate_with_deferred_redeemer(nullptr, certificate, fixed_payload_deferred_callback, NULL);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_add_certificate_with_deferred_redeemer(tx_builder, nullptr, fixed_payload_deferred_callback, NULL);
  EXPECT_EQ(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_tx_builder_unref(&tx_builder);
  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_add_certificate_with_deferred_redeemer(tx_builder, certificate, nullptr, NULL);
  EXPECT_EQ(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_tx_builder_unref(&tx_builder);
  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // A builder already in an error state must not register anything.
  tx_builder->last_error = CARDANO_ERROR_GENERIC;
  cardano_tx_builder_add_certificate_with_deferred_redeemer(tx_builder, certificate, fixed_payload_deferred_callback, NULL);
  EXPECT_EQ(tx_builder->last_error, CARDANO_ERROR_GENERIC);
  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(tx_builder->state.deferred_redeemers), 0U);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_certificate_unref(&certificate);
}

TEST(cardano_tx_builder_add_certificate_with_deferred_redeemer, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_certificate_t*         certificate = new_default_certificate();

  // Sweep the allocation failure point across the whole call until it succeeds, so that every
  // intermediate allocation (placeholder, declaration and deferred registration) fails at
  // least once.
  for (int i = 0; i < 512; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_add_certificate_with_deferred_redeemer(tx_builder, certificate, fixed_payload_deferred_callback, NULL);

    const cardano_error_t error = tx_builder->last_error;

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    cardano_tx_builder_unref(&tx_builder);

    if (error == CARDANO_SUCCESS)
    {
      break;
    }

    // Assert
    EXPECT_THAT(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);
  cardano_certificate_unref(&certificate);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_vote_with_deferred_redeemer, registersDeferredRedeemerForTheVote)
{
  // Arrange
  cardano_protocol_parameters_t*  params    = init_protocol_parameters();
  cardano_voter_t*                voter     = new_default_voter();
  cardano_voting_procedure_t*     procedure = new_default_voting_procedure();
  cardano_governance_action_id_t* action_id = nullptr;

  cardano_cbor_reader_t* gov_action_reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(gov_action_reader, &action_id), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // Act
  cardano_tx_builder_vote_with_deferred_redeemer(tx_builder, voter, action_id, procedure, fixed_payload_deferred_callback, NULL);

  // Assert
  EXPECT_EQ(tx_builder->last_error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(tx_builder->state.deferred_redeemers), 1U);
  EXPECT_EQ(cardano_blake2b_hash_to_redeemer_map_get_length(tx_builder->state.votes_to_redeemer_map), 1U);

  // The vote redeemer starts out as the placeholder `constr 0 []`...
  cardano_redeemer_t* redeemer = NULL;
  ASSERT_EQ(cardano_blake2b_hash_to_redeemer_map_get_value_at(tx_builder->state.votes_to_redeemer_map, 0U, &redeemer), CARDANO_SUCCESS);
  cardano_redeemer_unref(&redeemer);

  EXPECT_EQ(cardano_redeemer_get_tag(redeemer), CARDANO_REDEEMER_TAG_VOTING);

  cardano_plutus_data_t* placeholder = new_placeholder_plutus_data();
  cardano_plutus_data_t* payload     = cardano_redeemer_get_data(redeemer);

  EXPECT_TRUE(cardano_plutus_data_equals(payload, placeholder));
  cardano_plutus_data_unref(&payload);

  // ...and a resolution pass (one balancing iteration) replaces it with the callback payload.
  cardano_utxo_list_t* resolved_inputs = NULL;
  EXPECT_EQ(cardano_utxo_list_new(&resolved_inputs), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_deferred_redeemer_list_resolve(tx_builder->state.deferred_redeemers, tx_builder->state.transaction, resolved_inputs), CARDANO_SUCCESS);

  cardano_plutus_data_t* expected = NULL;
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(7, &expected), CARDANO_SUCCESS);

  payload = cardano_redeemer_get_data(redeemer);
  EXPECT_TRUE(cardano_plutus_data_equals(payload, expected));

  // Cleanup
  cardano_plutus_data_unref(&payload);
  cardano_plutus_data_unref(&expected);
  cardano_plutus_data_unref(&placeholder);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_voter_unref(&voter);
  cardano_voting_procedure_unref(&procedure);
  cardano_governance_action_id_unref(&action_id);
  cardano_cbor_reader_unref(&gov_action_reader);
}

TEST(cardano_tx_builder_vote_with_deferred_redeemer, latchesErrorOnNullArguments)
{
  // Arrange
  cardano_protocol_parameters_t*  params    = init_protocol_parameters();
  cardano_voter_t*                voter     = new_default_voter();
  cardano_voting_procedure_t*     procedure = new_default_voting_procedure();
  cardano_governance_action_id_t* action_id = nullptr;

  cardano_cbor_reader_t* gov_action_reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(gov_action_reader, &action_id), CARDANO_SUCCESS);

  // Act & Assert
  cardano_tx_builder_vote_with_deferred_redeemer(nullptr, voter, action_id, procedure, fixed_payload_deferred_callback, NULL);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_vote_with_deferred_redeemer(tx_builder, nullptr, action_id, procedure, fixed_payload_deferred_callback, NULL);
  EXPECT_EQ(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_tx_builder_unref(&tx_builder);
  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_vote_with_deferred_redeemer(tx_builder, voter, nullptr, procedure, fixed_payload_deferred_callback, NULL);
  EXPECT_EQ(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_tx_builder_unref(&tx_builder);
  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_vote_with_deferred_redeemer(tx_builder, voter, action_id, nullptr, fixed_payload_deferred_callback, NULL);
  EXPECT_EQ(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_tx_builder_unref(&tx_builder);
  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  cardano_tx_builder_vote_with_deferred_redeemer(tx_builder, voter, action_id, procedure, nullptr, NULL);
  EXPECT_EQ(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_tx_builder_unref(&tx_builder);
  tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

  // A builder already in an error state must not register anything.
  tx_builder->last_error = CARDANO_ERROR_GENERIC;
  cardano_tx_builder_vote_with_deferred_redeemer(tx_builder, voter, action_id, procedure, fixed_payload_deferred_callback, NULL);
  EXPECT_EQ(tx_builder->last_error, CARDANO_ERROR_GENERIC);
  EXPECT_EQ(cardano_deferred_redeemer_list_get_length(tx_builder->state.deferred_redeemers), 0U);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_voter_unref(&voter);
  cardano_voting_procedure_unref(&procedure);
  cardano_governance_action_id_unref(&action_id);
  cardano_cbor_reader_unref(&gov_action_reader);
}

TEST(cardano_tx_builder_vote_with_deferred_redeemer, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t*  params    = init_protocol_parameters();
  cardano_voter_t*                voter     = new_default_voter();
  cardano_voting_procedure_t*     procedure = new_default_voting_procedure();
  cardano_governance_action_id_t* action_id = nullptr;

  cardano_cbor_reader_t* gov_action_reader = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  EXPECT_EQ(cardano_governance_action_id_from_cbor(gov_action_reader, &action_id), CARDANO_SUCCESS);

  // Sweep the allocation failure point across the whole call until it succeeds, so that every
  // intermediate allocation (placeholder, declaration and deferred registration) fails at
  // least once.
  for (int i = 0; i < 512; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, &CARDANO_MAINNET_SLOT_CONFIG);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_vote_with_deferred_redeemer(tx_builder, voter, action_id, procedure, fixed_payload_deferred_callback, NULL);

    const cardano_error_t error = tx_builder->last_error;

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    cardano_tx_builder_unref(&tx_builder);

    if (error == CARDANO_SUCCESS)
    {
      break;
    }

    // Assert
    EXPECT_THAT(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  }

  reset_allocators_run_count();
  reset_limited_malloc();

  cardano_protocol_parameters_unref(&params);
  cardano_voter_unref(&voter);
  cardano_voting_procedure_unref(&procedure);
  cardano_governance_action_id_unref(&action_id);
  cardano_cbor_reader_unref(&gov_action_reader);
  cardano_set_allocators(malloc, realloc, free);
}
