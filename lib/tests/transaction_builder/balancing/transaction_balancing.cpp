/**
 * \file transaction_balancing.cpp
 *
 * \author angel.castillo
 * \date   Nov 04, 2024
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

#include "../../allocators_helpers.h"

#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/utxo.h>
#include <cardano/transaction_builder/balancing/transaction_balancing.h>

#include <allocators.h>
#include <cardano/address/enterprise_address.h>
#include <cardano/certs/certificate.h>
#include <cardano/certs/certificate_set.h>
#include <cardano/certs/registration_cert.h>
#include <cardano/certs/unregistration_cert.h>
#include <cardano/common/credential.h>
#include <cardano/common/guard_set.h>
#include <cardano/common/withdrawal_map.h>
#include <cardano/crypto/ed25519_private_key.h>
#include <cardano/crypto/ed25519_public_key.h>
#include <cardano/crypto/ed25519_signature.h>
#include <cardano/scripts/native_scripts/native_script.h>
#include <cardano/scripts/native_scripts/script_pubkey.h>
#include <cardano/transaction_builder/coin_selection/large_first_coin_selector.h>
#include <cardano/transaction_builder/coin_selection/random_improve_coin_selector.h>
#include <cardano/transaction_builder/fee.h>
#include <cardano/witness_set/bootstrap_witness.h>
#include <cardano/witness_set/bootstrap_witness_set.h>
#include <cardano/witness_set/native_script_set.h>
#include <cardano/witness_set/redeemer.h>
#include <cardano/witness_set/vkey_witness.h>
#include <cardano/witness_set/vkey_witness_set.h>
#include <gmock/gmock.h>

#include <algorithm>
#include <vector>

/* CONSTANTS *****************************************************************/

static const char* BALANCED_TX_CBOR    = "84a300d9010282825820027b68d4c11e97d7e065cc2702912cb1a21b6d0e56c6a74dd605889a5561138500825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a633010182a2005839005cf6c91279a859a072601779fb33bb07c34e1d641d45df51ff63b967f15db05f56035465bf8900a09bdaa16c3d8b8244fea686524408dd8001821a00e4e1c0a1581c0b0d621b5c26d0a1fd0893a4b04c19d860296a69ede1fbcfc5179882a1474e46542d30303101a200583900dc435fc2638f6684bd1f9f6f917d80c92ae642a4a33a412e516479e64245236ab8056760efceebbff57e8cab220182be3e36439e520a6454011a0d294e28021a00029eb9a0f5f6";
static const char* UNBALANCED_TX_CBOR  = "84a300d9010282825820027b68d4c11e97d7e065cc2702912cb1a21b6d0e56c6a74dd605889a5561138500825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a633010182a2005839005cf6c91279a859a072601779fb33bb07c34e1d641d45df51ff63b967f15db05f56035465bf8900a09bdaa16c3d8b8244fea686524408dd8001821a00e4e1c0a1581c0b0d621b5c26d0a1fd0893a4b04c19d860296a69ede1fbcfc5179882a1474e46542d30303101a200583900dc435fc2638f6684bd1f9f6f917d80c92ae642a4a33a412e516479e64245236ab8056760efceebbff57e8cab220182be3e36439e520a6454011a0d294e28021a00000000a0f5f6";
static const char* COMPLEX_TX_CBOR     = "84b000818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5000181825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e020a031903e8049182008200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d083078200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d00a83088200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d01483088200581cc37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f186482018200581cc37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f82008200581cc37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f8a03581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f81581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d58304581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d01901f483028200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0581c1732c16e26f8efb749c7f67113ec507a97fb3b382b8c147538e92db784108200581cb276b4f7a706a81364de606d890343a76af570268d4bbfee2fc8fcab05f683118200581cb276b4f7a706a81364de606d890343a76af570268d4bbfee2fc8fcab0584108200581cb276b4f7a706a81364de606d890343a76af570268d4bbfee2fc8fcab05f683118200581cb276b4f7a706a81364de606d890343a76af570268d4bbfee2fc8fcab05840b8200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0581c1732c16e26f8efb749c7f67113ec507a97fb3b382b8c147538e92db70a840c8200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d08200581cb276b4f7a706a81364de606d890343a76af570268d4bbfee2fc8fcab0a850d8200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0581c1732c16e26f8efb749c7f67113ec507a97fb3b382b8c147538e92db78200581cb276b4f7a706a81364de606d890343a76af570268d4bbfee2fc8fcab0a82018200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d005a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d00a0758202ceb364d93225b4a0f004a0975a13eb50c3cc6348474b4fe9121f8dc72ca0cfa08186409a3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e0b58206199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d38abc123de0d818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5010e81581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d3910825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e11186412818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5001481841864581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d08106827468747470733a2f2f74657374696e672e7468697358203e33018e8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80da700818258206199186adb51974690d7247d2646097d2c62763b767b528816fb7ed3f9f55d395840bdea87fca1b4b4df8a9b8fb4183c0fab2f8261eb6c5e4bc42c800bb9c8918755bdea87fca1b4b4df8a9b8fb4183c0fab2f8261eb6c5e4bc42c800bb9c891875501868205186482041901f48200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f548201818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f548202818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f54830301818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f540281845820deeb8f82f2af5836ebbc1b450b6dbf0b03c93afe5696f10d49e8a8304ebfac01584064676273786767746f6768646a7074657476746b636f6376796669647171676775726a687268716169697370717275656c6876797071786565777072796676775820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b45041a003815820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b4500481187b0582840100d87a9f187bff82190bb8191b58840201d87a9f187bff821913881907d006815820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b450f5a6011904d2026373747203821904d2637374720445627974657305a2667374726b6579187b81676c6973746b65796873747276616c75650626";
static const char* CBOR_DIFFERENT_VAL1 = "82825820027b68d4c11e97d7e065cc2702912cb1a21b6d0e56c6a74dd605889a5561138500a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a00118f32a1581c0b0d621b5c26d0a1fd0893a4b04c19d860296a69ede1fbcfc5179882a1474e46542d30303101";
static const char* CBOR_DIFFERENT_VAL2 = "82825820d3c887d17486d483a2b46b58b01cb9344745f15fdd8f8e70a57f854cdd88a63301a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8011a0dff3f6f";
// A sub transaction that spends 12 ADA, pays 10 ADA, withdraws 5 lovelace, registers a stake credential with a
// deposit of 10, unregisters one refunding 20, mints 5 units of one asset, burns 3 units of another and direct
// deposits 1 ADA into a reward account.
static const char* SUB_TX_CBOR         = "83a600d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5000181a200583900dc435fc2638f6684bd1f9f6f917d80c92ae642a4a33a412e516479e64245236ab8056760efceebbff57e8cab220182be3e36439e520a6454011a0098968004d901028283078200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d00a83088200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d01405a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f0509a2581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14005581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c41221819a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f1a000f4240a0f6";
static const char* SUB_TX_UTXO_CBOR    = "828258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d500a200583900dc435fc2638f6684bd1f9f6f917d80c92ae642a4a33a412e516479e64245236ab8056760efceebbff57e8cab220182be3e36439e520a6454011a00b71b00";
static const char* MINT_POLICY_ID      = "2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740";
static const char* BURN_POLICY_ID      = "659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82";
static const char* REWARD_ADDRESS      = "stake_test1uqfu74w3wh4gfzu8m6e7j987h4lq9r3t7ef5gaw497uu85qsqfy27";
static const char* REWARD_ADDRESS2     = "stake_test1uqehkck0lajq8gr28t9uxnuvgcqrc6070x3k9r8048z8y5gssrtvn";
static const char* CBOR_DIFFERENT_VAL3 = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a026679b8a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c420a";
static const char* OTHER_POLICY_ID     = "1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601";
static const char* NFT_POLICY_ID       = "0b0d621b5c26d0a1fd0893a4b04c19d860296a69ede1fbcfc5179882";
static const char* PLUTUS_SCRIPT_HEX   = "4d01000033222220051200120011";
static const char* REF_SCRIPT_V1_UTXO  = "82825820bb247abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d8185182014e4d01000033222220051200120011";
static const char* REF_SCRIPT_V2_UTXO  = "82825820bb257abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e002a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d8185182024e4d02000033222220051200120011";
static const char* REF_SCRIPT_V3_UTXO  = "82825820bb267abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e003a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d8185182034e4d03000033222220051200120011";
static const char* REF_SCRIPT_V4_UTXO  = "82825820bb287abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e004a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d8185182044e4d04000033222220051200120011";
static const char* REF_SCRIPT_NATIVE   = "82825820bb277abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e003a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d818582282008200581cb275b08c999097247f7c17e77007c7010cd19f20cc086ad99d398538";
static const char* BATCH_CHANGE_ADDR   = "addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk";
static const char* LEGACY_MODE_ERROR   = "The top level transaction needs a PlutusV1, PlutusV2 or PlutusV3 script, so the sub transactions must balance between themselves. Add a balancing sub transaction, top level change can not absorb their imbalance.";

/**
 * \brief Error reported when a sub transaction spends an input that none of the lists given to the balancer resolves.
 */
static const char* UNRESOLVED_SUB_TX_INPUT_ERROR = "A sub transaction spends an input that is not among the UTXOs given to the balancer. The resolved UTXOs of the sub transactions must be included in the available UTXOs.";

/**
 * \brief Error reported when every collateral UTXO given to the balancer is spent or referenced by a sub transaction.
 */
static const char* SUB_TX_COLLATERAL_ERROR = "Every collateral UTXO given to the balancer is spent or referenced by a sub transaction. The collateral of the top level transaction must come from UTXOs that its sub transactions do not use.";

/**
 * \brief Signing key seeds of the owners of the UTXOs spent by the transactions that get signed. They are the public
 * RFC 8032 test vectors, known to everyone, and must never hold funds.
 */
static const char* FIRST_SIGNER_KEY_HEX  = "9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60";
static const char* SECOND_SIGNER_KEY_HEX = "4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb";
static const char* THIRD_SIGNER_KEY_HEX  = "c5aa8df43f9f837bedb7442f31dcb7b166d38535076f094b85ce3a2e0b4458f7";

/**
 * \brief Hash of the script that controls the UTXO spent by the transaction that nobody signs.
 */
static const char* SCRIPT_HASH_HEX = "b275b08c999097247f7c17e77007c7010cd19f20cc086ad99d398538";

/**
 * \brief Zero based index, counted from the start of the balancing done by
 * balance_with_pre_selected_and_collateral_utxos, of the first allocation made while joining the
 * pre selected UTXOs and the coin selection into the resolved inputs.
 */
static const int PRE_SELECTED_RESOLVED_INPUTS_MALLOC_INDEX = 587;

/**
 * \brief Zero based index, counted from the start of the balancing done by
 * balance_with_pre_selected_and_collateral_utxos, of the first allocation made while adding the
 * collateral UTXOs to the resolved inputs.
 */
static const int COLLATERAL_RESOLVED_INPUTS_MALLOC_INDEX = 592;

/**
 * \brief Key hashes of the stake credentials that the certificates of the hand assembled bodies register and unregister.
 */
static const char* STAKE_KEY_HASH_HEX  = "13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0";
static const char* STAKE_KEY_HASH2_HEX = "c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f";

/**
 * \brief The error messages reported when the withdrawals, deposits or reclaimed deposits of a body wrap past UINT64_MAX.
 */
static const char* IMPLICIT_COIN_OVERFLOW_ERROR        = "The withdrawals, deposits or reclaimed deposits of the transaction add up to more than the maximum amount the balancer can represent.";
static const char* SUB_TX_IMPLICIT_COIN_OVERFLOW_ERROR = "The withdrawals, deposits or reclaimed deposits of a sub transaction add up to more than the maximum amount the balancer can represent.";

/**
 * \brief Half of the lovelace a 64 bit unsigned integer can hold, so two of them wrap past UINT64_MAX.
 */
static const uint64_t HALF_OF_UINT64_RANGE = 9223372036854775808U;

/**
 * \brief The most a fee may exceed the minimum fee of the signed transaction, in bytes of fee. The estimate of the VK
 * witnesses is exact, so the bound only leaves room for an amount that takes fewer bytes once the fee converges.
 */
static const int64_t MAX_FEE_EXCESS_IN_BYTES = 3;

/**
 * \brief Byron addresses, the CBOR encoded attributes their bootstrap witnesses carry and a chain code to sign with.
 */
static const char* BYRON_YOROI_ADDRESS           = "Ae2tdPwUPEZFRbyhz3cpfC2CumGzNkFBN2L42rcUc2yjQpEkxDbkPodpMAi";
static const char* BYRON_YOROI_ATTRIBUTES_HEX    = "a0";
static const char* BYRON_DAEDALUS_ADDRESS        = "37btjrVyb4KEB2STADSsj3MYSAdj52X5FrFWpw2r7Wmj2GDzXjFRsHWuZqrw7zSkwopv8Ci3VWeg6bisU9dgJxW5hb2MZYeduNKbQJrqz3zVBsu9nT";
static const char* BYRON_DAEDALUS_ATTRIBUTES_HEX = "a201581e581c9c1722f7e446689256e1a30260f3510d558d99d0c391f2ba89cb697702451a4170cb17";
static const char* BYRON_CHAIN_CODE_HEX          = "7e4ebd21ec7cd6a9a4e1e5a7e1f76fc4b8c3fb5d6ed83d2a9f1e6c5b4a392817";

/**
 * The fee of the native reference script that requires one signature, with the reference script price per byte set by
 * \ref init_protocol_parameters (15 lovelace): the 32 bytes of the CBOR of the native script, without the two
 * bytes of the array that holds the language tag and the script, all of it inside the first pricing tier.
 */
static const uint64_t NATIVE_REFERENCE_SCRIPT_FEE = 32U * 15U;

/**
 * The fee of the Plutus reference script \ref PLUTUS_SCRIPT_HEX, with the same price per byte: the 14 bytes of the
 * script, without the byte of the header of its byte string and the two bytes of the array that holds the language tag
 * and the script.
 */
static const uint64_t PLUTUS_REFERENCE_SCRIPT_FEE = 14U * 15U;

/* STRUCTURES ****************************************************************/

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
 * \brief The owner of a Byron UTXO: the key it signs with, the chain code of its extended key, the Byron address it
 * holds and the attributes of that address, which its bootstrap witness carries.
 */
struct byron_signer_t
{
    cardano_ed25519_private_key_t* private_key;
    cardano_ed25519_public_key_t*  public_key;
    cardano_buffer_t*              chain_code;
    cardano_buffer_t*              attributes;
    cardano_address_t*             address;
};

/* STATIC FUNCTIONS **********************************************************/

static cardano_transaction_t*
new_default_transaction(const char* cbor)
{
  cardano_transaction_t* transaction = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_transaction_from_cbor(reader, &transaction);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return transaction;
};

static cardano_transaction_t*
new_transaction_without_inputs(const char* cbor, const int64_t target_coin)
{
  cardano_transaction_t* transaction = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_transaction_from_cbor(reader, &transaction);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  cardano_transaction_input_set_t* inputs = NULL;
  result                                  = cardano_transaction_input_set_new(&inputs);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_transaction_body_t* body = cardano_transaction_get_body(transaction);
  cardano_transaction_body_unref(&body);

  result = cardano_transaction_body_set_inputs(body, inputs);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_transaction_input_set_unref(&inputs);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  cardano_transaction_output_t* output = NULL;
  result                               = cardano_transaction_output_list_get(outputs, 0, &output);
  cardano_transaction_output_unref(&output);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_value_t* value = cardano_transaction_output_get_value(output);
  result                 = cardano_value_set_coin(value, target_coin);
  cardano_value_unref(&value);
  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_transaction_output_list_t* new_outputs = NULL;
  result                                         = cardano_transaction_output_list_new(&new_outputs);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  result = cardano_transaction_output_list_add(new_outputs, output);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  result = cardano_transaction_body_set_outputs(body, new_outputs);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_transaction_output_list_unref(&new_outputs);

  return transaction;
};

static cardano_transaction_t*
new_transaction_without_inputs_no_assets(const char* cbor, const int64_t target_coin)
{
  cardano_transaction_t* transaction = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_transaction_from_cbor(reader, &transaction);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  cardano_transaction_input_set_t* inputs = NULL;
  result                                  = cardano_transaction_input_set_new(&inputs);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_transaction_body_t* body = cardano_transaction_get_body(transaction);
  cardano_transaction_body_unref(&body);

  result = cardano_transaction_body_set_inputs(body, inputs);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_transaction_input_set_unref(&inputs);

  cardano_transaction_output_list_t* outputs = cardano_transaction_body_get_outputs(body);
  cardano_transaction_output_list_unref(&outputs);

  cardano_transaction_output_t* output = NULL;
  result                               = cardano_transaction_output_list_get(outputs, 0, &output);
  cardano_transaction_output_unref(&output);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_value_t* value = cardano_value_new_zero();
  result                 = cardano_value_set_coin(value, target_coin);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  result = cardano_transaction_output_set_value(output, value);
  cardano_value_unref(&value);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_transaction_output_list_t* new_outputs = NULL;
  result                                         = cardano_transaction_output_list_new(&new_outputs);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  result = cardano_transaction_output_list_add(new_outputs, output);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  result = cardano_transaction_body_set_outputs(body, new_outputs);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_transaction_output_list_unref(&new_outputs);

  EXPECT_THAT(cardano_transaction_body_set_fee(body, 0), CARDANO_SUCCESS);

  return transaction;
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

  cardano_unit_interval_unref(&memory_prices);
  cardano_unit_interval_unref(&steps_prices);
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_ex_unit_prices_unref(&ex_unit_prices);

  return params;
}

static cardano_utxo_t*
new_default_utxo(const char* utxo)
{
  cardano_utxo_t*        utxo_obj = NULL;
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(utxo, strlen(utxo));

  cardano_error_t result = cardano_utxo_from_cbor(reader, &utxo_obj);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return utxo_obj;
};

