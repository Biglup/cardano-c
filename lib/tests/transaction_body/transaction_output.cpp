/**
 * \file transaction_output.cpp
 *
 * \author angel.castillo
 * \date   Sep 16, 2024
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
#include <cardano/transaction_body/transaction_output.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR                                  = "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011";
static const char* CBOR_DIFFERENT_ADDRESS                = "a400583900537ba48a023f0a3c66e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011";
static const char* CBOR_DIFFERENT_VALUE                  = "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4340a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011";
static const char* CBOR_DIFFERENT_SCRIPT                 = "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200122211";
static const char* LEGACY_OUTPUT_CBOR                    = "83583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a58200000000000000000000000000000000000000000000000000000000000000000";
static const char* LEGACY_OUTPUT_NO_DATUM_CBOR           = "82583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a";
static const char* BABBAGE_INLINE_DATUM_CBOR             = "a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff";
static const char* BABBAGE_DATUM_HASH_CBOR               = "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a0282005820000000000000000000000000000000000000000000000000000000000000000003d8185182014e4d01000033222220051200120011";
static const char* BABBAGE_REF_SCRIPT_CBOR               = "a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a03d8185182014e4d01000033222220051200120011";
static const char* BABBAGE_NO_OPTIONAL_FIELD_SCRIPT_CBOR = "82583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a";
static const char* MARY_OUTPUT_POINTER_CBOR              = "825826412813b99a80cfb4024374bd0f502959485aa56e0648564ff805f2e51bbcd9819561bddc66141a02faf080";
static const char* ADDRESS_IN_OUTPUTS                    = "addr_test1qpfhhfy2qgls50r9u4yh0l7z67xpg0a5rrhkmvzcuqrd0znuzcjqw982pcftgx53fu5527z2cj2tkx2h8ux2vxsg475q9gw0lz";
static const char* VALUE_CBOR                            = "821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a";
static const char* DATUM_CBOR                            = "8201d81849d8799f0102030405ff";
static const char* DATUM_HASH_CBOR                       = "820058200000000000000000000000000000000000000000000000000000000000000000";
static const char* SCRIPT_REF_CBOR                       = "82014E4D01000033222220051200120011";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the output.
 * @return A new instance of the output.
 */
static cardano_transaction_output_t*
new_default_output(const char* cbor)
{
  cardano_transaction_output_t* transaction_output = NULL;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));
  cardano_error_t               result             = cardano_transaction_output_from_cbor(reader, &transaction_output);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return transaction_output;
};

/**
 * Creates a new default instance of the address.
 *
 * @return A new instance of the address.
 */
static cardano_address_t*
new_default_address()
{
  cardano_address_t* address = NULL;
  cardano_error_t    result  = cardano_address_from_string(ADDRESS_IN_OUTPUTS, strlen(ADDRESS_IN_OUTPUTS), &address);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  return address;
}

/**
 * Creates a new default instance of the value.
 *
 * @return A new instance of the value.
 */
static cardano_value_t*
new_default_value()
{
  cardano_value_t*       value  = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(VALUE_CBOR, strlen(VALUE_CBOR));
  cardano_error_t        result = cardano_value_from_cbor(reader, &value);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return value;
}

/**
 * Creates a new default instance of the datum.
 *
 * @return A new instance of the datum.
 */
static cardano_datum_t*
new_default_datum()
{
  cardano_datum_t*       datum  = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DATUM_CBOR, strlen(DATUM_CBOR));
  cardano_error_t        result = cardano_datum_from_cbor(reader, &datum);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return datum;
}

/**
 * Creates a new default instance of the hash.
 *
 * @return A new instance of the hash.
 */
static cardano_datum_t*
new_default_datum_hash()
{
  cardano_datum_t*       datum  = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DATUM_HASH_CBOR, strlen(DATUM_HASH_CBOR));
  cardano_error_t        result = cardano_datum_from_cbor(reader, &datum);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return datum;
}

/**
 * Creates a new default instance of the script ref.
 *
 * @return A new instance of the script ref.
 */
