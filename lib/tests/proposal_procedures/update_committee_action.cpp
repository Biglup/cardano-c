/**
 * \file update_committee_action.cpp
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
#include <cardano/proposal_procedures/update_committee_action.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                       = "8504825820000000000000000000000000000000000000000000000000000000000000000003d90102828200581c000000000000000000000000000000000000000000000000000000008200581c20000000000000000000000000000000000000000000000000000000a28200581c30000000000000000000000000000000000000000000000000000000018200581c4000000000000000000000000000000000000000000000000000000002d81e820105";
static const char* CBOR_WITHOUT_GOV_ACTION    = "8504f6d90102828200581c000000000000000000000000000000000000000000000000000000008200581c20000000000000000000000000000000000000000000000000000000a28200581c30000000000000000000000000000000000000000000000000000000018200581c4000000000000000000000000000000000000000000000000000000002d81e820105";
static const char* GOV_ACTION_CBOR            = "825820000000000000000000000000000000000000000000000000000000000000000003";
static const char* MEMBERS_TO_BE_REMOVED_CBOR = "d90102828200581c000000000000000000000000000000000000000000000000000000008200581c20000000000000000000000000000000000000000000000000000000";
static const char* MEMBERS_TO_BE_ADDED_CBOR   = "a28200581c30000000000000000000000000000000000000000000000000000000018200581c4000000000000000000000000000000000000000000000000000000002";
static const char* QUORUM_CBOR                = "d81e820105";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the update_committee_action.
 * @return A new instance of the update_committee_action.
 */
static cardano_update_committee_action_t*
new_default_update_committee_action(const char* cbor)
{
  cardano_update_committee_action_t* update_committee_action = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t                    result                  = cardano_update_committee_action_from_cbor(reader, &update_committee_action);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return update_committee_action;
};

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
 * @param cbor The cbor to use for the protocol param update.
 * @return A new instance of the protocol param update.
 */
static cardano_credential_set_t*
new_default_credential_set(const char* cbor)
{
  cardano_credential_set_t* credential_set = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result = cardano_credential_set_from_cbor(reader, &credential_set);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return credential_set;
}

/**
 * Creates a new default instance of the protocol param update.
 * @param cbor The cbor to use for the protocol param update.
 * @return A new instance of the protocol param update.
 */
static cardano_unit_interval_t*
new_default_unit_interval(const char* cbor)
{
  cardano_unit_interval_t* unit_interval = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result = cardano_unit_interval_from_cbor(reader, &unit_interval);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return unit_interval;
}

/**
 * Creates a new default instance of the protocol param update.
 * @param cbor The cbor to use for the protocol param update.
 * @return A new instance of the protocol param update.
 */