static cardano_utxo_list_t*
new_default_utxo_list(const uint64_t donation = 0)
{
  cardano_utxo_list_t* list = NULL;

  cardano_error_t error = cardano_utxo_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_t* gai1 = new_default_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t* gai2 = new_default_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_t* gai3 = new_default_utxo(CBOR_DIFFERENT_VAL3);

  // Add donation to first UTXO
  if (donation > 0)
  {
    cardano_transaction_output_t* output = cardano_utxo_get_output(gai1);
    cardano_transaction_output_unref(&output);

    cardano_value_t* value = cardano_transaction_output_get_value(output);
    cardano_value_unref(&value);

    const uint64_t original_coin = cardano_value_get_coin(value);
    EXPECT_EQ(cardano_value_set_coin(value, original_coin + donation), CARDANO_SUCCESS);
  }

  EXPECT_EQ(cardano_utxo_list_add(list, gai2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(list, gai1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(list, gai3), CARDANO_SUCCESS);

  cardano_utxo_unref(&gai1);
  cardano_utxo_unref(&gai2);
  cardano_utxo_unref(&gai3);

  return list;
};

static cardano_sub_transaction_t*
new_default_sub_transaction()
{
  cardano_sub_transaction_t* sub_transaction = NULL;
  cardano_cbor_reader_t*     reader          = cardano_cbor_reader_from_hex(SUB_TX_CBOR, strlen(SUB_TX_CBOR));

  cardano_error_t result = cardano_sub_transaction_from_cbor(reader, &sub_transaction);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return sub_transaction;
};

static cardano_utxo_list_t*
new_sub_transaction_utxo_list()
{
  cardano_utxo_list_t* list = NULL;

  cardano_error_t error = cardano_utxo_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_utxo_t* utxo = new_default_utxo(SUB_TX_UTXO_CBOR);

  EXPECT_EQ(cardano_utxo_list_add(list, utxo), CARDANO_SUCCESS);

  cardano_utxo_unref(&utxo);

  return list;
};

static void
set_direct_deposit(cardano_transaction_t* tx, const uint64_t amount)
{
  cardano_transaction_body_t*   body            = cardano_transaction_get_body(tx);
  cardano_direct_deposit_map_t* direct_deposits = NULL;

  EXPECT_EQ(cardano_direct_deposit_map_new(&direct_deposits), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_direct_deposit_map_insert_ex(direct_deposits, REWARD_ADDRESS, strlen(REWARD_ADDRESS), amount), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_direct_deposits(body, direct_deposits), CARDANO_SUCCESS);

  cardano_direct_deposit_map_unref(&direct_deposits);
  cardano_transaction_body_unref(&body);
}

static void
set_direct_deposits(cardano_transaction_t* tx, const uint64_t amount, const uint64_t amount2)
{
  cardano_transaction_body_t*   body            = cardano_transaction_get_body(tx);
  cardano_direct_deposit_map_t* direct_deposits = NULL;

  EXPECT_EQ(cardano_direct_deposit_map_new(&direct_deposits), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_direct_deposit_map_insert_ex(direct_deposits, REWARD_ADDRESS, strlen(REWARD_ADDRESS), amount), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_direct_deposit_map_insert_ex(direct_deposits, REWARD_ADDRESS2, strlen(REWARD_ADDRESS2), amount2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_direct_deposits(body, direct_deposits), CARDANO_SUCCESS);

  cardano_direct_deposit_map_unref(&direct_deposits);
  cardano_transaction_body_unref(&body);
}

static void
set_fee(cardano_transaction_t* tx, const uint64_t fee)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);

  EXPECT_EQ(cardano_transaction_body_set_fee(body, fee), CARDANO_SUCCESS);

  cardano_transaction_body_unref(&body);
}

static void
set_mint(cardano_transaction_t* tx, const char* policy_hex, const char* asset_name, const int64_t amount)
{
  cardano_transaction_body_t* body      = cardano_transaction_get_body(tx);
  cardano_multi_asset_t*      mint      = cardano_transaction_body_get_mint(body);
  cardano_blake2b_hash_t*     policy_id = NULL;
  cardano_asset_name_t*       name      = NULL;

  if (mint == NULL)
  {
    EXPECT_EQ(cardano_multi_asset_new(&mint), CARDANO_SUCCESS);
  }

  EXPECT_EQ(cardano_blake2b_hash_from_hex(policy_hex, strlen(policy_hex), &policy_id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_from_string(asset_name, strlen(asset_name), &name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_set(mint, policy_id, name, amount), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_mint(body, mint), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&name);
  cardano_multi_asset_unref(&mint);
  cardano_transaction_body_unref(&body);
}

static size_t
get_policy_count(cardano_value_t* value)
{
  cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(value);
  const size_t           count       = cardano_multi_asset_get_policy_count(multi_asset);

  cardano_multi_asset_unref(&multi_asset);

  return count;
}

static int64_t
get_asset_amount(cardano_value_t* value, const char* policy_hex, const char* asset_name)
{
  cardano_multi_asset_t*  multi_asset = cardano_value_get_multi_asset(value);
  cardano_blake2b_hash_t* policy_id   = NULL;
  cardano_asset_name_t*   name        = NULL;
  int64_t                 amount      = 0;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(policy_hex, strlen(policy_hex), &policy_id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_from_string(asset_name, strlen(asset_name), &name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_get(multi_asset, policy_id, name, &amount), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&name);
  cardano_multi_asset_unref(&multi_asset);

  return amount;
}

static void
set_single_input(cardano_transaction_t* tx, cardano_utxo_t* utxo)
{
  cardano_transaction_body_t*      body   = cardano_transaction_get_body(tx);
  cardano_transaction_input_t*     input  = cardano_utxo_get_input(utxo);
  cardano_transaction_input_set_t* inputs = NULL;

  EXPECT_EQ(cardano_transaction_input_set_new(&inputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_add(inputs, input), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_inputs(body, inputs), CARDANO_SUCCESS);

  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_input_unref(&input);
  cardano_transaction_body_unref(&body);
}

static cardano_utxo_t*
new_utxo_with_output(const char* input_utxo_cbor, const char* output_utxo_cbor)
{
  cardano_utxo_t*               input_utxo  = new_default_utxo(input_utxo_cbor);
  cardano_utxo_t*               output_utxo = new_default_utxo(output_utxo_cbor);
  cardano_transaction_input_t*  input       = cardano_utxo_get_input(input_utxo);
  cardano_transaction_output_t* output      = cardano_utxo_get_output(output_utxo);
  cardano_utxo_t*               utxo        = NULL;

  EXPECT_EQ(cardano_utxo_new(input, output, &utxo), CARDANO_SUCCESS);

  cardano_transaction_input_unref(&input);
  cardano_transaction_output_unref(&output);
  cardano_utxo_unref(&input_utxo);
  cardano_utxo_unref(&output_utxo);

  return utxo;
}

static cardano_value_t*
get_utxo_value(cardano_utxo_t* utxo)
{
  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
  cardano_transaction_output_unref(&output);

  cardano_value_t* value = cardano_transaction_output_get_value(output);
  cardano_value_unref(&value);

  return value;
}

static cardano_multi_asset_t*
get_multi_asset_pointer(cardano_value_t* value)
{
  cardano_multi_asset_t* multi_asset = cardano_value_get_multi_asset(value);
  cardano_multi_asset_unref(&multi_asset);

  return multi_asset;
}

static void
add_asset(cardano_value_t* value, const char* policy_hex, const char* asset_name, const int64_t amount)
{
  cardano_blake2b_hash_t* policy_id = NULL;
  cardano_asset_name_t*   name      = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(policy_hex, strlen(policy_hex), &policy_id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_from_string(asset_name, strlen(asset_name), &name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_add_asset(value, policy_id, name, amount), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&name);
}

static size_t
get_asset_count(cardano_value_t* value, const char* policy_hex)
{
  cardano_multi_asset_t*    multi_asset = cardano_value_get_multi_asset(value);
  cardano_blake2b_hash_t*   policy_id   = NULL;
  cardano_asset_name_map_t* assets      = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(policy_hex, strlen(policy_hex), &policy_id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_get_assets(multi_asset, policy_id, &assets), CARDANO_SUCCESS);

  const size_t count = cardano_asset_name_map_get_length(assets);

  cardano_asset_name_map_unref(&assets);
  cardano_blake2b_hash_unref(&policy_id);
  cardano_multi_asset_unref(&multi_asset);

  return count;
}

static char*
encode_writer_hex(cardano_cbor_writer_t* writer)
{
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  EXPECT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return hex;
}

static char*
get_transaction_body_hex(cardano_transaction_t* tx)
{
  cardano_transaction_body_t* body   = cardano_transaction_get_body(tx);
  cardano_cbor_writer_t*      writer = cardano_cbor_writer_new();

  cardano_transaction_body_clear_cbor_cache(body);

  EXPECT_EQ(cardano_transaction_body_to_cbor(body, writer), CARDANO_SUCCESS);

  cardano_transaction_body_unref(&body);

  return encode_writer_hex(writer);
}

static char*
get_sub_transaction_body_hex(cardano_sub_transaction_t* sub_tx)
{
  cardano_sub_transaction_body_t* body   = cardano_sub_transaction_get_body(sub_tx);
  cardano_cbor_writer_t*          writer = cardano_cbor_writer_new();

  cardano_sub_transaction_body_clear_cbor_cache(body);

  EXPECT_EQ(cardano_sub_transaction_body_to_cbor(body, writer), CARDANO_SUCCESS);

  cardano_sub_transaction_body_unref(&body);

  return encode_writer_hex(writer);
}

static cardano_utxo_list_t*
new_empty_utxo_list()
{
  cardano_utxo_list_t* list = NULL;

  cardano_error_t error = cardano_utxo_list_new(&list);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  return list;
};

static cardano_value_t*
new_coin_value(const int64_t coin)
{
  cardano_value_t* value = NULL;

  EXPECT_EQ(cardano_value_new(coin, NULL, &value), CARDANO_SUCCESS);

  return value;
}

static cardano_sub_transaction_t*
new_sub_transaction(cardano_utxo_t* utxo, cardano_value_t* output_value)
{
  cardano_transaction_input_t*       input           = cardano_utxo_get_input(utxo);
  cardano_transaction_output_t*      utxo_output     = cardano_utxo_get_output(utxo);
  cardano_address_t*                 address         = cardano_transaction_output_get_address(utxo_output);
  cardano_transaction_input_set_t*   inputs          = NULL;
  cardano_transaction_output_t*      output          = NULL;
  cardano_transaction_output_list_t* outputs         = NULL;
  cardano_sub_transaction_body_t*    body            = NULL;
  cardano_witness_set_t*             witness_set     = NULL;
  cardano_sub_transaction_t*         sub_transaction = NULL;

  EXPECT_EQ(cardano_transaction_input_set_new(&inputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_add(inputs, input), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_new(address, 0, &output), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_set_value(output, output_value), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_list_new(&outputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_list_add(outputs, output), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_new(inputs, outputs, NULL, &body), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_witness_set_new(&witness_set), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_new(body, witness_set, NULL, &sub_transaction), CARDANO_SUCCESS);

  cardano_transaction_input_unref(&input);
  cardano_transaction_output_unref(&utxo_output);
  cardano_address_unref(&address);
  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_output_unref(&output);
  cardano_transaction_output_list_unref(&outputs);
  cardano_sub_transaction_body_unref(&body);
  cardano_witness_set_unref(&witness_set);

  return sub_transaction;
}

static cardano_sub_transaction_t*
new_coin_sub_transaction(cardano_utxo_t* utxo, const int64_t output_coin)
{
  cardano_value_t*           output_value    = new_coin_value(output_coin);
  cardano_sub_transaction_t* sub_transaction = new_sub_transaction(utxo, output_value);

  cardano_value_unref(&output_value);

  return sub_transaction;
}

static cardano_utxo_t*
new_counterpart_utxo()
{
  return new_utxo_with_output(CBOR_DIFFERENT_VAL3, CBOR_DIFFERENT_VAL2);
}

static cardano_sub_transaction_t*
new_counterpart_sub_transaction()
{
  cardano_utxo_t*  utxo         = new_counterpart_utxo();
  cardano_value_t* output_value = new_coin_value(cardano_value_get_coin(get_utxo_value(utxo)) + 1000015);

  add_asset(output_value, MINT_POLICY_ID, "", 5);

  cardano_sub_transaction_t*      sub_transaction = new_sub_transaction(utxo, output_value);
  cardano_sub_transaction_body_t* body            = cardano_sub_transaction_get_body(sub_transaction);
  cardano_multi_asset_t*          mint            = NULL;
  cardano_blake2b_hash_t*         policy_id       = NULL;
  cardano_asset_name_t*           name            = NULL;

  EXPECT_EQ(cardano_multi_asset_new(&mint), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_blake2b_hash_from_hex(BURN_POLICY_ID, strlen(BURN_POLICY_ID), &policy_id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_asset_name_from_string("TSLA", strlen("TSLA"), &name), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_multi_asset_set(mint, policy_id, name, 3), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_set_mint(body, mint), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&policy_id);
  cardano_asset_name_unref(&name);
  cardano_multi_asset_unref(&mint);
  cardano_sub_transaction_body_unref(&body);
  cardano_value_unref(&output_value);
  cardano_utxo_unref(&utxo);

  return sub_transaction;
}

static void
set_sub_transaction_direct_deposit(cardano_sub_transaction_t* sub_tx, const uint64_t amount)
{
  cardano_sub_transaction_body_t* body            = cardano_sub_transaction_get_body(sub_tx);
  cardano_direct_deposit_map_t*   direct_deposits = NULL;

  EXPECT_EQ(cardano_direct_deposit_map_new(&direct_deposits), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_direct_deposit_map_insert_ex(direct_deposits, REWARD_ADDRESS, strlen(REWARD_ADDRESS), amount), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_set_direct_deposits(body, direct_deposits), CARDANO_SUCCESS);

  cardano_direct_deposit_map_unref(&direct_deposits);
  cardano_sub_transaction_body_unref(&body);
}

static void
set_sub_transaction_direct_deposits(cardano_sub_transaction_t* sub_tx, const uint64_t amount, const uint64_t amount2)
{
  cardano_sub_transaction_body_t* body            = cardano_sub_transaction_get_body(sub_tx);
  cardano_direct_deposit_map_t*   direct_deposits = NULL;

  EXPECT_EQ(cardano_direct_deposit_map_new(&direct_deposits), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_direct_deposit_map_insert_ex(direct_deposits, REWARD_ADDRESS, strlen(REWARD_ADDRESS), amount), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_direct_deposit_map_insert_ex(direct_deposits, REWARD_ADDRESS2, strlen(REWARD_ADDRESS2), amount2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_set_direct_deposits(body, direct_deposits), CARDANO_SUCCESS);

  cardano_direct_deposit_map_unref(&direct_deposits);
  cardano_sub_transaction_body_unref(&body);
}

static void
set_sub_transaction_donation(cardano_sub_transaction_t* sub_tx, const uint64_t donation)
{
  cardano_sub_transaction_body_t* body = cardano_sub_transaction_get_body(sub_tx);

  EXPECT_EQ(cardano_sub_transaction_body_set_donation(body, &donation), CARDANO_SUCCESS);

  cardano_sub_transaction_body_unref(&body);
}

/**
 * Creates a withdrawal map with one or two reward accounts.
 * \param amount the amount withdrawn from the first reward account.
 * \param amount2 the amount withdrawn from the second reward account, or zero to leave it out.
 * \return the withdrawal map.
 */
static cardano_withdrawal_map_t*
new_withdrawal_map(const uint64_t amount, const uint64_t amount2)
{
  cardano_withdrawal_map_t* withdrawals = NULL;

  EXPECT_EQ(cardano_withdrawal_map_new(&withdrawals), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_withdrawal_map_insert_ex(withdrawals, REWARD_ADDRESS, strlen(REWARD_ADDRESS), amount), CARDANO_SUCCESS);

  if (amount2 > 0U)
  {
    EXPECT_EQ(cardano_withdrawal_map_insert_ex(withdrawals, REWARD_ADDRESS2, strlen(REWARD_ADDRESS2), amount2), CARDANO_SUCCESS);
  }

  return withdrawals;
}

/**
 * Creates a certificate set with a single unregistration certificate that reclaims the given deposit.
 * \param deposit the deposit the certificate reclaims.
 * \return the certificate set.
 */
static cardano_certificate_set_t*
new_unregistration_certificate_set(const uint64_t deposit)
{
  cardano_credential_t*          credential     = NULL;
  cardano_unregistration_cert_t* unregistration = NULL;
  cardano_certificate_t*         certificate    = NULL;
  cardano_certificate_set_t*     certificates   = NULL;

  EXPECT_EQ(cardano_credential_from_hash_hex(STAKE_KEY_HASH_HEX, strlen(STAKE_KEY_HASH_HEX), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_unregistration_cert_new(credential, deposit, &unregistration), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_certificate_new_unregistration(unregistration, &certificate), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_certificate_set_new(&certificates), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_certificate_set_add(certificates, certificate), CARDANO_SUCCESS);

  cardano_certificate_unref(&certificate);
  cardano_unregistration_cert_unref(&unregistration);
  cardano_credential_unref(&credential);

  return certificates;
}

/**
 * Adds a Conway registration certificate that pays the given deposit to a certificate set.
 * \param certificates the certificate set.
 * \param key_hash_hex the key hash of the stake credential to register.
 * \param deposit the deposit the certificate pays.
 */
static void
add_registration_certificate(cardano_certificate_set_t* certificates, const char* key_hash_hex, const uint64_t deposit)
{
  cardano_credential_t*        credential   = NULL;
  cardano_registration_cert_t* registration = NULL;
  cardano_certificate_t*       certificate  = NULL;

  EXPECT_EQ(cardano_credential_from_hash_hex(key_hash_hex, strlen(key_hash_hex), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_registration_cert_new(credential, deposit, &registration), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_certificate_new_registration(registration, &certificate), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_certificate_set_add(certificates, certificate), CARDANO_SUCCESS);

  cardano_certificate_unref(&certificate);
  cardano_registration_cert_unref(&registration);
  cardano_credential_unref(&credential);
}

/**
 * Creates a certificate set with two Conway registration certificates of two different stake credentials.
 * \param deposit the deposit the first certificate pays.
 * \param deposit2 the deposit the second certificate pays.
 * \return the certificate set.
 */
static cardano_certificate_set_t*
new_registration_certificate_set(const uint64_t deposit, const uint64_t deposit2)
{
  cardano_certificate_set_t* certificates = NULL;

  EXPECT_EQ(cardano_certificate_set_new(&certificates), CARDANO_SUCCESS);

  add_registration_certificate(certificates, STAKE_KEY_HASH_HEX, deposit);
  add_registration_certificate(certificates, STAKE_KEY_HASH2_HEX, deposit2);

  return certificates;
}

static void
set_registration_deposits(cardano_transaction_t* tx, const uint64_t deposit, const uint64_t deposit2)
{
  cardano_transaction_body_t* body         = cardano_transaction_get_body(tx);
  cardano_certificate_set_t*  certificates = new_registration_certificate_set(deposit, deposit2);

  EXPECT_EQ(cardano_transaction_body_set_certificates(body, certificates), CARDANO_SUCCESS);

  cardano_certificate_set_unref(&certificates);
  cardano_transaction_body_unref(&body);
}

static void
set_sub_transaction_registration_deposits(cardano_sub_transaction_t* sub_tx, const uint64_t deposit, const uint64_t deposit2)
{
  cardano_sub_transaction_body_t* body         = cardano_sub_transaction_get_body(sub_tx);
  cardano_certificate_set_t*      certificates = new_registration_certificate_set(deposit, deposit2);

  EXPECT_EQ(cardano_sub_transaction_body_set_certificates(body, certificates), CARDANO_SUCCESS);

  cardano_certificate_set_unref(&certificates);
  cardano_sub_transaction_body_unref(&body);
}

static void
set_withdrawals(cardano_transaction_t* tx, const uint64_t amount, const uint64_t amount2)
{
  cardano_transaction_body_t* body        = cardano_transaction_get_body(tx);
  cardano_withdrawal_map_t*   withdrawals = new_withdrawal_map(amount, amount2);

  EXPECT_EQ(cardano_transaction_body_set_withdrawals(body, withdrawals), CARDANO_SUCCESS);

  cardano_withdrawal_map_unref(&withdrawals);
  cardano_transaction_body_unref(&body);
}

static void
set_reclaimed_deposit(cardano_transaction_t* tx, const uint64_t deposit)
{
  cardano_transaction_body_t* body         = cardano_transaction_get_body(tx);
  cardano_certificate_set_t*  certificates = new_unregistration_certificate_set(deposit);

  EXPECT_EQ(cardano_transaction_body_set_certificates(body, certificates), CARDANO_SUCCESS);

  cardano_certificate_set_unref(&certificates);
  cardano_transaction_body_unref(&body);
}

static void
set_sub_transaction_withdrawals(cardano_sub_transaction_t* sub_tx, const uint64_t amount, const uint64_t amount2)
{
  cardano_sub_transaction_body_t* body        = cardano_sub_transaction_get_body(sub_tx);
  cardano_withdrawal_map_t*       withdrawals = new_withdrawal_map(amount, amount2);

  EXPECT_EQ(cardano_sub_transaction_body_set_withdrawals(body, withdrawals), CARDANO_SUCCESS);

  cardano_withdrawal_map_unref(&withdrawals);
  cardano_sub_transaction_body_unref(&body);
}

static void
set_sub_transaction_reclaimed_deposit(cardano_sub_transaction_t* sub_tx, const uint64_t deposit)
{
  cardano_sub_transaction_body_t* body         = cardano_sub_transaction_get_body(sub_tx);
  cardano_certificate_set_t*      certificates = new_unregistration_certificate_set(deposit);

  EXPECT_EQ(cardano_sub_transaction_body_set_certificates(body, certificates), CARDANO_SUCCESS);

  cardano_certificate_set_unref(&certificates);
  cardano_sub_transaction_body_unref(&body);
}

static void
add_sub_transaction(cardano_transaction_t* tx, cardano_sub_transaction_t* sub_tx)
{
  cardano_transaction_body_t*    body             = cardano_transaction_get_body(tx);
  cardano_sub_transaction_set_t* sub_transactions = cardano_transaction_body_get_sub_transactions(body);

  if (sub_transactions == NULL)
  {
    EXPECT_EQ(cardano_sub_transaction_set_new(&sub_transactions), CARDANO_SUCCESS);
  }

  EXPECT_EQ(cardano_sub_transaction_set_add(sub_transactions, sub_tx), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_body_set_sub_transactions(body, sub_transactions), CARDANO_SUCCESS);

  cardano_sub_transaction_set_unref(&sub_transactions);
  cardano_transaction_body_unref(&body);
}

static cardano_utxo_list_t*
new_batch_utxo_list()
{
  cardano_utxo_list_t* list        = new_empty_utxo_list();
  cardano_utxo_t*      gai1        = new_default_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*      gai2        = new_default_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_t*      sub_tx_utxo = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*      counterpart = new_counterpart_utxo();

  EXPECT_EQ(cardano_utxo_list_add(list, gai1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(list, gai2), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(list, sub_tx_utxo), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(list, counterpart), CARDANO_SUCCESS);

  cardano_utxo_unref(&gai1);
  cardano_utxo_unref(&gai2);
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&counterpart);

  return list;
}

static cardano_transaction_t*
new_batch_transaction(const char* cbor, const bool add_default, const bool add_counterpart)
{
  cardano_transaction_t* tx = new_default_transaction(cbor);

  if (add_default)
  {
    cardano_sub_transaction_t* sub_tx = new_default_sub_transaction();

    add_sub_transaction(tx, sub_tx);

    cardano_sub_transaction_unref(&sub_tx);
  }

  if (add_counterpart)
  {
    cardano_sub_transaction_t* sub_tx = new_counterpart_sub_transaction();

    add_sub_transaction(tx, sub_tx);

    cardano_sub_transaction_unref(&sub_tx);
  }

  return tx;
}

static cardano_tx_evaluator_impl_t
cardano_evaluator_impl_new()
{
  cardano_tx_evaluator_impl_t impl = { 0 };

  impl.evaluate = [](cardano_tx_evaluator_impl_t*, cardano_transaction_t* tx, cardano_utxo_list_t*, cardano_redeemer_list_t** output) -> cardano_error_t
  {
    cardano_witness_set_t* witness = cardano_transaction_get_witness_set(tx);
    cardano_witness_set_unref(&witness);

    cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness);
    cardano_redeemer_list_unref(&redeemers);

    cardano_redeemer_list_t* clone = NULL;
    EXPECT_EQ(cardano_redeemer_list_clone(redeemers, &clone), CARDANO_SUCCESS);

    const size_t redeemers_count = cardano_redeemer_list_get_length(clone);

    cardano_ex_units_t* ex_units = nullptr;
    EXPECT_EQ(cardano_ex_units_new(1000000000, 5000000000, &ex_units), CARDANO_SUCCESS);

    for (size_t i = 0; i < redeemers_count; i++)
    {
      cardano_redeemer_t* redeemer = NULL;
      EXPECT_EQ(cardano_redeemer_list_get(clone, i, &redeemer), CARDANO_SUCCESS);
      cardano_redeemer_unref(&redeemer);

      EXPECT_EQ(cardano_redeemer_set_ex_units(redeemer, ex_units), CARDANO_SUCCESS);
    }

    cardano_ex_units_unref(&ex_units);

    *output = clone;

    return CARDANO_SUCCESS;
  };

  return impl;
}

/**
 * Creates an evaluator implementation that keeps the execution units every redeemer declares.
 * \return The evaluator implementation.
 */
static cardano_tx_evaluator_impl_t
cardano_declared_units_evaluator_impl_new()
{
  cardano_tx_evaluator_impl_t impl = { 0 };

  impl.evaluate = [](cardano_tx_evaluator_impl_t*, cardano_transaction_t* tx, cardano_utxo_list_t*, cardano_redeemer_list_t** output) -> cardano_error_t
  {
    cardano_witness_set_t* witness = cardano_transaction_get_witness_set(tx);
    cardano_witness_set_unref(&witness);

    cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness);
    cardano_redeemer_list_unref(&redeemers);

    return cardano_redeemer_list_clone(redeemers, output);
  };

  return impl;
}

static cardano_address_t*
create_address(const char* address)
{
  cardano_address_t* payment_address = NULL;

  cardano_error_t result = cardano_address_from_string(address, strlen(address), &payment_address);

  if (result != CARDANO_SUCCESS)
  {
    return NULL;
  }

  return payment_address;
}

// Captures the resolved UTxO set handed to the evaluator so a test can assert
// which UTxOs the balancer forwarded (e.g. the reference inputs).
static cardano_utxo_list_t* g_recorded_eval_utxos = nullptr;

static cardano_tx_evaluator_impl_t
cardano_recording_evaluator_impl_new()
{
  cardano_tx_evaluator_impl_t impl = { 0 };

  impl.evaluate = [](cardano_tx_evaluator_impl_t*, cardano_transaction_t* tx, cardano_utxo_list_t* utxos, cardano_redeemer_list_t** output) -> cardano_error_t
  {
    cardano_utxo_list_unref(&g_recorded_eval_utxos);
    g_recorded_eval_utxos = utxos;
    cardano_utxo_list_ref(utxos);

    cardano_witness_set_t* witness = cardano_transaction_get_witness_set(tx);
    cardano_witness_set_unref(&witness);

    cardano_redeemer_list_t* redeemers = cardano_witness_set_get_redeemers(witness);
    cardano_redeemer_list_unref(&redeemers);

    cardano_redeemer_list_t* clone = NULL;
    EXPECT_EQ(cardano_redeemer_list_clone(redeemers, &clone), CARDANO_SUCCESS);

    const size_t redeemers_count = cardano_redeemer_list_get_length(clone);

    cardano_ex_units_t* ex_units = nullptr;
    EXPECT_EQ(cardano_ex_units_new(1000000000, 5000000000, &ex_units), CARDANO_SUCCESS);

    for (size_t i = 0; i < redeemers_count; i++)
    {
      cardano_redeemer_t* redeemer = NULL;
      EXPECT_EQ(cardano_redeemer_list_get(clone, i, &redeemer), CARDANO_SUCCESS);
      cardano_redeemer_unref(&redeemer);

      EXPECT_EQ(cardano_redeemer_set_ex_units(redeemer, ex_units), CARDANO_SUCCESS);
    }

    cardano_ex_units_unref(&ex_units);

    *output = clone;

    return CARDANO_SUCCESS;
  };

  return impl;
}

static bool
utxo_list_contains_input(cardano_utxo_list_t* list, cardano_blake2b_hash_t* id, uint64_t index)
{
  const size_t length = cardano_utxo_list_get_length(list);

  for (size_t i = 0; i < length; ++i)
  {
    cardano_utxo_t* utxo = NULL;
    EXPECT_EQ(cardano_utxo_list_get(list, i, &utxo), CARDANO_SUCCESS);

    cardano_transaction_input_t* input    = cardano_utxo_get_input(utxo);
    cardano_blake2b_hash_t*      input_id = cardano_transaction_input_get_id(input);

    const bool matches = (cardano_transaction_input_get_index(input) == index) && cardano_blake2b_hash_equals(input_id, id);

    cardano_blake2b_hash_unref(&input_id);
    cardano_transaction_input_unref(&input);
    cardano_utxo_unref(&utxo);

    if (matches)
    {
      return true;
    }
  }

  return false;
}

static cardano_utxo_list_t*
new_utxo_list_of(cardano_utxo_t* first, cardano_utxo_t* second)
{
  cardano_utxo_list_t* list = new_empty_utxo_list();

  EXPECT_EQ(cardano_utxo_list_add(list, first), CARDANO_SUCCESS);

  if (second != NULL)
  {
    EXPECT_EQ(cardano_utxo_list_add(list, second), CARDANO_SUCCESS);
  }

  return list;
}

static cardano_transaction_t*
new_top_level_transaction(cardano_sub_transaction_t* first, cardano_sub_transaction_t* second)
{
  cardano_transaction_t* tx = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);

  if (first != NULL)
  {
    add_sub_transaction(tx, first);
  }

  if (second != NULL)
  {
    add_sub_transaction(tx, second);
  }

  return tx;
}

/**
 * Adds to a list the UTXOs of another list whose input the list does not hold yet.
 * \param list the list that receives the UTXOs.
 * \param utxos the UTXOs to add, or NULL.
 */
static void
add_missing_utxos(cardano_utxo_list_t* list, cardano_utxo_list_t* utxos)
{
  for (size_t i = 0; i < cardano_utxo_list_get_length(utxos); ++i)
  {
    cardano_utxo_t* utxo = NULL;

    EXPECT_EQ(cardano_utxo_list_get(utxos, i, &utxo), CARDANO_SUCCESS);

    cardano_transaction_input_t* input = cardano_utxo_get_input(utxo);
    cardano_blake2b_hash_t*      id    = cardano_transaction_input_get_id(input);

    if (!utxo_list_contains_input(list, id, cardano_transaction_input_get_index(input)))
    {
      EXPECT_EQ(cardano_utxo_list_add(list, utxo), CARDANO_SUCCESS);
    }

    cardano_blake2b_hash_unref(&id);
    cardano_transaction_input_unref(&input);
    cardano_utxo_unref(&utxo);
  }
}

/**
 * Creates the available UTXOs a batch is balanced with: the UTXOs the top level transaction may spend plus the
 * resolved UTXOs of its sub transactions, which the balancer takes from the same list.
 * \param available_utxo the UTXOs the top level transaction may spend.
 * \param sub_transaction_inputs the UTXOs that resolve the inputs spent by the sub transactions, or NULL.
 * \param sub_transaction_references the UTXOs that resolve the reference inputs of the sub transactions, or NULL.
 * \return A new instance of the list.
 */
static cardano_utxo_list_t*
new_batch_available_utxo(
  cardano_utxo_list_t* available_utxo,
  cardano_utxo_list_t* sub_transaction_inputs,
  cardano_utxo_list_t* sub_transaction_references)
{
  cardano_utxo_list_t* list = new_empty_utxo_list();

  add_missing_utxos(list, available_utxo);
  add_missing_utxos(list, sub_transaction_inputs);
  add_missing_utxos(list, sub_transaction_references);

  return list;
}

/**
 * Makes a sub transaction reference the input of a UTXO.
 * \param sub_tx the sub transaction.
 * \param utxo the UTXO to reference.
 */
static void
set_sub_transaction_reference_input(cardano_sub_transaction_t* sub_tx, cardano_utxo_t* utxo)
{
  cardano_sub_transaction_body_t*  body             = cardano_sub_transaction_get_body(sub_tx);
  cardano_transaction_input_t*     input            = cardano_utxo_get_input(utxo);
  cardano_transaction_input_set_t* reference_inputs = NULL;

  EXPECT_EQ(cardano_transaction_input_set_new(&reference_inputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_set_add(reference_inputs, input), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_set_reference_inputs(body, reference_inputs), CARDANO_SUCCESS);

  cardano_transaction_input_set_unref(&reference_inputs);
  cardano_transaction_input_unref(&input);
  cardano_sub_transaction_body_unref(&body);
}

/**
 * Creates a sub transaction that spends no inputs and pays a single output.
 * \param utxo the UTXO whose address receives the output.
 * \param output_coin the lovelace paid by the output.
 * \return A new instance of the sub transaction.
 */
static cardano_sub_transaction_t*
new_sub_transaction_without_inputs(cardano_utxo_t* utxo, const int64_t output_coin)
{
  cardano_sub_transaction_t*       sub_transaction = new_coin_sub_transaction(utxo, output_coin);
  cardano_sub_transaction_body_t*  body            = cardano_sub_transaction_get_body(sub_transaction);
  cardano_transaction_input_set_t* inputs          = NULL;

  EXPECT_EQ(cardano_transaction_input_set_new(&inputs), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_set_inputs(body, inputs), CARDANO_SUCCESS);

  cardano_transaction_input_set_unref(&inputs);
  cardano_sub_transaction_body_unref(&body);

  return sub_transaction;
}

static cardano_error_t
balance_batch_with_pre_selected(
  cardano_transaction_t*         tx,
  cardano_protocol_parameters_t* protocol,
  cardano_utxo_list_t*           reference_inputs,
  cardano_utxo_list_t*           pre_selected_utxo,
  cardano_utxo_list_t*           sub_transaction_inputs,
  cardano_utxo_list_t*           available_utxo)
{
  cardano_coin_selector_t* coin_selector   = NULL;
  cardano_address_t*       change_address  = create_address(BATCH_CHANGE_ADDR);
  cardano_utxo_list_t*     batch_available = new_batch_available_utxo(available_utxo, sub_transaction_inputs, NULL);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);

  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    pre_selected_utxo,
    NULL,
    batch_available,
    coin_selector,
    change_address,
    NULL,
    change_address,
    NULL,
    nullptr);

  cardano_coin_selector_unref(&coin_selector);
  cardano_address_unref(&change_address);
  cardano_utxo_list_unref(&batch_available);

  return result;
}

static cardano_error_t
balance_batch_with_collateral(
  cardano_transaction_t*         tx,
  cardano_protocol_parameters_t* protocol,
  cardano_utxo_list_t*           reference_inputs,
  cardano_utxo_list_t*           sub_transaction_inputs,
  cardano_utxo_list_t*           sub_transaction_references,
  cardano_utxo_list_t*           available_utxo,
  cardano_utxo_list_t*           collateral_utxo,
  cardano_tx_evaluator_t*        evaluator = NULL)
{
  cardano_coin_selector_t* coin_selector   = NULL;
  cardano_address_t*       change_address  = create_address(BATCH_CHANGE_ADDR);
  cardano_utxo_list_t*     batch_available = new_batch_available_utxo(available_utxo, sub_transaction_inputs, sub_transaction_references);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);

  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    batch_available,
    coin_selector,
    change_address,
    collateral_utxo,
    change_address,
    evaluator,
    nullptr);

  cardano_coin_selector_unref(&coin_selector);
  cardano_address_unref(&change_address);
  cardano_utxo_list_unref(&batch_available);

  return result;
}

static void
set_sub_transaction_redeemer(cardano_sub_transaction_t* sub_tx, const uint64_t memory, const uint64_t cpu_steps)
{
  cardano_witness_set_t*   witness_set = cardano_sub_transaction_get_witness_set(sub_tx);
  cardano_redeemer_list_t* redeemers   = NULL;
  cardano_redeemer_t*      redeemer    = NULL;
  cardano_plutus_data_t*   data        = NULL;
  cardano_ex_units_t*      ex_units    = NULL;

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
}

static uint64_t
get_fee(cardano_transaction_t* tx)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  return cardano_transaction_body_get_fee(body);
}

static size_t
get_input_count(cardano_transaction_t* tx)
{
  cardano_transaction_body_t*      body   = cardano_transaction_get_body(tx);
  cardano_transaction_input_set_t* inputs = cardano_transaction_body_get_inputs(body);

  const size_t count = cardano_transaction_input_set_get_length(inputs);

  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_body_unref(&body);

  return count;
}

static size_t
get_collateral_count(cardano_transaction_t* tx)
{
  cardano_transaction_body_t*      body       = cardano_transaction_get_body(tx);
  cardano_transaction_input_set_t* collateral = cardano_transaction_body_get_collateral(body);

  const size_t count = cardano_transaction_input_set_get_length(collateral);

  cardano_transaction_input_set_unref(&collateral);
  cardano_transaction_body_unref(&body);

  return count;
}

/**
 * Checks whether the collateral of a transaction holds the input of a UTXO.
 * \param tx the transaction.
 * \param utxo the UTXO to look for.
 * \return true if the input of the UTXO is among the collateral inputs of the transaction.
 */
static bool
collateral_contains(cardano_transaction_t* tx, cardano_utxo_t* utxo)
{
  cardano_transaction_body_t*      body       = cardano_transaction_get_body(tx);
  cardano_transaction_input_set_t* collateral = cardano_transaction_body_get_collateral(body);
  cardano_transaction_input_t*     utxo_input = cardano_utxo_get_input(utxo);
  bool                             found      = false;

  for (size_t i = 0; i < cardano_transaction_input_set_get_length(collateral); ++i)
  {
    cardano_transaction_input_t* input = NULL;

    EXPECT_EQ(cardano_transaction_input_set_get(collateral, i, &input), CARDANO_SUCCESS);

    found = found || cardano_transaction_input_equals(input, utxo_input);

    cardano_transaction_input_unref(&input);
  }

  cardano_transaction_input_unref(&utxo_input);
  cardano_transaction_input_set_unref(&collateral);
  cardano_transaction_body_unref(&body);

  return found;
}

/**
 * Adds a spend redeemer to the witness set of a top level transaction.
 * \param tx the transaction.
 * \param memory the memory units the redeemer declares.
 * \param cpu_steps the CPU steps the redeemer declares.
 */
static void
set_top_level_redeemer(cardano_transaction_t* tx, const uint64_t memory, const uint64_t cpu_steps)
{
  cardano_witness_set_t*   witness_set = cardano_transaction_get_witness_set(tx);
  cardano_redeemer_list_t* redeemers   = NULL;
  cardano_redeemer_t*      redeemer    = NULL;
  cardano_plutus_data_t*   data        = NULL;
  cardano_ex_units_t*      ex_units    = NULL;

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
}

static uint64_t
get_total_collateral(cardano_transaction_t* tx)
{
  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  const uint64_t* total_collateral = cardano_transaction_body_get_total_collateral(body);

  return (total_collateral == NULL) ? 0U : *total_collateral;
}

static uint64_t
compute_ex_units_fee(cardano_protocol_parameters_t* protocol, const uint64_t memory, const uint64_t cpu_steps)
{
  cardano_ex_unit_prices_t* prices        = cardano_protocol_parameters_get_execution_costs(protocol);
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

static uint64_t
compute_size_fee(cardano_transaction_t* tx)
{
  uint64_t size_fee = 0U;

  EXPECT_EQ(cardano_compute_min_fee_without_scripts(tx, 155381, 44, &size_fee), CARDANO_SUCCESS);

  return size_fee;
}

static cardano_error_t
balance_batch(
  cardano_transaction_t*         tx,
  cardano_protocol_parameters_t* protocol,
  cardano_utxo_list_t*           reference_inputs,
  cardano_utxo_list_t*           sub_transaction_inputs,
  cardano_utxo_list_t*           available_utxo)
{
  return balance_batch_with_pre_selected(tx, protocol, reference_inputs, NULL, sub_transaction_inputs, available_utxo);
}

static bool
is_batch_balanced(
  cardano_transaction_t*         tx,
  cardano_protocol_parameters_t* protocol,
  cardano_utxo_list_t*           sub_transaction_inputs,
  cardano_utxo_list_t*           available_utxo)
{
  cardano_utxo_list_t* resolved_inputs = cardano_utxo_list_concat(available_utxo, sub_transaction_inputs);
  bool                 is_balanced     = false;

  EXPECT_EQ(cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced), CARDANO_SUCCESS);

  cardano_utxo_list_unref(&resolved_inputs);

  return is_balanced;
}

static cardano_value_t*
new_top_level_imbalance(
  cardano_transaction_t*         tx,
  cardano_protocol_parameters_t* protocol,
  cardano_utxo_list_t*           available_utxo)
{
  cardano_value_t* imbalance = NULL;

  EXPECT_EQ(cardano_compute_transaction_imbalance(tx, available_utxo, protocol, &imbalance), CARDANO_SUCCESS);

  return imbalance;
}

static bool
transaction_spends(cardano_transaction_t* tx, cardano_utxo_t* utxo)
{
  cardano_transaction_body_t*      body       = cardano_transaction_get_body(tx);
  cardano_transaction_input_set_t* inputs     = cardano_transaction_body_get_inputs(body);
  cardano_transaction_input_t*     utxo_input = cardano_utxo_get_input(utxo);
  bool                             is_spent   = false;

  for (size_t i = 0; i < cardano_transaction_input_set_get_length(inputs); ++i)
  {
    cardano_transaction_input_t* input = NULL;

    EXPECT_EQ(cardano_transaction_input_set_get(inputs, i, &input), CARDANO_SUCCESS);

    is_spent = is_spent || cardano_transaction_input_equals(input, utxo_input);

    cardano_transaction_input_unref(&input);
  }

  cardano_transaction_input_unref(&utxo_input);
  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_body_unref(&body);

  return is_spent;
}

static void
add_plutus_script(cardano_transaction_t* tx, const cardano_script_language_t language)
{
  cardano_witness_set_t* witness_set = cardano_transaction_get_witness_set(tx);

  if (language == CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1)
  {
    cardano_plutus_v1_script_t*     script  = NULL;
    cardano_plutus_v1_script_set_t* scripts = NULL;

    EXPECT_EQ(cardano_plutus_v1_script_new_bytes_from_hex(PLUTUS_SCRIPT_HEX, strlen(PLUTUS_SCRIPT_HEX), &script), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_v1_script_set_new(&scripts), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_v1_script_set_add(scripts, script), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_witness_set_set_plutus_v1_scripts(witness_set, scripts), CARDANO_SUCCESS);

    cardano_plutus_v1_script_unref(&script);
    cardano_plutus_v1_script_set_unref(&scripts);
  }
  else if (language == CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2)
  {
    cardano_plutus_v2_script_t*     script  = NULL;
    cardano_plutus_v2_script_set_t* scripts = NULL;

    EXPECT_EQ(cardano_plutus_v2_script_new_bytes_from_hex(PLUTUS_SCRIPT_HEX, strlen(PLUTUS_SCRIPT_HEX), &script), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_v2_script_set_new(&scripts), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_v2_script_set_add(scripts, script), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_witness_set_set_plutus_v2_scripts(witness_set, scripts), CARDANO_SUCCESS);

    cardano_plutus_v2_script_unref(&script);
    cardano_plutus_v2_script_set_unref(&scripts);
  }
  else
  {
    cardano_plutus_v3_script_t*     script  = NULL;
    cardano_plutus_v3_script_set_t* scripts = NULL;

    EXPECT_EQ(cardano_plutus_v3_script_new_bytes_from_hex(PLUTUS_SCRIPT_HEX, strlen(PLUTUS_SCRIPT_HEX), &script), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_v3_script_set_new(&scripts), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_plutus_v3_script_set_add(scripts, script), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_witness_set_set_plutus_v3_scripts(witness_set, scripts), CARDANO_SUCCESS);

    cardano_plutus_v3_script_unref(&script);
    cardano_plutus_v3_script_set_unref(&scripts);
  }

  cardano_witness_set_unref(&witness_set);
}

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
 * Creates the enterprise address controlled by a script, which no key signs for.
 * \param script_hash_hex the hash of the script.
 * \return A new instance of the address.
 */
static cardano_address_t*
new_script_address(const char* script_hash_hex)
{
  cardano_credential_t*         credential         = NULL;
  cardano_enterprise_address_t* enterprise_address = NULL;

  EXPECT_EQ(cardano_credential_from_hash_hex(script_hash_hex, strlen(script_hash_hex), CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, &credential), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_enterprise_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, credential, &enterprise_address), CARDANO_SUCCESS);

  cardano_address_t* address = cardano_enterprise_address_to_address(enterprise_address);

  cardano_credential_unref(&credential);
  cardano_enterprise_address_unref(&enterprise_address);

  return address;
}

/**
 * Creates a UTXO that holds only lovelace.
 * \param ordinal a number that makes the transaction id of the UTXO unique.
 * \param address the address that owns the UTXO.
 * \param coin the lovelace held by the UTXO.
 * \return A new instance of the UTXO.
 */
static cardano_utxo_t*
new_coin_utxo(const uint64_t ordinal, cardano_address_t* address, const int64_t coin)
{
  char hex[65] = { 0 };

  EXPECT_EQ(snprintf(hex, sizeof(hex), "%064llx", (unsigned long long)ordinal), 64);

  cardano_blake2b_hash_t*       id     = NULL;
  cardano_transaction_input_t*  input  = NULL;
  cardano_transaction_output_t* output = NULL;
  cardano_utxo_t*               utxo   = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(hex, 64, &id), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_input_new(id, 0, &input), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_transaction_output_new(address, (uint64_t)coin, &output), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_new(input, output, &utxo), CARDANO_SUCCESS);

  cardano_blake2b_hash_unref(&id);
  cardano_transaction_input_unref(&input);
  cardano_transaction_output_unref(&output);

  return utxo;
}

/**
 * Creates the VK witness of a signer over a message.
 * \param signer the signer that signs.
 * \param message the hash to sign, a transaction id.
 * \return A new instance of the VK witness.
 */
static cardano_vkey_witness_t*
new_vkey_witness(const signer_t& signer, cardano_blake2b_hash_t* message)
{
  cardano_ed25519_signature_t* signature = NULL;
  cardano_vkey_witness_t*      witness   = NULL;

  EXPECT_EQ(cardano_ed25519_private_key_sign(signer.private_key, cardano_blake2b_hash_get_data(message), cardano_blake2b_hash_get_bytes_size(message), &signature), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_vkey_witness_new(signer.public_key, signature, &witness), CARDANO_SUCCESS);

  cardano_ed25519_signature_unref(&signature);

  return witness;
}

/**
 * Signs a transaction with the keys of the given signers and applies their VK witnesses to it.
 * \param tx the transaction to sign.
 * \param signers the signers of the transaction.
 */
static void
sign_transaction(cardano_transaction_t* tx, const std::vector<const signer_t*>& signers)
{
  cardano_blake2b_hash_t*     tx_id     = cardano_transaction_get_id(tx);
  cardano_vkey_witness_set_t* witnesses = NULL;

  EXPECT_EQ(cardano_vkey_witness_set_new(&witnesses), CARDANO_SUCCESS);

  for (const signer_t* signer: signers)
  {
    cardano_vkey_witness_t* witness = new_vkey_witness(*signer, tx_id);

    EXPECT_EQ(cardano_vkey_witness_set_add(witnesses, witness), CARDANO_SUCCESS);

    cardano_vkey_witness_unref(&witness);
  }

  EXPECT_EQ(cardano_transaction_apply_vkey_witnesses(tx, witnesses), CARDANO_SUCCESS);

  cardano_vkey_witness_set_unref(&witnesses);
  cardano_blake2b_hash_unref(&tx_id);
}

/**
 * Gets the number of VK witnesses held by the witness set of a transaction.
 * \param tx the transaction.
 * \return The number of VK witnesses.
 */
static size_t
get_vkey_witness_count(cardano_transaction_t* tx)
{
  cardano_witness_set_t*      witness_set = cardano_transaction_get_witness_set(tx);
  cardano_vkey_witness_set_t* witnesses   = cardano_witness_set_get_vkeys(witness_set);

  const size_t count = cardano_vkey_witness_set_get_length(witnesses);

  cardano_vkey_witness_set_unref(&witnesses);
  cardano_witness_set_unref(&witness_set);

  return count;
}

/**
 * Adds to the witness set of a transaction a native script that requires the signature of a signer.
 * \param tx the transaction.
 * \param signer the signer the native script requires.
 */
static void
add_native_script(cardano_transaction_t* tx, const signer_t& signer)
{
  cardano_witness_set_t*       witness_set   = cardano_transaction_get_witness_set(tx);
  cardano_script_pubkey_t*     script_pubkey = NULL;
  cardano_native_script_t*     script        = NULL;
  cardano_native_script_set_t* scripts       = NULL;

  EXPECT_EQ(cardano_script_pubkey_new(signer.key_hash, &script_pubkey), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_native_script_new_pubkey(script_pubkey, &script), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_native_script_set_new(&scripts), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_native_script_set_add(scripts, script), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_witness_set_set_native_scripts(witness_set, scripts), CARDANO_SUCCESS);

  cardano_script_pubkey_unref(&script_pubkey);
  cardano_native_script_unref(&script);
  cardano_native_script_set_unref(&scripts);
  cardano_witness_set_unref(&witness_set);
}

/**
 * Sets on the witness set of a transaction the VK witnesses of the given signers, as a transaction that was handed over
 * already signed by other parties carries them.
 * \param tx the transaction.
 * \param signers the signers whose VK witnesses are added, possibly none.
 * \param use_tag whether the VK witness set is encoded with the set tag, false for a legacy untagged set.
 */
static void
set_vkey_witnesses(cardano_transaction_t* tx, const std::vector<const signer_t*>& signers, const bool use_tag)
{
  cardano_witness_set_t*      witness_set = cardano_transaction_get_witness_set(tx);
  cardano_blake2b_hash_t*     tx_id       = cardano_transaction_get_id(tx);
  cardano_vkey_witness_set_t* witnesses   = NULL;

  EXPECT_EQ(cardano_vkey_witness_set_new(&witnesses), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_vkey_witness_set_set_use_tag(witnesses, use_tag), CARDANO_SUCCESS);

  for (const signer_t* signer: signers)
  {
    cardano_vkey_witness_t* witness = new_vkey_witness(*signer, tx_id);

    EXPECT_EQ(cardano_vkey_witness_set_add(witnesses, witness), CARDANO_SUCCESS);

    cardano_vkey_witness_unref(&witness);
  }

  EXPECT_EQ(cardano_witness_set_set_vkeys(witness_set, witnesses), CARDANO_SUCCESS);

  cardano_vkey_witness_set_unref(&witnesses);
  cardano_blake2b_hash_unref(&tx_id);
  cardano_witness_set_unref(&witness_set);
}

/**
 * Adds to the witness set of a transaction the VK witness of a signer, as a transaction that was handed over already
 * signed by another party carries it.
 * \param tx the transaction.
 * \param signer the signer whose VK witness is added.
 */
static void
add_vkey_witness(cardano_transaction_t* tx, const signer_t& signer)
{
  set_vkey_witnesses(tx, { &signer }, true);
}

/**
 * Creates the owner of a Byron address from a private key.
 * \param private_key_hex the Ed25519 private key of the signer.
 * \param address the base58 representation of the Byron address the signer holds.
 * \param attributes_hex the CBOR encoded attributes of the address.
 * \return The new signer. The caller must release it with \ref free_byron_signer.
 */
static byron_signer_t
new_byron_signer(const char* private_key_hex, const char* address, const char* attributes_hex)
{
  byron_signer_t signer = { NULL, NULL, NULL, NULL, NULL };

  EXPECT_EQ(cardano_ed25519_private_key_from_normal_hex(private_key_hex, strlen(private_key_hex), &signer.private_key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_ed25519_private_key_get_public_key(signer.private_key, &signer.public_key), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_address_from_string(address, strlen(address), &signer.address), CARDANO_SUCCESS);

  signer.chain_code = cardano_buffer_from_hex(BYRON_CHAIN_CODE_HEX, strlen(BYRON_CHAIN_CODE_HEX));
  signer.attributes = cardano_buffer_from_hex(attributes_hex, strlen(attributes_hex));

  EXPECT_NE(signer.chain_code, nullptr);
  EXPECT_NE(signer.attributes, nullptr);

  return signer;
}

/**
 * Releases everything a Byron signer holds.
 * \param signer the signer to release.
 */
static void
free_byron_signer(byron_signer_t& signer)
{
  cardano_ed25519_private_key_unref(&signer.private_key);
  cardano_ed25519_public_key_unref(&signer.public_key);
  cardano_buffer_unref(&signer.chain_code);
  cardano_buffer_unref(&signer.attributes);
  cardano_address_unref(&signer.address);
}

/**
 * Adds to the witness set of a transaction the bootstrap witnesses of the given Byron signers, over the id of the
 * transaction, creating the bootstrap witness set as a new set if the witness set carries none.
 * \param tx the transaction.
 * \param signers the Byron signers whose bootstrap witnesses are added.
 */
static void
add_bootstrap_witnesses(cardano_transaction_t* tx, const std::vector<const byron_signer_t*>& signers)
{
  cardano_blake2b_hash_t*          tx_id       = cardano_transaction_get_id(tx);
  cardano_witness_set_t*           witness_set = cardano_transaction_get_witness_set(tx);
  cardano_bootstrap_witness_set_t* witnesses   = cardano_witness_set_get_bootstrap(witness_set);

  if (witnesses == NULL)
  {
    EXPECT_EQ(cardano_bootstrap_witness_set_new(&witnesses), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_witness_set_set_bootstrap(witness_set, witnesses), CARDANO_SUCCESS);
  }

  for (const byron_signer_t* signer: signers)
  {
    cardano_ed25519_signature_t* signature = NULL;
    cardano_bootstrap_witness_t* witness   = NULL;

    EXPECT_EQ(cardano_ed25519_private_key_sign(signer->private_key, cardano_blake2b_hash_get_data(tx_id), cardano_blake2b_hash_get_bytes_size(tx_id), &signature), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_bootstrap_witness_new(signer->public_key, signature, signer->chain_code, signer->attributes, &witness), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_bootstrap_witness_set_add(witnesses, witness), CARDANO_SUCCESS);

    cardano_ed25519_signature_unref(&signature);
    cardano_bootstrap_witness_unref(&witness);
  }

  cardano_bootstrap_witness_set_unref(&witnesses);
  cardano_witness_set_unref(&witness_set);
  cardano_blake2b_hash_unref(&tx_id);
}

/**
 * Gets the number of bootstrap witnesses held by the witness set of a transaction.
 * \param tx the transaction.
 * \return The number of bootstrap witnesses.
 */
static size_t
get_bootstrap_witness_count(cardano_transaction_t* tx)
{
  cardano_witness_set_t*           witness_set = cardano_transaction_get_witness_set(tx);
  cardano_bootstrap_witness_set_t* witnesses   = cardano_witness_set_get_bootstrap(witness_set);

  const size_t count = cardano_bootstrap_witness_set_get_length(witnesses);

  cardano_bootstrap_witness_set_unref(&witnesses);
  cardano_witness_set_unref(&witness_set);

  return count;
}

/**
 * Creates signers whose private keys are derived from their position, so each one holds a distinct key.
 * \param count the number of signers.
 * \return The new signers. The caller must release each one with \ref free_signer.
 */
static std::vector<signer_t>
new_signers(const size_t count)
{
  std::vector<signer_t> signers;

  for (size_t i = 0U; i < count; ++i)
  {
    char private_key_hex[65] = { 0 };

    for (size_t j = 0U; j < 32U; ++j)
    {
      (void)snprintf(&private_key_hex[j * 2U], 3U, "%02x", (unsigned int)((i + 1U) * (j + 7U)) & 0xFFU);
    }

    signers.push_back(new_signer(private_key_hex));
  }

  return signers;
}

/**
 * Computes the size of a transaction once encoded as CBOR.
 * \param tx the transaction.
 * \return The size of the transaction in bytes.
 */
static size_t
get_transaction_size(cardano_transaction_t* tx)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  EXPECT_EQ(cardano_transaction_to_cbor(tx, writer), CARDANO_SUCCESS);

  const size_t size = cardano_cbor_writer_get_encode_size(writer);

  cardano_cbor_writer_unref(&writer);

  return size;
}

/**
 * Balances a transaction that expects no other signatures than the ones of the owners of the UTXOs it spends.
 * \param tx the transaction to balance.
 * \param protocol the protocol parameters.
 * \param available_utxo the UTXOs coin selection may spend.
 * \param change_address the address that receives the change.
 * \return The result of balancing the transaction.
 */
static cardano_error_t
balance_without_foreign_signatures(
  cardano_transaction_t*         tx,
  cardano_protocol_parameters_t* protocol,
  cardano_utxo_list_t*           available_utxo,
  cardano_address_t*             change_address)
{
  cardano_coin_selector_t* coin_selector    = NULL;
  cardano_utxo_list_t*     reference_inputs = new_empty_utxo_list();

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);

  cardano_error_t result = cardano_balance_transaction(
    tx,
    0,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    available_utxo,
    coin_selector,
    change_address,
    NULL,
    change_address,
    NULL,
    nullptr);

  cardano_coin_selector_unref(&coin_selector);
  cardano_utxo_list_unref(&reference_inputs);

  return result;
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
 * Creates a PlutusV2 script, as a script reference carries it.
 * \return A new instance of the script.
 */
static cardano_script_t*
new_plutus_v2_reference_script()
{
  cardano_plutus_v2_script_t* plutus_script = NULL;
  cardano_script_t*           script        = NULL;

  EXPECT_EQ(cardano_plutus_v2_script_new_bytes_from_hex(PLUTUS_SCRIPT_HEX, strlen(PLUTUS_SCRIPT_HEX), &plutus_script), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_script_new_plutus_v2(plutus_script, &script), CARDANO_SUCCESS);

  cardano_plutus_v2_script_unref(&plutus_script);

  return script;
}

/**
 * Creates a UTXO that holds only lovelace and carries a reference script.
 * \param ordinal a number that makes the transaction id of the UTXO unique.
 * \param address the address that owns the UTXO.
 * \param coin the lovelace held by the UTXO.
 * \param script the reference script carried by the UTXO.
 * \return A new instance of the UTXO.
 */
static cardano_utxo_t*
new_reference_script_utxo(const uint64_t ordinal, cardano_address_t* address, const int64_t coin, cardano_script_t* script)
{
  cardano_utxo_t*               utxo   = new_coin_utxo(ordinal, address, coin);
  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);

  EXPECT_EQ(cardano_transaction_output_set_script_ref(output, script), CARDANO_SUCCESS);

  cardano_transaction_output_unref(&output);

  return utxo;
}

/**
 * Balances a transaction that expects no other signatures than the ones of the owners of the UTXOs it spends, given
 * every list of resolved UTXOs the balancer takes.
 * \param tx the transaction to balance.
 * \param protocol the protocol parameters.
 * \param reference_inputs the resolved reference inputs of the transaction.
 * \param pre_selected_utxo the UTXOs the transaction must spend, or NULL.
 * \param sub_transaction_inputs the UTXOs that resolve the inputs spent by the sub transactions, or NULL. They are
 * given to the balancer among the available UTXOs.
 * \param available_utxo the UTXOs coin selection may spend.
 * \param change_address the address that receives the change.
 * \return The result of balancing the transaction.
 */
static cardano_error_t
balance_with_resolved_utxos(
  cardano_transaction_t*         tx,
  cardano_protocol_parameters_t* protocol,
  cardano_utxo_list_t*           reference_inputs,
  cardano_utxo_list_t*           pre_selected_utxo,
  cardano_utxo_list_t*           sub_transaction_inputs,
  cardano_utxo_list_t*           available_utxo,
  cardano_address_t*             change_address)
{
  cardano_coin_selector_t* coin_selector   = NULL;
  cardano_utxo_list_t*     batch_available = new_batch_available_utxo(available_utxo, sub_transaction_inputs, NULL);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);

  cardano_error_t result = cardano_balance_transaction(
    tx,
    0,
    protocol,
    reference_inputs,
    pre_selected_utxo,
    NULL,
    batch_available,
    coin_selector,
    change_address,
    NULL,
    change_address,
    NULL,
    nullptr);

  cardano_coin_selector_unref(&coin_selector);
  cardano_utxo_list_unref(&batch_available);

  return result;
}

/**
 * Computes how much the fee of a transaction exceeds the minimum fee the ledger asks for the transaction as it is,
 * which is only the minimum fee the ledger enforces once the transaction carries all its signatures.
 * \param tx the transaction.
 * \param protocol the protocol parameters.
 * \param resolved_inputs the UTXOs that resolve the inputs of the transaction.
 * \return The fee excess in lovelace, negative if the fee is below the minimum fee.
 */
static int64_t
get_fee_excess(cardano_transaction_t* tx, cardano_protocol_parameters_t* protocol, cardano_utxo_list_t* resolved_inputs)
{
  uint64_t min_fee = 0U;

  EXPECT_EQ(cardano_compute_transaction_fee(tx, resolved_inputs, protocol, &min_fee), CARDANO_SUCCESS);

  return (int64_t)get_fee(tx) - (int64_t)min_fee;
}

/**
 * Computes the fee the ledger asks for a number of bytes of transaction.
 * \param protocol the protocol parameters.
 * \param bytes the number of bytes.
 * \return The fee in lovelace.
 */
static int64_t
get_fee_of_bytes(cardano_protocol_parameters_t* protocol, const int64_t bytes)
{
  return bytes * (int64_t)cardano_protocol_parameters_get_min_fee_a(protocol);
}

/**
 * Makes a transaction need a script, by listing the script hash among the guards of its body.
 * \param tx the transaction.
 * \param script the script the transaction needs.
 */
static void
require_script(cardano_transaction_t* tx, cardano_script_t* script)
{
  cardano_blake2b_hash_t*     hash       = cardano_script_get_hash(script);
  cardano_credential_t*       credential = NULL;
  cardano_guard_set_t*        guards     = NULL;
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
 * Creates the Plutus script that \ref add_plutus_script adds to the witness set.
 * \param language the language of the script, PlutusV1, PlutusV2 or PlutusV3.
 * \return A new instance of the script.
 */
static cardano_script_t*
new_witness_plutus_script(const cardano_script_language_t language)
{
  cardano_script_t* script = NULL;

  if (language == CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1)
  {
    cardano_plutus_v1_script_t* plutus_script = NULL;

    EXPECT_EQ(cardano_plutus_v1_script_new_bytes_from_hex(PLUTUS_SCRIPT_HEX, strlen(PLUTUS_SCRIPT_HEX), &plutus_script), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_script_new_plutus_v1(plutus_script, &script), CARDANO_SUCCESS);

    cardano_plutus_v1_script_unref(&plutus_script);
  }
  else if (language == CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2)
  {
    cardano_plutus_v2_script_t* plutus_script = NULL;

    EXPECT_EQ(cardano_plutus_v2_script_new_bytes_from_hex(PLUTUS_SCRIPT_HEX, strlen(PLUTUS_SCRIPT_HEX), &plutus_script), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_script_new_plutus_v2(plutus_script, &script), CARDANO_SUCCESS);

    cardano_plutus_v2_script_unref(&plutus_script);
  }
  else
  {
    cardano_plutus_v3_script_t* plutus_script = NULL;

    EXPECT_EQ(cardano_plutus_v3_script_new_bytes_from_hex(PLUTUS_SCRIPT_HEX, strlen(PLUTUS_SCRIPT_HEX), &plutus_script), CARDANO_SUCCESS);
    EXPECT_EQ(cardano_script_new_plutus_v3(plutus_script, &script), CARDANO_SUCCESS);

    cardano_plutus_v3_script_unref(&plutus_script);
  }

  return script;
}

/**
 * Adds a Plutus script to the witness set of a transaction and makes the transaction need it.
 * \param tx the transaction.
 * \param language the language of the script, PlutusV1, PlutusV2 or PlutusV3.
 */
static void
add_needed_plutus_script(cardano_transaction_t* tx, const cardano_script_language_t language)
{
  cardano_script_t* script = new_witness_plutus_script(language);

  add_plutus_script(tx, language);
  require_script(tx, script);

  cardano_script_unref(&script);
}

/**
 * Gets the reference script carried by a UTXO.
 * \param utxo the UTXO.
 * \return A new reference to the reference script of the UTXO.
 */
static cardano_script_t*
get_reference_script(cardano_utxo_t* utxo)
{
  cardano_transaction_output_t* output = cardano_utxo_get_output(utxo);
  cardano_script_t*             script = cardano_transaction_output_get_script_ref(output);

  EXPECT_NE(script, nullptr);

  cardano_transaction_output_unref(&output);

  return script;
}

/**
 * Makes a transaction need the reference script carried by a UTXO.
 * \param tx the transaction.
 * \param utxo the UTXO that carries the reference script.
 */
static void
require_reference_script(cardano_transaction_t* tx, cardano_utxo_t* utxo)
{
  cardano_script_t* script = get_reference_script(utxo);

  require_script(tx, script);

  cardano_script_unref(&script);
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
  cardano_credential_t*         credential = NULL;
  cardano_enterprise_address_t* enterprise = NULL;

  EXPECT_EQ(cardano_credential_new(hash, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, &credential), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_enterprise_address_from_credentials(CARDANO_NETWORK_ID_TEST_NET, credential, &enterprise), CARDANO_SUCCESS);

  cardano_address_t* address = cardano_enterprise_address_to_address(enterprise);

  cardano_enterprise_address_unref(&enterprise);
  cardano_credential_unref(&credential);
  cardano_blake2b_hash_unref(&hash);

  return address;
}

/**
 * Creates a UTXO locked by the reference script of another UTXO, optionally carrying that script as its own
 * reference script.
 * \param ordinal a number that makes the transaction id of the UTXO unique.
 * \param script_utxo the UTXO whose reference script locks the new UTXO.
 * \param coin the lovelace held by the UTXO.
 * \param carries_script whether the new UTXO carries the script as its reference script.
 * \return A new instance of the UTXO.
 */
static cardano_utxo_t*
new_script_locked_utxo(const uint64_t ordinal, cardano_utxo_t* script_utxo, const int64_t coin, const bool carries_script)
{
  cardano_script_t*  script  = get_reference_script(script_utxo);
  cardano_address_t* address = new_script_address(script);
  cardano_utxo_t*    utxo    = carries_script ? new_reference_script_utxo(ordinal, address, coin, script) : new_coin_utxo(ordinal, address, coin);

  cardano_address_unref(&address);
  cardano_script_unref(&script);

  return utxo;
}

/**
 * Balances a transaction that has redeemers, pre selected UTXOs and collateral UTXOs, failing exactly one allocation,
 * and checks the last error the balancer leaves on the transaction.
 * \param malloc_fail_index the zero based index of the allocation to fail, counted from the start of the balancing.
 * \param expected_error the last error the balancer must leave on the transaction.
 * \return The result of the balancing.
 */
static cardano_error_t
balance_with_pre_selected_and_collateral_utxos(const int malloc_fail_index, const char* expected_error)
{
  cardano_protocol_parameters_t* protocol          = init_protocol_parameters();
  cardano_address_t*             change_address    = create_address(BATCH_CHANGE_ADDR);
  cardano_utxo_t*                pre_selected_utxo = new_coin_utxo(1U, change_address, 5000000);
  cardano_utxo_list_t*           pre_selected      = new_utxo_list_of(pre_selected_utxo, NULL);
  cardano_utxo_list_t*           available_utxo    = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs  = new_empty_utxo_list();
  cardano_transaction_t*         tx                = new_transaction_without_inputs(COMPLEX_TX_CBOR, 15000000);
  cardano_coin_selector_t*       coin_selector     = NULL;
  cardano_tx_evaluator_t*        evaluator         = NULL;

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_declared_units_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  set_malloc_fail_index(malloc_fail_index);
  cardano_set_allocators(fail_malloc_at_exact_index, realloc, free);

  const cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    pre_selected,
    NULL,
    available_utxo,
    coin_selector,
    change_address,
    available_utxo,
    change_address,
    evaluator,
    nullptr);

  cardano_set_allocators(malloc, realloc, free);
  set_malloc_fail_index(-1);

  EXPECT_STREQ(cardano_transaction_get_last_error(tx), expected_error);

  cardano_protocol_parameters_unref(&protocol);
  cardano_address_unref(&change_address);
  cardano_utxo_unref(&pre_selected_utxo);
  cardano_utxo_list_unref(&pre_selected);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_transaction_unref(&tx);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);

  return result;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_balance_transaction, canBalanceATransaction)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs(BALANCED_TX_CBOR, 15000000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    reference_inputs,
    change_address,
    evaluator,
    nullptr);

  // Assert
  bool is_balanced = false;

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, balancesATransactionWithEmptyPreSelectedAndCollateralListsIfZeroSizeAllocationsFail)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs(BALANCED_TX_CBOR, 15000000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_utxo_list_t*           pre_selected     = new_empty_utxo_list();
  cardano_utxo_list_t*           collateral       = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_address_t*             change_address   = create_address(BATCH_CHANGE_ADDR);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);

  // Act
  cardano_set_allocators(fail_zero_size_malloc, realloc, free);

  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    pre_selected,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    collateral,
    change_address,
    NULL,
    nullptr);

  cardano_set_allocators(malloc, realloc, free);

  // Assert
  bool is_balanced = false;

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&pre_selected);
  cardano_utxo_list_unref(&collateral);
  cardano_coin_selector_unref(&coin_selector);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, canBalanceATransactionWithRandomImproveSelector)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs(BALANCED_TX_CBOR, 15000000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  EXPECT_EQ(cardano_random_improve_coin_selector_new_with_seed(42U, &coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    reference_inputs,
    change_address,
    evaluator,
    nullptr);

  // Assert
  bool is_balanced = false;

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, canBalanceATransaction2)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 234827000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    reference_inputs,
    change_address,
    evaluator,
    nullptr);

  // Assert
  bool is_balanced = false;

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, canBalanceATransactionWithDonations)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 234827000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  const uint64_t donation = 123456;
  EXPECT_EQ(cardano_transaction_body_set_donation(body, &donation), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    reference_inputs,
    change_address,
    evaluator,
    nullptr);

  // Assert
  bool is_balanced = false;

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, canBalanceATransactionWithDirectDeposits)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 234827000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  set_direct_deposit(tx, 2000000);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    reference_inputs,
    change_address,
    evaluator,
    nullptr);

  // Assert
  bool is_balanced = false;

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, returnsErrorIfTheDirectDepositsExceedTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 234827000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  set_direct_deposits(tx, (uint64_t)INT64_MAX, 1U);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    reference_inputs,
    change_address,
    evaluator,
    nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), "The direct deposits of the transaction add up to more than the maximum amount the balancer can represent.");

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, returnsErrorIfASingleDirectDepositExceedsTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 234827000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  set_direct_deposit(tx, (uint64_t)INT64_MAX + 1U);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    reference_inputs,
    change_address,
    evaluator,
    nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), "The direct deposits of the transaction add up to more than the maximum amount the balancer can represent.");

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, returnsErrorIfTheProducedCoinExceedsTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 234827000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  set_direct_deposit(tx, (uint64_t)INT64_MAX);
  set_fee(tx, 1000U);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    reference_inputs,
    change_address,
    evaluator,
    nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), "The coin produced by the transaction exceeds the maximum amount the balancer can represent.");

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, returnsErrorIfTheWithdrawalsExceedTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 234827000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  set_withdrawals(tx, (uint64_t)INT64_MAX, 1U);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    reference_inputs,
    change_address,
    evaluator,
    nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), "The coin consumed by the transaction exceeds the maximum amount the balancer can represent.");

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, returnsErrorIfASingleWithdrawalExceedsTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 234827000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  set_withdrawals(tx, (uint64_t)INT64_MAX + 1U, 0U);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    reference_inputs,
    change_address,
    evaluator,
    nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), "The coin consumed by the transaction exceeds the maximum amount the balancer can represent.");

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, returnsErrorIfTheWithdrawalsAndReclaimedDepositsExceedTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 234827000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  set_withdrawals(tx, (uint64_t)INT64_MAX, 0U);
  set_reclaimed_deposit(tx, 2000000U);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    reference_inputs,
    change_address,
    evaluator,
    nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), "The coin consumed by the transaction exceeds the maximum amount the balancer can represent.");

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, returnsErrorIfTheConsumedCoinOfASubTransactionExceedsTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 10000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  set_sub_transaction_withdrawals(sub_tx, (uint64_t)INT64_MAX, 1U);

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), "The coin consumed by a sub transaction exceeds the maximum amount the balancer can represent.");
  EXPECT_STREQ(cardano_sub_transaction_get_last_error(sub_tx), "The coin consumed by a sub transaction exceeds the maximum amount the balancer can represent.");

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, returnsErrorIfTheWithdrawalsWrapPastTheMaximumUnsignedAmount)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 234827000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  set_withdrawals(tx, HALF_OF_UINT64_RANGE, HALF_OF_UINT64_RANGE);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    reference_inputs,
    change_address,
    evaluator,
    nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), IMPLICIT_COIN_OVERFLOW_ERROR);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, returnsErrorIfTheDepositsWrapPastTheMaximumUnsignedAmount)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 234827000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  set_registration_deposits(tx, HALF_OF_UINT64_RANGE, HALF_OF_UINT64_RANGE);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    reference_inputs,
    change_address,
    evaluator,
    nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), IMPLICIT_COIN_OVERFLOW_ERROR);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, returnsErrorIfTheDepositsOfASubTransactionWrapPastTheMaximumUnsignedAmount)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 10000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  set_sub_transaction_registration_deposits(sub_tx, HALF_OF_UINT64_RANGE, HALF_OF_UINT64_RANGE);

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), SUB_TX_IMPLICIT_COIN_OVERFLOW_ERROR);
  EXPECT_STREQ(cardano_sub_transaction_get_last_error(sub_tx), SUB_TX_IMPLICIT_COIN_OVERFLOW_ERROR);

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, returnsErrorOnMemoryAllocationFailure)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 234827000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  for (int i = 0; i < 150; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_error_t result = cardano_balance_transaction(
      tx,
      1,
      protocol,
      reference_inputs,
      NULL,
      NULL,
      resolved_inputs,
      coin_selector,
      change_address,
      reference_inputs,
      change_address,
      evaluator,
      nullptr);

    EXPECT_NE(result, CARDANO_SUCCESS);
  }

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_set_allocators(malloc, realloc, free);

  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, useSuggestedFeeIfGivenAndEnough)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs(BALANCED_TX_CBOR, 15000000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  EXPECT_EQ(cardano_transaction_body_set_fee(body, 5000000), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    reference_inputs,
    change_address,
    evaluator,
    nullptr);

  // Assert
  bool is_balanced = false;

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, canBalanceTxWithScripts)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs(COMPLEX_TX_CBOR, 15000000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    resolved_inputs,
    change_address,
    evaluator,
    nullptr);

  // Assert
  bool is_balanced = false;

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced), CARDANO_SUCCESS);
  EXPECT_TRUE(is_balanced);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, recoversFromMemoryAllocationFailuresWithPreSelectedAndCollateralUtxos)
{
  // Arrange
  cardano_protocol_parameters_t* protocol          = init_protocol_parameters();
  cardano_address_t*             change_address    = create_address(BATCH_CHANGE_ADDR);
  cardano_utxo_t*                pre_selected_utxo = new_coin_utxo(1U, change_address, 5000000);
  cardano_utxo_list_t*           pre_selected      = new_utxo_list_of(pre_selected_utxo, NULL);
  cardano_utxo_list_t*           available_utxo    = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs  = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector     = NULL;
  cardano_tx_evaluator_t*        evaluator         = NULL;
  cardano_transaction_t*         expected_tx       = new_transaction_without_inputs(COMPLEX_TX_CBOR, 15000000);
  bool                           balanced          = false;

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_declared_units_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  EXPECT_EQ(
    cardano_balance_transaction(
      expected_tx,
      1,
      protocol,
      reference_inputs,
      pre_selected,
      NULL,
      available_utxo,
      coin_selector,
      change_address,
      available_utxo,
      change_address,
      evaluator,
      nullptr),
    CARDANO_SUCCESS);

  cardano_transaction_body_t* expected_body = cardano_transaction_get_body(expected_tx);
  cardano_transaction_body_unref(&expected_body);

  const uint64_t expected_fee = cardano_transaction_body_get_fee(expected_body);

  // Act
  for (int i = 0; !balanced && (i < 20000); ++i)
  {
    cardano_transaction_t* tx = new_transaction_without_inputs(COMPLEX_TX_CBOR, 15000000);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_error_t result = cardano_balance_transaction(
      tx,
      1,
      protocol,
      reference_inputs,
      pre_selected,
      NULL,
      available_utxo,
      coin_selector,
      change_address,
      available_utxo,
      change_address,
      evaluator,
      nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    balanced = (result == CARDANO_SUCCESS);

    if (balanced)
    {
      cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
      cardano_transaction_body_unref(&body);

      EXPECT_EQ(cardano_transaction_body_get_fee(body), expected_fee);
    }

    cardano_transaction_unref(&tx);
  }

  // Assert
  EXPECT_TRUE(balanced);

  // Cleanup
  cardano_transaction_unref(&expected_tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_address_unref(&change_address);
  cardano_utxo_unref(&pre_selected_utxo);
  cardano_utxo_list_unref(&pre_selected);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
}

TEST(cardano_balance_transaction, returnsErrorIfJoiningThePreSelectedUtxosAndTheSelectionFails)
{
  // Act
  const cardano_error_t result = balance_with_pre_selected_and_collateral_utxos(PRE_SELECTED_RESOLVED_INPUTS_MALLOC_INDEX, "Joining the pre selected UTXOs and the coin selection into the resolved inputs ran out of memory.");

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
}

TEST(cardano_balance_transaction, returnsErrorIfAddingTheCollateralUtxosToTheResolvedInputsFails)
{
  // Act
  const cardano_error_t result = balance_with_pre_selected_and_collateral_utxos(COLLATERAL_RESOLVED_INPUTS_MALLOC_INDEX, "Adding the collateral UTXOs to the resolved inputs ran out of memory.");

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
}

TEST(cardano_balance_transaction, forwardsReferenceInputsToTheEvaluator)
{
  // A script can live in a reference input (reference scripts) and the script context
  // resolves reference inputs against the same UTxO pool. Coin selection only yields the
  // spending inputs, so the balancer must fold the reference inputs into the set it hands
  // to the evaluator; otherwise the native evaluator fails to resolve them.

  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs(COMPLEX_TX_CBOR, 15000000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs  = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_tx_evaluator_t*        evaluator        = NULL;
  cardano_address_t*             change_address   = create_address("addr_test1qqnqfr70emn3kyywffxja44znvdw0y4aeyh0vdc3s3rky48vlp50u6nrq5s7k6h89uqrjnmr538y6e50crvz6jdv3vqqxah5fk");

  // A reference input whose (id, index) does not appear in the available UTxOs, so it can
  // only reach the evaluator if the balancer forwards it explicitly.
  static const char*      REF_INPUT_ID_HEX = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
  const uint64_t          REF_INPUT_INDEX  = 7;
  cardano_blake2b_hash_t* ref_input_id     = NULL;
  EXPECT_EQ(cardano_blake2b_hash_from_hex(REF_INPUT_ID_HEX, strlen(REF_INPUT_ID_HEX), &ref_input_id), CARDANO_SUCCESS);

  cardano_utxo_t*               base_utxo  = new_default_utxo(CBOR_DIFFERENT_VAL2);
  cardano_transaction_output_t* ref_output = cardano_utxo_get_output(base_utxo);

  cardano_transaction_input_t* ref_input = NULL;
  EXPECT_EQ(cardano_transaction_input_new(ref_input_id, REF_INPUT_INDEX, &ref_input), CARDANO_SUCCESS);

  cardano_utxo_t* ref_utxo = NULL;
  EXPECT_EQ(cardano_utxo_new(ref_input, ref_output, &ref_utxo), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_utxo_list_add(reference_inputs, ref_utxo), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_recording_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    resolved_inputs,
    coin_selector,
    change_address,
    resolved_inputs,
    change_address,
    evaluator,
    nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(g_recorded_eval_utxos, nullptr);
  EXPECT_TRUE(utxo_list_contains_input(g_recorded_eval_utxos, ref_input_id, REF_INPUT_INDEX));

  // Cleanup
  cardano_utxo_list_unref(&g_recorded_eval_utxos);
  cardano_blake2b_hash_unref(&ref_input_id);
  cardano_transaction_output_unref(&ref_output);
  cardano_transaction_input_unref(&ref_input);
  cardano_utxo_unref(&base_utxo);
  cardano_utxo_unref(&ref_utxo);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, balancesTheSameIfASubTransactionInputIsResolvedFromTheReferenceInputs)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo       = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx            = new_coin_sub_transaction(sub_tx_utxo, 15000000);
  cardano_transaction_t*         tx_with_available = new_top_level_transaction(sub_tx, NULL);
  cardano_transaction_t*         tx_with_reference = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol          = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo    = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs     = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs  = new_empty_utxo_list();

  // Act
  cardano_error_t available_result = balance_batch(tx_with_available, protocol, reference_inputs, sub_tx_inputs, available_utxo);
  cardano_error_t reference_result = balance_batch(tx_with_reference, protocol, sub_tx_inputs, NULL, available_utxo);

  // Assert
  char* available_body_hex = get_transaction_body_hex(tx_with_available);
  char* reference_body_hex = get_transaction_body_hex(tx_with_reference);

  EXPECT_EQ(available_result, CARDANO_SUCCESS);
  EXPECT_EQ(reference_result, CARDANO_SUCCESS);
  EXPECT_STREQ(available_body_hex, reference_body_hex);
  EXPECT_TRUE(is_batch_balanced(tx_with_available, protocol, sub_tx_inputs, available_utxo));
  EXPECT_TRUE(is_batch_balanced(tx_with_reference, protocol, sub_tx_inputs, available_utxo));

  // Cleanup
  free(available_body_hex);
  free(reference_body_hex);
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx_with_available);
  cardano_transaction_unref(&tx_with_reference);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, fundsTheDeficitOfTheSubTransactionsWithTopLevelInputs)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 15000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));

  cardano_value_t* top_level_imbalance = new_top_level_imbalance(tx, protocol, available_utxo);

  EXPECT_EQ(cardano_value_get_coin(top_level_imbalance), 3000000);
  EXPECT_EQ(get_policy_count(top_level_imbalance), 0);

  // Cleanup
  cardano_value_unref(&top_level_imbalance);
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, returnsErrorIfTheDirectDepositsOfASubTransactionExceedTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 10000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  set_sub_transaction_direct_deposits(sub_tx, (uint64_t)INT64_MAX, 1U);

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), "The direct deposits of a sub transaction add up to more than the maximum amount the balancer can represent.");
  EXPECT_STREQ(cardano_sub_transaction_get_last_error(sub_tx), "The direct deposits of a sub transaction add up to more than the maximum amount the balancer can represent.");

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, returnsErrorIfTheProducedCoinOfASubTransactionExceedsTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 10000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  set_sub_transaction_direct_deposit(sub_tx, (uint64_t)INT64_MAX);
  set_sub_transaction_donation(sub_tx, 1U);

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), "The coin produced by a sub transaction exceeds the maximum amount the balancer can represent.");
  EXPECT_STREQ(cardano_sub_transaction_get_last_error(sub_tx), "The coin produced by a sub transaction exceeds the maximum amount the balancer can represent.");

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, returnsTheSurplusOfTheSubTransactionsAsChange)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 10000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));

  cardano_value_t* top_level_imbalance = new_top_level_imbalance(tx, protocol, available_utxo);

  EXPECT_EQ(cardano_value_get_coin(top_level_imbalance), -2000000);
  EXPECT_EQ(get_policy_count(top_level_imbalance), 0);

  // Cleanup
  cardano_value_unref(&top_level_imbalance);
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, returnsASurplusLargerThanTheTopLevelRequirementAsChange)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 1000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));

  cardano_value_t* top_level_imbalance = new_top_level_imbalance(tx, protocol, available_utxo);

  EXPECT_EQ(cardano_value_get_coin(top_level_imbalance), -11000000);
  EXPECT_EQ(get_policy_count(top_level_imbalance), 0);

  // Cleanup
  cardano_value_unref(&top_level_imbalance);
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, spendsATopLevelInputIfTheSubTransactionsLeaveACoinAndAssetSurplus)
{
  // Arrange
  cardano_utxo_t* sub_tx_utxo = new_default_utxo(SUB_TX_UTXO_CBOR);

  add_asset(get_utxo_value(sub_tx_utxo), BURN_POLICY_ID, "TSLA", 10);

  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 1000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));
  EXPECT_EQ(get_input_count(tx), 1);

  cardano_value_t* top_level_imbalance = new_top_level_imbalance(tx, protocol, available_utxo);

  EXPECT_EQ(cardano_value_get_coin(top_level_imbalance), -11000000);
  EXPECT_EQ(get_policy_count(top_level_imbalance), 1);
  EXPECT_EQ(get_asset_amount(top_level_imbalance, BURN_POLICY_ID, "TSLA"), -10);

  // Cleanup
  cardano_value_unref(&top_level_imbalance);
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, balancesTheAssetDeficitAndSurplusOfTheSubTransactions)
{
  // Arrange
  cardano_utxo_t*                asset_utxo       = new_utxo_with_output(CBOR_DIFFERENT_VAL1, CBOR_DIFFERENT_VAL2);
  cardano_utxo_t*                coin_utxo        = new_default_utxo(CBOR_DIFFERENT_VAL2);
  cardano_sub_transaction_t*     sub_tx           = new_default_sub_transaction();
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           sub_tx_inputs    = new_sub_transaction_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  add_asset(get_utxo_value(asset_utxo), BURN_POLICY_ID, "TSLA", 10);

  cardano_utxo_list_t* available_utxo = new_utxo_list_of(asset_utxo, coin_utxo);

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));
  EXPECT_TRUE(transaction_spends(tx, asset_utxo));

  cardano_value_t* top_level_imbalance = new_top_level_imbalance(tx, protocol, available_utxo);

  EXPECT_EQ(cardano_value_get_coin(top_level_imbalance), -1000015);
  EXPECT_EQ(get_policy_count(top_level_imbalance), 2);
  EXPECT_EQ(get_asset_amount(top_level_imbalance, MINT_POLICY_ID, ""), -5);
  EXPECT_EQ(get_asset_amount(top_level_imbalance, BURN_POLICY_ID, "TSLA"), 3);

  // Cleanup
  cardano_value_unref(&top_level_imbalance);
  cardano_utxo_unref(&asset_utxo);
  cardano_utxo_unref(&coin_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, balancesABatchWithTheRandomImproveSelector)
{
  // Arrange
  cardano_utxo_t*                asset_utxo       = new_utxo_with_output(CBOR_DIFFERENT_VAL1, CBOR_DIFFERENT_VAL2);
  cardano_utxo_t*                coin_utxo        = new_default_utxo(CBOR_DIFFERENT_VAL2);
  cardano_sub_transaction_t*     sub_tx           = new_default_sub_transaction();
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           sub_tx_inputs    = new_sub_transaction_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_address_t*             change_address   = create_address(BATCH_CHANGE_ADDR);

  add_asset(get_utxo_value(asset_utxo), BURN_POLICY_ID, "TSLA", 10);

  cardano_utxo_list_t* available_utxo  = new_utxo_list_of(asset_utxo, coin_utxo);
  cardano_utxo_list_t* batch_available = new_batch_available_utxo(available_utxo, sub_tx_inputs, NULL);

  EXPECT_EQ(cardano_random_improve_coin_selector_new_with_seed(42U, &coin_selector), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    batch_available,
    coin_selector,
    change_address,
    NULL,
    change_address,
    NULL,
    nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));

  // Cleanup
  cardano_utxo_unref(&asset_utxo);
  cardano_utxo_unref(&coin_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&batch_available);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, paysOnlyTheFeeIfTheSubTransactionsCancelOut)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                counterpart_utxo = new_counterpart_utxo();
  cardano_utxo_t*                nft_utxo         = new_default_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                coin_utxo        = new_default_utxo(CBOR_DIFFERENT_VAL2);
  cardano_sub_transaction_t*     sub_tx           = new_default_sub_transaction();
  cardano_sub_transaction_t*     counterpart      = new_counterpart_sub_transaction();
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, counterpart);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_utxo_list_of(nft_utxo, coin_utxo);
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, counterpart_utxo);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));

  cardano_value_t* top_level_imbalance = new_top_level_imbalance(tx, protocol, available_utxo);

  EXPECT_TRUE(cardano_value_is_zero(top_level_imbalance));

  // Cleanup
  cardano_value_unref(&top_level_imbalance);
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&counterpart_utxo);
  cardano_utxo_unref(&nft_utxo);
  cardano_utxo_unref(&coin_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_transaction_unref(&counterpart);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, neverSelectsAUtxoSpentByASubTransaction)
{
  // Arrange
  cardano_utxo_t*                largest_utxo     = new_default_utxo(CBOR_DIFFERENT_VAL2);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(largest_utxo, cardano_value_get_coin(get_utxo_value(largest_utxo)));
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(largest_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_FALSE(transaction_spends(tx, largest_utxo));
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));
  EXPECT_EQ(cardano_utxo_list_get_length(available_utxo), 3U);

  // Cleanup
  cardano_utxo_unref(&largest_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, neverSelectsAUtxoReferencedByASubTransaction)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                largest_utxo     = new_default_utxo(CBOR_DIFFERENT_VAL2);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, cardano_value_get_coin(get_utxo_value(sub_tx_utxo)));
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  set_sub_transaction_reference_input(sub_tx, largest_utxo);

  cardano_transaction_t* tx = new_top_level_transaction(sub_tx, NULL);

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_GT(get_input_count(tx), 0U);
  EXPECT_FALSE(transaction_spends(tx, largest_utxo));
  EXPECT_FALSE(transaction_spends(tx, sub_tx_utxo));
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));
  EXPECT_EQ(cardano_utxo_list_get_length(available_utxo), 3U);

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&largest_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, returnsErrorIfTheOnlyFundsAreSpentByASubTransaction)
{
  // Arrange
  cardano_utxo_t*                largest_utxo     = new_default_utxo(CBOR_DIFFERENT_VAL2);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(largest_utxo, cardano_value_get_coin(get_utxo_value(largest_utxo)));
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_utxo_list_of(largest_utxo, NULL);
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(largest_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_BALANCE_INSUFFICIENT);

  // Cleanup
  cardano_utxo_unref(&largest_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, returnsErrorIfTheFundsDoNotCoverTheDeficitOfTheSubTransactions)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 412000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_BALANCE_INSUFFICIENT);

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, returnsErrorIfASubTransactionInputIsNotResolvedAndTheAvailableUtxosAreNull)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 10000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_address_t*             change_address   = create_address(BATCH_CHANGE_ADDR);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    NULL,
    coin_selector,
    change_address,
    NULL,
    change_address,
    NULL,
    nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), UNRESOLVED_SUB_TX_INPUT_ERROR);

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, returnsErrorIfASubTransactionInputIsNotResolved)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 10000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_empty_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), UNRESOLVED_SUB_TX_INPUT_ERROR);

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, returnsErrorIfAPreSelectedUtxoIsSpentByASubTransaction)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                other_utxo       = new_default_utxo(REF_SCRIPT_NATIVE);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, cardano_value_get_coin(get_utxo_value(sub_tx_utxo)));
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           pre_selected     = new_utxo_list_of(other_utxo, sub_tx_utxo);
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  // Act
  cardano_error_t result = balance_batch_with_pre_selected(tx, protocol, reference_inputs, pre_selected, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), "A pre selected input is already spent by a sub transaction. The inputs of the top level transaction must be disjoint from the inputs spent by its sub transactions.");
  EXPECT_FALSE(transaction_spends(tx, sub_tx_utxo));
  EXPECT_FALSE(transaction_spends(tx, other_utxo));

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&other_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&pre_selected);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, balancesABatchWithAPreSelectedUtxoNotSpentByTheSubTransactions)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                other_utxo       = new_default_utxo(REF_SCRIPT_NATIVE);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 15000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           pre_selected     = new_utxo_list_of(other_utxo, NULL);
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_utxo_list_t*           top_level_utxo   = cardano_utxo_list_concat(pre_selected, available_utxo);

  // Act
  cardano_error_t result = balance_batch_with_pre_selected(tx, protocol, reference_inputs, pre_selected, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(transaction_spends(tx, other_utxo));
  EXPECT_FALSE(transaction_spends(tx, sub_tx_utxo));
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, top_level_utxo));

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&other_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&pre_selected);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&top_level_utxo);
}

