/**
 * \file fee.cpp
 *
 * \author angel.castillo
 * \date   Oct 13, 2024
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

#include <cardano/transaction_builder/fee.h>

#include <allocators.h>
#include <cardano/transaction/sub_transaction.h>
#include <cardano/transaction_body/sub_transaction_set.h>
#include <cardano/transaction_body/transaction_output.h>
#include <gmock/gmock.h>
#include <tests/allocators_helpers.h>

/* CONSTANTS *****************************************************************/

static const uint64_t COST_PER_UTXO_BYTE = 4310U;

/* TEST VECTORS **************************************************************/

typedef struct
{
    uint64_t    fee;
    const char* cbor;
    size_t      cbor_length;
} fee_vector_t;

static const fee_vector_t min_ada_required_vectors[] = {
  { 978370, "825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc00" },
  { 1129220, "825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc8200a1581c8b8370c97ae17eb69a8c97f733888f7485b60fd820c69211c8bbeb56a14001" },
  { 1133530, "825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc8200a1581c8b8370c97ae17eb69a8c97f733888f7485b60fd820c69211c8bbeb56a1410001" },
  { 1159390, "825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc821a0017bc62a1581c8b8370c97ae17eb69a8c97f733888f7485b60fd820c69211c8bbeb56a3410001410101410201" },
  { 1271450, "825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc8200a2581cab8370c97ae17eb69a8c97f733888f7485b60fd820c69211c8bbeb56a14001581cbb8370c97ae17eb69a8c97f733888f7485b60fd820c69211c8bbeb56a14001" },
  { 1280070, "825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc8200a2581cab8370c97ae17eb69a8c97f733888f7485b60fd820c69211c8bbeb56a1410001581cbb8370c97ae17eb69a8c97f733888f7485b60fd820c69211c8bbeb56a141ff01" },
  { 1288690, "835839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc8200a1581cab8370c97ae17eb69a8c97f733888f7485b60fd820c69211c8bbeb56a1400158200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5" },
  { 1430920, "835839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc821a0017bc62a1581cab8370c97ae17eb69a8c97f733888f7485b60fd820c69211c8bbeb56a15820ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff0158200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5" },
  { 1430920, "835839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc8200a2581cab8370c97ae17eb69a8c97f733888f7485b60fd820c69211c8bbeb56a14001581cbb8370c97ae17eb69a8c97f733888f7485b60fd820c69211c8bbeb56a1400158200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5" },
  { 1305930, "a3005839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc018200a2581cab8370c97ae17eb69a8c97f733888f7485b60fd820c69211c8bbeb56a14001581cbb8370c97ae17eb69a8c97f733888f7485b60fd820c69211c8bbeb56a14001028201d81842187b" },
  { 1680900, "a4005839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc018200a2581cab8370c97ae17eb69a8c97f733888f7485b60fd820c69211c8bbeb56a14001581cbb8370c97ae17eb69a8c97f733888f7485b60fd820c69211c8bbeb56a14001028201d81842187b03d818585282008202828200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d3985388201838205190bb88200581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378204190fa0" },
};

static const fee_vector_t tx_fee_vectors[] = {
  { 176193, "84a500818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5000181825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e020a031903e8081864a200818258206199186adb51974690d7247d2646097d2c62763b767b528816fb7ed3f9f55d395840bdea87fca1b4b4df8a9b8fb4183c0fab2f8261eb6c5e4bc42c800bb9c8918755bdea87fca1b4b4df8a9b8fb4183c0fab2f8261eb6c5e4bc42c800bb9c89187550281845820deeb8f82f2af5836ebbc1b450b6dbf0b03c93afe5696f10d49e8a8304ebfac01584064676273786767746f6768646a7074657476746b636f6376796669647171676775726a687268716169697370717275656c6876797071786565777072796676775820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b45041a0f5f6" },
  { 257254, "84af00818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5000181825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e020a031903e804828304581c26b17b78de4f035dc0bfce60d1d3c3a8085c38dcce5fb8767e518bed1901f48405581c0d94e174732ef9aae73f395ab44507bfa983d65023c11a951f0c32e4581ca646474b8f5431261506b6c273d307c7569a4eb6c96b42dd4a29520a582003170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c11131405a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0050758202ceb364d93225b4a0f004a0975a13eb50c3cc6348474b4fe9121f8dc72ca0cfa08186409a3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c413831581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e0b58206199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d38abc123de0d818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5010e81581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d3910825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e11186412818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d500a700818258206199186adb51974690d7247d2646097d2c62763b767b528816fb7ed3f9f55d395840bdea87fca1b4b4df8a9b8fb4183c0fab2f8261eb6c5e4bc42c800bb9c8918755bdea87fca1b4b4df8a9b8fb4183c0fab2f8261eb6c5e4bc42c800bb9c891875501868205186482041901f48200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f548201818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f548202818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f54830301818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f540281845820deeb8f82f2af5836ebbc1b450b6dbf0b03c93afe5696f10d49e8a8304ebfac01584064676273786767746f6768646a7074657476746b636f6376796669647171676775726a687268716169697370717275656c6876797071786565777072796676775820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b45041a003815820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b4500481187b0582840100d8668200810182190bb8191b58840201d86682008102821913881907d006815820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b450f5a6011904d2026373747203821904d2637374720445627974657305a2667374726b6579187b81676c6973746b65796873747276616c75650626" },
  { 326472, "84ab0081825820fbecbe69bc3ee617653b95893f50b0362cbaff3e27b01a936969a25bfc100a7c000182835839319068a7a3f008803edac87af1619860f2cdcde40c26987325ace138ad2c967f4bd28944b06462e13c5e3f5d5fa6e03f8567569438cd833e6d1a093d1cc0582057ad45489e9d4e3d7df98fb6b273d647cbed6990125dc51815bdee9abbc3a84a82583901e6d3410be0b3d51a52b874983bc6f0e48bcf4274352552e683e1c211b91d1fe82203de4c0de2c150746383a893cd21eb1082565252af63a51a00eef6fe021a00052d02031a082ee80007582026e4e8217ceb7c9eee2dffc410d77bbe3efd952288573f1c9a19fe62979634bc0b5820fe1f0d446610edf6890cbce2c3e69ad4052c557fd6d044b1f195a4f916c3e0fe0d8182582047754bf3cb4adf7374496b17fa41c197043533355c7a31a1776207fe627f5a5d010e81581ce6d3410be0b3d51a52b874983bc6f0e48bcf4274352552e683e1c2111082583901e6d3410be0b3d51a52b874983bc6f0e48bcf4274352552e683e1c211b91d1fe82203de4c0de2c150746383a893cd21eb1082565252af63a51a00461558111a0007c38312818258209a32459bd4ef6bbafdeb8cf3b909d0e3e2ec806e4cc6268529280b0fc1d06f5b00a3008182582005e884ca7c466df47785af770be8495ec0998e60ebe63e4cd187cd17eeac5e9258402cdcec5c4ba1ea76c558554dea99f472b67488be18f1f7085bac4cc55376ca8f4ed61b23565ddbffcd85a4e84963c36c98272314d2637b238e65def639969f0b0481d8799f581ce6d3410be0b3d51a52b874983bc6f0e48bcf4274352552e683e1c2119fd8799fd8799fd87a9f581c84cc25ea4c29951d40b443b95bbc5676bc425470f96376d1984af9abffd8799fd8799fd87a9f581c2c967f4bd28944b06462e13c5e3f5d5fa6e03f8567569438cd833e6dffffffffa140d8799f00a1401a00342f60ffffd8799fd8799fd8799f581cf437291791dda80d0bba9f3616f8b7533c8a8db2f788b8468a26bd5affd8799fd8799fd8799f581ce3c9536e2947e33703d5793a02b593a8d32b49aaaef03ea0b2b03c87ffffffffa140d8799f00a1401a0104ece0ffffd8799fd8799fd8799f581ce6d3410be0b3d51a52b874983bc6f0e48bcf4274352552e683e1c211ffd8799fd8799fd8799f581cb91d1fe82203de4c0de2c150746383a893cd21eb1082565252af63a5ffffffffa1581cb2d25f829ebb7f4c97b5e847923a1115b23ebf78000722c229c9c9f7d8799f01a0ffffffff0581840000d87980821a000af3301a0b01ca09f5ae181e613518327840643837393966353831636536643334313062653062336435316135326238373439383362633666306534386263663432373433353235353265363833653163321833784031313966643837393966643837393966643837613966353831633834636332356561346332393935316434306234343362393562626335363736626334323534183478403730663936333736643139383461663961626666643837393966643837393966643837613966353831633263393637663462643238393434623036343632653118357840336335653366356435666136653033663835363735363934333863643833336536646666666666666666613134306438373939663030613134303161303032661836784034643630666666666438373939666438373939666438373939663538316366343337323931373931646461383064306262613966333631366638623735333363183778403861386462326637383862383436386132366264356166666438373939666438373939666438373939663538316365336339353336653239343765333337303318387840643537393361303262353933613864333262343961616165663033656130623262303363383766666666666666666131343064383739396630306131343031611839784030306563383265306666666664383739396664383739396664383739396635383163653664333431306265306233643531613532623837343938336263366630183a784065343862636634323734333532353532653638336531633231316666643837393966643837393966643837393966353831636239316431666538323230336465183b784034633064653263313530373436333833613839336364323165623130383235363532353261663633613566666666666666666131353831636232643235663832183c784039656262376634633937623565383437393233613131313562323365626637383030303732326332323963396339663764383739396630316130666666666666183d6366662c183e783c62326432356638323965626237663463393762356538343739323361313131356232336562663738303030373232633232396339633966373a3a3030" }
};

