/**
 * \file secure_key_handler_impl.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SECURE_KEY_HANDLER_IMPL_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SECURE_KEY_HANDLER_IMPL_H

/* INCLUDES ******************************************************************/

#include <cardano/crypto/bip32_public_key.h>
#include <cardano/crypto/ed25519_public_key.h>
#include <cardano/key_handlers/account_derivation_path.h>
#include <cardano/key_handlers/derivation_path.h>
#include <cardano/key_handlers/secure_key_handler_type.h>
#include <cardano/object.h>
#include <cardano/transaction/transaction.h>
#include <cardano/witness_set/vkey_witness_set.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* FORWARD DECLARATIONS *******************************************************/

/**
 * \brief Secure Key Handler Implementation for managing cryptographic key operations.
 *
 * The `cardano_secure_key_handler_impl_t` structure provides a collection of callbacks responsible for securely managing cryptographic key operations.
 *
 * Secure key handlers must ensure that:
 *
 * - Keys are securely stored, either on disk or in memory, in encrypted form.
 * - Keys are decrypted only when needed to perform operations (e.g., signing or deriving new keys),
 *   and immediately after, sensitive key material is wiped from memory.
 * - Memory management and cleanup are handled to reduce the risk of security vulnerabilities, sensitive data must not remain in memory after use.
 * - For hardware wallets, cryptographic operations are delegated to the hardware.
 *
 * The handler supports both cryptographic operations for BIP32 (HD) keys and raw Ed25519 keys (which do not support key derivation).
 *
 * \see cardano_secure_key_handler_t for the public interface.
 */
typedef struct cardano_secure_key_handler_impl_t cardano_secure_key_handler_impl_t;

/* CALLBACKS *****************************************************************/

/**
 * \brief Callback function type for signing a transaction using BIP32 Hierarchical Deterministic (HD) keys.
 *
 * The `cardano_bip32_sign_transaction_func_t` typedef defines the signature for a callback function responsible for
 * signing a transaction using BIP32 (HD) keys within the context of the secure key handler implementation.
 *
 * This function is expected to:
 * - Use the provided `secure_key_handler_impl` to securely handle the cryptographic key operations required for signing.
 * - Derive the appropriate private keys based on the provided `derivation_paths` and sign the `tx` (transaction).
 * - Generate a set of `vkey_witness_set` containing the necessary verification keys and signatures, representing
 *   the transaction witnesses.
 *
 * \param secure_key_handler_impl A pointer to the secure key handler implementation that manages cryptographic operations.
 * \param tx The transaction object to be signed.
 * \param derivation_paths An array of BIP32 derivation paths used to derive the private keys for signing the transaction.
 * \param num_paths The number of derivation paths provided in the `derivation_paths` array.
 * \param vkey_witness_set A pointer to the verification key witness set that will be populated with the generated signatures.
 *                         This set will contain the signatures and associated verification keys required for the transaction.
 *
 * \returns `cardano_error_t` indicating success or the type of error encountered during signing.
 *
 * \note The `vkey_witness_set` must be properly managed and released by the caller to avoid memory leaks.
 */
typedef cardano_error_t (*cardano_bip32_sign_transaction_func_t)(
  cardano_secure_key_handler_impl_t* secure_key_handler_impl,
  cardano_transaction_t*             tx,
  const cardano_derivation_path_t*   derivation_paths,
  size_t                             num_paths,
  cardano_vkey_witness_set_t**       vkey_witness_set);

/**
 * \brief Callback function type for retrieving a BIP32 extended account public key.
 *
 * The `cardano_bip32_get_extended_account_public_key_func_t` typedef defines the signature for a callback function that is
 * responsible for deriving and retrieving a BIP32 (Hierarchical Deterministic) extended account public key. This key includes both the
 * public key and the associated chain code, as required by BIP32.
 *
 * This function is expected to:
 * - Use the provided `secure_key_handler_impl` to securely handle the cryptographic key operations for deriving the extended
 *   account public key based on the provided `derivation_path`.
 * - Return the BIP32 extended account public key, which includes both the public key and the chain code.
 *
 * \param secure_key_handler_impl A pointer to the secure key handler implementation responsible for key management and operations.
 * \param account_derivation_path The BIP32 derivation path used to derive the extended account public key.
 * \param bip32_public_key A pointer to the location where the extended account public key (including chain code) will be stored.
 *                         The caller is responsible for managing the lifecycle of this object, ensuring proper cleanup after use.
 *
 * \returns `cardano_error_t` indicating success or providing details on any errors encountered during the process.
 *
 * \note The `bip32_public_key` object must be managed and released by the caller to avoid memory leaks.
 */