TEST(cardano_balance_transaction, returnsErrorIfSubTransactionsAreUnbalancedAndTheWitnessSetProvidesANeededPlutusV1Script)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 15000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  add_needed_plutus_script(tx, CARDANO_SCRIPT_LANGUAGE_PLUTUS_V1);

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNBALANCED_SUB_TRANSACTIONS);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), LEGACY_MODE_ERROR);

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, returnsErrorIfSubTransactionsAreUnbalancedAndTheWitnessSetProvidesANeededPlutusV2Script)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 15000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  add_needed_plutus_script(tx, CARDANO_SCRIPT_LANGUAGE_PLUTUS_V2);

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNBALANCED_SUB_TRANSACTIONS);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), LEGACY_MODE_ERROR);

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, returnsErrorIfSubTransactionsAreUnbalancedAndTheWitnessSetProvidesANeededPlutusV3Script)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 15000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  add_needed_plutus_script(tx, CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3);

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNBALANCED_SUB_TRANSACTIONS);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), LEGACY_MODE_ERROR);

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, returnsErrorIfSubTransactionsAreUnbalancedAndAReferenceInputProvidesANeededPlutusV1Script)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                reference_utxo   = new_default_utxo(REF_SCRIPT_V1_UTXO);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 10000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_utxo_list_of(reference_utxo, NULL);

  require_reference_script(tx, reference_utxo);

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNBALANCED_SUB_TRANSACTIONS);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), LEGACY_MODE_ERROR);

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, balancesUnbalancedSubTransactionsIfAReferenceInputHasAPlutusV2ScriptThatIsNotNeeded)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                reference_utxo   = new_default_utxo(REF_SCRIPT_V2_UTXO);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 10000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_utxo_list_of(reference_utxo, NULL);

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, balancesUnbalancedSubTransactionsIfTheWitnessSetHasAPlutusV3ScriptThatIsNotNeeded)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 15000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  add_plutus_script(tx, CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3);

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, returnsErrorIfSubTransactionsAreUnbalancedAndTheTransactionSpendsFromAPlutusV2ScriptOnAReferenceInput)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                reference_utxo   = new_default_utxo(REF_SCRIPT_V2_UTXO);
  cardano_utxo_t*                script_utxo      = new_script_locked_utxo(1U, reference_utxo, 3000000, false);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 10000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           pre_selected     = new_utxo_list_of(script_utxo, NULL);
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_utxo_list_of(reference_utxo, NULL);

  // Act
  cardano_error_t result = balance_batch_with_pre_selected(tx, protocol, reference_inputs, pre_selected, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNBALANCED_SUB_TRANSACTIONS);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), LEGACY_MODE_ERROR);

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_utxo_unref(&script_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&pre_selected);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, returnsErrorIfSubTransactionsAreUnbalancedAndAReferenceInputProvidesANeededPlutusV3Script)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                reference_utxo   = new_default_utxo(REF_SCRIPT_V3_UTXO);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 10000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_utxo_list_of(reference_utxo, NULL);

  require_reference_script(tx, reference_utxo);

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNBALANCED_SUB_TRANSACTIONS);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), LEGACY_MODE_ERROR);

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, balancesUnbalancedSubTransactionsIfAPreSelectedInputHasAPlutusV2ScriptThatIsNotNeeded)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                script_utxo      = new_default_utxo(REF_SCRIPT_V2_UTXO);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 10000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           pre_selected     = new_utxo_list_of(script_utxo, NULL);
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_utxo_list_t*           top_level_utxo   = cardano_utxo_list_concat(pre_selected, available_utxo);

  // Act
  cardano_error_t result = balance_batch_with_pre_selected(tx, protocol, reference_inputs, pre_selected, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(transaction_spends(tx, script_utxo));
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, top_level_utxo));

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&script_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&pre_selected);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&top_level_utxo);
}

