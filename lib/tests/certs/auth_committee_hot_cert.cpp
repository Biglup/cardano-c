/**
 * \file auth_committee_hot_cert.cpp
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

#include <cardano/cbor/cbor_reader.h>
#include <cardano/certs/auth_committee_hot_cert.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR            = "830e8200581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000";
static const char* CREDENTIAL_CBOR = "8200581c00000000000000000000000000000000000000000000000000000000";
static const char* CREDENTIAL_HASH = "00000000000000000000000000000000000000000000000000000000";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the certificate.
 * @return A new instance of the certificate.
 */
static cardano_auth_committee_hot_cert_t*
new_default_cert()
{
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t                    result                  = cardano_auth_committee_hot_cert_from_cbor(reader, &auth_committee_hot_cert);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return auth_committee_hot_cert;
};

/**
 * Creates a new default instance of the credential.
 *
 * @return A new instance of the credential.
 */
static cardano_credential_t*
new_default_cred()
{
  cardano_credential_t*  cred   = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CREDENTIAL_CBOR, strlen(CREDENTIAL_CBOR));
  cardano_error_t        result = cardano_credential_from_cbor(reader, &cred);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return cred;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_auth_committee_hot_cert_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = new_default_cert();
  EXPECT_NE(auth_committee_hot_cert, nullptr);

  // Act
  cardano_auth_committee_hot_cert_ref(auth_committee_hot_cert);

  // Assert
  EXPECT_THAT(auth_committee_hot_cert, testing::Not((cardano_auth_committee_hot_cert_t*)nullptr));
  EXPECT_EQ(cardano_auth_committee_hot_cert_refcount(auth_committee_hot_cert), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
}

TEST(cardano_auth_committee_hot_cert_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_auth_committee_hot_cert_ref(nullptr);
}

TEST(cardano_auth_committee_hot_cert_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = nullptr;

  // Act
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
}

TEST(cardano_auth_committee_hot_cert_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_auth_committee_hot_cert_unref((cardano_auth_committee_hot_cert_t**)nullptr);
}

TEST(cardano_auth_committee_hot_cert_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = new_default_cert();
  EXPECT_NE(auth_committee_hot_cert, nullptr);

  // Act
  cardano_auth_committee_hot_cert_ref(auth_committee_hot_cert);
  size_t ref_count = cardano_auth_committee_hot_cert_refcount(auth_committee_hot_cert);

  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
  size_t updated_ref_count = cardano_auth_committee_hot_cert_refcount(auth_committee_hot_cert);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
}

TEST(cardano_auth_committee_hot_cert_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = new_default_cert();
  EXPECT_NE(auth_committee_hot_cert, nullptr);

  // Act
  cardano_auth_committee_hot_cert_ref(auth_committee_hot_cert);
  size_t ref_count = cardano_auth_committee_hot_cert_refcount(auth_committee_hot_cert);

  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
  size_t updated_ref_count = cardano_auth_committee_hot_cert_refcount(auth_committee_hot_cert);

  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(auth_committee_hot_cert, (cardano_auth_committee_hot_cert_t*)nullptr);

  // Cleanup
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
}

TEST(cardano_auth_committee_hot_cert_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_auth_committee_hot_cert_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_auth_committee_hot_cert_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = nullptr;
  const char*                        message                 = "This is a test message";

  // Act
  cardano_auth_committee_hot_cert_set_last_error(auth_committee_hot_cert, message);

  // Assert
  EXPECT_STREQ(cardano_auth_committee_hot_cert_get_last_error(auth_committee_hot_cert), "Object is NULL.");
}

TEST(cardano_auth_committee_hot_cert_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = new_default_cert();
  EXPECT_NE(auth_committee_hot_cert, nullptr);

  const char* message = nullptr;

  // Act
  cardano_auth_committee_hot_cert_set_last_error(auth_committee_hot_cert, message);

  // Assert
  EXPECT_STREQ(cardano_auth_committee_hot_cert_get_last_error(auth_committee_hot_cert), "");

  // Cleanup
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
}

