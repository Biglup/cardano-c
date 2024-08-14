/**
 * \file parameter_change_action.cpp
 *
 * \author angel.castillo
 * \date   Aug 31, 2024
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
#include <cardano/proposal_procedures/parameter_change_action.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                       = "8400825820000000000000000000000000000000000000000000000000000000000000000003b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d";
static const char* CBOR_WITHOUT_GOV_ACTION    = "8400f6b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d";
static const char* CBOR_WITHOUT_POLICY_HASH   = "8400825820000000000000000000000000000000000000000000000000000000000000000003b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388f6";
static const char* GOV_ACTION_CBOR            = "825820000000000000000000000000000000000000000000000000000000000000000003";
static const char* PROTOCOL_PARAM_UPDATE_CBOR = "b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388";
static const char* POLICY_HASH                = "8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the parameter_change_action.
 * @return A new instance of the parameter_change_action.
 */
static cardano_parameter_change_action_t*
new_default_parameter_change_action()
{
  cardano_parameter_change_action_t* parameter_change_action = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t                    result                  = cardano_parameter_change_action_from_cbor(reader, &parameter_change_action);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return parameter_change_action;
};

/**
 * Creates a new default instance of the hash.
 * @return A new instance of the hash.
 */
static cardano_blake2b_hash_t*
new_default_hash(const char* hash)
{
  cardano_blake2b_hash_t* hash_instance = nullptr;

  cardano_error_t error = cardano_blake2b_hash_from_hex(hash, strlen(hash), &hash_instance);

  EXPECT_THAT(error, CARDANO_SUCCESS);

  return hash_instance;
}

/**
 * Creates a new default instance of the governance action id.
 * @return A new instance of the governance action id.
 */
static cardano_governance_action_id_t*
new_default_governance_action_id(const char* cbor)
{
  cardano_governance_action_id_t* governance_action_id = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result = cardano_governance_action_id_from_cbor(reader, &governance_action_id);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return governance_action_id;
}

/**
 * Creates a new default instance of the protocol param update.
 * @return A new instance of the protocol param update.
 */