static cardano_script_t*
new_default_script_ref()
{
  cardano_script_t*      script = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(SCRIPT_REF_CBOR, strlen(SCRIPT_REF_CBOR));
  cardano_error_t        result = cardano_script_from_cbor(reader, &script);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return script;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_transaction_output_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = new_default_output(CBOR);
  EXPECT_NE(transaction_output, nullptr);

  // Act
  cardano_transaction_output_ref(transaction_output);

  // Assert
  EXPECT_THAT(transaction_output, testing::Not((cardano_transaction_output_t*)nullptr));
  EXPECT_EQ(cardano_transaction_output_refcount(transaction_output), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_transaction_output_unref(&transaction_output);
  cardano_transaction_output_unref(&transaction_output);
}

TEST(cardano_transaction_output_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_transaction_output_ref(nullptr);
}

TEST(cardano_transaction_output_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = nullptr;

  // Act
  cardano_transaction_output_unref(&transaction_output);
}

TEST(cardano_transaction_output_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_transaction_output_unref((cardano_transaction_output_t**)nullptr);
}

TEST(cardano_transaction_output_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = new_default_output(CBOR);
  EXPECT_NE(transaction_output, nullptr);

  // Act
  cardano_transaction_output_ref(transaction_output);
  size_t ref_count = cardano_transaction_output_refcount(transaction_output);

  cardano_transaction_output_unref(&transaction_output);
  size_t updated_ref_count = cardano_transaction_output_refcount(transaction_output);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
}

TEST(cardano_transaction_output_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = new_default_output(CBOR);
  EXPECT_NE(transaction_output, nullptr);

  // Act
  cardano_transaction_output_ref(transaction_output);
  size_t ref_count = cardano_transaction_output_refcount(transaction_output);

  cardano_transaction_output_unref(&transaction_output);
  size_t updated_ref_count = cardano_transaction_output_refcount(transaction_output);

  cardano_transaction_output_unref(&transaction_output);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(transaction_output, (cardano_transaction_output_t*)nullptr);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
}

TEST(cardano_transaction_output_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_transaction_output_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_transaction_output_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = nullptr;
  const char*                   message            = "This is a test message";

  // Act
  cardano_transaction_output_set_last_error(transaction_output, message);

  // Assert
  EXPECT_STREQ(cardano_transaction_output_get_last_error(transaction_output), "Object is NULL.");
}

TEST(cardano_transaction_output_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = new_default_output(CBOR);
  EXPECT_NE(transaction_output, nullptr);

  const char* message = nullptr;

  // Act
  cardano_transaction_output_set_last_error(transaction_output, message);

  // Assert
  EXPECT_STREQ(cardano_transaction_output_get_last_error(transaction_output), "");

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
}

TEST(cardano_transaction_output_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = NULL;

  // Act
  cardano_error_t result = cardano_transaction_output_from_cbor(nullptr, &transaction_output);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_transaction_output_from_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_transaction_output_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t*        writer = cardano_cbor_writer_new();
  cardano_transaction_output_t* output = new_default_output(CBOR);
  EXPECT_NE(output, nullptr);

  // Act
  cardano_error_t result = cardano_transaction_output_to_cbor(output, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_transaction_output_unref(&output);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_transaction_output_to_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_transaction_output_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_transaction_output_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_transaction_output_to_cbor((cardano_transaction_output_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

// Cert specific tests

TEST(cardano_transaction_output_new, canCreateNewInstance)
{
  // Act
  cardano_address_t* address = new_default_address();

  cardano_transaction_output_t* transaction_output = NULL;

  cardano_error_t result = cardano_transaction_output_new(address, 1, &transaction_output);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(transaction_output, nullptr);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_address_unref(&address);
}

TEST(cardano_transaction_output_new, returnsErrorIfFirstArgIsNull)
{
  // Act
  cardano_transaction_output_t* transaction_output = NULL;

  cardano_error_t result = cardano_transaction_output_new(nullptr, 0, &transaction_output);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_transaction_output_new, returnsErrorIfOutputIsNull)
{
  // Act

  cardano_error_t result = cardano_transaction_output_new((cardano_address_t*)"", 0, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
}

TEST(cardano_transaction_output_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_address_t* address = new_default_address();

  // Act
  cardano_transaction_output_t* transaction_output = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_transaction_output_new(address, 0, &transaction_output);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_address_unref(&address);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_transaction_output_new, returnsErrorIfMemoryAllocationFails2)
{
  // Arrange
  cardano_address_t* address = new_default_address();

  // Act
  cardano_transaction_output_t* transaction_output = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_error_t result = cardano_transaction_output_new(address, 0, &transaction_output);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_address_unref(&address);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_transaction_output_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = NULL;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &transaction_output);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_cbor_reader_unref(&reader);
}

// Getters and Setters

TEST(cardano_transaction_output_set_address, canSetAddress)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = new_default_output(CBOR);
  cardano_address_t*            address            = new_default_address();

  // Act
  cardano_error_t result = cardano_transaction_output_set_address(transaction_output, address);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_address_unref(&address);
}

TEST(cardano_transaction_output_set_address, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_address_t* address = new_default_address();

  // Act
  cardano_error_t result = cardano_transaction_output_set_address(nullptr, address);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_address_unref(&address);
}

TEST(cardano_transaction_output_set_address, returnsErrorIfAddressIsNull)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = new_default_output(CBOR);

  // Act
  cardano_error_t result = cardano_transaction_output_set_address(transaction_output, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
}

TEST(cardano_transaction_output_get_address, canGetAddress)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = new_default_output(CBOR);
  cardano_address_t*            address            = new_default_address();

  EXPECT_EQ(cardano_transaction_output_set_address(transaction_output, address), CARDANO_SUCCESS);

  // Act
  cardano_address_t* address2 = cardano_transaction_output_get_address(transaction_output);

  // Assert
  EXPECT_NE(address2, nullptr);
  EXPECT_STREQ(cardano_address_get_string(address), cardano_address_get_string(address2));

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_address_unref(&address);
  cardano_address_unref(&address2);
}

TEST(cardano_transaction_output_get_address, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_address_t* address = cardano_transaction_output_get_address(nullptr);

  // Assert
  EXPECT_EQ(address, nullptr);
}

TEST(cardano_transaction_output_set_value, canSetValue)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = new_default_output(CBOR);
  cardano_value_t*              value              = new_default_value();

  // Act
  cardano_error_t result = cardano_transaction_output_set_value(transaction_output, value);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_value_unref(&value);
}

TEST(cardano_transaction_output_set_value, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_value_t* value = new_default_value();

  // Act
  cardano_error_t result = cardano_transaction_output_set_value(nullptr, value);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_value_unref(&value);
}

TEST(cardano_transaction_output_get_value, canGetValue)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = new_default_output(CBOR);
  cardano_value_t*              value              = new_default_value();

  EXPECT_EQ(cardano_transaction_output_set_value(transaction_output, value), CARDANO_SUCCESS);

  // Act
  cardano_value_t* value2 = cardano_transaction_output_get_value(transaction_output);

  // Assert
  EXPECT_NE(value2, nullptr);
  EXPECT_EQ(cardano_value_equals(value, value2), true);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_value_unref(&value);
  cardano_value_unref(&value2);
}

TEST(cardano_transaction_output_get_value, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_value_t* value = cardano_transaction_output_get_value(nullptr);

  // Assert
  EXPECT_EQ(value, nullptr);
}

TEST(cardano_transaction_output_set_datum, canSetDatum)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = new_default_output(CBOR);
  cardano_datum_t*              datum              = new_default_datum();

  // Act
  cardano_error_t result = cardano_transaction_output_set_datum(transaction_output, datum);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_datum_unref(&datum);
}

