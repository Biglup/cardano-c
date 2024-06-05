/**
 * \file plutus_v2_script.cpp
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
#include <cardano/scripts/plutus_scripts/plutus_v2_script.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <cstdlib>
#include <gmock/gmock.h>

static const char* PLUTUS_V2_SCRIPT = "5908920100003233223232323232332232323232323232323232332232323232322223232533532323232325335001101d13357389211e77726f6e67207573616765206f66207265666572656e636520696e7075740001c3232533500221533500221333573466e1c00800408007c407854cd4004840784078d40900114cd4c8d400488888888888802d40044c08526221533500115333533550222350012222002350022200115024213355023320015021001232153353235001222222222222300e00250052133550253200150233355025200100115026320013550272253350011502722135002225335333573466e3c00801c0940904d40b00044c01800c884c09526135001220023333573466e1cd55cea80224000466442466002006004646464646464646464646464646666ae68cdc39aab9d500c480008cccccccccccc88888888888848cccccccccccc00403403002c02802402001c01801401000c008cd405c060d5d0a80619a80b80c1aba1500b33501701935742a014666aa036eb94068d5d0a804999aa80dbae501a35742a01066a02e0446ae85401cccd5406c08dd69aba150063232323333573466e1cd55cea801240004664424660020060046464646666ae68cdc39aab9d5002480008cc8848cc00400c008cd40b5d69aba15002302e357426ae8940088c98c80c0cd5ce01901a01709aab9e5001137540026ae854008c8c8c8cccd5cd19b8735573aa004900011991091980080180119a816bad35742a004605c6ae84d5d1280111931901819ab9c03203402e135573ca00226ea8004d5d09aba2500223263202c33573805c06005426aae7940044dd50009aba1500533501775c6ae854010ccd5406c07c8004d5d0a801999aa80dbae200135742a00460426ae84d5d1280111931901419ab9c02a02c026135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d55cf280089baa00135742a00860226ae84d5d1280211931900d19ab9c01c01e018375a00a6666ae68cdc39aab9d375400a9000100e11931900c19ab9c01a01c016101b132632017335738921035054350001b135573ca00226ea800448c88c008dd6000990009aa80d911999aab9f0012500a233500930043574200460066ae880080608c8c8cccd5cd19b8735573aa004900011991091980080180118061aba150023005357426ae8940088c98c8050cd5ce00b00c00909aab9e5001137540024646464646666ae68cdc39aab9d5004480008cccc888848cccc00401401000c008c8c8c8cccd5cd19b8735573aa0049000119910919800801801180a9aba1500233500f014357426ae8940088c98c8064cd5ce00d80e80b89aab9e5001137540026ae854010ccd54021d728039aba150033232323333573466e1d4005200423212223002004357426aae79400c8cccd5cd19b875002480088c84888c004010dd71aba135573ca00846666ae68cdc3a801a400042444006464c6403666ae7007407c06406005c4d55cea80089baa00135742a00466a016eb8d5d09aba2500223263201533573802e03202626ae8940044d5d1280089aab9e500113754002266aa002eb9d6889119118011bab00132001355018223233335573e0044a010466a00e66442466002006004600c6aae754008c014d55cf280118021aba200301613574200222440042442446600200800624464646666ae68cdc3a800a400046a02e600a6ae84d55cf280191999ab9a3370ea00490011280b91931900819ab9c01201400e00d135573aa00226ea80048c8c8cccd5cd19b875001480188c848888c010014c01cd5d09aab9e500323333573466e1d400920042321222230020053009357426aae7940108cccd5cd19b875003480088c848888c004014c01cd5d09aab9e500523333573466e1d40112000232122223003005375c6ae84d55cf280311931900819ab9c01201400e00d00c00b135573aa00226ea80048c8c8cccd5cd19b8735573aa004900011991091980080180118029aba15002375a6ae84d5d1280111931900619ab9c00e01000a135573ca00226ea80048c8cccd5cd19b8735573aa002900011bae357426aae7940088c98c8028cd5ce00600700409baa001232323232323333573466e1d4005200c21222222200323333573466e1d4009200a21222222200423333573466e1d400d2008233221222222233001009008375c6ae854014dd69aba135744a00a46666ae68cdc3a8022400c4664424444444660040120106eb8d5d0a8039bae357426ae89401c8cccd5cd19b875005480108cc8848888888cc018024020c030d5d0a8049bae357426ae8940248cccd5cd19b875006480088c848888888c01c020c034d5d09aab9e500b23333573466e1d401d2000232122222223005008300e357426aae7940308c98c804ccd5ce00a80b80880800780700680600589aab9d5004135573ca00626aae7940084d55cf280089baa0012323232323333573466e1d400520022333222122333001005004003375a6ae854010dd69aba15003375a6ae84d5d1280191999ab9a3370ea0049000119091180100198041aba135573ca00c464c6401866ae700380400280244d55cea80189aba25001135573ca00226ea80048c8c8cccd5cd19b875001480088c8488c00400cdd71aba135573ca00646666ae68cdc3a8012400046424460040066eb8d5d09aab9e500423263200933573801601a00e00c26aae7540044dd500089119191999ab9a3370ea00290021091100091999ab9a3370ea00490011190911180180218031aba135573ca00846666ae68cdc3a801a400042444004464c6401466ae7003003802001c0184d55cea80089baa0012323333573466e1d40052002200623333573466e1d40092000200623263200633573801001400800626aae74dd5000a4c244004244002921035054310012333333357480024a00c4a00c4a00c46a00e6eb400894018008480044488c0080049400848488c00800c4488004448c8c00400488cc00cc0080080041";
static const char* PLUTUS_V2_HASH   = "b3b7938690083d898380ce6482fcd9094a5268248cef3868507ac2bc";
static const char* PLUTUS_V2_CBOR   = "5908955908920100003233223232323232332232323232323232323232332232323232322223232533532323232325335001101d13357389211e77726f6e67207573616765206f66207265666572656e636520696e7075740001c3232533500221533500221333573466e1c00800408007c407854cd4004840784078d40900114cd4c8d400488888888888802d40044c08526221533500115333533550222350012222002350022200115024213355023320015021001232153353235001222222222222300e00250052133550253200150233355025200100115026320013550272253350011502722135002225335333573466e3c00801c0940904d40b00044c01800c884c09526135001220023333573466e1cd55cea80224000466442466002006004646464646464646464646464646666ae68cdc39aab9d500c480008cccccccccccc88888888888848cccccccccccc00403403002c02802402001c01801401000c008cd405c060d5d0a80619a80b80c1aba1500b33501701935742a014666aa036eb94068d5d0a804999aa80dbae501a35742a01066a02e0446ae85401cccd5406c08dd69aba150063232323333573466e1cd55cea801240004664424660020060046464646666ae68cdc39aab9d5002480008cc8848cc00400c008cd40b5d69aba15002302e357426ae8940088c98c80c0cd5ce01901a01709aab9e5001137540026ae854008c8c8c8cccd5cd19b8735573aa004900011991091980080180119a816bad35742a004605c6ae84d5d1280111931901819ab9c03203402e135573ca00226ea8004d5d09aba2500223263202c33573805c06005426aae7940044dd50009aba1500533501775c6ae854010ccd5406c07c8004d5d0a801999aa80dbae200135742a00460426ae84d5d1280111931901419ab9c02a02c026135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d5d1280089aba25001135744a00226ae8940044d55cf280089baa00135742a00860226ae84d5d1280211931900d19ab9c01c01e018375a00a6666ae68cdc39aab9d375400a9000100e11931900c19ab9c01a01c016101b132632017335738921035054350001b135573ca00226ea800448c88c008dd6000990009aa80d911999aab9f0012500a233500930043574200460066ae880080608c8c8cccd5cd19b8735573aa004900011991091980080180118061aba150023005357426ae8940088c98c8050cd5ce00b00c00909aab9e5001137540024646464646666ae68cdc39aab9d5004480008cccc888848cccc00401401000c008c8c8c8cccd5cd19b8735573aa0049000119910919800801801180a9aba1500233500f014357426ae8940088c98c8064cd5ce00d80e80b89aab9e5001137540026ae854010ccd54021d728039aba150033232323333573466e1d4005200423212223002004357426aae79400c8cccd5cd19b875002480088c84888c004010dd71aba135573ca00846666ae68cdc3a801a400042444006464c6403666ae7007407c06406005c4d55cea80089baa00135742a00466a016eb8d5d09aba2500223263201533573802e03202626ae8940044d5d1280089aab9e500113754002266aa002eb9d6889119118011bab00132001355018223233335573e0044a010466a00e66442466002006004600c6aae754008c014d55cf280118021aba200301613574200222440042442446600200800624464646666ae68cdc3a800a400046a02e600a6ae84d55cf280191999ab9a3370ea00490011280b91931900819ab9c01201400e00d135573aa00226ea80048c8c8cccd5cd19b875001480188c848888c010014c01cd5d09aab9e500323333573466e1d400920042321222230020053009357426aae7940108cccd5cd19b875003480088c848888c004014c01cd5d09aab9e500523333573466e1d40112000232122223003005375c6ae84d55cf280311931900819ab9c01201400e00d00c00b135573aa00226ea80048c8c8cccd5cd19b8735573aa004900011991091980080180118029aba15002375a6ae84d5d1280111931900619ab9c00e01000a135573ca00226ea80048c8cccd5cd19b8735573aa002900011bae357426aae7940088c98c8028cd5ce00600700409baa001232323232323333573466e1d4005200c21222222200323333573466e1d4009200a21222222200423333573466e1d400d2008233221222222233001009008375c6ae854014dd69aba135744a00a46666ae68cdc3a8022400c4664424444444660040120106eb8d5d0a8039bae357426ae89401c8cccd5cd19b875005480108cc8848888888cc018024020c030d5d0a8049bae357426ae8940248cccd5cd19b875006480088c848888888c01c020c034d5d09aab9e500b23333573466e1d401d2000232122222223005008300e357426aae7940308c98c804ccd5ce00a80b80880800780700680600589aab9d5004135573ca00626aae7940084d55cf280089baa0012323232323333573466e1d400520022333222122333001005004003375a6ae854010dd69aba15003375a6ae84d5d1280191999ab9a3370ea0049000119091180100198041aba135573ca00c464c6401866ae700380400280244d55cea80189aba25001135573ca00226ea80048c8c8cccd5cd19b875001480088c8488c00400cdd71aba135573ca00646666ae68cdc3a8012400046424460040066eb8d5d09aab9e500423263200933573801601a00e00c26aae7540044dd500089119191999ab9a3370ea00290021091100091999ab9a3370ea00490011190911180180218031aba135573ca00846666ae68cdc3a801a400042444004464c6401466ae7003003802001c0184d55cea80089baa0012323333573466e1d40052002200623333573466e1d40092000200623263200633573801001400800626aae74dd5000a4c244004244002921035054310012333333357480024a00c4a00c4a00c46a00e6eb400894018008480044488c0080049400848488c00800c4488004448c8c00400488cc00cc0080080041";

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

TEST(cardano_plutus_v2_script_new, canCreateAPlutusV2Script)
{
  // Arrange
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V2_SCRIPT);
  cardano_plutus_v2_script_t* script = NULL;
  cardano_error_t             error  = cardano_plutus_v2_script_new_bytes(bytes.data(), bytes.size(), &script);
  cardano_cbor_writer_t*      writer = cardano_cbor_writer_new();

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act

  error = cardano_plutus_v2_script_to_cbor(script, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, hex, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Assert

  EXPECT_THAT(hex, testing::StrEq(PLUTUS_V2_CBOR));

  // cleanup
  cardano_plutus_v2_script_unref(&script);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_plutus_v2_script_new, returnsErrorIfGivenNullScript)
{
  // Arrange
  cardano_plutus_v2_script_t* script = NULL;
  cardano_error_t             error  = cardano_plutus_v2_script_new_bytes(NULL, 0, &script);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_v2_script_new, returnsErrorIfGivenNullScriptPointer)
{
  // Arrange
  std::vector<byte_t> bytes = hexToBytes(PLUTUS_V2_SCRIPT);
  cardano_error_t     error = cardano_plutus_v2_script_new_bytes(bytes.data(), bytes.size(), NULL);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_v2_script_new, returnsErrorIfGivenEmptyScript)
{
  // Arrange
  cardano_plutus_v2_script_t* script = NULL;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V2_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v2_script_new_bytes(bytes.data(), 0, &script);

  // Assert
  EXPECT_EQ(error, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_plutus_v2_script_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_v2_script_t* script = NULL;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V2_SCRIPT);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_v2_script_new_bytes(bytes.data(), bytes.size(), &script);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
}

TEST(cardano_plutus_v2_script_new, returnsErrorIfMemoryAllocationFails2)
{
  // Arrange
  cardano_plutus_v2_script_t* script = NULL;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V2_SCRIPT);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_two_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_v2_script_new_bytes(bytes.data(), bytes.size(), &script);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
}

TEST(cardano_plutus_v2_script_new_bytes_from_hex, canCreateAPlutusV2ScriptFromHex)
{
  // Arrange
  cardano_plutus_v2_script_t* script = NULL;
  cardano_error_t             error  = cardano_plutus_v2_script_new_bytes_from_hex(PLUTUS_V2_SCRIPT, strlen(PLUTUS_V2_SCRIPT), &script);
  cardano_cbor_writer_t*      writer = cardano_cbor_writer_new();

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act

  error = cardano_plutus_v2_script_to_cbor(script, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, hex, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Assert

  EXPECT_THAT(hex, testing::StrEq(PLUTUS_V2_CBOR));

  // cleanup
  cardano_plutus_v2_script_unref(&script);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_plutus_v2_script_new_bytes_from_hex, returnsErrorIfGivenNullHex)
{
  // Arrange
  cardano_plutus_v2_script_t* script = NULL;
  cardano_error_t             error  = cardano_plutus_v2_script_new_bytes_from_hex(NULL, 0, &script);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_v2_script_new_bytes_from_hex, returnsErrorIfGivenEmptyHex)
{
  // Arrange
  cardano_plutus_v2_script_t* script = NULL;
  cardano_error_t             error  = cardano_plutus_v2_script_new_bytes_from_hex(PLUTUS_V2_SCRIPT, 0, &script);

  // Assert
  EXPECT_EQ(error, CARDANO_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_plutus_v2_script_new_bytes_from_hex, returnsErrorIfGivenNullScriptPointer)
{
  // Arrange
  cardano_error_t error = cardano_plutus_v2_script_new_bytes_from_hex(PLUTUS_V2_SCRIPT, strlen(PLUTUS_V2_SCRIPT), NULL);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_v2_script_new_bytes_from_hex, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_v2_script_t* script = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_v2_script_new_bytes_from_hex(PLUTUS_V2_SCRIPT, strlen(PLUTUS_V2_SCRIPT), &script);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
}

TEST(cardano_plutus_v2_script_new_bytes_from_hex, returnsErrorIfMemoryAllocationFails2)
{
  // Arrange
  cardano_plutus_v2_script_t* script = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_v2_script_new_bytes_from_hex(PLUTUS_V2_SCRIPT, strlen(PLUTUS_V2_SCRIPT), &script);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
}

TEST(cardano_plutus_v2_script_from_cbor, canCreateAPlutusV2ScriptFromCbor)
{
  // Arrange
  cardano_plutus_v2_script_t* script = NULL;
  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(PLUTUS_V2_CBOR, strlen(PLUTUS_V2_CBOR));
  cardano_cbor_writer_t*      writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_plutus_v2_script_from_cbor(reader, &script);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act

  error = cardano_plutus_v2_script_to_cbor(script, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, hex, hex_size);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Assert

  EXPECT_THAT(hex, testing::StrEq(PLUTUS_V2_CBOR));

  // cleanup
  cardano_plutus_v2_script_unref(&script);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_plutus_v2_script_from_cbor, returnsErrorIfGivenNullReader)
{
  // Arrange
  cardano_plutus_v2_script_t* script = NULL;
  cardano_error_t             error  = cardano_plutus_v2_script_from_cbor(NULL, &script);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_v2_script_from_cbor, returnsErrorIfGivenNullScriptPointer)
{
  // Arrange
  cardano_error_t error = cardano_plutus_v2_script_from_cbor((cardano_cbor_reader_t*)"", NULL);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_v2_script_from_cbor, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_v2_script_t* script = NULL;
  cardano_cbor_reader_t*      reader = cardano_cbor_reader_from_hex(PLUTUS_V2_CBOR, strlen(PLUTUS_V2_CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_plutus_v2_script_from_cbor(reader, &script);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);

  // cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_plutus_v2_script_to_cbor, returnsErrorIfGivenNullScript)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_error_t        error  = cardano_plutus_v2_script_to_cbor(NULL, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_plutus_v2_script_to_cbor, returnsErrorIfGivenNullWriter)
{
  // Arrange
  cardano_error_t error = cardano_plutus_v2_script_to_cbor((cardano_plutus_v2_script_t*)"", NULL);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_v2_script_to_raw_bytes, canConvertPlutusV2ScriptToRawBytes)
{
  // Arrange
  cardano_plutus_v2_script_t* script = NULL;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V2_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v2_script_new_bytes(bytes.data(), bytes.size(), &script);
  cardano_buffer_t*           buffer = NULL;

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act

  error = cardano_plutus_v2_script_to_raw_bytes(script, &buffer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Assert

  EXPECT_EQ(cardano_buffer_get_size(buffer), bytes.size());

  for (size_t i = 0; i < bytes.size(); i++)
  {
    EXPECT_EQ(cardano_buffer_get_data(buffer)[i], bytes[i]);
  }

  // cleanup
  cardano_plutus_v2_script_unref(&script);
  cardano_buffer_unref(&buffer);
}

TEST(cardano_plutus_v2_script_to_raw_bytes, returnsErrorIfGivenNullScript)
{
  // Arrange
  cardano_buffer_t* buffer = NULL;
  cardano_error_t   error  = cardano_plutus_v2_script_to_raw_bytes(NULL, &buffer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_plutus_v2_script_to_raw_bytes, returnsErrorIfGivenNullBufferPointer)
{
  // Arrange
  cardano_plutus_v2_script_t* script = NULL;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V2_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v2_script_new_bytes(bytes.data(), bytes.size(), &script);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_plutus_v2_script_to_raw_bytes(script, NULL);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // cleanup
  cardano_plutus_v2_script_unref(&script);
}

TEST(cardano_plutus_v2_script_get_hash, canGetTheHashOfAPlutusV2Script)
{
  // Arrange
  cardano_plutus_v2_script_t* script = NULL;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V2_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v2_script_new_bytes(bytes.data(), bytes.size(), &script);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_t* hash = cardano_plutus_v2_script_get_hash(script);

  // Assert
  size_t hash_size = cardano_blake2b_hash_get_hex_size(hash);
  char*  hash_hex  = (char*)malloc(hash_size);

  cardano_error_t hash_error = cardano_blake2b_hash_to_hex(hash, hash_hex, hash_size);

  EXPECT_EQ(hash_error, CARDANO_SUCCESS);

  EXPECT_THAT(hash_hex, testing::StrEq(PLUTUS_V2_HASH));

  // cleanup
  cardano_plutus_v2_script_unref(&script);
  cardano_blake2b_hash_unref(&hash);
  free(hash_hex);
}

TEST(cardano_plutus_v2_script_get_hash, returnsNullIfGivenNullScript)
{
  // Act
  cardano_blake2b_hash_t* hash = cardano_plutus_v2_script_get_hash(NULL);

  // Assert
  EXPECT_EQ(hash, (cardano_blake2b_hash_t*)nullptr);
}

TEST(cardano_plutus_v2_script_get_hash, returnsNullIfMemoryAllocationFails)
{
  // Arrange
  cardano_plutus_v2_script_t* script = NULL;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V2_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v2_script_new_bytes(bytes.data(), bytes.size(), &script);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_blake2b_hash_t* hash = cardano_plutus_v2_script_get_hash(script);

  // Assert
  EXPECT_EQ(hash, (cardano_blake2b_hash_t*)nullptr);

  // cleanup
  cardano_plutus_v2_script_unref(&script);
}

TEST(cardano_plutus_v2_script_equals, returnsTrueIfTwoPlutusV2ScriptsAreEqual)
{
  // Arrange
  cardano_plutus_v2_script_t* script1 = NULL;
  cardano_plutus_v2_script_t* script2 = NULL;
  std::vector<byte_t>         bytes   = hexToBytes(PLUTUS_V2_SCRIPT);
  cardano_error_t             error   = cardano_plutus_v2_script_new_bytes(bytes.data(), bytes.size(), &script1);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_v2_script_new_bytes(bytes.data(), bytes.size(), &script2);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool are_equal = cardano_plutus_v2_script_equals(script1, script2);

  // Assert
  EXPECT_TRUE(are_equal);

  // cleanup
  cardano_plutus_v2_script_unref(&script1);
  cardano_plutus_v2_script_unref(&script2);
}

TEST(cardano_plutus_v2_script_equals, returnsFalseIfTwoPlutusV2ScriptsAreNotEqual)
{
  // Arrange
  cardano_plutus_v2_script_t* script1 = NULL;
  cardano_plutus_v2_script_t* script2 = NULL;
  std::vector<byte_t>         bytes   = hexToBytes(PLUTUS_V2_SCRIPT);
  cardano_error_t             error   = cardano_plutus_v2_script_new_bytes(bytes.data(), bytes.size(), &script1);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_plutus_v2_script_new_bytes(bytes.data(), bytes.size() - 1, &script2);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool are_equal = cardano_plutus_v2_script_equals(script1, script2);

  // Assert
  EXPECT_FALSE(are_equal);

  // cleanup
  cardano_plutus_v2_script_unref(&script1);
  cardano_plutus_v2_script_unref(&script2);
}

TEST(cardano_plutus_v2_script_equals, returnsFalseIfGivenNullScript)
{
  // Arrange
  cardano_plutus_v2_script_t* script = NULL;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V2_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v2_script_new_bytes(bytes.data(), bytes.size(), &script);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  bool are_equal = cardano_plutus_v2_script_equals(script, NULL);

  // Assert
  EXPECT_FALSE(are_equal);

  // cleanup
  cardano_plutus_v2_script_unref(&script);
}

TEST(cardano_plutus_v2_script_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_plutus_v2_script_t* script = nullptr;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V2_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v2_script_new_bytes(bytes.data(), bytes.size(), &script);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_v2_script_ref(script);

  // Assert
  EXPECT_THAT(script, testing::Not((cardano_plutus_v2_script_t*)nullptr));
  EXPECT_EQ(cardano_plutus_v2_script_refcount(script), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_plutus_v2_script_unref(&script);
  cardano_plutus_v2_script_unref(&script);
}

TEST(cardano_plutus_v2_script_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_plutus_v2_script_ref(nullptr);
}

TEST(cardano_plutus_v2_script_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_plutus_v2_script_t* script = nullptr;

  // Act
  cardano_plutus_v2_script_unref(&script);
}

TEST(cardano_plutus_v2_script_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_plutus_v2_script_unref((cardano_plutus_v2_script_t**)nullptr);
}

TEST(cardano_plutus_v2_script_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_plutus_v2_script_t* script = nullptr;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V2_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v2_script_new_bytes(bytes.data(), bytes.size(), &script);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_v2_script_ref(script);
  size_t ref_count = cardano_plutus_v2_script_refcount(script);

  cardano_plutus_v2_script_unref(&script);
  size_t updated_ref_count = cardano_plutus_v2_script_refcount(script);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_plutus_v2_script_unref(&script);
}

TEST(cardano_plutus_v2_script_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_plutus_v2_script_t* script = nullptr;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V2_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v2_script_new_bytes(bytes.data(), bytes.size(), &script);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_plutus_v2_script_ref(script);
  size_t ref_count = cardano_plutus_v2_script_refcount(script);

  cardano_plutus_v2_script_unref(&script);
  size_t updated_ref_count = cardano_plutus_v2_script_refcount(script);

  cardano_plutus_v2_script_unref(&script);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(script, (cardano_plutus_v2_script_t*)nullptr);

  // Cleanup
  cardano_plutus_v2_script_unref(&script);
}

TEST(cardano_plutus_v2_script_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_plutus_v2_script_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_plutus_v2_script_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_plutus_v2_script_t* script  = nullptr;
  const char*                 message = "This is a test message";

  // Act
  cardano_plutus_v2_script_set_last_error(script, message);

  // Assert
  EXPECT_STREQ(cardano_plutus_v2_script_get_last_error(script), "Object is NULL.");
}

TEST(cardano_plutus_v2_script_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_plutus_v2_script_t* script = nullptr;
  std::vector<byte_t>         bytes  = hexToBytes(PLUTUS_V2_SCRIPT);
  cardano_error_t             error  = cardano_plutus_v2_script_new_bytes(bytes.data(), bytes.size(), &script);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_plutus_v2_script_set_last_error(script, message);

  // Assert
  EXPECT_STREQ(cardano_plutus_v2_script_get_last_error(script), "");

  // Cleanup
  cardano_plutus_v2_script_unref(&script);
}