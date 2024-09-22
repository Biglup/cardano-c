/**
 * \file bootstrap_witness.cpp
 *
 * \author angel.castillo
 * \date   Sep 22, 2024
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
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/witness_set/bootstrap_witness.h>

#include "tests/allocators_helpers.h"

#include <allocators.h>

#include <gmock/gmock.h>

/* CONSTANTS *****************************************************************/

static const char* CBOR            = "8458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a0";
static const char* VKEY_CBOR       = "3D4017C3E843895A92B70AA74D1B7EBC9C982CCF2EC4968CC0CD55F12AF4660C";
static const char* SIGNATURE_CBOR  = "6291D657DEEC24024827E69C3ABE01A30CE548A284743A445E3680D7DB5AC3AC18FF9B538D16F290AE67F760984DC6594A7C15E9716ED28DC027BECEEA1EC40A";
static const char* CHAIN_CODE_CBOR = "0000000000000000000000000000000000000000000000000000000000000000";
static const char* ATTRIBUTES_CBOR = "A0";

/* STATIC FUNCTIONS **********************************************************/

/**
 * Creates a new default instance of the witness.
 * @return A new instance of the witness.
 */
static cardano_bootstrap_witness_t*
new_default_witness()
{
  cardano_bootstrap_witness_t* bootstrap_witness = NULL;
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));
  cardano_error_t              result            = cardano_bootstrap_witness_from_cbor(reader, &bootstrap_witness);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  cardano_cbor_reader_unref(&reader);

  return bootstrap_witness;
};

/**
 * Creates a new default instance of the cardano_ed25519_public_key_t.
 *
 * @return A new instance of the cardano_ed25519_public_key_t.
 */
static cardano_ed25519_public_key_t*
new_default_vkey()
{
  cardano_ed25519_public_key_t* key = NULL;

  cardano_error_t result = cardano_ed25519_public_key_from_hex(VKEY_CBOR, strlen(VKEY_CBOR), &key);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  return key;
}

/**
 * Creates a new default instance of the cardano_ed25519_signature_t.
 *
 * @return A new instance of the cardano_ed25519_signature_t.
 */
static cardano_ed25519_signature_t*
new_default_signature()
{
  cardano_ed25519_signature_t* sig = NULL;

  cardano_error_t result = cardano_ed25519_signature_from_hex(SIGNATURE_CBOR, strlen(SIGNATURE_CBOR), &sig);

  EXPECT_THAT(result, CARDANO_SUCCESS);

  return sig;
}

/**
 * Creates a new default instance of the cardano_buffer_t.
 *
 * @return A new instance of the cardano_buffer_t.
 */
static cardano_buffer_t*
new_default_buffer(const char* cbor)
{
  cardano_buffer_t* sig = cardano_buffer_from_hex(cbor, strlen(cbor));

  return sig;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_bootstrap_witness_ref, increasesTheReferenceCount)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = new_default_witness();
  EXPECT_NE(bootstrap_witness, nullptr);

  // Act
  cardano_bootstrap_witness_ref(bootstrap_witness);

  // Assert
  EXPECT_THAT(bootstrap_witness, testing::Not((cardano_bootstrap_witness_t*)nullptr));
  EXPECT_EQ(cardano_bootstrap_witness_refcount(bootstrap_witness), 2);

  // Cleanup - We need to unref twice since one reference was added.
  cardano_bootstrap_witness_unref(&bootstrap_witness);
  cardano_bootstrap_witness_unref(&bootstrap_witness);
}

TEST(cardano_bootstrap_witness_ref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_bootstrap_witness_ref(nullptr);
}

TEST(cardano_bootstrap_witness_unref, doesntCrashIfGivenAPtrToANullPtr)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = nullptr;

  // Act
  cardano_bootstrap_witness_unref(&bootstrap_witness);
}

TEST(cardano_bootstrap_witness_unref, doesntCrashIfGivenANullPtr)
{
  // Act
  cardano_bootstrap_witness_unref((cardano_bootstrap_witness_t**)nullptr);
}

