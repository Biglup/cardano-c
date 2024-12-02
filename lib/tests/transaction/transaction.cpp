/**
 * \file transaction.cpp
 *
 * \author angel.castillo
 * \date   Sep 24, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* INCLUDES ******************************************************************/

#include <cardano/error.h>

#include <cardano/auxiliary_data/auxiliary_data.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/transaction/transaction.h>
#include <cardano/transaction_body/transaction_body.h>
#include <cardano/witness_set/witness_set.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                       = "84af00d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5000181a2005839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc01820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e020a031903e804d90102828304581c26b17b78de4f035dc0bfce60d1d3c3a8085c38dcce5fb8767e518bed1901f48405581c0d94e174732ef9aae73f395ab44507bfa983d65023c11a951f0c32e4581ca646474b8f5431261506b6c273d307c7569a4eb6c96b42dd4a29520a582003170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c11131405a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0050758202ceb364d93225b4a0f004a0975a13eb50c3cc6348474b4fe9121f8dc72ca0cfa08186409a3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c413831581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e0b58206199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d38abc123de0dd90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5010ed9010281581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d3910a2005839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc01820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e11186412d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d500a700d90102818258206199186adb51974690d7247d2646097d2c62763b767b528816fb7ed3f9f55d395840bdea87fca1b4b4df8a9b8fb4183c0fab2f8261eb6c5e4bc42c800bb9c8918755bdea87fca1b4b4df8a9b8fb4183c0fab2f8261eb6c5e4bc42c800bb9c891875501d90102868205186482041901f48200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f548201818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f548202818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f54830301818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f5402d9010281845820deeb8f82f2af5836ebbc1b450b6dbf0b03c93afe5696f10d49e8a8304ebfac01584064676273786767746f6768646a7074657476746b636f6376796669647171676775726a687268716169697370717275656c6876797071786565777072796676775820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b45041a003d90102815820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b45004d9010281187b05a282010082d87a9f187bff82190bb8191b5882020182d87a9f187bff821913881907d006d90102815820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b450f5d90103a100a6011904d20263737472039f1904d263737472ff0445627974657305a2667374726b6579187b9f676c6973746b6579ff6873747276616c75650626";
static const char* CBOR2                      = "84a600d9010281825820260aed6e7a24044b1254a87a509468a649f522a4e54e830ac10f27ea7b5ec61f010183a300581d70b429738bd6cc58b5c7932d001aa2bd05cfea47020a556c8c753d4436011a004c4b40028200582007845f8f3841996e3d8157954e2f5e2fb90465f27112fc5fe9056d916fae245ba200583900b1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c68042f1946335c498d2e7556c5c647c4649c6a69d2b645cd1428a339ba011a04636769a200583900b1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c68042f1946335c498d2e7556c5c647c4649c6a69d2b645cd1428a339ba01821a00177a6ea2581c648823ffdad1610b4162f4dbc87bd47f6f9cf45d772ddef661eff198a5447742544319271044774554481a0031f9194577444f47451a0056898d4577555344431a000fc589467753484942411a000103c2581c659ab0b5658687c2e74cd10dba8244015b713bf503b90557769d77a7a14a57696e675269646572731a02269552021a0002e665031a01353f84081a013531740b58204107eada931c72a600a6e3305bd22c7aeb9ada7c3f6823b155f4db85de36a69aa200d9010281825820e686ade5bc97372f271fd2abc06cfd96c24b3d9170f9459de1d8e3dd8fd385575840653324a9dddad004f05a8ac99fa2d1811af5f00543591407fb5206cfe9ac91bb1412404323fa517e0e189684cd3592e7f74862e3f16afbc262519abec958180c04d9010281d8799fd8799fd8799fd8799f581cb1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c68ffd8799fd8799fd8799f581c042f1946335c498d2e7556c5c647c4649c6a69d2b645cd1428a339baffffffff581cb1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c681b000001863784a12ed8799fd8799f4040ffd8799f581c648823ffdad1610b4162f4dbc87bd47f6f9cf45d772ddef661eff1984577444f4745ffffffd8799fd87980190c8efffff5f6";
static const char* CBOR3                      = "84a40081825820f6dd880fb30480aa43117c73bfd09442ba30de5644c3ec1a91d9232fbe715aab000182a20058390071213dc119131f48f54d62e339053388d9d84faedecba9d8722ad2cad9debf34071615fc6452dfc743a4963f6bec68e488001c7384942c13011b0000000253c8e4f6a300581d702ed2631dbb277c84334453c5c437b86325d371f0835a28b910a91a6e011a001e848002820058209d7fee57d1dbb9b000b2a133256af0f2c83ffe638df523b2d1c13d405356d8ae021a0002fb050b582088e4779d217d10398a705530f9fb2af53ffac20aef6e75e85c26e93a00877556a10481d8799fd8799f40ffd8799fa1d8799fd8799fd87980d8799fd8799f581c71213dc119131f48f54d62e339053388d9d84faedecba9d8722ad2caffd8799fd8799fd8799f581cd9debf34071615fc6452dfc743a4963f6bec68e488001c7384942c13ffffffffffd8799f4040ffff1a001e8480a0a000ffd87c9f9fd8799fd8799fd8799fd87980d8799fd8799f581caa47de0ab3b7f0b1d8d196406b6af1b0d88cd46168c49ca0557b4f70ffd8799fd8799fd8799f581cd4b8fc88aec1d1c2f43ca5587898d88da20ef73964b8cf6f8f08ddfbffffffffffd8799fd87980d8799fd8799f581caa47de0ab3b7f0b1d8d196406b6af1b0d88cd46168c49ca0557b4f70ffd8799fd8799fd8799f581cd4b8fc88aec1d1c2f43ca5587898d88da20ef73964b8cf6f8f08ddfbffffffffffd8799f4040ffd87a9f1a00989680ffffd87c9f9fd8799fd87a9fd8799f4752656c65617365d8799fd87980d8799fd8799f581caa47de0ab3b7f0b1d8d196406b6af1b0d88cd46168c49ca0557b4f70ffd8799fd8799fd8799f581cd4b8fc88aec1d1c2f43ca5587898d88da20ef73964b8cf6f8f08ddfbffffffffffff9fd8799f0101ffffffd87c9f9fd8799fd87b9fd9050280ffd87980ffff1b000001884e1fb1c0d87980ffffff1b000001884e1fb1c0d87980ffffff1b000001884e1fb1c0d87980fffff5f6";
static const char* CBOR_NULLIFY_ENTROPY       = "83a50081825820bf30608a974d09c56dd62ca10199ec11746ea2d90dbd83649d4f37c629b1ba840001818258390117d237fb8f952c995cd28f73c555adc2307322d819b7f565196ce754348144bff68f23c1386b85dea0f8425ca574b1a11e188ffaba67537c1a0048f96f021a000351d1031a019732f30682a7581c162f94554ac8c225383a2248c245659eda870eaa82d0ef25fc7dcd82a10d8100581c2075a095b3c844a29c24317a94a643ab8e22d54a3a3a72a420260af6a10d8100581c268cfc0b89e910ead22e0ade91493d8212f53f3e2164b2e4bef0819ba10d8100581c60baee25cbc90047e83fd01e1e57dc0b06d3d0cb150d0ab40bbfead1a10d8100581cad5463153dc3d24b9ff133e46136028bdc1edbb897f5a7cf1b37950ca10d8100581cb9547b8a57656539a8d9bc42c008e38d9c8bd9c8adbb1e73ad529497a10d8100581cf7b341c14cd58fca4195a9b278cce1ef402dc0e06deb77e543cd1757a10d8100190103a1008882582061261a95b7613ee6bf2067dad77b70349729b0c50d57bc1cf30de0db4a1e73a858407d72721e7504e12d50204f7d9e9d9fe60d9c6a4fd18ad629604729df4f7f3867199b62885623fab68a02863e7877955ca4a56c867157a559722b7b350b668a0b8258209180d818e69cd997e34663c418a648c076f2e19cd4194e486e159d8580bc6cda5840af668e57c98f0c3d9b47c66eb9271213c39b4ea1b4d543b0892f03985edcef4216d1f98f7b731eedc260a2154124b5cab015bfeaf694d58966d124ad2ff60f0382582089c29f8c4af27b7accbe589747820134ebbaa1caf3ce949270a3d0c7dcfd541b58401ad69342385ba6c3bef937a79456d7280c0d539128072db15db120b1579c46ba95d18c1fa073d7dbffb4d975b1e02ebb7372936940cff0a96fce950616d2f504825820f14f712dc600d793052d4842d50cefa4e65884ea6cf83707079eb8ce302efc855840638f7410929e7eab565b1451effdfbeea2a8839f7cfcc4c4483c4931d489547a2e94b73e4b15f8494de7f42ea31e573c459a9a7e5269af17b0978e70567de80e8258208b53207629f9a30e4b2015044f337c01735abe67243c19470c9dae8c7b73279858400c4ed03254c33a19256b7a3859079a9b75215cad83871a9b74eb51d8bcab52911c37ea5c43bdd212d006d1e6670220ff1d03714addf94f490e482edacbb08f068258205fddeedade2714d6db2f9e1104743d2d8d818ecddc306e176108db14caadd4415840bf48f5dd577b5cb920bfe60e13c8b1b889366c23e2f2e28d51814ed23def3a0ff4a1964f806829d40180d83b5230728409c1f18ddb5a61c44e614b823bd43f01825820cbc6b506e94fbefe442eecee376f3b3ebaf89415ef5cd2efb666e06ddae48393584089bff8f81a20b22f2c3f8a2288b15f1798b51f3363e0437a46c0a2e4e283b7c1018eba0b2b192d6d522ac8df2f2e95b4c8941b387cda89857ab0ae77db14780c825820e8c03a03c0b2ddbea4195caf39f41e669f7d251ecf221fbb2f275c0a5d7e05d158402643ac53dd4da4f6e80fb192b2bf7d1dd9a333bbacea8f07531ba450dd8fb93e481589d370a6ef33a97e03b2f5816e4b2c6a8abf606a859108ba6f416e530d07f6";
static const char* TX_BODY_CBOR               = "b100818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5000181825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e020a031903e804828304581c26b17b78de4f035dc0bfce60d1d3c3a8085c38dcce5fb8767e518bed1901f48405581c0d94e174732ef9aae73f395ab44507bfa983d65023c11a951f0c32e4581ca646474b8f5431261506b6c273d307c7569a4eb6c96b42dd4a29520a582003170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c11131405a2581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d005581de0404b5a4088ae9abcf486a7e7b8f82069e6fcfe1bf226f1851ce72570030682a3581c00000000000000000000000000000000000000000000000000000001b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a10098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000002b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a10098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba581c00000000000000000000000000000000000000000000000000000003b60018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d8201582000000000000000000000000000000000000000000000000000000000000000000e820103101903e8111988b812a10098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba19020b0758202ceb364d93225b4a0f004a0975a13eb50c3cc6348474b4fe9121f8dc72ca0cfa08186409a3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c413831581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e0b58206199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d38abc123de0d818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5010e81581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d390f0110825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e11186412818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d500";
static const char* AUXILIARY_DATA_CBOR        = "d90103a500a11902d5a4187b1904d2636b65796576616c7565646b65793246000102030405a1190237656569676874a119029a6463616b6501848204038205098202818200581c3542acb3a64d80c29302260d62c3b87a742ad14abf855ebc6733081e830300818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f5402844746010000220010474601000022001147460100002200124746010000220013038447460100002200104746010000220011474601000022001247460100002200130483474601000022001047460100002200114746010000220012";
static const char* WITNESS_SET_CBOR           = "a100838258204a352f53eb4311d552aa9e1c6f0125846a3b607011d691f0e774d893d940b8525840c4f13cc397a50193061ce899b3eda906ad1adf3f3d515b52248ea5aa142781cd9c2ccc52ac62b2e1b5226de890104ec530bda4c38a19b691946da9addb3213f5825820290c08454c58a8c7fad6351e65a652460bd4f80f485f1ccfc350ff6a4d5bd4de5840026f47bab2f24da9690746bdb0e55d53a5eef45a969e3dd2873a3e6bb8ef3316d9f80489bacfd2f543108e284a40847ae7ce33fa358fcfe439a37990ad3107e98258204d953d6a9d556da3f3e26622c725923130f5733d1a3c4013ef8c34d15a070fd75840f9218e5a569c5ace38b1bb81e1f1c0b2d7fea2fe7fb913fdd06d79906436103345347a81494b83f83bf43466b0cebdbbdcef15384f67c255e826c249336ce2c7";
static const char* CBOR_TX_ID                 = "c7f20e9550b5631f07622a583a5103f19bcfa28eee89f39fff0eb24c2ad74619";
static const char* CBOR3_TX_ID                = "2d7f290c815e061fb7c27e91d2a898bd7b454a71c9b7a26660e2257ac31ebe32";
static const char* CBOR_NULLIFY_ENTROPY_TX_ID = "fc863a441b55acceebb7d25c81ff7259e4fc9b92fbdf6d594118fb8f1110a78c";
static const char* VKEY_WITNESS_CBOR          = "d90102848258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a8258203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the transaction.
 * @return A new instance of the transaction.
 */