static cardano_committee_members_map_t*
new_default_committee_members_map(const char* cbor)
{
  cardano_committee_members_map_t* members_map = nullptr;

  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result = cardano_committee_members_map_from_cbor(reader, &members_map);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return members_map;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_update_committee_action_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = new_default_update_committee_action(CBOR);
  EXPECT_NE(update_committee_action, nullptr);

  // Act
  cardano_update_committee_action_ref(update_committee_action);

  // Assert
  EXPECT_THAT(update_committee_action, testing::Not((cardano_update_committee_action_t*)nullptr));
  EXPECT_EQ(cardano_update_committee_action_refcount(update_committee_action), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_update_committee_action_unref(&update_committee_action);
  cardano_update_committee_action_unref(&update_committee_action);
}

TEST(cardano_update_committee_action_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_update_committee_action_ref(nullptr);
}

TEST(cardano_update_committee_action_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = nullptr;

  // Act
  cardano_update_committee_action_unref(&update_committee_action);
}

TEST(cardano_update_committee_action_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_update_committee_action_unref((cardano_update_committee_action_t**)nullptr);
}

TEST(cardano_update_committee_action_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = new_default_update_committee_action(CBOR);
  EXPECT_NE(update_committee_action, nullptr);

  // Act
  cardano_update_committee_action_ref(update_committee_action);
  size_t ref_count = cardano_update_committee_action_refcount(update_committee_action);

  cardano_update_committee_action_unref(&update_committee_action);
  size_t updated_ref_count = cardano_update_committee_action_refcount(update_committee_action);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
}

TEST(cardano_update_committee_action_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = new_default_update_committee_action(CBOR);
  EXPECT_NE(update_committee_action, nullptr);

  // Act
  cardano_update_committee_action_ref(update_committee_action);
  size_t ref_count = cardano_update_committee_action_refcount(update_committee_action);

  cardano_update_committee_action_unref(&update_committee_action);
  size_t updated_ref_count = cardano_update_committee_action_refcount(update_committee_action);

  cardano_update_committee_action_unref(&update_committee_action);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(update_committee_action, (cardano_update_committee_action_t*)nullptr);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
}

TEST(cardano_update_committee_action_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_update_committee_action_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_update_committee_action_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = nullptr;
  const char*                        message                 = "This is a test message";

  // Act
  cardano_update_committee_action_set_last_error(update_committee_action, message);

  // Assert
  EXPECT_STREQ(cardano_update_committee_action_get_last_error(update_committee_action), "Object is NULL.");
}

TEST(cardano_update_committee_action_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = new_default_update_committee_action(CBOR);
  EXPECT_NE(update_committee_action, nullptr);

  const char* message = nullptr;

  // Act
  cardano_update_committee_action_set_last_error(update_committee_action, message);

  // Assert
  EXPECT_STREQ(cardano_update_committee_action_get_last_error(update_committee_action), "");

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
}

TEST(cardano_update_committee_action_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = NULL;

  // Act
  cardano_error_t result = cardano_update_committee_action_from_cbor(nullptr, &update_committee_action);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_update_committee_action_from_cbor, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_update_committee_action_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_update_committee_action_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t*             writer = cardano_cbor_writer_new();
  cardano_update_committee_action_t* cert   = new_default_update_committee_action(CBOR);
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_update_committee_action_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_update_committee_action_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_update_committee_action_to_cbor, returnsErrorIfActionIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_update_committee_action_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_update_committee_action_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_update_committee_action_to_cbor((cardano_update_committee_action_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

// Action specific tests

TEST(cardano_update_committee_action_new, canCreateNewInstanceWithoutGovAction)
{
  // Arrange
  cardano_credential_set_t*        credential_set      = new_default_credential_set(MEMBERS_TO_BE_REMOVED_CBOR);
  cardano_unit_interval_t*         quorum              = new_default_unit_interval(QUORUM_CBOR);
  cardano_committee_members_map_t* members_to_be_added = new_default_committee_members_map(MEMBERS_TO_BE_ADDED_CBOR);

  // Act
  cardano_update_committee_action_t* update_committee_action = NULL;

  cardano_error_t result = cardano_update_committee_action_new(credential_set, members_to_be_added, quorum, nullptr, &update_committee_action);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(update_committee_action, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  result = cardano_update_committee_action_to_cbor(update_committee_action, writer);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_WITHOUT_GOV_ACTION);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
  cardano_credential_set_unref(&credential_set);
  cardano_unit_interval_unref(&quorum);
  cardano_committee_members_map_unref(&members_to_be_added);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_update_committee_action_new, canCreateNewInstanceWithGovAction)
{
  // Arrange
  cardano_credential_set_t*        credential_set       = new_default_credential_set(MEMBERS_TO_BE_REMOVED_CBOR);
  cardano_unit_interval_t*         quorum               = new_default_unit_interval(QUORUM_CBOR);
  cardano_committee_members_map_t* members_to_be_added  = new_default_committee_members_map(MEMBERS_TO_BE_ADDED_CBOR);
  cardano_governance_action_id_t*  governance_action_id = new_default_governance_action_id(GOV_ACTION_CBOR);

  // Act
  cardano_update_committee_action_t* update_committee_action = NULL;

  cardano_error_t result = cardano_update_committee_action_new(credential_set, members_to_be_added, quorum, governance_action_id, &update_committee_action);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(update_committee_action, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  result = cardano_update_committee_action_to_cbor(update_committee_action, writer);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);

  char* hex = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
  cardano_governance_action_id_unref(&governance_action_id);
  cardano_credential_set_unref(&credential_set);
  cardano_unit_interval_unref(&quorum);
  cardano_committee_members_map_unref(&members_to_be_added);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_update_committee_action_new, returnsErrorIfFirstArgIsNull)
{
  // Act
  cardano_update_committee_action_t* update_committee_action = NULL;

  cardano_error_t result = cardano_update_committee_action_new(nullptr, nullptr, nullptr, nullptr, &update_committee_action);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_update_committee_action_new, returnsErrorIfSecondArgIsNull)
{
  // Act
  cardano_update_committee_action_t* update_committee_action = NULL;

  cardano_error_t result = cardano_update_committee_action_new((cardano_credential_set_t*)"", nullptr, nullptr, nullptr, &update_committee_action);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_update_committee_action_new, returnsErrorIfThirdArgIsNull)
{
  // Act
  cardano_update_committee_action_t* update_committee_action = NULL;

  cardano_error_t result = cardano_update_committee_action_new((cardano_credential_set_t*)"", (cardano_committee_members_map_t*)"", nullptr, nullptr, &update_committee_action);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_update_committee_action_new, returnsErrorIfFifthArgIsNull)
{
  // Act
  cardano_update_committee_action_t* update_committee_action = NULL;

  cardano_error_t result = cardano_update_committee_action_new((cardano_credential_set_t*)"", (cardano_committee_members_map_t*)"", (cardano_unit_interval_t*)"", nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_update_committee_action_new, returnsErrorIfMemoryAllocationFails)
{
  // Act
  cardano_credential_set_t*          credential_set          = new_default_credential_set(MEMBERS_TO_BE_REMOVED_CBOR);
  cardano_unit_interval_t*           quorum                  = new_default_unit_interval(QUORUM_CBOR);
  cardano_committee_members_map_t*   members_to_be_added     = new_default_committee_members_map(MEMBERS_TO_BE_ADDED_CBOR);
  cardano_update_committee_action_t* update_committee_action = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_update_committee_action_new(credential_set, members_to_be_added, quorum, nullptr, &update_committee_action);

  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
  cardano_credential_set_unref(&credential_set);
  cardano_unit_interval_unref(&quorum);
  cardano_committee_members_map_unref(&members_to_be_added);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_update_committee_action_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_update_committee_action_from_cbor(reader, &update_committee_action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_update_committee_action_from_cbor, returnsErrorIfInvalidArraySize)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("8100", strlen("8100"));

  // Act
  cardano_error_t result = cardano_update_committee_action_from_cbor(reader, &update_committee_action);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_update_committee_action_from_cbor, returnsErrorIfInvalidId)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("85effe820103", strlen("8501fe820103"));
  cardano_update_committee_action_t* update_committee_action = NULL;

  // Act
  cardano_error_t result = cardano_update_committee_action_from_cbor(reader, &update_committee_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_update_committee_action_from_cbor, returnsErrorIfInvalidGovId)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("8504efb81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d", strlen("8400f6b81f0018640118c80219012c03190190041901f4051a001e8480061a0bebc200071903200819038409d81e8201020ad81e8201030bd81e8201040cd81e8201050d820158200000000000000000000000000000000000000000000000000000000000000000101903e8111988b812a20098a61a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0374f693194a1f0a0198af1a0003236119032c01011903e819023b00011903e8195e7104011903e818201a0001ca761928eb041959d818641959d818641959d818641959d818641959d818641959d81864186418641959d81864194c5118201a0002acfa182019b551041a000363151901ff00011a00015c3518201a000797751936f404021a0002ff941a0006ea7818dc0001011903e8196ff604021a0003bd081a00034ec5183e011a00102e0f19312a011a00032e801901a5011a0002da781903e819cf06011a00013a34182019a8f118201903e818201a00013aac0119e143041903e80a1a00030219189c011a00030219189c011a0003207c1901d9011a000330001901ff0119ccf3182019fd40182019ffd5182019581e18201940b318201a00012adf18201a0002ff941a0006ea7818dc0001011a00010f92192da7000119eabb18201a0002ff941a0006ea7818dc0001011a0002ff941a0006ea7818dc0001011a0011b22c1a0005fdde00021a000c504e197712041a001d6af61a0001425b041a00040c660004001a00014fab18201a0003236119032c010119a0de18201a00033d7618201979f41820197fb8182019a95d1820197df718201995aa18201a0223accc0a1a0374f693194a1f0a1a02515e841980b30a1382d81e820102d81e82010214821b00000001000000001b000000010000000015821b00000001000000001b0000000100000000161903ba1719035418181864181985d81e820000d81e820101d81e820202d81e820303d81e820101181a8ad81e820000d81e820101d81e820202d81e820303d81e820404d81e820505d81e820606d81e820707d81e820808d81e820909181b1864181c18c8181d19012c181e1903e8181f1907d01820191388581c8293d319ef5b3ac72366dd28006bd315b715f7e7cfcbd3004129b80d"));
  cardano_update_committee_action_t* update_committee_action = NULL;

  // Act
  cardano_error_t result = cardano_update_committee_action_from_cbor(reader, &update_committee_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_update_committee_action_from_cbor, returnsErrorIfInvalidMembersToBeAdded)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("8504825820000000000000000000000000000000000000000000000000000000000000000003d90102828200581c000000000000000000000000000000000000000000000000000000008200581c20000000000000000000000000000000000000000000000000000000ef8200581c30000000000000000000000000000000000000000000000000000000018200581c4000000000000000000000000000000000000000000000000000000002d81e820105", strlen("8504825820000000000000000000000000000000000000000000000000000000000000000003d90102828200581c000000000000000000000000000000000000000000000000000000008200581c20000000000000000000000000000000000000000000000000000000a28200581c30000000000000000000000000000000000000000000000000000000018200581c4000000000000000000000000000000000000000000000000000000002d81e820105"));
  cardano_update_committee_action_t* update_committee_action = NULL;

  // Act
  cardano_error_t result = cardano_update_committee_action_from_cbor(reader, &update_committee_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_update_committee_action_from_cbor, returnsErrorIfInvalidMembersToBeRemoved)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("8504825820000000000000000000000000000000000000000000000000000000000000000003d9010282ef00581c000000000000000000000000000000000000000000000000000000008200581c20000000000000000000000000000000000000000000000000000000a28200581c30000000000000000000000000000000000000000000000000000000018200581c4000000000000000000000000000000000000000000000000000000002d81e820105", strlen("8504825820000000000000000000000000000000000000000000000000000000000000000003d90102828200581c000000000000000000000000000000000000000000000000000000008200581c20000000000000000000000000000000000000000000000000000000a28200581c30000000000000000000000000000000000000000000000000000000018200581c4000000000000000000000000000000000000000000000000000000002d81e820105"));
  cardano_update_committee_action_t* update_committee_action = NULL;

  // Act
  cardano_error_t result = cardano_update_committee_action_from_cbor(reader, &update_committee_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_update_committee_action_from_cbor, returnsErrorIfInvalidQuorum)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("8504825820000000000000000000000000000000000000000000000000000000000000000003d90102828200581c000000000000000000000000000000000000000000000000000000008200581c20000000000000000000000000000000000000000000000000000000a28200581c30000000000000000000000000000000000000000000000000000000018200581c4000000000000000000000000000000000000000000000000000000002efef820105", strlen("8504825820000000000000000000000000000000000000000000000000000000000000000003d90102828200581c000000000000000000000000000000000000000000000000000000008200581c20000000000000000000000000000000000000000000000000000000a28200581c30000000000000000000000000000000000000000000000000000000018200581c4000000000000000000000000000000000000000000000000000000002d81e820105"));
  cardano_update_committee_action_t* update_committee_action = NULL;

  // Act
  cardano_error_t result = cardano_update_committee_action_from_cbor(reader, &update_committee_action);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_update_committee_action_from_cbor, canDeserializeWithoutGovId)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR_WITHOUT_GOV_ACTION, strlen(CBOR_WITHOUT_GOV_ACTION));
  cardano_update_committee_action_t* update_committee_action = NULL;

  // Act
  cardano_error_t result = cardano_update_committee_action_from_cbor(reader, &update_committee_action);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(update_committee_action, nullptr);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  result = cardano_update_committee_action_to_cbor(update_committee_action, writer);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_WITHOUT_GOV_ACTION);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

// Getters and Setters

TEST(cardano_update_committee_action_set_governance_action_id, canSetGovernanceActionId)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = new_default_update_committee_action(CBOR);
  cardano_governance_action_id_t*    governance_action_id    = new_default_governance_action_id(GOV_ACTION_CBOR);

  // Act
  cardano_error_t result = cardano_update_committee_action_set_governance_action_id(update_committee_action, governance_action_id);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
  cardano_governance_action_id_unref(&governance_action_id);
}

TEST(cardano_update_committee_action_set_governance_action_id, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_governance_action_id_t* governance_action_id = new_default_governance_action_id(GOV_ACTION_CBOR);

  // Act
  cardano_error_t result = cardano_update_committee_action_set_governance_action_id(nullptr, governance_action_id);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_governance_action_id_unref(&governance_action_id);
}

TEST(cardano_update_committee_action_set_governance_action_id, canSetGovActionToNull)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = new_default_update_committee_action(CBOR);

  // Act
  cardano_error_t result = cardano_update_committee_action_set_governance_action_id(update_committee_action, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
}

TEST(cardano_update_committee_action_get_governance_action_id, canGetGovernanceActionId)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = new_default_update_committee_action(CBOR);
  cardano_governance_action_id_t*    governance_action_id    = new_default_governance_action_id(GOV_ACTION_CBOR);

  EXPECT_EQ(cardano_update_committee_action_set_governance_action_id(update_committee_action, governance_action_id), CARDANO_SUCCESS);

  // Act
  cardano_governance_action_id_t* governance_action_id_out = cardano_update_committee_action_get_governance_action_id(update_committee_action);

  // Assert
  EXPECT_NE(governance_action_id_out, nullptr);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
  cardano_governance_action_id_unref(&governance_action_id);
  cardano_governance_action_id_unref(&governance_action_id_out);
}

