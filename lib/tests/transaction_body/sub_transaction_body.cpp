/**
 * \file sub_transaction_body.cpp
 *
 * \author angel.castillo
 * \date   Jul 17, 2026
 *
 * Copyright 2026 Biglup Labs
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
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/transaction_body/sub_transaction_body.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* MINIMAL_CBOR      = "a200d90102800180";
static const char* MINIMAL_CBOR_HASH = "da4603f4488d798af667794ab542f130a4bd1c20c7ed950c4648aaa95d0be4f4";
static const char* MAXIMAL_CBOR      = "b300d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d500018182583900"
                                       "9493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740"
                                       "ffb8e7aa9e5232dc820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f"
                                       "2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a1196"
                                       "68ae52a49b73725e326dc16579dcc373a240182846504154415445181e031903e804d90102818304581c26b17b78de4f"
                                       "035dc0bfce60d1d3c3a8085c38dcce5fb8767e518bed1901f405a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a"
                                       "2d965a99893828ec810f050758200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d50818"
                                       "6409a3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b3366"
                                       "7463ee575eeac1845bbc736b9c0bbc40ba82a14454534c413831581c7eae28af2208be856f7a119668ae52a49b73725e"
                                       "326dc16579dcc373a240182846504154415445181e0b58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6"
                                       "c56fe0e78f19d9d50ed90102828200581c00112233445566778899aabbccddeeff00112233445566778899aabb820158"
                                       "1caabbccddeeff00112233445566778899aabbccddeeff0011223344550f0112d90102818258200f3abbc8fc19c2e61b"
                                       "ab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d50013a18202581c00112233445566778899aabbccddeeff0011"
                                       "2233445566778899aabba18258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d50382"
                                       "01f614d9010281841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f81068276"
                                       "68747470733a2f2f7777772e736f6d6575726c2e696f582000000000000000000000000000000000000000000000000"
                                       "00000000000000000151907d0161903e81818a28200581c00112233445566778899aabbccddeeff00112233445566778"
                                       "899aabbf68201581caabbccddeeff00112233445566778899aabbccddeeff001122334455d879801819a1581de1cb0ec"
                                       "2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f1a000f4240181aa18200581c001122334455667788"
                                       "99aabbccddeeff00112233445566778899aabb821864191388";
static const char* MAXIMAL_CBOR_HASH = "dd44d62727dbc372683dd8f26712e419175895f1c32cf6e9ae2e72fc6746b459";

// Same body as MAXIMAL_CBOR, re-encoded without the cache: keys stay in ascending order and the
// output at key 1 is written in the post-Alonzo map form instead of the legacy array form.
static const char* MAXIMAL_FRESH_CBOR = "b300d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5000181a2005839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc01820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e031903e804d90102818304581c26b17b78de4f035dc0bfce60d1d3c3a8085c38dcce5fb8767e518bed1901f405a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f050758200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d508186409a3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c413831581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e0b58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d50ed90102828200581c00112233445566778899aabbccddeeff00112233445566778899aabb8201581caabbccddeeff00112233445566778899aabbccddeeff0011223344550f0112d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d50013a18202581c00112233445566778899aabbccddeeff00112233445566778899aabba18258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5038201f614d9010281841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8106827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000151907d0161903e81818a28200581c00112233445566778899aabbccddeeff00112233445566778899aabbf68201581caabbccddeeff00112233445566778899aabbccddeeff001122334455d879801819a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f1a000f4240181aa18200581c00112233445566778899aabbccddeeff00112233445566778899aabb821864191388";

static const char* EMPTY_INPUT_SET_CBOR     = "d9010280";
static const char* EMPTY_OUTPUT_LIST_CBOR   = "80";
static const char* INPUT_SET_CBOR           = "d90102818258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d500";
static const char* OUTPUT_LIST_CBOR         = "81825839009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e32c728d3861e164cab28cb8f006448139c8f1740ffb8e7aa9e5232dc820aa3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c411832581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e";
static const char* CERTIFICATES_CBOR        = "d90102818304581c26b17b78de4f035dc0bfce60d1d3c3a8085c38dcce5fb8767e518bed1901f4";
static const char* WITHDRAWALS_CBOR         = "a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f05";
static const char* HASH_CBOR                = "58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5";
static const char* MINT_CBOR                = "a3581c2a286ad895d091f2b3d168a6091ad2627d30a72761a5bc36eef00740a14014581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c413831581c7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373a240182846504154415445181e";
static const char* GUARDS_CBOR              = "d90102828200581c00112233445566778899aabbccddeeff00112233445566778899aabb8201581caabbccddeeff00112233445566778899aabbccddeeff001122334455";
static const char* VOTING_PROCEDURES_CBOR   = "a18202581c00112233445566778899aabbccddeeff00112233445566778899aabba18258200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5038201f6";
static const char* PROPOSAL_PROCEDURES_CBOR = "d9010281841a000f4240581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8106827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* REQUIRED_GUARDS_CBOR     = "a28200581c00112233445566778899aabbccddeeff00112233445566778899aabbf68201581caabbccddeeff00112233445566778899aabbccddeeff001122334455d87980";
static const char* DIRECT_DEPOSITS_CBOR     = "a1581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f1a000f4240";
static const char* BALANCE_INTERVALS_CBOR   = "a18200581c00112233445566778899aabbccddeeff00112233445566778899aabb821864191388";

static const char* FEE_KEY_CBOR               = "a300d90102800180020a";
static const char* COLLATERAL_KEY_CBOR        = "a300d901028001800d00";
static const char* COLLATERAL_RETURN_KEY_CBOR = "a300d901028001801000";
static const char* TOTAL_COLLATERAL_KEY_CBOR  = "a300d901028001801100";
static const char* NESTED_SUB_TX_KEY_CBOR     = "a300d901028001801700";
static const char* UNKNOWN_KEY_CBOR           = "a300d90102800180186300";
static const char* MISSING_INPUTS_CBOR        = "a10180";
static const char* MISSING_OUTPUTS_CBOR       = "a100d9010280";
static const char* EMPTY_CERTIFICATES_CBOR    = "a300d901028001800480";
static const char* EMPTY_WITHDRAWALS_CBOR     = "a300d9010280018005a0";
static const char* EMPTY_MINT_CBOR            = "a300d9010280018009a0";
static const char* EMPTY_GUARDS_CBOR          = "a300d901028001800e80";
static const char* EMPTY_REQUIRED_GUARDS_CBOR = "a300d901028001801818a0";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the sub_transaction_body.
 * @return A new instance of the sub_transaction_body.
 */
static cardano_sub_transaction_body_t*
new_default_sub_transaction_body()
{
  cardano_sub_transaction_body_t* sub_transaction_body = NULL;
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(MAXIMAL_CBOR, strlen(MAXIMAL_CBOR));
  cardano_error_t                 result               = cardano_sub_transaction_body_from_cbor(reader, &sub_transaction_body);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return sub_transaction_body;
};

