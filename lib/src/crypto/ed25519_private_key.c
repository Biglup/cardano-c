/**
 * \file ed25519_private_key.c
 *
 * \author angel.castillo
 * \date   Mar 20, 2024
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

#include <cardano/crypto/ed25519_private_key.h>

#include <cardano/buffer.h>
#include <cardano/export.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../string_safe.h"

#include <assert.h>
#include <sodium.h>
#include <string.h>

/* COSTANTS ******************************************************************/

static const size_t SCALAR_SIZE = 32U;

/* ENUMS *********************************************************************/

/**
 * \brief Enumerates the types of Ed25519 private keys based on their size and structure.
 */
typedef enum
{
  /**
   * Represents a standard Ed25519 private key, which is 32 bytes in length.
   */
  CARDANO_ED25519_PRIVATE_KEY_SIZE_NORMAL = 32,

  /**
   * Represents an extended Ed25519 private key, which extends the standard format to 64 bytes in length.
   * This extended format is divided into two parts:
   *  - The first 32 bytes consist of an Ed25519 curve scalar, slightly modified (tweaked) for compatibility
   *    with the ED25519-BIP32 specification.
   *  - The second 32 bytes serve as an initialization vector (IV).
   */
  CARDANO_ED25519_PRIVATE_KEY_SIZE_EXTENDED = 64,
} cardano_ed25519_private_key_size_t;

/* STRUCTURES ****************************************************************/

/**
 * \brief Represents an Ed25519 private key within the Cardano ecosystem.
 *
 * This structure encapsulates the information necessary to represent and manage an Ed25519
 * private key. It includes both the raw key material and metadata indicating the key's type
 * (normal or extended).
 */