TEST(cardano_transaction_output_set_datum, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_datum_t* datum = new_default_datum();

  // Act
  cardano_error_t result = cardano_transaction_output_set_datum(nullptr, datum);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_datum_unref(&datum);
}

TEST(cardano_transaction_output_get_datum, canGetDatum)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = new_default_output(BABBAGE_INLINE_DATUM_CBOR);
  cardano_datum_t*              datum              = new_default_datum();

  EXPECT_EQ(cardano_transaction_output_set_datum(transaction_output, datum), CARDANO_SUCCESS);

  // Act
  cardano_datum_t* datum2 = cardano_transaction_output_get_datum(transaction_output);

  // Assert
  EXPECT_NE(datum2, nullptr);
  EXPECT_EQ(datum, datum2);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_datum_unref(&datum);
  cardano_datum_unref(&datum2);
}

TEST(cardano_transaction_output_get_datum, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_datum_t* datum = cardano_transaction_output_get_datum(nullptr);

  // Assert
  EXPECT_EQ(datum, nullptr);
}

TEST(cardano_transaction_output_set_datum_hash, canSetDatumHash)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = new_default_output(BABBAGE_DATUM_HASH_CBOR);
  cardano_datum_t*              datum              = new_default_datum_hash();

  // Act
  cardano_error_t result = cardano_transaction_output_set_datum(transaction_output, datum);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_datum_unref(&datum);
}

