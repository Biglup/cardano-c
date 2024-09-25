/**
 * \file bip32_key_derivation.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_BIP32_KEY_DERIVATION_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_BIP32_KEY_DERIVATION_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Determines if a given index represents a hardened derivation in hierarchical deterministic (HD) wallets.
 *
 * In the context of HD wallet key derivation paths, indices greater than or equal to 2^31 are considered hardened.
 * This function checks if the provided index falls into the hardened category according to the BIP32 specification.
 * Hardened indices are used to increase security by ensuring that child private keys cannot be derived from parent
 * public keys, thereby protecting against certain types of attacks if a child private key is compromised.
 *
 * \param index The 32-bit integer index to verify. This index is part of a derivation path for generating
 *              child keys in an HD wallet. The BIP32 specification dictates that indices in the range
 *              [0, 2^31 - 1] are non-hardened, while indices in the range [2^31, 2^32 - 1] are hardened.
 *
 * \returns Returns `true` if the index represents a hardened derivation (i.e., is greater than or equal to 2^31).
 *          Returns `false` otherwise, indicating a non-hardened index.
 */
bool _cardano_crypto_is_hardened_derivation(uint32_t index);

/**
 * \brief Derives a child private key using a hardened index from a given parent key in hierarchical
 * deterministic (HD) wallets.
 *
 * This function performs hardened derivation of a child private key from a parent private key
 * using the Ed25519 curve. Hardened key derivation ensures that even if a child private key is
 * compromised, attackers cannot derive parent or sibling keys, enhancing security. This method
 * is part of the HD wallet structure that allows for generating a tree of keys from a single seed.
 *
 * \param index The derivation index for generating the child key. For hardened derivation,
 *              the index must be in the range [2^31, 2^32 - 1]. This function assumes the index
 *              is already hardened and does not check or modify it.
 * \param scalar A pointer to the 32-byte Ed25519 scalar (private key) of the parent from which
 *               the child key will be derived.
 * \param iv A pointer to the 32-byte Ed25519 initialization vector (IV) associated with the parent
 *           key.
 * \param chain_code A pointer to the 32-byte chain code associated with the parent key. The chain
 *                  code is used to add entropy to the derivation process and differentiate keys
 *                  derived from the same parent scalar but different indices.
 * \param[out] z_mac A pointer to the output buffer where the derived scalar (z) MAC (Message Authentication Code)
 *                   will be stored. This buffer must be pre-allocated with at least 32 bytes of space.
 * \param[out] cc_mac A pointer to the output buffer where the derived chain code MAC will be stored.
 *                    This buffer must also be pre-allocated with at least 32 bytes of space.
 *
 * \return cardano_error_t Returns an error code indicating the status of the operation.
 */
cardano_error_t _cardano_crypto_derive_hardened(
  int32_t       index,
  const byte_t* scalar,
  const byte_t* iv,
  const byte_t* chain_code,
  byte_t*       z_mac,
  byte_t*       cc_mac);

/**
 * \brief Derives a child private key using a non-hardened (soft) index from a given parent key in
 * hierarchical deterministic (HD) wallets.
 *
 * This function facilitates the soft derivation of a child private key from a parent private key using the
 * Ed25519 curve. In contrast to hardened derivation, soft derivation allows the child public key to be
 * derived from the parent public key without needing access to the parent private key. This property is
 * useful for creating public key hierarchies where public keys can be generated independently of private keys,
 * though it offers less security against certain types of attacks compared to hardened derivation.
 *
 * \param index The derivation index for generating the child key. For soft derivation, the index must be in the
 *              range [0, 2^31 - 1].
 * \param scalar A pointer to the 32-byte Ed25519 SCALAR (private key) of the parent from which the child key will
 *               be derived.
 * \param chain_code A pointer to the 32-byte chain code associated with the parent key. The chain code is used
 *                  to introduce additional entropy into the derivation process and ensures that keys derived
 *                  from the same scalar but different indices are unique and cryptographically separate.
 * \param[out] z_mac A pointer to the output buffer where the derived scalar (z) MAC (Message Authentication Code)
 *                   for the child key will be stored. This buffer must be pre-allocated with at least 32 bytes of space.
 * \param[out] cc_mac A pointer to the output buffer where the derived chain code MAC for the child key will be stored.
 *                    Like the z_mac buffer, this buffer must also be pre-allocated with at least 32 bytes of space.
 *
 * \return cardano_error_t Returns an error code indicating the status of the operation.
 */
cardano_error_t _cardano_crypto_derive_soft(
  int32_t       index,
  const byte_t* scalar,
  const byte_t* chain_code,
  byte_t*       z_mac,
  byte_t*       cc_mac);