TEST(cardano_bootstrap_witness_unref, decreasesTheReferenceCount)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = new_default_witness();
  EXPECT_NE(bootstrap_witness, nullptr);

  // Act
  cardano_bootstrap_witness_ref(bootstrap_witness);
  size_t ref_count = cardano_bootstrap_witness_refcount(bootstrap_witness);

  cardano_bootstrap_witness_unref(&bootstrap_witness);
  size_t updated_ref_count = cardano_bootstrap_witness_refcount(bootstrap_witness);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
}

TEST(cardano_bootstrap_witness_unref, freesTheObjectIfReferenceReachesZero)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = new_default_witness();
  EXPECT_NE(bootstrap_witness, nullptr);

  // Act
  cardano_bootstrap_witness_ref(bootstrap_witness);
  size_t ref_count = cardano_bootstrap_witness_refcount(bootstrap_witness);

  cardano_bootstrap_witness_unref(&bootstrap_witness);
  size_t updated_ref_count = cardano_bootstrap_witness_refcount(bootstrap_witness);

  cardano_bootstrap_witness_unref(&bootstrap_witness);

  // Assert
  EXPECT_EQ(ref_count, 2);
  EXPECT_EQ(updated_ref_count, 1);
  EXPECT_EQ(bootstrap_witness, (cardano_bootstrap_witness_t*)nullptr);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
}

TEST(cardano_bootstrap_witness_refcount, returnsZeroIfGivenANullPtr)
{
  // Act
  size_t ref_count = cardano_bootstrap_witness_refcount(nullptr);

  // Assert
  EXPECT_EQ(ref_count, 0);
}

TEST(cardano_bootstrap_witness_set_last_error, doesNothingWhenObjectIsNull)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = nullptr;
  const char*                  message           = "This is a test message";

  // Act
  cardano_bootstrap_witness_set_last_error(bootstrap_witness, message);

  // Assert
  EXPECT_STREQ(cardano_bootstrap_witness_get_last_error(bootstrap_witness), "Object is NULL.");
}

TEST(cardano_bootstrap_witness_set_last_error, doesNothingWhenWhenMessageIsNull)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = new_default_witness();
  EXPECT_NE(bootstrap_witness, nullptr);

  const char* message = nullptr;

  // Act
  cardano_bootstrap_witness_set_last_error(bootstrap_witness, message);

  // Assert
  EXPECT_STREQ(cardano_bootstrap_witness_get_last_error(bootstrap_witness), "");

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
}

TEST(cardano_bootstrap_witness_from_cbor, returnsErrorIfReaderIsNull)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = NULL;

  // Act
  cardano_error_t result = cardano_bootstrap_witness_from_cbor(nullptr, &bootstrap_witness);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_bootstrap_witness_from_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_reader_t* reader = cardano_cbor_reader_from_hex(CBOR, strlen(CBOR));

  // Act
  cardano_error_t result = cardano_bootstrap_witness_from_cbor(reader, nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_bootstrap_witness_to_cbor, canSerialize)
{
  // Arrange
  cardano_cbor_writer_t*       writer  = cardano_cbor_writer_new();
  cardano_bootstrap_witness_t* witness = new_default_witness();
  EXPECT_NE(witness, nullptr);

  // Act
  cardano_error_t result = cardano_bootstrap_witness_to_cbor(witness, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_SUCCESS);

  size_t hex_size = cardano_cbor_writer_get_hex_size(writer);
  char*  hex      = (char*)malloc(hex_size);

  ASSERT_EQ(cardano_cbor_writer_encode_hex(writer, hex, hex_size), CARDANO_SUCCESS);

  EXPECT_STREQ(hex, CBOR);

  // Cleanup
  cardano_bootstrap_witness_unref(&witness);
  cardano_cbor_writer_unref(&writer);
  free(hex);
}

TEST(cardano_bootstrap_witness_to_cbor, returnsErrorIfCertIsNull)
{
  // Arrange
  cardano_cbor_writer_t* writer = cardano_cbor_writer_new();

  // Act
  cardano_error_t result = cardano_bootstrap_witness_to_cbor(nullptr, writer);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_cbor_writer_unref(&writer);
}

TEST(cardano_bootstrap_witness_to_cbor, returnsErrorIfWriterIsNull)
{
  // Act
  cardano_error_t result = cardano_bootstrap_witness_to_cbor((cardano_bootstrap_witness_t*)"", nullptr);

  // Assert
  ASSERT_EQ(result, CARDANO_POINTER_IS_NULL);
}

