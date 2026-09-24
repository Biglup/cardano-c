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
#include <cardano/scripts/native_scripts/native_script.h>
#include <cardano/scripts/native_scripts/script_invalid_before.h>
#include <cardano/scripts/plutus_scripts/plutus_v1_script.h>
#include <cardano/scripts/script.h>
#include <cardano/transaction/sub_transaction.h>
#include <cardano/transaction_body/sub_transaction_set.h>
#include <cardano/transaction_body/transaction_output.h>
#include <gmock/gmock.h>
#include <tests/allocators_helpers.h>

#include <vector>

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
  { 257179, "84af00818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5000181825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e020a031903e804828304581c26b17b78de4f035dc0bfce60d1d3c3a8085c38dcce5fb8767e518bed1901f48405581c0d94e174732ef9aae73f395ab44507bfa983d65023c11a951f0c32e4581ca646474b8f5431261506b6c273d307c7569a4eb6c96b42dd4a29520a582003170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c11131405a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0050758202ceb364d93225b4a0f004a0975a13eb50c3cc6348474b4fe9121f8dc72ca0cfa08186409a3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c413831581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e0b58206199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d38abc123de0d818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5010e81581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d3910825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e11186412818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d500a700818258206199186adb51974690d7247d2646097d2c62763b767b528816fb7ed3f9f55d395840bdea87fca1b4b4df8a9b8fb4183c0fab2f8261eb6c5e4bc42c800bb9c8918755bdea87fca1b4b4df8a9b8fb4183c0fab2f8261eb6c5e4bc42c800bb9c891875501868205186482041901f48200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f548201818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f548202818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f54830301818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f540281845820deeb8f82f2af5836ebbc1b450b6dbf0b03c93afe5696f10d49e8a8304ebfac01584064676273786767746f6768646a7074657476746b636f6376796669647171676775726a687268716169697370717275656c6876797071786565777072796676775820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b45041a003815820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b4500481187b0582840100d8668200810182190bb8191b58840201d86682008102821913881907d006815820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b450f5a6011904d2026373747203821904d2637374720445627974657305a2667374726b6579187b81676c6973746b65796873747276616c75650626" },
  { 326397, "84ab0081825820fbecbe69bc3ee617653b95893f50b0362cbaff3e27b01a936969a25bfc100a7c000182835839319068a7a3f008803edac87af1619860f2cdcde40c26987325ace138ad2c967f4bd28944b06462e13c5e3f5d5fa6e03f8567569438cd833e6d1a093d1cc0582057ad45489e9d4e3d7df98fb6b273d647cbed6990125dc51815bdee9abbc3a84a82583901e6d3410be0b3d51a52b874983bc6f0e48bcf4274352552e683e1c211b91d1fe82203de4c0de2c150746383a893cd21eb1082565252af63a51a00eef6fe021a00052d02031a082ee80007582026e4e8217ceb7c9eee2dffc410d77bbe3efd952288573f1c9a19fe62979634bc0b5820fe1f0d446610edf6890cbce2c3e69ad4052c557fd6d044b1f195a4f916c3e0fe0d8182582047754bf3cb4adf7374496b17fa41c197043533355c7a31a1776207fe627f5a5d010e81581ce6d3410be0b3d51a52b874983bc6f0e48bcf4274352552e683e1c2111082583901e6d3410be0b3d51a52b874983bc6f0e48bcf4274352552e683e1c211b91d1fe82203de4c0de2c150746383a893cd21eb1082565252af63a51a00461558111a0007c38312818258209a32459bd4ef6bbafdeb8cf3b909d0e3e2ec806e4cc6268529280b0fc1d06f5b00a3008182582005e884ca7c466df47785af770be8495ec0998e60ebe63e4cd187cd17eeac5e9258402cdcec5c4ba1ea76c558554dea99f472b67488be18f1f7085bac4cc55376ca8f4ed61b23565ddbffcd85a4e84963c36c98272314d2637b238e65def639969f0b0481d8799f581ce6d3410be0b3d51a52b874983bc6f0e48bcf4274352552e683e1c2119fd8799fd8799fd87a9f581c84cc25ea4c29951d40b443b95bbc5676bc425470f96376d1984af9abffd8799fd8799fd87a9f581c2c967f4bd28944b06462e13c5e3f5d5fa6e03f8567569438cd833e6dffffffffa140d8799f00a1401a00342f60ffffd8799fd8799fd8799f581cf437291791dda80d0bba9f3616f8b7533c8a8db2f788b8468a26bd5affd8799fd8799fd8799f581ce3c9536e2947e33703d5793a02b593a8d32b49aaaef03ea0b2b03c87ffffffffa140d8799f00a1401a0104ece0ffffd8799fd8799fd8799f581ce6d3410be0b3d51a52b874983bc6f0e48bcf4274352552e683e1c211ffd8799fd8799fd8799f581cb91d1fe82203de4c0de2c150746383a893cd21eb1082565252af63a5ffffffffa1581cb2d25f829ebb7f4c97b5e847923a1115b23ebf78000722c229c9c9f7d8799f01a0ffffffff0581840000d87980821a000af3301a0b01ca09f5ae181e613518327840643837393966353831636536643334313062653062336435316135326238373439383362633666306534386263663432373433353235353265363833653163321833784031313966643837393966643837393966643837613966353831633834636332356561346332393935316434306234343362393562626335363736626334323534183478403730663936333736643139383461663961626666643837393966643837393966643837613966353831633263393637663462643238393434623036343632653118357840336335653366356435666136653033663835363735363934333863643833336536646666666666666666613134306438373939663030613134303161303032661836784034643630666666666438373939666438373939666438373939663538316366343337323931373931646461383064306262613966333631366638623735333363183778403861386462326637383862383436386132366264356166666438373939666438373939666438373939663538316365336339353336653239343765333337303318387840643537393361303262353933613864333262343961616165663033656130623262303363383766666666666666666131343064383739396630306131343031611839784030306563383265306666666664383739396664383739396664383739396635383163653664333431306265306233643531613532623837343938336263366630183a784065343862636634323734333532353532653638336531633231316666643837393966643837393966643837393966353831636239316431666538323230336465183b784034633064653263313530373436333833613839336364323165623130383235363532353261663633613566666666666666666131353831636232643235663832183c784039656262376634633937623565383437393233613131313562323365626637383030303732326332323963396339663764383739396630316130666666666666183d6366662c183e783c62326432356638323965626237663463393762356538343739323361313131356232336562663738303030373232633232396339633966373a3a3030" }
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
static const char* NON_MINIMAL_NATIVE_SCRIPT_CBOR       = "820082041b0000000000000005";
static const char* PLUTUS_SCRIPT_CBORS[]                = {
  "82014e4d01000033222220051200120011",
  "82024e4d02000033222220051200120011",
  "82034e4d03000033222220051200120011",
  "82044e4d04000033222220051200120011"
};