static const char* SUB_TX_INPUT_ID                      = "0f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5";
static const char* RESOLVED_INPUT                       = "82825820fbecbe69bc3ee617653b95893f50b0362cbaff3e27b01a936969a25bfc100a7c00835839319068a7a3f008803edac87af1619860f2cdcde40c26987325ace138ad2c967f4bd28944b06462e13c5e3f5d5fa6e03f8567569438cd833e6d1a0a3140c05820c6b9e0671fef714142bda45beedf7b51c2d4e3676f79196964082fef164ef7e4";
static const char* RESOLVED_INPUT_WITH_REFERENCE_SCRIPT = "828258209a32459bd4ef6bbafdeb8cf3b909d0e3e2ec806e4cc6268529280b0fc1d06f5b00a3005839119068a7a3f008803edac87af1619860f2cdcde40c26987325ace138ad81728e7ed4cf324e1323135e7e6d931f01e30792d9cdf17129cb806d011a02625a0003d818590a068202590a015909fe010000323232323232323232323232323232323232323232323232323232323232323232323232323232323232323232323232222323232533535533357346064606a0062646464642466002008004a666ae68c0d8c0e00044c848c004008c078d5d0981b8008191baa357426ae88c0d80154ccd5cd1819981b0008991919191919191919191919191919191919191919190919999999999980080b80a8098088078068058048038028018011aba135744004666068eb88004d5d08009aba2002357420026ae88008cc0c9d71aba1001357440046ae84004d5d10011aba1001357440046ae84004d5d10011aba1001357440046ae84004d5d10011981300f1aba1001357440046ae84004d5d1181b001198111192999ab9a30353038001132321233001003002301d357426ae88c0e0008c078d5d0981b8008191baa00135742606a0020606ea8d5d0981a001817911a8011111111111111a80691919299aa99a998149aa99a80109815a481035054380022100203d00303903a03a1533501213302549101350033302330340362350012232333027303803a235001223500122533533302b0440040062153353333026303e040223500222533500321533533303104a0030062153353302b0010031303f3305722533500104c221350022253353305100200a100313304d33047002001300600300215335330370010031303f333302d04b0043370200200600409209008e60720020044266060920102313000333573466e20ccd54c0fc104c0a8cc0f1c024000400266aa608008246a00209600200809208e266ae712410231310004813357389201023132000470023335530360393501b0403501b04233355303603922533535002222253353302200800413038003042213303d001002100103f010333301c303403622350022253353303c00b002100313333020303803a235001222533533302a0210030012133330260220043355303e03f235001223303d002333500120012235002223500322330433370000800466aa608e09046a002446608c004666a0024002e008004ccc0c013400c0048004ccc09c11000c0040084cccc09408400c00800400c0040f140044cc0952410134003330233034036235001223303b00a0025001153353355303403523500122350012222302c533350021303104821001213304e2253350011303404a221350022253353304800200710011300600300c0011302a49010136002213355303603723500122350012222302e533350021303304a2100121330502253350011303604c221350022253353304a00200710011300600300e0033335530310342253353353530283500203f03d203f253353303c001330482253350011302e044221350022253353303000200a135302f001223350022303504b20011300600301003b1302c4901013300133037002001100103a00d1120011533573892010350543500165333573460640020502a666ae68c0c400409c0b8c0ccdd50019baa00133019223355301f020235001223301e002335530220232350012233021002333500137009000380233700002900000099aa980f81011a800911980f001199a800919aa981181211a8009119811001180880080091199806815001000919aa981181211a80091198110011809000800999804012801000812111919807198021a8018139a801013a99a9a80181490a99a8011099a801119a80111980400100091101711119a80210171112999ab9a3370e00c0062a666ae68cdc38028010998068020008158158120a99a80090120121a8008141119a801119a8011198128010009014119a801101411981280100091199ab9a3370e00400204604a44446666aa00866032444600660040024002006002004444466aa603803a46a0024466036004666a0024002052400266600a0080026603c66030006004046444666aa603003603866aa603403646a00244660320046010002666aa6030036446a00444a66a666aa603a03e60106603444a66a00404a200204e46a002446601400400a00c200626604000800604200266aa603403646a00244660320046605e44a66a002260160064426a00444a66a6601800401022444660040140082600c00600800446602644666a0060420040026a00204242444600600842444600200844604e44a66a0020364426a00444a66a6601000400e2602a0022600c0064466aa0046602000603600244a66a004200202e44a66a00202e266ae7000806c8c94ccd5cd180f9811000899190919800801801198079192999ab9a3022302500113232123300100300233301075c464a666ae68c094c0a00044c8cc0514cd4cc028005200110011300e4901022d330033301375c464a66a660180029000080089808249022d3200375a0026ae84d5d118140011bad35742604e0020446ea8004d5d09aba23025002300c35742604800203e6ea8004d5d09aba23022002375c6ae84c084004070dd500091199ab9a3371200400203202e46a002444400844a666ae68cdc79a80100b1a80080b0999ab9a3370e6a0040306a00203002a02e024464a666ae68c06cc0780044c8c8c8c8c8c8c8c848cccc00402401c00c008d5d09aba20045333573466e1d2004001132122230020043574260460042a666ae68c0880044c84888c004010dd71aba1302300215333573460420022244400603c60460026ea8d5d08009aba200233300a75c66014eb9d69aba100135744603c004600a6ae84c074004060dd50009299ab9c001162325333573460326038002264646424660020060046eb4d5d09aba2301d003533357346034603a00226eb8d5d0980e00080b9baa35742603600202c6ea80048c94ccd5cd180c180d80089919191909198008028012999ab9a301b00113232300953335734603c00226464646424466600200c0080066eb4d5d09aba2002375a6ae84004d5d118100019bad35742603e0042a666ae68c0740044c8488c00800cc020d5d0980f80100d180f8009baa35742603a0042a666ae68c070004044060c074004dd51aba135744603600460066ae84c068004054dd5000919192999ab9a30190011321223001003375c6ae84c06800854ccd5cd180c00089909118010019bae35742603400402a60340026ea80048488c00800c888cc06888cccd55cf800900911919807198041803980e8009803180e00098021aba2003357420040166eac0048848cc00400c00888cc05c88cccd55cf800900791980518029aba10023003357440040106eb0004c05088448894cd40044008884cc014008ccd54c01c028014010004c04c88448894cd40044d400c040884ccd4014040c010008ccd54c01c024014010004c0488844894cd4004024884cc020c010008cd54c01801c0100044800488488cc00401000cc03c8894cd40080108854cd4cc02000800c01c4cc01400400c4014400888ccd5cd19b8f0020010030051001220021001220011533573892010350543100164901022d31004901013700370e90001b874800955cf2ab9d2323001001223300330020020011";
static const char* NATIVE_REFERENCE_SCRIPT_UTXO         = "82825820bb277abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e003a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d818582282008200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d398538";
static const char* PLUTUS_V1_REFERENCE_SCRIPT_UTXO      = "82825820bb247abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d8185182014e4d01000033222220051200120011";
static const char* PLUTUS_V2_REFERENCE_SCRIPT_UTXO      = "82825820bb257abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e002a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d8185182024e4d02000033222220051200120011";
static const char* PLUTUS_V3_REFERENCE_SCRIPT_UTXO      = "82825820bb267abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e003a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d8185182034e4d03000033222220051200120011";
static const char* PLUTUS_V4_REFERENCE_SCRIPT_UTXO      = "82825820bb287abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e004a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d8185182044e4d04000033222220051200120011";
static const char* NATIVE_SCRIPT_CBOR                   = "82008200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d398538";
static const char* PLUTUS_SCRIPT_CBORS[]                = {
  "82014e4d01000033222220051200120011",
  "82024e4d02000033222220051200120011",
  "82034e4d03000033222220051200120011",
  "82044e4d04000033222220051200120011"
};

