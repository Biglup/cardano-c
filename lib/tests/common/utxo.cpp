/**
 * \file utxo.cpp
 *
 * \author angel.castillo
 * \date   Sep 24, 2024
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

#include "tests/allocators_helpers.h"
#include <cardano/cbor/cbor_reader.h>
#include <cardano/common/utxo.h>
#include <cardano/transaction_body/transaction_input.h>
#include <cardano/transaction_body/transaction_output.h>

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                  = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c410a";
static const char* CBOR_DIFFERENT_INPUT  = "82825820bb217abaca60fc0ca78c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c410a";
static const char* CBOR_DIFFERENT_OUTPUT = "82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001a200583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c420a";
static const char* INPUT_CBOR            = "825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001";
static const char* OUTPUT_CBOR           = "82583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c410a";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the utxo.
 * @return A new instance of the utxo.
 */
static cardano_utxo_t*
new_default_utxo(const char* cbor = CBOR)
{
  cardano_utxo_t*        utxo   = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t        result = cardano_utxo_from_cbor(reader, &utxo);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return utxo;
};

/**
 * Creates a new default instance of the transaction_input.
 *
 * @return A new instance of the transaction_input.
 */
static cardano_transaction_input_t*
new_default_input()
{
  cardano_transaction_input_t* input  = NULL;
  cardano_cbor_reader_t*       reader = cardano_cbor_reader_from_hex(INPUT_CBOR, strlen(INPUT_CBOR));
  cardano_error_t              result = cardano_transaction_input_from_cbor(reader, &input);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return input;
}

/**
 * Creates a new default instance of the transaction_output.
 *
 * @return A new instance of the transaction_output.
 */
static cardano_transaction_output_t*
new_default_output()
{
  cardano_transaction_output_t* output = NULL;
  cardano_cbor_reader_t*        reader = cardano_cbor_reader_from_hex(OUTPUT_CBOR, strlen(OUTPUT_CBOR));
  cardano_error_t               result = cardano_transaction_output_from_cbor(reader, &output);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return output;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_utxo_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_utxo_t* utxo = new_default_utxo();
  EXPECT_NE(utxo, nullptr);

  // Act
  cardano_utxo_ref(utxo);

  // Assert
  EXPECT_THAT(utxo, testing::Not((cardano_utxo_t*)nullptr));
  EXPECT_EQ(cardano_utxo_refcount(utxo), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&utxo);
}

TEST(cardano_utxo_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_utxo_ref(nullptr);
}

TEST(cardano_utxo_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_utxo_t* utxo = nullptr;

  // Act
  cardano_utxo_unref(&utxo);
}

TEST(cardano_utxo_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_utxo_unref((cardano_utxo_t**)nullptr);
}

TEST(cardano_utxo_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_utxo_t* utxo = new_default_utxo();
  EXPECT_NE(utxo, nullptr);

  // Act
  cardano_utxo_ref(utxo);
  size_t ref_count = cardano_utxo_refcount(utxo);

  cardano_utxo_unref(&utxo);
  size_t updated_ref_count = cardano_utxo_refcount(utxo);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_utxo_unref(&utxo);
}

TEST(cardano_utxo_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_utxo_t* utxo = new_default_utxo();
  EXPECT_NE(utxo, nullptr);

  // Act
  cardano_utxo_ref(utxo);
  size_t ref_count = cardano_utxo_refcount(utxo);

  cardano_utxo_unref(&utxo);
  size_t updated_ref_count = cardano_utxo_refcount(utxo);

  cardano_utxo_unref(&utxo);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(utxo, (cardano_utxo_t*)nullptr);

  // Cleanup
  cardano_utxo_unref(&utxo);
}

TEST(cardano_utxo_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_utxo_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_utxo_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_utxo_t* utxo    = nullptr;
  const char*     message = "This is a test message";

  // Act
  cardano_utxo_set_last_error(utxo, message);

  // Assert
  EXPECT_STREQ(cardano_utxo_get_last_error(utxo), "Object is NULL.");
}

TEST(cardano_utxo_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_utxo_t* utxo = new_default_utxo();
  EXPECT_NE(utxo, nullptr);

  const char* message = nullptr;

  // Act
  cardano_utxo_set_last_error(utxo, message);

  // Assert
  EXPECT_STREQ(cardano_utxo_get_last_error(utxo), "");

  // Cleanup
  cardano_utxo_unref(&utxo);
}

