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
#include <cardano/crypto/crc32.h>
#include <cardano/crypto/ed25519_private_key.h>
#include <cardano/crypto/ed25519_public_key.h>
#include <cardano/crypto/emip3.h>
#include <cardano/key_handlers/secure_key_handler.h>
#include <cardano/key_handlers/secure_key_handler_impl.h>
#include <cardano/key_handlers/secure_key_handler_type.h>
#include <cardano/key_handlers/software_secure_key_handler.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../string_safe.h"

#include <assert.h>
#include <sodium/utils.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const uint32_t SW_SECURE_KEY_BINARY_FORMAT_HANDLER_MAGIC = 0x0A0A0A0A;
static const uint8_t  SW_SECURE_KEY_BINARY_FORMAT_VERSION       = 0x01;

/* STRUCTURES ****************************************************************/

/**
 * \brief Context structure for the Software Secure Key Handler.
 *
 * The `software_secure_key_handler_context_t` structure represents the internal context of the software-based
 * secure key handler.
 *
 * This context manages the encrypted key data as well as the mechanism for retrieving the passphrase when
 * required to decrypt the key for operations like signing or key derivation.
 */
typedef struct software_secure_key_handler_context_t
{
    cardano_object_t              base;
    cardano_buffer_t*             encrypted_data;
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

  cardano_buffer_memzero(context->encrypted_data);
  cardano_buffer_unref(&context->encrypted_data);

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
  data->encrypted_data     = NULL;
  data->get_passphrase     = NULL;

  return data;
}

/**
 * \brief Serializes the secure key handler context into a buffer.
 *
 * The `serialize` function is responsible for serializing the internal state of the secure key handler,
 * including its encrypted key material and relevant configuration, into a buffer that can later be deserialized
 * to restore the state.
 *
 * This function allows secure key handlers to persist their state across different sessions, enabling
 * operations such as storing the handler's state on disk for later retrieval.
 *
 * The serialized data includes:
 * - The encrypted key material (`encrypted_data` field).
 * - Other internal state required to reconstruct the key handler (e.g., key handler type).
 *
 * \param[in]  secure_key_handler_impl A pointer to the secure key handler implementation to be serialized.
 * \param[out] serialized_data A pointer to the buffer where the serialized data will be stored. The caller is
 *                             responsible for managing the lifecycle of this buffer, including freeing it after use.
 *
 * \returns `cardano_error_t` indicating success or the type of error encountered during serialization.
 *
 * \note The caller is responsible for freeing the memory associated with `serialized_data` by calling `cardano_buffer_unref`
 *       when it is no longer needed.
 *
 * \see cardano_secure_key_handler_deserialize for deserialization of the serialized data.
 */
