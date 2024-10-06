/**
 * \file pbkdf2.cpp
 *
 * \author angel.castillo
 * \date   Sep 09, 2023
 *
 * \section LICENSE
 *
 * Copyright 2023 Biglup Labs
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

#include <cardano/crypto/pbkdf2.h>

#include <gmock/gmock.h>

/* TEST VECTORS **************************************************************/

typedef struct
{
    const byte_t* password;
    size_t        passwordLength;
    const byte_t* salt;
    size_t        saltLength;
    uint32_t      iterations;
    size_t        dkLen;
    const char*   expectedSha512;
} pbkdf2_hmac_sha512_vectors_t;

// Test vectors taken from https://github.com/browserify/pbkdf2/blob/master/test/fixtures.json
static const pbkdf2_hmac_sha512_vectors_t pbkdf2_hmac_sha512_test_vectors[] = {
  { (const byte_t*)"password", strlen("password"), (const byte_t*)"salt", strlen("salt"), 1, 32, "867f70cf1ade02cff3752599a3a53dc4af34c7a669815ae5d513554e1c8cf252" },
  { (const byte_t*)"password", strlen("password"), (const byte_t*)"salt", strlen("salt"), 2, 32, "e1d9c16aa681708a45f5c7c4e215ceb66e011a2e9f0040713f18aefdb866d53c" },
  { (const byte_t*)"password", strlen("password"), (const byte_t*)"salt", strlen("salt"), 1, 64, "867f70cf1ade02cff3752599a3a53dc4af34c7a669815ae5d513554e1c8cf252c02d470a285a0501bad999bfe943c08f050235d7d68b1da55e63f73b60a57fce" },
  { (const byte_t*)"password", strlen("password"), (const byte_t*)"salt", strlen("salt"), 2, 64, "e1d9c16aa681708a45f5c7c4e215ceb66e011a2e9f0040713f18aefdb866d53cf76cab2868a39b9f7840edce4fef5a82be67335c77a6068e04112754f27ccf4e" },
  { (const byte_t*)"password", strlen("password"), (const byte_t*)"salt", strlen("salt"), 4096, 32, "d197b1b33db0143e018b12f3d1d1479e6cdebdcc97c5c0f87f6902e072f457b5" },
  { (const byte_t*)"passwordPASSWORDpassword", strlen("passwordPASSWORDpassword"), (const byte_t*)"saltSALTsaltSALTsaltSALTsaltSALTsalt", strlen("saltSALTsaltSALTsaltSALTsaltSALTsalt"), 4096, 40, "8c0511f4c6e597c6ac6315d8f0362e225f3c501495ba23b868c005174dc4ee71115b59f9e60cd953" },
  { (const byte_t*)"pass\u00000word", 10, (const byte_t*)"sa\u00000lt", 6, 4096, 16, "336d14366099e8aac2c46c94a8f178d2" },
  { (const byte_t*)"password", strlen("password"), (const byte_t*)"salt", strlen("salt"), 1, 10, "867f70cf1ade02cff375" },
  { (const byte_t*)"password", strlen("password"), (const byte_t*)"salt", strlen("salt"), 1, 100, "867f70cf1ade02cff3752599a3a53dc4af34c7a669815ae5d513554e1c8cf252c02d470a285a0501bad999bfe943c08f050235d7d68b1da55e63f73b60a57fce7b532e206c2967d4c7d2ffa460539fc4d4e5eec70125d74c6c7cf86d25284f297907fcea" },
  { (const byte_t*)"password", 0, (const byte_t*)"salt", strlen("salt"), 1, 100, "00ef42cdbfc98d29db20976608e455567fdddf141f6eb03b5a85addd25974f5d2375bd5082b803e8f4cfa88ae1bd25256fcbddd2318676566ff2797792302aee6ca733014ec4a8969e9b4d25a196e71b38d7e3434496810e7ffedd58624f2fd53874cfa5" },
  { NULL, 0, (const byte_t*)"salt", strlen("salt"), 1, 100, "00ef42cdbfc98d29db20976608e455567fdddf141f6eb03b5a85addd25974f5d2375bd5082b803e8f4cfa88ae1bd25256fcbddd2318676566ff2797792302aee6ca733014ec4a8969e9b4d25a196e71b38d7e3434496810e7ffedd58624f2fd53874cfa5" },
  { NULL, 0, (const byte_t*)"salt", strlen("salt"), 19162, 32, "879094d1113e95e3bc05c4a2d2b2a66cbc7876d454ee3c886cdf1a14c72188c7" }
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_crypto_pbkdf2_hmac_sha512, correctlyComputesHashesForTestVectors)
{
  for (size_t i = 0; i < sizeof(pbkdf2_hmac_sha512_test_vectors) / sizeof(pbkdf2_hmac_sha512_test_vectors[0]); ++i)
  {
    const pbkdf2_hmac_sha512_vectors_t* vector = &pbkdf2_hmac_sha512_test_vectors[i];

    byte_t derived_key_buffer[vector->dkLen] = { 0 };

    cardano_error_t result = cardano_crypto_pbkdf2_hmac_sha512(
      vector->password,
      vector->passwordLength,
      vector->salt,
      vector->saltLength,
      vector->iterations,
      derived_key_buffer,
      vector->dkLen);

    EXPECT_EQ(result, CARDANO_SUCCESS);

    char derived_key_hex[2 * vector->dkLen + 1];

    for (size_t j = 0; j < vector->dkLen; ++j)
    {
      sprintf(&derived_key_hex[2 * j], "%02x", derived_key_buffer[j]);
    }

    derived_key_hex[2 * vector->dkLen] = '\0';

    EXPECT_STREQ(derived_key_hex, vector->expectedSha512);
  }
}

TEST(cardano_crypto_pbkdf2_hmac_sha512, returnErrorOnNullSalt)
{
  // Arrange
  byte_t derived_key[64U] = { 0 };

  // Act
  cardano_error_t result = cardano_crypto_pbkdf2_hmac_sha512(
    (const byte_t*)"password",
    strlen("password"),
    NULL,
    0U,
    1U,
    derived_key,
    sizeof(derived_key));

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_crypto_pbkdf2_hmac_sha512, returnErrorOnNullDerivedKey)
{
  // Act
  cardano_error_t result = cardano_crypto_pbkdf2_hmac_sha512(
    (const byte_t*)"password",
    strlen("password"),
    (const byte_t*)"salt",
    strlen("salt"),
    1U,
    NULL,
    0U);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_crypto_pbkdf2_hmac_sha512, returnErrorOnZeroSaltLength)
{
  // Arrange
  byte_t derived_key[64U] = { 0 };

  // Act
  cardano_error_t result = cardano_crypto_pbkdf2_hmac_sha512(
    (const byte_t*)"password",
    strlen("password"),
    (const byte_t*)"salt",
    0U,
    1U,
    derived_key,
    sizeof(derived_key));

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE);
}

TEST(cardano_crypto_pbkdf2_hmac_sha512, returnErrorOnZeroDerivedKeyLength)
{
  // Arrange
  byte_t derived_key[64U] = { 0 };

  // Act
  cardano_error_t result = cardano_crypto_pbkdf2_hmac_sha512(
    (const byte_t*)"password",
    strlen("password"),
    (const byte_t*)"salt",
    strlen("salt"),
    1U,
    derived_key,
    0U);

  // Assert
  EXPECT_EQ(result, CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE);
}