static cardano_transaction_t*
new_default_transaction(const char* cbor)
{
  cardano_transaction_t* transaction = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result      = cardano_transaction_from_cbor(reader, &transaction);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return transaction;
};

/**
 * Creates a new default instance of the transaction_body.
 * @return A new instance of the transaction_body.
 */
static cardano_transaction_body_t*
new_default_transaction_body(const char* cbor)
{
  cardano_transaction_body_t* transaction_body = NULL;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t             result           = cardano_transaction_body_from_cbor(reader, &transaction_body);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return transaction_body;
};

/**
 * Creates a new default instance of the auxiliary_data.
 * @return A new instance of the auxiliary_data.
 */
static cardano_auxiliary_data_t*
new_default_auxiliary_data(const char* cbor)
{
  cardano_auxiliary_data_t* auxiliary_data = NULL;
  cardano_cbor_reader_t*    reader         = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t           result         = cardano_auxiliary_data_from_cbor(reader, &auxiliary_data);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return auxiliary_data;
};

/**
 * Creates a new default instance of the witness_set.
 * @return A new instance of the witness_set.
 */
static cardano_witness_set_t*
new_default_witness_set(const char* cbor)
{
  cardano_witness_set_t* witness_set = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result      = cardano_witness_set_from_cbor(reader, &witness_set);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return witness_set;
};