/**
 * The size of the script that \ref RESOLVED_INPUT_WITH_REFERENCE_SCRIPT carries: a PlutusV2 script of 2561 bytes, the three
 * bytes of the header of its byte string and the two bytes of the array that holds the language tag and the script.
 */
static const uint64_t LARGE_REFERENCE_SCRIPT_SIZE = 2566U;

/**
 * The size of the script that \ref NATIVE_REFERENCE_SCRIPT_UTXO carries: a native script of 32 bytes that requires one
 * signature and the two bytes of the array that holds the language tag and the script.
 */
static const uint64_t NATIVE_REFERENCE_SCRIPT_SIZE = 34U;

/**
 * The size of the script that each Plutus reference script UTXO carries: a Plutus script of 14 bytes, the byte of the
 * header of its byte string and the two bytes of the array that holds the language tag and the script.
 */
static const uint64_t PLUTUS_REFERENCE_SCRIPT_SIZE = 17U;

/**
 * The size of the first pricing tier of the reference scripts, in bytes. Every further tier costs 1.2 times as much per
 * byte as the previous one.
 */
static const uint64_t REFERENCE_SCRIPT_TIER_SIZE = 25600U;

/**
 * The price of a byte of reference script in the first tier, the one set by \ref create_protocol_parameters.
 */
static const uint64_t REFERENCE_SCRIPT_BYTE_COST = 15U;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Builds a tx output from its CBOR representation.
 * \param cbor The CBOR representation of the tx output.
 * \return A pointer to the tx output.
 */
static cardano_transaction_output_t*
create_transaction_output(const char* cbor)
{
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_transaction_output_t* output = NULL;

  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &output);

  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  return output;
}

/**
 * \brief Builds a tx from its CBOR representation.
 * \param cbor The CBOR representation of the tx.
 * \return A pointer to the tx.
 */
static cardano_transaction_t*
create_transaction(const char* cbor)
{
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_transaction_t* tx = NULL;

  cardano_error_t result = cardano_transaction_from_cbor(reader, &tx);

  cardano_cbor_reader_unref(&reader);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  return tx;
}

/**
 * \brief Builds dummy protocol parameters.
 * \return A pointer to the protocol parameters.
 */
static cardano_protocol_parameters_t*
create_protocol_parameters()
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

  result = cardano_protocol_parameters_set_ada_per_utxo_byte(params, COST_PER_UTXO_BYTE);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_ex_unit_prices_unref(&ex_unit_prices);

  return params;
}

/**
 * \brief Builds a list of utxos.
 * \return The list of utxos.
 */
static cardano_utxo_list_t*
create_resolved_inputs()
{
  cardano_cbor_reader_t* reader1 = cardano_cbor_reader_from_hex(RESOLVED_INPUT, strlen(RESOLVED_INPUT));
  cardano_cbor_reader_t* reader2 = cardano_cbor_reader_from_hex(RESOLVED_INPUT_WITH_REFERENCE_SCRIPT, strlen(RESOLVED_INPUT_WITH_REFERENCE_SCRIPT));

  cardano_utxo_list_t* utxo_list = NULL;

  cardano_error_t result = cardano_utxo_list_new(&utxo_list);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_utxo_t* utxo1 = NULL;
  cardano_utxo_t* utxo2 = NULL;

  result = cardano_utxo_from_cbor(reader1, &utxo1);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_utxo_from_cbor(reader2, &utxo2);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_utxo_list_add(utxo_list, utxo1);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_utxo_list_add(utxo_list, utxo2);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader1);
  cardano_cbor_reader_unref(&reader2);
  cardano_utxo_unref(&utxo1);
  cardano_utxo_unref(&utxo2);

  return utxo_list;
}

/**
 * \brief Checks if a tx has reference inputs.
 *
 * \param tx The tx to check.
 * \return True if the tx has reference inputs, false otherwise.
 */
static bool
has_reference_inputs(cardano_transaction_t* tx)
{
  cardano_transaction_body_t*      body   = cardano_transaction_get_body(tx);
  cardano_transaction_input_set_t* inputs = cardano_transaction_body_get_reference_inputs(body);

  bool has_inputs = cardano_transaction_input_set_get_length(inputs) > 0;

  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_body_unref(&body);

  return has_inputs;
}