/**
 * The size of the script that \ref RESOLVED_INPUT_WITH_REFERENCE_SCRIPT carries: the 2561 bytes of a PlutusV2 script, without
 * the three bytes of the header of its byte string and the two bytes of the array that holds the language tag and the script.
 */
static const uint64_t LARGE_REFERENCE_SCRIPT_SIZE = 2561U;

/**
 * The size of the script that \ref NATIVE_REFERENCE_SCRIPT_UTXO carries: the 32 bytes of the CBOR of a native script that
 * requires one signature, without the two bytes of the array that holds the language tag and the script.
 */
static const uint64_t NATIVE_REFERENCE_SCRIPT_SIZE = 32U;

/**
 * The size of the script that each Plutus reference script UTXO carries: the 14 bytes of a Plutus script, without the byte
 * of the header of its byte string and the two bytes of the array that holds the language tag and the script.
 */
static const uint64_t PLUTUS_REFERENCE_SCRIPT_SIZE = 14U;

/**
 * The size of the first pricing tier of the reference scripts, in bytes. Every further tier costs 1.2 times as much per
 * byte as the previous one.
 */
static const uint64_t REFERENCE_SCRIPT_TIER_SIZE = 25600U;

/**
 * The price of a byte of reference script in the first tier, the one set by \ref create_protocol_parameters.
 */
static const uint64_t REFERENCE_SCRIPT_BYTE_COST = 15U;

/**
 * \brief A total size of reference scripts, a price per byte and the fee the ledger charges for them.
 */
typedef struct
{
    uint64_t size;
    uint64_t cost_numerator;
    uint64_t cost_denominator;
    uint64_t fee;
    uint64_t per_tier_ceiling_fee;
} tiered_fee_vector_t;

/**
 * The fee the ledger charges for a total size of reference scripts: the floor, taken once, of the exact sum of the price
 * of every tier, where the tiers are 25600 bytes long and each costs 6/5 as much per byte as the previous one. The last
 * column is the fee obtained by rounding every tier up instead, which is never lower.
 */
static const tiered_fee_vector_t tiered_fee_vectors[] = {
  { 0U, 15U, 1U, 0U, 0U },
  { 1U, 15U, 1U, 15U, 15U },
  { 25599U, 15U, 1U, 383985U, 383985U },
  { 25600U, 15U, 1U, 384000U, 384000U },
  { 25601U, 15U, 1U, 384018U, 384018U },
  { 51200U, 15U, 1U, 844800U, 844800U },
  { 60000U, 15U, 1U, 1034880U, 1034880U },
  { 204800U, 15U, 1U, 6335648U, 6335650U },
  { 0U, 44U, 3U, 0U, 0U },
  { 1U, 44U, 3U, 14U, 15U },
  { 25599U, 44U, 3U, 375452U, 375452U },
  { 25600U, 44U, 3U, 375466U, 375467U },
  { 25601U, 44U, 3U, 375484U, 375485U },
  { 51200U, 44U, 3U, 826026U, 826027U },
  { 60000U, 44U, 3U, 1011882U, 1011883U },
  { 204800U, 44U, 3U, 6194856U, 6194859U },
};

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

/**
 * \brief Builds a list with a utxo whose reference script is a PlutusV1 script with a given number of bytes.
 *
 * \param size The number of bytes of the script, or zero for an empty list.
 * \return The list of utxos.
 */
static cardano_utxo_list_t*
create_sized_reference_script_inputs(const size_t size)
{
  cardano_utxo_list_t* utxo_list = NULL;

  EXPECT_EQ(cardano_utxo_list_new(&utxo_list), CARDANO_SUCCESS);

  if (size == 0U)
  {
    return utxo_list;
  }

  const std::vector<byte_t>     bytes(size, 0x01U);
  cardano_cbor_reader_t*        reader        = cardano_cbor_reader_from_hex(PLUTUS_V1_REFERENCE_SCRIPT_UTXO, strlen(PLUTUS_V1_REFERENCE_SCRIPT_UTXO));
  cardano_utxo_t*               utxo          = NULL;
  cardano_plutus_v1_script_t*   plutus_script = NULL;
  cardano_script_t*             script        = NULL;
  cardano_transaction_output_t* output        = NULL;

  EXPECT_EQ(cardano_utxo_from_cbor(reader, &utxo), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_plutus_v1_script_new_bytes(bytes.data(), bytes.size(), &plutus_script), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_script_new_plutus_v1(plutus_script, &script), CARDANO_SUCCESS);

  output = cardano_utxo_get_output(utxo);

  EXPECT_EQ(cardano_transaction_output_set_script_ref(output, script), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(utxo_list, utxo), CARDANO_SUCCESS);

  cardano_transaction_output_unref(&output);
  cardano_script_unref(&script);
  cardano_plutus_v1_script_unref(&plutus_script);
  cardano_utxo_unref(&utxo);
  cardano_cbor_reader_unref(&reader);

  return utxo_list;
}

/**
 * \brief Computes the price of a total size of reference scripts with a tiered model of any stride and multiplier, in
 * exact arithmetic over a common denominator: the denominator of the price times the denominator of the multiplier to
 * the power of the index of the last tier.
 *
 * \param size The total size of the reference scripts, in bytes.
 * \param numerator The numerator of the price of a byte in the first tier.
 * \param denominator The denominator of the price of a byte in the first tier.
 * \param stride The size of a tier, in bytes.
 * \param multiplier_numerator The numerator of the factor that scales the price from one tier to the next.
 * \param multiplier_denominator The denominator of the factor that scales the price from one tier to the next.
 * \param round_every_tier Whether the price of every tier is rounded up, instead of flooring the exact sum once.
 * \return The fee.
 */
static uint64_t
compute_exact_fee_with_tiers(
  const uint64_t size,
  const uint64_t numerator,
  const uint64_t denominator,
  const uint64_t stride,
  const uint64_t multiplier_numerator,
  const uint64_t multiplier_denominator,
  const bool     round_every_tier)
{
  std::vector<uint64_t> tier_sizes;
  uint64_t              remaining = size;

  while (remaining > 0U)
  {
    const uint64_t tier_size = (remaining < stride) ? remaining : stride;

    tier_sizes.push_back(tier_size);
    remaining -= tier_size;
  }

  unsigned __int128 sum_numerator     = 0U;
  unsigned __int128 sum_denominator   = denominator;
  unsigned __int128 rounded_fee       = 0U;
  unsigned __int128 numerator_power   = 1U;
  unsigned __int128 denominator_power = 1U;

  for (size_t i = 1U; i < tier_sizes.size(); ++i)
  {
    sum_denominator *= multiplier_denominator;
  }

  for (size_t k = 0U; k < tier_sizes.size(); ++k)
  {
    const unsigned __int128 tier_numerator   = (unsigned __int128)tier_sizes[k] * numerator * numerator_power;
    const unsigned __int128 tier_denominator = (unsigned __int128)denominator * denominator_power;

    rounded_fee       += (tier_numerator + tier_denominator - 1U) / tier_denominator;
    sum_numerator     += tier_numerator * (sum_denominator / tier_denominator);
    numerator_power   *= multiplier_numerator;
    denominator_power *= multiplier_denominator;
  }

  return (uint64_t)(round_every_tier ? rounded_fee : (sum_numerator / sum_denominator));
}

