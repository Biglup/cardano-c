/**
 * \file emip3.h
 *
 * \author angel.castillo
 * \date   Oct 08, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_EMIP3_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_EMIP3_H

/* INCLUDES ******************************************************************/

#include <cardano/buffer.h>
#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Encrypts data using the EMIP-003 standard encryption format with password-based key derivation.
 *
 * The `cardano_crypto_emip3_encrypt` function securely encrypts the provided \p data using a passphrase,
 * following the encryption scheme outlined in [EMIP-003](https://github.com/Emurgo/EmIPs/blob/master/specs/emip-003.md).
 * This method is particularly suitable for encrypting sensitive data, such as cryptographic keys,
 * before storing them on disk.
 *
 * \section encryption_methodology Encryption Methodology
 *
 * 1. **Password-Based Key Derivation (PBKDF2):**
 *    - Uses HMAC-SHA512 [RFC 6234] as the Pseudo-Random Function (PRF) for the derivation process.
 *    - A randomly generated 32-byte salt is applied.
 *    - Iterates 19,162 times (as defined in EMIP-003) to strengthen the key derivation process against brute-force attacks.
 *    - The derived key size is 32 bytes.
 *
 * 2. **ChaCha20Poly1305 Encryption:**
 *    - Encrypts the data using ChaCha20 [RFC 8439] with the derived key.
 *    - A randomly-initialized 12-byte nonce is used for the encryption process.
 *    - Poly1305 ensures authenticity and integrity of the encrypted data, providing AEAD (Authenticated Encryption with Associated Data).
 *    - An empty AAD (Additional Authenticated Data) is used.
 *    - The output includes the MAC (Message Authentication Code) for data verification.
 *
 * 3. **Output Format:**
 *    - The resulting encrypted data is concatenated as follows:
 *      - The 32-byte salt.
 *      - The 12-byte nonce.
 *      - The 16-byte MAC.
 *      - The encrypted data.
 *
 * \param[in] data The raw data to be encrypted.
 * \param[in] data_length The length of the raw data.
 * \param[in] passphrase The user-provided passphrase used for deriving the encryption key.
 * \param[in] passphrase_length The length of the passphrase.
 * \param[out] encrypted_data A pointer to a buffer where the resulting encrypted byte array will be stored.
 *             The caller is responsible for managing this buffer and securely wiping it after use.
 *
 * \return `cardano_error_t` An error code indicating success or failure.
 *
 * \note The passphrase and sensitive data are securely wiped from memory after encryption is completed.
 *
 * \see [EMIP-003](https://github.com/Emurgo/EmIPs/blob/master/specs/emip-003.md) for full specification details.
 *
 * Example:
 * \code
 * // Encrypt sensitive data using a passphrase
 * const byte_t* data = (const byte_t*)"My secret data";
 * const size_t data_len = strlen((const char*)data);
 * const byte_t* passphrase = (const byte_t*)"MyStrongPassphrase";
 * const size_t passphrase_len = strlen((const char*)passphrase);
 * cardano_buffer_t* encrypted_data = NULL;
 *
 * // Encrypt the data
 * cardano_error_t result = cardano_crypto_emip3_encrypt(data, data_len, passphrase, passphrase_len, &encrypted_data);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Encryption successful! Encrypted data length: %zu\n", encrypted_data->size);
 *
 *   cardano_buffer_unref(encrypted_data);
 * }
 * else
 * {
 *   printf("Encryption failed!\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_crypto_emip3_encrypt(
  const byte_t*      data,
  size_t             data_length,
  const byte_t*      passphrase,
  size_t             passphrase_length,
  cardano_buffer_t** encrypted_data);

/**
 * \brief Decrypts data that was encrypted using the EMIP-003 standard encryption format.
 *
 * The `cardano_crypto_emip3_decrypt` function securely decrypts the provided \p encrypted_data using a passphrase,
 * following the decryption scheme outlined in [EMIP-003](https://github.com/Emurgo/EmIPs/blob/master/specs/emip-003.md).
 * This function expects data that was encrypted using the `cardano_crypto_emip3_encrypt` function and will return
 * the original data in its raw form.
 *
 * \section decryption_methodology Decryption Methodology
 *
 * 1. **Password-Based Key Derivation (PBKDF2):**
 *    - The decryption process regenerates the key using PBKDF2, with the same parameters that were used during encryption.
 *    - HMAC-SHA512 is used as the Pseudo-Random Function (PRF) for key derivation, and the salt is extracted from the
 *      provided \p encrypted_data.
 *    - 19,162 iterations are used to produce a 32-byte key.
 *
 * 2. **ChaCha20Poly1305 Decryption:**
 *    - The derived key is used to decrypt the data, which is extracted from the \p encrypted_data.
 *    - The 12-byte nonce and the 16-byte MAC are also extracted from the encrypted data.
 *    - ChaCha20Poly1305 is then used to decrypt and authenticate the data, ensuring both confidentiality and integrity.
 *
 * 3. **Output Format:**
 *    - The decrypted data is returned as a buffer, with the size of the original data before encryption.
 *
 * \param[in] encrypted_data The encrypted data to be decrypted.
 * \param[in] encrypted_data_length The length of the encrypted data.
 * \param[in] passphrase The user-provided passphrase used for deriving the decryption key.
 * \param[in] passphrase_length The length of the passphrase.
 * \param[out] data A pointer to a buffer where the resulting decrypted byte array will be stored.
 *             The caller is responsible for managing this buffer and securely wiping it after use.
 *
 * \return `cardano_error_t` An error code indicating success or failure.
 *
 * \note It is important to securely wipe sensitive data from memory after its use. Before calling `cardano_buffer_unref`
 * to free the \p data, you must call `cardano_buffer_memzero` to ensure that the sensitive information is properly
 * erased from memory, reducing the risk of exposing private data.
 *
 * \see [EMIP-003](https://github.com/Emurgo/EmIPs/blob/master/specs/emip-003.md) for full specification details.
 *
 * Example:
 * \code
 *
 * const byte_t*     encrypted_data     = ...; // Encrypted data from the encryption function
 * const size_t      encrypted_data_len = ...; // Length of the encrypted data
 * const byte_t*     passphrase         = (const byte_t*)"MyStrongPassphrase";
 * const size_t      passphrase_len     = strlen((const char*)passphrase);
 * cardano_buffer_t* decrypted_data     = NULL;
 *
 * // Decrypt the data
 * cardano_error_t result = cardano_crypto_emip3_decrypt(encrypted_data, encrypted_data_len, passphrase, passphrase_len, &decrypted_data);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the decrypted data
 *   ...
 *   // Securely wipe sensitive data from memory before freeing the buffer
 *   cardano_buffer_memzero(decrypted_data);
 *   cardano_buffer_unref(decrypted_data);
 * }
 * else
 * {
 *   // Handle decryption failure
 *   printf("Decryption failed!\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_crypto_emip3_decrypt(
  const byte_t*      encrypted_data,
  size_t             encrypted_data_length,
  const byte_t*      passphrase,
  size_t             passphrase_length,
  cardano_buffer_t** data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_EMIP3_H