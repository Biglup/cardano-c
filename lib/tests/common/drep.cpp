/**
 * \file drep.cpp
 *
 * \author angel.castillo
 * \date   Apr 14, 2024
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

#include <cardano/common/drep.h>

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* DREP_KEY_HASH_CBOR      = "8200581c00000000000000000000000000000000000000000000000000000000";
static const char* DREP_SCRIPT_HASH_CBOR   = "8201581c00000000000000000000000000000000000000000000000000000000";
static const char* DREP_ABSTAIN_CBOR       = "8102";
static const char* DREP_NO_CONFIDENCE_CBOR = "8103";
static const char* DREP_CRED_HASH          = "00000000000000000000000000000000000000000000000000000000";

/* UNIT TESTS ****************************************************************/

TEST(cardano_drep_to_cbor, canSerializeDrep)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_drep_t*        drep   = nullptr;
  cardano_error_t        error  = cardano_drep_new(CARDANO_DREP_TYPE_ABSTAIN, NULL, &drep);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_drep_to_cbor(drep, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex      = (char*)malloc(cbor_hex_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, DREP_ABSTAIN_CBOR);

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_drep_unref(&drep);
}

TEST(cardano_drep_to_cbor, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_drep_t* drep  = nullptr;
  cardano_error_t error = cardano_drep_new(CARDANO_DREP_TYPE_ABSTAIN, NULL, &drep);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_drep_to_cbor(drep, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_drep_unref(&drep);
}

TEST(cardano_drep_to_cbor, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t error = cardano_drep_to_cbor(nullptr, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_drep_from_cbor, canDeserializeDrep)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_ABSTAIN_CBOR, strlen(DREP_ABSTAIN_CBOR));
  cardano_drep_t*        drep   = nullptr;

  // Act
  cardano_error_t error = cardano_drep_from_cbor(reader, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(drep, testing::Not((cardano_drep_t*)nullptr));

  cardano_drep_type_t type = CARDANO_DREP_TYPE_KEY_HASH;

  error = cardano_drep_get_type(drep, &type);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(type, CARDANO_DREP_TYPE_ABSTAIN);

  // Cleanup
  cardano_drep_unref(&drep);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_new, canCreateDrep)
{
  // Arrange
  cardano_drep_t* drep = nullptr;

  // Act
  cardano_error_t error = cardano_drep_new(CARDANO_DREP_TYPE_ABSTAIN, NULL, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(drep, testing::Not((cardano_drep_t*)nullptr));

  cardano_drep_type_t type = CARDANO_DREP_TYPE_KEY_HASH;
  error                    = cardano_drep_get_type(drep, &type);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_DREP_TYPE_ABSTAIN);

  // Cleanup
  cardano_drep_unref(&drep);
}

TEST(cardano_drep_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_drep_t* drep = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_drep_new(CARDANO_DREP_TYPE_ABSTAIN, NULL, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(drep, (cardano_drep_t*)nullptr);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_drep_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_drep_t* drep  = nullptr;
  cardano_error_t error = cardano_drep_new(CARDANO_DREP_TYPE_ABSTAIN, NULL, &drep);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_drep_ref(drep);

  // Assert
  EXPECT_THAT(drep, testing::Not((cardano_drep_t*)nullptr));
  EXPECT_EQ(cardano_drep_refcount(drep), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_drep_unref(&drep);
  cardano_drep_unref(&drep);
}

TEST(cardano_drep_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_drep_ref(nullptr);
}

TEST(cardano_drep_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_drep_t* drep = nullptr;

  // Act
  cardano_drep_unref(&drep);
}

TEST(cardano_drep_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_drep_unref((cardano_drep_t**)nullptr);
}

TEST(cardano_drep_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_drep_t* drep  = nullptr;
  cardano_error_t error = cardano_drep_new(CARDANO_DREP_TYPE_ABSTAIN, NULL, &drep);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_drep_ref(drep);
  size_t ref_count = cardano_drep_refcount(drep);

  cardano_drep_unref(&drep);
  size_t updated_ref_count = cardano_drep_refcount(drep);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_drep_unref(&drep);
}

TEST(cardano_drep_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_drep_t* drep  = nullptr;
  cardano_error_t error = cardano_drep_new(CARDANO_DREP_TYPE_ABSTAIN, NULL, &drep);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  cardano_drep_ref(drep);
  size_t ref_count = cardano_drep_refcount(drep);

  cardano_drep_unref(&drep);
  size_t updated_ref_count = cardano_drep_refcount(drep);

  cardano_drep_unref(&drep);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(drep, (cardano_drep_t*)nullptr);

  // Cleanup
  cardano_drep_unref(&drep);
}

TEST(cardano_drep_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_drep_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_drep_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_drep_t* drep    = nullptr;
  const char*     message = "This is a test message";

  // Act
  cardano_drep_set_last_error(drep, message);

  // Assert
  EXPECT_STREQ(cardano_drep_get_last_error(drep), "Object is NULL.");
}

TEST(cardano_drep_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_drep_t* drep  = nullptr;
  cardano_error_t error = cardano_drep_new(CARDANO_DREP_TYPE_ABSTAIN, NULL, &drep);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* message = nullptr;

  // Act
  cardano_drep_set_last_error(drep, message);

  // Assert
  EXPECT_STREQ(cardano_drep_get_last_error(drep), "");

  // Cleanup
  cardano_drep_unref(&drep);
}

TEST(cardano_drep_new, canCreateDrepWithKeyHash)
{
  // Arrange
  cardano_drep_t*       drep       = nullptr;
  cardano_credential_t* credential = NULL;
  cardano_error_t       error      = cardano_credential_from_hash_hex(DREP_CRED_HASH, strlen(DREP_CRED_HASH), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_drep_new(CARDANO_DREP_TYPE_KEY_HASH, credential, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(drep, testing::Not((cardano_drep_t*)nullptr));

  cardano_drep_type_t type = CARDANO_DREP_TYPE_KEY_HASH;
  error                    = cardano_drep_get_type(drep, &type);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_DREP_TYPE_KEY_HASH);

  // Cleanup
  cardano_drep_unref(&drep);
  cardano_credential_unref(&credential);
}

TEST(cardano_drep_new, canCreateDrepWithScriptHash)
{
  // Arrange
  cardano_drep_t*       drep       = nullptr;
  cardano_credential_t* credential = NULL;
  cardano_error_t       error      = cardano_credential_from_hash_hex(DREP_CRED_HASH, strlen(DREP_CRED_HASH), CARDANO_CREDENTIAL_TYPE_SCRIPT_HASH, &credential);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_drep_new(CARDANO_DREP_TYPE_SCRIPT_HASH, credential, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(drep, testing::Not((cardano_drep_t*)nullptr));

  cardano_drep_type_t type = CARDANO_DREP_TYPE_KEY_HASH;
  error                    = cardano_drep_get_type(drep, &type);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_DREP_TYPE_SCRIPT_HASH);

  // Cleanup
  cardano_drep_unref(&drep);
  cardano_credential_unref(&credential);
}

TEST(cardano_drep_new, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_drep_t* drep = nullptr;

  // Act
  cardano_error_t error = cardano_drep_new(CARDANO_DREP_TYPE_KEY_HASH, NULL, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(drep, (cardano_drep_t*)nullptr);
}

TEST(cardano_drep_new, returnsErrorIfTypeIsInvalid)
{
  // Arrange
  cardano_drep_t*       drep       = nullptr;
  cardano_credential_t* credential = NULL;
  cardano_error_t       error      = cardano_credential_from_hash_hex(DREP_CRED_HASH, strlen(DREP_CRED_HASH), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_drep_new(CARDANO_DREP_TYPE_ABSTAIN, credential, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(drep, (cardano_drep_t*)nullptr);

  // Cleanup
  cardano_credential_unref(&credential);
}

TEST(cardano_drep_get_credential, canGetCredential)
{
  // Arrange
  cardano_drep_t*       drep       = nullptr;
  cardano_credential_t* credential = NULL;
  cardano_error_t       error      = cardano_credential_from_hash_hex(DREP_CRED_HASH, strlen(DREP_CRED_HASH), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_drep_new(CARDANO_DREP_TYPE_KEY_HASH, credential, &drep);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* retrieved_credential = nullptr;

  // Act
  error = cardano_drep_get_credential(drep, &retrieved_credential);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(retrieved_credential, testing::Not((cardano_credential_t*)nullptr));

  // Cleanup
  cardano_credential_unref(&credential);
  cardano_drep_unref(&drep);
}

TEST(cardano_drep_get_credential, returnsErrorIfDrepIsNull)
{
  // Arrange
  cardano_credential_t* credential = nullptr;

  // Act
  cardano_error_t error = cardano_drep_get_credential(nullptr, &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
  EXPECT_EQ(credential, (cardano_credential_t*)nullptr);
}

TEST(cardano_drep_get_credential, returnsErrorIfCredentialIsNull)
{
  // Act
  cardano_error_t error = cardano_drep_get_credential((cardano_drep_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_drep_set_credential, canSetCredential)
{
  // Arrange
  cardano_drep_t*       drep       = nullptr;
  cardano_credential_t* credential = NULL;
  cardano_error_t       error      = cardano_credential_from_hash_hex(DREP_CRED_HASH, strlen(DREP_CRED_HASH), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_drep_new(CARDANO_DREP_TYPE_ABSTAIN, NULL, &drep);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_drep_set_type(drep, CARDANO_DREP_TYPE_KEY_HASH);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_drep_set_credential(drep, credential);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* retrieved_credential = nullptr;

  error = cardano_drep_get_credential(drep, &retrieved_credential);

  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(retrieved_credential, testing::Not((cardano_credential_t*)nullptr));

  // Cleanup
  cardano_credential_unref(&credential);
  cardano_drep_unref(&drep);
}

TEST(cardano_drep_set_credential, returnsErrorIfDrepIsNull)
{
  // Arrange
  cardano_credential_t* credential = nullptr;

  // Act
  cardano_error_t error = cardano_drep_set_credential(nullptr, credential);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_drep_set_credential, returnsErrorIfCredentialIsNull)
{
  // Act
  cardano_error_t error = cardano_drep_set_credential((cardano_drep_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_drep_get_type, canGetType)
{
  // Arrange
  cardano_drep_t* drep  = nullptr;
  cardano_error_t error = cardano_drep_new(CARDANO_DREP_TYPE_ABSTAIN, NULL, &drep);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  cardano_drep_type_t type = CARDANO_DREP_TYPE_KEY_HASH;

  // Act
  error = cardano_drep_get_type(drep, &type);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_EQ(type, CARDANO_DREP_TYPE_ABSTAIN);

  // Cleanup
  cardano_drep_unref(&drep);
}

TEST(cardano_drep_get_type, returnsErrorIfDrepIsNull)
{
  // Arrange
  cardano_drep_type_t type = CARDANO_DREP_TYPE_KEY_HASH;

  // Act
  cardano_error_t error = cardano_drep_get_type(nullptr, &type);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_drep_get_type, returnsErrorIfTypeIsNull)
{
  // Arrange
  cardano_drep_t* drep = nullptr;

  // Act
  cardano_error_t error = cardano_drep_get_type(drep, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_drep_new, returnErrorIfDrepIsNull)
{
  // Arrange
  cardano_error_t error = cardano_drep_new(CARDANO_DREP_TYPE_ABSTAIN, NULL, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_drep_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_drep_t* drep = nullptr;

  // Act
  cardano_error_t error = cardano_drep_from_cbor(nullptr, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_drep_from_cbor, returnsErrorIfDrepIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_ABSTAIN_CBOR, strlen(DREP_ABSTAIN_CBOR));

  // Act
  cardano_error_t error = cardano_drep_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_from_cbor, returnsErrorIfCborIsInvalid)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8109", 4);
  cardano_drep_t*        drep   = nullptr;

  // Act
  cardano_error_t error = cardano_drep_from_cbor(reader, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_CBOR_VALUE);
  EXPECT_EQ(drep, (cardano_drep_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_from_cbor, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_ABSTAIN_CBOR, strlen(DREP_ABSTAIN_CBOR));
  cardano_drep_t*        drep   = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_drep_from_cbor(reader, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(drep, (cardano_drep_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_drep_from_cbor, canDeserializeDrepWithKeyHash)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_KEY_HASH_CBOR, strlen(DREP_KEY_HASH_CBOR));
  cardano_drep_t*        drep   = nullptr;

  // Act
  cardano_error_t error = cardano_drep_from_cbor(reader, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(drep, testing::Not((cardano_drep_t*)nullptr));

  cardano_drep_type_t type = CARDANO_DREP_TYPE_KEY_HASH;

  error = cardano_drep_get_type(drep, &type);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(type, CARDANO_DREP_TYPE_KEY_HASH);

  // Cleanup
  cardano_drep_unref(&drep);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_from_cbor, canDeserializeDrepWithScriptHash)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_SCRIPT_HASH_CBOR, strlen(DREP_SCRIPT_HASH_CBOR));
  cardano_drep_t*        drep   = nullptr;

  // Act
  cardano_error_t error = cardano_drep_from_cbor(reader, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(drep, testing::Not((cardano_drep_t*)nullptr));

  cardano_drep_type_t type = CARDANO_DREP_TYPE_KEY_HASH;

  error = cardano_drep_get_type(drep, &type);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(type, CARDANO_DREP_TYPE_SCRIPT_HASH);

  // Cleanup
  cardano_drep_unref(&drep);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_from_cbor, canDeserializeDrepWithAbstain)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_ABSTAIN_CBOR, strlen(DREP_ABSTAIN_CBOR));
  cardano_drep_t*        drep   = nullptr;

  // Act
  cardano_error_t error = cardano_drep_from_cbor(reader, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(drep, testing::Not((cardano_drep_t*)nullptr));

  cardano_drep_type_t type = CARDANO_DREP_TYPE_KEY_HASH;

  error = cardano_drep_get_type(drep, &type);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(type, CARDANO_DREP_TYPE_ABSTAIN);

  // Cleanup
  cardano_drep_unref(&drep);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_from_cbor, canDeserializeDrepWithNoConfidence)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_NO_CONFIDENCE_CBOR, strlen(DREP_NO_CONFIDENCE_CBOR));
  cardano_drep_t*        drep   = nullptr;

  // Act
  cardano_error_t error = cardano_drep_from_cbor(reader, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(drep, testing::Not((cardano_drep_t*)nullptr));

  cardano_drep_type_t type = CARDANO_DREP_TYPE_KEY_HASH;

  error = cardano_drep_get_type(drep, &type);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_EQ(type, CARDANO_DREP_TYPE_NO_CONFIDENCE);

  // Cleanup
  cardano_drep_unref(&drep);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_from_cbor, invalidCborKeyHashAndNoHashBytes)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8100", 4);
  cardano_drep_t*        drep   = nullptr;

  // Act
  cardano_error_t error = cardano_drep_from_cbor(reader, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
  EXPECT_EQ(drep, (cardano_drep_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_from_cbor, invalidCborKeyHashAndNoByteString)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8200fe", 6);
  cardano_drep_t*        drep   = nullptr;

  // Act
  cardano_error_t error = cardano_drep_from_cbor(reader, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_DECODING);
  EXPECT_EQ(drep, (cardano_drep_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_drep_from_cbor, returnsErrorIfEventualMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_KEY_HASH_CBOR, strlen(DREP_KEY_HASH_CBOR));
  cardano_drep_t*        drep   = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_six_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_drep_from_cbor(reader, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(drep, (cardano_drep_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_drep_from_cbor, returnsErrorIfEventualMemoryAllocationFails2)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_KEY_HASH_CBOR, strlen(DREP_KEY_HASH_CBOR));
  cardano_drep_t*        drep   = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_nine_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_drep_from_cbor(reader, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(drep, (cardano_drep_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_drep_from_cbor, returnsErrorIfEventualMemoryAllocationFails3)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_KEY_HASH_CBOR, strlen(DREP_KEY_HASH_CBOR));
  cardano_drep_t*        drep   = nullptr;

  reset_allocators_run_count();
  cardano_set_allocators(fail_after_thirteen_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_drep_from_cbor(reader, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(drep, (cardano_drep_t*)nullptr);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_drep_to_cbor, canSerializeKeyHashDRep)
{
  // Arrange
  cardano_cbor_writer_t* writer     = cardano_cbor_writer_new();
  cardano_drep_t*        drep       = nullptr;
  cardano_credential_t*  credential = NULL;
  cardano_error_t        error      = cardano_credential_from_hash_hex(DREP_CRED_HASH, strlen(DREP_CRED_HASH), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  error = cardano_drep_new(CARDANO_DREP_TYPE_KEY_HASH, credential, &drep);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_drep_to_cbor(drep, writer);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const size_t cbor_hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        cbor_hex      = (char*)malloc(cbor_hex_size);

  error = cardano_cbor_writer_encode_hex(writer, cbor_hex, cbor_hex_size);
  EXPECT_EQ(error, CARDANO_SUCCESS);

  EXPECT_STREQ(cbor_hex, DREP_KEY_HASH_CBOR);

  // Cleanup
  free(cbor_hex);
  cardano_cbor_writer_unref(&writer);
  cardano_drep_unref(&drep);
  cardano_credential_unref(&credential);
}

TEST(cardano_drep_get_credential, returnsErrorIfDrepIsNotKeyHash)
{
  // Arrange
  cardano_drep_t* drep  = nullptr;
  cardano_error_t error = cardano_drep_new(CARDANO_DREP_TYPE_ABSTAIN, NULL, &drep);

  ASSERT_EQ(error, CARDANO_SUCCESS);

  cardano_credential_t* credential = nullptr;

  // Act
  error = cardano_drep_get_credential(drep, &credential);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);
  EXPECT_EQ(credential, (cardano_credential_t*)nullptr);

  // Cleanup
  cardano_drep_unref(&drep);
}

TEST(cardano_drep_set_credential, returnErrorIfNotKeyHashOrScriptHash)
{
  // Arrange
  cardano_drep_t*       drep       = nullptr;
  cardano_credential_t* credential = NULL;
  cardano_error_t       error      = cardano_credential_from_hash_hex(DREP_CRED_HASH, strlen(DREP_CRED_HASH), CARDANO_CREDENTIAL_TYPE_KEY_HASH, &credential);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  error = cardano_drep_new(CARDANO_DREP_TYPE_ABSTAIN, NULL, &drep);

  EXPECT_EQ(error, CARDANO_SUCCESS);

  // Act
  error = cardano_drep_set_credential(drep, credential);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_INVALID_ARGUMENT);

  // Cleanup
  cardano_credential_unref(&credential);
  cardano_drep_unref(&drep);
}

TEST(cardano_drep_get_type, returnNullIfTypeIsNull)
{
  // Act
  cardano_error_t error = cardano_drep_get_type((cardano_drep_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_drep_set_type, returnErrorIfDrepIsNull)
{
  // Act
  cardano_error_t error = cardano_drep_set_type(nullptr, CARDANO_DREP_TYPE_KEY_HASH);

  // Assert
  EXPECT_EQ(error, CARDANO_POINTER_IS_NULL);
}