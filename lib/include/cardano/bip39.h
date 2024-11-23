/**
 * \file bip39.h
 *
 * \author angel.castillo
 * \date   Nov 22, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BIP39S_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BIP39S_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Converts entropy into a BIP-39 mnemonic word sequence.
 *
 * This function takes entropy (a binary sequence) and converts it into a corresponding
 * BIP-39 mnemonic sequence of words. The mnemonic words are pointers to a static,
 * preloaded wordlist and do not require dynamic memory management. The caller must
 * not attempt to free the returned pointers.
 *
 * \param[in] entropy        Pointer to the entropy buffer. Must not be NULL.
 * \param[in] entropy_size   Size of the entropy in bytes. Supported sizes are 16, 20, 24, 28, or 32 bytes.
 * \param[out] words         Array of pointers to char* where the mnemonic words will be stored.
 *                           The caller must provide an array of size 24 to hold the maximum possible words.
 *                           Each element in the array will point to a statically allocated string.
 * \param[out] word_count    Pointer to a size_t where the number of words in the mnemonic will be stored.
 *                           The number of words depends on the entropy size:
 *                             - 16 bytes (128 bits) → 12 words
 *                             - 20 bytes (160 bits) → 15 words
 *                             - 24 bytes (192 bits) → 18 words
 *                             - 28 bytes (224 bits) → 21 words
 *                             - 32 bytes (256 bits) → 24 words
 *
 * \return On success, returns CARDANO_SUCCESS. On failure, returns an error code indicating the
 * reason for failure.
 *
 * \note
 * - This function only supports the **English BIP-39 wordlist**.
 * - The function does not allocate memory for the words; it simply assigns pointers
 *   to statically allocated strings from the preloaded English wordlist.
 * - The caller must not free the pointers in the `words` array as they point to
 *   static memory.
 *
 * \code{.c}
 * byte_t entropy[32] = { 256 bits of entropy  };
 * char* words[24] = { 0 };
 * size_t word_count = 0U;
 *
 * cardano_error_t result = cardano_bip39_entropy_to_mnemonic_words(entropy, sizeof(entropy), words, &word_count);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Mnemonic words:\n");
 *
 *   for (size_t i = 0; i < word_count; ++i)
 *   {
 *     printf("%s ", words[i]);
 *   }
 *
 *   printf("\n");
 * }
 * else
 * {
 *    printf("Failed to create mnemonic words: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_bip39_entropy_to_mnemonic_words(
  const byte_t* entropy,
  size_t        entropy_size,
  const char**  words,
  size_t*       word_count);

/**
 * \brief Converts a BIP-39 mnemonic word sequence back into entropy.
 *
 * This function takes a mnemonic sequence of BIP-39 words and converts it back
 * into the corresponding binary entropy. The provided mnemonic words must belong
 * to the English BIP-39 wordlist. The resulting entropy will be written to the
 * provided buffer.
 *
 * \param[in] words            Array of pointers to the mnemonic words. Each word must be a valid
 *                             BIP-39 English word. Must not be NULL.
 * \param[in] word_count       Number of words in the mnemonic sequence. Supported values are:
 *                               - 12 words (128-bit entropy)
 *                               - 15 words (160-bit entropy)
 *                               - 18 words (192-bit entropy)
 *                               - 21 words (224-bit entropy)
 *                               - 24 words (256-bit entropy)
 * \param[out] entropy         Pointer to the buffer where the resulting entropy will be stored.
 *                             Must not be NULL.
 * \param[in] entropy_buf_size Size of the entropy buffer provided (in bytes). Must be at least
 *                             the size of the expected entropy corresponding to the word count
 *                             (16–32 bytes).
 * \param[out] entropy_size    Pointer to a size_t where the actual size of the resulting entropy
 *                             (in bytes) will be stored. Must not be NULL.
 *
 * \return On success, returns `CARDANO_SUCCESS`. On failure, returns an error code indicating the
 * reason for failure.
 *
 * \note
 * - This function only supports the **English BIP-39 wordlist**.
 * - The provided mnemonic words must be valid and match the BIP-39 specification.
 * - The caller must ensure the entropy buffer is large enough to accommodate the resulting entropy.
 *
 * \code{.c}
 * const char* words[12] = {
 *   "abandon", "abandon", "abandon", "abandon", "abandon", "abandon",
 *   "abandon", "abandon", "abandon", "abandon", "abandon", "about"
 * };
 * uint8_t entropy[32] = { 0 };
 * size_t entropy_size = 0U;
 *
 * cardano_error_t result = cardano_bip39_mnemonic_words_to_entropy(
 *     words,
 *     12,
 *     entropy,
 *     sizeof(entropy),  // Provide the size of the buffer
 *     &entropy_size     // Output parameter for the actual entropy size
 * );
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Entropy (size: %zu bytes): ", entropy_size);
 *
 *   for (size_t i = 0; i < entropy_size; ++i)
 *   {
 *     printf("%02x", entropy[i]);
 *   }
 *
 *   printf("\n");
 * }
 * else
 * {
 *   printf("Failed to convert mnemonic words to entropy: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_bip39_mnemonic_words_to_entropy(
  const char** words,
  size_t       word_count,
  byte_t*      entropy,
  size_t       entropy_buf_size,
  size_t*      entropy_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BIP39S_H