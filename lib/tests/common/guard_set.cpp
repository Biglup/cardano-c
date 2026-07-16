/**
 * \file guard_set.cpp
 *
 * \author angel.castillo
 * \date   Jul 16, 2026
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

#include <cardano/common/credential.h>
#include <cardano/common/guard_set.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* KEY_HASH_HEX    = "6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d39";
static const char* SCRIPT_HASH_HEX = "966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";

static const char* KEY_HASH_FORM_CBOR            = "81581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d39";
static const char* KEY_HASH_FORM_TAGGED_CBOR     = "d9010281581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d39";
static const char* CREDENTIAL_FORM_CBOR          = "828200581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d398201581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";
static const char* CREDENTIAL_FORM_TAGGED_CBOR   = "d90102828200581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d398201581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";
static const char* CREDENTIAL_FORM_REVERSED_CBOR = "828201581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c378200581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d39";
static const char* SINGLE_CREDENTIAL_CBOR        = "818200581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d39";

static const char* KEY_HASH_CREDENTIAL_CBOR    = "8200581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d39";
static const char* SCRIPT_HASH_CREDENTIAL_CBOR = "8201581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";

static const char* MIXED_KEY_HASH_FIRST_CBOR    = "82581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d398201581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";
static const char* MIXED_CREDENTIAL_FIRST_CBOR  = "828200581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d39581c966e394a544f242081e41d1965137b1bb412ac230d40ed5407821c37";
static const char* DUPLICATE_KEY_HASH_FORM_CBOR = "82581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d39581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d39";
static const char* DUPLICATE_CRED_FORM_CBOR     = "828200581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d398200581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d39";

/**
 * Creates a new default instance of the credential.
 * @return A new instance of the credential.
 */
static cardano_credential_t*
new_default_credential(const char* cbor)
{
  cardano_credential_t*  credential = nullptr;
  cardano_cbor_reader_t* reader     = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_credential_from_cbor(reader, &credential);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_credential_unref(&credential);
    return nullptr;
  }

  return credential;
}

/**
 * Decodes the given CBOR hex string into a guard set.
 * @return A new instance of the guard set.
 */
static cardano_guard_set_t*
new_default_guard_set(const char* cbor)
{
  cardano_guard_set_t*   guard_set = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  cardano_error_t error = cardano_guard_set_from_cbor(reader, &guard_set);

  cardano_cbor_reader_unref(&reader);

  if (error != CARDANO_SUCCESS)
  {
    cardano_guard_set_unref(&guard_set);
    return nullptr;
  }

  return guard_set;
}

/**
 * Encodes the given guard set to a CBOR hex string.
 * @return The CBOR hex string. The caller must free the returned string.
 */