static cardano_error_t
serialize(
  cardano_secure_key_handler_impl_t* secure_key_handler_impl,
  cardano_buffer_t**                 serialized_data)
{
  if ((secure_key_handler_impl == NULL) || (serialized_data == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  software_secure_key_handler_context_t* context = (software_secure_key_handler_context_t*)((void*)secure_key_handler_impl->context);

  if ((context == NULL) || (context->encrypted_data == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  const size_t encrypted_data_size = cardano_buffer_get_size(context->encrypted_data);

  const size_t total_size = (size_t)4UL // Magic
    + (size_t)1UL                       // Version
    + (size_t)1UL                       // Type
    + (size_t)4UL                       // Encrypted data size
    + encrypted_data_size               // Encrypted data
    + (size_t)4UL;                      // Checksum

  *serialized_data = cardano_buffer_new(total_size);

  if (*serialized_data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_buffer_t* buffer = *serialized_data;
  cardano_error_t   result = CARDANO_SUCCESS;

  result = cardano_buffer_write_uint32_be(buffer, SW_SECURE_KEY_BINARY_FORMAT_HANDLER_MAGIC);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(serialized_data);

    return result;
  }

  result = cardano_buffer_write(buffer, &SW_SECURE_KEY_BINARY_FORMAT_VERSION, 1);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(serialized_data);

    return result;
  }

  result = cardano_buffer_write(buffer, (const byte_t*)&secure_key_handler_impl->type, 1);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(serialized_data);

    return result;
  }

  result = cardano_buffer_write_uint32_be(buffer, (uint32_t)encrypted_data_size);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(serialized_data);

    return result;
  }

  result = cardano_buffer_write(buffer, cardano_buffer_get_data(context->encrypted_data), encrypted_data_size);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(serialized_data);

    return result;
  }

  const uint32_t crc32_checksum = cardano_checksum_crc32(cardano_buffer_get_data(buffer), cardano_buffer_get_size(buffer));

  result = cardano_buffer_write_uint32_be(buffer, crc32_checksum);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(serialized_data);

    return result;
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Retrieves the extended BIP32 public key for a given account derivation path.
 *
 * The `bip32_get_extended_account_public_key` function retrieves the extended BIP32 public key corresponding
 * to a specific account derivation path from the secure key handler. This public key can be used for operations
 * such as address generation or public key authentication.
 *
 * The BIP32 public key includes both the public key and the chain code, enabling derivation of child keys without
 * access to the private key. This allows for secure public key operations while keeping private keys secure.
 *
 * \param[in]  secure_key_handler_impl A pointer to the secure key handler implementation that manages the cryptographic operations.
 * \param[in]  derivation_path The account derivation path used to derive the corresponding public key.
 * \param[out] bip32_public_key A pointer to the BIP32 extended public key structure. This will be populated with the derived
 *                              public key and chain code. The caller is responsible for managing the lifecycle of this
 *                              public key by calling `cardano_bip32_public_key_unref` when it is no longer needed.
 *
 * \returns `cardano_error_t` indicating success or the type of error encountered during the public key derivation process.
 *
 * \note The caller must ensure proper memory management by unreferencing the `bip32_public_key` when it is no longer needed.
 *
 * \see cardano_bip32_public_key_unref for proper memory cleanup of the public key.
 */
static cardano_error_t
bip32_get_extended_account_public_key(
  cardano_secure_key_handler_impl_t*      secure_key_handler_impl,
  const cardano_account_derivation_path_t derivation_path,
  cardano_bip32_public_key_t**            bip32_public_key)
{
  if ((secure_key_handler_impl == NULL) || (bip32_public_key == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  software_secure_key_handler_context_t* context = (software_secure_key_handler_context_t*)((void*)secure_key_handler_impl->context);

  byte_t passphrase[128] = { 0 };

  int32_t pass_len = context->get_passphrase(&passphrase[0], sizeof(passphrase));

  if ((pass_len <= 0) || (pass_len > 128))
  {
    sodium_memzero(passphrase, sizeof(passphrase));

    return CARDANO_ERROR_INVALID_PASSPHRASE;
  }

  cardano_buffer_t* decrypted_data = NULL;

  cardano_error_t result = cardano_crypto_emip3_decrypt(
    cardano_buffer_get_data(context->encrypted_data),
    cardano_buffer_get_size(context->encrypted_data),
    passphrase,
    (size_t)pass_len,
    &decrypted_data);

  sodium_memzero(&passphrase[0], sizeof(passphrase));

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_bip32_private_key_t* root_private_key    = NULL;
  cardano_bip32_private_key_t* account_private_key = NULL;

  result = cardano_bip32_private_key_from_bip39_entropy(NULL, 0, cardano_buffer_get_data(decrypted_data), cardano_buffer_get_size(decrypted_data), &root_private_key);

  cardano_buffer_memzero(decrypted_data);
  cardano_buffer_unref(&decrypted_data);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  const uint32_t path[3] = {
    cardano_bip32_harden((uint32_t)derivation_path.purpose),
    cardano_bip32_harden((uint32_t)derivation_path.coin_type),
    cardano_bip32_harden((uint32_t)derivation_path.account)
  };

  result = cardano_bip32_private_key_derive(root_private_key, path, sizeof(path) / sizeof(uint32_t), &account_private_key);

  cardano_bip32_private_key_unref(&root_private_key);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_bip32_public_key_t* account_public_key = NULL;

  result = cardano_bip32_private_key_get_public_key(account_private_key, &account_public_key);

  cardano_bip32_private_key_unref(&account_private_key);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  *bip32_public_key = account_public_key;

  return CARDANO_SUCCESS;
}

/**
 * \brief Signs a transaction using BIP32 Hierarchical Deterministic (HD) keys.
 *
 * The `bip32_sign_transaction` function is responsible for signing a transaction by deriving the appropriate
 * BIP32 private keys using the provided derivation paths. It generates a verification key witness set that contains
 * the necessary signatures for the transaction.
 *
 * This function uses the secure key handler to access and manage the cryptographic operations necessary for deriving
 * private keys and signing the transaction.
 *
 * \param[in]  secure_key_handler_impl A pointer to the secure key handler implementation that securely manages cryptographic operations.
 * \param[in]  tx The transaction object to be signed.
 * \param[in]  derivation_paths An array of derivation paths specifying the private keys used to sign the transaction.
 * \param[in]  num_paths The number of derivation paths provided in the `derivation_paths` array.
 * \param[out] vkey_witness_set A pointer to the verification key witness set, which will be populated with the
 *                              signatures generated during the signing process. The caller is responsible for managing
 *                              the lifecycle of the witness set by calling `cardano_vkey_witness_set_unref` when it is no longer needed.
 *
 * \returns `cardano_error_t` indicating success or the type of error encountered during the signing process.
 *
 * \note The function assumes that the necessary private keys are securely stored and managed by the key handler.
 *       The caller is responsible for ensuring that the `vkey_witness_set` is unreferenced properly to prevent memory leaks.
 *
 * \see cardano_vkey_witness_set_unref for proper memory cleanup of the witness set.
 */
static cardano_error_t
bip32_sign_transaction(
  cardano_secure_key_handler_impl_t* secure_key_handler_impl,
  cardano_transaction_t*             tx,
  const cardano_derivation_path_t*   derivation_paths,
  const size_t                       num_paths,
  cardano_vkey_witness_set_t**       vkey_witness_set)
{
  if ((secure_key_handler_impl == NULL) || (tx == NULL) || (derivation_paths == NULL) || (vkey_witness_set == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  software_secure_key_handler_context_t* context = (software_secure_key_handler_context_t*)((void*)secure_key_handler_impl->context);

  if (context == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_buffer_t* decrypted_data = NULL;

  byte_t passphrase[128] = { 0 };

  int32_t pass_len = context->get_passphrase(&passphrase[0], sizeof(passphrase));

  if ((pass_len <= 0) || (pass_len > 128))
  {
    sodium_memzero(passphrase, sizeof(passphrase));

    return CARDANO_ERROR_INVALID_PASSPHRASE;
  }

  cardano_error_t result = cardano_crypto_emip3_decrypt(
    cardano_buffer_get_data(context->encrypted_data),
    cardano_buffer_get_size(context->encrypted_data),
    passphrase,
    (size_t)pass_len,
    &decrypted_data);

  sodium_memzero(passphrase, sizeof(passphrase));

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_memzero(decrypted_data);
    cardano_buffer_unref(&decrypted_data);

    return result;
  }

  cardano_bip32_private_key_t* root_private_key = NULL;

  result = cardano_bip32_private_key_from_bip39_entropy(NULL, 0, cardano_buffer_get_data(decrypted_data), cardano_buffer_get_size(decrypted_data), &root_private_key);

  cardano_buffer_memzero(decrypted_data);
  cardano_buffer_unref(&decrypted_data);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_blake2b_hash_t* hash = cardano_transaction_get_id(tx);

  if (hash == NULL)
  {
    cardano_bip32_private_key_unref(&root_private_key);

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = cardano_vkey_witness_set_new(vkey_witness_set);

  if (result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&hash);
    cardano_bip32_private_key_unref(&root_private_key);

    return result;
  }

  for (size_t i = 0U; i < num_paths; ++i)
  {
    cardano_ed25519_private_key_t* ed25519_private_key = NULL;
    cardano_bip32_private_key_t*   bip32_private_key   = NULL;

    const uint32_t path[5] = {
      cardano_bip32_harden((uint32_t)derivation_paths[i].purpose),
      cardano_bip32_harden((uint32_t)derivation_paths[i].coin_type),
      cardano_bip32_harden((uint32_t)derivation_paths[i].account),
      (uint32_t)derivation_paths[i].role,
      (uint32_t)derivation_paths[i].index
    };

    result = cardano_bip32_private_key_derive(root_private_key, &path[0], 5, &bip32_private_key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_unref(&hash);
      cardano_vkey_witness_set_unref(vkey_witness_set);
      cardano_bip32_private_key_unref(&root_private_key);

      return result;
    }

    result = cardano_bip32_private_key_to_ed25519_key(bip32_private_key, &ed25519_private_key);

    cardano_bip32_private_key_unref(&bip32_private_key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_unref(&hash);
      cardano_vkey_witness_set_unref(vkey_witness_set);
      cardano_bip32_private_key_unref(&root_private_key);

      return result;
    }

    cardano_ed25519_signature_t*  signature  = NULL;
    cardano_ed25519_public_key_t* public_key = NULL;
    cardano_vkey_witness_t*       witness    = NULL;

    result = cardano_ed25519_private_key_sign(ed25519_private_key, cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash), &signature);

    if (result != CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_unref(&hash);
      cardano_vkey_witness_set_unref(vkey_witness_set);
      cardano_bip32_private_key_unref(&root_private_key);

      return result;
    }

    result = cardano_ed25519_private_key_get_public_key(ed25519_private_key, &public_key);

    cardano_ed25519_private_key_unref(&ed25519_private_key);

    if (result != CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_unref(&hash);
      cardano_vkey_witness_set_unref(vkey_witness_set);
      cardano_bip32_private_key_unref(&root_private_key);

      return result;
    }

    result = cardano_vkey_witness_new(public_key, signature, &witness);

    cardano_ed25519_public_key_unref(&public_key);
    cardano_ed25519_signature_unref(&signature);

    if (result != CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_unref(&hash);
      cardano_vkey_witness_unref(&witness);
      cardano_vkey_witness_set_unref(vkey_witness_set);
      cardano_bip32_private_key_unref(&root_private_key);

      return result;
    }

    result = cardano_vkey_witness_set_add(*vkey_witness_set, witness);

    if (result != CARDANO_SUCCESS)
    {
      cardano_blake2b_hash_unref(&hash);
      cardano_vkey_witness_set_unref(vkey_witness_set);
      cardano_bip32_private_key_unref(&root_private_key);

      return result;
    }

    cardano_vkey_witness_unref(&witness);
  }

  cardano_blake2b_hash_unref(&hash);
  cardano_bip32_private_key_unref(&root_private_key);

  return CARDANO_SUCCESS;
}

/**
 * \brief Retrieves the public key for the Ed25519 key pair.
 *
 * The `ed25519_get_public_key` function retrieves the Ed25519 public key associated with the private key
 * managed by the secure key handler implementation. The retrieved public key is intended for cryptographic
 * operations like transaction verification or signature validation.
 *
 * \param[in]  secure_key_handler_impl A pointer to the secure key handler implementation that securely manages
 *                                     the Ed25519 key operations.
 * \param[out] public_key A pointer to the Ed25519 public key that will be populated by the function.
 *                        The caller is responsible for managing the lifecycle of the public key by calling
 *                        `cardano_ed25519_public_key_unref` when it is no longer needed.
 *
 * \returns `cardano_error_t` indicating success or the type of error encountered during the process of retrieving the public key.
 *
 * \note The public key is derived from the private key managed by the secure key handler. This function ensures
 *       that the private key is securely accessed, and the public key is properly returned. The caller is responsible for
 *       unreferencing the public key to prevent memory leaks.
 *
 * \see cardano_ed25519_public_key_unref for proper memory cleanup of the public key.
 */
static cardano_error_t
ed25519_get_public_key(
  cardano_secure_key_handler_impl_t* secure_key_handler_impl,
  cardano_ed25519_public_key_t**     public_key)
{
  if ((secure_key_handler_impl == NULL) || (public_key == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  software_secure_key_handler_context_t* context = (software_secure_key_handler_context_t*)((void*)secure_key_handler_impl->context);

  if (context == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_buffer_t* decrypted_data = NULL;

  byte_t  passphrase[128] = { 0 };
  int32_t pass_len        = context->get_passphrase(&passphrase[0], sizeof(passphrase));

  if ((pass_len <= 0) || (pass_len > 128))
  {
    sodium_memzero(passphrase, sizeof(passphrase));

    return CARDANO_ERROR_INVALID_PASSPHRASE;
  }

  cardano_error_t result = cardano_crypto_emip3_decrypt(
    cardano_buffer_get_data(context->encrypted_data),
    cardano_buffer_get_size(context->encrypted_data),
    passphrase,
    (size_t)pass_len,
    &decrypted_data);

  sodium_memzero(passphrase, sizeof(passphrase));

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_memzero(decrypted_data);
    cardano_buffer_unref(&decrypted_data);

    return result;
  }

  cardano_ed25519_private_key_t* ed25519_private_key = NULL;

  if (cardano_buffer_get_size(decrypted_data) == 64U)
  {
    result = cardano_ed25519_private_key_from_extended_bytes(cardano_buffer_get_data(decrypted_data), cardano_buffer_get_size(decrypted_data), &ed25519_private_key);
  }
  else
  {
    result = cardano_ed25519_private_key_from_normal_bytes(cardano_buffer_get_data(decrypted_data), cardano_buffer_get_size(decrypted_data), &ed25519_private_key);
  }

  cardano_buffer_memzero(decrypted_data);
  cardano_buffer_unref(&decrypted_data);

  if (result != CARDANO_SUCCESS)
  {
    cardano_ed25519_private_key_unref(&ed25519_private_key);

    return result;
  }

  result = cardano_ed25519_private_key_get_public_key(ed25519_private_key, public_key);

  cardano_ed25519_private_key_unref(&ed25519_private_key);

  return result;
}

/**
 * \brief Signs a transaction using the Ed25519 private key.
 *
 * The `ed25519_sign_transaction` function signs the provided transaction using the Ed25519 private key
 * managed by the secure key handler implementation. The function generates the necessary verification
 * key witness set, which contains the public keys and signatures required for transaction validation.
 *
 * \param[in]  secure_key_handler_impl A pointer to the secure key handler implementation that securely manages
 *                                     the Ed25519 key operations.
 * \param[in]  tx                      The transaction object to be signed.
 * \param[out] vkey_witness_set        A pointer to the verification key witness set, which will be populated by the
 *                                     function with the signatures and associated public keys required for the transaction.
 *                                     The caller is responsible for managing the lifecycle of this witness set and releasing
 *                                     it when no longer needed.
 *
 * \returns `cardano_error_t` indicating success or the type of error encountered during the signing process.
 *
 * \note The Ed25519 private key is securely accessed to sign the transaction. The public key and signature generated
 *       are added to the `vkey_witness_set`. The caller must ensure that the `vkey_witness_set` is properly released
 *       after use to avoid memory leaks.
 *
 * \see cardano_vkey_witness_set_unref for proper memory cleanup of the witness set.
 */
static cardano_error_t
ed25519_sign_transaction(
  cardano_secure_key_handler_impl_t* secure_key_handler_impl,
  cardano_transaction_t*             tx,
  cardano_vkey_witness_set_t**       vkey_witness_set)
{
  if ((secure_key_handler_impl == NULL) || (tx == NULL) || (vkey_witness_set == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  software_secure_key_handler_context_t* context = (software_secure_key_handler_context_t*)((void*)secure_key_handler_impl->context);

  if (context == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_buffer_t* decrypted_data = NULL;

  byte_t passphrase[128] = { 0 };

  int32_t pass_len = context->get_passphrase(&passphrase[0], sizeof(passphrase));

  if ((pass_len <= 0) || (pass_len > 128))
  {
    sodium_memzero(passphrase, sizeof(passphrase));

    return CARDANO_ERROR_INVALID_PASSPHRASE;
  }

  cardano_error_t result = cardano_crypto_emip3_decrypt(
    cardano_buffer_get_data(context->encrypted_data),
    cardano_buffer_get_size(context->encrypted_data),
    passphrase,
    (size_t)pass_len,
    &decrypted_data);

  sodium_memzero(passphrase, sizeof(passphrase));

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_memzero(decrypted_data);
    cardano_buffer_unref(&decrypted_data);

    return result;
  }

  cardano_ed25519_private_key_t* private_key = NULL;
  cardano_ed25519_public_key_t*  public_key  = NULL;
  cardano_ed25519_signature_t*   signature   = NULL;
  cardano_vkey_witness_t*        witness     = NULL;

  if (cardano_buffer_get_size(decrypted_data) == 64U)
  {
    result = cardano_ed25519_private_key_from_extended_bytes(cardano_buffer_get_data(decrypted_data), cardano_buffer_get_size(decrypted_data), &private_key);
  }
  else
  {
    result = cardano_ed25519_private_key_from_normal_bytes(cardano_buffer_get_data(decrypted_data), cardano_buffer_get_size(decrypted_data), &private_key);
  }

  cardano_buffer_memzero(decrypted_data);
  cardano_buffer_unref(&decrypted_data);

  if (result != CARDANO_SUCCESS)
  {
    cardano_ed25519_private_key_unref(&private_key);

    return result;
  }

  cardano_blake2b_hash_t* hash = cardano_transaction_get_id(tx);

  if (hash == NULL)
  {
    cardano_ed25519_private_key_unref(&private_key);

    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  result = cardano_vkey_witness_set_new(vkey_witness_set);

  if (result != CARDANO_SUCCESS)
  {
    cardano_blake2b_hash_unref(&hash);
    cardano_ed25519_private_key_unref(&private_key);

    return result;
  }

  result = cardano_ed25519_private_key_sign(private_key, cardano_blake2b_hash_get_data(hash), cardano_blake2b_hash_get_bytes_size(hash), &signature);
  cardano_blake2b_hash_unref(&hash);

  if (result != CARDANO_SUCCESS)
  {
    cardano_ed25519_signature_unref(&signature);
    cardano_vkey_witness_set_unref(vkey_witness_set);
    cardano_ed25519_private_key_unref(&private_key);

    return result;
  }

  result = cardano_ed25519_private_key_get_public_key(private_key, &public_key);

  cardano_ed25519_private_key_unref(&private_key);

  if (result != CARDANO_SUCCESS)
  {
    cardano_ed25519_signature_unref(&signature);
    cardano_vkey_witness_set_unref(vkey_witness_set);

    return result;
  }

  result = cardano_vkey_witness_new(public_key, signature, &witness);

  cardano_ed25519_public_key_unref(&public_key);
  cardano_ed25519_signature_unref(&signature);

  if (result != CARDANO_SUCCESS)
  {
    cardano_vkey_witness_set_unref(vkey_witness_set);

    return result;
  }

  result = cardano_vkey_witness_set_add(*vkey_witness_set, witness);

  if (result != CARDANO_SUCCESS)
  {
    cardano_vkey_witness_unref(&witness);
    cardano_vkey_witness_set_unref(vkey_witness_set);

    return result;
  }

  cardano_vkey_witness_unref(&witness);

  return CARDANO_SUCCESS;
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
  static const char*                handler_name = "BIP32 Software Secure Key Handler";
  cardano_secure_key_handler_impl_t impl         = { 0 };

  cardano_safe_memcpy(impl.name, 256U, handler_name, cardano_safe_strlen(handler_name, 256U));

  if (entropy_bytes == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (entropy_bytes_len == 0U)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  if (passphrase == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (passphrase_len == 0U)
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

  cardano_error_t result = cardano_crypto_emip3_encrypt(entropy_bytes, entropy_bytes_len, passphrase, passphrase_len, &context->encrypted_data);

  if (result != CARDANO_SUCCESS)
  {
    cardano_secure_key_handler_deallocate(context);

    return result;
  }

  impl.bip32_get_extended_account_public_key = bip32_get_extended_account_public_key;
  impl.bip32_sign_transaction                = bip32_sign_transaction;
  impl.ed25519_get_public_key                = NULL;
  impl.ed25519_sign_transaction              = NULL;
  impl.serialize                             = serialize;
  impl.type                                  = CARDANO_SECURE_KEY_HANDLER_TYPE_BIP32;

  context->get_passphrase = get_passphrase;

  impl.context = (cardano_object_t*)((void*)context);

  return cardano_secure_key_handler_new(impl, secure_key_handler);
}

cardano_error_t
cardano_software_secure_key_handler_ed25519_new(
  cardano_ed25519_private_key_t* ed25519_private_key,
  const byte_t*                  passphrase,
  size_t                         passphrase_len,
  cardano_get_passphrase_func_t  get_passphrase,
  cardano_secure_key_handler_t** secure_key_handler)
{
  static const char*                handler_name = "Ed25519 Software Secure Key Handler";
  cardano_secure_key_handler_impl_t impl         = { 0 };

  cardano_safe_memcpy(impl.name, 256U, handler_name, cardano_safe_strlen(handler_name, 256U));

  if (ed25519_private_key == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (passphrase == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (passphrase_len == 0U)
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

  cardano_error_t result = cardano_crypto_emip3_encrypt(cardano_ed25519_private_key_get_data(ed25519_private_key), cardano_ed25519_private_key_get_bytes_size(ed25519_private_key), passphrase, passphrase_len, &context->encrypted_data);

  if (result != CARDANO_SUCCESS)
  {
    cardano_secure_key_handler_deallocate(context);

    return result;
  }

  impl.bip32_get_extended_account_public_key = NULL;
  impl.bip32_sign_transaction                = NULL;
  impl.ed25519_get_public_key                = ed25519_get_public_key;
  impl.ed25519_sign_transaction              = ed25519_sign_transaction;
  impl.serialize                             = serialize;
  impl.type                                  = CARDANO_SECURE_KEY_HANDLER_TYPE_ED25519;

  context->get_passphrase = get_passphrase;

  impl.context = (cardano_object_t*)((void*)context);

  return cardano_secure_key_handler_new(impl, secure_key_handler);
}

cardano_error_t
cardano_software_secure_key_handler_deserialize(
  const byte_t*                  serialized_data,
  const size_t                   serialized_data_len,
  cardano_get_passphrase_func_t  get_passphrase,
  cardano_secure_key_handler_t** secure_key_handler)
{
  if ((serialized_data == NULL) || (get_passphrase == NULL) || (secure_key_handler == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (serialized_data_len == 0U)
  {
    return CARDANO_ERROR_OUT_OF_BOUNDS_MEMORY_READ;
  }

  cardano_buffer_t* data = cardano_buffer_new(serialized_data_len);

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_error_t result = cardano_buffer_write(data, serialized_data, serialized_data_len);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&data);

    return result;
  }

  uint32_t                          magic          = 0U;
  uint8_t                           version        = 0U;
  cardano_secure_key_handler_type_t type           = CARDANO_SECURE_KEY_HANDLER_TYPE_ED25519;
  cardano_buffer_t*                 encrypted_data = NULL;
  uint32_t                          checksum       = 0U;

  result = cardano_buffer_read_uint32_be(data, &magic);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&data);

    return result;
  }

  if (magic != SW_SECURE_KEY_BINARY_FORMAT_HANDLER_MAGIC)
  {
    cardano_buffer_unref(&data);

    return CARDANO_ERROR_INVALID_MAGIC;
  }

  result = cardano_buffer_read(data, &version, 1U);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&data);

    return result;
  }

  if (version != SW_SECURE_KEY_BINARY_FORMAT_VERSION)
  {
    cardano_buffer_unref(&data);

    return CARDANO_ERROR_DECODING;
  }

  result = cardano_buffer_read(data, (byte_t*)&type, 1U);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&data);

    return result;
  }

  uint32_t encrypted_data_size = 0U;
  result                       = cardano_buffer_read_uint32_be(data, &encrypted_data_size);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&data);

    return result;
  }

  if (encrypted_data_size == 0U)
  {
    cardano_buffer_unref(&data);

    return CARDANO_ERROR_DECODING;
  }

  encrypted_data = cardano_buffer_new(encrypted_data_size);

  if (encrypted_data == NULL)
  {
    cardano_buffer_unref(&data);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result = cardano_buffer_set_size(encrypted_data, encrypted_data_size);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&encrypted_data);
    cardano_buffer_unref(&data);

    return result;
  }

  result = cardano_buffer_read(data, cardano_buffer_get_data(encrypted_data), encrypted_data_size);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&encrypted_data);
    cardano_buffer_unref(&data);

    return result;
  }

  result = cardano_buffer_read_uint32_be(data, &checksum);

  if (result != CARDANO_SUCCESS)
  {
    cardano_buffer_unref(&encrypted_data);
    cardano_buffer_unref(&data);

    return result;
  }

  const uint32_t expected_checksum = cardano_checksum_crc32(serialized_data, (serialized_data_len - 4U));

  if (checksum != expected_checksum)
  {
    cardano_buffer_unref(&encrypted_data);
    cardano_buffer_unref(&data);

    return CARDANO_ERROR_CHECKSUM_MISMATCH;
  }

  cardano_secure_key_handler_impl_t impl = { 0 };

  software_secure_key_handler_context_t* context = cardano_software_secure_key_handler_context_new();

  if (context == NULL)
  {
    cardano_buffer_unref(&encrypted_data);
    cardano_buffer_unref(&data);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  switch (type)
  {
    case CARDANO_SECURE_KEY_HANDLER_TYPE_ED25519:
    {
      cardano_safe_memcpy(impl.name, 256, "Ed25519 Software Secure Key Handler", 256);
      impl.bip32_get_extended_account_public_key = NULL;
      impl.bip32_sign_transaction                = NULL;
      impl.ed25519_get_public_key                = ed25519_get_public_key;
      impl.ed25519_sign_transaction              = ed25519_sign_transaction;
      impl.serialize                             = serialize;

      context->get_passphrase = get_passphrase;
      context->encrypted_data = encrypted_data;

      impl.type    = CARDANO_SECURE_KEY_HANDLER_TYPE_ED25519;
      impl.context = (cardano_object_t*)((void*)context);

      break;
    }
    case CARDANO_SECURE_KEY_HANDLER_TYPE_BIP32:
    {
      cardano_safe_memcpy(impl.name, 256, "BIP32 Software Secure Key Handler", 256);

      impl.bip32_get_extended_account_public_key = bip32_get_extended_account_public_key;
      impl.bip32_sign_transaction                = bip32_sign_transaction;
      impl.ed25519_get_public_key                = NULL;
      impl.ed25519_sign_transaction              = NULL;
      impl.serialize                             = serialize;

      context->get_passphrase = get_passphrase;
      context->encrypted_data = encrypted_data;

      impl.type    = CARDANO_SECURE_KEY_HANDLER_TYPE_BIP32;
      impl.context = (cardano_object_t*)((void*)context);

      break;
    }
    default:
    {
      cardano_secure_key_handler_deallocate(context);
      cardano_buffer_unref(&encrypted_data);
      cardano_buffer_unref(&data);

      return CARDANO_ERROR_DECODING;
    }
  }

  cardano_buffer_unref(&data);

  result = cardano_secure_key_handler_new(impl, secure_key_handler);

  if (result != CARDANO_SUCCESS)
  {
    cardano_secure_key_handler_deallocate(context);
  }

  return result;
}