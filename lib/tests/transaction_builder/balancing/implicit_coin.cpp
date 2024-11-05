/**
 * \file implicit_coin.cpp
 *
 * \author angel.castillo
 * \date   Nov 03, 2024
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

#include <cardano/common/utxo.h>
#include <cardano/transaction_builder/balancing/implicit_coin.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR = "84b000818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5000181825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e020a031903e8049182008200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d083078200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d00a83088200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d01483088200581cc37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f186482018200581cc37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f82008200581cc37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f8a03581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f81581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d58304581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d01901f483028200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0581c1732c16e26f8efb749c7f67113ec507a97fb3b382b8c147538e92db784108200581cb276b4f7a706a81364de606d890343a76af570268d4bbfee2fc8fcab05f683118200581cb276b4f7a706a81364de606d890343a76af570268d4bbfee2fc8fcab0584108200581cb276b4f7a706a81364de606d890343a76af570268d4bbfee2fc8fcab05f683118200581cb276b4f7a706a81364de606d890343a76af570268d4bbfee2fc8fcab05840b8200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0581c1732c16e26f8efb749c7f67113ec507a97fb3b382b8c147538e92db70a840c8200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d08200581cb276b4f7a706a81364de606d890343a76af570268d4bbfee2fc8fcab0a850d8200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d0581c1732c16e26f8efb749c7f67113ec507a97fb3b382b8c147538e92db78200581cb276b4f7a706a81364de606d890343a76af570268d4bbfee2fc8fcab0a82018200581c13cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d005a1581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d00a0758202ceb364d93225b4a0f004a0975a13eb50c3cc6348474b4fe9121f8dc72ca0cfa08186409a3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c413831581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e0b58206199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d38abc123de0d818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5010e81581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d3910825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e11186412818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5001481841864581de013cf55d175ea848b87deb3e914febd7e028e2bf6534475d52fb9c3d08106827468747470733a2f2f74657374696e672e7468697358203e33018e8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80da700818258206199186adb51974690d7247d2646097d2c62763b767b528816fb7ed3f9f55d395840bdea87fca1b4b4df8a9b8fb4183c0fab2f8261eb6c5e4bc42c800bb9c8918755bdea87fca1b4b4df8a9b8fb4183c0fab2f8261eb6c5e4bc42c800bb9c891875501868205186482041901f48200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f548201818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f548202818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f54830301818200581cb5ae663aaea8e500157bdf4baafd6f5ba0ce5759f7cd4101fc132f540281845820deeb8f82f2af5836ebbc1b450b6dbf0b03c93afe5696f10d49e8a8304ebfac01584064676273786767746f6768646a7074657476746b636f6376796669647171676775726a687268716169697370717275656c6876797071786565777072796676775820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b45041a003815820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b4500481187b0582840100d87a9f187bff82190bb8191b58840201d87a9f187bff821913881907d006815820b6dbf0b03c93afe5696f10d49e8a8304ebfac01deeb8f82f2af5836ebbc1b450f5a6011904d2026373747203821904d2637374720445627974657305a2667374726b6579187b81676c6973746b65796873747276616c75650626";

/* STATIC FUNCTIONS **********************************************************/

static cardano_transaction_t*
new_default_transaction()
{
  cardano_transaction_t* transaction = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  cardano_error_t result = cardano_transaction_from_cbor(reader, &transaction);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return transaction;
};

static cardano_protocol_parameters_t*
init_protocol_parameters()
{
  cardano_protocol_parameters_t* parameters = NULL;

  cardano_error_t error = cardano_protocol_parameters_new(&parameters);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_protocol_parameters_set_key_deposit(parameters, 2);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_protocol_parameters_set_pool_deposit(parameters, 3);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_protocol_parameters_set_drep_deposit(parameters, 5);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  return parameters;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_compute_implicit_coin, canComputeImplicitCoin)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction();
  cardano_protocol_parameters_t* protocol_params = init_protocol_parameters();
  cardano_implicit_coin_t        implicit_coin   = { 0 };

  // Act
  cardano_error_t result = cardano_compute_implicit_coin(tx, protocol_params, &implicit_coin);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(implicit_coin.withdrawals, 10);
  EXPECT_EQ(implicit_coin.deposits, 157);
  EXPECT_EQ(implicit_coin.reclaim_deposits, 137);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol_params);
}

TEST(cardano_compute_implicit_coin, returnsErrorIfGivenNull)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction();
  cardano_protocol_parameters_t* protocol_params = init_protocol_parameters();
  cardano_implicit_coin_t        implicit_coin   = { 0 };

  // Act
  cardano_error_t result = cardano_compute_implicit_coin(NULL, protocol_params, &implicit_coin);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol_params);
}

TEST(cardano_compute_implicit_coin, returnsErrorIfGivenNullProtocolParameters)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction();
  cardano_protocol_parameters_t* protocol_params = init_protocol_parameters();
  cardano_implicit_coin_t        implicit_coin   = { 0 };

  // Act
  cardano_error_t result = cardano_compute_implicit_coin(tx, NULL, &implicit_coin);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol_params);
}

TEST(cardano_compute_implicit_coin, returnsErrorIfGivenNullImplicitCoin)
{
  // Arrange
  cardano_transaction_t*         tx              = new_default_transaction();
  cardano_protocol_parameters_t* protocol_params = init_protocol_parameters();

  // Act
  cardano_error_t result = cardano_compute_implicit_coin(tx, protocol_params, NULL);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_unref(&tx);
  cardano_protocol_parameters_unref(&protocol_params);
}