/**
 * \brief Computes the price of a total size of reference scripts with the tiered model of Conway, in exact arithmetic:
 * tiers of 25600 bytes, each of them 6/5 as expensive per byte as the previous one.
 *
 * \param size The total size of the reference scripts, in bytes.
 * \param numerator The numerator of the price of a byte in the first tier.
 * \param denominator The denominator of the price of a byte in the first tier.
 * \param round_every_tier Whether the price of every tier is rounded up, instead of flooring the exact sum once.
 * \return The fee.
 */
static uint64_t
compute_exact_tiered_fee(const uint64_t size, const uint64_t numerator, const uint64_t denominator, const bool round_every_tier)
{
  return compute_exact_fee_with_tiers(size, numerator, denominator, 25600U, 6U, 5U, round_every_tier);
}

/**
 * \brief Builds dummy protocol parameters that carry a reference script cost stride and multiplier.
 *
 * \param stride The reference script cost stride, in bytes.
 * \param multiplier_numerator The numerator of the reference script cost multiplier.
 * \param multiplier_denominator The denominator of the reference script cost multiplier.
 * \return A pointer to the protocol parameters.
 */
static cardano_protocol_parameters_t*
create_protocol_parameters_with_tiers(const uint64_t stride, const uint64_t multiplier_numerator, const uint64_t multiplier_denominator)
{
  cardano_protocol_parameters_t* params     = create_protocol_parameters();
  cardano_unit_interval_t*       multiplier = NULL;

  EXPECT_EQ(cardano_unit_interval_new(multiplier_numerator, multiplier_denominator, &multiplier), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_ref_script_cost_stride(params, stride), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_ref_script_cost_multiplier(params, multiplier), CARDANO_SUCCESS);

  cardano_unit_interval_unref(&multiplier);

  return params;
}

/**
 * \brief Sets the price of a byte of reference script of dummy protocol parameters.
 *
 * \param params The protocol parameters.
 * \param numerator The numerator of the price.
 * \param denominator The denominator of the price.
 */
static void
set_reference_script_byte_cost(cardano_protocol_parameters_t* params, const uint64_t numerator, const uint64_t denominator)
{
  cardano_unit_interval_t* cost = NULL;

  EXPECT_EQ(cardano_unit_interval_new(numerator, denominator, &cost), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_ref_script_cost_per_byte(params, cost), CARDANO_SUCCESS);

  cardano_unit_interval_unref(&cost);
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
  cardano_script_t* script = create_script(NATIVE_SCRIPT_CBOR);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  size_t size = 0U;

  cardano_error_t result = cardano_get_serialized_script_size(script, &size);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(size, 0U);

  cardano_set_allocators(malloc, realloc, free);
  cardano_script_unref(&script);
}

TEST(cardano_fee_get_serialized_script_size, measuresANativeScriptWithoutItsLanguageTag)
{
  // Arrange
  cardano_script_t* script = create_script(NATIVE_SCRIPT_CBOR);
  size_t            size   = 0U;

  // Act
  cardano_error_t result = cardano_get_serialized_script_size(script, &size);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(size, (strlen(NATIVE_SCRIPT_CBOR) / 2U) - 2U);
  EXPECT_EQ(size, NATIVE_REFERENCE_SCRIPT_SIZE);

  // Cleanup
  cardano_script_unref(&script);
}

TEST(cardano_fee_get_serialized_script_size, measuresADecodedNativeScriptWithTheNonMinimalBytesItWasDecodedFrom)
{
  // Arrange
  cardano_script_t*                script         = create_script(NON_MINIMAL_NATIVE_SCRIPT_CBOR);
  cardano_script_invalid_before_t* invalid_before = NULL;
  cardano_native_script_t*         native_script  = NULL;
  cardano_script_t*                rebuilt        = NULL;
  size_t                           size           = 0U;
  size_t                           rebuilt_size   = 0U;

  EXPECT_EQ(cardano_script_invalid_before_new(5U, &invalid_before), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_native_script_new_invalid_before(invalid_before, &native_script), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_script_new_native(native_script, &rebuilt), CARDANO_SUCCESS);

  // Act
  cardano_error_t result         = cardano_get_serialized_script_size(script, &size);
  cardano_error_t rebuilt_result = cardano_get_serialized_script_size(rebuilt, &rebuilt_size);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(rebuilt_result, CARDANO_SUCCESS);
  EXPECT_EQ(size, (strlen(NON_MINIMAL_NATIVE_SCRIPT_CBOR) / 2U) - 2U);
  EXPECT_EQ(size, 11U);
  EXPECT_EQ(rebuilt_size, 3U);

  // Cleanup
  cardano_script_unref(&script);
  cardano_script_unref(&rebuilt);
  cardano_native_script_unref(&native_script);
  cardano_script_invalid_before_unref(&invalid_before);
}

TEST(cardano_fee_get_serialized_script_size, measuresTheBytesOfALargePlutusScriptWithoutTheHeaderOfItsByteString)
{
  // Arrange
  cardano_utxo_list_t*          utxo_list = create_reference_script_inputs(1);
  cardano_utxo_t*               utxo      = NULL;
  cardano_transaction_output_t* output    = NULL;
  cardano_script_t*             script    = NULL;
  cardano_cbor_writer_t*        writer    = cardano_cbor_writer_new();
  size_t                        size      = 0U;

  EXPECT_EQ(cardano_utxo_list_get(utxo_list, 0U, &utxo), CARDANO_SUCCESS);

  output = cardano_utxo_get_output(utxo);
  script = cardano_transaction_output_get_script_ref(output);

  EXPECT_EQ(cardano_script_to_cbor(script, writer), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_get_serialized_script_size(script, &size);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(size, LARGE_REFERENCE_SCRIPT_SIZE);
  EXPECT_EQ(size, cardano_cbor_writer_get_encode_size(writer) - 5U);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
  cardano_script_unref(&script);
  cardano_transaction_output_unref(&output);
  cardano_utxo_unref(&utxo);
  cardano_utxo_list_unref(&utxo_list);
}

TEST(cardano_fee_get_serialized_script_size, measuresAPlutusScriptOfEveryVersionWithoutItsLanguageTag)
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
    EXPECT_EQ(size, (strlen(PLUTUS_SCRIPT_CBORS[i]) / 2U) - 3U);
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
  EXPECT_EQ(fee, 38415U);

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
  EXPECT_EQ(fee, 480U);

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
  EXPECT_EQ(fee_once, 2561U * 15U);
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
  EXPECT_EQ(fee, 480U);

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
  EXPECT_EQ(total_size, 2649U);
  EXPECT_EQ(fee, total_size * REFERENCE_SCRIPT_BYTE_COST);
  EXPECT_EQ(fee, 39735U);

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
  EXPECT_EQ(second_tier_size, 2571U);
  EXPECT_EQ(fee, (REFERENCE_SCRIPT_TIER_SIZE * REFERENCE_SCRIPT_BYTE_COST) + (second_tier_size * 18U));
  EXPECT_EQ(fee, 430278U);

  // Cleanup
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_utxo_list_unref(&utxo_list);
}