// Cert specific tests

TEST(cardano_bootstrap_witness_new, canCreateNewInstance)
{
  // Act
  cardano_ed25519_public_key_t* vk   = new_default_vkey();
  cardano_ed25519_signature_t*  sig  = new_default_signature();
  cardano_buffer_t*             cc   = new_default_buffer(CHAIN_CODE_CBOR);
  cardano_buffer_t*             attr = new_default_buffer(ATTRIBUTES_CBOR);

  cardano_bootstrap_witness_t* bootstrap_witness = NULL;

  cardano_error_t result = cardano_bootstrap_witness_new(vk, sig, cc, attr, &bootstrap_witness);

  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Assert
  EXPECT_NE(bootstrap_witness, nullptr);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
  cardano_ed25519_public_key_unref(&vk);
  cardano_ed25519_signature_unref(&sig);
  cardano_buffer_unref(&cc);
  cardano_buffer_unref(&attr);
}

TEST(cardano_bootstrap_witness_new, returnsErrorIfFirstArgIsNull)
{
  // Act
  cardano_bootstrap_witness_t* bootstrap_witness = NULL;

  cardano_error_t result = cardano_bootstrap_witness_new(nullptr, nullptr, nullptr, nullptr, &bootstrap_witness);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_bootstrap_witness_new, returnsErrorIfSecondArgIsNull)
{
  // Act
  cardano_bootstrap_witness_t* bootstrap_witness = NULL;

  cardano_error_t result = cardano_bootstrap_witness_new((cardano_ed25519_public_key_t*)"", nullptr, nullptr, nullptr, &bootstrap_witness);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);
}

TEST(cardano_bootstrap_witness_new, returnsErrorIfThirdArgIsNull)
{
  // Act

  cardano_error_t result = cardano_bootstrap_witness_new((cardano_ed25519_public_key_t*)"", (cardano_ed25519_signature_t*)"", nullptr, nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
}

TEST(cardano_bootstrap_witness_new, returnsErrorIfFourthArgIsNull)
{
  // Act

  cardano_error_t result = cardano_bootstrap_witness_new((cardano_ed25519_public_key_t*)"", (cardano_ed25519_signature_t*)"", (cardano_buffer_t*)"", nullptr, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
}

TEST(cardano_bootstrap_witness_new, returnsErrorIfWitnessArgIsNull)
{
  // Act

  cardano_error_t result = cardano_bootstrap_witness_new((cardano_ed25519_public_key_t*)"", (cardano_ed25519_signature_t*)"", (cardano_buffer_t*)"", (cardano_buffer_t*)"", nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
}

TEST(cardano_bootstrap_witness_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  cardano_ed25519_public_key_t* vk   = new_default_vkey();
  cardano_ed25519_signature_t*  sig  = new_default_signature();
  cardano_buffer_t*             cc   = new_default_buffer(CHAIN_CODE_CBOR);
  cardano_buffer_t*             attr = new_default_buffer(ATTRIBUTES_CBOR);

  // Act
  cardano_bootstrap_witness_t* bootstrap_witness = NULL;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  cardano_error_t result = cardano_bootstrap_witness_new(vk, sig, cc, attr, &bootstrap_witness);

  // Assert
  EXPECT_EQ(result, CARDANO_MEMORY_ALLOCATION_FAILED);

  // Cleanup
  cardano_buffer_unref(&cc);
  cardano_buffer_unref(&attr);
  cardano_ed25519_public_key_unref(&vk);
  cardano_ed25519_signature_unref(&sig);
  cardano_bootstrap_witness_unref(&bootstrap_witness);

  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_bootstrap_witness_from_cbor, returnsErrorIfDoesntStartWithArray)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = NULL;
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex("01", strlen("01"));

  // Act
  cardano_error_t result = cardano_bootstrap_witness_from_cbor(reader, &bootstrap_witness);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_UNEXPECTED_CBOR_TYPE);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_bootstrap_witness_from_cbor, returnsErrorIfInvalidKey)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = NULL;
  const char*                  cbor              = "84ef203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a0";
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_bootstrap_witness_from_cbor(reader, &bootstrap_witness);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_bootstrap_witness_from_cbor, returnsErrorIfInvalidSignature)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = NULL;
  const char*                  cbor              = "8458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660cef406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a5820000000000000000000000000000000000000000000000000000000000000000041a0";
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_bootstrap_witness_from_cbor(reader, &bootstrap_witness);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_bootstrap_witness_from_cbor, returnsErrorIfInvalidChainCode)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = NULL;
  const char*                  cbor              = "8458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40aef20000000000000000000000000000000000000000000000000000000000000000041a0";
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_bootstrap_witness_from_cbor(reader, &bootstrap_witness);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
  cardano_cbor_reader_unref(&reader);
}

