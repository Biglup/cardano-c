/**
 * \file bech32.cpp
 *
 * \author angel.castillo
 * \date   Apr 06, 2024
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

#include <cardano/encoding/bech32.h>
#include <gmock/gmock.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <vector>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Converts a hexadecimal string to a byte vector.
 *
 * \param hex The hexadecimal string.
 *
 * \return The byte vector.
 */
static auto
hex_to_bytes(const std::string& hex) -> std::vector<byte_t>
{
  std::vector<byte_t> bytes;

  for (size_t i = 0; i < hex.length(); i += 2)
  {
    std::string const byteString = hex.substr(i, 2);
    auto              byte       = (byte_t)strtol(byteString.c_str(), nullptr, 16);
    bytes.push_back(byte);
  }

  return bytes;
}

/**
 * \brief Verifies that the Bech32 encoding of a given data is correct.
 */
static void
verify_encode(const std::string& bech32, const std::string& hrp, const std::string& hex_data)
{
  // Arrange
  const std::vector<byte_t> data = hex_to_bytes(hex_data);

  // Act
  const size_t bech32_size   = cardano_encoding_bech32_get_encoded_length(hrp.c_str(), hrp.size(), data.data(), data.size());
  char*        bech32_string = (char*)malloc(bech32_size);

  cardano_error_t result = cardano_encoding_bech32_encode(hrp.c_str(), hrp.size(), data.data(), data.size(), bech32_string, bech32_size);

  // Assert
  ASSERT_THAT(bech32_size, testing::Eq(bech32.size() + 1));
  ASSERT_THAT(result, testing::Eq(CARDANO_SUCCESS));
  ASSERT_THAT(bech32_string, testing::StrEq(bech32));

  // Cleanup
  free(bech32_string);
}

/**
 * \brief Verifies that the Bech32 decoding of a given data is correct.
 */
