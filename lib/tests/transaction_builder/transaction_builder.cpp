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

#include <cardano/transaction_builder/transaction_builder.h>

#include <allocators.h>
#include <cardano/transaction_body/transaction_output.h>
#include <gmock/gmock.h>
#include <tests/allocators_helpers.h>

/* TX BUILDER INTERNALS ******************************************************/

typedef struct cardano_tx_builder_t
{
    cardano_object_t               base;
    cardano_error_t                last_error;
    cardano_transaction_t*         transaction;
    cardano_protocol_parameters_t* params;
    cardano_provider_t*            provider;
    cardano_coin_selector_t*       coin_selector;
    cardano_tx_evaluator_t*        tx_evaluator;
    cardano_address_t*             change_address;
    cardano_address_t*             collateral_address;
    cardano_utxo_list_t*           available_utxos;
    cardano_utxo_list_t*           collateral_utxos;
    cardano_utxo_list_t*           pre_selected_inputs;
    cardano_utxo_list_t*           reference_inputs;
    bool                           has_plutus_v1;
    bool                           has_plutus_v2;
    bool                           has_plutus_v3;
    size_t                         additional_signature_count;
} cardano_tx_builder_t;

/* CONSTANTS *****************************************************************/

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
static const char* ASSET_ID_HEX                = "0000000000000000000000000000000000000000000000000000000054455854";
static const char* PLUTUS_V1_CBOR              = "82014e4d01000033222220051200120011";
static const char* PLUTUS_V2_CBOR              = "82025908955908920100003233223232323232332232323232323232323232332232323232322223232533532323232325335001101d13357389211e77726f6e67207573616765206f66207265666572656e636520696e7075740001c3232533500221533500221333573466e1c00800408007c407854cd4004840784078d40900114cd4c8d400488888888888802d40044c08526221533500115333533550222350012222002350022200115024213355023320015021001232153353235001222222222222300e00250052133550253200150233355025200100115026320013550272253350011502722135002225335333573466e3c00801c0940904d40b00044c01800c884c09526135001220023333573466e1cd55cea80224000466442466002006004646464646464646464646464646666ae68cdc39aab9d500c480008cccccccccccc88888888888848cccccccccccc00403403002c02802402001c01801401000c008cd405c060d5d0a80619a80b80c1aba1500b33501701935742a014666aa036eb94068d5d0a804999aa80dbae501a35742a01066a02e0446ae85401cccd5406c08dd69aba150063232323333573466e1cd55cea801240004664424660020060046464646666ae68cdc39aab9d5002480008cc8848cc00400c008cd40b5d69aba15002302e357426ae8940088c98c80c0cd5ce01901a01709aab9e5001137540026ae854008c8c8c8cccd5cd19b8735573aa004900011991091980080180119a816bad35742a004605c6ae84d5d1280111931901819ab9c03203402e135573ca00226ea8004d5d09aba2500223263202c33573805c06005426aae7940044dd50009aba1500533501775c6ae854010ccd5406c07c8004d5d0a801999aa80dbae200135742a00460426ae84d5d1280111931901419ab9c02a02c026135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d55cf280089baa00135742a00860226ae84d5d1280211931900d19ab9c01c01e018375a00a6666ae68cdc39aab9d375400a9000100e11931900c19ab9c01a01c016101b132632017335738921035054350001b135573ca00226ea800448c88c008dd6000990009aa80d911999aab9f0012500a233500930043574200460066ae880080608c8c8cccd5cd19b8735573aa004900011991091980080180118061aba150023005357426ae8940088c98c8050cd5ce00b00c00909aab9e5001137540024646464646666ae68cdc39aab9d5004480008cccc888848cccc00401401000c008c8c8c8cccd5cd19b8735573aa0049000119910919800801801180a9aba1500233500f014357426ae8940088c98c8064cd5ce00d80e80b89aab9e5001137540026ae854010ccd54021d728039aba150033232323333573466e1d4005200423212223002004357426aae79400c8cccd5cd19b875002480088c84888c004010dd71aba135573ca00846666ae68cdc3a801a400042444006464c6403666ae7007407c06406005c4d55cea80089baa00135742a00466a016eb8d5d09aba2500223263201533573802e03202626ae8940044d5d1280089aab9e500113754002266aa002eb9d6889119118011bab00132001355018223233335573e0044a010466a00e66442466002006004600c6aae754008c014d55cf280118021aba200301613574200222440042442446600200800624464646666ae68cdc3a800a400046a02e600a6ae84d55cf280191999ab9a3370ea00490011280b91931900819ab9c01201400e00d135573aa00226ea80048c8c8cccd5cd19b875001480188c848888c010014c01cd5d09aab9e500323333573466e1d400920042321222230020053009357426aae7940108cccd5cd19b875003480088c848888c004014c01cd5d09aab9e500523333573466e1d40112000232122223003005375c6ae84d55cf280311931900819ab9c01201400e00d00c00b135573aa00226ea80048c8c8cccd5cd19b8735573aa004900011991091980080180118029aba15002375a6ae84d5d1280111931900619ab9c00e01000a135573ca00226ea80048c8cccd5cd19b8735573aa002900011bae357426aae7940088c98c8028cd5ce00600700409baa001232323232323333573466e1d4005200c21222222200323333573466e1d4009200a21222222200423333573466e1d400d2008233221222222233001009008375c6ae854014dd69aba135744a00a46666ae68cdc3a8022400c4664424444444660040120106eb8d5d0a8039bae357426ae89401c8cccd5cd19b875005480108cc8848888888cc018024020c030d5d0a8049bae357426ae8940248cccd5cd19b875006480088c848888888c01c020c034d5d09aab9e500b23333573466e1d401d2000232122222223005008300e357426aae7940308c98c804ccd5ce00a80b80880800780700680600589aab9d5004135573ca00626aae7940084d55cf280089baa0012323232323333573466e1d400520022333222122333001005004003375a6ae854010dd69aba15003375a6ae84d5d1280191999ab9a3370ea0049000119091180100198041aba135573ca00c464c6401866ae700380400280244d55cea80189aba25001135573ca00226ea80048c8c8cccd5cd19b875001480088c8488c00400cdd71aba135573ca00646666ae68cdc3a8012400046424460040066eb8d5d09aab9e500423263200933573801601a00e00c26aae7540044dd500089119191999ab9a3370ea00290021091100091999ab9a3370ea00490011190911180180218031aba135573ca00846666ae68cdc3a801a400042444004464c6401466ae7003003802001c0184d55cea80089baa0012323333573466e1d40052002200623333573466e1d40092000200623263200633573801001400800626aae74dd5000a4c244004244002921035054310012333333357480024a00c4a00c4a00c46a00e6eb400894018008480044488c0080049400848488c00800c4488004448c8c00400488cc00cc0080080041";
static const char* PLUTUS_V3_CBOR              = "82035908955908920100003233223232323232332232323232323232323232332232323232322223232533532323232325335001101d13357389211e77726f6e67207573616765206f66207265666572656e636520696e7075740001c3232533500221533500221333573466e1c00800408007c407854cd4004840784078d40900114cd4c8d400488888888888802d40044c08526221533500115333533550222350012222002350022200115024213355023320015021001232153353235001222222222222300e00250052133550253200150233355025200100115026320013550272253350011502722135002225335333573466e3c00801c0940904d40b00044c01800c884c09526135001220023333573466e1cd55cea80224000466442466002006004646464646464646464646464646666ae68cdc39aab9d500c480008cccccccccccc88888888888848cccccccccccc00403403002c02802402001c01801401000c008cd405c060d5d0a80619a80b80c1aba1500b33501701935742a014666aa036eb94068d5d0a804999aa80dbae501a35742a01066a02e0446ae85401cccd5406c08dd69aba150063232323333573466e1cd55cea801240004664424660020060046464646666ae68cdc39aab9d5002480008cc8848cc00400c008cd40b5d69aba15002302e357426ae8940088c98c80c0cd5ce01901a01709aab9e5001137540026ae854008c8c8c8cccd5cd19b8735573aa004900011991091980080180119a816bad35742a004605c6ae84d5d1280111931901819ab9c03203402e135573ca00226ea8004d5d09aba2500223263202c33573805c06005426aae7940044dd50009aba1500533501775c6ae854010ccd5406c07c8004d5d0a801999aa80dbae200135742a00460426ae84d5d1280111931901419ab9c02a02c026135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d55cf280089baa00135742a00860226ae84d5d1280211931900d19ab9c01c01e018375a00a6666ae68cdc39aab9d375400a9000100e11931900c19ab9c01a01c016101b132632017335738921035054350001b135573ca00226ea800448c88c008dd6000990009aa80d911999aab9f0012500a233500930043574200460066ae880080608c8c8cccd5cd19b8735573aa004900011991091980080180118061aba150023005357426ae8940088c98c8050cd5ce00b00c00909aab9e5001137540024646464646666ae68cdc39aab9d5004480008cccc888848cccc00401401000c008c8c8c8cccd5cd19b8735573aa0049000119910919800801801180a9aba1500233500f014357426ae8940088c98c8064cd5ce00d80e80b89aab9e5001137540026ae854010ccd54021d728039aba150033232323333573466e1d4005200423212223002004357426aae79400c8cccd5cd19b875002480088c84888c004010dd71aba135573ca00846666ae68cdc3a801a400042444006464c6403666ae7007407c06406005c4d55cea80089baa00135742a00466a016eb8d5d09aba2500223263201533573802e03202626ae8940044d5d1280089aab9e500113754002266aa002eb9d6889119118011bab00132001355018223233335573e0044a010466a00e66442466002006004600c6aae754008c014d55cf280118021aba200301613574200222440042442446600200800624464646666ae68cdc3a800a400046a02e600a6ae84d55cf280191999ab9a3370ea00490011280b91931900819ab9c01201400e00d135573aa00226ea80048c8c8cccd5cd19b875001480188c848888c010014c01cd5d09aab9e500323333573466e1d400920042321222230020053009357426aae7940108cccd5cd19b875003480088c848888c004014c01cd5d09aab9e500523333573466e1d40112000232122223003005375c6ae84d55cf280311931900819ab9c01201400e00d00c00b135573aa00226ea80048c8c8cccd5cd19b8735573aa004900011991091980080180118029aba15002375a6ae84d5d1280111931900619ab9c00e01000a135573ca00226ea80048c8cccd5cd19b8735573aa002900011bae357426aae7940088c98c8028cd5ce00600700409baa001232323232323333573466e1d4005200c21222222200323333573466e1d4009200a21222222200423333573466e1d400d2008233221222222233001009008375c6ae854014dd69aba135744a00a46666ae68cdc3a8022400c4664424444444660040120106eb8d5d0a8039bae357426ae89401c8cccd5cd19b875005480108cc8848888888cc018024020c030d5d0a8049bae357426ae8940248cccd5cd19b875006480088c848888888c01c020c034d5d09aab9e500b23333573466e1d401d2000232122222223005008300e357426aae7940308c98c804ccd5ce00a80b80880800780700680600589aab9d5004135573ca00626aae7940084d55cf280089baa0012323232323333573466e1d400520022333222122333001005004003375a6ae854010dd69aba15003375a6ae84d5d1280191999ab9a3370ea0049000119091180100198041aba135573ca00c464c6401866ae700380400280244d55cea80189aba25001135573ca00226ea80048c8c8cccd5cd19b875001480088c8488c00400cdd71aba135573ca00646666ae68cdc3a8012400046424460040066eb8d5d09aab9e500423263200933573801601a00e00c26aae7540044dd500089119191999ab9a3370ea00290021091100091999ab9a3370ea00490011190911180180218031aba135573ca00846666ae68cdc3a801a400042444004464c6401466ae7003003802001c0184d55cea80089baa0012323333573466e1d40052002200623333573466e1d40092000200623263200633573801001400800626aae74dd5000a4c244004244002921035054310012333333357480024a00c4a00c4a00c46a00e6eb400894018008480044488c0080049400848488c00800c4488004448c8c00400488cc00cc0080080041";
static const char* NATIVE_SCRIPT_CBOR          = "82008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0";
static const char* REWARD_ADDRESS              = "stake_test1uppy2gm2hqzkwc80em4mlat73j4jyqvzhclrvsu72g9xg4q2yweet";
static const char* DREP_KEY_HASH_CBOR          = "8200581c00000000000000000000000000000000000000000000000000000000";
static const char* DREP_ID                     = "drep15cfxz9exyn5rx0807zvxfrvslrjqfchrd4d47kv9e0f46uedqtc";
static const char* ANCHOR_HASH                 = "26ce09df4e6f64fe5cf248968ab78f4b8a0092580c234d78f68c079c0fce34f0";
static const char* ANCHOR_URL                  = "https://storage.googleapis.com/biglup/Angel_Castillo.jsonld";
static const char* GOVERNANCE_ACTION_ID_CBOR   = "825820000000000000000000000000000000000000000000000000000000000000000003";
static const char* CBOR_YES_WITH_ANCHOR        = "8201827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";