TEST(cardano_bootstrap_witness_from_cbor, returnsErrorIfInvalidAttributes)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = NULL;
  const char*                  cbor              = "8458203d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c58406291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28dc027beceea1ec40a58200000000000000000000000000000000000000000000000000000000000000000efa0";
  cardano_cbor_reader_t*       reader            = cardano_cbor_reader_from_hex(cbor, strlen(cbor));

  // Act
  cardano_error_t result = cardano_bootstrap_witness_from_cbor(reader, &bootstrap_witness);

  // Assert
  ASSERT_EQ(result, CARDANO_ERROR_DECODING);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
  cardano_cbor_reader_unref(&reader);
}

// Getters and Setters

TEST(cardano_bootstrap_witness_get_vkey, canGetVkey)
{
  // Arrange
  cardano_bootstrap_witness_t*  bootstrap_witness = new_default_witness();
  cardano_ed25519_public_key_t* vkey              = new_default_vkey();

  // Act
  cardano_ed25519_public_key_t* vkey2 = cardano_bootstrap_witness_get_vkey(bootstrap_witness);

  // Assert
  EXPECT_NE(vkey2, nullptr);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
  cardano_ed25519_public_key_unref(&vkey);
  cardano_ed25519_public_key_unref(&vkey2);
}

TEST(cardano_bootstrap_witness_get_vkey, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_ed25519_public_key_t* vkey = cardano_bootstrap_witness_get_vkey(nullptr);

  // Assert
  EXPECT_EQ(vkey, nullptr);
}

TEST(cardano_bootstrap_witness_set_vkey, canSetVkey)
{
  // Arrange
  cardano_bootstrap_witness_t*  bootstrap_witness = new_default_witness();
  cardano_ed25519_public_key_t* vkey              = new_default_vkey();

  // Act
  cardano_error_t result = cardano_bootstrap_witness_set_vkey(bootstrap_witness, vkey);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
  cardano_ed25519_public_key_unref(&vkey);
}

TEST(cardano_bootstrap_witness_set_vkey, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_ed25519_public_key_t* vkey = new_default_vkey();

  // Act
  cardano_error_t result = cardano_bootstrap_witness_set_vkey(nullptr, vkey);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_ed25519_public_key_unref(&vkey);
}

TEST(cardano_bootstrap_witness_set_vkey, returnsErrorIfVkeyIsNull)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = new_default_witness();

  // Act
  cardano_error_t result = cardano_bootstrap_witness_set_vkey(bootstrap_witness, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
}

TEST(cardano_bootstrap_witness_get_signature, canGetSignature)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = new_default_witness();
  cardano_ed25519_signature_t* sig               = new_default_signature();

  // Act
  cardano_ed25519_signature_t* sig2 = cardano_bootstrap_witness_get_signature(bootstrap_witness);

  // Assert
  EXPECT_NE(sig2, nullptr);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
  cardano_ed25519_signature_unref(&sig);
  cardano_ed25519_signature_unref(&sig2);
}

TEST(cardano_bootstrap_witness_get_signature, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_ed25519_signature_t* sig = cardano_bootstrap_witness_get_signature(nullptr);

  // Assert
  EXPECT_EQ(sig, nullptr);
}

TEST(cardano_bootstrap_witness_set_signature, canSetSignature)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = new_default_witness();
  cardano_ed25519_signature_t* sig               = new_default_signature();

  // Act
  cardano_error_t result = cardano_bootstrap_witness_set_signature(bootstrap_witness, sig);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
  cardano_ed25519_signature_unref(&sig);
}