TEST(cardano_update_committee_action_get_governance_action_id, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_governance_action_id_t* governance_action_id = cardano_update_committee_action_get_governance_action_id(nullptr);

  // Assert
  EXPECT_EQ(governance_action_id, nullptr);
}

TEST(cardano_update_committee_action_get_governance_action_id, returnsNullIfGovActionIsNotSet)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = new_default_update_committee_action(CBOR_WITHOUT_GOV_ACTION);

  // Act
  cardano_governance_action_id_t* governance_action_id = cardano_update_committee_action_get_governance_action_id(update_committee_action);

  // Assert
  EXPECT_EQ(governance_action_id, nullptr);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
}

TEST(cardano_update_committee_action_set_members_to_be_removed, canSetCredentialSet)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = new_default_update_committee_action(CBOR);
  cardano_credential_set_t*          credential_set          = new_default_credential_set(MEMBERS_TO_BE_REMOVED_CBOR);

  // Act
  cardano_error_t result = cardano_update_committee_action_set_members_to_be_removed(update_committee_action, credential_set);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
  cardano_credential_set_unref(&credential_set);
}

TEST(cardano_update_committee_action_set_members_to_be_removed, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_credential_set_t* credential_set = new_default_credential_set(MEMBERS_TO_BE_REMOVED_CBOR);

  // Act
  cardano_error_t result = cardano_update_committee_action_set_members_to_be_removed(nullptr, credential_set);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_set_unref(&credential_set);
}

