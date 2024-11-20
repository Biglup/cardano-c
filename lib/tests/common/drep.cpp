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

static const char* DREP_KEY_HASH_CBOR        = "8200581c00000000000000000000000000000000000000000000000000000000";
static const char* DREP_SCRIPT_HASH_CBOR     = "8201581c00000000000000000000000000000000000000000000000000000000";
static const char* DREP_ABSTAIN_CBOR         = "8102";
static const char* DREP_NO_CONFIDENCE_CBOR   = "8103";
static const char* DREP_CRED_HASH            = "00000000000000000000000000000000000000000000000000000000";
static const char* DREP_CIP105_KEY_HASH      = "drep19we4mh7zaxqmyasqgpr7h7hcuq5m6dwpx99j4mrcd3e4ufxuc8n";
static const char* DREP_CIP105_SCRIPT_HASH   = "drep_script1rxdd99vu338y659qfg8nmpemdyhlsmaudgv4m4zdz7m5vz8uzt6";
static const char* DREP_CIP129_KEY_HASH      = "drep1yg4mxhwlct5crvnkqpqy06l6lrszn0f4cyc5k2hv0pk8xhsvluu37";
static const char* DREP_CIP129_SCRIPT_HASH   = "drep1yvve4554njxyun2s5p9q70v88d5jl7r0h34pjhw5f5tmw3sjtrutp";
static const char* DREP_SCRIPT_HASH          = "199ad2959c8c4e4d50a04a0f3d873b692ff86fbc6a195dd44d17b746";
static const char* DREP_KEY_HASH             = "2bb35ddfc2e981b276004047ebfaf8e029bd35c1314b2aec786c735e";
static const char* DREP_INVALID_HASH_SIZE    = "drep1478q9x7ntsf3fv4wc7rvwdgw2uk75x";
static const char* DREP_INVALID_KEY_TYPE     = "drep1yqqzh0wlct5crvnkqpqy06l6lrszn0f4cyc5k2hv0pk8xhsx9kyk8";
static const char* DREP_INVALID_GOV_KEY_TYPE = "drep1qgqzh0wlct5crvnkqpqy06l6lrszn0f4cyc5k2hv0pk8xhs5cw03f";

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
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

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
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

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
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
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
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
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
  cardano_credential_unref(&retrieved_credential);
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
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(credential, (cardano_credential_t*)nullptr);
}