TEST(cardano_transaction_output_set_datum_hash, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_datum_t* datum = new_default_datum_hash();

  // Act
  cardano_error_t result = cardano_transaction_output_set_datum(nullptr, datum);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_datum_unref(&datum);
}

TEST(cardano_transaction_output_get_datum_hash, canGetDatumHash)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = new_default_output(BABBAGE_DATUM_HASH_CBOR);
  cardano_datum_t*              datum              = new_default_datum_hash();

  EXPECT_EQ(cardano_transaction_output_set_datum(transaction_output, datum), CARDANO_SUCCESS);

  // Act
  cardano_datum_t* datum2 = cardano_transaction_output_get_datum(transaction_output);

  // Assert
  EXPECT_NE(datum2, nullptr);
  EXPECT_EQ(datum, datum2);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_datum_unref(&datum);
  cardano_datum_unref(&datum2);
}

TEST(cardano_transaction_output_get_datum_hash, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_datum_t* datum = cardano_transaction_output_get_datum(nullptr);

  // Assert
  EXPECT_EQ(datum, nullptr);
}

TEST(cardano_transaction_output_set_script, canSetScript)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = new_default_output(BABBAGE_REF_SCRIPT_CBOR);
  cardano_script_t*             script             = new_default_script_ref();

  // Act
  cardano_error_t result = cardano_transaction_output_set_script_ref(transaction_output, script);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_script_unref(&script);
}

TEST(cardano_transaction_output_set_script, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_script_t* script = new_default_script_ref();

  // Act
  cardano_error_t result = cardano_transaction_output_set_script_ref(nullptr, script);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_script_unref(&script);
}

TEST(cardano_transaction_output_get_script, canGetScript)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = new_default_output(BABBAGE_REF_SCRIPT_CBOR);
  cardano_script_t*             script             = new_default_script_ref();

  EXPECT_EQ(cardano_transaction_output_set_script_ref(transaction_output, script), CARDANO_SUCCESS);

  // Act
  cardano_script_t* script2 = cardano_transaction_output_get_script_ref(transaction_output);

  // Assert
  EXPECT_NE(script2, nullptr);
  EXPECT_EQ(script, script2);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_script_unref(&script);
  cardano_script_unref(&script2);
}

TEST(cardano_transaction_output_get_script, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_script_t* script = cardano_transaction_output_get_script_ref(nullptr);

  // Assert
  EXPECT_EQ(script, nullptr);
}