typedef cardano_error_t (*cardano_bip32_get_extended_account_public_key_func_t)(
  cardano_secure_key_handler_impl_t* secure_key_handler_impl,
  cardano_account_derivation_path_t  account_derivation_path,
  cardano_bip32_public_key_t**       bip32_public_key);

/**
 * \brief Callback function type for signing a transaction using an Ed25519 key.
 *
 * The `cardano_ed25519_sign_transaction_func_t` typedef defines the signature for a callback function that is responsible
 * for signing a given transaction using an Ed25519 private key. This function interacts with the secure key handler
 * implementation to ensure the private key is securely accessed and used for signing.
 *
 * This function is expected to:
 * - Use the provided `secure_key_handler_impl` to handle the Ed25519 key operations required to sign the transaction.
 * - Sign the given transaction (`tx`) and produce a `vkey_witness_set` containing the verification key and signature.
 *
 * \param secure_key_handler_impl A pointer to the secure key handler implementation responsible for key management and signing operations.
 * \param tx A pointer to the transaction that needs to be signed.
 * \param vkey_witness_set A pointer to the location where the signature and verification key set will be stored after signing.
 *                         The caller is responsible for managing the lifecycle of this object, ensuring proper cleanup after use.
 *
 * \returns `cardano_error_t` indicating success or providing details on any errors encountered during the process.
 *
 * \note The signing process is done securely, ensuring the private key is not exposed beyond the scope of the cryptographic operation.
 * The caller must manage and release the `vkey_witness_set` object to avoid memory leaks.
 */
typedef cardano_error_t (*cardano_ed25519_sign_transaction_func_t)(
  cardano_secure_key_handler_impl_t* secure_key_handler_impl,
  cardano_transaction_t*             tx,
  cardano_vkey_witness_set_t**       vkey_witness_set);

/**
 * \brief Callback function type for retrieving an Ed25519 public key.
 *
 * The `cardano_ed25519_get_public_key_func_t` typedef defines the signature for a callback function responsible for
 * securely retrieving the public key associated with an Ed25519 private key from the secure key handler implementation.
 *
 * This function is expected to:
 * - Use the provided `secure_key_handler_impl` to access and retrieve the associated Ed25519 public key.
 * - Store the public key in the location pointed to by the `public_key` parameter.
 *
 * \param secure_key_handler_impl A pointer to the secure key handler implementation responsible for managing cryptographic key operations.
 * \param public_key A pointer to the location where the Ed25519 public key will be stored.
 *                   The caller is responsible for managing the lifecycle of the returned public key, ensuring proper cleanup after use.
 *
 * \returns `cardano_error_t` indicating success or providing details on any errors encountered during the process.
 *
 * \note This operation ensures that only the public key is retrieved, with no exposure of the private key. The caller must
 * manage and release the `public_key` object appropriately to avoid memory leaks.
 */
typedef cardano_error_t (*cardano_ed25519_get_public_key_func_t)(
  cardano_secure_key_handler_impl_t* secure_key_handler_impl,
  cardano_ed25519_public_key_t**     public_key);

/**
 * \brief Callback function type for serializing a secure key handler.
 *
 * The `cardano_serialize_secure_key_handler_func_t` typedef defines the signature for a callback function responsible for
 * serializing the state associated with a secure key handler implementation into a buffer.
 *
 * This function is expected to:
 * - Use the provided `secure_key_handler_impl` to access the internal state or key material.
 * - Serialize the key handler data into a `cardano_buffer_t` that can be stored or transmitted.
 * - The serialized data must not contain any sensitive information, such as private keys, unless encrypted.
 *
 * \param secure_key_handler_impl A pointer to the secure key handler implementation that manages cryptographic operations.
 * \param serialized_data A pointer to a `cardano_buffer_t` that will be populated with the serialized data.
 *
 * \returns `cardano_error_t` indicating success or the type of error encountered during serialization.
 *
 * \note The `serialized_data` must be properly managed and released by the caller to avoid memory leaks.
 */
typedef cardano_error_t (*cardano_serialize_secure_key_handler_func_t)(
  cardano_secure_key_handler_impl_t* secure_key_handler_impl,
  cardano_buffer_t**                 serialized_data);

/* STRUCTURES ****************************************************************/

