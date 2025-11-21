/**
 * \file cip8.h
 *
 * \author angel.castillo
 * \date   Nov 21, 2025
 *
 * Copyright 2025 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CIP8_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CIP8_H

/* INCLUDES ******************************************************************/

#include <cardano/address/address.h>
#include <cardano/buffer.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/crypto/ed25519_private_key.h>
#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Signs arbitrary data using CIP-8 / COSE and binds the signature to a Cardano address.
 *
 * This function creates a COSE_Sign1 and COSE_Key structure compatible with CIP-8 and the
 * CIP-30 `signData` API. The message is signed directly (no pre-hashing), with the CIP-8
 * "hashed" flag set to false and an empty external_aad.
 *
 * The protected headers include:
 *   - alg     : EdDSA (-8)
 *   - address : raw bytes of \p address
 *
 * The resulting \p cose_sign1_out and \p cose_key_out buffers contain CBOR-encoded COSE
 * structures.
 *
 * \param[in]  message         Pointer to the message bytes to sign.
 * \param[in]  message_size    Size of \p message in bytes.
 * \param[in]  address         Cardano address to bind the signature to. Must not be NULL. If you need to bind
 *                             the signature to a key hash instead, use \ref cardano_cip8_sign_ex.
 * \param[in]  signing_key     Ed25519 private key used to produce the signature. Must not be NULL.
 * \param[out] cose_sign1_out  On success, receives a newly allocated buffer containing the
 *                             CBOR-encoded COSE_Sign1 structure.
 * \param[out] cose_key_out    On success, receives a newly allocated buffer containing the
 *                             CBOR-encoded COSE_Key structure corresponding to \p signing_key.
 *
 * \returns \ref CARDANO_SUCCESS on success, or an error code on failure.
 *
 * \post On success, the caller owns \p cose_sign1_out and \p cose_key_out and must release them
 *       with cardano_buffer_unref().
 *
 * \code{.c}
 * const byte_t message[] = "Hello, Cardano!";
 * cardano_address_t* address = // previously constructed
 * cardano_ed25519_private_key_t* sk = // derived key;
 *
 * cardano_buffer_t* cose_sign1 = NULL;
 * cardano_buffer_t* cose_key   = NULL;
 *
 * cardano_error_t result = cardano_cip8_sign(
 *   message,
 *   sizeof(message) - 1,
 *   address,
 *   sk,
 *   &cose_sign1,
 *   &cose_key);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   // handle error...
 * }
 *
 * cardano_buffer_unref(&cose_sign1);
 * cardano_buffer_unref(&cose_key);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cip8_sign(
  const byte_t*                        message,
  size_t                               message_size,
  const cardano_address_t*             address,
  const cardano_ed25519_private_key_t* signing_key,
  cardano_buffer_t**                   cose_sign1_out,
  cardano_buffer_t**                   cose_key_out);

/**
 * \brief Signs arbitrary data using CIP-8 / COSE and binds the signature to a key hash.
 *
 * This function creates a COSE_Sign1 and COSE_Key structure compatible with CIP-8 and the
 * CIP-30 `signData` API, binding the signature to a key hash rather than a full Cardano
 * address. The message is signed directly (no pre-hashing), with the CIP-8 "hashed" flag
 * set to false and an empty external_aad.
 *
 * The protected headers include:
 *   - alg     : EdDSA (-8)
 *   - keyHash : raw bytes of \p key_hash
 *
 * The resulting \p cose_sign1_out and \p cose_key_out buffers contain CBOR-encoded COSE
 * structures.
 *
 * \param[in]  message         Pointer to the message bytes to sign.
 * \param[in]  message_size    Size of \p message in bytes.
 * \param[in]  key_hash        Key hash to bind the signature to (typically a Blake2b-224
 *                             hash of a public key). Must not be NULL.
 * \param[in]  signing_key     Ed25519 private key used to produce the signature. Must not be NULL.
 * \param[out] cose_sign1_out  On success, receives a newly allocated buffer containing the
 *                             CBOR-encoded COSE_Sign1 structure.
 * \param[out] cose_key_out    On success, receives a newly allocated buffer containing the
 *                             CBOR-encoded COSE_Key structure corresponding to \p signing_key.
 *
 * \returns \ref CARDANO_SUCCESS on success, or an error code on failure.
 *
 * \post On success, the caller owns \p cose_sign1_out and \p cose_key_out and must release them
 *       with cardano_buffer_unref().
 *
 * \code{.c}
 * const byte_t message[] = "Hello, dRep!";
 * cardano_blake2b_hash_t* key_hash = // previously computed key hash;
 * cardano_ed25519_private_key_t* sk = // derived key;
 *
 * cardano_buffer_t* cose_sign1 = NULL;
 * cardano_buffer_t* cose_key   = NULL;
 *
 * cardano_error_t result = cardano_cip8_sign_ex(
 *   message,
 *   sizeof(message) - 1,
 *   key_hash,
 *   sk,
 *   &cose_sign1,
 *   &cose_key);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   // handle error...
 * }
 *
 * cardano_buffer_unref(&cose_sign1);
 * cardano_buffer_unref(&cose_key);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cip8_sign_ex(
  const byte_t*                        message,
  size_t                               message_size,
  const cardano_blake2b_hash_t*        key_hash,
  const cardano_ed25519_private_key_t* signing_key,
  cardano_buffer_t**                   cose_sign1_out,
  cardano_buffer_t**                   cose_key_out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CIP8_H