TEST(cardano_transaction_output_set_script, canSetScriptToNull)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = new_default_output(BABBAGE_REF_SCRIPT_CBOR);
  cardano_script_t*             script             = new_default_script_ref();

  EXPECT_EQ(cardano_transaction_output_set_script_ref(transaction_output, script), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_transaction_output_set_script_ref(transaction_output, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_script_unref(&script);
}

TEST(cardano_transaction_output_set_value, returnsErrorIfValueIsNull)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = new_default_output(CBOR);

  // Act
  cardano_error_t result = cardano_transaction_output_set_value(transaction_output, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
}

TEST(cardano_transaction_output_to_cbor, canDeserializeLegacyOutput)
{
  // Arrange
  cardano_cbor_writer_t*        writer = cardano_cbor_writer_new();
  cardano_transaction_output_t* output = new_default_output(LEGACY_OUTPUT_CBOR);
  EXPECT_NE(output, nullptr);

  // Act
  cardano_error_t result = cardano_transaction_output_to_cbor(output, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, "a300583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a02820058200000000000000000000000000000000000000000000000000000000000000000");

  // Cleanup
  cardano_transaction_output_unref(&output);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_transaction_output_to_cbor, canDeserializeLegacyOutput2)
{
  // Arrange
  cardano_cbor_writer_t*        writer = cardano_cbor_writer_new();
  cardano_transaction_output_t* output = new_default_output(LEGACY_OUTPUT_NO_DATUM_CBOR);
  EXPECT_NE(output, nullptr);

  // Act
  cardano_error_t result = cardano_transaction_output_to_cbor(output, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, "a200583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a");

  // Cleanup
  cardano_transaction_output_unref(&output);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_transaction_output_to_cbor, canDeserializeBabbageNoOptionalFields)
{
  // Arrange
  cardano_cbor_writer_t*        writer = cardano_cbor_writer_new();
  cardano_transaction_output_t* output = new_default_output(BABBAGE_NO_OPTIONAL_FIELD_SCRIPT_CBOR);
  EXPECT_NE(output, nullptr);

  // Act
  cardano_error_t result = cardano_transaction_output_to_cbor(output, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, "a200583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a");

  // Cleanup
  cardano_transaction_output_unref(&output);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_transaction_output_to_cbor, canDeserializeMayOutputWithPointerAddress)
{
  // Arrange
  cardano_cbor_writer_t*        writer = cardano_cbor_writer_new();
  cardano_transaction_output_t* output = new_default_output(MARY_OUTPUT_POINTER_CBOR);
  EXPECT_NE(output, nullptr);

  // Act
  cardano_error_t result = cardano_transaction_output_to_cbor(output, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, "a2005826412813b99a80cfb4024374bd0f502959485aa56e0648564ff805f2e51b8cd9819561bddc6614011a02faf080");

  cardano_address_t*     address = cardano_transaction_output_get_address(output);
  cardano_address_type_t type;

  EXPECT_EQ(cardano_address_get_type(address, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_ADDRESS_TYPE_POINTER_KEY);

  // Cleanup
  cardano_transaction_output_unref(&output);
  cardano_cbor_writer_unref(&writer);
  cardano_address_unref(&address);
  free(hex);
}

// a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011
TEST(cardano_transaction_output_from_cbor, returnsErrorIfInvalidMap)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = NULL;
  const char*                   cbor               = "ef00583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011";
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &transaction_output);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_from_cbor, returnsErrorIfInvalidKeyFormat)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = NULL;
  const char*                   cbor               = "a4ef583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011";
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &transaction_output);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_from_cbor, returnsErrorIfInvalidAddress)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = NULL;
  const char*                   cbor               = "a400ef3900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011";
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &transaction_output);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_from_cbor, returnsErrorIfInvalidAddress2)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = NULL;
  const char*                   cbor               = "a400583100537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011";
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &transaction_output);

  // Assert
  ASSERT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_from_cbor, returnsErrorIfInvalidValue)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = NULL;
  const char*                   cbor               = "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801ef1a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011";
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &transaction_output);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_from_cbor, returnsErrorIfInvalidDatum)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = NULL;
  const char*                   cbor               = "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a02ef01d81849d8799f0102030405ff03d8185182014e4d01000033222220051200120011";
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &transaction_output);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_from_cbor, returnsErrorIfScript)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = NULL;
  const char*                   cbor               = "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03ef185182014e4d01000033222220051200120011";
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &transaction_output);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_from_cbor, returnsErrorIfScriptTag)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = NULL;
  const char*                   cbor               = "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d8195182014e4d01000033222220051200120011";
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &transaction_output);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_VALUE);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_from_cbor, returnsErrorIfScriptBytes)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = NULL;
  const char*                   cbor               = "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d818ef82014e4d01000033222220051200120011";
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &transaction_output);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_from_cbor, returnsErrorIfScriptBytes2)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = NULL;
  const char*                   cbor               = "a400583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d81851ef014e4d01000033222220051200120011";
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &transaction_output);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_from_cbor, returnsErrorIfInvalidKey)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = NULL;
  const char*                   cbor               = "a409583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa801821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a028201d81849d8799f0102030405ff03d818ef82014e4d01000033222220051200120011";
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &transaction_output);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_MAP_KEY);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_from_cbor, returnsErrorIfInvalidAddressLegacyOutput)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = NULL;
  const char*                   cbor               = "83ef3900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a58200000000000000000000000000000000000000000000000000000000000000000";
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &transaction_output);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_from_cbor, returnsErrorIfInvalidAddress2LegacyOutput)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = NULL;
  const char*                   cbor               = "83583100537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a58200000000000000000000000000000000000000000000000000000000000000000";
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &transaction_output);

  // Assert
  ASSERT_EQ(result, CARDANO_INVALID_ADDRESS_FORMAT);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_from_cbor, returnsErrorIfInvalidValueLegacyOutput)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = NULL;
  const char*                   cbor               = "83583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8ef1a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420a58200000000000000000000000000000000000000000000000000000000000000000";
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &transaction_output);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_from_cbor, returnsErrorIfInvalidDatumLegacyOutput)
{
  // Arrange
  cardano_transaction_output_t* transaction_output = NULL;
  const char*                   cbor               = "83583900537ba48a023f0a3c65e54977ffc2d78c143fb418ef6db058e006d78a7c16240714ea0e12b41a914f2945784ac494bb19573f0ca61a08afa8821a000f4240a2581c00000000000000000000000000000000000000000000000000000000a3443031323218644433343536186344404142420a581c11111111111111111111111111111111111111111111111111111111a3443031323218644433343536186344404142420aef200000000000000000000000000000000000000000000000000000000000000000";
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_transaction_output_from_cbor(reader, &transaction_output);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_transaction_output_unref(&transaction_output);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_transaction_output_equals, returnsTrueIfEqual)
{
  // Arrange
  cardano_transaction_output_t* output  = new_default_output(CBOR);
  cardano_transaction_output_t* output2 = new_default_output(CBOR);

  // Act
  bool result = cardano_transaction_output_equals(output, output2);

  // Assert
  EXPECT_TRUE(result);

  // Cleanup
  cardano_transaction_output_unref(&output);
  cardano_transaction_output_unref(&output2);
}