TEST(cardano_fee_compute_script_ref_fee, floorsTheExactSumOfTheTiersOnce)
{
  for (size_t i = 0U; i < sizeof(tiered_fee_vectors) / sizeof(tiered_fee_vectors[0]); ++i)
  {
    // Arrange
    const tiered_fee_vector_t* vector          = &tiered_fee_vectors[i];
    cardano_unit_interval_t*   script_ref_cost = NULL;
    cardano_utxo_list_t*       utxo_list       = create_sized_reference_script_inputs(vector->size);
    uint64_t                   fee             = 1U;

    EXPECT_EQ(cardano_unit_interval_new(vector->cost_numerator, vector->cost_denominator, &script_ref_cost), CARDANO_SUCCESS);

    // Act
    cardano_error_t result = cardano_compute_script_ref_fee(utxo_list, script_ref_cost, &fee);

    // Assert
    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_EQ(fee, vector->fee);
    EXPECT_EQ(fee, compute_exact_tiered_fee(vector->size, vector->cost_numerator, vector->cost_denominator, false));
    EXPECT_EQ(vector->per_tier_ceiling_fee, compute_exact_tiered_fee(vector->size, vector->cost_numerator, vector->cost_denominator, true));
    EXPECT_LE(fee, vector->per_tier_ceiling_fee);

    // Cleanup
    cardano_unit_interval_unref(&script_ref_cost);
    cardano_utxo_list_unref(&utxo_list);
  }
}

