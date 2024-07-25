/**
 * \file pool_registration_cert.cpp
 *
 * \author angel.castillo
 * \date   Aug 05, 2024
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
#include <cardano/certs/pool_registration_cert.h>
#include <cardano/crypto/blake2b_hash.h>

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR             = "8a03581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5";
static const char* POOL_PARAMS_CBOR = "581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the certificate.
 * @return A new instance of the certificate.
 */
static cardano_pool_registration_cert_t*
new_default_cert()
{
  cardano_pool_registration_cert_t* pool_registration_cert = NULL;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t                   result                 = cardano_pool_registration_cert_from_cbor(reader, &pool_registration_cert);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return pool_registration_cert;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_pool_registration_cert_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_pool_registration_cert_t* pool_registration_cert = new_default_cert();
  EXPECT_NE(pool_registration_cert, nullptr);

  // Act
  cardano_pool_registration_cert_ref(pool_registration_cert);

  // Assert
  EXPECT_THAT(pool_registration_cert, testing::Not((cardano_pool_registration_cert_t*)nullptr));
  EXPECT_EQ(cardano_pool_registration_cert_refcount(pool_registration_cert), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_pool_registration_cert_unref(&pool_registration_cert);
  cardano_pool_registration_cert_unref(&pool_registration_cert);
}

TEST(cardano_pool_registration_cert_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_pool_registration_cert_ref(nullptr);
}

TEST(cardano_pool_registration_cert_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_pool_registration_cert_t* pool_registration_cert = nullptr;

  // Act
  cardano_pool_registration_cert_unref(&pool_registration_cert);
}

TEST(cardano_pool_registration_cert_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_pool_registration_cert_unref((cardano_pool_registration_cert_t**)nullptr);
}

TEST(cardano_pool_registration_cert_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_pool_registration_cert_t* pool_registration_cert = new_default_cert();
  EXPECT_NE(pool_registration_cert, nullptr);

  // Act
  cardano_pool_registration_cert_ref(pool_registration_cert);
  size_t ref_count = cardano_pool_registration_cert_refcount(pool_registration_cert);

  cardano_pool_registration_cert_unref(&pool_registration_cert);
  size_t updated_ref_count = cardano_pool_registration_cert_refcount(pool_registration_cert);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_pool_registration_cert_unref(&pool_registration_cert);
}

TEST(cardano_pool_registration_cert_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_pool_registration_cert_t* pool_registration_cert = new_default_cert();
  EXPECT_NE(pool_registration_cert, nullptr);

  // Act
  cardano_pool_registration_cert_ref(pool_registration_cert);
  size_t ref_count = cardano_pool_registration_cert_refcount(pool_registration_cert);

  cardano_pool_registration_cert_unref(&pool_registration_cert);
  size_t updated_ref_count = cardano_pool_registration_cert_refcount(pool_registration_cert);

  cardano_pool_registration_cert_unref(&pool_registration_cert);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(pool_registration_cert, (cardano_pool_registration_cert_t*)nullptr);

  // Cleanup
  cardano_pool_registration_cert_unref(&pool_registration_cert);
}

TEST(cardano_pool_registration_cert_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_pool_registration_cert_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_pool_registration_cert_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_pool_registration_cert_t* pool_registration_cert = nullptr;
  const char*                       message                = "This is a test message";

  // Act
  cardano_pool_registration_cert_set_last_error(pool_registration_cert, message);

  // Assert
  EXPECT_STREQ(cardano_pool_registration_cert_get_last_error(pool_registration_cert), "Object is NULL.");
}

TEST(cardano_pool_registration_cert_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_pool_registration_cert_t* pool_registration_cert = new_default_cert();
  EXPECT_NE(pool_registration_cert, nullptr);

  const char* message = nullptr;

  // Act
  cardano_pool_registration_cert_set_last_error(pool_registration_cert, message);

  // Assert
  EXPECT_STREQ(cardano_pool_registration_cert_get_last_error(pool_registration_cert), "");

  // Cleanup
  cardano_pool_registration_cert_unref(&pool_registration_cert);
}

TEST(cardano_pool_registration_cert_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_pool_registration_cert_t* pool_registration_cert = NULL;

  // Act
  cardano_error_t result = cardano_pool_registration_cert_from_cbor(nullptr, &pool_registration_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_registration_cert_from_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_pool_registration_cert_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_registration_cert_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t*            writer = cardano_cbor_writer_new();
  cardano_pool_registration_cert_t* cert   = new_default_cert();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_pool_registration_cert_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_pool_registration_cert_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_pool_registration_cert_to_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_pool_registration_cert_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_pool_registration_cert_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_pool_registration_cert_to_cbor((cardano_pool_registration_cert_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

// Cert specific tests

TEST(cardano_pool_registration_cert_new, canCreateNewInstance)
{
  // Act
  cardano_pool_params_t* params = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(POOL_PARAMS_CBOR, strlen(POOL_PARAMS_CBOR));

  EXPECT_EQ(cardano_pool_params_from_cbor(reader, &params), CARDANO_SUCCESS);

  cardano_pool_registration_cert_t* pool_registration_cert = NULL;

  cardano_error_t result = cardano_pool_registration_cert_new(params, &pool_registration_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(pool_registration_cert, nullptr);

  // Cleanup
  cardano_pool_registration_cert_unref(&pool_registration_cert);
  cardano_pool_params_unref(&params);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_registration_cert_new, returnsErrorIfFirstArgIsNull)
{
  // Act
  cardano_pool_registration_cert_t* pool_registration_cert = NULL;

  cardano_error_t result = cardano_pool_registration_cert_new(nullptr, &pool_registration_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_registration_cert_new, returnsErrorIfCertIsNull)
{
  // Act
  cardano_error_t result = cardano_pool_registration_cert_new((cardano_pool_params_t*)"", nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_registration_cert_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_pool_params_t* params = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(POOL_PARAMS_CBOR, strlen(POOL_PARAMS_CBOR));

  EXPECT_EQ(cardano_pool_params_from_cbor(reader, &params), CARDANO_SUCCESS);

  cardano_pool_registration_cert_t* pool_registration_cert = NULL;

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);
  cardano_error_t result = cardano_pool_registration_cert_new(params, &pool_registration_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_pool_params_unref(&params);
  cardano_cbor_reader_unref(&reader);
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_pool_registration_cert_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_pool_registration_cert_t* pool_registration_cert = NULL;
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_pool_registration_cert_from_cbor(reader, &pool_registration_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_pool_registration_cert_unref(&pool_registration_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_registration_cert_from_cbor, returnsErrorIfInvalidUintAsType)
{
  // Arrange
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("8aef", strlen("8aef"));
  cardano_pool_registration_cert_t* pool_registration_cert = NULL;

  // Act
  cardano_error_t result = cardano_pool_registration_cert_from_cbor(reader, &pool_registration_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_registration_cert_from_cbor, returnsErrorIfInvalidCertType)
{
  // Arrange
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("8a00", strlen("8a00"));
  cardano_pool_registration_cert_t* pool_registration_cert = NULL;

  // Act
  cardano_error_t result = cardano_pool_registration_cert_from_cbor(reader, &pool_registration_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INVALID_CBOR_VALUE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_registration_cert_from_cbor, returnsErrorIfInvalidPoolParams)
{
  // Arrange
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex("8a03ef1cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5", strlen("8a03581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5"));
  cardano_pool_registration_cert_t* pool_registration_cert = NULL;

  // Act
  cardano_error_t result = cardano_pool_registration_cert_from_cbor(reader, &pool_registration_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

// Getters and Setters

TEST(cardano_pool_registration_cert_get_params, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_pool_params_t* params = NULL;

  // Act
  cardano_error_t result = cardano_pool_registration_cert_get_params(nullptr, &params);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_registration_cert_get_params, returnsErrorIfParamsIsNull)
{
  // Arrange
  cardano_pool_registration_cert_t* pool_registration_cert = new_default_cert();
  EXPECT_NE(pool_registration_cert, nullptr);

  // Act
  cardano_error_t result = cardano_pool_registration_cert_get_params(pool_registration_cert, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_registration_cert_unref(&pool_registration_cert);
}

TEST(cardano_pool_registration_cert_get_params, canGetParams)
{
  // Arrange
  cardano_pool_registration_cert_t* pool_registration_cert = new_default_cert();
  EXPECT_NE(pool_registration_cert, nullptr);

  cardano_pool_params_t* params = NULL;

  // Act
  cardano_error_t result = cardano_pool_registration_cert_get_params(pool_registration_cert, &params);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(params, nullptr);

  // Cleanup
  cardano_pool_params_unref(&params);
  cardano_pool_registration_cert_unref(&pool_registration_cert);
}

TEST(cardano_pool_registration_cert_set_params, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_pool_params_t* params = NULL;

  // Act
  cardano_error_t result = cardano_pool_registration_cert_set_params(nullptr, params);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_pool_registration_cert_set_params, returnsErrorIfParamsIsNull)
{
  // Arrange
  cardano_pool_registration_cert_t* pool_registration_cert = new_default_cert();
  EXPECT_NE(pool_registration_cert, nullptr);

  // Act
  cardano_error_t result = cardano_pool_registration_cert_set_params(pool_registration_cert, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_registration_cert_unref(&pool_registration_cert);
}

TEST(cardano_pool_registration_cert_set_params, canSetParams)
{
  // Arrange
  cardano_pool_registration_cert_t* pool_registration_cert = new_default_cert();
  EXPECT_NE(pool_registration_cert, nullptr);

  cardano_pool_params_t* params = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(POOL_PARAMS_CBOR, strlen(POOL_PARAMS_CBOR));

  EXPECT_EQ(cardano_pool_params_from_cbor(reader, &params), CARDANO_SUCCESS);

  // Act
  cardano_error_t result = cardano_pool_registration_cert_set_params(pool_registration_cert, params);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_pool_params_unref(&params);
  cardano_pool_registration_cert_unref(&pool_registration_cert);
  cardano_cbor_reader_unref(&reader);
}