TEST(cardano_balance_transaction, returnsErrorIfSubTransactionsAreUnbalancedAndAPreSelectedInputProvidesANeededPlutusV2Script)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                reference_utxo   = new_default_utxo(REF_SCRIPT_V2_UTXO);
  cardano_utxo_t*                script_utxo      = new_script_locked_utxo(1U, reference_utxo, 3000000, true);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 10000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           pre_selected     = new_utxo_list_of(script_utxo, NULL);
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  // Act
  cardano_error_t result = balance_batch_with_pre_selected(tx, protocol, reference_inputs, pre_selected, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNBALANCED_SUB_TRANSACTIONS);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), LEGACY_MODE_ERROR);

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_utxo_unref(&script_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&pre_selected);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, returnsErrorIfSubTransactionsAreUnbalancedAndACoinSelectedInputProvidesANeededPlutusV2Script)
{
  // Arrange
  cardano_address_t*             owner            = create_address(BATCH_CHANGE_ADDR);
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                reference_utxo   = new_default_utxo(REF_SCRIPT_V2_UTXO);
  cardano_script_t*              script           = get_reference_script(reference_utxo);
  cardano_utxo_t*                funding_utxo     = new_reference_script_utxo(1U, owner, 20000000, script);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 10000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_utxo_list_of(funding_utxo, NULL);
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  require_script(tx, script);

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNBALANCED_SUB_TRANSACTIONS);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), LEGACY_MODE_ERROR);

  // Cleanup
  cardano_address_unref(&owner);
  cardano_script_unref(&script);
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_utxo_unref(&funding_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, returnsErrorIfSubTransactionsAreUnbalancedAndCoinSelectionSpendsAPlutusV2ScriptInputThatCarriesItsScript)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                reference_utxo   = new_default_utxo(REF_SCRIPT_V2_UTXO);
  cardano_utxo_t*                script_utxo      = new_script_locked_utxo(1U, reference_utxo, 20000000, true);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 10000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_utxo_list_of(script_utxo, NULL);
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNBALANCED_SUB_TRANSACTIONS);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), LEGACY_MODE_ERROR);

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_utxo_unref(&script_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, balancesUnbalancedSubTransactionsIfACoinSelectedInputHasAPlutusV2ScriptThatIsNotNeeded)
{
  // Arrange
  cardano_address_t*             owner            = create_address(BATCH_CHANGE_ADDR);
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                reference_utxo   = new_default_utxo(REF_SCRIPT_V2_UTXO);
  cardano_script_t*              script           = get_reference_script(reference_utxo);
  cardano_utxo_t*                funding_utxo     = new_reference_script_utxo(1U, owner, 20000000, script);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 10000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_utxo_list_of(funding_utxo, NULL);
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(transaction_spends(tx, funding_utxo));
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));

  // Cleanup
  cardano_address_unref(&owner);
  cardano_script_unref(&script);
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_utxo_unref(&funding_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, balancesUnbalancedSubTransactionsIfAReferenceInputProvidesANeededNativeScript)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                reference_utxo   = new_default_utxo(REF_SCRIPT_NATIVE);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 15000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_utxo_list_of(reference_utxo, NULL);

  require_reference_script(tx, reference_utxo);

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, balancesUnbalancedSubTransactionsIfAReferenceInputProvidesANeededPlutusV4Script)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                reference_utxo   = new_default_utxo(REF_SCRIPT_V4_UTXO);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 15000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_utxo_list_of(reference_utxo, NULL);

  require_reference_script(tx, reference_utxo);

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, balancesTheTopLevelByItselfInLegacyModeIfTheSubTransactionsCancelOut)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                counterpart_utxo = new_counterpart_utxo();
  cardano_utxo_t*                reference_utxo   = new_default_utxo(REF_SCRIPT_V2_UTXO);
  cardano_utxo_t*                nft_utxo         = new_default_utxo(CBOR_DIFFERENT_VAL1);
  cardano_utxo_t*                coin_utxo        = new_default_utxo(CBOR_DIFFERENT_VAL2);
  cardano_sub_transaction_t*     sub_tx           = new_default_sub_transaction();
  cardano_sub_transaction_t*     counterpart      = new_counterpart_sub_transaction();
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, counterpart);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_utxo_list_of(nft_utxo, coin_utxo);
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, counterpart_utxo);
  cardano_utxo_list_t*           reference_inputs = new_utxo_list_of(reference_utxo, NULL);

  add_needed_plutus_script(tx, CARDANO_SCRIPT_LANGUAGE_PLUTUS_V3);

  // Act
  cardano_error_t result = balance_batch(tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));

  cardano_value_t* top_level_imbalance = new_top_level_imbalance(tx, protocol, available_utxo);

  EXPECT_TRUE(cardano_value_is_zero(top_level_imbalance));

  // Cleanup
  cardano_value_unref(&top_level_imbalance);
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&counterpart_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_utxo_unref(&nft_utxo);
  cardano_utxo_unref(&coin_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_transaction_unref(&counterpart);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, returnsErrorOnMemoryAllocationFailureForABatch)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                reference_utxo   = new_default_utxo(REF_SCRIPT_NATIVE);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 15000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           batch_available  = new_batch_available_utxo(available_utxo, sub_tx_inputs, NULL);
  cardano_utxo_list_t*           reference_inputs = new_utxo_list_of(reference_utxo, NULL);
  cardano_coin_selector_t*       coin_selector    = NULL;
  cardano_address_t*             change_address   = create_address(BATCH_CHANGE_ADDR);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);

  // Act
  for (int i = 0; i < 150; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_error_t result = cardano_balance_transaction(
      tx,
      1,
      protocol,
      reference_inputs,
      NULL,
      NULL,
      batch_available,
      coin_selector,
      change_address,
      NULL,
      change_address,
      NULL,
      nullptr);

    EXPECT_NE(result, CARDANO_SUCCESS);
  }

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_set_allocators(malloc, realloc, free);

  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&batch_available);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, balancesABatchWhoseFirstSubTransactionHasNoInputsIfZeroSizeAllocationsFail)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo     = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     inputless       = new_sub_transaction_without_inputs(sub_tx_utxo, 2000000);
  cardano_sub_transaction_t*     sub_tx          = new_coin_sub_transaction(sub_tx_utxo, 15000000);
  cardano_transaction_t*         tx              = new_top_level_transaction(inputless, sub_tx);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo  = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs   = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           batch_available = new_batch_available_utxo(available_utxo, sub_tx_inputs, NULL);
  cardano_utxo_list_t*           no_references   = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector   = NULL;
  cardano_address_t*             change_address  = create_address(BATCH_CHANGE_ADDR);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);

  // Act
  cardano_set_allocators(fail_zero_size_malloc, realloc, free);

  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    no_references,
    NULL,
    NULL,
    batch_available,
    coin_selector,
    change_address,
    NULL,
    change_address,
    NULL,
    nullptr);

  cardano_set_allocators(malloc, realloc, free);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&inputless);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&batch_available);
  cardano_utxo_list_unref(&no_references);
  cardano_coin_selector_unref(&coin_selector);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, addsCollateralIfOnlyASubTransactionHasRedeemers)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 12000000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  EXPECT_EQ(cardano_protocol_parameters_set_collateral_percentage(protocol, 150), CARDANO_SUCCESS);

  set_sub_transaction_redeemer(sub_tx, 1000000, 200000000);

  cardano_transaction_t* tx = new_top_level_transaction(sub_tx, NULL);

  // Act
  cardano_error_t result = balance_batch_with_collateral(tx, protocol, reference_inputs, sub_tx_inputs, NULL, available_utxo, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));
  EXPECT_GT(get_collateral_count(tx), 0U);
  EXPECT_EQ(get_total_collateral(tx), (uint64_t)ceil(((double)get_fee(tx) * 150.0) / 100.0));

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, doesNotAddCollateralIfTheBatchHasNoRedeemers)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 12000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  // Act
  cardano_error_t result = balance_batch_with_collateral(tx, protocol, reference_inputs, sub_tx_inputs, NULL, available_utxo, available_utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));
  EXPECT_EQ(get_collateral_count(tx), 0U);
  EXPECT_EQ(get_total_collateral(tx), 0U);

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, doesNotAddCollateralForASubTransactionRedeemerIfNoCollateralUtxosAreGiven)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 12000000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  set_sub_transaction_redeemer(sub_tx, 1000000, 200000000);

  cardano_transaction_t* tx = new_top_level_transaction(sub_tx, NULL);

  // Act
  cardano_error_t result = balance_batch_with_collateral(tx, protocol, reference_inputs, sub_tx_inputs, NULL, available_utxo, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));
  EXPECT_EQ(get_collateral_count(tx), 0U);

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, doesNotUseACollateralUtxoSpentByASubTransaction)
{
  // Arrange
  cardano_utxo_t*                spent_utxo       = new_default_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_t*                clean_utxo       = new_default_utxo(CBOR_DIFFERENT_VAL3);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(spent_utxo, cardano_value_get_coin(get_utxo_value(spent_utxo)));
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(spent_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_utxo_list_t*           collateral_utxo  = new_utxo_list_of(spent_utxo, clean_utxo);

  set_top_level_redeemer(tx, 1000000, 200000000);

  cardano_tx_evaluator_t* evaluator = NULL;

  EXPECT_EQ(cardano_tx_evaluator_new(cardano_declared_units_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = balance_batch_with_collateral(tx, protocol, reference_inputs, sub_tx_inputs, NULL, available_utxo, collateral_utxo, evaluator);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(get_collateral_count(tx), 1U);
  EXPECT_TRUE(collateral_contains(tx, clean_utxo));
  EXPECT_FALSE(collateral_contains(tx, spent_utxo));
  EXPECT_FALSE(transaction_spends(tx, spent_utxo));
  EXPECT_EQ(cardano_utxo_list_get_length(collateral_utxo), 2U);

  // Cleanup
  cardano_tx_evaluator_unref(&evaluator);
  cardano_utxo_unref(&spent_utxo);
  cardano_utxo_unref(&clean_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&collateral_utxo);
}

TEST(cardano_balance_transaction, doesNotUseACollateralUtxoReferencedByASubTransaction)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo       = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                referenced_utxo   = new_default_utxo(CBOR_DIFFERENT_VAL2);
  cardano_utxo_t*                clean_utxo        = new_default_utxo(CBOR_DIFFERENT_VAL3);
  cardano_sub_transaction_t*     sub_tx            = new_coin_sub_transaction(sub_tx_utxo, 12000000);
  cardano_protocol_parameters_t* protocol          = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo    = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs     = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           sub_tx_ref_inputs = new_utxo_list_of(referenced_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs  = new_empty_utxo_list();
  cardano_utxo_list_t*           collateral_utxo   = new_utxo_list_of(referenced_utxo, clean_utxo);

  set_sub_transaction_reference_input(sub_tx, referenced_utxo);

  cardano_transaction_t* tx = new_top_level_transaction(sub_tx, NULL);

  set_top_level_redeemer(tx, 1000000, 200000000);

  cardano_tx_evaluator_t* evaluator = NULL;

  EXPECT_EQ(cardano_tx_evaluator_new(cardano_declared_units_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = balance_batch_with_collateral(tx, protocol, reference_inputs, sub_tx_inputs, sub_tx_ref_inputs, available_utxo, collateral_utxo, evaluator);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(get_collateral_count(tx), 1U);
  EXPECT_TRUE(collateral_contains(tx, clean_utxo));
  EXPECT_FALSE(collateral_contains(tx, referenced_utxo));
  EXPECT_FALSE(transaction_spends(tx, referenced_utxo));

  // Cleanup
  cardano_tx_evaluator_unref(&evaluator);
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&referenced_utxo);
  cardano_utxo_unref(&clean_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&sub_tx_ref_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&collateral_utxo);
}

TEST(cardano_balance_transaction, returnsErrorIfEveryCollateralUtxoIsUsedByASubTransaction)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo       = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                referenced_utxo   = new_default_utxo(CBOR_DIFFERENT_VAL2);
  cardano_sub_transaction_t*     sub_tx            = new_coin_sub_transaction(sub_tx_utxo, 12000000);
  cardano_protocol_parameters_t* protocol          = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo    = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs     = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           sub_tx_ref_inputs = new_utxo_list_of(referenced_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs  = new_empty_utxo_list();
  cardano_utxo_list_t*           collateral_utxo   = new_utxo_list_of(sub_tx_utxo, referenced_utxo);

  set_sub_transaction_reference_input(sub_tx, referenced_utxo);

  cardano_transaction_t* tx = new_top_level_transaction(sub_tx, NULL);

  set_top_level_redeemer(tx, 1000000, 200000000);

  cardano_tx_evaluator_t* evaluator = NULL;

  EXPECT_EQ(cardano_tx_evaluator_new(cardano_declared_units_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = balance_batch_with_collateral(tx, protocol, reference_inputs, sub_tx_inputs, sub_tx_ref_inputs, available_utxo, collateral_utxo, evaluator);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_BALANCE_INSUFFICIENT);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), SUB_TX_COLLATERAL_ERROR);
  EXPECT_EQ(get_collateral_count(tx), 0U);

  // Cleanup
  cardano_tx_evaluator_unref(&evaluator);
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&referenced_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&sub_tx_ref_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&collateral_utxo);
}

TEST(cardano_balance_transaction, returnsErrorIfTheOnlyCollateralUtxoIsSpentByASubTransaction)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 12000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  set_top_level_redeemer(tx, 1000000, 200000000);

  cardano_tx_evaluator_t* evaluator = NULL;

  EXPECT_EQ(cardano_tx_evaluator_new(cardano_declared_units_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = balance_batch_with_collateral(tx, protocol, reference_inputs, sub_tx_inputs, NULL, available_utxo, sub_tx_inputs, evaluator);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_BALANCE_INSUFFICIENT);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), SUB_TX_COLLATERAL_ERROR);
  EXPECT_EQ(get_collateral_count(tx), 0U);

  // Cleanup
  cardano_tx_evaluator_unref(&evaluator);
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, ignoresCollateralUtxosUsedBySubTransactionsIfTheBatchHasNoRedeemers)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 12000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  // Act
  cardano_error_t result = balance_batch_with_collateral(tx, protocol, reference_inputs, sub_tx_inputs, NULL, available_utxo, sub_tx_inputs);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));
  EXPECT_EQ(get_collateral_count(tx), 0U);

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, paysForTheExecutionUnitsOfTheSubTransactions)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo      = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_sub_transaction_t*     plain_sub_tx     = new_coin_sub_transaction(sub_tx_utxo, 12000000);
  cardano_sub_transaction_t*     script_sub_tx    = new_coin_sub_transaction(sub_tx_utxo, 12000000);
  cardano_transaction_t*         plain_tx         = new_top_level_transaction(plain_sub_tx, NULL);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo   = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  set_sub_transaction_redeemer(script_sub_tx, 1000000, 200000000);

  cardano_transaction_t* script_tx = new_top_level_transaction(script_sub_tx, NULL);

  // Act
  cardano_error_t plain_result  = balance_batch(plain_tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);
  cardano_error_t script_result = balance_batch(script_tx, protocol, reference_inputs, sub_tx_inputs, available_utxo);

  // Assert
  const uint64_t ex_units_fee = compute_ex_units_fee(protocol, 1000000, 200000000);

  EXPECT_EQ(plain_result, CARDANO_SUCCESS);
  EXPECT_EQ(script_result, CARDANO_SUCCESS);
  EXPECT_EQ(ex_units_fee, 72120U);
  EXPECT_TRUE(is_batch_balanced(script_tx, protocol, sub_tx_inputs, available_utxo));
  EXPECT_GE(get_fee(script_tx), compute_size_fee(script_tx) + ex_units_fee);
  EXPECT_GE(get_fee(script_tx), get_fee(plain_tx) + ex_units_fee);

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_sub_transaction_unref(&plain_sub_tx);
  cardano_sub_transaction_unref(&script_sub_tx);
  cardano_transaction_unref(&plain_tx);
  cardano_transaction_unref(&script_tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
}