static cardano_protocol_param_update_t*
new_default_protocol_param_update(const char* cbor)
{
  cardano_protocol_param_update_t* protocol_param_update = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result = cardano_protocol_param_update_from_cbor(reader, &protocol_param_update);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return protocol_param_update;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_parameter_change_action_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_parameter_change_action_t* parameter_change_action = new_default_parameter_change_action();
  EXPECT_NE(parameter_change_action, nullptr);

  // Act
  cardano_parameter_change_action_ref(parameter_change_action);

  // Assert
  EXPECT_THAT(parameter_change_action, testing::Not((cardano_parameter_change_action_t*)nullptr));
  EXPECT_EQ(cardano_parameter_change_action_refcount(parameter_change_action), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_parameter_change_action_unref(&parameter_change_action);
  cardano_parameter_change_action_unref(&parameter_change_action);
}

TEST(cardano_parameter_change_action_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_parameter_change_action_ref(nullptr);
}

TEST(cardano_parameter_change_action_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_parameter_change_action_t* parameter_change_action = nullptr;

  // Act
  cardano_parameter_change_action_unref(&parameter_change_action);
}

TEST(cardano_parameter_change_action_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_parameter_change_action_unref((cardano_parameter_change_action_t**)nullptr);
}

TEST(cardano_parameter_change_action_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_parameter_change_action_t* parameter_change_action = new_default_parameter_change_action();
  EXPECT_NE(parameter_change_action, nullptr);

  // Act
  cardano_parameter_change_action_ref(parameter_change_action);
  size_t ref_count = cardano_parameter_change_action_refcount(parameter_change_action);

  cardano_parameter_change_action_unref(&parameter_change_action);
  size_t updated_ref_count = cardano_parameter_change_action_refcount(parameter_change_action);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
}

TEST(cardano_parameter_change_action_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_parameter_change_action_t* parameter_change_action = new_default_parameter_change_action();
  EXPECT_NE(parameter_change_action, nullptr);

  // Act
  cardano_parameter_change_action_ref(parameter_change_action);
  size_t ref_count = cardano_parameter_change_action_refcount(parameter_change_action);

  cardano_parameter_change_action_unref(&parameter_change_action);
  size_t updated_ref_count = cardano_parameter_change_action_refcount(parameter_change_action);

  cardano_parameter_change_action_unref(&parameter_change_action);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(parameter_change_action, (cardano_parameter_change_action_t*)nullptr);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
}

TEST(cardano_parameter_change_action_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_parameter_change_action_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_parameter_change_action_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_parameter_change_action_t* parameter_change_action = nullptr;
  const char*                        message                 = "This is a test message";

  // Act
  cardano_parameter_change_action_set_last_error(parameter_change_action, message);

  // Assert
  EXPECT_STREQ(cardano_parameter_change_action_get_last_error(parameter_change_action), "Object is NULL.");
}

TEST(cardano_parameter_change_action_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_parameter_change_action_t* parameter_change_action = new_default_parameter_change_action();
  EXPECT_NE(parameter_change_action, nullptr);

  const char* message = nullptr;

  // Act
  cardano_parameter_change_action_set_last_error(parameter_change_action, message);

  // Assert
  EXPECT_STREQ(cardano_parameter_change_action_get_last_error(parameter_change_action), "");

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
}

TEST(cardano_parameter_change_action_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_parameter_change_action_t* parameter_change_action = NULL;

  // Act
  cardano_error_t result = cardano_parameter_change_action_from_cbor(nullptr, &parameter_change_action);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_parameter_change_action_from_cbor, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_parameter_change_action_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_parameter_change_action_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t*             writer = cardano_cbor_writer_new();
  cardano_parameter_change_action_t* cert   = new_default_parameter_change_action();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_parameter_change_action_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_parameter_change_action_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_parameter_change_action_to_cbor, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_parameter_change_action_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_parameter_change_action_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_parameter_change_action_to_cbor((cardano_parameter_change_action_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

// Action specific tests

TEST(cardano_parameter_change_action_new, canCreateNewInstanceWithoutGovAction)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = new_default_protocol_param_update(PROTOCOL_PARAM_UPDATE_CBOR);
  cardano_blake2b_hash_t*          policy_hash           = new_default_hash(POLICY_HASH);

  // Act
  cardano_parameter_change_action_t* parameter_change_action = NULL;

  cardano_error_t result = cardano_parameter_change_action_new(protocol_param_update, nullptr, policy_hash, &parameter_change_action);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(parameter_change_action, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  result = cardano_parameter_change_action_to_cbor(parameter_change_action, writer);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_WITHOUT_GOV_ACTION);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_blake2b_hash_unref(&policy_hash);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_parameter_change_action_new, canCreateNewInstanceWithGovAction)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = new_default_protocol_param_update(PROTOCOL_PARAM_UPDATE_CBOR);
  cardano_governance_action_id_t*  governance_action_id  = new_default_governance_action_id(GOV_ACTION_CBOR);
  cardano_blake2b_hash_t*          policy_hash           = new_default_hash(POLICY_HASH);

  // Act
  cardano_parameter_change_action_t* parameter_change_action = NULL;

  cardano_error_t result = cardano_parameter_change_action_new(protocol_param_update, governance_action_id, policy_hash, &parameter_change_action);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(parameter_change_action, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  result = cardano_parameter_change_action_to_cbor(parameter_change_action, writer);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);

  char* hex = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_governance_action_id_unref(&governance_action_id);
  cardano_blake2b_hash_unref(&policy_hash);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_parameter_change_action_new, canCreateNewInstanceWithoutPolicyHash)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = new_default_protocol_param_update(PROTOCOL_PARAM_UPDATE_CBOR);
  cardano_governance_action_id_t*  governance_action_id  = new_default_governance_action_id(GOV_ACTION_CBOR);

  // Act
  cardano_parameter_change_action_t* parameter_change_action = NULL;
  cardano_error_t                    result                  = cardano_parameter_change_action_new(protocol_param_update, governance_action_id, nullptr, &parameter_change_action);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(parameter_change_action, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  result = cardano_parameter_change_action_to_cbor(parameter_change_action, writer);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);

  char* hex = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_WITHOUT_POLICY_HASH);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_governance_action_id_unref(&governance_action_id);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_parameter_change_action_new, returnsErrorIfFirstArgIsNull)
{
  // Act
  cardano_parameter_change_action_t* parameter_change_action = NULL;

  cardano_error_t result = cardano_parameter_change_action_new(nullptr, nullptr, nullptr, &parameter_change_action);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_parameter_change_action_new, canSetPolicyHashToNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = new_default_protocol_param_update(PROTOCOL_PARAM_UPDATE_CBOR);

  // Act
  cardano_parameter_change_action_t* parameter_change_action = NULL;

  cardano_error_t result = cardano_parameter_change_action_new(protocol_param_update, nullptr, nullptr, &parameter_change_action);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(parameter_change_action, nullptr);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_parameter_change_action_new, returnsErrorIfFourthArgIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = new_default_protocol_param_update(PROTOCOL_PARAM_UPDATE_CBOR);
  cardano_blake2b_hash_t*          policy_hash           = new_default_hash(POLICY_HASH);

  // Act
  cardano_error_t result = cardano_parameter_change_action_new(protocol_param_update, nullptr, policy_hash, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_blake2b_hash_unref(&policy_hash);
}

TEST(cardano_parameter_change_action_new, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_parameter_change_action_t* parameter_change_action = NULL;
  cardano_protocol_param_update_t*   param_update            = new_default_protocol_param_update(PROTOCOL_PARAM_UPDATE_CBOR);
  cardano_blake2b_hash_t*            policy_hash             = new_default_hash(POLICY_HASH);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_parameter_change_action_new(param_update, nullptr, policy_hash, &parameter_change_action);

  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
  cardano_protocol_param_update_unref(&param_update);
  cardano_blake2b_hash_unref(&policy_hash);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_parameter_change_action_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_parameter_change_action_t* parameter_change_action = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_parameter_change_action_from_cbor(reader, &parameter_change_action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_parameter_change_action_from_cbor, returnsErrorIfInvalidArraySize)
{
  // Arrange
  cardano_parameter_change_action_t* parameter_change_action = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("8100", strlen("8100"));

  // Act
  cardano_error_t result = cardano_parameter_change_action_from_cbor(reader, &parameter_change_action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_parameter_change_action_from_cbor, returnsErrorIfInvalidId)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("84effe820103", strlen("8401fe820103"));
  cardano_parameter_change_action_t* parameter_change_action = NULL;

  // Act
  cardano_error_t result = cardano_parameter_change_action_from_cbor(reader, &parameter_change_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_parameter_change_action_from_cbor, returnsErrorIfInvalidGovAction)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("8400efb81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d", strlen("8400f6b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d"));
  cardano_parameter_change_action_t* parameter_change_action = NULL;

  // Act
  cardano_error_t result = cardano_parameter_change_action_from_cbor(reader, &parameter_change_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_parameter_change_action_from_cbor, returnsErrorIfInvalidProtocolParamUpdate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8400f6ef1f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d", strlen("8400f6b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d"));

  cardano_parameter_change_action_t* parameter_change_action = NULL;

  // Act
  cardano_error_t result = cardano_parameter_change_action_from_cbor(reader, &parameter_change_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_parameter_change_action_from_cbor, returnsErrorIfInvalidPolicyHash)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8400f6b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388ef1c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d", strlen("8400f6b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d"));

  cardano_parameter_change_action_t* parameter_change_action = NULL;

  // Act
  cardano_error_t result = cardano_parameter_change_action_from_cbor(reader, &parameter_change_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_parameter_change_action_from_cbor, canDeserializeWithoutGovId)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR_WITHOUT_GOV_ACTION, strlen(CBOR_WITHOUT_GOV_ACTION));
  cardano_parameter_change_action_t* parameter_change_action = NULL;

  // Act
  cardano_error_t result = cardano_parameter_change_action_from_cbor(reader, &parameter_change_action);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(parameter_change_action, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  result = cardano_parameter_change_action_to_cbor(parameter_change_action, writer);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_WITHOUT_GOV_ACTION);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_parameter_change_action_from_cbor, canDeserializeWithoutPolicyHash)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR_WITHOUT_POLICY_HASH, strlen(CBOR_WITHOUT_POLICY_HASH));
  cardano_parameter_change_action_t* parameter_change_action = NULL;

  // Act
  cardano_error_t result = cardano_parameter_change_action_from_cbor(reader, &parameter_change_action);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(parameter_change_action, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  result = cardano_parameter_change_action_to_cbor(parameter_change_action, writer);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_WITHOUT_POLICY_HASH);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

// Getters and Setters

TEST(cardano_parameter_change_action_set_governance_action_id, canSetGovernanceActionId)
{
  // Arrange
  cardano_parameter_change_action_t* parameter_change_action = new_default_parameter_change_action();
  cardano_governance_action_id_t*    governance_action_id    = new_default_governance_action_id(GOV_ACTION_CBOR);

  // Act
  cardano_error_t result = cardano_parameter_change_action_set_governance_action_id(parameter_change_action, governance_action_id);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
  cardano_governance_action_id_unref(&governance_action_id);
}

TEST(cardano_parameter_change_action_set_governance_action_id, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = new_default_governance_action_id(GOV_ACTION_CBOR);

  // Act
  cardano_error_t result = cardano_parameter_change_action_set_governance_action_id(nullptr, governance_action_id);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_governance_action_id_unref(&governance_action_id);
}

TEST(cardano_parameter_change_action_set_governance_action_id, canSetGovActionToNull)
{
  // Arrange
  cardano_parameter_change_action_t* parameter_change_action = new_default_parameter_change_action();

  // Act
  cardano_error_t result = cardano_parameter_change_action_set_governance_action_id(parameter_change_action, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
}

TEST(cardano_parameter_change_action_get_governance_action_id, canGetGovernanceActionId)
{
  // Arrange
  cardano_parameter_change_action_t* parameter_change_action = new_default_parameter_change_action();
  cardano_governance_action_id_t*    governance_action_id    = new_default_governance_action_id(GOV_ACTION_CBOR);

  EXPECT_EQ(cardano_parameter_change_action_set_governance_action_id(parameter_change_action, governance_action_id), CARDANO_SUCCESS);

  // Act
  cardano_governance_action_id_t* governance_action_id_out = cardano_parameter_change_action_get_governance_action_id(parameter_change_action);

  // Assert
  EXPECT_NE(governance_action_id_out, nullptr);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
  cardano_governance_action_id_unref(&governance_action_id);
  cardano_governance_action_id_unref(&governance_action_id_out);
}

TEST(cardano_parameter_change_action_get_governance_action_id, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_governance_action_id_t* governance_action_id = cardano_parameter_change_action_get_governance_action_id(nullptr);

  // Assert
  EXPECT_EQ(governance_action_id, nullptr);
}

TEST(cardano_parameter_change_action_set_protocol_param_update, canSetProtocolParamUpdate)
{
  // Arrange
  cardano_parameter_change_action_t* parameter_change_action = new_default_parameter_change_action();
  cardano_protocol_param_update_t*   protocol_param_update   = new_default_protocol_param_update(PROTOCOL_PARAM_UPDATE_CBOR);

  // Act
  cardano_error_t result = cardano_parameter_change_action_set_protocol_param_update(parameter_change_action, protocol_param_update);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_parameter_change_action_set_protocol_param_update, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_protocol_param_update_t* protocol_param_update = new_default_protocol_param_update(PROTOCOL_PARAM_UPDATE_CBOR);

  // Act
  cardano_error_t result = cardano_parameter_change_action_set_protocol_param_update(nullptr, protocol_param_update);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_protocol_param_update_unref(&protocol_param_update);
}

TEST(cardano_parameter_change_action_set_protocol_param_update, canSetProtocolParamUpdateToNull)
{
  // Arrange
  cardano_parameter_change_action_t* parameter_change_action = new_default_parameter_change_action();

  // Act
  cardano_error_t result = cardano_parameter_change_action_set_protocol_param_update(parameter_change_action, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
}

TEST(cardano_parameter_change_action_get_protocol_param_update, canGetProtocolParamUpdate)
{
  // Arrange
  cardano_parameter_change_action_t* parameter_change_action = new_default_parameter_change_action();
  cardano_protocol_param_update_t*   protocol_param_update   = new_default_protocol_param_update(PROTOCOL_PARAM_UPDATE_CBOR);

  EXPECT_EQ(cardano_parameter_change_action_set_protocol_param_update(parameter_change_action, protocol_param_update), CARDANO_SUCCESS);

  // Act
  cardano_protocol_param_update_t* protocol_param_update_out = cardano_parameter_change_action_get_protocol_param_update(parameter_change_action);

  // Assert
  EXPECT_NE(protocol_param_update_out, nullptr);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
  cardano_protocol_param_update_unref(&protocol_param_update);
  cardano_protocol_param_update_unref(&protocol_param_update_out);
}

TEST(cardano_parameter_change_action_get_protocol_param_update, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_protocol_param_update_t* protocol_param_update = cardano_parameter_change_action_get_protocol_param_update(nullptr);

  // Assert
  EXPECT_EQ(protocol_param_update, nullptr);
}

TEST(cardano_parameter_change_action_set_policy_hash, canSetPolicyHash)
{
  // Arrange
  cardano_parameter_change_action_t* parameter_change_action = new_default_parameter_change_action();
  cardano_blake2b_hash_t*            policy_hash             = new_default_hash(POLICY_HASH);

  // Act
  cardano_error_t result = cardano_parameter_change_action_set_policy_hash(parameter_change_action, policy_hash);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
  cardano_blake2b_hash_unref(&policy_hash);
}

TEST(cardano_parameter_change_action_set_policy_hash, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_blake2b_hash_t* policy_hash = new_default_hash(POLICY_HASH);

  // Act
  cardano_error_t result = cardano_parameter_change_action_set_policy_hash(nullptr, policy_hash);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_blake2b_hash_unref(&policy_hash);
}

TEST(cardano_parameter_change_action_set_policy_hash, canSetPolicyHashToNull)
{
  // Arrange
  cardano_parameter_change_action_t* parameter_change_action = new_default_parameter_change_action();

  // Act
  cardano_error_t result = cardano_parameter_change_action_set_policy_hash(parameter_change_action, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
}

TEST(cardano_parameter_change_action_get_policy_hash, canGetPolicyHash)
{
  // Arrange
  cardano_parameter_change_action_t* parameter_change_action = new_default_parameter_change_action();
  cardano_blake2b_hash_t*            policy_hash             = new_default_hash(POLICY_HASH);

  EXPECT_EQ(cardano_parameter_change_action_set_policy_hash(parameter_change_action, policy_hash), CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_t* policy_hash_out = cardano_parameter_change_action_get_policy_hash(parameter_change_action);

  // Assert
  EXPECT_NE(policy_hash_out, nullptr);

  // Cleanup
  cardano_parameter_change_action_unref(&parameter_change_action);
  cardano_blake2b_hash_unref(&policy_hash);
  cardano_blake2b_hash_unref(&policy_hash_out);
}

TEST(cardano_parameter_change_action_get_policy_hash, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_blake2b_hash_t* policy_hash = cardano_parameter_change_action_get_policy_hash(nullptr);

  // Assert
  EXPECT_EQ(policy_hash, nullptr);
}
