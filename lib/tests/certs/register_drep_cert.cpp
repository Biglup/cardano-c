/**
 * \file register_drep_cert.cpp
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
#include <cardano/certs/register_drep_cert.h>
#include <cardano/common/anchor.h>

#include "../json_helpers.h"
#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR             = "84108200581c0000000000000000000000000000000000000000000000000000000000f6";
static const char* CBOR_WITH_ANCHOR = "84108200581c0000000000000000000000000000000000000000000000000000000000827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* CREDENTIAL_CBOR  = "8200581c00000000000000000000000000000000000000000000000000000000";
static const char* ANCHOR_CBOR      = "827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the certificate.
 * @return A new instance of the certificate.
 */
static cardano_register_drep_cert_t*
new_default_cert()
{
  cardano_register_drep_cert_t* register_drep_cert = NULL;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t               result             = cardano_register_drep_cert_from_cbor(reader, &register_drep_cert);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return register_drep_cert;
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

TEST(cardano_register_drep_cert_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_register_drep_cert_t* register_drep_cert = new_default_cert();
  EXPECT_NE(register_drep_cert, nullptr);

  // Act
  cardano_register_drep_cert_ref(register_drep_cert);

  // Assert
  EXPECT_THAT(register_drep_cert, testing::Not((cardano_register_drep_cert_t*)nullptr));
  EXPECT_EQ(cardano_register_drep_cert_refcount(register_drep_cert), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_register_drep_cert_unref(&register_drep_cert);
  cardano_register_drep_cert_unref(&register_drep_cert);
}

TEST(cardano_register_drep_cert_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_register_drep_cert_ref(nullptr);
}

TEST(cardano_register_drep_cert_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_register_drep_cert_t* register_drep_cert = nullptr;

  // Act
  cardano_register_drep_cert_unref(&register_drep_cert);
}

TEST(cardano_register_drep_cert_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_register_drep_cert_unref((cardano_register_drep_cert_t**)nullptr);
}

TEST(cardano_register_drep_cert_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_register_drep_cert_t* register_drep_cert = new_default_cert();
  EXPECT_NE(register_drep_cert, nullptr);

  // Act
  cardano_register_drep_cert_ref(register_drep_cert);
  size_t ref_count = cardano_register_drep_cert_refcount(register_drep_cert);

  cardano_register_drep_cert_unref(&register_drep_cert);
  size_t updated_ref_count = cardano_register_drep_cert_refcount(register_drep_cert);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_register_drep_cert_unref(&register_drep_cert);
}

TEST(cardano_register_drep_cert_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_register_drep_cert_t* register_drep_cert = new_default_cert();
  EXPECT_NE(register_drep_cert, nullptr);

  // Act
  cardano_register_drep_cert_ref(register_drep_cert);
  size_t ref_count = cardano_register_drep_cert_refcount(register_drep_cert);

  cardano_register_drep_cert_unref(&register_drep_cert);
  size_t updated_ref_count = cardano_register_drep_cert_refcount(register_drep_cert);

  cardano_register_drep_cert_unref(&register_drep_cert);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(register_drep_cert, (cardano_register_drep_cert_t*)nullptr);

  // Cleanup
  cardano_register_drep_cert_unref(&register_drep_cert);
}

TEST(cardano_register_drep_cert_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_register_drep_cert_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_register_drep_cert_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_register_drep_cert_t* register_drep_cert = nullptr;
  const char*                   message            = "This is a test message";

  // Act
  cardano_register_drep_cert_set_last_error(register_drep_cert, message);

  // Assert
  EXPECT_STREQ(cardano_register_drep_cert_get_last_error(register_drep_cert), "Object is NULL.");
}

TEST(cardano_register_drep_cert_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_register_drep_cert_t* register_drep_cert = new_default_cert();
  EXPECT_NE(register_drep_cert, nullptr);

  const char* message = nullptr;

  // Act
  cardano_register_drep_cert_set_last_error(register_drep_cert, message);

  // Assert
  EXPECT_STREQ(cardano_register_drep_cert_get_last_error(register_drep_cert), "");

  // Cleanup
  cardano_register_drep_cert_unref(&register_drep_cert);
}

TEST(cardano_register_drep_cert_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_register_drep_cert_t* register_drep_cert = NULL;

  // Act
  cardano_error_t result = cardano_register_drep_cert_from_cbor(nullptr, &register_drep_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_register_drep_cert_from_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_register_drep_cert_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_register_drep_cert_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t*        writer = cardano_cbor_writer_new();
  cardano_register_drep_cert_t* cert   = new_default_cert();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_register_drep_cert_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_register_drep_cert_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_register_drep_cert_to_cbor, canSerializeWithAnchor)
{
  // Arrange
  cardano_cbor_writer_t*        writer = cardano_cbor_writer_new();
  cardano_register_drep_cert_t* cert   = new_default_cert();
  EXPECT_NE(cert, nullptr);

  cardano_anchor_t*      anchor = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));

  cardano_error_t result = cardano_anchor_from_cbor(reader, &anchor);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  EXPECT_EQ(cardano_register_drep_cert_set_anchor(cert, anchor), CARDANO_SUCCESS);

  // Act
  result = cardano_register_drep_cert_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_WITH_ANCHOR);

  // Cleanup
  cardano_register_drep_cert_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  cardano_anchor_unref(&anchor);
  cardano_cbor_reader_unref(&reader);
  free(hex);
}