TEST(cardano_update_committee_action_set_members_to_be_removed, returnsErrorIfMembersIsNull)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = new_default_update_committee_action(CBOR);

  // Act
  cardano_error_t result = cardano_update_committee_action_set_members_to_be_removed(update_committee_action, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
}

TEST(cardano_update_committee_action_get_members_to_be_removed, canGetMembersToBeRemoved)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = new_default_update_committee_action(CBOR);
  cardano_credential_set_t*          credential_set          = new_default_credential_set(MEMBERS_TO_BE_REMOVED_CBOR);

  EXPECT_EQ(cardano_update_committee_action_set_members_to_be_removed(update_committee_action, credential_set), CARDANO_SUCCESS);

  // Act
  cardano_credential_set_t* credential_set_out = cardano_update_committee_action_get_members_to_be_removed(update_committee_action);

  // Assert
  EXPECT_NE(credential_set_out, nullptr);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
  cardano_credential_set_unref(&credential_set);
  cardano_credential_set_unref(&credential_set_out);
}

TEST(cardano_update_committee_action_get_members_to_be_removed, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_credential_set_t* credential_set = cardano_update_committee_action_get_members_to_be_removed(nullptr);

  // Assert
  EXPECT_EQ(credential_set, nullptr);
}

TEST(cardano_update_committee_action_set_members_to_be_added, canSetMembersToBeAdded)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = new_default_update_committee_action(CBOR);
  cardano_committee_members_map_t*   members_to_be_added     = new_default_committee_members_map(MEMBERS_TO_BE_ADDED_CBOR);

  // Act
  cardano_error_t result = cardano_update_committee_action_set_members_to_be_added(update_committee_action, members_to_be_added);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
  cardano_committee_members_map_unref(&members_to_be_added);
}