/**
 * \brief Builds a list with a single spend redeemer.
 *
 * \param memory The memory units of the redeemer.
 * \param cpu_steps The CPU steps of the redeemer.
 * \return The list of redeemers.
 */
static cardano_redeemer_list_t*
create_redeemers(const uint64_t memory, const uint64_t cpu_steps)
{
  cardano_redeemer_list_t* redeemers = NULL;
  cardano_redeemer_t*      redeemer  = NULL;
  cardano_plutus_data_t*   data      = NULL;
  cardano_ex_units_t*      ex_units  = NULL;

  EXPECT_EQ(cardano_redeemer_list_new(&redeemers), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_data_new_integer_from_int(42, &data), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ex_units_new(memory, cpu_steps, &ex_units), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_new(CARDANO_REDEEMER_TAG_SPEND, 0, data, ex_units, &redeemer), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_redeemer_list_add(redeemers, redeemer), CARDANO_SUCCESS);

  cardano_plutus_data_unref(&data);
  cardano_ex_units_unref(&ex_units);
  cardano_redeemer_unref(&redeemer);

  return redeemers;
}

/**
 * \brief Builds a sub transaction that spends a single input.
 *
 * \param input_index The index of the input it spends, which makes its id unique.
 * \param redeemers The redeemers of its witness set, or NULL if it has none.
 * \return The sub transaction.
 */