static char*
encode_guard_set(cardano_guard_set_t* guard_set)
{
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  cardano_error_t error = cardano_guard_set_to_cbor(guard_set, writer);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t hex_size    = cardano_cbor_writer_get_hex_size(writer);
  char*        actual_cbor = (char*)malloc(hex_size);

  error = cardano_cbor_writer_encode_hex(writer, actual_cbor, hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_cbor_writer_unref(&writer);

  return actual_cbor;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_guard_set_new, canCreateGuardSet)
{
  // Arrange
  cardano_guard_set_t* guard_set = nullptr;

  // Act
  cardano_error_t error = cardano_guard_set_new(&guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(guard_set, testing::Not((cardano_guard_set_t*)nullptr));

  // Cleanup
  cardano_guard_set_unref(&guard_set);
}

TEST(cardano_guard_set_new, returnsErrorIfGuardSetIsNull)
{
  // Act
  cardano_error_t error = cardano_guard_set_new(nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_guard_set_new, returnsErrorIfMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_guard_set_t* guard_set = nullptr;

  // Act
  cardano_error_t error = cardano_guard_set_new(&guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(guard_set, (cardano_guard_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_guard_set_new, returnsErrorIfEventualMemoryAllocationFails)
{
  reset_allocators_run_count();
  cardano_set_allocators(fail_after_one_malloc, realloc, free);

  cardano_guard_set_t* guard_set = nullptr;

  // Act
  cardano_error_t error = cardano_guard_set_new(&guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(guard_set, (cardano_guard_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_guard_set_from_cbor, canDeserializeKeyHashForm)
{
  // Arrange
  cardano_guard_set_t*   guard_set = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex(KEY_HASH_FORM_CBOR, strlen(KEY_HASH_FORM_CBOR));

  // Act
  cardano_error_t error = cardano_guard_set_from_cbor(reader, &guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(guard_set, testing::Not((cardano_guard_set_t*)nullptr));
  EXPECT_EQ(cardano_guard_set_get_length(guard_set), 1);
  EXPECT_FALSE(cardano_guard_set_is_tagged(guard_set));

  cardano_credential_t* element = nullptr;
  EXPECT_EQ(cardano_guard_set_get(guard_set, 0, &element), CARDANO_SUCCESS);

  cardano_credential_type_t type = CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH;
  EXPECT_EQ(cardano_credential_get_type(element, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  EXPECT_STREQ(cardano_credential_get_hash_hex(element), KEY_HASH_HEX);

  // Cleanup
  cardano_credential_unref(&element);
  cardano_guard_set_unref(&guard_set);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_guard_set_from_cbor, canDeserializeKeyHashFormWithTag)
{
  // Arrange
  cardano_guard_set_t*   guard_set = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex(KEY_HASH_FORM_TAGGED_CBOR, strlen(KEY_HASH_FORM_TAGGED_CBOR));

  // Act
  cardano_error_t error = cardano_guard_set_from_cbor(reader, &guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_guard_set_get_length(guard_set), 1);
  EXPECT_TRUE(cardano_guard_set_is_tagged(guard_set));

  cardano_credential_t* element = nullptr;
  EXPECT_EQ(cardano_guard_set_get(guard_set, 0, &element), CARDANO_SUCCESS);
  EXPECT_STREQ(cardano_credential_get_hash_hex(element), KEY_HASH_HEX);

  // Cleanup
  cardano_credential_unref(&element);
  cardano_guard_set_unref(&guard_set);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_guard_set_from_cbor, canDeserializeCredentialForm)
{
  // Arrange
  cardano_guard_set_t*   guard_set = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex(CREDENTIAL_FORM_CBOR, strlen(CREDENTIAL_FORM_CBOR));

  // Act
  cardano_error_t error = cardano_guard_set_from_cbor(reader, &guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_guard_set_get_length(guard_set), 2);
  EXPECT_FALSE(cardano_guard_set_is_tagged(guard_set));

  cardano_credential_t* elem1 = nullptr;
  cardano_credential_t* elem2 = nullptr;

  EXPECT_EQ(cardano_guard_set_get(guard_set, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_guard_set_get(guard_set, 1, &elem2), CARDANO_SUCCESS);

  cardano_credential_type_t type = CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH;
  EXPECT_EQ(cardano_credential_get_type(elem1, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  EXPECT_STREQ(cardano_credential_get_hash_hex(elem1), KEY_HASH_HEX);

  EXPECT_EQ(cardano_credential_get_type(elem2, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);
  EXPECT_STREQ(cardano_credential_get_hash_hex(elem2), SCRIPT_HASH_HEX);

  // Cleanup
  cardano_credential_unref(&elem1);
  cardano_credential_unref(&elem2);
  cardano_guard_set_unref(&guard_set);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_guard_set_from_cbor, canDeserializeCredentialFormWithTag)
{
  // Arrange
  cardano_guard_set_t*   guard_set = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex(CREDENTIAL_FORM_TAGGED_CBOR, strlen(CREDENTIAL_FORM_TAGGED_CBOR));

  // Act
  cardano_error_t error = cardano_guard_set_from_cbor(reader, &guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_guard_set_get_length(guard_set), 2);
  EXPECT_TRUE(cardano_guard_set_is_tagged(guard_set));

  // Cleanup
  cardano_guard_set_unref(&guard_set);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_guard_set_from_cbor, canDeserializeCredentialFormWithOnlyKeyHashCredentials)
{
  // Arrange
  cardano_guard_set_t*   guard_set = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex(SINGLE_CREDENTIAL_CBOR, strlen(SINGLE_CREDENTIAL_CBOR));

  // Act
  cardano_error_t error = cardano_guard_set_from_cbor(reader, &guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(cardano_guard_set_get_length(guard_set), 1);

  cardano_credential_t* element = nullptr;
  EXPECT_EQ(cardano_guard_set_get(guard_set, 0, &element), CARDANO_SUCCESS);

  cardano_credential_type_t type = CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH;
  EXPECT_EQ(cardano_credential_get_type(element, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_CREDENTIAL_TYPE_KEY_HASH);
  EXPECT_STREQ(cardano_credential_get_hash_hex(element), KEY_HASH_HEX);

  // Cleanup
  cardano_credential_unref(&element);
  cardano_guard_set_unref(&guard_set);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_guard_set_from_cbor, returnErrorIfEmptySet)
{
  // Arrange
  cardano_guard_set_t*   guard_set = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("80", 2);

  // Act
  cardano_error_t error = cardano_guard_set_from_cbor(reader, &guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);
  EXPECT_EQ(guard_set, (cardano_guard_set_t*)nullptr);
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'guard_set', the set must not be empty.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_guard_set_from_cbor, returnErrorIfEmptyTaggedSet)
{
  // Arrange
  cardano_guard_set_t*   guard_set = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("d9010280", 8);

  // Act
  cardano_error_t error = cardano_guard_set_from_cbor(reader, &guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);
  EXPECT_EQ(guard_set, (cardano_guard_set_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_guard_set_from_cbor, returnErrorIfMixedFormsKeyHashFirst)
{
  // Arrange
  cardano_guard_set_t*   guard_set = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex(MIXED_KEY_HASH_FIRST_CBOR, strlen(MIXED_KEY_HASH_FIRST_CBOR));

  // Act
  cardano_error_t error = cardano_guard_set_from_cbor(reader, &guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);
  EXPECT_EQ(guard_set, (cardano_guard_set_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_guard_set_from_cbor, returnErrorIfMixedFormsCredentialFirst)
{
  // Arrange
  cardano_guard_set_t*   guard_set = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex(MIXED_CREDENTIAL_FIRST_CBOR, strlen(MIXED_CREDENTIAL_FIRST_CBOR));

  // Act
  cardano_error_t error = cardano_guard_set_from_cbor(reader, &guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);
  EXPECT_EQ(guard_set, (cardano_guard_set_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_guard_set_from_cbor, returnErrorIfFirstElementIsNotAKeyHashOrCredential)
{
  // Arrange
  cardano_guard_set_t*   guard_set = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("8101", 4);

  // Act
  cardano_error_t error = cardano_guard_set_from_cbor(reader, &guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);
  EXPECT_EQ(guard_set, (cardano_guard_set_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_guard_set_from_cbor, returnErrorIfDuplicatedKeyHash)
{
  // Arrange
  cardano_guard_set_t*   guard_set = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex(DUPLICATE_KEY_HASH_FORM_CBOR, strlen(DUPLICATE_KEY_HASH_FORM_CBOR));

  // Act
  cardano_error_t error = cardano_guard_set_from_cbor(reader, &guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_EQ(guard_set, (cardano_guard_set_t*)nullptr);
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "There was an error decoding 'guard_set', the set must not contain duplicated elements.");

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_guard_set_from_cbor, returnErrorIfDuplicatedCredential)
{
  // Arrange
  cardano_guard_set_t*   guard_set = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex(DUPLICATE_CRED_FORM_CBOR, strlen(DUPLICATE_CRED_FORM_CBOR));

  // Act
  cardano_error_t error = cardano_guard_set_from_cbor(reader, &guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_EQ(guard_set, (cardano_guard_set_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_guard_set_from_cbor, returnErrorIfInvalidKeyHashSize)
{
  // Arrange
  cardano_guard_set_t*   guard_set = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("81581b6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d", 60);

  // Act
  cardano_error_t error = cardano_guard_set_from_cbor(reader, &guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_VALUE);
  EXPECT_EQ(guard_set, (cardano_guard_set_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_guard_set_from_cbor, returnErrorIfInvalidTag)
{
  // Arrange
  cardano_guard_set_t*   guard_set = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("d9010081581c6199186adb51974690d7247d2646097d2c62763b16fb7ed3f9f55d39", 68);

  // Act
  cardano_error_t error = cardano_guard_set_from_cbor(reader, &guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_VALUE);
  EXPECT_EQ(guard_set, (cardano_guard_set_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_guard_set_from_cbor, returnErrorIfNotAnArray)
{
  // Arrange
  cardano_guard_set_t*   guard_set = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("01", 2);

  // Act
  cardano_error_t error = cardano_guard_set_from_cbor(reader, &guard_set);

  // Assert
  EXPECT_STREQ(cardano_cbor_reader_get_last_error(reader), "Major type mismatch.");
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_guard_set_from_cbor, returnErrorIfInvalidCbor)
{
  // Arrange
  cardano_guard_set_t*   guard_set = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex("ff", 2);

  // Act
  cardano_error_t error = cardano_guard_set_from_cbor(reader, &guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_guard_set_from_cbor, returnErrorIfGuardSetIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(KEY_HASH_FORM_CBOR, strlen(KEY_HASH_FORM_CBOR));

  // Act
  cardano_error_t error = cardano_guard_set_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_guard_set_from_cbor, returnErrorIfReaderIsNull)
{
  // Arrange
  cardano_guard_set_t* guard_set = nullptr;

  // Act
  cardano_error_t error = cardano_guard_set_from_cbor(nullptr, &guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_guard_set_from_cbor, returnErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_guard_set_t*   guard_set = nullptr;
  cardano_cbor_reader_t* reader    = cardano_cbor_reader_from_hex(KEY_HASH_FORM_CBOR, strlen(KEY_HASH_FORM_CBOR));

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_guard_set_from_cbor(reader, &guard_set);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(guard_set, (cardano_guard_set_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_guard_set_to_cbor, canRoundTripKeyHashForm)
{
  // Arrange
  cardano_guard_set_t* guard_set = new_default_guard_set(KEY_HASH_FORM_CBOR);
  ASSERT_THAT(guard_set, testing::Not((cardano_guard_set_t*)nullptr));

  // Act
  char* actual_cbor = encode_guard_set(guard_set);

  // Assert
  EXPECT_STREQ(actual_cbor, KEY_HASH_FORM_CBOR);

  // Cleanup
  cardano_guard_set_unref(&guard_set);
  free(actual_cbor);
}

TEST(cardano_guard_set_to_cbor, canRoundTripKeyHashFormWithTag)
{
  // Arrange
  cardano_guard_set_t* guard_set = new_default_guard_set(KEY_HASH_FORM_TAGGED_CBOR);
  ASSERT_THAT(guard_set, testing::Not((cardano_guard_set_t*)nullptr));

  // Act
  char* actual_cbor = encode_guard_set(guard_set);

  // Assert
  EXPECT_STREQ(actual_cbor, KEY_HASH_FORM_TAGGED_CBOR);

  // Cleanup
  cardano_guard_set_unref(&guard_set);
  free(actual_cbor);
}

TEST(cardano_guard_set_to_cbor, canRoundTripCredentialForm)
{
  // Arrange
  cardano_guard_set_t* guard_set = new_default_guard_set(CREDENTIAL_FORM_CBOR);
  ASSERT_THAT(guard_set, testing::Not((cardano_guard_set_t*)nullptr));

  // Act
  char* actual_cbor = encode_guard_set(guard_set);

  // Assert
  EXPECT_STREQ(actual_cbor, CREDENTIAL_FORM_CBOR);

  // Cleanup
  cardano_guard_set_unref(&guard_set);
  free(actual_cbor);
}

TEST(cardano_guard_set_to_cbor, canRoundTripCredentialFormWithTag)
{
  // Arrange
  cardano_guard_set_t* guard_set = new_default_guard_set(CREDENTIAL_FORM_TAGGED_CBOR);
  ASSERT_THAT(guard_set, testing::Not((cardano_guard_set_t*)nullptr));

  // Act
  char* actual_cbor = encode_guard_set(guard_set);

  // Assert
  EXPECT_STREQ(actual_cbor, CREDENTIAL_FORM_TAGGED_CBOR);

  // Cleanup
  cardano_guard_set_unref(&guard_set);
  free(actual_cbor);
}

TEST(cardano_guard_set_to_cbor, preservesCredentialOrder)
{
  // Arrange
  cardano_guard_set_t* guard_set = new_default_guard_set(CREDENTIAL_FORM_REVERSED_CBOR);
  ASSERT_THAT(guard_set, testing::Not((cardano_guard_set_t*)nullptr));

  cardano_credential_t* element = nullptr;
  EXPECT_EQ(cardano_guard_set_get(guard_set, 0, &element), CARDANO_SUCCESS);

  cardano_credential_type_t type = CARDANO_CREDENTIAL_TYPE_KEY_HASH;
  EXPECT_EQ(cardano_credential_get_type(element, &type), CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH);

  // Act
  char* actual_cbor = encode_guard_set(guard_set);

  // Assert
  EXPECT_STREQ(actual_cbor, CREDENTIAL_FORM_REVERSED_CBOR);

  // Cleanup
  cardano_credential_unref(&element);
  cardano_guard_set_unref(&guard_set);
  free(actual_cbor);
}

TEST(cardano_guard_set_to_cbor, encodesKeyHashOnlyCredentialFormAsKeyHashForm)
{
  // Arrange
  cardano_guard_set_t* guard_set = new_default_guard_set(SINGLE_CREDENTIAL_CBOR);
  ASSERT_THAT(guard_set, testing::Not((cardano_guard_set_t*)nullptr));

  // Act
  char* actual_cbor = encode_guard_set(guard_set);

  // Assert
  EXPECT_STREQ(actual_cbor, KEY_HASH_FORM_CBOR);

  // Cleanup
  cardano_guard_set_unref(&guard_set);
  free(actual_cbor);
}

TEST(cardano_guard_set_to_cbor, canSerializeKeyHashOnlyGuardSet)
{
  // Arrange
  cardano_guard_set_t* guard_set = nullptr;

  cardano_error_t error = cardano_guard_set_new(&guard_set);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* element = new_default_credential(KEY_HASH_CREDENTIAL_CBOR);
  EXPECT_EQ(cardano_guard_set_add(guard_set, element), CARDANO_SUCCESS);
  cardano_credential_unref(&element);

  // Act
  char* actual_cbor = encode_guard_set(guard_set);

  // Assert
  EXPECT_STREQ(actual_cbor, KEY_HASH_FORM_TAGGED_CBOR);

  // Cleanup
  cardano_guard_set_unref(&guard_set);
  free(actual_cbor);
}

TEST(cardano_guard_set_to_cbor, canSerializeGuardSetWithScriptHashCredential)
{
  // Arrange
  cardano_guard_set_t* guard_set = nullptr;

  cardano_error_t error = cardano_guard_set_new(&guard_set);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  const char* credentials[] = { KEY_HASH_CREDENTIAL_CBOR, SCRIPT_HASH_CREDENTIAL_CBOR };

  for (size_t i = 0; i < 2; ++i)
  {
    cardano_credential_t* element = new_default_credential(credentials[i]);

    EXPECT_EQ(cardano_guard_set_add(guard_set, element), CARDANO_SUCCESS);

    cardano_credential_unref(&element);
  }

  // Act
  char* actual_cbor = encode_guard_set(guard_set);

  // Assert
  EXPECT_STREQ(actual_cbor, CREDENTIAL_FORM_TAGGED_CBOR);

  // Cleanup
  cardano_guard_set_unref(&guard_set);
  free(actual_cbor);
}

TEST(cardano_guard_set_to_cbor, canSerializeAnEmptyGuardSet)
{
  // Arrange
  cardano_guard_set_t* guard_set = nullptr;

  cardano_error_t error = cardano_guard_set_new(&guard_set);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  char* actual_cbor = encode_guard_set(guard_set);

  // Assert
  EXPECT_STREQ(actual_cbor, "d9010280");

  // Cleanup
  cardano_guard_set_unref(&guard_set);
  free(actual_cbor);
}

TEST(cardano_guard_set_to_cbor, returnsErrorIfGivenANullPtr)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_guard_set_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_guard_set_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_guard_set_t* guard_set = nullptr;

  cardano_error_t error = cardano_guard_set_new(&guard_set);
  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_guard_set_to_cbor(guard_set, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_guard_set_unref(&guard_set);
}

TEST(cardano_guard_set_get_length, returnsZeroIfGuardSetIsNull)
{
  // Act
  size_t length = cardano_guard_set_get_length(nullptr);

  // Assert
  EXPECT_EQ(length, 0);
}

TEST(cardano_guard_set_get_length, returnsZeroIfGuardSetIsEmpty)
{
  // Arrange
  cardano_guard_set_t* guard_set = nullptr;
  cardano_error_t      error     = cardano_guard_set_new(&guard_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  size_t length = cardano_guard_set_get_length(guard_set);

  // Assert
  EXPECT_EQ(length, 0);

  // Cleanup
  cardano_guard_set_unref(&guard_set);
}

TEST(cardano_guard_set_get, returnsErrorIfGuardSetIsNull)
{
  // Arrange
  cardano_credential_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_guard_set_get(nullptr, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_guard_set_get, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_guard_set_t* guard_set = nullptr;
  cardano_error_t      error     = cardano_guard_set_new(&guard_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_guard_set_get(guard_set, 0, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_guard_set_unref(&guard_set);
}

TEST(cardano_guard_set_get, returnsErrorIfIndexIsOutOfBounds)
{
  // Arrange
  cardano_guard_set_t* guard_set = nullptr;
  cardano_error_t      error     = cardano_guard_set_new(&guard_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_credential_t* data = nullptr;
  error                      = cardano_guard_set_get(guard_set, 0, &data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ);

  // Cleanup
  cardano_guard_set_unref(&guard_set);
}

TEST(cardano_guard_set_add, returnsErrorIfGuardSetIsNull)
{
  // Arrange
  cardano_credential_t* data = nullptr;

  // Act
  cardano_error_t error = cardano_guard_set_add(nullptr, data);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_guard_set_add, returnsErrorIfDataIsNull)
{
  // Arrange
  cardano_guard_set_t* guard_set = nullptr;
  cardano_error_t      error     = cardano_guard_set_new(&guard_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_guard_set_add(guard_set, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_guard_set_unref(&guard_set);
}

TEST(cardano_guard_set_add, returnsErrorIfElementIsDuplicated)
{
  // Arrange
  cardano_guard_set_t* guard_set = nullptr;
  cardano_error_t      error     = cardano_guard_set_new(&guard_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* element = new_default_credential(KEY_HASH_CREDENTIAL_CBOR);

  // Act
  EXPECT_EQ(cardano_guard_set_add(guard_set, element), CARDANO_SUCCESS);
  error = cardano_guard_set_add(guard_set, element);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DUPLICATED_KEY);
  EXPECT_EQ(cardano_guard_set_get_length(guard_set), 1);

  // Cleanup
  cardano_credential_unref(&element);
  cardano_guard_set_unref(&guard_set);
}

TEST(cardano_guard_set_add, preservesInsertionOrder)
{
  // Arrange
  cardano_guard_set_t* guard_set = nullptr;
  cardano_error_t      error     = cardano_guard_set_new(&guard_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* credentials[] = { SCRIPT_HASH_CREDENTIAL_CBOR, KEY_HASH_CREDENTIAL_CBOR };

  for (size_t i = 0; i < 2; ++i)
  {
    cardano_credential_t* element = new_default_credential(credentials[i]);

    EXPECT_EQ(cardano_guard_set_add(guard_set, element), CARDANO_SUCCESS);

    cardano_credential_unref(&element);
  }

  // Act
  cardano_credential_t* elem1 = nullptr;
  cardano_credential_t* elem2 = nullptr;

  EXPECT_EQ(cardano_guard_set_get(guard_set, 0, &elem1), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_guard_set_get(guard_set, 1, &elem2), CARDANO_SUCCESS);

  // Assert
  EXPECT_STREQ(cardano_credential_get_hash_hex(elem1), SCRIPT_HASH_HEX);
  EXPECT_STREQ(cardano_credential_get_hash_hex(elem2), KEY_HASH_HEX);

  // Cleanup
  cardano_credential_unref(&elem1);
  cardano_credential_unref(&elem2);
  cardano_guard_set_unref(&guard_set);
}

TEST(cardano_guard_set_is_tagged, returnsFalseIfGuardSetIsNull)
{
  // Act
  bool is_tagged = cardano_guard_set_is_tagged(nullptr);

  // Assert
  EXPECT_FALSE(is_tagged);
}

TEST(cardano_guard_set_is_tagged, returnsTrueIfFromCborReadTaggedSet)
{
  // Arrange
  cardano_guard_set_t* guard_set = new_default_guard_set(CREDENTIAL_FORM_TAGGED_CBOR);
  ASSERT_THAT(guard_set, testing::Not((cardano_guard_set_t*)nullptr));

  // Act
  bool is_tagged = cardano_guard_set_is_tagged(guard_set);

  // Assert
  EXPECT_TRUE(is_tagged);

  // Cleanup
  cardano_guard_set_unref(&guard_set);
}

TEST(cardano_guard_set_is_tagged, returnsFalseIfFromCborReadUntaggedSet)
{
  // Arrange
  cardano_guard_set_t* guard_set = new_default_guard_set(CREDENTIAL_FORM_CBOR);
  ASSERT_THAT(guard_set, testing::Not((cardano_guard_set_t*)nullptr));

  // Act
  bool is_tagged = cardano_guard_set_is_tagged(guard_set);

  // Assert
  EXPECT_FALSE(is_tagged);

  // Cleanup
  cardano_guard_set_unref(&guard_set);
}

TEST(cardano_guard_set_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_guard_set_t* guard_set = nullptr;
  cardano_error_t      error     = cardano_guard_set_new(&guard_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_guard_set_ref(guard_set);

  // Assert
  EXPECT_THAT(guard_set, testing::Not((cardano_guard_set_t*)nullptr));
  EXPECT_EQ(cardano_guard_set_refcount(guard_set), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_guard_set_unref(&guard_set);
  cardano_guard_set_unref(&guard_set);
}

TEST(cardano_guard_set_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_guard_set_ref(nullptr);
}

TEST(cardano_guard_set_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_guard_set_t* guard_set = nullptr;

  // Act
  cardano_guard_set_unref(&guard_set);
}

TEST(cardano_guard_set_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_guard_set_unref((cardano_guard_set_t**)nullptr);
}

TEST(cardano_guard_set_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_guard_set_t* guard_set = nullptr;
  cardano_error_t      error     = cardano_guard_set_new(&guard_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_guard_set_ref(guard_set);
  size_t ref_count = cardano_guard_set_refcount(guard_set);

  cardano_guard_set_unref(&guard_set);
  size_t updated_ref_count = cardano_guard_set_refcount(guard_set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_guard_set_unref(&guard_set);
}

TEST(cardano_guard_set_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_guard_set_t* guard_set = nullptr;
  cardano_error_t      error     = cardano_guard_set_new(&guard_set);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_guard_set_ref(guard_set);
  size_t ref_count = cardano_guard_set_refcount(guard_set);

  cardano_guard_set_unref(&guard_set);
  size_t updated_ref_count = cardano_guard_set_refcount(guard_set);

  cardano_guard_set_unref(&guard_set);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(guard_set, (cardano_guard_set_t*)nullptr);

  // Cleanup
  cardano_guard_set_unref(&guard_set);
}

TEST(cardano_guard_set_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_guard_set_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_guard_set_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_guard_set_t* guard_set = nullptr;
  const char*          message   = "This is a test message";

  // Act
  cardano_guard_set_set_last_error(guard_set, message);

  // Assert
  EXPECT_STREQ(cardano_guard_set_get_last_error(guard_set), "Object is NULL.");
}

TEST(cardano_guard_set_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_guard_set_t* guard_set = nullptr;
  cardano_error_t      error     = cardano_guard_set_new(&guard_set);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_guard_set_set_last_error(guard_set, message);

  // Assert
  EXPECT_STREQ(cardano_guard_set_get_last_error(guard_set), "");

  // Cleanup
  cardano_guard_set_unref(&guard_set);
}