/**
 * Creates a new default instance of the vkey_witness_set.
 *
 * @return A new instance of the vkey_witness_set.
 */
static cardano_vkey_witness_set_t*
new_default_vkey_witness_set(const char* cbor)
{
  cardano_vkey_witness_set_t* vkey_witness_set = NULL;
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t             result           = cardano_vkey_witness_set_from_cbor(reader, &vkey_witness_set);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return vkey_witness_set;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_transaction_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_transaction_t* transaction = new_default_transaction(CBOR);
  EXPECT_NE(transaction, nullptr);

  // Act
  cardano_transaction_ref(transaction);

  // Assert
  EXPECT_THAT(transaction, testing::Not((cardano_transaction_t*)nullptr));
  EXPECT_EQ(cardano_transaction_refcount(transaction), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_transaction_unref(&transaction);
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_transaction_ref(nullptr);
}

TEST(cardano_transaction_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_transaction_t* transaction = nullptr;

  // Act
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_transaction_unref((cardano_transaction_t**)nullptr);
}

TEST(cardano_transaction_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_transaction_t* transaction = new_default_transaction(CBOR);
  EXPECT_NE(transaction, nullptr);

  // Act
  cardano_transaction_ref(transaction);
  size_t ref_count = cardano_transaction_refcount(transaction);

  cardano_transaction_unref(&transaction);
  size_t updated_ref_count = cardano_transaction_refcount(transaction);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_transaction_t* transaction = new_default_transaction(CBOR);
  EXPECT_NE(transaction, nullptr);

  // Act
  cardano_transaction_ref(transaction);
  size_t ref_count = cardano_transaction_refcount(transaction);

  cardano_transaction_unref(&transaction);
  size_t updated_ref_count = cardano_transaction_refcount(transaction);

  cardano_transaction_unref(&transaction);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(transaction, (cardano_transaction_t*)nullptr);

  // Cleanup
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_transaction_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_transaction_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_transaction_t* transaction = nullptr;
  const char*            message     = "This is a test message";

  // Act
  cardano_transaction_set_last_error(transaction, message);

  // Assert
  EXPECT_STREQ(cardano_transaction_get_last_error(transaction), "Object is NULL.");
}

TEST(cardano_transaction_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_transaction_t* transaction = new_default_transaction(CBOR);
  EXPECT_NE(transaction, nullptr);

  const char* message = nullptr;

  // Act
  cardano_transaction_set_last_error(transaction, message);

  // Assert
  EXPECT_STREQ(cardano_transaction_get_last_error(transaction), "");

  // Cleanup
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_transaction_t* transaction = NULL;

  // Act
  cardano_error_t result = cardano_transaction_from_cbor(nullptr, &transaction);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_from_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_transaction_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_to_cbor, canSerializeFromCache)
{
  // Arrange
  cardano_cbor_writer_t* writer      = cardano_cbor_writer_new();
  cardano_transaction_t* transaction = new_default_transaction(CBOR);
  EXPECT_NE(transaction, nullptr);

  // Act
  cardano_error_t result = cardano_transaction_to_cbor(transaction, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_transaction_unref(&transaction);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_transaction_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t* writer      = cardano_cbor_writer_new();
  cardano_transaction_t* transaction = new_default_transaction(CBOR);
  EXPECT_NE(transaction, nullptr);

  // Act
  cardano_transaction_clear_cbor_cache(transaction);
  cardano_error_t result = cardano_transaction_to_cbor(transaction, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_transaction_unref(&transaction);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_transaction_to_cbor, canSerialize2)
{
  // Arrange
  cardano_cbor_writer_t* writer      = cardano_cbor_writer_new();
  cardano_transaction_t* transaction = new_default_transaction(CBOR2);
  EXPECT_NE(transaction, nullptr);

  // Act
  cardano_transaction_clear_cbor_cache(transaction);
  cardano_error_t result = cardano_transaction_to_cbor(transaction, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR2);

  // Cleanup
  cardano_transaction_unref(&transaction);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_transaction_to_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_transaction_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_transaction_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_transaction_to_cbor((cardano_transaction_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

// Cert specific tests

TEST(cardano_transaction_new, canCreateNewInstance)
{
  // Act
  cardano_transaction_body_t* transaction_body = new_default_transaction_body(TX_BODY_CBOR);
  cardano_auxiliary_data_t*   auxiliary_data   = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);
  cardano_witness_set_t*      witness_set      = new_default_witness_set(WITNESS_SET_CBOR);

  cardano_transaction_t* transaction = NULL;

  cardano_error_t result = cardano_transaction_new(transaction_body, witness_set, auxiliary_data, &transaction);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(transaction, nullptr);

  // Cleanup
  cardano_transaction_unref(&transaction);
  cardano_transaction_body_unref(&transaction_body);
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_witness_set_unref(&witness_set);
}

TEST(cardano_transaction_new, returnsErrorIfFirstArgIsNull)
{
  // Act
  cardano_transaction_t* transaction = NULL;

  cardano_error_t result = cardano_transaction_new(nullptr, nullptr, nullptr, &transaction);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_new, returnsErrorIfSecondArgIsNull)
{
  // Act
  cardano_transaction_t* transaction = NULL;

  cardano_error_t result = cardano_transaction_new((cardano_transaction_body_t*)"", nullptr, nullptr, &transaction);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_new, returnsErrorIfCertIsNull)
{
  // Act

  cardano_error_t result = cardano_transaction_new((cardano_transaction_body_t*)"", (cardano_witness_set_t*)"", (cardano_auxiliary_data_t*)"", nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
}

TEST(cardano_transaction_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_transaction_body_t* transaction_body = new_default_transaction_body(TX_BODY_CBOR);
  cardano_auxiliary_data_t*   auxiliary_data   = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);
  cardano_witness_set_t*      witness_set      = new_default_witness_set(WITNESS_SET_CBOR);

  // Act
  cardano_transaction_t* transaction = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_transaction_new(transaction_body, witness_set, auxiliary_data, &transaction);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_transaction_body_unref(&transaction_body);
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_witness_set_unref(&witness_set);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_transaction_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_transaction_t* transaction = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_transaction_from_cbor(reader, &transaction);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_transaction_unref(&transaction);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_from_cbor, returnsErrorIfInvalidTxBody)
{
  // Arrange
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex("84ef", strlen("84ef"));
  cardano_transaction_t* transaction = NULL;

  // Act
  cardano_error_t result = cardano_transaction_from_cbor(reader, &transaction);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_from_cbor, returnsErrorIfInvalidTxWitnessSet)
{
  // Arrange
  const char*            cbor        = "84a600d9010281825820260aed6e7a24044b1254a87a509468a649f522a4e54e830ac10f27ea7b5ec61f010183a300581d70b429738bd6cc58b5c7932d001aa2bd05cfea47020a556c8c753d4436011a004c4b40028200582007845f8f3841996e3d8157954e2f5e2fb90465f27112fc5fe9056d916fae245ba200583900b1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c68042f1946335c498d2e7556c5c647c4649c6a69d2b645cd1428a339ba011a04636769a200583900b1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c68042f1946335c498d2e7556c5c647c4649c6a69d2b645cd1428a339ba01821a00177a6ea2581c648823ffdad1610b4162f4dbc87bd47f6f9cf45d772ddef661eff198a5447742544319271044774554481a0031f9194577444f47451a0056898d4577555344431a000fc589467753484942411a000103c2581c659ab0b5658687c2e74cd10dba8244015b713bf503b90557769d77a7a14a57696e675269646572731a02269552021a0002e665031a01353f84081a013531740b58204107eada931c72a600a6e3305bd22c7aeb9ada7c3f6823b155f4db85de36a69aef00d9010281825820e686ade5bc97372f271fd2abc06cfd96c24b3d9170f9459de1d8e3dd8fd385575840653324a9dddad004f05a8ac99fa2d1811af5f00543591407fb5206cfe9ac91bb1412404323fa517e0e189684cd3592e7f74862e3f16afbc262519abec958180c04d9010281d8799fd8799fd8799fd8799f581cb1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c68ffd8799fd8799fd8799f581c042f1946335c498d2e7556c5c647c4649c6a69d2b645cd1428a339baffffffff581cb1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c681b000001863784a12ed8799fd8799f4040ffd8799f581c648823ffdad1610b4162f4dbc87bd47f6f9cf45d772ddef661eff1984577444f4745ffffffd8799fd87980190c8efffff5f6";
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_transaction_t* transaction = NULL;

  // Act
  cardano_error_t result = cardano_transaction_from_cbor(reader, &transaction);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_from_cbor, returnsErrorIfInvalidIsValid)
{
  // Arrange
  const char*            cbor        = "84a600d9010281825820260aed6e7a24044b1254a87a509468a649f522a4e54e830ac10f27ea7b5ec61f010183a300581d70b429738bd6cc58b5c7932d001aa2bd05cfea47020a556c8c753d4436011a004c4b40028200582007845f8f3841996e3d8157954e2f5e2fb90465f27112fc5fe9056d916fae245ba200583900b1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c68042f1946335c498d2e7556c5c647c4649c6a69d2b645cd1428a339ba011a04636769a200583900b1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c68042f1946335c498d2e7556c5c647c4649c6a69d2b645cd1428a339ba01821a00177a6ea2581c648823ffdad1610b4162f4dbc87bd47f6f9cf45d772ddef661eff198a5447742544319271044774554481a0031f9194577444f47451a0056898d4577555344431a000fc589467753484942411a000103c2581c659ab0b5658687c2e74cd10dba8244015b713bf503b90557769d77a7a14a57696e675269646572731a02269552021a0002e665031a01353f84081a013531740b58204107eada931c72a600a6e3305bd22c7aeb9ada7c3f6823b155f4db85de36a69aa200d9010281825820e686ade5bc97372f271fd2abc06cfd96c24b3d9170f9459de1d8e3dd8fd385575840653324a9dddad004f05a8ac99fa2d1811af5f00543591407fb5206cfe9ac91bb1412404323fa517e0e189684cd3592e7f74862e3f16afbc262519abec958180c04d9010281d8799fd8799fd8799fd8799f581cb1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c68ffd8799fd8799fd8799f581c042f1946335c498d2e7556c5c647c4649c6a69d2b645cd1428a339baffffffff581cb1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c681b000001863784a12ed8799fd8799f4040ffd8799f581c648823ffdad1610b4162f4dbc87bd47f6f9cf45d772ddef661eff1984577444f4745ffffffd8799fd87980190c8effffeff6";
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_transaction_t* transaction = NULL;

  // Act
  cardano_error_t result = cardano_transaction_from_cbor(reader, &transaction);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_from_cbor, returnsErrorIfInvalidAuxData)
{
  // Arrange
  const char*            cbor        = "84a600d9010281825820260aed6e7a24044b1254a87a509468a649f522a4e54e830ac10f27ea7b5ec61f010183a300581d70b429738bd6cc58b5c7932d001aa2bd05cfea47020a556c8c753d4436011a004c4b40028200582007845f8f3841996e3d8157954e2f5e2fb90465f27112fc5fe9056d916fae245ba200583900b1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c68042f1946335c498d2e7556c5c647c4649c6a69d2b645cd1428a339ba011a04636769a200583900b1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c68042f1946335c498d2e7556c5c647c4649c6a69d2b645cd1428a339ba01821a00177a6ea2581c648823ffdad1610b4162f4dbc87bd47f6f9cf45d772ddef661eff198a5447742544319271044774554481a0031f9194577444f47451a0056898d4577555344431a000fc589467753484942411a000103c2581c659ab0b5658687c2e74cd10dba8244015b713bf503b90557769d77a7a14a57696e675269646572731a02269552021a0002e665031a01353f84081a013531740b58204107eada931c72a600a6e3305bd22c7aeb9ada7c3f6823b155f4db85de36a69aa200d9010281825820e686ade5bc97372f271fd2abc06cfd96c24b3d9170f9459de1d8e3dd8fd385575840653324a9dddad004f05a8ac99fa2d1811af5f00543591407fb5206cfe9ac91bb1412404323fa517e0e189684cd3592e7f74862e3f16afbc262519abec958180c04d9010281d8799fd8799fd8799fd8799f581cb1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c68ffd8799fd8799fd8799f581c042f1946335c498d2e7556c5c647c4649c6a69d2b645cd1428a339baffffffff581cb1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c681b000001863784a12ed8799fd8799f4040ffd8799f581c648823ffdad1610b4162f4dbc87bd47f6f9cf45d772ddef661eff1984577444f4745ffffffd8799fd87980190c8efffff587";
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_transaction_t* transaction = NULL;

  // Act
  cardano_error_t result = cardano_transaction_from_cbor(reader, &transaction);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

// Getters and Setters

TEST(cardano_transaction_get_body, canGetBody)
{
  // Arrange
  cardano_transaction_t*      transaction = new_default_transaction(CBOR);
  cardano_transaction_body_t* body        = new_default_transaction_body(TX_BODY_CBOR);

  EXPECT_EQ(cardano_transaction_set_body(transaction, body), CARDANO_SUCCESS);

  // Act
  cardano_transaction_body_t* body2 = cardano_transaction_get_body(transaction);

  // Assert
  EXPECT_NE(body2, nullptr);

  // Cleanup
  cardano_transaction_unref(&transaction);
  cardano_transaction_body_unref(&body);
  cardano_transaction_body_unref(&body2);
}

TEST(cardano_transaction_get_body, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_transaction_body_t* body = cardano_transaction_get_body(nullptr);

  // Assert
  EXPECT_EQ(body, nullptr);
}

TEST(cardano_transaction_set_body, canSetBody)
{
  // Arrange
  cardano_transaction_t*      transaction = new_default_transaction(CBOR);
  cardano_transaction_body_t* body        = new_default_transaction_body(TX_BODY_CBOR);

  // Act
  cardano_error_t result = cardano_transaction_set_body(transaction, body);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_unref(&transaction);
  cardano_transaction_body_unref(&body);
}

TEST(cardano_transaction_set_body, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_transaction_body_t* body = new_default_transaction_body(TX_BODY_CBOR);

  // Act
  cardano_error_t result = cardano_transaction_set_body(nullptr, body);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_body_unref(&body);
}

TEST(cardano_transaction_set_body, returnsErrorIfBodyIsNull)
{
  // Arrange
  cardano_transaction_t* transaction = new_default_transaction(CBOR);

  // Act
  cardano_error_t result = cardano_transaction_set_body(transaction, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_get_auxiliary_data, canGetAuxiliaryData)
{
  // Arrange
  cardano_transaction_t*    transaction    = new_default_transaction(CBOR);
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  EXPECT_EQ(cardano_transaction_set_auxiliary_data(transaction, auxiliary_data), CARDANO_SUCCESS);

  // Act
  cardano_auxiliary_data_t* auxiliary_data2 = cardano_transaction_get_auxiliary_data(transaction);

  // Assert
  EXPECT_NE(auxiliary_data2, nullptr);

  // Cleanup
  cardano_transaction_unref(&transaction);
  cardano_auxiliary_data_unref(&auxiliary_data);
  cardano_auxiliary_data_unref(&auxiliary_data2);
}

TEST(cardano_transaction_get_auxiliary_data, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = cardano_transaction_get_auxiliary_data(nullptr);

  // Assert
  EXPECT_EQ(auxiliary_data, nullptr);
}

TEST(cardano_transaction_set_auxiliary_data, canSetAuxiliaryData)
{
  // Arrange
  cardano_transaction_t*    transaction    = new_default_transaction(CBOR);
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  // Act
  cardano_error_t result = cardano_transaction_set_auxiliary_data(transaction, auxiliary_data);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_unref(&transaction);
  cardano_auxiliary_data_unref(&auxiliary_data);
}

TEST(cardano_transaction_set_auxiliary_data, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_auxiliary_data_t* auxiliary_data = new_default_auxiliary_data(AUXILIARY_DATA_CBOR);

  // Act
  cardano_error_t result = cardano_transaction_set_auxiliary_data(nullptr, auxiliary_data);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_auxiliary_data_unref(&auxiliary_data);
}

TEST(cardano_transaction_set_auxiliary_data, canSetNullAuxiliaryData)
{
  // Arrange
  cardano_transaction_t* transaction = new_default_transaction(CBOR);

  // Act
  cardano_error_t result = cardano_transaction_set_auxiliary_data(transaction, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_get_witness_set, canGetWitnessSet)
{
  // Arrange
  cardano_transaction_t* transaction = new_default_transaction(CBOR);
  cardano_witness_set_t* witness_set = new_default_witness_set(WITNESS_SET_CBOR);

  EXPECT_EQ(cardano_transaction_set_witness_set(transaction, witness_set), CARDANO_SUCCESS);

  // Act
  cardano_witness_set_t* witness_set2 = cardano_transaction_get_witness_set(transaction);

  // Assert
  EXPECT_NE(witness_set2, nullptr);

  // Cleanup
  cardano_transaction_unref(&transaction);
  cardano_witness_set_unref(&witness_set);
  cardano_witness_set_unref(&witness_set2);
}

TEST(cardano_transaction_get_witness_set, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(nullptr);

  // Assert
  EXPECT_EQ(witness_set, nullptr);
}

TEST(cardano_transaction_set_witness_set, canSetWitnessSet)
{
  // Arrange
  cardano_transaction_t* transaction = new_default_transaction(CBOR);
  cardano_witness_set_t* witness_set = new_default_witness_set(WITNESS_SET_CBOR);

  // Act
  cardano_error_t result = cardano_transaction_set_witness_set(transaction, witness_set);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_unref(&transaction);
  cardano_witness_set_unref(&witness_set);
}

TEST(cardano_transaction_set_witness_set, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_witness_set_t* witness_set = new_default_witness_set(WITNESS_SET_CBOR);

  // Act
  cardano_error_t result = cardano_transaction_set_witness_set(nullptr, witness_set);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_witness_set_unref(&witness_set);
}

TEST(cardano_transaction_set_witness_set, returnsErrorIfWitnessSetIsNull)
{
  // Arrange
  cardano_transaction_t* transaction = new_default_transaction(CBOR);

  // Act
  cardano_error_t result = cardano_transaction_set_witness_set(transaction, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_set_is_valid, canSetIsValid)
{
  // Arrange
  cardano_transaction_t* transaction = new_default_transaction(CBOR);

  // Act
  cardano_error_t result = cardano_transaction_set_is_valid(transaction, true);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_set_is_valid, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_error_t result = cardano_transaction_set_is_valid(nullptr, true);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_get_is_valid, canGetIsValid)
{
  // Arrange
  cardano_transaction_t* transaction = new_default_transaction(CBOR);

  EXPECT_EQ(cardano_transaction_set_is_valid(transaction, true), CARDANO_SUCCESS);

  // Act
  bool is_valid = cardano_transaction_get_is_valid(transaction);

  // Assert
  EXPECT_EQ(is_valid, true);

  // Cleanup
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_get_is_valid, returnsFalseIfObjectIsNull)
{
  // Act
  bool is_valid = cardano_transaction_get_is_valid(nullptr);

  // Assert
  EXPECT_EQ(is_valid, false);
}

TEST(cardano_transaction_set_is_valid, canSetIsValidFalse)
{
  // Arrange
  cardano_transaction_t* transaction = new_default_transaction(CBOR);

  // Act
  cardano_error_t result = cardano_transaction_set_is_valid(transaction, false);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_get_id, canGetId)
{
  // Arrange
  cardano_transaction_t* transaction = new_default_transaction(CBOR);

  // Act
  cardano_blake2b_hash_t* id = cardano_transaction_get_id(transaction);

  size_t size = cardano_blake2b_hash_get_hex_size(id);
  char*  hex  = (char*)malloc(size);

  cardano_error_t result = cardano_blake2b_hash_to_hex(id, hex, size);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(hex, CBOR_TX_ID);

  // Cleanup
  cardano_transaction_unref(&transaction);
  cardano_blake2b_hash_unref(&id);
  free(hex);
}

TEST(cardano_transaction_get_id, canGetId2)
{
  // Arrange
  cardano_transaction_t* transaction = new_default_transaction(CBOR3);

  // Act
  cardano_blake2b_hash_t* id = cardano_transaction_get_id(transaction);

  size_t size = cardano_blake2b_hash_get_hex_size(id);
  char*  hex  = (char*)malloc(size);

  cardano_error_t result = cardano_blake2b_hash_to_hex(id, hex, size);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(hex, CBOR3_TX_ID);

  // Cleanup
  cardano_transaction_unref(&transaction);
  cardano_blake2b_hash_unref(&id);
  free(hex);
}

TEST(cardano_transaction_get_id, canGetId3)
{
  // Arrange
  cardano_transaction_t* transaction = new_default_transaction(CBOR_NULLIFY_ENTROPY);

  // Act
  cardano_blake2b_hash_t* id = cardano_transaction_get_id(transaction);

  size_t size = cardano_blake2b_hash_get_hex_size(id);
  char*  hex  = (char*)malloc(size);

  cardano_error_t result = cardano_blake2b_hash_to_hex(id, hex, size);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(hex, CBOR_NULLIFY_ENTROPY_TX_ID);

  // Cleanup
  cardano_transaction_unref(&transaction);
  cardano_blake2b_hash_unref(&id);
  free(hex);
}

TEST(cardano_transaction_get_id, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_blake2b_hash_t* id = cardano_transaction_get_id(nullptr);

  // Assert
  EXPECT_EQ(id, nullptr);
}

TEST(cardano_transaction_clear_cbor_cache, doesNothingIfGivenNullPtr)
{
  // Act
  cardano_transaction_clear_cbor_cache(nullptr);
}

// Fuzzer found crashes

TEST(cardano_transaction_from_cbor, fuzzerCase1DoesntCrash)
{
  cardano_transaction_t* transaction = NULL;
  const char*            cbor        = "84a70081825820857753f212e04d4cf8adaf337cdf6fa648d4bc1f8a915101c524665c04c7dbee00018182583900f892eeda68418590c4e63a0b3ab6e298eddafcab732b76c3cbe0d565fb49ad91d5fb425d08d2e2b1d7e970d633a026d6175dbf692b9b157c1a00495b0f021a0002f031031a01bba3a6075820fd27fd29ca1544192304ae5567be38a8844cb4b6bb002dc5d4ff027596c45a9f08160e81581cfb49ad91d5fb425d08d2e2b1d7e970d633a026d6175dbf692b9b157ca100818258204b41aa92e8d7a4043768b54cf4c361e11b3948cdadde7d77e710dfe584007b735f80294c2c96d1bcbedd1e3712524c18f97f7e52b9bc5a7b3b6975295b688ecc0bdce420ab3dd75bb215481dbb215481dfad0a1d0335f38dc7d1e63769bba2520a0bf5a119167901";
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result      = cardano_transaction_from_cbor(reader, &transaction);

  CARDANO_UNUSED(result);

  cardano_cbor_reader_unref(&reader);
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_from_cbor, fuzzerCase2DoesntCrash)
{
  cardano_transaction_t* transaction = NULL;
  const char*            cbor        = "84a70081825820857753f212e04d4cf8adaf337cdf6fa648d4bc1f8a915101c524665c04c7dbee000181825753f212e04d4cf8adaf337cdf6fa648d4bc1f8a915101c524665c04c7dbee00018182583900f892eeddafcab732b76c3cbe0d565fb49ad91d5fbbe0d565fb49ad91d5fb425d08d2e2b1d7e970d633a026d6175dbf692b9b157c1a00495b0f021a0002f031031a01bba3a6075820fd27fd29ca1544192304ae5567be38a8844cb4b6bb002dc5d4ff027596c45a9f08160e81581cfb49ad91d5fb425d08d2e2b1d7e970d633a066d6175dbf692b9b157ca100818258204b41aa92e8d7a4043768b54cf4c361e11b3948cdadd239dc422fe7d7783900f892eeddafcab732b76c3cbe0d565fb49ad91d5fbbe0d565fb49ad91d5fb425d08d2e2b1d7e970d633a026d6175dbf692b9b157c1a00495b0f021a0002f031031a01bba3a6075820fd27fd29ca1544192305657ae4be38a8844cb4b6bb002dc5d4ff027596c45a9f08160e81581cfb49ad91d5fb425d08d2e2b1d7e970d633a066d6175dbf692b9b157ca100818258204b41aa92e8d7a4043768b54cf4c361e11b3948cdadd239dc422fe7d77e710dfe584007b735f80294c2c96d1bcbedd1e3712524c18f97f7e52b9bc5a7b3b6975295b688ecc0bdce420ab3dd75bb215481dfad0a1d0335f38dc7d1e63769bba2520a0bf5a119167901";
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result      = cardano_transaction_from_cbor(reader, &transaction);

  CARDANO_UNUSED(result);

  cardano_cbor_reader_unref(&reader);
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_from_cbor, fuzzerCase3DoesntCrash)
{
  cardano_transaction_t* transaction = NULL;
  const char*            cbor        = "84a70081825820857753f212e04d4cf8adaf337cdf6fa648d4bc1f8a917101c524665c04c7dbee00018182495b0f021a0002f031031a01bba3a6075820fd27fd29ca1544192304ae5567be38a8844cb4b6bb002dc5d4ff027596c45a9f08160e81581cfb49ad91d5fb425d08d2e2b1d7e970d633a066d6175dbfd77e710dfe584007b735f80294c2c96d1bcbedd1e3712524c18f97f7e52b9bc5a7b3b6975295b688ecc0bdce420ab3dd75bb215481dfad0a1d0335f38dc7d1e637695820fd27fd29ca19167901";
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result      = cardano_transaction_from_cbor(reader, &transaction);

  CARDANO_UNUSED(result);

  cardano_cbor_reader_unref(&reader);
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_from_cbor, fuzzerCase4DoesntCrash)
{
  cardano_transaction_t* transaction = NULL;
  const char*            cbor        = "84a70081825820857753f212e04d4cf8adaf337cdf6fa648d4bc1f8a915101c524665c04c7dbee00018182525820857753f212e04d4cf8adaf337cdf6fa648d4bc1f8a915101c524665c04c7dbee00018182583900f892eeddafcab732b76c3cbe0d565fb49ad91d5fbbe0d565fb49ad91d5fb425d08d2e2b1d7e970d633a026d6175dbf692b9b157c1a00495b0f021a0002f031031a01bba3a6075820fd27fd29ca1544192304ae5567be38a8844cb4b6bb002dc5d4ff027596c45a9f08160e81581cfb49ad91d5fb425d08d2e2b1d7e970d633a066d6175dbf692b9b157ca100818258204b41aa92e8d7a4043768b54cf4c361e11b3948cdadd239dc422fe7d77e710dfe584007b735f80294c2c7d1e63769bba2520a0bf5a1191c96d1bcbedd1e3712524c18f97f7e52b9bc5a7b3b6975295b688ecc0bdce420ab3dd75bb215481dfad0a1d0335f386dc7d1e637679019bba2520a0bf5a119167901";
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result      = cardano_transaction_from_cbor(reader, &transaction);

  CARDANO_UNUSED(result);

  cardano_cbor_reader_unref(&reader);
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_from_cbor, fuzzerCase5DoesntCrash)
{
  cardano_transaction_t* transaction = NULL;
  const char*            cbor        = "83a70081825820857753f212e04d4cf8adaf337cdf6fa647d4bc1f8a915101c524665c04c7dbee00018182525820857753f212e04d4cf8adaf337cdf6fa647d4bc1f8a915101c524665c04c7dbee00018182583900f892eeddafcab732b76c3cbe0d565fb49ad91d5fbbe0d565fb49ad91d5fb425d08d2e2b1d7e970d633a026d6175dbf692b9b157c1a00495b0f021a0002f031031a01bba3a6075820fd27fd29ca1544192304ae5567be38a8844cb4b6bb002dc5d4ff027596c45a9f08160e815812e2b1d7e970d633a066d6175dbf692b9b157ca100818258204b41aa92e8d7a4043768b54cf4c361e11b3948cdadd239dc422fe7d77e710dfe584007b735f80294c2c96d1bcbedd1e3712524c18f97f7e52b9bc5a7b9167901";
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result      = cardano_transaction_from_cbor(reader, &transaction);

  CARDANO_UNUSED(result);

  cardano_cbor_reader_unref(&reader);
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_from_cbor, fuzzerCase6DoesntCrash)
{
  cardano_transaction_t* transaction = NULL;
  const char*            cbor        = "83a70081825820857753f212e04d4cf8adaf337cdf6fa647d4bc1f8a915101c524665c04c7dbee00018182525820857753f212e04d4cf8adaf337cdf6fa647d4bc1f8a915101c524665c04c7dbee00018182583900f892eeddafcab732b76c3cbe0d565fb49ad91d5fbbe0d565fb49ad91d5fb425d08d2e2b1d7e970d633a026d6175dbf692b9b157c1a00495b0f021a0002f031031a01bba3a6075820fd27fd29ca1544192304ae5567be38a8844cb4b6bb002dc5d4ff027596c45a9f08160e815812e2b1d7e970d633a066d6175dbf692b9b157ca100818258204b41aa92e8d7a4043768b54cf4c361e11b3948cdadd239dc422fe7d77e710dfe584007b735f80294c2c96d1bcbedd1e3712524c18f97f7e52b9bc5a7b3b6975295b688ecc0bdce420ab3dd75bb215481dfad0a1d05f38dc7d1e63769bba2520a0bf5a1183900f892eeddafcab732b76c3cbe0d565fb49ad91d5fbbe0d565fb49ad91d5fb425d08d2e2b1d7e970d633a026d6175dbf692b9b157c1a00495b0f021a0002f031031a01bba3a6075820fd27fd29ca1544192304ae5567be38a8844cb4b6bb002dc5d4ff027596c45a9f08160e815812e2b1d7e970d633a066d6175dbf692b9b157ca107753f212e04d4cf8adaf337cdf6fa647d4bc1f8a915101c524665c04c7dbee00018182583900f892eeddafcab732b76c3cbe0d565fb49ad91d5fbbe0d565fb49ad91d5f565fb49ad91d5fbbe0d565fb49ad91d5fb425d08d2e2b1d7e970d633a026d6175dbf692b9b167901";
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result      = cardano_transaction_from_cbor(reader, &transaction);

  CARDANO_UNUSED(result);

  cardano_cbor_reader_unref(&reader);
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_apply_vkey_witnesses, canUpdateWitnessSet)
{
  // Arrange
  cardano_transaction_t*      transaction      = new_default_transaction(CBOR);
  cardano_witness_set_t*      witness_set      = new_default_witness_set(WITNESS_SET_CBOR);
  cardano_vkey_witness_set_t* vkey_witness_set = new_default_vkey_witness_set(VKEY_WITNESS_CBOR);

  EXPECT_EQ(cardano_transaction_set_witness_set(transaction, witness_set), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_transaction_apply_vkey_witnesses(transaction, vkey_witness_set);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_unref(&transaction);
  cardano_witness_set_unref(&witness_set);
  cardano_vkey_witness_set_unref(&vkey_witness_set);
}

TEST(cardano_transaction_apply_vkey_witnesses, canUpdateWitnessSetEvenIfVkeyIsNull)
{
  // Arrange
  cardano_transaction_t*      transaction      = new_default_transaction(CBOR);
  cardano_witness_set_t*      witness_set      = NULL;
  cardano_vkey_witness_set_t* vkey_witness_set = new_default_vkey_witness_set(VKEY_WITNESS_CBOR);

  EXPECT_EQ(cardano_witness_set_new(&witness_set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_set_witness_set(transaction, witness_set), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_transaction_apply_vkey_witnesses(transaction, vkey_witness_set);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_unref(&transaction);
  cardano_witness_set_unref(&witness_set);
  cardano_vkey_witness_set_unref(&vkey_witness_set);
}

TEST(cardano_transaction_apply_vkey_witnesses, returnsErrorIfNull)
{
  EXPECT_EQ(cardano_transaction_apply_vkey_witnesses(nullptr, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_transaction_apply_vkey_witnesses((cardano_transaction_t*)"", nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_transaction_apply_vkey_witnesses(nullptr, (cardano_vkey_witness_set_t*)""), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_transaction_has_script_data, returnsFalseifNull)
{
  EXPECT_EQ(cardano_transaction_has_script_data(nullptr), false);
}

TEST(cardano_transaction_has_script_data, returnsTrueIfHasScriptData)
{
  // Arrange
  cardano_transaction_t* transaction = new_default_transaction(CBOR);

  // Act
  bool has_script_data = cardano_transaction_has_script_data(transaction);

  // Assert
  EXPECT_TRUE(has_script_data);

  // Cleanup
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_has_script_data, returnsFalseIfNoScriptData)
{
  // Arrange
  cardano_transaction_t* transaction = new_default_transaction(CBOR_NULLIFY_ENTROPY);

  // Act
  bool has_script_data = cardano_transaction_has_script_data(transaction);

  // Assert
  EXPECT_FALSE(has_script_data);

  // Cleanup
  cardano_transaction_unref(&transaction);
}

// Fuzzer found crashes

TEST(cardano_transaction_from_cbor, returnsDecodingErrorIfRepeatedKeyInOutput)
{
  // Arrange
  cardano_transaction_t* transaction = NULL;
  const char*            cbor        = "9a80820260a30208048010a30108010a30100424008f37086f30088f88fff8f9889898";
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_transaction_from_cbor(reader, &transaction);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_from_cbor, returnsDecodingErrorIfInvalidAddressInKeyInOutput)
{
  // Arrange
  cardano_transaction_t* transaction = NULL;
  const char*            cbor        = "88a2080210a2010000f0";
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_transaction_from_cbor(reader, &transaction);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_transaction_unref(&transaction);
}

TEST(cardano_transaction_from_cbor, returnsErrorIfInvalidAssetNameInTransaction)
{
  // Arrange
  cardano_transaction_t* transaction = NULL;
  const char*            cbor        = "84a600d9010281825820260aed6e7a24044b1254a87a509468a649f522a4e54e830ac10f27ea7b5ec61f010183a300581d70b429738bd6cc58b5c7932d001aa2bd05cfea47020a556c8c753d4436011a004c4b40028200582007845f8f3841996e3d815747c4649c6a69d2b645cd1428a339ba011a04636769a200583900b1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c68042f1946335c498d2e7556c5c647c4649c6a69d2b645cd1428a339ba01821a00177a6ea2581c648823ffdad1610b4162f4dbc87bd47f6f9cf45d772ddef661eff198a5467742544319271044774554481a0031f9194577444f474500583900b1814238b0d287a8a46ce7348c6ad79ab8995b0e6d46010e2d9e1c68042f1946335c498d2e7556c5c647c4649c6a69d2b645cd1428a339ba01821a00177a6ea2581c648823ffdad1610b4162f4dbc87bd47f6f9cf45d772ddef661eff198a5467742544319271044774554481a0031f9194577444f47451a0056898d4577555344431a000fc589467753484942411a000103c2581c659ab0b5658687c2e74cd10dba8244015b713bf503b90557769d77a7a14a57696e675269646572731a02269552021a0002e665031a01353f84081a013531740b58204107eada9c7a90Ae1a0056898d4577555344431a000fc589467753484942411a000103c2581c659ab0b5658687c2e74cd10dba8244015b713bf503b90557769d77a7a14a57696e675269646572731a02269552021a0002e665031a01353f84081a013531740b58204107eada9c7a90Aeb9ada7c3f0A26823b1de36610b4162f4dbc87bd4d87980190c8efffff4f6";
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_transaction_from_cbor(reader, &transaction);

  // Assert
  EXPECT_NE(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_transaction_unref(&transaction);
}