TEST(cardano_transaction_output_equals, returnsFalseIfDifferent)
{
  // Arrange
  cardano_transaction_output_t* output  = new_default_output(CBOR);
  cardano_transaction_output_t* output2 = new_default_output(LEGACY_OUTPUT_NO_DATUM_CBOR);

  // Act
  bool result = cardano_transaction_output_equals(output, output2);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_transaction_output_unref(&output);
  cardano_transaction_output_unref(&output2);
}

TEST(cardano_transaction_output_equals, returnsFalseIfDifferent2)
{
  // Arrange
  cardano_transaction_output_t* output  = new_default_output(CBOR);
  cardano_transaction_output_t* output2 = new_default_output(CBOR_DIFFERENT_ADDRESS);

  // Act
  bool result = cardano_transaction_output_equals(output, output2);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_transaction_output_unref(&output);
  cardano_transaction_output_unref(&output2);
}

TEST(cardano_transaction_output_equals, returnsFalseIfDifferent3)
{
  // Arrange
  cardano_transaction_output_t* output  = new_default_output(CBOR);
  cardano_transaction_output_t* output2 = new_default_output(CBOR_DIFFERENT_VALUE);

  // Act
  bool result = cardano_transaction_output_equals(output, output2);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_transaction_output_unref(&output);
  cardano_transaction_output_unref(&output2);
}

TEST(cardano_transaction_output_equals, returnsFalseIfDifferent4)
{
  // Arrange
  cardano_transaction_output_t* output  = new_default_output(CBOR);
  cardano_transaction_output_t* output2 = new_default_output(CBOR_DIFFERENT_SCRIPT);

  // Act
  bool result = cardano_transaction_output_equals(output, output2);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_transaction_output_unref(&output);
  cardano_transaction_output_unref(&output2);
}

TEST(cardano_transaction_output_equals, returnsTrueIfBothNull)
{
  // Arrange
  cardano_transaction_output_t* output = NULL;

  // Act
  bool result = cardano_transaction_output_equals(output, output);

  // Assert
  EXPECT_TRUE(result);
}

TEST(cardano_transaction_output_equals, returnsFalseIfOneIsNull)
{
  // Arrange
  cardano_transaction_output_t* output = new_default_output(CBOR);

  // Act
  bool result = cardano_transaction_output_equals(output, nullptr);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_transaction_output_unref(&output);
}

TEST(cardano_transaction_output_equals, returnsFalseIfOneIsNull2)
{
  // Arrange
  cardano_transaction_output_t* output = new_default_output(CBOR);

  // Act
  bool result = cardano_transaction_output_equals(nullptr, output);

  // Assert
  EXPECT_FALSE(result);

  // Cleanup
  cardano_transaction_output_unref(&output);
}
