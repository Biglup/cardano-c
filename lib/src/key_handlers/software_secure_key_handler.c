/**
 * \file secure_key_handler.c
 *
 * \author angel.castillo
 * \date   Oct 06, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License") {}
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

#include <cardano/crypto/bip32_private_key.h>
#include <cardano/crypto/bip32_public_key.h>
#include <cardano/crypto/emip3.h>
#include <cardano/crypto/pbkdf2.h>
#include <cardano/key_handlers/secure_key_handler.h>
#include <cardano/key_handlers/secure_key_handler_impl.h>
#include <cardano/key_handlers/software_secure_key_handler.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../string_safe.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t SW_SECURE_KEY_BINARY_FORMAT_HANDLER_MAGIC = 0x0A0A0A0A;
static const int64_t SW_SECURE_KEY_BINARY_FORMAT_VERSION       = 0x01;

/* STRUCTURES ****************************************************************/

typedef struct software_secure_key_handler_context_t
{
    cardano_object_t              base;
    cardano_buffer_t*             encrypted_entropy;
    cardano_bip32_public_key_t*   root_public_key;
    cardano_get_passphrase_func_t get_passphrase;
} software_secure_key_handler_context_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a secure_key_handler object.
 *
 * This function is responsible for properly deallocating a secure_key_handler object (`cardano_secure_key_handler_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the secure_key_handler object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_secure_key_handler_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the secure_key_handler
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_secure_key_handler_deallocate(void* object)
{
  assert(object != NULL);

  software_secure_key_handler_context_t* context = (software_secure_key_handler_context_t*)object;

  cardano_bip32_public_key_unref(&context->root_public_key);
  cardano_buffer_unref(&context->encrypted_entropy);

  _cardano_free(object);
}

/**
 * \brief Creates a new secure_key_handler context object.
 *
 * \return A pointer to the newly created secure_key_handler context object, or `NULL` if the operation failed.
 */
static software_secure_key_handler_context_t*
cardano_software_secure_key_handler_context_new(void)
{
  software_secure_key_handler_context_t* data = _cardano_malloc(sizeof(software_secure_key_handler_context_t));

  if (data == NULL)
  {
    return NULL;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_secure_key_handler_deallocate;
  data->encrypted_entropy  = NULL;
  data->root_public_key    = NULL;

  return data;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_software_secure_key_handler_new(
  const byte_t*                  entropy_bytes,
  size_t                         entropy_bytes_len,
  const byte_t*                  passphrase,
  size_t                         passphrase_len,
  cardano_get_passphrase_func_t  get_passphrase,
  cardano_secure_key_handler_t** secure_key_handler)
{
  cardano_secure_key_handler_impl_t impl = { 0 };

  cardano_safe_memcpy(impl.name, 256, "Software Secure Key Handler", cardano_safe_strlen("Software Secure Key Handler", 256));

  if (entropy_bytes == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (entropy_bytes_len == 0)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  if (passphrase == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (passphrase_len == 0)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  if (get_passphrase == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (secure_key_handler == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  software_secure_key_handler_context_t* context = cardano_software_secure_key_handler_context_new();

  if (context == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_crypto_emip3_encrypt(entropy_bytes, entropy_bytes_len, passphrase, passphrase_len, &context->encrypted_entropy);

  if (result != CARDANO_SUCCESS)
  {
    cardano_secure_key_handler_deallocate(context);

    return result;
  }

  cardano_bip32_private_key_t* root_private_key = NULL;
  cardano_bip32_public_key_t*  root_public_key  = NULL;

  result = cardano_bip32_private_key_from_bip39_entropy(NULL, 0, entropy_bytes, entropy_bytes_len, &root_private_key);

  if (result != CARDANO_SUCCESS)
  {
    cardano_secure_key_handler_deallocate(context);

    return result;
  }

  result = cardano_bip32_private_key_get_public_key(root_private_key, &root_public_key);

  // This performs a safe cleanup of the key material from memory
  cardano_bip32_private_key_unref(&root_private_key);

  if (result != CARDANO_SUCCESS)
  {
    cardano_secure_key_handler_deallocate(context);

    return result;
  }

  impl.bip32_get_extended_public_key = NULL;
  impl.bip32_get_public_key          = NULL;
  impl.bip32_sign_transaction        = NULL;
  impl.ed25519_get_public_key        = NULL;
  impl.ed25519_sign_transaction      = NULL;

  context->root_public_key = root_public_key;
  context->get_passphrase  = get_passphrase;

  impl.context = (cardano_object_t*)context;

  return cardano_secure_key_handler_new(impl, secure_key_handler);
}

/*
cardano_error_t
cardano_software_secure_key_handler_deserialize(
  const byte_t*                  serialized_data,
  const size_t                   serialized_data_len,
  cardano_get_passphrase_func_t  get_passphrase,
  cardano_secure_key_handler_t** secure_key_handler)
{
}*/
/*
// Function to serialize data into a buffer
int
serialize_secure_key_handler_data(
  const unsigned char* entropy,
  size_t               entropy_len,
  const unsigned char* passphrase,
  size_t               passphrase_len,
  const unsigned char* root_public_key,
  size_t               root_public_key_len,
  cardano_buffer_t**   buffer)
{
  // Ensure libsodium is initialized
  if (sodium_init() < 0)
  {
    // Initialization failed
    return -1;
  }

  int ret = -1; // Return value, default to error

  // Combine entropy and root public key into plaintext
  size_t         plaintext_len = entropy_len + root_public_key_len + sizeof(uint32_t);
  unsigned char* plaintext     = malloc(plaintext_len);

  if (!plaintext)
  {
    return -1;
  }

  // Store entropy length (big-endian) at the beginning of plaintext
  uint32_t entropy_len_be = htonl((uint32_t)entropy_len);
  memcpy(plaintext, &entropy_len_be, sizeof(uint32_t));

  // Copy entropy
  memcpy(plaintext + sizeof(uint32_t), entropy, entropy_len);

  // Copy root public key
  memcpy(plaintext + sizeof(uint32_t) + entropy_len, root_public_key, root_public_key_len);

  // Generate salt
  unsigned char salt[crypto_pwhash_SALTBYTES] = { 0 };
  randombytes_buf(salt, sizeof salt);

  // Derive key from passphrase
  unsigned char key[crypto_secretbox_KEYBYTES];
  if (crypto_pwhash(key, sizeof key, (const char*)passphrase, passphrase_len, salt, crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE, crypto_pwhash_ALG_DEFAULT) != 0)
  {
    // Key derivation failed
    goto cleanup;
  }

  // Generate nonce
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  randombytes_buf(nonce, sizeof nonce);

  // Allocate memory for ciphertext
  size_t         ciphertext_len = plaintext_len + crypto_secretbox_MACBYTES;
  unsigned char* ciphertext     = malloc(ciphertext_len);
  if (!ciphertext)
  {
    // Allocation failed
    sodium_memzero(key, sizeof key);
    goto cleanup;
  }

  // Encrypt plaintext
  if (crypto_secretbox_easy(ciphertext, plaintext, plaintext_len, nonce, key) != 0)
  {
    // Encryption failed
    sodium_memzero(key, sizeof key);
    goto cleanup_ciphertext;
  }

  // Securely wipe the key
  sodium_memzero(key, sizeof key);

  // Calculate total buffer length
  size_t buffer_len = 4 + 1 + 1 + sizeof(salt) + sizeof(nonce) + 4 + ciphertext_len;

  // Allocate buffer
  *buffer = malloc(*buffer_len);
  if (!*buffer)
  {
    // Allocation failed
    goto cleanup_ciphertext;
  }

  unsigned char* ptr = *buffer;

  // Write Magic Number
  uint32_t magic_be = htonl(MAGIC_NUMBER);
  memcpy(ptr, &magic_be, sizeof magic_be);
  ptr += sizeof magic_be;

  // Write Version
  *ptr++ = VERSION;

  // Write Algorithm ID
  *ptr++ = ALGORITHM_ID_SECRETBOX;

  // Write Salt
  memcpy(ptr, salt, sizeof salt);
  ptr += sizeof salt;

  // Write Nonce
  memcpy(ptr, nonce, sizeof nonce);
  ptr += sizeof nonce;

  // Write Ciphertext Length (big-endian)
  uint32_t ciphertext_len_be = htonl((uint32_t)ciphertext_len);
  memcpy(ptr, &ciphertext_len_be, sizeof ciphertext_len_be);
  ptr += sizeof ciphertext_len_be;

  // Write Ciphertext
  memcpy(ptr, ciphertext, ciphertext_len);

  // Success
  ret = 0;

cleanup_ciphertext:
  sodium_memzero(ciphertext, ciphertext_len);
  free(ciphertext);

cleanup:
  sodium_memzero(plaintext, plaintext_len);
  free(plaintext);

  return ret;
}

// Function to deserialize data from a buffer
int
deserialize_secure_key_handler_data(
  const unsigned char* buffer,
  size_t               buffer_len,
  const unsigned char* passphrase,
  size_t               passphrase_len,
  unsigned char**      entropy,
  size_t*              entropy_len,
  unsigned char**      root_public_key,
  size_t*              root_public_key_len)
{
  // Ensure libsodium is initialized
  if (sodium_init() < 0)
  {
    // Initialization failed
    return -1;
  }

  int ret = -1; // Return value, default to error

  const unsigned char* ptr       = buffer;
  size_t               remaining = buffer_len;

  // Check buffer length
  if (remaining < 4 + 1 + 1 + crypto_pwhash_SALTBYTES + crypto_secretbox_NONCEBYTES + 4)
  {
    // Buffer too small
    return -1;
  }

  // Read Magic Number
  uint32_t magic_be;
  memcpy(&magic_be, ptr, sizeof magic_be);
  ptr            += sizeof magic_be;
  remaining      -= sizeof magic_be;
  uint32_t magic = ntohl(magic_be);
  if (magic != MAGIC_NUMBER)
  {
    // Invalid magic number
    return -1;
  }

  // Read Version
  uint8_t version = *ptr++;
  remaining       -= 1;
  if (version != VERSION)
  {
    // Unsupported version
    return -1;
  }

  // Read Algorithm ID
  uint8_t algorithm_id = *ptr++;
  remaining            -= 1;
  if (algorithm_id != ALGORITHM_ID_SECRETBOX)
  {
    // Unsupported algorithm
    return -1;
  }

  // Read Salt
  unsigned char salt[crypto_pwhash_SALTBYTES];
  memcpy(salt, ptr, sizeof salt);
  ptr       += sizeof salt;
  remaining -= sizeof salt;

  // Read Nonce
  unsigned char nonce[crypto_secretbox_NONCEBYTES];
  memcpy(nonce, ptr, sizeof nonce);
  ptr       += sizeof nonce;
  remaining -= sizeof nonce;

  // Read Ciphertext Length
  uint32_t ciphertext_len_be;
  memcpy(&ciphertext_len_be, ptr, sizeof ciphertext_len_be);
  ptr                     += sizeof ciphertext_len_be;
  remaining               -= sizeof ciphertext_len_be;
  uint32_t ciphertext_len = ntohl(ciphertext_len_be);

  // Check if remaining buffer size matches ciphertext length
  if (remaining != ciphertext_len)
  {
    // Mismatch in ciphertext length
    return -1;
  }

  // Allocate memory for ciphertext
  unsigned char* ciphertext = malloc(ciphertext_len);
  if (!ciphertext)
  {
    // Allocation failed
    return -1;
  }
  memcpy(ciphertext, ptr, ciphertext_len);

  // Derive key from passphrase
  unsigned char key[crypto_secretbox_KEYBYTES];
  if (crypto_pwhash(key, sizeof key, (const char*)passphrase, passphrase_len, salt, crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE, crypto_pwhash_ALG_DEFAULT) != 0)
  {
    // Key derivation failed
    free(ciphertext);
    return -1;
  }

  // Allocate memory for decrypted data
  size_t         decrypted_len = ciphertext_len - crypto_secretbox_MACBYTES;
  unsigned char* decrypted     = malloc(decrypted_len);
  if (!decrypted)
  {
    sodium_memzero(key, sizeof key);
    free(ciphertext);
    return -1;
  }

  // Decrypt data
  if (crypto_secretbox_open_easy(decrypted, ciphertext, ciphertext_len, nonce, key) != 0)
  {
    // Decryption failed
    sodium_memzero(key, sizeof key);
    free(ciphertext);
    free(decrypted);
    return -1;
  }

  // Securely wipe the key
  sodium_memzero(key, sizeof key);
  free(ciphertext);

  // Read entropy length (big-endian)
  if (decrypted_len < sizeof(uint32_t))
  {
    // Decrypted data too small
    free(decrypted);
    return -1;
  }
  uint32_t entropy_len_be;
  memcpy(&entropy_len_be, decrypted, sizeof(uint32_t));
  *entropy_len = ntohl(entropy_len_be);

  // Check if entropy length is valid
  if (*entropy_len > decrypted_len - sizeof(uint32_t))
  {
    // Invalid entropy length
    sodium_memzero(decrypted, decrypted_len);
    free(decrypted);
    return -1;
  }

  // Extract entropy
  *entropy = malloc(*entropy_len);
  if (!*entropy)
  {
    sodium_memzero(decrypted, decrypted_len);
    free(decrypted);
    return -1;
  }
  memcpy(*entropy, decrypted + sizeof(uint32_t), *entropy_len);

  // Extract root public key
  *root_public_key_len = decrypted_len - sizeof(uint32_t) - *entropy_len;
  *root_public_key     = malloc(*root_public_key_len);
  if (!*root_public_key)
  {
    sodium_memzero(decrypted, decrypted_len);
    free(decrypted);
    free(*entropy);
    return -1;
  }
  memcpy(*root_public_key, decrypted + sizeof(uint32_t) + *entropy_len, *root_public_key_len);

  // Securely wipe and free decrypted data
  sodium_memzero(decrypted, decrypted_len);
  free(decrypted);

  // Success
  ret = 0;

  return ret;
}*/