/* STATIC FUNCTIONS **********************************************************/

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
    CARDANO_UNUSED(memccpy((void*)&context->key[0], "This is a test key", strlen("This is a test key"), sizeof(context->key)));

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
  CARDANO_UNUSED(memccpy((void*)&impl.name[0], "Empty Coin Selector", strlen("Empty Coin Selector"), sizeof(impl.name)));

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
  CARDANO_UNUSED(memccpy((void*)&impl.name[0], "Empty Tx Evaluator", strlen("Empty Tx Evaluator"), sizeof(impl.name)));

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

/* UNIT TESTS ****************************************************************/

TEST(cardano_tx_builder_new, canCreateATxBuilder)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Clean up
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_new, returnsErrorOnMemoryAllocationFailure)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  for (int i = 0; i < 25; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);
    cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

    EXPECT_EQ(builder, nullptr);
  }

  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_set_allocators(malloc, realloc, free);
  cardano_provider_unref(&provider);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_tx_builder_set_coin_selector, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_coin_selector(nullptr, nullptr);
  cardano_tx_builder_set_coin_selector(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_coin_selector, canSetCoinSelector)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_coin_selector_t*       selector = NULL;

  EXPECT_EQ(cardano_coin_selector_new(cardano_empty_coin_selector_impl_new(), &selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_coin_selector(builder, selector);

  // Assert
  EXPECT_EQ(builder->coin_selector, selector);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_coin_selector_unref(&selector);
}

TEST(cardano_tx_builder_set_network_id, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_network_id(nullptr, CARDANO_NETWORK_ID_MAIN_NET);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_network_id, canSetNetworkId)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_network_id(builder, CARDANO_NETWORK_ID_MAIN_NET);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  const cardano_network_id_t* network_id = cardano_transaction_body_get_network_id(body);

  // Assert
  EXPECT_EQ(*network_id, CARDANO_NETWORK_ID_MAIN_NET);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_network_id, returnsErroIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_transaction_unref(&builder->transaction);
  builder->transaction = NULL;

  // Act
  cardano_tx_builder_set_network_id(builder, CARDANO_NETWORK_ID_MAIN_NET);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_network_id, returnsErroIfMemoryAllocaitonFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_network_id(builder, CARDANO_NETWORK_ID_MAIN_NET);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_tx_evaluator, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_tx_evaluator(nullptr, nullptr);
  cardano_tx_builder_set_tx_evaluator(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_tx_evaluator, canSetTxEvaluator)
{
  // Arrange
  cardano_protocol_parameters_t* params    = init_protocol_parameters();
  cardano_provider_t*            provider  = NULL;
  cardano_tx_evaluator_t*        evaluator = NULL;

  EXPECT_EQ(cardano_tx_evaluator_new(cardano_empty_tx_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_tx_evaluator(builder, evaluator);

  // Assert
  EXPECT_EQ(builder->tx_evaluator, evaluator);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_evaluator_unref(&evaluator);
}

TEST(cardano_tx_builder_set_change_address, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_change_address(nullptr, nullptr);
  cardano_tx_builder_set_change_address(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_change_address, canSetChangeAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_change_address(builder, address);

  // Assert
  EXPECT_EQ(builder->change_address, address);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_set_change_address_ex, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_change_address_ex(nullptr, "", 0);
  cardano_tx_builder_set_change_address_ex(builder, nullptr, 0);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_change_address_ex, canSetChangeAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_change_address_ex(builder, "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"));

  // Assert
  EXPECT_STREQ(cardano_address_get_string(builder->change_address), cardano_address_get_string(address));

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_set_change_address_ex, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_change_address_ex(builder, "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"));

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_INVALID_ADDRESS_FORMAT);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_collateral_change_address, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_collateral_change_address(nullptr, nullptr);
  cardano_tx_builder_set_collateral_change_address(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_collateral_change_address, canSetCollateralChangeAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_collateral_change_address(builder, address);

  // Assert
  EXPECT_EQ(builder->collateral_address, address);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_set_collateral_change_address_ex, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_collateral_change_address_ex(nullptr, "", 0);
  cardano_tx_builder_set_collateral_change_address_ex(builder, nullptr, 0);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_collateral_change_address_ex, canSetCollateralChangeAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_collateral_change_address_ex(builder, "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"));

  // Assert
  EXPECT_STREQ(cardano_address_get_string(builder->collateral_address), cardano_address_get_string(address));

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_set_collateral_change_address_ex, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_collateral_change_address_ex(builder, "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"));

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_INVALID_ADDRESS_FORMAT);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_minimum_fee, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_minimum_fee(nullptr, 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_minimum_fee, canSetMinimumFee)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_minimum_fee(builder, 1000);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_EQ(cardano_transaction_body_get_fee(body), 1000);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_minimum_fee, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_transaction_unref(&builder->transaction);
  builder->transaction = NULL;

  // Act
  cardano_tx_builder_set_minimum_fee(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_utxo_list_t*           utxos    = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_utxos(nullptr, nullptr);
  cardano_tx_builder_set_utxos(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_list_unref(&utxos);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_utxos, canSetUtxos)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_utxo_list_t*           utxos    = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_utxos(builder, utxos);

  // Assert
  EXPECT_EQ(builder->available_utxos, utxos);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_new, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Assert
  EXPECT_THAT(builder, nullptr);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_collateral_utxos, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_utxo_list_t*           utxos    = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_collateral_utxos(nullptr, nullptr);
  cardano_tx_builder_set_collateral_utxos(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_list_unref(&utxos);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_collateral_utxos, canSetCollateralUtxos)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_utxo_list_t*           utxos    = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&utxos), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_collateral_utxos(builder, utxos);

  // Assert
  EXPECT_EQ(builder->collateral_utxos, utxos);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_set_invalid_after, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_invalid_after(nullptr, 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_invalid_after, canSetInvalidAfter)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_invalid_after(builder, 1000);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_EQ(*cardano_transaction_body_get_invalid_after(body), 1000);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_invalid_after, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_transaction_unref(&builder->transaction);
  builder->transaction = NULL;

  // Act
  cardano_tx_builder_set_invalid_after(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_invalid_after, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_invalid_after(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_invalid_after_ex, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_invalid_after_ex(nullptr, 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_invalid_after_ex, canSetInvalidAfter)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_invalid_after_ex(builder, 1730901968);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_EQ(*cardano_transaction_body_get_invalid_after(body), 139335677);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_invalid_after_ex, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_transaction_unref(&builder->transaction);
  builder->transaction = NULL;

  // Act
  cardano_tx_builder_set_invalid_after_ex(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_invalid_after_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_invalid_after_ex(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_invalid_before, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_invalid_before(nullptr, 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_set_invalid_before, canSetInvalidBefore)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_invalid_before(builder, 1000);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_EQ(*cardano_transaction_body_get_invalid_before(body), 1000);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_invalid_before, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_transaction_unref(&builder->transaction);
  builder->transaction = NULL;

  // Act
  cardano_tx_builder_set_invalid_before(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_invalid_before, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_invalid_before(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_set_invalid_before_ex, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_set_invalid_before_ex(nullptr, 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_invalid_before_ex, canSetInvalidBefore)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_invalid_before_ex(builder, 1730901968);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  // Assert
  EXPECT_EQ(*cardano_transaction_body_get_invalid_before(body), 139335677);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_invalid_before_ex, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_transaction_unref(&builder->transaction);
  builder->transaction = NULL;

  cardano_tx_builder_set_invalid_before_ex(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_invalid_before_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_set_invalid_before_ex(builder, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_add_reference_input, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_utxo_t*                utxo     = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_add_reference_input(nullptr, utxo);
  cardano_tx_builder_add_reference_input(builder, nullptr);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_unref(&utxo);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_add_reference_input, canAddReferenceInput)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_utxo_t*                utxo1    = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);
  cardano_utxo_t*                utxo2    = create_utxo(UTXO_WITH_REF_SCRIPT_PV2);
  cardano_utxo_t*                utxo3    = create_utxo(UTXO_WITH_REF_SCRIPT_PV3);
  cardano_utxo_t*                utxo4    = create_utxo(UTXO_WITH_REF_SCRIPT_NATIVE);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_add_reference_input(builder, utxo1);
  cardano_tx_builder_add_reference_input(builder, utxo2);
  cardano_tx_builder_add_reference_input(builder, utxo3);
  cardano_tx_builder_add_reference_input(builder, utxo4);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_transaction_input_set_t* inputs = cardano_transaction_body_get_reference_inputs(body);
  cardano_transaction_input_set_unref(&inputs);

  // Assert
  EXPECT_EQ(cardano_transaction_input_set_get_length(inputs), 4);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_unref(&utxo1);
  cardano_utxo_unref(&utxo2);
  cardano_utxo_unref(&utxo3);
  cardano_utxo_unref(&utxo4);
}

TEST(cardano_tx_builder_add_reference_input, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_utxo_t*                utxo     = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_transaction_unref(&builder->transaction);
  builder->transaction = NULL;

  // Act
  cardano_tx_builder_add_reference_input(builder, utxo);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_tx_builder_add_reference_input, returnsErrorIfReferenceInputsIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_utxo_t*                utxo     = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_utxo_list_unref(&builder->reference_inputs);
  builder->reference_inputs = NULL;

  cardano_tx_builder_add_reference_input(builder, utxo);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_tx_builder_add_reference_input, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_utxo_t*                utxo     = create_utxo(UTXO_WITH_REF_SCRIPT_PV1);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_add_reference_input(builder, utxo);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_utxo_unref(&utxo);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_send_lovelace, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_send_lovelace(nullptr, address, 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_send_lovelace, canSendLovelace)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_send_lovelace(builder, address, 1000);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
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
  cardano_provider_unref(&provider);
  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_send_lovelace, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_transaction_unref(&builder->transaction);
  builder->transaction = NULL;

  // Act
  cardano_tx_builder_send_lovelace(builder, address, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_send_lovelace, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_send_lovelace(builder, nullptr, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_send_lovelace, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_send_lovelace(builder, address, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&address);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_send_lovelace_ex, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  const char* address = "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg";

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_send_lovelace_ex(nullptr, address, strlen(address), 0);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_send_lovelace_ex, canSendLovelace)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  const char* address = "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg";

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_lovelace_ex(builder, address, strlen(address), 1000);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
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
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_send_lovelace_ex, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  const char* address = nullptr;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_lovelace_ex(builder, address, 0, 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_send_lovelace_ex, returnsErrorIfInvalidAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  const char* address = "invalid_address";

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_lovelace_ex(builder, address, strlen(address), 1000);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_send_value, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;
  cardano_value_t*               value    = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_send_value(nullptr, address, value);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&address);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value, canSendValue)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;
  cardano_value_t*               value    = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_set_coin(value, 1000), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_value(builder, address, value);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
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
  cardano_provider_unref(&provider);
  cardano_address_unref(&address);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;
  cardano_value_t*               value    = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_set_coin(value, 1000), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_transaction_unref(&builder->transaction);
  builder->transaction = NULL;

  cardano_tx_builder_send_value(builder, address, value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
  cardano_address_unref(&address);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_value_t*               value    = cardano_value_new_zero();

  cardano_address_t* address = nullptr;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_set_coin(value, 1000), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_value(builder, address, value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;

  cardano_value_t* value = nullptr;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_value(builder, address, value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
  cardano_address_unref(&address);
}

TEST(cardano_tx_builder_send_value, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_address_t*             address  = NULL;
  cardano_value_t*               value    = cardano_value_new_zero();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_tx_builder_send_value(builder, address, value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_address_unref(&address);
  cardano_value_unref(&value);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_send_value_ex, doesntCrashWehnGivenNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  const char*      address = "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg";
  cardano_value_t* value   = cardano_value_new_zero();

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_send_value_ex(nullptr, address, strlen(address), value);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value_ex, canSendValue)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  const char*      address = "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg";
  cardano_value_t* value   = cardano_value_new_zero();

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_value_ex(builder, address, strlen(address), value);

  cardano_transaction_body_t* body = cardano_transaction_get_body(builder->transaction);
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
  cardano_provider_unref(&provider);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value_ex, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  const char*      address = nullptr;
  cardano_value_t* value   = cardano_value_new_zero();

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_value_ex(builder, address, 0, value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value_ex, returnsErrorIfInvalidAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  const char*      address = "invalid_address";
  cardano_value_t* value   = cardano_value_new_zero();

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_value_ex(builder, address, strlen(address), value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
  cardano_value_unref(&value);
}

TEST(cardano_tx_builder_send_value_ex, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  const char*      address = "addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg";
  cardano_value_t* value   = nullptr;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_send_value_ex(builder, address, strlen(address), value);

  // Assert
  EXPECT_THAT(builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_tx_builder_unref(&builder);
}

TEST(cardano_tx_builder_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_ref(tx_builder);

  // Assert
  EXPECT_THAT(tx_builder, testing::Not((cardano_tx_builder_t*)nullptr));
  EXPECT_EQ(cardano_tx_builder_refcount(tx_builder), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_tx_builder_unref(&tx_builder);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_pad_signer_count, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_tx_builder_pad_signer_count(nullptr, 0);
}

TEST(cardano_tx_builder_pad_signer_count, canSetTheSignerCount)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_pad_signer_count(builder, 10);

  // Assert
  EXPECT_EQ(builder->additional_signature_count, 10);

  // Cleanup
  cardano_tx_builder_unref(&builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  const char* message = nullptr;

  // Act
  cardano_tx_builder_set_last_error(tx_builder, message);

  // Assert
  EXPECT_STREQ(cardano_tx_builder_get_last_error(tx_builder), "");

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_error_t result = cardano_tx_builder_build(tx_builder, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_build, returnsErrorIfBuilderIsInErrorState)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_transaction_t* tx = nullptr;

  tx_builder->last_error = CARDANO_ERROR_POINTER_IS_NULL;
  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_build, returnsErrorIfChangeAddressNotSet)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_build, returnsErrorIfUtxosNotSet)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
  cardano_address_unref(&change_address);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_tx_builder_build, returnsErrorIfBalancingFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_address_t*             change_address = nullptr;
  cardano_utxo_list_t*           utxos          = new_utxo_list();

  EXPECT_EQ(cardano_address_from_string("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", strlen("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg"), &change_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_set_change_address(tx_builder, change_address);
  cardano_tx_builder_set_utxos(tx_builder, utxos);

  // Act
  cardano_transaction_t* tx = nullptr;

  cardano_transaction_unref(&tx_builder->transaction);
  tx_builder->transaction = NULL;

  cardano_error_t result = cardano_tx_builder_build(tx_builder, &tx);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
  cardano_transaction_unref(&tx);
}

TEST(cardano_tx_builder_add_input, canAddInput)
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 9; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 1024; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_metadata(tx_builder, 0, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_metadata, canSetMetadata)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_metadatum_t*           metadata = NULL;

  EXPECT_EQ(cardano_metadatum_new_string("TEST", 4, &metadata), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_metadata(tx_builder, 0, metadata);

  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(tx_builder->transaction);
  cardano_witness_set_unref(&witnesses);

  cardano_auxiliary_data_t* aux_data = cardano_transaction_get_auxiliary_data(tx_builder->transaction);
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
  cardano_provider_unref(&provider);
  cardano_metadatum_unref(&metadata);
}

TEST(cardano_tx_builder_set_metadata, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_metadatum_t*           metadata = NULL;

  EXPECT_EQ(cardano_metadatum_new_string("TEST", 4, &metadata), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 5; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_metadata_ex(tx_builder, 0, nullptr, 0);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_metadata_ex, canSetMetadata)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_set_metadata_ex(tx_builder, 0, "{ \"name\": \"test\" }", strlen("{ \"name\": \"test\" }"));

  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(tx_builder->transaction);
  cardano_witness_set_unref(&witnesses);

  cardano_auxiliary_data_t* aux_data = cardano_transaction_get_auxiliary_data(tx_builder->transaction);
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
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_set_metadata_ex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 16; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_set_metadata_ex(tx_builder, 0, "{ \"name\": \"test\" }", strlen("{ \"name\": \"test\" }"));

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
  cardano_provider_unref(&provider);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_mint_token, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_mint_token(nullptr, nullptr, nullptr, 0, nullptr);
  cardano_tx_builder_mint_token(tx_builder, nullptr, nullptr, 0, nullptr);
  cardano_tx_builder_mint_token(tx_builder, (cardano_blake2b_hash_t*)"", nullptr, 0, nullptr);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_mint_token, canSentMintToken)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_protocol_parameters();
  cardano_provider_t*            provider   = NULL;
  cardano_asset_name_t*          asset_name = NULL;
  cardano_blake2b_hash_t*        policy_id  = NULL;
  cardano_plutus_data_t*         redeemer   = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_asset_name_from_string("TEXT", 4, &asset_name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_id), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_transaction_t* tx = nullptr;
  cardano_tx_builder_mint_token(tx_builder, policy_id, asset_name, 4, redeemer);
  cardano_tx_builder_mint_token(tx_builder, policy_id, asset_name, 4, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
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
  cardano_provider_unref(&provider);
  cardano_transaction_unref(&tx);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
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

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 14; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_mint_token(tx_builder, policy_id, asset_name, 4, redeemer);

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
  cardano_provider_unref(&provider);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_plutus_data_unref(&redeemer);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_mint_token_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_mint_token_ex(nullptr, nullptr, 0, nullptr, 0, 0, nullptr);
  cardano_tx_builder_mint_token_ex(tx_builder, nullptr, 0, nullptr, 0, 0, nullptr);
  tx_builder->last_error = CARDANO_SUCCESS;
  cardano_tx_builder_mint_token_ex(tx_builder, "1", 1, nullptr, 0, 0, nullptr);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_mint_token_ex, canSentMintToken)
{
  // Arrange
  cardano_protocol_parameters_t* params     = init_protocol_parameters();
  cardano_provider_t*            provider   = NULL;
  cardano_plutus_data_t*         redeemer   = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_asset_name_t*          asset_name = NULL;
  cardano_blake2b_hash_t*        policy_id  = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_asset_name_from_string("TEXT", 4, &asset_name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &policy_id), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_transaction_t* tx = nullptr;
  cardano_tx_builder_mint_token_ex(tx_builder, HASH_HEX, strlen(HASH_HEX), "54455854", strlen("54455854"), 4, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
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
  cardano_provider_unref(&provider);
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

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 18; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_plutus_data_unref(&redeemer);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_mint_token_with_id, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_mint_token_with_id(nullptr, nullptr, 0, nullptr);
  cardano_tx_builder_mint_token_with_id(tx_builder, nullptr, 0, nullptr);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_id_from_hex(ASSET_ID_HEX, strlen(ASSET_ID_HEX), &asset_id), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_transaction_t* tx = nullptr;
  cardano_tx_builder_mint_token_with_id(tx_builder, asset_id, 4, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
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
  cardano_provider_unref(&provider);
  cardano_transaction_unref(&tx);
  cardano_plutus_data_unref(&redeemer);
  cardano_asset_name_unref(&asset_name);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_id_unref(&asset_id);
}

TEST(cardano_tx_builder_mint_token_with_id_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_mint_token_with_id_ex(nullptr, nullptr, 0, 0, nullptr);
  cardano_tx_builder_mint_token_with_id_ex(tx_builder, nullptr, 0, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_id_from_hex(ASSET_ID_HEX, strlen(ASSET_ID_HEX), &asset_id), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_transaction_t* tx = nullptr;
  cardano_tx_builder_mint_token_with_id_ex(tx_builder, ASSET_ID_HEX, strlen(ASSET_ID_HEX), 4, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
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
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_id_from_hex(ASSET_ID_HEX, strlen(ASSET_ID_HEX), &asset_id), CARDANO_SUCCESS);

  for (int i = 0; i < 19; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_add_signer(tx_builder, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_add_signer, canAddSigner)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_provider_t*            provider    = NULL;
  cardano_blake2b_hash_t*        signing_key = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &signing_key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_add_signer(tx_builder, signing_key);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_blake2b_hash_set_t* signers = cardano_transaction_body_get_required_signers(body);
  cardano_blake2b_hash_set_unref(&signers);

  cardano_blake2b_hash_t* signer = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_set_get(signers, 0, &signer), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(signer, signing_key);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_blake2b_hash_unref(&signing_key);
  cardano_blake2b_hash_unref(&signer);
}

TEST(cardano_tx_builder_add_signer, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_provider_t*            provider    = NULL;
  cardano_blake2b_hash_t*        signing_key = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &signing_key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 3; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
  cardano_blake2b_hash_unref(&signing_key);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_add_signer_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  cardano_tx_builder_add_signer_ex(nullptr, nullptr, 0);
  cardano_tx_builder_add_signer_ex(tx_builder, nullptr, 0);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_add_signer_ex, canAddSigner)
{
  // Arrange
  cardano_protocol_parameters_t* params      = init_protocol_parameters();
  cardano_provider_t*            provider    = NULL;
  cardano_blake2b_hash_t*        signing_key = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(HASH_HEX, strlen(HASH_HEX), &signing_key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_add_signer_ex(tx_builder, HASH_HEX, strlen(HASH_HEX));

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_blake2b_hash_set_t* signers = cardano_transaction_body_get_required_signers(body);
  cardano_blake2b_hash_set_unref(&signers);

  cardano_blake2b_hash_t* signer = nullptr;

  EXPECT_EQ(cardano_blake2b_hash_set_get(signers, 0, &signer), CARDANO_SUCCESS);

  // Assert
  EXPECT_TRUE(cardano_blake2b_hash_equals(signer, signing_key));

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 6; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
  cardano_blake2b_hash_unref(&signing_key);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_add_datum, doesntCrashIfGivenNull)
{
  cardano_tx_builder_add_datum(nullptr, nullptr);
}

TEST(cardano_tx_builder_add_datum, returnsErrorIfDatumIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_add_datum(tx_builder, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_add_datum, canAddDatum)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_plutus_data_t*         datum    = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_add_datum(tx_builder, datum);

  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(tx_builder->transaction);
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
  cardano_provider_unref(&provider);
  cardano_plutus_data_unref(&datum);
  cardano_plutus_data_unref(&datum_out);
}

TEST(cardano_tx_builder_add_datum, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_plutus_data_t*         datum    = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 3; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_add_script(tx_builder, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_add_script(tx_builder, scriptV1);
  cardano_tx_builder_add_script(tx_builder, scriptV2);
  cardano_tx_builder_add_script(tx_builder, scriptV3);
  cardano_tx_builder_add_script(tx_builder, scriptNative);

  cardano_witness_set_t* witnesses = cardano_transaction_get_witness_set(tx_builder->transaction);
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
  EXPECT_TRUE(tx_builder->has_plutus_v1);
  EXPECT_TRUE(tx_builder->has_plutus_v2);
  EXPECT_TRUE(tx_builder->has_plutus_v3);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_script_unref(&scriptV1);
  cardano_script_unref(&scriptV2);
  cardano_script_unref(&scriptV3);
  cardano_script_unref(&scriptNative);
}

TEST(cardano_tx_builder_add_script, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;
  cardano_script_t*              script   = create_script(PLUTUS_V1_CBOR);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 3; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_withdraw_rewards(tx_builder, nullptr, 0, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_withdraw_rewards, returnsErrorIfRewardAmountLessThanZero)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_reward_address_t*      reward_address = nullptr;

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_withdraw_rewards(tx_builder, reward_address, -1, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_reward_address_unref(&reward_address);
}

TEST(cardano_tx_builder_withdraw_rewards, canWithdrawRewards)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_withdraw_rewards(tx_builder, reward_address, 1000, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
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
  cardano_provider_unref(&provider);
  cardano_reward_address_unref(&reward_address);
  cardano_plutus_data_unref(&redeemer);
}

TEST(cardano_tx_builder_withdraw_rewards, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 13; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_withdraw_rewards(tx_builder, reward_address, 1000, redeemer);

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
  cardano_provider_unref(&provider);
  cardano_reward_address_unref(&reward_address);
  cardano_plutus_data_unref(&redeemer);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_withdraw_rewards_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_withdraw_rewards_ex(nullptr, nullptr, 0, 0, nullptr);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_withdraw_rewards_ex(tx_builder, nullptr, 0, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_withdraw_rewards_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_withdraw_rewards_ex(tx_builder, REWARD_ADDRESS, 1, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_withdraw_rewards_ex, canWithdrawRewards)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_withdraw_rewards_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), 1000, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
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
  cardano_provider_unref(&provider);
  cardano_reward_address_unref(&reward_address);
  cardano_plutus_data_unref(&redeemer);
}

TEST(cardano_tx_builder_register_reward_address, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_register_reward_address(nullptr, nullptr, nullptr);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_register_reward_address(tx_builder, nullptr, nullptr);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_register_reward_address, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_register_reward_address(tx_builder, nullptr, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_register_reward_address, canRegisterRewardAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_register_reward_address(tx_builder, reward_address, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 10; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
  cardano_reward_address_unref(&reward_address);
  cardano_plutus_data_unref(&redeemer);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_register_reward_address_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_register_reward_address_ex(nullptr, nullptr, 0, nullptr);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_register_reward_address_ex(tx_builder, nullptr, 0, nullptr);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_register_reward_address_ex, canRegisterRewardAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_register_reward_address_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_deregister_reward_address(tx_builder, nullptr, nullptr);

  // Assert
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_deregister_reward_address, canDeregisterRewardAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_deregister_reward_address(tx_builder, reward_address, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 10; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
  cardano_reward_address_unref(&reward_address);
  cardano_plutus_data_unref(&redeemer);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_deregister_reward_address_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = NULL;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_deregister_reward_address_ex(nullptr, nullptr, 0, nullptr);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_deregister_reward_address_ex(tx_builder, nullptr, 0, nullptr);

  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_deregister_reward_address_ex, canDeregisterRewardAddress)
{
  // Arrange
  cardano_protocol_parameters_t* params         = init_protocol_parameters();
  cardano_provider_t*            provider       = NULL;
  cardano_reward_address_t*      reward_address = nullptr;
  cardano_plutus_data_t*         redeemer       = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_reward_address_from_bech32(REWARD_ADDRESS, strlen(REWARD_ADDRESS), &reward_address), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_deregister_reward_address_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_delegate_stake(tx_builder, nullptr, pool_id, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_delegate_stake(tx_builder, reward_address, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_delegate_stake(tx_builder, reward_address, pool_id, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 10; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
  cardano_reward_address_unref(&reward_address);
  cardano_blake2b_hash_unref(&pool_id);
  cardano_plutus_data_unref(&redeemer);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_delegate_stake_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_delegate_stake_ex(nullptr, nullptr, 0, nullptr, 0, nullptr);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_delegate_stake_ex(tx_builder, nullptr, 0, nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_delegate_stake_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_delegate_stake_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), "pool1pzdqdxrv0k74p4q33y98f2u7vzaz95et7mjeedjcfy0jcgk754f", strlen("pool1pzdqdxrv0k74p4q33y98f2u7vzaz95et7mjeedjcfy0jcgk754f"), redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  // Act
  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_delegate_stake_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), "pool1pzdqdxrv0k74p4q33y98f2u7vzaz95et7mjeedjcfy0jcgk754", strlen("pool1pzdqdxrv0k74p4q33y98f2u7vzaz95et7mjeedjcfy0jcgk754"), redeemer);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_DECODING);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_delegate_stake_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), "test1pzdqdxrv0k74p4q33y98f2u7vzaz95et7mjeedjcfy0jclsudc9", strlen("test1pzdqdxrv0k74p4q33y98f2u7vzaz95et7mjeedjcfy0jclsudc9"), redeemer);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_INVALID_ARGUMENT);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 44; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_delegate_voting_power(tx_builder, nullptr, drep, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_delegate_voting_power(tx_builder, reward_address, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_delegate_voting_power(tx_builder, reward_address, drep, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 10; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_add_certificate(tx_builder, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_delegate_voting_power_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_delegate_voting_power_ex(nullptr, nullptr, 0, nullptr, 0, nullptr);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_delegate_voting_power_ex(tx_builder, nullptr, 0, nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_delegate_voting_power_ex(tx_builder, "1", 1, nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_delegate_voting_power_ex(tx_builder, "1", 1, "1", 1, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_delegate_voting_power_ex(tx_builder, REWARD_ADDRESS, strlen(REWARD_ADDRESS), DREP_ID, strlen(DREP_ID), redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_register_drep(tx_builder, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_register_drep(tx_builder, drep, anchor, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 10; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_register_drep_ex(nullptr, nullptr, 0, nullptr, 0, nullptr, 0, nullptr);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_register_drep_ex(tx_builder, nullptr, 0, nullptr, 0, nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_register_drep_ex(tx_builder, DREP_ID, strlen(DREP_ID), nullptr, 0, nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_register_drep_ex(tx_builder, DREP_ID, strlen(DREP_ID), ANCHOR_URL, strlen(DREP_ID), nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_register_drep_ex(tx_builder, "1", 1, ANCHOR_URL, strlen(DREP_ID), nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_register_drep_ex(tx_builder, DREP_ID, strlen(DREP_ID), "1", 1, "1", 1, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_register_drep_ex, canRegisterDrep)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_anchor_t*              anchor   = nullptr;

  EXPECT_EQ(cardano_anchor_from_hash_hex(ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), &anchor), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_register_drep_ex(tx_builder, DREP_ID, strlen(DREP_ID), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_update_drep(tx_builder, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_update_drep(tx_builder, drep, anchor, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 10; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
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

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_update_drep_ex(nullptr, nullptr, 0, nullptr, 0, nullptr, 0, nullptr);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_update_drep_ex(tx_builder, nullptr, 0, nullptr, 0, nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_update_drep_ex(tx_builder, DREP_ID, strlen(DREP_ID), nullptr, 0, nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_update_drep_ex(tx_builder, DREP_ID, strlen(DREP_ID), ANCHOR_URL, strlen(ANCHOR_URL), nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_update_drep_ex(tx_builder, "1", 1, ANCHOR_URL, strlen(ANCHOR_URL), "1", 1, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_update_drep_ex(tx_builder, "1", 1, ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), nullptr);
  cardano_tx_builder_unref(&tx_builder);

  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_update_drep_ex, canUpdateDrep)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_anchor_t*              anchor   = nullptr;

  EXPECT_EQ(cardano_anchor_from_hash_hex(ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), &anchor), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_update_drep_ex(tx_builder, DREP_ID, strlen(DREP_ID), ANCHOR_URL, strlen(ANCHOR_URL), ANCHOR_HASH, strlen(ANCHOR_HASH), redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_deregister_drep(tx_builder, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_deregister_drep(tx_builder, drep, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 10; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

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
  cardano_provider_unref(&provider);
  cardano_drep_unref(&drep);
  cardano_plutus_data_unref(&redeemer);
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_tx_builder_deregister_drep_ex, doesntCrashIfGivenNull)
{
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_deregister_drep_ex(nullptr, nullptr, 0, nullptr);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_deregister_drep_ex(tx_builder, nullptr, 0, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_deregister_drep_ex(tx_builder, DREP_ID, strlen(DREP_ID), nullptr);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_deregister_drep_ex(tx_builder, "1", 1, nullptr);
  cardano_tx_builder_unref(&tx_builder);

  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
}

TEST(cardano_tx_builder_deregister_drep_ex, canDeregisterDrep)
{
  // Arrange
  cardano_protocol_parameters_t* params   = init_protocol_parameters();
  cardano_provider_t*            provider = nullptr;
  cardano_plutus_data_t*         redeemer = create_plutus_data(PLUTUS_DATA_CBOR);

  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_deregister_drep_ex(tx_builder, DREP_ID, strlen(DREP_ID), redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
  cardano_transaction_body_unref(&body);

  cardano_certificate_set_t* certs = cardano_transaction_body_get_certificates(body);
  cardano_certificate_set_unref(&certs);

  // Assert
  EXPECT_EQ(cardano_certificate_set_get_length(certs), 1);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
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
  cardano_plutus_data_t*          redeemer  = create_plutus_data(PLUTUS_DATA_CBOR);
  cardano_governance_action_id_t* action_id = nullptr;
  cardano_voting_procedure_t*     procedure = nullptr;

  cardano_cbor_reader_t* gov_action_reader       = cardano_cbor_reader_from_hex(GOVERNANCE_ACTION_ID_CBOR, strlen(GOVERNANCE_ACTION_ID_CBOR));
  cardano_cbor_reader_t* voting_procedure_reader = cardano_cbor_reader_from_hex(CBOR_YES_WITH_ANCHOR, strlen(CBOR_YES_WITH_ANCHOR));

  EXPECT_EQ(cardano_governance_action_id_from_cbor(gov_action_reader, &action_id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_voting_procedure_from_cbor(voting_procedure_reader, &procedure), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_vote(tx_builder, nullptr, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_vote(tx_builder, voter, nullptr, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  tx_builder = cardano_tx_builder_new(params, provider);
  cardano_tx_builder_vote(tx_builder, voter, action_id, nullptr, nullptr);
  EXPECT_THAT(tx_builder->last_error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_tx_builder_unref(&tx_builder);

  // Cleanup
  cardano_tx_builder_unref(&tx_builder);
  cardano_protocol_parameters_unref(&params);
  cardano_provider_unref(&provider);
  cardano_plutus_data_unref(&redeemer);
  cardano_governance_action_id_unref(&action_id);
  cardano_voting_procedure_unref(&procedure);
  cardano_cbor_reader_unref(&gov_action_reader);
  cardano_cbor_reader_unref(&voting_procedure_reader);
  cardano_voter_unref(&voter);
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

  // Act
  cardano_tx_builder_vote(tx_builder, voter, action_id, procedure, redeemer);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx_builder->transaction);
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
  cardano_provider_unref(&provider);
  cardano_plutus_data_unref(&redeemer);
  cardano_governance_action_id_unref(&action_id);
  cardano_voting_procedure_unref(&procedure);
  cardano_cbor_reader_unref(&gov_action_reader);
  cardano_cbor_reader_unref(&voting_procedure_reader);
  cardano_voter_unref(&voter);
  cardano_voter_list_unref(&voters);
}

TEST(cardano_tx_builder_vote, returnsErrorIfMemoryAllocationFails)
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
  EXPECT_EQ(cardano_provider_new(cardano_provider_impl_new(), &provider), CARDANO_SUCCESS);

  for (int i = 0; i < 10; ++i)
  {
    cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(params, provider);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    // Act
    cardano_tx_builder_vote(tx_builder, voter, action_id, procedure, redeemer);

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
  cardano_provider_unref(&provider);
  cardano_plutus_data_unref(&redeemer);
  cardano_governance_action_id_unref(&action_id);
  cardano_voting_procedure_unref(&procedure);
  cardano_cbor_reader_unref(&gov_action_reader);
  cardano_cbor_reader_unref(&voting_procedure_reader);
  cardano_voter_unref(&voter);
  cardano_set_allocators(malloc, realloc, free);
}