/**
 * Decodes the given CBOR hex string and expects the decode to fail with the given error.
 *
 * @param cbor The CBOR hex string to decode.
 * @param expected_error The error the decode is expected to fail with.
 */
static void
expect_decode_failure(const char* cbor, const cardano_error_t expected_error)
{
  cardano_sub_transaction_body_t* sub_transaction_body = NULL;
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t result = cardano_sub_transaction_body_from_cbor(reader, &sub_transaction_body);

  EXPECT_EQ(result, expected_error);
  EXPECT_EQ(sub_transaction_body, (cardano_sub_transaction_body_t*)nullptr);

  cardano_cbor_reader_unref(&reader);
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_sub_transaction_body_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_sub_transaction_body_ref(sub_transaction_body);

  // Assert
  EXPECT_THAT(sub_transaction_body, testing::Not((cardano_sub_transaction_body_t*)nullptr));
  EXPECT_EQ(cardano_sub_transaction_body_refcount(sub_transaction_body), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_sub_transaction_body_ref(nullptr);
}

TEST(cardano_sub_transaction_body_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = nullptr;

  // Act
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_sub_transaction_body_unref((cardano_sub_transaction_body_t**)nullptr);
}

TEST(cardano_sub_transaction_body_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_sub_transaction_body_ref(sub_transaction_body);
  size_t ref_count = cardano_sub_transaction_body_refcount(sub_transaction_body);

  cardano_sub_transaction_body_unref(&sub_transaction_body);
  size_t updated_ref_count = cardano_sub_transaction_body_refcount(sub_transaction_body);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_sub_transaction_body_ref(sub_transaction_body);
  size_t ref_count = cardano_sub_transaction_body_refcount(sub_transaction_body);

  cardano_sub_transaction_body_unref(&sub_transaction_body);
  size_t updated_ref_count = cardano_sub_transaction_body_refcount(sub_transaction_body);

  cardano_sub_transaction_body_unref(&sub_transaction_body);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(sub_transaction_body, (cardano_sub_transaction_body_t*)nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_sub_transaction_body_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_sub_transaction_body_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = nullptr;
  const char*                     message              = "This is a test message";

  // Act
  cardano_sub_transaction_body_set_last_error(sub_transaction_body, message);

  // Assert
  EXPECT_STREQ(cardano_sub_transaction_body_get_last_error(sub_transaction_body), "Object is NULL.");
}

TEST(cardano_sub_transaction_body_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  const char* message = nullptr;

  // Act
  cardano_sub_transaction_body_set_last_error(sub_transaction_body, message);

  // Assert
  EXPECT_STREQ(cardano_sub_transaction_body_get_last_error(sub_transaction_body), "");

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = NULL;

  // Act
  cardano_error_t result = cardano_sub_transaction_body_from_cbor(nullptr, &sub_transaction_body);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_from_cbor, returnsErrorIfSubTransactionBodyIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(MAXIMAL_CBOR, strlen(MAXIMAL_CBOR));

  // Act
  cardano_error_t result = cardano_sub_transaction_body_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_body_from_cbor, returnsErrorIfNotAMap)
{
  // Act & Assert
  expect_decode_failure("80", CARDANO_ERROR_DECODING);
}

TEST(cardano_sub_transaction_body_from_cbor, rejectsFeeKey)
{
  // Act & Assert
  expect_decode_failure(FEE_KEY_CBOR, CARDANO_ERROR_INVALID_CBOR_MAP_KEY);
}

TEST(cardano_sub_transaction_body_from_cbor, rejectsCollateralKey)
{
  // Act & Assert
  expect_decode_failure(COLLATERAL_KEY_CBOR, CARDANO_ERROR_INVALID_CBOR_MAP_KEY);
}

TEST(cardano_sub_transaction_body_from_cbor, rejectsCollateralReturnKey)
{
  // Act & Assert
  expect_decode_failure(COLLATERAL_RETURN_KEY_CBOR, CARDANO_ERROR_INVALID_CBOR_MAP_KEY);
}

TEST(cardano_sub_transaction_body_from_cbor, rejectsTotalCollateralKey)
{
  // Act & Assert
  expect_decode_failure(TOTAL_COLLATERAL_KEY_CBOR, CARDANO_ERROR_INVALID_CBOR_MAP_KEY);
}

TEST(cardano_sub_transaction_body_from_cbor, rejectsNestedSubTransactionsKey)
{
  // Act & Assert
  expect_decode_failure(NESTED_SUB_TX_KEY_CBOR, CARDANO_ERROR_INVALID_CBOR_MAP_KEY);
}

TEST(cardano_sub_transaction_body_from_cbor, rejectsUnknownKey)
{
  // Act & Assert
  expect_decode_failure(UNKNOWN_KEY_CBOR, CARDANO_ERROR_INVALID_CBOR_MAP_KEY);
}

TEST(cardano_sub_transaction_body_from_cbor, rejectsUnusedKeys)
{
  // Act & Assert
  expect_decode_failure("a300d901028001800600", CARDANO_ERROR_INVALID_CBOR_MAP_KEY);
  expect_decode_failure("a300d901028001800a00", CARDANO_ERROR_INVALID_CBOR_MAP_KEY);
  expect_decode_failure("a300d901028001800c00", CARDANO_ERROR_INVALID_CBOR_MAP_KEY);
}

TEST(cardano_sub_transaction_body_from_cbor, rejectsDuplicatedKeys)
{
  // Act & Assert
  expect_decode_failure("a300d9010280018000d9010280", CARDANO_ERROR_DUPLICATED_CBOR_MAP_KEY);
}

TEST(cardano_sub_transaction_body_from_cbor, rejectsMissingInputs)
{
  // Act & Assert
  expect_decode_failure(MISSING_INPUTS_CBOR, CARDANO_ERROR_DECODING);
}

TEST(cardano_sub_transaction_body_from_cbor, rejectsMissingOutputs)
{
  // Act & Assert
  expect_decode_failure(MISSING_OUTPUTS_CBOR, CARDANO_ERROR_DECODING);
}

TEST(cardano_sub_transaction_body_from_cbor, rejectsEmptyCertificates)
{
  // Act & Assert
  expect_decode_failure(EMPTY_CERTIFICATES_CBOR, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);
}

TEST(cardano_sub_transaction_body_from_cbor, rejectsEmptyWithdrawals)
{
  // Act & Assert
  expect_decode_failure(EMPTY_WITHDRAWALS_CBOR, CARDANO_ERROR_INVALID_CBOR_MAP_SIZE);
}

TEST(cardano_sub_transaction_body_from_cbor, rejectsEmptyMint)
{
  // Act & Assert
  expect_decode_failure(EMPTY_MINT_CBOR, CARDANO_ERROR_INVALID_CBOR_MAP_SIZE);
}

TEST(cardano_sub_transaction_body_from_cbor, rejectsEmptyGuards)
{
  // Act & Assert
  expect_decode_failure(EMPTY_GUARDS_CBOR, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);
}

TEST(cardano_sub_transaction_body_from_cbor, rejectsEmptyRequiredTopLevelGuards)
{
  // Act & Assert
  expect_decode_failure(EMPTY_REQUIRED_GUARDS_CBOR, CARDANO_ERROR_INVALID_CBOR_MAP_SIZE);
}

TEST(cardano_sub_transaction_body_from_cbor, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = NULL;
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(MAXIMAL_CBOR, strlen(MAXIMAL_CBOR));

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_sub_transaction_body_from_cbor(reader, &sub_transaction_body);

  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  result = cardano_sub_transaction_body_from_cbor(reader, &sub_transaction_body);

  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_two_malloc, realloc, free);

  result = cardano_sub_transaction_body_from_cbor(reader, &sub_transaction_body);

  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_three_malloc, realloc, free);

  result = cardano_sub_transaction_body_from_cbor(reader, &sub_transaction_body);

  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_four_malloc, realloc, free);

  result = cardano_sub_transaction_body_from_cbor(reader, &sub_transaction_body);

  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_five_malloc, realloc, free);

  result = cardano_sub_transaction_body_from_cbor(reader, &sub_transaction_body);

  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_six_malloc, realloc, free);

  result = cardano_sub_transaction_body_from_cbor(reader, &sub_transaction_body);

  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_seventh_malloc, realloc, free);

  result = cardano_sub_transaction_body_from_cbor(reader, &sub_transaction_body);

  cardano_set_allocators(malloc, realloc, free);

  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_sub_transaction_body_to_cbor, preservesMinimalOriginalCborWhenEncoding)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = NULL;
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(MINIMAL_CBOR, strlen(MINIMAL_CBOR));

  // Act
  cardano_error_t result = cardano_sub_transaction_body_from_cbor(reader, &sub_transaction_body);

  ASSERT_EQ(result, CARDANO_SUCCESS);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_THAT(cardano_sub_transaction_body_to_cbor(sub_transaction_body, writer), CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(hex, MINIMAL_CBOR);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_sub_transaction_body_to_cbor, preservesMaximalOriginalCborWhenEncoding)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_THAT(cardano_sub_transaction_body_to_cbor(sub_transaction_body, writer), CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(hex, MAXIMAL_CBOR);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_sub_transaction_body_to_cbor, encodesKeysInAscendingOrderAfterClearingTheCache)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_sub_transaction_body_clear_cbor_cache(sub_transaction_body);

  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_THAT(cardano_sub_transaction_body_to_cbor(sub_transaction_body, writer), CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(hex, MAXIMAL_FRESH_CBOR);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_sub_transaction_body_to_cbor, returnsErrorIfSubTransactionBodyIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_sub_transaction_body_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_sub_transaction_body_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_sub_transaction_body_to_cbor((cardano_sub_transaction_body_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_new, canCreateNewInstance)
{
  // Act
  cardano_transaction_input_set_t*   inputs               = NULL;
  cardano_transaction_output_list_t* outputs              = NULL;
  cardano_sub_transaction_body_t*    sub_transaction_body = NULL;
  cardano_cbor_reader_t*             inputs_reader        = cardano_cbor_reader_from_hex(EMPTY_INPUT_SET_CBOR, strlen(EMPTY_INPUT_SET_CBOR));
  cardano_cbor_reader_t*             outputs_reader       = cardano_cbor_reader_from_hex(EMPTY_OUTPUT_LIST_CBOR, strlen(EMPTY_OUTPUT_LIST_CBOR));

  EXPECT_THAT(cardano_transaction_input_set_from_cbor(inputs_reader, &inputs), CARDANO_SUCCESS);
  EXPECT_THAT(cardano_transaction_output_list_from_cbor(outputs_reader, &outputs), CARDANO_SUCCESS);
  EXPECT_THAT(cardano_sub_transaction_body_new(inputs, outputs, NULL, &sub_transaction_body), CARDANO_SUCCESS);

  // Assert
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_THAT(cardano_sub_transaction_body_to_cbor(sub_transaction_body, writer), CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, MINIMAL_CBOR);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_output_list_unref(&outputs);
  cardano_cbor_reader_unref(&inputs_reader);
  cardano_cbor_reader_unref(&outputs_reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_sub_transaction_body_new, canCreateNewInstanceWithTTL)
{
  // Act
  cardano_transaction_input_set_t*   inputs               = NULL;
  cardano_transaction_output_list_t* outputs              = NULL;
  const uint64_t                     ttl                  = 1000;
  cardano_sub_transaction_body_t*    sub_transaction_body = NULL;
  cardano_cbor_reader_t*             inputs_reader        = cardano_cbor_reader_from_hex(EMPTY_INPUT_SET_CBOR, strlen(EMPTY_INPUT_SET_CBOR));
  cardano_cbor_reader_t*             outputs_reader       = cardano_cbor_reader_from_hex(EMPTY_OUTPUT_LIST_CBOR, strlen(EMPTY_OUTPUT_LIST_CBOR));

  EXPECT_THAT(cardano_transaction_input_set_from_cbor(inputs_reader, &inputs), CARDANO_SUCCESS);
  EXPECT_THAT(cardano_transaction_output_list_from_cbor(outputs_reader, &outputs), CARDANO_SUCCESS);
  EXPECT_THAT(cardano_sub_transaction_body_new(inputs, outputs, &ttl, &sub_transaction_body), CARDANO_SUCCESS);

  // Assert
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  EXPECT_THAT(cardano_sub_transaction_body_to_cbor(sub_transaction_body, writer), CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, "a300d90102800180031903e8");

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_output_list_unref(&outputs);
  cardano_cbor_reader_unref(&inputs_reader);
  cardano_cbor_reader_unref(&outputs_reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_sub_transaction_body_new, returnsErrorIfInputsIsNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = NULL;

  // Act
  cardano_error_t result = cardano_sub_transaction_body_new(nullptr, (cardano_transaction_output_list_t*)"", NULL, &sub_transaction_body);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_new, returnsErrorIfOutputsIsNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = NULL;

  // Act
  cardano_error_t result = cardano_sub_transaction_body_new((cardano_transaction_input_set_t*)"", nullptr, NULL, &sub_transaction_body);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_new, returnsErrorIfSubTransactionBodyIsNull)
{
  // Act
  cardano_error_t result = cardano_sub_transaction_body_new((cardano_transaction_input_set_t*)"", (cardano_transaction_output_list_t*)"", NULL, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_transaction_input_set_t*   inputs               = NULL;
  cardano_transaction_output_list_t* outputs              = NULL;
  const uint64_t                     ttl                  = 65000;
  cardano_sub_transaction_body_t*    sub_transaction_body = NULL;
  cardano_cbor_reader_t*             inputs_reader        = cardano_cbor_reader_from_hex(INPUT_SET_CBOR, strlen(INPUT_SET_CBOR));
  cardano_cbor_reader_t*             outputs_reader       = cardano_cbor_reader_from_hex(OUTPUT_LIST_CBOR, strlen(OUTPUT_LIST_CBOR));

  EXPECT_THAT(cardano_transaction_input_set_from_cbor(inputs_reader, &inputs), CARDANO_SUCCESS);
  EXPECT_THAT(cardano_transaction_output_list_from_cbor(outputs_reader, &outputs), CARDANO_SUCCESS);

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_sub_transaction_body_new(inputs, outputs, &ttl, &sub_transaction_body);

  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  result = cardano_sub_transaction_body_new(inputs, outputs, &ttl, &sub_transaction_body);

  cardano_set_allocators(malloc, realloc, free);

  ASSERT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_output_list_unref(&outputs);
  cardano_cbor_reader_unref(&inputs_reader);
  cardano_cbor_reader_unref(&outputs_reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_sub_transaction_body_get_inputs, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  cardano_transaction_input_set_t* inputs = cardano_sub_transaction_body_get_inputs(nullptr);

  // Assert
  EXPECT_EQ(inputs, nullptr);
}

TEST(cardano_sub_transaction_body_get_inputs, returnsInputs)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_transaction_input_set_t* inputs = cardano_sub_transaction_body_get_inputs(sub_transaction_body);

  // Assert
  EXPECT_NE(inputs, nullptr);
  EXPECT_EQ(cardano_transaction_input_set_get_length(inputs), 1);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_transaction_input_set_unref(&inputs);
}

TEST(cardano_sub_transaction_body_set_inputs, returnsErrorIfInputsNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_inputs(sub_transaction_body, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_inputs, returnsErrorIfSubTransactionBodyNull)
{
  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_inputs(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_set_inputs, canSetInputs)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  cardano_transaction_input_set_t* inputs = NULL;
  cardano_cbor_reader_t*           reader = cardano_cbor_reader_from_hex(INPUT_SET_CBOR, strlen(INPUT_SET_CBOR));

  EXPECT_THAT(cardano_transaction_input_set_from_cbor(reader, &inputs), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_inputs(sub_transaction_body, inputs);

  cardano_transaction_input_set_t* inputs_from_body = cardano_sub_transaction_body_get_inputs(sub_transaction_body);

  EXPECT_EQ(inputs_from_body, inputs);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_transaction_input_set_unref(&inputs);
  cardano_transaction_input_set_unref(&inputs_from_body);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_body_get_outputs, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  cardano_transaction_output_list_t* outputs = cardano_sub_transaction_body_get_outputs(nullptr);

  // Assert
  EXPECT_EQ(outputs, nullptr);
}

TEST(cardano_sub_transaction_body_get_outputs, returnsOutputs)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_transaction_output_list_t* outputs = cardano_sub_transaction_body_get_outputs(sub_transaction_body);

  // Assert
  EXPECT_NE(outputs, nullptr);
  EXPECT_EQ(cardano_transaction_output_list_get_length(outputs), 1);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_transaction_output_list_unref(&outputs);
}

TEST(cardano_sub_transaction_body_set_outputs, returnsErrorIfOutputsNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_outputs(sub_transaction_body, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_outputs, returnsErrorIfSubTransactionBodyNull)
{
  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_outputs(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_set_outputs, canSetOutputs)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  cardano_transaction_output_list_t* outputs = NULL;
  cardano_cbor_reader_t*             reader  = cardano_cbor_reader_from_hex(OUTPUT_LIST_CBOR, strlen(OUTPUT_LIST_CBOR));

  EXPECT_THAT(cardano_transaction_output_list_from_cbor(reader, &outputs), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_outputs(sub_transaction_body, outputs);

  cardano_transaction_output_list_t* outputs_from_body = cardano_sub_transaction_body_get_outputs(sub_transaction_body);

  EXPECT_EQ(outputs_from_body, outputs);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_transaction_output_list_unref(&outputs);
  cardano_transaction_output_list_unref(&outputs_from_body);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_body_get_invalid_after, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  const uint64_t* invalid_after = cardano_sub_transaction_body_get_invalid_after(nullptr);

  // Assert
  EXPECT_EQ(invalid_after, nullptr);
}

TEST(cardano_sub_transaction_body_get_invalid_after, returnsInvalidAfter)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  const uint64_t* invalid_after = cardano_sub_transaction_body_get_invalid_after(sub_transaction_body);

  // Assert
  ASSERT_NE(invalid_after, nullptr);
  EXPECT_EQ(*invalid_after, 1000);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_invalid_after, returnsErrorIfSubTransactionBodyIsNull)
{
  // Act
  uint64_t        slot   = 100;
  cardano_error_t result = cardano_sub_transaction_body_set_invalid_after(nullptr, &slot);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_set_invalid_after, canSetInvalidAfterToNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_invalid_after(sub_transaction_body, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_get_invalid_after(sub_transaction_body), nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_invalid_after, canSetInvalidAfter)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  uint64_t slot = 200000;

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_invalid_after(sub_transaction_body, &slot);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  const uint64_t* invalid_after = cardano_sub_transaction_body_get_invalid_after(sub_transaction_body);

  ASSERT_NE(invalid_after, nullptr);
  EXPECT_EQ(*invalid_after, slot);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_get_certificates, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  cardano_certificate_set_t* certificates = cardano_sub_transaction_body_get_certificates(nullptr);

  // Assert
  EXPECT_EQ(certificates, nullptr);
}

TEST(cardano_sub_transaction_body_get_certificates, returnsCertificates)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_certificate_set_t* certificates = cardano_sub_transaction_body_get_certificates(sub_transaction_body);

  // Assert
  EXPECT_NE(certificates, nullptr);
  EXPECT_EQ(cardano_certificate_set_get_length(certificates), 1);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_certificate_set_unref(&certificates);
}

TEST(cardano_sub_transaction_body_set_certificates, canSetCertificatesToNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_certificates(sub_transaction_body, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_get_certificates(sub_transaction_body), nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_certificates, returnsErrorIfSubTransactionBodyNull)
{
  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_certificates(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_set_certificates, canSetCertificates)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  cardano_certificate_set_t* certificates = NULL;
  cardano_cbor_reader_t*     reader       = cardano_cbor_reader_from_hex(CERTIFICATES_CBOR, strlen(CERTIFICATES_CBOR));

  EXPECT_THAT(cardano_certificate_set_from_cbor(reader, &certificates), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_certificates(sub_transaction_body, certificates);

  cardano_certificate_set_t* certificates_from_body = cardano_sub_transaction_body_get_certificates(sub_transaction_body);

  EXPECT_EQ(certificates_from_body, certificates);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_certificate_set_unref(&certificates);
  cardano_certificate_set_unref(&certificates_from_body);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_body_get_withdrawals, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  cardano_withdrawal_map_t* withdrawals = cardano_sub_transaction_body_get_withdrawals(nullptr);

  // Assert
  EXPECT_EQ(withdrawals, nullptr);
}

TEST(cardano_sub_transaction_body_get_withdrawals, returnsWithdrawals)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_withdrawal_map_t* withdrawals = cardano_sub_transaction_body_get_withdrawals(sub_transaction_body);

  // Assert
  EXPECT_NE(withdrawals, nullptr);
  EXPECT_EQ(cardano_withdrawal_map_get_length(withdrawals), 1);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_withdrawal_map_unref(&withdrawals);
}

TEST(cardano_sub_transaction_body_set_withdrawals, canSetWithdrawalsToNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_withdrawals(sub_transaction_body, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_get_withdrawals(sub_transaction_body), nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_withdrawals, returnsErrorIfSubTransactionBodyNull)
{
  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_withdrawals(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_set_withdrawals, canSetWithdrawals)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  cardano_withdrawal_map_t* withdrawals = NULL;
  cardano_cbor_reader_t*    reader      = cardano_cbor_reader_from_hex(WITHDRAWALS_CBOR, strlen(WITHDRAWALS_CBOR));

  EXPECT_THAT(cardano_withdrawal_map_from_cbor(reader, &withdrawals), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_withdrawals(sub_transaction_body, withdrawals);

  cardano_withdrawal_map_t* withdrawals_from_body = cardano_sub_transaction_body_get_withdrawals(sub_transaction_body);

  EXPECT_EQ(withdrawals_from_body, withdrawals);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_withdrawal_map_unref(&withdrawals);
  cardano_withdrawal_map_unref(&withdrawals_from_body);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_body_get_aux_data_hash, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  cardano_blake2b_hash_t* aux_data_hash = cardano_sub_transaction_body_get_aux_data_hash(nullptr);

  // Assert
  EXPECT_EQ(aux_data_hash, nullptr);
}

TEST(cardano_sub_transaction_body_get_aux_data_hash, returnsAuxDataHash)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_blake2b_hash_t* aux_data_hash = cardano_sub_transaction_body_get_aux_data_hash(sub_transaction_body);

  // Assert
  EXPECT_NE(aux_data_hash, nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_blake2b_hash_unref(&aux_data_hash);
}

TEST(cardano_sub_transaction_body_set_aux_data_hash, canSetAuxDataHashToNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_aux_data_hash(sub_transaction_body, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_get_aux_data_hash(sub_transaction_body), nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_aux_data_hash, returnsErrorIfSubTransactionBodyNull)
{
  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_aux_data_hash(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_set_aux_data_hash, canSetAuxDataHash)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  cardano_blake2b_hash_t* aux_data_hash = NULL;
  cardano_cbor_reader_t*  reader        = cardano_cbor_reader_from_hex(HASH_CBOR, strlen(HASH_CBOR));

  EXPECT_THAT(cardano_blake2b_hash_from_cbor(reader, &aux_data_hash), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_aux_data_hash(sub_transaction_body, aux_data_hash);

  cardano_blake2b_hash_t* aux_data_hash_from_body = cardano_sub_transaction_body_get_aux_data_hash(sub_transaction_body);

  EXPECT_EQ(aux_data_hash_from_body, aux_data_hash);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_blake2b_hash_unref(&aux_data_hash);
  cardano_blake2b_hash_unref(&aux_data_hash_from_body);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_body_get_invalid_before, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  const uint64_t* invalid_before = cardano_sub_transaction_body_get_invalid_before(nullptr);

  // Assert
  EXPECT_EQ(invalid_before, nullptr);
}

TEST(cardano_sub_transaction_body_get_invalid_before, returnsInvalidBefore)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  const uint64_t* invalid_before = cardano_sub_transaction_body_get_invalid_before(sub_transaction_body);

  // Assert
  ASSERT_NE(invalid_before, nullptr);
  EXPECT_EQ(*invalid_before, 100);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_invalid_before, returnsErrorIfSubTransactionBodyNull)
{
  // Act
  uint64_t        slot   = 100;
  cardano_error_t result = cardano_sub_transaction_body_set_invalid_before(nullptr, &slot);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_set_invalid_before, canSetInvalidBeforeToNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_invalid_before(sub_transaction_body, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_get_invalid_before(sub_transaction_body), nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_invalid_before, canSetInvalidBefore)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  uint64_t slot = 500;

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_invalid_before(sub_transaction_body, &slot);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  const uint64_t* invalid_before = cardano_sub_transaction_body_get_invalid_before(sub_transaction_body);

  ASSERT_NE(invalid_before, nullptr);
  EXPECT_EQ(*invalid_before, slot);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_get_mint, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  cardano_multi_asset_t* mint = cardano_sub_transaction_body_get_mint(nullptr);

  // Assert
  EXPECT_EQ(mint, nullptr);
}

TEST(cardano_sub_transaction_body_get_mint, returnsMint)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_multi_asset_t* mint = cardano_sub_transaction_body_get_mint(sub_transaction_body);

  // Assert
  EXPECT_NE(mint, nullptr);
  EXPECT_EQ(cardano_multi_asset_get_policy_count(mint), 3);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_multi_asset_unref(&mint);
}

TEST(cardano_sub_transaction_body_set_mint, canSetMintToNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_mint(sub_transaction_body, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_get_mint(sub_transaction_body), nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_mint, returnsErrorIfSubTransactionBodyNull)
{
  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_mint(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_set_mint, canSetMint)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  cardano_multi_asset_t* mint   = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(MINT_CBOR, strlen(MINT_CBOR));

  EXPECT_THAT(cardano_multi_asset_from_cbor(reader, &mint), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_mint(sub_transaction_body, mint);

  cardano_multi_asset_t* mint_from_body = cardano_sub_transaction_body_get_mint(sub_transaction_body);

  EXPECT_EQ(mint_from_body, mint);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_multi_asset_unref(&mint);
  cardano_multi_asset_unref(&mint_from_body);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_body_get_script_data_hash, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  cardano_blake2b_hash_t* script_data_hash = cardano_sub_transaction_body_get_script_data_hash(nullptr);

  // Assert
  EXPECT_EQ(script_data_hash, nullptr);
}

TEST(cardano_sub_transaction_body_get_script_data_hash, returnsScriptDataHash)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_blake2b_hash_t* script_data_hash = cardano_sub_transaction_body_get_script_data_hash(sub_transaction_body);

  // Assert
  EXPECT_NE(script_data_hash, nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_blake2b_hash_unref(&script_data_hash);
}

TEST(cardano_sub_transaction_body_set_script_data_hash, canSetScriptDataHashToNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_script_data_hash(sub_transaction_body, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_get_script_data_hash(sub_transaction_body), nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_script_data_hash, returnsErrorIfSubTransactionBodyNull)
{
  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_script_data_hash(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_set_script_data_hash, canSetScriptDataHash)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  cardano_blake2b_hash_t* script_data_hash = NULL;
  cardano_cbor_reader_t*  reader           = cardano_cbor_reader_from_hex(HASH_CBOR, strlen(HASH_CBOR));

  EXPECT_THAT(cardano_blake2b_hash_from_cbor(reader, &script_data_hash), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_script_data_hash(sub_transaction_body, script_data_hash);

  cardano_blake2b_hash_t* script_data_hash_from_body = cardano_sub_transaction_body_get_script_data_hash(sub_transaction_body);

  EXPECT_EQ(script_data_hash_from_body, script_data_hash);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_blake2b_hash_unref(&script_data_hash);
  cardano_blake2b_hash_unref(&script_data_hash_from_body);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_body_get_guards, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  cardano_guard_set_t* guards = cardano_sub_transaction_body_get_guards(nullptr);

  // Assert
  EXPECT_EQ(guards, nullptr);
}

TEST(cardano_sub_transaction_body_get_guards, returnsGuards)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_guard_set_t* guards = cardano_sub_transaction_body_get_guards(sub_transaction_body);

  // Assert
  EXPECT_NE(guards, nullptr);
  EXPECT_EQ(cardano_guard_set_get_length(guards), 2);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_guard_set_unref(&guards);
}

TEST(cardano_sub_transaction_body_set_guards, canSetGuardsToNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_guards(sub_transaction_body, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_get_guards(sub_transaction_body), nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_guards, returnsErrorIfSubTransactionBodyNull)
{
  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_guards(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_set_guards, canSetGuards)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  cardano_guard_set_t*   guards = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(GUARDS_CBOR, strlen(GUARDS_CBOR));

  EXPECT_THAT(cardano_guard_set_from_cbor(reader, &guards), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_guards(sub_transaction_body, guards);

  cardano_guard_set_t* guards_from_body = cardano_sub_transaction_body_get_guards(sub_transaction_body);

  EXPECT_EQ(guards_from_body, guards);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_guard_set_unref(&guards);
  cardano_guard_set_unref(&guards_from_body);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_body_get_network_id, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  const cardano_network_id_t* network_id = cardano_sub_transaction_body_get_network_id(nullptr);

  // Assert
  EXPECT_EQ(network_id, nullptr);
}

TEST(cardano_sub_transaction_body_get_network_id, returnsNetworkId)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  const cardano_network_id_t* network_id = cardano_sub_transaction_body_get_network_id(sub_transaction_body);

  // Assert
  ASSERT_NE(network_id, nullptr);
  EXPECT_EQ(*network_id, CARDANO_NETWORK_ID_MAIN_NET);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_network_id, returnsErrorIfSubTransactionBodyNull)
{
  // Act
  cardano_network_id_t network_id = CARDANO_NETWORK_ID_MAIN_NET;
  cardano_error_t      result     = cardano_sub_transaction_body_set_network_id(nullptr, &network_id);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_set_network_id, canSetNetworkIdToNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_network_id(sub_transaction_body, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_get_network_id(sub_transaction_body), nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_network_id, canSetNetworkId)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  cardano_network_id_t network_id = CARDANO_NETWORK_ID_TEST_NET;

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_network_id(sub_transaction_body, &network_id);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  const cardano_network_id_t* network_id_from_body = cardano_sub_transaction_body_get_network_id(sub_transaction_body);

  ASSERT_NE(network_id_from_body, nullptr);
  EXPECT_EQ(*network_id_from_body, network_id);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_get_reference_inputs, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  cardano_transaction_input_set_t* reference_inputs = cardano_sub_transaction_body_get_reference_inputs(nullptr);

  // Assert
  EXPECT_EQ(reference_inputs, nullptr);
}

TEST(cardano_sub_transaction_body_get_reference_inputs, returnsReferenceInputs)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_transaction_input_set_t* reference_inputs = cardano_sub_transaction_body_get_reference_inputs(sub_transaction_body);

  // Assert
  EXPECT_NE(reference_inputs, nullptr);
  EXPECT_EQ(cardano_transaction_input_set_get_length(reference_inputs), 1);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_transaction_input_set_unref(&reference_inputs);
}

TEST(cardano_sub_transaction_body_set_reference_inputs, canSetReferenceInputsToNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_reference_inputs(sub_transaction_body, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_get_reference_inputs(sub_transaction_body), nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_reference_inputs, returnsErrorIfSubTransactionBodyNull)
{
  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_reference_inputs(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_set_reference_inputs, canSetReferenceInputs)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  cardano_transaction_input_set_t* reference_inputs = NULL;
  cardano_cbor_reader_t*           reader           = cardano_cbor_reader_from_hex(INPUT_SET_CBOR, strlen(INPUT_SET_CBOR));

  EXPECT_THAT(cardano_transaction_input_set_from_cbor(reader, &reference_inputs), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_reference_inputs(sub_transaction_body, reference_inputs);

  cardano_transaction_input_set_t* reference_inputs_from_body = cardano_sub_transaction_body_get_reference_inputs(sub_transaction_body);

  EXPECT_EQ(reference_inputs_from_body, reference_inputs);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_transaction_input_set_unref(&reference_inputs);
  cardano_transaction_input_set_unref(&reference_inputs_from_body);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_body_get_voting_procedures, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  cardano_voting_procedures_t* voting_procedures = cardano_sub_transaction_body_get_voting_procedures(nullptr);

  // Assert
  EXPECT_EQ(voting_procedures, nullptr);
}

TEST(cardano_sub_transaction_body_get_voting_procedures, returnsVotingProcedures)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_voting_procedures_t* voting_procedures = cardano_sub_transaction_body_get_voting_procedures(sub_transaction_body);

  // Assert
  EXPECT_NE(voting_procedures, nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_voting_procedures_unref(&voting_procedures);
}

TEST(cardano_sub_transaction_body_set_voting_procedures, canSetVotingProceduresToNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_voting_procedures(sub_transaction_body, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_get_voting_procedures(sub_transaction_body), nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_voting_procedures, returnsErrorIfSubTransactionBodyNull)
{
  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_voting_procedures(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_set_voting_procedures, canSetVotingProcedures)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  cardano_voting_procedures_t* voting_procedures = NULL;
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex(VOTING_PROCEDURES_CBOR, strlen(VOTING_PROCEDURES_CBOR));

  EXPECT_THAT(cardano_voting_procedures_from_cbor(reader, &voting_procedures), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_voting_procedures(sub_transaction_body, voting_procedures);

  cardano_voting_procedures_t* voting_procedures_from_body = cardano_sub_transaction_body_get_voting_procedures(sub_transaction_body);

  EXPECT_EQ(voting_procedures_from_body, voting_procedures);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_voting_procedures_unref(&voting_procedures);
  cardano_voting_procedures_unref(&voting_procedures_from_body);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_body_get_proposal_procedures, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  cardano_proposal_procedure_set_t* proposal_procedures = cardano_sub_transaction_body_get_proposal_procedures(nullptr);

  // Assert
  EXPECT_EQ(proposal_procedures, nullptr);
}

TEST(cardano_sub_transaction_body_get_proposal_procedures, returnsProposalProcedures)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_proposal_procedure_set_t* proposal_procedures = cardano_sub_transaction_body_get_proposal_procedures(sub_transaction_body);

  // Assert
  EXPECT_NE(proposal_procedures, nullptr);
  EXPECT_EQ(cardano_proposal_procedure_set_get_length(proposal_procedures), 1);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_proposal_procedure_set_unref(&proposal_procedures);
}

TEST(cardano_sub_transaction_body_set_proposal_procedures, canSetProposalProceduresToNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_proposal_procedures(sub_transaction_body, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_get_proposal_procedures(sub_transaction_body), nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_proposal_procedures, returnsErrorIfSubTransactionBodyNull)
{
  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_proposal_procedures(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_set_proposal_procedures, canSetProposalProcedures)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  cardano_proposal_procedure_set_t* proposal_procedures = NULL;
  cardano_cbor_reader_t*            reader              = cardano_cbor_reader_from_hex(PROPOSAL_PROCEDURES_CBOR, strlen(PROPOSAL_PROCEDURES_CBOR));

  EXPECT_THAT(cardano_proposal_procedure_set_from_cbor(reader, &proposal_procedures), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_proposal_procedures(sub_transaction_body, proposal_procedures);

  cardano_proposal_procedure_set_t* proposal_procedures_from_body = cardano_sub_transaction_body_get_proposal_procedures(sub_transaction_body);

  EXPECT_EQ(proposal_procedures_from_body, proposal_procedures);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_proposal_procedure_set_unref(&proposal_procedures);
  cardano_proposal_procedure_set_unref(&proposal_procedures_from_body);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_body_get_treasury_value, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  const uint64_t* treasury_value = cardano_sub_transaction_body_get_treasury_value(nullptr);

  // Assert
  EXPECT_EQ(treasury_value, nullptr);
}

TEST(cardano_sub_transaction_body_get_treasury_value, returnsTreasuryValue)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  const uint64_t* treasury_value = cardano_sub_transaction_body_get_treasury_value(sub_transaction_body);

  // Assert
  ASSERT_NE(treasury_value, nullptr);
  EXPECT_EQ(*treasury_value, 2000);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_treasury_value, returnsErrorIfSubTransactionBodyNull)
{
  // Act
  uint64_t        treasury_value = 100;
  cardano_error_t result         = cardano_sub_transaction_body_set_treasury_value(nullptr, &treasury_value);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_set_treasury_value, canSetTreasuryValueToNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_treasury_value(sub_transaction_body, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_get_treasury_value(sub_transaction_body), nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_treasury_value, canSetTreasuryValue)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  uint64_t treasury_value = 5000;

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_treasury_value(sub_transaction_body, &treasury_value);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  const uint64_t* treasury_value_from_body = cardano_sub_transaction_body_get_treasury_value(sub_transaction_body);

  ASSERT_NE(treasury_value_from_body, nullptr);
  EXPECT_EQ(*treasury_value_from_body, treasury_value);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_get_donation, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  const uint64_t* donation = cardano_sub_transaction_body_get_donation(nullptr);

  // Assert
  EXPECT_EQ(donation, nullptr);
}

TEST(cardano_sub_transaction_body_get_donation, returnsDonation)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  const uint64_t* donation = cardano_sub_transaction_body_get_donation(sub_transaction_body);

  // Assert
  ASSERT_NE(donation, nullptr);
  EXPECT_EQ(*donation, 1000);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_donation, returnsErrorIfSubTransactionBodyNull)
{
  // Act
  uint64_t        donation = 100;
  cardano_error_t result   = cardano_sub_transaction_body_set_donation(nullptr, &donation);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_set_donation, canSetDonationToNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_donation(sub_transaction_body, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_get_donation(sub_transaction_body), nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_donation, canSetDonation)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  uint64_t donation = 3000;

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_donation(sub_transaction_body, &donation);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  const uint64_t* donation_from_body = cardano_sub_transaction_body_get_donation(sub_transaction_body);

  ASSERT_NE(donation_from_body, nullptr);
  EXPECT_EQ(*donation_from_body, donation);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_get_required_top_level_guards, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  cardano_required_guards_map_t* required_top_level_guards = cardano_sub_transaction_body_get_required_top_level_guards(nullptr);

  // Assert
  EXPECT_EQ(required_top_level_guards, nullptr);
}

TEST(cardano_sub_transaction_body_get_required_top_level_guards, returnsRequiredTopLevelGuards)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_required_guards_map_t* required_top_level_guards = cardano_sub_transaction_body_get_required_top_level_guards(sub_transaction_body);

  // Assert
  EXPECT_NE(required_top_level_guards, nullptr);
  EXPECT_EQ(cardano_required_guards_map_get_length(required_top_level_guards), 2);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_required_guards_map_unref(&required_top_level_guards);
}

TEST(cardano_sub_transaction_body_set_required_top_level_guards, canSetRequiredTopLevelGuardsToNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_required_top_level_guards(sub_transaction_body, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_get_required_top_level_guards(sub_transaction_body), nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_required_top_level_guards, returnsErrorIfSubTransactionBodyNull)
{
  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_required_top_level_guards(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_set_required_top_level_guards, canSetRequiredTopLevelGuards)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  cardano_required_guards_map_t* required_top_level_guards = NULL;
  cardano_cbor_reader_t*         reader                    = cardano_cbor_reader_from_hex(REQUIRED_GUARDS_CBOR, strlen(REQUIRED_GUARDS_CBOR));

  EXPECT_THAT(cardano_required_guards_map_from_cbor(reader, &required_top_level_guards), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_required_top_level_guards(sub_transaction_body, required_top_level_guards);

  cardano_required_guards_map_t* required_top_level_guards_from_body = cardano_sub_transaction_body_get_required_top_level_guards(sub_transaction_body);

  EXPECT_EQ(required_top_level_guards_from_body, required_top_level_guards);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_required_guards_map_unref(&required_top_level_guards);
  cardano_required_guards_map_unref(&required_top_level_guards_from_body);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_body_get_direct_deposits, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  cardano_direct_deposit_map_t* direct_deposits = cardano_sub_transaction_body_get_direct_deposits(nullptr);

  // Assert
  EXPECT_EQ(direct_deposits, nullptr);
}

TEST(cardano_sub_transaction_body_get_direct_deposits, returnsDirectDeposits)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_direct_deposit_map_t* direct_deposits = cardano_sub_transaction_body_get_direct_deposits(sub_transaction_body);

  // Assert
  EXPECT_NE(direct_deposits, nullptr);
  EXPECT_EQ(cardano_direct_deposit_map_get_length(direct_deposits), 1);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_direct_deposit_map_unref(&direct_deposits);
}

TEST(cardano_sub_transaction_body_set_direct_deposits, canSetDirectDepositsToNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_direct_deposits(sub_transaction_body, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_get_direct_deposits(sub_transaction_body), nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_direct_deposits, returnsErrorIfSubTransactionBodyNull)
{
  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_direct_deposits(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_set_direct_deposits, canSetDirectDeposits)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  cardano_direct_deposit_map_t* direct_deposits = NULL;
  cardano_cbor_reader_t*        reader          = cardano_cbor_reader_from_hex(DIRECT_DEPOSITS_CBOR, strlen(DIRECT_DEPOSITS_CBOR));

  EXPECT_THAT(cardano_direct_deposit_map_from_cbor(reader, &direct_deposits), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_direct_deposits(sub_transaction_body, direct_deposits);

  cardano_direct_deposit_map_t* direct_deposits_from_body = cardano_sub_transaction_body_get_direct_deposits(sub_transaction_body);

  EXPECT_EQ(direct_deposits_from_body, direct_deposits);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_direct_deposit_map_unref(&direct_deposits);
  cardano_direct_deposit_map_unref(&direct_deposits_from_body);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_body_get_account_balance_intervals, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  cardano_account_balance_intervals_map_t* account_balance_intervals = cardano_sub_transaction_body_get_account_balance_intervals(nullptr);

  // Assert
  EXPECT_EQ(account_balance_intervals, nullptr);
}

TEST(cardano_sub_transaction_body_get_account_balance_intervals, returnsAccountBalanceIntervals)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_account_balance_intervals_map_t* account_balance_intervals = cardano_sub_transaction_body_get_account_balance_intervals(sub_transaction_body);

  // Assert
  EXPECT_NE(account_balance_intervals, nullptr);
  EXPECT_EQ(cardano_account_balance_intervals_map_get_length(account_balance_intervals), 1);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals);
}

TEST(cardano_sub_transaction_body_set_account_balance_intervals, canSetAccountBalanceIntervalsToNull)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_account_balance_intervals(sub_transaction_body, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_sub_transaction_body_get_account_balance_intervals(sub_transaction_body), nullptr);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
}

TEST(cardano_sub_transaction_body_set_account_balance_intervals, returnsErrorIfSubTransactionBodyNull)
{
  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_account_balance_intervals(nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_sub_transaction_body_set_account_balance_intervals, canSetAccountBalanceIntervals)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = new_default_sub_transaction_body();
  EXPECT_NE(sub_transaction_body, nullptr);

  cardano_account_balance_intervals_map_t* account_balance_intervals = NULL;
  cardano_cbor_reader_t*                   reader                    = cardano_cbor_reader_from_hex(BALANCE_INTERVALS_CBOR, strlen(BALANCE_INTERVALS_CBOR));

  EXPECT_THAT(cardano_account_balance_intervals_map_from_cbor(reader, &account_balance_intervals), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_sub_transaction_body_set_account_balance_intervals(sub_transaction_body, account_balance_intervals);

  cardano_account_balance_intervals_map_t* account_balance_intervals_from_body = cardano_sub_transaction_body_get_account_balance_intervals(sub_transaction_body);

  EXPECT_EQ(account_balance_intervals_from_body, account_balance_intervals);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals);
  cardano_account_balance_intervals_map_unref(&account_balance_intervals_from_body);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_sub_transaction_body_get_hash, returnsNullIfSubTransactionBodyIsNull)
{
  // Act
  const cardano_blake2b_hash_t* hash = cardano_sub_transaction_body_get_hash(nullptr);

  // Assert
  EXPECT_EQ(hash, nullptr);
}

TEST(cardano_sub_transaction_body_get_hash, returnsHash)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body1 = NULL;
  cardano_sub_transaction_body_t* sub_transaction_body2 = NULL;

  cardano_cbor_reader_t* reader1 = cardano_cbor_reader_from_hex(MINIMAL_CBOR, strlen(MINIMAL_CBOR));
  cardano_cbor_reader_t* reader2 = cardano_cbor_reader_from_hex(MAXIMAL_CBOR, strlen(MAXIMAL_CBOR));

  EXPECT_THAT(cardano_sub_transaction_body_from_cbor(reader1, &sub_transaction_body1), CARDANO_SUCCESS);
  EXPECT_THAT(cardano_sub_transaction_body_from_cbor(reader2, &sub_transaction_body2), CARDANO_SUCCESS);

  // Act
  cardano_blake2b_hash_t* hash1 = cardano_sub_transaction_body_get_hash(sub_transaction_body1);
  cardano_blake2b_hash_t* hash2 = cardano_sub_transaction_body_get_hash(sub_transaction_body2);

  size_t hex_size1 = cardano_blake2b_hash_get_hex_size(hash1);
  char*  hex1      = (char*)malloc(hex_size1);

  size_t hex_size2 = cardano_blake2b_hash_get_hex_size(hash2);
  char*  hex2      = (char*)malloc(hex_size2);

  EXPECT_THAT(cardano_blake2b_hash_to_hex(hash1, hex1, hex_size1), CARDANO_SUCCESS);
  EXPECT_THAT(cardano_blake2b_hash_to_hex(hash2, hex2, hex_size2), CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(hex1, MINIMAL_CBOR_HASH);
  EXPECT_STREQ(hex2, MAXIMAL_CBOR_HASH);

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body1);
  cardano_sub_transaction_body_unref(&sub_transaction_body2);
  cardano_blake2b_hash_unref(&hash1);
  cardano_blake2b_hash_unref(&hash2);
  cardano_cbor_reader_unref(&reader1);
  cardano_cbor_reader_unref(&reader2);
  free(hex1);
  free(hex2);
}

TEST(cardano_sub_transaction_body_clear_cbor_cache, doesntCrashIfSubTransactionBodyIsNull)
{
  // Act
  cardano_sub_transaction_body_clear_cbor_cache(nullptr);

  // Assert
  EXPECT_TRUE(true);
}

TEST(cardano_sub_transaction_body_clear_cbor_cache, clearsTheCache)
{
  // Arrange
  cardano_sub_transaction_body_t* sub_transaction_body = NULL;
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(MINIMAL_CBOR, strlen(MINIMAL_CBOR));

  EXPECT_THAT(cardano_sub_transaction_body_from_cbor(reader, &sub_transaction_body), CARDANO_SUCCESS);

  uint64_t slot = 1000;
  EXPECT_THAT(cardano_sub_transaction_body_set_invalid_after(sub_transaction_body, &slot), CARDANO_SUCCESS);

  // Act
  // While the cache is present the original CBOR is preserved
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  EXPECT_THAT(cardano_sub_transaction_body_to_cbor(sub_transaction_body, writer), CARDANO_SUCCESS);

  size_t cbor_size = cardano_cbor_writer_get_hex_size(writer);
  char*  cbor      = (char*)malloc(cbor_size);

  EXPECT_THAT(cardano_cbor_writer_encode_hex(writer, cbor, cbor_size), CARDANO_SUCCESS);
  EXPECT_STREQ(cbor, MINIMAL_CBOR);

  // Clear the cache
  cardano_sub_transaction_body_clear_cbor_cache(sub_transaction_body);

  // Encode to CBOR and compare
  cardano_cbor_writer_t* writer2 = cardano_cbor_writer_new();

  EXPECT_THAT(cardano_sub_transaction_body_to_cbor(sub_transaction_body, writer2), CARDANO_SUCCESS);

  size_t cbor_size2 = cardano_cbor_writer_get_hex_size(writer2);
  char*  cbor2      = (char*)malloc(cbor_size2);

  EXPECT_THAT(cardano_cbor_writer_encode_hex(writer2, cbor2, cbor_size2), CARDANO_SUCCESS);

  EXPECT_STREQ(cbor2, "a300d90102800180031903e8");

  // Cleanup
  cardano_sub_transaction_body_unref(&sub_transaction_body);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  cardano_cbor_writer_unref(&writer2);
  free(cbor);
  free(cbor2);
}