TEST(cardano_update_committee_action_set_members_to_be_added, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_committee_members_map_t* members_to_be_added = new_default_committee_members_map(MEMBERS_TO_BE_ADDED_CBOR);

  // Act
  cardano_error_t result = cardano_update_committee_action_set_members_to_be_added(nullptr, members_to_be_added);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_committee_members_map_unref(&members_to_be_added);
}

TEST(cardano_update_committee_action_set_members_to_be_added, returnsErrorIfMembersIsNull)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = new_default_update_committee_action(CBOR);

  // Act
  cardano_error_t result = cardano_update_committee_action_set_members_to_be_added(update_committee_action, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
}

TEST(cardano_update_committee_action_get_members_to_be_added, canGetMembersToBeAdded)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = new_default_update_committee_action(CBOR);
  cardano_committee_members_map_t*   members_to_be_added     = new_default_committee_members_map(MEMBERS_TO_BE_ADDED_CBOR);

  EXPECT_EQ(cardano_update_committee_action_set_members_to_be_added(update_committee_action, members_to_be_added), CARDANO_SUCCESS);

  // Act
  cardano_committee_members_map_t* members_to_be_added_out = cardano_update_committee_action_get_members_to_be_added(update_committee_action);

  // Assert
  EXPECT_NE(members_to_be_added_out, nullptr);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
  cardano_committee_members_map_unref(&members_to_be_added);
  cardano_committee_members_map_unref(&members_to_be_added_out);
}

