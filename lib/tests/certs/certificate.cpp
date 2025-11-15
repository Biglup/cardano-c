/**
 * \file certificate.cpp
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
#include <cardano/certs/certificate.h>
#include <cardano/certs/genesis_key_delegation_cert.h>
#include <cardano/certs/mir_cert.h>
#include <cardano/certs/pool_registration_cert.h>
#include <cardano/certs/pool_retirement_cert.h>
#include <cardano/certs/register_drep_cert.h>
#include <cardano/certs/registration_cert.h>
#include <cardano/certs/resign_committee_cold_cert.h>
#include <cardano/certs/stake_delegation_cert.h>
#include <cardano/certs/stake_deregistration_cert.h>
#include <cardano/certs/stake_registration_cert.h>
#include <cardano/certs/stake_registration_delegation_cert.h>
#include <cardano/certs/stake_vote_delegation_cert.h>
#include <cardano/certs/stake_vote_registration_delegation_cert.h>
#include <cardano/certs/unregister_drep_cert.h>
#include <cardano/certs/unregistration_cert.h>
#include <cardano/certs/update_drep_cert.h>
#include <cardano/certs/vote_delegation_cert.h>
#include <cardano/certs/vote_registration_delegation_cert.h>

#include "../json_helpers.h"
#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR_AUTHORIZE_COMMITTEE_HOT            = "830e8200581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000";
static const char* CBOR_GENESIS_DELEGATION                 = "8405581c00010001000100010001000100010001000100010001000100010001581c0002000200020002000200020002000200020002000200020002000258200003000300030003000300030003000300030003000300030003000300030003";
static const char* CBOR_MIR                                = "820682001a000f4240";
static const char* CBOR_POOL_REGISTRATION                  = "8a03581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef9258208dd154228946bd12967c12bedb1cb6038b78f8b84a1760b1a788fa72a4af3db01927101903e8d81e820105581de1cb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810fd9010281581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f8383011913886b6578616d706c652e636f6d8400191770447f000001f682026b6578616d706c652e636f6d827368747470733a2f2f6578616d706c652e636f6d58200f3abbc8fc19c2e61bab6059bf8a466e6e754833a08a62a6c56fe0e78f19d9d5";
static const char* CBOR_POOL_RETIREMENT                    = "8304581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef921903e8";
static const char* CBOR_REGISTER_DREP                      = "84108200581c0000000000000000000000000000000000000000000000000000000000f6";
static const char* CBOR_REGISTRATION                       = "83078200581c0000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_RESIGN_COMMITTEE_COLD              = "830f8200581c00000000000000000000000000000000000000000000000000000000f6";
static const char* CBOR_STAKE_DELEGATION                   = "83028200581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f581cd85087c646951407198c27b1b950fd2e99f28586c000ce39f6e6ef92";
static const char* CBOR_STAKE_DEREGISTRATION               = "82018200581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f";
static const char* CBOR_STAKE_REGISTRATION                 = "82008200581ccb0ec2692497b458e46812c8a5bfa2931d1a2d965a99893828ec810f";
static const char* CBOR_STAKE_REGISTRATION_DELEGATION      = "840b8200581c00000000000000000000000000000000000000000000000000000000581c0000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_STAKE_VOTE_DELEGATION              = "840a8200581c00000000000000000000000000000000000000000000000000000000581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000";
static const char* CBOR_STAKE_VOTE_REGISTRATION_DELEGATION = "850d8200581c00000000000000000000000000000000000000000000000000000000581c000000000000000000000000000000000000000000000000000000008200581c0000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_UNREGISTER_DREP                    = "83118200581c0000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_UNREGISTRATION                     = "83088200581c0000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_UPDATE_DREP                        = "83128200581c00000000000000000000000000000000000000000000000000000000827668747470733a2f2f7777772e736f6d6575726c2e696f58200000000000000000000000000000000000000000000000000000000000000000";
static const char* CBOR_VOTE_DELEGATION                    = "83098200581c000000000000000000000000000000000000000000000000000000008200581c00000000000000000000000000000000000000000000000000000000";
static const char* CBOR_VOTE_REGISTRATION_DELEGATION       = "840c8200581c000000000000000000000000000000000000000000000000000000008200581c0000000000000000000000000000000000000000000000000000000000";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the certificate.
 * @return A new instance of the certificate.
 */