/**
 * \brief Secure Key Handler Implementation structure.
 *
 * The `cardano_secure_key_handler_impl_t` structure defines the interface for implementing secure key management
 * functionalities. This structure contains function pointers (callbacks) for cryptographic operations, providing a way
 * to securely manage and perform cryptographic actions using both BIP32 Hierarchical Deterministic (HD) keys and Ed25519 keys.
 *
 * Implementers are responsible for providing the specific functionality of these cryptographic operations through
 * the defined function pointers. These operations include signing transactions, retrieving public keys, and
 * performing key derivation.
 *
 * This structure does not directly implement any cryptographic logic but instead serves as a collection of callbacks
 * to be defined by the implementers based on their secure key handling strategy.
 *
 * It supports the following key operations:
 * - BIP32 key handling, including signing and public key retrieval with support for hierarchical derivation.
 * - Ed25519 key operations for signing and retrieving public keys, though Ed25519 keys do not support derivation.
 *
 * \note The secure key handler's responsibility is to ensure that sensitive key material is only decrypted for the
 * brief period required to perform the cryptographic operation, after which it is immediately wiped from memory.
 *
 * \see cardano_secure_key_handler_t for the public interface that interacts with this structure.
 */
typedef struct cardano_secure_key_handler_impl_t
{
    /**
     * \brief Name of the key handler implementation.
     */
    char name[256];

    /**
     * \brief Error message buffer for provider-specific error messages.
     */
    char error_message[1024];

    /**
     * \brief Opaque pointer to the implementation-specific context.
     *
     * This pointer holds the state or context required by the provider implementation.
     * Users should not access or modify this directly.
     */
    cardano_object_t* context;

    /**
     * \brief Type of secure key handler.
     *
     * The two types supported are:
     * - `CARDANO_SECURE_KEY_HANDLER_TYPE_ED25519`:
     *   This typ of handler manages Ed25519 keys, which do not support hierarchical key derivation (BIP32). Any operations on Ed25519
     *   keys must use the `secure_key_handler_ed25519_*` family of functions.
     *
     * - `CARDANO_SECURE_KEY_HANDLER_TYPE_BIP32`:
     *   This type of handler manages BIP32 keys, which support hierarchical deterministic (HD) key derivation, allowing the generation
     *   of multiple keys from a master key. Operations on BIP32 keys must use the `secure_key_handler_bip32_*` family of functions.
     *
     * \see cardano_secure_key_handler_type_t for the enumeration of secure key handler types.
     */
    cardano_secure_key_handler_type_t type;

    /**
     * \brief Callback function to sign a transaction using BIP32 keys.
     *
     * \note
     * This function is only applicable to key handlers of type `CARDANO_SECURE_KEY_HANDLER_TYPE_BIP32`. For other key types,
     * this field should not be used.
     *
     * \see cardano_bip32_sign_transaction_func_t for more details on the function signature.
     */
    cardano_bip32_sign_transaction_func_t bip32_sign_transaction;

    /**
     * \brief Callback function to retrieve a BIP32 extended account public key.
     *
     * \note
     * This function is only applicable to key handlers of type `CARDANO_SECURE_KEY_HANDLER_TYPE_BIP32`. For other key types,
     * this field should not be used.
     *
     * \see cardano_bip32_get_extended_account_public_key_func_t for more details on the function signature.
     */
    cardano_bip32_get_extended_account_public_key_func_t bip32_get_extended_account_public_key;

    /**
     * \brief Callback function to sign a transaction using an Ed25519 key.
     *
     * \note
     * This function is only applicable to key handlers of type `CARDANO_SECURE_KEY_HANDLER_TYPE_ED25519`. For other key types,
     * this field should not be used.
     *
     * \see cardano_ed25519_sign_transaction_func_t for more details on the function signature.
     */
    cardano_ed25519_sign_transaction_func_t ed25519_sign_transaction;

    /**
     * \brief Callback function to retrieve an Ed25519 public key.
     *
     * \note
     * This function is only applicable to key handlers of type `CARDANO_SECURE_KEY_HANDLER_TYPE_ED25519
     * For other key types, this field should not be used.
     *
     * \see cardano_ed25519_get_public_key_func_t for more details on the function signature.
     */
    cardano_ed25519_get_public_key_func_t ed25519_get_public_key;

    /**
     * \brief Callback function to serialize the secure key handler implementation.
     *
     * \see cardano_serialize_secure_key_handler_func_t for more details on the function signature.
     */
    cardano_serialize_secure_key_handler_func_t serialize;
} cardano_secure_key_handler_impl_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SECURE_KEY_HANDLER_IMPL_H