TEST(cardano_utxo_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_utxo_t* utxo = NULL;

  // Act
  cardano_error_t result = cardano_utxo_from_cbor(nullptr, &utxo);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_utxo_from_cbor, returnsErrorIfUtxoIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_utxo_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_utxo_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_utxo_t*        cert   = new_default_utxo();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_utxo_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_utxo_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_utxo_to_cbor, returnsErrorIfUtxoIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_utxo_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_utxo_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_utxo_to_cbor((cardano_utxo_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

// Utxo specific tests

TEST(cardano_utxo_new, canCreateNewInstance)
{
  // Act
  cardano_transaction_input_t*  input  = new_default_input();
  cardano_transaction_output_t* output = new_default_output();

  cardano_utxo_t* utxo = NULL;

  cardano_error_t result = cardano_utxo_new(input, output, &utxo);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(utxo, nullptr);

  // Cleanup
  cardano_utxo_unref(&utxo);
  cardano_transaction_input_unref(&input);
  cardano_transaction_output_unref(&output);
}

TEST(cardano_utxo_new, returnsErrorIfFirstArgIsNull)
{
  // Act
  cardano_utxo_t* utxo = NULL;

  cardano_error_t result = cardano_utxo_new(nullptr, nullptr, &utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_utxo_new, returnsErrorIfSecondArgIsNull)
{
  // Act
  cardano_utxo_t* utxo = NULL;

  cardano_error_t result = cardano_utxo_new((cardano_transaction_input_t*)"", nullptr, &utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_utxo_new, returnsErrorIfUtxoIsNull)
{
  // Act

  cardano_error_t result = cardano_utxo_new((cardano_transaction_input_t*)"", (cardano_transaction_output_t*)"", nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
}

TEST(cardano_utxo_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_transaction_input_t*  input  = new_default_input();
  cardano_transaction_output_t* output = new_default_output();

  // Act
  cardano_utxo_t* utxo = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_utxo_new(input, output, &utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_transaction_input_unref(&input);
  cardano_transaction_output_unref(&output);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_utxo_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_utxo_t*        utxo   = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_utxo_from_cbor(reader, &utxo);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_utxo_unref(&utxo);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_utxo_from_cbor, returnsErrorIfInvalidInput)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("822ef", strlen("82ef"));
  cardano_utxo_t*        utxo   = NULL;

  // Act
  cardano_error_t result = cardano_utxo_from_cbor(reader, &utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_utxo_from_cbor, returnsErrorIfInvalidOutput)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e001ef583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c410a", strlen("82825820bb217abaca60fc0ca68c1555eca6a96d2478547818ae76ce6836133f3cc546e00182583900287a7e37219128cfb05322626daa8b19d1ad37c6779d21853f7b94177c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8821af0078c21a2581c1ec85dcee27f2d90ec1f9a1e4ce74a667dc9be8b184463223f9c9601a14350584c05581c659f2917fb63f12b33667463ee575eeac1845bbc736b9c0bbc40ba82a14454534c410a"));
  cardano_utxo_t*        utxo   = NULL;

  // Act
  cardano_error_t result = cardano_utxo_from_cbor(reader, &utxo);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

// Getters and Setters

TEST(cardano_utxo_get_input, canGetInput)
{
  // Arrange
  cardano_utxo_t*              utxo  = new_default_utxo();
  cardano_transaction_input_t* input = new_default_input();

  EXPECT_EQ(cardano_utxo_set_input(utxo, input), CARDANO_SUCCESS);

  // Act
  cardano_transaction_input_t* input2 = cardano_utxo_get_input(utxo);

  // Assert
  EXPECT_NE(input2, nullptr);

  // Cleanup
  cardano_utxo_unref(&utxo);
  cardano_transaction_input_unref(&input);
  cardano_transaction_input_unref(&input2);
}

TEST(cardano_utxo_get_input, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_transaction_input_t* input = cardano_utxo_get_input(nullptr);

  // Assert
  EXPECT_EQ(input, nullptr);
}

TEST(cardano_utxo_set_input, canSetInput)
{
  // Arrange
  cardano_utxo_t*              utxo  = new_default_utxo();
  cardano_transaction_input_t* input = new_default_input();

  // Act
  cardano_error_t result = cardano_utxo_set_input(utxo, input);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_utxo_unref(&utxo);
  cardano_transaction_input_unref(&input);
}

TEST(cardano_utxo_set_input, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_transaction_input_t* input = new_default_input();

  // Act
  cardano_error_t result = cardano_utxo_set_input(nullptr, input);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_input_unref(&input);
}

TEST(cardano_utxo_set_input, returnsErrorIfInputIsNull)
{
  // Arrange
  cardano_utxo_t* utxo = new_default_utxo();

  // Act
  cardano_error_t result = cardano_utxo_set_input(utxo, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_utxo_unref(&utxo);
}

TEST(cardano_utxo_get_output, canGetOutput)
{
  // Arrange
  cardano_utxo_t*               utxo   = new_default_utxo();
  cardano_transaction_output_t* output = new_default_output();

  EXPECT_EQ(cardano_utxo_set_output(utxo, output), CARDANO_SUCCESS);

  // Act
  cardano_transaction_output_t* output2 = cardano_utxo_get_output(utxo);

  // Assert
  EXPECT_NE(output2, nullptr);

  // Cleanup
  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&output);
  cardano_transaction_output_unref(&output2);
}

TEST(cardano_utxo_get_output, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_transaction_output_t* output = cardano_utxo_get_output(nullptr);

  // Assert
  EXPECT_EQ(output, nullptr);
}

TEST(cardano_utxo_set_output, canSetOutput)
{
  // Arrange
  cardano_utxo_t*               utxo   = new_default_utxo();
  cardano_transaction_output_t* output = new_default_output();

  // Act
  cardano_error_t result = cardano_utxo_set_output(utxo, output);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_utxo_unref(&utxo);
  cardano_transaction_output_unref(&output);
}

TEST(cardano_utxo_set_output, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_transaction_output_t* output = new_default_output();

  // Act
  cardano_error_t result = cardano_utxo_set_output(nullptr, output);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_output_unref(&output);
}

TEST(cardano_utxo_set_output, returnsErrorIfOutputIsNull)
{
  // Arrange
  cardano_utxo_t* utxo = new_default_utxo();

  // Act
  cardano_error_t result = cardano_utxo_set_output(utxo, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_utxo_unref(&utxo);
}

TEST(cardano_utxo_equals, returnsTrueIfEqual)
{
  // Arrange
  cardano_utxo_t* utxo  = new_default_utxo(CBOR);
  cardano_utxo_t* utxo2 = new_default_utxo(CBOR);

  // Act
  bool result = cardano_utxo_equals(utxo, utxo2);

  // Assert
  EXPECT_TRUE(result);

  // Cleanup
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&utxo2);
}

TEST(cardano_utxo_equals, returnsFalseIfDifferent)
{
  // Arrange
  cardano_utxo_t* utxo  = new_default_utxo(CBOR);
  cardano_utxo_t* utxo2 = new_default_utxo(CBOR_DIFFERENT_INPUT);

  // Act
  bool result = cardano_utxo_equals(utxo, utxo2);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&utxo2);
}

TEST(cardano_utxo_equals, returnsFalseIfDifferent2)
{
  // Arrange
  cardano_utxo_t* utxo  = new_default_utxo(CBOR);
  cardano_utxo_t* utxo2 = new_default_utxo(CBOR_DIFFERENT_OUTPUT);

  // Act
  bool result = cardano_utxo_equals(utxo, utxo2);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_utxo_unref(&utxo);
  cardano_utxo_unref(&utxo2);
}

TEST(cardano_utxo_equals, returnsTrueIfBothNull)
{
  // Arrange
  cardano_utxo_t* utxo = NULL;

  // Act
  bool result = cardano_utxo_equals(utxo, utxo);

  // Assert
  EXPECT_TRUE(result);
}

TEST(cardano_utxo_equals, returnsFalseIfOneIsNull)
{
  // Arrange
  cardano_utxo_t* utxo = new_default_utxo();

  // Act
  bool result = cardano_utxo_equals(utxo, nullptr);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_utxo_unref(&utxo);
}

TEST(cardano_utxo_equals, returnsFalseIfOneIsNull2)
{
  // Arrange
  cardano_utxo_t* utxo = new_default_utxo();

  // Act
  bool result = cardano_utxo_equals(nullptr, utxo);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_utxo_unref(&utxo);
}