TEST(cardano_bootstrap_witness_set_signature, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_ed25519_signature_t* sig = new_default_signature();

  // Act
  cardano_error_t result = cardano_bootstrap_witness_set_signature(nullptr, sig);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_ed25519_signature_unref(&sig);
}

TEST(cardano_bootstrap_witness_set_signature, returnsErrorIfSignatureIsNull)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = new_default_witness();

  // Act
  cardano_error_t result = cardano_bootstrap_witness_set_signature(bootstrap_witness, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
}

TEST(cardano_bootstrap_witness_get_chain_code, canGetChainCode)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = new_default_witness();
  cardano_buffer_t*            cc                = new_default_buffer(CHAIN_CODE_CBOR);

  // Act
  cardano_buffer_t* cc2 = cardano_bootstrap_witness_get_chain_code(bootstrap_witness);

  // Assert
  EXPECT_NE(cc2, nullptr);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
  cardano_buffer_unref(&cc);
  cardano_buffer_unref(&cc2);
}

TEST(cardano_bootstrap_witness_get_chain_code, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_buffer_t* cc = cardano_bootstrap_witness_get_chain_code(nullptr);

  // Assert
  EXPECT_EQ(cc, nullptr);
}

TEST(cardano_bootstrap_witness_set_chain_code, canSetChainCode)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = new_default_witness();
  cardano_buffer_t*            cc                = new_default_buffer(CHAIN_CODE_CBOR);

  // Act
  cardano_error_t result = cardano_bootstrap_witness_set_chain_code(bootstrap_witness, cc);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
  cardano_buffer_unref(&cc);
}

TEST(cardano_bootstrap_witness_set_chain_code, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_buffer_t* cc = new_default_buffer(CHAIN_CODE_CBOR);

  // Act
  cardano_error_t result = cardano_bootstrap_witness_set_chain_code(nullptr, cc);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_buffer_unref(&cc);
}

TEST(cardano_bootstrap_witness_set_chain_code, returnsErrorIfChainCodeIsNull)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = new_default_witness();

  // Act
  cardano_error_t result = cardano_bootstrap_witness_set_chain_code(bootstrap_witness, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
}

TEST(cardano_bootstrap_witness_get_attributes, canGetAttributes)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = new_default_witness();
  cardano_buffer_t*            attr              = new_default_buffer(ATTRIBUTES_CBOR);

  // Act
  cardano_buffer_t* attr2 = cardano_bootstrap_witness_get_attributes(bootstrap_witness);

  // Assert
  EXPECT_NE(attr2, nullptr);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
  cardano_buffer_unref(&attr);
  cardano_buffer_unref(&attr2);
}

TEST(cardano_bootstrap_witness_get_attributes, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_buffer_t* attr = cardano_bootstrap_witness_get_attributes(nullptr);

  // Assert
  EXPECT_EQ(attr, nullptr);
}

TEST(cardano_bootstrap_witness_set_attributes, canSetAttributes)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = new_default_witness();
  cardano_buffer_t*            attr              = new_default_buffer(ATTRIBUTES_CBOR);

  // Act
  cardano_error_t result = cardano_bootstrap_witness_set_attributes(bootstrap_witness, attr);

  // Assert
  EXPECT_EQ(result, CARDANO_SUCCESS);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
  cardano_buffer_unref(&attr);
}

TEST(cardano_bootstrap_witness_set_attributes, returnsErrorIfObjectIsNull)
{
  // Arrange
  cardano_buffer_t* attr = new_default_buffer(ATTRIBUTES_CBOR);

  // Act
  cardano_error_t result = cardano_bootstrap_witness_set_attributes(nullptr, attr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_buffer_unref(&attr);
}

TEST(cardano_bootstrap_witness_set_attributes, returnsErrorIfAttributesIsNull)
{
  // Arrange
  cardano_bootstrap_witness_t* bootstrap_witness = new_default_witness();

  // Act
  cardano_error_t result = cardano_bootstrap_witness_set_attributes(bootstrap_witness, nullptr);

  // Assert
  EXPECT_EQ(result, CARDANO_POINTER_IS_NULL);

  // Cleanup
  cardano_bootstrap_witness_unref(&bootstrap_witness);
}