static cardano_sub_transaction_t*
create_sub_transaction(const uint64_t input_index, cardano_redeemer_list_t* redeemers)
{
  cardano_transaction_input_t*       input           = NULL;
  cardano_transaction_input_set_t*   inputs          = NULL;
  cardano_transaction_output_list_t* outputs         = NULL;
  cardano_sub_transaction_body_t*    body            = NULL;
  cardano_witness_set_t*             witness_set     = NULL;
  cardano_sub_transaction_t*         sub_transaction = NULL;

  EXPECT_EQ(cardano_transaction_input_from_hex(SUB_TX_INPUT_ID, strlen(SUB_TX_INPUT_ID), input_index, &input), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_new(&inputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_add(inputs, input), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_list_new(&outputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_new(inputs, outputs, NULL, &body), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_witness_set_new(&witness_set), CARDANO_SUCCESS);

  if (redeemers != NULL)
  {
    EXPECT_EQ(cardano_witness_set_set_redeemers(witness_set, redeemers), CARDANO_SUCCESS);
  }

  EXPECT_EQ(cardano_sub_transaction_new(body, witness_set, NULL, &sub_transaction), CARDANO_SUCCESS);

  cardano_transaction_input_unref(&input);
  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_output_list_unref(&outputs);
  cardano_sub_transaction_body_unref(&body);
  cardano_witness_set_unref(&witness_set);

  return sub_transaction;
}

/**
 * \brief Adds a sub transaction with a single redeemer to a tx.
 *
 * \param tx The tx that carries the sub transaction.
 * \param input_index The index of the input the sub transaction spends.
 * \param memory The memory units of the redeemer, or zero together with the CPU steps for a sub transaction without redeemers.
 * \param cpu_steps The CPU steps of the redeemer.
 */
static void
add_sub_transaction(cardano_transaction_t* tx, const uint64_t input_index, const uint64_t memory, const uint64_t cpu_steps)
{
  cardano_redeemer_list_t*       redeemers        = ((memory > 0U) || (cpu_steps > 0U)) ? create_redeemers(memory, cpu_steps) : NULL;
  cardano_sub_transaction_t*     sub_transaction  = create_sub_transaction(input_index, redeemers);
  cardano_transaction_body_t*    body             = cardano_transaction_get_body(tx);
  cardano_sub_transaction_set_t* sub_transactions = cardano_transaction_body_get_sub_transactions(body);

  if (sub_transactions == NULL)
  {
    EXPECT_EQ(cardano_sub_transaction_set_new(&sub_transactions), CARDANO_SUCCESS);
  }

  EXPECT_EQ(cardano_sub_transaction_set_add(sub_transactions, sub_transaction), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_sub_transactions(body, sub_transactions), CARDANO_SUCCESS);

  cardano_transaction_clear_cbor_cache(tx);

  cardano_redeemer_list_unref(&redeemers);
  cardano_sub_transaction_unref(&sub_transaction);
  cardano_sub_transaction_set_unref(&sub_transactions);
  cardano_transaction_body_unref(&body);
}

/**
 * \brief Computes the fee of a given amount of execution units.
 *
 * \param params The protocol parameters with the execution unit prices.
 * \param memory The memory units.
 * \param cpu_steps The CPU steps.
 * \return The fee of the execution units.
 */
static uint64_t
compute_ex_units_fee(cardano_protocol_parameters_t* params, const uint64_t memory, const uint64_t cpu_steps)
{
  cardano_ex_unit_prices_t* prices        = cardano_protocol_parameters_get_execution_costs(params);
  cardano_unit_interval_t*  memory_prices = NULL;
  cardano_unit_interval_t*  steps_prices  = NULL;

  EXPECT_EQ(cardano_ex_unit_prices_get_memory_prices(prices, &memory_prices), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ex_unit_prices_get_steps_prices(prices, &steps_prices), CARDANO_SUCCESS);

  const double memory_price = cardano_unit_interval_to_double(memory_prices);
  const double steps_price  = cardano_unit_interval_to_double(steps_prices);

  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
  cardano_ex_unit_prices_unref(&prices);

  return (uint64_t)ceil(((double)cpu_steps * steps_price) + ((double)memory * memory_price));
}

/**
 * \brief Computes the script fee of a tx with the prices of the given protocol parameters.
 *
 * \param tx The tx.
 * \param params The protocol parameters.
 * \param resolved_reference_inputs The resolved reference inputs.
 * \param min_fee The computed script fee.
 * \return The result of the computation.
 */
static cardano_error_t
compute_min_script_fee(
  cardano_transaction_t*         tx,
  cardano_protocol_parameters_t* params,
  cardano_utxo_list_t*           resolved_reference_inputs,
  uint64_t*                      min_fee)
{
  cardano_ex_unit_prices_t* prices          = cardano_protocol_parameters_get_execution_costs(params);
  cardano_unit_interval_t*  script_ref_cost = cardano_protocol_parameters_get_ref_script_cost_per_byte(params);

  cardano_error_t result = cardano_compute_min_script_fee(tx, prices, resolved_reference_inputs, script_ref_cost, min_fee);

  cardano_ex_unit_prices_unref(&prices);
  cardano_unit_interval_unref(&script_ref_cost);

  return result;
}

/**
 * \brief Builds a list with the utxo that carries a reference script, repeated a number of times.
 *
 * \param count The number of times the utxo is listed.
 * \return The list of utxos.
 */
static cardano_utxo_list_t*
create_reference_script_inputs(const size_t count)
{
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex(RESOLVED_INPUT_WITH_REFERENCE_SCRIPT, strlen(RESOLVED_INPUT_WITH_REFERENCE_SCRIPT));
  cardano_utxo_list_t*   utxo_list = NULL;
  cardano_utxo_t*        utxo      = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&utxo_list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_from_cbor(reader, &utxo), CARDANO_SUCCESS);

  for (size_t i = 0; i < count; ++i)
  {
    EXPECT_EQ(cardano_utxo_list_add(utxo_list, utxo), CARDANO_SUCCESS);
  }

  cardano_cbor_reader_unref(&reader);
  cardano_utxo_unref(&utxo);

  return utxo_list;
}

/**
 * \brief Builds a script from its CBOR representation.
 *
 * \param cbor The CBOR representation of the script.
 * \return A pointer to the script.
 */
static cardano_script_t*
create_script(const char* cbor)
{
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_script_t*      script = NULL;

  EXPECT_EQ(cardano_script_from_cbor(reader, &script), CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return script;
}

/**
 * \brief Adds a utxo to a list from its CBOR representation.
 *
 * \param utxo_list The list that receives the utxo.
 * \param cbor The CBOR representation of the utxo.
 */
static void
add_utxo(cardano_utxo_list_t* utxo_list, const char* cbor)
{
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_utxo_t*        utxo   = NULL;

  EXPECT_EQ(cardano_utxo_from_cbor(reader, &utxo), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(utxo_list, utxo), CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);
  cardano_utxo_unref(&utxo);
}

/**
 * \brief Builds a list with one utxo per script language, each of them carrying a reference script, plus the utxo that
 * carries the large reference script.
 *
 * \return The list of utxos.
 */
static cardano_utxo_list_t*
create_mixed_language_reference_script_inputs()
{
  cardano_utxo_list_t* utxo_list = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&utxo_list), CARDANO_SUCCESS);

  add_utxo(utxo_list, NATIVE_REFERENCE_SCRIPT_UTXO);
  add_utxo(utxo_list, PLUTUS_V1_REFERENCE_SCRIPT_UTXO);
  add_utxo(utxo_list, PLUTUS_V2_REFERENCE_SCRIPT_UTXO);
  add_utxo(utxo_list, PLUTUS_V3_REFERENCE_SCRIPT_UTXO);
  add_utxo(utxo_list, PLUTUS_V4_REFERENCE_SCRIPT_UTXO);
  add_utxo(utxo_list, RESOLVED_INPUT_WITH_REFERENCE_SCRIPT);

  return utxo_list;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_fee_compute_min_ada_required, correctlyCorrectMinAdaFromVectors)
{
  cardano_protocol_parameters_t* params = create_protocol_parameters();

  for (size_t i = 0; i < sizeof(min_ada_required_vectors) / sizeof(min_ada_required_vectors[0]); ++i)
  {
    const fee_vector_t* vector = &min_ada_required_vectors[i];

    cardano_transaction_output_t* output = create_transaction_output(vector->cbor);

    uint64_t min_ada_required = 0U;

    cardano_error_t result = cardano_compute_min_ada_required(
      output,
      cardano_protocol_parameters_get_ada_per_utxo_byte(params),
      &min_ada_required);

    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_EQ(min_ada_required, vector->fee);

    cardano_transaction_output_unref(&output);
  }

  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_transaction_fee, correctlyComputesTxFeesForTestVectors)
{
  cardano_protocol_parameters_t* params = create_protocol_parameters();

  for (size_t i = 0; i < sizeof(tx_fee_vectors) / sizeof(tx_fee_vectors[0]); ++i)
  {
    const fee_vector_t* vector = &tx_fee_vectors[i];

    cardano_transaction_t* tx        = create_transaction(vector->cbor);
    cardano_utxo_list_t*   utxo_list = NULL;
    cardano_error_t        result    = CARDANO_SUCCESS;

    if (has_reference_inputs(tx))
    {
      utxo_list = create_resolved_inputs();

      EXPECT_EQ(result, CARDANO_SUCCESS);
    }
    else
    {
      result = cardano_utxo_list_new(&utxo_list);
      EXPECT_EQ(result, CARDANO_SUCCESS);
    }

    uint64_t fee = 0U;

    result = cardano_compute_transaction_fee(
      tx,
      utxo_list,
      params,
      &fee);

    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_EQ(fee, vector->fee);

    cardano_transaction_unref(&tx);
    cardano_utxo_list_unref(&utxo_list);
  }

  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_get_serialized_coin_size, returnsErrorIfOutParamIsNull)
{
  cardano_error_t result = cardano_get_serialized_coin_size(0, NULL);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_get_serialized_coin_size, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  size_t          size   = 0U;
  cardano_error_t result = cardano_get_serialized_coin_size(0, &size);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_fee_get_serialized_output_size, returnsErrorIfOutParamIsNull)
{
  cardano_error_t result = cardano_get_serialized_output_size((cardano_transaction_output_t*)"", NULL);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_get_serialized_output_size, returnsErrorIfOutputIsNull)
{
  size_t          size   = 0U;
  cardano_error_t result = cardano_get_serialized_output_size(NULL, &size);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_get_serialized_output_size, returnsErrorIfMemoryAllocationFails)
{
  cardano_transaction_output_t* output = create_transaction_output(min_ada_required_vectors[0].cbor);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  size_t          size   = 0U;
  cardano_error_t result = cardano_get_serialized_output_size(output, &size);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_transaction_output_unref(&output);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_fee_gget_serialized_script_size, returnsErrorIfOutParamIsNull)
{
  cardano_error_t result = cardano_get_serialized_script_size((cardano_script_t*)"", NULL);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_gget_serialized_script_size, returnsErrorIfScriptIsNull)
{
  size_t          size   = 0U;
  cardano_error_t result = cardano_get_serialized_script_size(NULL, &size);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_get_serialized_script_size, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  size_t size = 0U;

  cardano_error_t result = cardano_get_serialized_script_size((cardano_script_t*)"", &size);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_fee_get_serialized_script_size, measuresANativeScriptTogetherWithItsLanguageTag)
{
  // Arrange
  cardano_script_t* script = create_script(NATIVE_SCRIPT_CBOR);
  size_t            size   = 0U;

  // Act
  cardano_error_t result = cardano_get_serialized_script_size(script, &size);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(size, strlen(NATIVE_SCRIPT_CBOR) / 2U);
  EXPECT_EQ(size, NATIVE_REFERENCE_SCRIPT_SIZE);

  // Cleanup
  cardano_script_unref(&script);
}

TEST(cardano_fee_get_serialized_script_size, measuresAPlutusScriptOfEveryVersionTogetherWithItsLanguageTag)
{
  for (size_t i = 0; i < sizeof(PLUTUS_SCRIPT_CBORS) / sizeof(PLUTUS_SCRIPT_CBORS[0]); ++i)
  {
    // Arrange
    cardano_script_t* script = create_script(PLUTUS_SCRIPT_CBORS[i]);
    size_t            size   = 0U;

    // Act
    cardano_error_t result = cardano_get_serialized_script_size(script, &size);

    // Assert
    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_EQ(size, strlen(PLUTUS_SCRIPT_CBORS[i]) / 2U);
    EXPECT_EQ(size, PLUTUS_REFERENCE_SCRIPT_SIZE);

    // Cleanup
    cardano_script_unref(&script);
  }
}

