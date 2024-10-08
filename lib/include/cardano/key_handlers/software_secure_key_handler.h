/**
 * \file software_secure_key_handler.h
 *
 * \author angel.castillo
 * \date   Oct 06, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SOFTWARE_SECURE_KEY_HANDLER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SOFTWARE_SECURE_KEY_HANDLER_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/key_handlers/secure_key_handler.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Callback function type for securely retrieving a passphrase.
 *
 * The `cardano_get_passphrase_func_t` typedef defines the signature for a callback function responsible
 * for retrieving a passphrase, which must be stored in the provided buffer. The buffer must be large
 * enough to hold the entire passphrase.
 *
 * This function is expected to:
 * - Retrieve the passphrase securely and write it into the provided `buffer`.
 * - Ensure that the passphrase length does not exceed the `buffer_len` provided, and if it does
 *   return an error.
 *
 * \param buffer A pointer to the buffer where the passphrase will be written.
 * \param buffer_len The maximum length of the `buffer` to hold the passphrase.
 *
 * \returns `cardano_error_t` indicating success or the type of error encountered while retrieving the passphrase.
 *
 * \note The software key handler ensures that the passphrase is securely erased from memory immediately after it is no
 * longer needed. The memory is wiped using measures that prevent the passphrase from lingering in memory or being retained in
 * registers, caches, or other hardware components that could expose it. These precautions include:
 * - Ensuring that the passphrase is overwritten completely, even in cases where compiler optimizations might
 *   otherwise skip the memory clearing.
 * - Preventing sensitive data from being inadvertently copied or cached in temporary memory locations.
 * - Guaranteeing that even if the program crashes, the sensitive data will not be left exposed in memory.
 *
 * Implementers of this callback must take equivalent precautions, ensuring that any sensitive data used
 * within the callback, such as temporary buffers, is securely erased after use to protect against
 * potential memory inspection or side-channel attacks.
 */
typedef cardano_error_t (*cardano_get_passphrase_func_t)(byte_t* buffer, size_t buffer_len);

/**
 * \brief Creates a new software-based secure key handler with encrypted entropy.
 *
 * The `cardano_software_secure_key_handler_new` function initializes a software-based secure key handler
 * for managing cryptographic key operations, using provided entropy bytes and a passphrase for encryption.
 * This key handler ensures the secure management and encryption of sensitive cryptographic material.
 *
 * Upon creation, the handler immediately encrypts the provided entropy bytes using the given passphrase.
 * All intermediary sensitive data used in the process, such as unencrypted entropy and passphrase,
 * is securely wiped from memory once the encryption process is complete.
 *
 * This secure key handler will:
 * - Encrypt the entropy bytes with the provided passphrase for secure storage.
 * - Use the `get_passphrase` callback to retrieve the passphrase when needed for decrypting the entropy
 *   during cryptographic operations.
 * - Ensure that any decrypted entropy is wiped from memory immediately after use, employing secure
 *   memory clearing measures that prevent sensitive data from lingering in memory.
 *
 * \param entropy_bytes A pointer to a buffer containing entropy bytes used to generate cryptographic key material.
 *                      This entropy should be high-quality and random.
 * \param entropy_bytes_len The length of the `entropy_bytes` buffer.
 * \param get_passphrase A callback function used to securely retrieve the passphrase during cryptographic operations.
 * \param passphrase A pointer to the buffer containing the passphrase, which will be used to encrypt the entropy.
 * \param passphrase_len The length of the `passphrase` buffer.
 * \param secure_key_handler A pointer to the resulting secure key handler. This handler manages cryptographic operations
 *                           such as signing and key derivation.
 *
 * \returns `cardano_error_t` indicating success or the type of error encountered during initialization.
 *
 * \note This function securely wipes all intermediary sensitive data from memory, once they have been used for encryption.
 * The caller must also ensure that both the passphrase and entropy bytes are securely erased from memory after
 * calling this function.
 *
 * \note The `get_passphrase` callback is invoked every time the key material is required. The seed is decrypted only
 * for the short period needed to perform the cryptographic operation, after which it is securely wiped from memory.
 *
 * \see cardano_secure_key_handler_t for the public interface to interact with this handler.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_software_secure_key_handler_new(
  const byte_t*                  entropy_bytes,
  size_t                         entropy_bytes_len,
  const byte_t*                  passphrase,
  size_t                         passphrase_len,
  cardano_get_passphrase_func_t  get_passphrase,
  cardano_secure_key_handler_t** secure_key_handler);

/**
 * \brief Deserializes a previously serialized software-based secure key handler.
 *
 * The `cardano_software_secure_key_handler_deserialize` function initializes a secure key handler from
 * serialized data. The serialized data is expected to be created using the \ref cardano_secure_key_handler_serialize function of a secure
 * software key handler.
 *
 * \param serialized_data A pointer to the buffer containing the serialized representation of the secure key handler.
 *                        This data should have been generated using the \ref cardano_secure_key_handler_serialize function of a secure software key handler.
 * \param serialized_data_len The length of the `serialized_data` buffer.
 * \param get_passphrase A callback function that will be used to securely retrieve the passphrase when sensitive data
 *                       must be decrypted.
 * \param secure_key_handler A pointer to the resulting secure key handler, which can now be used for cryptographic
 *                           operations such as signing and key derivation. The key handler will only decrypt sensitive
 *                           data when needed, invoking the `get_passphrase` callback at those times.
 *
 * \returns `cardano_error_t` indicating success or the type of error encountered during deserialization.
 *
 * \note The key handler will not decrypt any sensitive data during the deserialization process. The sensitive key
 * material will remain encrypted, and the `get_passphrase` callback will be invoked to retrieve the passphrase
 * only when needed to decrypt secrets for specific cryptographic operations.
 *
 * \see cardano_secure_key_handler_t for the public interface to interact with this handler.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_software_secure_key_handler_deserialize(
  const byte_t*                  serialized_data,
  const size_t                   serialized_data_len,
  cardano_get_passphrase_func_t  get_passphrase,
  cardano_secure_key_handler_t** secure_key_handler);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SOFTWARE_SECURE_KEY_HANDLER_H