TEST(cardano_balance_transaction, paysForTheReferenceScriptsOfTheSubTransactionsOncePerSubTransaction)
{
  // Arrange
  cardano_address_t*             party_address        = create_address(BATCH_CHANGE_ADDR);
  cardano_utxo_t*                sub_tx_utxo          = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                other_utxo           = new_coin_utxo(1U, party_address, 10000000);
  cardano_utxo_t*                reference_utxo       = new_default_utxo(REF_SCRIPT_V4_UTXO);
  cardano_sub_transaction_t*     sub_tx               = new_coin_sub_transaction(sub_tx_utxo, 12000000);
  cardano_sub_transaction_t*     other_sub_tx         = new_coin_sub_transaction(other_utxo, 10000000);
  cardano_protocol_parameters_t* protocol             = init_protocol_parameters();
  cardano_unit_interval_t*       script_ref_cost      = cardano_protocol_parameters_get_ref_script_cost_per_byte(protocol);
  cardano_utxo_list_t*           available_utxo       = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs        = new_utxo_list_of(sub_tx_utxo, other_utxo);
  cardano_utxo_list_t*           reference_inputs     = new_empty_utxo_list();
  cardano_utxo_list_t*           sub_tx_ref_inputs    = new_utxo_list_of(reference_utxo, NULL);
  cardano_utxo_list_t*           priced_ref_inputs    = new_utxo_list_of(reference_utxo, reference_utxo);
  uint64_t                       reference_script_fee = 0U;

  set_sub_transaction_redeemer(sub_tx, 1000000, 200000000);
  set_sub_transaction_reference_input(sub_tx, reference_utxo);
  set_sub_transaction_reference_input(other_sub_tx, reference_utxo);

  cardano_transaction_t* tx_without_refs = new_top_level_transaction(sub_tx, other_sub_tx);
  cardano_transaction_t* tx_with_refs    = new_top_level_transaction(sub_tx, other_sub_tx);

  EXPECT_EQ(cardano_compute_script_ref_fee(priced_ref_inputs, script_ref_cost, &reference_script_fee), CARDANO_SUCCESS);

  // Act
  cardano_error_t result_without_refs = balance_batch_with_collateral(tx_without_refs, protocol, reference_inputs, sub_tx_inputs, NULL, available_utxo, available_utxo);
  cardano_error_t result_with_refs    = balance_batch_with_collateral(tx_with_refs, protocol, reference_inputs, sub_tx_inputs, sub_tx_ref_inputs, available_utxo, available_utxo);

  // Assert
  const uint64_t batch_min_fee = compute_size_fee(tx_with_refs) + compute_ex_units_fee(protocol, 1000000, 200000000) + reference_script_fee;

  EXPECT_EQ(result_without_refs, CARDANO_SUCCESS);
  EXPECT_EQ(result_with_refs, CARDANO_SUCCESS);
  EXPECT_EQ(reference_script_fee, 2U * 14U * 15U);
  EXPECT_TRUE(is_batch_balanced(tx_with_refs, protocol, sub_tx_inputs, available_utxo));
  EXPECT_GE(get_fee(tx_with_refs), batch_min_fee);
  EXPECT_EQ(get_fee(tx_with_refs), get_fee(tx_without_refs) + reference_script_fee);

  // Cleanup
  cardano_address_unref(&party_address);
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&other_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_sub_transaction_unref(&other_sub_tx);
  cardano_transaction_unref(&tx_without_refs);
  cardano_transaction_unref(&tx_with_refs);
  cardano_protocol_parameters_unref(&protocol);
  cardano_unit_interval_unref(&script_ref_cost);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&sub_tx_ref_inputs);
  cardano_utxo_list_unref(&priced_ref_inputs);
}

TEST(cardano_balance_transaction, balancesUnbalancedSubTransactionsIfASubTransactionReferenceInputHasAPlutusV2Script)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo       = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                reference_utxo    = new_default_utxo(REF_SCRIPT_V2_UTXO);
  cardano_sub_transaction_t*     sub_tx            = new_coin_sub_transaction(sub_tx_utxo, 10000000);
  cardano_protocol_parameters_t* protocol          = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo    = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs     = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs  = new_empty_utxo_list();
  cardano_utxo_list_t*           sub_tx_ref_inputs = new_utxo_list_of(reference_utxo, NULL);

  set_sub_transaction_reference_input(sub_tx, reference_utxo);

  cardano_transaction_t* tx = new_top_level_transaction(sub_tx, NULL);

  // Act
  cardano_error_t result = balance_batch_with_collateral(tx, protocol, reference_inputs, sub_tx_inputs, sub_tx_ref_inputs, available_utxo, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_FALSE(transaction_spends(tx, reference_utxo));
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));

  // Cleanup
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&sub_tx_ref_inputs);
}