TEST(cardano_fee_get_serialized_transaction_size, returnsErrorIfOutParamIsNull)
{
  cardano_error_t result = cardano_get_serialized_transaction_size((cardano_transaction_t*)"", NULL);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_get_serialized_transaction_size, returnsErrorIfTransactionIsNull)
{
  size_t          size   = 0U;
  cardano_error_t result = cardano_get_serialized_transaction_size(NULL, &size);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_get_serialized_transaction_size, returnsErrorIfMemoryAllocationFails)
{
  cardano_transaction_t* tx = create_transaction(tx_fee_vectors[0].cbor);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  size_t          size   = 0U;
  cardano_error_t result = cardano_get_serialized_transaction_size(tx, &size);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_transaction_unref(&tx);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_fee_get_total_ex_units_in_redeemers, returnsErrorIfOutParamIsNull)
{
  cardano_error_t result = cardano_get_total_ex_units_in_redeemers((cardano_redeemer_list_t*)"", NULL);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_get_total_ex_units_in_redeemers, returnsErrorIfRedeemersIsNull)
{
  cardano_error_t result = cardano_get_total_ex_units_in_redeemers(NULL, (cardano_ex_units_t**)"");

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_get_total_ex_units_in_redeemers, returnsErrorIfMemoryAllocationFails)
{
  cardano_redeemer_list_t* redeemers = NULL;

  cardano_error_t result = cardano_redeemer_list_new(&redeemers);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_ex_units_t* ex_units = NULL;

  result = cardano_get_total_ex_units_in_redeemers(redeemers, &ex_units);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_redeemer_list_unref(&redeemers);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_fee_compute_script_ref_fee, returnsErrorIfFirstParamIsNull)
{
  cardano_error_t result = cardano_compute_script_ref_fee(NULL, (cardano_unit_interval_t*)"", (uint64_t*)"");

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_compute_script_ref_fee, returnsErrorIfSecondParamIsNull)
{
  cardano_error_t result = cardano_compute_script_ref_fee((cardano_utxo_list_t*)"", NULL, (uint64_t*)"");

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_compute_script_ref_fee, returnsErrorIfThirdParamIsNull)
{
  cardano_error_t result = cardano_compute_script_ref_fee((cardano_utxo_list_t*)"", (cardano_unit_interval_t*)"", NULL);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_compute_script_ref_fee, returnsErrorIfScriptRefCostIsNull)
{
  cardano_unit_interval_t* script_ref_cost = NULL;

  cardano_error_t result = cardano_unit_interval_from_double(15.0, &script_ref_cost);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  uint64_t fee = 0U;

  result = cardano_compute_script_ref_fee(NULL, script_ref_cost, &fee);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_unit_interval_unref(&script_ref_cost);
}

