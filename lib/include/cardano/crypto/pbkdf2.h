/**
 * \file pbkdf2.h
 *
 * \author angel.castillo
 * \date   Mar 02, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_PBKDF2_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_PBKDF2_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Performs key derivation using the PBKDF2 algorithm with HMAC-SHA512.
 *
 * PBKDF2 (Password-Based Key Derivation Function 2) is a key derivation function that has a sliding computational cost,
 * aimed to reduce vulnerabilities to brute force attacks. It applies a pseudorandom function, such as HMAC-SHA512,
 * to the input password along with a salt value and repeats the process multiple times to produce a derived key.
 * The iterations parameter controls the number of times the pseudorandom function is applied, which increases
 * the computational cost and the time required to compute the derived key, thereby enhancing security.
 *
 * \param[in] password The input password from which the key is derived.
 * \param[in] password_length The length of the password.
 * \param[in] salt A cryptographic salt.
 * \param[in] salt_length The length of the salt.
 * \param[in] iterations The number of iterations specifies how many times the pseudorandom function is applied.
 * \param[out] derived_key The buffer where the derived key will be stored.
 * \param[in] derived_key_length The desired length of the derived key.
 *
 * \return A \ref cardano_error_t indicating the result of the operation: \ref CARDANO_SUCCESS on success,
 *         or an appropriate error code indicating the failure reason. Refer to \ref cardano_error_t documentation
 *         for details on possible error codes.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_crypto_pbkdf2_hmac_sha512(
  const byte_t* password,
  size_t        password_length,
  const byte_t* salt,
  size_t        salt_length,
  uint32_t      iterations,
  byte_t*       derived_key,
  size_t        derived_key_length);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_PBKDF2_H