TEST(cardano_balance_transaction, doesNotForwardTheSubTransactionReferenceInputsToTheEvaluator)
{
  // Arrange
  cardano_transaction_t*         tx                = new_transaction_without_inputs(COMPLEX_TX_CBOR, 15000000);
  cardano_protocol_parameters_t* protocol          = init_protocol_parameters();
  cardano_address_t*             change_address    = create_address(BATCH_CHANGE_ADDR);
  cardano_utxo_t*                sub_tx_utxo       = new_coin_utxo(1U, change_address, 12000000);
  cardano_utxo_t*                reference_utxo    = new_default_utxo(REF_SCRIPT_V4_UTXO);
  cardano_sub_transaction_t*     sub_tx            = new_coin_sub_transaction(sub_tx_utxo, 12000000);
  cardano_transaction_input_t*   reference_input   = cardano_utxo_get_input(reference_utxo);
  cardano_blake2b_hash_t*        reference_id      = cardano_transaction_input_get_id(reference_input);
  cardano_utxo_list_t*           resolved_inputs   = new_default_utxo_list();
  cardano_utxo_list_t*           reference_inputs  = new_empty_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs     = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           sub_tx_ref_inputs = new_utxo_list_of(reference_utxo, NULL);
  cardano_utxo_list_t*           batch_available   = new_batch_available_utxo(resolved_inputs, sub_tx_inputs, sub_tx_ref_inputs);
  cardano_coin_selector_t*       coin_selector     = NULL;
  cardano_tx_evaluator_t*        evaluator         = NULL;

  set_sub_transaction_reference_input(sub_tx, reference_utxo);
  add_sub_transaction(tx, sub_tx);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_tx_evaluator_new(cardano_recording_evaluator_impl_new(), &evaluator), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_balance_transaction(
    tx,
    1,
    protocol,
    reference_inputs,
    NULL,
    NULL,
    batch_available,
    coin_selector,
    change_address,
    resolved_inputs,
    change_address,
    evaluator,
    nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  ASSERT_NE(g_recorded_eval_utxos, nullptr);
  EXPECT_FALSE(utxo_list_contains_input(g_recorded_eval_utxos, reference_id, cardano_transaction_input_get_index(reference_input)));

  // Cleanup
  cardano_utxo_list_unref(&g_recorded_eval_utxos);
  cardano_blake2b_hash_unref(&reference_id);
  cardano_transaction_input_unref(&reference_input);
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&sub_tx_ref_inputs);
  cardano_utxo_list_unref(&batch_available);
  cardano_coin_selector_unref(&coin_selector);
  cardano_tx_evaluator_unref(&evaluator);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, returnsErrorOnMemoryAllocationFailureForABatchWithScripts)
{
  // Arrange
  cardano_utxo_t*                sub_tx_utxo       = new_default_utxo(SUB_TX_UTXO_CBOR);
  cardano_utxo_t*                reference_utxo    = new_default_utxo(REF_SCRIPT_V4_UTXO);
  cardano_sub_transaction_t*     sub_tx            = new_coin_sub_transaction(sub_tx_utxo, 15000000);
  cardano_protocol_parameters_t* protocol          = init_protocol_parameters();
  cardano_utxo_list_t*           available_utxo    = new_default_utxo_list();
  cardano_utxo_list_t*           sub_tx_inputs     = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs  = new_empty_utxo_list();
  cardano_utxo_list_t*           sub_tx_ref_inputs = new_utxo_list_of(reference_utxo, NULL);
  cardano_utxo_list_t*           batch_available   = new_batch_available_utxo(available_utxo, sub_tx_inputs, sub_tx_ref_inputs);
  cardano_coin_selector_t*       coin_selector     = NULL;
  cardano_address_t*             change_address    = create_address(BATCH_CHANGE_ADDR);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);

  set_sub_transaction_redeemer(sub_tx, 1000000, 200000000);
  set_sub_transaction_reference_input(sub_tx, reference_utxo);

  cardano_transaction_t* tx = new_top_level_transaction(sub_tx, NULL);

  // Act
  for (int i = 0; i < 150; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_error_t result = cardano_balance_transaction(
      tx,
      1,
      protocol,
      reference_inputs,
      NULL,
      NULL,
      batch_available,
      coin_selector,
      change_address,
      available_utxo,
      change_address,
      NULL,
      nullptr);

    EXPECT_NE(result, CARDANO_SUCCESS);
  }

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_set_allocators(malloc, realloc, free);

  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&sub_tx_ref_inputs);
  cardano_utxo_list_unref(&batch_available);
  cardano_coin_selector_unref(&coin_selector);
  cardano_address_unref(&change_address);
}

TEST(cardano_balance_transaction, paysTheMinimumFeeOfTheSignedTransactionWithOneSigner)
{
  // Arrange
  cardano_transaction_t*         tx       = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_protocol_parameters_t* protocol = init_protocol_parameters();
  signer_t                       signer   = new_signer(FIRST_SIGNER_KEY_HEX);
  cardano_utxo_t*                utxo     = new_coin_utxo(1U, signer.address, 20000000);
  cardano_utxo_list_t*           utxos    = new_utxo_list_of(utxo, NULL);

  // Act
  cardano_error_t result = balance_without_foreign_signatures(tx, protocol, utxos, signer.address);

  sign_transaction(tx, { &signer });

  // Assert
  const int64_t fee_excess = get_fee_excess(tx, protocol, utxos);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(get_vkey_witness_count(tx), 1U);
  EXPECT_GE(fee_excess, 0);
  EXPECT_LT(fee_excess, get_fee_of_bytes(protocol, MAX_FEE_EXCESS_IN_BYTES));

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_unref(&utxo);
  cardano_utxo_list_unref(&utxos);
  free_signer(signer);
}

TEST(cardano_balance_transaction, paysTheMinimumFeeOfTheSignedTransactionWithTwoSigners)
{
  // Arrange
  cardano_transaction_t*         tx          = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_protocol_parameters_t* protocol    = init_protocol_parameters();
  signer_t                       first       = new_signer(FIRST_SIGNER_KEY_HEX);
  signer_t                       second      = new_signer(SECOND_SIGNER_KEY_HEX);
  cardano_utxo_t*                first_utxo  = new_coin_utxo(1U, first.address, 4000000);
  cardano_utxo_t*                second_utxo = new_coin_utxo(2U, second.address, 4000000);
  cardano_utxo_list_t*           utxos       = new_utxo_list_of(first_utxo, second_utxo);

  // Act
  cardano_error_t result = balance_without_foreign_signatures(tx, protocol, utxos, first.address);

  sign_transaction(tx, { &first, &second });

  // Assert
  const int64_t fee_excess = get_fee_excess(tx, protocol, utxos);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(transaction_spends(tx, first_utxo));
  EXPECT_TRUE(transaction_spends(tx, second_utxo));
  EXPECT_EQ(get_vkey_witness_count(tx), 2U);
  EXPECT_GE(fee_excess, 0);
  EXPECT_LT(fee_excess, get_fee_of_bytes(protocol, MAX_FEE_EXCESS_IN_BYTES));

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_unref(&first_utxo);
  cardano_utxo_unref(&second_utxo);
  cardano_utxo_list_unref(&utxos);
  free_signer(first);
  free_signer(second);
}

TEST(cardano_balance_transaction, paysTheMinimumFeeOfTheSignedTransactionIfTheWitnessSetHoldsANativeScript)
{
  // Arrange
  cardano_transaction_t*         tx       = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_protocol_parameters_t* protocol = init_protocol_parameters();
  signer_t                       signer   = new_signer(FIRST_SIGNER_KEY_HEX);
  cardano_utxo_t*                utxo     = new_coin_utxo(1U, signer.address, 20000000);
  cardano_utxo_list_t*           utxos    = new_utxo_list_of(utxo, NULL);

  add_native_script(tx, signer);

  // Act
  cardano_error_t result = balance_without_foreign_signatures(tx, protocol, utxos, signer.address);

  sign_transaction(tx, { &signer });

  // Assert
  const int64_t fee_excess = get_fee_excess(tx, protocol, utxos);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(get_vkey_witness_count(tx), 1U);
  EXPECT_GE(fee_excess, 0);
  EXPECT_LT(fee_excess, get_fee_of_bytes(protocol, MAX_FEE_EXCESS_IN_BYTES));

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_unref(&utxo);
  cardano_utxo_list_unref(&utxos);
  free_signer(signer);
}

TEST(cardano_balance_transaction, paysTheMinimumFeeOfTheSignedTransactionWithOneByronSigner)
{
  // Arrange
  cardano_transaction_t*         tx       = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_protocol_parameters_t* protocol = init_protocol_parameters();
  byron_signer_t                 signer   = new_byron_signer(FIRST_SIGNER_KEY_HEX, BYRON_DAEDALUS_ADDRESS, BYRON_DAEDALUS_ATTRIBUTES_HEX);
  cardano_utxo_t*                utxo     = new_coin_utxo(1U, signer.address, 20000000);
  cardano_utxo_list_t*           utxos    = new_utxo_list_of(utxo, NULL);

  // Act
  cardano_error_t result = balance_without_foreign_signatures(tx, protocol, utxos, signer.address);

  add_bootstrap_witnesses(tx, { &signer });

  // Assert
  const int64_t fee_excess = get_fee_excess(tx, protocol, utxos);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(get_vkey_witness_count(tx), 0U);
  EXPECT_EQ(get_bootstrap_witness_count(tx), 1U);
  EXPECT_GE(fee_excess, 0);
  EXPECT_LT(fee_excess, get_fee_of_bytes(protocol, MAX_FEE_EXCESS_IN_BYTES));

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_unref(&utxo);
  cardano_utxo_list_unref(&utxos);
  free_byron_signer(signer);
}

TEST(cardano_balance_transaction, paysTheMinimumFeeOfTheSignedTransactionWithTwoByronSigners)
{
  // Arrange
  cardano_transaction_t*         tx          = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_protocol_parameters_t* protocol    = init_protocol_parameters();
  byron_signer_t                 first       = new_byron_signer(FIRST_SIGNER_KEY_HEX, BYRON_YOROI_ADDRESS, BYRON_YOROI_ATTRIBUTES_HEX);
  byron_signer_t                 second      = new_byron_signer(SECOND_SIGNER_KEY_HEX, BYRON_DAEDALUS_ADDRESS, BYRON_DAEDALUS_ATTRIBUTES_HEX);
  cardano_utxo_t*                first_utxo  = new_coin_utxo(1U, first.address, 4000000);
  cardano_utxo_t*                second_utxo = new_coin_utxo(2U, second.address, 4000000);
  cardano_utxo_list_t*           utxos       = new_utxo_list_of(first_utxo, second_utxo);

  // Act
  cardano_error_t result = balance_without_foreign_signatures(tx, protocol, utxos, first.address);

  add_bootstrap_witnesses(tx, { &first, &second });

  // Assert
  const int64_t fee_excess = get_fee_excess(tx, protocol, utxos);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(transaction_spends(tx, first_utxo));
  EXPECT_TRUE(transaction_spends(tx, second_utxo));
  EXPECT_EQ(get_bootstrap_witness_count(tx), 2U);
  EXPECT_GE(fee_excess, 0);
  EXPECT_LT(fee_excess, get_fee_of_bytes(protocol, MAX_FEE_EXCESS_IN_BYTES));

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_unref(&first_utxo);
  cardano_utxo_unref(&second_utxo);
  cardano_utxo_list_unref(&utxos);
  free_byron_signer(first);
  free_byron_signer(second);
}

TEST(cardano_balance_transaction, paysTheMinimumFeeOfTheSignedTransactionWithOneByronSignerForTwoInputsOfTheSameAddress)
{
  // Arrange
  cardano_transaction_t*         tx          = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_protocol_parameters_t* protocol    = init_protocol_parameters();
  byron_signer_t                 signer      = new_byron_signer(FIRST_SIGNER_KEY_HEX, BYRON_DAEDALUS_ADDRESS, BYRON_DAEDALUS_ATTRIBUTES_HEX);
  cardano_utxo_t*                first_utxo  = new_coin_utxo(1U, signer.address, 4000000);
  cardano_utxo_t*                second_utxo = new_coin_utxo(2U, signer.address, 4000000);
  cardano_utxo_list_t*           utxos       = new_utxo_list_of(first_utxo, second_utxo);

  // Act
  cardano_error_t result = balance_without_foreign_signatures(tx, protocol, utxos, signer.address);

  add_bootstrap_witnesses(tx, { &signer });

  // Assert
  const int64_t fee_excess = get_fee_excess(tx, protocol, utxos);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(transaction_spends(tx, first_utxo));
  EXPECT_TRUE(transaction_spends(tx, second_utxo));
  EXPECT_EQ(get_bootstrap_witness_count(tx), 1U);
  EXPECT_GE(fee_excess, 0);
  EXPECT_LT(fee_excess, get_fee_of_bytes(protocol, MAX_FEE_EXCESS_IN_BYTES));

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_unref(&first_utxo);
  cardano_utxo_unref(&second_utxo);
  cardano_utxo_list_unref(&utxos);
  free_byron_signer(signer);
}

TEST(cardano_balance_transaction, paysTheMinimumFeeOfTheSignedTransactionWithByronAndShelleySigners)
{
  // Arrange
  cardano_transaction_t*         tx           = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_protocol_parameters_t* protocol     = init_protocol_parameters();
  byron_signer_t                 byron        = new_byron_signer(FIRST_SIGNER_KEY_HEX, BYRON_DAEDALUS_ADDRESS, BYRON_DAEDALUS_ATTRIBUTES_HEX);
  signer_t                       shelley      = new_signer(SECOND_SIGNER_KEY_HEX);
  cardano_utxo_t*                byron_utxo   = new_coin_utxo(1U, byron.address, 4000000);
  cardano_utxo_t*                shelley_utxo = new_coin_utxo(2U, shelley.address, 4000000);
  cardano_utxo_list_t*           utxos        = new_utxo_list_of(byron_utxo, shelley_utxo);

  // Act
  cardano_error_t result = balance_without_foreign_signatures(tx, protocol, utxos, shelley.address);

  sign_transaction(tx, { &shelley });
  add_bootstrap_witnesses(tx, { &byron });

  // Assert
  const int64_t fee_excess = get_fee_excess(tx, protocol, utxos);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(transaction_spends(tx, byron_utxo));
  EXPECT_TRUE(transaction_spends(tx, shelley_utxo));
  EXPECT_EQ(get_vkey_witness_count(tx), 1U);
  EXPECT_EQ(get_bootstrap_witness_count(tx), 1U);
  EXPECT_GE(fee_excess, 0);
  EXPECT_LT(fee_excess, get_fee_of_bytes(protocol, MAX_FEE_EXCESS_IN_BYTES));

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_unref(&byron_utxo);
  cardano_utxo_unref(&shelley_utxo);
  cardano_utxo_list_unref(&utxos);
  free_byron_signer(byron);
  free_signer(shelley);
}

TEST(cardano_balance_transaction, doesNotPayForTheBootstrapWitnessesKeyIfTheWitnessSetAlreadyHoldsBootstrapWitnesses)
{
  // Arrange
  cardano_transaction_t*         tx             = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_protocol_parameters_t* protocol       = init_protocol_parameters();
  byron_signer_t                 signer         = new_byron_signer(FIRST_SIGNER_KEY_HEX, BYRON_DAEDALUS_ADDRESS, BYRON_DAEDALUS_ATTRIBUTES_HEX);
  byron_signer_t                 foreign_signer = new_byron_signer(THIRD_SIGNER_KEY_HEX, BYRON_YOROI_ADDRESS, BYRON_YOROI_ATTRIBUTES_HEX);
  cardano_utxo_t*                utxo           = new_coin_utxo(1U, signer.address, 20000000);
  cardano_utxo_list_t*           utxos          = new_utxo_list_of(utxo, NULL);

  add_bootstrap_witnesses(tx, { &foreign_signer });

  // Act
  cardano_error_t result = balance_without_foreign_signatures(tx, protocol, utxos, signer.address);

  add_bootstrap_witnesses(tx, { &signer });

  // Assert
  const int64_t fee_excess = get_fee_excess(tx, protocol, utxos);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(get_bootstrap_witness_count(tx), 2U);
  EXPECT_GE(fee_excess, 0);
  EXPECT_LT(fee_excess, get_fee_of_bytes(protocol, MAX_FEE_EXCESS_IN_BYTES));

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_unref(&utxo);
  cardano_utxo_list_unref(&utxos);
  free_byron_signer(signer);
  free_byron_signer(foreign_signer);
}