TEST(cardano_register_drep_cert_to_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_register_drep_cert_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_register_drep_cert_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_register_drep_cert_to_cbor((cardano_register_drep_cert_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

// Cert specific tests

TEST(cardano_register_drep_cert_new, canCreateNewInstance)
{
  // Act
  cardano_credential_t* cred = new_default_cred();

  cardano_register_drep_cert_t* register_drep_cert = NULL;

  cardano_error_t result = cardano_register_drep_cert_new(cred, 0, NULL, &register_drep_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(register_drep_cert, nullptr);

  // Cleanup
  cardano_register_drep_cert_unref(&register_drep_cert);
  cardano_credential_unref(&cred);
}

TEST(cardano_register_drep_cert_new, canCreateNewInstancewithAnchor)
{
  // Act
  cardano_credential_t*  cred   = new_default_cred();
  cardano_anchor_t*      anchor = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));

  cardano_register_drep_cert_t* register_drep_cert = NULL;

  cardano_error_t result = cardano_anchor_from_cbor(reader, &anchor);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_register_drep_cert_new(cred, 0, anchor, &register_drep_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(register_drep_cert, nullptr);

  // Cleanup
  cardano_register_drep_cert_unref(&register_drep_cert);
  cardano_credential_unref(&cred);
  cardano_anchor_unref(&anchor);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_register_drep_cert_new, returnsErrorIfFirstArgIsNull)
{
  // Act
  cardano_register_drep_cert_t* register_drep_cert = NULL;

  cardano_error_t result = cardano_register_drep_cert_new(nullptr, 0, NULL, &register_drep_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_register_drep_cert_new, returnsErrorIfCertIsNull)
{
  // Act

  cardano_error_t result = cardano_register_drep_cert_new((cardano_credential_t*)"", 0, NULL, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
}

TEST(cardano_register_drep_cert_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_credential_t* cred = new_default_cred();

  // Act
  cardano_register_drep_cert_t* register_drep_cert = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_register_drep_cert_new(cred, 0, NULL, &register_drep_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_credential_unref(&cred);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_register_drep_cert_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_register_drep_cert_t* register_drep_cert = NULL;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_register_drep_cert_from_cbor(reader, &register_drep_cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_register_drep_cert_unref(&register_drep_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_register_drep_cert_from_cbor, returnsErrorIfInvalidUintAsType)
{
  // Arrange
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex("84ef", strlen("84ef"));
  cardano_register_drep_cert_t* register_drep_cert = NULL;

  // Act
  cardano_error_t result = cardano_register_drep_cert_from_cbor(reader, &register_drep_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_register_drep_cert_from_cbor, returnsErrorIfInvalidFirstCredential)
{
  // Arrange
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex("8410ef00581c0000000000000000000000000000000000000000000000000000000000f6", strlen("84108200581c0000000000000000000000000000000000000000000000000000000000f6"));
  cardano_register_drep_cert_t* register_drep_cert = NULL;

  // Act
  cardano_error_t result = cardano_register_drep_cert_from_cbor(reader, &register_drep_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_register_drep_cert_from_cbor, returnsErrorIfInvalidDeposit)
{
  // Arrange
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex("84108200581c00000000000000000000000000000000000000000000000000000000eff6", strlen("84108200581c0000000000000000000000000000000000000000000000000000000000f6"));
  cardano_register_drep_cert_t* register_drep_cert = NULL;

  // Act
  cardano_error_t result = cardano_register_drep_cert_from_cbor(reader, &register_drep_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_register_drep_cert_from_cbor, returnsErrorIfInvalidAnchor)
{
  // Arrange
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex("84108200581c0000000000000000000000000000000000000000000000000000000000ef", strlen("84108200581c0000000000000000000000000000000000000000000000000000000000f6"));
  cardano_register_drep_cert_t* register_drep_cert = NULL;

  // Act
  cardano_error_t result = cardano_register_drep_cert_from_cbor(reader, &register_drep_cert);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

// Getters and Setters

TEST(cardano_register_drep_cert_set_credential, canSetCredential)
{
  // Arrange
  cardano_register_drep_cert_t* register_drep_cert = new_default_cert();
  cardano_credential_t*         cred               = new_default_cred();

  // Act
  cardano_error_t result = cardano_register_drep_cert_set_credential(register_drep_cert, cred);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_register_drep_cert_unref(&register_drep_cert);
  cardano_credential_unref(&cred);
}

TEST(cardano_register_drep_cert_set_credential, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_credential_t* cred = new_default_cred();

  // Act
  cardano_error_t result = cardano_register_drep_cert_set_credential(nullptr, cred);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_credential_unref(&cred);
}

TEST(cardano_register_drep_cert_set_credential, returnsErrorIfCredentialIsNull)
{
  // Arrange
  cardano_register_drep_cert_t* register_drep_cert = new_default_cert();

  // Act
  cardano_error_t result = cardano_register_drep_cert_set_credential(register_drep_cert, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_register_drep_cert_unref(&register_drep_cert);
}

TEST(cardano_register_drep_cert_get_credential, canGetCredential)
{
  // Arrange
  cardano_register_drep_cert_t* register_drep_cert = new_default_cert();
  cardano_credential_t*         cred               = new_default_cred();

  EXPECT_EQ(cardano_register_drep_cert_set_credential(register_drep_cert, cred), CARDANO_SUCCESS);

  // Act
  cardano_credential_t* cred2 = cardano_register_drep_cert_get_credential(register_drep_cert);

  // Assert
  EXPECT_NE(cred2, nullptr);

  // Cleanup
  cardano_register_drep_cert_unref(&register_drep_cert);
  cardano_credential_unref(&cred);
  cardano_credential_unref(&cred2);
}

TEST(cardano_register_drep_cert_get_credential, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_credential_t* cred = cardano_register_drep_cert_get_credential(nullptr);

  // Assert
  EXPECT_EQ(cred, nullptr);
}

// Setter and getter

TEST(cardano_register_drep_cert_get_deposit, canGetDeposit)
{
  // Arrange
  cardano_register_drep_cert_t* register_drep_cert = new_default_cert();

  // Act
  uint64_t deposit = cardano_register_drep_cert_get_deposit(register_drep_cert);

  // Assert
  EXPECT_EQ(deposit, 0);

  // Cleanup
  cardano_register_drep_cert_unref(&register_drep_cert);
}

TEST(cardano_register_drep_cert_get_deposit, returnsErrorIfObjectIsNull)
{
  // Act
  uint64_t deposit = cardano_register_drep_cert_get_deposit(nullptr);

  // Assert
  EXPECT_EQ(deposit, 0);
}

TEST(cardano_register_drep_cert_set_deposit, canSetDeposit)
{
  // Arrange
  cardano_register_drep_cert_t* register_drep_cert = new_default_cert();

  // Act
  cardano_error_t result = cardano_register_drep_cert_set_deposit(register_drep_cert, 100);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_register_drep_cert_unref(&register_drep_cert);
}

TEST(cardano_register_drep_cert_set_deposit, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_error_t result = cardano_register_drep_cert_set_deposit(nullptr, 100);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_register_drep_cert_get_anchor, canGetAnchor)
{
  // Arrange
  cardano_register_drep_cert_t* register_drep_cert = new_default_cert();

  // Act
  cardano_anchor_t* anchor = cardano_register_drep_cert_get_anchor(register_drep_cert);

  // Assert
  EXPECT_EQ(anchor, nullptr);

  // Cleanup
  cardano_register_drep_cert_unref(&register_drep_cert);
}

TEST(cardano_register_drep_cert_set_anchor, canSetAnchor)
{
  // Arrange
  cardano_register_drep_cert_t* register_drep_cert = new_default_cert();
  cardano_anchor_t*             anchor             = NULL;
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));

  cardano_error_t result = cardano_anchor_from_cbor(reader, &anchor);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_register_drep_cert_set_anchor(register_drep_cert, anchor);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_register_drep_cert_unref(&register_drep_cert);
  cardano_anchor_unref(&anchor);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_register_drep_cert_set_anchor, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_anchor_t*      anchor = NULL;
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));

  cardano_error_t result = cardano_anchor_from_cbor(reader, &anchor);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Act
  result = cardano_register_drep_cert_set_anchor(nullptr, anchor);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_anchor_unref(&anchor);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_register_drep_cert_set_anchor, returnsErrorIfAnchorIsNull)
{
  // Arrange
  cardano_register_drep_cert_t* register_drep_cert = new_default_cert();

  // Act
  cardano_error_t result = cardano_register_drep_cert_set_anchor(register_drep_cert, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_register_drep_cert_unref(&register_drep_cert);
}

TEST(cardano_register_drep_cert_get_anchor, returnsErrorIfObjectIsNull)
{
  // Act
  cardano_anchor_t* anchor = cardano_register_drep_cert_get_anchor(nullptr);

  // Assert
  EXPECT_EQ(anchor, nullptr);
}

TEST(cardano_register_drep_cert_to_cip116_json, canConvertToCip116JsonWithAnchor)
{
  // Arrange
  cardano_error_t error = CARDANO_SUCCESS;

  // Credential
  cardano_register_drep_cert_t* cert   = new_default_cert();
  cardano_anchor_t*             anchor = NULL;
  cardano_cbor_reader_t*        reader = cardano_cbor_reader_from_hex(ANCHOR_CBOR, strlen(ANCHOR_CBOR));

  cardano_error_t result = cardano_anchor_from_cbor(reader, &anchor);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_register_drep_cert_set_anchor(cert, anchor);
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Act
  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  error          = cardano_register_drep_cert_to_cip116_json(cert, json);
  char* json_str = encode_json(json);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  const char* expected = R"({"tag":"register_drep","drep_credential":{"tag":"pubkey_hash","value":"00000000000000000000000000000000000000000000000000000000"},"coin":"0","anchor":{"url":"https://www.someurl.io","data_hash":"0000000000000000000000000000000000000000000000000000000000000000"}})";
  EXPECT_STREQ(json_str, expected);

  // Cleanup
  cardano_json_writer_unref(&json);
  cardano_register_drep_cert_unref(&cert);
  cardano_anchor_unref(&anchor);
  free(json_str);
}

TEST(cardano_register_drep_cert_to_cip116_json, canConvertToCip116JsonWithoutAnchor)
{
  // Arrange
  cardano_error_t error = CARDANO_SUCCESS;

  cardano_register_drep_cert_t* cert = new_default_cert();

  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  error          = cardano_register_drep_cert_to_cip116_json(cert, json);
  char* json_str = encode_json(json);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(json_str, R"({"tag":"register_drep","drep_credential":{"tag":"pubkey_hash","value":"00000000000000000000000000000000000000000000000000000000"},"coin":"0","anchor":null})");

  // Cleanup
  cardano_json_writer_unref(&json);
  cardano_register_drep_cert_unref(&cert);
  free(json_str);
}

TEST(cardano_register_drep_cert_to_cip116_json, returnsErrorIfCertIsNull)
{
  cardano_json_writer_t* json  = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);
  cardano_error_t        error = cardano_register_drep_cert_to_cip116_json(nullptr, json);
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_json_writer_unref(&json);
}

TEST(cardano_register_drep_cert_to_cip116_json, returnsErrorIfWriterIsNull)
{
  // Arrange
  cardano_register_drep_cert_t* cert = new_default_cert();
  // Act
  cardano_error_t error = cardano_register_drep_cert_to_cip116_json(cert, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_register_drep_cert_unref(&cert);
}