/**
 * \brief Computes the scalar multiplication of a truncated secret key by 8 and the curve's base point.
 *
 * This function performs a specific scalar multiplication operation where the first 28 bytes of a little-endian
 * encoded secret key (`sk`) are multiplied by 8, and then the result is used as the scalar for multiplication with
 * the curve's base point, `G`. The operation `(8 * sk[:28])*G` is common in certain cryptographic protocols and
 * optimizations, where `sk[:28]` represents the truncation of the secret key to its first 28 bytes. This is typically
 * done to fit within certain computational or protocol-specific constraints while ensuring the resulting point
 * remains on the curve.
 *
 * \param[in] sk A pointer to the secret key, represented as a 32-byte array in little-endian format. Only the first
 *               28 bytes of this array are used in the computation, with the remaining bytes being ignored.
 * \param[out] out A pointer to the output buffer where the result of the scalar multiplication will be stored.
 *                 The result is the x-coordinate of the point resulting from the scalar multiplication of
 *                 the truncated secret key by 8 with the curve's base point, encoded as a 32-byte array. The
 *                 caller must ensure this buffer is allocated with sufficient space (typically 32 bytes) to
 *                 hold the result.
 *
 * \return cardano_error_t Returns an error code indicating the status of the operation.
 */
cardano_error_t _cardano_crypto_point_of_trunc28_mul8(const byte_t* sk, byte_t* out);

/**
 * Derives a child private key from a given parent private key using a specified derivation index.
 *
 * This function supports both hard and soft derivation processes as defined in the hierarchical deterministic (HD)
 * wallet architecture (BIP32). The derivation type (hard or soft) is determined by the value of the provided index.
 *
 * \param[in] key A pointer to the 32-byte parent private key from which the child key will be derived.
 * \param[in] index The derivation index. If the index is less than 0x80000000, it specifies a soft derivation;
 *                    otherwise, it specifies a hard derivation.
 * \param[out] out A pointer to the output buffer where the derived 32-byte child private key will be stored.
 * \param[in] out_size The size of the output buffer.
 *
 * \section Security considerations
 * - Hard derivation is secure under the assumption that the parent private key remains confidential. It prevents
 *   the derivation of sibling keys from the knowledge of a child key. However, hard derivation does not allow for
 *   the public key of the derived child key to be computed without access to the parent private key.
 * - Soft derivation allows for the associated public key of the derived child key to be computed directly from
 *   the parent public key, enhancing usability in scenarios where public key distribution is necessary. However,
 *   it introduces potential security risks if a child private key is compromised, potentially compromising sibling
 *   keys.
 *
 * \section Hard derivation vs. Soft derivation
 * - Soft derivation (index < 0x80000000): Allows the associated public key to be derived directly, facilitating
 *   scenarios where the public key needs to be shared or published without exposing the private key. The derived
 *   private and public keys maintain a direct relationship, allowing for streamlined key management in hierarchical
 *   structures.
 * - Hard derivation (index >= 0x80000000): Enhances security by isolating the derivation path, preventing the derivation
 *   of sibling keys from the knowledge of a child key. This is recommended for scenarios where the utmost security
 *   is required, and public key derivation from the parent key is not necessary.
 *
 * \section Operational semantics
 * - Derivation of the private key should always succeed provided the inputs are valid and correctly formatted.
 * - Derivation of the associated public key may fail if attempted with a hard derivation index, as the public key
 *   cannot be derived without access to the corresponding private key information.
 *
 * \return cardano_error_t Returns an error code indicating the status of the operation.
 */
cardano_error_t _cardano_crypto_derive_private(const byte_t* key, int32_t index, byte_t* out, size_t out_size);

/**
 * Derives a public key from a given private key using a specified derivation index.
 *
 * This function supports the derivation of a child public key based on BIP32 specifications. It allows
 * for both hardened and non-hardened (soft) derivations.
 *
 * \param[in] key A pointer to the 32-byte private key from which the public key will be derived.
 * \param[in] index The derivation index. Indices in the range [0x80000000, 0xFFFFFFFF] specify a hardened
 *                  derivation, while indices in the range [0, 0x7FFFFFFF] specify a non-hardened (soft) derivation.
 * \param[out] out A pointer to the output buffer where the derived 32-byte key will be stored.
 * \param[in] out_size The size of the output buffer.
 *
 * \return cardano_error_t Returns an error code indicating the status of the operation.
 */
cardano_error_t _cardano_crypto_derive_public(const byte_t* key, int32_t index, byte_t* out, size_t out_size);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_BIP32_KEY_DERIVATION_H