static cardano_certificate_t*
new_default_cert()
{
  cardano_certificate_t* certificate = NULL;
  cardano_cbor_reader_t* reader      = cardano_cbor_reader_from_hex(CBOR_STAKE_REGISTRATION, strlen(CBOR_STAKE_REGISTRATION));
  cardano_error_t        result      = cardano_certificate_from_cbor(reader, &certificate);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return certificate;
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_certificate_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_certificate_t* certificate = new_default_cert();
  EXPECT_NE(certificate, nullptr);

  // Act
  cardano_certificate_ref(certificate);

  // Assert
  EXPECT_THAT(certificate, testing::Not((cardano_certificate_t*)nullptr));
  EXPECT_EQ(cardano_certificate_refcount(certificate), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_certificate_unref(&certificate);
  cardano_certificate_unref(&certificate);
}

TEST(cardano_certificate_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_certificate_ref(nullptr);
}

TEST(cardano_certificate_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_certificate_t* certificate = nullptr;

  // Act
  cardano_certificate_unref(&certificate);
}

TEST(cardano_certificate_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_certificate_unref((cardano_certificate_t**)nullptr);
}

TEST(cardano_certificate_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_certificate_t* certificate = new_default_cert();
  EXPECT_NE(certificate, nullptr);

  // Act
  cardano_certificate_ref(certificate);
  size_t ref_count = cardano_certificate_refcount(certificate);

  cardano_certificate_unref(&certificate);
  size_t updated_ref_count = cardano_certificate_refcount(certificate);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_certificate_unref(&certificate);
}

TEST(cardano_certificate_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_certificate_t* certificate = new_default_cert();
  EXPECT_NE(certificate, nullptr);

  // Act
  cardano_certificate_ref(certificate);
  size_t ref_count = cardano_certificate_refcount(certificate);

  cardano_certificate_unref(&certificate);
  size_t updated_ref_count = cardano_certificate_refcount(certificate);

  cardano_certificate_unref(&certificate);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(certificate, (cardano_certificate_t*)nullptr);

  // Cleanup
  cardano_certificate_unref(&certificate);
}

TEST(cardano_certificate_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_certificate_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_certificate_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_certificate_t* certificate = nullptr;
  const char*            message     = "This is a test message";

  // Act
  cardano_certificate_set_last_error(certificate, message);

  // Assert
  EXPECT_STREQ(cardano_certificate_get_last_error(certificate), "Object is NULL.");
}

TEST(cardano_certificate_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_certificate_t* certificate = new_default_cert();
  EXPECT_NE(certificate, nullptr);

  const char* message = nullptr;

  // Act
  cardano_certificate_set_last_error(certificate, message);

  // Assert
  EXPECT_STREQ(cardano_certificate_get_last_error(certificate), "");

  // Cleanup
  cardano_certificate_unref(&certificate);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_certificate_t* certificate = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(nullptr, &certificate);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_REGISTRATION, strlen(CBOR_STAKE_REGISTRATION));

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = new_default_cert();
  EXPECT_NE(cert, nullptr);

  // Act
  cardano_error_t result = cardano_certificate_to_cbor(cert, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_STAKE_REGISTRATION);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_certificate_to_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_certificate_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_certificate_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_certificate_to_cbor((cardano_certificate_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

// Cert specific tests

TEST(cardano_certificate_new_auth_committee_hot, canCreate)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR_AUTHORIZE_COMMITTEE_HOT, strlen(CBOR_AUTHORIZE_COMMITTEE_HOT));
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = NULL;
  cardano_certificate_t*             cert                    = NULL;

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_from_cbor(reader, &auth_committee_hot_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_certificate_new_auth_committee_hot(auth_committee_hot_cert, &cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_auth_committee_hot, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_error_t result = cardano_certificate_new_auth_committee_hot(NULL, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_auth_committee_hot, returnsErrorIfCertIsInvalid)
{
  // Act
  cardano_error_t result = cardano_certificate_new_auth_committee_hot((cardano_auth_committee_hot_cert_t*)"", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_auth_committee_hot, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR_AUTHORIZE_COMMITTEE_HOT, strlen(CBOR_AUTHORIZE_COMMITTEE_HOT));
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = NULL;
  cardano_certificate_t*             cert                    = NULL;

  // Act
  cardano_error_t result = cardano_auth_committee_hot_cert_from_cbor(reader, &auth_committee_hot_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_certificate_new_auth_committee_hot(auth_committee_hot_cert, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_genesis_key_delegation, canCreate)
{
  // Arrange
  cardano_cbor_reader_t*                 reader                      = cardano_cbor_reader_from_hex(CBOR_GENESIS_DELEGATION, strlen(CBOR_GENESIS_DELEGATION));
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;
  cardano_certificate_t*                 cert                        = NULL;

  // Act
  cardano_error_t result = cardano_genesis_key_delegation_cert_from_cbor(reader, &genesis_key_delegation_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_certificate_new_genesis_key_delegation(genesis_key_delegation_cert, &cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_genesis_key_delegation, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_error_t result = cardano_certificate_new_genesis_key_delegation(NULL, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_genesis_key_delegation, returnsErrorIfCertIsInvalid)
{
  // Act
  cardano_error_t result = cardano_certificate_new_genesis_key_delegation((cardano_genesis_key_delegation_cert_t*)"", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_genesis_key_delegation, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*                 reader                      = cardano_cbor_reader_from_hex(CBOR_GENESIS_DELEGATION, strlen(CBOR_GENESIS_DELEGATION));
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;
  cardano_certificate_t*                 cert                        = NULL;

  // Act
  cardano_error_t result = cardano_genesis_key_delegation_cert_from_cbor(reader, &genesis_key_delegation_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_certificate_new_genesis_key_delegation(genesis_key_delegation_cert, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_mir, canCreate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_MIR, strlen(CBOR_MIR));
  cardano_mir_cert_t*    mir    = NULL;
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_mir_cert_from_cbor(reader, &mir);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_certificate_new_mir(mir, &cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_mir_cert_unref(&mir);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_mir, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_error_t result = cardano_certificate_new_mir(NULL, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_mir, returnsErrorIfCertIsInvalid)
{
  // Act
  cardano_error_t result = cardano_certificate_new_mir((cardano_mir_cert_t*)"", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_mir, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_MIR, strlen(CBOR_MIR));
  cardano_mir_cert_t*    mir    = NULL;
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_mir_cert_from_cbor(reader, &mir);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_certificate_new_mir(mir, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_mir_cert_unref(&mir);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_pool_registration, canCreate)
{
  // Arrange
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR_POOL_REGISTRATION, strlen(CBOR_POOL_REGISTRATION));
  cardano_pool_registration_cert_t* pool_registration_cert = NULL;
  cardano_certificate_t*            cert                   = NULL;

  // Act
  cardano_error_t result = cardano_pool_registration_cert_from_cbor(reader, &pool_registration_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_certificate_new_pool_registration(pool_registration_cert, &cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_pool_registration_cert_unref(&pool_registration_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_pool_registration, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_error_t result = cardano_certificate_new_pool_registration(NULL, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_pool_registration, returnsErrorIfCertIsInvalid)
{
  // Act
  cardano_error_t result = cardano_certificate_new_pool_registration((cardano_pool_registration_cert_t*)"", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_pool_registration, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*            reader                 = cardano_cbor_reader_from_hex(CBOR_POOL_REGISTRATION, strlen(CBOR_POOL_REGISTRATION));
  cardano_pool_registration_cert_t* pool_registration_cert = NULL;
  cardano_certificate_t*            cert                   = NULL;

  // Act
  cardano_error_t result = cardano_pool_registration_cert_from_cbor(reader, &pool_registration_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_certificate_new_pool_registration(pool_registration_cert, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_pool_registration_cert_unref(&pool_registration_cert);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_pool_retirement, canCreate)
{
  // Arrange
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(CBOR_POOL_RETIREMENT, strlen(CBOR_POOL_RETIREMENT));
  cardano_pool_retirement_cert_t* pool_retirement_cert = NULL;
  cardano_certificate_t*          cert                 = NULL;

  // Act
  cardano_error_t result = cardano_pool_retirement_cert_from_cbor(reader, &pool_retirement_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_certificate_new_pool_retirement(pool_retirement_cert, &cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_pool_retirement_cert_unref(&pool_retirement_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_pool_retirement, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_error_t result = cardano_certificate_new_pool_retirement(NULL, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_pool_retirement, returnsErrorIfCertIsInvalid)
{
  // Act
  cardano_error_t result = cardano_certificate_new_pool_retirement((cardano_pool_retirement_cert_t*)"", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_pool_retirement, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(CBOR_POOL_RETIREMENT, strlen(CBOR_POOL_RETIREMENT));
  cardano_pool_retirement_cert_t* pool_retirement_cert = NULL;
  cardano_certificate_t*          cert                 = NULL;

  // Act
  cardano_error_t result = cardano_pool_retirement_cert_from_cbor(reader, &pool_retirement_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_certificate_new_pool_retirement(pool_retirement_cert, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_pool_retirement_cert_unref(&pool_retirement_cert);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_register_drep, canCreate)
{
  // Arrange
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(CBOR_REGISTER_DREP, strlen(CBOR_REGISTER_DREP));
  cardano_register_drep_cert_t* register_drep_cert = NULL;
  cardano_certificate_t*        cert               = NULL;

  // Act
  cardano_error_t result = cardano_register_drep_cert_from_cbor(reader, &register_drep_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_certificate_new_register_drep(register_drep_cert, &cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_register_drep_cert_unref(&register_drep_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_register_drep, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_error_t result = cardano_certificate_new_register_drep(NULL, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_register_drep, returnsErrorIfCertIsInvalid)
{
  // Act
  cardano_error_t result = cardano_certificate_new_register_drep((cardano_register_drep_cert_t*)"", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_register_drep, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*        reader             = cardano_cbor_reader_from_hex(CBOR_REGISTER_DREP, strlen(CBOR_REGISTER_DREP));
  cardano_register_drep_cert_t* register_drep_cert = NULL;
  cardano_certificate_t*        cert               = NULL;

  // Act
  cardano_error_t result = cardano_register_drep_cert_from_cbor(reader, &register_drep_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_certificate_new_register_drep(register_drep_cert, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_register_drep_cert_unref(&register_drep_cert);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_registration, canCreate)
{
  // Arrange
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_registration_cert_t* registration_cert = NULL;
  cardano_certificate_t*       cert              = NULL;

  // Act
  cardano_error_t result = cardano_registration_cert_from_cbor(reader, &registration_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_certificate_new_registration(registration_cert, &cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_registration_cert_unref(&registration_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_registration, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_error_t result = cardano_certificate_new_registration(NULL, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_registration, returnsErrorIfCertIsInvalid)
{
  // Act
  cardano_error_t result = cardano_certificate_new_registration((cardano_registration_cert_t*)"", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_registration, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_registration_cert_t* registration_cert = NULL;
  cardano_certificate_t*       cert              = NULL;

  // Act
  cardano_error_t result = cardano_registration_cert_from_cbor(reader, &registration_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_certificate_new_registration(registration_cert, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_registration_cert_unref(&registration_cert);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_resign_committee_cold, canCreate)
{
  // Arrange
  cardano_cbor_reader_t*                reader                     = cardano_cbor_reader_from_hex(CBOR_RESIGN_COMMITTEE_COLD, strlen(CBOR_RESIGN_COMMITTEE_COLD));
  cardano_resign_committee_cold_cert_t* resign_committee_cold_cert = NULL;
  cardano_certificate_t*                cert                       = NULL;

  // Act
  cardano_error_t result = cardano_resign_committee_cold_cert_from_cbor(reader, &resign_committee_cold_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_certificate_new_resign_committee_cold(resign_committee_cold_cert, &cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_resign_committee_cold_cert_unref(&resign_committee_cold_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_resign_committee_cold, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_error_t result = cardano_certificate_new_resign_committee_cold(NULL, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_resign_committee_cold, returnsErrorIfCertIsInvalid)
{
  // Act
  cardano_error_t result = cardano_certificate_new_resign_committee_cold((cardano_resign_committee_cold_cert_t*)"", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_resign_committee_cold, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*                reader                     = cardano_cbor_reader_from_hex(CBOR_RESIGN_COMMITTEE_COLD, strlen(CBOR_RESIGN_COMMITTEE_COLD));
  cardano_resign_committee_cold_cert_t* resign_committee_cold_cert = NULL;
  cardano_certificate_t*                cert                       = NULL;

  // Act
  cardano_error_t result = cardano_resign_committee_cold_cert_from_cbor(reader, &resign_committee_cold_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_certificate_new_resign_committee_cold(resign_committee_cold_cert, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_resign_committee_cold_cert_unref(&resign_committee_cold_cert);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_stake_delegation, canCreate)
{
  // Arrange
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR_STAKE_DELEGATION, strlen(CBOR_STAKE_DELEGATION));
  cardano_stake_delegation_cert_t* stake_delegation_cert = NULL;
  cardano_certificate_t*           cert                  = NULL;

  // Act
  cardano_error_t result = cardano_stake_delegation_cert_from_cbor(reader, &stake_delegation_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_certificate_new_stake_delegation(stake_delegation_cert, &cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_stake_delegation_cert_unref(&stake_delegation_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_stake_delegation, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_error_t result = cardano_certificate_new_stake_delegation(NULL, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_stake_delegation, returnsErrorIfCertIsInvalid)
{
  // Act
  cardano_error_t result = cardano_certificate_new_stake_delegation((cardano_stake_delegation_cert_t*)"", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_stake_delegation, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*           reader                = cardano_cbor_reader_from_hex(CBOR_STAKE_DELEGATION, strlen(CBOR_STAKE_DELEGATION));
  cardano_stake_delegation_cert_t* stake_delegation_cert = NULL;
  cardano_certificate_t*           cert                  = NULL;

  // Act
  cardano_error_t result = cardano_stake_delegation_cert_from_cbor(reader, &stake_delegation_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_certificate_new_stake_delegation(stake_delegation_cert, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_stake_delegation_cert_unref(&stake_delegation_cert);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_stake_deregistration, canCreate)
{
  // Arrange
  cardano_cbor_reader_t*               reader                    = cardano_cbor_reader_from_hex(CBOR_STAKE_DEREGISTRATION, strlen(CBOR_STAKE_DEREGISTRATION));
  cardano_stake_deregistration_cert_t* stake_deregistration_cert = NULL;
  cardano_certificate_t*               cert                      = NULL;

  // Act
  cardano_error_t result = cardano_stake_deregistration_cert_from_cbor(reader, &stake_deregistration_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_certificate_new_stake_deregistration(stake_deregistration_cert, &cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_stake_deregistration_cert_unref(&stake_deregistration_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_stake_deregistration, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_error_t result = cardano_certificate_new_stake_deregistration(NULL, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_stake_deregistration, returnsErrorIfCertIsInvalid)
{
  // Act
  cardano_error_t result = cardano_certificate_new_stake_deregistration((cardano_stake_deregistration_cert_t*)"", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_stake_deregistration, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*               reader                    = cardano_cbor_reader_from_hex(CBOR_STAKE_DEREGISTRATION, strlen(CBOR_STAKE_DEREGISTRATION));
  cardano_stake_deregistration_cert_t* stake_deregistration_cert = NULL;
  cardano_certificate_t*               cert                      = NULL;

  // Act
  cardano_error_t result = cardano_stake_deregistration_cert_from_cbor(reader, &stake_deregistration_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_certificate_new_stake_deregistration(stake_deregistration_cert, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_stake_deregistration_cert_unref(&stake_deregistration_cert);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_stake_registration, canCreate)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR_STAKE_REGISTRATION, strlen(CBOR_STAKE_REGISTRATION));
  cardano_stake_registration_cert_t* stake_registration_cert = NULL;
  cardano_certificate_t*             cert                    = NULL;

  // Act
  cardano_error_t result = cardano_stake_registration_cert_from_cbor(reader, &stake_registration_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_certificate_new_stake_registration(stake_registration_cert, &cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_stake_registration_cert_unref(&stake_registration_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_stake_registration, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_error_t result = cardano_certificate_new_stake_registration(NULL, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_stake_registration, returnsErrorIfCertIsInvalid)
{
  // Act
  cardano_error_t result = cardano_certificate_new_stake_registration((cardano_stake_registration_cert_t*)"", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_stake_registration, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR_STAKE_REGISTRATION, strlen(CBOR_STAKE_REGISTRATION));
  cardano_stake_registration_cert_t* stake_registration_cert = NULL;
  cardano_certificate_t*             cert                    = NULL;

  // Act
  cardano_error_t result = cardano_stake_registration_cert_from_cbor(reader, &stake_registration_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_certificate_new_stake_registration(stake_registration_cert, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_stake_registration_cert_unref(&stake_registration_cert);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_stake_registration_delegation, canCreate)
{
  // Arrange
  cardano_cbor_reader_t*                        reader                             = cardano_cbor_reader_from_hex(CBOR_STAKE_REGISTRATION_DELEGATION, strlen(CBOR_STAKE_REGISTRATION_DELEGATION));
  cardano_stake_registration_delegation_cert_t* stake_registration_delegation_cert = NULL;
  cardano_certificate_t*                        cert                               = NULL;

  // Act
  cardano_error_t result = cardano_stake_registration_delegation_cert_from_cbor(reader, &stake_registration_delegation_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_certificate_new_stake_registration_delegation(stake_registration_delegation_cert, &cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_stake_registration_delegation_cert_unref(&stake_registration_delegation_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_stake_registration_delegation, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_error_t result = cardano_certificate_new_stake_registration_delegation(NULL, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_stake_registration_delegation, returnsErrorIfCertIsInvalid)
{
  // Act
  cardano_error_t result = cardano_certificate_new_stake_registration_delegation((cardano_stake_registration_delegation_cert_t*)"", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_stake_registration_delegation, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*                        reader                             = cardano_cbor_reader_from_hex(CBOR_STAKE_REGISTRATION_DELEGATION, strlen(CBOR_STAKE_REGISTRATION_DELEGATION));
  cardano_stake_registration_delegation_cert_t* stake_registration_delegation_cert = NULL;
  cardano_certificate_t*                        cert                               = NULL;

  // Act
  cardano_error_t result = cardano_stake_registration_delegation_cert_from_cbor(reader, &stake_registration_delegation_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_certificate_new_stake_registration_delegation(stake_registration_delegation_cert, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_stake_registration_delegation_cert_unref(&stake_registration_delegation_cert);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_stake_vote_delegation, canCreate)
{
  // Arrange
  cardano_cbor_reader_t*                reader                     = cardano_cbor_reader_from_hex(CBOR_STAKE_VOTE_DELEGATION, strlen(CBOR_STAKE_VOTE_DELEGATION));
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = NULL;
  cardano_certificate_t*                cert                       = NULL;

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_from_cbor(reader, &stake_vote_delegation_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_certificate_new_stake_vote_delegation(stake_vote_delegation_cert, &cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_stake_vote_delegation, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_error_t result = cardano_certificate_new_stake_vote_delegation(NULL, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_stake_vote_delegation, returnsErrorIfCertIsInvalid)
{
  // Act
  cardano_error_t result = cardano_certificate_new_stake_vote_delegation((cardano_stake_vote_delegation_cert_t*)"", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_stake_vote_delegation, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*                reader                     = cardano_cbor_reader_from_hex(CBOR_STAKE_VOTE_DELEGATION, strlen(CBOR_STAKE_VOTE_DELEGATION));
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation_cert = NULL;
  cardano_certificate_t*                cert                       = NULL;

  // Act
  cardano_error_t result = cardano_stake_vote_delegation_cert_from_cbor(reader, &stake_vote_delegation_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_certificate_new_stake_vote_delegation(stake_vote_delegation_cert, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation_cert);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_stake_vote_registration_delegation, canCreate)
{
  // Arrange
  cardano_cbor_reader_t*                             reader                                  = cardano_cbor_reader_from_hex(CBOR_STAKE_VOTE_REGISTRATION_DELEGATION, strlen(CBOR_STAKE_VOTE_REGISTRATION_DELEGATION));
  cardano_stake_vote_registration_delegation_cert_t* stake_vote_registration_delegation_cert = NULL;
  cardano_certificate_t*                             cert                                    = NULL;

  // Act
  cardano_error_t result = cardano_stake_vote_registration_delegation_cert_from_cbor(reader, &stake_vote_registration_delegation_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_certificate_new_stake_vote_registration_delegation(stake_vote_registration_delegation_cert, &cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_stake_vote_registration_delegation_cert_unref(&stake_vote_registration_delegation_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_stake_vote_registration_delegation, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_error_t result = cardano_certificate_new_stake_vote_registration_delegation(NULL, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_stake_vote_registration_delegation, returnsErrorIfCertIsInvalid)
{
  // Act
  cardano_error_t result = cardano_certificate_new_stake_vote_registration_delegation((cardano_stake_vote_registration_delegation_cert_t*)"", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_stake_vote_registration_delegation, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*                             reader                                  = cardano_cbor_reader_from_hex(CBOR_STAKE_VOTE_REGISTRATION_DELEGATION, strlen(CBOR_STAKE_VOTE_REGISTRATION_DELEGATION));
  cardano_stake_vote_registration_delegation_cert_t* stake_vote_registration_delegation_cert = NULL;
  cardano_certificate_t*                             cert                                    = NULL;

  // Act
  cardano_error_t result = cardano_stake_vote_registration_delegation_cert_from_cbor(reader, &stake_vote_registration_delegation_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_certificate_new_stake_vote_registration_delegation(stake_vote_registration_delegation_cert, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_stake_vote_registration_delegation_cert_unref(&stake_vote_registration_delegation_cert);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_unregister_drep, canCreate)
{
  // Arrange
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(CBOR_UNREGISTER_DREP, strlen(CBOR_UNREGISTER_DREP));
  cardano_unregister_drep_cert_t* unregister_drep_cert = NULL;
  cardano_certificate_t*          cert                 = NULL;

  // Act
  cardano_error_t result = cardano_unregister_drep_cert_from_cbor(reader, &unregister_drep_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_certificate_new_unregister_drep(unregister_drep_cert, &cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_unregister_drep_cert_unref(&unregister_drep_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_unregister_drep, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_error_t result = cardano_certificate_new_unregister_drep(NULL, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_unregister_drep, returnsErrorIfCertIsInvalid)
{
  // Act
  cardano_error_t result = cardano_certificate_new_unregister_drep((cardano_unregister_drep_cert_t*)"", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_unregister_drep, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(CBOR_UNREGISTER_DREP, strlen(CBOR_UNREGISTER_DREP));
  cardano_unregister_drep_cert_t* unregister_drep_cert = NULL;
  cardano_certificate_t*          cert                 = NULL;

  // Act
  cardano_error_t result = cardano_unregister_drep_cert_from_cbor(reader, &unregister_drep_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_certificate_new_unregister_drep(unregister_drep_cert, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_unregister_drep_cert_unref(&unregister_drep_cert);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_unregistration, canCreate)
{
  // Arrange
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex(CBOR_UNREGISTRATION, strlen(CBOR_UNREGISTRATION));
  cardano_unregistration_cert_t* unregistration_cert = NULL;
  cardano_certificate_t*         cert                = NULL;

  // Act
  cardano_error_t result = cardano_unregistration_cert_from_cbor(reader, &unregistration_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_certificate_new_unregistration(unregistration_cert, &cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_unregistration_cert_unref(&unregistration_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_unregistration, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_error_t result = cardano_certificate_new_unregistration(NULL, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_unregistration, returnsErrorIfCertIsInvalid)
{
  // Act
  cardano_error_t result = cardano_certificate_new_unregistration((cardano_unregistration_cert_t*)"", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_unregistration, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*         reader              = cardano_cbor_reader_from_hex(CBOR_UNREGISTRATION, strlen(CBOR_UNREGISTRATION));
  cardano_unregistration_cert_t* unregistration_cert = NULL;
  cardano_certificate_t*         cert                = NULL;

  // Act
  cardano_error_t result = cardano_unregistration_cert_from_cbor(reader, &unregistration_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_certificate_new_unregistration(unregistration_cert, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_unregistration_cert_unref(&unregistration_cert);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_update_drep, canCreate)
{
  // Arrange
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex(CBOR_UPDATE_DREP, strlen(CBOR_UPDATE_DREP));
  cardano_update_drep_cert_t* update_drep_cert = NULL;
  cardano_certificate_t*      cert             = NULL;

  // Act
  cardano_error_t result = cardano_update_drep_cert_from_cbor(reader, &update_drep_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_certificate_new_update_drep(update_drep_cert, &cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_update_drep_cert_unref(&update_drep_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_update_drep, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_error_t result = cardano_certificate_new_update_drep(NULL, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_update_drep, returnsErrorIfCertIsInvalid)
{
  // Act
  cardano_error_t result = cardano_certificate_new_update_drep((cardano_update_drep_cert_t*)"", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_update_drep, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*      reader           = cardano_cbor_reader_from_hex(CBOR_UPDATE_DREP, strlen(CBOR_UPDATE_DREP));
  cardano_update_drep_cert_t* update_drep_cert = NULL;
  cardano_certificate_t*      cert             = NULL;

  // Act
  cardano_error_t result = cardano_update_drep_cert_from_cbor(reader, &update_drep_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_certificate_new_update_drep(update_drep_cert, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_update_drep_cert_unref(&update_drep_cert);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_vote_delegation, canCreate)
{
  // Arrange
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(CBOR_VOTE_DELEGATION, strlen(CBOR_VOTE_DELEGATION));
  cardano_vote_delegation_cert_t* vote_delegation_cert = NULL;
  cardano_certificate_t*          cert                 = NULL;

  // Act
  cardano_error_t result = cardano_vote_delegation_cert_from_cbor(reader, &vote_delegation_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_certificate_new_vote_delegation(vote_delegation_cert, &cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_vote_delegation_cert_unref(&vote_delegation_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_vote_delegation, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_error_t result = cardano_certificate_new_vote_delegation(NULL, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_vote_delegation, returnsErrorIfCertIsInvalid)
{
  // Act
  cardano_error_t result = cardano_certificate_new_vote_delegation((cardano_vote_delegation_cert_t*)"", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_vote_delegation, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*          reader               = cardano_cbor_reader_from_hex(CBOR_VOTE_DELEGATION, strlen(CBOR_VOTE_DELEGATION));
  cardano_vote_delegation_cert_t* vote_delegation_cert = NULL;
  cardano_certificate_t*          cert                 = NULL;

  // Act
  cardano_error_t result = cardano_vote_delegation_cert_from_cbor(reader, &vote_delegation_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_certificate_new_vote_delegation(vote_delegation_cert, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_vote_delegation_cert_unref(&vote_delegation_cert);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_vote_registration_delegation, canCreate)
{
  // Arrange
  cardano_cbor_reader_t*                       reader                            = cardano_cbor_reader_from_hex(CBOR_VOTE_REGISTRATION_DELEGATION, strlen(CBOR_VOTE_REGISTRATION_DELEGATION));
  cardano_vote_registration_delegation_cert_t* vote_registration_delegation_cert = NULL;
  cardano_certificate_t*                       cert                              = NULL;

  // Act
  cardano_error_t result = cardano_vote_registration_delegation_cert_from_cbor(reader, &vote_registration_delegation_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  result = cardano_certificate_new_vote_registration_delegation(vote_registration_delegation_cert, &cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_vote_registration_delegation_cert_unref(&vote_registration_delegation_cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_new_vote_registration_delegation, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_error_t result = cardano_certificate_new_vote_registration_delegation(NULL, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_vote_registration_delegation, returnsErrorIfCertIsInvalid)
{
  // Act
  cardano_error_t result = cardano_certificate_new_vote_registration_delegation((cardano_vote_registration_delegation_cert_t*)"", NULL);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_certificate_new_vote_registration_delegation, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t*                       reader                            = cardano_cbor_reader_from_hex(CBOR_VOTE_REGISTRATION_DELEGATION, strlen(CBOR_VOTE_REGISTRATION_DELEGATION));
  cardano_vote_registration_delegation_cert_t* vote_registration_delegation_cert = NULL;
  cardano_certificate_t*                       cert                              = NULL;

  // Act
  cardano_error_t result = cardano_vote_registration_delegation_cert_from_cbor(reader, &vote_registration_delegation_cert);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  result = cardano_certificate_new_vote_registration_delegation(vote_registration_delegation_cert, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref((cardano_certificate_t**)&cert);
  cardano_vote_registration_delegation_cert_unref(&vote_registration_delegation_cert);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_REGISTRATION, strlen(CBOR_STAKE_REGISTRATION));
  cardano_certificate_t* cert   = NULL;

  // Act
  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  EXPECT_EQ(result, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_set_allocators(malloc, realloc, free);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfCborDoesntStartWithArray)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("00", 2);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfCborDoesntHaveCertId)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("82ef", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, canDecodeRegistrationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfInvalidRegistrationCert)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8307", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, canDecodeResignCommitteeColdCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_RESIGN_COMMITTEE_COLD, strlen(CBOR_RESIGN_COMMITTEE_COLD));
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfInvalidResignCommitteeColdCert)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("830f", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, canDecodeStakeDelegationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_DELEGATION, strlen(CBOR_STAKE_DELEGATION));
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfInvalidStakeDelegationCert)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8302", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, canDecodeStakeDeregistrationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_DEREGISTRATION, strlen(CBOR_STAKE_DEREGISTRATION));
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfInvalidStakeDeregistrationCert)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8301", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, canDecodeStakeRegistrationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_REGISTRATION, strlen(CBOR_STAKE_REGISTRATION));
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfInvalidStakeRegistrationCert)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8300", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, canDecodeStakeRegistrationDelegationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_REGISTRATION_DELEGATION, strlen(CBOR_STAKE_REGISTRATION_DELEGATION));
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfInvalidStakeRegistrationDelegationCert)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("830b", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, canDecodeStakeVoteDelegationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_VOTE_DELEGATION, strlen(CBOR_STAKE_VOTE_DELEGATION));
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfInvalidStakeVoteDelegationCert)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("830a", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, canDecodeStakeVoteRegistrationDelegationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_VOTE_REGISTRATION_DELEGATION, strlen(CBOR_STAKE_VOTE_REGISTRATION_DELEGATION));
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfInvalidStakeVoteRegistrationDelegationCert)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("830d", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, canDecodeUnregisterDrepCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_UNREGISTER_DREP, strlen(CBOR_UNREGISTER_DREP));
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfInvalidUnregisterDrepCert)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8311", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, canDecodeUnregistrationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_UNREGISTRATION, strlen(CBOR_UNREGISTRATION));
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfInvalidUnregistrationCert)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8308", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, canDecodeUpdateDrepCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_UPDATE_DREP, strlen(CBOR_UPDATE_DREP));
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfInvalidUpdateDrepCert)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8312", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, canDecodeVoteDelegationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_VOTE_DELEGATION, strlen(CBOR_VOTE_DELEGATION));
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfInvalidVoteDelegationCert)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8309", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, canDecodeVoteRegistrationDelegationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_VOTE_REGISTRATION_DELEGATION, strlen(CBOR_VOTE_REGISTRATION_DELEGATION));
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfInvalidVoteRegistrationDelegationCert)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("830c", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, canDecodeAuthCommitteeHot)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_AUTHORIZE_COMMITTEE_HOT, strlen(CBOR_AUTHORIZE_COMMITTEE_HOT));
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfInvalidAuthCommitteeHot)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("830e", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, canDecodeGenesisKeyDelegation)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_GENESIS_DELEGATION, strlen(CBOR_GENESIS_DELEGATION));
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfInvalidGenesisKeyDelegation)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8305", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, canDecodeMir)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_MIR, strlen(CBOR_MIR));
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfInvalidMir)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8306", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, canDecodePoolRegistration)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_POOL_REGISTRATION, strlen(CBOR_POOL_REGISTRATION));
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfInvalidPoolRegistration)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8a03", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, canDecodePoolRetirement)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_POOL_RETIREMENT, strlen(CBOR_POOL_RETIREMENT));
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfInvalidPoolRetirement)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8b04", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, canDecodeDrepRegistration)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_REGISTER_DREP, strlen(CBOR_REGISTER_DREP));
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_from_cbor, returnsErrorIfInvalidDrepRegistration)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex("8c10", 4);
  cardano_certificate_t* cert   = NULL;

  // Act
  cardano_error_t result = cardano_certificate_from_cbor(reader, &cert);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_INVALID_CBOR_ARRAY_SIZE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_to_cbor, canEncodeRegistrationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_cbor(cert, writer), CARDANO_SUCCESS);

  // Assert
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_REGISTRATION);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_certificate_to_cbor, canEncodeResignCommitteeColdCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_RESIGN_COMMITTEE_COLD, strlen(CBOR_RESIGN_COMMITTEE_COLD));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_cbor(cert, writer), CARDANO_SUCCESS);

  // Assert
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_RESIGN_COMMITTEE_COLD);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_certificate_to_cbor, canEncodeStakeDelegationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_DELEGATION, strlen(CBOR_STAKE_DELEGATION));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_cbor(cert, writer), CARDANO_SUCCESS);

  // Assert
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_STAKE_DELEGATION);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_certificate_to_cbor, canEncodeStakeDeregistrationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_DEREGISTRATION, strlen(CBOR_STAKE_DEREGISTRATION));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_cbor(cert, writer), CARDANO_SUCCESS);

  // Assert
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_STAKE_DEREGISTRATION);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_certificate_to_cbor, canEncodeStakeRegistrationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_REGISTRATION, strlen(CBOR_STAKE_REGISTRATION));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_cbor(cert, writer), CARDANO_SUCCESS);

  // Assert
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_STAKE_REGISTRATION);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_certificate_to_cbor, canEncodeStakeRegistrationDelegationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_REGISTRATION_DELEGATION, strlen(CBOR_STAKE_REGISTRATION_DELEGATION));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_cbor(cert, writer), CARDANO_SUCCESS);

  // Assert
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_STAKE_REGISTRATION_DELEGATION);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_certificate_to_cbor, canEncodeStakeVoteDelegationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_VOTE_DELEGATION, strlen(CBOR_STAKE_VOTE_DELEGATION));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_cbor(cert, writer), CARDANO_SUCCESS);

  // Assert
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_STAKE_VOTE_DELEGATION);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_certificate_to_cbor, canEncodeStakeVoteRegistrationDelegationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_VOTE_REGISTRATION_DELEGATION, strlen(CBOR_STAKE_VOTE_REGISTRATION_DELEGATION));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_cbor(cert, writer), CARDANO_SUCCESS);

  // Assert
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_STAKE_VOTE_REGISTRATION_DELEGATION);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_certificate_to_cbor, canEncodeUnregisterDrepCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_UNREGISTER_DREP, strlen(CBOR_UNREGISTER_DREP));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_cbor(cert, writer), CARDANO_SUCCESS);

  // Assert
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_UNREGISTER_DREP);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_certificate_to_cbor, canEncodeUnregistrationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_UNREGISTRATION, strlen(CBOR_UNREGISTRATION));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_cbor(cert, writer), CARDANO_SUCCESS);

  // Assert
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_UNREGISTRATION);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_certificate_to_cbor, canEncodeUpdateDrepCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_UPDATE_DREP, strlen(CBOR_UPDATE_DREP));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_cbor(cert, writer), CARDANO_SUCCESS);

  // Assert
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_UPDATE_DREP);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_certificate_to_cbor, canEncodeVoteDelegationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_VOTE_DELEGATION, strlen(CBOR_VOTE_DELEGATION));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_cbor(cert, writer), CARDANO_SUCCESS);

  // Assert
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_VOTE_DELEGATION);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_certificate_to_cbor, canEncodeVoteRegistrationDelegationCertificate)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_VOTE_REGISTRATION_DELEGATION, strlen(CBOR_VOTE_REGISTRATION_DELEGATION));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_cbor(cert, writer), CARDANO_SUCCESS);

  // Assert
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_VOTE_REGISTRATION_DELEGATION);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_certificate_to_cbor, canEncodeAuthCommitteeHot)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_AUTHORIZE_COMMITTEE_HOT, strlen(CBOR_AUTHORIZE_COMMITTEE_HOT));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_cbor(cert, writer), CARDANO_SUCCESS);

  // Assert
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_AUTHORIZE_COMMITTEE_HOT);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_certificate_to_cbor, canEncodeGenesisKeyDelegation)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_GENESIS_DELEGATION, strlen(CBOR_GENESIS_DELEGATION));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_cbor(cert, writer), CARDANO_SUCCESS);

  // Assert
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_GENESIS_DELEGATION);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_certificate_to_cbor, canEncodeMir)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_MIR, strlen(CBOR_MIR));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_cbor(cert, writer), CARDANO_SUCCESS);

  // Assert
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_MIR);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_certificate_to_cbor, canEncodePoolRegistration)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_POOL_REGISTRATION, strlen(CBOR_POOL_REGISTRATION));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_cbor(cert, writer), CARDANO_SUCCESS);

  // Assert
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_POOL_REGISTRATION);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_certificate_to_cbor, canEncodePoolRetirement)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_POOL_RETIREMENT, strlen(CBOR_POOL_RETIREMENT));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_cbor(cert, writer), CARDANO_SUCCESS);

  // Assert
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*        hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_POOL_RETIREMENT);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_certificate_to_cbor, canEncodeDrepRegistration)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_REGISTER_DREP, strlen(CBOR_REGISTER_DREP));
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_cbor(cert, writer), CARDANO_SUCCESS);

  // Assert
  const size_t hex_size = cardano_cbor_writer_get_hex_size(writer);

  char* hex = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR_REGISTER_DREP);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_cert_get_type, canGetCertificateType)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  cardano_cert_type_t type;

  ASSERT_EQ(cardano_cert_get_type(cert, &type), CARDANO_SUCCESS);

  // Assert
  ASSERT_EQ(type, CARDANO_CERT_TYPE_REGISTRATION);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_cert_get_type, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t* cert = NULL;

  // Act
  cardano_cert_type_t type;

  ASSERT_EQ(cardano_cert_get_type(cert, &type), CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_cert_get_type, returnsErrorIfTypeIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_cert_get_type(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_to_auth_committee_hot, canConvertCertificateToAuthCommittee)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR_AUTHORIZE_COMMITTEE_HOT, strlen(CBOR_AUTHORIZE_COMMITTEE_HOT));
  cardano_certificate_t*             cert                    = NULL;
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_auth_committee_hot(cert, &auth_committee_hot_cert), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(auth_committee_hot_cert, nullptr);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
}

TEST(cardano_certificate_to_auth_committee_hot, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t*             cert                    = NULL;
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = NULL;

  // Act
  ASSERT_EQ(cardano_certificate_to_auth_committee_hot(cert, &auth_committee_hot_cert), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
}

TEST(cardano_certificate_to_auth_committee_hot, returnsErrorIfAuthCommitteeIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_AUTHORIZE_COMMITTEE_HOT, strlen(CBOR_AUTHORIZE_COMMITTEE_HOT));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_auth_committee_hot(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_auth_committee_hot_cert_get_key, returnErrorIfInvalidType)
{
  // Arrange
  cardano_cbor_reader_t*             reader                  = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t*             cert                    = NULL;
  cardano_auth_committee_hot_cert_t* auth_committee_hot_cert = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_auth_committee_hot(cert, &auth_committee_hot_cert), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_auth_committee_hot_cert_unref(&auth_committee_hot_cert);
}

TEST(cardano_certificate_to_genesis_key_delegation, canConvertCertificateToGenesisKeyDelegation)
{
  // Arrange
  cardano_cbor_reader_t*                 reader                      = cardano_cbor_reader_from_hex(CBOR_GENESIS_DELEGATION, strlen(CBOR_GENESIS_DELEGATION));
  cardano_certificate_t*                 cert                        = NULL;
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_genesis_key_delegation(cert, &genesis_key_delegation_cert), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(genesis_key_delegation_cert, nullptr);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
}

TEST(cardano_certificate_to_genesis_key_delegation, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t*                 cert                        = NULL;
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;

  // Act
  ASSERT_EQ(cardano_certificate_to_genesis_key_delegation(cert, &genesis_key_delegation_cert), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
}

TEST(cardano_certificate_to_genesis_key_delegation, returnsErrorIfGenesisKeyDelegationIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_GENESIS_DELEGATION, strlen(CBOR_GENESIS_DELEGATION));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_genesis_key_delegation(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_genesis_key_delegation_cert_get_key, returnErrorIfInvalidType)
{
  // Arrange
  cardano_cbor_reader_t*                 reader                      = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t*                 cert                        = NULL;
  cardano_genesis_key_delegation_cert_t* genesis_key_delegation_cert = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_genesis_key_delegation(cert, &genesis_key_delegation_cert), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_genesis_key_delegation_cert_unref(&genesis_key_delegation_cert);
}

TEST(cardano_certificate_to_mir, canConvertCertificateToMir)
{
  // Arrange
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(CBOR_MIR, strlen(CBOR_MIR));
  cardano_certificate_t* cert     = NULL;
  cardano_mir_cert_t*    mir_cert = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_mir(cert, &mir_cert), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(mir_cert, nullptr);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_mir_cert_unref(&mir_cert);
}

TEST(cardano_certificate_to_mir, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t* cert     = NULL;
  cardano_mir_cert_t*    mir_cert = NULL;

  // Act
  ASSERT_EQ(cardano_certificate_to_mir(cert, &mir_cert), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_mir_cert_unref(&mir_cert);
}

TEST(cardano_certificate_to_mir, returnsErrorIfMirIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_MIR, strlen(CBOR_MIR));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_mir(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_certificate_to_mir, returnErrorIfInvalidType)
{
  // Arrange
  cardano_cbor_reader_t* reader   = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t* cert     = NULL;
  cardano_mir_cert_t*    mir_cert = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_mir(cert, &mir_cert), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_mir_cert_unref(&mir_cert);
}

TEST(cardano_certificate_to_pool_registration, canConvertCertificateToPoolRegistration)
{
  // Arrange
  cardano_cbor_reader_t*            reader    = cardano_cbor_reader_from_hex(CBOR_POOL_REGISTRATION, strlen(CBOR_POOL_REGISTRATION));
  cardano_certificate_t*            cert      = NULL;
  cardano_pool_registration_cert_t* pool_cert = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_pool_registration(cert, &pool_cert), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(pool_cert, nullptr);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_pool_registration_cert_unref(&pool_cert);
}

TEST(cardano_certificate_to_pool_registration, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t*            cert      = NULL;
  cardano_pool_registration_cert_t* pool_cert = NULL;

  // Act
  ASSERT_EQ(cardano_certificate_to_pool_registration(cert, &pool_cert), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_registration_cert_unref(&pool_cert);
}

TEST(cardano_certificate_to_pool_registration, returnsErrorIfPoolRegistrationIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_POOL_REGISTRATION, strlen(CBOR_POOL_REGISTRATION));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_pool_registration(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_registration_cert_get_pool_id, returnErrorIfInvalidType)
{
  // Arrange
  cardano_cbor_reader_t*            reader    = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t*            cert      = NULL;
  cardano_pool_registration_cert_t* pool_cert = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_pool_registration(cert, &pool_cert), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_pool_registration_cert_unref(&pool_cert);
}

TEST(cardano_certificate_to_pool_retirement, canConvertCertificateToPoolRetirement)
{
  // Arrange
  cardano_cbor_reader_t*          reader          = cardano_cbor_reader_from_hex(CBOR_POOL_RETIREMENT, strlen(CBOR_POOL_RETIREMENT));
  cardano_certificate_t*          cert            = NULL;
  cardano_pool_retirement_cert_t* pool_retirement = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_pool_retirement(cert, &pool_retirement), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(pool_retirement, nullptr);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_pool_retirement_cert_unref(&pool_retirement);
}

TEST(cardano_certificate_to_pool_retirement, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t*          cert            = NULL;
  cardano_pool_retirement_cert_t* pool_retirement = NULL;

  // Act
  ASSERT_EQ(cardano_certificate_to_pool_retirement(cert, &pool_retirement), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_pool_retirement_cert_unref(&pool_retirement);
}

TEST(cardano_certificate_to_pool_retirement, returnsErrorIfPoolRetirementIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_POOL_RETIREMENT, strlen(CBOR_POOL_RETIREMENT));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_pool_retirement(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_pool_retirement_cert_get_pool_id, returnErrorIfInvalidType)
{
  // Arrange
  cardano_cbor_reader_t*          reader          = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t*          cert            = NULL;
  cardano_pool_retirement_cert_t* pool_retirement = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_pool_retirement(cert, &pool_retirement), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_pool_retirement_cert_unref(&pool_retirement);
}

TEST(cardano_certificate_to_register_drep, canConvertCertificateToRegisterDrep)
{
  // Arrange
  cardano_cbor_reader_t*        reader        = cardano_cbor_reader_from_hex(CBOR_REGISTER_DREP, strlen(CBOR_REGISTER_DREP));
  cardano_certificate_t*        cert          = NULL;
  cardano_register_drep_cert_t* register_drep = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_register_drep(cert, &register_drep), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(register_drep, nullptr);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_register_drep_cert_unref(&register_drep);
}

TEST(cardano_certificate_to_register_drep, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t*        cert          = NULL;
  cardano_register_drep_cert_t* register_drep = NULL;

  // Act
  ASSERT_EQ(cardano_certificate_to_register_drep(cert, &register_drep), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_register_drep_cert_unref(&register_drep);
}

TEST(cardano_certificate_to_register_drep, returnsErrorIfRegisterDrepIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_REGISTER_DREP, strlen(CBOR_REGISTER_DREP));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_register_drep(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_register_drep_cert_get_pool_id, returnErrorIfInvalidType)
{
  // Arrange
  cardano_cbor_reader_t*        reader        = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t*        cert          = NULL;
  cardano_register_drep_cert_t* register_drep = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_register_drep(cert, &register_drep), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_register_drep_cert_unref(&register_drep);
}

TEST(cardano_certificate_to_registration, canConvertCertificateToRegistration)
{
  // Arrange
  cardano_cbor_reader_t*       reader       = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t*       cert         = NULL;
  cardano_registration_cert_t* registration = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_registration(cert, &registration), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(registration, nullptr);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_registration_cert_unref(&registration);
}

TEST(cardano_certificate_to_registration, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t*       cert         = NULL;
  cardano_registration_cert_t* registration = NULL;

  // Act
  ASSERT_EQ(cardano_certificate_to_registration(cert, &registration), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_registration_cert_unref(&registration);
}

TEST(cardano_certificate_to_registration, returnsErrorIfRegistrationIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_registration(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_registration_cert_get_pool_id, returnErrorIfInvalidType)
{
  // Arrange
  cardano_cbor_reader_t*       reader       = cardano_cbor_reader_from_hex(CBOR_MIR, strlen(CBOR_MIR));
  cardano_certificate_t*       cert         = NULL;
  cardano_registration_cert_t* registration = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_registration(cert, &registration), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_registration_cert_unref(&registration);
}

TEST(cardano_certificate_to_resign_committee_cold, canConvertCertificateToResignCommitteeCold)
{
  // Arrange
  cardano_cbor_reader_t*                reader                = cardano_cbor_reader_from_hex(CBOR_RESIGN_COMMITTEE_COLD, strlen(CBOR_RESIGN_COMMITTEE_COLD));
  cardano_certificate_t*                cert                  = NULL;
  cardano_resign_committee_cold_cert_t* resign_committee_cold = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_resign_committee_cold(cert, &resign_committee_cold), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(resign_committee_cold, nullptr);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_resign_committee_cold_cert_unref(&resign_committee_cold);
}

TEST(cardano_certificate_to_resign_committee_cold, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t*                cert                  = NULL;
  cardano_resign_committee_cold_cert_t* resign_committee_cold = NULL;

  // Act
  ASSERT_EQ(cardano_certificate_to_resign_committee_cold(cert, &resign_committee_cold), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_resign_committee_cold_cert_unref(&resign_committee_cold);
}

TEST(cardano_certificate_to_resign_committee_cold, returnsErrorIfResignCommitteeColdIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_RESIGN_COMMITTEE_COLD, strlen(CBOR_RESIGN_COMMITTEE_COLD));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_resign_committee_cold(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_resign_committee_cold_cert_get_key, returnErrorIfInvalidType)
{
  // Arrange
  cardano_cbor_reader_t*                reader                = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t*                cert                  = NULL;
  cardano_resign_committee_cold_cert_t* resign_committee_cold = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_resign_committee_cold(cert, &resign_committee_cold), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_resign_committee_cold_cert_unref(&resign_committee_cold);
}

TEST(cardano_certificate_to_stake_delegation, canConvertCertificateToStakeDelegation)
{
  // Arrange
  cardano_cbor_reader_t*           reader           = cardano_cbor_reader_from_hex(CBOR_STAKE_DELEGATION, strlen(CBOR_STAKE_DELEGATION));
  cardano_certificate_t*           cert             = NULL;
  cardano_stake_delegation_cert_t* stake_delegation = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_delegation(cert, &stake_delegation), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(stake_delegation, nullptr);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_stake_delegation_cert_unref(&stake_delegation);
}

TEST(cardano_certificate_to_stake_delegation, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t*           cert             = NULL;
  cardano_stake_delegation_cert_t* stake_delegation = NULL;

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_delegation(cert, &stake_delegation), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_stake_delegation_cert_unref(&stake_delegation);
}

TEST(cardano_certificate_to_stake_delegation, returnsErrorIfStakeDelegationIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_DELEGATION, strlen(CBOR_STAKE_DELEGATION));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_delegation(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_stake_delegation_cert_get_key, returnErrorIfInvalidType)
{
  // Arrange
  cardano_cbor_reader_t*           reader           = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t*           cert             = NULL;
  cardano_stake_delegation_cert_t* stake_delegation = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_delegation(cert, &stake_delegation), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_stake_delegation_cert_unref(&stake_delegation);
}

TEST(cardano_certificate_to_stake_deregistration, canConvertCertificateToStakeDeregistration)
{
  // Arrange
  cardano_cbor_reader_t*               reader               = cardano_cbor_reader_from_hex(CBOR_STAKE_DEREGISTRATION, strlen(CBOR_STAKE_DEREGISTRATION));
  cardano_certificate_t*               cert                 = NULL;
  cardano_stake_deregistration_cert_t* stake_deregistration = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_deregistration(cert, &stake_deregistration), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(stake_deregistration, nullptr);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_stake_deregistration_cert_unref(&stake_deregistration);
}

TEST(cardano_certificate_to_stake_deregistration, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t*               cert                 = NULL;
  cardano_stake_deregistration_cert_t* stake_deregistration = NULL;

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_deregistration(cert, &stake_deregistration), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_stake_deregistration_cert_unref(&stake_deregistration);
}

TEST(cardano_certificate_to_stake_deregistration, returnsErrorIfStakeDeregistrationIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_DEREGISTRATION, strlen(CBOR_STAKE_DEREGISTRATION));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_deregistration(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_stake_deregistration_cert_get_key, returnErrorIfInvalidType)
{
  // Arrange
  cardano_cbor_reader_t*               reader               = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t*               cert                 = NULL;
  cardano_stake_deregistration_cert_t* stake_deregistration = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_deregistration(cert, &stake_deregistration), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_stake_deregistration_cert_unref(&stake_deregistration);
}

TEST(cardano_certificate_to_stake_registration, canConvertCertificateToStakeRegistration)
{
  // Arrange
  cardano_cbor_reader_t*             reader             = cardano_cbor_reader_from_hex(CBOR_STAKE_REGISTRATION, strlen(CBOR_STAKE_REGISTRATION));
  cardano_certificate_t*             cert               = NULL;
  cardano_stake_registration_cert_t* stake_registration = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_registration(cert, &stake_registration), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(stake_registration, nullptr);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_stake_registration_cert_unref(&stake_registration);
}

TEST(cardano_certificate_to_stake_registration, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t*             cert               = NULL;
  cardano_stake_registration_cert_t* stake_registration = NULL;

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_registration(cert, &stake_registration), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_stake_registration_cert_unref(&stake_registration);
}

TEST(cardano_certificate_to_stake_registration, returnsErrorIfStakeRegistrationIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_REGISTRATION, strlen(CBOR_STAKE_REGISTRATION));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_registration(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_stake_registration_cert_get_key, returnErrorIfInvalidType)
{
  // Arrange
  cardano_cbor_reader_t*             reader             = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t*             cert               = NULL;
  cardano_stake_registration_cert_t* stake_registration = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_registration(cert, &stake_registration), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_stake_registration_cert_unref(&stake_registration);
}

TEST(cardano_certificate_to_stake_registration_delegation, canConvertCertificateToStakeRegistrationDelegation)
{
  // Arrange
  cardano_cbor_reader_t*                        reader                        = cardano_cbor_reader_from_hex(CBOR_STAKE_REGISTRATION_DELEGATION, strlen(CBOR_STAKE_REGISTRATION_DELEGATION));
  cardano_certificate_t*                        cert                          = NULL;
  cardano_stake_registration_delegation_cert_t* stake_registration_delegation = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_registration_delegation(cert, &stake_registration_delegation), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(stake_registration_delegation, nullptr);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_stake_registration_delegation_cert_unref(&stake_registration_delegation);
}

TEST(cardano_certificate_to_stake_registration_delegation, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t*                        cert                          = NULL;
  cardano_stake_registration_delegation_cert_t* stake_registration_delegation = NULL;

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_registration_delegation(cert, &stake_registration_delegation), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_stake_registration_delegation_cert_unref(&stake_registration_delegation);
}

TEST(cardano_certificate_to_stake_registration_delegation, returnsErrorIfStakeRegistrationDelegationIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_REGISTRATION_DELEGATION, strlen(CBOR_STAKE_REGISTRATION_DELEGATION));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_registration_delegation(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_stake_registration_delegation_cert_get_key, returnErrorIfInvalidType)
{
  // Arrange
  cardano_cbor_reader_t*                        reader                        = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t*                        cert                          = NULL;
  cardano_stake_registration_delegation_cert_t* stake_registration_delegation = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_registration_delegation(cert, &stake_registration_delegation), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_stake_registration_delegation_cert_unref(&stake_registration_delegation);
}

TEST(cardano_certificate_to_stake_vote_delegation, canConvertCertificateToStakeVoteDelegation)
{
  // Arrange
  cardano_cbor_reader_t*                reader                = cardano_cbor_reader_from_hex(CBOR_STAKE_VOTE_DELEGATION, strlen(CBOR_STAKE_VOTE_DELEGATION));
  cardano_certificate_t*                cert                  = NULL;
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_vote_delegation(cert, &stake_vote_delegation), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(stake_vote_delegation, nullptr);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation);
}

TEST(cardano_certificate_to_stake_vote_delegation, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t*                cert                  = NULL;
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation = NULL;

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_vote_delegation(cert, &stake_vote_delegation), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation);
}

TEST(cardano_certificate_to_stake_vote_delegation, returnsErrorIfStakeVoteDelegationIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_VOTE_DELEGATION, strlen(CBOR_STAKE_VOTE_DELEGATION));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_vote_delegation(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_stake_vote_delegation_cert_get_key, returnErrorIfInvalidType)
{
  // Arrange
  cardano_cbor_reader_t*                reader                = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t*                cert                  = NULL;
  cardano_stake_vote_delegation_cert_t* stake_vote_delegation = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_vote_delegation(cert, &stake_vote_delegation), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_stake_vote_delegation_cert_unref(&stake_vote_delegation);
}

TEST(cardano_certificate_to_stake_vote_registration_delegation, canConvertCertificateToStakeVoteRegistrationDelegation)
{
  // Arrange
  cardano_cbor_reader_t*                             reader                             = cardano_cbor_reader_from_hex(CBOR_STAKE_VOTE_REGISTRATION_DELEGATION, strlen(CBOR_STAKE_VOTE_REGISTRATION_DELEGATION));
  cardano_certificate_t*                             cert                               = NULL;
  cardano_stake_vote_registration_delegation_cert_t* stake_vote_registration_delegation = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_vote_registration_delegation(cert, &stake_vote_registration_delegation), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(stake_vote_registration_delegation, nullptr);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_stake_vote_registration_delegation_cert_unref(&stake_vote_registration_delegation);
}

TEST(cardano_certificate_to_stake_vote_registration_delegation, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t*                             cert                               = NULL;
  cardano_stake_vote_registration_delegation_cert_t* stake_vote_registration_delegation = NULL;

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_vote_registration_delegation(cert, &stake_vote_registration_delegation), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_stake_vote_registration_delegation_cert_unref(&stake_vote_registration_delegation);
}

TEST(cardano_certificate_to_stake_vote_registration_delegation, returnsErrorIfStakeVoteRegistrationDelegationIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_STAKE_VOTE_REGISTRATION_DELEGATION, strlen(CBOR_STAKE_VOTE_REGISTRATION_DELEGATION));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_vote_registration_delegation(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_stake_vote_registration_delegation_cert_get_key, returnErrorIfInvalidType)
{
  // Arrange
  cardano_cbor_reader_t*                             reader                             = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t*                             cert                               = NULL;
  cardano_stake_vote_registration_delegation_cert_t* stake_vote_registration_delegation = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_stake_vote_registration_delegation(cert, &stake_vote_registration_delegation), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_stake_vote_registration_delegation_cert_unref(&stake_vote_registration_delegation);
}

TEST(cardano_certificate_to_unregister_drep, canConvertCertificateToUnregisterDrep)
{
  // Arrange
  cardano_cbor_reader_t*          reader          = cardano_cbor_reader_from_hex(CBOR_UNREGISTER_DREP, strlen(CBOR_UNREGISTER_DREP));
  cardano_certificate_t*          cert            = NULL;
  cardano_unregister_drep_cert_t* unregister_drep = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_unregister_drep(cert, &unregister_drep), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(unregister_drep, nullptr);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_unregister_drep_cert_unref(&unregister_drep);
}

TEST(cardano_certificate_to_unregister_drep, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t*          cert            = NULL;
  cardano_unregister_drep_cert_t* unregister_drep = NULL;

  // Act
  ASSERT_EQ(cardano_certificate_to_unregister_drep(cert, &unregister_drep), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_unregister_drep_cert_unref(&unregister_drep);
}

TEST(cardano_certificate_to_unregister_drep, returnsErrorIfUnregisterDrepIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_UNREGISTER_DREP, strlen(CBOR_UNREGISTER_DREP));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_unregister_drep(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_unregister_drep_cert_get_key, returnErrorIfInvalidType)
{
  // Arrange
  cardano_cbor_reader_t*          reader          = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t*          cert            = NULL;
  cardano_unregister_drep_cert_t* unregister_drep = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_unregister_drep(cert, &unregister_drep), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_unregister_drep_cert_unref(&unregister_drep);
}

TEST(cardano_certificate_to_unregistration, canConvertCertificateToUnregistration)
{
  // Arrange
  cardano_cbor_reader_t*         reader         = cardano_cbor_reader_from_hex(CBOR_UNREGISTRATION, strlen(CBOR_UNREGISTRATION));
  cardano_certificate_t*         cert           = NULL;
  cardano_unregistration_cert_t* unregistration = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_unregistration(cert, &unregistration), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(unregistration, nullptr);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_unregistration_cert_unref(&unregistration);
}

TEST(cardano_certificate_to_unregistration, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t*         cert           = NULL;
  cardano_unregistration_cert_t* unregistration = NULL;

  // Act
  ASSERT_EQ(cardano_certificate_to_unregistration(cert, &unregistration), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_unregistration_cert_unref(&unregistration);
}

TEST(cardano_certificate_to_unregistration, returnsErrorIfUnregistrationIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_UNREGISTRATION, strlen(CBOR_UNREGISTRATION));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_unregistration(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_unregistration_cert_get_key, returnErrorIfInvalidType)
{
  // Arrange
  cardano_cbor_reader_t*         reader         = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t*         cert           = NULL;
  cardano_unregistration_cert_t* unregistration = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_unregistration(cert, &unregistration), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_unregistration_cert_unref(&unregistration);
}

TEST(cardano_certificate_to_update_drep, canConvertCertificateToUpdateDrep)
{
  // Arrange
  cardano_cbor_reader_t*      reader      = cardano_cbor_reader_from_hex(CBOR_UPDATE_DREP, strlen(CBOR_UPDATE_DREP));
  cardano_certificate_t*      cert        = NULL;
  cardano_update_drep_cert_t* update_drep = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_update_drep(cert, &update_drep), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(update_drep, nullptr);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_update_drep_cert_unref(&update_drep);
}

TEST(cardano_certificate_to_update_drep, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t*      cert        = NULL;
  cardano_update_drep_cert_t* update_drep = NULL;

  // Act
  ASSERT_EQ(cardano_certificate_to_update_drep(cert, &update_drep), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_update_drep_cert_unref(&update_drep);
}

TEST(cardano_certificate_to_update_drep, returnsErrorIfUpdateDrepIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_UPDATE_DREP, strlen(CBOR_UPDATE_DREP));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_update_drep(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_update_drep_cert_get_key, returnErrorIfInvalidType)
{
  // Arrange
  cardano_cbor_reader_t*      reader      = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t*      cert        = NULL;
  cardano_update_drep_cert_t* update_drep = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_update_drep(cert, &update_drep), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_update_drep_cert_unref(&update_drep);
}

TEST(cardano_certificate_to_vote_delegation, canConvertCertificateToVoteDelegation)
{
  // Arrange
  cardano_cbor_reader_t*          reader          = cardano_cbor_reader_from_hex(CBOR_VOTE_DELEGATION, strlen(CBOR_VOTE_DELEGATION));
  cardano_certificate_t*          cert            = NULL;
  cardano_vote_delegation_cert_t* vote_delegation = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_vote_delegation(cert, &vote_delegation), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(vote_delegation, nullptr);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_vote_delegation_cert_unref(&vote_delegation);
}

TEST(cardano_certificate_to_vote_delegation, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t*          cert            = NULL;
  cardano_vote_delegation_cert_t* vote_delegation = NULL;

  // Act
  ASSERT_EQ(cardano_certificate_to_vote_delegation(cert, &vote_delegation), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_vote_delegation_cert_unref(&vote_delegation);
}

TEST(cardano_certificate_to_vote_delegation, returnsErrorIfVoteDelegationIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_VOTE_DELEGATION, strlen(CBOR_VOTE_DELEGATION));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_vote_delegation(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_vote_delegation_cert_get_key, returnErrorIfInvalidType)
{
  // Arrange
  cardano_cbor_reader_t*          reader          = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t*          cert            = NULL;
  cardano_vote_delegation_cert_t* vote_delegation = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_vote_delegation(cert, &vote_delegation), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_vote_delegation_cert_unref(&vote_delegation);
}

TEST(cardano_certificate_to_vote_registration_delegation, canConvertCertificateToVoteRegistrationDelegation)
{
  // Arrange
  cardano_cbor_reader_t*                       reader                       = cardano_cbor_reader_from_hex(CBOR_VOTE_REGISTRATION_DELEGATION, strlen(CBOR_VOTE_REGISTRATION_DELEGATION));
  cardano_certificate_t*                       cert                         = NULL;
  cardano_vote_registration_delegation_cert_t* vote_registration_delegation = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_vote_registration_delegation(cert, &vote_registration_delegation), CARDANO_SUCCESS);

  // Assert
  ASSERT_NE(vote_registration_delegation, nullptr);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_vote_registration_delegation_cert_unref(&vote_registration_delegation);
}

TEST(cardano_certificate_to_vote_registration_delegation, returnsErrorIfCertificateIsNull)
{
  // Arrange
  cardano_certificate_t*                       cert                         = NULL;
  cardano_vote_registration_delegation_cert_t* vote_registration_delegation = NULL;

  // Act
  ASSERT_EQ(cardano_certificate_to_vote_registration_delegation(cert, &vote_registration_delegation), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_vote_registration_delegation_cert_unref(&vote_registration_delegation);
}

TEST(cardano_certificate_to_vote_registration_delegation, returnsErrorIfVoteRegistrationDelegationIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR_VOTE_REGISTRATION_DELEGATION, strlen(CBOR_VOTE_REGISTRATION_DELEGATION));
  cardano_certificate_t* cert   = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_vote_registration_delegation(cert, NULL), CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_vote_registration_delegation_cert_get_key, returnErrorIfInvalidType)
{
  // Arrange
  cardano_cbor_reader_t*                       reader                       = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t*                       cert                         = NULL;
  cardano_vote_registration_delegation_cert_t* vote_registration_delegation = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  // Act
  ASSERT_EQ(cardano_certificate_to_vote_registration_delegation(cert, &vote_registration_delegation), CARDANO_ERROR_INVALID_CERTIFICATE_TYPE);

  // Cleanup
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  cardano_vote_registration_delegation_cert_unref(&vote_registration_delegation);
}

TEST(cardano_certificate_to_cip116_json, canConvertWrappedRegistrationCert)
{
  // Arrange
  cardano_cbor_reader_t*                       reader                       = cardano_cbor_reader_from_hex(CBOR_REGISTRATION, strlen(CBOR_REGISTRATION));
  cardano_certificate_t*                       cert                         = NULL;
  cardano_vote_registration_delegation_cert_t* vote_registration_delegation = NULL;
  ASSERT_EQ(cardano_certificate_from_cbor(reader, &cert), CARDANO_SUCCESS);

  cardano_json_writer_t* json = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);

  // Act
  cardano_error_t error    = cardano_certificate_to_cip116_json(cert, json);
  char*           json_str = encode_json(json);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_STREQ(json_str, R"({"tag":"registration","credential":{"tag":"pubkey_hash","value":"00000000000000000000000000000000000000000000000000000000"},"coin":"0"})");

  // Cleanup
  cardano_json_writer_unref(&json);
  cardano_certificate_unref(&cert);
  cardano_cbor_reader_unref(&reader);
  free(json_str);
}

TEST(cardano_certificate_to_cip116_json, returnsErrorIfCertIsNull)
{
  cardano_json_writer_t* json  = cardano_json_writer_new(CARDANO_JSON_FORMAT_COMPACT);
  cardano_error_t        error = cardano_certificate_to_cip116_json(nullptr, json);
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
  cardano_json_writer_unref(&json);
}

TEST(cardano_certificate_to_cip116_json, returnsErrorIfWriterIsNull)
{
  cardano_error_t error = cardano_certificate_to_cip116_json((cardano_certificate_t*)"", nullptr);
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}