typedef struct cardano_ed25519_private_key_t
{
    cardano_object_t                   base;
    cardano_buffer_t*                  key_material;
    cardano_ed25519_private_key_size_t key_size;
} cardano_ed25519_private_key_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a Ed25519 private key object.
 *
 * This function is responsible for properly deallocating a Ed25519 private key object (`cardano_ed25519_private_key_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the Ed25519 private key object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_ed25519_private_key_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the Ed25519 private key
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_ed25519_private_key_deallocate(void* object)
{
  assert(object != NULL);

  cardano_ed25519_private_key_t* private_key = (cardano_ed25519_private_key_t*)object;

  if (private_key->key_material != NULL)
  {
    sodium_memzero(cardano_buffer_get_data(private_key->key_material), cardano_buffer_get_size(private_key->key_material));
    cardano_buffer_unref(&private_key->key_material);
  }

  _cardano_free(private_key);
}

/**
 * \brief Creates an Ed25519 private key object from a raw byte array.
 *
 * This function constructs an Ed25519 private key from a provided array of bytes.
 *
 * \param[in] key_bytes A pointer to the byte array containing the binary representation of the Ed25519 private key.
 * \param[in] key_length The length of the byte array pointed to by `key`. For Ed25519, this is typically 32 bytes.
 * \param[out] private_key A pointer to a pointer that will be updated to point to the newly created
 *            `cardano_ed25519_private_key_t` object. The caller is responsible for managing
 *            the lifecycle of this object, including its deallocation, to prevent memory leaks.
 * \param[in] key_size The size of the Ed25519 private key, which determines the structure of the key.
 *
 * \return cardano_error_t Returns `CARDANO_SUCCESS` if the private key was successfully created
 *         from the byte array. On failure, a non-zero error code is returned, indicating an issue
 *         with the input parameters or an internal error in the key creation process. If the function
 *         fails, the value pointed to by `key` is set to `NULL`.
 */
static cardano_error_t
private_key_from_bytes(
  const byte_t*                            data,
  const size_t                             data_length,
  cardano_ed25519_private_key_t**          private_key,
  const cardano_ed25519_private_key_size_t key_size)
{
  if (private_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data == NULL)
  {
    *private_key = NULL;

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (data_length != (size_t)key_size)
  {
    *private_key = NULL;

    return CARDANO_ERROR_INVALID_ED25519_PRIVATE_KEY_SIZE;
  }

  cardano_ed25519_private_key_t* ed25519_private_key = (cardano_ed25519_private_key_t*)_cardano_malloc(sizeof(cardano_ed25519_private_key_t));

  if (ed25519_private_key == NULL)
  {
    *private_key = NULL;

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  ed25519_private_key->base.ref_count     = 1;
  ed25519_private_key->base.deallocator   = cardano_ed25519_private_key_deallocate;
  ed25519_private_key->base.last_error[0] = '\0';
  ed25519_private_key->key_material       = cardano_buffer_new_from(data, data_length);
  ed25519_private_key->key_size           = key_size;

  if (ed25519_private_key->key_material == NULL)
  {
    *private_key = NULL;
    _cardano_free(ed25519_private_key);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *private_key = ed25519_private_key;

  return CARDANO_SUCCESS;
}

/**
 * \brief Generates an Ed25519 private key from a hexadecimal string.
 *
 * This function is designed to create an Ed25519 private key from a provided hexadecimal string,
 * which should represent the bytes of the private key.
 *
 * \param[in] hex A pointer to the null-terminated hexadecimal string representing the seed bytes.
 * \param[in] hex_length The length of the hexadecimal string, not including the null terminator.
 * \param[out] key A pointer to a pointer that will be updated to point to the newly created
 *            `cardano_ed25519_private_key_t` object. The caller is responsible for the lifecycle
 *            management of this object, including its secure deallocation, to prevent memory leaks.
 * \param[in] key_size The size of the Ed25519 private key, which determines the structure of the key.
 *
 * \return cardano_error_t Returns `CARDANO_SUCCESS` if the private key is successfully created
 *         from the hexadecimal string. On failure, returns a non-zero error code indicating
 *         an issue with the input parameters. If the function fails, the value pointed to by `key` is set to `NULL`.
 */
static cardano_error_t
private_key_from_hex(
  const char*                              hex,
  const size_t                             hex_length,
  cardano_ed25519_private_key_t**          private_key,
  const cardano_ed25519_private_key_size_t key_size)
{
  if (private_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hex == NULL)
  {
    *private_key = NULL;

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (hex_length != ((size_t)key_size * 2U))
  {
    *private_key = NULL;

    return CARDANO_ERROR_INVALID_ED25519_PRIVATE_KEY_SIZE;
  }

  cardano_ed25519_private_key_t* ed25519_private_key = (cardano_ed25519_private_key_t*)_cardano_malloc(sizeof(cardano_ed25519_private_key_t));

  if (ed25519_private_key == NULL)
  {
    *private_key = NULL;

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  ed25519_private_key->base.ref_count     = 1;
  ed25519_private_key->base.deallocator   = cardano_ed25519_private_key_deallocate;
  ed25519_private_key->base.last_error[0] = '\0';
  ed25519_private_key->key_material       = cardano_buffer_from_hex(hex, hex_length);
  ed25519_private_key->key_size           = key_size;

  if (ed25519_private_key->key_material == NULL)
  {
    *private_key = NULL;
    _cardano_free(ed25519_private_key);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  *private_key = ed25519_private_key;

  return CARDANO_SUCCESS;
}

/**
 * \brief Computes the public key from a "normal" Ed25519 private key.
 *
 * \param private_key A pointer to the cardano_ed25519_private_key_t object representing
 *                    the normal Ed25519 private key.
 * \param[out] public_key On success, this will point to a newly allocated
 *                        cardano_ed25519_public_key_t object representing the corresponding
 *                        public key. The caller is responsible for deallocating this object.
 * \return Returns CARDANO_SUCCESS on successful computation of the public key. On failure,
 *         returns a non-zero error code indicating the type of failure.
 */
static cardano_error_t
compute_normal_public_key(
  const cardano_ed25519_private_key_t* private_key,
  cardano_ed25519_public_key_t**       public_key)
{
  assert(private_key != NULL);
  assert(public_key != NULL);

  byte_t public_key_bytes[32U] = { 0 };
  byte_t secret_key_bytes[64U] = { 0 };

  if (crypto_sign_seed_keypair(public_key_bytes, secret_key_bytes, cardano_buffer_get_data(private_key->key_material)) == -1)
  {
    return CARDANO_ERROR_GENERIC;
  }

  sodium_memzero(secret_key_bytes, 64U);

  cardano_ed25519_public_key_t* ed25519_public_key = NULL;
  cardano_error_t               error              = cardano_ed25519_public_key_from_bytes(public_key_bytes, SCALAR_SIZE, &ed25519_public_key);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  *public_key = ed25519_public_key;

  return CARDANO_SUCCESS;
}

/**
 * \brief Computes the public key from an "extended" Ed25519 private key.
 *
 * \param private_key A pointer to the cardano_ed25519_private_key_t object representing
 *                    the extended Ed25519 private key.
 * \param[out] public_key On success, this will point to a newly allocated
 *                        cardano_ed25519_public_key_t object representing the corresponding
 *                        public key. The caller is responsible for deallocating this object.
 * \return Returns CARDANO_SUCCESS on successful computation of the public key. On failure,
 *         returns CARDANO_ERROR_GENERIC or another non-zero error code indicating the type
 *         of failure.
 */
static cardano_error_t
compute_extended_public_key(
  const cardano_ed25519_private_key_t* private_key,
  cardano_ed25519_public_key_t**       public_key)
{
  assert(private_key != NULL);
  assert(public_key != NULL);

  byte_t* extended_scalar  = cardano_buffer_get_data(private_key->key_material);
  byte_t  scalar_data[32U] = { 0 };

  assert(extended_scalar != NULL);
  assert(cardano_buffer_get_size(private_key->key_material) >= SCALAR_SIZE);

  if (crypto_scalarmult_ed25519_base_noclamp(&scalar_data[0], extended_scalar) == -1)
  {
    return CARDANO_ERROR_GENERIC;
  }

  cardano_ed25519_public_key_t* ed25519_public_key = NULL;
  cardano_error_t               error              = cardano_ed25519_public_key_from_bytes(scalar_data, SCALAR_SIZE, &ed25519_public_key);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  *public_key = ed25519_public_key;

  return CARDANO_SUCCESS;
}

/**
 * \brief Determines the type of the private key (normal or extended) and computes the corresponding
 * public key.
 *
 * This function checks the size of the private key to determine whether it is a normal
 * or extended key and then calls the appropriate function to compute the public key.
 *
 * \param private_key A pointer to the cardano_ed25519_private_key_t object for which the public
 *                    key is to be computed. The key type is determined based on its size.
 * \param[out] public_key On success, this will point to a newly allocated
 *                        cardano_ed25519_public_key_t object representing the computed public
 *                        key. The caller is responsible for deallocating this object.
 * \return Returns CARDANO_SUCCESS if the public key is successfully computed. On failure,
 *         returns a non-zero error code indicating the type of failure.
 */
static cardano_error_t
compute_public_key(
  const cardano_ed25519_private_key_t* private_key,
  cardano_ed25519_public_key_t**       public_key)
{
  assert(private_key != NULL);
  assert(public_key != NULL);

  cardano_error_t error = CARDANO_SUCCESS;

  if (private_key->key_size == CARDANO_ED25519_PRIVATE_KEY_SIZE_NORMAL)
  {
    error = compute_normal_public_key(private_key, public_key);
  }
  else
  {
    error = compute_extended_public_key(private_key, public_key);
  }

  return error;
}

/**
 * Computes a digital signature using an extended Ed25519 private key.
 *
 * This function generates a digital signature for a given message using
 * the extended Ed25519 private key.
 *
 * @param[in] private_key A pointer to a `cardano_ed25519_private_key_t` structure representing
 *                        the extended Ed25519 private key used for signing.
 * @param[in] message A pointer to the byte array containing the message to be signed.
 * @param[in] message_length The length of the message in bytes.
 * @param[out] signature On successful execution, this parameter will be updated to point
 *                       to a newly allocated `cardano_ed25519_signature_t` structure
 *                       containing the signature of the message. The caller is responsible
 *                       for managing the lifecycle of this object, including deallocating
 *                       it when it's no longer needed.
 *
 * @return Returns `CARDANO_SUCCESS` if the signature was successfully generated. If an error
 *         occurs, a non-zero error code is returned, indicating the type of error encountered
 *         during the execution. Possible errors include invalid input parameters, failure in
 *         cryptographic operations, or memory allocation issues. If the function fails, the
 *         value pointed to by `signature` is set to `NULL`.
 */
static cardano_error_t
compute_signature_with_extended_key(
  const cardano_ed25519_private_key_t* private_key,
  const byte_t*                        message,
  const size_t                         message_length,
  cardano_ed25519_signature_t**        signature)
{
  static const size_t IV_SIZE = 32U;

  assert(private_key != NULL);
  assert(message != NULL);
  assert(signature != NULL);

  byte_t* extended_scalar = cardano_buffer_get_data(private_key->key_material);

  assert(extended_scalar != NULL);
  assert(cardano_buffer_get_size(private_key->key_material) == (SCALAR_SIZE * 2U));

  byte_t public_key[32U];
  byte_t hash_output[64U]     = { 0 };
  byte_t nonce[32U]           = { 0 };
  byte_t hram[64U]            = { 0 };
  byte_t hram_reduced[32U]    = { 0 };
  byte_t r[32U]               = { 0 };
  byte_t s[32U]               = { 0 };
  byte_t signature_bytes[64U] = { 0 };

  if (crypto_scalarmult_ed25519_base_noclamp(public_key, extended_scalar) != 0)
  {
    return CARDANO_ERROR_GENERIC;
  }

  size_t  digest_input_size = IV_SIZE + message_length;
  byte_t* digest_input      = (byte_t*)_cardano_malloc(digest_input_size);

  if (!digest_input)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_safe_memcpy(digest_input, digest_input_size, &extended_scalar[SCALAR_SIZE], IV_SIZE);
  cardano_safe_memcpy(&digest_input[IV_SIZE], digest_input_size - IV_SIZE, message, message_length);

  if (crypto_hash_sha512(hash_output, digest_input, digest_input_size) != 0)
  {
    return CARDANO_ERROR_GENERIC;
  }

  sodium_memzero(digest_input, digest_input_size);
  _cardano_free(digest_input);

  crypto_core_ed25519_scalar_reduce(nonce, hash_output);

  if (crypto_scalarmult_ed25519_base_noclamp(r, nonce) != 0)
  {
    return CARDANO_ERROR_GENERIC;
  }

  digest_input_size  = sizeof(r) + sizeof(public_key) + message_length;
  byte_t* hram_input = (byte_t*)_cardano_malloc(digest_input_size);

  if (!hram_input)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_safe_memcpy(hram_input, digest_input_size, r, sizeof(r));
  cardano_safe_memcpy(&hram_input[sizeof(r)], digest_input_size - sizeof(r), public_key, sizeof(public_key));
  cardano_safe_memcpy(
    &hram_input[sizeof(r) + sizeof(public_key)],
    digest_input_size - sizeof(r) - sizeof(public_key),
    message,
    message_length);

  if (crypto_hash_sha512(hram, hram_input, digest_input_size) != 0)
  {
    return CARDANO_ERROR_GENERIC;
  }

  sodium_memzero(hram_input, digest_input_size);
  _cardano_free(hram_input);

  crypto_core_ed25519_scalar_reduce(hram_reduced, hram);

  crypto_core_ed25519_scalar_mul(s, hram_reduced, extended_scalar);
  crypto_core_ed25519_scalar_add(s, s, nonce);

  cardano_safe_memcpy(signature_bytes, sizeof(signature_bytes), r, sizeof(r));
  cardano_safe_memcpy(&signature_bytes[sizeof(r)], sizeof(signature_bytes) - sizeof(r), s, sizeof(s));

  return cardano_ed25519_signature_from_bytes(signature_bytes, sizeof(signature_bytes), signature);
}

/**
 * Computes a digital signature using an normal Ed25519 private key.
 *
 * This function generates a digital signature for a given message using
 * the normal Ed25519 private key.
 *
 * @param[in] private_key A pointer to a `cardano_ed25519_private_key_t` structure representing
 *                        the normal Ed25519 private key used for signing.
 * @param[in] message A pointer to the byte array containing the message to be signed.
 * @param[in] message_length The length of the message in bytes.
 * @param[out] signature On successful execution, this parameter will be updated to point
 *                       to a newly allocated `cardano_ed25519_signature_t` structure
 *                       containing the signature of the message. The caller is responsible
 *                       for managing the lifecycle of this object, including deallocating
 *                       it when it's no longer needed.
 *
 * @return Returns `CARDANO_SUCCESS` if the signature was successfully generated. If an error
 *         occurs, a non-zero error code is returned, indicating the type of error encountered
 *         during the execution. Possible errors include invalid input parameters, failure in
 *         cryptographic operations, or memory allocation issues. If the function fails, the
 *         value pointed to by `signature` is set to `NULL`.
 */
static cardano_error_t
compute_signature_with_normal_key(
  const cardano_ed25519_private_key_t* private_key,
  const byte_t*                        message,
  const size_t                         message_length,
  cardano_ed25519_signature_t**        signature)
{
  assert(private_key != NULL);
  assert(signature != NULL);

  cardano_ed25519_public_key_t* public_key            = NULL;
  cardano_error_t               compute_pub_key_error = compute_normal_public_key(private_key, &public_key);

  if (compute_pub_key_error != CARDANO_SUCCESS)
  {
    return compute_pub_key_error;
  }

  const byte_t* private_key_bytes = cardano_buffer_get_data(private_key->key_material);

  assert(private_key_bytes != NULL);

  byte_t signature_bytes[64U] = { 0 };
  byte_t sk[64U]              = { 0 };

  cardano_safe_memcpy(sk, sizeof(sk), private_key_bytes, 32U);
  cardano_safe_memcpy(&sk[32U], sizeof(sk) - 32U, cardano_ed25519_public_key_get_data(public_key), 32U);

  cardano_ed25519_public_key_unref(&public_key);

  if (crypto_sign_detached(signature_bytes, NULL, message, message_length, sk) == -1)
  {
    return CARDANO_ERROR_GENERIC;
  }

  cardano_ed25519_signature_t* ed25519_signature = NULL;
  cardano_error_t              error             = cardano_ed25519_signature_from_bytes(signature_bytes, crypto_sign_BYTES, &ed25519_signature);

  if (error != CARDANO_SUCCESS)
  {
    return error;
  }

  *signature = ed25519_signature;

  return CARDANO_SUCCESS;
}

/**
 * Computes a digital signature for a given message using either a normal or an extended Ed25519 private key.
 *
 * This function abstracts the process of generating a digital signature by automatically determining
 * whether the provided private key is normal or extended, and then calling the appropriate internal
 * function to perform the signature computation.
 *
 * @param[in] private_key A pointer to a `cardano_ed25519_private_key_t` structure that represents
 *                        the Ed25519 private key (either normal or extended) to be used for signing.
 * @param[in] message A pointer to the byte array containing the message data to be signed.
 * @param[in] message_length The length of the message in bytes.
 * @param[out] signature Upon successful execution, this parameter will be updated to point to a newly
 *                       allocated `cardano_ed25519_signature_t` structure that contains the generated
 *                       signature.
 * @return Returns `CARDANO_SUCCESS` if the signature was successfully generated. If the function encounters
 *         an error, it returns a non-zero error code indicative of the type of error. Possible errors include
 *         issues with the input parameters, failure to correctly determine the key type, or errors during the
 *         signature computation process. If the function fails, the value pointed to by `signature` is set to `NULL`.
 */
static cardano_error_t
compute_signature(
  const cardano_ed25519_private_key_t* private_key,
  const byte_t*                        message,
  const size_t                         message_length,
  cardano_ed25519_signature_t**        signature)
{
  assert(private_key != NULL);
  assert(signature != NULL);

  cardano_error_t error = CARDANO_SUCCESS;

  if (private_key->key_size == CARDANO_ED25519_PRIVATE_KEY_SIZE_NORMAL)
  {
    error = compute_signature_with_normal_key(private_key, message, message_length, signature);
  }
  else
  {
    error = compute_signature_with_extended_key(private_key, message, message_length, signature);
  }

  return error;
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_ed25519_private_key_from_normal_bytes(
  const byte_t*                   data,
  const size_t                    data_length,
  cardano_ed25519_private_key_t** private_key)
{
  return private_key_from_bytes(data, data_length, private_key, CARDANO_ED25519_PRIVATE_KEY_SIZE_NORMAL);
}

cardano_error_t
cardano_ed25519_private_key_from_extended_bytes(
  const byte_t*                   data,
  const size_t                    data_length,
  cardano_ed25519_private_key_t** private_key)
{
  return private_key_from_bytes(data, data_length, private_key, CARDANO_ED25519_PRIVATE_KEY_SIZE_EXTENDED);
}

cardano_error_t
cardano_ed25519_private_key_from_normal_hex(
  const char*                     hex,
  const size_t                    hex_length,
  cardano_ed25519_private_key_t** private_key)
{
  return private_key_from_hex(hex, hex_length, private_key, CARDANO_ED25519_PRIVATE_KEY_SIZE_NORMAL);
}

cardano_error_t
cardano_ed25519_private_key_from_extended_hex(
  const char*                     hex,
  const size_t                    hex_length,
  cardano_ed25519_private_key_t** private_key)
{
  return private_key_from_hex(hex, hex_length, private_key, CARDANO_ED25519_PRIVATE_KEY_SIZE_EXTENDED);
}

void
cardano_ed25519_private_key_unref(cardano_ed25519_private_key_t** ed25519_private_key)
{
  if ((ed25519_private_key == NULL) || (*ed25519_private_key == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*ed25519_private_key)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *ed25519_private_key = NULL;
    return;
  }
}

void
cardano_ed25519_private_key_ref(cardano_ed25519_private_key_t* ed25519_private_key)
{
  if (ed25519_private_key == NULL)
  {
    return;
  }

  cardano_object_ref(&ed25519_private_key->base);
}

size_t
cardano_ed25519_private_key_refcount(const cardano_ed25519_private_key_t* ed25519_private_key)
{
  if (ed25519_private_key == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&ed25519_private_key->base);
}

cardano_error_t
cardano_ed25519_private_key_sign(
  const cardano_ed25519_private_key_t* private_key,
  const byte_t*                        message,
  const size_t                         message_length,
  cardano_ed25519_signature_t**        signature)
{
  if (private_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (signature == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (sodium_init() == -1)
  {
    return CARDANO_ERROR_GENERIC;
  }

  return compute_signature(private_key, message, message_length, signature);
}

cardano_error_t
cardano_ed25519_private_key_get_public_key(
  const cardano_ed25519_private_key_t* private_key,
  cardano_ed25519_public_key_t**       public_key)
{
  if (private_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (public_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (sodium_init() == -1)
  {
    return CARDANO_ERROR_GENERIC;
  }

  return compute_public_key(private_key, public_key);
}

const byte_t*
cardano_ed25519_private_key_get_data(const cardano_ed25519_private_key_t* ed25519_private_key)
{
  if (ed25519_private_key == NULL)
  {
    return NULL;
  }

  return cardano_buffer_get_data(ed25519_private_key->key_material);
}

size_t
cardano_ed25519_private_key_get_bytes_size(const cardano_ed25519_private_key_t* ed25519_private_key)
{
  if (ed25519_private_key == NULL)
  {
    return 0;
  }

  return cardano_buffer_get_size(ed25519_private_key->key_material);
}

cardano_error_t
cardano_ed25519_private_key_to_bytes(
  const cardano_ed25519_private_key_t* private_key,
  byte_t*                              out_key_bytes,
  const size_t                         out_key_length)
{
  if (private_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_buffer_copy_bytes(private_key->key_material, out_key_bytes, out_key_length);
}

size_t
cardano_ed25519_private_key_get_hex_size(const cardano_ed25519_private_key_t* private_key)
{
  if (private_key == NULL)
  {
    return 0;
  }

  return cardano_buffer_get_hex_size(private_key->key_material);
}

cardano_error_t
cardano_ed25519_private_key_to_hex(
  const cardano_ed25519_private_key_t* private_key,
  char*                                hex,
  const size_t                         hex_length)
{
  if (private_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return cardano_buffer_to_hex(private_key->key_material, hex, hex_length);
}