TEST(cardano_balance_transaction, recoversFromMemoryAllocationFailuresWithAByronInput)
{
  // Arrange
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  byron_signer_t                 signer           = new_byron_signer(FIRST_SIGNER_KEY_HEX, BYRON_DAEDALUS_ADDRESS, BYRON_DAEDALUS_ATTRIBUTES_HEX);
  cardano_utxo_t*                utxo             = new_coin_utxo(1U, signer.address, 20000000);
  cardano_utxo_list_t*           utxos            = new_utxo_list_of(utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_coin_selector_t*       coin_selector    = NULL;
  bool                           balanced         = false;

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);

  // Act
  for (int i = 0; !balanced && (i < 5000); ++i)
  {
    cardano_transaction_t* tx = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_error_t result = cardano_balance_transaction(
      tx,
      0,
      protocol,
      reference_inputs,
      NULL,
      NULL,
      utxos,
      coin_selector,
      signer.address,
      NULL,
      signer.address,
      NULL,
      nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    balanced = (result == CARDANO_SUCCESS);

    cardano_transaction_unref(&tx);
  }

  // Assert
  EXPECT_TRUE(balanced);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_unref(&utxo);
  cardano_utxo_list_unref(&utxos);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  free_byron_signer(signer);
}

TEST(cardano_balance_transaction, doesNotPayForTheVkeyWitnessesKeyIfTheTransactionHasNoSigners)
{
  // Arrange
  cardano_transaction_t*         tx             = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_protocol_parameters_t* protocol       = init_protocol_parameters();
  cardano_address_t*             script_address = new_script_address(SCRIPT_HASH_HEX);
  cardano_utxo_t*                utxo           = new_coin_utxo(1U, script_address, 20000000);
  cardano_utxo_list_t*           utxos          = new_utxo_list_of(utxo, NULL);

  // Act
  cardano_error_t result = balance_without_foreign_signatures(tx, protocol, utxos, script_address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(get_vkey_witness_count(tx), 0U);
  EXPECT_EQ(get_fee_excess(tx, protocol, utxos), 0);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_address_unref(&script_address);
  cardano_utxo_unref(&utxo);
  cardano_utxo_list_unref(&utxos);
}

TEST(cardano_balance_transaction, doesNotPayForTheVkeyWitnessesKeyIfTheWitnessSetAlreadyHoldsVkeyWitnesses)
{
  // Arrange
  cardano_transaction_t*         tx             = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_protocol_parameters_t* protocol       = init_protocol_parameters();
  signer_t                       signer         = new_signer(FIRST_SIGNER_KEY_HEX);
  signer_t                       foreign_signer = new_signer(THIRD_SIGNER_KEY_HEX);
  cardano_utxo_t*                utxo           = new_coin_utxo(1U, signer.address, 20000000);
  cardano_utxo_list_t*           utxos          = new_utxo_list_of(utxo, NULL);

  add_vkey_witness(tx, foreign_signer);

  // Act
  cardano_error_t result = balance_without_foreign_signatures(tx, protocol, utxos, signer.address);

  sign_transaction(tx, { &signer });

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(get_vkey_witness_count(tx), 2U);
  EXPECT_EQ(get_fee_excess(tx, protocol, utxos), 0);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_unref(&utxo);
  cardano_utxo_list_unref(&utxos);
  free_signer(signer);
  free_signer(foreign_signer);
}

TEST(cardano_balance_transaction, paysForTheGrowthOfTheVkeyWitnessesHeaderIfSigningCrossesTheCborSizeBoundary)
{
  // Arrange
  cardano_transaction_t*         tx              = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  signer_t                       signer          = new_signer(FIRST_SIGNER_KEY_HEX);
  std::vector<signer_t>          foreign_signers = new_signers(23U);
  cardano_utxo_t*                utxo            = new_coin_utxo(1U, signer.address, 20000000);
  cardano_utxo_list_t*           utxos           = new_utxo_list_of(utxo, NULL);

  std::vector<const signer_t*> foreign_signer_refs(foreign_signers.size());

  std::transform(foreign_signers.begin(), foreign_signers.end(), foreign_signer_refs.begin(), [](const signer_t& foreign_signer)
                 { return &foreign_signer; });

  set_vkey_witnesses(tx, foreign_signer_refs, true);

  // Act
  cardano_error_t result = balance_without_foreign_signatures(tx, protocol, utxos, signer.address);

  const size_t unsigned_size = get_transaction_size(tx);

  sign_transaction(tx, { &signer });

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(get_vkey_witness_count(tx), 24U);
  EXPECT_EQ(get_transaction_size(tx), unsigned_size + 101U + 1U);
  EXPECT_EQ(get_fee_excess(tx, protocol, utxos), 0);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_unref(&utxo);
  cardano_utxo_list_unref(&utxos);
  free_signer(signer);

  for (signer_t& foreign_signer: foreign_signers)
  {
    free_signer(foreign_signer);
  }
}

TEST(cardano_balance_transaction, doesNotPayForATagIfTheWitnessSetAlreadyHoldsUntaggedVkeyWitnesses)
{
  // Arrange
  cardano_transaction_t*         tx             = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_protocol_parameters_t* protocol       = init_protocol_parameters();
  signer_t                       signer         = new_signer(FIRST_SIGNER_KEY_HEX);
  signer_t                       foreign_signer = new_signer(THIRD_SIGNER_KEY_HEX);
  cardano_utxo_t*                utxo           = new_coin_utxo(1U, signer.address, 20000000);
  cardano_utxo_list_t*           utxos          = new_utxo_list_of(utxo, NULL);

  set_vkey_witnesses(tx, { &foreign_signer }, false);

  // Act
  cardano_error_t result = balance_without_foreign_signatures(tx, protocol, utxos, signer.address);

  const size_t unsigned_size = get_transaction_size(tx);

  sign_transaction(tx, { &signer });

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(get_vkey_witness_count(tx), 2U);
  EXPECT_EQ(get_transaction_size(tx), unsigned_size + 101U);
  EXPECT_EQ(get_fee_excess(tx, protocol, utxos), 0);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_unref(&utxo);
  cardano_utxo_list_unref(&utxos);
  free_signer(signer);
  free_signer(foreign_signer);
}

TEST(cardano_balance_transaction, doesNotPayForATagIfSigningFillsAnEmptyUntaggedVkeyWitnessSet)
{
  // Arrange
  cardano_transaction_t*         tx       = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_protocol_parameters_t* protocol = init_protocol_parameters();
  signer_t                       signer   = new_signer(FIRST_SIGNER_KEY_HEX);
  cardano_utxo_t*                utxo     = new_coin_utxo(1U, signer.address, 20000000);
  cardano_utxo_list_t*           utxos    = new_utxo_list_of(utxo, NULL);

  set_vkey_witnesses(tx, {}, false);

  // Act
  cardano_error_t result = balance_without_foreign_signatures(tx, protocol, utxos, signer.address);

  const size_t unsigned_size = get_transaction_size(tx);

  sign_transaction(tx, { &signer });

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(get_vkey_witness_count(tx), 1U);
  EXPECT_EQ(get_transaction_size(tx), unsigned_size + 1U + 1U + 101U);
  EXPECT_EQ(get_fee_excess(tx, protocol, utxos), 0);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_unref(&utxo);
  cardano_utxo_list_unref(&utxos);
  free_signer(signer);
}

TEST(cardano_balance_transaction, paysForTheTagIfSigningCreatesTheVkeyWitnessSet)
{
  // Arrange
  cardano_transaction_t*         tx       = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_protocol_parameters_t* protocol = init_protocol_parameters();
  signer_t                       signer   = new_signer(FIRST_SIGNER_KEY_HEX);
  cardano_utxo_t*                utxo     = new_coin_utxo(1U, signer.address, 20000000);
  cardano_utxo_list_t*           utxos    = new_utxo_list_of(utxo, NULL);

  // Act
  cardano_error_t result = balance_without_foreign_signatures(tx, protocol, utxos, signer.address);

  const size_t unsigned_size = get_transaction_size(tx);

  sign_transaction(tx, { &signer });

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(get_vkey_witness_count(tx), 1U);
  EXPECT_EQ(get_transaction_size(tx), unsigned_size + 1U + 3U + 1U + 101U);
  EXPECT_EQ(get_fee_excess(tx, protocol, utxos), 0);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_unref(&utxo);
  cardano_utxo_list_unref(&utxos);
  free_signer(signer);
}

TEST(cardano_balance_transaction, paysForAReferenceScriptOnAPreSelectedInput)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_transaction_t*         plain_tx         = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  signer_t                       signer           = new_signer(FIRST_SIGNER_KEY_HEX);
  cardano_script_t*              script           = new_native_reference_script(signer);
  cardano_utxo_t*                script_utxo      = new_reference_script_utxo(1U, signer.address, 20000000, script);
  cardano_utxo_t*                plain_utxo       = new_coin_utxo(1U, signer.address, 20000000);
  cardano_utxo_t*                other_utxo       = new_coin_utxo(2U, signer.address, 20000000);
  cardano_utxo_list_t*           pre_selected     = new_utxo_list_of(script_utxo, NULL);
  cardano_utxo_list_t*           plain_selected   = new_utxo_list_of(plain_utxo, NULL);
  cardano_utxo_list_t*           available_utxo   = new_utxo_list_of(other_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  // Act
  cardano_error_t result       = balance_with_resolved_utxos(tx, protocol, reference_inputs, pre_selected, NULL, available_utxo, signer.address);
  cardano_error_t plain_result = balance_with_resolved_utxos(plain_tx, protocol, reference_inputs, plain_selected, NULL, available_utxo, signer.address);

  sign_transaction(tx, { &signer });

  // Assert
  const int64_t fee_excess = get_fee_excess(tx, protocol, pre_selected);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(plain_result, CARDANO_SUCCESS);
  EXPECT_TRUE(transaction_spends(tx, script_utxo));
  EXPECT_FALSE(transaction_spends(tx, other_utxo));
  EXPECT_EQ(get_fee(tx), get_fee(plain_tx) + NATIVE_REFERENCE_SCRIPT_FEE);
  EXPECT_GE(fee_excess, 0);
  EXPECT_LT(fee_excess, get_fee_of_bytes(protocol, MAX_FEE_EXCESS_IN_BYTES));

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_transaction_unref(&plain_tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_script_unref(&script);
  cardano_utxo_unref(&script_utxo);
  cardano_utxo_unref(&plain_utxo);
  cardano_utxo_unref(&other_utxo);
  cardano_utxo_list_unref(&pre_selected);
  cardano_utxo_list_unref(&plain_selected);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&reference_inputs);
  free_signer(signer);
}

TEST(cardano_balance_transaction, paysForAReferenceScriptOnACoinSelectedInput)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_transaction_t*         plain_tx         = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  signer_t                       signer           = new_signer(FIRST_SIGNER_KEY_HEX);
  cardano_script_t*              script           = new_native_reference_script(signer);
  cardano_utxo_t*                script_utxo      = new_reference_script_utxo(1U, signer.address, 20000000, script);
  cardano_utxo_t*                plain_utxo       = new_coin_utxo(1U, signer.address, 20000000);
  cardano_utxo_list_t*           available_utxo   = new_utxo_list_of(script_utxo, NULL);
  cardano_utxo_list_t*           plain_available  = new_utxo_list_of(plain_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  // Act
  cardano_error_t result       = balance_with_resolved_utxos(tx, protocol, reference_inputs, NULL, NULL, available_utxo, signer.address);
  cardano_error_t plain_result = balance_with_resolved_utxos(plain_tx, protocol, reference_inputs, NULL, NULL, plain_available, signer.address);

  sign_transaction(tx, { &signer });

  // Assert
  const int64_t fee_excess = get_fee_excess(tx, protocol, available_utxo);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(plain_result, CARDANO_SUCCESS);
  EXPECT_TRUE(transaction_spends(tx, script_utxo));
  EXPECT_EQ(get_fee(tx), get_fee(plain_tx) + NATIVE_REFERENCE_SCRIPT_FEE);
  EXPECT_GE(fee_excess, 0);
  EXPECT_LT(fee_excess, get_fee_of_bytes(protocol, MAX_FEE_EXCESS_IN_BYTES));

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_transaction_unref(&plain_tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_script_unref(&script);
  cardano_utxo_unref(&script_utxo);
  cardano_utxo_unref(&plain_utxo);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&plain_available);
  cardano_utxo_list_unref(&reference_inputs);
  free_signer(signer);
}

TEST(cardano_balance_transaction, paysForAReferenceScriptOnAReferenceInputIfTheTransactionHasNoRedeemers)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_transaction_t*         plain_tx         = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  signer_t                       signer           = new_signer(FIRST_SIGNER_KEY_HEX);
  cardano_script_t*              script           = new_native_reference_script(signer);
  cardano_utxo_t*                script_utxo      = new_reference_script_utxo(1U, signer.address, 2000000, script);
  cardano_utxo_t*                funding_utxo     = new_coin_utxo(2U, signer.address, 20000000);
  cardano_utxo_list_t*           available_utxo   = new_utxo_list_of(funding_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_utxo_list_of(script_utxo, NULL);
  cardano_utxo_list_t*           no_references    = new_empty_utxo_list();
  cardano_utxo_list_t*           priced_utxos     = new_utxo_list_of(script_utxo, funding_utxo);

  // Act
  cardano_error_t result       = balance_with_resolved_utxos(tx, protocol, reference_inputs, NULL, NULL, available_utxo, signer.address);
  cardano_error_t plain_result = balance_with_resolved_utxos(plain_tx, protocol, no_references, NULL, NULL, available_utxo, signer.address);

  sign_transaction(tx, { &signer });

  // Assert
  const int64_t fee_excess = get_fee_excess(tx, protocol, priced_utxos);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(plain_result, CARDANO_SUCCESS);
  EXPECT_FALSE(transaction_spends(tx, script_utxo));
  EXPECT_EQ(get_fee(tx), get_fee(plain_tx) + NATIVE_REFERENCE_SCRIPT_FEE);
  EXPECT_GE(fee_excess, 0);
  EXPECT_LT(fee_excess, get_fee_of_bytes(protocol, MAX_FEE_EXCESS_IN_BYTES));

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_transaction_unref(&plain_tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_script_unref(&script);
  cardano_utxo_unref(&script_utxo);
  cardano_utxo_unref(&funding_utxo);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&no_references);
  cardano_utxo_list_unref(&priced_utxos);
  free_signer(signer);
}

TEST(cardano_balance_transaction, paysOnceForAReferenceScriptOnAnInputThatIsBothSpentAndReferenced)
{
  // Arrange
  cardano_transaction_t*         tx           = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_transaction_t*         plain_tx     = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_protocol_parameters_t* protocol     = init_protocol_parameters();
  signer_t                       signer       = new_signer(FIRST_SIGNER_KEY_HEX);
  cardano_script_t*              script       = new_native_reference_script(signer);
  cardano_utxo_t*                script_utxo  = new_reference_script_utxo(1U, signer.address, 20000000, script);
  cardano_utxo_t*                plain_utxo   = new_coin_utxo(1U, signer.address, 20000000);
  cardano_utxo_list_t*           script_utxos = new_utxo_list_of(script_utxo, NULL);
  cardano_utxo_list_t*           plain_utxos  = new_utxo_list_of(plain_utxo, NULL);
  cardano_utxo_list_t*           no_utxos     = new_empty_utxo_list();

  // Act
  cardano_error_t result       = balance_with_resolved_utxos(tx, protocol, script_utxos, script_utxos, NULL, no_utxos, signer.address);
  cardano_error_t plain_result = balance_with_resolved_utxos(plain_tx, protocol, plain_utxos, plain_utxos, NULL, no_utxos, signer.address);

  sign_transaction(tx, { &signer });

  // Assert
  const int64_t fee_excess = get_fee_excess(tx, protocol, script_utxos);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(plain_result, CARDANO_SUCCESS);
  EXPECT_TRUE(transaction_spends(tx, script_utxo));
  EXPECT_EQ(get_fee(tx), get_fee(plain_tx) + NATIVE_REFERENCE_SCRIPT_FEE);
  EXPECT_GE(fee_excess, 0);
  EXPECT_LT(fee_excess, get_fee_of_bytes(protocol, MAX_FEE_EXCESS_IN_BYTES));

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_transaction_unref(&plain_tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_script_unref(&script);
  cardano_utxo_unref(&script_utxo);
  cardano_utxo_unref(&plain_utxo);
  cardano_utxo_list_unref(&script_utxos);
  cardano_utxo_list_unref(&plain_utxos);
  cardano_utxo_list_unref(&no_utxos);
  free_signer(signer);
}

TEST(cardano_balance_transaction, paysTwiceForTheSameReferenceScriptOnTwoInputs)
{
  // Arrange
  cardano_transaction_t*         tx               = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_transaction_t*         plain_tx         = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 5000000);
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  signer_t                       signer           = new_signer(FIRST_SIGNER_KEY_HEX);
  cardano_script_t*              script           = new_native_reference_script(signer);
  cardano_utxo_t*                first_utxo       = new_reference_script_utxo(1U, signer.address, 20000000, script);
  cardano_utxo_t*                second_utxo      = new_reference_script_utxo(2U, signer.address, 20000000, script);
  cardano_utxo_t*                first_plain      = new_coin_utxo(1U, signer.address, 20000000);
  cardano_utxo_t*                second_plain     = new_coin_utxo(2U, signer.address, 20000000);
  cardano_utxo_list_t*           pre_selected     = new_utxo_list_of(first_utxo, second_utxo);
  cardano_utxo_list_t*           plain_selected   = new_utxo_list_of(first_plain, second_plain);
  cardano_utxo_list_t*           available_utxo   = new_empty_utxo_list();
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  // Act
  cardano_error_t result       = balance_with_resolved_utxos(tx, protocol, reference_inputs, pre_selected, NULL, available_utxo, signer.address);
  cardano_error_t plain_result = balance_with_resolved_utxos(plain_tx, protocol, reference_inputs, plain_selected, NULL, available_utxo, signer.address);

  sign_transaction(tx, { &signer });

  // Assert
  const int64_t fee_excess = get_fee_excess(tx, protocol, pre_selected);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(plain_result, CARDANO_SUCCESS);
  EXPECT_TRUE(transaction_spends(tx, first_utxo));
  EXPECT_TRUE(transaction_spends(tx, second_utxo));
  EXPECT_EQ(get_fee(tx), get_fee(plain_tx) + (2U * NATIVE_REFERENCE_SCRIPT_FEE));
  EXPECT_GE(fee_excess, 0);
  EXPECT_LT(fee_excess, get_fee_of_bytes(protocol, MAX_FEE_EXCESS_IN_BYTES));

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_transaction_unref(&plain_tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_script_unref(&script);
  cardano_utxo_unref(&first_utxo);
  cardano_utxo_unref(&second_utxo);
  cardano_utxo_unref(&first_plain);
  cardano_utxo_unref(&second_plain);
  cardano_utxo_list_unref(&pre_selected);
  cardano_utxo_list_unref(&plain_selected);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&reference_inputs);
  free_signer(signer);
}

TEST(cardano_balance_transaction, paysForAReferenceScriptOnAnInputSpentByASubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  signer_t                       signer           = new_signer(FIRST_SIGNER_KEY_HEX);
  signer_t                       party            = new_signer(SECOND_SIGNER_KEY_HEX);
  cardano_script_t*              script           = new_native_reference_script(party);
  cardano_utxo_t*                script_utxo      = new_reference_script_utxo(1U, party.address, 10000000, script);
  cardano_utxo_t*                plain_utxo       = new_coin_utxo(1U, party.address, 10000000);
  cardano_utxo_t*                unspent_utxo     = new_reference_script_utxo(2U, party.address, 10000000, script);
  cardano_utxo_t*                funding_utxo     = new_coin_utxo(3U, signer.address, 20000000);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(script_utxo, 10000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_transaction_t*         plain_tx         = new_top_level_transaction(sub_tx, NULL);
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(script_utxo, unspent_utxo);
  cardano_utxo_list_t*           plain_inputs     = new_utxo_list_of(plain_utxo, unspent_utxo);
  cardano_utxo_list_t*           available_utxo   = new_utxo_list_of(funding_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_utxo_list_t*           priced_utxos     = new_utxo_list_of(funding_utxo, script_utxo);

  // Act
  cardano_error_t result       = balance_with_resolved_utxos(tx, protocol, reference_inputs, NULL, sub_tx_inputs, available_utxo, signer.address);
  cardano_error_t plain_result = balance_with_resolved_utxos(plain_tx, protocol, reference_inputs, NULL, plain_inputs, available_utxo, signer.address);

  sign_transaction(tx, { &signer });

  // Assert
  const int64_t fee_excess = get_fee_excess(tx, protocol, priced_utxos);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(plain_result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));
  EXPECT_EQ(get_fee(tx), get_fee(plain_tx) + NATIVE_REFERENCE_SCRIPT_FEE);
  EXPECT_GE(fee_excess, 0);
  EXPECT_LT(fee_excess, get_fee_of_bytes(protocol, MAX_FEE_EXCESS_IN_BYTES));

  // Cleanup
  cardano_protocol_parameters_unref(&protocol);
  cardano_script_unref(&script);
  cardano_utxo_unref(&script_utxo);
  cardano_utxo_unref(&plain_utxo);
  cardano_utxo_unref(&unspent_utxo);
  cardano_utxo_unref(&funding_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_transaction_unref(&plain_tx);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&plain_inputs);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&priced_utxos);
  free_signer(signer);
  free_signer(party);
}

TEST(cardano_balance_transaction, paysOnceForAReferenceScriptOnAnInputThatASubTransactionBothSpendsAndReferences)
{
  // Arrange
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  signer_t                       signer           = new_signer(FIRST_SIGNER_KEY_HEX);
  signer_t                       party            = new_signer(SECOND_SIGNER_KEY_HEX);
  cardano_script_t*              script           = new_native_reference_script(party);
  cardano_utxo_t*                script_utxo      = new_reference_script_utxo(1U, party.address, 10000000, script);
  cardano_utxo_t*                plain_utxo       = new_coin_utxo(1U, party.address, 10000000);
  cardano_utxo_t*                funding_utxo     = new_coin_utxo(2U, signer.address, 20000000);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(script_utxo, 10000000);
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(script_utxo, NULL);
  cardano_utxo_list_t*           plain_inputs     = new_utxo_list_of(plain_utxo, NULL);
  cardano_utxo_list_t*           available_utxo   = new_utxo_list_of(funding_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();
  cardano_utxo_list_t*           priced_utxos     = new_utxo_list_of(funding_utxo, script_utxo);

  set_sub_transaction_reference_input(sub_tx, script_utxo);

  cardano_transaction_t* tx       = new_top_level_transaction(sub_tx, NULL);
  cardano_transaction_t* plain_tx = new_top_level_transaction(sub_tx, NULL);

  // Act
  cardano_error_t result       = balance_with_resolved_utxos(tx, protocol, reference_inputs, NULL, sub_tx_inputs, available_utxo, signer.address);
  cardano_error_t plain_result = balance_with_resolved_utxos(plain_tx, protocol, reference_inputs, NULL, plain_inputs, available_utxo, signer.address);

  sign_transaction(tx, { &signer });

  // Assert
  const int64_t fee_excess = get_fee_excess(tx, protocol, priced_utxos);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(plain_result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));
  EXPECT_EQ(get_fee(tx), get_fee(plain_tx) + NATIVE_REFERENCE_SCRIPT_FEE);
  EXPECT_GE(fee_excess, 0);
  EXPECT_LT(fee_excess, get_fee_of_bytes(protocol, MAX_FEE_EXCESS_IN_BYTES));

  // Cleanup
  cardano_protocol_parameters_unref(&protocol);
  cardano_script_unref(&script);
  cardano_utxo_unref(&script_utxo);
  cardano_utxo_unref(&plain_utxo);
  cardano_utxo_unref(&funding_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_transaction_unref(&plain_tx);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&plain_inputs);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&priced_utxos);
  free_signer(signer);
  free_signer(party);
}

TEST(cardano_balance_transaction, paysOncePerBodyForAReferenceInputSharedByTheTopLevelAndASubTransaction)
{
  // Arrange
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  signer_t                       signer           = new_signer(FIRST_SIGNER_KEY_HEX);
  signer_t                       party            = new_signer(SECOND_SIGNER_KEY_HEX);
  cardano_script_t*              script           = new_native_reference_script(party);
  cardano_utxo_t*                script_utxo      = new_reference_script_utxo(1U, party.address, 2000000, script);
  cardano_utxo_t*                plain_utxo       = new_coin_utxo(1U, party.address, 2000000);
  cardano_utxo_t*                sub_tx_utxo      = new_coin_utxo(2U, party.address, 10000000);
  cardano_utxo_t*                funding_utxo     = new_coin_utxo(3U, signer.address, 20000000);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 10000000);
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           available_utxo   = new_utxo_list_of(funding_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_utxo_list_of(script_utxo, NULL);
  cardano_utxo_list_t*           plain_references = new_utxo_list_of(plain_utxo, NULL);
  cardano_utxo_list_t*           priced_utxos     = new_utxo_list_of(script_utxo, funding_utxo);

  EXPECT_EQ(cardano_utxo_list_add(priced_utxos, script_utxo), CARDANO_SUCCESS);

  set_sub_transaction_reference_input(sub_tx, script_utxo);

  cardano_transaction_t* tx       = new_top_level_transaction(sub_tx, NULL);
  cardano_transaction_t* plain_tx = new_top_level_transaction(sub_tx, NULL);

  // Act
  cardano_error_t result       = balance_with_resolved_utxos(tx, protocol, reference_inputs, NULL, sub_tx_inputs, available_utxo, signer.address);
  cardano_error_t plain_result = balance_with_resolved_utxos(plain_tx, protocol, plain_references, NULL, sub_tx_inputs, available_utxo, signer.address);

  sign_transaction(tx, { &signer });

  // Assert
  const int64_t fee_excess = get_fee_excess(tx, protocol, priced_utxos);

  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(plain_result, CARDANO_SUCCESS);
  EXPECT_FALSE(transaction_spends(tx, script_utxo));
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));
  EXPECT_EQ(get_fee(tx), get_fee(plain_tx) + (2U * NATIVE_REFERENCE_SCRIPT_FEE));
  EXPECT_GE(fee_excess, 0);
  EXPECT_LT(fee_excess, get_fee_of_bytes(protocol, MAX_FEE_EXCESS_IN_BYTES));

  // Cleanup
  cardano_protocol_parameters_unref(&protocol);
  cardano_script_unref(&script);
  cardano_utxo_unref(&script_utxo);
  cardano_utxo_unref(&plain_utxo);
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&funding_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_transaction_unref(&plain_tx);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_utxo_list_unref(&plain_references);
  cardano_utxo_list_unref(&priced_utxos);
  free_signer(signer);
  free_signer(party);
}

TEST(cardano_balance_transaction, balancesUnbalancedSubTransactionsIfASubTransactionSpendInputHasAPlutusV2Script)
{
  // Arrange
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  signer_t                       signer           = new_signer(FIRST_SIGNER_KEY_HEX);
  signer_t                       party            = new_signer(SECOND_SIGNER_KEY_HEX);
  cardano_script_t*              script           = new_plutus_v2_reference_script();
  cardano_utxo_t*                script_utxo      = new_reference_script_utxo(1U, party.address, 10000000, script);
  cardano_utxo_t*                plain_utxo       = new_coin_utxo(1U, party.address, 10000000);
  cardano_utxo_t*                funding_utxo     = new_coin_utxo(2U, signer.address, 20000000);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(script_utxo, 8000000);
  cardano_transaction_t*         tx               = new_top_level_transaction(sub_tx, NULL);
  cardano_transaction_t*         plain_tx         = new_top_level_transaction(sub_tx, NULL);
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(script_utxo, NULL);
  cardano_utxo_list_t*           plain_inputs     = new_utxo_list_of(plain_utxo, NULL);
  cardano_utxo_list_t*           available_utxo   = new_utxo_list_of(funding_utxo, NULL);
  cardano_utxo_list_t*           reference_inputs = new_empty_utxo_list();

  // Act
  cardano_error_t result       = balance_with_resolved_utxos(tx, protocol, reference_inputs, NULL, sub_tx_inputs, available_utxo, signer.address);
  cardano_error_t plain_result = balance_with_resolved_utxos(plain_tx, protocol, reference_inputs, NULL, plain_inputs, available_utxo, signer.address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(plain_result, CARDANO_SUCCESS);
  EXPECT_TRUE(is_batch_balanced(tx, protocol, sub_tx_inputs, available_utxo));
  EXPECT_EQ(get_fee(tx), get_fee(plain_tx) + PLUTUS_REFERENCE_SCRIPT_FEE);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol);
  cardano_script_unref(&script);
  cardano_utxo_unref(&script_utxo);
  cardano_utxo_unref(&plain_utxo);
  cardano_utxo_unref(&funding_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_transaction_unref(&plain_tx);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&plain_inputs);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&reference_inputs);
  free_signer(signer);
  free_signer(party);
}

TEST(cardano_balance_transaction, returnsErrorOnMemoryAllocationFailureIfReferenceScriptsArePriced)
{
  // Arrange
  cardano_protocol_parameters_t* protocol         = init_protocol_parameters();
  signer_t                       signer           = new_signer(FIRST_SIGNER_KEY_HEX);
  signer_t                       party            = new_signer(SECOND_SIGNER_KEY_HEX);
  cardano_script_t*              script           = new_native_reference_script(party);
  cardano_utxo_t*                sub_tx_utxo      = new_reference_script_utxo(1U, party.address, 10000000, script);
  cardano_utxo_t*                reference_utxo   = new_reference_script_utxo(2U, party.address, 2000000, script);
  cardano_utxo_t*                pre_selected     = new_reference_script_utxo(3U, signer.address, 20000000, script);
  cardano_utxo_t*                funding_utxo     = new_reference_script_utxo(4U, signer.address, 20000000, script);
  cardano_sub_transaction_t*     sub_tx           = new_coin_sub_transaction(sub_tx_utxo, 10000000);
  cardano_utxo_list_t*           sub_tx_inputs    = new_utxo_list_of(sub_tx_utxo, NULL);
  cardano_utxo_list_t*           pre_selected_set = new_utxo_list_of(pre_selected, NULL);
  cardano_utxo_list_t*           available_utxo   = new_utxo_list_of(funding_utxo, NULL);
  cardano_utxo_list_t*           batch_available  = new_batch_available_utxo(available_utxo, sub_tx_inputs, NULL);
  cardano_utxo_list_t*           reference_inputs = new_utxo_list_of(reference_utxo, NULL);
  cardano_coin_selector_t*       coin_selector    = NULL;
  size_t                         failure_count    = 0U;
  bool                           is_balanced      = false;

  set_sub_transaction_reference_input(sub_tx, reference_utxo);

  cardano_transaction_t* expected_tx = new_top_level_transaction(sub_tx, NULL);

  EXPECT_EQ(cardano_large_first_coin_selector_new(&coin_selector), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_balance_transaction(expected_tx, 0, protocol, reference_inputs, pre_selected_set, NULL, batch_available, coin_selector, signer.address, NULL, signer.address, NULL, nullptr), CARDANO_SUCCESS);

  // Act
  for (int i = 0; (i < 5000) && !is_balanced; ++i)
  {
    cardano_transaction_t* tx = new_top_level_transaction(sub_tx, NULL);

    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_error_t result = cardano_balance_transaction(
      tx,
      0,
      protocol,
      reference_inputs,
      pre_selected_set,
      NULL,
      batch_available,
      coin_selector,
      signer.address,
      NULL,
      signer.address,
      NULL,
      nullptr);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);

    if (result == CARDANO_SUCCESS)
    {
      is_balanced = true;

      EXPECT_EQ(get_fee(tx), get_fee(expected_tx));
    }
    else
    {
      ++failure_count;
    }

    cardano_transaction_unref(&tx);
  }

  // Assert
  EXPECT_TRUE(is_balanced);
  EXPECT_GT(failure_count, 0U);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol);
  cardano_script_unref(&script);
  cardano_utxo_unref(&sub_tx_utxo);
  cardano_utxo_unref(&reference_utxo);
  cardano_utxo_unref(&pre_selected);
  cardano_utxo_unref(&funding_utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&expected_tx);
  cardano_utxo_list_unref(&sub_tx_inputs);
  cardano_utxo_list_unref(&pre_selected_set);
  cardano_utxo_list_unref(&available_utxo);
  cardano_utxo_list_unref(&batch_available);
  cardano_utxo_list_unref(&reference_inputs);
  cardano_coin_selector_unref(&coin_selector);
  free_signer(signer);
  free_signer(party);
}

TEST(cardano_is_transaction_balanced, returnsTrueIfTheTransactionIsBalanced)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  // Act
  bool is_balanced = false;

  cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced);

  // Assert
  EXPECT_TRUE(is_balanced);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsTrueIfTheTransactionIsBalancedAnHasDeposit)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list(2000000);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  const uint64_t donation = 2000000;
  EXPECT_EQ(cardano_transaction_body_set_donation(body, &donation), CARDANO_SUCCESS);

  // Act
  bool is_balanced = false;

  cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced);

  // Assert
  EXPECT_TRUE(is_balanced);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  // Act
  for (int i = 0; i < 36; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    bool is_balanced = false;

    cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced);

    // Assert
    EXPECT_NE(result, CARDANO_SUCCESS);
  }

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_set_allocators(malloc, realloc, free);

  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsFalseIfTheTransactionIsNotBalanced)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(UNBALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  // Act
  bool is_balanced = false;

  cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced);

  // Assert
  EXPECT_FALSE(is_balanced);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsErrorIfTxIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  // Act
  bool is_balanced = false;

  cardano_error_t result = cardano_is_transaction_balanced(NULL, resolved_inputs, protocol, &is_balanced);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsErrorIfProtocolIsNull)
{
  // Arrange
  cardano_transaction_t* tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_utxo_list_t*   resolved_inputs = new_default_utxo_list();

  // Act
  bool is_balanced = false;

  cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, NULL, &is_balanced);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsErrorIfIsBalancedIsNull)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  // Act
  cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, protocol, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsErrorIfResolvedInputsIsNull)
{
  // Arrange
  cardano_transaction_t*         tx       = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol = init_protocol_parameters();

  // Act
  bool is_balanced = false;

  cardano_error_t result = cardano_is_transaction_balanced(tx, NULL, protocol, &is_balanced);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
}