TEST(cardano_fee_compute_script_ref_fee, chargesLessThanRoundingEveryTierUpWhenTheTiersHaveFractions)
{
  // Arrange
  cardano_unit_interval_t* script_ref_cost = NULL;
  cardano_utxo_list_t*     utxo_list       = create_sized_reference_script_inputs(204800U);
  uint64_t                 fee             = 0U;

  EXPECT_EQ(cardano_unit_interval_new(15U, 1U, &script_ref_cost), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_compute_script_ref_fee(utxo_list, script_ref_cost, &fee);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(fee, 6335648U);
  EXPECT_EQ(compute_exact_tiered_fee(204800U, 15U, 1U, true), 6335650U);

  // Cleanup
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_utxo_list_unref(&utxo_list);
}

TEST(cardano_fee_compute_script_ref_fee, reducesThePriceBeforePricingTheTiers)
{
  // Arrange
  cardano_unit_interval_t* reduced_cost    = NULL;
  cardano_unit_interval_t* unreduced_cost  = NULL;
  cardano_utxo_list_t*     utxo_list       = create_sized_reference_script_inputs(204800U);
  uint64_t                 reduced_fee     = 0U;
  uint64_t                 unreduced_fee   = 0U;
  const uint64_t           large_divisor   = 1000000000000U;
  const uint64_t           large_numerator = 15U * large_divisor;

  EXPECT_EQ(cardano_unit_interval_new(15U, 1U, &reduced_cost), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unit_interval_new(large_numerator, large_divisor, &unreduced_cost), CARDANO_SUCCESS);

  // Act
  cardano_error_t reduced_result   = cardano_compute_script_ref_fee(utxo_list, reduced_cost, &reduced_fee);
  cardano_error_t unreduced_result = cardano_compute_script_ref_fee(utxo_list, unreduced_cost, &unreduced_fee);

  // Assert
  EXPECT_EQ(reduced_result, CARDANO_SUCCESS);
  EXPECT_EQ(unreduced_result, CARDANO_SUCCESS);
  EXPECT_EQ(unreduced_fee, reduced_fee);

  // Cleanup
  cardano_unit_interval_unref(&reduced_cost);
  cardano_unit_interval_unref(&unreduced_cost);
  cardano_utxo_list_unref(&utxo_list);
}

TEST(cardano_fee_compute_script_ref_fee, returnsErrorIfTheFeeOfASingleTierOverflows)
{
  // Arrange
  cardano_unit_interval_t* script_ref_cost = NULL;
  cardano_utxo_list_t*     utxo_list       = NULL;
  uint64_t                 fee             = 1U;

  EXPECT_EQ(cardano_unit_interval_new(UINT64_MAX, 1U, &script_ref_cost), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_new(&utxo_list), CARDANO_SUCCESS);

  add_utxo(utxo_list, NATIVE_REFERENCE_SCRIPT_UTXO);

  // Act
  cardano_error_t result = cardano_compute_script_ref_fee(utxo_list, script_ref_cost, &fee);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(fee, 0U);

  // Cleanup
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_utxo_list_unref(&utxo_list);
}

TEST(cardano_fee_compute_script_ref_fee, returnsErrorIfTheFeeOverflows)
{
  // Arrange
  cardano_unit_interval_t* script_ref_cost = NULL;
  cardano_utxo_list_t*     utxo_list       = create_sized_reference_script_inputs(REFERENCE_SCRIPT_TIER_SIZE + 1U);
  uint64_t                 fee             = 1U;

  EXPECT_EQ(cardano_unit_interval_new(UINT64_MAX / REFERENCE_SCRIPT_TIER_SIZE, 1U, &script_ref_cost), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_compute_script_ref_fee(utxo_list, script_ref_cost, &fee);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(fee, 0U);

  // Cleanup
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_utxo_list_unref(&utxo_list);
}

TEST(cardano_fee_compute_script_ref_fee, boundsTheFeeIfThePriceDenominatorOfTheNextTierOverflows)
{
  // Arrange
  const uint64_t           denominator     = UINT64_MAX / 2U;
  cardano_unit_interval_t* script_ref_cost = NULL;
  cardano_utxo_list_t*     utxo_list       = create_sized_reference_script_inputs(REFERENCE_SCRIPT_TIER_SIZE + 1U);
  uint64_t                 fee             = 1U;

  EXPECT_EQ(cardano_unit_interval_new(1U, denominator, &script_ref_cost), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_compute_script_ref_fee(utxo_list, script_ref_cost, &fee);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_GE(fee, compute_exact_tiered_fee(REFERENCE_SCRIPT_TIER_SIZE + 1U, 1U, denominator, false));
  EXPECT_EQ(fee, 0U);

  // Cleanup
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_utxo_list_unref(&utxo_list);
}

TEST(cardano_fee_compute_script_ref_fee, boundsTheFeeOfAPriceConvertedFromAFloatingPointValue)
{
  // Arrange
  const uint64_t numerator   = 7333333333333333U;
  const uint64_t denominator = 500000000000000U;
  const uint64_t sizes[]     = { 60000U, 204800U };
  const uint64_t fees[]      = { 1011882U, 6194856U };

  for (size_t i = 0U; i < sizeof(sizes) / sizeof(sizes[0]); ++i)
  {
    cardano_unit_interval_t* script_ref_cost = NULL;
    cardano_utxo_list_t*     utxo_list       = create_sized_reference_script_inputs(sizes[i]);
    uint64_t                 fee             = 0U;

    EXPECT_EQ(cardano_unit_interval_new(numerator, denominator, &script_ref_cost), CARDANO_SUCCESS);

    // Act
    cardano_error_t result = cardano_compute_script_ref_fee(utxo_list, script_ref_cost, &fee);

    // Assert
    const uint64_t exact_fee = compute_exact_tiered_fee(sizes[i], numerator, denominator, false);

    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_GE(fee, exact_fee);
    EXPECT_LE(fee - exact_fee, 2U);
    EXPECT_EQ(fee, fees[i]);

    // Cleanup
    cardano_unit_interval_unref(&script_ref_cost);
    cardano_utxo_list_unref(&utxo_list);
  }
}

TEST(cardano_fee_compute_script_ref_fee, pricesExactlyIfTheScaledPriceOfATierOverflowsWithASmallDenominator)
{
  // Arrange
  const uint64_t numerators[]   = { 1099511627777U, 17179869184U };
  const uint64_t denominators[] = { 3U, 1U };
  const uint64_t fees[]         = { 154802650327712072U, 7256374234104903U };
  const uint64_t size           = 8U * REFERENCE_SCRIPT_TIER_SIZE;

  for (size_t i = 0U; i < sizeof(numerators) / sizeof(numerators[0]); ++i)
  {
    cardano_unit_interval_t* script_ref_cost = NULL;
    cardano_utxo_list_t*     utxo_list       = create_sized_reference_script_inputs(size);
    uint64_t                 fee             = 0U;

    EXPECT_EQ(cardano_unit_interval_new(numerators[i], denominators[i], &script_ref_cost), CARDANO_SUCCESS);

    // Act
    cardano_error_t result = cardano_compute_script_ref_fee(utxo_list, script_ref_cost, &fee);

    // Assert
    const uint64_t exact_fee = compute_exact_tiered_fee(size, numerators[i], denominators[i], false);

    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_EQ(fee, exact_fee);
    EXPECT_EQ(fee, fees[i]);

    // Cleanup
    cardano_unit_interval_unref(&script_ref_cost);
    cardano_utxo_list_unref(&utxo_list);
  }
}

TEST(cardano_fee_compute_script_ref_fee, pricesExactlyAVeryLargePriceWithAnIntegerDenominator)
{
  // Arrange
  const uint64_t           price           = 10000000000000U;
  const uint64_t           size            = 8U * REFERENCE_SCRIPT_TIER_SIZE;
  cardano_unit_interval_t* script_ref_cost = NULL;
  cardano_utxo_list_t*     utxo_list       = create_sized_reference_script_inputs(size);
  uint64_t                 fee             = 0U;

  EXPECT_EQ(cardano_unit_interval_new(price, 1U, &script_ref_cost), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_compute_script_ref_fee(utxo_list, script_ref_cost, &fee);

  // Assert
  const uint64_t exact_fee = compute_exact_tiered_fee(size, price, 1U, false);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(exact_fee, 4223765708800000000U);
  EXPECT_GE(fee, exact_fee);
  EXPECT_EQ(fee, 4223765708800000000U);

  // Cleanup
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_utxo_list_unref(&utxo_list);
}

TEST(cardano_fee_compute_script_ref_fee, pricesExactlyAPriceWhoseDenominatorIsBelowTheOneOfTheMultiplier)
{
  // Arrange
  const uint64_t           size            = 466199U;
  const uint64_t           halved_fee      = 65046994U;
  cardano_unit_interval_t* script_ref_cost = NULL;
  cardano_utxo_list_t*     utxo_list       = create_sized_reference_script_inputs(size);
  uint64_t                 fee             = 0U;

  EXPECT_EQ(cardano_unit_interval_new(37U, 2U, &script_ref_cost), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_compute_script_ref_fee(utxo_list, script_ref_cost, &fee);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(fee, compute_exact_tiered_fee(size, 37U, 2U, false));
  EXPECT_EQ(fee, 63335231U);
  EXPECT_LE(fee, halved_fee);

  // Cleanup
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_utxo_list_unref(&utxo_list);
}

TEST(cardano_fee_compute_script_ref_fee, returnsErrorIfThePriceHasAZeroDenominator)
{
  // Arrange
  cardano_unit_interval_t* script_ref_cost = NULL;
  cardano_utxo_list_t*     utxo_list       = NULL;
  cardano_utxo_list_t*     no_scripts      = NULL;
  uint64_t                 fee             = 1U;
  uint64_t                 no_scripts_fee  = 1U;

  EXPECT_EQ(cardano_unit_interval_new(15U, 0U, &script_ref_cost), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_new(&utxo_list), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_new(&no_scripts), CARDANO_SUCCESS);

  add_utxo(utxo_list, NATIVE_REFERENCE_SCRIPT_UTXO);
  add_utxo(no_scripts, RESOLVED_INPUT);

  // Act
  cardano_error_t result            = cardano_compute_script_ref_fee(utxo_list, script_ref_cost, &fee);
  cardano_error_t no_scripts_result = cardano_compute_script_ref_fee(no_scripts, script_ref_cost, &no_scripts_fee);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(fee, 0U);
  EXPECT_EQ(no_scripts_result, CARDANO_SUCCESS);
  EXPECT_EQ(no_scripts_fee, 0U);

  // Cleanup
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_utxo_list_unref(&utxo_list);
  cardano_utxo_list_unref(&no_scripts);
}

TEST(cardano_fee_compute_script_ref_fee, returnsErrorIfMemoryAllocationFailsWhileMeasuringANativeScript)
{
  // Arrange
  cardano_unit_interval_t* script_ref_cost = NULL;
  cardano_utxo_list_t*     utxo_list       = NULL;
  uint64_t                 fee             = 1U;

  EXPECT_EQ(cardano_unit_interval_new(15U, 1U, &script_ref_cost), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_new(&utxo_list), CARDANO_SUCCESS);

  add_utxo(utxo_list, NATIVE_REFERENCE_SCRIPT_UTXO);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_compute_script_ref_fee(utxo_list, script_ref_cost, &fee);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(fee, 0U);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
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
  EXPECT_EQ(fee, tx_fee_vectors[0].fee + 39735U);

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

TEST(cardano_fee_compute_script_ref_fee_with_params, returnsErrorIfFirstParamIsNull)
{
  cardano_error_t result = cardano_compute_script_ref_fee_with_params(NULL, (cardano_protocol_parameters_t*)"", (uint64_t*)"");

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_compute_script_ref_fee_with_params, returnsErrorIfSecondParamIsNull)
{
  cardano_error_t result = cardano_compute_script_ref_fee_with_params((cardano_utxo_list_t*)"", NULL, (uint64_t*)"");

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_compute_script_ref_fee_with_params, returnsErrorIfThirdParamIsNull)
{
  cardano_error_t result = cardano_compute_script_ref_fee_with_params((cardano_utxo_list_t*)"", (cardano_protocol_parameters_t*)"", NULL);

  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_fee_compute_script_ref_fee_with_params, returnsErrorIfTheCostPerByteIsNotSet)
{
  // Arrange
  cardano_protocol_parameters_t* params    = create_protocol_parameters();
  cardano_utxo_list_t*           utxo_list = create_reference_script_inputs(1);
  uint64_t                       fee       = 1U;

  EXPECT_EQ(cardano_protocol_parameters_set_ref_script_cost_per_byte(params, NULL), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_compute_script_ref_fee_with_params(utxo_list, params, &fee);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(fee, 0U);

  // Cleanup
  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_script_ref_fee_with_params, pricesWithTheStrideAndTheMultiplierOfTheParameters)
{
  // Arrange
  const uint64_t sizes[]                   = { 0U, 1U, 10000U, 10001U, 25601U, 204800U };
  const uint64_t price_numerators[]        = { 15U, 44U };
  const uint64_t price_denominators[]      = { 1U, 3U };
  const uint64_t multiplier_numerators[]   = { 3U, 1U, 1U };
  const uint64_t multiplier_denominators[] = { 2U, 1U, 2U };

  for (size_t m = 0U; m < sizeof(multiplier_numerators) / sizeof(multiplier_numerators[0]); ++m)
  {
    for (size_t p = 0U; p < sizeof(price_numerators) / sizeof(price_numerators[0]); ++p)
    {
      for (size_t i = 0U; i < sizeof(sizes) / sizeof(sizes[0]); ++i)
      {
        cardano_protocol_parameters_t* params    = create_protocol_parameters_with_tiers(10000U, multiplier_numerators[m], multiplier_denominators[m]);
        cardano_utxo_list_t*           utxo_list = create_sized_reference_script_inputs(sizes[i]);
        uint64_t                       fee       = 1U;

        set_reference_script_byte_cost(params, price_numerators[p], price_denominators[p]);

        // Act
        cardano_error_t result = cardano_compute_script_ref_fee_with_params(utxo_list, params, &fee);

        // Assert
        EXPECT_EQ(result, CARDANO_SUCCESS);
        EXPECT_EQ(fee, compute_exact_fee_with_tiers(sizes[i], price_numerators[p], price_denominators[p], 10000U, multiplier_numerators[m], multiplier_denominators[m], false));

        // Cleanup
        cardano_utxo_list_unref(&utxo_list);
        cardano_protocol_parameters_unref(&params);
      }
    }
  }
}

TEST(cardano_fee_compute_script_ref_fee_with_params, matchesHandComputedFees)
{
  // Arrange
  const uint64_t sizes[]                   = { 25601U, 204800U, 204800U, 204800U, 204800U };
  const uint64_t price_numerators[]        = { 15U, 15U, 44U, 15U, 44U };
  const uint64_t price_denominators[]      = { 1U, 1U, 3U, 1U, 3U };
  const uint64_t multiplier_numerators[]   = { 3U, 3U, 3U, 1U, 1U };
  const uint64_t multiplier_denominators[] = { 2U, 2U, 2U, 1U, 2U };
  const uint64_t fees[]                    = { 564033U, 1236695503U, 1209213381U, 3072000U, 293333U };

  for (size_t i = 0U; i < sizeof(sizes) / sizeof(sizes[0]); ++i)
  {
    cardano_protocol_parameters_t* params    = create_protocol_parameters_with_tiers(10000U, multiplier_numerators[i], multiplier_denominators[i]);
    cardano_utxo_list_t*           utxo_list = create_sized_reference_script_inputs(sizes[i]);
    uint64_t                       fee       = 0U;

    set_reference_script_byte_cost(params, price_numerators[i], price_denominators[i]);

    // Act
    cardano_error_t result = cardano_compute_script_ref_fee_with_params(utxo_list, params, &fee);

    // Assert
    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_EQ(fee, fees[i]);

    // Cleanup
    cardano_utxo_list_unref(&utxo_list);
    cardano_protocol_parameters_unref(&params);
  }
}

TEST(cardano_fee_compute_script_ref_fee_with_params, fallsBackToTheConwayValuesIfTheParametersDoNotCarryThem)
{
  for (size_t i = 0U; i < sizeof(tiered_fee_vectors) / sizeof(tiered_fee_vectors[0]); ++i)
  {
    // Arrange
    const tiered_fee_vector_t*     vector    = &tiered_fee_vectors[i];
    cardano_protocol_parameters_t* params    = create_protocol_parameters();
    cardano_utxo_list_t*           utxo_list = create_sized_reference_script_inputs(vector->size);
    uint64_t                       fee       = 1U;

    set_reference_script_byte_cost(params, vector->cost_numerator, vector->cost_denominator);

    cardano_unit_interval_t* multiplier = cardano_protocol_parameters_get_ref_script_cost_multiplier(params);

    // Act
    cardano_error_t result = cardano_compute_script_ref_fee_with_params(utxo_list, params, &fee);

    // Assert
    EXPECT_EQ(cardano_protocol_parameters_get_ref_script_cost_stride(params), 0U);
    EXPECT_EQ(multiplier, (cardano_unit_interval_t*)nullptr);
    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_EQ(fee, vector->fee);

    // Cleanup
    cardano_utxo_list_unref(&utxo_list);
    cardano_protocol_parameters_unref(&params);
  }
}

TEST(cardano_fee_compute_script_ref_fee_with_params, fallsBackToTheConwayValueOfTheParameterThatIsNotSet)
{
  // Arrange
  cardano_protocol_parameters_t* stride_only     = create_protocol_parameters();
  cardano_protocol_parameters_t* multiplier_only = create_protocol_parameters_with_tiers(10000U, 3U, 2U);
  cardano_utxo_list_t*           utxo_list       = create_sized_reference_script_inputs(60000U);
  uint64_t                       stride_fee      = 0U;
  uint64_t                       multiplier_fee  = 0U;

  EXPECT_EQ(cardano_protocol_parameters_set_ref_script_cost_stride(stride_only, 10000U), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_ref_script_cost_multiplier(multiplier_only, NULL), CARDANO_SUCCESS);

  // Act
  cardano_error_t stride_result     = cardano_compute_script_ref_fee_with_params(utxo_list, stride_only, &stride_fee);
  cardano_error_t multiplier_result = cardano_compute_script_ref_fee_with_params(utxo_list, multiplier_only, &multiplier_fee);

  // Assert
  EXPECT_EQ(stride_result, CARDANO_SUCCESS);
  EXPECT_EQ(multiplier_result, CARDANO_SUCCESS);
  EXPECT_EQ(stride_fee, compute_exact_fee_with_tiers(60000U, 15U, 1U, 10000U, 6U, 5U, false));
  EXPECT_EQ(stride_fee, 1489488U);
  EXPECT_EQ(multiplier_fee, stride_fee);

  // Cleanup
  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&stride_only);
  cardano_protocol_parameters_unref(&multiplier_only);
}

TEST(cardano_fee_compute_script_ref_fee_with_params, pricesAMultiplierSetWithoutAStrideWithTheConwayStride)
{
  // Arrange
  cardano_protocol_parameters_t* params     = create_protocol_parameters();
  cardano_unit_interval_t*       multiplier = NULL;
  cardano_utxo_list_t*           utxo_list  = create_sized_reference_script_inputs(60000U);
  uint64_t                       fee        = 0U;

  EXPECT_EQ(cardano_unit_interval_new(3U, 2U, &multiplier), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_protocol_parameters_set_ref_script_cost_multiplier(params, multiplier), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_compute_script_ref_fee_with_params(utxo_list, params, &fee);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(fee, compute_exact_fee_with_tiers(60000U, 15U, 1U, 25600U, 3U, 2U, false));
  EXPECT_EQ(fee, 1257000U);

  // Cleanup
  cardano_unit_interval_unref(&multiplier);
  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_script_ref_fee_with_params, matchesTheConwayFeeWithTheConwayValues)
{
  // Arrange
  cardano_protocol_parameters_t* params          = create_protocol_parameters_with_tiers(25600U, 6U, 5U);
  cardano_unit_interval_t*       script_ref_cost = cardano_protocol_parameters_get_ref_script_cost_per_byte(params);
  cardano_utxo_list_t*           utxo_list       = create_sized_reference_script_inputs(204800U);
  uint64_t                       fee             = 0U;
  uint64_t                       conway_fee      = 1U;

  // Act
  cardano_error_t result        = cardano_compute_script_ref_fee_with_params(utxo_list, params, &fee);
  cardano_error_t conway_result = cardano_compute_script_ref_fee(utxo_list, script_ref_cost, &conway_fee);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(conway_result, CARDANO_SUCCESS);
  EXPECT_EQ(fee, conway_fee);
  EXPECT_EQ(fee, 6335648U);

  // Cleanup
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_script_ref_fee_with_params, returnsErrorIfTheMultiplierHasAZeroDenominator)
{
  // Arrange
  cardano_protocol_parameters_t* params         = create_protocol_parameters_with_tiers(10000U, 3U, 0U);
  cardano_utxo_list_t*           utxo_list      = create_sized_reference_script_inputs(100U);
  cardano_utxo_list_t*           no_scripts     = create_sized_reference_script_inputs(0U);
  uint64_t                       fee            = 1U;
  uint64_t                       no_scripts_fee = 1U;

  // Act
  cardano_error_t result            = cardano_compute_script_ref_fee_with_params(utxo_list, params, &fee);
  cardano_error_t no_scripts_result = cardano_compute_script_ref_fee_with_params(no_scripts, params, &no_scripts_fee);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(fee, 0U);
  EXPECT_EQ(no_scripts_result, CARDANO_SUCCESS);
  EXPECT_EQ(no_scripts_fee, 0U);

  // Cleanup
  cardano_utxo_list_unref(&utxo_list);
  cardano_utxo_list_unref(&no_scripts);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_script_ref_fee_with_params, pricesExactlyAMultiplierWithALargeDenominator)
{
  // Arrange
  cardano_protocol_parameters_t* params    = create_protocol_parameters_with_tiers(25600U, 123456789U, 100000000U);
  cardano_utxo_list_t*           utxo_list = create_sized_reference_script_inputs(60000U);
  uint64_t                       fee       = 0U;

  // Act
  cardano_error_t result = cardano_compute_script_ref_fee_with_params(utxo_list, params, &fee);

  // Assert
  const uint64_t exact_fee = compute_exact_fee_with_tiers(60000U, 15U, 1U, 25600U, 123456789U, 100000000U, false);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(exact_fee, 1059262U);
  EXPECT_GE(fee, exact_fee);
  EXPECT_LE(fee - exact_fee, 2U);
  EXPECT_EQ(fee, 1059262U);

  // Cleanup
  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_script_ref_fee_with_params, pricesExactlyAMultiplierWithALargeNumerator)
{
  // Arrange
  const uint64_t                 multiplier_numerator   = 1234567890123456789U;
  const uint64_t                 multiplier_denominator = 1000000000000000000U;
  cardano_protocol_parameters_t* params                 = create_protocol_parameters_with_tiers(25600U, multiplier_numerator, multiplier_denominator);
  cardano_utxo_list_t*           utxo_list              = create_sized_reference_script_inputs(30000U);
  uint64_t                       fee                    = 0U;

  // Act
  cardano_error_t result = cardano_compute_script_ref_fee_with_params(utxo_list, params, &fee);

  // Assert
  const uint64_t exact_fee = compute_exact_fee_with_tiers(30000U, 15U, 1U, 25600U, multiplier_numerator, multiplier_denominator, false);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(exact_fee, 465481U);
  EXPECT_GE(fee, exact_fee);
  EXPECT_EQ(fee, 465481U);

  // Cleanup
  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_script_ref_fee_with_params, pricesExactlyManyTiersOfASmallStride)
{
  // Arrange
  const uint64_t strides[]                 = { 10000U, 10000U };
  const uint64_t multiplier_numerators[]   = { 6U, 11U };
  const uint64_t multiplier_denominators[] = { 5U, 10U };
  const uint64_t price_numerators[]        = { 15U, 44U };
  const uint64_t price_denominators[]      = { 1U, 3U };
  const uint64_t fees[]                    = { 30763507U, 8873949U };
  const uint64_t size                      = 204800U;

  for (size_t i = 0U; i < sizeof(fees) / sizeof(fees[0]); ++i)
  {
    cardano_protocol_parameters_t* params    = create_protocol_parameters_with_tiers(strides[i], multiplier_numerators[i], multiplier_denominators[i]);
    cardano_utxo_list_t*           utxo_list = create_sized_reference_script_inputs(size);
    uint64_t                       fee       = 0U;

    set_reference_script_byte_cost(params, price_numerators[i], price_denominators[i]);

    // Act
    cardano_error_t result = cardano_compute_script_ref_fee_with_params(utxo_list, params, &fee);

    // Assert
    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_EQ(fee, compute_exact_fee_with_tiers(size, price_numerators[i], price_denominators[i], strides[i], multiplier_numerators[i], multiplier_denominators[i], false));
    EXPECT_EQ(fee, fees[i]);

    // Cleanup
    cardano_utxo_list_unref(&utxo_list);
    cardano_protocol_parameters_unref(&params);
  }
}

TEST(cardano_fee_compute_script_ref_fee_with_params, boundsTheFeeCloselyIfTheDenominatorsOfTheTierPricesDoNotFit)
{
  // Arrange
  const uint64_t strides[]                 = { 10000U, 1291U };
  const uint64_t multiplier_numerators[]   = { 101U, 1371U };
  const uint64_t multiplier_denominators[] = { 100U, 859U };
  const uint64_t price_numerators[]        = { 443U, 759U };
  const uint64_t price_denominators[]      = { 100U, 29U };
  const uint64_t sizes[]                   = { 204800U, 79714U };
  const uint64_t exact_fees[]              = { 1001387U, 199042058512434346U };
  const uint64_t fees[]                    = { 1001387U, 199042058512434347U };

  for (size_t i = 0U; i < sizeof(fees) / sizeof(fees[0]); ++i)
  {
    cardano_protocol_parameters_t* params    = create_protocol_parameters_with_tiers(strides[i], multiplier_numerators[i], multiplier_denominators[i]);
    cardano_utxo_list_t*           utxo_list = create_sized_reference_script_inputs(sizes[i]);
    uint64_t                       fee       = 0U;

    set_reference_script_byte_cost(params, price_numerators[i], price_denominators[i]);

    // Act
    cardano_error_t result = cardano_compute_script_ref_fee_with_params(utxo_list, params, &fee);

    // Assert
    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_GE(fee, exact_fees[i]);
    EXPECT_LE(fee - exact_fees[i], 1U);
    EXPECT_EQ(fee, fees[i]);

    // Cleanup
    cardano_utxo_list_unref(&utxo_list);
    cardano_protocol_parameters_unref(&params);
  }
}

TEST(cardano_fee_compute_script_ref_fee_with_params, boundsTheFeeIfTheHalvedFractionsOfATierReachOne)
{
  // Arrange
  const uint64_t multiplier_numerators[]   = { 1U, 3499778882U };
  const uint64_t multiplier_denominators[] = { 7U, 3499778883U };
  const uint64_t price_numerators[]        = { 42U, 1U };
  const uint64_t sizes[]                   = { 38U, 83U };
  const uint64_t exact_fees[]              = { 48U, 82U };
  const uint64_t fees[]                    = { 49U, 82U };

  for (size_t i = 0U; i < sizeof(fees) / sizeof(fees[0]); ++i)
  {
    cardano_protocol_parameters_t* params    = create_protocol_parameters_with_tiers(1U, multiplier_numerators[i], multiplier_denominators[i]);
    cardano_utxo_list_t*           utxo_list = create_sized_reference_script_inputs(sizes[i]);
    uint64_t                       fee       = 0U;

    set_reference_script_byte_cost(params, price_numerators[i], 1U);

    // Act
    cardano_error_t result = cardano_compute_script_ref_fee_with_params(utxo_list, params, &fee);

    // Assert
    EXPECT_EQ(result, CARDANO_SUCCESS);
    EXPECT_GE(fee, exact_fees[i]);
    EXPECT_LE(fee - exact_fees[i], 1U);
    EXPECT_EQ(fee, fees[i]);

    // Cleanup
    cardano_utxo_list_unref(&utxo_list);
    cardano_protocol_parameters_unref(&params);
  }
}

TEST(cardano_fee_compute_script_ref_fee_with_params, returnsErrorIfMemoryAllocationFailsWhileMeasuringANativeScript)
{
  // Arrange
  cardano_protocol_parameters_t* params    = create_protocol_parameters_with_tiers(10000U, 3U, 2U);
  cardano_utxo_list_t*           utxo_list = NULL;
  uint64_t                       fee       = 1U;

  EXPECT_EQ(cardano_utxo_list_new(&utxo_list), CARDANO_SUCCESS);

  add_utxo(utxo_list, NATIVE_REFERENCE_SCRIPT_UTXO);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_compute_script_ref_fee_with_params(utxo_list, params, &fee);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(fee, 0U);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&params);
}

TEST(cardano_fee_compute_transaction_fee, pricesTheReferenceScriptsWithTheStrideAndTheMultiplierOfTheParameters)
{
  // Arrange
  cardano_protocol_parameters_t* params         = create_protocol_parameters_with_tiers(1000U, 3U, 2U);
  cardano_protocol_parameters_t* conway_params  = create_protocol_parameters();
  cardano_transaction_t*         tx             = create_transaction(tx_fee_vectors[0].cbor);
  cardano_utxo_list_t*           utxo_list      = create_mixed_language_reference_script_inputs();
  uint64_t                       fee            = 0U;
  uint64_t                       conway_fee     = 0U;
  uint64_t                       ref_script_fee = 0U;

  // Act
  cardano_error_t result        = cardano_compute_transaction_fee(tx, utxo_list, params, &fee);
  cardano_error_t conway_result = cardano_compute_transaction_fee(tx, utxo_list, conway_params, &conway_fee);

  // Assert
  const uint64_t total_size = NATIVE_REFERENCE_SCRIPT_SIZE + (4U * PLUTUS_REFERENCE_SCRIPT_SIZE) + LARGE_REFERENCE_SCRIPT_SIZE;

  EXPECT_EQ(cardano_compute_script_ref_fee_with_params(utxo_list, params, &ref_script_fee), CARDANO_SUCCESS);
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(conway_result, CARDANO_SUCCESS);
  EXPECT_EQ(total_size, 2649U);
  EXPECT_EQ(ref_script_fee, compute_exact_fee_with_tiers(total_size, 15U, 1U, 1000U, 3U, 2U, false));
  EXPECT_EQ(ref_script_fee, 59403U);
  EXPECT_EQ(fee, tx_fee_vectors[0].fee + 59403U);
  EXPECT_EQ(conway_fee, tx_fee_vectors[0].fee + 39735U);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxo_list);
  cardano_protocol_parameters_unref(&params);
  cardano_protocol_parameters_unref(&conway_params);
}

TEST(cardano_fee_compute_transaction_fee, returnsErrorIfTheMultiplierHasAZeroDenominator)
{
  // Arrange
  cardano_protocol_parameters_t* params    = create_protocol_parameters_with_tiers(1000U, 3U, 0U);
  cardano_transaction_t*         tx        = create_transaction(tx_fee_vectors[0].cbor);
  cardano_utxo_list_t*           utxo_list = create_mixed_language_reference_script_inputs();
  uint64_t                       fee       = 0U;

  // Act
  cardano_error_t result = cardano_compute_transaction_fee(tx, utxo_list, params, &fee);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&utxo_list);
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