TEST(cardano_fee_compute_min_script_fee, returnsErrorIfFirstParamIsNull)
{
  cardano_error_t result = cardano_compute_min_script_fee(NULL, (cardano_ex_unit_prices_t*)"", (cardano_utxo_list_t*)"", (cardano_unit_interval_t*)"", (uint64_t*)"");

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_compute_min_script_fee, returnsErrorIfSecondParamIsNull)
{
  cardano_error_t result = cardano_compute_min_script_fee((cardano_transaction_t*)"", NULL, (cardano_utxo_list_t*)"", (cardano_unit_interval_t*)"", (uint64_t*)"");

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_compute_min_script_fee, returnsErrorIfThirdParamIsNull)
{
  cardano_error_t result = cardano_compute_min_script_fee((cardano_transaction_t*)"", (cardano_ex_unit_prices_t*)"", NULL, (cardano_unit_interval_t*)"", (uint64_t*)"");

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_compute_min_script_fee, returnsErrorIfFourthParamIsNull)
{
  cardano_error_t result = cardano_compute_min_script_fee((cardano_transaction_t*)"", (cardano_ex_unit_prices_t*)"", (cardano_utxo_list_t*)"", NULL, (uint64_t*)"");

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_compute_min_script_fee, returnsErrorIfOutParamIsNull)
{
  cardano_error_t result = cardano_compute_min_script_fee((cardano_transaction_t*)"", (cardano_ex_unit_prices_t*)"", (cardano_utxo_list_t*)"", (cardano_unit_interval_t*)"", NULL);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_compute_min_script_fee, pricesTheReferenceScriptsIfTheBatchHasNoRedeemers)
{
  // Arrange
  cardano_protocol_parameters_t* params    = create_protocol_parameters();
  cardano_transaction_t*         tx        = create_transaction(tx_fee_vectors[0].cbor);
  cardano_utxo_list_t*           utxo_list = create_reference_script_inputs(1);
  uint64_t                       fee       = 1U;

  add_sub_transaction(tx, 1, 0, 0);

  // Act
  cardano_error_t result = compute_min_script_fee(tx, params, utxo_list, &fee);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(fee, LARGE_REFERENCE_SCRIPT_SIZE * REFERENCE_SCRIPT_BYTE_COST);
  EXPECT_EQ(fee, 38490U);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_min_script_fee, returnsZeroIfThereAreNoRedeemersAndNoReferenceScripts)
{
  // Arrange
  cardano_protocol_parameters_t* params    = create_protocol_parameters();
  cardano_transaction_t*         tx        = create_transaction(tx_fee_vectors[0].cbor);
  cardano_utxo_list_t*           utxo_list = NULL;
  uint64_t                       fee       = 1U;

  EXPECT_EQ(cardano_utxo_list_new(&utxo_list), CARDANO_SUCCESS);

  add_utxo(utxo_list, RESOLVED_INPUT);
  add_sub_transaction(tx, 1, 0, 0);

  // Act
  cardano_error_t result = compute_min_script_fee(tx, params, utxo_list, &fee);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(fee, 0U);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_min_script_fee, pricesANativeReferenceScriptIfTheTransactionHasNoRedeemers)
{
  // Arrange
  cardano_protocol_parameters_t* params    = create_protocol_parameters();
  cardano_transaction_t*         tx        = create_transaction(tx_fee_vectors[0].cbor);
  cardano_utxo_list_t*           utxo_list = NULL;
  uint64_t                       fee       = 0U;

  EXPECT_EQ(cardano_utxo_list_new(&utxo_list), CARDANO_SUCCESS);

  add_utxo(utxo_list, NATIVE_REFERENCE_SCRIPT_UTXO);

  // Act
  cardano_error_t result = compute_min_script_fee(tx, params, utxo_list, &fee);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(fee, NATIVE_REFERENCE_SCRIPT_SIZE * REFERENCE_SCRIPT_BYTE_COST);
  EXPECT_EQ(fee, 510U);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_min_script_fee, addsTheReferenceScriptsToTheExecutionUnitsOfTheRedeemers)
{
  // Arrange
  cardano_protocol_parameters_t* params    = create_protocol_parameters();
  cardano_transaction_t*         tx        = create_transaction(tx_fee_vectors[1].cbor);
  cardano_utxo_list_t*           utxo_list = create_mixed_language_reference_script_inputs();
  uint64_t                       fee       = 0U;

  // Act
  cardano_error_t result = compute_min_script_fee(tx, params, utxo_list, &fee);

  // Assert
  const uint64_t total_size = NATIVE_REFERENCE_SCRIPT_SIZE + (4U * PLUTUS_REFERENCE_SCRIPT_SIZE) + LARGE_REFERENCE_SCRIPT_SIZE;

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(fee, compute_ex_units_fee(params, 8000, 9000) + (total_size * REFERENCE_SCRIPT_BYTE_COST));

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_min_script_fee, addsTheExecutionUnitsOfASubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params    = create_protocol_parameters();
  cardano_transaction_t*         tx        = create_transaction(tx_fee_vectors[0].cbor);
  cardano_utxo_list_t*           utxo_list = NULL;
  uint64_t                       fee       = 0U;

  EXPECT_EQ(cardano_utxo_list_new(&utxo_list), CARDANO_SUCCESS);

  add_sub_transaction(tx, 1, 1000000, 200000000);

  // Act
  cardano_error_t result = compute_min_script_fee(tx, params, utxo_list, &fee);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(fee, 72120U);
  EXPECT_EQ(fee, compute_ex_units_fee(params, 1000000, 200000000));

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_min_script_fee, addsTheExecutionUnitsOfSeveralSubTransactionsToTheTopLevelOnes)
{
  // Arrange
  cardano_protocol_parameters_t* params    = create_protocol_parameters();
  cardano_transaction_t*         tx        = create_transaction(tx_fee_vectors[1].cbor);
  cardano_utxo_list_t*           utxo_list = NULL;
  uint64_t                       top_fee   = 0U;
  uint64_t                       batch_fee = 0U;

  EXPECT_EQ(cardano_utxo_list_new(&utxo_list), CARDANO_SUCCESS);
  EXPECT_EQ(compute_min_script_fee(tx, params, utxo_list, &top_fee), CARDANO_SUCCESS);

  add_sub_transaction(tx, 1, 1000001, 200000001);
  add_sub_transaction(tx, 2, 0, 0);
  add_sub_transaction(tx, 3, 2000003, 300000007);

  // Act
  cardano_error_t result = compute_min_script_fee(tx, params, utxo_list, &batch_fee);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(top_fee, compute_ex_units_fee(params, 8000, 9000));
  EXPECT_EQ(batch_fee, compute_ex_units_fee(params, 8000 + 1000001 + 2000003, 9000 + 200000001 + 300000007));
  EXPECT_GT(batch_fee, top_fee);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_min_script_fee, isUnchangedBySubTransactionsWithoutRedeemers)
{
  // Arrange
  cardano_protocol_parameters_t* params    = create_protocol_parameters();
  cardano_transaction_t*         tx        = create_transaction(tx_fee_vectors[2].cbor);
  cardano_utxo_list_t*           utxo_list = create_resolved_inputs();
  uint64_t                       top_fee   = 0U;
  uint64_t                       batch_fee = 0U;

  EXPECT_EQ(compute_min_script_fee(tx, params, utxo_list, &top_fee), CARDANO_SUCCESS);

  add_sub_transaction(tx, 1, 0, 0);
  add_sub_transaction(tx, 2, 0, 0);

  // Act
  cardano_error_t result = compute_min_script_fee(tx, params, utxo_list, &batch_fee);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_GT(top_fee, 0U);
  EXPECT_EQ(batch_fee, top_fee);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_min_script_fee, pricesTheReferenceScriptsIfOnlyASubTransactionHasRedeemers)
{
  // Arrange
  cardano_protocol_parameters_t* params          = create_protocol_parameters();
  cardano_unit_interval_t*       script_ref_cost = cardano_protocol_parameters_get_ref_script_cost_per_byte(params);
  cardano_transaction_t*         tx              = create_transaction(tx_fee_vectors[0].cbor);
  cardano_utxo_list_t*           utxo_list       = create_reference_script_inputs(1);
  uint64_t                       ref_script_fee  = 0U;
  uint64_t                       fee             = 0U;

  EXPECT_EQ(cardano_compute_script_ref_fee(utxo_list, script_ref_cost, &ref_script_fee), CARDANO_SUCCESS);

  add_sub_transaction(tx, 1, 1000000, 200000000);

  // Act
  cardano_error_t result = compute_min_script_fee(tx, params, utxo_list, &fee);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_GT(ref_script_fee, 0U);
  EXPECT_EQ(fee, compute_ex_units_fee(params, 1000000, 200000000) + ref_script_fee);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxo_list);
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_min_script_fee, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_protocol_parameters_t* params    = create_protocol_parameters();
  cardano_transaction_t*         tx        = create_transaction(tx_fee_vectors[1].cbor);
  cardano_utxo_list_t*           utxo_list = NULL;
  uint64_t                       fee       = 0U;

  EXPECT_EQ(cardano_utxo_list_new(&utxo_list), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = compute_min_script_fee(tx, params, utxo_list, &fee);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_min_script_fee, returnsErrorIfMemoryAllocationFailsForASubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* params    = create_protocol_parameters();
  cardano_transaction_t*         tx        = create_transaction(tx_fee_vectors[1].cbor);
  cardano_utxo_list_t*           utxo_list = NULL;
  uint64_t                       fee       = 0U;

  EXPECT_EQ(cardano_utxo_list_new(&utxo_list), CARDANO_SUCCESS);

  add_sub_transaction(tx, 1, 1000000, 200000000);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t result = compute_min_script_fee(tx, params, utxo_list, &fee);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_script_ref_fee, countsAReferenceScriptListedTwiceTwice)
{
  // Arrange
  cardano_unit_interval_t* script_ref_cost = NULL;
  cardano_utxo_list_t*     listed_once     = create_reference_script_inputs(1);
  cardano_utxo_list_t*     listed_twice    = create_reference_script_inputs(2);
  uint64_t                 fee_once        = 0U;
  uint64_t                 fee_twice       = 0U;

  EXPECT_EQ(cardano_unit_interval_from_double(15.0, &script_ref_cost), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_compute_script_ref_fee(listed_once, script_ref_cost, &fee_once), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_compute_script_ref_fee(listed_twice, script_ref_cost, &fee_twice), CARDANO_SUCCESS);

  // Assert
  EXPECT_EQ(fee_once, 2566U * 15U);
  EXPECT_EQ(fee_twice, 2U * fee_once);

  // Cleanup
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_utxo_list_unref(&listed_once);
  cardano_utxo_list_unref(&listed_twice);
}

TEST(cardano_fee_compute_script_ref_fee, pricesANativeReferenceScript)
{
  // Arrange
  cardano_unit_interval_t* script_ref_cost = NULL;
  cardano_utxo_list_t*     utxo_list       = NULL;
  uint64_t                 fee             = 0U;

  EXPECT_EQ(cardano_unit_interval_from_double(15.0, &script_ref_cost), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_new(&utxo_list), CARDANO_SUCCESS);

  add_utxo(utxo_list, NATIVE_REFERENCE_SCRIPT_UTXO);

  // Act
  cardano_error_t result = cardano_compute_script_ref_fee(utxo_list, script_ref_cost, &fee);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(fee, NATIVE_REFERENCE_SCRIPT_SIZE * REFERENCE_SCRIPT_BYTE_COST);
  EXPECT_EQ(fee, 510U);

  // Cleanup
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_utxo_list_unref(&utxo_list);
}