TEST(cardano_is_transaction_balanced, returnsFalseIfADirectDepositIsNotCovered)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  set_direct_deposit(tx, 2000000);

  // Act
  bool is_balanced = true;

  cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced);

  // Assert
  EXPECT_FALSE(is_balanced);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsTrueIfADirectDepositIsCoveredByTheInputs)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list(2000000);

  set_direct_deposit(tx, 2000000);

  // Act
  bool is_balanced = false;

  cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced);

  // Assert
  EXPECT_TRUE(is_balanced);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsErrorIfTheDirectDepositsExceedTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  set_direct_deposits(tx, (uint64_t)INT64_MAX, 1U);

  // Act
  bool is_balanced = true;

  cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_FALSE(is_balanced);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), "The direct deposits of the transaction add up to more than the maximum amount the balancer can represent.");

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsErrorIfTheConsumedCoinExceedsTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  set_withdrawals(tx, (uint64_t)INT64_MAX, 1U);

  // Act
  bool is_balanced = true;

  cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_FALSE(is_balanced);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), "The coin consumed by the transaction exceeds the maximum amount the balancer can represent.");

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, returnsZeroIfTheTransactionIsBalanced)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_value_is_zero(imbalance));

  // Cleanup
  cardano_value_unref(&imbalance);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, returnsPositiveCoinIfInputsExceedOutputsAndFee)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(UNBALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(imbalance), 171705);
  EXPECT_EQ(get_policy_count(imbalance), 0);

  // Cleanup
  cardano_value_unref(&imbalance);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, returnsNegativeCoinIfOutputsAndFeeExceedInputs)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  EXPECT_EQ(cardano_transaction_body_set_fee(body, cardano_transaction_body_get_fee(body) + 1000), CARDANO_SUCCESS);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(imbalance), -1000);
  EXPECT_EQ(get_policy_count(imbalance), 0);

  // Cleanup
  cardano_value_unref(&imbalance);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, reportsMintedAssetsAsPositiveAndBurnedAssetsAsNegative)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  set_mint(tx, MINT_POLICY_ID, "", 5);
  set_mint(tx, BURN_POLICY_ID, "TSLA", -3);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(imbalance), 0);
  EXPECT_EQ(get_policy_count(imbalance), 2);
  EXPECT_EQ(get_asset_amount(imbalance, MINT_POLICY_ID, ""), 5);
  EXPECT_EQ(get_asset_amount(imbalance, BURN_POLICY_ID, "TSLA"), -3);

  // Cleanup
  cardano_value_unref(&imbalance);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, reportsDirectDepositsAsProducedValue)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  set_direct_deposit(tx, 2000000);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(imbalance), -2000000);
  EXPECT_EQ(get_policy_count(imbalance), 0);

  // Cleanup
  cardano_value_unref(&imbalance);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, returnsErrorIfTheDirectDepositsExceedTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  set_direct_deposits(tx, (uint64_t)INT64_MAX, 1U);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(imbalance, nullptr);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), "The direct deposits of the transaction add up to more than the maximum amount the balancer can represent.");

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, returnsErrorIfASingleDirectDepositExceedsTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  set_direct_deposit(tx, (uint64_t)INT64_MAX + 1U);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(imbalance, nullptr);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), "The direct deposits of the transaction add up to more than the maximum amount the balancer can represent.");

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, returnsErrorIfTheProducedCoinExceedsTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  set_direct_deposit(tx, (uint64_t)INT64_MAX);
  set_fee(tx, 1000U);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(imbalance, nullptr);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), "The coin produced by the transaction exceeds the maximum amount the balancer can represent.");

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, returnsErrorIfTheWithdrawalsExceedTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  set_withdrawals(tx, (uint64_t)INT64_MAX, 1U);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(imbalance, nullptr);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), "The coin consumed by the transaction exceeds the maximum amount the balancer can represent.");

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, returnsErrorIfASingleWithdrawalExceedsTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  set_withdrawals(tx, (uint64_t)INT64_MAX + 1U, 0U);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(imbalance, nullptr);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), "The coin consumed by the transaction exceeds the maximum amount the balancer can represent.");

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, returnsErrorIfTheWithdrawalsAndReclaimedDepositsExceedTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  set_withdrawals(tx, (uint64_t)INT64_MAX, 0U);
  set_reclaimed_deposit(tx, 2000000U);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(imbalance, nullptr);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), "The coin consumed by the transaction exceeds the maximum amount the balancer can represent.");

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, returnsErrorIfTheWithdrawalsWrapPastTheMaximumUnsignedAmount)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  set_withdrawals(tx, HALF_OF_UINT64_RANGE, HALF_OF_UINT64_RANGE);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(imbalance, nullptr);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), IMPLICIT_COIN_OVERFLOW_ERROR);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, returnsErrorIfTheDepositsWrapPastTheMaximumUnsignedAmount)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  set_registration_deposits(tx, HALF_OF_UINT64_RANGE, HALF_OF_UINT64_RANGE);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(imbalance, nullptr);
  EXPECT_STREQ(cardano_transaction_get_last_error(tx), IMPLICIT_COIN_OVERFLOW_ERROR);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, returnsErrorIfAnInputIsNotResolved)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_empty_utxo_list();

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_EQ(imbalance, nullptr);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  // Act
  for (int i = 0; i < 36; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_value_t* imbalance = NULL;

    cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, &imbalance);

    // Assert
    EXPECT_NE(result, CARDANO_SUCCESS);
    EXPECT_EQ(imbalance, nullptr);
  }

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_set_allocators(malloc, realloc, free);

  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, returnsErrorIfTxIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(NULL, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(imbalance, nullptr);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, returnsErrorIfResolvedInputsIsNull)
{
  // Arrange
  cardano_transaction_t*         tx       = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol = init_protocol_parameters();

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(tx, NULL, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(imbalance, nullptr);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
}

TEST(cardano_compute_transaction_imbalance, returnsErrorIfProtocolIsNull)
{
  // Arrange
  cardano_transaction_t* tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_utxo_list_t*   resolved_inputs = new_default_utxo_list();

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, NULL, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(imbalance, nullptr);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, returnsErrorIfImbalanceIsNull)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  // Act
  cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_sub_transaction_imbalance, canComputeTheImbalanceOfASubTransaction)
{
  // Arrange
  cardano_sub_transaction_t*     sub_tx          = new_default_sub_transaction();
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_sub_transaction_utxo_list();

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(imbalance), 1000015);
  EXPECT_EQ(get_policy_count(imbalance), 2);
  EXPECT_EQ(get_asset_amount(imbalance, MINT_POLICY_ID, ""), 5);
  EXPECT_EQ(get_asset_amount(imbalance, BURN_POLICY_ID, "TSLA"), -3);

  // Cleanup
  cardano_value_unref(&imbalance);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_sub_transaction_imbalance, returnsErrorIfTheDirectDepositsExceedTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_sub_transaction_t*     sub_tx          = new_default_sub_transaction();
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_sub_transaction_utxo_list();

  set_sub_transaction_direct_deposits(sub_tx, (uint64_t)INT64_MAX, 1U);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(imbalance, nullptr);
  EXPECT_STREQ(cardano_sub_transaction_get_last_error(sub_tx), "The direct deposits of a sub transaction add up to more than the maximum amount the balancer can represent.");

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_sub_transaction_imbalance, returnsErrorIfTheProducedCoinExceedsTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_sub_transaction_t*     sub_tx          = new_default_sub_transaction();
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_sub_transaction_utxo_list();

  set_sub_transaction_direct_deposit(sub_tx, (uint64_t)INT64_MAX);
  set_sub_transaction_donation(sub_tx, 1U);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(imbalance, nullptr);
  EXPECT_STREQ(cardano_sub_transaction_get_last_error(sub_tx), "The coin produced by a sub transaction exceeds the maximum amount the balancer can represent.");

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_sub_transaction_imbalance, returnsErrorIfTheWithdrawalsExceedTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_sub_transaction_t*     sub_tx          = new_default_sub_transaction();
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_sub_transaction_utxo_list();

  set_sub_transaction_withdrawals(sub_tx, (uint64_t)INT64_MAX, 1U);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(imbalance, nullptr);
  EXPECT_STREQ(cardano_sub_transaction_get_last_error(sub_tx), "The coin consumed by a sub transaction exceeds the maximum amount the balancer can represent.");

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_sub_transaction_imbalance, returnsErrorIfASingleWithdrawalExceedsTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_sub_transaction_t*     sub_tx          = new_default_sub_transaction();
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_sub_transaction_utxo_list();

  set_sub_transaction_withdrawals(sub_tx, (uint64_t)INT64_MAX + 1U, 0U);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(imbalance, nullptr);
  EXPECT_STREQ(cardano_sub_transaction_get_last_error(sub_tx), "The coin consumed by a sub transaction exceeds the maximum amount the balancer can represent.");

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_sub_transaction_imbalance, returnsErrorIfTheWithdrawalsAndReclaimedDepositsExceedTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_sub_transaction_t*     sub_tx          = new_default_sub_transaction();
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_sub_transaction_utxo_list();

  set_sub_transaction_withdrawals(sub_tx, (uint64_t)INT64_MAX, 0U);
  set_sub_transaction_reclaimed_deposit(sub_tx, 2000000U);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(imbalance, nullptr);
  EXPECT_STREQ(cardano_sub_transaction_get_last_error(sub_tx), "The coin consumed by a sub transaction exceeds the maximum amount the balancer can represent.");

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_sub_transaction_imbalance, returnsErrorIfTheWithdrawalsWrapPastTheMaximumUnsignedAmount)
{
  // Arrange
  cardano_sub_transaction_t*     sub_tx          = new_default_sub_transaction();
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_sub_transaction_utxo_list();

  set_sub_transaction_withdrawals(sub_tx, HALF_OF_UINT64_RANGE, HALF_OF_UINT64_RANGE);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(imbalance, nullptr);
  EXPECT_STREQ(cardano_sub_transaction_get_last_error(sub_tx), SUB_TX_IMPLICIT_COIN_OVERFLOW_ERROR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_sub_transaction_imbalance, returnsErrorIfTheDepositsWrapPastTheMaximumUnsignedAmount)
{
  // Arrange
  cardano_sub_transaction_t*     sub_tx          = new_default_sub_transaction();
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_sub_transaction_utxo_list();

  set_sub_transaction_registration_deposits(sub_tx, HALF_OF_UINT64_RANGE, HALF_OF_UINT64_RANGE);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(imbalance, nullptr);
  EXPECT_STREQ(cardano_sub_transaction_get_last_error(sub_tx), SUB_TX_IMPLICIT_COIN_OVERFLOW_ERROR);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_sub_transaction_imbalance, returnsErrorIfAnInputIsNotResolved)
{
  // Arrange
  cardano_sub_transaction_t*     sub_tx          = new_default_sub_transaction();
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_empty_utxo_list();

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_EQ(imbalance, nullptr);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_sub_transaction_imbalance, returnsErrorIfSubTxIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_sub_transaction_utxo_list();

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_sub_transaction_imbalance(NULL, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(imbalance, nullptr);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_sub_transaction_imbalance, returnsErrorIfResolvedInputsIsNull)
{
  // Arrange
  cardano_sub_transaction_t*     sub_tx   = new_default_sub_transaction();
  cardano_protocol_parameters_t* protocol = init_protocol_parameters();

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, NULL, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(imbalance, nullptr);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_protocol_parameters_unref(&protocol);
}

TEST(cardano_compute_sub_transaction_imbalance, returnsErrorIfProtocolIsNull)
{
  // Arrange
  cardano_sub_transaction_t* sub_tx          = new_default_sub_transaction();
  cardano_utxo_list_t*       resolved_inputs = new_sub_transaction_utxo_list();

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, NULL, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(imbalance, nullptr);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_sub_transaction_imbalance, returnsErrorIfImbalanceIsNull)
{
  // Arrange
  cardano_sub_transaction_t*     sub_tx          = new_default_sub_transaction();
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_sub_transaction_utxo_list();

  // Act
  cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, protocol, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, returnsAValueIndependentOfTheMintField)
{
  // Arrange
  cardano_transaction_t*         tx              = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 1000000);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_empty_utxo_list();
  cardano_utxo_t*                utxo            = new_default_utxo(CBOR_DIFFERENT_VAL2);

  EXPECT_EQ(cardano_utxo_list_add(resolved_inputs, utxo), CARDANO_SUCCESS);

  set_single_input(tx, utxo);
  set_mint(tx, MINT_POLICY_ID, "", 5);

  cardano_transaction_body_t* body = cardano_transaction_get_body(tx);
  cardano_transaction_body_unref(&body);

  cardano_multi_asset_t* mint = cardano_transaction_body_get_mint(body);
  cardano_multi_asset_unref(&mint);

  char* body_hex_before = get_transaction_body_hex(tx);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, &imbalance);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  add_asset(imbalance, OTHER_POLICY_ID, "PXL", 7);
  add_asset(imbalance, MINT_POLICY_ID, "PXL", 7);

  char* body_hex_after = get_transaction_body_hex(tx);

  // Assert
  EXPECT_NE(get_multi_asset_pointer(imbalance), mint);
  EXPECT_EQ(cardano_value_get_coin(imbalance), 233831727);
  EXPECT_EQ(get_policy_count(imbalance), 2);
  EXPECT_EQ(get_asset_amount(imbalance, MINT_POLICY_ID, ""), 5);
  EXPECT_EQ(get_asset_amount(imbalance, MINT_POLICY_ID, "PXL"), 7);
  EXPECT_EQ(get_asset_amount(imbalance, OTHER_POLICY_ID, "PXL"), 7);
  EXPECT_EQ(cardano_multi_asset_get_policy_count(mint), 1);
  EXPECT_STREQ(body_hex_after, body_hex_before);

  // Cleanup
  free(body_hex_before);
  free(body_hex_after);
  cardano_value_unref(&imbalance);
  cardano_utxo_unref(&utxo);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_imbalance, returnsAValueIndependentOfTheResolvedInputs)
{
  // Arrange
  cardano_transaction_t*         tx              = new_transaction_without_inputs_no_assets(BALANCED_TX_CBOR, 1000000);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_empty_utxo_list();
  cardano_utxo_t*                utxo            = new_default_utxo(CBOR_DIFFERENT_VAL1);
  cardano_value_t*               utxo_value      = get_utxo_value(utxo);

  EXPECT_EQ(cardano_utxo_list_add(resolved_inputs, utxo), CARDANO_SUCCESS);

  set_single_input(tx, utxo);

  char* body_hex_before = get_transaction_body_hex(tx);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, &imbalance);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  add_asset(imbalance, OTHER_POLICY_ID, "PXL", 7);
  add_asset(imbalance, NFT_POLICY_ID, "PXL", 7);

  char* body_hex_after = get_transaction_body_hex(tx);

  // Assert
  EXPECT_NE(get_multi_asset_pointer(imbalance), get_multi_asset_pointer(utxo_value));
  EXPECT_EQ(cardano_value_get_coin(imbalance), 150770);
  EXPECT_EQ(get_policy_count(imbalance), 2);
  EXPECT_EQ(get_asset_amount(imbalance, NFT_POLICY_ID, "NFT-001"), 1);
  EXPECT_EQ(get_asset_amount(imbalance, NFT_POLICY_ID, "PXL"), 7);
  EXPECT_EQ(get_asset_amount(imbalance, OTHER_POLICY_ID, "PXL"), 7);
  EXPECT_EQ(cardano_value_get_coin(utxo_value), 1150770);
  EXPECT_EQ(get_policy_count(utxo_value), 1);
  EXPECT_EQ(get_asset_count(utxo_value, NFT_POLICY_ID), 1);
  EXPECT_STREQ(body_hex_after, body_hex_before);

  // Cleanup
  free(body_hex_before);
  free(body_hex_after);
  cardano_value_unref(&imbalance);
  cardano_utxo_unref(&utxo);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_sub_transaction_imbalance, returnsAValueIndependentOfTheMintField)
{
  // Arrange
  cardano_sub_transaction_t*     sub_tx          = new_default_sub_transaction();
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_sub_transaction_utxo_list();

  cardano_sub_transaction_body_t* body = cardano_sub_transaction_get_body(sub_tx);
  cardano_sub_transaction_body_unref(&body);

  cardano_multi_asset_t* mint = cardano_sub_transaction_body_get_mint(body);
  cardano_multi_asset_unref(&mint);

  char* body_hex_before = get_sub_transaction_body_hex(sub_tx);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, protocol, &imbalance);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  add_asset(imbalance, OTHER_POLICY_ID, "PXL", 7);
  add_asset(imbalance, MINT_POLICY_ID, "PXL", 7);

  char* body_hex_after = get_sub_transaction_body_hex(sub_tx);

  // Assert
  EXPECT_NE(get_multi_asset_pointer(imbalance), mint);
  EXPECT_EQ(cardano_value_get_coin(imbalance), 1000015);
  EXPECT_EQ(get_policy_count(imbalance), 3);
  EXPECT_EQ(get_asset_amount(imbalance, MINT_POLICY_ID, ""), 5);
  EXPECT_EQ(get_asset_amount(imbalance, MINT_POLICY_ID, "PXL"), 7);
  EXPECT_EQ(get_asset_amount(imbalance, BURN_POLICY_ID, "TSLA"), -3);
  EXPECT_EQ(get_asset_amount(imbalance, OTHER_POLICY_ID, "PXL"), 7);
  EXPECT_EQ(cardano_multi_asset_get_policy_count(mint), 2);
  EXPECT_STREQ(body_hex_after, body_hex_before);

  // Cleanup
  free(body_hex_before);
  free(body_hex_after);
  cardano_value_unref(&imbalance);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_sub_transaction_imbalance, returnsAValueIndependentOfTheResolvedInputs)
{
  // Arrange
  cardano_sub_transaction_t*     sub_tx          = new_default_sub_transaction();
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_empty_utxo_list();
  cardano_utxo_t*                utxo            = new_utxo_with_output(SUB_TX_UTXO_CBOR, CBOR_DIFFERENT_VAL1);
  cardano_value_t*               utxo_value      = get_utxo_value(utxo);

  EXPECT_EQ(cardano_utxo_list_add(resolved_inputs, utxo), CARDANO_SUCCESS);

  cardano_sub_transaction_body_t* body = cardano_sub_transaction_get_body(sub_tx);
  cardano_sub_transaction_body_unref(&body);

  EXPECT_EQ(cardano_sub_transaction_body_set_mint(body, NULL), CARDANO_SUCCESS);

  char* body_hex_before = get_sub_transaction_body_hex(sub_tx);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_sub_transaction_imbalance(sub_tx, resolved_inputs, protocol, &imbalance);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  add_asset(imbalance, OTHER_POLICY_ID, "PXL", 7);
  add_asset(imbalance, NFT_POLICY_ID, "PXL", 7);

  char* body_hex_after = get_sub_transaction_body_hex(sub_tx);

  // Assert
  EXPECT_NE(get_multi_asset_pointer(imbalance), get_multi_asset_pointer(utxo_value));
  EXPECT_EQ(get_policy_count(imbalance), 2);
  EXPECT_EQ(get_asset_amount(imbalance, NFT_POLICY_ID, "NFT-001"), 1);
  EXPECT_EQ(get_asset_amount(imbalance, NFT_POLICY_ID, "PXL"), 7);
  EXPECT_EQ(get_asset_amount(imbalance, OTHER_POLICY_ID, "PXL"), 7);
  EXPECT_EQ(cardano_value_get_coin(utxo_value), 1150770);
  EXPECT_EQ(get_policy_count(utxo_value), 1);
  EXPECT_EQ(get_asset_count(utxo_value, NFT_POLICY_ID), 1);
  EXPECT_STREQ(body_hex_after, body_hex_before);

  // Cleanup
  free(body_hex_before);
  free(body_hex_after);
  cardano_value_unref(&imbalance);
  cardano_utxo_unref(&utxo);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsTrueIfTheSubTransactionsCancelOut)
{
  // Arrange
  cardano_transaction_t*         tx              = new_batch_transaction(BALANCED_TX_CBOR, true, true);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_batch_utxo_list();

  // Act
  bool is_balanced = false;

  cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced);

  // Assert
  EXPECT_TRUE(is_balanced);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsTrueIfASubTransactionCoversTheDeficitOfTheTopLevelBody)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_batch_utxo_list();
  cardano_utxo_t*                utxo            = new_counterpart_utxo();
  cardano_sub_transaction_t*     sub_tx          = new_coin_sub_transaction(utxo, cardano_value_get_coin(get_utxo_value(utxo)) - 2000000);

  set_direct_deposit(tx, 2000000);

  bool is_balanced_without_sub_tx = true;

  EXPECT_EQ(cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced_without_sub_tx), CARDANO_SUCCESS);

  add_sub_transaction(tx, sub_tx);

  // Act
  bool is_balanced = false;

  cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced);

  // Assert
  EXPECT_FALSE(is_balanced_without_sub_tx);
  EXPECT_TRUE(is_balanced);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_utxo_unref(&utxo);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsFalseIfOnlyTheTopLevelBodyIsBalanced)
{
  // Arrange
  cardano_transaction_t*         tx              = new_batch_transaction(BALANCED_TX_CBOR, true, false);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_batch_utxo_list();
  cardano_value_t*               top_level       = NULL;

  EXPECT_EQ(cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, &top_level), CARDANO_SUCCESS);

  // Act
  bool is_balanced = true;

  cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced);

  // Assert
  EXPECT_TRUE(cardano_value_is_zero(top_level));
  EXPECT_FALSE(is_balanced);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_value_unref(&top_level);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsErrorIfASubTransactionInputIsNotResolved)
{
  // Arrange
  cardano_transaction_t*         tx              = new_batch_transaction(BALANCED_TX_CBOR, true, true);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  // Act
  bool is_balanced = true;

  cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_FALSE(is_balanced);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_is_transaction_balanced, returnsErrorIfMemoryAllocationFailsForABatch)
{
  // Arrange
  cardano_transaction_t*         tx              = new_batch_transaction(BALANCED_TX_CBOR, true, true);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_batch_utxo_list();

  // Act
  for (int i = 0; i < 157; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    bool is_balanced = false;

    cardano_error_t result = cardano_is_transaction_balanced(tx, resolved_inputs, protocol, &is_balanced);

    // Assert
    EXPECT_NE(result, CARDANO_SUCCESS);
    EXPECT_FALSE(is_balanced);
  }

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_set_allocators(malloc, realloc, free);

  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_batch_imbalance, returnsTheTransactionImbalanceIfThereAreNoSubTransactions)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(UNBALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();
  cardano_value_t*               top_level       = NULL;

  set_mint(tx, MINT_POLICY_ID, "", 5);
  set_mint(tx, BURN_POLICY_ID, "TSLA", -3);

  EXPECT_EQ(cardano_compute_transaction_imbalance(tx, resolved_inputs, protocol, &top_level), CARDANO_SUCCESS);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_batch_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_value_equals(imbalance, top_level));
  EXPECT_EQ(cardano_value_get_coin(imbalance), 171705);
  EXPECT_EQ(get_policy_count(imbalance), 2);
  EXPECT_EQ(get_asset_amount(imbalance, MINT_POLICY_ID, ""), 5);
  EXPECT_EQ(get_asset_amount(imbalance, BURN_POLICY_ID, "TSLA"), -3);

  // Cleanup
  cardano_value_unref(&imbalance);
  cardano_value_unref(&top_level);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_batch_imbalance, returnsZeroIfTheSubTransactionsCancelOut)
{
  // Arrange
  cardano_transaction_t*         tx              = new_batch_transaction(BALANCED_TX_CBOR, true, true);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_batch_utxo_list();

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_batch_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_TRUE(cardano_value_is_zero(imbalance));
  EXPECT_EQ(cardano_value_get_coin(imbalance), 0);
  EXPECT_EQ(get_policy_count(imbalance), 0);

  // Cleanup
  cardano_value_unref(&imbalance);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_batch_imbalance, returnsPositiveValueIfTheBatchHasASurplus)
{
  // Arrange
  cardano_transaction_t*         tx              = new_batch_transaction(UNBALANCED_TX_CBOR, true, false);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_batch_utxo_list();

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_batch_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(imbalance), 171705 + 1000015);
  EXPECT_EQ(get_policy_count(imbalance), 2);
  EXPECT_EQ(get_asset_amount(imbalance, MINT_POLICY_ID, ""), 5);
  EXPECT_EQ(get_asset_amount(imbalance, BURN_POLICY_ID, "TSLA"), -3);

  // Cleanup
  cardano_value_unref(&imbalance);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_batch_imbalance, returnsNegativeValueIfTheBatchHasADeficit)
{
  // Arrange
  cardano_transaction_t*         tx              = new_batch_transaction(BALANCED_TX_CBOR, false, true);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_batch_utxo_list();

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_batch_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(imbalance), -1000015);
  EXPECT_EQ(get_policy_count(imbalance), 2);
  EXPECT_EQ(get_asset_amount(imbalance, MINT_POLICY_ID, ""), -5);
  EXPECT_EQ(get_asset_amount(imbalance, BURN_POLICY_ID, "TSLA"), 3);

  // Cleanup
  cardano_value_unref(&imbalance);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_batch_imbalance, addsTheAssetsOfTheTopLevelBodyAndOfEverySubTransaction)
{
  // Arrange
  cardano_transaction_t*         tx              = new_batch_transaction(UNBALANCED_TX_CBOR, true, true);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_batch_utxo_list();

  set_mint(tx, MINT_POLICY_ID, "", 2);
  set_mint(tx, OTHER_POLICY_ID, "PXL", -7);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_batch_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(imbalance), 171705);
  EXPECT_EQ(get_policy_count(imbalance), 2);
  EXPECT_EQ(get_asset_amount(imbalance, MINT_POLICY_ID, ""), 2);
  EXPECT_EQ(get_asset_amount(imbalance, OTHER_POLICY_ID, "PXL"), -7);

  // Cleanup
  cardano_value_unref(&imbalance);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_batch_imbalance, countsDirectDepositsOfASubTransactionAsProducedValue)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_batch_utxo_list();
  cardano_utxo_t*                utxo            = new_counterpart_utxo();
  cardano_sub_transaction_t*     sub_tx          = new_coin_sub_transaction(utxo, cardano_value_get_coin(get_utxo_value(utxo)));

  set_sub_transaction_direct_deposit(sub_tx, 2000000);
  add_sub_transaction(tx, sub_tx);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_batch_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_value_get_coin(imbalance), -2000000);
  EXPECT_EQ(get_policy_count(imbalance), 0);

  // Cleanup
  cardano_value_unref(&imbalance);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_utxo_unref(&utxo);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_batch_imbalance, returnsErrorIfTheDirectDepositsOfASubTransactionExceedTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_batch_utxo_list();
  cardano_utxo_t*                utxo            = new_counterpart_utxo();
  cardano_sub_transaction_t*     sub_tx          = new_coin_sub_transaction(utxo, cardano_value_get_coin(get_utxo_value(utxo)));

  set_sub_transaction_direct_deposits(sub_tx, (uint64_t)INT64_MAX, 1U);
  add_sub_transaction(tx, sub_tx);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_batch_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(imbalance, nullptr);
  EXPECT_STREQ(cardano_sub_transaction_get_last_error(sub_tx), "The direct deposits of a sub transaction add up to more than the maximum amount the balancer can represent.");

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_utxo_unref(&utxo);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_batch_imbalance, returnsErrorIfTheConsumedCoinOfASubTransactionExceedsTheMaximumRepresentableAmount)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_batch_utxo_list();
  cardano_utxo_t*                utxo            = new_counterpart_utxo();
  cardano_sub_transaction_t*     sub_tx          = new_coin_sub_transaction(utxo, cardano_value_get_coin(get_utxo_value(utxo)));

  set_sub_transaction_withdrawals(sub_tx, (uint64_t)INT64_MAX, 1U);
  add_sub_transaction(tx, sub_tx);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_batch_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INTEGER_OVERFLOW);
  EXPECT_EQ(imbalance, nullptr);
  EXPECT_STREQ(cardano_sub_transaction_get_last_error(sub_tx), "The coin consumed by a sub transaction exceeds the maximum amount the balancer can represent.");

  // Cleanup
  cardano_sub_transaction_unref(&sub_tx);
  cardano_utxo_unref(&utxo);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_batch_imbalance, returnsAValueIndependentOfTheSubTransactions)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction(BALANCED_TX_CBOR);
  cardano_sub_transaction_t*     sub_tx          = new_default_sub_transaction();
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_batch_utxo_list();

  add_sub_transaction(tx, sub_tx);

  cardano_sub_transaction_body_t* body = cardano_sub_transaction_get_body(sub_tx);
  cardano_sub_transaction_body_unref(&body);

  cardano_multi_asset_t* mint = cardano_sub_transaction_body_get_mint(body);
  cardano_multi_asset_unref(&mint);

  char* body_hex_before = get_sub_transaction_body_hex(sub_tx);

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_batch_imbalance(tx, resolved_inputs, protocol, &imbalance);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  add_asset(imbalance, OTHER_POLICY_ID, "PXL", 7);
  add_asset(imbalance, MINT_POLICY_ID, "PXL", 7);

  char* body_hex_after = get_sub_transaction_body_hex(sub_tx);

  // Assert
  EXPECT_NE(get_multi_asset_pointer(imbalance), mint);
  EXPECT_EQ(cardano_value_get_coin(imbalance), 1000015);
  EXPECT_EQ(get_policy_count(imbalance), 3);
  EXPECT_EQ(get_asset_amount(imbalance, MINT_POLICY_ID, ""), 5);
  EXPECT_EQ(get_asset_amount(imbalance, MINT_POLICY_ID, "PXL"), 7);
  EXPECT_EQ(get_asset_amount(imbalance, BURN_POLICY_ID, "TSLA"), -3);
  EXPECT_EQ(get_asset_amount(imbalance, OTHER_POLICY_ID, "PXL"), 7);
  EXPECT_EQ(cardano_multi_asset_get_policy_count(mint), 2);
  EXPECT_STREQ(body_hex_after, body_hex_before);

  // Cleanup
  free(body_hex_before);
  free(body_hex_after);
  cardano_value_unref(&imbalance);
  cardano_sub_transaction_unref(&sub_tx);
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_batch_imbalance, returnsErrorIfATopLevelInputIsNotResolved)
{
  // Arrange
  cardano_transaction_t*         tx              = new_batch_transaction(BALANCED_TX_CBOR, true, false);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_sub_transaction_utxo_list();

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_batch_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_EQ(imbalance, nullptr);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_batch_imbalance, returnsErrorIfASubTransactionInputIsNotResolved)
{
  // Arrange
  cardano_transaction_t*         tx              = new_batch_transaction(BALANCED_TX_CBOR, true, false);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_default_utxo_list();

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_batch_imbalance(tx, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_ELEMENT_NOT_FOUND);
  EXPECT_EQ(imbalance, nullptr);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_batch_imbalance, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_transaction_t*         tx              = new_batch_transaction(BALANCED_TX_CBOR, true, true);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_batch_utxo_list();

  // Act
  for (int i = 0; i < 157; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_value_t* imbalance = NULL;

    cardano_error_t result = cardano_compute_transaction_batch_imbalance(tx, resolved_inputs, protocol, &imbalance);

    // Assert
    EXPECT_NE(result, CARDANO_SUCCESS);
    EXPECT_EQ(imbalance, nullptr);
  }

  // Cleanup
  reset_allocators_run_count();
  reset_limited_malloc();
  cardano_set_allocators(malloc, realloc, free);

  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_batch_imbalance, returnsErrorIfTxIsNull)
{
  // Arrange
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_batch_utxo_list();

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_batch_imbalance(NULL, resolved_inputs, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(imbalance, nullptr);

  // Cleanup
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_batch_imbalance, returnsErrorIfResolvedInputsIsNull)
{
  // Arrange
  cardano_transaction_t*         tx       = new_batch_transaction(BALANCED_TX_CBOR, true, true);
  cardano_protocol_parameters_t* protocol = init_protocol_parameters();

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_batch_imbalance(tx, NULL, protocol, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(imbalance, nullptr);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
}

TEST(cardano_compute_transaction_batch_imbalance, returnsErrorIfProtocolIsNull)
{
  // Arrange
  cardano_transaction_t* tx              = new_batch_transaction(BALANCED_TX_CBOR, true, true);
  cardano_utxo_list_t*   resolved_inputs = new_batch_utxo_list();

  // Act
  cardano_value_t* imbalance = NULL;

  cardano_error_t result = cardano_compute_transaction_batch_imbalance(tx, resolved_inputs, NULL, &imbalance);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(imbalance, nullptr);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_utxo_list_unref(&resolved_inputs);
}

TEST(cardano_compute_transaction_batch_imbalance, returnsErrorIfImbalanceIsNull)
{
  // Arrange
  cardano_transaction_t*         tx              = new_batch_transaction(BALANCED_TX_CBOR, true, true);
  cardano_protocol_parameters_t* protocol        = init_protocol_parameters();
  cardano_utxo_list_t*           resolved_inputs = new_batch_utxo_list();

  // Act
  cardano_error_t result = cardano_compute_transaction_batch_imbalance(tx, resolved_inputs, protocol, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol);
  cardano_utxo_list_unref(&resolved_inputs);
}
