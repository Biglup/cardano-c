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

static const char* RESOLVED_INPUT                       = "82825820fbecbe69bc3ee617653b95893f50b0362cbaff3e27b01a936969a25bfc100a7c00835839319068a7a3f008803edac87af1619860f2cdcde40c26987325ace138ad2c967f4bd28944b06462e13c5e3f5d5fa6e03f8567569438cd833e6d1a0a3140c05820c6b9e0671fef714142bda45beedf7b51c2d4e3676f79196964082fef164ef7e4";
static const char* RESOLVED_INPUT_WITH_REFERENCE_SCRIPT = "828258209a32459bd4ef6bbafdeb8cf3b909d0e3e2ec806e4cc6268529280b0fc1d06f5b00a3005839119068a7a3f008803edac87af1619860f2cdcde40c26987325ace138ad81728e7ed4cf324e1323135e7e6d931f01e30792d9cdf17129cb806d011a02625a0003d818590a068202590a015909fe010000323232323232323232323232323232323232323232323232323232323232323232323232323232323232323232323232222323232533535533357346064606a0062646464642466002008004a666ae68c0d8c0e00044c848c004008c078d5d0981b8008191baa357426ae88c0d80154ccd5cd1819981b0008991919191919191919191919191919191919191919190919999999999980080b80a8098088078068058048038028018011aba135744004666068eb88004d5d08009aba2002357420026ae88008cc0c9d71aba1001357440046ae84004d5d10011aba1001357440046ae84004d5d10011aba1001357440046ae84004d5d10011981300f1aba1001357440046ae84004d5d1181b001198111192999ab9a30353038001132321233001003002301d357426ae88c0e0008c078d5d0981b8008191baa00135742606a0020606ea8d5d0981a001817911a8011111111111111a80691919299aa99a998149aa99a80109815a481035054380022100203d00303903a03a1533501213302549101350033302330340362350012232333027303803a235001223500122533533302b0440040062153353333026303e040223500222533500321533533303104a0030062153353302b0010031303f3305722533500104c221350022253353305100200a100313304d33047002001300600300215335330370010031303f333302d04b0043370200200600409209008e60720020044266060920102313000333573466e20ccd54c0fc104c0a8cc0f1c024000400266aa608008246a00209600200809208e266ae712410231310004813357389201023132000470023335530360393501b0403501b04233355303603922533535002222253353302200800413038003042213303d001002100103f010333301c303403622350022253353303c00b002100313333020303803a235001222533533302a0210030012133330260220043355303e03f235001223303d002333500120012235002223500322330433370000800466aa608e09046a002446608c004666a0024002e008004ccc0c013400c0048004ccc09c11000c0040084cccc09408400c00800400c0040f140044cc0952410134003330233034036235001223303b00a0025001153353355303403523500122350012222302c533350021303104821001213304e2253350011303404a221350022253353304800200710011300600300c0011302a49010136002213355303603723500122350012222302e533350021303304a2100121330502253350011303604c221350022253353304a00200710011300600300e0033335530310342253353353530283500203f03d203f253353303c001330482253350011302e044221350022253353303000200a135302f001223350022303504b20011300600301003b1302c4901013300133037002001100103a00d1120011533573892010350543500165333573460640020502a666ae68c0c400409c0b8c0ccdd50019baa00133019223355301f020235001223301e002335530220232350012233021002333500137009000380233700002900000099aa980f81011a800911980f001199a800919aa981181211a8009119811001180880080091199806815001000919aa981181211a80091198110011809000800999804012801000812111919807198021a8018139a801013a99a9a80181490a99a8011099a801119a80111980400100091101711119a80210171112999ab9a3370e00c0062a666ae68cdc38028010998068020008158158120a99a80090120121a8008141119a801119a8011198128010009014119a801101411981280100091199ab9a3370e00400204604a44446666aa00866032444600660040024002006002004444466aa603803a46a0024466036004666a0024002052400266600a0080026603c66030006004046444666aa603003603866aa603403646a00244660320046010002666aa6030036446a00444a66a666aa603a03e60106603444a66a00404a200204e46a002446601400400a00c200626604000800604200266aa603403646a00244660320046605e44a66a002260160064426a00444a66a6601800401022444660040140082600c00600800446602644666a0060420040026a00204242444600600842444600200844604e44a66a0020364426a00444a66a6601000400e2602a0022600c0064466aa0046602000603600244a66a004200202e44a66a00202e266ae7000806c8c94ccd5cd180f9811000899190919800801801198079192999ab9a3022302500113232123300100300233301075c464a666ae68c094c0a00044c8cc0514cd4cc028005200110011300e4901022d330033301375c464a66a660180029000080089808249022d3200375a0026ae84d5d118140011bad35742604e0020446ea8004d5d09aba23025002300c35742604800203e6ea8004d5d09aba23022002375c6ae84c084004070dd500091199ab9a3371200400203202e46a002444400844a666ae68cdc79a80100b1a80080b0999ab9a3370e6a0040306a00203002a02e024464a666ae68c06cc0780044c8c8c8c8c8c8c8c848cccc00402401c00c008d5d09aba20045333573466e1d2004001132122230020043574260460042a666ae68c0880044c84888c004010dd71aba1302300215333573460420022244400603c60460026ea8d5d08009aba200233300a75c66014eb9d69aba100135744603c004600a6ae84c074004060dd50009299ab9c001162325333573460326038002264646424660020060046eb4d5d09aba2301d003533357346034603a00226eb8d5d0980e00080b9baa35742603600202c6ea80048c94ccd5cd180c180d80089919191909198008028012999ab9a301b00113232300953335734603c00226464646424466600200c0080066eb4d5d09aba2002375a6ae84004d5d118100019bad35742603e0042a666ae68c0740044c8488c00800cc020d5d0980f80100d180f8009baa35742603a0042a666ae68c070004044060c074004dd51aba135744603600460066ae84c068004054dd5000919192999ab9a30190011321223001003375c6ae84c06800854ccd5cd180c00089909118010019bae35742603400402a60340026ea80048488c00800c888cc06888cccd55cf800900911919807198041803980e8009803180e00098021aba2003357420040166eac0048848cc00400c00888cc05c88cccd55cf800900791980518029aba10023003357440040106eb0004c05088448894cd40044008884cc014008ccd54c01c028014010004c04c88448894cd40044d400c040884ccd4014040c010008ccd54c01c024014010004c0488844894cd4004024884cc020c010008cd54c01801c0100044800488488cc00401000cc03c8894cd40080108854cd4cc02000800c01c4cc01400400c4014400888ccd5cd19b8f0020010030051001220021001220011533573892010350543100164901022d31004901013700370e90001b874800955cf2ab9d2323001001223300330020020011";

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

  uint64_t        size   = 0U;
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
  uint64_t        size   = 0U;
  cardano_error_t result = cardano_get_serialized_output_size(NULL, &size);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_get_serialized_output_size, returnsErrorIfMemoryAllocationFails)
{
  cardano_transaction_output_t* output = create_transaction_output(min_ada_required_vectors[0].cbor);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  uint64_t        size   = 0U;
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
  uint64_t        size   = 0U;
  cardano_error_t result = cardano_get_serialized_script_size(NULL, &size);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_get_serialized_script_size, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  uint64_t size = 0U;

  cardano_error_t result = cardano_get_serialized_script_size((cardano_script_t*)"", &size);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_fee_get_serialized_transaction_size, returnsErrorIfOutParamIsNull)
{
  cardano_error_t result = cardano_get_serialized_transaction_size((cardano_transaction_t*)"", NULL);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_get_serialized_transaction_size, returnsErrorIfTransactionIsNull)
{
  uint64_t        size   = 0U;
  cardano_error_t result = cardano_get_serialized_transaction_size(NULL, &size);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_get_serialized_transaction_size, returnsErrorIfMemoryAllocationFails)
{
  cardano_transaction_t* tx = create_transaction(tx_fee_vectors[0].cbor);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  uint64_t        size   = 0U;
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