TEST(cardano_fee_compute_script_ref_fee, pricesTheReferenceScriptsOfEveryLanguage)
{
  // Arrange
  cardano_unit_interval_t* script_ref_cost = NULL;
  cardano_utxo_list_t*     utxo_list       = create_mixed_language_reference_script_inputs();
  uint64_t                 fee             = 0U;

  EXPECT_EQ(cardano_unit_interval_from_double(15.0, &script_ref_cost), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_compute_script_ref_fee(utxo_list, script_ref_cost, &fee);

  // Assert
  const uint64_t total_size = NATIVE_REFERENCE_SCRIPT_SIZE + (4U * PLUTUS_REFERENCE_SCRIPT_SIZE) + LARGE_REFERENCE_SCRIPT_SIZE;

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(total_size, 2668U);
  EXPECT_EQ(fee, total_size * REFERENCE_SCRIPT_BYTE_COST);
  EXPECT_EQ(fee, 40020U);

  // Cleanup
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_utxo_list_unref(&utxo_list);
}

TEST(cardano_fee_compute_script_ref_fee, pricesTheBytesOfTheSecondTierAtAHigherRate)
{
  // Arrange
  cardano_unit_interval_t* script_ref_cost = NULL;
  cardano_utxo_list_t*     utxo_list       = create_reference_script_inputs(11);
  uint64_t                 fee             = 0U;

  EXPECT_EQ(cardano_unit_interval_from_double(15.0, &script_ref_cost), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_compute_script_ref_fee(utxo_list, script_ref_cost, &fee);

  // Assert
  const uint64_t total_size       = 11U * LARGE_REFERENCE_SCRIPT_SIZE;
  const uint64_t second_tier_size = total_size - REFERENCE_SCRIPT_TIER_SIZE;

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(second_tier_size, 2626U);
  EXPECT_EQ(fee, (REFERENCE_SCRIPT_TIER_SIZE * REFERENCE_SCRIPT_BYTE_COST) + (second_tier_size * 18U));
  EXPECT_EQ(fee, 431268U);

  // Cleanup
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_utxo_list_unref(&utxo_list);
}

TEST(cardano_fee_compute_transaction_fee, pricesTheReferenceScriptsOfATransactionWithoutRedeemers)
{
  // Arrange
  cardano_protocol_parameters_t* params        = create_protocol_parameters();
  cardano_transaction_t*         tx            = create_transaction(tx_fee_vectors[0].cbor);
  cardano_utxo_list_t*           utxo_list     = create_mixed_language_reference_script_inputs();
  cardano_utxo_list_t*           no_utxos      = NULL;
  uint64_t                       fee           = 0U;
  uint64_t                       no_script_fee = 0U;

  EXPECT_EQ(cardano_utxo_list_new(&no_utxos), CARDANO_SUCCESS);

  // Act
  cardano_error_t result           = cardano_compute_transaction_fee(tx, utxo_list, params, &fee);
  cardano_error_t no_script_result = cardano_compute_transaction_fee(tx, no_utxos, params, &no_script_fee);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(no_script_result, CARDANO_SUCCESS);
  EXPECT_EQ(no_script_fee, tx_fee_vectors[0].fee);
  EXPECT_EQ(fee, tx_fee_vectors[0].fee + 40020U);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxo_list);
  cardano_utxo_list_unref(&no_utxos);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_transaction_fee, coversTheExecutionUnitsAndTheReferenceScriptsOfTheSubTransactions)
{
  // Arrange
  cardano_protocol_parameters_t* params          = create_protocol_parameters();
  cardano_unit_interval_t*       script_ref_cost = cardano_protocol_parameters_get_ref_script_cost_per_byte(params);
  cardano_transaction_t*         tx              = create_transaction(tx_fee_vectors[0].cbor);
  cardano_utxo_list_t*           utxo_list       = create_reference_script_inputs(2);
  uint64_t                       ref_script_fee  = 0U;
  uint64_t                       size_fee        = 0U;
  uint64_t                       fee             = 0U;

  add_sub_transaction(tx, 1, 1000000, 200000000);
  add_sub_transaction(tx, 2, 500000, 100000000);

  EXPECT_EQ(cardano_compute_script_ref_fee(utxo_list, script_ref_cost, &ref_script_fee), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_compute_min_fee_without_scripts(tx, 155381, 44, &size_fee), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_compute_transaction_fee(tx, utxo_list, params, &fee);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_GT(size_fee, tx_fee_vectors[0].fee);
  EXPECT_EQ(fee, size_fee + compute_ex_units_fee(params, 1500000, 300000000) + ref_script_fee);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxo_list);
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_ccompute_min_fee_without_scripts, returnsErrorIfFirstParamIsNull)
{
  cardano_error_t result = cardano_compute_min_fee_without_scripts(NULL, 0, 0, (uint64_t*)"");

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_ccompute_min_fee_without_scripts, returnsErrorIfOutParamIsNull)
{
  cardano_error_t result = cardano_compute_min_fee_without_scripts((cardano_transaction_t*)"", 0, 0, NULL);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_compute_transaction_fee, returnsErrorIfTransactionIsNull)
{
  cardano_utxo_list_t*           utxo_list = create_resolved_inputs();
  cardano_protocol_parameters_t* params    = create_protocol_parameters();

  uint64_t fee = 0U;

  cardano_error_t result = cardano_compute_transaction_fee(NULL, utxo_list, params, &fee);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_transaction_fee, returnsErrorIfUtxoListIsNull)
{
  cardano_transaction_t*         tx     = create_transaction(tx_fee_vectors[0].cbor);
  cardano_protocol_parameters_t* params = create_protocol_parameters();

  uint64_t fee = 0U;

  cardano_error_t result = cardano_compute_transaction_fee(tx, NULL, params, &fee);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_transaction_fee, returnsErrorIfProtocolParametersIsNull)
{
  cardano_transaction_t* tx        = create_transaction(tx_fee_vectors[0].cbor);
  cardano_utxo_list_t*   utxo_list = create_resolved_inputs();

  uint64_t fee = 0U;

  cardano_error_t result = cardano_compute_transaction_fee(tx, utxo_list, NULL, &fee);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxo_list);
}

TEST(cardano_fee_compute_transaction_fee, returnsErrorIfOutParamIsNull)
{
  cardano_transaction_t*         tx        = create_transaction(tx_fee_vectors[0].cbor);
  cardano_utxo_list_t*           utxo_list = create_resolved_inputs();
  cardano_protocol_parameters_t* params    = create_protocol_parameters();

  cardano_error_t result = cardano_compute_transaction_fee(tx, utxo_list, params, NULL);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_min_ada_required, returnsErrorIfOutputIsNull)
{
  uint64_t        min_ada_required = 0U;
  cardano_error_t result           = cardano_compute_min_ada_required(NULL, 0, &min_ada_required);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_compute_min_ada_required, returnsErrorIfOutParamIsNull)
{
  cardano_transaction_output_t* output = create_transaction_output(min_ada_required_vectors[0].cbor);

  cardano_error_t result = cardano_compute_min_ada_required(output, 0, NULL);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  cardano_transaction_output_unref(&output);
}

TEST(cardano_fee_compute_min_ada_required, returnsErrorIfMemoryAllocationFails)
{
  cardano_transaction_output_t* output = create_transaction_output(min_ada_required_vectors[0].cbor);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  uint64_t min_ada_required = 0U;

  cardano_error_t result = cardano_compute_min_ada_required(output, 0, &min_ada_required);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_transaction_output_unref(&output);
  cardano_set_allocators(malloc, realloc, free);
}