TEST(cardano_update_committee_action_get_members_to_be_added, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_committee_members_map_t* members_to_be_added = cardano_update_committee_action_get_members_to_be_added(nullptr);

  // Assert
  EXPECT_EQ(members_to_be_added, nullptr);
}

TEST(cardano_update_committee_action_set_quorum, canSetQuorum)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = new_default_update_committee_action(CBOR);
  cardano_unit_interval_t*           quorum                  = new_default_unit_interval(QUORUM_CBOR);

  // Act
  cardano_error_t result = cardano_update_committee_action_set_quorum(update_committee_action, quorum);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
  cardano_unit_interval_unref(&quorum);
}

TEST(cardano_update_committee_action_set_quorum, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_unit_interval_t* quorum = new_default_unit_interval(QUORUM_CBOR);

  // Act
  cardano_error_t result = cardano_update_committee_action_set_quorum(nullptr, quorum);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_unit_interval_unref(&quorum);
}

TEST(cardano_update_committee_action_set_quorum, returnsErrorIfQuorumIsNull)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = new_default_update_committee_action(CBOR);

  // Act
  cardano_error_t result = cardano_update_committee_action_set_quorum(update_committee_action, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
}

TEST(cardano_update_committee_action_get_quorum, canGetQuorum)
{
  // Arrange
  cardano_update_committee_action_t* update_committee_action = new_default_update_committee_action(CBOR);
  cardano_unit_interval_t*           quorum                  = new_default_unit_interval(QUORUM_CBOR);

  EXPECT_EQ(cardano_update_committee_action_set_quorum(update_committee_action, quorum), CARDANO_SUCCESS);

  // Act
  cardano_unit_interval_t* quorum_out = cardano_update_committee_action_get_quorum(update_committee_action);

  // Assert
  EXPECT_NE(quorum_out, nullptr);

  // Cleanup
  cardano_update_committee_action_unref(&update_committee_action);
  cardano_unit_interval_unref(&quorum);
  cardano_unit_interval_unref(&quorum_out);
}

TEST(cardano_update_committee_action_get_quorum, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_unit_interval_t* quorum = cardano_update_committee_action_get_quorum(nullptr);

  // Assert
  EXPECT_EQ(quorum, nullptr);
}