static void
verify_decode(const std::string& bech32, const std::string& expected_hrp, const std::string& hex_data)
{
  const std::vector<byte_t> expected_data = hex_to_bytes(hex_data);

  // Act
  size_t       hrp_size  = 0;
  const size_t data_size = cardano_encoding_bech32_get_decoded_length(bech32.c_str(), bech32.size(), &hrp_size);
  byte_t*      data      = (byte_t*)malloc(data_size);
  char*        hrp       = (char*)malloc(hrp_size);

  cardano_error_t result = cardano_encoding_bech32_decode(bech32.c_str(), bech32.size(), hrp, hrp_size, data, data_size);

  // Assert
  ASSERT_THAT(data_size, expected_data.size());
  ASSERT_THAT(hrp_size, testing::Eq(expected_hrp.size() + 1));
  ASSERT_THAT(result, testing::Eq(CARDANO_SUCCESS));
  ASSERT_THAT(memcmp(data, &expected_data[0], data_size), testing::Eq(0));
  ASSERT_THAT(strcmp(hrp, expected_hrp.c_str()), testing::Eq(0));

  // Cleanup
  free(hrp);
  free(data);
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_encoding_bech32_decode, canDecodeBech32Strings)
{
  verify_decode("addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgse35a3x", "addr", "019493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  verify_decode("addr1vpu5vlrf4xkxv2qpwngf6cjhtw542ayty80v8dyr49rf5eg0yu80w", "addr", "6079467c69a9ac66280174d09d62575ba955748b21dec3b483a9469a65");
  verify_decode("stake1vpu5vlrf4xkxv2qpwngf6cjhtw542ayty80v8dyr49rf5egfu2p0u", "stake", "6079467c69a9ac66280174d09d62575ba955748b21dec3b483a9469a65");
  verify_decode("addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgse35a3x", "addr", "019493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  verify_decode("addr1z8phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgs9yc0hh", "addr", "11c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  verify_decode("addr1yx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzerkr0vd4msrxnuwnccdxlhdjar77j6lg0wypcc9uar5d2shs2z78ve", "addr", "219493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8ec37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f");
  verify_decode("addr1x8phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gt7r0vd4msrxnuwnccdxlhdjar77j6lg0wypcc9uar5d2shskhj42g", "addr", "31c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542fc37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f");
  verify_decode("addr1gx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer5pnz75xxcrzqf96k", "addr", "419493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e8198bd431b03");
  verify_decode("addr128phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtupnz75xxcrtw79hu", "addr", "51c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f8198bd431b03");
  verify_decode("addr1vx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzers66hrl8", "addr", "619493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e");
  verify_decode("addr1w8phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcyjy7wx", "addr", "71c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f");
  verify_decode("stake1uyehkck0lajq8gr28t9uxnuvgcqrc6070x3k9r8048z8y5gh6ffgw", "stake", "e1337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  verify_decode("stake178phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcccycj5", "stake", "f1c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f");
  verify_decode("addr_test1qz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgs68faae", "addr_test", "009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  verify_decode("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", "addr_test", "10c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  verify_decode("addr_test1yz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzerkr0vd4msrxnuwnccdxlhdjar77j6lg0wypcc9uar5d2shsf5r8qx", "addr_test", "209493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8ec37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f");
  verify_decode("addr_test1xrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gt7r0vd4msrxnuwnccdxlhdjar77j6lg0wypcc9uar5d2shs4p04xh", "addr_test", "30c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542fc37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f");
  verify_decode("addr_test1gz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer5pnz75xxcrdw5vky", "addr_test", "409493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e8198bd431b03");
  verify_decode("addr_test12rphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtupnz75xxcryqrvmw", "addr_test", "50c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f8198bd431b03");
  verify_decode("addr_test1vz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzerspjrlsz", "addr_test", "609493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e");
  verify_decode("addr_test1wrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcl6szpr", "addr_test", "70c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f");
  verify_decode("stake_test1uqehkck0lajq8gr28t9uxnuvgcqrc6070x3k9r8048z8y5gssrtvn", "stake_test", "e0337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  verify_decode("stake_test17rphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcljw6kf", "stake_test", "f0c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f");
  verify_decode("A12UEL5L", "a", "");
  verify_decode("an83characterlonghumanreadablepartthatcontainsthenumber1andtheexcludedcharactersbio1tt5tgs", "an83characterlonghumanreadablepartthatcontainsthenumber1andtheexcludedcharactersbio", "");
  verify_decode("abcdef1qpzry9x8gf2tvdw0s3jn54khce6mua7lmqqqxw", "abcdef", "00443214c74254b635cf84653a56d7c675be77df");
  verify_decode("11qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqc8247j", "1", "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
  verify_decode("split1checkupstagehandshakeupstreamerranterredcaperred2y9e3w", "split", "c5f38b70305f519bf66d85fb6cf03058f3dde463ecd7918f2dc743918f2d");
}

TEST(cardano_encoding_bech32_encode, canEncodeBech32Strings)
{
  verify_encode("addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgse35a3x", "addr", "019493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  verify_encode("addr1vpu5vlrf4xkxv2qpwngf6cjhtw542ayty80v8dyr49rf5eg0yu80w", "addr", "6079467c69a9ac66280174d09d62575ba955748b21dec3b483a9469a65");
  verify_encode("stake1vpu5vlrf4xkxv2qpwngf6cjhtw542ayty80v8dyr49rf5egfu2p0u", "stake", "6079467c69a9ac66280174d09d62575ba955748b21dec3b483a9469a65");
  verify_encode("addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgse35a3x", "addr", "019493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  verify_encode("addr1z8phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgs9yc0hh", "addr", "11c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  verify_encode("addr1yx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzerkr0vd4msrxnuwnccdxlhdjar77j6lg0wypcc9uar5d2shs2z78ve", "addr", "219493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8ec37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f");
  verify_encode("addr1x8phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gt7r0vd4msrxnuwnccdxlhdjar77j6lg0wypcc9uar5d2shskhj42g", "addr", "31c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542fc37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f");
  verify_encode("addr1gx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer5pnz75xxcrzqf96k", "addr", "419493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e8198bd431b03");
  verify_encode("addr128phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtupnz75xxcrtw79hu", "addr", "51c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f8198bd431b03");
  verify_encode("addr1vx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzers66hrl8", "addr", "619493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e");
  verify_encode("addr1w8phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcyjy7wx", "addr", "71c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f");
  verify_encode("stake1uyehkck0lajq8gr28t9uxnuvgcqrc6070x3k9r8048z8y5gh6ffgw", "stake", "e1337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  verify_encode("stake178phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcccycj5", "stake", "f1c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f");
  verify_encode("addr_test1qz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgs68faae", "addr_test", "009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  verify_encode("addr_test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgsxj90mg", "addr_test", "10c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  verify_encode("addr_test1yz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzerkr0vd4msrxnuwnccdxlhdjar77j6lg0wypcc9uar5d2shsf5r8qx", "addr_test", "209493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8ec37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f");
  verify_encode("addr_test1xrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gt7r0vd4msrxnuwnccdxlhdjar77j6lg0wypcc9uar5d2shs4p04xh", "addr_test", "30c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542fc37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f");
  verify_encode("addr_test1gz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer5pnz75xxcrdw5vky", "addr_test", "409493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e8198bd431b03");
  verify_encode("addr_test12rphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtupnz75xxcryqrvmw", "addr_test", "50c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f8198bd431b03");
  verify_encode("addr_test1vz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzerspjrlsz", "addr_test", "609493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e");
  verify_encode("addr_test1wrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcl6szpr", "addr_test", "70c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f");
  verify_encode("stake_test1uqehkck0lajq8gr28t9uxnuvgcqrc6070x3k9r8048z8y5gssrtvn", "stake_test", "e0337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  verify_encode("stake_test17rphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcljw6kf", "stake_test", "f0c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f");
  verify_encode("a12uel5l", "a", "");
  verify_encode("an83characterlonghumanreadablepartthatcontainsthenumber1andtheexcludedcharactersbio1tt5tgs", "an83characterlonghumanreadablepartthatcontainsthenumber1andtheexcludedcharactersbio", "");
  verify_encode("abcdef1qpzry9x8gf2tvdw0s3jn54khce6mua7lmqqqxw", "abcdef", "00443214c74254b635cf84653a56d7c675be77df");
  verify_encode("11qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqc8247j", "1", "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
  verify_encode("split1checkupstagehandshakeupstreamerranterredcaperred2y9e3w", "split", "c5f38b70305f519bf66d85fb6cf03058f3dde463ecd7918f2dc743918f2d");
}

TEST(cardano_encoding_bech32_encode, invalidBech32StringsReturnsError)
{
  std::vector<std::string> invalidChecksum = {
    "tc1qw508d6qejxtdg4y5r3zarvary0c5xw7kg3g4ty",
    "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t5",
    "BC13W508D6QEJXTDG4Y5R3ZARVARY0C5XW7KN40WF2",
    "bc1rw5uspcuh",
    "bc10w508d6qejxtdg4y5r3zarvary0c5xw7kw508d6qejxtdg4y5r3zarvary0c5xw7kw5rljs90",
    "BC1QR508D6QEJXTDG4Y5R3ZARVARYV98GJ9P",
    "tb1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3q0sL5k7",
    "stake_test1uyuqtqq84v9jrqm0asptaehtw7srrr7cnwuxyqz38a6e8scm6lcf3",
    "addr_test1qxkmuf2gqzsm5ejxm2amrwuq3pcc02cw6tttgsgqgafj46klskg5jjufdyf4znw8sjn37enwn5ge5l66qsx8srrpg3tq8du7us",
    "stake1ur84236ycjkxvt0r5l7tdqaatlhhec0hrpncqlv5gp58e0q2ajrqx",
    "addr1qznd7jmvw2a53ykmgg5c6dcqd9f35mtts77zf57wn6ern5x024r5f39vvck78fluk6pm6hl00nslwxr8sp7egsrg0j7q8y2a9d",
    "BC1QR508D6QEJXTdg4y5r3zarvaryv98gj9p",
    "21ibccqr508d6qejxtdg4y5r3zarvar98gj9p",
    "BCCQR508D6QEJXTdg4y5r3zarvaryv98gj9p",
    "2"
  };

  for (size_t i = 0; i < invalidChecksum.size(); ++i)
  {
    const std::string bech32    = invalidChecksum[i];
    size_t            hrp_size  = 0;
    const size_t      data_size = cardano_encoding_bech32_get_decoded_length(bech32.c_str(), bech32.size(), &hrp_size);
    byte_t*           data      = (byte_t*)malloc(data_size);
    char*             hrp       = (char*)malloc(hrp_size);

    cardano_error_t result = cardano_encoding_bech32_decode(bech32.c_str(), bech32.size(), hrp, hrp_size, data, data_size);

    // Assert
    EXPECT_THAT(result, testing::Not(CARDANO_SUCCESS));

    // Cleanup
    free(hrp);
    free(data);
  }
}

TEST(cardano_encoding_bech32_get_decoded_length, returnErrorIfBech32IsNull)
{
  // Act
  size_t hrp_size  = 0;
  size_t data_size = cardano_encoding_bech32_get_decoded_length(nullptr, 0, &hrp_size);

  // Assert
  EXPECT_THAT(data_size, testing::Eq(0));
  EXPECT_THAT(hrp_size, testing::Eq(0));
}

TEST(cardano_encoding_bech32_get_decoded_length, returnErrorIfHrpIsNull)
{
  // Act
  size_t data_size = cardano_encoding_bech32_get_decoded_length("", 0, nullptr);

  // Assert
  EXPECT_THAT(data_size, testing::Eq(0));
}

TEST(cardano_encoding_bech32_get_decoded_length, returnErrorIfDataIsNull)
{
  // Act
  size_t data_size = cardano_encoding_bech32_get_decoded_length("", 1, nullptr);

  // Assert
  EXPECT_THAT(data_size, testing::Eq(0));
}

TEST(cardano_encoding_bech32_encode, returnErrorIfHrpIsNull)
{
  // Act
  cardano_error_t result = cardano_encoding_bech32_encode(nullptr, 0, (const byte_t*)"", 0, (char*)"", 0);

  // Assert
  EXPECT_THAT(result, testing::Not(CARDANO_SUCCESS));
}

TEST(cardano_encoding_bech32_encode, returnErrorIfDataIsNull)
{
  // Act
  cardano_error_t result = cardano_encoding_bech32_encode("", 0, nullptr, 10, (char*)"", 0);

  // Assert
  EXPECT_THAT(result, testing::Not(CARDANO_SUCCESS));
}

TEST(cardano_encoding_bech32_encode, returnErrorIfOutputNull)
{
  // Act
  cardano_error_t result = cardano_encoding_bech32_encode("", 0, (byte_t*)"", 0, nullptr, 0);

  // Assert
  EXPECT_THAT(result, testing::Not(CARDANO_SUCCESS));
}

TEST(cardano_encoding_bech32_encode, returnErrorIfOutputLengthIsZero)
{
  // Act
  cardano_error_t result = cardano_encoding_bech32_encode("", 0, (byte_t*)"", 0, (char*)"", 0);

  // Assert
  EXPECT_THAT(result, testing::Not(CARDANO_SUCCESS));
}

TEST(cardano_encoding_bech32_encode, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  const std::vector<byte_t> data     = hex_to_bytes("019493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  const char*               hpr      = "addr";
  const size_t              hrp_size = 0;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  const size_t bech32_size   = cardano_encoding_bech32_get_encoded_length(hpr, hrp_size, data.data(), data.size());
  char*        bech32_string = (char*)malloc(bech32_size);

  cardano_error_t result = cardano_encoding_bech32_encode(hpr, hrp_size, data.data(), data.size(), bech32_string, bech32_size);

  // Assert
  ASSERT_THAT(result, testing::Eq(CARDANO_MEMORY_ALLOCATION_FAILED));

  // Cleanup
  free(bech32_string);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_encoding_bech32_encode, returnErrorIfOutputBufferIsTooSmall)
{
  const std::vector<byte_t> data     = hex_to_bytes("019493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  const char*               hpr      = "addr";
  const size_t              hrp_size = 0;

  // Act
  const size_t bech32_size   = cardano_encoding_bech32_get_encoded_length(hpr, hrp_size, data.data(), data.size());
  char*        bech32_string = (char*)malloc(bech32_size - 1);

  cardano_error_t result = cardano_encoding_bech32_encode(hpr, hrp_size, data.data(), data.size(), bech32_string, 1);

  // Assert
  ASSERT_THAT(result, testing::Eq(CARDANO_INSUFFICIENT_BUFFER_SIZE));

  // Cleanup
  free(bech32_string);
}

TEST(cardano_encoding_bech32_encode, returnErrorIfEventualMemoryAllocationFails1)
{
  // Arrange
  const std::vector<byte_t> data     = hex_to_bytes("019493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  const char*               hpr      = "addr";
  const size_t              hrp_size = 0;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  const size_t bech32_size   = cardano_encoding_bech32_get_encoded_length(hpr, hrp_size, data.data(), data.size());
  char*        bech32_string = (char*)malloc(bech32_size);

  cardano_error_t result = cardano_encoding_bech32_encode(hpr, hrp_size, data.data(), data.size(), bech32_string, bech32_size);

  // Assert
  ASSERT_THAT(result, testing::Eq(CARDANO_MEMORY_ALLOCATION_FAILED));

  // Cleanup
  free(bech32_string);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_encoding_bech32_encode, returnErrorIfEventualMemoryAllocationFails2)
{
  // Arrange
  const std::vector<byte_t> data     = hex_to_bytes("019493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  const char*               hpr      = "addr";
  const size_t              hrp_size = 0;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_two_malloc, realloc, free);

  // Act
  const size_t bech32_size   = cardano_encoding_bech32_get_encoded_length(hpr, hrp_size, data.data(), data.size());
  char*        bech32_string = (char*)malloc(bech32_size);

  cardano_error_t result = cardano_encoding_bech32_encode(hpr, hrp_size, data.data(), data.size(), bech32_string, bech32_size);

  // Assert
  ASSERT_THAT(result, testing::Eq(CARDANO_MEMORY_ALLOCATION_FAILED));

  // Cleanup
  free(bech32_string);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_encoding_bech32_encode, returnErrorIfEventualMemoryAllocationFails3)
{
  // Arrange
  const std::vector<byte_t> data     = hex_to_bytes("019493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  const char*               hpr      = "addr";
  const size_t              hrp_size = 0;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  const size_t bech32_size   = cardano_encoding_bech32_get_encoded_length(hpr, hrp_size, data.data(), data.size());
  char*        bech32_string = (char*)malloc(bech32_size);

  cardano_error_t result = cardano_encoding_bech32_encode(hpr, hrp_size, data.data(), data.size(), bech32_string, bech32_size);

  // Assert
  ASSERT_THAT(result, testing::Eq(CARDANO_MEMORY_ALLOCATION_FAILED));

  // Cleanup
  free(bech32_string);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_encoding_bech32_encode, returnErrorIfEventualMemoryAllocationFails4)
{
  // Arrange
  const std::vector<byte_t> data     = hex_to_bytes("019493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  const char*               hpr      = "addr";
  const size_t              hrp_size = 0;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_four_malloc, realloc, free);

  // Act
  const size_t bech32_size   = cardano_encoding_bech32_get_encoded_length(hpr, hrp_size, data.data(), data.size());
  char*        bech32_string = (char*)malloc(bech32_size);

  cardano_error_t result = cardano_encoding_bech32_encode(hpr, hrp_size, data.data(), data.size(), bech32_string, bech32_size);

  // Assert
  ASSERT_THAT(result, testing::Eq(CARDANO_MEMORY_ALLOCATION_FAILED));

  // Cleanup
  free(bech32_string);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_encoding_bech32_encode, returnErrorIfEventualMemoryAllocationFails5)
{
  // Arrange
  const std::vector<byte_t> data     = hex_to_bytes("019493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251");
  const char*               hpr      = "addr";
  const size_t              hrp_size = 0;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_five_malloc, realloc, free);

  // Act
  const size_t bech32_size   = cardano_encoding_bech32_get_encoded_length(hpr, hrp_size, data.data(), data.size());
  char*        bech32_string = (char*)malloc(bech32_size);

  cardano_error_t result = cardano_encoding_bech32_encode(hpr, hrp_size, data.data(), data.size(), bech32_string, bech32_size);

  // Assert
  ASSERT_THAT(result, testing::Eq(CARDANO_MEMORY_ALLOCATION_FAILED));

  // Cleanup
  free(bech32_string);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_encoding_bech32_decode, returnErrorIfInputIsNull)
{
  // Act
  cardano_error_t result = cardano_encoding_bech32_decode(nullptr, 0, (char*)"", 0, (byte_t*)"", 0);

  // Assert
  EXPECT_THAT(result, testing::Not(CARDANO_SUCCESS));
}

TEST(cardano_encoding_bech32_decode, returnErrorIfHrpIsNull)
{
  // Act
  cardano_error_t result = cardano_encoding_bech32_decode((char*)"", 0, nullptr, 0, (byte_t*)"", 0);

  // Assert
  EXPECT_THAT(result, testing::Not(CARDANO_SUCCESS));
}

TEST(cardano_encoding_bech32_decode, returnErrorIfDataIsNull)
{
  // Act
  cardano_error_t result = cardano_encoding_bech32_decode("", 0, (char*)"", 0, nullptr, 10);

  // Assert
  EXPECT_THAT(result, testing::Not(CARDANO_SUCCESS));
}

TEST(cardano_encoding_bech32_decode, returnErrorIfDecodedLengthIsLessThanOutputLength)
{
  // Arrange
  const std::string bech32    = "addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgse35a3x";
  size_t            hrp_size  = 0;
  const size_t      data_size = cardano_encoding_bech32_get_decoded_length(bech32.c_str(), bech32.size(), &hrp_size);
  byte_t*           data      = (byte_t*)malloc(data_size);
  char*             hrp       = (char*)malloc(hrp_size);

  // Act
  cardano_error_t result = cardano_encoding_bech32_decode(bech32.c_str(), bech32.size(), hrp, hrp_size, data, data_size - 1);

  // Assert
  EXPECT_THAT(result, testing::Not(CARDANO_SUCCESS));

  // Cleanup
  free(hrp);
  free(data);
}

TEST(cardano_encoding_bech32_decode, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  const std::string bech32    = "addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgse35a3x";
  size_t            hrp_size  = 0;
  const size_t      data_size = cardano_encoding_bech32_get_decoded_length(bech32.c_str(), bech32.size(), &hrp_size);
  byte_t*           data      = (byte_t*)malloc(data_size);
  char*             hrp       = (char*)malloc(hrp_size);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_encoding_bech32_decode(bech32.c_str(), bech32.size(), hrp, hrp_size, data, data_size);

  // Assert
  EXPECT_THAT(result, testing::Eq(CARDANO_MEMORY_ALLOCATION_FAILED));

  // Cleanup
  free(hrp);
  free(data);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_encoding_bech32_decode, returnErrorIfEventualMemoryAllocationFails1)
{
  // Arrange
  const std::string bech32    = "addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgse35a3x";
  size_t            hrp_size  = 0;
  const size_t      data_size = cardano_encoding_bech32_get_decoded_length(bech32.c_str(), bech32.size(), &hrp_size);
  byte_t*           data      = (byte_t*)malloc(data_size);
  char*             hrp       = (char*)malloc(hrp_size);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_encoding_bech32_decode(bech32.c_str(), bech32.size(), hrp, hrp_size, data, data_size);

  // Assert
  EXPECT_THAT(result, testing::Eq(CARDANO_ERROR_DECODING));

  // Cleanup
  free(hrp);
  free(data);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_encoding_bech32_decode, returnErrorIfEventualMemoryAllocationFails2)
{
  // Arrange
  const std::string bech32    = "addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgse35a3x";
  size_t            hrp_size  = 0;
  const size_t      data_size = cardano_encoding_bech32_get_decoded_length(bech32.c_str(), bech32.size(), &hrp_size);
  byte_t*           data      = (byte_t*)malloc(data_size);
  char*             hrp       = (char*)malloc(hrp_size);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_two_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_encoding_bech32_decode(bech32.c_str(), bech32.size(), hrp, hrp_size, data, data_size);

  // Assert
  EXPECT_THAT(result, testing::Eq(CARDANO_ERROR_DECODING));

  // Cleanup
  free(hrp);
  free(data);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_encoding_bech32_decode, returnErrorIfEventualMemoryAllocationFails3)
{
  // Arrange
  const std::string bech32    = "addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgse35a3x";
  size_t            hrp_size  = 0;
  const size_t      data_size = cardano_encoding_bech32_get_decoded_length(bech32.c_str(), bech32.size(), &hrp_size);
  byte_t*           data      = (byte_t*)malloc(data_size);
  char*             hrp       = (char*)malloc(hrp_size);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_encoding_bech32_decode(bech32.c_str(), bech32.size(), hrp, hrp_size, data, data_size);

  // Assert
  EXPECT_THAT(result, testing::Eq(CARDANO_ERROR_DECODING));

  // Cleanup
  free(hrp);
  free(data);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_encoding_bech32_decode, returnErrorIfEventualMemoryAllocationFails4)
{
  // Arrange
  const std::string bech32    = "addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgse35a3x";
  size_t            hrp_size  = 0;
  const size_t      data_size = cardano_encoding_bech32_get_decoded_length(bech32.c_str(), bech32.size(), &hrp_size);
  byte_t*           data      = (byte_t*)malloc(data_size);
  char*             hrp       = (char*)malloc(hrp_size);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_four_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_encoding_bech32_decode(bech32.c_str(), bech32.size(), hrp, hrp_size, data, data_size);

  // Assert
  EXPECT_THAT(result, testing::Eq(CARDANO_ERROR_DECODING));

  // Cleanup
  free(hrp);
  free(data);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_encoding_bech32_decode, returnErrorIfEventualMemoryAllocationFails5)
{
  // Arrange
  const std::string bech32    = "addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgse35a3x";
  size_t            hrp_size  = 0;
  const size_t      data_size = cardano_encoding_bech32_get_decoded_length(bech32.c_str(), bech32.size(), &hrp_size);
  byte_t*           data      = (byte_t*)malloc(data_size);
  char*             hrp       = (char*)malloc(hrp_size);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_five_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_encoding_bech32_decode(bech32.c_str(), bech32.size(), hrp, hrp_size, data, data_size);

  // Assert
  EXPECT_THAT(result, testing::Eq(CARDANO_ERROR_DECODING));

  // Cleanup
  free(hrp);
  free(data);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_encoding_bech32_decode, returnErrorIfEventualMemoryAllocationFails6)
{
  // Arrange
  const std::string bech32    = "addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgse35a3x";
  size_t            hrp_size  = 0;
  const size_t      data_size = cardano_encoding_bech32_get_decoded_length(bech32.c_str(), bech32.size(), &hrp_size);
  byte_t*           data      = (byte_t*)malloc(data_size);
  char*             hrp       = (char*)malloc(hrp_size);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_six_malloc, realloc, free);

  // Act
  cardano_error_t result = cardano_encoding_bech32_decode(bech32.c_str(), bech32.size(), hrp, hrp_size, data, data_size);

  // Assert
  EXPECT_THAT(result, testing::Eq(CARDANO_MEMORY_ALLOCATION_FAILED));

  // Cleanup
  free(hrp);
  free(data);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_encoding_bech32_decode, returnErrorIfHrpBufferIsTooSmall)
{
  // Arrange
  const std::string bech32    = "addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgse35a3x";
  size_t            hrp_size  = 0;
  const size_t      data_size = cardano_encoding_bech32_get_decoded_length(bech32.c_str(), bech32.size(), &hrp_size);
  byte_t*           data      = (byte_t*)malloc(data_size);
  char*             hrp       = (char*)malloc(hrp_size - 1);

  // Act
  cardano_error_t result = cardano_encoding_bech32_decode(bech32.c_str(), bech32.size(), hrp, hrp_size - 1, data, data_size);

  // Assert
  EXPECT_THAT(result, testing::Not(CARDANO_SUCCESS));

  // Cleanup
  free(hrp);
  free(data);
}
