/**
 * \file script.cpp
 *
 * \author angel.castillo
 * \date   May 14, 2024
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
#include <cardano/scripts/script.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <cardano/scripts/native_scripts/native_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v1_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v2_script.h>
#include <cardano/scripts/plutus_scripts/plutus_v3_script.h>
#include <gmock/gmock.h>

// clang-format on

/* CONSTS ********************************************************************/

static const char* PUBKEY_SCRIPT =
  "{\n"
  "  \"type\": \"sig\",\n"
  "  \"keyHash\": \"966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37\"\n"
  "}";

static const char* PUBKEY_SCRIPT2 =
  "{\n"
  "  \"type\": \"sig\",\n"
  "  \"keyHash\": \"566e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37\"\n"
  "}";

static const char* PLUTUS_V1_SCRIPT = "4d01000033222220051200120011";
static const char* PLUTUS_V1_HASH   = "67f33146617a5e61936081db3b2117cbf59bd2123748f58ac9678656";
static const char* PLUTUS_V1_CBOR   = "82014e4d01000033222220051200120011";
static const char* PLUTUS_V2_SCRIPT = "5908920100003233223232323232332232323232323232323232332232323232322223232533532323232325335001101d13357389211e77726f6e67207573616765206f66207265666572656e636520696e7075740001c3232533500221533500221333573466e1c00800408007c407854cd4004840784078d40900114cd4c8d400488888888888802d40044c08526221533500115333533550222350012222002350022200115024213355023320015021001232153353235001222222222222300e00250052133550253200150233355025200100115026320013550272253350011502722135002225335333573466e3c00801c0940904d40b00044c01800c884c09526135001220023333573466e1cd55cea80224000466442466002006004646464646464646464646464646666ae68cdc39aab9d500c480008cccccccccccc88888888888848cccccccccccc00403403002c02802402001c01801401000c008cd405c060d5d0a80619a80b80c1aba1500b33501701935742a014666aa036eb94068d5d0a804999aa80dbae501a35742a01066a02e0446ae85401cccd5406c08dd69aba150063232323333573466e1cd55cea801240004664424660020060046464646666ae68cdc39aab9d5002480008cc8848cc00400c008cd40b5d69aba15002302e357426ae8940088c98c80c0cd5ce01901a01709aab9e5001137540026ae854008c8c8c8cccd5cd19b8735573aa004900011991091980080180119a816bad35742a004605c6ae84d5d1280111931901819ab9c03203402e135573ca00226ea8004d5d09aba2500223263202c33573805c06005426aae7940044dd50009aba1500533501775c6ae854010ccd5406c07c8004d5d0a801999aa80dbae200135742a00460426ae84d5d1280111931901419ab9c02a02c026135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d55cf280089baa00135742a00860226ae84d5d1280211931900d19ab9c01c01e018375a00a6666ae68cdc39aab9d375400a9000100e11931900c19ab9c01a01c016101b132632017335738921035054350001b135573ca00226ea800448c88c008dd6000990009aa80d911999aab9f0012500a233500930043574200460066ae880080608c8c8cccd5cd19b8735573aa004900011991091980080180118061aba150023005357426ae8940088c98c8050cd5ce00b00c00909aab9e5001137540024646464646666ae68cdc39aab9d5004480008cccc888848cccc00401401000c008c8c8c8cccd5cd19b8735573aa0049000119910919800801801180a9aba1500233500f014357426ae8940088c98c8064cd5ce00d80e80b89aab9e5001137540026ae854010ccd54021d728039aba150033232323333573466e1d4005200423212223002004357426aae79400c8cccd5cd19b875002480088c84888c004010dd71aba135573ca00846666ae68cdc3a801a400042444006464c6403666ae7007407c06406005c4d55cea80089baa00135742a00466a016eb8d5d09aba2500223263201533573802e03202626ae8940044d5d1280089aab9e500113754002266aa002eb9d6889119118011bab00132001355018223233335573e0044a010466a00e66442466002006004600c6aae754008c014d55cf280118021aba200301613574200222440042442446600200800624464646666ae68cdc3a800a400046a02e600a6ae84d55cf280191999ab9a3370ea00490011280b91931900819ab9c01201400e00d135573aa00226ea80048c8c8cccd5cd19b875001480188c848888c010014c01cd5d09aab9e500323333573466e1d400920042321222230020053009357426aae7940108cccd5cd19b875003480088c848888c004014c01cd5d09aab9e500523333573466e1d40112000232122223003005375c6ae84d55cf280311931900819ab9c01201400e00d00c00b135573aa00226ea80048c8c8cccd5cd19b8735573aa004900011991091980080180118029aba15002375a6ae84d5d1280111931900619ab9c00e01000a135573ca00226ea80048c8cccd5cd19b8735573aa002900011bae357426aae7940088c98c8028cd5ce00600700409baa001232323232323333573466e1d4005200c21222222200323333573466e1d4009200a21222222200423333573466e1d400d2008233221222222233001009008375c6ae854014dd69aba135744a00a46666ae68cdc3a8022400c4664424444444660040120106eb8d5d0a8039bae357426ae89401c8cccd5cd19b875005480108cc8848888888cc018024020c030d5d0a8049bae357426ae8940248cccd5cd19b875006480088c848888888c01c020c034d5d09aab9e500b23333573466e1d401d2000232122222223005008300e357426aae7940308c98c804ccd5ce00a80b80880800780700680600589aab9d5004135573ca00626aae7940084d55cf280089baa0012323232323333573466e1d400520022333222122333001005004003375a6ae854010dd69aba15003375a6ae84d5d1280191999ab9a3370ea0049000119091180100198041aba135573ca00c464c6401866ae700380400280244d55cea80189aba25001135573ca00226ea80048c8c8cccd5cd19b875001480088c8488c00400cdd71aba135573ca00646666ae68cdc3a8012400046424460040066eb8d5d09aab9e500423263200933573801601a00e00c26aae7540044dd500089119191999ab9a3370ea00290021091100091999ab9a3370ea00490011190911180180218031aba135573ca00846666ae68cdc3a801a400042444004464c6401466ae7003003802001c0184d55cea80089baa0012323333573466e1d40052002200623333573466e1d40092000200623263200633573801001400800626aae74dd5000a4c244004244002921035054310012333333357480024a00c4a00c4a00c46a00e6eb400894018008480044488c0080049400848488c00800c4488004448c8c00400488cc00cc0080080041";
static const char* PLUTUS_V2_HASH   = "b3b7938690083d898380ce6482fcd9094a5268248cef3868507ac2bc";
static const char* PLUTUS_V2_CBOR   = "82025908955908920100003233223232323232332232323232323232323232332232323232322223232533532323232325335001101d13357389211e77726f6e67207573616765206f66207265666572656e636520696e7075740001c3232533500221533500221333573466e1c00800408007c407854cd4004840784078d40900114cd4c8d400488888888888802d40044c08526221533500115333533550222350012222002350022200115024213355023320015021001232153353235001222222222222300e00250052133550253200150233355025200100115026320013550272253350011502722135002225335333573466e3c00801c0940904d40b00044c01800c884c09526135001220023333573466e1cd55cea80224000466442466002006004646464646464646464646464646666ae68cdc39aab9d500c480008cccccccccccc88888888888848cccccccccccc00403403002c02802402001c01801401000c008cd405c060d5d0a80619a80b80c1aba1500b33501701935742a014666aa036eb94068d5d0a804999aa80dbae501a35742a01066a02e0446ae85401cccd5406c08dd69aba150063232323333573466e1cd55cea801240004664424660020060046464646666ae68cdc39aab9d5002480008cc8848cc00400c008cd40b5d69aba15002302e357426ae8940088c98c80c0cd5ce01901a01709aab9e5001137540026ae854008c8c8c8cccd5cd19b8735573aa004900011991091980080180119a816bad35742a004605c6ae84d5d1280111931901819ab9c03203402e135573ca00226ea8004d5d09aba2500223263202c33573805c06005426aae7940044dd50009aba1500533501775c6ae854010ccd5406c07c8004d5d0a801999aa80dbae200135742a00460426ae84d5d1280111931901419ab9c02a02c026135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d55cf280089baa00135742a00860226ae84d5d1280211931900d19ab9c01c01e018375a00a6666ae68cdc39aab9d375400a9000100e11931900c19ab9c01a01c016101b132632017335738921035054350001b135573ca00226ea800448c88c008dd6000990009aa80d911999aab9f0012500a233500930043574200460066ae880080608c8c8cccd5cd19b8735573aa004900011991091980080180118061aba150023005357426ae8940088c98c8050cd5ce00b00c00909aab9e5001137540024646464646666ae68cdc39aab9d5004480008cccc888848cccc00401401000c008c8c8c8cccd5cd19b8735573aa0049000119910919800801801180a9aba1500233500f014357426ae8940088c98c8064cd5ce00d80e80b89aab9e5001137540026ae854010ccd54021d728039aba150033232323333573466e1d4005200423212223002004357426aae79400c8cccd5cd19b875002480088c84888c004010dd71aba135573ca00846666ae68cdc3a801a400042444006464c6403666ae7007407c06406005c4d55cea80089baa00135742a00466a016eb8d5d09aba2500223263201533573802e03202626ae8940044d5d1280089aab9e500113754002266aa002eb9d6889119118011bab00132001355018223233335573e0044a010466a00e66442466002006004600c6aae754008c014d55cf280118021aba200301613574200222440042442446600200800624464646666ae68cdc3a800a400046a02e600a6ae84d55cf280191999ab9a3370ea00490011280b91931900819ab9c01201400e00d135573aa00226ea80048c8c8cccd5cd19b875001480188c848888c010014c01cd5d09aab9e500323333573466e1d400920042321222230020053009357426aae7940108cccd5cd19b875003480088c848888c004014c01cd5d09aab9e500523333573466e1d40112000232122223003005375c6ae84d55cf280311931900819ab9c01201400e00d00c00b135573aa00226ea80048c8c8cccd5cd19b8735573aa004900011991091980080180118029aba15002375a6ae84d5d1280111931900619ab9c00e01000a135573ca00226ea80048c8cccd5cd19b8735573aa002900011bae357426aae7940088c98c8028cd5ce00600700409baa001232323232323333573466e1d4005200c21222222200323333573466e1d4009200a21222222200423333573466e1d400d2008233221222222233001009008375c6ae854014dd69aba135744a00a46666ae68cdc3a8022400c4664424444444660040120106eb8d5d0a8039bae357426ae89401c8cccd5cd19b875005480108cc8848888888cc018024020c030d5d0a8049bae357426ae8940248cccd5cd19b875006480088c848888888c01c020c034d5d09aab9e500b23333573466e1d401d2000232122222223005008300e357426aae7940308c98c804ccd5ce00a80b80880800780700680600589aab9d5004135573ca00626aae7940084d55cf280089baa0012323232323333573466e1d400520022333222122333001005004003375a6ae854010dd69aba15003375a6ae84d5d1280191999ab9a3370ea0049000119091180100198041aba135573ca00c464c6401866ae700380400280244d55cea80189aba25001135573ca00226ea80048c8c8cccd5cd19b875001480088c8488c00400cdd71aba135573ca00646666ae68cdc3a8012400046424460040066eb8d5d09aab9e500423263200933573801601a00e00c26aae7540044dd500089119191999ab9a3370ea00290021091100091999ab9a3370ea00490011190911180180218031aba135573ca00846666ae68cdc3a801a400042444004464c6401466ae7003003802001c0184d55cea80089baa0012323333573466e1d40052002200623333573466e1d40092000200623263200633573801001400800626aae74dd5000a4c244004244002921035054310012333333357480024a00c4a00c4a00c46a00e6eb400894018008480044488c0080049400848488c00800c4488004448c8c00400488cc00cc0080080041";
static const char* PLUTUS_V3_SCRIPT = "5908920100003233223232323232332232323232323232323232332232323232322223232533532323232325335001101d13357389211e77726f6e67207573616765206f66207265666572656e636520696e7075740001c3232533500221533500221333573466e1c00800408007c407854cd4004840784078d40900114cd4c8d400488888888888802d40044c08526221533500115333533550222350012222002350022200115024213355023320015021001232153353235001222222222222300e00250052133550253200150233355025200100115026320013550272253350011502722135002225335333573466e3c00801c0940904d40b00044c01800c884c09526135001220023333573466e1cd55cea80224000466442466002006004646464646464646464646464646666ae68cdc39aab9d500c480008cccccccccccc88888888888848cccccccccccc00403403002c02802402001c01801401000c008cd405c060d5d0a80619a80b80c1aba1500b33501701935742a014666aa036eb94068d5d0a804999aa80dbae501a35742a01066a02e0446ae85401cccd5406c08dd69aba150063232323333573466e1cd55cea801240004664424660020060046464646666ae68cdc39aab9d5002480008cc8848cc00400c008cd40b5d69aba15002302e357426ae8940088c98c80c0cd5ce01901a01709aab9e5001137540026ae854008c8c8c8cccd5cd19b8735573aa004900011991091980080180119a816bad35742a004605c6ae84d5d1280111931901819ab9c03203402e135573ca00226ea8004d5d09aba2500223263202c33573805c06005426aae7940044dd50009aba1500533501775c6ae854010ccd5406c07c8004d5d0a801999aa80dbae200135742a00460426ae84d5d1280111931901419ab9c02a02c026135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d55cf280089baa00135742a00860226ae84d5d1280211931900d19ab9c01c01e018375a00a6666ae68cdc39aab9d375400a9000100e11931900c19ab9c01a01c016101b132632017335738921035054350001b135573ca00226ea800448c88c008dd6000990009aa80d911999aab9f0012500a233500930043574200460066ae880080608c8c8cccd5cd19b8735573aa004900011991091980080180118061aba150023005357426ae8940088c98c8050cd5ce00b00c00909aab9e5001137540024646464646666ae68cdc39aab9d5004480008cccc888848cccc00401401000c008c8c8c8cccd5cd19b8735573aa0049000119910919800801801180a9aba1500233500f014357426ae8940088c98c8064cd5ce00d80e80b89aab9e5001137540026ae854010ccd54021d728039aba150033232323333573466e1d4005200423212223002004357426aae79400c8cccd5cd19b875002480088c84888c004010dd71aba135573ca00846666ae68cdc3a801a400042444006464c6403666ae7007407c06406005c4d55cea80089baa00135742a00466a016eb8d5d09aba2500223263201533573802e03202626ae8940044d5d1280089aab9e500113754002266aa002eb9d6889119118011bab00132001355018223233335573e0044a010466a00e66442466002006004600c6aae754008c014d55cf280118021aba200301613574200222440042442446600200800624464646666ae68cdc3a800a400046a02e600a6ae84d55cf280191999ab9a3370ea00490011280b91931900819ab9c01201400e00d135573aa00226ea80048c8c8cccd5cd19b875001480188c848888c010014c01cd5d09aab9e500323333573466e1d400920042321222230020053009357426aae7940108cccd5cd19b875003480088c848888c004014c01cd5d09aab9e500523333573466e1d40112000232122223003005375c6ae84d55cf280311931900819ab9c01201400e00d00c00b135573aa00226ea80048c8c8cccd5cd19b8735573aa004900011991091980080180118029aba15002375a6ae84d5d1280111931900619ab9c00e01000a135573ca00226ea80048c8cccd5cd19b8735573aa002900011bae357426aae7940088c98c8028cd5ce00600700409baa001232323232323333573466e1d4005200c21222222200323333573466e1d4009200a21222222200423333573466e1d400d2008233221222222233001009008375c6ae854014dd69aba135744a00a46666ae68cdc3a8022400c4664424444444660040120106eb8d5d0a8039bae357426ae89401c8cccd5cd19b875005480108cc8848888888cc018024020c030d5d0a8049bae357426ae8940248cccd5cd19b875006480088c848888888c01c020c034d5d09aab9e500b23333573466e1d401d2000232122222223005008300e357426aae7940308c98c804ccd5ce00a80b80880800780700680600589aab9d5004135573ca00626aae7940084d55cf280089baa0012323232323333573466e1d400520022333222122333001005004003375a6ae854010dd69aba15003375a6ae84d5d1280191999ab9a3370ea0049000119091180100198041aba135573ca00c464c6401866ae700380400280244d55cea80189aba25001135573ca00226ea80048c8c8cccd5cd19b875001480088c8488c00400cdd71aba135573ca00646666ae68cdc3a8012400046424460040066eb8d5d09aab9e500423263200933573801601a00e00c26aae7540044dd500089119191999ab9a3370ea00290021091100091999ab9a3370ea00490011190911180180218031aba135573ca00846666ae68cdc3a801a400042444004464c6401466ae7003003802001c0184d55cea80089baa0012323333573466e1d40052002200623333573466e1d40092000200623263200633573801001400800626aae74dd5000a4c244004244002921035054310012333333357480024a00c4a00c4a00c46a00e6eb400894018008480044488c0080049400848488c00800c4488004448c8c00400488cc00cc0080080041";
static const char* PLUTUS_V3_HASH   = "16df94237e8e3abce4016304952b88720ec897b59a5b4b7ce4e1b6b4";
static const char* PLUTUS_V3_CBOR   = "82035908955908920100003233223232323232332232323232323232323232332232323232322223232533532323232325335001101d13357389211e77726f6e67207573616765206f66207265666572656e636520696e7075740001c3232533500221533500221333573466e1c00800408007c407854cd4004840784078d40900114cd4c8d400488888888888802d40044c08526221533500115333533550222350012222002350022200115024213355023320015021001232153353235001222222222222300e00250052133550253200150233355025200100115026320013550272253350011502722135002225335333573466e3c00801c0940904d40b00044c01800c884c09526135001220023333573466e1cd55cea80224000466442466002006004646464646464646464646464646666ae68cdc39aab9d500c480008cccccccccccc88888888888848cccccccccccc00403403002c02802402001c01801401000c008cd405c060d5d0a80619a80b80c1aba1500b33501701935742a014666aa036eb94068d5d0a804999aa80dbae501a35742a01066a02e0446ae85401cccd5406c08dd69aba150063232323333573466e1cd55cea801240004664424660020060046464646666ae68cdc39aab9d5002480008cc8848cc00400c008cd40b5d69aba15002302e357426ae8940088c98c80c0cd5ce01901a01709aab9e5001137540026ae854008c8c8c8cccd5cd19b8735573aa004900011991091980080180119a816bad35742a004605c6ae84d5d1280111931901819ab9c03203402e135573ca00226ea8004d5d09aba2500223263202c33573805c06005426aae7940044dd50009aba1500533501775c6ae854010ccd5406c07c8004d5d0a801999aa80dbae200135742a00460426ae84d5d1280111931901419ab9c02a02c026135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d55cf280089baa00135742a00860226ae84d5d1280211931900d19ab9c01c01e018375a00a6666ae68cdc39aab9d375400a9000100e11931900c19ab9c01a01c016101b132632017335738921035054350001b135573ca00226ea800448c88c008dd6000990009aa80d911999aab9f0012500a233500930043574200460066ae880080608c8c8cccd5cd19b8735573aa004900011991091980080180118061aba150023005357426ae8940088c98c8050cd5ce00b00c00909aab9e5001137540024646464646666ae68cdc39aab9d5004480008cccc888848cccc00401401000c008c8c8c8cccd5cd19b8735573aa0049000119910919800801801180a9aba1500233500f014357426ae8940088c98c8064cd5ce00d80e80b89aab9e5001137540026ae854010ccd54021d728039aba150033232323333573466e1d4005200423212223002004357426aae79400c8cccd5cd19b875002480088c84888c004010dd71aba135573ca00846666ae68cdc3a801a400042444006464c6403666ae7007407c06406005c4d55cea80089baa00135742a00466a016eb8d5d09aba2500223263201533573802e03202626ae8940044d5d1280089aab9e500113754002266aa002eb9d6889119118011bab00132001355018223233335573e0044a010466a00e66442466002006004600c6aae754008c014d55cf280118021aba200301613574200222440042442446600200800624464646666ae68cdc3a800a400046a02e600a6ae84d55cf280191999ab9a3370ea00490011280b91931900819ab9c01201400e00d135573aa00226ea80048c8c8cccd5cd19b875001480188c848888c010014c01cd5d09aab9e500323333573466e1d400920042321222230020053009357426aae7940108cccd5cd19b875003480088c848888c004014c01cd5d09aab9e500523333573466e1d40112000232122223003005375c6ae84d55cf280311931900819ab9c01201400e00d00c00b135573aa00226ea80048c8c8cccd5cd19b8735573aa004900011991091980080180118029aba15002375a6ae84d5d1280111931900619ab9c00e01000a135573ca00226ea80048c8cccd5cd19b8735573aa002900011bae357426aae7940088c98c8028cd5ce00600700409baa001232323232323333573466e1d4005200c21222222200323333573466e1d4009200a21222222200423333573466e1d400d2008233221222222233001009008375c6ae854014dd69aba135744a00a46666ae68cdc3a8022400c4664424444444660040120106eb8d5d0a8039bae357426ae89401c8cccd5cd19b875005480108cc8848888888cc018024020c030d5d0a8049bae357426ae8940248cccd5cd19b875006480088c848888888c01c020c034d5d09aab9e500b23333573466e1d401d2000232122222223005008300e357426aae7940308c98c804ccd5ce00a80b80880800780700680600589aab9d5004135573ca00626aae7940084d55cf280089baa0012323232323333573466e1d400520022333222122333001005004003375a6ae854010dd69aba15003375a6ae84d5d1280191999ab9a3370ea0049000119091180100198041aba135573ca00c464c6401866ae700380400280244d55cea80189aba25001135573ca00226ea80048c8c8cccd5cd19b875001480088c8488c00400cdd71aba135573ca00646666ae68cdc3a8012400046424460040066eb8d5d09aab9e500423263200933573801601a00e00c26aae7540044dd500089119191999ab9a3370ea00290021091100091999ab9a3370ea00490011190911180180218031aba135573ca00846666ae68cdc3a801a400042444004464c6401466ae7003003802001c0184d55cea80089baa0012323333573466e1d40052002200623333573466e1d40092000200623263200633573801001400800626aae74dd5000a4c244004244002921035054310012333333357480024a00c4a00c4a00c46a00e6eb400894018008480044488c0080049400848488c00800c4488004448c8c00400488cc00cc0080080041";