TEST(cardano_auth_committee_hot_cert_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = NULL;

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_from_cbor(nullptr, &auth_committee_hot_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_auth_committee_hot_cert_from_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auth_committee_hot_cert_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t*             writer = cardano_cbor_writer_new();
  cardano_auth_committee_hot_cert_t* cert   = new_default_cert();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_auth_committee_hot_cert_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_auth_committee_hot_cert_to_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_auth_committee_hot_cert_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_to_cbor((cardano_auth_committee_hot_cert_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

// Cert specific tests

TEST(cardano_auth_committee_hot_cert_new, canCreateNewInstance)
{
  // Act
  cardano_credential_t* hot  = new_default_cred();
  cardano_credential_t* cold = new_default_cred();

  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = NULL;

  cardano_error_t result = cardano_auth_committee_hot_cert_new(hot, cold, &auth_committee_hot_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(auth_committee_hot_cert, nullptr);

  // Cleanup
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
  cardano_credential_unref(&hot);
  cardano_credential_unref(&cold);
}

TEST(cardano_auth_committee_hot_cert_new, returnsErrorIfFirstArgIsNull)
{
  // Arrange
  cardano_credential_t* cold = new_default_cred();

  // Act
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = NULL;

  cardano_error_t result = cardano_auth_committee_hot_cert_new(nullptr, cold, &auth_committee_hot_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&cold);
}

TEST(cardano_auth_committee_hot_cert_new, returnsErrorIfSecondArgIsNull)
{
  // Arrange
  cardano_credential_t* hot = new_default_cred();

  // Act
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = NULL;

  cardano_error_t result = cardano_auth_committee_hot_cert_new(hot, nullptr, &auth_committee_hot_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&hot);
}

TEST(cardano_auth_committee_hot_cert_new, returnsErrorIfThirdArgIsNull)
{
  // Arrange
  cardano_credential_t* hot  = new_default_cred();
  cardano_credential_t* cold = new_default_cred();

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_new(hot, cold, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&hot);
  cardano_credential_unref(&cold);
}

TEST(cardano_auth_committee_hot_cert_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_credential_t* hot  = new_default_cred();
  cardano_credential_t* cold = new_default_cred();

  // Act
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_auth_committee_hot_cert_new(hot, cold, &auth_committee_hot_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_credential_unref(&hot);
  cardano_credential_unref(&cold);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_auth_committee_hot_cert_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = NULL;
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_from_cbor(reader, &auth_committee_hot_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auth_committee_hot_cert_from_cbor, returnsErrorIfInvalidUintAsType)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("83ef", strlen("83ef"));
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = NULL;

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_from_cbor(reader, &auth_committee_hot_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auth_committee_hot_cert_from_cbor, returnsErrorIfInvalidFirstCredential)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("830e82005efc000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000", strlen("830e8200581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000"));
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = NULL;

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_from_cbor(reader, &auth_committee_hot_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auth_committee_hot_cert_from_cbor, returnsErrorIfInvalidSecondCredential)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex("830e8200581c0000000000000000000000000000000000000000000000000000000082005efc00000000000000000000000000000000000000000000000000000000", strlen("830e8200581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000"));
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = NULL;

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_from_cbor(reader, &auth_committee_hot_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

// Getters and Setters

TEST(cardano_auth_committee_hot_cert_set_cold_cred, canSetColdCredential)
{
  // Arrange
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = new_default_cert();
  cardano_credential_t*              cold                    = new_default_cred();

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_set_cold_cred(auth_committee_hot_cert, cold);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
  cardano_credential_unref(&cold);
}

TEST(cardano_auth_committee_hot_cert_set_cold_cred, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_credential_t* cold = new_default_cred();

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_set_cold_cred(nullptr, cold);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&cold);
}

TEST(cardano_auth_committee_hot_cert_set_cold_cred, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = new_default_cert();

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_set_cold_cred(auth_committee_hot_cert, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
}

TEST(cardano_auth_committee_hot_cert_get_cold_cred, canGetColdCredential)
{
  // Arrange
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = new_default_cert();
  cardano_credential_t*              cold                    = new_default_cred();

  EXPECT_EQ(cardano_auth_committee_hot_cert_set_cold_cred(auth_committee_hot_cert, cold), CARDANO_SUCCESS);

  // Act
  cardano_credential_t* cold_out = NULL;
  cardano_error_t       result   = cardano_auth_committee_hot_cert_get_cold_cred(auth_committee_hot_cert, &cold_out);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(cold_out, nullptr);

  // Cleanup
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
  cardano_credential_unref(&cold);
}

TEST(cardano_auth_committee_hot_cert_get_cold_cred, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_credential_t* cold_out = NULL;

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_get_cold_cred(nullptr, &cold_out);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_auth_committee_hot_cert_get_cold_cred, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = new_default_cert();

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_get_cold_cred(auth_committee_hot_cert, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
}

TEST(cardano_auth_committee_hot_cert_set_hot_cred, canSetHotCredential)
{
  // Arrange
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = new_default_cert();
  cardano_credential_t*              hot                     = new_default_cred();

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_set_hot_cred(auth_committee_hot_cert, hot);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
  cardano_credential_unref(&hot);
}

TEST(cardano_auth_committee_hot_cert_set_hot_cred, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_credential_t* hot = new_default_cred();

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_set_hot_cred(nullptr, hot);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&hot);
}

TEST(cardano_auth_committee_hot_cert_set_hot_cred, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = new_default_cert();

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_set_hot_cred(auth_committee_hot_cert, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
}

TEST(cardano_auth_committee_hot_cert_get_hot_cred, canGetHotCredential)
{
  // Arrange
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = new_default_cert();
  cardano_credential_t*              hot                     = new_default_cred();

  EXPECT_EQ(cardano_auth_committee_hot_cert_set_hot_cred(auth_committee_hot_cert, hot), CARDANO_SUCCESS);

  // Act
  cardano_credential_t* hot_out = NULL;
  cardano_error_t       result  = cardano_auth_committee_hot_cert_get_hot_cred(auth_committee_hot_cert, &hot_out);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);
  EXPECT_NE(hot_out, nullptr);

  // Cleanup
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
  cardano_credential_unref(&hot);
}

TEST(cardano_auth_committee_hot_cert_get_hot_cred, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_credential_t* hot_out = NULL;

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_get_hot_cred(nullptr, &hot_out);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_auth_committee_hot_cert_get_hot_cred, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = new_default_cert();

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_get_hot_cred(auth_committee_hot_cert, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
}