TEST(cardano_drep_get_credential, returnsErrorIfCredentialIsNull)
{
  // Act
  cardano_error_t error = cardano_drep_get_credential((cardano_drep_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
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
  cardano_credential_unref(&retrieved_credential);
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
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_set_credential, returnsErrorIfCredentialIsNull)
{
  // Act
  cardano_error_t error = cardano_drep_set_credential((cardano_drep_t*)"", nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
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
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_get_type, returnsErrorIfTypeIsNull)
{
  // Arrange
  cardano_drep_t* drep = nullptr;

  // Act
  cardano_error_t error = cardano_drep_get_type(drep, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_new, returnErrorIfDrepIsNull)
{
  // Arrange
  cardano_error_t error = cardano_drep_new(CARDANO_DREP_TYPE_ABSTAIN, NULL, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_drep_t* drep = nullptr;

  // Act
  cardano_error_t error = cardano_drep_from_cbor(nullptr, &drep);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_from_cbor, returnsErrorIfDrepIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(DREP_ABSTAIN_CBOR, strlen(DREP_ABSTAIN_CBOR));

  // Act
  cardano_error_t error = cardano_drep_from_cbor(reader, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

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
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
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
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
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
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
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
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
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
  cardano_credential_unref(&credential);
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
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_set_type, returnErrorIfDrepIsNull)
{
  // Act
  cardano_error_t error = cardano_drep_set_type(nullptr, CARDANO_DREP_TYPE_KEY_HASH);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_from_string, canCreateDrepWithKeyHashCip105)
{
  // Arrange
  cardano_drep_t*         drep      = nullptr;
  cardano_blake2b_hash_t* cred_hash = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(DREP_KEY_HASH, strlen(DREP_KEY_HASH), &cred_hash), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_drep_from_string(DREP_CIP105_KEY_HASH, strlen(DREP_CIP105_KEY_HASH), &drep), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(drep, testing::Not((cardano_drep_t*)nullptr));

  cardano_drep_type_t type = CARDANO_DREP_TYPE_KEY_HASH;
  EXPECT_EQ(cardano_drep_get_type(drep, &type), CARDANO_SUCCESS);

  EXPECT_EQ(type, CARDANO_DREP_TYPE_KEY_HASH);

  cardano_credential_t* credential = nullptr;
  EXPECT_EQ(cardano_drep_get_credential(drep, &credential), CARDANO_SUCCESS);

  cardano_blake2b_hash_t* cred_hash2 = cardano_credential_get_hash(credential);

  EXPECT_EQ(cardano_blake2b_hash_compare(cred_hash, cred_hash2), 0);

  // Cleanup
  cardano_drep_unref(&drep);
  cardano_blake2b_hash_unref(&cred_hash);
  cardano_blake2b_hash_unref(&cred_hash2);
  cardano_credential_unref(&credential);
}

TEST(cardano_drep_from_string, canCreateDrepWithScriptHashCip105)
{
  // Arrange
  cardano_drep_t*         drep      = nullptr;
  cardano_blake2b_hash_t* cred_hash = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(DREP_SCRIPT_HASH, strlen(DREP_SCRIPT_HASH), &cred_hash), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_drep_from_string(DREP_CIP105_SCRIPT_HASH, strlen(DREP_CIP105_SCRIPT_HASH), &drep), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(drep, testing::Not((cardano_drep_t*)nullptr));

  cardano_drep_type_t type = CARDANO_DREP_TYPE_KEY_HASH;
  EXPECT_EQ(cardano_drep_get_type(drep, &type), CARDANO_SUCCESS);

  EXPECT_EQ(type, CARDANO_DREP_TYPE_SCRIPT_HASH);

  cardano_credential_t* credential = nullptr;
  EXPECT_EQ(cardano_drep_get_credential(drep, &credential), CARDANO_SUCCESS);

  cardano_blake2b_hash_t* cred_hash2 = cardano_credential_get_hash(credential);

  EXPECT_EQ(cardano_blake2b_hash_compare(cred_hash, cred_hash2), 0);

  // Cleanup
  cardano_drep_unref(&drep);
  cardano_blake2b_hash_unref(&cred_hash);
  cardano_blake2b_hash_unref(&cred_hash2);
  cardano_credential_unref(&credential);
}

TEST(cardano_drep_from_string, canCreateDrepWithKeyHashCip129)
{
  // Arrange
  cardano_drep_t*         drep      = nullptr;
  cardano_blake2b_hash_t* cred_hash = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(DREP_KEY_HASH, strlen(DREP_KEY_HASH), &cred_hash), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_drep_from_string(DREP_CIP129_KEY_HASH, strlen(DREP_CIP129_KEY_HASH), &drep), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(drep, testing::Not((cardano_drep_t*)nullptr));

  cardano_drep_type_t type = CARDANO_DREP_TYPE_KEY_HASH;
  EXPECT_EQ(cardano_drep_get_type(drep, &type), CARDANO_SUCCESS);

  EXPECT_EQ(type, CARDANO_DREP_TYPE_KEY_HASH);

  cardano_credential_t* credential = nullptr;
  EXPECT_EQ(cardano_drep_get_credential(drep, &credential), CARDANO_SUCCESS);

  cardano_blake2b_hash_t* cred_hash2 = cardano_credential_get_hash(credential);

  EXPECT_EQ(cardano_blake2b_hash_compare(cred_hash, cred_hash2), 0);

  // Cleanup
  cardano_drep_unref(&drep);
  cardano_blake2b_hash_unref(&cred_hash);
  cardano_blake2b_hash_unref(&cred_hash2);
  cardano_credential_unref(&credential);
}

TEST(cardano_drep_from_string, canCreateDrepWithScriptHashCip129)
{
  // Arrange
  cardano_drep_t*         drep      = nullptr;
  cardano_blake2b_hash_t* cred_hash = NULL;

  EXPECT_EQ(cardano_blake2b_hash_from_hex(DREP_SCRIPT_HASH, strlen(DREP_SCRIPT_HASH), &cred_hash), CARDANO_SUCCESS);

  // Act
  EXPECT_EQ(cardano_drep_from_string(DREP_CIP129_SCRIPT_HASH, strlen(DREP_CIP129_SCRIPT_HASH), &drep), CARDANO_SUCCESS);

  // Assert
  EXPECT_THAT(drep, testing::Not((cardano_drep_t*)nullptr));

  cardano_drep_type_t type = CARDANO_DREP_TYPE_KEY_HASH;
  EXPECT_EQ(cardano_drep_get_type(drep, &type), CARDANO_SUCCESS);

  EXPECT_EQ(type, CARDANO_DREP_TYPE_SCRIPT_HASH);

  cardano_credential_t* credential = nullptr;
  EXPECT_EQ(cardano_drep_get_credential(drep, &credential), CARDANO_SUCCESS);

  cardano_blake2b_hash_t* cred_hash2 = cardano_credential_get_hash(credential);

  EXPECT_EQ(cardano_blake2b_hash_compare(cred_hash, cred_hash2), 0);

  // Cleanup
  cardano_drep_unref(&drep);
  cardano_blake2b_hash_unref(&cred_hash);
  cardano_blake2b_hash_unref(&cred_hash2);
  cardano_credential_unref(&credential);
}

TEST(cardano_drep_from_string, returnsErrorIfGivenNull)
{
  EXPECT_EQ(cardano_drep_from_string(nullptr, 0, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_drep_from_string("", 0, nullptr), CARDANO_ERROR_INVALID_ADDRESS_FORMAT);
  EXPECT_EQ(cardano_drep_from_string("1", 1, nullptr), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_drep_from_string, returnsErrorIfInvalidBech32String)
{
  cardano_drep_t* drep = nullptr;
  EXPECT_EQ(cardano_drep_from_string("1", 1, &drep), CARDANO_ERROR_INVALID_ADDRESS_FORMAT);
}

TEST(cardano_drep_from_string, returnsErrorIfInvalidPrefix)
{
  cardano_drep_t* drep    = nullptr;
  const char*     address = "addr1z8phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc3sq835lu7drv2xwl2wywfgs9yc0hh";

  EXPECT_EQ(cardano_drep_from_string(address, strlen(address), &drep), CARDANO_ERROR_INVALID_ADDRESS_FORMAT);
}

TEST(cardano_drep_from_string, returnsErrorIfInvalidHashSize)
{
  cardano_drep_t* drep = nullptr;
  EXPECT_EQ(cardano_drep_from_string(DREP_INVALID_HASH_SIZE, strlen(DREP_INVALID_HASH_SIZE), &drep), CARDANO_ERROR_INVALID_ADDRESS_FORMAT);
}

TEST(cardano_drep_from_string, returnsErrorIfInvalidKeyType)
{
  cardano_drep_t* drep = nullptr;
  EXPECT_EQ(cardano_drep_from_string(DREP_INVALID_KEY_TYPE, strlen(DREP_INVALID_KEY_TYPE), &drep), CARDANO_ERROR_INVALID_ADDRESS_FORMAT);
}

TEST(cardano_drep_from_string, returnsErrorIfInvalidGovKeyType)
{
  cardano_drep_t* drep = nullptr;
  EXPECT_EQ(cardano_drep_from_string(DREP_INVALID_GOV_KEY_TYPE, strlen(DREP_INVALID_GOV_KEY_TYPE), &drep), CARDANO_ERROR_INVALID_ADDRESS_FORMAT);
}

TEST(cardano_drep_from_string, returnsErrorIfMemoryAllocationFailsCip105)
{
  for (int i = 0; i < 15; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_drep_t* drep = nullptr;
    EXPECT_NE(cardano_drep_from_string(DREP_CIP105_SCRIPT_HASH, strlen(DREP_CIP105_SCRIPT_HASH), &drep), CARDANO_SUCCESS);

    cardano_drep_unref(&drep);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }
}

TEST(cardano_drep_from_string, returnsErrorIfMemoryAllocationFailsCip129)
{
  for (int i = 0; i < 15; ++i)
  {
    reset_allocators_run_count();
    set_malloc_limit(i);
    cardano_set_allocators(fail_malloc_at_limit, realloc, free);

    cardano_drep_t* drep = nullptr;
    EXPECT_NE(cardano_drep_from_string(DREP_CIP129_KEY_HASH, strlen(DREP_CIP129_KEY_HASH), &drep), CARDANO_SUCCESS);

    cardano_drep_unref(&drep);

    reset_allocators_run_count();
    reset_limited_malloc();
    cardano_set_allocators(malloc, realloc, free);
  }
}

TEST(cardano_drep_get_string_size, returnsZeroIfDrepIsNull)
{
  EXPECT_EQ(cardano_drep_get_string_size(nullptr), 0);
}

TEST(cardano_drep_get_string_size, returnsZeroIfNotKeyHashOrScriptHash)
{
  cardano_drep_t* drep = nullptr;

  EXPECT_EQ(cardano_drep_new(CARDANO_DREP_TYPE_ABSTAIN, NULL, &drep), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_drep_get_string_size(drep), 0);
  cardano_drep_unref(&drep);
}

TEST(cardano_drep_get_string_size, getsCorrectStringSize)
{
  cardano_drep_t* drep = nullptr;

  EXPECT_EQ(cardano_drep_from_string(DREP_CIP129_KEY_HASH, strlen(DREP_CIP129_KEY_HASH), &drep), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_drep_get_string_size(drep), strlen(DREP_CIP129_KEY_HASH) + 1);
  cardano_drep_unref(&drep);

  EXPECT_EQ(cardano_drep_from_string(DREP_CIP129_SCRIPT_HASH, strlen(DREP_CIP129_SCRIPT_HASH), &drep), CARDANO_SUCCESS);
  EXPECT_EQ(cardano_drep_get_string_size(drep), strlen(DREP_CIP129_SCRIPT_HASH) + 1);
  cardano_drep_unref(&drep);
}

TEST(cardano_drep_to_string, returnsErrorIfDrepIsNull)
{
  cardano_drep_t* drep        = nullptr;
  char            message[99] = { 0 };

  EXPECT_EQ(cardano_drep_from_string(DREP_CIP129_KEY_HASH, strlen(DREP_CIP129_KEY_HASH), &drep), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_drep_to_string(nullptr, nullptr, 0), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_drep_to_string(drep, nullptr, 0), CARDANO_ERROR_POINTER_IS_NULL);
  EXPECT_EQ(cardano_drep_to_string(drep, &message[0], 0), CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE);

  EXPECT_EQ(cardano_drep_set_type(drep, CARDANO_DREP_TYPE_ABSTAIN), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_drep_to_string(drep, &message[0], 99), CARDANO_ERROR_INVALID_ARGUMENT);

  cardano_drep_unref(&drep);
}

TEST(cardano_drep_to_string, canConvertToString)
{
  cardano_drep_t* drep        = nullptr;
  char            message[99] = { 0 };

  EXPECT_EQ(cardano_drep_from_string(DREP_CIP129_KEY_HASH, strlen(DREP_CIP129_KEY_HASH), &drep), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_drep_to_string(drep, &message[0], 99), CARDANO_SUCCESS);
  EXPECT_STREQ(message, DREP_CIP129_KEY_HASH);

  cardano_drep_unref(&drep);

  EXPECT_EQ(cardano_drep_from_string(DREP_CIP129_SCRIPT_HASH, strlen(DREP_CIP129_SCRIPT_HASH), &drep), CARDANO_SUCCESS);

  EXPECT_EQ(cardano_drep_to_string(drep, &message[0], 99), CARDANO_SUCCESS);
  EXPECT_STREQ(message, DREP_CIP129_SCRIPT_HASH);

  cardano_drep_unref(&drep);
}