static auto
hexToBytes(const std::string& hex) -> std::vector<byte_t>
{
  std::vector<byte_t> bytes;

  for (unsigned int i = 0; i < hex.length(); i += 2)
  {
    std::string const byteString = hex.substr(i, 2);
    char              byte       = (char)strtol(byteString.c_str(), nullptr, 16);
    bytes.push_back(byte);
  }

  return bytes;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_script_new_native, canCreateNativeScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_script_t*        script        = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &native_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_native(native_script, &script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(script, nullptr);

  // Cleanup
  cardano_script_unref(&script);
  cardano_native_script_unref(&native_script);
}

TEST(cardano_script_new_native, returnsErrorIfNativeScriptIsNull)
{
  // Arrange
  cardano_script_t* script = NULL;

  // Act
  cardano_error_t result = cardano_script_new_native(NULL, &script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(script, nullptr);
}

TEST(cardano_script_new_native, returnsErrorIfScriptIsNull)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &native_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_native(native_script, NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_native_script_unref(&native_script);
}

TEST(cardano_script_new_native, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_script_t*        script        = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &native_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_script_new_native(native_script, &script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  ASSERT_EQ(script, nullptr);

  // Cleanup
  cardano_native_script_unref(&native_script);
}

TEST(cardano_script_new_plutus_v1, canCreatePlutusV1Script)
{
  // Arrange
  cardano_plutus_v1_script_t* plutus_script = NULL;
  cardano_script_t*           script        = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v1_script_new_bytes_from_hex(PLUTUS_V1_SCRIPT, strlen(PLUTUS_V1_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v1(plutus_script, &script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(script, nullptr);

  // Cleanup
  cardano_script_unref(&script);
  cardano_plutus_v1_script_unref(&plutus_script);
}

TEST(cardano_script_new_plutus_v1, returnsErrorIfPlutusV1ScriptIsNull)
{
  // Arrange
  cardano_script_t* script = NULL;

  // Act
  cardano_error_t result = cardano_script_new_plutus_v1(NULL, &script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(script, nullptr);
}

TEST(cardano_script_new_plutus_v1, returnsErrorIfScriptIsNull)
{
  // Arrange
  cardano_plutus_v1_script_t* plutus_script = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v1_script_new_bytes_from_hex(PLUTUS_V1_SCRIPT, strlen(PLUTUS_V1_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v1(plutus_script, NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_v1_script_unref(&plutus_script);
}

TEST(cardano_script_new_plutus_v1, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_v1_script_t* plutus_script = NULL;
  cardano_script_t*           script        = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v1_script_new_bytes_from_hex(PLUTUS_V1_SCRIPT, strlen(PLUTUS_V1_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_script_new_plutus_v1(plutus_script, &script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  ASSERT_EQ(script, nullptr);

  // Cleanup
  cardano_plutus_v1_script_unref(&plutus_script);
}

TEST(cardano_script_new_plutus_v2, canCreatePlutusV2Script)
{
  // Arrange
  cardano_plutus_v2_script_t* plutus_script = NULL;
  cardano_script_t*           script        = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v2_script_new_bytes_from_hex(PLUTUS_V2_SCRIPT, strlen(PLUTUS_V2_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v2(plutus_script, &script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(script, nullptr);

  // Cleanup
  cardano_script_unref(&script);
  cardano_plutus_v2_script_unref(&plutus_script);
}

TEST(cardano_script_new_plutus_v2, returnsErrorIfPlutusV2ScriptIsNull)
{
  // Arrange
  cardano_script_t* script = NULL;

  // Act
  cardano_error_t result = cardano_script_new_plutus_v2(NULL, &script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(script, nullptr);
}

TEST(cardano_script_new_plutus_v2, returnsErrorIfScriptIsNull)
{
  // Arrange
  cardano_plutus_v2_script_t* plutus_script = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v2_script_new_bytes_from_hex(PLUTUS_V2_SCRIPT, strlen(PLUTUS_V2_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v2(plutus_script, NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_v2_script_unref(&plutus_script);
}

TEST(cardano_script_new_plutus_v2, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_v2_script_t* plutus_script = NULL;
  cardano_script_t*           script        = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v2_script_new_bytes_from_hex(PLUTUS_V2_SCRIPT, strlen(PLUTUS_V2_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_script_new_plutus_v2(plutus_script, &script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  ASSERT_EQ(script, nullptr);

  // Cleanup
  cardano_plutus_v2_script_unref(&plutus_script);
}

TEST(cardano_script_new_plutus_v3, canCreatePlutusV3Script)
{
  // Arrange
  cardano_plutus_v3_script_t* plutus_script = NULL;
  cardano_script_t*           script        = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v3_script_new_bytes_from_hex(PLUTUS_V3_SCRIPT, strlen(PLUTUS_V3_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v3(plutus_script, &script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(script, nullptr);

  // Cleanup
  cardano_script_unref(&script);
  cardano_plutus_v3_script_unref(&plutus_script);
}

TEST(cardano_script_new_plutus_v3, returnsErrorIfPlutusV3ScriptIsNull)
{
  // Arrange
  cardano_script_t* script = NULL;

  // Act
  cardano_error_t result = cardano_script_new_plutus_v3(NULL, &script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(script, nullptr);
}

TEST(cardano_script_new_plutus_v3, returnsErrorIfScriptIsNull)
{
  // Arrange
  cardano_plutus_v3_script_t* plutus_script = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v3_script_new_bytes_from_hex(PLUTUS_V3_SCRIPT, strlen(PLUTUS_V3_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v3(plutus_script, NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_plutus_v3_script_unref(&plutus_script);
}

TEST(cardano_script_new_plutus_v3, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_v3_script_t* plutus_script = NULL;
  cardano_script_t*           script        = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v3_script_new_bytes_from_hex(PLUTUS_V3_SCRIPT, strlen(PLUTUS_V3_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_script_new_plutus_v3(plutus_script, &script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  ASSERT_EQ(script, nullptr);

  // Cleanup
  cardano_plutus_v3_script_unref(&plutus_script);
}

TEST(cardano_script_from_cbor, canCreateNativeScriptFromCBOR)
{
  // Arrange
  cardano_script_t*      script = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("82008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0", strlen("82008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0"));

  // Act
  cardano_error_t result = cardano_script_from_cbor(reader, &script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_script_get_hash(script);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "8b8370c97ae17eb69a8c97f733888f7485b60fd820c69211c8bbeb56");

  // Cleanup
  cardano_script_unref(&script);
  cardano_cbor_reader_unref(&reader);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_script_from_cbor, returnsErrorIfInvalidCBOR)
{
  // Arrange
  cardano_script_t*      script = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("82fe8202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0", strlen("82008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0"));

  // Act
  cardano_error_t result = cardano_script_from_cbor(reader, &script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_script_unref(&script);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_from_cbor, returnsErrorIfInvalidCBOR2)
{
  // Arrange
  cardano_script_t*      script = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("fefe8202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0", strlen("82008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0"));

  // Act
  cardano_error_t result = cardano_script_from_cbor(reader, &script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_script_unref(&script);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_from_cbor, canCreatePlutusV1ScriptFromCBOR)
{
  // Arrange
  cardano_script_t*      script = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(PLUTUS_V1_CBOR, strlen(PLUTUS_V1_CBOR));
  // Act
  cardano_error_t result = cardano_script_from_cbor(reader, &script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_script_get_hash(script);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, PLUTUS_V1_HASH);

  // Cleanup
  cardano_script_unref(&script);
  cardano_cbor_reader_unref(&reader);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_script_from_cbor, canCreatePlutusV2ScriptFromCBOR)
{
  // Arrange
  cardano_script_t*      script = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(PLUTUS_V2_CBOR, strlen(PLUTUS_V2_CBOR));

  // Act
  cardano_error_t result = cardano_script_from_cbor(reader, &script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_script_get_hash(script);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, PLUTUS_V2_HASH);

  // Cleanup
  cardano_script_unref(&script);
  cardano_cbor_reader_unref(&reader);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_script_from_cbor, canCreatePlutusV3ScriptFromCBOR)
{
  // Arrange
  cardano_script_t*      script = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(PLUTUS_V3_CBOR, strlen(PLUTUS_V3_CBOR));

  // Act
  cardano_error_t result = cardano_script_from_cbor(reader, &script);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(script, nullptr);

  cardano_blake2b_hash_t* hash = cardano_script_get_hash(script);
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, PLUTUS_V3_HASH);

  // Cleanup
  cardano_script_unref(&script);
  cardano_cbor_reader_unref(&reader);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_script_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_script_t* script = NULL;

  // Act
  cardano_error_t result = cardano_script_from_cbor(NULL, &script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(script, nullptr);
}

TEST(cardano_script_from_cbor, returnsErrorIfScriptIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("82008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0", strlen("82008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0"));

  // Act
  cardano_error_t result = cardano_script_from_cbor(reader, NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_to_cbor, canConvertNativeScriptToCBOR)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_script_t*        script        = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &native_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_native(native_script, &script);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  ASSERT_EQ(cardano_script_to_cbor(script, writer), CARDANO_SUCCESS);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(script, nullptr);

  // Cleanup
  cardano_script_unref(&script);
  cardano_native_script_unref(&native_script);
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_script_to_cbor, canConvertPlutusV1ScriptToCBOR)
{
  // Arrange
  cardano_plutus_v1_script_t* plutus_script = NULL;
  cardano_script_t*           script        = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v1_script_new_bytes_from_hex(PLUTUS_V1_SCRIPT, strlen(PLUTUS_V1_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v1(plutus_script, &script);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  ASSERT_EQ(cardano_script_to_cbor(script, writer), CARDANO_SUCCESS);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(script, nullptr);

  // Cleanup
  cardano_script_unref(&script);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_v1_script_unref(&plutus_script);
}

TEST(cardano_script_to_cbor, canConvertPlutusV2ScriptToCBOR)
{
  // Arrange
  cardano_plutus_v2_script_t* plutus_script = NULL;
  cardano_script_t*           script        = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v2_script_new_bytes_from_hex(PLUTUS_V2_SCRIPT, strlen(PLUTUS_V2_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v2(plutus_script, &script);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  ASSERT_EQ(cardano_script_to_cbor(script, writer), CARDANO_SUCCESS);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(script, nullptr);

  // Cleanup
  cardano_script_unref(&script);
  cardano_cbor_writer_unref(&writer);
  cardano_plutus_v2_script_unref(&plutus_script);
}

TEST(cardano_script_to_cbor, canConvertPlutusV3ScriptToCBOR)
{
  // Arrange
  cardano_plutus_v3_script_t* plutus_script = NULL;
  cardano_script_t*           script        = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v3_script_new_bytes_from_hex(PLUTUS_V3_SCRIPT, strlen(PLUTUS_V3_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v3(plutus_script, &script);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  ASSERT_EQ(cardano_script_to_cbor(script, writer), CARDANO_SUCCESS);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(script, nullptr);

  // Cleanup
  cardano_script_unref(&script);
  cardano_plutus_v3_script_unref(&plutus_script);
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_script_to_cbor, returnsErrorIfScriptIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_script_to_cbor(NULL, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_script_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_script_t*        script        = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &native_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_native(native_script, &script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_to_cbor(script, NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_script_unref(&script);
  cardano_native_script_unref(&native_script);
}

TEST(cardano_script_get_language, canGetNativeScriptLanguage)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_script_t*        script        = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &native_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_native(native_script, &script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_script_language_t language;

  result = cardano_script_get_language(script, &language);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_EQ(language, CARDANO_SCRIPT_LANGUAGE_NATIVE);

  // Cleanup
  cardano_script_unref(&script);
  cardano_native_script_unref(&native_script);
}

TEST(cardano_script_get_language, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_script_language_t language;

  // Act
  cardano_error_t result = cardano_script_get_language(NULL, &language);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_script_get_language, returnsErrorIfLanguageIsNull)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_script_t*        script        = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &native_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_native(native_script, &script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_get_language(script, NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_script_unref(&script);
  cardano_native_script_unref(&native_script);
}

TEST(cardano_script_to_native, canConvertScriptToNativeScript)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_script_t*        script        = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &native_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_native(native_script, &script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_native_script_t* native_script2 = NULL;

  result = cardano_script_to_native(script, &native_script2);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(native_script2, nullptr);

  // Cleanup
  cardano_native_script_unref(&native_script2);
  cardano_script_unref(&script);
  cardano_native_script_unref(&native_script);
}

TEST(cardano_script_to_native, returnsErrorIfScriptIsNull)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;

  // Act
  cardano_error_t result = cardano_script_to_native(NULL, &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(native_script, nullptr);
}

TEST(cardano_script_to_native, returnsErrorIfNativeScriptIsNull)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_script_t*        script        = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &native_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_native(native_script, &script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_to_native(script, NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_script_unref(&script);
  cardano_native_script_unref(&native_script);
}

TEST(cardano_script_to_native, returnsErrorIfNotNativeScript)
{
  // Arrange
  cardano_script_t*           script        = NULL;
  cardano_plutus_v1_script_t* plutus_script = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v1_script_new_bytes_from_hex(PLUTUS_V1_SCRIPT, strlen(PLUTUS_V1_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v1(plutus_script, &script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_native_script_t* native_script = NULL;

  result = cardano_script_to_native(script, &native_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE);
  ASSERT_EQ(native_script, nullptr);

  // Cleanup
  cardano_script_unref(&script);
  cardano_plutus_v1_script_unref(&plutus_script);
}

TEST(cardano_script_to_plutus_v1, canConvertScriptToPlutusV1Script)
{
  // Arrange
  cardano_plutus_v1_script_t* plutus_script = NULL;
  cardano_script_t*           script        = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v1_script_new_bytes_from_hex(PLUTUS_V1_SCRIPT, strlen(PLUTUS_V1_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v1(plutus_script, &script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_plutus_v1_script_t* plutus_script2 = NULL;

  result = cardano_script_to_plutus_v1(script, &plutus_script2);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(plutus_script2, nullptr);

  // Cleanup
  cardano_plutus_v1_script_unref(&plutus_script2);
  cardano_script_unref(&script);
  cardano_plutus_v1_script_unref(&plutus_script);
}

TEST(cardano_script_to_plutus_v1, returnsErrorIfScriptIsNull)
{
  // Arrange
  cardano_plutus_v1_script_t* plutus_script = NULL;

  // Act
  cardano_error_t result = cardano_script_to_plutus_v1(NULL, &plutus_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(plutus_script, nullptr);
}

TEST(cardano_script_to_plutus_v1, returnsErrorIfPlutusV1ScriptIsNull)
{
  // Arrange
  cardano_plutus_v1_script_t* plutus_script = NULL;
  cardano_script_t*           script        = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v1_script_new_bytes_from_hex(PLUTUS_V1_SCRIPT, strlen(PLUTUS_V1_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v1(plutus_script, &script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_to_plutus_v1(script, NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_script_unref(&script);
  cardano_plutus_v1_script_unref(&plutus_script);
}

TEST(cardano_script_to_plutus_v1, returnsErrorIfNotPlutusV1Script)
{
  // Arrange
  cardano_script_t*           script        = NULL;
  cardano_plutus_v2_script_t* plutus_script = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v2_script_new_bytes_from_hex(PLUTUS_V2_SCRIPT, strlen(PLUTUS_V2_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v2(plutus_script, &script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_plutus_v1_script_t* plutus_script2 = NULL;

  result = cardano_script_to_plutus_v1(script, &plutus_script2);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE);
  ASSERT_EQ(plutus_script2, nullptr);

  // Cleanup
  cardano_script_unref(&script);
  cardano_plutus_v2_script_unref(&plutus_script);
}

TEST(cardano_script_to_plutus_v2, canConvertScriptToPlutusV2Script)
{
  // Arrange
  cardano_plutus_v2_script_t* plutus_script = NULL;
  cardano_script_t*           script        = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v2_script_new_bytes_from_hex(PLUTUS_V2_SCRIPT, strlen(PLUTUS_V2_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v2(plutus_script, &script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_plutus_v2_script_t* plutus_script2 = NULL;

  result = cardano_script_to_plutus_v2(script, &plutus_script2);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(plutus_script2, nullptr);

  // Cleanup
  cardano_plutus_v2_script_unref(&plutus_script2);
  cardano_script_unref(&script);
  cardano_plutus_v2_script_unref(&plutus_script);
}

TEST(cardano_script_to_plutus_v2, returnsErrorIfScriptIsNull)
{
  // Arrange
  cardano_plutus_v2_script_t* plutus_script = NULL;

  // Act
  cardano_error_t result = cardano_script_to_plutus_v2(NULL, &plutus_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(plutus_script, nullptr);
}

TEST(cardano_script_to_plutus_v2, returnsErrorIfPlutusV2ScriptIsNull)
{
  // Arrange
  cardano_plutus_v2_script_t* plutus_script = NULL;
  cardano_script_t*           script        = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v2_script_new_bytes_from_hex(PLUTUS_V2_SCRIPT, strlen(PLUTUS_V2_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v2(plutus_script, &script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_to_plutus_v2(script, NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_script_unref(&script);
  cardano_plutus_v2_script_unref(&plutus_script);
}

TEST(cardano_script_to_plutus_v2, returnsErrorIfNotPlutusV2Script)
{
  // Arrange
  cardano_script_t*           script        = NULL;
  cardano_plutus_v1_script_t* plutus_script = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v1_script_new_bytes_from_hex(PLUTUS_V1_SCRIPT, strlen(PLUTUS_V1_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v1(plutus_script, &script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_plutus_v2_script_t* plutus_script2 = NULL;

  result = cardano_script_to_plutus_v2(script, &plutus_script2);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE);
  ASSERT_EQ(plutus_script2, nullptr);

  // Cleanup
  cardano_script_unref(&script);
  cardano_plutus_v1_script_unref(&plutus_script);
}

TEST(cardano_script_to_plutus_v3, canConvertScriptToPlutusV3Script)
{
  // Arrange
  cardano_plutus_v3_script_t* plutus_script = NULL;
  cardano_script_t*           script        = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v3_script_new_bytes_from_hex(PLUTUS_V3_SCRIPT, strlen(PLUTUS_V3_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v3(plutus_script, &script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_plutus_v3_script_t* plutus_script2 = NULL;

  result = cardano_script_to_plutus_v3(script, &plutus_script2);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(plutus_script2, nullptr);

  // Cleanup
  cardano_plutus_v3_script_unref(&plutus_script2);
  cardano_script_unref(&script);
  cardano_plutus_v3_script_unref(&plutus_script);
}

TEST(cardano_script_to_plutus_v3, returnsErrorIfScriptIsNull)
{
  // Arrange
  cardano_plutus_v3_script_t* plutus_script = NULL;

  // Act
  cardano_error_t result = cardano_script_to_plutus_v3(NULL, &plutus_script);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  ASSERT_EQ(plutus_script, nullptr);
}

TEST(cardano_script_to_plutus_v3, returnsErrorIfPlutusV3ScriptIsNull)
{
  // Arrange
  cardano_plutus_v3_script_t* plutus_script = NULL;
  cardano_script_t*           script        = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v3_script_new_bytes_from_hex(PLUTUS_V3_SCRIPT, strlen(PLUTUS_V3_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v3(plutus_script, &script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_to_plutus_v3(script, NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_script_unref(&script);
  cardano_plutus_v3_script_unref(&plutus_script);
}

TEST(cardano_script_to_plutus_v3, returnsErrorIfNotPlutusV3Script)
{
  // Arrange
  cardano_script_t*           script        = NULL;
  cardano_plutus_v1_script_t* plutus_script = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v1_script_new_bytes_from_hex(PLUTUS_V1_SCRIPT, strlen(PLUTUS_V1_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v1(plutus_script, &script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_plutus_v3_script_t* plutus_script2 = NULL;

  result = cardano_script_to_plutus_v3(script, &plutus_script2);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE);
  ASSERT_EQ(plutus_script2, nullptr);

  // Cleanup
  cardano_script_unref(&script);
  cardano_plutus_v1_script_unref(&plutus_script);
}

TEST(cardano_script_get_hash, canGetScriptHash)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_script_t*        script        = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &native_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_native(native_script, &script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_blake2b_hash_t* hash = cardano_script_get_hash(script);

  // Assert
  ASSERT_NE(hash, nullptr);

  const size_t hex_size = cardano_blake2b_hash_get_hex_size(hash);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_blake2b_hash_to_hex(hash, hex, hex_size), CARDANO_SUCCESS);
  EXPECT_STREQ(hex, "44e8537337e941f125478607b7ab91515b5eca4ef647b10c16c63ed2");

  // Cleanup
  cardano_script_unref(&script);
  cardano_native_script_unref(&native_script);
  cardano_blake2b_hash_unref(&hash);
  free(hex);
}

TEST(cardano_script_get_hash, returnsErrorIfScriptIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* hash = cardano_script_get_hash(NULL);

  // Assert
  ASSERT_EQ(hash, nullptr);
}

TEST(cardano_script_equals, canCompareTwoScripts)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_script_t*        script1       = NULL;
  cardano_script_t*        script2       = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &native_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_native(native_script, &script1);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_native(native_script, &script2);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  bool equals = cardano_script_equals(script1, script2);

  // Assert
  ASSERT_TRUE(equals);

  // Cleanup
  cardano_script_unref(&script1);
  cardano_script_unref(&script2);
  cardano_native_script_unref(&native_script);
}

TEST(cardano_script_equals, returnsFalseIfScriptsAreDifferent)
{
  // Arrange
  cardano_native_script_t* native_script  = NULL;
  cardano_native_script_t* native_script2 = NULL;
  cardano_script_t*        script1        = NULL;
  cardano_script_t*        script2        = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &native_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_native(native_script, &script1);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_native_script_from_json(PUBKEY_SCRIPT2, strlen(PUBKEY_SCRIPT2), &native_script2);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_native(native_script2, &script2);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  bool equals = cardano_script_equals(script1, script2);

  // Assert
  ASSERT_FALSE(equals);

  // Cleanup
  cardano_script_unref(&script1);
  cardano_script_unref(&script2);
  cardano_native_script_unref(&native_script);
  cardano_native_script_unref(&native_script2);
}

TEST(cardano_script_equals, returnsFalseIfOneScriptIsNull)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_script_t*        script1       = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &native_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_native(native_script, &script1);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  bool equals = cardano_script_equals(script1, NULL);

  // Assert
  ASSERT_FALSE(equals);

  // Cleanup
  cardano_script_unref(&script1);
  cardano_native_script_unref(&native_script);
}

TEST(cardano_script_equals, returnsTrueIfSamePlutusScript)
{
  // Arrange
  cardano_plutus_v1_script_t* plutus_script = NULL;
  cardano_script_t*           script1       = NULL;
  cardano_script_t*           script2       = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v1_script_new_bytes_from_hex(PLUTUS_V1_SCRIPT, strlen(PLUTUS_V1_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v1(plutus_script, &script1);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v1(plutus_script, &script2);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  bool equals = cardano_script_equals(script1, script2);

  // Assert
  ASSERT_TRUE(equals);

  // Cleanup
  cardano_script_unref(&script1);
  cardano_script_unref(&script2);
  cardano_plutus_v1_script_unref(&plutus_script);
}

TEST(cardano_script_equals, returnsTrueIfSamePlutusV2Script)
{
  // Arrange
  cardano_plutus_v2_script_t* plutus_script = NULL;
  cardano_script_t*           script1       = NULL;
  cardano_script_t*           script2       = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v2_script_new_bytes_from_hex(PLUTUS_V2_SCRIPT, strlen(PLUTUS_V2_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v2(plutus_script, &script1);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v2(plutus_script, &script2);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  bool equals = cardano_script_equals(script1, script2);

  // Assert
  ASSERT_TRUE(equals);

  // Cleanup
  cardano_script_unref(&script1);
  cardano_script_unref(&script2);
  cardano_plutus_v2_script_unref(&plutus_script);
}

TEST(cardano_script_equals, returnsTrueIfSamePlutusV3Script)
{
  // Arrange
  cardano_plutus_v3_script_t* plutus_script = NULL;
  cardano_script_t*           script1       = NULL;
  cardano_script_t*           script2       = NULL;

  // Act
  cardano_error_t result = cardano_plutus_v3_script_new_bytes_from_hex(PLUTUS_V3_SCRIPT, strlen(PLUTUS_V3_SCRIPT), &plutus_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v3(plutus_script, &script1);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v3(plutus_script, &script2);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  bool equals = cardano_script_equals(script1, script2);

  // Assert
  ASSERT_TRUE(equals);

  // Cleanup
  cardano_script_unref(&script1);
  cardano_script_unref(&script2);
  cardano_plutus_v3_script_unref(&plutus_script);
}

TEST(cardano_script_equals, returnsTrueIfBothAreNull)
{
  // Arrange
  cardano_script_t* script1 = NULL;
  cardano_script_t* script2 = NULL;

  // Act
  bool equals = cardano_script_equals(script1, script2);

  // Assert
  ASSERT_TRUE(equals);
}

TEST(cardano_script_equals, returnsFalseIfDifferentType)
{
  // Arrange
  cardano_native_script_t* native_script  = NULL;
  cardano_native_script_t* native_script2 = NULL;
  cardano_script_t*        script1        = NULL;
  cardano_script_t*        script2        = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &native_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_native(native_script, &script1);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_plutus_v1_script_new_bytes_from_hex(PLUTUS_V1_SCRIPT, strlen(PLUTUS_V1_SCRIPT), (cardano_plutus_v1_script_t**)&native_script2);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_plutus_v1((cardano_plutus_v1_script_t*)native_script2, &script2);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  bool equals = cardano_script_equals(script1, script2);

  // Assert
  ASSERT_FALSE(equals);

  // Cleanup
  cardano_script_unref(&script1);
  cardano_script_unref(&script2);
  cardano_native_script_unref(&native_script);
  cardano_native_script_unref(&native_script2);
}

TEST(cardano_script_equals, returnsFalseIfLhsIsNull)
{
  // Arrange
  cardano_native_script_t* native_script = NULL;
  cardano_script_t*        script        = NULL;

  // Act
  cardano_error_t result = cardano_native_script_from_json(PUBKEY_SCRIPT, strlen(PUBKEY_SCRIPT), &native_script);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  result = cardano_script_new_native(native_script, &script);

  bool equals = cardano_script_equals(NULL, script);

  // Assert
  ASSERT_FALSE(equals);

  // Cleanup
  cardano_script_unref(&script);
  cardano_native_script_unref(&native_script);
}

TEST(cardano_script_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_script_t*      script = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("82008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0", strlen("82008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0"));
  cardano_error_t        error  = cardano_script_from_cbor(reader, &script);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_script_ref(script);

  // Assert
  EXPECT_THAT(script, testing::Not((cardano_script_t*)nullptr));
  EXPECT_EQ(cardano_script_refcount(script), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_script_unref(&script);
  cardano_script_unref(&script);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_script_ref(nullptr);
}

TEST(cardano_script_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_script_t* script = nullptr;

  // Act
  cardano_script_unref(&script);
}

TEST(cardano_script_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_script_unref((cardano_script_t**)nullptr);
}

TEST(cardano_script_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_script_t*      script = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("82008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0", strlen("82008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0"));
  cardano_error_t        error  = cardano_script_from_cbor(reader, &script);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_script_ref(script);
  size_t ref_count = cardano_script_refcount(script);

  cardano_script_unref(&script);
  size_t updated_ref_count = cardano_script_refcount(script);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_script_unref(&script);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_script_t*      script = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("82008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0", strlen("82008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0"));
  cardano_error_t        error  = cardano_script_from_cbor(reader, &script);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_script_ref(script);
  size_t ref_count = cardano_script_refcount(script);

  cardano_script_unref(&script);
  size_t updated_ref_count = cardano_script_refcount(script);

  cardano_script_unref(&script);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(script, (cardano_script_t*)nullptr);

  // Cleanup
  cardano_script_unref(&script);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_script_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_script_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_script_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_script_t* script  = nullptr;
  const char*       message = "This is a test message";

  // Act
  cardano_script_set_last_error(script, message);

  // Assert
  EXPECT_STREQ(cardano_script_get_last_error(script), "Object is NULL.");
}

TEST(cardano_script_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_script_t*      script = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("82008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0", strlen("82008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0"));
  cardano_error_t        error  = cardano_script_from_cbor(reader, &script);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_script_set_last_error(script, message);

  // Assert
  EXPECT_STREQ(cardano_script_get_last_error(script), "");

  // Cleanup
  cardano_script_unref(&script);
  cardano_cbor_reader_unref(&reader);
}